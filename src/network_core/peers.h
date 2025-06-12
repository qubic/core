// handling of peers, connections, receiving, and transmitting
// (general and low level, independent of specific network message types)

#pragma once

#include <lib/platform_common/processor.h>
#include <lib/platform_efi/uefi.h>
#include "platform/random.h"
#include "platform/concurrency.h"
#include "platform/profiling.h"

#include "network_messages/common_def.h"
#include "network_messages/header.h"
#include "network_messages/common_response.h"

#include "tcp4.h"
#include "kangaroo_twelve.h"

#include "text_output.h"


#define DEJAVU_SWAP_LIMIT 1000000
#define DISSEMINATION_MULTIPLIER 6
#define NUMBER_OF_OUTGOING_CONNECTIONS 8
#define NUMBER_OF_INCOMING_CONNECTIONS 88
#define MAX_NUMBER_OF_PUBLIC_PEERS 1024
#define REQUEST_QUEUE_BUFFER_SIZE 1073741824
#define REQUEST_QUEUE_LENGTH 65536 // Must be 65536
#define RESPONSE_QUEUE_BUFFER_SIZE 1073741824
#define RESPONSE_QUEUE_LENGTH 65536 // Must be 65536
#define NUMBER_OF_PUBLIC_PEERS_TO_KEEP 10
#define NUMBER_OF_WHITE_LIST_PEERS sizeof(whiteListPeers) / sizeof(whiteListPeers[0])
#define NUMBER_OF_INCOMING_CONNECTIONS_RESERVED_FOR_WHITELIST_IPS 16
static_assert((NUMBER_OF_INCOMING_CONNECTIONS / NUMBER_OF_OUTGOING_CONNECTIONS) >= 11, "Number of incoming connections must be x11+ number of outgoing connections to keep healthy network");

static volatile bool listOfPeersIsStatic = false;


struct Peer
{
    EFI_TCP4_PROTOCOL* tcp4Protocol;
    EFI_TCP4_LISTEN_TOKEN connectAcceptToken;
    IPv4Address address;
    void* receiveBuffer;
    EFI_TCP4_RECEIVE_DATA receiveData;
    EFI_TCP4_IO_TOKEN receiveToken;
    EFI_TCP4_TRANSMIT_DATA transmitData;
    EFI_TCP4_IO_TOKEN transmitToken;
    char* dataToTransmit;
    unsigned int dataToTransmitSize;
    BOOLEAN isConnectingAccepting;
    BOOLEAN isConnectedAccepted;
    BOOLEAN isReceiving, isTransmitting;
    BOOLEAN exchangedPublicPeers;
    BOOLEAN isClosing;
    // Indicate the peer is incomming connection type
    BOOLEAN isIncommingConnection;

    // Extra data to determine if this peer is a fullnode
    // Note: an **active fullnode** is a peer that is able to reply valid tick data, tick vote to this node after getting requested
    // If a peer is an active fullnode, it will receive more requests from this node than others, as well as longer alive connection time.
    // Here we also consider relayer as a fullnode (as long as it transmits new&valid tick data)
    static constexpr unsigned int dejavuListSize = 32;
    unsigned int trackRequestedDejavu[dejavuListSize];
    unsigned int trackRequestedTick[dejavuListSize];
    long trackRequestedCounter; // "long" to discard warning from intrin.h
    unsigned int lastActiveTick; // indicate the tick number that this peer transfer valid tick/vote data

    bool isFullNode() const
    {
        return (lastActiveTick >= system.tick - 100);
    }

    // store a dejavu number into local list
    void trackDejavu(unsigned int dejavu)
    {
        if (!dejavu) return;
        long index = _InterlockedIncrement(&trackRequestedCounter);
        index %= dejavuListSize;
        trackRequestedDejavu[index] = dejavu;
        trackRequestedTick[index] = system.tick;
    }

    // check if dejavu is inside local list
    // if found, return the tick when the request (with dejavu) was sent
    unsigned int getDejavuTick(unsigned int dejavu)
    {
        if (!dejavu) return 0; // no check for 0 dejavu
        for (int i = 0; i < dejavuListSize; i++)
        {
            if (dejavu == trackRequestedDejavu[i])
            {
                return trackRequestedTick[i];
            }
        }
        return 0;
    }

    // set handler to null and all params to false/zeroes
    void reset()
    {
        tcp4Protocol = NULL;
        isConnectingAccepting = FALSE;
        isConnectedAccepted = FALSE;
        isReceiving = FALSE;
        isTransmitting = FALSE;
        exchangedPublicPeers = FALSE;
        isClosing = FALSE;
        isIncommingConnection = FALSE;
        dataToTransmitSize = 0;
        lastActiveTick = 0;
        trackRequestedCounter = 0;
        setMem(trackRequestedTick, sizeof(trackRequestedTick), 0);
        setMem(trackRequestedDejavu, sizeof(trackRequestedDejavu), 0);
    }
};

