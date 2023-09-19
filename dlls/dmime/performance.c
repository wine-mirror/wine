/* IDirectMusicPerformance Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmime_private.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct pchannel_block {
    DWORD block_num;   /* Block 0 is PChannels 0-15, Block 1 is PChannels 16-31, etc */
    struct {
       DWORD channel;  /* MIDI channel */
       DWORD group;    /* MIDI group */
       IDirectMusicPort *port;
    } pchannel[16];
    struct wine_rb_entry entry;
};

struct performance
{
    IDirectMusicPerformance8 IDirectMusicPerformance8_iface;
    IDirectMusicGraph IDirectMusicGraph_iface;
    IDirectMusicTool IDirectMusicTool_iface;
    LONG ref;
    IDirectMusic *dmusic;
    IDirectSound *dsound;
    IDirectMusicGraph *pToolGraph;
    BOOL fAutoDownload;
    char cMasterGrooveLevel;
    float fMasterTempo;
    long lMasterVolume;
    /* performance channels */
    struct wine_rb_tree pchannels;

    BOOL audio_paths_enabled;
    IDirectMusicAudioPath *pDefaultPath;
    REFERENCE_TIME rtLatencyTime;
    DWORD dwBumperLength;
    DWORD dwPrepareTime;
    /** Message Processing */
    HANDLE procThread;
    DWORD procThreadId;
    BOOL procThreadTicStarted;
    CRITICAL_SECTION safe;

    IReferenceClock *master_clock;
    REFERENCE_TIME init_time;
    struct list messages;

    struct list notifications;
    REFERENCE_TIME notification_timeout;
    HANDLE notification_event;
    BOOL notification_performance;
    BOOL notification_segment;
};

struct message
{
    struct list entry;
    DMUS_PMSG msg;
};

static inline struct message *message_from_DMUS_PMSG(DMUS_PMSG *msg)
{
    return msg ? CONTAINING_RECORD(msg, struct message, msg) : NULL;
}

static HRESULT performance_process_message(struct performance *This, DMUS_PMSG *msg, DWORD *timeout)
{
    static const DWORD delivery_flags = DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME;
    IDirectMusicPerformance *performance = (IDirectMusicPerformance *)&This->IDirectMusicPerformance8_iface;
    HRESULT hr;

    do
    {
        REFERENCE_TIME current, offset = 0;
        IDirectMusicTool *tool;

        if (FAILED(hr = IDirectMusicPerformance_GetTime(performance, &current, NULL))) return hr;
        if (!(tool = msg->pTool)) tool = &This->IDirectMusicTool_iface;

        switch (msg->dwFlags & delivery_flags)
        {
        default:
            WARN("No delivery flag found for message %p\n", msg);
            /* fallthrough */
        case DMUS_PMSGF_TOOL_IMMEDIATE:
            hr = IDirectMusicTool_ProcessPMsg(tool, performance, msg);
            break;
        case DMUS_PMSGF_TOOL_QUEUE:
            offset = This->dwBumperLength * 10000;
            /* fallthrough */
        case DMUS_PMSGF_TOOL_ATTIME:
            if (msg->rtTime >= offset && msg->rtTime - offset >= current)
            {
                if (timeout) *timeout = (msg->rtTime - offset - current) / 10000;
                return DMUS_S_REQUEUE;
            }

            hr = IDirectMusicTool_ProcessPMsg(tool, performance, msg);
            break;
        }
    } while (hr == DMUS_S_REQUEUE);

    if (hr == DMUS_S_FREE) hr = IDirectMusicPerformance_FreePMsg(performance, msg);
    if (FAILED(hr)) WARN("Failed to process message, hr %#lx\n", hr);
    return hr;
}

#define PROCESSMSG_START           (WM_APP + 0)
#define PROCESSMSG_EXIT            (WM_APP + 1)
#define PROCESSMSG_REMOVE          (WM_APP + 2)
#define PROCESSMSG_ADD             (WM_APP + 4)

static DWORD WINAPI ProcessMsgThread(LPVOID lpParam)
{
    struct performance *This = lpParam;
    DWORD timeout = INFINITE;
    MSG msg;
    HRESULT hr;
    struct message *message, *next;

    while (TRUE)
    {
        if (timeout > 0) MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_POSTMESSAGE | QS_SENDMESSAGE | QS_TIMER);
        timeout = INFINITE;

        EnterCriticalSection(&This->safe);

        LIST_FOR_EACH_ENTRY_SAFE(message, next, &This->messages, struct message, entry)
        {
            list_remove(&message->entry);
            list_init(&message->entry);

            hr = performance_process_message(This, &message->msg, &timeout);
            if (hr == DMUS_S_REQUEUE) list_add_before(&next->entry, &message->entry);
            if (hr != S_OK) break;
        }

        LeaveCriticalSection(&This->safe);

        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /** if hwnd we suppose that is a windows event ... */
            if (NULL != msg.hwnd)
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            else
            {
                switch (msg.message)
                {
                case WM_QUIT:
                case PROCESSMSG_EXIT: goto outofthread;
                case PROCESSMSG_START: break;
                case PROCESSMSG_ADD: break;
                case PROCESSMSG_REMOVE: break;
                default: ERR("Unhandled message %u. Critical Path\n", msg.message); break;
                }
            }
        }

        /** here we should run a little of current AudioPath */
    }

outofthread:
    TRACE("(%p): Exiting\n", This);

    return 0;
}

static BOOL PostMessageToProcessMsgThread(struct performance *This, UINT iMsg) {
  if (FALSE == This->procThreadTicStarted && PROCESSMSG_EXIT != iMsg) {
    BOOL res;
    This->procThread = CreateThread(NULL, 0, ProcessMsgThread, This, 0, &This->procThreadId);
    if (NULL == This->procThread) return FALSE;
    SetThreadPriority(This->procThread, THREAD_PRIORITY_TIME_CRITICAL);
    This->procThreadTicStarted = TRUE;
    while(1) {
      res = PostThreadMessageA(This->procThreadId, iMsg, 0, 0);
      /* Let the thread creates its message queue (with MsgWaitForMultipleObjects call) by yielding and retrying */
      if (!res && (GetLastError() == ERROR_INVALID_THREAD_ID))
	Sleep(0);
      else
	break;
    }
    return res;
  }
  return PostThreadMessageA(This->procThreadId, iMsg, 0, 0);
}

static HRESULT performance_send_dirty_pmsg(struct performance *This, MUSIC_TIME music_time)
{
    IDirectMusicPerformance8 *performance = &This->IDirectMusicPerformance8_iface;
    IDirectMusicGraph *graph = &This->IDirectMusicGraph_iface;
    DMUS_PMSG *msg;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPerformance8_AllocPMsg(performance, sizeof(*msg), &msg)))
        return hr;

    msg->mtTime = music_time;
    msg->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_QUEUE;
    msg->dwType = DMUS_PMSGT_DIRTY;

    if (FAILED(hr = IDirectMusicGraph_StampPMsg(graph, msg))
            || FAILED(hr = IDirectMusicPerformance8_SendPMsg(performance, msg)))
        IDirectMusicPerformance8_FreePMsg(performance, msg);

    return hr;
}

static HRESULT performance_send_notification_pmsg(struct performance *This, MUSIC_TIME music_time, BOOL stamp,
        GUID type, DWORD option, IUnknown *object)
{
    IDirectMusicPerformance8 *performance = &This->IDirectMusicPerformance8_iface;
    IDirectMusicGraph *graph = &This->IDirectMusicGraph_iface;
    DMUS_NOTIFICATION_PMSG *msg;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPerformance8_AllocPMsg(performance, sizeof(*msg), (DMUS_PMSG **)&msg)))
        return hr;

    msg->mtTime = music_time;
    msg->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_QUEUE;
    msg->dwType = DMUS_PMSGT_NOTIFICATION;
    if ((msg->punkUser = object)) IUnknown_AddRef(object);
    msg->guidNotificationType = type;
    msg->dwNotificationOption = option;

    /* only stamp the message if notifications are enabled, otherwise send them directly to the output tool */
    if ((stamp && FAILED(hr = IDirectMusicGraph_StampPMsg(graph, (DMUS_PMSG *)msg)))
            || FAILED(hr = IDirectMusicPerformance8_SendPMsg(performance, (DMUS_PMSG *)msg)))
        IDirectMusicPerformance8_FreePMsg(performance, (DMUS_PMSG *)msg);

    return hr;
}

