#pragma once

#include <lib/platform_efi/uefi.h>
#include "platform/console_logging.h"

#include "network_messages/header.h"


// Must be 2 * RequestResponseHeader::max_size (maximum message size) because
// double buffering is used to avoid waiting
#define BUFFER_SIZE 33554432
static_assert(RequestResponseHeader::max_size * 2 + 2 == BUFFER_SIZE, "unexpected buffer size");


static EFI_GUID tcp4ServiceBindingProtocolGuid = EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;
static EFI_SERVICE_BINDING_PROTOCOL* tcp4ServiceBindingProtocol = NULL;
static EFI_GUID tcp4ProtocolGuid = EFI_TCP4_PROTOCOL_GUID;
static EFI_TCP4_PROTOCOL* peerTcp4Protocol = NULL;
static EFI_HANDLE peerChildHandle = NULL;
static EFI_IPv4_ADDRESS nodeAddress;


static EFI_HANDLE getTcp4Protocol(const unsigned char* remoteAddress, const unsigned short port, EFI_TCP4_PROTOCOL** tcp4Protocol)
{
    EFI_STATUS status;
    EFI_HANDLE childHandle = NULL;
    if (status = tcp4ServiceBindingProtocol->CreateChild(tcp4ServiceBindingProtocol, &childHandle))
    {
        logStatusToConsole(L"EFI_TCP4_SERVICE_BINDING_PROTOCOL.CreateChild() fails", status, __LINE__);

        return NULL;
    }
    else
    {
        if (status = bs->OpenProtocol(childHandle, &tcp4ProtocolGuid, (void**)tcp4Protocol, ih, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.OpenProtocol() fails", status, __LINE__);

            return NULL;
        }
        else
        {
            EFI_TCP4_CONFIG_DATA configData;
            setMem(&configData, sizeof(configData), 0);
            configData.TimeToLive = 64;
            configData.AccessPoint.UseDefaultAddress = TRUE;
            if (!remoteAddress)
            {
                configData.AccessPoint.StationPort = port;
            }
            else
            {
                *((int*)configData.AccessPoint.RemoteAddress.Addr) = *((int*)remoteAddress);
                configData.AccessPoint.RemotePort = port;
                configData.AccessPoint.ActiveFlag = TRUE;
            }
            EFI_TCP4_OPTION option;
            setMem(&option, sizeof(option), 0);
            option.ReceiveBufferSize = BUFFER_SIZE;
            option.SendBufferSize = BUFFER_SIZE;
            option.KeepAliveProbes = 1;
            option.EnableWindowScaling = TRUE;
            configData.ControlOption = &option;

            if ((status = (*tcp4Protocol)->Configure(*tcp4Protocol, &configData))
                && status != EFI_NO_MAPPING)
            {
                logStatusToConsole(L"EFI_TCP4_PROTOCOL.Configure() fails", status, __LINE__);

                return NULL;
            }
            else
            {
                EFI_IP4_MODE_DATA modeData;

                if (status == EFI_NO_MAPPING)
                {
                    WAIT_WHILE(!(status = (*tcp4Protocol)->GetModeData(*tcp4Protocol, NULL, NULL, &modeData, NULL, NULL))
                        && !modeData.IsConfigured);
                    if (!status)
                    {
                        if (status = (*tcp4Protocol)->Configure(*tcp4Protocol, &configData))
                        {
                            logStatusToConsole(L"EFI_TCP4_PROTOCOL.Configure() fails", status, __LINE__);

                            return NULL;
                        }
                    }
                }

                if (status = (*tcp4Protocol)->GetModeData(*tcp4Protocol, NULL, &configData, &modeData, NULL, NULL))
                {
                    logStatusToConsole(L"EFI_TCP4_PROTOCOL.GetModeData() fails", status, __LINE__);

                    return NULL;
                }
                else
                {
                    if (!modeData.IsStarted || !modeData.IsConfigured)
                    {
                        logToConsole(L"EFI_TCP4_PROTOCOL is not configured!");

                        return NULL;
                    }
                    else
                    {
                        if (!remoteAddress)
                        {
                            setText(message, L"Local address = ");
                            appendIPv4Address(message, configData.AccessPoint.StationAddress);
                            appendText(message, L":");
                            appendNumber(message, configData.AccessPoint.StationPort, FALSE);
                            appendText(message, L".");
                            logToConsole(message);
                            nodeAddress = configData.AccessPoint.StationAddress;

                            logToConsole(L"Routes:");
                            for (unsigned int i = 0; i < modeData.RouteCount; i++)
                            {
                                setText(message, L"Address = ");
                                appendIPv4Address(message, modeData.RouteTable[i].SubnetAddress);
                                appendText(message, L" | mask = ");
                                appendIPv4Address(message, modeData.RouteTable[i].SubnetMask);
                                appendText(message, L" | gateway = ");
                                appendIPv4Address(message, modeData.RouteTable[i].GatewayAddress);
                                appendText(message, L".");
                                logToConsole(message);
                            }
                        }

                        return childHandle;
                    }
                }
            }
        }
    }
}

static bool initTcp4(unsigned short local_port)
{
    EFI_STATUS status;
    if (status = bs->LocateProtocol(&tcp4ServiceBindingProtocolGuid, NULL, (void**)&tcp4ServiceBindingProtocol))
    {
        logStatusToConsole(L"EFI_TCP4_SERVICE_BINDING_PROTOCOL is not located", status, __LINE__);
        return false;
    }

    peerChildHandle = getTcp4Protocol(NULL, local_port, &peerTcp4Protocol);
    if (!peerChildHandle)
    {
        return false;
    }

    return true;
}

static void deinitTcp4()
{
    bs->CloseProtocol(peerChildHandle, &tcp4ProtocolGuid, ih, NULL);
    tcp4ServiceBindingProtocol->DestroyChild(tcp4ServiceBindingProtocol, peerChildHandle);
}