typedef struct
{
    bool isHandshaked;
    bool isFullnode;
    IPv4Address address;
} PublicPeer;

static Peer peers[NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS];
static volatile long long numberOfReceivedBytes = 0, prevNumberOfReceivedBytes = 0;
static volatile long long numberOfTransmittedBytes = 0, prevNumberOfTransmittedBytes = 0;
static int numberOfAcceptedIncommingConnection = 0;

static volatile char publicPeersLock = 0;
static unsigned int numberOfPublicPeers = 0;
static PublicPeer publicPeers[MAX_NUMBER_OF_PUBLIC_PEERS];

static unsigned long long* dejavu0 = NULL;
static unsigned long long* dejavu1 = NULL;
static unsigned int dejavuSwapCounter = DEJAVU_SWAP_LIMIT;

static volatile long long numberOfProcessedRequests = 0, prevNumberOfProcessedRequests = 0;
static volatile long long numberOfDiscardedRequests = 0, prevNumberOfDiscardedRequests = 0;
static volatile long long numberOfDuplicateRequests = 0, prevNumberOfDuplicateRequests = 0;
static volatile long long numberOfDisseminatedRequests = 0, prevNumberOfDisseminatedRequests = 0;

static unsigned char* requestQueueBuffer = NULL;
static unsigned char* responseQueueBuffer = NULL;

static struct Request
{
    Peer* peer;
    unsigned int offset;
} requestQueueElements[REQUEST_QUEUE_LENGTH];

static struct Response
{
    Peer* peer;
    unsigned int offset;
} responseQueueElements[RESPONSE_QUEUE_LENGTH];

static volatile unsigned int requestQueueBufferHead = 0, requestQueueBufferTail = 0;
static volatile unsigned int responseQueueBufferHead = 0, responseQueueBufferTail = 0;
static volatile unsigned short requestQueueElementHead = 0, requestQueueElementTail = 0;
static volatile unsigned short responseQueueElementHead = 0, responseQueueElementTail = 0;
static volatile char requestQueueTailLock = 0;
static volatile char responseQueueHeadLock = 0;
static volatile unsigned long long queueProcessingNumerator = 0, queueProcessingDenominator = 0;
static volatile unsigned long long tickerLoopNumerator = 0, tickerLoopDenominator = 0;

/*
static bool isWhiteListPeer(unsigned char address[4])
{
    for (unsigned int i = 0; i < NUMBER_OF_WHITE_LIST_PEERS; i++)
    {
        const auto& whiteListIp = whiteListPeers[i];
        if (address[0] == whiteListIp[0]
            && address[1] == whiteListIp[1]
            && address[2] == whiteListIp[2]
            && address[3] == whiteListIp[3])
        {
            return true;
        }
    }
    return false;
}
*/

static void closePeer(Peer* peer)
{
    PROFILE_SCOPE();
    ASSERT(isMainProcessor());
    if (((unsigned long long)peer->tcp4Protocol) > 1)
    {
        if (!peer->isClosing)
        {
            EFI_STATUS status;
            if (status = peer->tcp4Protocol->Configure(peer->tcp4Protocol, NULL))
            {
                logStatusToConsole(L"EFI_TCP4_PROTOCOL.Configure() fails", status, __LINE__);
            }

            peer->isClosing = TRUE;
        }

        if (!peer->isConnectingAccepting && !peer->isReceiving && !peer->isTransmitting)
        {
            bs->CloseProtocol(peer->connectAcceptToken.NewChildHandle, &tcp4ProtocolGuid, ih, NULL);
            EFI_STATUS status;
            if (status = tcp4ServiceBindingProtocol->DestroyChild(tcp4ServiceBindingProtocol, peer->connectAcceptToken.NewChildHandle))
            {
                logStatusToConsole(L"EFI_TCP4_SERVICE_BINDING_PROTOCOL.DestroyChild() fails", status, __LINE__);
            }

            // Decrease the accepted counter
            if (peer->isConnectedAccepted && peer->isIncommingConnection)
            {
                numberOfAcceptedIncommingConnection--;
                ASSERT(numberOfAcceptedIncommingConnection >= 0);
            }
            peer->reset();
        }
    }
}