static int pchannel_block_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct pchannel_block *b = WINE_RB_ENTRY_VALUE(entry, const struct pchannel_block, entry);

    return *(DWORD *)key - b->block_num;
}

static void pchannel_block_free(struct wine_rb_entry *entry, void *context)
{
    struct pchannel_block *b = WINE_RB_ENTRY_VALUE(entry, struct pchannel_block, entry);

    free(b);
}

static struct pchannel_block *pchannel_block_set(struct wine_rb_tree *tree, DWORD block_num,
        IDirectMusicPort *port, DWORD group, BOOL only_set_new)
{
    struct pchannel_block *block;
    struct wine_rb_entry *entry;
    unsigned int i;

    entry = wine_rb_get(tree, &block_num);
    if (entry) {
        block = WINE_RB_ENTRY_VALUE(entry, struct pchannel_block, entry);
        if (only_set_new)
            return block;
    } else {
        if (!(block = malloc(sizeof(*block)))) return NULL;
        block->block_num = block_num;
    }

    for (i = 0; i < 16; ++i) {
        block->pchannel[i].port = port;
        block->pchannel[i].group = group;
        block->pchannel[i].channel = i;
    }
    if (!entry)
        wine_rb_put(tree, &block->block_num, &block->entry);

    return block;
}

static inline struct performance *impl_from_IDirectMusicPerformance8(IDirectMusicPerformance8 *iface)
{
    return CONTAINING_RECORD(iface, struct performance, IDirectMusicPerformance8_iface);
}

/* IDirectMusicPerformance8 IUnknown part: */
static HRESULT WINAPI performance_QueryInterface(IDirectMusicPerformance8 *iface, REFIID riid, void **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_dmguid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirectMusicPerformance)
            || IsEqualGUID(riid, &IID_IDirectMusicPerformance2)
            || IsEqualGUID(riid, &IID_IDirectMusicPerformance8))
    {
        *ret_iface = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectMusicGraph))
    {
        *ret_iface = &This->IDirectMusicGraph_iface;
        IDirectMusicGraph_AddRef(&This->IDirectMusicGraph_iface);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectMusicTool))
    {
        *ret_iface = &This->IDirectMusicTool_iface;
        IDirectMusicTool_AddRef(&This->IDirectMusicTool_iface);
        return S_OK;
    }

    *ret_iface = NULL;
    WARN("(%p, %s, %p): not found\n", iface, debugstr_dmguid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI performance_AddRef(IDirectMusicPerformance8 *iface)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p): ref=%ld\n", This, ref);

  return ref;
}

static ULONG WINAPI performance_Release(IDirectMusicPerformance8 *iface)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p): ref=%ld\n", This, ref);
  
  if (ref == 0) {
    wine_rb_destroy(&This->pchannels, pchannel_block_free, NULL);
    This->safe.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->safe);
    free(This);
  }

  return ref;
}

static HRESULT performance_init_dsound(struct performance *This, HWND hwnd)
{
    IDirectSound *dsound;
    HRESULT hr;

    if (FAILED(hr = DirectSoundCreate(NULL, &dsound, NULL))) return hr;

    if (!hwnd) hwnd = GetForegroundWindow();
    hr = IDirectSound_SetCooperativeLevel(dsound, hwnd, DSSCL_PRIORITY);

    if (SUCCEEDED(hr)) This->dsound = dsound;
    else IDirectSound_Release(dsound);

    return hr;
}

static HRESULT performance_init_dmusic(struct performance *This, IDirectSound *dsound)
{
    IDirectMusic *dmusic;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusic, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusic8, (void **)&dmusic)))
        return hr;

    hr = IDirectMusic_SetDirectSound(dmusic, dsound, NULL);

    if (SUCCEEDED(hr)) This->dmusic = dmusic;
    else IDirectSound_Release(dmusic);

    return hr;
}

static HRESULT WINAPI performance_Init(IDirectMusicPerformance8 *iface, IDirectMusic **dmusic,
        IDirectSound *dsound, HWND hwnd)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    HRESULT hr;

    TRACE("(%p, %p, %p, %p)\n", iface, dmusic, dsound, hwnd);

    if (This->dmusic) return DMUS_E_ALREADY_INITED;

    if ((This->dsound = dsound)) IDirectMusic8_AddRef(This->dsound);
    else if (FAILED(hr = performance_init_dsound(This, hwnd))) return hr;

    if (dmusic && (This->dmusic = *dmusic)) IDirectMusic_AddRef(This->dmusic);
    else if (FAILED(hr = performance_init_dmusic(This, This->dsound)))
    {
        IDirectMusicPerformance_CloseDown(iface);
        return hr;
    }

    if (FAILED(hr = IDirectMusic_GetMasterClock(This->dmusic, NULL, &This->master_clock))
            || FAILED(hr = IDirectMusicPerformance8_GetTime(iface, &This->init_time, NULL)))
    {
        IDirectMusicPerformance_CloseDown(iface);
        return hr;
    }

    PostMessageToProcessMsgThread(This, PROCESSMSG_START);

    if (dmusic && !*dmusic)
    {
        *dmusic = This->dmusic;
        IDirectMusic_AddRef(*dmusic);
    }
    return S_OK;
}

static HRESULT WINAPI performance_PlaySegment(IDirectMusicPerformance8 *iface, IDirectMusicSegment *segment,
        DWORD segment_flags, INT64 start_time, IDirectMusicSegmentState **ret_state)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    TRACE("(%p, %p, %ld, %I64d, %p)\n", This, segment, segment_flags, start_time, ret_state);

    return IDirectMusicPerformance8_PlaySegmentEx(iface, (IUnknown *)segment, NULL, NULL,
            segment_flags, start_time, ret_state, NULL, NULL);
}

static HRESULT WINAPI performance_Stop(IDirectMusicPerformance8 *iface, IDirectMusicSegment *pSegment,
        IDirectMusicSegmentState *pSegmentState, MUSIC_TIME mtTime, DWORD dwFlags)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %p, %p, %ld, %ld): stub\n", This, pSegment, pSegmentState, mtTime, dwFlags);
	return S_OK;
}

static HRESULT WINAPI performance_GetSegmentState(IDirectMusicPerformance8 *iface,
        IDirectMusicSegmentState **ppSegmentState, MUSIC_TIME mtTime)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p,%p, %ld): stub\n", This, ppSegmentState, mtTime);
	return S_OK;
}

static HRESULT WINAPI performance_SetPrepareTime(IDirectMusicPerformance8 *iface, DWORD dwMilliSeconds)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);

  TRACE("(%p, %ld)\n", This, dwMilliSeconds);
  This->dwPrepareTime = dwMilliSeconds;
  return S_OK;
}

static HRESULT WINAPI performance_GetPrepareTime(IDirectMusicPerformance8 *iface, DWORD *pdwMilliSeconds)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);

  TRACE("(%p, %p)\n", This, pdwMilliSeconds);
  if (NULL == pdwMilliSeconds) {
    return E_POINTER;
  }
  *pdwMilliSeconds = This->dwPrepareTime;
  return S_OK;
}

static HRESULT WINAPI performance_SetBumperLength(IDirectMusicPerformance8 *iface, DWORD dwMilliSeconds)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);

  TRACE("(%p, %ld)\n", This, dwMilliSeconds);
  This->dwBumperLength =  dwMilliSeconds;
  return S_OK;
}

static HRESULT WINAPI performance_GetBumperLength(IDirectMusicPerformance8 *iface, DWORD *pdwMilliSeconds)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);

  TRACE("(%p, %p)\n", This, pdwMilliSeconds);
  if (NULL == pdwMilliSeconds) {
    return E_POINTER;
  }
  *pdwMilliSeconds = This->dwBumperLength;
  return S_OK;
}

