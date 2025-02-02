@ stub BluetoothAuthenticateDevice
@ stub BluetoothAuthenticateMultipleDevices
@ stub BluetoothAuthenticationAgent
@ stub BluetoothDisconnectDevice
@ stub BluetoothDisplayDeviceProperties
@ stub BluetoothEnableDiscovery
@ stdcall BluetoothEnableIncomingConnections(ptr long) bthprops.cpl.BluetoothEnableIncomingConnections
@ stub BluetoothEnumerateInstalledServices
@ stub BluetoothFindBrowseGroupClose
@ stub BluetoothFindClassIdClose
@ stdcall -import BluetoothFindDeviceClose(ptr)
@ stub BluetoothFindFirstBrowseGroup
@ stub BluetoothFindFirstClassId
@ stdcall -import BluetoothFindFirstDevice(ptr ptr)
@ stub BluetoothFindFirstProfileDescriptor
@ stub BluetoothFindFirstProtocolDescriptorStack
@ stub BluetoothFindFirstProtocolEntry
@ stdcall -import BluetoothFindFirstRadio(ptr ptr)
@ stub BluetoothFindFirstService
@ stub BluetoothFindNextBrowseGroup
@ stub BluetoothFindNextClassId
@ stdcall -import BluetoothFindNextDevice(ptr ptr)
@ stub BluetoothFindNextProfileDescriptor
@ stub BluetoothFindNextProtocolDescriptorStack
@ stub BluetoothFindNextProtocolEntry
@ stdcall -import BluetoothFindNextRadio(ptr ptr)
@ stub BluetoothFindNextService
@ stub BluetoothFindProfileDescriptorClose
@ stub BluetoothFindProtocolDescriptorStackClose
@ stub BluetoothFindProtocolEntryClose
@ stdcall -import BluetoothFindRadioClose(ptr)
@ stub BluetoothFindServiceClose
@ stub BluetoothGetDeviceInfo
@ stdcall -import BluetoothGetRadioInfo(ptr ptr)
@ stdcall BluetoothIsConnectable(ptr) bthprops.cpl.BluetoothIsConnectable
@ stdcall BluetoothIsDiscoverable(ptr) bthprops.cpl.BluetoothIsDiscoverable
@ stub BluetoothMapClassOfDeviceToImageIndex
@ stub BluetoothMapClassOfDeviceToString
@ stub BluetoothRegisterForAuthentication
@ stub BluetoothRemoveDevice
@ stdcall -import BluetoothSdpEnumAttributes(ptr long ptr ptr)
@ stdcall -import BluetoothSdpGetAttributeValue(ptr long long ptr)
@ stdcall -import BluetoothSdpGetContainerElementData(ptr long ptr ptr)
@ stdcall -import BluetoothSdpGetElementData(ptr long ptr)
@ stub BluetoothSdpGetString
@ stub BluetoothSelectDevices
@ stub BluetoothSelectDevicesFree
@ stub BluetoothSendAuthenticationResponse
@ stub BluetoothSetServiceState
@ stdcall -import BluetoothUnregisterAuthentication(long)
@ stub BluetoothUpdateDeviceRecord
#@ stub CPlApplet
