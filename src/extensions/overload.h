#pragma once

////////////////// Extensions \\\\\\\\\\\\

#if defined(_WIN32)
#include <Windows.h>
#include <conio.h>
#elif defined(__linux__)
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#define CreateEvent CreateEvent

void __writecr4_1(unsigned int) {

}

unsigned int __readcr4_1() {
    return 0;
}

unsigned long long _xsetbv_1(unsigned int, unsigned long long) {
    return 0;
}

#define __writecr4 __writecr4_1
#define __readcr4 __readcr4_1
#define _xsetbv _xsetbv_1

uint32_t getCurrentCpuIndex() {
#if defined(_WIN32)
    return GetCurrentProcessorNumber();
#elif defined(__linux__)
    return static_cast<uint32_t>(sched_getcpu());
#else
    return 0; // not supported
#endif
}

#ifndef _MSC_VER

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

void setNonBlockingInput(bool enable) {
    static termios oldt;
    termios newt;

    if (enable) {
        // Save old settings
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        // Disable canonical mode and echo
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // Set stdin non-blocking
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    } else {
        // Restore old settings
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, 0);
    }
}

std::vector<unsigned char> readInput() {
    std::vector<unsigned char> buffer;
    unsigned char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        buffer.push_back(c);
    }
    return buffer;
}
#endif

void updateTime() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::gmtime(&t);
    utcTime.Year = tm->tm_year + 1900;
    utcTime.Month = tm->tm_mon + 1;
    utcTime.Day = tm->tm_mday;
    utcTime.Hour = tm->tm_hour;
    utcTime.Minute = tm->tm_min;
    utcTime.Second = tm->tm_sec;
    utcTime.Nanosecond = 0;
    utcTime.TimeZone = 0;
    utcTime.Daylight = 0;
}

unsigned long long now_ms()
{
    return ms((unsigned char)utcTime.Year % 100, utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second, utcTime.Nanosecond / 1000000);
}

void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    memset(buffer, value, size);
}

void copyMem(void* destination, const void* source, unsigned long long length)
{
    memcpy(destination, source, length);
}

bool allocatePool(unsigned long long size, void** buffer)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        *buffer = ptr;
        return true;
    }
    return false;
}

void freePool(void* buffer)
{
    free(buffer);
}

inline void closeEvent(EFI_EVENT Event)
{
    bs->CloseEvent(Event);
}

inline EFI_STATUS createEvent(unsigned int Type, EFI_TPL NotifyTpl, void* NotifyFunction, void* NotifyContext, EFI_EVENT* Event)
{
    return bs->CreateEvent(Type, NotifyTpl, NotifyFunction, NotifyContext, Event);
}

struct Overload {

    struct TcpData {
        EFI_TCP4_CONFIG_DATA configData;
        SOCKET socket;
        bool isGlobal;
    };

    struct EventData {
        EFI_EVENT event;
        void* context;
        void* notifyFunction;
    };

    inline static std::vector<std::thread> threads;
    inline static std::map<unsigned long long, SOCKET> incomingSocketMap;
    inline static std::map<unsigned long long, TcpData> tcpDataMap;
    inline static std::map<unsigned long long, EventData> eventDataMap;