static HRESULT WINAPI performance_SendPMsg(IDirectMusicPerformance8 *iface, DMUS_PMSG *msg)
{
    const DWORD delivery_flags = DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME;
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct message *message, *next;
    HRESULT hr;

    FIXME("(%p, %p): semi-stub\n", This, msg);

    if (!(message = message_from_DMUS_PMSG(msg))) return E_POINTER;
    if (!This->dmusic) return DMUS_E_NO_MASTER_CLOCK;
    if (!(msg->dwFlags & (DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_REFTIME))) return E_INVALIDARG;

    EnterCriticalSection(&This->safe);

    if (!list_empty(&message->entry))
        hr = DMUS_E_ALREADY_SENT;
    else
    {
        if (!(msg->dwFlags & delivery_flags)) msg->dwFlags |= DMUS_PMSGF_TOOL_IMMEDIATE;
        if (!(msg->dwFlags & DMUS_PMSGF_MUSICTIME))
        {
            if (FAILED(hr = IDirectMusicPerformance8_ReferenceToMusicTime(iface,
                    msg->rtTime, &msg->mtTime)))
                goto done;
            msg->dwFlags |= DMUS_PMSGF_MUSICTIME;
        }
        if (!(msg->dwFlags & DMUS_PMSGF_REFTIME))
        {
            if (FAILED(hr = IDirectMusicPerformance8_MusicToReferenceTime(iface,
                    msg->mtTime, &msg->rtTime)))
                goto done;
            msg->dwFlags |= DMUS_PMSGF_REFTIME;
        }

        if (msg->dwFlags & DMUS_PMSGF_TOOL_IMMEDIATE)
        {
            hr = performance_process_message(This, &message->msg, NULL);
            if (hr != DMUS_S_REQUEUE) goto done;
        }

        LIST_FOR_EACH_ENTRY(next, &This->messages, struct message, entry)
            if (next->msg.rtTime > message->msg.rtTime) break;
        list_add_before(&next->entry, &message->entry);
        PostThreadMessageW(This->procThreadId, PROCESSMSG_ADD, 0, 0);

        hr = S_OK;
    }

done:
    LeaveCriticalSection(&This->safe);

    return hr;
}

static HRESULT WINAPI performance_MusicToReferenceTime(IDirectMusicPerformance8 *iface,
        MUSIC_TIME music_time, REFERENCE_TIME *time)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %ld, %p): semi-stub\n", This, music_time, time);

    if (!time) return E_POINTER;
    *time = 0;

    if (!This->master_clock) return DMUS_E_NO_MASTER_CLOCK;

    /* FIXME: This should be (music_time * 60) / (DMUS_PPQ * tempo)
     * but it gives innacurate results */
    *time = This->init_time + (music_time * 6510);

    return S_OK;
}

static HRESULT WINAPI performance_ReferenceToMusicTime(IDirectMusicPerformance8 *iface,
        REFERENCE_TIME time, MUSIC_TIME *music_time)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %I64d, %p): semi-stub\n", This, time, music_time);

    if (!music_time) return E_POINTER;
    *music_time = 0;

    if (!This->master_clock) return DMUS_E_NO_MASTER_CLOCK;

    /* FIXME: This should be (time * DMUS_PPQ * tempo) / 60
     * but it gives innacurate results */
    *music_time = (time - This->init_time) / 6510;

    return S_OK;
}

static HRESULT WINAPI performance_IsPlaying(IDirectMusicPerformance8 *iface,
        IDirectMusicSegment *pSegment, IDirectMusicSegmentState *pSegState)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %p, %p): stub\n", This, pSegment, pSegState);
	return S_FALSE;
}

static HRESULT WINAPI performance_GetTime(IDirectMusicPerformance8 *iface, REFERENCE_TIME *time, MUSIC_TIME *music_time)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    REFERENCE_TIME now;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, time, music_time);

    if (!This->master_clock) return DMUS_E_NO_MASTER_CLOCK;
    if (FAILED(hr = IReferenceClock_GetTime(This->master_clock, &now))) return hr;

    if (time) *time = now;
    if (music_time) hr = IDirectMusicPerformance8_ReferenceToMusicTime(iface, now, music_time);

    return hr;
}

static HRESULT WINAPI performance_AllocPMsg(IDirectMusicPerformance8 *iface, ULONG size, DMUS_PMSG **msg)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct message *message;

    TRACE("(%p, %ld, %p)\n", This, size, msg);

    if (!msg) return E_POINTER;
    if (size < sizeof(DMUS_PMSG)) return E_INVALIDARG;

    if (!(message = calloc(1, size - sizeof(DMUS_PMSG) + sizeof(struct message)))) return E_OUTOFMEMORY;
    message->msg.dwSize = size;
    list_init(&message->entry);
    *msg = &message->msg;

    return S_OK;
}

static HRESULT WINAPI performance_FreePMsg(IDirectMusicPerformance8 *iface, DMUS_PMSG *msg)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct message *message;
    HRESULT hr;

    TRACE("(%p, %p)\n", This, msg);

    if (!(message = message_from_DMUS_PMSG(msg))) return E_POINTER;

    EnterCriticalSection(&This->safe);
    hr = !list_empty(&message->entry) ? DMUS_E_CANNOT_FREE : S_OK;
    LeaveCriticalSection(&This->safe);

    if (SUCCEEDED(hr))
    {
        if (msg->pTool) IDirectMusicTool_Release(msg->pTool);
        if (msg->pGraph) IDirectMusicGraph_Release(msg->pGraph);
        if (msg->punkUser) IUnknown_Release(msg->punkUser);
        free(message);
    }

    return hr;
}

static HRESULT WINAPI performance_GetGraph(IDirectMusicPerformance8 *iface, IDirectMusicGraph **graph)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    TRACE("(%p, %p)\n", This, graph);

    if (!graph)
        return E_POINTER;

    *graph = This->pToolGraph;
    if (This->pToolGraph) {
        IDirectMusicGraph_AddRef(*graph);
    }

    return *graph ? S_OK : DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI performance_SetGraph(IDirectMusicPerformance8 *iface, IDirectMusicGraph *pGraph)
{
  struct performance *This = impl_from_IDirectMusicPerformance8(iface);

  FIXME("(%p, %p): to check\n", This, pGraph);
  
  if (NULL != This->pToolGraph) {
    /* Todo clean buffers and tools before */
    IDirectMusicGraph_Release(This->pToolGraph);
  }
  This->pToolGraph = pGraph;
  if (NULL != This->pToolGraph) {
    IDirectMusicGraph_AddRef(This->pToolGraph);
  }
  return S_OK;
}

static HRESULT WINAPI performance_SetNotificationHandle(IDirectMusicPerformance8 *iface,
        HANDLE notification_event, REFERENCE_TIME minimum_time)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    TRACE("(%p, %p, %I64d)\n", This, notification_event, minimum_time);

    This->notification_event = notification_event;
    if (minimum_time)
        This->notification_timeout = minimum_time;
    else if (!This->notification_timeout)
        This->notification_timeout = 20000000; /* 2 seconds */

    return S_OK;
}

static HRESULT WINAPI performance_GetNotificationPMsg(IDirectMusicPerformance8 *iface,
        DMUS_NOTIFICATION_PMSG **ret_msg)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct list *entry;

    TRACE("(%p, %p)\n", This, ret_msg);

    if (!ret_msg) return E_POINTER;

    EnterCriticalSection(&This->safe);
    if ((entry = list_head(&This->notifications)))
    {
        struct message *message = LIST_ENTRY(entry, struct message, entry);
        list_remove(&message->entry);
        list_init(&message->entry);
        *ret_msg = (DMUS_NOTIFICATION_PMSG *)&message->msg;
    }
    LeaveCriticalSection(&This->safe);

    return entry ? S_OK : S_FALSE;
}

static HRESULT WINAPI performance_AddNotificationType(IDirectMusicPerformance8 *iface, REFGUID type)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    HRESULT hr = S_OK;

    FIXME("(%p, %s): stub\n", This, debugstr_dmguid(type));

    if (IsEqualGUID(type, &GUID_NOTIFICATION_PERFORMANCE))
    {
        hr = This->notification_performance ? S_FALSE : S_OK;
        This->notification_performance = TRUE;
    }
    if (IsEqualGUID(type, &GUID_NOTIFICATION_SEGMENT))
    {
        hr = This->notification_segment ? S_FALSE : S_OK;
        This->notification_segment = TRUE;
    }

    return hr;
}

