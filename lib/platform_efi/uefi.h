#pragma once

/********* UEFI *********/

#define FALSE ((BOOLEAN)0)
#define IN
#define OPTIONAL
#define OUT
#define TRUE ((BOOLEAN)1)

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR (1 | 0x8000000000000000)
#define EFI_INVALID_PARAMETER (2 | 0x8000000000000000)
#define EFI_UNSUPPORTED (3 | 0x8000000000000000)
#define EFI_BAD_BUFFER_SIZE (4 | 0x8000000000000000)
#define EFI_BUFFER_TOO_SMALL (5 | 0x8000000000000000)
#define EFI_NOT_READY (6 | 0x8000000000000000)
#define EFI_DEVICE_ERROR (7 | 0x8000000000000000)
#define EFI_WRITE_PROTECTED (8 | 0x8000000000000000)
#define EFI_OUT_OF_RESOURCES (9 | 0x8000000000000000)
#define EFI_VOLUME_CORRUPTED (10 | 0x8000000000000000)
#define EFI_VOLUME_FULL (11 | 0x8000000000000000)
#define EFI_NO_MEDIA (12 | 0x8000000000000000)
#define EFI_MEDIA_CHANGED (13 | 0x8000000000000000)
#define EFI_NOT_FOUND (14 | 0x8000000000000000)
#define EFI_ACCESS_DENIED (15 | 0x8000000000000000)
#define EFI_NO_RESPONSE (16 | 0x8000000000000000)
#define EFI_NO_MAPPING (17 | 0x8000000000000000)
#define EFI_TIMEOUT (18 | 0x8000000000000000)
#define EFI_NOT_STARTED (19 | 0x8000000000000000)
#define EFI_ALREADY_STARTED (20 | 0x8000000000000000)
#define EFI_ABORTED (21 | 0x8000000000000000)
#define EFI_ICMP_ERROR (22 | 0x8000000000000000)
#define EFI_TFTP_ERROR (23 | 0x8000000000000000)
#define EFI_PROTOCOL_ERROR (24 | 0x8000000000000000)
#define EFI_INCOMPATIBLE_VERSION (25 | 0x8000000000000000)
#define EFI_SECURITY_VIOLATION (26 | 0x8000000000000000)
#define EFI_CRC_ERROR (27 | 0x8000000000000000)
#define EFI_END_OF_MEDIA (28 | 0x8000000000000000)
#define EFI_END_OF_FILE (31 | 0x8000000000000000)
#define EFI_INVALID_LANGUAGE (32 | 0x8000000000000000)
#define EFI_COMPROMISED_DATA (33 | 0x8000000000000000)
#define EFI_IP_ADDRESS_CONFLICT (34 | 0x8000000000000000)
#define EFI_HTTP_ERROR (35 | 0x8000000000000000)
#define EFI_NETWORK_UNREACHABLE (100 | 0x8000000000000000)
#define EFI_HOST_UNREACHABLE (101 | 0x8000000000000000)
#define EFI_PROTOCOL_UNREACHABLE (102 | 0x8000000000000000)
#define EFI_PORT_UNREACHABLE (103 | 0x8000000000000000)
#define EFI_CONNECTION_FIN (104 | 0x8000000000000000)
#define EFI_CONNECTION_RESET (105 | 0x8000000000000000)
#define EFI_CONNECTION_REFUSED (106 | 0x8000000000000000)

#define EFI_FILE_SYSTEM_INFO_ID {0x09576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
#define EFI_MP_SERVICES_PROTOCOL_GUID {0x3fdda605, 0xa76e, 0x4f46, {0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0x3d, 0x08}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
#define EFI_TCP4_PROTOCOL_GUID {0x65530BC7, 0xA359, 0x410f, {0xB0, 0x10, 0x5A, 0xAD, 0xC7, 0xEC, 0x2B, 0x62}}
#define EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID {0x00720665, 0x67EB, 0x4a99, {0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9}}
#define EFI_FILE_INFO_ID { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }}

#define EFI_FILE_MODE_READ 0x0000000000000001
#define EFI_FILE_MODE_WRITE 0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000
#define EFI_FILE_READ_ONLY 0x0000000000000001
#define EFI_FILE_HIDDEN 0x0000000000000002
#define EFI_FILE_SYSTEM 0x0000000000000004
#define EFI_FILE_RESERVED 0x0000000000000008
#define EFI_FILE_DIRECTORY 0x0000000000000010
#define EFI_FILE_ARCHIVE 0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037
#define EFI_FILE_PROTOCOL_REVISION 0x00010000
#define EFI_FILE_PROTOCOL_REVISION2 0x00020000
#define EFI_FILE_PROTOCOL_LATEST_REVISION EFI_FILE_PROTOCOL_REVISION2
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER 0x00000010
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001
#define EFI_OPEN_PROTOCOL_EXCLUSIVE 0x00000020
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL 0x00000004
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000
#define EFI_UNSPECIFIED_TIMEZONE 0x07FF
#define END_OF_CPU_LIST 0xFFFFFFFF
#define EVT_NOTIFY_SIGNAL 0x00000200
#define EVT_NOTIFY_WAIT 0x00000100
#define EVT_RUNTIME 0x40000000
#define EVT_SIGNAL_EXIT_BOOT_SERVICES 0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202
#define EVT_TIMER 0x80000000
#define EXCEPT_X64_DIVIDE_ERROR 0
#define MAX_MCAST_FILTER_CNT 16
#define PROCESSOR_AS_BSP_BIT 0x00000001
#define PROCESSOR_ENABLED_BIT 0x00000002
#define PROCESSOR_HEALTH_STATUS_BIT 0x00000004
#define TPL_APPLICATION 4
#define TPL_CALLBACK 8
#define TPL_HIGH_LEVEL 31
#define TPL_NOTIFY 16