    // Directly call the setup function without using custom stack.
    static void startThread(EFI_AP_PROCEDURE procedure, void* data, unsigned long long ProcessorNumber, EFI_EVENT WaitEvent, unsigned long long TimeoutInMicroseconds) {
		bool isThreadFinished = false;
        std::thread thread([&isThreadFinished, procedure, data, ProcessorNumber]() {
           /* while (true) {
                unsigned long long currentProcessorNumber;
                WhoAmI(NULL, &currentProcessorNumber);
                if (currentProcessorNumber == ProcessorNumber) {
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }*/
            CustomStack* me = reinterpret_cast<CustomStack*>(data);
            me->setupFuncToCall(me->setupDataToPass, ProcessorNumber);
            isThreadFinished = true;
            });

        #ifdef _MSC_VER
        HANDLE hThread = (HANDLE)thread.native_handle();
        SetThreadAffinityMask(hThread, 1ULL << ProcessorNumber);
        #else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(ProcessorNumber, &cpuset);
        int rc = pthread_setaffinity_np(thread.native_handle(),
                                    sizeof(cpu_set_t),
                                    &cpuset);
        if (rc != 0) {
            logToConsole(L"Error calling pthread_setaffinity_np");
        }
        #endif

        if (TimeoutInMicroseconds > 0) {
            thread.detach();
        }
        else {
			thread.join(); // Wait for the thread to finish if no timeout is specified
			isThreadFinished = true; // Mark the thread as finished
        }

        if (TimeoutInMicroseconds > 0) {
            while (!isThreadFinished && TimeoutInMicroseconds > 0) {
                // Sleep for a short duration to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                TimeoutInMicroseconds -= 100;
            }

            if (!isThreadFinished) {
                #ifdef _MSC_VER
                TerminateThread(hThread, 0); // Forcefully terminate the thread if it doesn't finish
                #else
                pthread_cancel(thread.native_handle());
                #endif
            }
        }

        // call the event call back
        if (WaitEvent) {
            auto it = eventDataMap.find((unsigned long long)WaitEvent);
            if (it != eventDataMap.end()) {
                EventData& eventData = it->second;
                if (eventData.notifyFunction) {
                    // Call the notify function with the context
                    void (*notifyFunction)(void*, void*) = reinterpret_cast<void (*)(void*, void*)>(eventData.notifyFunction);
                    notifyFunction(WaitEvent, eventData.context);
                }
            }
            else {
                logToConsole(L"Event callback not found for the given event.");
            }
        }
    }

    ////////////// RuntimeServices Implementation //////////////