static HRESULT WINAPI performance_RemoveNotificationType(IDirectMusicPerformance8 *iface, REFGUID type)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    HRESULT hr = S_FALSE;

    FIXME("(%p, %s): stub\n", This, debugstr_dmguid(type));

    if (IsEqualGUID(type, &GUID_NOTIFICATION_PERFORMANCE))
    {
        hr = This->notification_performance ? S_OK : S_FALSE;
        This->notification_performance = FALSE;
    }
    if (IsEqualGUID(type, &GUID_NOTIFICATION_SEGMENT))
    {
        hr = This->notification_segment ? S_OK : S_FALSE;
        This->notification_segment = FALSE;
    }

    return hr;
}

static HRESULT perf_dmport_create(struct performance *perf, DMUS_PORTPARAMS *params)
{
    IDirectMusicPort *port;
    GUID guid;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = IDirectMusic8_GetDefaultPort(perf->dmusic, &guid)))
        return hr;

    if (FAILED(hr = IDirectMusic8_CreatePort(perf->dmusic, &guid, params, &port, NULL)))
        return hr;

    if (FAILED(hr = IDirectMusicPort_SetDirectSound(port, perf->dsound, NULL))
            || FAILED(hr = IDirectMusicPort_Activate(port, TRUE)))
    {
        IDirectMusicPort_Release(port);
        return hr;
    }

    for (i = 0; i < params->dwChannelGroups; i++)
        pchannel_block_set(&perf->pchannels, i, port, i + 1, FALSE);

    return S_OK;
}

static HRESULT WINAPI performance_AddPort(IDirectMusicPerformance8 *iface, IDirectMusicPort *port)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %p): semi-stub\n", This, port);

    if (!This->dmusic) return DMUS_E_NOT_INIT;
    if (This->audio_paths_enabled) return DMUS_E_AUDIOPATHS_IN_USE;

    if (!port) {
        DMUS_PORTPARAMS params = {
            .dwSize = sizeof(params),
            .dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS,
            .dwChannelGroups = 1
        };

        return perf_dmport_create(This, &params);
    }

    IDirectMusicPort_AddRef(port);
    /**
     * We should remember added Ports (for example using a list)
     * and control if Port is registered for each api who use ports
     */
    return S_OK;
}

static HRESULT WINAPI performance_RemovePort(IDirectMusicPerformance8 *iface, IDirectMusicPort *pPort)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    if (This->audio_paths_enabled) return DMUS_E_AUDIOPATHS_IN_USE;

    FIXME("(%p, %p): stub\n", This, pPort);
    IDirectMusicPort_Release(pPort);
    return S_OK;
}

static HRESULT WINAPI performance_AssignPChannelBlock(IDirectMusicPerformance8 *iface,
        DWORD block_num, IDirectMusicPort *port, DWORD group)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %ld, %p, %ld): semi-stub\n", This, block_num, port, group);

    if (!port) return E_POINTER;
    if (block_num > MAXDWORD / 16) return E_INVALIDARG;
    if (This->audio_paths_enabled) return DMUS_E_AUDIOPATHS_IN_USE;

    pchannel_block_set(&This->pchannels, block_num, port, group, FALSE);

    return S_OK;
}

static HRESULT WINAPI performance_AssignPChannel(IDirectMusicPerformance8 *iface, DWORD pchannel,
        IDirectMusicPort *port, DWORD group, DWORD channel)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct pchannel_block *block;

    FIXME("(%p)->(%ld, %p, %ld, %ld) semi-stub\n", This, pchannel, port, group, channel);

    if (!port) return E_POINTER;
    if (This->audio_paths_enabled) return DMUS_E_AUDIOPATHS_IN_USE;

    block = pchannel_block_set(&This->pchannels, pchannel / 16, port, 0, TRUE);
    if (block) {
        block->pchannel[pchannel % 16].group = group;
        block->pchannel[pchannel % 16].channel = channel;
    }

    return S_OK;
}

static HRESULT WINAPI performance_PChannelInfo(IDirectMusicPerformance8 *iface, DWORD pchannel,
        IDirectMusicPort **port, DWORD *group, DWORD *channel)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct pchannel_block *block;
    struct wine_rb_entry *entry;
    DWORD block_num = pchannel / 16;
    unsigned int index = pchannel % 16;

    TRACE("(%p)->(%ld, %p, %p, %p)\n", This, pchannel, port, group, channel);

    entry = wine_rb_get(&This->pchannels, &block_num);
    if (!entry)
        return E_INVALIDARG;
    block = WINE_RB_ENTRY_VALUE(entry, struct pchannel_block, entry);

    if (port) {
        *port = block->pchannel[index].port;
        IDirectMusicPort_AddRef(*port);
    }
    if (group)
        *group = block->pchannel[index].group;
    if (channel)
        *channel = block->pchannel[index].channel;

    return S_OK;
}

static HRESULT WINAPI performance_DownloadInstrument(IDirectMusicPerformance8 *iface,
        IDirectMusicInstrument *instrument, DWORD port_channel,
        IDirectMusicDownloadedInstrument **downloaded, DMUS_NOTERANGE *note_ranges,
        DWORD note_range_count, IDirectMusicPort **port, DWORD *group, DWORD *music_channel)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    IDirectMusicPort *tmp_port = NULL;
    HRESULT hr;

    TRACE("(%p, %p, %ld, %p, %p, %ld, %p, %p, %p)\n", This, instrument, port_channel, downloaded,
            note_ranges, note_range_count, port, group, music_channel);

    if (!port) port = &tmp_port;
    if (FAILED(hr = IDirectMusicPerformance_PChannelInfo(iface, port_channel, port, group, music_channel)))
        return hr;

    hr = IDirectMusicPort_DownloadInstrument(*port, instrument, downloaded, note_ranges, note_range_count);
    if (tmp_port) IDirectMusicPort_Release(tmp_port);
    return hr;
}

static HRESULT WINAPI performance_Invalidate(IDirectMusicPerformance8 *iface, MUSIC_TIME mtTime, DWORD dwFlags)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %ld, %ld): stub\n", This, mtTime, dwFlags);
	return S_OK;
}

static HRESULT WINAPI performance_GetParam(IDirectMusicPerformance8 *iface, REFGUID rguidType,
        DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, mtTime, pmtNext, pParam);
	return S_OK;
}

static HRESULT WINAPI performance_SetParam(IDirectMusicPerformance8 *iface, REFGUID rguidType,
        DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, void *pParam)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %p): stub\n", This, debugstr_dmguid(rguidType), dwGroupBits, dwIndex, mtTime, pParam);
	return S_OK;
}

static HRESULT WINAPI performance_GetGlobalParam(IDirectMusicPerformance8 *iface, REFGUID rguidType,
        void *pParam, DWORD dwSize)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	TRACE("(%p, %s, %p, %ld): stub\n", This, debugstr_dmguid(rguidType), pParam, dwSize);

	if (IsEqualGUID (rguidType, &GUID_PerfAutoDownload))
		memcpy(pParam, &This->fAutoDownload, sizeof(This->fAutoDownload));
	if (IsEqualGUID (rguidType, &GUID_PerfMasterGrooveLevel))
		memcpy(pParam, &This->cMasterGrooveLevel, sizeof(This->cMasterGrooveLevel));
	if (IsEqualGUID (rguidType, &GUID_PerfMasterTempo))
		memcpy(pParam, &This->fMasterTempo, sizeof(This->fMasterTempo));
	if (IsEqualGUID (rguidType, &GUID_PerfMasterVolume))
		memcpy(pParam, &This->lMasterVolume, sizeof(This->lMasterVolume));

	return S_OK;
}

static HRESULT WINAPI performance_SetGlobalParam(IDirectMusicPerformance8 *iface, REFGUID rguidType,
        void *pParam, DWORD dwSize)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	TRACE("(%p, %s, %p, %ld)\n", This, debugstr_dmguid(rguidType), pParam, dwSize);

	if (IsEqualGUID (rguidType, &GUID_PerfAutoDownload)) {
		memcpy(&This->fAutoDownload, pParam, dwSize);
		TRACE("=> AutoDownload set to %d\n", This->fAutoDownload);
	}
	if (IsEqualGUID (rguidType, &GUID_PerfMasterGrooveLevel)) {
		memcpy(&This->cMasterGrooveLevel, pParam, dwSize);
		TRACE("=> MasterGrooveLevel set to %i\n", This->cMasterGrooveLevel);
	}
	if (IsEqualGUID (rguidType, &GUID_PerfMasterTempo)) {
		memcpy(&This->fMasterTempo, pParam, dwSize);
		TRACE("=> MasterTempo set to %f\n", This->fMasterTempo);
	}
	if (IsEqualGUID (rguidType, &GUID_PerfMasterVolume)) {
		memcpy(&This->lMasterVolume, pParam, dwSize);
		TRACE("=> MasterVolume set to %li\n", This->lMasterVolume);
	}

	return S_OK;
}