typedef unsigned char BOOLEAN;
typedef unsigned short CHAR16;
typedef void* EFI_EVENT;
typedef void* EFI_HANDLE;
typedef unsigned long long EFI_PHYSICAL_ADDRESS;
typedef unsigned long long EFI_STATUS;
typedef unsigned long long EFI_TPL;
typedef unsigned long long EFI_VIRTUAL_ADDRESS;

typedef enum
{
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum
{
	EFI_NATIVE_INTERFACE
} EFI_INTERFACE_TYPE;

typedef enum
{
	AllHandles,
	ByRegisterNotify,
	ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef enum
{
	EfiReservedMemoryType,
	EfiLoaderCode,
	EfiLoaderData,
	EfiBootServicesCode,
	EfiBootServicesData,
	EfiRuntimeServicesCode,
	EfiRuntimeServicesData,
	EfiConventionalMemory,
	EfiUnusableMemory,
	EfiACPIReclaimMemory,
	EfiACPIMemoryNVS,
	EfiMemoryMappedIO,
	EfiMemoryMappedIOPortSpace,
	EfiPalCode,
	EfiPersistentMemory,
	EfiUnacceptedMemoryType,
	EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum
{
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown,
	EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef enum
{
	Tcp4StateClosed = 0,
	Tcp4StateListen = 1,
	Tcp4StateSynSent = 2,
	Tcp4StateSynReceived = 3,
	Tcp4StateEstablished = 4,
	Tcp4StateFinWait1 = 5,
	Tcp4StateFinWait2 = 6,
	Tcp4StateClosing = 7,
	Tcp4StateTimeWait = 8,
	Tcp4StateCloseWait = 9,
	Tcp4StateLastAck = 10
} EFI_TCP4_CONNECTION_STATE;

typedef enum
{
	TimerCancel,
	TimerPeriodic,
	TimerRelative
} EFI_TIMER_DELAY;

typedef struct
{
	unsigned int Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} EFI_GUID;

typedef struct
{
	EFI_GUID CapsuleGuid;
	unsigned int HeaderSize;
	unsigned int Flags;
	unsigned int CapsuleImageSize;
} EFI_CAPSULE_HEADER;

typedef struct
{
	unsigned int Package;
	unsigned int Core;
	unsigned int Thread;
} EFI_CPU_PHYSICAL_LOCATION;

typedef struct
{
	unsigned char Type;
	unsigned char SubType;
	unsigned char Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

typedef struct
{
	EFI_EVENT Event;
	EFI_STATUS Status;
	unsigned long long BufferSize;
	void* Buffer;
} EFI_FILE_IO_TOKEN;

typedef struct
{
	unsigned long long Size;
	BOOLEAN ReadOnly;
	unsigned long long VolumeSize;
	unsigned long long FreeSpace;
	unsigned int BlockSize;
	CHAR16 VolumeLabel[256];
} EFI_FILE_SYSTEM_INFO;

typedef struct
{
	unsigned short ScanCode;
	CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct
{
	unsigned char Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct
{
	unsigned char DefaultProtocol;
	BOOLEAN AcceptAnyProtocol;
	BOOLEAN AcceptIcmpErrors;
	BOOLEAN AcceptBroadcast;
	BOOLEAN AcceptPromiscuous;
	BOOLEAN UseDefaultAddress;
	EFI_IPv4_ADDRESS StationAddress;
	EFI_IPv4_ADDRESS SubnetMask;
	unsigned char TypeOfService;
	unsigned char TimeToLive;
	BOOLEAN DoNotFragment;
	BOOLEAN RawData;
	unsigned int ReceiveTimeout;
	unsigned int TransmitTimeout;
} EFI_IP4_CONFIG_DATA;

typedef struct
{
	unsigned char Type;
	unsigned char Code;
} EFI_IP4_ICMP_TYPE;

typedef struct
{
	EFI_IPv4_ADDRESS SubnetAddress;
	EFI_IPv4_ADDRESS SubnetMask;
	EFI_IPv4_ADDRESS GatewayAddress;
} EFI_IP4_ROUTE_TABLE;

typedef struct
{
	BOOLEAN IsStarted;
	unsigned int MaxPacketSize;
	EFI_IP4_CONFIG_DATA ConfigData;
	BOOLEAN IsConfigured;
	unsigned int GroupCount;
	EFI_IPv4_ADDRESS* GroupTable;
	unsigned int RouteCount;
	EFI_IP4_ROUTE_TABLE* RouteTable;
	unsigned int IcmpTypeCount;
	EFI_IP4_ICMP_TYPE* IcmpTypeList;
} EFI_IP4_MODE_DATA;

typedef struct
{
	unsigned char Addr[32];
} EFI_MAC_ADDRESS;

typedef struct
{
	unsigned int ReceivedQueueTimeoutValue;
	unsigned int TransmitQueueTimeoutValue;
	unsigned short ProtocolTypeFilter;
	BOOLEAN EnableUnicastReceive;
	BOOLEAN EnableMulticastReceive;
	BOOLEAN EnableBroadcastReceive;
	BOOLEAN EnablePromiscuousReceive;
	BOOLEAN FlushQueuesOnReset;
	BOOLEAN EnableReceiveTimestamps;
	BOOLEAN DisableBackgroundPolling;
} EFI_MANAGED_NETWORK_CONFIG_DATA;

typedef struct
{
	unsigned int Type;
	EFI_PHYSICAL_ADDRESS PhysicalStart;
	EFI_VIRTUAL_ADDRESS VirtualStart;
	unsigned long long NumberOfPages;
	unsigned long long Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct
{
	EFI_HANDLE AgentHandle;
	EFI_HANDLE ControllerHandle;
	unsigned int Attributes;
	unsigned int OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef struct
{
	unsigned long long ProcessorId;
	unsigned int StatusFlag;
	EFI_CPU_PHYSICAL_LOCATION Location;
} EFI_PROCESSOR_INFORMATION;

typedef struct
{
	unsigned int State;
	unsigned int HwAddressSize;
	unsigned int MediaHeaderSize;
	unsigned int MaxPacketSize;
	unsigned int NvRamSize;
	unsigned int NvRamAccessSize;
	unsigned int ReceiveFilterMask;
	unsigned int ReceiveFilterSetting;
	unsigned int MaxMCastFilterCount;
	unsigned int MCastFilterCount;
	EFI_MAC_ADDRESS MCastFilter[MAX_MCAST_FILTER_CNT];
	EFI_MAC_ADDRESS CurrentAddress;
	EFI_MAC_ADDRESS BroadcastAddress;
	EFI_MAC_ADDRESS PermanentAddress;
	unsigned char IfType;
	BOOLEAN MacAddressChangeable;
	BOOLEAN MultipleTxSupported;
	BOOLEAN MediaPresentSupported;
	BOOLEAN MediaPresent;
} EFI_SIMPLE_NETWORK_MODE;

typedef struct
{
	unsigned long long Signature;
	unsigned int Revision;
	unsigned int HeaderSize;
	unsigned int CRC32;
	unsigned int Reserved;
} EFI_TABLE_HEADER;

typedef struct
{
	BOOLEAN UseDefaultAddress;
	EFI_IPv4_ADDRESS StationAddress;
	EFI_IPv4_ADDRESS SubnetMask;
	unsigned short StationPort;
	EFI_IPv4_ADDRESS RemoteAddress;
	unsigned short RemotePort;
	BOOLEAN ActiveFlag;
} EFI_TCP4_ACCESS_POINT;

typedef struct
{
	EFI_EVENT Event;
	EFI_STATUS Status;
} EFI_TCP4_COMPLETION_TOKEN;

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	BOOLEAN AbortOnClose;
} EFI_TCP4_CLOSE_TOKEN;

typedef struct
{
	unsigned int ReceiveBufferSize;
	unsigned int SendBufferSize;
	unsigned int MaxSynBackLog;
	unsigned int ConnectionTimeout;
	unsigned int DataRetries;
	unsigned int FinTimeout;
	unsigned int TimeWaitTimeout;
	unsigned int KeepAliveProbes;
	unsigned int KeepAliveTime;
	unsigned int KeepAliveInterval;
	BOOLEAN EnableNagle;
	BOOLEAN EnableTimeStamp;
	BOOLEAN EnableWindowScaling;
	BOOLEAN EnableSelectiveAck;
	BOOLEAN EnablePathMtuDiscovery;
} EFI_TCP4_OPTION;

typedef struct
{
	unsigned char TypeOfService;
	unsigned char TimeToLive;
	EFI_TCP4_ACCESS_POINT AccessPoint;
	EFI_TCP4_OPTION* ControlOption;
} EFI_TCP4_CONFIG_DATA;

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
} EFI_TCP4_CONNECTION_TOKEN;

typedef struct
{
	unsigned int FragmentLength;
	void* FragmentBuffer;
} EFI_TCP4_FRAGMENT_DATA;

typedef struct
{
	BOOLEAN UrgentFlag;
	unsigned int DataLength;
	unsigned int FragmentCount;
	EFI_TCP4_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP4_RECEIVE_DATA;

typedef struct
{
	BOOLEAN Push;
	BOOLEAN Urgent;
	unsigned int DataLength;
	unsigned int FragmentCount;
	EFI_TCP4_FRAGMENT_DATA FragmentTable[1];
} EFI_TCP4_TRANSMIT_DATA;

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	union
	{
		EFI_TCP4_RECEIVE_DATA* RxData;
		EFI_TCP4_TRANSMIT_DATA* TxData;
	} Packet;
} EFI_TCP4_IO_TOKEN;

typedef struct
{
	EFI_TCP4_COMPLETION_TOKEN CompletionToken;
	EFI_HANDLE NewChildHandle;
} EFI_TCP4_LISTEN_TOKEN;

typedef struct
{
	unsigned short Year;
	unsigned char Month;
	unsigned char Day;
	unsigned char Hour;
	unsigned char Minute;
	unsigned char Second;
	unsigned char Pad1;
	unsigned int Nanosecond;
	short TimeZone;
	unsigned char Daylight;
	unsigned char Pad2;
} EFI_TIME;

typedef struct
{
	unsigned int Resolution;
	unsigned int Accuracy;
	BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;

typedef struct
{
	int MaxMode;
	int Mode;
	int Attribute;
	int CursorColumn;
	int CursorRow;
	BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef EFI_STATUS(__cdecl* EFI_ALLOCATE_PAGES) (IN EFI_ALLOCATE_TYPE Type, IN EFI_MEMORY_TYPE MemoryType, IN unsigned long long Pages, IN OUT EFI_PHYSICAL_ADDRESS* Memory);
typedef EFI_STATUS(__cdecl* EFI_ALLOCATE_POOL) (IN EFI_MEMORY_TYPE PoolType, IN unsigned long long Size, OUT void** Buffer);
typedef void(__cdecl* EFI_AP_PROCEDURE) (IN void* ProcedureArgument);
typedef EFI_STATUS(__cdecl* EFI_CALCULATE_CRC32) (IN void* Data, IN unsigned long long DataSize, OUT unsigned int* Crc32);
typedef EFI_STATUS(__cdecl* EFI_CHECK_EVENT) (IN EFI_EVENT Event);
typedef EFI_STATUS(__cdecl* EFI_CLOSE_EVENT) (IN EFI_EVENT Event);
typedef EFI_STATUS(__cdecl* EFI_CLOSE_PROTOCOL) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, IN EFI_HANDLE AgentHandle, IN EFI_HANDLE ControllerHandle);
typedef EFI_STATUS(__cdecl* EFI_CONNECT_CONTROLLER) (IN EFI_HANDLE ControllerHandle, IN EFI_HANDLE* DriverImageHandle OPTIONAL, IN EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath OPTIONAL, IN BOOLEAN Recursive);
typedef EFI_STATUS(__cdecl* EFI_CONVERT_POINTER) (IN unsigned long long DebugDisposition, IN OUT void** Address);
typedef void(__cdecl* EFI_COPY_MEM) (IN void* Destination, IN void* Source, IN unsigned long long Length);
typedef EFI_STATUS(__cdecl* EFI_CREATE_EVENT) (IN unsigned int Type, IN EFI_TPL NotifyTpl, IN void* NotifyFunction, OPTIONAL IN void* NotifyContext, OPTIONAL OUT EFI_EVENT* Event);
typedef EFI_STATUS(__cdecl* EFI_CREATE_EVENT_EX) (IN unsigned int Type, IN EFI_TPL NotifyTpl, IN void* NotifyFunction OPTIONAL, IN const void* NotifyContext OPTIONAL, IN const EFI_GUID* EventGroup OPTIONAL, OUT EFI_EVENT* Event);
typedef EFI_STATUS(__cdecl* EFI_DISCONNECT_CONTROLLER) (IN EFI_HANDLE ControllerHandle, IN EFI_HANDLE DriverImageHandle OPTIONAL, IN EFI_HANDLE ChildHandle OPTIONAL);
typedef void(__cdecl* EFI_EVENT_NOTIFY) (IN EFI_EVENT Event, IN void* Context);
typedef EFI_STATUS(__cdecl* EFI_EXIT) (IN EFI_HANDLE ImageHandle, IN EFI_STATUS ExitStatus, IN unsigned long long ExitDataSize, IN CHAR16* ExitData OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_EXIT_BOOT_SERVICES) (IN EFI_HANDLE ImageHandle, IN unsigned long long MapKey);
typedef EFI_STATUS(__cdecl* EFI_FILE_CLOSE) (IN void* This);
typedef EFI_STATUS(__cdecl* EFI_FILE_DELETE) (IN void* This);
typedef EFI_STATUS(__cdecl* EFI_FILE_FLUSH) (IN void* This);
typedef EFI_STATUS(__cdecl* EFI_FILE_FLUSH_EX) (IN void* This, IN OUT EFI_FILE_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_FILE_GET_INFO) (IN void* This, IN EFI_GUID* InformationType, IN OUT unsigned long long* BufferSize, OUT void* Buffer);
typedef EFI_STATUS(__cdecl* EFI_FILE_GET_POSITION) (IN void* This, OUT unsigned long long* Position);
typedef EFI_STATUS(__cdecl* EFI_FILE_OPEN) (IN void* This, OUT void** NewHandle, IN CHAR16* FileName, IN unsigned long long OpenMode, IN unsigned long long Attributes);
typedef EFI_STATUS(__cdecl* EFI_FILE_OPEN_EX) (IN void* This, OUT void** NewHandle, IN CHAR16* FileName, IN unsigned long long OpenMode, IN unsigned long long Attributes, IN OUT EFI_FILE_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_FILE_READ) (IN void* This, IN OUT unsigned long long* BufferSize, OUT void* Buffer);
typedef EFI_STATUS(__cdecl* EFI_FILE_READ_EX) (IN void* This, IN OUT EFI_FILE_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_FILE_SET_INFO) (IN void* This, IN EFI_GUID* InformationType, IN unsigned long long BufferSize, IN void* Buffer);
typedef EFI_STATUS(__cdecl* EFI_FILE_SET_POSITION) (IN void* This, IN unsigned long long Position);
typedef EFI_STATUS(__cdecl* EFI_FILE_WRITE) (IN void* This, IN OUT unsigned long long* BufferSize, IN void* Buffer);
typedef EFI_STATUS(__cdecl* EFI_FILE_WRITE_EX) (IN void* This, IN OUT EFI_FILE_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_FREE_PAGES) (IN EFI_PHYSICAL_ADDRESS Memory, IN unsigned long long Pages);
typedef EFI_STATUS(__cdecl* EFI_FREE_POOL) (IN void* Buffer);
typedef EFI_STATUS(__cdecl* EFI_GET_MEMORY_MAP) (IN OUT unsigned long long* MemoryMapSize, OUT EFI_MEMORY_DESCRIPTOR* MemoryMap, OUT unsigned long long* MapKey, OUT unsigned long long* DescriptorSize, OUT unsigned int* DescriptorVersion);
typedef EFI_STATUS(__cdecl* EFI_GET_NEXT_HIGH_MONO_COUNT) (OUT unsigned int* HighCount);
typedef EFI_STATUS(__cdecl* EFI_GET_NEXT_MONOTONIC_COUNT) (OUT unsigned long long* Count);
typedef EFI_STATUS(__cdecl* EFI_GET_NEXT_VARIABLE_NAME) (IN OUT unsigned long long* VariableNameSize, IN OUT CHAR16* VariableName, IN OUT EFI_GUID* VendorGuid);
typedef EFI_STATUS(__cdecl* EFI_GET_TIME) (OUT EFI_TIME* Time, OUT EFI_TIME_CAPABILITIES* Capabilities OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_GET_VARIABLE) (IN CHAR16* VariableName, IN EFI_GUID* VendorGuid, OUT unsigned int* Attributes OPTIONAL, IN OUT unsigned long long* DataSize, OUT void* Data);
typedef EFI_STATUS(__cdecl* EFI_GET_WAKEUP_TIME) (OUT BOOLEAN* Enabled, OUT BOOLEAN* Pending, OUT EFI_TIME* Time);
typedef EFI_STATUS(__cdecl* EFI_HANDLE_PROTOCOL) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, OUT void** Interface);
typedef EFI_STATUS(__cdecl* EFI_IMAGE_LOAD) (IN BOOLEAN BootPolicy, IN EFI_HANDLE ParentImageHandle, IN EFI_DEVICE_PATH_PROTOCOL* DevicePath, IN void* SourceBuffer OPTIONAL, IN unsigned long long SourceSize, OUT EFI_HANDLE* ImageHandle);
typedef EFI_STATUS(__cdecl* EFI_IMAGE_START) (IN EFI_HANDLE ImageHandle, OUT unsigned long long* ExitDataSize, OUT CHAR16** ExitData OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_IMAGE_UNLOAD) (IN EFI_HANDLE ImageHandle);
typedef EFI_STATUS(__cdecl* EFI_INPUT_READ_KEY) (IN void* This, OUT EFI_INPUT_KEY* Key);
typedef EFI_STATUS(__cdecl* EFI_INPUT_RESET) (IN void* This, IN BOOLEAN ExtendedVerification);
typedef EFI_STATUS(__cdecl* EFI_INSTALL_CONFIGURATION_TABLE) (IN EFI_GUID* Guid, IN void* Table);
typedef EFI_STATUS(__cdecl* EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES) (IN OUT EFI_HANDLE* Handle, ...);
typedef EFI_STATUS(__cdecl* EFI_INSTALL_PROTOCOL_INTERFACE) (IN OUT EFI_HANDLE* Handle, IN EFI_GUID* Protocol, IN EFI_INTERFACE_TYPE InterfaceType, IN void* Interface);
typedef EFI_STATUS(__cdecl* EFI_LOCATE_DEVICE_PATH) (IN EFI_GUID* Protocol, IN OUT EFI_DEVICE_PATH_PROTOCOL** DevicePath, OUT EFI_HANDLE* Device);
typedef EFI_STATUS(__cdecl* EFI_LOCATE_HANDLE) (IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID* Protocol OPTIONAL, IN void* SearchKey OPTIONAL, IN OUT unsigned long long* BufferSize, OUT EFI_HANDLE* Buffer);
typedef EFI_STATUS(__cdecl* EFI_LOCATE_HANDLE_BUFFER) (IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID* Protocol OPTIONAL, IN void* SearchKey OPTIONAL, OUT unsigned long long* NoHandles, OUT EFI_HANDLE** Buffer);
typedef EFI_STATUS(__cdecl* EFI_LOCATE_PROTOCOL) (IN EFI_GUID* Protocol, IN void* Registration OPTIONAL, OUT void** Interface);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_ENABLEDISABLEAP) (IN void* This, IN unsigned long long ProcessorNumber, IN BOOLEAN EnableAP, IN unsigned int* HealthFlag OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS) (IN void* This, OUT unsigned long long* NumberOfProcessors, OUT unsigned long long* NumberOfEnabledProcessors);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_GET_PROCESSOR_INFO) (IN void* This, IN unsigned long long ProcessorNumber, OUT EFI_PROCESSOR_INFORMATION* ProcessorInfoBuffer);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_STARTUP_ALL_APS) (IN void* This, IN EFI_AP_PROCEDURE Procedure, IN BOOLEAN SingleThread, IN EFI_EVENT WaitEvent OPTIONAL, IN unsigned long long TimeoutInMicroSeconds, IN void* ProcedureArgument OPTIONAL, OUT unsigned long long** FailedCpuList OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_STARTUP_THIS_AP) (IN void* This, IN EFI_AP_PROCEDURE Procedure, IN unsigned long long ProcessorNumber, IN EFI_EVENT WaitEvent OPTIONAL, IN unsigned long long TimeoutInMicroseconds, IN void* ProcedureArgument OPTIONAL, OUT BOOLEAN* Finished OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_SWITCH_BSP) (IN void* This, IN unsigned long long ProcessorNumber, IN BOOLEAN EnableOldBSP);
typedef EFI_STATUS(__cdecl* EFI_MP_SERVICES_WHOAMI) (IN void* This, OUT unsigned long long* ProcessorNumber);
typedef EFI_STATUS(__cdecl* EFI_OPEN_PROTOCOL) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, OUT void** Interface OPTIONAL, IN EFI_HANDLE AgentHandle, IN EFI_HANDLE ControllerHandle, IN unsigned int Attributes);
typedef EFI_STATUS(__cdecl* EFI_OPEN_PROTOCOL_INFORMATION) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY** EntryBuffer, OUT unsigned long long* EntryCount);
typedef EFI_STATUS(__cdecl* EFI_PROTOCOLS_PER_HANDLE) (IN EFI_HANDLE Handle, OUT EFI_GUID*** ProtocolBuffer, OUT unsigned long long* ProtocolBufferCount);
typedef EFI_STATUS(__cdecl* EFI_QUERY_CAPSULE_CAPABILITIES) (IN EFI_CAPSULE_HEADER** CapsuleHeaderArray, IN unsigned long long CapsuleCount, OUT unsigned long long* MaximumCapsuleSize, OUT EFI_RESET_TYPE* ResetType);
typedef EFI_STATUS(__cdecl* EFI_QUERY_VARIABLE_INFO) (IN unsigned int Attributes, OUT unsigned long long* MaximumVariableStorageSize, OUT unsigned long long* RemainingVariableStorageSize, OUT unsigned long long* MaximumVariableSize);
typedef EFI_TPL(__cdecl* EFI_RAISE_TPL) (IN EFI_TPL NewTpl);
typedef EFI_STATUS(__cdecl* EFI_REGISTER_PROTOCOL_NOTIFY) (IN EFI_GUID* Protocol, IN EFI_EVENT Event, OUT void** Registration);
typedef EFI_STATUS(__cdecl* EFI_REINSTALL_PROTOCOL_INTERFACE) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, IN void* OldInterface, IN void* NewInterface);
typedef EFI_STATUS(__cdecl* EFI_RESET_SYSTEM) (IN EFI_RESET_TYPE ResetType, IN EFI_STATUS ResetStatus, IN unsigned long long DataSize, IN CHAR16* ResetData OPTIONAL);
typedef void(__cdecl* EFI_RESTORE_TPL) (IN EFI_TPL OldTpl);
typedef EFI_STATUS(__cdecl* EFI_SERVICE_BINDING_CREATE_CHILD) (IN void* This, IN OUT EFI_HANDLE* ChildHandle);
typedef EFI_STATUS(__cdecl* EFI_SERVICE_BINDING_DESTROY_CHILD) (IN void* This, IN EFI_HANDLE ChildHandle);
typedef void(__cdecl* EFI_SET_MEM) (IN void* Buffer, IN unsigned long long Size, IN unsigned char Value);
typedef EFI_STATUS(__cdecl* EFI_SET_TIME) (IN EFI_TIME* Time);
typedef EFI_STATUS(__cdecl* EFI_SET_TIMER) (IN EFI_EVENT Event, IN EFI_TIMER_DELAY Type, IN unsigned long long TriggerTime);
typedef EFI_STATUS(__cdecl* EFI_SET_VARIABLE) (IN CHAR16* VariableName, IN EFI_GUID* VendorGuid, IN unsigned int Attributes, IN unsigned long long DataSize, IN void* Data);
typedef EFI_STATUS(__cdecl* EFI_SET_VIRTUAL_ADDRESS_MAP) (IN unsigned long long MemoryMapSize, IN unsigned long long DescriptorSize, IN unsigned int DescriptorVersion, IN EFI_MEMORY_DESCRIPTOR* VirtualMap);
typedef EFI_STATUS(__cdecl* EFI_SET_WAKEUP_TIME) (IN BOOLEAN Enable, IN EFI_TIME* Time OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_SET_WATCHDOG_TIMER) (IN unsigned long long Timeout, IN unsigned long long WatchdogCode, IN unsigned long long DataSize, IN CHAR16* WatchdogData OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_SIGNAL_EVENT) (IN EFI_EVENT Event);
typedef EFI_STATUS(__cdecl* EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME) (IN void* This, OUT void** Root);
typedef EFI_STATUS(__cdecl* EFI_STALL) (IN unsigned long long Microseconds);
typedef EFI_STATUS(__cdecl* EFI_TCP4_ACCEPT) (IN void* This, IN EFI_TCP4_LISTEN_TOKEN* ListenToken);
typedef EFI_STATUS(__cdecl* EFI_TCP4_CANCEL)(IN void* This, IN EFI_TCP4_COMPLETION_TOKEN* Token OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_TCP4_CLOSE)(IN void* This, IN EFI_TCP4_CLOSE_TOKEN* CloseToken);
typedef EFI_STATUS(__cdecl* EFI_TCP4_CONFIGURE) (IN void* This, IN EFI_TCP4_CONFIG_DATA* TcpConfigData OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_TCP4_CONNECT) (IN void* This, IN EFI_TCP4_CONNECTION_TOKEN* ConnectionToken);
typedef EFI_STATUS(__cdecl* EFI_TCP4_GET_MODE_DATA) (IN void* This, OUT EFI_TCP4_CONNECTION_STATE* Tcp4State OPTIONAL, OUT EFI_TCP4_CONFIG_DATA* Tcp4ConfigData OPTIONAL, OUT EFI_IP4_MODE_DATA* Ip4ModeData OPTIONAL, OUT EFI_MANAGED_NETWORK_CONFIG_DATA* MnpConfigData OPTIONAL, OUT EFI_SIMPLE_NETWORK_MODE* SnpModeData OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_TCP4_POLL) (IN void* This);
typedef EFI_STATUS(__cdecl* EFI_TCP4_RECEIVE) (IN void* This, IN EFI_TCP4_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_TCP4_ROUTES) (IN void* This, IN BOOLEAN DeleteRoute, IN EFI_IPv4_ADDRESS* SubnetAddress, IN EFI_IPv4_ADDRESS* SubnetMask, IN EFI_IPv4_ADDRESS* GatewayAddress);
typedef EFI_STATUS(__cdecl* EFI_TCP4_TRANSMIT) (IN void* This, IN EFI_TCP4_IO_TOKEN* Token);
typedef EFI_STATUS(__cdecl* EFI_TEXT_CLEAR_SCREEN) (IN void* This);
typedef EFI_STATUS(__cdecl* EFI_TEXT_ENABLE_CURSOR) (IN void* This, IN BOOLEAN Visible);
typedef EFI_STATUS(__cdecl* EFI_TEXT_QUERY_MODE) (IN void* This, IN unsigned long long ModeNumber, OUT unsigned long long* Columns, OUT unsigned long long* Rows);
typedef EFI_STATUS(__cdecl* EFI_TEXT_RESET) (IN void* This, IN BOOLEAN ExtendedVerification);
typedef EFI_STATUS(__cdecl* EFI_TEXT_SET_ATTRIBUTE) (IN void* This, IN unsigned long long Attribute);
typedef EFI_STATUS(__cdecl* EFI_TEXT_SET_CURSOR_POSITION) (IN void* This, IN unsigned long long Column, IN unsigned long long Row);
typedef EFI_STATUS(__cdecl* EFI_TEXT_SET_MODE) (IN void* This, IN unsigned long long ModeNumber);
typedef EFI_STATUS(__cdecl* EFI_TEXT_STRING) (IN void* This, IN CHAR16* String);
typedef EFI_STATUS(__cdecl* EFI_TEXT_TEST_STRING) (IN void* This, IN CHAR16* String);
typedef EFI_STATUS(__cdecl* EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES) (IN EFI_HANDLE Handle, ...);
typedef EFI_STATUS(__cdecl* EFI_UNINSTALL_PROTOCOL_INTERFACE) (IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, IN void* Interface);
typedef EFI_STATUS(__cdecl* EFI_UPDATE_CAPSULE) (IN EFI_CAPSULE_HEADER** CapsuleHeaderArray, IN unsigned long long CapsuleCount, IN EFI_PHYSICAL_ADDRESS ScatterGatherList OPTIONAL);
typedef EFI_STATUS(__cdecl* EFI_WAIT_FOR_EVENT) (IN unsigned long long NumberOfEvents, IN EFI_EVENT* Event, OUT unsigned long long* Index);

typedef struct
{
	EFI_TABLE_HEADER Hdr;
	EFI_RAISE_TPL RaiseTPL;
	EFI_RESTORE_TPL RestoreTPL;
	EFI_ALLOCATE_PAGES AllocatePages;
	EFI_FREE_PAGES FreePages;
	EFI_GET_MEMORY_MAP GetMemoryMap;
	EFI_ALLOCATE_POOL AllocatePool;
	EFI_FREE_POOL FreePool;
	EFI_CREATE_EVENT CreateEvent;
	EFI_SET_TIMER SetTimer;
	EFI_WAIT_FOR_EVENT WaitForEvent;
	EFI_SIGNAL_EVENT SignalEvent;
	EFI_CLOSE_EVENT CloseEvent;
	EFI_CHECK_EVENT CheckEvent;
	EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface;
	EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface;
	EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface;
	EFI_HANDLE_PROTOCOL HandleProtocol;
	void* Reserved;
	EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
	EFI_LOCATE_HANDLE LocateHandle;
	EFI_LOCATE_DEVICE_PATH LocateDevicePath;
	EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;
	EFI_IMAGE_LOAD LoadImage;
	EFI_IMAGE_START StartImage;
	EFI_EXIT Exit;
	EFI_IMAGE_UNLOAD UnloadImage;
	EFI_EXIT_BOOT_SERVICES ExitBootServices;
	EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount;
	EFI_STALL Stall;
	EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;
	EFI_CONNECT_CONTROLLER ConnectController;
	EFI_DISCONNECT_CONTROLLER DisconnectController;
	EFI_OPEN_PROTOCOL OpenProtocol;
	EFI_CLOSE_PROTOCOL CloseProtocol;
	EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation;
	EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle;
	EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
	EFI_LOCATE_PROTOCOL LocateProtocol;
	EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces;
	EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces;
	EFI_CALCULATE_CRC32 CalculateCrc32;
	EFI_COPY_MEM CopyMem;
	EFI_SET_MEM SetMem;
	EFI_CREATE_EVENT_EX CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct
{
	EFI_GUID VendorGuid;
	void* VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct
{
	unsigned long long Revision;
	EFI_FILE_OPEN Open;
	EFI_FILE_CLOSE Close;
	EFI_FILE_DELETE Delete;
	EFI_FILE_READ Read;
	EFI_FILE_WRITE Write;
	EFI_FILE_GET_POSITION GetPosition;
	EFI_FILE_SET_POSITION SetPosition;
	EFI_FILE_GET_INFO GetInfo;
	EFI_FILE_SET_INFO SetInfo;
	EFI_FILE_FLUSH Flush;
	EFI_FILE_OPEN_EX OpenEx;
	EFI_FILE_READ_EX ReadEx;
	EFI_FILE_WRITE_EX WriteEx;
	EFI_FILE_FLUSH_EX FlushEx;
} EFI_FILE_PROTOCOL;

/*
Template for development
typedef struct {
	unsigned long long             Size;
	unsigned long long             FileSize;
	unsigned long long             PhysicalSize;
	EFI_TIME                       CreateTime;
	EFI_TIME                       LastAccessTime;
	EFI_TIME                       ModificationTime;
	unsigned long long             Attribute;
	CHAR16                         FileName[];
} EFI_FILE_INFO;
*/
typedef struct
{
	EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS GetNumberOfProcessors;
	EFI_MP_SERVICES_GET_PROCESSOR_INFO GetProcessorInfo;
	EFI_MP_SERVICES_STARTUP_ALL_APS StartupAllAPs;
	EFI_MP_SERVICES_STARTUP_THIS_AP StartupThisAP;
	EFI_MP_SERVICES_SWITCH_BSP SwitchBSP;
	EFI_MP_SERVICES_ENABLEDISABLEAP EnableDisableAP;
	EFI_MP_SERVICES_WHOAMI WhoAmI;
} EFI_MP_SERVICES_PROTOCOL;

typedef struct
{
	EFI_TABLE_HEADER Hdr;
	EFI_GET_TIME GetTime;
	EFI_SET_TIME SetTime;
	EFI_GET_WAKEUP_TIME GetWakeupTime;
	EFI_SET_WAKEUP_TIME SetWakeupTime;
	EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
	EFI_CONVERT_POINTER ConvertPointer;
	EFI_GET_VARIABLE GetVariable;
	EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
	EFI_SET_VARIABLE SetVariable;
	EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
	EFI_RESET_SYSTEM ResetSystem;
	EFI_UPDATE_CAPSULE UpdateCapsule;
	EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;
	EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct
{
	EFI_SERVICE_BINDING_CREATE_CHILD CreateChild;
	EFI_SERVICE_BINDING_DESTROY_CHILD DestroyChild;
} EFI_SERVICE_BINDING_PROTOCOL;

typedef struct
{
	unsigned long long Revision;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct
{
	EFI_INPUT_RESET Reset;
	EFI_INPUT_READ_KEY ReadKeyStroke;
	EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct
{
	EFI_TEXT_RESET Reset;
	EFI_TEXT_STRING OutputString;
	EFI_TEXT_TEST_STRING TestString;
	EFI_TEXT_QUERY_MODE QueryMode;
	EFI_TEXT_SET_MODE SetMode;
	EFI_TEXT_SET_ATTRIBUTE SetAttribute;
	EFI_TEXT_CLEAR_SCREEN ClearScreen;
	EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
	EFI_TEXT_ENABLE_CURSOR EnableCursor;
	SIMPLE_TEXT_OUTPUT_MODE* Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct
{
	EFI_TABLE_HEADER Hdr;
	CHAR16* FirmwareVendor;
	unsigned int FirmwareRevision;
	EFI_HANDLE ConsoleInHandle;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL* ConIn;
	EFI_HANDLE ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
	EFI_HANDLE StandardErrorHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
	EFI_RUNTIME_SERVICES* RuntimeServices;
	EFI_BOOT_SERVICES* BootServices;
	unsigned long long NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE* ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef struct
{
	EFI_TCP4_GET_MODE_DATA GetModeData;
	EFI_TCP4_CONFIGURE Configure;
	EFI_TCP4_ROUTES Routes;
	EFI_TCP4_CONNECT Connect;
	EFI_TCP4_ACCEPT Accept;
	EFI_TCP4_TRANSMIT Transmit;
	EFI_TCP4_RECEIVE Receive;
	EFI_TCP4_CLOSE Close;
	EFI_TCP4_CANCEL Cancel;
	EFI_TCP4_POLL Poll;
} EFI_TCP4_PROTOCOL;