    static EFI_STATUS GetTime(OUT EFI_TIME* Time, OUT EFI_TIME_CAPABILITIES* Capabilities OPTIONAL) {
        logToConsole(L"GetTime IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS SetTime(IN EFI_TIME* Time) {
        logToConsole(L"SetTime IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    ////////////// BootServices Implementation //////////////

    static EFI_STATUS Stall(IN unsigned long long Microseconds) {
        // Simulate a stall by doing nothing for the specified time
        std::this_thread::sleep_for(std::chrono::microseconds(Microseconds));
        return EFI_SUCCESS;
    }

    static EFI_STATUS SetWatchdogTimer(IN unsigned long long Timeout, IN unsigned long long WatchdogCode, IN unsigned long long DataSize, IN CHAR16* WatchdogData OPTIONAL) {
        //logToConsole(L"SetWatchdogTimer IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS CloseProtocol(IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, IN EFI_HANDLE AgentHandle, IN EFI_HANDLE ControllerHandle) {
        return EFI_SUCCESS;
    }

    static EFI_STATUS LocateProtocol(IN EFI_GUID* Protocol, IN void* Registration OPTIONAL, OUT void** Interface) {
        EFI_GUID mpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
        if (memcmp(Protocol, &(tcp4ServiceBindingProtocolGuid), sizeof(EFI_GUID)) == 0) {
            ////// TCP4 Service Binding Protocol Implementation ///////
            *Interface = new EFI_SERVICE_BINDING_PROTOCOL;
            tcp4ServiceBindingProtocol->CreateChild = Overload::CreateChild;
            tcp4ServiceBindingProtocol->DestroyChild = Overload::DestroyChild;
            return EFI_SUCCESS;
        }
        else if (memcmp(Protocol, &(mpServiceProtocolGuid), sizeof(EFI_GUID)) == 0) {
            ///// MP Services Protocol Implementation /////
            *Interface = new EFI_MP_SERVICES_PROTOCOL;
            mpServicesProtocol->GetNumberOfProcessors = Overload::GetNumberOfProcessors;
            mpServicesProtocol->WhoAmI = Overload::WhoAmI;
            mpServicesProtocol->GetProcessorInfo = Overload::GetProcessorInfo;
            mpServicesProtocol->StartupThisAP = Overload::StartupThisAP;
            return EFI_SUCCESS;
        }

        logToConsole(L"LocateProtocol IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS WaitForEvent(IN unsigned long long NumberOfEvents, IN EFI_EVENT* Event, OUT unsigned long long* Index) {
        logToConsole(L"WaitForEvent IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS OpenProtocol(IN EFI_HANDLE Handle, IN EFI_GUID* Protocol, OUT void** Interface OPTIONAL, IN EFI_HANDLE AgentHandle, IN EFI_HANDLE ControllerHandle, IN unsigned int Attributes) {
        if (memcmp(Protocol, &tcp4ProtocolGuid, sizeof(EFI_GUID)) == 0) {
            *Interface = new EFI_TCP4_PROTOCOL;
            // Check if this is a incomming socket and set socket instance if it is
            if (incomingSocketMap.contains((unsigned long long)Handle)) {
                TcpData& tcpData = tcpDataMap[(unsigned long long) * Interface];
                tcpData.socket = incomingSocketMap[(unsigned long long)Handle];

                incomingSocketMap.erase((unsigned long long)Handle);
            }

            // Map handle to the tcp4Protocol so we can get tcp4Protocol from the handle
            *(unsigned long long*)Handle = (unsigned long long) * Interface;

            EFI_TCP4_PROTOCOL* tcp4Protocol = reinterpret_cast<EFI_TCP4_PROTOCOL*>(*Interface);
            tcp4Protocol->GetModeData = Overload::GetModeData;
            tcp4Protocol->Poll = Overload::Poll;
            tcp4Protocol->Transmit = Overload::Transmit;
            tcp4Protocol->Receive = Overload::Receive;
            tcp4Protocol->Close = Overload::Close;
            tcp4Protocol->Cancel = Overload::Cancel;
            tcp4Protocol->Configure = Overload::Configure;
            tcp4Protocol->Accept = Overload::Accept;
            tcp4Protocol->Connect = Overload::Connect;
            tcp4Protocol->Routes = Overload::Routes;
            return EFI_SUCCESS;
        }

        logToConsole(L"OpenProtocol IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS LocateHandleBuffer(IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID* Protocol OPTIONAL, IN void* SearchKey OPTIONAL, OUT unsigned long long* NoHandles, OUT EFI_HANDLE** Buffer) {
        logToConsole(L"LocateHandleBuffer IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS CreateEvent(IN unsigned int Type, IN EFI_TPL NotifyTpl, IN void* NotifyFunction, OPTIONAL IN void* NotifyContext, OPTIONAL OUT EFI_EVENT* Event) {
        if (Type == EVT_NOTIFY_SIGNAL && (NotifyTpl == TPL_CALLBACK || NotifyTpl == TPL_NOTIFY)) {
            *Event = new unsigned long long;
            EventData eventData;
            eventData.event = *Event;
            eventData.context = NotifyContext;
            eventData.notifyFunction = NotifyFunction;
            eventDataMap[(unsigned long long) * Event] = eventData;
            return EFI_SUCCESS;
        }

        logToConsole(L"Create Event IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS CloseEvent(IN EFI_EVENT Event) {
        auto it = eventDataMap.find((unsigned long long)Event);
        if (it != eventDataMap.end()) {
            delete (unsigned long long*)Event; // Free the allocated memory for the event
            eventDataMap.erase(it); // Remove from the map
            return EFI_SUCCESS;
        }

        logToConsole(L"No event found in map");
        return EFI_NOT_FOUND;
    }

    ////////////// SystemTable Implementation //////////////

    static EFI_STATUS ClearScreen(IN void* This) {
        return EFI_SUCCESS;
    }

    static EFI_STATUS ReadKeyStroke(IN void* This, OUT EFI_INPUT_KEY* Key) {
#ifdef _MSC_VER
        if (_kbhit()) {               // check if key was pressed
            int ch = _getch();        // now it's safe to read
            if (ch == 27) {
                Key->ScanCode = 0x17;
            };

            if (ch == 0 || ch == 224) {
                int code = _getch();

                // check f2->f12
                switch (code) {
                case 60:  Key->ScanCode = 0x0C; break;
                case 61:  Key->ScanCode = 0x0D; break;
                case 62:  Key->ScanCode = 0x0E; break;
                case 63:  Key->ScanCode = 0x0F; break;
                case 64:  Key->ScanCode = 0x10; break;
                case 65:  Key->ScanCode = 0x11; break;
                case 66:  Key->ScanCode = 0x12; break;
                case 67:  Key->ScanCode = 0x13; break;
                case 68:  Key->ScanCode = 0x14; break;
                case 133: Key->ScanCode = 0x15; break;
                case 134: Key->ScanCode = 0x16; break;
                }
            }
            else {
                if (ch == 'p') {
                    Key->ScanCode = 0x48;
                }
            }

            return EFI_SUCCESS;
        }
#else
        static std::map<std::vector<unsigned char>, std::string> keyMap = {
            {{27,79,80}, "F1"}, {{27,79,81}, "F2"},
            {{27,79,82}, "F3"}, {{27,79,83}, "F4"},
            {{27,91,49,53,126}, "F5"}, {{27,91,49,55,126}, "F6"},
            {{27,91,49,56,126}, "F7"}, {{27,91,49,57,126}, "F8"},
            {{27,91,50,48,126}, "F9"}, {{27,91,50,49,126}, "F10"},
            {{27,91,50,51,126}, "F11"}, {{27,91,50,52,126}, "F12"}
        };

        std::vector<unsigned char> input = readInput();
        if (!input.empty()) {
            // Try to match against known sequences
            if (keyMap.count(input)) {
                // Map f2->f12 to EFI_INPUT_KEY
                std::string keyName = keyMap[input];

                if (keyName == "F2")  Key->ScanCode = 0x0C;
                else if (keyName == "F3")  Key->ScanCode = 0x0D;
                else if (keyName == "F4")  Key->ScanCode = 0x0E;
                else if (keyName == "F5")  Key->ScanCode = 0x0F;
                else if (keyName == "F6")  Key->ScanCode = 0x10;
                else if (keyName == "F7")  Key->ScanCode = 0x11;
                else if (keyName == "F8")  Key->ScanCode = 0x12;
                else if (keyName == "F9")  Key->ScanCode = 0x13;
                else if (keyName == "F10") Key->ScanCode = 0x14;
                else if (keyName == "F11") Key->ScanCode = 0x15;
                else if (keyName == "F12") Key->ScanCode = 0x16;
            } else {
                // map 'p' to fake pause key
                if (input.size() == 1 && input[0] == 'p') {
                    Key->ScanCode = 0x48;
                }
            }

            return EFI_SUCCESS;
        }
#endif

        return EFI_NOT_READY;
    }

    ////////////// MP Services Protocol Implementation //////////////

    static EFI_STATUS GetNumberOfProcessors(IN void* This, OUT unsigned long long* NumberOfProcessors, OUT unsigned long long* NumberOfEnabledProcessors) {
        *NumberOfProcessors = (unsigned long long)std::thread::hardware_concurrency();
        *NumberOfEnabledProcessors = *NumberOfProcessors; // Assume all processors are enabled
        return EFI_SUCCESS;
    }

    static EFI_STATUS WhoAmI(IN void* This, OUT unsigned long long* ProcessorNumber) {
        *ProcessorNumber = getCurrentCpuIndex();
        return EFI_SUCCESS;
    }

    static EFI_STATUS GetProcessorInfo(IN void* This, IN unsigned long long ProcessorNumber, OUT EFI_PROCESSOR_INFORMATION* ProcessorInfoBuffer) {
        ProcessorInfoBuffer->StatusFlag = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT; // Assume the processor is enabled and healthy
        ProcessorInfoBuffer->Location = { 0, 0 }; // Location is not used in this implementation
        ProcessorInfoBuffer->ProcessorId = ProcessorNumber; // Use the processor number as the ID
        return EFI_SUCCESS;
    }

    // Use custom stack in std:thread will break the runtime because it expectes the OS-provided stack
    // so we can bypass the custom stack because stack size in window/linux already is large enough
    static EFI_STATUS StartupThisAP(IN void* This, IN EFI_AP_PROCEDURE Procedure, IN unsigned long long ProcessorNumber, IN EFI_EVENT WaitEvent OPTIONAL, IN unsigned long long TimeoutInMicroseconds, IN void* ProcedureArgument OPTIONAL, OUT BOOLEAN* Finished OPTIONAL) {
        std::thread thread(startThread, Procedure, ProcedureArgument, ProcessorNumber, WaitEvent, TimeoutInMicroseconds);
        thread.detach();
        return EFI_SUCCESS;
    }

    ////////////// TCP4 Service Binding Protocol Implementation //////////////

    static EFI_STATUS CreateChild(IN void* This, OUT EFI_HANDLE* ChildHandle) {
        // Preserve 8 bytes to hold the address of tcp4Protocol
        void* _8Bytes = new unsigned long long;
        *ChildHandle = _8Bytes;
        return EFI_SUCCESS;
    }

    static EFI_STATUS DestroyChild(IN void* This, IN EFI_HANDLE ChildHandle) {
		// remove tcp4Protocol data from handle
		if (tcpDataMap.contains(*(unsigned long long*)ChildHandle)) {
			TcpData& tcpData = tcpDataMap[*(unsigned long long*)ChildHandle];
			if (tcpData.socket != INVALID_SOCKET) {
				closesocket(tcpData.socket);
				tcpData.socket = INVALID_SOCKET;
			}
			tcpDataMap.erase(*(unsigned long long*)ChildHandle);
		}
        freePool(ChildHandle);
        return EFI_SUCCESS;
    }

    ////////////// TCP4 Protocol Implementation //////////////

    static EFI_STATUS Routes(IN void* This, IN BOOLEAN DeleteRoute, IN EFI_IPv4_ADDRESS* SubnetAddress, IN EFI_IPv4_ADDRESS* SubnetMask, IN EFI_IPv4_ADDRESS* GatewayAddress) {
        logToConsole(L"Routes IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS Close(IN void* This, IN EFI_TCP4_CLOSE_TOKEN* CloseToken) {
        logToConsole(L"Close IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS Cancel(IN void* This, IN EFI_TCP4_COMPLETION_TOKEN* Token OPTIONAL) {
        logToConsole(L"Cancel IS NOT IMPLEMENTED");
        return EFI_UNSUPPORTED;
    }

    static EFI_STATUS Poll(IN void* This) {
        return EFI_SUCCESS;
    }

    static EFI_STATUS Transmit(IN void* This, IN EFI_TCP4_IO_TOKEN* Token) {
        TcpData* tcpData = nullptr;
        unsigned long long key = (unsigned long long)This;
        if (tcpDataMap.contains(key)) {
            tcpData = &tcpDataMap[key];
        }
        else {
            logToConsole(L"No Tcp Data For This Connect!");
            return EFI_UNSUPPORTED;
        }

        if (tcpData->socket == INVALID_SOCKET) {
            logToConsole(L"No Available Socket Connect!");
            return EFI_UNSUPPORTED;
        }

        const auto& fragment = Token->Packet.TxData->FragmentTable[0];
        int totalSentBytes = 0;
        while (totalSentBytes < fragment.FragmentLength) {
#ifdef _MSC_VER
#define MSG_NOSIGNAL 0
#define MSG_DONTWAIT 0
#endif
            int sentBytes = send(tcpData->socket, (const char*)fragment.FragmentBuffer + totalSentBytes, fragment.FragmentLength - totalSentBytes, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (sentBytes == 0) {
                // connection closed
                Token->CompletionToken.Status = -1;
                return EFI_ABORTED;
            } else if (sentBytes == SOCKET_ERROR)
            {
#ifdef _MSC_VER
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    // // wait until socket is writable
                    // fd_set wfds;
                    // FD_ZERO(&wfds);
                    // FD_SET(tcpData->socket, &wfds);
                    //
                    // int timeout_ms = 500; // no timeout
                    //
                    // TIMEVAL tv;
                    // TIMEVAL* ptv = nullptr;
                    // if (timeout_ms >= 0) {
                    //     tv.tv_sec = timeout_ms / 1000;
                    //     tv.tv_usec = (timeout_ms % 1000) * 1000;
                    //     ptv = &tv;
                    // }
                    //
                    // int ret = select(0, nullptr, &wfds, nullptr, ptv);
                    // if (ret <= 0) {
                    //     // timeout or error
                    //     Token->CompletionToken.Status = -1;
                    //     return EFI_ABORTED;
                    // }
                    continue; // retry send
				}
				else {
					Token->CompletionToken.Status = -1;
					return EFI_ABORTED;
				}
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // pollfd pfd;
                    // pfd.fd = tcpData->socket;
                    // pfd.events = POLLOUT;
                    // int ret = poll(&pfd, 1, -1);
                    // if (ret <= 0) {
                    //     // timeout or error
                    //     Token->CompletionToken.Status = -1;
                    //     return EFI_ABORTED;
                    // }
                    continue; // retry
                } else
                {
                    Token->CompletionToken.Status = -1;
                    return EFI_ABORTED;
                }
#endif
            } else
            {
                totalSentBytes += sentBytes;
            }
        }

        Token->CompletionToken.Status = EFI_SUCCESS;

        return EFI_SUCCESS;
    }

    static EFI_STATUS Receive(IN void* This, IN EFI_TCP4_IO_TOKEN* Token) {
        TcpData* tcpData = nullptr;
        unsigned long long key = (unsigned long long)This;
        if (tcpDataMap.contains(key)) {
            tcpData = &tcpDataMap[key];
        }
        else {
            logToConsole(L"No Tcp Data For This Connect!");
            return EFI_ABORTED;
        }

        if (tcpData->socket == INVALID_SOCKET) {
            logToConsole(L"No Available Socket Connect!");
            return EFI_ABORTED;
        }

        char buffer[8 * 1024];
        int totalReceivedBytes = 0;
        memset(buffer, 0, sizeof(buffer));
        Token->Packet.RxData->DataLength = 0;

        while (true)
        {
#ifdef _MSC_VER
#define MSG_DONTWAIT 0
#endif
            int bytes = recv(tcpData->socket, buffer, sizeof(buffer), MSG_DONTWAIT);
            if (bytes > 0)
            {
                memcpy((char *)Token->Packet.RxData->FragmentTable[0].FragmentBuffer + totalReceivedBytes, buffer, bytes);
                totalReceivedBytes += bytes;
                Token->Packet.RxData->DataLength += bytes;
            } else if (bytes == SOCKET_ERROR)
            {
#ifdef _MSC_VER
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    // nothing left to read right now
                    Token->CompletionToken.Status = EFI_SUCCESS;
                    break;
                } else {
                    // real error
                    Token->CompletionToken.Status = -1;
                    return EFI_ABORTED;
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // nothing left to read right now
                    Token->CompletionToken.Status = EFI_SUCCESS;
                    break;
                } else
                {
                    Token->CompletionToken.Status = -1;
                    return EFI_ABORTED;
                }
#endif
            } else if (bytes == 0)
            {
                // connection closed, mark status as error to reconnect in main thread
                Token->CompletionToken.Status = -1;
                return EFI_CONNECTION_FIN;
            }
        }

        Token->CompletionToken.Status = EFI_SUCCESS;
        return EFI_SUCCESS;
    }

    static EFI_STATUS GetModeData(IN void* This, OUT EFI_TCP4_CONNECTION_STATE* Tcp4State OPTIONAL, OUT EFI_TCP4_CONFIG_DATA* Tcp4ConfigData OPTIONAL, OUT EFI_IP4_MODE_DATA* Ip4ModeData OPTIONAL, OUT EFI_MANAGED_NETWORK_CONFIG_DATA* MnpConfigData OPTIONAL, OUT EFI_SIMPLE_NETWORK_MODE* SnpModeData OPTIONAL) {
        if (Ip4ModeData) {
            Ip4ModeData->IsConfigured = true;
            Ip4ModeData->IsStarted = true;
            Ip4ModeData->RouteCount = 0;
            Ip4ModeData->RouteTable = new EFI_IP4_ROUTE_TABLE;
            memset(Ip4ModeData->RouteTable->GatewayAddress.Addr, 0, sizeof(IPv4Address));
            memset(Ip4ModeData->RouteTable->SubnetMask.Addr, 0, sizeof(IPv4Address));
            memset(Ip4ModeData->RouteTable->SubnetAddress.Addr, 0, sizeof(IPv4Address));
        }

        if (Tcp4State) {
			if (tcpDataMap.contains((unsigned long long)This)) {
				TcpData& tcpData = tcpDataMap[(unsigned long long)This];
				if (tcpData.socket == INVALID_SOCKET) {
					*Tcp4State = Tcp4StateClosed;
				}
				else {
					*Tcp4State = Tcp4StateEstablished;
				}
			}
			else {
				*Tcp4State = Tcp4StateClosed;
			}
        }

        if (Tcp4ConfigData) {
        }

        if (MnpConfigData) {
            logToConsole(L"GetModeData for MnpConfigData is not implemented");
            return EFI_ABORTED;
        }

        if (SnpModeData) {
            logToConsole(L"GetModeData for SnpModeData is not implemented");
            return EFI_ABORTED;
        }

        return EFI_SUCCESS;
    }

    static EFI_STATUS Configure(IN void* This, IN EFI_TCP4_CONFIG_DATA* TcpConfigData OPTIONAL) {
        static bool isGlobalSocketInitialized = false;
        if (!TcpConfigData) {
            return EFI_SUCCESS;
        }

        TcpData data;
        data.configData = *TcpConfigData;
        data.isGlobal = *((unsigned int*)TcpConfigData->AccessPoint.RemoteAddress.Addr) == 0;
        data.socket = INVALID_SOCKET;

        // Global set up for accepting new connections
        if ((unsigned long long)This == (unsigned long long)peerTcp4Protocol && !isGlobalSocketInitialized) {
            #ifdef _MSC_VER
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                logToConsole(L"WSAStartup failed!!");
                return EFI_ABORTED;
            }
            #endif

            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                logToConsole(L"Socket creation failed!!");
                return EFI_ABORTED;
            }

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(TcpConfigData->AccessPoint.StationPort);
            addr.sin_addr.s_addr = INADDR_ANY;

            if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                logToConsole(L"Failed to bind socket!");
                closesocket(sock);

                return EFI_ABORTED;
            }

            if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
                logToConsole(L"Failed to listen socket!");
                closesocket(sock);

                return EFI_ABORTED;
            }

            logToConsole(L"Socket binded");
            data.socket = sock;
			isGlobalSocketInitialized = true;
        }

        unsigned long long key = (unsigned long long)This;
        tcpDataMap[key] = data;
        return EFI_SUCCESS;
    }

    // Note: Only global tcp4Protocol call this function, peers don't call
    static EFI_STATUS Accept(IN void* This, IN EFI_TCP4_LISTEN_TOKEN* ListenToken) {
        TcpData* tcpData = nullptr;
        unsigned long long key = (unsigned long long)This;
        if (tcpDataMap.contains(key)) {
            tcpData = &tcpDataMap[key];
        }
        else {
            logToConsole(L"No Tcp Data For Global Tcp Connect!");
            return EFI_UNSUPPORTED;
        }

        // accept in a thread
        std::thread acceptThread([tcpData, ListenToken]() {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(tcpData->configData.AccessPoint.StationPort);
            addr.sin_addr.s_addr = INADDR_ANY;
            #ifdef _MSC_VER
            int addrlen = sizeof(addr);
            #else
            socklen_t addrlen = sizeof(addr);
            #endif
            SOCKET clientSocket = accept(tcpData->socket, (sockaddr*)&addr, &addrlen);
            if (clientSocket == INVALID_SOCKET) {
                logToConsole(L"Obtained tcpData failed");
                ListenToken->CompletionToken.Status = EFI_ABORTED;
                return;
            }

#ifdef _MSC_VER
            u_long mode = 1;
            ioctlsocket(clientSocket, FIONBIO, &mode);
#endif

            CreateChild(NULL, &ListenToken->NewChildHandle);
            // At this point we dont know the tcp4Protocol for this peer (tcp4Protocol will be inititialzed in peerConnectionNewlyEstablished())
            // so we map the clientSocket to the handle to process it later in peerConnectionNewlyEstablished()
            incomingSocketMap[(unsigned long long)ListenToken->NewChildHandle] = clientSocket;
            ListenToken->CompletionToken.Status = EFI_SUCCESS;
            });
        acceptThread.detach();
        return EFI_SUCCESS;
    }

    static EFI_STATUS Connect(IN void* This, IN EFI_TCP4_CONNECTION_TOKEN* ConnectionToken) {
        TcpData* tcpData = nullptr;
        unsigned long long key = (unsigned long long)This;
        if (tcpDataMap.contains(key)) {
            tcpData = &tcpDataMap[key];
        }
        else {
            logToConsole(L"No Tcp Data For This Connect!");
            return EFI_UNSUPPORTED;
        }

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            logToConsole(L"Socket creation failed!!");

            return EFI_ABORTED;
        }
        tcpData->socket = sock;

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(tcpData->configData.AccessPoint.RemotePort);
        #ifdef _MSC_VER
        serverAddr.sin_addr.S_un.S_addr = *((unsigned long*)tcpData->configData.AccessPoint.RemoteAddress.Addr);
        #else
        serverAddr.sin_addr.s_addr = *((unsigned long*)tcpData->configData.AccessPoint.RemoteAddress.Addr);
        #endif

#ifdef _MSC_VER
        u_long mode = 1;
        ioctlsocket(tcpData->socket, FIONBIO, &mode);
#endif

        // connect in a thread
        std::thread connectThread([tcpData, serverAddr, ConnectionToken]() {
            if (connect(tcpData->socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                ConnectionToken->CompletionToken.Status = EFI_ABORTED;
            }
            else {
                ConnectionToken->CompletionToken.Status = EFI_SUCCESS;
            }
            });
        connectThread.detach();

        return EFI_SUCCESS;
    }

    static void initializeUefi() {
        #ifndef _MSC_VER
        setNonBlockingInput(true);

        // Pin the main thread to CPU 0 to make sure main thread cpu id wont change during process
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
        #else
        // Pin the main thread to CPU 0 to make sure main thread cpu id wont change during process
		// NOTE: In MSVC Release Mode, so the scheduler often just keeps the main thread on one CPU core (the best core), dont need to set affinity because it will slow down the main thread performance
        //HANDLE hThread = GetCurrentThread();
        //SetThreadAffinityMask(hThread, 1ULL << 0);
        #endif

        ih = new EFI_HANDLE;
        st = new EFI_SYSTEM_TABLE;
        st->BootServices = new EFI_BOOT_SERVICES;
        st->ConOut = new EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
        st->ConIn = new EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
        bs = st->BootServices;
        rs = new EFI_RUNTIME_SERVICES;

        ////// RuntimeServices Implementation ///////
        rs->GetTime = Overload::GetTime;
        rs->SetTime = Overload::SetTime;

        ////// BootServices Implementation ///////
        bs->Stall = Overload::Stall;
        bs->SetWatchdogTimer = Overload::SetWatchdogTimer;
        bs->LocateProtocol = Overload::LocateProtocol;
        bs->CloseProtocol = Overload::CloseProtocol;
        bs->OpenProtocol = Overload::OpenProtocol;
        bs->WaitForEvent = Overload::WaitForEvent;
        bs->LocateHandleBuffer = Overload::LocateHandleBuffer;
        bs->CreateEvent = Overload::CreateEvent;
        bs->CloseEvent = Overload::CloseEvent;

        ///// SystemTable Implementation /////
        st->ConOut->ClearScreen = Overload::ClearScreen;
        st->ConIn->ReadKeyStroke = Overload::ReadKeyStroke;
    }
};

#define logToConsole logToConsole_1