static HRESULT WINAPI performance_GetLatencyTime(IDirectMusicPerformance8 *iface, REFERENCE_TIME *prtTime)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	TRACE("(%p, %p): stub\n", This, prtTime);
	*prtTime = This->rtLatencyTime;
	return S_OK;
}

static HRESULT WINAPI performance_GetQueueTime(IDirectMusicPerformance8 *iface, REFERENCE_TIME *prtTime)

{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %p): stub\n", This, prtTime);
	return S_OK;
}

static HRESULT WINAPI performance_AdjustTime(IDirectMusicPerformance8 *iface, REFERENCE_TIME rtAmount)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, 0x%s): stub\n", This, wine_dbgstr_longlong(rtAmount));
	return S_OK;
}

static HRESULT WINAPI performance_CloseDown(IDirectMusicPerformance8 *iface)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    struct message *message, *next;
    HRESULT hr;

    FIXME("(%p): semi-stub\n", This);

    if (PostMessageToProcessMsgThread(This, PROCESSMSG_EXIT)) {
        WaitForSingleObject(This->procThread, INFINITE);
        This->procThreadTicStarted = FALSE;
        CloseHandle(This->procThread);
    }

    This->notification_performance = FALSE;
    This->notification_segment = FALSE;

    LIST_FOR_EACH_ENTRY_SAFE(message, next, &This->messages, struct message, entry)
    {
        list_remove(&message->entry);
        list_init(&message->entry);

        /* process notifications to end any pending segment states */
        if (message->msg.dwType == DMUS_PMSGT_NOTIFICATION)
            hr = IDirectMusicTool_ProcessPMsg(&This->IDirectMusicTool_iface,
                    (IDirectMusicPerformance *)iface, &message->msg);
        else
            hr = DMUS_S_FREE;

        if (hr == DMUS_S_FREE && FAILED(hr = IDirectMusicPerformance8_FreePMsg(iface, &message->msg)))
            WARN("Failed to free message %p, hr %#lx\n", message, hr);
    }

    LIST_FOR_EACH_ENTRY_SAFE(message, next, &This->notifications, struct message, entry)
    {
        list_remove(&message->entry);
        list_init(&message->entry);

        if (FAILED(hr = IDirectMusicPerformance8_FreePMsg(iface, &message->msg)))
            WARN("Failed to free message %p, hr %#lx\n", message, hr);
    }

    if (This->master_clock)
    {
        IReferenceClock_Release(This->master_clock);
        This->master_clock = NULL;
    }
    if (This->dsound) {
        IDirectSound_Release(This->dsound);
        This->dsound = NULL;
    }
    if (This->dmusic) {
        IDirectMusic8_SetDirectSound(This->dmusic, NULL, NULL);
        IDirectMusic8_Release(This->dmusic);
        This->dmusic = NULL;
    }
    This->audio_paths_enabled = FALSE;

    return S_OK;
}

static HRESULT WINAPI performance_GetResolvedTime(IDirectMusicPerformance8 *iface,
        REFERENCE_TIME rtTime, REFERENCE_TIME *prtResolved, DWORD dwTimeResolveFlags)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, 0x%s, %p, %ld): stub\n", This, wine_dbgstr_longlong(rtTime),
	    prtResolved, dwTimeResolveFlags);
	return S_OK;
}

static HRESULT WINAPI performance_MIDIToMusic(IDirectMusicPerformance8 *iface, BYTE bMIDIValue,
        DMUS_CHORD_KEY *pChord, BYTE bPlayMode, BYTE bChordLevel, WORD *pwMusicValue)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, bMIDIValue, pChord, bPlayMode, bChordLevel, pwMusicValue);
	return S_OK;
}

static HRESULT WINAPI performance_MusicToMIDI(IDirectMusicPerformance8 *iface, WORD wMusicValue,
        DMUS_CHORD_KEY *pChord, BYTE bPlayMode, BYTE bChordLevel, BYTE *pbMIDIValue)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %d, %p, %d, %d, %p): stub\n", This, wMusicValue, pChord, bPlayMode, bChordLevel, pbMIDIValue);
	return S_OK;
}

static HRESULT WINAPI performance_TimeToRhythm(IDirectMusicPerformance8 *iface, MUSIC_TIME mtTime,
        DMUS_TIMESIGNATURE *pTimeSig, WORD *pwMeasure, BYTE *pbBeat, BYTE *pbGrid, short *pnOffset)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %ld, %p, %p, %p, %p, %p): stub\n", This, mtTime, pTimeSig, pwMeasure, pbBeat, pbGrid, pnOffset);
	return S_OK;
}

static HRESULT WINAPI performance_RhythmToTime(IDirectMusicPerformance8 *iface, WORD wMeasure,
        BYTE bBeat, BYTE bGrid, short nOffset, DMUS_TIMESIGNATURE *pTimeSig, MUSIC_TIME *pmtTime)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %d, %d, %d, %i, %p, %p): stub\n", This, wMeasure, bBeat, bGrid, nOffset, pTimeSig, pmtTime);
	return S_OK;
}

/* IDirectMusicPerformance8 Interface part follow: */
static HRESULT WINAPI performance_InitAudio(IDirectMusicPerformance8 *iface, IDirectMusic **dmusic,
        IDirectSound **dsound, HWND hwnd, DWORD default_path_type, DWORD num_channels, DWORD flags,
        DMUS_AUDIOPARAMS *params)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %p, %p, %lx, %lu, %lx, %p)\n", This, dmusic, dsound, hwnd, default_path_type,
            num_channels, flags, params);

    if (flags) FIXME("flags parameter not used\n");
    if (params) FIXME("params parameter not used\n");

    if (FAILED(hr = IDirectMusicPerformance8_Init(iface, dmusic && *dmusic ? dmusic : NULL,
            dsound ? *dsound : NULL, hwnd)))
        return hr;

    This->audio_paths_enabled = TRUE;
    if (default_path_type)
    {
        hr = IDirectMusicPerformance8_CreateStandardAudioPath(iface, default_path_type,
                num_channels, FALSE, &This->pDefaultPath);
        if (FAILED(hr))
        {
            IDirectMusicPerformance_CloseDown(iface);
            return hr;
        }
    }

    if (dsound && !*dsound) {
        *dsound = This->dsound;
        IDirectSound_AddRef(*dsound);
    }
    if (dmusic && !*dmusic) {
        *dmusic = (IDirectMusic *)This->dmusic;
        IDirectMusic_AddRef(*dmusic);
    }

    return S_OK;
}

