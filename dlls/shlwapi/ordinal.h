/*
 * Internal structures (read "undocumented") used by the
 * ordinal entry points.
 *
 * Determined by experimentation.
 */

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCSTR  ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCSTR  ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_1;

DWORD WINAPI SHLWAPI_1(LPCSTR x, UNKNOWN_SHLWAPI_1 *y);

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCWSTR ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCWSTR ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_2;

DWORD WINAPI SHLWAPI_2(LPCWSTR x, UNKNOWN_SHLWAPI_2 *y);