// Add message to sending buffer of specific peer, can only called from main thread (not thread-safe).
static void push(Peer* peer, RequestResponseHeader* requestResponseHeader)
{
    PROFILE_SCOPE();

    // The sending buffer may queue multiple messages, each of which may need to transmitted in many small packets.
    if (peer->tcp4Protocol && peer->isConnectedAccepted && !peer->isClosing)
    {
        if (peer->dataToTransmitSize + requestResponseHeader->size() > BUFFER_SIZE)
        {
            // Buffer is full, which indicates a problem
#ifndef NDEBUG
            {
                CHAR16 debugMessage[256];
                setText(debugMessage, L"Warning: Peer transmit buffer overflow. IP: ");
                appendIPv4Address(debugMessage, peer->address);
                appendText(debugMessage, L" | dataToTransmitSize: ");
                appendNumber(debugMessage, peer->dataToTransmitSize, true);
                appendText(debugMessage, L" | requestResponseHeader->size(): ");
                appendNumber(debugMessage, requestResponseHeader->size(), true);
                addDebugMessage(debugMessage);
            }
#endif
            closePeer(peer);
        }
        else
        {
            // Add message to buffer
            copyMem(&peer->dataToTransmit[peer->dataToTransmitSize], requestResponseHeader, requestResponseHeader->size());
            peer->dataToTransmitSize += requestResponseHeader->size();
            peer->trackDejavu(requestResponseHeader->dejavu());
            _InterlockedIncrement64(&numberOfDisseminatedRequests);
        }
    }
}

// Add message to sending buffer of custom filtered (and random) peer, can only called from main thread (not thread-safe).
static void pushCustom(RequestResponseHeader* requestResponseHeader, int numberOfReceivers, bool filterFullNode)
{
    unsigned short suitablePeerIndices[NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS];
    unsigned short numberOfSuitablePeers = 0;
    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].tcp4Protocol && peers[i].isConnectedAccepted && peers[i].exchangedPublicPeers && !peers[i].isClosing)
        {
            if ((filterFullNode && peers[i].isFullNode()) || (!filterFullNode))
            {
                suitablePeerIndices[numberOfSuitablePeers++] = i;
            }
        }
    }
    unsigned short numberOfRemainingSuitablePeers = numberOfReceivers;
    while (numberOfRemainingSuitablePeers-- && numberOfSuitablePeers)
    {
        const unsigned short index = random(numberOfSuitablePeers);
        push(&peers[suitablePeerIndices[index]], requestResponseHeader);
        suitablePeerIndices[index] = suitablePeerIndices[--numberOfSuitablePeers];
    }
}

// Add message to sending buffer of random peer, can only called from main thread (not thread-safe).
static void pushToAny(RequestResponseHeader* requestResponseHeader)
{
    PROFILE_SCOPE();
    const bool filterFullNode = false;
    pushCustom(requestResponseHeader, 1, filterFullNode);
}

// Add message to sending buffer of some(DISSEMINATION_MULTIPLIER) random peers, can only called from main thread (not thread-safe).
static void pushToSeveral(RequestResponseHeader* requestResponseHeader)
{
    PROFILE_SCOPE();
    const bool filterFullNode = false;
    pushCustom(requestResponseHeader, DISSEMINATION_MULTIPLIER, filterFullNode);
}

// Add message to sending buffer of any full node peer, can only called from main thread (not thread-safe).
static void pushToAnyFullNode(RequestResponseHeader* requestResponseHeader)
{
    PROFILE_SCOPE();
    const bool filterFullNode = true;
    pushCustom(requestResponseHeader, 1, filterFullNode);
}

// Add message to sending buffer of some full node peers, can only called from main thread (not thread-safe).
static void pushToSeveralFullNode(RequestResponseHeader* requestResponseHeader)
{
    PROFILE_SCOPE();
    const bool filterFullNode = true;
    pushCustom(requestResponseHeader, DISSEMINATION_MULTIPLIER, filterFullNode);
}

// Add message to sending buffer of some (limit by DISSEMINATION_MULTIPLIER) full node peer, can only called from main thread (not thread-safe).
static void pushToFullNodes(RequestResponseHeader* requestResponseHeader, int numberOfReceivers)
{
    PROFILE_SCOPE();
    if (numberOfReceivers > DISSEMINATION_MULTIPLIER)
    {
        pushToSeveralFullNode(requestResponseHeader);
    }
    else
    {
        const bool filterFullNode = true;
        pushCustom(requestResponseHeader, numberOfReceivers, filterFullNode);
    }
}