static HRESULT WINAPI performance_PlaySegmentEx(IDirectMusicPerformance8 *iface, IUnknown *source,
        WCHAR *segment_name, IUnknown *transition, DWORD segment_flags, INT64 start_time,
        IDirectMusicSegmentState **segment_state, IUnknown *from, IUnknown *audio_path)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    IDirectMusicSegmentState *state;
    IDirectMusicSegment *segment;
    MUSIC_TIME length;
    HRESULT hr;

    FIXME("(%p, %p, %s, %p, %#lx, %I64d, %p, %p, %p): stub\n", This, source, debugstr_w(segment_name),
            transition, segment_flags, start_time, segment_state, from, audio_path);

    /* NOTE: The time is in music time unless the DMUS_SEGF_REFTIME flag is set. */
    if (segment_flags) FIXME("flags %#lx not implemented\n", segment_flags);
    if (start_time) FIXME("start_time %I64d not implemented\n", start_time);

    if (FAILED(hr = IUnknown_QueryInterface(source, &IID_IDirectMusicSegment, (void **)&segment)))
        return hr;
    if (FAILED(hr = segment_state_create(segment, start_time, (IDirectMusicPerformance *)iface, &state)))
    {
        IDirectMusicSegment_Release(segment);
        return hr;
    }

    hr = IDirectMusicSegment_GetLength(segment, &length);
    if (SUCCEEDED(hr))
        hr = performance_send_notification_pmsg(This, start_time, This->notification_performance,
                GUID_NOTIFICATION_PERFORMANCE, DMUS_NOTIFICATION_MUSICSTARTED, NULL);
    if (SUCCEEDED(hr))
        hr = performance_send_notification_pmsg(This, start_time, This->notification_segment,
                GUID_NOTIFICATION_SEGMENT, DMUS_NOTIFICATION_SEGSTART, (IUnknown *)state);
    if (SUCCEEDED(hr))
        hr = performance_send_dirty_pmsg(This, start_time);

    if (SUCCEEDED(hr))
        hr = segment_state_play(state, (IDirectMusicPerformance *)iface);

    if (SUCCEEDED(hr))
        hr = performance_send_notification_pmsg(This, start_time + length, This->notification_segment,
                GUID_NOTIFICATION_SEGMENT, DMUS_NOTIFICATION_SEGEND, (IUnknown *)state);
    if (SUCCEEDED(hr))
        hr = performance_send_notification_pmsg(This, start_time + length, This->notification_segment,
                GUID_NOTIFICATION_SEGMENT, DMUS_NOTIFICATION_SEGALMOSTEND, (IUnknown *)state);
    if (SUCCEEDED(hr))
        hr = performance_send_dirty_pmsg(This, start_time + length);
    if (SUCCEEDED(hr))
        hr = performance_send_notification_pmsg(This, start_time + length, This->notification_performance,
                GUID_NOTIFICATION_PERFORMANCE, DMUS_NOTIFICATION_MUSICSTOPPED, NULL);

    if (SUCCEEDED(hr) && segment_state)
    {
        *segment_state = state;
        IDirectMusicSegmentState_AddRef(state);
    }

    IDirectMusicSegmentState_Release(state);
    IDirectMusicSegment_Release(segment);
    return hr;
}

static HRESULT WINAPI performance_StopEx(IDirectMusicPerformance8 *iface, IUnknown *pObjectToStop,
        __int64 i64StopTime, DWORD dwFlags)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %p, 0x%s, %ld): stub\n", This, pObjectToStop,
	    wine_dbgstr_longlong(i64StopTime), dwFlags);
	return S_OK;
}

static HRESULT WINAPI performance_ClonePMsg(IDirectMusicPerformance8 *iface, DMUS_PMSG *msg, DMUS_PMSG **clone)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", This, msg, clone);

    if (!msg || !clone) return E_POINTER;
    if (FAILED(hr = IDirectMusicPerformance8_AllocPMsg(iface, msg->dwSize, clone))) return hr;

    memcpy(*clone, msg, msg->dwSize);
    if (msg->pTool) IDirectMusicTool_AddRef(msg->pTool);
    if (msg->pGraph) IDirectMusicGraph_AddRef(msg->pGraph);
    if (msg->punkUser) IUnknown_AddRef(msg->punkUser);

    return S_OK;
}

static HRESULT WINAPI performance_CreateAudioPath(IDirectMusicPerformance8 *iface,
        IUnknown *pSourceConfig, BOOL fActivate, IDirectMusicAudioPath **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    IDirectMusicAudioPath *pPath;

    FIXME("(%p, %p, %d, %p): stub\n", This, pSourceConfig, fActivate, ret_iface);

    if (!ret_iface) return E_POINTER;
    if (!This->audio_paths_enabled) return DMUS_E_AUDIOPATH_INACTIVE;

    create_dmaudiopath(&IID_IDirectMusicAudioPath, (void **)&pPath);
    set_audiopath_perf_pointer(pPath, iface);

    /** TODO */
    *ret_iface = pPath;
    return IDirectMusicAudioPath_Activate(*ret_iface, fActivate);
}

static HRESULT WINAPI performance_CreateStandardAudioPath(IDirectMusicPerformance8 *iface,
        DWORD dwType, DWORD pchannel_count, BOOL fActivate, IDirectMusicAudioPath **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);
    IDirectMusicAudioPath *pPath;
    DSBUFFERDESC desc;
    WAVEFORMATEX format;
    DMUS_PORTPARAMS params = {0};
    IDirectSoundBuffer *buffer, *primary_buffer;
    HRESULT hr = S_OK;

    FIXME("(%p)->(%ld, %ld, %d, %p): semi-stub\n", This, dwType, pchannel_count, fActivate, ret_iface);

    if (!ret_iface) return E_POINTER;
    if (!This->audio_paths_enabled) return DMUS_E_AUDIOPATH_INACTIVE;

    *ret_iface = NULL;

	/* Secondary buffer description */
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = 44000;
	format.nAvgBytesPerSec = 44000*2;
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	format.cbSize = 0;
	
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
        desc.dwFlags = DSBCAPS_CTRLFX | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = DSBSIZE_MIN;
	desc.dwReserved = 0;
	desc.lpwfxFormat = &format;
	desc.guid3DAlgorithm = GUID_NULL;
	
	switch(dwType) {
	case DMUS_APATH_DYNAMIC_3D:
                desc.dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY | DSBCAPS_MUTE3DATMAXDISTANCE;
		break;
	case DMUS_APATH_DYNAMIC_MONO:
                desc.dwFlags |= DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
		break;
	case DMUS_APATH_SHARED_STEREOPLUSREVERB:
	        /* normally we have to create 2 buffers (one for music other for reverb)
		 * in this case. See msdn
                 */
	case DMUS_APATH_DYNAMIC_STEREO:
                desc.dwFlags |= DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
		format.nChannels = 2;
		format.nBlockAlign *= 2;
		format.nAvgBytesPerSec *=2;
		break;
	default:
	        return E_INVALIDARG;
	}

        /* Create a port */
        params.dwSize = sizeof(params);
        params.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_AUDIOCHANNELS;
        params.dwChannelGroups = (pchannel_count + 15) / 16;
        params.dwAudioChannels = format.nChannels;
        if (FAILED(hr = perf_dmport_create(This, &params)))
                return hr;

        hr = IDirectSound_CreateSoundBuffer(This->dsound, &desc, &buffer, NULL);
	if (FAILED(hr))
	        return DSERR_BUFFERLOST;

	/* Update description for creating primary buffer */
	desc.dwFlags |= DSBCAPS_PRIMARYBUFFER;
	desc.dwFlags &= ~DSBCAPS_CTRLFX;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = NULL;

        hr = IDirectSound_CreateSoundBuffer(This->dsound, &desc, &primary_buffer, NULL);
	if (FAILED(hr)) {
                IDirectSoundBuffer_Release(buffer);
	        return DSERR_BUFFERLOST;
	}

	create_dmaudiopath(&IID_IDirectMusicAudioPath, (void**)&pPath);
	set_audiopath_perf_pointer(pPath, iface);
	set_audiopath_dsound_buffer(pPath, buffer);
	set_audiopath_primary_dsound_buffer(pPath, primary_buffer);

    *ret_iface = pPath;
    TRACE(" returning IDirectMusicAudioPath interface at %p.\n", *ret_iface);
    return IDirectMusicAudioPath_Activate(*ret_iface, fActivate);
}

static HRESULT WINAPI performance_SetDefaultAudioPath(IDirectMusicPerformance8 *iface, IDirectMusicAudioPath *audio_path)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %p): semi-stub\n", This, audio_path);

    if (!This->audio_paths_enabled) return DMUS_E_AUDIOPATH_INACTIVE;

    if (This->pDefaultPath) IDirectMusicAudioPath_Release(This->pDefaultPath);
    if ((This->pDefaultPath = audio_path))
    {
        IDirectMusicAudioPath_AddRef(This->pDefaultPath);
        set_audiopath_perf_pointer(This->pDefaultPath, iface);
    }

    return S_OK;
}

static HRESULT WINAPI performance_GetDefaultAudioPath(IDirectMusicPerformance8 *iface,
        IDirectMusicAudioPath **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicPerformance8(iface);

    FIXME("(%p, %p): semi-stub (%p)\n", This, ret_iface, This->pDefaultPath);

    if (!ret_iface) return E_POINTER;
    if (!This->audio_paths_enabled) return DMUS_E_AUDIOPATH_INACTIVE;

    if ((*ret_iface = This->pDefaultPath)) IDirectMusicAudioPath_AddRef(*ret_iface);

    return S_OK;
}

static HRESULT WINAPI performance_GetParamEx(IDirectMusicPerformance8 *iface, REFGUID rguidType, DWORD dwTrackID,
        DWORD dwGroupBits, DWORD dwIndex, MUSIC_TIME mtTime, MUSIC_TIME *pmtNext, void *pParam)
{
        struct performance *This = impl_from_IDirectMusicPerformance8(iface);

	FIXME("(%p, %s, %ld, %ld, %ld, %ld, %p, %p): stub\n", This, debugstr_dmguid(rguidType), dwTrackID, dwGroupBits, dwIndex, mtTime, pmtNext, pParam);

	return S_OK;
}

static const IDirectMusicPerformance8Vtbl performance_vtbl =
{
    performance_QueryInterface,
    performance_AddRef,
    performance_Release,
    performance_Init,
    performance_PlaySegment,
    performance_Stop,
    performance_GetSegmentState,
    performance_SetPrepareTime,
    performance_GetPrepareTime,
    performance_SetBumperLength,
    performance_GetBumperLength,
    performance_SendPMsg,
    performance_MusicToReferenceTime,
    performance_ReferenceToMusicTime,
    performance_IsPlaying,
    performance_GetTime,
    performance_AllocPMsg,
    performance_FreePMsg,
    performance_GetGraph,
    performance_SetGraph,
    performance_SetNotificationHandle,
    performance_GetNotificationPMsg,
    performance_AddNotificationType,
    performance_RemoveNotificationType,
    performance_AddPort,
    performance_RemovePort,
    performance_AssignPChannelBlock,
    performance_AssignPChannel,
    performance_PChannelInfo,
    performance_DownloadInstrument,
    performance_Invalidate,
    performance_GetParam,
    performance_SetParam,
    performance_GetGlobalParam,
    performance_SetGlobalParam,
    performance_GetLatencyTime,
    performance_GetQueueTime,
    performance_AdjustTime,
    performance_CloseDown,
    performance_GetResolvedTime,
    performance_MIDIToMusic,
    performance_MusicToMIDI,
    performance_TimeToRhythm,
    performance_RhythmToTime,
    performance_InitAudio,
    performance_PlaySegmentEx,
    performance_StopEx,
    performance_ClonePMsg,
    performance_CreateAudioPath,
    performance_CreateStandardAudioPath,
    performance_SetDefaultAudioPath,
    performance_GetDefaultAudioPath,
    performance_GetParamEx,
};

static inline struct performance *impl_from_IDirectMusicGraph(IDirectMusicGraph *iface)
{
    return CONTAINING_RECORD(iface, struct performance, IDirectMusicGraph_iface);
}

static HRESULT WINAPI performance_graph_QueryInterface(IDirectMusicGraph *iface, REFIID riid, void **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicPerformance8_QueryInterface(&This->IDirectMusicPerformance8_iface, riid, ret_iface);
}

static ULONG WINAPI performance_graph_AddRef(IDirectMusicGraph *iface)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicPerformance8_AddRef(&This->IDirectMusicPerformance8_iface);
}

static ULONG WINAPI performance_graph_Release(IDirectMusicGraph *iface)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    return IDirectMusicPerformance8_Release(&This->IDirectMusicPerformance8_iface);
}

static HRESULT WINAPI performance_graph_StampPMsg(IDirectMusicGraph *iface, DMUS_PMSG *msg)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    HRESULT hr;

    TRACE("(%p, %p)\n", This, msg);

    if (!msg) return E_POINTER;

    /* FIXME: Implement segment and audio path graphs support */
    if (!This->pToolGraph) hr = DMUS_S_LAST_TOOL;
    else if (FAILED(hr = IDirectMusicGraph_StampPMsg(This->pToolGraph, msg))) return hr;

    if (msg->pGraph)
    {
        IDirectMusicTool_Release(msg->pGraph);
        msg->pGraph = NULL;
    }

    if (hr == DMUS_S_LAST_TOOL)
    {
        const DWORD delivery_flags = DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME;
        msg->dwFlags &= ~delivery_flags;
        msg->dwFlags |= DMUS_PMSGF_TOOL_QUEUE;

        if (msg->pTool) IDirectMusicTool_Release(msg->pTool);
        msg->pTool = &This->IDirectMusicTool_iface;
        IDirectMusicTool_AddRef(msg->pTool);
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        msg->pGraph = &This->IDirectMusicGraph_iface;
        IDirectMusicTool_AddRef(msg->pGraph);
    }

    return hr;
}

static HRESULT WINAPI performance_graph_InsertTool(IDirectMusicGraph *iface, IDirectMusicTool *tool,
        DWORD *channels, DWORD channels_count, LONG index)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %p, %p, %lu, %ld)\n", This, tool, channels, channels_count, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI performance_graph_GetTool(IDirectMusicGraph *iface, DWORD index, IDirectMusicTool **tool)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %lu, %p)\n", This, index, tool);
    return E_NOTIMPL;
}

static HRESULT WINAPI performance_graph_RemoveTool(IDirectMusicGraph *iface, IDirectMusicTool *tool)
{
    struct performance *This = impl_from_IDirectMusicGraph(iface);
    TRACE("(%p, %p)\n", This, tool);
    return E_NOTIMPL;
}

static const IDirectMusicGraphVtbl performance_graph_vtbl =
{
    performance_graph_QueryInterface,
    performance_graph_AddRef,
    performance_graph_Release,
    performance_graph_StampPMsg,
    performance_graph_InsertTool,
    performance_graph_GetTool,
    performance_graph_RemoveTool,
};

static inline struct performance *impl_from_IDirectMusicTool(IDirectMusicTool *iface)
{
    return CONTAINING_RECORD(iface, struct performance, IDirectMusicTool_iface);
}

static HRESULT WINAPI performance_tool_QueryInterface(IDirectMusicTool *iface, REFIID riid, void **ret_iface)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    return IDirectMusicPerformance8_QueryInterface(&This->IDirectMusicPerformance8_iface, riid, ret_iface);
}

static ULONG WINAPI performance_tool_AddRef(IDirectMusicTool *iface)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    return IDirectMusicPerformance8_AddRef(&This->IDirectMusicPerformance8_iface);
}

static ULONG WINAPI performance_tool_Release(IDirectMusicTool *iface)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    return IDirectMusicPerformance8_Release(&This->IDirectMusicPerformance8_iface);
}

static HRESULT WINAPI performance_tool_Init(IDirectMusicTool *iface, IDirectMusicGraph *graph)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    TRACE("(%p, %p)\n", This, graph);
    return E_NOTIMPL;
}

static HRESULT WINAPI performance_tool_GetMsgDeliveryType(IDirectMusicTool *iface, DWORD *type)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    TRACE("(%p, %p)\n", This, type);
    *type = DMUS_PMSGF_TOOL_IMMEDIATE;
    return S_OK;
}

static HRESULT WINAPI performance_tool_GetMediaTypeArraySize(IDirectMusicTool *iface, DWORD *size)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    TRACE("(%p, %p)\n", This, size);
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI performance_tool_GetMediaTypes(IDirectMusicTool *iface, DWORD **types, DWORD size)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    TRACE("(%p, %p, %lu)\n", This, types, size);
    return E_NOTIMPL;
}

static HRESULT performance_send_midi_pmsg(struct performance *This, DMUS_PMSG *msg, UINT flags,
        BYTE status, BYTE byte1, BYTE byte2)
{
    IDirectMusicPerformance8 *performance = &This->IDirectMusicPerformance8_iface;
    DMUS_MIDI_PMSG *midi;
    HRESULT hr;

    if (FAILED(hr = IDirectMusicPerformance8_AllocPMsg(performance, sizeof(*midi),
            (DMUS_PMSG **)&midi)))
        return hr;

    if (flags & DMUS_PMSGF_REFTIME) midi->rtTime = msg->rtTime;
    if (flags & DMUS_PMSGF_MUSICTIME) midi->mtTime = msg->mtTime;
    midi->dwFlags = flags;
    midi->dwPChannel = msg->dwPChannel;
    midi->dwVirtualTrackID = msg->dwVirtualTrackID;
    midi->dwVoiceID = msg->dwVoiceID;
    midi->dwGroupID = msg->dwGroupID;
    midi->dwType = DMUS_PMSGT_MIDI;
    midi->bStatus = status;
    midi->bByte1 = byte1;
    midi->bByte2 = byte2;

    if (FAILED(hr = IDirectMusicPerformance8_SendPMsg(performance, (DMUS_PMSG *)midi)))
        IDirectMusicPerformance8_FreePMsg(performance, (DMUS_PMSG *)midi);

    return hr;
}