// Add message to response queue of specific peer. If peer is NULL, it will be sent to random peers. Can be called from any thread.
static void enqueueResponse(Peer* peer, RequestResponseHeader* responseHeader)
{
    PROFILE_SCOPE();

    ACQUIRE(responseQueueHeadLock);

    if ((responseQueueBufferHead >= responseQueueBufferTail || responseQueueBufferHead + responseHeader->size() < responseQueueBufferTail)
        && (unsigned short)(responseQueueElementHead + 1) != responseQueueElementTail)
    {
        ASSERT(responseQueueElementHead < RESPONSE_QUEUE_LENGTH);
        ASSERT(responseQueueBufferHead < RESPONSE_QUEUE_BUFFER_SIZE);
        ASSERT(responseQueueBufferHead + responseHeader->size() < RESPONSE_QUEUE_BUFFER_SIZE);

        responseQueueElements[responseQueueElementHead].offset = responseQueueBufferHead;
        copyMem(&responseQueueBuffer[responseQueueBufferHead], responseHeader, responseHeader->size());
        responseQueueBufferHead += responseHeader->size();
        responseQueueElements[responseQueueElementHead].peer = peer;
        if (responseQueueBufferHead > RESPONSE_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
        {
            responseQueueBufferHead = 0;
        }
        responseQueueElementHead++;
    }

    RELEASE(responseQueueHeadLock);
}

// Add message to response queue of specific peer. If peer is NULL, it will be sent to random peers. Can be called from any thread.
static void enqueueResponse(Peer* peer, unsigned int dataSize, unsigned char type, unsigned int dejavu, const void* data)
{
    PROFILE_SCOPE();

    ACQUIRE(responseQueueHeadLock);

    if ((responseQueueBufferHead >= responseQueueBufferTail || responseQueueBufferHead + sizeof(RequestResponseHeader) + dataSize < responseQueueBufferTail)
        && (unsigned short)(responseQueueElementHead + 1) != responseQueueElementTail)
    {
        ASSERT(responseQueueElementHead < RESPONSE_QUEUE_LENGTH);
        ASSERT(responseQueueBufferHead < RESPONSE_QUEUE_BUFFER_SIZE);
        ASSERT(responseQueueBufferHead + sizeof(RequestResponseHeader) + dataSize < RESPONSE_QUEUE_BUFFER_SIZE);

        responseQueueElements[responseQueueElementHead].offset = responseQueueBufferHead;
        RequestResponseHeader* responseHeader = (RequestResponseHeader*)&responseQueueBuffer[responseQueueBufferHead];
        if (!responseHeader->checkAndSetSize(sizeof(RequestResponseHeader) + dataSize))
        {
#ifndef NDEBUG
            addDebugMessage(L"Error: Message size exceeds maximum message size!");
#endif
        }
        responseHeader->setType(type);
        responseHeader->setDejavu(dejavu);
        if (data)
        {
            copyMem(&responseQueueBuffer[responseQueueBufferHead + sizeof(RequestResponseHeader)], data, dataSize);
        }
        responseQueueBufferHead += responseHeader->size();
        responseQueueElements[responseQueueElementHead].peer = peer;
        if (responseQueueBufferHead > RESPONSE_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
        {
            responseQueueBufferHead = 0;
        }
        responseQueueElementHead++;
    }

    RELEASE(responseQueueHeadLock);
}

/**
* checks if a given address is a bogon address
* a bogon address is an ip address which should not be used publicly (e.g. private networks)
*
* @param address the ip address to be checked
* @return true if address is bogon or false if not
*/
static bool isBogonAddress(const IPv4Address& address)
{
    return false;
    return (!address.u8[0])
        || (address.u8[0] == 127)
        || (address.u8[0] == 10)
        || (address.u8[0] == 172 && address.u8[1] >= 16 && address.u8[1] <= 31)
        || (address.u8[0] == 192 && address.u8[1] == 168)
        || (address.u8[0] == 255);
}

/**
* checks if a given address was manually set in the initial list of known public peers
*
* @param address the ip address to be checked
* @return true if the ip address is in the Known Public Peers or false if not
*/
static bool isAddressInKnownPublicPeers(const IPv4Address& address)
{
    // keep this exception to avoid bogon addresses kept for outgoing connections
    if (isBogonAddress(address))
        return false;

    for (unsigned int i = 0; i < sizeof(knownPublicPeers) / sizeof(knownPublicPeers[0]); i++)
    {
        const IPv4Address& peer_ip = *reinterpret_cast<const IPv4Address*>(knownPublicPeers[i]);
        if (peer_ip == address)
            return true;
    }
    return false;
}


// Forget public peer (no matter if verified or not) if we have more than the minium number of peers
static void forgetPublicPeer(const IPv4Address& address)
{
    // if address is one of our initial peers we don't forget it
    if (isAddressInKnownPublicPeers(address))
    {
        return;
    }

    if (listOfPeersIsStatic)
    {
        return;
    }

    ACQUIRE(publicPeersLock);

    for (unsigned int i = 0; numberOfPublicPeers > NUMBER_OF_PUBLIC_PEERS_TO_KEEP && i < numberOfPublicPeers; i++)
    {
        if (publicPeers[i].address == address)
        {
            if (i != --numberOfPublicPeers)
            {
                copyMem(&publicPeers[i], &publicPeers[numberOfPublicPeers], sizeof(PublicPeer));
            }

            break;
        }
    }

    RELEASE(publicPeersLock);
}

// Penalize rejected connection by setting verified peer to non-verified or forgetting a non-verified peer
static void penalizePublicPeerRejectedConnection(const IPv4Address& address)
{
    bool forgetPeer = false;

    ACQUIRE(publicPeersLock);

    for (unsigned int i = 0; i < numberOfPublicPeers; i++)
    {
        if (publicPeers[i].address == address)
        {
            if (publicPeers[i].isHandshaked || publicPeers[i].isFullnode)
            {
                publicPeers[i].isHandshaked = false;
                publicPeers[i].isFullnode = false;
            }
            else
            {
                forgetPeer = true;
            }
            break;
        }
    }

    RELEASE(publicPeersLock);

    if (forgetPeer)
    {
        forgetPublicPeer(address);
    }
}


static void addPublicPeer(const IPv4Address& address)
{
    if (isBogonAddress(address)) // not add bogon ip
    {
        return;
    }
    if (address.u32 == 0) // not add zero address
    {
        return;
    }
    for (unsigned int i = 0; i < numberOfPublicPeers; i++)
    {
        if (address == publicPeers[i].address)
        {
            return;
        }
    }

    ACQUIRE(publicPeersLock);

    if (numberOfPublicPeers < MAX_NUMBER_OF_PUBLIC_PEERS)
    {
        publicPeers[numberOfPublicPeers].isHandshaked = false;
        publicPeers[numberOfPublicPeers].isFullnode = false;
        publicPeers[numberOfPublicPeers++].address = address;
    }

    RELEASE(publicPeersLock);
}

static bool peerConnectionNewlyEstablished(unsigned int i)
{
    PROFILE_SCOPE();

    // handle new connections (called in main loop)
    if (((unsigned long long)peers[i].tcp4Protocol)
        && peers[i].connectAcceptToken.CompletionToken.Status != -1)
    {
        peers[i].isConnectingAccepting = FALSE;

        if (i < NUMBER_OF_OUTGOING_CONNECTIONS)
        {
            // outgoing connection
            peers[i].isIncommingConnection = FALSE;
            if (peers[i].connectAcceptToken.CompletionToken.Status)
            {
                // connection rejected
                peers[i].connectAcceptToken.CompletionToken.Status = -1;
                penalizePublicPeerRejectedConnection(peers[i].address);
                closePeer(&peers[i]);
            }
            else
            {
                peers[i].connectAcceptToken.CompletionToken.Status = -1;
                if (peers[i].isClosing)
                {
                    closePeer(&peers[i]);
                }
                else
                {
                    peers[i].isConnectedAccepted = TRUE;
                }
            }
        }
        else
        {
            // incoming connection
            peers[i].isIncommingConnection = TRUE;
            if (peers[i].connectAcceptToken.CompletionToken.Status)
            {
                // connection error
                peers[i].connectAcceptToken.CompletionToken.Status = -1;
                peers[i].tcp4Protocol = NULL;
            }
            else
            {
                peers[i].connectAcceptToken.CompletionToken.Status = -1;
                if (peers[i].isClosing)
                {
                    closePeer(&peers[i]);
                }
                else
                {
                    EFI_STATUS status = bs->OpenProtocol(peers[i].connectAcceptToken.NewChildHandle, &tcp4ProtocolGuid, (void**)&peers[i].tcp4Protocol, ih, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
                    if (status)
                    {
                        logStatusToConsole(L"EFI_BOOT_SERVICES.OpenProtocol() fails", status, __LINE__);

                        tcp4ServiceBindingProtocol->DestroyChild(tcp4ServiceBindingProtocol, peers[i].connectAcceptToken.NewChildHandle);
                        peers[i].tcp4Protocol = NULL;
                    }
                    else
                    {
                        /* GetModeData() freezes the node occasionally: the quick-fix is to disable whitelisting
                        // If number of unused incoming connection slots is low, only accept white list IPs
                        if (NUMBER_OF_INCOMING_CONNECTIONS - numberOfAcceptedIncommingConnection < NUMBER_OF_INCOMING_CONNECTIONS_RESERVED_FOR_WHITELIST_IPS)
                        {
                            EFI_TCP4_CONFIG_DATA tcp4ConfigData;
                            if (peers[i].tcp4Protocol
                                && !peers[i].tcp4Protocol->GetModeData(peers[i].tcp4Protocol, NULL, &tcp4ConfigData, NULL, NULL, NULL))
                            {
                                if (!isWhiteListPeer(tcp4ConfigData.AccessPoint.RemoteAddress.Addr))
                                {
                                    closePeer(&peers[i]);
                                    return false;
                                }
                                else
                                {
                                    peers[i].isConnectedAccepted = TRUE;
                                    peers[i].address.u32 = *(unsigned int*)tcp4ConfigData.AccessPoint.RemoteAddress.Addr;
                                }
                            }
                            else
                            {
                                closePeer(&peers[i]);
                                return false;
                            }
                        }
                        else
                        */
                        {
                            peers[i].isConnectedAccepted = TRUE;
                        }
                    }
                }
            }
        }
        // new connection has been established
        if (peers[i].isConnectedAccepted)
        {
            if (peers[i].isIncommingConnection)
            {
                numberOfAcceptedIncommingConnection++;
                ASSERT(numberOfAcceptedIncommingConnection <= NUMBER_OF_INCOMING_CONNECTIONS);
            }
            return true;
        }
    }
    return false;
}

// This function process all data that arrive in FragmentBuffer.
// based on RequestResponseHeader to determine whether the received packet is completed or not
// if it receives a completed packet, it will copy the packet to requestQueueElements to process later in requestProcessors
static void processReceivedData(unsigned int i, unsigned int salt)
{
    PROFILE_SCOPE();

    if (((unsigned long long)peers[i].tcp4Protocol) > 1)
    {
        if (peers[i].receiveToken.CompletionToken.Status != -1)
        {
            peers[i].isReceiving = FALSE;
            if (peers[i].receiveToken.CompletionToken.Status)
            {
                peers[i].receiveToken.CompletionToken.Status = -1;
                closePeer(&peers[i]);
            }
            else
            {
                peers[i].receiveToken.CompletionToken.Status = -1;
                if (peers[i].isClosing)
                {
                    closePeer(&peers[i]);
                }
                else
                {
                    numberOfReceivedBytes += peers[i].receiveData.DataLength;
                    *((unsigned long long*) & peers[i].receiveData.FragmentTable[0].FragmentBuffer) += peers[i].receiveData.DataLength;

                iteration:
                    unsigned int receivedDataSize = (unsigned int)(((unsigned long long)peers[i].receiveData.FragmentTable[0].FragmentBuffer) - ((unsigned long long)peers[i].receiveBuffer));

                    if (receivedDataSize >= sizeof(RequestResponseHeader))
                    {
                        RequestResponseHeader* requestResponseHeader = (RequestResponseHeader*)peers[i].receiveBuffer;
                        if (requestResponseHeader->size() < sizeof(RequestResponseHeader))
                        {
                            // protocol violation -> forget peer
                            setText(message, L"Forgetting ");
                            appendIPv4Address(message, peers[i].address);
                            appendText(message, L"...");
                            logToConsole(message);
                            forgetPublicPeer(peers[i].address);
                            closePeer(&peers[i]);
                        }
                        else
                        {
                            if (receivedDataSize >= requestResponseHeader->size())
                            {
                                // Compute saltId of packet with K12 of payload and header (size + type temporarily
                                // overwritten with salt). This is used recognized and skip packet duplicates with
                                // dejavu0 (checking/setting flag for received package). After receiving a certain
                                // number of packages (DEJAVU_SWAP_LIMIT), dejavu0 is moved to dejavu1 for checking
                                // and dejavu0 is initialized with an empty buffer for checking/setting.
                                unsigned int saltedId;
                                const unsigned int header = *((unsigned int*)requestResponseHeader);
                                *((unsigned int*)requestResponseHeader) = salt;
                                KangarooTwelve(requestResponseHeader, header & 0xFFFFFF, &saltedId, sizeof(saltedId));
                                *((unsigned int*)requestResponseHeader) = header;

                                // Initiate transfer of already received packet to processing thread
                                // (or drop it without processing if Dejavu filter tells to ignore it)
                                if (!((dejavu0[saltedId >> 6] | dejavu1[saltedId >> 6]) & (1ULL << (saltedId & 63))))
                                {
                                    if ((requestQueueBufferHead >= requestQueueBufferTail || requestQueueBufferHead + requestResponseHeader->size() < requestQueueBufferTail)
                                        && (unsigned short)(requestQueueElementHead + 1) != requestQueueElementTail)
                                    {
                                        dejavu0[saltedId >> 6] |= (1ULL << (saltedId & 63));

                                        ASSERT(requestQueueElementHead < REQUEST_QUEUE_LENGTH);
                                        ASSERT(requestQueueBufferHead < REQUEST_QUEUE_BUFFER_SIZE);
                                        ASSERT(requestQueueBufferHead + requestResponseHeader->size() < REQUEST_QUEUE_BUFFER_SIZE);

                                        requestQueueElements[requestQueueElementHead].offset = requestQueueBufferHead;
                                        copyMem(&requestQueueBuffer[requestQueueBufferHead], peers[i].receiveBuffer, requestResponseHeader->size());
                                        requestQueueBufferHead += requestResponseHeader->size();
                                        requestQueueElements[requestQueueElementHead].peer = &peers[i];
                                        if (requestQueueBufferHead > REQUEST_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
                                        {
                                            requestQueueBufferHead = 0;
                                        }
                                        // TODO: Place a fence
                                        requestQueueElementHead++;

                                        if (!(--dejavuSwapCounter))
                                        {
                                            unsigned long long* tmp = dejavu1;
                                            dejavu1 = dejavu0;
                                            setMem(dejavu0 = tmp, 536870912, 0);
                                            dejavuSwapCounter = DEJAVU_SWAP_LIMIT;
                                        }
                                    }
                                    else
                                    {
                                        _InterlockedIncrement64(&numberOfDiscardedRequests);

                                        enqueueResponse(&peers[i], 0, TryAgain::type, requestResponseHeader->dejavu(), NULL);
                                    }
                                }
                                else
                                {
                                    _InterlockedIncrement64(&numberOfDuplicateRequests);
                                }

                                copyMem(peers[i].receiveBuffer, ((char*)peers[i].receiveBuffer) + requestResponseHeader->size(), receivedDataSize -= requestResponseHeader->size());
                                peers[i].receiveData.FragmentTable[0].FragmentBuffer = ((char*)peers[i].receiveBuffer) + receivedDataSize;

                                goto iteration;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Signaling the system to receive data from this connection
static void receiveData(unsigned int i, unsigned int salt)
{
    PROFILE_SCOPE();

    EFI_STATUS status;
    if (((unsigned long long)peers[i].tcp4Protocol) > 1)
    {
        if (!peers[i].isReceiving && peers[i].isConnectedAccepted && !peers[i].isClosing)
        {
            // check that receive buffer has enough space (less than BUFFER_SIZE is used)
            if ((((unsigned long long)peers[i].receiveData.FragmentTable[0].FragmentBuffer) - ((unsigned long long)peers[i].receiveBuffer)) < BUFFER_SIZE)
            {
                unsigned int receiveSize = BUFFER_SIZE - (unsigned int)(((unsigned long long)peers[i].receiveData.FragmentTable[0].FragmentBuffer) - ((unsigned long long)peers[i].receiveBuffer));
                peers[i].receiveData.DataLength = receiveSize;
                peers[i].receiveData.FragmentTable[0].FragmentLength = receiveSize;
                if (peers[i].receiveData.DataLength)
                {
                    EFI_TCP4_CONNECTION_STATE state;
                    if ((status = peers[i].tcp4Protocol->GetModeData(peers[i].tcp4Protocol, &state, NULL, NULL, NULL, NULL))
                        || state == Tcp4StateClosed)
                    {
                        closePeer(&peers[i]);
                    }
                    else
                    {
                        // Initiate receiving data. At the same moment buffer may already contain a message of
                        // up to 16MB size, we don't wait for this message to be passed on to processing thread.
                        if (status = peers[i].tcp4Protocol->Receive(peers[i].tcp4Protocol, &peers[i].receiveToken))
                        {
                            if (status != EFI_CONNECTION_FIN)
                            {
                                logStatusToConsole(L"EFI_TCP4_PROTOCOL.Receive() fails", status, __LINE__);
                            }

                            closePeer(&peers[i]);
                        }
                        else
                        {
                            peers[i].isReceiving = TRUE;
                        }
                    }
                }
            }
        }
    }
}

// Checking the status last queued data for transmitting
static void processTransmittedData(unsigned int i, unsigned int salt)
{
    PROFILE_SCOPE();

    if (((unsigned long long)peers[i].tcp4Protocol) > 1)
    {
        // check if transmission is completed
        if (peers[i].transmitToken.CompletionToken.Status != -1)
        {
            peers[i].isTransmitting = FALSE;
            if (peers[i].transmitToken.CompletionToken.Status)
            {
                // transmission error
                peers[i].transmitToken.CompletionToken.Status = -1;
                closePeer(&peers[i]);
            }
            else
            {
                peers[i].transmitToken.CompletionToken.Status = -1;
                if (peers[i].isClosing)
                {
                    closePeer(&peers[i]);
                }
                else
                {
                    // success
                    numberOfTransmittedBytes += peers[i].transmitData.DataLength;
                }
            }
        }
    }
}

// Enqueue data for transmitting
static void transmitData(unsigned int i, unsigned int salt)
{
    PROFILE_SCOPE();

    EFI_STATUS status;
    if (((unsigned long long)peers[i].tcp4Protocol) > 1)
    {
        if (peers[i].dataToTransmitSize && !peers[i].isTransmitting && peers[i].isConnectedAccepted && !peers[i].isClosing)
        {
            EFI_TCP4_CONNECTION_STATE state;
            if ((status = peers[i].tcp4Protocol->GetModeData(peers[i].tcp4Protocol, &state, NULL, NULL, NULL, NULL))
                || state == Tcp4StateClosed)
            {
                closePeer(&peers[i]);
            }
            else
            {
                // initiate transmission
                copyMem(peers[i].transmitData.FragmentTable[0].FragmentBuffer, peers[i].dataToTransmit, peers[i].transmitData.DataLength = peers[i].transmitData.FragmentTable[0].FragmentLength = peers[i].dataToTransmitSize);
                peers[i].dataToTransmitSize = 0;
                if (status = peers[i].tcp4Protocol->Transmit(peers[i].tcp4Protocol, &peers[i].transmitToken))
                {
                    logStatusToConsole(L"EFI_TCP4_PROTOCOL.Transmit() fails", status, __LINE__);

                    closePeer(&peers[i]);
                }
                else
                {
                    peers[i].isTransmitting = TRUE;
                }
            }
        }

    }
}

static void peerReceiveAndTransmit(unsigned int i, unsigned int salt)
{
    // poll to receive incoming data and transmit outgoing segments
    if (((unsigned long long)peers[i].tcp4Protocol) > 1)
    {
        PROFILE_SCOPE();
        peers[i].tcp4Protocol->Poll(peers[i].tcp4Protocol);
        processReceivedData(i, salt);
        receiveData(i, salt);
        processTransmittedData(i, salt);
        transmitData(i, salt);
    }
}

static void peerReconnectIfInactive(unsigned int i, unsigned short port)
{
    PROFILE_SCOPE();

    EFI_STATUS status;
    if (!peers[i].tcp4Protocol)
    {
        peers[i].reset();
        // peer slot without active connection
        if (i < NUMBER_OF_OUTGOING_CONNECTIONS)
        {
            // outgoing connection:
            // randomly select public peer and try to connect if we do not
            // yet have an outgoing connection to it
            peers[i].address = publicPeers[random(numberOfPublicPeers)].address;
            peers[i].isIncommingConnection = FALSE;

            if (peers[i].address.u32 != 0)
            {
                unsigned int j;
                for (j = 0; j < NUMBER_OF_OUTGOING_CONNECTIONS; j++)
                {
                    if (peers[j].tcp4Protocol && peers[j].address == peers[i].address)
                    {
                        break;
                    }
                }
                if (j == NUMBER_OF_OUTGOING_CONNECTIONS)
                {
                    if (peers[i].connectAcceptToken.NewChildHandle = getTcp4Protocol(peers[i].address.u8, port, &peers[i].tcp4Protocol))
                    {
                        peers[i].receiveData.FragmentTable[0].FragmentBuffer = peers[i].receiveBuffer;

                        if (status = peers[i].tcp4Protocol->Connect(peers[i].tcp4Protocol, (EFI_TCP4_CONNECTION_TOKEN*)&peers[i].connectAcceptToken))
                        {
                            logStatusToConsole(L"EFI_TCP4_PROTOCOL.Connect() fails", status, __LINE__);

                            bs->CloseProtocol(peers[i].connectAcceptToken.NewChildHandle, &tcp4ProtocolGuid, ih, NULL);
                            tcp4ServiceBindingProtocol->DestroyChild(tcp4ServiceBindingProtocol, peers[i].connectAcceptToken.NewChildHandle);
                            peers[i].tcp4Protocol = NULL;
                        }
                        else
                        {
                            peers[i].isConnectingAccepting = TRUE;
                        }
                    }
                    else
                    {
                        peers[i].tcp4Protocol = NULL;
                    }
                }
            }
        }
        else
        {
            // incoming connection:
            // accept connections if peer list is not static
            if (!listOfPeersIsStatic)
            {
                peers[i].isIncommingConnection = TRUE;
                peers[i].receiveData.FragmentTable[0].FragmentBuffer = peers[i].receiveBuffer;

                if (status = peerTcp4Protocol->Accept(peerTcp4Protocol, &peers[i].connectAcceptToken))
                {
                    logStatusToConsole(L"EFI_TCP4_PROTOCOL.Accept() fails", status, __LINE__);
                    if (peerTcp4Protocol == NULL)
                    {
                        logToConsole(L"[CRITICAL-Report to dev] peerTcp4Protocol is NULL. Please report!");
                    }
                    else
                    {
                        logStatusToConsole(L"[CRITICAL-Report to dev] peers[i].connectAcceptToken.CompletionToken.Status", peers[i].connectAcceptToken.CompletionToken.Status, __LINE__);
                    }
                    unsigned long long ptr = (unsigned long long)(&peers[i].connectAcceptToken);
                    setText(message, L"[CRITICAL-Report to dev] &peers[i].connectAcceptToken: ");
                    appendNumber(message, ptr, false);
                    logToConsole(message);
                }
                else
                {
                    peers[i].isConnectingAccepting = TRUE;
                    peers[i].tcp4Protocol = (EFI_TCP4_PROTOCOL*)1;
                }
            }
        }
    }
}