static HRESULT WINAPI performance_tool_ProcessPMsg(IDirectMusicTool *iface,
        IDirectMusicPerformance *performance, DMUS_PMSG *msg)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    struct message *message = message_from_DMUS_PMSG(msg);
    HRESULT hr;

    FIXME("(%p, %p, %p): semi-stub\n", This, performance, msg);

    switch (msg->dwType)
    {
    case DMUS_PMSGT_MIDI:
    {
        static const UINT event_size = sizeof(DMUS_EVENTHEADER) + sizeof(DWORD);
        DMUS_BUFFERDESC desc = {.dwSize = sizeof(desc), .cbBuffer = 2 * event_size};
        DMUS_MIDI_PMSG *midi = (DMUS_MIDI_PMSG *)msg;
        IDirectMusicBuffer *buffer;
        IDirectMusicPort *port;
        DWORD group, channel;
        UINT value = 0;

        if (FAILED(hr = IDirectMusicPerformance_PChannelInfo(performance, msg->dwPChannel,
                &port, &group, &channel)))
        {
            WARN("Failed to get message port, hr %#lx\n", hr);
            return DMUS_S_FREE;
        }

        value |= channel;
        value |= (UINT)midi->bStatus;
        value |= (UINT)midi->bByte1 << 8;
        value |= (UINT)midi->bByte2 << 16;

        if (SUCCEEDED(hr = IDirectMusic_CreateMusicBuffer(This->dmusic, &desc, &buffer, NULL)))
        {
            hr = IDirectMusicBuffer_PackStructured(buffer, msg->rtTime, group, value);
            if (SUCCEEDED(hr)) hr = IDirectMusicPort_PlayBuffer(port, buffer);
            IDirectMusicBuffer_Release(buffer);
        }

        IDirectMusicPort_Release(port);
        break;
    }

    case DMUS_PMSGT_NOTE:
    {
        DMUS_NOTE_PMSG *note = (DMUS_NOTE_PMSG *)msg;

        if (FAILED(hr = performance_send_midi_pmsg(This, msg, DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE,
                0x90 /* NOTE_ON */, note->bMidiValue, note->bVelocity)))
            WARN("Failed to translate message to MIDI, hr %#lx\n", hr);

        msg->mtTime += note->mtDuration;
        if (FAILED(hr = performance_send_midi_pmsg(This, msg, DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_QUEUE,
                0x80 /* NOTE_OFF */, note->bMidiValue, 0)))
            WARN("Failed to translate message to MIDI, hr %#lx\n", hr);

        break;
    }

    case DMUS_PMSGT_PATCH:
    {
        DMUS_PATCH_PMSG *patch = (DMUS_PATCH_PMSG *)msg;

        if (FAILED(hr = performance_send_midi_pmsg(This, msg, DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE,
                0xb0 /* Control Change */, 0x00 /* CC: Bank MSB */, patch->byMSB)))
            WARN("Failed to translate message to MIDI, hr %#lx\n", hr);

        if (FAILED(hr = performance_send_midi_pmsg(This, msg, DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE,
                0xb0 /* Control Change */, 0x20 /* CC: Bank LSB */, patch->byLSB)))
            WARN("Failed to translate message to MIDI, hr %#lx\n", hr);

        if (FAILED(hr = performance_send_midi_pmsg(This, msg, DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE,
                0xc0 /* Program Change */, patch->byInstrument, 0)))
            WARN("Failed to translate message to MIDI, hr %#lx\n", hr);

        break;
    }

    case DMUS_PMSGT_NOTIFICATION:
    {
        DMUS_NOTIFICATION_PMSG *notif = (DMUS_NOTIFICATION_PMSG *)msg;
        struct message *previous;
        BOOL enabled = FALSE;

        if (IsEqualGUID(&notif->guidNotificationType, &GUID_NOTIFICATION_SEGMENT)
                && notif->dwNotificationOption == DMUS_NOTIFICATION_SEGEND)
        {
            if (FAILED(hr = segment_state_end_play((IDirectMusicSegmentState *)notif->punkUser)))
                WARN("Failed to end segment state %p, hr %#lx\n", notif->punkUser, hr);
        }

        if (IsEqualGUID(&notif->guidNotificationType, &GUID_NOTIFICATION_PERFORMANCE))
            enabled = This->notification_performance;
        if (IsEqualGUID(&notif->guidNotificationType, &GUID_NOTIFICATION_SEGMENT))
            enabled = This->notification_segment;
        if (!enabled) return DMUS_S_FREE;

        list_add_tail(&This->notifications, &message->entry);

        /* discard old notification messages */
        do
        {
            previous = LIST_ENTRY(list_head(&This->notifications), struct message, entry);
            if (message->msg.rtTime - previous->msg.rtTime <= This->notification_timeout) break;
            list_remove(&previous->entry);
            list_init(&previous->entry);
        } while (SUCCEEDED(hr = IDirectMusicPerformance_FreePMsg(performance, &previous->msg)));

        SetEvent(This->notification_event);
        return S_OK;
    }

    case DMUS_PMSGT_WAVE:
        if (FAILED(hr = IDirectSoundBuffer_Play((IDirectSoundBuffer *)msg->punkUser, 0, 0, 0)))
            WARN("Failed to play wave buffer, hr %#lx\n", hr);
        break;

    default:
        FIXME("Unhandled message type %#lx\n", msg->dwType);
        break;
    }

    return DMUS_S_FREE;
}

static HRESULT WINAPI performance_tool_Flush(IDirectMusicTool *iface,
        IDirectMusicPerformance *performance, DMUS_PMSG *msg, REFERENCE_TIME time)
{
    struct performance *This = impl_from_IDirectMusicTool(iface);
    FIXME("(%p, %p, %p, %I64d): stub\n", This, performance, msg, time);
    return E_NOTIMPL;
}

static const IDirectMusicToolVtbl performance_tool_vtbl =
{
    performance_tool_QueryInterface,
    performance_tool_AddRef,
    performance_tool_Release,
    performance_tool_Init,
    performance_tool_GetMsgDeliveryType,
    performance_tool_GetMediaTypeArraySize,
    performance_tool_GetMediaTypes,
    performance_tool_ProcessPMsg,
    performance_tool_Flush,
};

/* for ClassFactory */
HRESULT create_dmperformance(REFIID iid, void **ret_iface)
{
    struct performance *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(iid), ret_iface);

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicPerformance8_iface.lpVtbl = &performance_vtbl;
    obj->IDirectMusicGraph_iface.lpVtbl = &performance_graph_vtbl;
    obj->IDirectMusicTool_iface.lpVtbl = &performance_tool_vtbl;
    obj->ref = 1;

    obj->pDefaultPath = NULL;
    InitializeCriticalSection(&obj->safe);
    obj->safe.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": performance->safe");
    wine_rb_init(&obj->pchannels, pchannel_block_compare);

    list_init(&obj->messages);
    list_init(&obj->notifications);

    obj->rtLatencyTime  = 100;  /* 100 ms TO FIX */
    obj->dwBumperLength =   50; /* 50 ms default */
    obj->dwPrepareTime  = 1000; /* 1000 ms default */

    hr = IDirectMusicPerformance8_QueryInterface(&obj->IDirectMusicPerformance8_iface, iid, ret_iface);
    IDirectMusicPerformance_Release(&obj->IDirectMusicPerformance8_iface);
    return hr;
}

static inline struct performance *unsafe_impl_from_IDirectMusicPerformance8(IDirectMusicPerformance8 *iface)
{
    if (iface->lpVtbl != &performance_vtbl) return NULL;
    return CONTAINING_RECORD(iface, struct performance, IDirectMusicPerformance8_iface);
}

HRESULT performance_get_dsound(IDirectMusicPerformance8 *iface, IDirectSound **dsound)
{
    struct performance *This = unsafe_impl_from_IDirectMusicPerformance8(iface);
    if (!This || !(*dsound = This->dsound)) return E_FAIL;
    IDirectSound_AddRef(*dsound);
    return S_OK;
}
