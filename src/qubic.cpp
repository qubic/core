#define SINGLE_COMPILE_UNIT

// contract_def.h needs to be included first to make sure that contracts have minimal access
#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include <lib/platform_common/qintrin.h>

#include "network_messages/all.h"

#include "private_settings.h"
#include "public_settings.h"



////////// C++ helpers \\\\\\\\\\

#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/concurrency_impl.h"
// TODO: Use "long long" instead of "int" for DB indices


#include <lib/platform_efi/uefi.h>
#include <lib/platform_common/processor.h>
#include <lib/platform_common/compiler_optimization.h>
#include "platform/time.h"
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"
#include "platform/memory_util.h"
#include "platform/profiling.h"

#include "platform/custom_stack.h"

#include "text_output.h"

#include "K12/kangaroo_twelve_xkcp.h"
#include "kangaroo_twelve.h"
#include "four_q.h"
#include "score.h"

#include "network_core/tcp4.h"
#include "network_core/peers.h"

#include "system.h"
#include "contract_core/qpi_system_impl.h"

#include "assets/assets.h"
#include "assets/net_msg_impl.h"
#include "contract_core/qpi_asset_impl.h"

#include "spectrum/spectrum.h"
#include "contract_core/qpi_spectrum_impl.h"

#include "logging/logging.h"
#include "logging/net_msg_impl.h"

#include "ticking/ticking.h"
#include "contract_core/qpi_ticking_impl.h"
#include "vote_counter.h"

#include "contract_core/ipo.h"
#include "contract_core/qpi_ipo_impl.h"

#include "addons/tx_status_request.h"

#include "files/files.h"
#include "mining/mining.h"
#include "oracles/oracle_machines.h"

#include "contract_core/qpi_mining_impl.h"
#include "revenue.h"

////////// Qubic \\\\\\\\\\

#define CONTRACT_STATES_DEPTH 10 // Is derived from MAX_NUMBER_OF_CONTRACTS (=N)
#define TICK_REQUESTING_PERIOD 500ULL
#define MAX_NUMBER_EPOCH 1000ULL
#define MAX_NUMBER_OF_MINERS 8192
#define NUMBER_OF_MINER_SOLUTION_FLAGS 0x100000000
#define MAX_MESSAGE_PAYLOAD_SIZE MAX_TRANSACTION_SIZE
#define MAX_UNIVERSE_SIZE 1073741824
#define MESSAGE_DISSEMINATION_THRESHOLD 1000000000
#define PORT 21841
#define SYSTEM_DATA_SAVING_PERIOD 300000ULL
#define TICK_TRANSACTIONS_PUBLICATION_OFFSET 2 // Must be only 2
#define MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET 3 // Must be 3+
#define TIME_ACCURACY 5000
constexpr unsigned long long TARGET_MAINTHREAD_LOOP_DURATION = 30; // mcs, it is the target duration of the main thread loop


struct Processor : public CustomStack
{
    enum Type { Unused = 0, RequestProcessor, TickProcessor, ContractProcessor };
    Type type;
    EFI_EVENT event;
    Peer* peer;
    void* buffer;
};




static volatile int shutDownNode = 0;
static volatile unsigned char mainAuxStatus = 0;
static volatile unsigned char isVirtualMachine = 0; // indicate that it is running on VM, to avoid running some functions for BM  (for testing and developing purposes)
static volatile bool forceRefreshPeerList = false;
static volatile bool forceNextTick = false;
static volatile bool forceSwitchEpoch = false;
static volatile char criticalSituation = 0;
static volatile bool systemMustBeSaved = false, spectrumMustBeSaved = false, universeMustBeSaved = false, computerMustBeSaved = false;

static int misalignedState = 0;

static volatile unsigned char epochTransitionState = 0;
static volatile unsigned char epochTransitionCleanMemoryFlag = 1;
static volatile long epochTransitionWaitingRequestProcessors = 0;

// data closely related to system
static int solutionPublicationTicks[MAX_NUMBER_OF_SOLUTIONS]; // scheduled tick to broadcast solution, -1 means already broadcasted, -2 means obsolete solution
#define SOLUTION_RECORDED_FLAG -1
#define SOLUTION_OBSOLETE_FLAG -2

static unsigned long long faultyComputorFlags[(NUMBER_OF_COMPUTORS + 63) / 64];
static unsigned int gTickNumberOfComputors = 0, gTickTotalNumberOfComputors = 0, gFutureTickTotalNumberOfComputors = 0;
static unsigned int nextTickTransactionsSemaphore = 0, numberOfNextTickTransactions = 0, numberOfKnownNextTickTransactions = 0;
static unsigned short numberOfOwnComputorIndices;
static unsigned short ownComputorIndices[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static unsigned short ownComputorIndicesMapping[sizeof(computorSeeds) / sizeof(computorSeeds[0])];

static TickStorage ts;
static VoteCounter voteCounter;
static TickData nextTickData;

static m256i uniqueNextTickTransactionDigests[NUMBER_OF_COMPUTORS];
static unsigned int uniqueNextTickTransactionDigestCounters[NUMBER_OF_COMPUTORS];

static unsigned int resourceTestingDigest = 0;

static unsigned int numberOfTransactions = 0;
static volatile char entityPendingTransactionsLock = 0;
static unsigned char* entityPendingTransactions = NULL;
static unsigned char* entityPendingTransactionDigests = NULL;
static unsigned int entityPendingTransactionIndices[SPECTRUM_CAPACITY]; // [SPECTRUM_CAPACITY] must be >= than [NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR]
static volatile char computorPendingTransactionsLock = 0;
static unsigned char* computorPendingTransactions = NULL;
static unsigned char* computorPendingTransactionDigests = NULL;
static unsigned long long spectrumChangeFlags[SPECTRUM_CAPACITY / (sizeof(unsigned long long) * 8)];

static unsigned long long mainLoopNumerator = 0, mainLoopDenominator = 0;
static unsigned char contractProcessorState = 0;
static unsigned int contractProcessorPhase;
static const Transaction* contractProcessorTransaction = 0; // does not have signature in some cases, see notifyContractOfIncomingTransfer()
static int contractProcessorTransactionMoneyflew = 0;
static unsigned char contractProcessorPostIncomingTransferType = 0;
static EFI_EVENT contractProcessorEvent;
static m256i contractStateDigests[MAX_NUMBER_OF_CONTRACTS * 2 - 1];
const unsigned long long contractStateDigestsSizeInBytes = sizeof(contractStateDigests);

// targetNextTickDataDigestIsKnown == true signals that we need to fetch TickData (update the version in this node)
// targetNextTickDataDigestIsKnown == false means there is no consensus on next tick data yet
static bool targetNextTickDataDigestIsKnown = false;
static m256i targetNextTickDataDigest;
static m256i lastExpectedTickTransactionDigest;
XKCP::KangarooTwelve_Instance g_k12_instance;
// rdtsc (timestamp) of ticks
static unsigned long long tickTicks[11];

static unsigned int numberOfProcessors = 0;
static Processor processors[MAX_NUMBER_OF_PROCESSORS];

// Variables for tracking the detail of processors(CPU core) and function, this is useful for resource management and debugging
static unsigned long long tickProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function tickProcessor
static unsigned long long requestProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function requestProcessor
static unsigned long long contractProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function contractProcessor

static unsigned long long solutionProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that will process solution
static bool solutionProcessorFlags[MAX_NUMBER_OF_PROCESSORS]; // flag array to indicate that whether a procId should help processing solutions or not
static int nTickProcessorIDs = 0;
static int nRequestProcessorIDs = 0;
static int nContractProcessorIDs = 0;
static int nSolutionProcessorIDs = 0;

static ScoreFunction<
    NUMBER_OF_INPUT_NEURONS,
    NUMBER_OF_OUTPUT_NEURONS,
    NUMBER_OF_TICKS,
    NUMBER_OF_NEIGHBORS,
    POPULATION_THRESHOLD,
    NUMBER_OF_MUTATIONS,
    SOLUTION_THRESHOLD_DEFAULT,
    NUMBER_OF_SOLUTION_PROCESSORS
> * score = nullptr;
static volatile char solutionsLock = 0;
static unsigned long long* minerSolutionFlags = NULL;
static volatile m256i minerPublicKeys[MAX_NUMBER_OF_MINERS + 1];
static volatile unsigned int minerScores[MAX_NUMBER_OF_MINERS + 1];
static volatile unsigned int numberOfMiners = NUMBER_OF_COMPUTORS;
static m256i competitorPublicKeys[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
static unsigned int competitorScores[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
static bool competitorComputorStatuses[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
static unsigned int minimumComputorScore = 0, minimumCandidateScore = 0;
static int solutionThreshold[MAX_NUMBER_EPOCH] = { -1 };
static unsigned long long solutionTotalExecutionTicks = 0;
static unsigned long long K12MeasurementsCount = 0;
static unsigned long long K12MeasurementsSum = 0;
static volatile char minerScoreArrayLock = 0;
static SpecialCommandGetMiningScoreRanking<MAX_NUMBER_OF_MINERS> requestMiningScoreRanking;

// Custom mining related variables and constants
static unsigned int gCustomMiningSharesCount[NUMBER_OF_COMPUTORS] = { 0 };
static CustomMiningSharesCounter gCustomMiningSharesCounter;

// variables and declare for persisting state
static volatile int requestPersistingNodeState = 0;
static volatile int persistingNodeStateTickProcWaiting = 0;
static m256i initialRandomSeedFromPersistingState;
static bool loadMiningSeedFromFile = false;
static bool loadAllNodeStateFromFile = false;
#if TICK_STORAGE_AUTOSAVE_MODE
static unsigned int nextPersistingNodeStateTick = 0;
struct
{
    Tick etalonTick;
    m256i minerPublicKeys[MAX_NUMBER_OF_MINERS + 1];
    unsigned int minerScores[MAX_NUMBER_OF_MINERS + 1];
    m256i competitorPublicKeys[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
    unsigned int competitorScores[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
    bool competitorComputorStatuses[(NUMBER_OF_COMPUTORS - QUORUM) * 2];
    m256i currentRandomSeed;    
    int solutionPublicationTicks[MAX_NUMBER_OF_SOLUTIONS];
    unsigned long long faultyComputorFlags[(NUMBER_OF_COMPUTORS + 63) / 64];
    unsigned char voteCounterData[VoteCounter::VoteCounterDataSize];
    BroadcastComputors broadcastedComputors;
    unsigned int resourceTestingDigest;
    unsigned int numberOfMiners;
    unsigned int numberOfTransactions;
    unsigned char customMiningSharesCounterData[CustomMiningSharesCounter::_customMiningSolutionCounterDataSize];
} nodeStateBuffer;
#endif
static bool saveComputer(CHAR16* directory = NULL);
static bool saveSystem(CHAR16* directory = NULL);
static bool loadComputer(CHAR16* directory = NULL, bool forceLoadFromFile = false);
static bool saveRevenueComponents(CHAR16* directory = NULL);

#if ENABLED_LOGGING
#define PAUSE_BEFORE_CLEAR_MEMORY 1 // Requiring operators to press F10 to clear memory (before switching epoch)
#else
#define PAUSE_BEFORE_CLEAR_MEMORY 0
#endif

BroadcastFutureTickData broadcastedFutureTickData;

static struct
{
    Transaction transaction;
    unsigned char data[VOTE_COUNTER_DATA_SIZE_IN_BYTES];
    m256i dataLock;
    unsigned char signature[SIGNATURE_SIZE];
} voteCounterPayload;


static struct
{
    RequestResponseHeader header;
} requestedComputors;

static struct
{
    RequestResponseHeader header;
    RequestQuorumTick requestQuorumTick;
} requestedQuorumTick;

static struct
{
    RequestResponseHeader header;
    RequestTickData requestTickData;
} requestedTickData;

static struct
{
    RequestResponseHeader header;
    RequestedTickTransactions requestedTickTransactions;
} requestedTickTransactions;

static struct {
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} threadTimeCheckin[MAX_NUMBER_OF_PROCESSORS];

static struct {
    unsigned int tick;
    unsigned long long clock;
    unsigned long long lastTryClock; // last time it rolling the dice
} emptyTickResolver;

static struct {
    static constexpr unsigned long long MAX_WAITING_TIME = 60000; // time to trigger resending tick votes
    unsigned int lastTick;
    unsigned int lastTickMode; // 0 AUX - 1 MAIN
    unsigned long long lastCheck;
} autoResendTickVotes;

static void logToConsole(const CHAR16* message)
{
    if (consoleLoggingLevel == 0)
    {
        return;
    }

    timestampedMessage[0] = (utcTime.Year % 100) / 10 + L'0';
    timestampedMessage[1] = utcTime.Year % 10 + L'0';
    timestampedMessage[2] = utcTime.Month / 10 + L'0';
    timestampedMessage[3] = utcTime.Month % 10 + L'0';
    timestampedMessage[4] = utcTime.Day / 10 + L'0';
    timestampedMessage[5] = utcTime.Day % 10 + L'0';
    timestampedMessage[6] = utcTime.Hour / 10 + L'0';
    timestampedMessage[7] = utcTime.Hour % 10 + L'0';
    timestampedMessage[8] = utcTime.Minute / 10 + L'0';
    timestampedMessage[9] = utcTime.Minute % 10 + L'0';
    timestampedMessage[10] = utcTime.Second / 10 + L'0';
    timestampedMessage[11] = utcTime.Second % 10 + L'0';
    timestampedMessage[12] = ' ';
    timestampedMessage[13] = 0;

    appendNumber(timestampedMessage, gTickNumberOfComputors / 100, FALSE);
    appendNumber(timestampedMessage, (gTickNumberOfComputors % 100) / 10, FALSE);
    appendNumber(timestampedMessage, gTickNumberOfComputors % 10, FALSE);
    appendText(timestampedMessage, L":");
    appendNumber(timestampedMessage, (gTickTotalNumberOfComputors - gTickNumberOfComputors) / 100, FALSE);
    appendNumber(timestampedMessage, ((gTickTotalNumberOfComputors - gTickNumberOfComputors) % 100) / 10, FALSE);
    appendNumber(timestampedMessage, (gTickTotalNumberOfComputors - gTickNumberOfComputors) % 10, FALSE);
    appendText(timestampedMessage, L"(");
    appendNumber(timestampedMessage, gFutureTickTotalNumberOfComputors / 100, FALSE);
    appendNumber(timestampedMessage, (gFutureTickTotalNumberOfComputors % 100) / 10, FALSE);
    appendNumber(timestampedMessage, gFutureTickTotalNumberOfComputors % 10, FALSE);
    appendText(timestampedMessage, L").");
    appendNumber(timestampedMessage, system.tick, FALSE);
    appendText(timestampedMessage, L".");
    appendNumber(timestampedMessage, system.epoch, FALSE);
    appendText(timestampedMessage, L" ");

    appendText(timestampedMessage, message);
    appendText(timestampedMessage, L"\r\n");

#ifdef NDEBUG
    outputStringToConsole(timestampedMessage);
#else
    bool logAsDebugMessage = epochTransitionState
                                || system.tick - system.initialTick < 3
                                || system.tick % 10 == 0
                                || misalignedState == 2
                                || forceLogToConsoleAsAddDebugMessage
        ;
    if (logAsDebugMessage)
        addDebugMessage(timestampedMessage);
    else
        outputStringToConsole(timestampedMessage);
#endif
}

static int computorIndex(m256i computor)
{
    for (int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        if (broadcastedComputors.computors.publicKeys[computorIndex] == computor)
        {
            return computorIndex;
        }
    }

    return -1;
}

static inline bool isMainMode()
{
    return (mainAuxStatus & 1) == 1;
}

// NOTE: this function doesn't work well on a few CPUs, some bits will be flipped after calling this. It's probably microcode bug.
static void enableAVX()
{
    __writecr4(__readcr4() | 0x40000);
    _xsetbv(_XCR_XFEATURE_ENABLED_MASK, _xgetbv(_XCR_XFEATURE_ENABLED_MASK) | (7
#ifdef __AVX512F__
        | 224
#endif
        ));
}

// Should only be called from tick processor to avoid concurrent state changes, which can cause race conditions as detailed in FIXME below.
static void getComputerDigest(m256i& digest)
{
    PROFILE_SCOPE();

    unsigned int digestIndex;
    for (digestIndex = 0; digestIndex < MAX_NUMBER_OF_CONTRACTS; digestIndex++)
    {
        if (contractStateChangeFlags[digestIndex >> 6] & (1ULL << (digestIndex & 63)))
        {
            const unsigned long long size = digestIndex < contractCount ? contractDescriptions[digestIndex].stateSize : 0;
            if (!size)
            {
                contractStateDigests[digestIndex] = m256i::zero();
            }
            else
            {
                // FIXME: We may have a race condition here if a digest is computed here by thread A, the state is changed
                // + contractStateChangeFlags set afterwards by thread B and contractStateChangeFlags cleared below below
                // by thread A. We then have a changed state but a cleared contractStateChangeFlags flag leading to wrong
                // digest.
                // This is currently avoided by calling getComputerDigest() from tick processor only (and in non-concurrent init)
                contractStateLock[digestIndex].acquireRead();

                const unsigned long long startTick = __rdtsc();
                KangarooTwelve(contractStates[digestIndex], (unsigned int)size, &contractStateDigests[digestIndex], 32);
                const unsigned long long executionTicks = __rdtsc() - startTick;

                contractStateLock[digestIndex].releaseRead();

                // K12 of state is included in contract execution time
                _interlockedadd64(&contractTotalExecutionTicks[digestIndex], executionTicks);

                // Gather data for comparing different versions of K12
                if (K12MeasurementsCount < 500)
                {
                    K12MeasurementsSum += executionTicks;
                    K12MeasurementsCount++;
                }
            }
        }
    }
    unsigned int previousLevelBeginning = 0;
    unsigned int numberOfLeafs = MAX_NUMBER_OF_CONTRACTS;
    while (numberOfLeafs > 1)
    {
        for (unsigned int i = 0; i < numberOfLeafs; i += 2)
        {
            if (contractStateChangeFlags[i >> 6] & (3ULL << (i & 63)))
            {
                KangarooTwelve64To32(&contractStateDigests[previousLevelBeginning + i], &contractStateDigests[digestIndex]);
                contractStateChangeFlags[i >> 6] &= ~(3ULL << (i & 63));
                contractStateChangeFlags[i >> 7] |= (1ULL << ((i >> 1) & 63));
            }
            digestIndex++;
        }
        previousLevelBeginning += numberOfLeafs;
        numberOfLeafs >>= 1;
    }
    contractStateChangeFlags[0] = 0;

    digest = contractStateDigests[(MAX_NUMBER_OF_CONTRACTS * 2 - 1) - 1];
}


static void processExchangePublicPeers(Peer* peer, RequestResponseHeader* header)
{
    if (!peer->exchangedPublicPeers)
    {
        peer->exchangedPublicPeers = TRUE; // A race condition is possible

        // Set isHandshaked if sExchangePublicPeers was received on outgoing connection
        if (peer->address.u32)
        {
            for (unsigned int j = 0; j < numberOfPublicPeers; j++)
            {
                if (peer->address == publicPeers[j].address)
                {
                    publicPeers[j].isHandshaked = true;

                    break;
                }
            }
        }
    }

    ExchangePublicPeers* request = header->getPayload<ExchangePublicPeers>();
    for (unsigned int j = 0; j < NUMBER_OF_EXCHANGED_PEERS && numberOfPublicPeers < MAX_NUMBER_OF_PUBLIC_PEERS; j++)
    {
        if (!listOfPeersIsStatic)
        {
            addPublicPeer(request->peers[j]);
        }
    }
}

static void processBroadcastMessage(const unsigned long long processorNumber, RequestResponseHeader* header)
{
    BroadcastMessage* request = header->getPayload<BroadcastMessage>();
    if (header->size() <= sizeof(RequestResponseHeader) + sizeof(BroadcastMessage) + MAX_MESSAGE_PAYLOAD_SIZE + SIGNATURE_SIZE
        && header->size() >= sizeof(RequestResponseHeader) + sizeof(BroadcastMessage) + SIGNATURE_SIZE
        && !isZero(request->sourcePublicKey))
    {
        const unsigned int messageSize = header->size() - sizeof(RequestResponseHeader);
        bool ok = true;
        m256i digest;
        KangarooTwelve(request, messageSize - SIGNATURE_SIZE, &digest, sizeof(digest));
        if (verify(request->sourcePublicKey.m256i_u8, digest.m256i_u8, (((const unsigned char*)request) + (messageSize - SIGNATURE_SIZE))))
        {

            // Balance checking
            bool hasEnoughBalance = false;
            const int spectrumIndex = ::spectrumIndex(request->sourcePublicKey);
            if (spectrumIndex >= 0 && energy(spectrumIndex) >= MESSAGE_DISSEMINATION_THRESHOLD)
            {
                hasEnoughBalance = true;
            }

            // Broadcast message if balance is enough or this message has been signed by a computor
            if ((hasEnoughBalance || computorIndex(request->sourcePublicKey) >=0 || request->sourcePublicKey == dispatcherPublicKey) && header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            if (isZero(request->destinationPublicKey))
            {
                const unsigned int messagePayloadSize = messageSize - sizeof(BroadcastMessage) - SIGNATURE_SIZE;

                // Only record task and solution message in idle phase
                char recordCustomMining = 0;
                ACQUIRE(gIsInCustomMiningStateLock);
                recordCustomMining = gIsInCustomMiningState;
                RELEASE(gIsInCustomMiningStateLock);

                if (messagePayloadSize == sizeof(CustomMiningTask) && request->sourcePublicKey == dispatcherPublicKey)
                {
                    // See CustomMiningTaskMessage structure
                    // MESSAGE_TYPE_CUSTOM_MINING_TASK

                     // Compute the gamming key to get the sub-type of message
                    unsigned char sharedKeyAndGammingNonce[64];
                    setMem(sharedKeyAndGammingNonce, 32, 0);
                    copyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                    unsigned char gammingKey[32];
                    KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);
                    
                    // Record the task emitted by dispatcher
                    if (recordCustomMining && gammingKey[0] == MESSAGE_TYPE_CUSTOM_MINING_TASK)
                    {
                        const CustomMiningTask* task = ((CustomMiningTask*)((unsigned char*)request + sizeof(BroadcastMessage)));

                        // Determine the task part id
                        int partId = customMiningGetPartitionID(task->firstComputorIndex, task->lastComputorIndex);
                        if (partId >= 0)
                        {
                            // Record the task message
                            ACQUIRE(gCustomMiningTaskStorageLock);
                            int taskAddSts = gCustomMiningStorage._taskStorage[partId].addData(task);
                            RELEASE(gCustomMiningTaskStorageLock);

                            if (CustomMiningTaskStorage::OK == taskAddSts)
                            {
                                ATOMIC_INC64(gCustomMiningStats.phase[partId].tasks);
                            }
                        }
                    }
                }
                else if (messagePayloadSize == sizeof(CustomMiningSolution))
                {
                    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                    {
                        if (request->sourcePublicKey == broadcastedComputors.computors.publicKeys[i])
                        {
                            // Compute the gamming key to get the sub-type of message
                            unsigned char sharedKeyAndGammingNonce[64];
                            setMem(sharedKeyAndGammingNonce, 32, 0);
                            copyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                            unsigned char gammingKey[32];
                            KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);

                            if (recordCustomMining && gammingKey[0] == MESSAGE_TYPE_CUSTOM_MINING_SOLUTION)
                            {
                                // Record the solution
                                bool isSolutionGood = false;
                                const CustomMiningSolution* solution = ((CustomMiningSolution*)((unsigned char*)request + sizeof(BroadcastMessage)));

                                int partId = customMiningGetPartitionID(solution->firstComputorIndex, solution->lastComputorIndex);

                                // TODO: taskIndex can use for detect for-sure stale shares
                                if (partId >= 0 && solution->taskIndex > 0)
                                {
                                    CustomMiningSolutionCacheEntry cacheEntry;
                                    cacheEntry.set(solution);

                                    unsigned int cacheIndex = 0;
                                    int sts = gSystemCustomMiningSolutionCache[partId].tryFetching(cacheEntry, cacheIndex);

                                    // Check for duplicated solution
                                    if (sts == CUSTOM_MINING_CACHE_MISS)
                                    {
                                        gSystemCustomMiningSolutionCache[partId].addEntry(cacheEntry, cacheIndex);
                                        isSolutionGood = true;
                                    }

                                    if (isSolutionGood)
                                    {
                                        // Check the computor idx of this solution.
                                        unsigned short computorID = customMiningGetComputorID(solution->nonce, partId);
                                        if (computorID <= gTaskPartition[partId].lastComputorIdx)
                                        {

                                            ACQUIRE(gCustomMiningSharesCountLock);
                                            gCustomMiningSharesCount[computorID]++;
                                            RELEASE(gCustomMiningSharesCountLock);

                                            CustomMiningSolutionStorageEntry solutionStorageEntry;
                                            solutionStorageEntry.taskIndex = solution->taskIndex;
                                            solutionStorageEntry.nonce = solution->nonce;
                                            solutionStorageEntry.cacheEntryIndex = cacheIndex;

                                            ACQUIRE(gCustomMiningSolutionStorageLock);
                                            gCustomMiningStorage._solutionStorage[partId].addData(&solutionStorageEntry);
                                            RELEASE(gCustomMiningSolutionStorageLock);

                                        }
                                    }

                                    // Record stats
                                    const unsigned int hitCount = gSystemCustomMiningSolutionCache[partId].hitCount();
                                    const unsigned int missCount = gSystemCustomMiningSolutionCache[partId].missCount();
                                    const unsigned int collision = gSystemCustomMiningSolutionCache[partId].collisionCount();

                                    ATOMIC_STORE64(gCustomMiningStats.phase[partId].shares, missCount);
                                    ATOMIC_STORE64(gCustomMiningStats.phase[partId].duplicated, hitCount);
                                    ATOMIC_MAX64(gCustomMiningStats.maxCollisionShareCount, collision);

                                }
                            }
                            break;
                        }
                    }
                }
                else if (messagePayloadSize == sizeof(CustomMiningTaskV2) && request->sourcePublicKey == dispatcherPublicKey)
                {
                    unsigned char sharedKeyAndGammingNonce[64];
                    setMem(sharedKeyAndGammingNonce, 32, 0);
                    copyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                    unsigned char gammingKey[32];
                    KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);

                    // Record the task emitted by dispatcher
                    if (recordCustomMining && gammingKey[0] == MESSAGE_TYPE_CUSTOM_MINING_TASK)
                    {
                        const CustomMiningTaskV2* task = ((CustomMiningTaskV2*)((unsigned char*)request + sizeof(BroadcastMessage)));

                        // Record the task message
                        ACQUIRE(gCustomMiningTaskStorageLock);
                        int taskAddSts = gCustomMiningStorage._taskV2Storage.addData(task);
                        if (CustomMiningTaskStorage::OK == taskAddSts)
                        {
                            ATOMIC_INC64(gCustomMiningStats.phaseV2.tasks);
                            gCustomMiningStorage.updateTaskIndex(task->taskIndex);
                        }
                        RELEASE(gCustomMiningTaskStorageLock);
                    }
                }
                else if (messagePayloadSize == sizeof(CustomMiningSolutionV2))
                {
                    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                    {
                        if (request->sourcePublicKey == broadcastedComputors.computors.publicKeys[i])
                        {
                            // Compute the gamming key to get the sub-type of message
                            unsigned char sharedKeyAndGammingNonce[64];
                            setMem(sharedKeyAndGammingNonce, 32, 0);
                            copyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                            unsigned char gammingKey[32];
                            KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);

                            if (recordCustomMining && gammingKey[0] == MESSAGE_TYPE_CUSTOM_MINING_SOLUTION)
                            {
                                // Record the solution
                                bool isSolutionGood = false;
                                const CustomMiningSolutionV2* solution = ((CustomMiningSolutionV2*)((unsigned char*)request + sizeof(BroadcastMessage)));

                                CustomMiningSolutionV2CacheEntry cacheEntry;
                                cacheEntry.set(solution);

                                unsigned int cacheIndex = 0;
                                int sts = gSystemCustomMiningSolutionV2Cache.tryFetching(cacheEntry, cacheIndex);

                                // Check for duplicated solution
                                if (sts == CUSTOM_MINING_CACHE_MISS)
                                {
                                    gSystemCustomMiningSolutionV2Cache.addEntry(cacheEntry, cacheIndex);
                                    isSolutionGood = true;
                                }
                                if (gCustomMiningStorage.isSolutionStale(solution->taskIndex))
                                {
                                    isSolutionGood = false;
                                }

                                if (isSolutionGood)
                                {
                                    // Check the computor idx of this solution.
                                    unsigned short computorID = 0;
                                    if (solution->reserve0 == 0)
                                    {
                                        computorID = (solution->nonce >> 32ULL) % 676ULL;
                                    }
                                    else
                                    {
                                        computorID = solution->reserve1 % 676ULL;
                                    }

                                    ACQUIRE(gCustomMiningSharesCountLock);
                                    gCustomMiningSharesCount[computorID]++;
                                    RELEASE(gCustomMiningSharesCountLock);

                                    CustomMiningSolutionStorageEntry solutionStorageEntry;
                                    solutionStorageEntry.taskIndex = solution->taskIndex;
                                    solutionStorageEntry.nonce = solution->nonce;
                                    solutionStorageEntry.cacheEntryIndex = cacheIndex;

                                    ACQUIRE(gCustomMiningSolutionStorageLock);
                                    gCustomMiningStorage._solutionV2Storage.addData(&solutionStorageEntry);
                                    RELEASE(gCustomMiningSolutionStorageLock);
                                }

                                // Record stats
                                const unsigned int hitCount = gSystemCustomMiningSolutionV2Cache.hitCount();
                                const unsigned int missCount = gSystemCustomMiningSolutionV2Cache.missCount();
                                const unsigned int collision = gSystemCustomMiningSolutionV2Cache.collisionCount();

                                ATOMIC_STORE64(gCustomMiningStats.phaseV2.shares, missCount);
                                ATOMIC_STORE64(gCustomMiningStats.phaseV2.duplicated, hitCount);
                                ATOMIC_MAX64(gCustomMiningStats.maxCollisionShareCount, collision);
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
                {
                    if (request->destinationPublicKey == computorPublicKeys[i])
                    {
                        const unsigned int messagePayloadSize = messageSize - sizeof(BroadcastMessage) - SIGNATURE_SIZE;
                        if (messagePayloadSize)
                        {
                            unsigned char sharedKeyAndGammingNonce[64];
                            // sourcePublicKey != computorPublicKeys
                            if (request->sourcePublicKey != computorPublicKeys[i])
                            {
                                // Signing pubkey is not in the computor list. Only process if it has enough QUS
                                if (!hasEnoughBalance)
                                {
                                    ok = false;
                                }
                                else
                                {
                                    // it is an un-encrypted message, all zeros for first 32 bytes sharedKey
                                    setMem(sharedKeyAndGammingNonce, 32, 0);
                                }
                            }
                            else
                            {
                                // sourcePublicKey == computorPublicKeys, it is an encrypted message, get sharedKey.
                                if (!getSharedKey(computorPrivateKeys[i].m256i_u8, request->sourcePublicKey.m256i_u8, sharedKeyAndGammingNonce))
                                {
                                    ok = false;
                                }
                            }

                            if (ok)
                            {
                                copyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                                unsigned char gammingKey[32];
                                KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);
                                setMem(sharedKeyAndGammingNonce, 32, 0); // Zero the shared key in case stack content could be leaked later
                                unsigned char gamma[MAX_MESSAGE_PAYLOAD_SIZE];
                                KangarooTwelve(gammingKey, sizeof(gammingKey), gamma, messagePayloadSize);
                                for (unsigned int j = 0; j < messagePayloadSize; j++)
                                {
                                    ((unsigned char*)request)[sizeof(BroadcastMessage) + j] ^= gamma[j];
                                }

                                switch (gammingKey[0])
                                {
                                case MESSAGE_TYPE_SOLUTION:
                                {
                                    if (messagePayloadSize >= 32 + 32)
                                    {

                                        const m256i& solution_miningSeed = *(m256i*)((unsigned char*)request + sizeof(BroadcastMessage));
                                        const m256i& solution_nonce = *(m256i*)((unsigned char*)request + sizeof(BroadcastMessage) + 32);
                                        unsigned int k;
                                        for (k = 0; k < system.numberOfSolutions; k++)
                                        {
                                            if (solution_nonce == system.solutions[k].nonce
                                                && solution_miningSeed == system.solutions[k].miningSeed
                                                && request->destinationPublicKey == system.solutions[k].computorPublicKey)
                                            {
                                                break;
                                            }
                                        }
                                        if (k == system.numberOfSolutions)
                                        {
                                            unsigned int solutionScore = (*score)(processorNumber, request->destinationPublicKey, solution_miningSeed, solution_nonce);
                                            const int threshold = (system.epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[system.epoch] : SOLUTION_THRESHOLD_DEFAULT;
                                            if (system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS
                                                && score->isValidScore(solutionScore)
                                                && score->isGoodScore(solutionScore, threshold))
                                            {
                                                ACQUIRE(solutionsLock);

                                                for (k = 0; k < system.numberOfSolutions; k++)
                                                {
                                                    if (solution_nonce == system.solutions[k].nonce
                                                        && solution_miningSeed == system.solutions[k].miningSeed
                                                        && request->destinationPublicKey == system.solutions[k].computorPublicKey)
                                                    {
                                                        break;
                                                    }
                                                }
                                                if (k == system.numberOfSolutions)
                                                {
                                                    system.solutions[system.numberOfSolutions].computorPublicKey = request->destinationPublicKey;
                                                    system.solutions[system.numberOfSolutions].miningSeed = solution_miningSeed;
                                                    system.solutions[system.numberOfSolutions++].nonce = solution_nonce;
                                                }

                                                RELEASE(solutionsLock);
                                            }
                                        }
                                    }
                                }
                                break;
                                }
                            }
                        }

                        break;
                    }
                }
            }
        }
    }
}

static void processBroadcastComputors(Peer* peer, RequestResponseHeader* header)
{
    BroadcastComputors* request = header->getPayload<BroadcastComputors>();

    // Only accept computor list from current epoch (important in seamless epoch transition if this node is
    // lagging behind the others that already switched epoch).
    if (request->computors.epoch == system.epoch && request->computors.epoch > broadcastedComputors.computors.epoch)
    {
        // Verify that all addresses are non-zeroes. Otherwise, discard it even if ARB broadcasted it.
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            if (isZero(request->computors.publicKeys[i]))
            {
                return;
            }
        }

        // Verify that list is signed by Arbitrator
        unsigned char digest[32];
        KangarooTwelve(request, sizeof(BroadcastComputors) - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify((unsigned char*)&arbitratorPublicKey, digest, request->computors.signature))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            // Copy computor list
            copyMem(&broadcastedComputors.computors, &request->computors, sizeof(Computors));

            // Update ownComputorIndices and minerPublicKeys
            if (request->computors.epoch == system.epoch)
            {
                ACQUIRE(minerScoreArrayLock);
                numberOfOwnComputorIndices = 0;
                for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                {
                    minerPublicKeys[i] = request->computors.publicKeys[i];

                    for (unsigned int j = 0; j < sizeof(computorSeeds) / sizeof(computorSeeds[0]); j++)
                    {
                        if (request->computors.publicKeys[i] == computorPublicKeys[j])
                        {
                            ownComputorIndices[numberOfOwnComputorIndices] = i;
                            ownComputorIndicesMapping[numberOfOwnComputorIndices++] = j;

                            break;
                        }
                    }
                }
                RELEASE(minerScoreArrayLock);
            }
        }
    }
}

static bool verifyTickVoteSignature(const unsigned char* publicKey, const unsigned char* messageDigest, const unsigned char* signature, const bool curveVerify = true)
{
    unsigned int score = _byteswap_ulong(((unsigned int*)signature)[0]);
    if (score > TARGET_TICK_VOTE_SIGNATURE) return false;
    if (curveVerify)
    {
        if (!verify(publicKey, messageDigest, signature)) return false;
    }
    return true;
}

static void processBroadcastTick(Peer* peer, RequestResponseHeader* header)
{
    BroadcastTick* request = header->getPayload<BroadcastTick>();
    if (request->tick.computorIndex < NUMBER_OF_COMPUTORS
        && request->tick.epoch == system.epoch
        && request->tick.tick >= system.tick
        && ts.tickInCurrentEpochStorage(request->tick.tick)
        && request->tick.month >= 1 && request->tick.month <= 12
        && request->tick.day >= 1 && request->tick.day <= ((request->tick.month == 1 || request->tick.month == 3 || request->tick.month == 5 || request->tick.month == 7 || request->tick.month == 8 || request->tick.month == 10 || request->tick.month == 12) ? 31 : ((request->tick.month == 4 || request->tick.month == 6 || request->tick.month == 9 || request->tick.month == 11) ? 30 : ((request->tick.year & 3) ? 28 : 29)))
        && request->tick.hour <= 23
        && request->tick.minute <= 59
        && request->tick.second <= 59
        && request->tick.millisecond <= 999)
    {
        unsigned char digest[32];
        request->tick.computorIndex ^= BroadcastTick::type;
        KangarooTwelve(&request->tick, sizeof(Tick) - SIGNATURE_SIZE, digest, sizeof(digest));
        request->tick.computorIndex ^= BroadcastTick::type;
        const bool verifyFourQCurve = true;
        if (verifyTickVoteSignature(broadcastedComputors.computors.publicKeys[request->tick.computorIndex].m256i_u8, digest, request->tick.signature, verifyFourQCurve))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            ts.ticks.acquireLock(request->tick.computorIndex);

            // Find element in tick storage and check if contains data (epoch is set to 0 on init)
            Tick* tsTick = ts.ticks.getByTickInCurrentEpoch(request->tick.tick) + request->tick.computorIndex;
            if (tsTick->epoch == system.epoch)
            {
                // Check if the sent tick matches the tick in tick storage
                if (*((unsigned long long*)&request->tick.millisecond) != *((unsigned long long*)&tsTick->millisecond)
                    || request->tick.prevSpectrumDigest != tsTick->prevSpectrumDigest
                    || request->tick.prevUniverseDigest != tsTick->prevUniverseDigest
                    || request->tick.prevComputerDigest != tsTick->prevComputerDigest
                    || request->tick.saltedSpectrumDigest != tsTick->saltedSpectrumDigest
                    || request->tick.saltedUniverseDigest != tsTick->saltedUniverseDigest
                    || request->tick.saltedComputerDigest != tsTick->saltedComputerDigest
                    || request->tick.transactionDigest != tsTick->transactionDigest
                    || request->tick.saltedTransactionBodyDigest != tsTick->saltedTransactionBodyDigest
                    || request->tick.expectedNextTickTransactionDigest != tsTick->expectedNextTickTransactionDigest)
                {
                    faultyComputorFlags[request->tick.computorIndex >> 6] |= (1ULL << (request->tick.computorIndex & 63));
                }
            }
            else
            {
                // Copy the sent tick to the tick storage
                copyMem(tsTick, &request->tick, sizeof(Tick));
                peer->lastActiveTick = max(peer->lastActiveTick, peer->getDejavuTick(header->dejavu()));
            }

            ts.ticks.releaseLock(request->tick.computorIndex);
        }
    }
}

static void processBroadcastFutureTickData(Peer* peer, RequestResponseHeader* header)
{
    BroadcastFutureTickData* request = header->getPayload<BroadcastFutureTickData>();
    if (request->tickData.epoch == system.epoch
        && request->tickData.tick > system.tick
        && ts.tickInCurrentEpochStorage(request->tickData.tick)
        && request->tickData.tick % NUMBER_OF_COMPUTORS == request->tickData.computorIndex
        && request->tickData.month >= 1 && request->tickData.month <= 12
        && request->tickData.day >= 1 && request->tickData.day <= ((request->tickData.month == 1 || request->tickData.month == 3 || request->tickData.month == 5 || request->tickData.month == 7 || request->tickData.month == 8 || request->tickData.month == 10 || request->tickData.month == 12) ? 31 : ((request->tickData.month == 4 || request->tickData.month == 6 || request->tickData.month == 9 || request->tickData.month == 11) ? 30 : ((request->tickData.year & 3) ? 28 : 29)))
        && request->tickData.hour <= 23
        && request->tickData.minute <= 59
        && request->tickData.second <= 59
        && request->tickData.millisecond <= 999
        && ms(request->tickData.year, request->tickData.month, request->tickData.day, request->tickData.hour, request->tickData.minute, request->tickData.second, request->tickData.millisecond) <= ms(utcTime.Year - 2000, utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second, utcTime.Nanosecond / 1000000) + TIME_ACCURACY)
    {
        bool ok = true;
        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK && ok; i++)
        {
            if (!isZero(request->tickData.transactionDigests[i]))
            {
                // Check if same transactionDigest is present twice
                for (unsigned int j = 0; j < i; j++)
                {
                    if (request->tickData.transactionDigests[i] == request->tickData.transactionDigests[j])
                    {
                        ok = false;

                        break;
                    }
                }
            }
        }
        if (ok)
        {
            unsigned char digest[32];
            request->tickData.computorIndex ^= BroadcastFutureTickData::type;
            KangarooTwelve(&request->tickData, sizeof(TickData) - SIGNATURE_SIZE, digest, sizeof(digest));
            request->tickData.computorIndex ^= BroadcastFutureTickData::type;
            if (verify(broadcastedComputors.computors.publicKeys[request->tickData.computorIndex].m256i_u8, digest, request->tickData.signature))
            {
                if (header->isDejavuZero())
                {
                    enqueueResponse(NULL, header);
                }

                ts.tickData.acquireLock();
                TickData& td = ts.tickData.getByTickInCurrentEpoch(request->tickData.tick);
                if (td.epoch != INVALIDATED_TICK_DATA)
                {
                    if (request->tickData.tick == system.tick + 1 && targetNextTickDataDigestIsKnown)
                    {
                        if (!isZero(targetNextTickDataDigest))
                        {
                            unsigned char digest[32];
                            KangarooTwelve(&request->tickData, sizeof(TickData), digest, 32);
                            if (digest == targetNextTickDataDigest)
                            {
                                copyMem(&td, &request->tickData, sizeof(TickData));
                                peer->lastActiveTick = max(peer->lastActiveTick, peer->getDejavuTick(header->dejavu()));
                            }
                        }
                    }
                    else
                    {
                        if (td.epoch == system.epoch)
                        {
                            // Tick data already available. Mark computor as faulty if the data that was sent differs.
                            if (*((unsigned long long*) & request->tickData.millisecond) != *((unsigned long long*) & td.millisecond))
                            {
                                faultyComputorFlags[request->tickData.computorIndex >> 6] |= (1ULL << (request->tickData.computorIndex & 63));
                            }
                            else
                            {
                                for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                                {
                                    if (request->tickData.transactionDigests[i] != td.transactionDigests[i])
                                    {
                                        faultyComputorFlags[request->tickData.computorIndex >> 6] |= (1ULL << (request->tickData.computorIndex & 63));

                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            copyMem(&td, &request->tickData, sizeof(TickData));
                            peer->lastActiveTick = max(peer->lastActiveTick, peer->getDejavuTick(header->dejavu()));
                        }
                    }
                }
                ts.tickData.releaseLock();
            }
        }
    }
}

static void processBroadcastTransaction(Peer* peer, RequestResponseHeader* header)
{
    Transaction* request = header->getPayload<Transaction>();
    const unsigned int transactionSize = request->totalSize();
    if (request->checkValidity() && transactionSize == header->size() - sizeof(RequestResponseHeader))
    {
        unsigned char digest[32];
        KangarooTwelve(request, transactionSize - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify(request->sourcePublicKey.m256i_u8, digest, request->signaturePtr()))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            const int computorIndex = ::computorIndex(request->sourcePublicKey);
            if (computorIndex >= 0)
            {
                ACQUIRE(computorPendingTransactionsLock);

                const unsigned int offset = random(MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR);
                if (((Transaction*)&computorPendingTransactions[computorIndex * offset * MAX_TRANSACTION_SIZE])->tick < request->tick
                    && request->tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
                {
                    copyMem(&computorPendingTransactions[computorIndex * offset * MAX_TRANSACTION_SIZE], request, transactionSize);
                    KangarooTwelve(request, transactionSize, &computorPendingTransactionDigests[computorIndex * offset * 32ULL], 32);
                }

                RELEASE(computorPendingTransactionsLock);
            }
            else
            {
                const int spectrumIndex = ::spectrumIndex(request->sourcePublicKey);
                if (spectrumIndex >= 0)
                {
                    ACQUIRE(entityPendingTransactionsLock);

                    // Pending transactions pool follows the rule: A transaction with a higher tick overwrites previous transaction from the same address.
                    // The second filter is to avoid accident made by users/devs (setting scheduled tick too high) and get locked until end of epoch.
                    // It also makes sense that a node doesn't need to store a transaction that is scheduled on a tick that node will never reach.
                    // Notice: MAX_NUMBER_OF_TICKS_PER_EPOCH is not set globally since every node may have different TARGET_TICK_DURATION time due to memory limitation.
                    if (((Transaction*)&entityPendingTransactions[spectrumIndex * MAX_TRANSACTION_SIZE])->tick < request->tick
                        && request->tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
                    {
                        copyMem(&entityPendingTransactions[spectrumIndex * MAX_TRANSACTION_SIZE], request, transactionSize);
                        KangarooTwelve(request, transactionSize, &entityPendingTransactionDigests[spectrumIndex * 32ULL], 32);
                    }

                    RELEASE(entityPendingTransactionsLock);
                }
            }

            unsigned int tickIndex = ts.tickToIndexCurrentEpoch(request->tick);
            ts.tickData.acquireLock();
            if (request->tick == system.tick + 1
                && ts.tickData[tickIndex].epoch == system.epoch)
            {
                KangarooTwelve(request, transactionSize, digest, sizeof(digest));
                auto* tsReqTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);
                for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                {
                    if (digest == ts.tickData[tickIndex].transactionDigests[i])
                    {
                        ts.tickTransactions.acquireLock();
                        if (!tsReqTickTransactionOffsets[i])
                        {
                            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                            {
                                tsReqTickTransactionOffsets[i] = ts.nextTickTransactionOffset;
                                copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), request, transactionSize);
                                ts.nextTickTransactionOffset += transactionSize;
                            }
                        }
                        ts.tickTransactions.releaseLock();
                        break;
                    }
                }
            }
            ts.tickData.releaseLock();
        }
    }
}

static void processRequestComputors(Peer* peer, RequestResponseHeader* header)
{
    if (broadcastedComputors.computors.epoch)
    {
        enqueueResponse(peer, sizeof(broadcastedComputors), BroadcastComputors::type, header->dejavu(), &broadcastedComputors);
    }
    else
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
}

/**
 * Sends Tick data for computors *not* marked in request->voteFlags (0 = requester wants the tick of this computer).
 * Shuffles order, ends with EndResponse.
 */
static void processRequestQuorumTick(Peer* peer, RequestResponseHeader* header)
{
    RequestQuorumTick* request = header->getPayload<RequestQuorumTick>();

    unsigned short tickEpoch = 0;
    const Tick* tsCompTicks;
    if (ts.tickInCurrentEpochStorage(request->quorumTick.tick))
    {
        tickEpoch = system.epoch;
        tsCompTicks = ts.ticks.getByTickInCurrentEpoch(request->quorumTick.tick);
    }
    else if (ts.tickInPreviousEpochStorage(request->quorumTick.tick))
    {
        tickEpoch = system.epoch - 1;
        tsCompTicks = ts.ticks.getByTickInPreviousEpoch(request->quorumTick.tick);
    }

    if (tickEpoch != 0)
    {
        // Send Tick struct data from tick storage as requested by tick and voteFlags in request->quorumTick.
        // The order of the computors is randomized using a Fisher–Yates shuffle
        // Todo: This function may be optimized by moving the checking of voteFlags in the first loop, reducing the number of calls to random().
        unsigned short computorIndices[NUMBER_OF_COMPUTORS];
        unsigned short numberOfComputorIndices;
        for (numberOfComputorIndices = 0; numberOfComputorIndices < NUMBER_OF_COMPUTORS; numberOfComputorIndices++)
        {
            computorIndices[numberOfComputorIndices] = numberOfComputorIndices;
        }
        while (numberOfComputorIndices)
        {
            const unsigned short index = random(numberOfComputorIndices);

            if (!(request->quorumTick.voteFlags[computorIndices[index] >> 3] & (1 << (computorIndices[index] & 7))))
            {
                // Todo: We should acquire ts.ticks lock here if tick >= system.tick
                const Tick* tsTick = tsCompTicks + computorIndices[index];
                if (tsTick->epoch == tickEpoch)
                {
                    ts.ticks.acquireLock(computorIndices[index]);
                    enqueueResponse(peer, sizeof(Tick), BroadcastTick::type, header->dejavu(), tsTick);
                    ts.ticks.releaseLock(computorIndices[index]);
                }
            }

            computorIndices[index] = computorIndices[--numberOfComputorIndices];
        }
    }
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

static void processRequestTickData(Peer* peer, RequestResponseHeader* header)
{
    RequestTickData* request = header->getPayload<RequestTickData>();
    TickData* td = ts.tickData.getByTickIfNotEmpty(request->requestedTickData.tick);
    if (td)
    {
        enqueueResponse(peer, sizeof(TickData), BroadcastFutureTickData::type, header->dejavu(), td);
    }
    else
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
}

static void processRequestTickTransactions(Peer* peer, RequestResponseHeader* header)
{
    RequestedTickTransactions* request = header->getPayload<RequestedTickTransactions>();

    unsigned short tickEpoch = 0;
    const unsigned long long* tsReqTickTransactionOffsets;
    if (ts.tickInCurrentEpochStorage(request->tick))
    {
        tickEpoch = system.epoch;
        tsReqTickTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(request->tick);
    }
    else if (ts.tickInPreviousEpochStorage(request->tick))
    {
        tickEpoch = system.epoch - 1;
        tsReqTickTransactionOffsets = ts.tickTransactionOffsets.getByTickInPreviousEpoch(request->tick);
    }

    if (tickEpoch != 0)
    {
        unsigned int tickTransactionIndices[NUMBER_OF_TRANSACTIONS_PER_TICK];
        unsigned int numberOfTickTransactions;
        for (numberOfTickTransactions = 0; numberOfTickTransactions < NUMBER_OF_TRANSACTIONS_PER_TICK; numberOfTickTransactions++)
        {
            tickTransactionIndices[numberOfTickTransactions] = numberOfTickTransactions;
        }
        while (numberOfTickTransactions)
        {
            const unsigned int index = random(numberOfTickTransactions);

            if (!(request->transactionFlags[tickTransactionIndices[index] >> 3] & (1 << (tickTransactionIndices[index] & 7))))
            {
                unsigned long long tickTransactionOffset = tsReqTickTransactionOffsets[tickTransactionIndices[index]];
                if (tickTransactionOffset)
                {
                    const Transaction* transaction = ts.tickTransactions(tickTransactionOffset);
                    if (transaction->tick == request->tick && transaction->checkValidity())
                    {
                        enqueueResponse(peer, transaction->totalSize(), BROADCAST_TRANSACTION, header->dejavu(), (void*)transaction);
                    }
                    else
                    {
                        // tick storage messed up -> indicates bug such as buffer overflow
#if !defined(NDEBUG)
                        CHAR16 dbgMsg[200];
                        setText(dbgMsg, L"Invalid transaction found in processRequestTickTransactions(), tick ");
                        appendNumber(dbgMsg, request->tick, FALSE);
                        addDebugMessage(dbgMsg);
                        ts.checkStateConsistencyWithAssert();
#endif
                    }
                }
            }

            tickTransactionIndices[index] = tickTransactionIndices[--numberOfTickTransactions];
        }
    }
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

static void processRequestTransactionInfo(Peer* peer, RequestResponseHeader* header)
{
    RequestedTransactionInfo* request = header->getPayload<RequestedTransactionInfo>();
    const Transaction* transaction = ts.transactionsDigestAccess.findTransaction(request->txDigest);
    if (transaction)
    {
        enqueueResponse(peer, transaction->totalSize(), BROADCAST_TRANSACTION, header->dejavu(), (void*)transaction);
    }
    else
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
}

static void processRequestCurrentTickInfo(Peer* peer, RequestResponseHeader* header)
{
    CurrentTickInfo currentTickInfo;

    if (broadcastedComputors.computors.epoch)
    {
        unsigned long long tickDuration = (__rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1]) / frequency;
        if (tickDuration > 0xFFFF)
        {
            tickDuration = 0xFFFF;
        }
        currentTickInfo.tickDuration = (unsigned short)tickDuration;

        currentTickInfo.epoch = system.epoch;
        currentTickInfo.tick = system.tick;
        currentTickInfo.numberOfAlignedVotes = gTickNumberOfComputors;
        currentTickInfo.numberOfMisalignedVotes = (gTickTotalNumberOfComputors - gTickNumberOfComputors);
        currentTickInfo.initialTick = system.initialTick;
    }
    else
    {
        setMem(&currentTickInfo, sizeof(CurrentTickInfo), 0);
    }

    enqueueResponse(peer, sizeof(currentTickInfo), RESPOND_CURRENT_TICK_INFO, header->dejavu(), &currentTickInfo);
}

static void processResponseCurrentTickInfo(Peer* peer, RequestResponseHeader* header)
{
    if (header->size() == sizeof(RequestResponseHeader) + sizeof(CurrentTickInfo))
    {
        CurrentTickInfo currentTickInfo = *(header->getPayload< CurrentTickInfo>());
        // avoid malformed data
        if (currentTickInfo.initialTick == system.initialTick
            && currentTickInfo.epoch == system.epoch 
            && currentTickInfo.tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH
            && currentTickInfo.tick >= system.initialTick)
        {
            // TODO: reserved handler for future use when we are able to verify CurrentTickInfo
        }
    }
}

static void processRequestEntity(Peer* peer, RequestResponseHeader* header)
{
    RespondedEntity respondedEntity;

    RequestedEntity* request = header->getPayload<RequestedEntity>();
    respondedEntity.entity.publicKey = request->publicKey;
    // Inside spectrumIndex already have acquire/release lock
    respondedEntity.spectrumIndex = spectrumIndex(respondedEntity.entity.publicKey);
    respondedEntity.tick = system.tick;
    if (respondedEntity.spectrumIndex < 0)
    {
        respondedEntity.entity.incomingAmount = 0;
        respondedEntity.entity.outgoingAmount = 0;
        respondedEntity.entity.numberOfIncomingTransfers = 0;
        respondedEntity.entity.numberOfOutgoingTransfers = 0;
        respondedEntity.entity.latestIncomingTransferTick = 0;
        respondedEntity.entity.latestOutgoingTransferTick = 0;

        setMem(respondedEntity.siblings, sizeof(respondedEntity.siblings), 0);
    }
    else
    {
        copyMem(&respondedEntity.entity, &spectrum[respondedEntity.spectrumIndex], sizeof(EntityRecord));
        ACQUIRE(spectrumLock);
        getSiblings<SPECTRUM_DEPTH>(respondedEntity.spectrumIndex, spectrumDigests, respondedEntity.siblings);
        RELEASE(spectrumLock);
    }


    enqueueResponse(peer, sizeof(respondedEntity), RESPOND_ENTITY, header->dejavu(), &respondedEntity);
}

static void processRequestContractIPO(Peer* peer, RequestResponseHeader* header)
{
    RespondContractIPO respondContractIPO;

    RequestContractIPO* request = header->getPayload<RequestContractIPO>();
    respondContractIPO.contractIndex = request->contractIndex;
    respondContractIPO.tick = system.tick;
    if (request->contractIndex >= contractCount
        || system.epoch != (contractDescriptions[request->contractIndex].constructionEpoch - 1))
    {
        setMem(respondContractIPO.publicKeys, sizeof(respondContractIPO.publicKeys), 0);
        setMem(respondContractIPO.prices, sizeof(respondContractIPO.prices), 0);
    }
    else
    {
        contractStateLock[request->contractIndex].acquireRead();
        IPO* ipo = (IPO*)contractStates[request->contractIndex];
        copyMem(respondContractIPO.publicKeys, ipo->publicKeys, sizeof(respondContractIPO.publicKeys));
        copyMem(respondContractIPO.prices, ipo->prices, sizeof(respondContractIPO.prices));
        contractStateLock[request->contractIndex].releaseRead();
    }

    enqueueResponse(peer, sizeof(respondContractIPO), RespondContractIPO::type, header->dejavu(), &respondContractIPO);
}

static void processRequestContractFunction(Peer* peer, const unsigned long long processorNumber, RequestResponseHeader* header)
{
    // TODO: Invoked function may enter endless loop, so a timeout (and restart) is required for request processing threads

    RequestContractFunction* request = header->getPayload<RequestContractFunction>();
    if (header->size() != sizeof(RequestResponseHeader) + sizeof(RequestContractFunction) + request->inputSize
        || !request->contractIndex || request->contractIndex >= contractCount
        || system.epoch < contractDescriptions[request->contractIndex].constructionEpoch
        || !contractUserFunctions[request->contractIndex][request->inputType])
    {
        enqueueResponse(peer, 0, RespondContractFunction::type, header->dejavu(), NULL);
    }
    else
    {
        QpiContextUserFunctionCall qpiContext(request->contractIndex);
        auto errorCode = qpiContext.call(request->inputType, (((unsigned char*)request) + sizeof(RequestContractFunction)), request->inputSize);
        if (errorCode == NoContractError)
        {
            // success: respond with function output
            enqueueResponse(peer, qpiContext.outputSize, RespondContractFunction::type, header->dejavu(), qpiContext.outputBuffer);
        }
        else
        {
            // error: respond with empty output, send TryAgain if the function was stopped to resolve a potential
            // deadlock
            unsigned char type = RespondContractFunction::type;
            if (errorCode == ContractErrorStoppedToResolveDeadlock)
                type = TryAgain::type;
            enqueueResponse(peer, 0, type, header->dejavu(), NULL);
        }
    }
}

static void processRequestSystemInfo(Peer* peer, RequestResponseHeader* header)
{
    RespondSystemInfo respondedSystemInfo;

    respondedSystemInfo.version = system.version;
    respondedSystemInfo.epoch = system.epoch;
    respondedSystemInfo.tick = system.tick;
    respondedSystemInfo.initialTick = system.initialTick;
    respondedSystemInfo.latestCreatedTick = system.latestCreatedTick;

    respondedSystemInfo.initialMillisecond = system.initialMillisecond;
    respondedSystemInfo.initialSecond = system.initialSecond;
    respondedSystemInfo.initialMinute = system.initialMinute;
    respondedSystemInfo.initialHour = system.initialHour;
    respondedSystemInfo.initialDay = system.initialDay;
    respondedSystemInfo.initialMonth = system.initialMonth;
    respondedSystemInfo.initialYear = system.initialYear;

    respondedSystemInfo.numberOfEntities = spectrumInfo.numberOfEntities;
    respondedSystemInfo.numberOfTransactions = numberOfTransactions;

    respondedSystemInfo.randomMiningSeed = score->currentRandomSeed;
    respondedSystemInfo.solutionThreshold = (system.epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[system.epoch] : SOLUTION_THRESHOLD_DEFAULT;

    respondedSystemInfo.totalSpectrumAmount = spectrumInfo.totalAmount;
    respondedSystemInfo.currentEntityBalanceDustThreshold = (dustThresholdBurnAll > dustThresholdBurnHalf) ? dustThresholdBurnAll : dustThresholdBurnHalf;

    respondedSystemInfo.targetTickVoteSignature = TARGET_TICK_VOTE_SIGNATURE;
    enqueueResponse(peer, sizeof(respondedSystemInfo), RESPOND_SYSTEM_INFO, header->dejavu(), &respondedSystemInfo);
}


// Process the request for custom mining solution verification.
// The request contains a single solution along with its validity status.
// Once the validity is determined, the solution is marked as verified in storage
// to prevent it from being re-sent for verification.
static void processRequestedCustomMiningSolutionVerificationRequest(Peer* peer, RequestResponseHeader* header)
{
    RequestedCustomMiningSolutionVerification* request = header->getPayload<RequestedCustomMiningSolutionVerification>();
    if (header->size() >= sizeof(RequestResponseHeader) + sizeof(RequestedCustomMiningSolutionVerification) + SIGNATURE_SIZE)
    {
        unsigned char digest[32];
        KangarooTwelve(request, header->size() - sizeof(RequestResponseHeader) - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify(operatorPublicKey.m256i_u8, digest, ((const unsigned char*)header + (header->size() - SIGNATURE_SIZE))))
        {
            RespondCustomMiningSolutionVerification respond;

            // Update the share counting
            // Only record shares in idle phase
            char recordSolutions = 0;
            ACQUIRE(gIsInCustomMiningStateLock);
            recordSolutions = gIsInCustomMiningState;
            RELEASE(gIsInCustomMiningStateLock);

            if (recordSolutions)
            {
                CustomMiningSolutionCacheEntry fullEntry;
                fullEntry.set(request->taskIndex, request->nonce, request->firstComputorIdx, request->lastComputorIdx);
                fullEntry.setVerified(true);
                fullEntry.setValid(request->isValid > 0);

                // Make sure the solution still existed
                int partId = customMiningGetPartitionID(request->firstComputorIdx, request->lastComputorIdx);
                // Check the computor idx of this solution
                int computorID = NUMBER_OF_COMPUTORS;
                if (partId >= 0)
                {
                    computorID = customMiningGetComputorID(request->nonce, partId);
                }

                if (partId >=0 
                    && computorID <= gTaskPartition[partId].lastComputorIdx
                    && CUSTOM_MINING_CACHE_HIT == gSystemCustomMiningSolutionCache[partId].tryFetchingAndUpdate(fullEntry, CUSTOM_MINING_CACHE_HIT))
                {
                    // Reduce the share of this nonce if it is invalid
                    if (0 == request->isValid)
                    {
                        ACQUIRE(gCustomMiningSharesCountLock);
                        gCustomMiningSharesCount[computorID] = gCustomMiningSharesCount[computorID] > 0 ? gCustomMiningSharesCount[computorID] - 1 : 0;
                        RELEASE(gCustomMiningSharesCountLock);

                        // Save the number of invalid share count
                        ATOMIC_INC64(gCustomMiningStats.phase[partId].invalid);

                        respond.status = RespondCustomMiningSolutionVerification::invalid;
                    }
                    else
                    {
                        ATOMIC_INC64(gCustomMiningStats.phase[partId].valid);
                        respond.status = RespondCustomMiningSolutionVerification::valid;
                    }
                }
                else
                {
                    respond.status = RespondCustomMiningSolutionVerification::notExisted;
                }
            }
            else
            {
                respond.status = RespondCustomMiningSolutionVerification::customMiningStateEnded;
            }

            respond.taskIndex = request->taskIndex;
            respond.firstComputorIdx = request->firstComputorIdx;
            respond.lastComputorIdx = request->lastComputorIdx;
            respond.nonce = request->nonce;
            enqueueResponse(peer, sizeof(respond), RespondCustomMiningSolutionVerification::type, header->dejavu(), &respond);
        }
    }
}

// Process custom mining data requests.
// Currently supports:
// - Requesting a range of tasks (using Unix timestamps as unique indexes; each task has only one unique index).
// - Requesting all solutions corresponding to a specific task index. 
//   The total size of the response will not exceed CUSTOM_MINING_RESPOND_MESSAGE_MAX_SIZE.
// For the solution respond, only respond solution that has not been verified yet
static void processCustomMiningDataRequest(Peer* peer, const unsigned long long processorNumber, RequestResponseHeader* header)
{
    RequestedCustomMiningData* request = header->getPayload<RequestedCustomMiningData>();
    if (header->size() >= sizeof(RequestResponseHeader) + sizeof(RequestedCustomMiningData) + SIGNATURE_SIZE)
    {

        unsigned char digest[32];
        KangarooTwelve(request, header->size() - sizeof(RequestResponseHeader) - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify(operatorPublicKey.m256i_u8, digest, ((const unsigned char*)header + (header->size() - SIGNATURE_SIZE))))
        {
            unsigned char* respond = NULL;
            // Request tasks
            if (request->dataType == RequestedCustomMiningData::taskType)
            {
                // For task type, return all data from the current phase
                ACQUIRE(gCustomMiningTaskStorageLock);
                // Pack all the task data
                respond = gCustomMiningStorage.getSerializedTaskData(request->fromTaskIndex, request->toTaskIndex, processorNumber);
                RELEASE(gCustomMiningTaskStorageLock);

                if (NULL != respond)
                {
                    CustomMiningRespondDataHeader* customMiningInternalHeader = (CustomMiningRespondDataHeader*)respond;
                    customMiningInternalHeader->respondType = RespondCustomMiningData::taskType;
                    const unsigned long long respondDataSize = sizeof(CustomMiningRespondDataHeader) + customMiningInternalHeader->itemCount * customMiningInternalHeader->itemSize;
                    ASSERT(respondDataSize < ((1ULL << 32) - 1));
                    enqueueResponse(
                        peer,
                        (unsigned int)respondDataSize,
                        RespondCustomMiningData::type, header->dejavu(), respond);
                }
                else
                {
                    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
                }

            }
            // Request solutions
            else if (request->dataType == RequestedCustomMiningData::solutionType)
            {
                // For solution type, return all solution from the current phase
                int partId = customMiningGetPartitionID(request->firstComputorIdx, request->lastComputorIdx);
                if (partId >= 0)
                {
                    ACQUIRE(gCustomMiningSolutionStorageLock);
                    // Look for all solution data
                    respond = gCustomMiningStorage._solutionStorage[partId].getSerializedData(request->fromTaskIndex, processorNumber);
                    RELEASE(gCustomMiningSolutionStorageLock);
                }
                else
                {
                    respond = NULL;
                }

                // Has the solutions
                if (NULL != respond)
                {
                    unsigned char* respondSolution = gCustomMiningStorage._dataBuffer[processorNumber];
                    CustomMiningRespondDataHeader* customMiningInternalHeader = (CustomMiningRespondDataHeader*)respondSolution;
                    CustomMiningSolutionStorageEntry* solutionEntries = (CustomMiningSolutionStorageEntry*)(respond + sizeof(CustomMiningRespondDataHeader));
                    copyMem(customMiningInternalHeader, (CustomMiningRespondDataHeader*)respond, sizeof(CustomMiningRespondDataHeader));

                    // Extract the solutions and respond
                    unsigned char* respondSolutionPayload = respondSolution + sizeof(CustomMiningRespondDataHeader);
                    long long remainedDataToSend = CUSTOM_MINING_RESPOND_MESSAGE_MAX_SIZE;
                    int sendItem = 0;
                    for (int k = 0; k < customMiningInternalHeader->itemCount && remainedDataToSend > sizeof(CustomMiningSolution); k++)
                    {
                        CustomMiningSolutionStorageEntry entry = solutionEntries[k];
                        CustomMiningSolutionCacheEntry fullEntry;

                        gSystemCustomMiningSolutionCache[partId].getEntry(fullEntry, (unsigned int)entry.cacheEntryIndex);

                        // Check data is matched and not verifed yet
                        if (!fullEntry.isEmpty() 
                            && !fullEntry.isVerified() 
                            && fullEntry.getTaskIndex() == entry.taskIndex
                            && fullEntry.getNonce() == entry.nonce)
                        {
                            // Append data to send
                            CustomMiningSolution solution;
                            fullEntry.get(solution);

                            copyMem(respondSolutionPayload + k * sizeof(CustomMiningSolution), &solution, sizeof(CustomMiningSolution));
                            remainedDataToSend -= sizeof(CustomMiningSolution);
                            sendItem++;
                        }
                    }
                    
                    customMiningInternalHeader->itemSize = sizeof(CustomMiningSolution);
                    customMiningInternalHeader->itemCount = sendItem;
                    customMiningInternalHeader->respondType = RespondCustomMiningData::solutionType;
                    const unsigned long long respondDataSize = sizeof(CustomMiningRespondDataHeader) + customMiningInternalHeader->itemCount * customMiningInternalHeader->itemSize;
                    ASSERT(respondDataSize < ((1ULL << 32) - 1));
                    enqueueResponse(
                        peer,
                        (unsigned int)respondDataSize,
                        RespondCustomMiningData::type, header->dejavu(), respondSolution);
                }
                else
                {
                    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
                }
            }
            else // Unknonwn type
            {
                enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
            }
        }
    }
}

static void processSpecialCommand(Peer* peer, RequestResponseHeader* header)
{
    SpecialCommand* request = header->getPayload<SpecialCommand>();
    if (header->size() >= sizeof(RequestResponseHeader) + sizeof(SpecialCommand) + SIGNATURE_SIZE
        && (request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) > system.latestOperatorNonce)
    {
        unsigned char digest[32];
        KangarooTwelve(request, header->size() - sizeof(RequestResponseHeader) - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify(operatorPublicKey.m256i_u8, digest, ((const unsigned char*)header + (header->size() - SIGNATURE_SIZE))))
        {
            system.latestOperatorNonce = request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF;

            switch (request->everIncreasingNonceAndCommandType >> 56)
            {
            case SPECIAL_COMMAND_SHUT_DOWN:
            {
                shutDownNode = 1;
            }
            break;

            case SPECIAL_COMMAND_SET_SOLUTION_THRESHOLD_REQUEST:
            {
                SpecialCommandSetSolutionThresholdRequestAndResponse* _request = header->getPayload<SpecialCommandSetSolutionThresholdRequestAndResponse>();
                // can only set future epoch
                if (_request->epoch > system.epoch && _request->epoch < MAX_NUMBER_EPOCH)
                {
                    solutionThreshold[_request->epoch] = _request->threshold;
                }
                SpecialCommandSetSolutionThresholdRequestAndResponse response;
                response.everIncreasingNonceAndCommandType = _request->everIncreasingNonceAndCommandType;
                response.epoch = _request->epoch;
                response.threshold = (_request->epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[_request->epoch] : SOLUTION_THRESHOLD_DEFAULT;
                enqueueResponse(peer, sizeof(SpecialCommandSetSolutionThresholdRequestAndResponse), SpecialCommand::type, header->dejavu(), &response);
            }
            break;
            case SPECIAL_COMMAND_TOGGLE_MAIN_MODE_REQUEST:
            {
                SpecialCommandToggleMainModeRequestAndResponse* _request = header->getPayload<SpecialCommandToggleMainModeRequestAndResponse>();
                if (requestPersistingNodeState == 1 || persistingNodeStateTickProcWaiting == 1)
                {
                    //logToConsole(L"Unable to switch mode because node is saving states.");
                }
                else
                {
                    mainAuxStatus = _request->mainModeFlag;                    
                }
                enqueueResponse(peer, sizeof(SpecialCommandToggleMainModeRequestAndResponse), SpecialCommand::type, header->dejavu(), _request);
            }
            break;
            case SPECIAL_COMMAND_REFRESH_PEER_LIST:
            {
                forceRefreshPeerList = true;
                enqueueResponse(peer, sizeof(SpecialCommand), SpecialCommand::type, header->dejavu(), request); // echo back to indicate success
            }
            break;
            case SPECIAL_COMMAND_FORCE_NEXT_TICK:
            {
                forceNextTick = true;
                enqueueResponse(peer, sizeof(SpecialCommand), SpecialCommand::type, header->dejavu(), request); // echo back to indicate success
            }
            break;
            case SPECIAL_COMMAND_REISSUE_VOTE:
            {
                system.latestCreatedTick--;
                enqueueResponse(peer, sizeof(SpecialCommand), SpecialCommand::type, header->dejavu(), request); // echo back to indicate success
            }
            break;
            case SPECIAL_COMMAND_SEND_TIME:
            {
                // set time
                SpecialCommandSendTime* _request = header->getPayload<SpecialCommandSendTime>();
                EFI_TIME newTime;
                copyMem(&newTime, &_request->utcTime, sizeof(_request->utcTime)); // caution: response.utcTime is subset of newTime (smaller size)
                newTime.TimeZone = 0;
                newTime.Daylight = 0;
                EFI_STATUS status = rs->SetTime(&newTime);
#ifndef NDEBUG
                if (status != EFI_SUCCESS)
                {
                    addDebugMessage(L"SetTime() SPECIAL_COMMAND_SEND_TIME failed");
                }
#endif
            }
            // this has no break by intention, because SPECIAL_COMMAND_SEND_TIME responds the same way as SPECIAL_COMMAND_QUERY_TIME
            case SPECIAL_COMMAND_QUERY_TIME:
            {
                // send back current time
                // We don't call updateTime() here, because:
                // - calling it on non-main processor can cause unexpected behavior (very rare but observed)
                // - frequency of calling updateTime() in main processor is high compared to the granularity of utcTime
                //   (1 second, because nanoseconds are not provided on the platforms tested so far)
                SpecialCommandSendTime response;
                response.everIncreasingNonceAndCommandType = (request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) | (SPECIAL_COMMAND_SEND_TIME << 56);
                copyMem(&response.utcTime, &utcTime, sizeof(response.utcTime)); // caution: response.utcTime is subset of global utcTime (smaller size)
                enqueueResponse(peer, sizeof(SpecialCommandSendTime), SpecialCommand::type, header->dejavu(), &response);
            }
            break;
            case SPECIAL_COMMAND_GET_MINING_SCORE_RANKING:
            {
                requestMiningScoreRanking.everIncreasingNonceAndCommandType = 
                    (request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) | (SPECIAL_COMMAND_GET_MINING_SCORE_RANKING << 56);

                ACQUIRE(minerScoreArrayLock);
                requestMiningScoreRanking.numberOfRankings = numberOfMiners;
                for (unsigned int i = 0; i < requestMiningScoreRanking.numberOfRankings; ++i)
                {
                    requestMiningScoreRanking.rankings[i].minerPublicKey = minerPublicKeys[i];
                    requestMiningScoreRanking.rankings[i].minerScore = minerScores[i];
                }
                RELEASE(minerScoreArrayLock);
                enqueueResponse(peer,
                    sizeof(requestMiningScoreRanking.everIncreasingNonceAndCommandType)
                    + sizeof(requestMiningScoreRanking.numberOfRankings)
                    + sizeof(requestMiningScoreRanking.rankings[0]) * requestMiningScoreRanking.numberOfRankings,
                    SpecialCommand::type,
                    header->dejavu(),
                    &requestMiningScoreRanking);
            }
            break;

            case SPECIAL_COMMAND_FORCE_SWITCH_EPOCH:
            {
                forceSwitchEpoch = true;
                enqueueResponse(peer, sizeof(SpecialCommand), SpecialCommand::type, header->dejavu(), request); // echo back to indicate success
            }
            break;

            case SPECIAL_COMMAND_CONTINUE_SWITCH_EPOCH:
            {
                epochTransitionCleanMemoryFlag = 1;
                enqueueResponse(peer, sizeof(SpecialCommand), SpecialCommand::type, header->dejavu(), request); // echo back to indicate success
            }
            break;

            case SPECIAL_COMMAND_SET_CONSOLE_LOGGING_MODE:
            {
                const auto* _request = header->getPayload<SpecialCommandSetConsoleLoggingModeRequestAndResponse>();
                consoleLoggingLevel = _request->loggingMode;
                enqueueResponse(peer, sizeof(SpecialCommandSetConsoleLoggingModeRequestAndResponse), SpecialCommand::type, header->dejavu(), _request);
            }
            break;
            }
        }
    }
}

// a tracker to detect if a thread is crashed
static void checkinTime(unsigned long long processorNumber)
{
    threadTimeCheckin[processorNumber].second = utcTime.Second;
    threadTimeCheckin[processorNumber].minute = utcTime.Minute;
    threadTimeCheckin[processorNumber].hour = utcTime.Hour;
    threadTimeCheckin[processorNumber].day = utcTime.Day;
}

static void setNewMiningSeed()
{
    score->initMiningData(spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1]);
}

// Total number of external mining event.
// Can set to zero to disable event 
static constexpr int gNumberOfFullExternalMiningEvents = sizeof(gFullExternalComputationTimes) > 0 ? sizeof(gFullExternalComputationTimes) / sizeof(gFullExternalComputationTimes[0]) : 0;
struct FullExternallEvent
{
    WeekDay startTime;
    WeekDay endTime;
};
FullExternallEvent* gFullExternalEventTime = NULL;
static bool gSpecialEventFullExternalComputationPeriod = false; // a flag indicates a special event (period) that the network running 100% external computation
static WeekDay currentEventEndTime;


static bool isFullExternalComputationTime(TimeDate tickDate)
{
    // No event
    if (gNumberOfFullExternalMiningEvents <= 0)
    {
        return false;
    }

    // Get current day of the week
    WeekDay tickWeekDay;
    tickWeekDay.hour = tickDate.hour;
    tickWeekDay.minute = tickDate.minute;
    tickWeekDay.second = tickDate.second;
    tickWeekDay.millisecond = tickDate.millisecond;
    tickWeekDay.dayOfWeek = getDayOfWeek(tickDate.day, tickDate.month, 2000 + tickDate.year);

    // Check if the day is in range. Expect the time is not overlap.
    for (int i = 0; i < gNumberOfFullExternalMiningEvents; ++i)
    {
        if (isWeekDayInRange(tickWeekDay, gFullExternalEventTime[i].startTime, gFullExternalEventTime[i].endTime))
        {
            gSpecialEventFullExternalComputationPeriod = true;

            currentEventEndTime = gFullExternalEventTime[i].endTime;
            return true;
        }
    }

    // When not in range, and the time pass the gFullExternalEndTime. We need to make sure the ending happen
    // in custom mining period, so that the score of custom mining is recorded.
    if (gSpecialEventFullExternalComputationPeriod)
    {
        // Check time pass the end time
        TimeDate endTimeDate = tickDate;
        endTimeDate.hour = currentEventEndTime.hour;
        endTimeDate.minute = currentEventEndTime.minute;
        endTimeDate.second = currentEventEndTime.second;

        if (compareTimeDate(tickDate, endTimeDate) == 1)
        {
            // Check time is in custom mining phase. If it is still in qubic mining phase
            // don't stop the event
            if (getTickInMiningPhaseCycle() <= INTERNAL_COMPUTATIONS_INTERVAL)
            {
                return true;
            }
        }
    }
    
    // Event is marked as end
    gSpecialEventFullExternalComputationPeriod = false;
    return false;
}

// Clean up before custom mining phase. Thread-safe function
static void beginCustomMiningPhase()
{
    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        gSystemCustomMiningSolutionCache[i].reset();
    }

    gSystemCustomMiningSolutionV2Cache.reset();
    gCustomMiningStorage.reset();
    gCustomMiningStats.phaseResetAndEpochAccumulate();
}

// resetPhase: If true, allows reinitializing mining seed and the custom mining phase flag
// even when already inside the current phase. These values are normally set only once
// at the beginning of a phase.
static void checkAndSwitchMiningPhase(short tickEpoch, TimeDate tickDate, bool resetPhase)
{
    bool isBeginOfCustomMiningPhase = false;
    char isInCustomMiningPhase = 0;

    // When resetting the phase:
    // - If in the internal mining phase => reset the mining seed for the new epoch
    // - If in the external (custom) mining phase => reset mining data (counters, etc.)
    if (resetPhase)
    {
        const unsigned int r = getTickInMiningPhaseCycle();
        if (r < INTERNAL_COMPUTATIONS_INTERVAL)
        {
            setNewMiningSeed();
        }
        else
        {
            score->initMiningData(m256i::zero());
            isBeginOfCustomMiningPhase = true;
            isInCustomMiningPhase = 1;
        }
    }
    else
    {
        // Track whether we’re currently in a full external computation window
        static bool isInFullExternalTime = false;

        // Make sure the tick is valid and not in the reset phase state
        if (tickEpoch == system.epoch)
        {
            if (isFullExternalComputationTime(tickDate))
            {
                // Trigger time
                if (!isInFullExternalTime)
                {
                    isInFullExternalTime = true;

                    // Turn off the qubic mining phase
                    score->initMiningData(m256i::zero());

                    // Start the custom mining phase
                    isBeginOfCustomMiningPhase = true;
                }
                isInCustomMiningPhase = 1;
            }
            else
            {
                // Not in the full external phase anymore
                isInFullExternalTime = false;
            }
        }

        // Incase of the full custom mining is just end. The setNewMiningSeed() will wait for next period of qubic mining phase
        if (!isInFullExternalTime)
        {
            const unsigned int r = getTickInMiningPhaseCycle();
            if (!r)
            {
                setNewMiningSeed();
            }
            else
            {
                if (r == INTERNAL_COMPUTATIONS_INTERVAL + 3) // 3 is added because of 3-tick shift for transaction confirmation
                {
                    score->initMiningData(m256i::zero());
                }

                // Setting for custom mining phase
                isInCustomMiningPhase = 0;
                if (r >= INTERNAL_COMPUTATIONS_INTERVAL)
                {
                    isInCustomMiningPhase = 1;
                    // Begin of custom mining phase. Turn the flag on so we can reset some state variables
                    if (r == INTERNAL_COMPUTATIONS_INTERVAL)
                    {
                        isBeginOfCustomMiningPhase = true;
                    }
                }
            }
        }
    }

    // Variables need to be reset in the beginning of custom mining phase
    if (isBeginOfCustomMiningPhase)
    {
        beginCustomMiningPhase();
    }

    // Turn on the custom mining state 
    ACQUIRE(gIsInCustomMiningStateLock);
    gIsInCustomMiningState = isInCustomMiningPhase;
    RELEASE(gIsInCustomMiningStateLock);
}

// Updates the global numberTickTransactions based on the tick data in the tick storage.
static void updateNumberOfTickTransactions()
{
    const unsigned int tickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    
    ts.tickData.acquireLock();
    if (ts.tickData[tickIndex].epoch == 0 || ts.tickData[tickIndex].epoch == INVALIDATED_TICK_DATA)
    {
        numberTickTransactions = -1;
    }
    else
    {
        numberTickTransactions = 0;
        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(ts.tickData[tickIndex].transactionDigests[transactionIndex]))
            {
                numberTickTransactions++;
            }
        }
    }
    ts.tickData.releaseLock();
}

// Disabling the optimizer for requestProcessor() is a workaround introduced to solve an issue
// that has been observed in testnets/2024-11-23-release-227-qvault.
// In this test, the processors calling requestProcessor() were stuck before entering the function.
// Probably, this was caused by a bug in the optimizer, because disabling the optimizer solved the
// problem.
OPTIMIZE_OFF()
static void requestProcessor(void* ProcedureArgument)
{
    enableAVX();

    const unsigned long long processorNumber = getRunningProcessorID();

    Processor* processor = (Processor*)ProcedureArgument;
    RequestResponseHeader* header = (RequestResponseHeader*)processor->buffer;
    while (!shutDownNode)
    {
        checkinTime(processorNumber);
        // in epoch transition, wait here
        if (epochTransitionState)
        {
            _InterlockedIncrement(&epochTransitionWaitingRequestProcessors);
            BEGIN_WAIT_WHILE(epochTransitionState)
            {
                {
                    // to avoid potential overflow: consume the queue without processing requests
                    ACQUIRE(requestQueueTailLock);
                    if (requestQueueElementTail == requestQueueElementHead)
                    {
                        RELEASE(requestQueueTailLock);
                    }
                    else
                    {
                        {
                            RequestResponseHeader* requestHeader = (RequestResponseHeader*)&requestQueueBuffer[requestQueueElements[requestQueueElementTail].offset];
                            copyMem(header, requestHeader, requestHeader->size());
                            requestQueueBufferTail += requestHeader->size();
                        }

                        Peer* peer = requestQueueElements[requestQueueElementTail].peer;

                        if (requestQueueBufferTail > REQUEST_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
                        {
                            requestQueueBufferTail = 0;
                        }
                        requestQueueElementTail++;
                        RELEASE(requestQueueTailLock);
                    }
                }
            }
            END_WAIT_WHILE();
            _InterlockedDecrement(&epochTransitionWaitingRequestProcessors);
        }

        // try to compute a solution if any is queued and this thread is assigned to compute solution
        if (solutionProcessorFlags[processorNumber])
        {
            PROFILE_NAMED_SCOPE("requestProcessor(): solution processing");
            score->tryProcessSolution(processorNumber);
        }
        
        if (requestQueueElementTail == requestQueueElementHead)
        {
            _mm_pause();
        }
        else
        {
            ACQUIRE(requestQueueTailLock);

            if (requestQueueElementTail == requestQueueElementHead)
            {
                RELEASE(requestQueueTailLock);
            }
            else
            {
                PROFILE_NAMED_SCOPE("requestProcessor(): request processing");
                const unsigned long long beginningTick = __rdtsc();

                {
                    RequestResponseHeader* requestHeader = (RequestResponseHeader*)&requestQueueBuffer[requestQueueElements[requestQueueElementTail].offset];
                    copyMem(header, requestHeader, requestHeader->size());
                    requestQueueBufferTail += requestHeader->size();
                }

                Peer* peer = requestQueueElements[requestQueueElementTail].peer;

                if (requestQueueBufferTail > REQUEST_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
                {
                    requestQueueBufferTail = 0;
                }
                requestQueueElementTail++;

                RELEASE(requestQueueTailLock);
                switch (header->type())
                {
                case ExchangePublicPeers::type:
                {
                    processExchangePublicPeers(peer, header);
                }
                break;

                case BroadcastMessage::type:
                {
                    processBroadcastMessage(processorNumber, header);
                }
                break;

                case BroadcastComputors::type:
                {
                    processBroadcastComputors(peer, header);
                }
                break;

                case BroadcastTick::type:
                {
                    processBroadcastTick(peer, header);
                }
                break;

                case BroadcastFutureTickData::type:
                {
                    processBroadcastFutureTickData(peer, header);
                }
                break;

                case BROADCAST_TRANSACTION:
                {
                    processBroadcastTransaction(peer, header);
                }
                break;

                case RequestComputors::type:
                {
                    processRequestComputors(peer, header);
                }
                break;

                case RequestQuorumTick::type:
                {
                    processRequestQuorumTick(peer, header);
                }
                break;

                case RequestTickData::type:
                {
                    processRequestTickData(peer, header);
                }
                break;

                case REQUEST_TICK_TRANSACTIONS:
                {
                    processRequestTickTransactions(peer, header);
                }
                break;

                case REQUEST_TRANSACTION_INFO:
                {
                    processRequestTransactionInfo(peer, header);
                }
                break;

                case REQUEST_CURRENT_TICK_INFO:
                {
                    processRequestCurrentTickInfo(peer, header);
                }
                break;

                case RESPOND_CURRENT_TICK_INFO:
                {
                    processResponseCurrentTickInfo(peer, header);
                }
                break;

                case REQUEST_ENTITY:
                {
                    processRequestEntity(peer, header);
                }
                break;

                case RequestContractIPO::type:
                {
                    processRequestContractIPO(peer, header);
                }
                break;

                case RequestIssuedAssets::type:
                {
                    processRequestIssuedAssets(peer, header);
                }
                break;

                case RequestOwnedAssets::type:
                {
                    processRequestOwnedAssets(peer, header);
                }
                break;

                case RequestPossessedAssets::type:
                {
                    processRequestPossessedAssets(peer, header);
                }
                break;

                case RequestContractFunction::type:
                {
                    processRequestContractFunction(peer, processorNumber, header);
                }
                break;

                case RequestLog::type:
                {
                    logger.processRequestLog(processorNumber, peer, header);
                }
                break;

                case RequestLogIdRangeFromTx::type:
                {
                    logger.processRequestTxLogInfo(processorNumber, peer, header);
                }
                break;

                case RequestAllLogIdRangesFromTick::type:
                {
                    logger.processRequestTickTxLogInfo(processorNumber, peer, header);
                }
                break;

                case RequestPruningLog::type:
                {
                    logger.processRequestPrunePageFile(peer, header);
                }
                break;

                case RequestLogStateDigest::type:
                {
                    logger.processRequestGetLogDigest(peer, header);
                }
                break;

                case REQUEST_SYSTEM_INFO:
                {
                    processRequestSystemInfo(peer, header);
                }
                break;

                case RequestAssets::type:
                {
                    processRequestAssets(peer, header);
                }
                break;
                case RequestedCustomMiningSolutionVerification::type:
                {
                    processRequestedCustomMiningSolutionVerificationRequest(peer, header);
                }
                break;
                case RequestedCustomMiningData::type:
                {
                    processCustomMiningDataRequest(peer, processorNumber, header);
                }
                break;

                case SpecialCommand::type:
                {
                    processSpecialCommand(peer, header);
                }
                break;

#if ADDON_TX_STATUS_REQUEST
                /* qli: process RequestTxStatus message */
                case REQUEST_TX_STATUS:
                {
                    processRequestConfirmedTx(processorNumber, peer, header);
                }
                break;
#endif

                }

                queueProcessingNumerator += __rdtsc() - beginningTick;
                queueProcessingDenominator++;

                _InterlockedIncrement64(&numberOfProcessedRequests);
            }
        }
    }
}
OPTIMIZE_ON()

static void contractProcessor(void*)
{
    enableAVX();

    PROFILE_SCOPE();

    const unsigned long long processorNumber = getRunningProcessorID();

    unsigned int executedContractIndex;
    switch (contractProcessorPhase)
    {
    case INITIALIZE:
    {
        for (executedContractIndex = 1; executedContractIndex < contractCount; executedContractIndex++)
        {
            if (system.epoch == contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                setMem(contractStates[executedContractIndex], contractDescriptions[executedContractIndex].stateSize, 0);
                QpiContextSystemProcedureCall qpiContext(executedContractIndex, INITIALIZE);
                qpiContext.call();
            }
        }
    }
    break;

    case BEGIN_EPOCH:
    {
        for (executedContractIndex = 1; executedContractIndex < contractCount; executedContractIndex++)
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                QpiContextSystemProcedureCall qpiContext(executedContractIndex, BEGIN_EPOCH);
                qpiContext.call();
            }
        }
    }
    break;

    case BEGIN_TICK:
    {
        for (executedContractIndex = 1; executedContractIndex < contractCount; executedContractIndex++)
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                QpiContextSystemProcedureCall qpiContext(executedContractIndex, BEGIN_TICK);
                qpiContext.call();
            }
        }
    }
    break;

    case END_TICK:
    {
        for (executedContractIndex = contractCount; executedContractIndex-- > 1; )
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                QpiContextSystemProcedureCall qpiContext(executedContractIndex, END_TICK);
                qpiContext.call();
            }
        }
    }
    break;

    case END_EPOCH:
    {
        for (executedContractIndex = contractCount; executedContractIndex-- > 1; )
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                QpiContextSystemProcedureCall qpiContext(executedContractIndex, END_EPOCH);
                qpiContext.call();
            }
        }
    }
    break;

    // TODO: rename to invoke (with option to have amount)
    case USER_PROCEDURE_CALL:
    case POST_INCOMING_TRANSFER:
    {
        const Transaction* transaction = contractProcessorTransaction;
        ASSERT(transaction && transaction->checkValidity());

        ASSERT(transaction->destinationPublicKey.m256i_u64[0] < contractCount);
        ASSERT(transaction->destinationPublicKey.m256i_u64[1] == 0);
        ASSERT(transaction->destinationPublicKey.m256i_u64[2] == 0);
        ASSERT(transaction->destinationPublicKey.m256i_u64[3] == 0);

        unsigned int contractIndex = (unsigned int)transaction->destinationPublicKey.m256i_u64[0];
        ASSERT(system.epoch >= contractDescriptions[contractIndex].constructionEpoch);
        ASSERT(system.epoch < contractDescriptions[contractIndex].destructionEpoch);

        if (transaction->amount > 0 && contractSystemProcedures[contractIndex][POST_INCOMING_TRANSFER])
        {
            // Run callback system procedure POST_INCOMING_TRANSFER
            const unsigned char type = contractProcessorPostIncomingTransferType;
            if (contractProcessorPhase == USER_PROCEDURE_CALL)
            {
                ASSERT(type == QPI::TransferType::procedureTransaction);
            }
            else
            {
                ASSERT(
                    type == QPI::TransferType::standardTransaction
                    || type == QPI::TransferType::revenueDonation
                    || type == QPI::TransferType::ipoBidRefund
                );
            }

            QpiContextSystemProcedureCall qpiContext(contractIndex, POST_INCOMING_TRANSFER);
            QPI::PostIncomingTransfer_input input{ transaction->sourcePublicKey, transaction->amount, type };
            qpiContext.call(input);
        }

        if (contractProcessorPhase == USER_PROCEDURE_CALL)
        {
            // Run user procedure
            ASSERT(contractUserProcedures[contractIndex][transaction->inputType]);

            QpiContextUserProcedureCall qpiContext(contractIndex, transaction->sourcePublicKey, transaction->amount);
            qpiContext.call(transaction->inputType, transaction->inputPtr(), transaction->inputSize);

            if (contractActionTracker.getOverallQuTransferBalance(transaction->sourcePublicKey) == 0)
                contractProcessorTransactionMoneyflew = 0;
            else
                contractProcessorTransactionMoneyflew = 1;
        }

        contractProcessorTransaction = 0;
    }
    break;
    }

    if (!isVirtualMachine)
    {
        // at the moment, this can only apply on BM
        // Set state to inactive, signaling end of contractProcessor() execution before contractProcessorShutdownCallback()
        // for reducing waiting time in tick processor.
        contractProcessorState = 0;
    }
}

// Notify dest of incoming transfer if dest is a contract.
// CAUTION: Cannot be called from contract processor or main processor! If called from QPI functions, it will get stuck.
static void notifyContractOfIncomingTransfer(const m256i& source, const m256i& dest, long long amount, unsigned char type)
{
    // Only notify if amount > 0 and dest is contract
    if (amount <= 0 || dest.u64._0 >= contractCount || dest.u64._1 || dest.u64._2 || dest.u64._3)
        return;

    // Also don't run contract processor if the callback isn't implemented in the dest contract
    if (!contractSystemProcedures[dest.u64._0][POST_INCOMING_TRANSFER])
        return;

    ASSERT(type == QPI::TransferType::revenueDonation || type == QPI::TransferType::ipoBidRefund);

    // Caution: Transaction has no signature, because it is a pseudo-transaction just used to hand over information to
    // the contract processor.
    Transaction tx;
    tx.sourcePublicKey = source;
    tx.destinationPublicKey = dest;
    tx.amount = amount;
    tx.tick = system.tick;
    tx.inputType = 0;
    tx.inputSize = 0;

    contractProcessorTransaction = &tx;
    contractProcessorPostIncomingTransferType = type;
    contractProcessorPhase = POST_INCOMING_TRANSFER;
    contractProcessorState = 1;
    WAIT_WHILE(contractProcessorState);
}


static void processTickTransactionContractIPO(const Transaction* transaction, const int spectrumIndex, const unsigned int contractIndex)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(transaction->tick == system.tick);
    ASSERT(!transaction->amount && transaction->inputSize == sizeof(ContractIPOBid));
    ASSERT(spectrumIndex >= 0);
    ASSERT(contractIndex < contractCount);
    ASSERT(system.epoch == (contractDescriptions[contractIndex].constructionEpoch - 1));

    ContractIPOBid* contractIPOBid = (ContractIPOBid*)transaction->inputPtr();
    bidInContractIPO(contractIPOBid->price, contractIPOBid->quantity, transaction->sourcePublicKey, spectrumIndex, contractIndex);
}

// Return if money flew
static bool processTickTransactionContractProcedure(const Transaction* transaction, const int spectrumIndex, const unsigned int contractIndex)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(transaction->tick == system.tick);
    ASSERT(spectrumIndex >= 0);
    ASSERT(contractIndex < contractCount);
    ASSERT(system.epoch >= contractDescriptions[contractIndex].constructionEpoch);
    ASSERT(system.epoch < contractDescriptions[contractIndex].destructionEpoch);

    if (contractUserProcedures[contractIndex][transaction->inputType])
    {
        // Run user procedure call of transaction in contract processor
        // and wait for completion
        // With USER_PROCEDURE_CALL, contract processor also notifies the contract
        // of the incoming transfer if the invocation reward = amount > 0.
        contractProcessorTransaction = transaction;
        contractProcessorPostIncomingTransferType = QPI::TransferType::procedureTransaction;
        contractProcessorPhase = USER_PROCEDURE_CALL;
        contractProcessorState = 1;
        WAIT_WHILE(contractProcessorState);

        return contractProcessorTransactionMoneyflew;
    }
    else if (transaction->amount > 0)
    {
        // Transaction sending qu to contract without invoking registered user procedure:
        // Run POST_INCOMING_TRANSFER notification in contract processor and wait for completion.
        contractProcessorTransaction = transaction;
        contractProcessorPostIncomingTransferType = QPI::TransferType::standardTransaction;
        contractProcessorPhase = POST_INCOMING_TRANSFER;
        contractProcessorState = 1;
        WAIT_WHILE(contractProcessorState);
    }

    // if transaction tries to invoke non-registered procedure, transaction amount is not reimbursed
    return transaction->amount > 0;
}

static void processTickTransactionSolution(const MiningSolutionTransaction* transaction, const unsigned long long processorNumber)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(transaction->tick == system.tick);
    ASSERT(isZero(transaction->destinationPublicKey));
    ASSERT(transaction->amount >=MiningSolutionTransaction::minAmount()
            && transaction->inputSize == 64
            && transaction->inputType == MiningSolutionTransaction::transactionType());

    m256i data[3] = { transaction->sourcePublicKey, transaction->miningSeed, transaction->nonce };
    static_assert(sizeof(data) == 3 * 32, "Unexpected array size");
    unsigned int flagIndex;
    KangarooTwelve(data, sizeof(data), &flagIndex, sizeof(flagIndex));
    if (!(minerSolutionFlags[flagIndex >> 6] & (1ULL << (flagIndex & 63))))
    {
        minerSolutionFlags[flagIndex >> 6] |= (1ULL << (flagIndex & 63));

        unsigned int solutionScore = (*::score)(processorNumber, transaction->sourcePublicKey, transaction->miningSeed, transaction->nonce);
        if (score->isValidScore(solutionScore))
        {
            resourceTestingDigest ^= solutionScore;
            KangarooTwelve(&resourceTestingDigest, sizeof(resourceTestingDigest), &resourceTestingDigest, sizeof(resourceTestingDigest));

            const int threshold = (system.epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[system.epoch] : SOLUTION_THRESHOLD_DEFAULT;
            if (score->isGoodScore(solutionScore, threshold))
            {
                // Solution deposit return
                {
                    increaseEnergy(transaction->sourcePublicKey, transaction->amount);

                    const QuTransfer quTransfer = { m256i::zero(), transaction->sourcePublicKey, transaction->amount };
                    logger.logQuTransfer(quTransfer);
                }

                for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
                {
                    if (transaction->sourcePublicKey == computorPublicKeys[i])
                    {
                        ACQUIRE(solutionsLock);

                        unsigned int j;
                        for (j = 0; j < system.numberOfSolutions; j++)
                        {
                            if (transaction->nonce == system.solutions[j].nonce
                                && transaction->miningSeed == system.solutions[j].miningSeed
                                && transaction->sourcePublicKey == system.solutions[j].computorPublicKey)
                            {
                                solutionPublicationTicks[j] = SOLUTION_RECORDED_FLAG;

                                break;
                            }
                        }
                        if (j == system.numberOfSolutions
                            && system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS)
                        {
                            system.solutions[system.numberOfSolutions].computorPublicKey = transaction->sourcePublicKey;
                            system.solutions[system.numberOfSolutions].miningSeed = transaction->miningSeed;
                            system.solutions[system.numberOfSolutions].nonce = transaction->nonce;
                            solutionPublicationTicks[system.numberOfSolutions++] = SOLUTION_RECORDED_FLAG;
                        }

                        RELEASE(solutionsLock);

                        break;
                    }
                }

                ACQUIRE(minerScoreArrayLock);
                unsigned int minerIndex;
                for (minerIndex = 0; minerIndex < numberOfMiners; minerIndex++)
                {
                    if (transaction->sourcePublicKey == minerPublicKeys[minerIndex])
                    {
                        minerScores[minerIndex]++;

                        break;
                    }
                }
                if (minerIndex == numberOfMiners
                    && numberOfMiners < MAX_NUMBER_OF_MINERS)
                {
                    minerPublicKeys[numberOfMiners] = transaction->sourcePublicKey;
                    minerScores[numberOfMiners++] = 1;
                }

                const m256i tmpPublicKey = minerPublicKeys[minerIndex];
                const unsigned int tmpScore = minerScores[minerIndex];
                while (minerIndex > (unsigned int)(minerIndex < NUMBER_OF_COMPUTORS ? 0 : NUMBER_OF_COMPUTORS)
                    && minerScores[minerIndex - 1] < minerScores[minerIndex])
                {
                    minerPublicKeys[minerIndex] = minerPublicKeys[minerIndex - 1];
                    minerScores[minerIndex] = minerScores[minerIndex - 1];
                    minerPublicKeys[--minerIndex] = tmpPublicKey;
                    minerScores[minerIndex] = tmpScore;
                }

                // combine 225 worst current computors with 225 best candidates
                for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS - QUORUM; i++)
                {
                    competitorPublicKeys[i] = minerPublicKeys[QUORUM + i];
                    competitorScores[i] = minerScores[QUORUM + i];
                    competitorComputorStatuses[i] = true;

                    if (NUMBER_OF_COMPUTORS + i < numberOfMiners)
                    {
                        competitorPublicKeys[i + (NUMBER_OF_COMPUTORS - QUORUM)] = minerPublicKeys[NUMBER_OF_COMPUTORS + i];
                        competitorScores[i + (NUMBER_OF_COMPUTORS - QUORUM)] = minerScores[NUMBER_OF_COMPUTORS + i];
                    }
                    else
                    {
                        competitorScores[i + (NUMBER_OF_COMPUTORS - QUORUM)] = 0;
                    }
                    competitorComputorStatuses[i + (NUMBER_OF_COMPUTORS - QUORUM)] = false;
                }
                RELEASE(minerScoreArrayLock);

                // bubble sorting -> top 225 from competitorPublicKeys have computors and candidates which are the best from that subset
                for (unsigned int i = NUMBER_OF_COMPUTORS - QUORUM; i < (NUMBER_OF_COMPUTORS - QUORUM) * 2; i++)
                {
                    int j = i;
                    const m256i tmpPublicKey = competitorPublicKeys[j];
                    const unsigned int tmpScore = competitorScores[j];
                    const bool tmpComputorStatus = false;
                    while (j
                        && competitorScores[j - 1] < competitorScores[j])
                    {
                        competitorPublicKeys[j] = competitorPublicKeys[j - 1];
                        competitorScores[j] = competitorScores[j - 1];
                        competitorComputorStatuses[j] = competitorComputorStatuses[j - 1];
                        competitorPublicKeys[--j] = tmpPublicKey;
                        competitorScores[j] = tmpScore;
                        competitorComputorStatuses[j] = tmpComputorStatus;
                    }
                }

                minimumComputorScore = competitorScores[NUMBER_OF_COMPUTORS - QUORUM - 1];

                unsigned char candidateCounter = 0;
                for (unsigned int i = 0; i < (NUMBER_OF_COMPUTORS - QUORUM) * 2; i++)
                {
                    if (!competitorComputorStatuses[i])
                    {
                        minimumCandidateScore = competitorScores[i];
                        candidateCounter++;
                    }
                }
                if (candidateCounter < NUMBER_OF_COMPUTORS - QUORUM)
                {
                    minimumCandidateScore = minimumComputorScore;
                }

                ACQUIRE(minerScoreArrayLock);
                for (unsigned int i = 0; i < QUORUM; i++)
                {
                    system.futureComputors[i] = minerPublicKeys[i];
                }
                RELEASE(minerScoreArrayLock);

                for (unsigned int i = QUORUM; i < NUMBER_OF_COMPUTORS; i++)
                {
                    system.futureComputors[i] = competitorPublicKeys[i - QUORUM];
                }
            }
        }        
    }
    else
    {
        for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
        {
            if (transaction->sourcePublicKey == computorPublicKeys[i])
            {
                ACQUIRE(solutionsLock);

                unsigned int j;
                for (j = 0; j < system.numberOfSolutions; j++)
                {
                    if (transaction->nonce == system.solutions[j].nonce
                        && transaction->miningSeed == system.solutions[j].miningSeed
                        && transaction->sourcePublicKey == system.solutions[j].computorPublicKey)
                    {
                        solutionPublicationTicks[j] = SOLUTION_RECORDED_FLAG;

                        break;
                    }
                }
                if (j == system.numberOfSolutions
                    && system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS)
                {
                    system.solutions[system.numberOfSolutions].computorPublicKey = transaction->sourcePublicKey;
                    system.solutions[system.numberOfSolutions].miningSeed = transaction->miningSeed;
                    system.solutions[system.numberOfSolutions].nonce = transaction->nonce;
                    solutionPublicationTicks[system.numberOfSolutions++] = SOLUTION_RECORDED_FLAG;
                }

                RELEASE(solutionsLock);

                break;
            }
        }
    }
}

static void processTickTransactionOracleReplyCommit(const OracleReplyCommitTransaction* transaction)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(isZero(transaction->destinationPublicKey));
    ASSERT(transaction->tick == system.tick);

    // TODO
}

static void processTickTransactionOracleReplyReveal(const OracleReplyRevealTransactionPrefix* transaction)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(isZero(transaction->destinationPublicKey));
    ASSERT(transaction->tick == system.tick);

    // TODO
}

static void processTickTransaction(const Transaction* transaction, const m256i& transactionDigest, const m256i& dataLock, unsigned long long processorNumber)
{
    PROFILE_SCOPE();

    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(transaction->tick == system.tick);

    // Record the tx with digest
    ts.transactionsDigestAccess.acquireLock();
    ts.transactionsDigestAccess.insertTransaction(transactionDigest, transaction);
    ts.transactionsDigestAccess.releaseLock();

    const int spectrumIndex = ::spectrumIndex(transaction->sourcePublicKey);
    if (spectrumIndex >= 0)
    {
        numberOfTransactions++;
        bool moneyFlew = false;
#if ADDON_TX_STATUS_REQUEST
        txStatusData.tickTxIndexStart[system.tick - system.initialTick + 1] = numberOfTransactions; // qli: part of tx_status_request add-on
#endif
        if (decreaseEnergy(spectrumIndex, transaction->amount))
        {
            increaseEnergy(transaction->destinationPublicKey, transaction->amount);
            {
                const QuTransfer quTransfer = { transaction->sourcePublicKey , transaction->destinationPublicKey , transaction->amount };
                logger.logQuTransfer(quTransfer);
            }
            if (transaction->amount)
            {
                moneyFlew = true;
            }

            if (isZero(transaction->destinationPublicKey))
            {
                // Destination is system
                switch (transaction->inputType)
                {
                case VOTE_COUNTER_INPUT_TYPE:
                {
                    voteCounter.processTransactionData(transaction, dataLock);
                }
                break;

                case FileHeaderTransaction::transactionType():
                {
                    if (transaction->amount >= FileFragmentTransactionPrefix::minAmount()
                        && transaction->inputSize >= FileFragmentTransactionPrefix::minInputSize())
                    {
                        // Do nothing
                    }
                }
                break;

                case FileFragmentTransactionPrefix::transactionType():
                {
                    if (transaction->amount >= FileFragmentTransactionPrefix::minAmount()
                        && transaction->inputSize >= FileFragmentTransactionPrefix::minInputSize())
                    {
                        // Do nothing
                    }
                }
                break;

                case FileTrailerTransaction::transactionType():
                {
                    if (transaction->amount >= FileFragmentTransactionPrefix::minAmount()
                        && transaction->inputSize >= FileFragmentTransactionPrefix::minInputSize())
                    {
                        // Do nothing
                    }
                }
                break;

                case MiningSolutionTransaction::transactionType():
                {
                    if (transaction->amount >= MiningSolutionTransaction::minAmount()
                        && transaction->inputSize >= MiningSolutionTransaction::minInputSize())
                    {
                        processTickTransactionSolution((MiningSolutionTransaction*)transaction, processorNumber);
                    }
                }
                break;

                case OracleReplyCommitTransaction::transactionType():
                {
                    if (computorIndex(transaction->sourcePublicKey) >= 0
                        && transaction->inputSize == sizeof(OracleReplyCommitTransaction))
                    {
                        processTickTransactionOracleReplyCommit((OracleReplyCommitTransaction*)transaction);
                    }
                }
                break;

                case OracleReplyRevealTransactionPrefix::transactionType():
                {
                    if (computorIndex(transaction->sourcePublicKey) >= 0
                        && transaction->inputSize >= sizeof(OracleReplyRevealTransactionPrefix) + sizeof(OracleReplyRevealTransactionPostfix))
                    {
                        processTickTransactionOracleReplyReveal((OracleReplyRevealTransactionPrefix*)transaction);
                    }
                }
                break;

                case CustomMiningSolutionTransaction::transactionType():
                {
                    gCustomMiningSharesCounter.processTransactionData(transaction, dataLock);
                }
                break;

                }
            }
            else
            {
                // Destination is a contract or any other entity.
                // Contracts are identified by their index stored in the first 64 bits of the id, all
                // other bits are zeroed. However, the max number of contracts is limited to 2^32 - 1,
                // only 32 bits are used for the contract index.
                m256i maskedDestinationPublicKey = transaction->destinationPublicKey;
                maskedDestinationPublicKey.m256i_u64[0] &= ~(MAX_NUMBER_OF_CONTRACTS - 1ULL);
                unsigned int contractIndex = (unsigned int)transaction->destinationPublicKey.m256i_u64[0];
                if (isZero(maskedDestinationPublicKey)
                    && contractIndex < contractCount)
                {
                    // Contract transactions
                    if (system.epoch == (contractDescriptions[contractIndex].constructionEpoch - 1))
                    {
                        // IPO
                        if (!transaction->amount
                            && transaction->inputSize == sizeof(ContractIPOBid))
                        {
                            processTickTransactionContractIPO(transaction, spectrumIndex, contractIndex);
                        }
                    }
                    else if (system.epoch >= contractDescriptions[contractIndex].constructionEpoch 
                        && system.epoch < contractDescriptions[contractIndex].destructionEpoch)
                    {
                        // Regular contract procedure invocation
                        moneyFlew = processTickTransactionContractProcedure(transaction, spectrumIndex, contractIndex);
                    }
                }
            }
        }

#if ADDON_TX_STATUS_REQUEST
        saveConfirmedTx(numberOfTransactions - 1, moneyFlew, system.tick, transactionDigest); // qli: save tx
#endif
    }
}

static void makeAndBroadcastTickVotesTransaction(int i, BroadcastFutureTickData& td, int txSlot)
{
    PROFILE_NAMED_SCOPE("processTick(): broadcast vote counter tx");
    ASSERT(txSlot < NUMBER_OF_TRANSACTIONS_PER_TICK);
    auto& payload = voteCounterPayload; // note: not thread-safe
    payload.transaction.sourcePublicKey = computorPublicKeys[ownComputorIndicesMapping[i]];
    payload.transaction.destinationPublicKey = m256i::zero();
    payload.transaction.amount = 0;
    payload.transaction.tick = system.tick + TICK_VOTE_COUNTER_PUBLICATION_OFFSET;
    payload.transaction.inputType = VOTE_COUNTER_INPUT_TYPE;
    payload.transaction.inputSize = sizeof(payload.data) + sizeof(payload.dataLock);
    voteCounter.compressNewVotesPacket(system.tick - 675, system.tick + 1, ownComputorIndices[i], payload.data);
    payload.dataLock = td.tickData.timelock;
    unsigned char digest[32];
    KangarooTwelve(&payload.transaction, sizeof(payload.transaction) + sizeof(payload.data) + sizeof(payload.dataLock), digest, sizeof(digest));
    sign(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, payload.signature);
    enqueueResponse(NULL, sizeof(payload), BROADCAST_TRANSACTION, 0, &payload);

    // copy the content of this vote packet to local memory
    unsigned int tickIndex = ts.tickToIndexCurrentEpoch(td.tickData.tick);
    unsigned int transactionSize = sizeof(voteCounterPayload);
    KangarooTwelve(&payload, transactionSize, digest, sizeof(digest));
    auto* tsReqTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);
    if (txSlot < NUMBER_OF_TRANSACTIONS_PER_TICK) // valid slot
    {
        // TODO: refactor function add transaction to txStorage
        ts.tickTransactions.acquireLock();
        if (!tsReqTickTransactionOffsets[txSlot]) // not yet have value
        {
            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch) //have enough space
            {
                td.tickData.transactionDigests[txSlot] = m256i(digest);
                tsReqTickTransactionOffsets[txSlot] = ts.nextTickTransactionOffset;
                copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), &payload, transactionSize);
                ts.nextTickTransactionOffset += transactionSize;
            }
        }
        ts.tickTransactions.releaseLock();
    }
}

static bool makeAndBroadcastCustomMiningTransaction(int i, BroadcastFutureTickData& td, int txSlot)
{
    if (!gCustomMiningBroadcastTxBuffer[i].isBroadcasted)
    {
        gCustomMiningBroadcastTxBuffer[i].isBroadcasted = true;
        auto& payload = gCustomMiningBroadcastTxBuffer[i].payload;
        if (gCustomMiningSharesCounter.isEmptyPacket(payload.packedScore) == false) // only continue processing if packet isn't empty
        {
            payload.transaction.tick = system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET;
            payload.dataLock = td.tickData.timelock;
            unsigned char digest[32];
            KangarooTwelve(&payload.transaction, sizeof(payload.transaction) + sizeof(payload.packedScore) + sizeof(payload.dataLock), digest, sizeof(digest));
            sign(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, payload.signature);
            enqueueResponse(NULL, sizeof(payload), BROADCAST_TRANSACTION, 0, &payload);

            // copy the content of this xmr point packet to local memory
            unsigned int tickIndex = ts.tickToIndexCurrentEpoch(td.tickData.tick);
            unsigned int transactionSize = sizeof(payload);
            KangarooTwelve(&payload, transactionSize, digest, sizeof(digest));
            auto* tsReqTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);
            if (txSlot < NUMBER_OF_TRANSACTIONS_PER_TICK) // valid slot
            {
                // TODO: refactor function add transaction to txStorage
                ts.tickTransactions.acquireLock();
                if (!tsReqTickTransactionOffsets[txSlot]) // not yet have value
                {
                    if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch) //have enough space
                    {
                        td.tickData.transactionDigests[txSlot] = m256i(digest);
                        tsReqTickTransactionOffsets[txSlot] = ts.nextTickTransactionOffset;
                        copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), &payload, transactionSize);
                        ts.nextTickTransactionOffset += transactionSize;
                    }
                }
                ts.tickTransactions.releaseLock();
            }
            return true;
        }
    }
    return false;
}

OPTIMIZE_OFF()
static void processTick(unsigned long long processorNumber)
{
    PROFILE_SCOPE();

    if (system.tick > system.initialTick)
    {
        etalonTick.prevResourceTestingDigest = resourceTestingDigest;
        etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
        getUniverseDigest(etalonTick.prevUniverseDigest);
        getComputerDigest(etalonTick.prevComputerDigest);
        etalonTick.prevTransactionBodyDigest = etalonTick.saltedTransactionBodyDigest;
    }
    else if (system.tick == system.initialTick) // the first tick of an epoch
    {
        // RULE: prevDigests of tick T are the digests of tick T-1, so epoch number doesn't matter.
        // For seamless transition, spectrum and universe and computer have been changed after endEpoch event
        // (miner rewards, IPO finalizing, contract endEpoch procedures,...)
        // Here we still let prevDigests == digests of the last tick of last epoch
        // so that lite client can verify the state of spectrum

#if START_NETWORK_FROM_SCRATCH // only update it if the whole network starts from scratch
        // everything starts from files, there is no previous tick of the last epoch
        // thus, prevDigests are the digests of the files
        if (system.epoch == EPOCH)
        {
            etalonTick.prevResourceTestingDigest = resourceTestingDigest;
            etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
            getUniverseDigest(etalonTick.prevUniverseDigest);
            getComputerDigest(etalonTick.prevComputerDigest);
            etalonTick.prevTransactionBodyDigest = 0;
        }
#endif
    }
    else
    {
        // it should never go here
    }

    // Ensure to only call INITIALIZE and BEGIN_EPOCH once per epoch:
    // system.initialTick usually is the first tick of the epoch, except when the network is restarted
    // from scratch with a new TICK (which shall be indicated by TICK_IS_FIRST_TICK_OF_EPOCH == 0).
    // However, after seamless epoch transition (system.epoch > EPOCH), system.initialTick is the first
    // tick of the epoch in any case.
    if (system.tick == system.initialTick && (TICK_IS_FIRST_TICK_OF_EPOCH || system.epoch > EPOCH))
    {
        PROFILE_NAMED_SCOPE_BEGIN("processTick(): INITIALIZE");
        logger.registerNewTx(system.tick, logger.SC_INITIALIZE_TX);
        contractProcessorPhase = INITIALIZE;
        contractProcessorState = 1;
        WAIT_WHILE(contractProcessorState);
        PROFILE_SCOPE_END();

        PROFILE_NAMED_SCOPE_BEGIN("processTick(): BEGIN_EPOCH");
        logger.registerNewTx(system.tick, logger.SC_BEGIN_EPOCH_TX);
        contractProcessorPhase = BEGIN_EPOCH;
        contractProcessorState = 1;
        WAIT_WHILE(contractProcessorState);
        PROFILE_SCOPE_END();
    }

    PROFILE_NAMED_SCOPE_BEGIN("processTick(): BEGIN_TICK");
    logger.registerNewTx(system.tick, logger.SC_BEGIN_TICK_TX);
    contractProcessorPhase = BEGIN_TICK;
    contractProcessorState = 1;
    WAIT_WHILE(contractProcessorState);
    PROFILE_SCOPE_END();

    unsigned int tickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    ts.tickData.acquireLock();
    copyMem(&nextTickData, &ts.tickData[tickIndex], sizeof(TickData));
    ts.tickData.releaseLock();
    unsigned long long solutionProcessStartTick = __rdtsc(); // for tracking the time processing solutions
    if (nextTickData.epoch == system.epoch)
    {
        auto* tsCurrentTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);
#if ADDON_TX_STATUS_REQUEST
        txStatusData.tickTxIndexStart[system.tick - system.initialTick] = numberOfTransactions; // qli: part of tx_status_request add-on
#endif
        PROFILE_NAMED_SCOPE_BEGIN("processTick(): pre-scan solutions");
        // reset solution task queue
        score->resetTaskQueue();
        // pre-scan any solution tx and add them to solution task queue
        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(nextTickData.transactionDigests[transactionIndex]))
            {
                if (tsCurrentTickTransactionOffsets[transactionIndex])
                {
                    Transaction* transaction = ts.tickTransactions(tsCurrentTickTransactionOffsets[transactionIndex]);
                    ASSERT(transaction->checkValidity());
                    ASSERT(transaction->tick == system.tick);
                    const int spectrumIndex = ::spectrumIndex(transaction->sourcePublicKey);
                    if (spectrumIndex >= 0)
                    {
                        // Solution transactions
                        if (isZero(transaction->destinationPublicKey)
                            && transaction->amount >= MiningSolutionTransaction::minAmount()
                            && transaction->inputType == MiningSolutionTransaction::transactionType())
                        {
                            if (transaction->inputSize == 32 + 32)
                            {
                                const m256i& solution_miningSeed = *(m256i*)transaction->inputPtr();
                                const m256i& solution_nonce = *(m256i*)(transaction->inputPtr() + 32);
                                m256i data[3] = { transaction->sourcePublicKey, solution_miningSeed, solution_nonce };
                                static_assert(sizeof(data) == 3 * 32, "Unexpected array size");
                                unsigned int flagIndex;
                                KangarooTwelve(data, sizeof(data), &flagIndex, sizeof(flagIndex));
                                if (!(minerSolutionFlags[flagIndex >> 6] & (1ULL << (flagIndex & 63))))
                                {
                                    score->addTask(transaction->sourcePublicKey, solution_miningSeed, solution_nonce);
                                }
                            }
                        }
                    }
                }
            }
        }
        PROFILE_SCOPE_END();

        {
            // Process solutions in this tick and store in cache. In parallel, score->tryProcessSolution() is called by
            // request processors to speed up solution processing.
            PROFILE_NAMED_SCOPE("processTick(): process solutions");
            score->startProcessTaskQueue();
            while (!score->isTaskQueueProcessed())
            {
                score->tryProcessSolution(processorNumber);
            }
            score->stopProcessTaskQueue();
        }
        solutionTotalExecutionTicks = __rdtsc() - solutionProcessStartTick; // for tracking the time processing solutions

        // Process all transaction of the tick
        PROFILE_NAMED_SCOPE_BEGIN("processTick(): process transactions");
        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(nextTickData.transactionDigests[transactionIndex]))
            {
                if (tsCurrentTickTransactionOffsets[transactionIndex])
                {
                    Transaction* transaction = ts.tickTransactions(tsCurrentTickTransactionOffsets[transactionIndex]);
                    logger.registerNewTx(transaction->tick, transactionIndex);
                    processTickTransaction(transaction, nextTickData.transactionDigests[transactionIndex], nextTickData.timelock, processorNumber);
                }
                else
                {
                    while (true)
                    {
                        criticalSituation = 1;
                    }
                }
            }
        }
        PROFILE_SCOPE_END();
    }

    PROFILE_NAMED_SCOPE_BEGIN("processTick(): END_TICK");
    logger.registerNewTx(system.tick, logger.SC_END_TICK_TX);
    contractProcessorPhase = END_TICK;
    contractProcessorState = 1;
    WAIT_WHILE(contractProcessorState);
    PROFILE_SCOPE_END();

    PROFILE_NAMED_SCOPE_BEGIN("processTick(): get spectrum digest");
    unsigned int digestIndex;
    ACQUIRE(spectrumLock);
    for (digestIndex = 0; digestIndex < SPECTRUM_CAPACITY; digestIndex++)
    {
        if (spectrum[digestIndex].latestIncomingTransferTick == system.tick || spectrum[digestIndex].latestOutgoingTransferTick == system.tick)
        {
            KangarooTwelve64To32(&spectrum[digestIndex], &spectrumDigests[digestIndex]);
            spectrumChangeFlags[digestIndex >> 6] |= (1ULL << (digestIndex & 63));
        }
    }
    unsigned int previousLevelBeginning = 0;
    unsigned int numberOfLeafs = SPECTRUM_CAPACITY;
    while (numberOfLeafs > 1)
    {
        for (unsigned int i = 0; i < numberOfLeafs; i += 2)
        {
            if (spectrumChangeFlags[i >> 6] & (3ULL << (i & 63)))
            {
                KangarooTwelve64To32(&spectrumDigests[previousLevelBeginning + i], &spectrumDigests[digestIndex]);
                spectrumChangeFlags[i >> 6] &= ~(3ULL << (i & 63));
                spectrumChangeFlags[i >> 7] |= (1ULL << ((i >> 1) & 63));
            }
            digestIndex++;
        }
        previousLevelBeginning += numberOfLeafs;
        numberOfLeafs >>= 1;
    }
    spectrumChangeFlags[0] = 0;

    etalonTick.saltedSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
    RELEASE(spectrumLock);
    PROFILE_SCOPE_END();

    getUniverseDigest(etalonTick.saltedUniverseDigest);
    getComputerDigest(etalonTick.saltedComputerDigest);

    // prepare custom mining shares packet ONCE
    if (isMainMode())
    {
        // In the begining of mining phase.
        // Also skip the begining of the epoch, because the no thing to do
        if (getTickInMiningPhaseCycle() == 0)
        {
            PROFILE_NAMED_SCOPE("processTick(): prepare custom mining shares tx");            
            long long customMiningCountOverflow = 0;
            for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
            {
                // Update the custom mining share counter
                ACQUIRE(gCustomMiningSharesCountLock);
                for (int k = 0; k < NUMBER_OF_COMPUTORS; k++)
                {
                    if (gCustomMiningSharesCount[k] > CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL)
                    {
                        // Save the max of overflow case
                        if (gCustomMiningSharesCount[k] > customMiningCountOverflow)
                        {
                            customMiningCountOverflow = gCustomMiningSharesCount[k];
                        }

                        // Threshold the value
                        gCustomMiningSharesCount[k] = CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL;
                    }
                }
                gCustomMiningSharesCounter.registerNewShareCount(gCustomMiningSharesCount);
                RELEASE(gCustomMiningSharesCountLock);

                // Save the transaction to be broadcasted
                auto& payload = gCustomMiningBroadcastTxBuffer[i].payload;
                payload.transaction.sourcePublicKey = computorPublicKeys[ownComputorIndicesMapping[i]];
                payload.transaction.destinationPublicKey = m256i::zero();
                payload.transaction.amount = 0;
                payload.transaction.tick = 0;
                payload.transaction.inputType = CustomMiningSolutionTransaction::transactionType();
                payload.transaction.inputSize = sizeof(payload.packedScore) + sizeof(payload.dataLock);
                gCustomMiningSharesCounter.compressNewSharesPacket(ownComputorIndices[i], payload.packedScore);
                // Set the flag to false, indicating that the transaction is not broadcasted yet
                gCustomMiningBroadcastTxBuffer[i].isBroadcasted = false;
            }

            // Keep the max of overflow case
            ATOMIC_MAX64(gCustomMiningStats.maxOverflowShareCount, customMiningCountOverflow);

            // reset the phase counter
            ACQUIRE(gCustomMiningSharesCountLock);
            setMem(gCustomMiningSharesCount, sizeof(gCustomMiningSharesCount), 0);
            RELEASE(gCustomMiningSharesCountLock);
        }
    }

    // If node is MAIN and has ID of tickleader for system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET,
    // prepare tickData and enqueue it
    for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
    {
        if ((system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET) % NUMBER_OF_COMPUTORS == ownComputorIndices[i])
        {
            if (system.tick > system.latestLedTick)
            {
                if (isMainMode())
                {
                    PROFILE_NAMED_SCOPE("processTick(): tick leader tick data construction");

                    // This is the tick leader in MAIN mode -> construct future tick data (selecting transactions to
                    // include into tick)
                    broadcastedFutureTickData.tickData.computorIndex = ownComputorIndices[i] ^ BroadcastFutureTickData::type; // We XOR almost all packets with their type value to make sure an entity cannot be tricked into signing one thing while actually signing something else
                    broadcastedFutureTickData.tickData.epoch = system.epoch;
                    broadcastedFutureTickData.tickData.tick = system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET;

                    broadcastedFutureTickData.tickData.millisecond = utcTime.Nanosecond / 1000000;
                    broadcastedFutureTickData.tickData.second = utcTime.Second;
                    broadcastedFutureTickData.tickData.minute = utcTime.Minute;
                    broadcastedFutureTickData.tickData.hour = utcTime.Hour;
                    broadcastedFutureTickData.tickData.day = utcTime.Day;
                    broadcastedFutureTickData.tickData.month = utcTime.Month;
                    broadcastedFutureTickData.tickData.year = utcTime.Year - 2000;

                    m256i timelockPreimage[3];
                    static_assert(sizeof(timelockPreimage) == 3 * 32, "Unexpected array size");
                    timelockPreimage[0] = etalonTick.saltedSpectrumDigest;
                    timelockPreimage[1] = etalonTick.saltedUniverseDigest;
                    timelockPreimage[2] = etalonTick.saltedComputerDigest;
                    KangarooTwelve(timelockPreimage, sizeof(timelockPreimage), &broadcastedFutureTickData.tickData.timelock, sizeof(broadcastedFutureTickData.tickData.timelock));

                    unsigned int j = 0;

                    ACQUIRE(computorPendingTransactionsLock);

                    // Get indices of pending computor transactions that are scheduled to be included in tickData
                    unsigned int numberOfEntityPendingTransactionIndices = 0;
                    for (unsigned int k = 0; k < NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR; k++)
                    {
                        const Transaction* tx = ((Transaction*)&computorPendingTransactions[k * MAX_TRANSACTION_SIZE]);
                        if (tx->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET)
                        {
                            entityPendingTransactionIndices[numberOfEntityPendingTransactionIndices++] = k;
                        }
                    }

                    // Randomly select computor tx scheduled for the tick until tick is full or all pending tx are included
                    while (j < NUMBER_OF_TRANSACTIONS_PER_TICK && numberOfEntityPendingTransactionIndices)
                    {
                        const unsigned int index = random(numberOfEntityPendingTransactionIndices);

                        const Transaction* pendingTransaction = ((Transaction*)&computorPendingTransactions[entityPendingTransactionIndices[index] * MAX_TRANSACTION_SIZE]);
                        ASSERT(pendingTransaction->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET);
                        {
                            ASSERT(pendingTransaction->checkValidity());
                            const unsigned int transactionSize = pendingTransaction->totalSize();
                            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                            {
                                ts.tickTransactions.acquireLock();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    ts.tickTransactionOffsets(pendingTransaction->tick, j) = ts.nextTickTransactionOffset;
                                    copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), (void*)pendingTransaction, transactionSize);
                                    broadcastedFutureTickData.tickData.transactionDigests[j] = &computorPendingTransactionDigests[entityPendingTransactionIndices[index] * 32ULL];
                                    j++;
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                                ts.tickTransactions.releaseLock();
                            }
                        }

                        entityPendingTransactionIndices[index] = entityPendingTransactionIndices[--numberOfEntityPendingTransactionIndices];
                    }

                    RELEASE(computorPendingTransactionsLock);

                    ACQUIRE(entityPendingTransactionsLock);

                    // Get indices of pending non-computor transactions that are scheduled to be included in tickData
                    numberOfEntityPendingTransactionIndices = 0;
                    for (unsigned int k = 0; k < SPECTRUM_CAPACITY; k++)
                    {
                        const Transaction* tx = ((Transaction*)&entityPendingTransactions[k * MAX_TRANSACTION_SIZE]);
                        if (tx->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET)
                        {
                            entityPendingTransactionIndices[numberOfEntityPendingTransactionIndices++] = k;
                        }
                    }

                    // Randomly select non-computor tx scheduled for the tick until tick is full or all pending tx are included
                    while (j < NUMBER_OF_TRANSACTIONS_PER_TICK && numberOfEntityPendingTransactionIndices)
                    {
                        const unsigned int index = random(numberOfEntityPendingTransactionIndices);

                        const Transaction* pendingTransaction = ((Transaction*)&entityPendingTransactions[entityPendingTransactionIndices[index] * MAX_TRANSACTION_SIZE]);
                        ASSERT(pendingTransaction->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET);
                        {
                            ASSERT(pendingTransaction->checkValidity());
                            const unsigned int transactionSize = pendingTransaction->totalSize();
                            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                            {
                                ts.tickTransactions.acquireLock();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    ts.tickTransactionOffsets(pendingTransaction->tick, j) = ts.nextTickTransactionOffset;
                                    copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), (void*)pendingTransaction, transactionSize);
                                    broadcastedFutureTickData.tickData.transactionDigests[j] = &entityPendingTransactionDigests[entityPendingTransactionIndices[index] * 32ULL];
                                    j++;
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                                ts.tickTransactions.releaseLock();
                            }
                        }

                        entityPendingTransactionIndices[index] = entityPendingTransactionIndices[--numberOfEntityPendingTransactionIndices];
                    }

                    RELEASE(entityPendingTransactionsLock);

                    {
                        // insert & broadcast vote counter tx
                        makeAndBroadcastTickVotesTransaction(i, broadcastedFutureTickData, j++);
                    }
                    {
                        // insert & broadcast custom mining share
                        if (makeAndBroadcastCustomMiningTransaction(i, broadcastedFutureTickData, j)) // this type of tx is only broadcasted in mining phases
                        {
                            j++;
                        }
                    }

                    for (; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                    {
                        broadcastedFutureTickData.tickData.transactionDigests[j] = m256i::zero();
                    }

                    setMem(broadcastedFutureTickData.tickData.contractFees, sizeof(broadcastedFutureTickData.tickData.contractFees), 0);

                    unsigned char digest[32];
                    KangarooTwelve(&broadcastedFutureTickData.tickData, sizeof(TickData) - SIGNATURE_SIZE, digest, sizeof(digest));
                    broadcastedFutureTickData.tickData.computorIndex ^= BroadcastFutureTickData::type;
                    sign(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, broadcastedFutureTickData.tickData.signature);

                    enqueueResponse(NULL, sizeof(broadcastedFutureTickData), BroadcastFutureTickData::type, 0, &broadcastedFutureTickData);
                }

                system.latestLedTick = system.tick;
            }

            break;
        }
    }

    if (isMainMode())
    {
        // Publish solutions that were sent via BroadcastMessage as MiningSolutionTransaction
        PROFILE_NAMED_SCOPE("processTick(): broadcast solutions as tx (from BroadcastMessage)");
        for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
        {
            int solutionIndexToPublish = -1;

            // Select solution to publish as tx (and mark solutions as obsolete, whose mining seed does not match).
            // Primarily, consider solutions of the computor i that were already selected for tx before.
            unsigned int j;
            for (j = 0; j < system.numberOfSolutions; j++)
            {
                // solutionPublicationTicks[j] > 0 means the solution has already been selected for creating tx
                // but has neither been RECORDED (successfully processed by tx) nor marked OBSOLETE (outdated mining seed)
                if (solutionPublicationTicks[j] > 0
                    && system.solutions[j].computorPublicKey == computorPublicKeys[i])
                {
                    // Only consider this sol if the scheduled tick has passed already (tx not successful)
                    if (solutionPublicationTicks[j] <= (int)system.tick)
                    {
                        if (system.solutions[j].miningSeed == score->currentRandomSeed)
                        {
                            solutionIndexToPublish = j;
                            break;
                        }
                        else
                        {
                            solutionPublicationTicks[j] = SOLUTION_OBSOLETE_FLAG;
                        }
                    }
                }
            }
            // Secondarily, if no solution has been selected above, consider new solutions without previous tx
            if (j == system.numberOfSolutions)
            {
                for (j = 0; j < system.numberOfSolutions; j++)
                {
                    if (!solutionPublicationTicks[j]
                        && system.solutions[j].computorPublicKey == computorPublicKeys[i])
                    {
                        if (system.solutions[j].miningSeed == score->currentRandomSeed)
                        {
                            solutionIndexToPublish = j;
                            break;
                        }
                        else
                        {
                            solutionPublicationTicks[j] = SOLUTION_OBSOLETE_FLAG;
                        }
                    }
                }
            }

            if (solutionIndexToPublish >= 0)
            {
                // Compute tick offset, when to publish solution
                unsigned int publishingTickOffset = MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET;

                // Do not publish, if the solution tx would end up after reset of mining seed, preventing loss of security deposit
                if (getTickInMiningPhaseCycle() + publishingTickOffset >= INTERNAL_COMPUTATIONS_INTERVAL + 3)
                    continue;

                // Prepare, sign, and broadcast MiningSolutionTransaction
                struct
                {
                    Transaction transaction;
                    m256i miningSeed;
                    m256i nonce;
                    unsigned char signature[SIGNATURE_SIZE];
                } payload;
                static_assert(sizeof(payload) == sizeof(Transaction) + 32 + 32 + SIGNATURE_SIZE, "Unexpected struct size!");
                payload.transaction.sourcePublicKey = computorPublicKeys[i];
                payload.transaction.destinationPublicKey = m256i::zero();
                payload.transaction.amount = MiningSolutionTransaction::minAmount();
                solutionPublicationTicks[solutionIndexToPublish] = payload.transaction.tick = system.tick + publishingTickOffset;
                payload.transaction.inputType = MiningSolutionTransaction::transactionType();
                payload.transaction.inputSize = sizeof(payload.miningSeed) + sizeof(payload.nonce);
                payload.miningSeed = system.solutions[solutionIndexToPublish].miningSeed;
                payload.nonce = system.solutions[solutionIndexToPublish].nonce;

                unsigned char digest[32];
                KangarooTwelve(&payload.transaction, sizeof(payload.transaction) + sizeof(payload.miningSeed) + sizeof(payload.nonce), digest, sizeof(digest));
                sign(computorSubseeds[i].m256i_u8, computorPublicKeys[i].m256i_u8, digest, payload.signature);

                enqueueResponse(NULL, sizeof(payload), BROADCAST_TRANSACTION, 0, &payload);
            }
        }
    }

#ifndef NDEBUG
    // Check that continuous updating of spectrum info is consistent with counting from scratch
    SpectrumInfo si;
    updateSpectrumInfo(si);
    if (si.numberOfEntities != spectrumInfo.numberOfEntities || si.totalAmount != spectrumInfo.totalAmount)
    {
        addDebugMessage(L"BUG DETECTED: Spectrum info of continuous updating is inconsistent with counting from scratch!");
    }
#endif

    // Update entity category populations and dust thresholds each 8 ticks
    if ((system.tick & 7) == 0)
        updateAndAnalzeEntityCategoryPopulations();
    logger.updateTick(system.tick);
}

OPTIMIZE_ON()

static void resetCustomMining()
{
    gCustomMiningSharesCounter.init();
    setMem(gCustomMiningSharesCount, sizeof(gCustomMiningSharesCount), 0);

    for (int i = 0; i < NUMBER_OF_TASK_PARTITIONS; i++)
    {
        gSystemCustomMiningSolutionCache[i].reset();
    }

    gSystemCustomMiningSolutionV2Cache.reset();
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        // Initialize the broadcast transaction buffer. Assume the all previous is broadcasted.
        gCustomMiningBroadcastTxBuffer[i].isBroadcasted = true;
    }
    gCustomMiningStorage.reset();

    // Clear all data of epoch
    gCustomMiningStats.epochReset();
}

static void beginEpoch()
{
    // This version doesn't support migration from contract IPO to contract operation!

    numberOfOwnComputorIndices = 0;

    broadcastedComputors.computors.epoch = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        broadcastedComputors.computors.publicKeys[i].setRandomValue();
    }
    setMem(&broadcastedComputors.computors.signature, sizeof(broadcastedComputors.computors.signature), 0);

#ifndef NDEBUG
    ts.checkStateConsistencyWithAssert();
#endif
    ts.beginEpoch(system.initialTick);
    voteCounter.init();
#ifndef NDEBUG
    ts.checkStateConsistencyWithAssert();
#endif
#if ADDON_TX_STATUS_REQUEST
    beginEpochTxStatusRequestAddOn(system.initialTick);
#endif

    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR; i++)
    {
        ((Transaction*)&computorPendingTransactions[i * MAX_TRANSACTION_SIZE])->tick = 0;
    }
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        ((Transaction*)&entityPendingTransactions[i * MAX_TRANSACTION_SIZE])->tick = 0;
    }

    setMem(solutionPublicationTicks, sizeof(solutionPublicationTicks), 0);
    setMem(faultyComputorFlags, sizeof(faultyComputorFlags), 0);

    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

    score->initMemory();
    score->resetTaskQueue();
    setMem(minerSolutionFlags, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, 0);
    setMem((void*)minerPublicKeys, sizeof(minerPublicKeys), 0);
    setMem((void*)minerScores, sizeof(minerScores), 0);
    numberOfMiners = NUMBER_OF_COMPUTORS;
    setMem(competitorPublicKeys, sizeof(competitorPublicKeys), 0);
    setMem(competitorScores, sizeof(competitorScores), 0);
    setMem(competitorComputorStatuses, sizeof(competitorComputorStatuses), 0);
    minimumComputorScore = 0;
    minimumCandidateScore = 0;

    if (system.epoch < MAX_NUMBER_EPOCH && (solutionThreshold[system.epoch] <= 0 || solutionThreshold[system.epoch] > NUMBER_OF_OUTPUT_NEURONS)) { // invalid threshold
        solutionThreshold[system.epoch] = SOLUTION_THRESHOLD_DEFAULT;
    }

    system.latestOperatorNonce = 0;
    system.numberOfSolutions = 0;
    setMem(system.solutions, sizeof(system.solutions), 0);
    setMem(system.futureComputors, sizeof(system.futureComputors), 0);

    resetCustomMining();

    // Reset resource testing digest at beginning of the epoch
    // there are many global variables that were init at declaration, may need to re-check all of them again
    resourceTestingDigest = 0;

    numberOfTransactions = 0;
#if TICK_STORAGE_AUTOSAVE_MODE
    ts.initMetaData(system.epoch); // for save/load state
#endif

    logger.reset(system.initialTick);

}


// called by tickProcessor() after system.tick has been incremented
static void endEpoch()
{
    logger.registerNewTx(system.tick, logger.SC_END_EPOCH_TX);
    contractProcessorPhase = END_EPOCH;
    contractProcessorState = 1;
    WAIT_WHILE(contractProcessorState);

    // treating endEpoch as a tick, start updating etalonTick:
    // this is the last tick of an epoch, should we set prevResourceTestingDigest to zero? nodes that start from scratch (for the new epoch)
    // would be unable to compute this value(!?)
    etalonTick.prevResourceTestingDigest = resourceTestingDigest;
    etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
    getUniverseDigest(etalonTick.prevUniverseDigest);
    getComputerDigest(etalonTick.prevComputerDigest);
    etalonTick.prevTransactionBodyDigest = etalonTick.saltedTransactionBodyDigest;

    // Handle IPO
    finishIPOs();

    system.initialMillisecond = etalonTick.millisecond;
    system.initialSecond = etalonTick.second;
    system.initialMinute = etalonTick.minute;
    system.initialHour = etalonTick.hour;
    system.initialDay = etalonTick.day;
    system.initialMonth = etalonTick.month;
    system.initialYear = etalonTick.year;


    // Only issue qus if the max supply is not yet reached
    if (spectrumInfo.totalAmount + ISSUANCE_RATE <= MAX_SUPPLY)
    {
        // Compute revenue scores of computors
        unsigned long long revenueScore[NUMBER_OF_COMPUTORS];
        setMem(revenueScore, sizeof(revenueScore), 0);
        for (unsigned int tick = system.initialTick; tick < system.tick; tick++)
        {
            ts.tickData.acquireLock();
            TickData& td = ts.tickData.getByTickInCurrentEpoch(tick);
            if (td.epoch == system.epoch)
            {
                unsigned int numberOfTransactions = 0;
                for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
                {
                    if (!isZero(td.transactionDigests[transactionIndex]))
                    {
                        numberOfTransactions++;
                    }
                }
                revenueScore[tick % NUMBER_OF_COMPUTORS] += gTxRevenuePoints[numberOfTransactions];
            }
            ts.tickData.releaseLock();
        }

        // Save data of custom mining.
        {
            for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                gRevenueComponents.voteScore[i] = voteCounter.getVoteCount(i);
                gRevenueComponents.txScore[i] = revenueScore[i];
                gRevenueComponents.customMiningScore[i] = gCustomMiningSharesCounter.getSharesCount(i);
            }
            computeRevenue(
                gRevenueComponents.txScore,
                gRevenueComponents.voteScore,
                gRevenueComponents.customMiningScore,
                gRevenueComponents.revenue);
        }

        // Get revenue donation data by calling contract GQMPROP::GetRevenueDonation()
        QpiContextUserFunctionCall qpiContext(GQMPROP::__contract_index);
        qpiContext.call(5, "", 0);
        ASSERT(qpiContext.outputSize == sizeof(GQMPROP::RevenueDonationT));
        const GQMPROP::RevenueDonationT* emissionDist = (GQMPROP::RevenueDonationT*)qpiContext.outputBuffer;

        // Compute revenue of computors and arbitrator
        long long arbitratorRevenue = ISSUANCE_RATE;
        constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;
        for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
        {
            // Compute initial computor revenue, reducing arbitrator revenue
            long long revenue = gRevenueComponents.revenue[computorIndex];
            arbitratorRevenue -= revenue;

            // Reduce computor revenue based on revenue donation table agreed on by quorum
            for (unsigned long long i = 0; i < emissionDist->capacity(); ++i)
            {
                const GQMPROP::RevenueDonationEntry& rdEntry = emissionDist->get(i);
                if (isZero(rdEntry.destinationPublicKey))
                {
                    // There are no gaps in the table, so first empty entry means we are done
                    break;
                }
                if (rdEntry.millionthAmount > 0 && rdEntry.millionthAmount <= 1000000 && system.epoch >= rdEntry.firstEpoch)
                {
                    // Compute donation and update revenue
                    const long long donation = revenue * rdEntry.millionthAmount / 1000000;
                    revenue -= donation;

                    // Generate revenue donation
                    increaseEnergy(rdEntry.destinationPublicKey, donation);
                    const QuTransfer quTransfer = { m256i::zero(), rdEntry.destinationPublicKey, donation };
                    logger.logQuTransfer(quTransfer);
                    if (revenue)
                    {
                        notifyContractOfIncomingTransfer(m256i::zero(), rdEntry.destinationPublicKey, donation, QPI::TransferType::revenueDonation);
                    }
                }
            }

            // Generate computor revenue
            increaseEnergy(broadcastedComputors.computors.publicKeys[computorIndex], revenue);
            const QuTransfer quTransfer = { m256i::zero(), broadcastedComputors.computors.publicKeys[computorIndex], revenue };
            logger.logQuTransfer(quTransfer);
        }
        emissionDist = nullptr; qpiContext.freeBuffer(); // Free buffer holding revenue donation table, because we don't need it anymore

        // Generate arbitrator revenue
        increaseEnergy(arbitratorPublicKey, arbitratorRevenue);
        const QuTransfer quTransfer = { m256i::zero(), arbitratorPublicKey, arbitratorRevenue };
        logger.logQuTransfer(quTransfer);
    }

    // Reorganize spectrum hash map (also updates spectrumInfo)
    {
        ACQUIRE(spectrumLock);

        reorganizeSpectrum();

        RELEASE(spectrumLock);
    }

    assetsEndEpoch();

    logger.updateTick(system.tick);
#if PAUSE_BEFORE_CLEAR_MEMORY
    // re-open request processors for other services to query
    epochTransitionState = 0;
    WAIT_WHILE(epochTransitionWaitingRequestProcessors != 0);

    epochTransitionCleanMemoryFlag = 0;
    WAIT_WHILE(epochTransitionCleanMemoryFlag == 0); // wait until operator flip this flag to 1 to continue the beginEpoch procedures

    // close all request processors
    epochTransitionState = 1;
    WAIT_WHILE(epochTransitionWaitingRequestProcessors < nRequestProcessorIDs);
#endif

    system.epoch++;
    system.initialTick = system.tick;

    mainAuxStatus = ((mainAuxStatus & 1) << 1) | ((mainAuxStatus & 2) >> 1);
}


#if !START_NETWORK_FROM_SCRATCH

static bool haveSamePrevDigestsAndTime(const Tick& A, const Tick& B)
{
    return A.prevComputerDigest == B.prevComputerDigest &&
        A.prevResourceTestingDigest == B.prevResourceTestingDigest &&
        A.prevTransactionBodyDigest == B.prevTransactionBodyDigest &&
        A.prevSpectrumDigest == B.prevSpectrumDigest &&
        A.prevUniverseDigest == B.prevUniverseDigest &&
        *((unsigned long long*) & A.millisecond) == *((unsigned long long*) & B.millisecond);
}

// Try to pull quorum tick and update the etalonTick to correct timeStamp and digests.
// A corner case: this function can't get initial time+digests if more than 451 ID failed to switch the epoch.
// In this case, START_NETWORK_FROM_SCRATCH must be 1 to boot the network from scratch again, ie: first tick timestamp: 2022-04-13 12:00:00 UTC.
// On the first tick of an epoch after seamless transition, it contains the prevDigests of the last tick of last epoch, which can't be obtained
// by a node that starts from scratch.
static void initializeFirstTick()
{
    unsigned int uniqueVoteIndex[NUMBER_OF_COMPUTORS];
    int uniqueVoteCount[NUMBER_OF_COMPUTORS];
    int uniqueCount = 0;
    const unsigned int firstTickIndex = ts.tickToIndexCurrentEpoch(system.initialTick);
    while (!shutDownNode)
    {
        if (broadcastedComputors.computors.epoch == system.epoch)
        {
            // group ticks with same digest+timestamp and count votes (how many are in each group)
            setMem(uniqueVoteIndex, sizeof(uniqueVoteIndex), 0);
            setMem(uniqueVoteCount, sizeof(uniqueVoteCount), 0);
            uniqueCount = 0;

            const Tick* tsCompTicks = ts.ticks.getByTickIndex(firstTickIndex);
            for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                ts.ticks.acquireLock(i);
                const Tick* tick = &tsCompTicks[i];
                if (tick->epoch == system.epoch)
                {
                    // compare tick with all ticks with unique digest+timestamp that we have found before
                    int unique_idx = -1;
                    for (int j = 0; j < uniqueCount; j++) {
                        ts.ticks.acquireLock(uniqueVoteIndex[j]);
                        const Tick* unique = &tsCompTicks[uniqueVoteIndex[j]];
                        if (haveSamePrevDigestsAndTime(*unique , *tick))
                        {
                            unique_idx = j;
                            ts.ticks.releaseLock(uniqueVoteIndex[j]);
                            break;
                        }
                        ts.ticks.releaseLock(uniqueVoteIndex[j]);
                    }

                    if (unique_idx == -1)
                    {
                        // tick is not in array of unique votes -> add and init counter to 1
                        uniqueVoteIndex[uniqueCount] = i;
                        uniqueVoteCount[uniqueCount] = 1;
                        uniqueCount++;
                    }
                    else
                    {
                        // tick already is in array of unique ticks -> increment counter
                        uniqueVoteCount[unique_idx]++;
                    }
                }

                ts.ticks.releaseLock(i);
            }

            // if we have groups...
            if (uniqueCount > 0)
            {
                // find group with most votes
                int maxUniqueVoteCountIndex = 0;
                for (int i = 1; i < uniqueCount; i++)
                {
                    if (uniqueVoteCount[i] > uniqueVoteCount[maxUniqueVoteCountIndex])
                    {
                        maxUniqueVoteCountIndex = i;
                    }
                }
                
                int numberOfVote = uniqueVoteCount[maxUniqueVoteCountIndex];

                // accept the tick with most votes if it has more than 1/3 of quorum
                if (numberOfVote >= NUMBER_OF_COMPUTORS - QUORUM)
                {
                    ts.ticks.acquireLock(uniqueVoteIndex[maxUniqueVoteCountIndex]);
                    const Tick* unique = &tsCompTicks[uniqueVoteIndex[maxUniqueVoteCountIndex]];
                    ts.ticks.releaseLock(uniqueVoteIndex[maxUniqueVoteCountIndex]);
                    *((unsigned long long*) & etalonTick.millisecond) = *((unsigned long long*) & unique->millisecond);
                    etalonTick.prevComputerDigest = unique->prevComputerDigest;
                    etalonTick.prevResourceTestingDigest = unique->prevResourceTestingDigest;
                    etalonTick.prevSpectrumDigest = unique->prevSpectrumDigest;
                    etalonTick.prevUniverseDigest = unique->prevUniverseDigest;
                    etalonTick.prevTransactionBodyDigest = unique->prevTransactionBodyDigest;
                    return;
                }
            }
        }
        _mm_pause();
    }
}
#endif

#if TICK_STORAGE_AUTOSAVE_MODE

// Invalid snapshot data
static bool invalidateNodeStates(CHAR16* directory)
{
    return ts.saveInvalidateData(system.epoch, directory);
}

// can only called from main thread
static bool saveAllNodeStates()
{
    PROFILE_SCOPE();

    CHAR16 directory[16];
    setText(directory, L"ep");
    appendNumber(directory, system.epoch, false);

    logToConsole(L"Start saving node states from main thread");

    // Mark current snapshot metadata as invalid at the beginning.
    // Any reasons make the valid metadata can not be overwritten at the final step will keep this invalid file
    // and make the loadAllNodeStates see this saving as an invalid save.
    if (!invalidateNodeStates(directory))
    {
        logToConsole(L"Failed to init snapshot metadata");
        return false;
    }

    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 4] = L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 3] = L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 2] = L'0';
    setText(message, L"Saving spectrum to ");
    appendText(message, directory); appendText(message, L"/");
    appendText(message, SPECTRUM_FILE_NAME);
    logToConsole(message);
    if (!saveSpectrum(SPECTRUM_FILE_NAME, directory))
    {
        logToConsole(L"Failed to save spectrum");
        return false;
    }

    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 4] = L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 3] = L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 2] = L'0';
    setText(message, L"Saving universe to ");
    appendText(message, directory); appendText(message, L"/");
    appendText(message, UNIVERSE_FILE_NAME);
    logToConsole(message);
    if (!saveUniverse(UNIVERSE_FILE_NAME, directory))
    {
        logToConsole(L"Failed to save universe");
        return false;
    }

    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 4] = L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 3] = L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 2] = L'0';
    setText(message, L"Saving computer files");
    logToConsole(message);
    if (!saveComputer(directory))
    {
        logToConsole(L"Failed to save computer");
        return false;
    }
    setText(message, L"Saving system to system.snp");
    logToConsole(message);

    static unsigned short SYSTEM_SNAPSHOT_FILE_NAME[] = L"system.snp";
    long long savedSize = save(SYSTEM_SNAPSHOT_FILE_NAME, sizeof(system), (unsigned char*)&system, directory);
    if (savedSize != sizeof(system))
    {
        logToConsole(L"Failed to save system");
        return false;
    }
    
    score->saveScoreCache(system.epoch, directory);
    
    copyMem(&nodeStateBuffer.etalonTick, &etalonTick, sizeof(etalonTick));
    copyMem(nodeStateBuffer.minerPublicKeys, (void*)minerPublicKeys, sizeof(minerPublicKeys));
    copyMem(nodeStateBuffer.minerScores, (void*)minerScores, sizeof(minerScores));
    copyMem(nodeStateBuffer.competitorPublicKeys, (void*)competitorPublicKeys, sizeof(competitorPublicKeys));
    copyMem(nodeStateBuffer.competitorScores, (void*)competitorScores, sizeof(competitorScores));
    copyMem(nodeStateBuffer.competitorComputorStatuses, (void*)competitorComputorStatuses, sizeof(competitorComputorStatuses));
    copyMem(nodeStateBuffer.solutionPublicationTicks, (void*)solutionPublicationTicks, sizeof(solutionPublicationTicks));
    copyMem(nodeStateBuffer.faultyComputorFlags, (void*)faultyComputorFlags, sizeof(faultyComputorFlags));
    copyMem(&nodeStateBuffer.broadcastedComputors, (void*)&broadcastedComputors, sizeof(broadcastedComputors));
    copyMem(&nodeStateBuffer.resourceTestingDigest, &resourceTestingDigest, sizeof(resourceTestingDigest));
    nodeStateBuffer.currentRandomSeed = score->currentRandomSeed;
    nodeStateBuffer.numberOfMiners = numberOfMiners;
    nodeStateBuffer.numberOfTransactions = numberOfTransactions;    
    voteCounter.saveAllDataToArray(nodeStateBuffer.voteCounterData);
    gCustomMiningSharesCounter.saveAllDataToArray(nodeStateBuffer.customMiningSharesCounterData);

    CHAR16 NODE_STATE_FILE_NAME[] = L"snapshotNodeMiningState";
    savedSize = save(NODE_STATE_FILE_NAME, sizeof(nodeStateBuffer), (unsigned char*)&nodeStateBuffer, directory);
    logToConsole(L"Saving mining states");
    if (savedSize != sizeof(nodeStateBuffer))
    {
        logToConsole(L"Failed to save etalon tick and other states");
        return false;
    }
    
    CHAR16 SPECTRUM_DIGEST_FILE_NAME[] = L"snapshotSpectrumDigest";
    savedSize = save(SPECTRUM_DIGEST_FILE_NAME, spectrumDigestsSizeInByte, (unsigned char*)spectrumDigests, directory);
    logToConsole(L"Saving spectrum digests");
    if (savedSize != spectrumDigestsSizeInByte)
    {
        logToConsole(L"Failed to save spectrum digest");
        return false;
    }

    CHAR16 UNIVERSE_DIGEST_FILE_NAME[] = L"snapshotUniverseDigest";
    savedSize = save(UNIVERSE_DIGEST_FILE_NAME, assetDigestsSizeInBytes, (unsigned char*)assetDigests, directory);
    logToConsole(L"Saving universe digests");
    if (savedSize != assetDigestsSizeInBytes)
    {
        logToConsole(L"Failed to save universe digest");
        return false;
    }

    CHAR16 COMPUTER_DIGEST_FILE_NAME[] = L"snapshotComputerDigest";
    savedSize = save(COMPUTER_DIGEST_FILE_NAME, contractStateDigestsSizeInBytes, (unsigned char*)contractStateDigests, directory);
    logToConsole(L"Saving computer digests");
    if (savedSize != contractStateDigestsSizeInBytes)
    {
        logToConsole(L"Failed to save computer digest");
        return false;
    }

    CHAR16 MINER_SOL_FLAG_FILE_NAME[] = L"snapshotMinerSolutionFlag";
    logToConsole(L"Saving miner solution flags");
    savedSize = save(MINER_SOL_FLAG_FILE_NAME, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, (unsigned char*)minerSolutionFlags, directory);
    if (savedSize != NUMBER_OF_MINER_SOLUTION_FLAGS / 8)
    {
        logToConsole(L"Failed to save miner solution flag");
        return false;
    }

    setText(message, L"Saving tick storage ");
    logToConsole(message);
    if (ts.trySaveToFile(system.epoch, system.tick, directory) != 0)
    {
        logToConsole(L"Failed to save tick storage");
        return false;
    }

#if ADDON_TX_STATUS_REQUEST
    if (!saveStateTxStatus(numberOfTransactions, directory))
    {
        logToConsole(L"Failed to save tx status");
        return false;
    }
#endif
#if ENABLED_LOGGING
    logger.saveCurrentLoggingStates(directory);
#endif
    return true;
}

static bool loadAllNodeStates()
{
    CHAR16 directory[16];
    setText(directory, L"ep");
    appendNumber(directory, system.epoch, false);

    // No directory of all saved states. Start from scratch
    if (!checkDir(directory))
    {
        logToConsole(L"Not find epoch snapshot directory. Skip using node states snapshot.");
        return false;
    }
    else
    {
        logToConsole(L"Found epoch snapshot directory. Using node states snapshot.");
    }

    if (ts.tryLoadFromFile(system.epoch, directory) != 0)
    {
        logToConsole(L"Failed to load tick storage");
        return false;
    }
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 4] = L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 3] = L'0';
    SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 2] = L'0';
    if (!loadSpectrum(SPECTRUM_FILE_NAME, directory))
    {
        logToConsole(L"Failed to load spectrum");
        return false;
    }

    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 4] = L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 3] = L'0';
    UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 2] = L'0';
    if (!loadUniverse(UNIVERSE_FILE_NAME, directory))
    {
        logToConsole(L"Failed to load universe");
        return false;
    }

    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 4] = L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 3] = L'0';
    CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 2] = L'0';
    const bool forceLoadContractFile = true;
    if (!loadComputer(directory, forceLoadContractFile))
    {
        logToConsole(L"Failed to load computer");
        return false;
    }

    CHAR16 NODE_STATE_FILE_NAME[] = L"snapshotNodeMiningState";
    long long loadedSize = load(NODE_STATE_FILE_NAME, sizeof(nodeStateBuffer), (unsigned char*)&nodeStateBuffer, directory);
    if (loadedSize != sizeof(nodeStateBuffer))
    {
        logToConsole(L"Failed to load mining state");
        return false;
    }
    copyMem(&etalonTick, &nodeStateBuffer.etalonTick, sizeof(etalonTick));
    copyMem((void*)minerPublicKeys, nodeStateBuffer.minerPublicKeys, sizeof(minerPublicKeys));
    copyMem((void*)minerScores, nodeStateBuffer.minerScores, sizeof(minerScores));
    copyMem((void*)competitorPublicKeys, nodeStateBuffer.competitorPublicKeys, sizeof(competitorPublicKeys));
    copyMem((void*)competitorScores, nodeStateBuffer.competitorScores, sizeof(competitorScores));
    copyMem((void*)competitorComputorStatuses, nodeStateBuffer.competitorComputorStatuses, sizeof(competitorComputorStatuses));
    copyMem((void*)solutionPublicationTicks, nodeStateBuffer.solutionPublicationTicks, sizeof(solutionPublicationTicks));
    copyMem((void*)faultyComputorFlags, nodeStateBuffer.faultyComputorFlags, sizeof(faultyComputorFlags));
    copyMem((void*)&broadcastedComputors, &nodeStateBuffer.broadcastedComputors, sizeof(broadcastedComputors));
    copyMem(&resourceTestingDigest, &nodeStateBuffer.resourceTestingDigest, sizeof(resourceTestingDigest));
    numberOfMiners = nodeStateBuffer.numberOfMiners;
    initialRandomSeedFromPersistingState = nodeStateBuffer.currentRandomSeed;
    numberOfTransactions = nodeStateBuffer.numberOfTransactions;
    loadMiningSeedFromFile = true;
    voteCounter.loadAllDataFromArray(nodeStateBuffer.voteCounterData);
    gCustomMiningSharesCounter.loadAllDataFromArray(nodeStateBuffer.customMiningSharesCounterData);

    // update own computor indices
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        for (unsigned int j = 0; j < sizeof(computorSeeds) / sizeof(computorSeeds[0]); j++)
        {
            if (broadcastedComputors.computors.publicKeys[i] == computorPublicKeys[j])
            {
                ownComputorIndices[numberOfOwnComputorIndices] = i;
                ownComputorIndicesMapping[numberOfOwnComputorIndices++] = j;

                break;
            }
        }
    }

    static unsigned short SYSTEM_SNAPSHOT_FILE_NAME[] = L"system.snp";
    loadedSize = load(SYSTEM_SNAPSHOT_FILE_NAME, sizeof(system), (unsigned char*)&system, directory);
    if (loadedSize != sizeof(system))
    {
        logToConsole(L"Failed to load system");
        return false;
    }
    updateNumberOfTickTransactions();

    setMem(assetChangeFlags, sizeof(assetChangeFlags), 0);
    setMem(spectrumChangeFlags, sizeof(spectrumChangeFlags), 0);
    CHAR16 SPECTRUM_DIGEST_FILE_NAME[] = L"snapshotSpectrumDigest";
    loadedSize = load(SPECTRUM_DIGEST_FILE_NAME, spectrumDigestsSizeInByte, (unsigned char*)spectrumDigests, directory);
    logToConsole(L"Loading spectrum digests");
    if (loadedSize != spectrumDigestsSizeInByte)
    {
        logToConsole(L"Failed to load spectrum digest");
        return false;
    }

    CHAR16 UNIVERSE_DIGEST_FILE_NAME[] = L"snapshotUniverseDigest";
    loadedSize = load(UNIVERSE_DIGEST_FILE_NAME, assetDigestsSizeInBytes, (unsigned char*)assetDigests, directory);
    logToConsole(L"Loading universe digests");
    if (loadedSize != assetDigestsSizeInBytes)
    {
        logToConsole(L"Failed to load universe digest");
        return false;
    }

    CHAR16 COMPUTER_DIGEST_FILE_NAME[] = L"snapshotComputerDigest";
    loadedSize = load(COMPUTER_DIGEST_FILE_NAME, contractStateDigestsSizeInBytes, (unsigned char*)contractStateDigests, directory);
    logToConsole(L"Loading computer digests");
    if (loadedSize != contractStateDigestsSizeInBytes)
    {
        logToConsole(L"Failed to load computer digest");
        return false;
    }

    CHAR16 MINER_SOL_FLAG_FILE_NAME[] = L"snapshotMinerSolutionFlag";
    logToConsole(L"Loading miner solution flags");
    loadedSize = load(MINER_SOL_FLAG_FILE_NAME, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, (unsigned char*)minerSolutionFlags, directory);
    if (loadedSize != NUMBER_OF_MINER_SOLUTION_FLAGS / 8)
    {
        logToConsole(L"Failed to load miner solution flag");
        return false;
    }

#if ADDON_TX_STATUS_REQUEST
    if (!loadStateTxStatus(numberOfTransactions, directory))
    {
        logToConsole(L"Failed to load tx status");
        return false;
    }
#endif

#if ENABLED_LOGGING
    logToConsole(L"Loading old logger...");
    logger.loadLastLoggingStates(directory);
#endif
    return true;
}

#endif

// Count the number of future tick vote (system.tick + 1) and then update it to gFutureTickTotalNumberOfComputors
static void updateFutureTickCount()
{
    const unsigned int nextTick = system.tick + 1;
    const unsigned int nextTickIndex = ts.tickToIndexCurrentEpoch(nextTick);
    const Tick* tsCompTicks = ts.ticks.getByTickIndex(nextTickIndex);
    unsigned int futureTickTotalNumberOfComputors = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (tsCompTicks[i].epoch == system.epoch)
        {
            futureTickTotalNumberOfComputors++;
        }
    }
    gFutureTickTotalNumberOfComputors = futureTickTotalNumberOfComputors;
}

// find next tick data digest from next tick votes
// Scan all tick votes of the next tick (system.tick + 1):
// if there are 451+ (QUORUM) votes agree on the same transactionDigest - or 226+ (VETO) votes agree on empty tick
// then next tick digest is known (from the point of view of the node) - targetNextTickDataDigest
static void findNextTickDataDigestFromNextTickVotes()
{
    const unsigned int nextTick = system.tick + 1;
    const unsigned int nextTickIndex = ts.tickToIndexCurrentEpoch(nextTick);
    const Tick* tsCompTicks = ts.ticks.getByTickIndex(nextTickIndex);
    unsigned int numberOfEmptyNextTickTransactionDigest = 0;
    unsigned int numberOfUniqueNextTickTransactionDigests = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (tsCompTicks[i].epoch == system.epoch)
        {
            unsigned int j;
            for (j = 0; j < numberOfUniqueNextTickTransactionDigests; j++)
            {
                if (tsCompTicks[i].transactionDigest == uniqueNextTickTransactionDigests[j])
                {
                    break;
                }
            }
            if (j == numberOfUniqueNextTickTransactionDigests)
            {
                uniqueNextTickTransactionDigests[numberOfUniqueNextTickTransactionDigests] = tsCompTicks[i].transactionDigest;
                uniqueNextTickTransactionDigestCounters[numberOfUniqueNextTickTransactionDigests++] = 1;
            }
            else
            {
                uniqueNextTickTransactionDigestCounters[j]++;
            }

            if (isZero(tsCompTicks[i].transactionDigest))
            {
                numberOfEmptyNextTickTransactionDigest++;
            }
        }
    }
    unsigned int mostPopularUniqueNextTickTransactionDigestIndex = 0, totalUniqueNextTickTransactionDigestCounter = uniqueNextTickTransactionDigestCounters[0];
    for (unsigned int i = 1; i < numberOfUniqueNextTickTransactionDigests; i++)
    {
        if (uniqueNextTickTransactionDigestCounters[i] > uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex])
        {
            mostPopularUniqueNextTickTransactionDigestIndex = i;
        }
        totalUniqueNextTickTransactionDigestCounter += uniqueNextTickTransactionDigestCounters[i];
    }
    if (uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex] >= QUORUM)
    {
        targetNextTickDataDigest = uniqueNextTickTransactionDigests[mostPopularUniqueNextTickTransactionDigestIndex];
        targetNextTickDataDigestIsKnown = true;
    }
    else
    {
        if (numberOfEmptyNextTickTransactionDigest > NUMBER_OF_COMPUTORS - QUORUM
            || uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex] + (NUMBER_OF_COMPUTORS - totalUniqueNextTickTransactionDigestCounter) < QUORUM)
        {
            // Create empty tick
            targetNextTickDataDigest = m256i::zero();
            targetNextTickDataDigestIsKnown = true;
        }
    }
}

// working the same as findNextTickDataDigestFromNextTickVotes
// but it will scan current tick (system.tick) votes, instead of next tick
static void findNextTickDataDigestFromCurrentTickVotes()
{
    const unsigned int currentTickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    const Tick* tsCompTicks = ts.ticks.getByTickIndex(currentTickIndex);
    unsigned int numberOfEmptyNextTickTransactionDigest = 0;
    unsigned int numberOfUniqueNextTickTransactionDigests = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (tsCompTicks[i].epoch == system.epoch)
        {
            unsigned int j;
            for (j = 0; j < numberOfUniqueNextTickTransactionDigests; j++)
            {
                if (tsCompTicks[i].expectedNextTickTransactionDigest == uniqueNextTickTransactionDigests[j])
                {
                    break;
                }
            }
            if (j == numberOfUniqueNextTickTransactionDigests)
            {
                uniqueNextTickTransactionDigests[numberOfUniqueNextTickTransactionDigests] = tsCompTicks[i].expectedNextTickTransactionDigest;
                uniqueNextTickTransactionDigestCounters[numberOfUniqueNextTickTransactionDigests++] = 1;
            }
            else
            {
                uniqueNextTickTransactionDigestCounters[j]++;
            }

            if (isZero(tsCompTicks[i].expectedNextTickTransactionDigest))
            {
                numberOfEmptyNextTickTransactionDigest++;
            }
        }
    }
    if (numberOfUniqueNextTickTransactionDigests)
    {
        unsigned int mostPopularUniqueNextTickTransactionDigestIndex = 0, totalUniqueNextTickTransactionDigestCounter = uniqueNextTickTransactionDigestCounters[0];
        for (unsigned int i = 1; i < numberOfUniqueNextTickTransactionDigests; i++)
        {
            if (uniqueNextTickTransactionDigestCounters[i] > uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex])
            {
                mostPopularUniqueNextTickTransactionDigestIndex = i;
            }
            totalUniqueNextTickTransactionDigestCounter += uniqueNextTickTransactionDigestCounters[i];
        }
        if (uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex] >= QUORUM)
        {
            targetNextTickDataDigest = uniqueNextTickTransactionDigests[mostPopularUniqueNextTickTransactionDigestIndex];
            targetNextTickDataDigestIsKnown = true;
        }
        else
        {
            if (numberOfEmptyNextTickTransactionDigest > NUMBER_OF_COMPUTORS - QUORUM
                || uniqueNextTickTransactionDigestCounters[mostPopularUniqueNextTickTransactionDigestIndex] + (NUMBER_OF_COMPUTORS - totalUniqueNextTickTransactionDigestCounter) < QUORUM)
            {
                targetNextTickDataDigest = m256i::zero();
                targetNextTickDataDigestIsKnown = true;
            }
        }
    }
}

// return number of current tick vote
static unsigned int countCurrentTickVote()
{
    const unsigned int currentTickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    unsigned int tickTotalNumberOfComputors = 0;
    const Tick* tsCompTicks = ts.ticks.getByTickIndex(currentTickIndex);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        ts.ticks.acquireLock(i);

        if (tsCompTicks[i].epoch == system.epoch)
        {
            tickTotalNumberOfComputors++;
        }

        ts.ticks.releaseLock(i);
    }
    return tickTotalNumberOfComputors;
}

// This function scans through all transactions digest in next tickData
// and look for those txs in local memory (pending txs and tickstorage). If a transaction doesn't exist, it will try to update requestedTickTransactions
// The main loop (MAIN thread) will try to fetch missing txs based on the data inside requestedTickTransactions.
// This function also counts numberOfNextTickTransactions and numberOfKnownNextTickTransactions for checking if all tx are in tick tx storage.
// Current code assumes the limits:
// - 1 tx per source publickey per tick
// - 128 txs per computor publickey per tick
// Note requestedTickTransactions.transactionFlags are set to 0 for tx we want to request and 1 for tx we are not interested in
static void prepareNextTickTransactions()
{
    const unsigned int nextTick = system.tick + 1;
    const unsigned int nextTickIndex = ts.tickToIndexCurrentEpoch(nextTick);

    nextTickTransactionsSemaphore = 1; // signal a flag for displaying on the console log


    // unknownTransactions is set to 1 if a transaction is missing in the local storage
    unsigned long long unknownTransactions[NUMBER_OF_TRANSACTIONS_PER_TICK / 64];
    setMem(unknownTransactions, sizeof(unknownTransactions), 0);
    const auto* tsNextTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(nextTickIndex);
    
    // This function maybe called multiple times per tick due to lack of data (txs or votes)
    // Here we do a simple pre scan to check txs via tsNextTickTransactionOffsets (already processed - aka already copying from pendingTransaction array to tickTransaction)
    // Mark all transaction that are not in the tickStorage as missing
    for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
    {
        if (!isZero(nextTickData.transactionDigests[i]))
        {
            numberOfNextTickTransactions++;

            ts.tickTransactions.acquireLock();

            if (tsNextTickTransactionOffsets[i])
            {
                const Transaction* transaction = ts.tickTransactions(tsNextTickTransactionOffsets[i]);
                ASSERT(transaction->checkValidity());
                ASSERT(transaction->tick == nextTick);
                unsigned char digest[32];
                KangarooTwelve(transaction, transaction->totalSize(), digest, sizeof(digest));
                if (digest == nextTickData.transactionDigests[i])
                {
                    numberOfKnownNextTickTransactions++;
                }
                else
                {
                    unknownTransactions[i >> 6] |= (1ULL << (i & 63));
                }
            }
            else
            {
                unknownTransactions[i >> 6] |= (1ULL << (i & 63));
            }
            ts.tickTransactions.releaseLock();
        }
    }

    if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
    {
        // Checks if any of the missing transactions is available in the computorPendingTransaction and remove unknownTransaction flag if found
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR; i++)
        {
            Transaction* pendingTransaction = (Transaction*)&computorPendingTransactions[i * MAX_TRANSACTION_SIZE];
            if (pendingTransaction->tick == nextTick)
            {
                ACQUIRE(computorPendingTransactionsLock);

                ASSERT(pendingTransaction->checkValidity());
                auto* tsPendingTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(pendingTransaction->tick);
                for (unsigned int j = 0; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                {
                    if (unknownTransactions[j >> 6] & (1ULL << (j & 63)))
                    {
                        if (&computorPendingTransactionDigests[i * 32ULL] == nextTickData.transactionDigests[j])
                        {
                            ts.tickTransactions.acquireLock();
                            // write tx to tick tx storage, no matter if tsNextTickTransactionOffsets[i] is 0 (new tx)
                            // or not (tx with digest that doesn't match tickData needs to be overwritten)
                            {
                                const unsigned int transactionSize = pendingTransaction->totalSize();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    tsPendingTransactionOffsets[j] = ts.nextTickTransactionOffset;
                                    copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), pendingTransaction, transactionSize);
                                    ts.nextTickTransactionOffset += transactionSize;

                                    numberOfKnownNextTickTransactions++;
                                }
                            }
                            ts.tickTransactions.releaseLock();

                            unknownTransactions[j >> 6] &= ~(1ULL << (j & 63));

                            break;
                        }
                    }
                }

                RELEASE(computorPendingTransactionsLock);
            }
        }
        // Checks if any of the missing transactions is available in the entityPendingTransaction and remove unknownTransaction flag if found
        for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
        {
            Transaction* pendingTransaction = (Transaction*)&entityPendingTransactions[i * MAX_TRANSACTION_SIZE];
            if (pendingTransaction->tick == nextTick)
            {
                ACQUIRE(entityPendingTransactionsLock);

                ASSERT(pendingTransaction->checkValidity());
                auto* tsPendingTransactionOffsets = ts.tickTransactionOffsets.getByTickInCurrentEpoch(pendingTransaction->tick);
                for (unsigned int j = 0; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                {
                    if (unknownTransactions[j >> 6] & (1ULL << (j & 63)))
                    {
                        if (&entityPendingTransactionDigests[i * 32ULL] == nextTickData.transactionDigests[j])
                        {
                            ts.tickTransactions.acquireLock();
                            // write tx to tick tx storage, no matter if tsNextTickTransactionOffsets[i] is 0 (new tx)
                            // or not (tx with digest that doesn't match tickData needs to be overwritten)
                            {
                                const unsigned int transactionSize = pendingTransaction->totalSize();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    tsPendingTransactionOffsets[j] = ts.nextTickTransactionOffset;
                                    copyMem(ts.tickTransactions(ts.nextTickTransactionOffset), pendingTransaction, transactionSize);
                                    ts.nextTickTransactionOffset += transactionSize;

                                    numberOfKnownNextTickTransactions++;
                                }
                            }
                            ts.tickTransactions.releaseLock();

                            unknownTransactions[j >> 6] &= ~(1ULL << (j & 63));

                            break;
                        }
                    }
                }

                RELEASE(entityPendingTransactionsLock);
            }
        }

        // At this point unknownTransactions is set to 1 for all transactions that are unknown
        // Update requestedTickTransactions the list of txs that not exist in memory so the MAIN loop can try to fetch them from peers
        // We prepare the transactionFlags so that missing transactions are set to 0 (initialized to all 1)
        // As processNextTickTransactions returns tx for which the flag ist set to 0 (tx with flag set to 1 are not returned)

        // We check if the last tickTransactionRequest it already sent
        if(requestedTickTransactions.requestedTickTransactions.tick == 0){
            // Initialize transactionFlags to one so that by default we do not request any transaction
            setMem(requestedTickTransactions.requestedTickTransactions.transactionFlags, sizeof(requestedTickTransactions.requestedTickTransactions.transactionFlags), 0xff);
            for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
            {
                if (unknownTransactions[i >> 6] & (1ULL << (i & 63)))
                {
                    requestedTickTransactions.requestedTickTransactions.transactionFlags[i >> 3] &= ~(1 << (i & 7));
                }
            }
        }
    }
    nextTickTransactionsSemaphore = 0;
}


// Computes the digest of all tx bodies of a certain tick and saves it in etalonTick (4 bytes)
// This function can only be called by tickProcessor
static void computeTxBodyDigestBase(const int tick)
{
    ASSERT(nextTickData.epoch == system.epoch); // nextTickData need to be valid
    constexpr size_t outputLen = 4; // output length in bytes

    XKCP::KangarooTwelve_Initialize(&g_k12_instance, 128, outputLen);

    const unsigned int tickIndex = ts.tickToIndexCurrentEpoch(tick);
    const auto* tsTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);

    for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
    {
        if (!isZero(nextTickData.transactionDigests[i]))
        {
            // TODO: Optimization to check: We have the ts locked for the whole K12_Update.
            //       It might be worth to copy the transaction and release lock before update the K12 state.
            // TODO: Here we check each Tx against the nextTickData again (before we did it in prepareNextTickTransactions
            //       Might be worth to do only do it once.
            ts.tickTransactions.acquireLock();

            if (tsTransactionOffsets[i]) {
                const Transaction* transaction = ts.tickTransactions(tsTransactionOffsets[i]);

                if (transaction->checkValidity() && transaction->tick == tick) {
                    unsigned char digest[32];
                    KangarooTwelve(transaction, transaction->totalSize(), digest, sizeof(digest));
                    if (digest == nextTickData.transactionDigests[i])
                    {
                        int ret = 1;
                        while(ret == 1)
                        {
                            ret = XKCP::KangarooTwelve_Update(&g_k12_instance, reinterpret_cast<const unsigned char *>(transaction), transaction->totalSize());
                            if (ret == 0)
                            {
                                break;
                            }
#if !defined(NDEBUG)
                            else
                            {
                                setText(message, L"txBodyDigest: XKCP failed to create hash of tx");
                                addDebugMessage(message);
                            }
#endif
                        }
                    }
                }
#if !defined(NDEBUG)
                else
                {
                    setText(message, L"txBodyDigest: Transaction verification failed");
                    addDebugMessage(message);
                }
#endif
            }

            ts.tickTransactions.releaseLock();
        }
    }

    int ret = 1;
    while(ret == 1)
    {
        ret = XKCP::KangarooTwelve_Final(&g_k12_instance, reinterpret_cast<unsigned char*>(&etalonTick.saltedTransactionBodyDigest), (const unsigned char *)"", 0);
#if !defined(NDEBUG)
        if(ret == 1)
        {
            setText(message, L"txBodyDigest: XKCP failed to finalize hash");
            addDebugMessage(message);
        }
#endif
    }
}

// special procedure to sign the tick vote
static void signTickVote(const unsigned char* subseed, const unsigned char* publicKey, const unsigned char* messageDigest, unsigned char* signature)
{
    PROFILE_SCOPE();

    signWithRandomK(subseed, publicKey, messageDigest, signature);
    bool isOk = verifyTickVoteSignature(publicKey, messageDigest, signature, false);
    while (!isOk)
    {
        signWithRandomK(subseed, publicKey, messageDigest, signature);
        isOk = verifyTickVoteSignature(publicKey, messageDigest, signature, false);
    }
}

// broadcast all tickVotes from all IDs in this node
static void broadcastTickVotes()
{
    BroadcastTick broadcastTick;
    copyMem(&broadcastTick.tick, &etalonTick, sizeof(Tick));
    for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
    {
        broadcastTick.tick.computorIndex = ownComputorIndices[i] ^ BroadcastTick::type;
        broadcastTick.tick.epoch = system.epoch;
        m256i saltedData[2];
        saltedData[0] = computorPublicKeys[ownComputorIndicesMapping[i]];
        saltedData[1].m256i_u32[0] = resourceTestingDigest;
        KangarooTwelve(saltedData, 32 + sizeof(resourceTestingDigest), &broadcastTick.tick.saltedResourceTestingDigest, sizeof(broadcastTick.tick.saltedResourceTestingDigest));

        saltedData[1] = etalonTick.saltedSpectrumDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedSpectrumDigest);

        saltedData[1] = etalonTick.saltedUniverseDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedUniverseDigest);

        saltedData[1] = etalonTick.saltedComputerDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedComputerDigest);

        saltedData[1] = m256i::zero();
        saltedData[1].m256i_u32[0] = etalonTick.saltedTransactionBodyDigest;
        KangarooTwelve(saltedData, 32 + sizeof(etalonTick.saltedTransactionBodyDigest), &broadcastTick.tick.saltedTransactionBodyDigest, sizeof(broadcastTick.tick.saltedTransactionBodyDigest));

        unsigned char digest[32];
        KangarooTwelve(&broadcastTick.tick, sizeof(Tick) - SIGNATURE_SIZE, digest, sizeof(digest));
        broadcastTick.tick.computorIndex ^= BroadcastTick::type;
        signTickVote(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, broadcastTick.tick.signature);


        enqueueResponse(NULL, sizeof(broadcastTick), BroadcastTick::type, 0, &broadcastTick);
        // NOTE: here we don't copy these votes to memory, instead we wait other nodes echoing these votes back because:
        // - if own votes don't get echoed back, that indicates this node has internet/topo issue, and need to reissue vote (F9)
        // - all votes need to be processed in a single place of code (for further handling)
        // - all votes are treated equally (own votes and their votes)
    }
}

// count the votes of current tick (system.tick) and compare it with etalonTick
// tickNumberOfComputors: total number of votes that have matched digests with this node states
// tickTotalNumberOfComputors: total number of received votes
// NOTE: this doesn't compare expectedNextTickTransactionDigest
static void updateVotesCount(unsigned int& tickNumberOfComputors, unsigned int& tickTotalNumberOfComputors)
{
    const unsigned int currentTickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    const Tick* tsCompTicks = ts.ticks.getByTickIndex(currentTickIndex);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        ts.ticks.acquireLock(i);

        const Tick* tick = &tsCompTicks[i];
        if (tick->epoch == system.epoch)
        {
            tickTotalNumberOfComputors++;

            if (*((unsigned long long*) & tick->millisecond) == *((unsigned long long*) & etalonTick.millisecond)
                && tick->prevSpectrumDigest == etalonTick.prevSpectrumDigest
                && tick->prevUniverseDigest == etalonTick.prevUniverseDigest
                && tick->prevComputerDigest == etalonTick.prevComputerDigest
                && tick->transactionDigest == etalonTick.transactionDigest)
            {
                m256i saltedData[2];
                m256i saltedDigest;
                saltedData[0] = broadcastedComputors.computors.publicKeys[tick->computorIndex];
                saltedData[1].m256i_u32[0] = resourceTestingDigest;
                KangarooTwelve(saltedData, 32 + sizeof(resourceTestingDigest), &saltedDigest, sizeof(resourceTestingDigest));
                if (tick->saltedResourceTestingDigest == saltedDigest.m256i_u32[0])
                {
                    saltedData[1] = etalonTick.saltedSpectrumDigest;
                    KangarooTwelve64To32(saltedData, &saltedDigest);
                    if (tick->saltedSpectrumDigest == saltedDigest)
                    {
                        saltedData[1] = etalonTick.saltedUniverseDigest;
                        KangarooTwelve64To32(saltedData, &saltedDigest);
                        if (tick->saltedUniverseDigest == saltedDigest)
                        {
                            saltedData[1] = etalonTick.saltedComputerDigest;
                            KangarooTwelve64To32(saltedData, &saltedDigest);
                            if (tick->saltedComputerDigest == saltedDigest)
                            {
                                // expectedNextTickTransactionDigest and txBodyDigest is ignored to find consensus of current tick
                                tickNumberOfComputors++;

                                // Vote of a node is only counting if txBodyDigest is matching with the version of the node
                                if (!isZero(etalonTick.expectedNextTickTransactionDigest))
                                {
                                    saltedData[1] = m256i::zero();
                                    saltedData[1].m256i_u32[0] = etalonTick.saltedTransactionBodyDigest;
                                    KangarooTwelve(saltedData, 32 + sizeof(etalonTick.saltedTransactionBodyDigest), &saltedDigest, sizeof(etalonTick.saltedTransactionBodyDigest));
                                    if(tick->saltedTransactionBodyDigest == saltedDigest.m256i_u32[0])
                                    {
                                        // to avoid submitting invalid votes (eg: all zeroes with valid signature)
                                        // only count votes that matched etalonTick
                                        voteCounter.registerNewVote(tick->tick, tick->computorIndex);
                                    }
                                }
                                else // If expectedNextTickTransactionDigest changes to to empty due to time-out,
                                     // we count votes anyway, otherwise we may end up with no or very few votes
                                {
                                    voteCounter.registerNewVote(tick->tick, tick->computorIndex);
                                }
                            }
                        }
                    }
                }
            }
        }
        ts.ticks.releaseLock(i);
    }
}

// try to resend tick votes if local system.tick gets stuck for too long
static void tryResendTickVotes()
{
    if (autoResendTickVotes.lastTick != system.tick)
    {
        autoResendTickVotes.lastTick = system.tick;
        autoResendTickVotes.lastTickMode = isMainMode() ? 1 : 0;
        autoResendTickVotes.lastCheck = __rdtsc();
    }
    else
    {
        unsigned long long elapsed = (__rdtsc() - autoResendTickVotes.lastCheck) * 1000 / frequency; //millisec
        // Resend vote from this node when:
        // - timeout
        // - on the last tick, this node was in MAIN mode
        if (elapsed >= autoResendTickVotes.MAX_WAITING_TIME && (autoResendTickVotes.lastTickMode == 1))
        {
            if (system.latestCreatedTick > 0) system.latestCreatedTick--;
            autoResendTickVotes.lastCheck = __rdtsc();
        }
    }
}

// try forcing next tick to be empty after certain amount of time
static void tryForceEmptyNextTick()
{
    if (!targetNextTickDataDigestIsKnown)
    {
        // auto f5 logic:
        // if these conditions are met:
        // - this node is on MAIN mode
        // - not reach consensus for next tick digest => (!targetNextTickDataDigestIsKnown)
        // - 451+ votes agree on the current tick (prev digests, tick data) | aka: gTickNumberOfComputors >= QUORUM
        // - the network was stuck for a certain time, (10x of target tick duration by default)
        // then:
        // - randomly (8% chance) force next tick to be empty every sec
        // This will force the node to set: 
        // (1) currentTick.expectedNextTickTransactionDigest  = 0
        // (2) nextTick.transactionDigest = 0 - it invalidates ts.tickData, no way to recover
        if ((isMainMode()) && (AUTO_FORCE_NEXT_TICK_THRESHOLD != 0))
        {
            if (emptyTickResolver.tick != system.tick)
            {
                emptyTickResolver.tick = system.tick;
                emptyTickResolver.clock = __rdtsc();
            }
            else
            {
                if (__rdtsc() - emptyTickResolver.clock > frequency * TARGET_TICK_DURATION * AUTO_FORCE_NEXT_TICK_THRESHOLD / 1000)
                {
                    if (__rdtsc() - emptyTickResolver.lastTryClock > frequency)
                    {
                        unsigned int randNumber = random(10000);
                        if (randNumber < PROBABILITY_TO_FORCE_EMPTY_TICK)
                        {

                            forceNextTick = true; // auto-F5
#if !defined(NDEBUG)
                            {
                                CHAR16 dbgMsg[200];
                                setText(dbgMsg, L"Consensus: activated auto-F5 at tick ");
                                appendNumber(dbgMsg, system.tick, true);
                                addDebugMessage(dbgMsg);
                            }                            
#endif
                        }
                        emptyTickResolver.lastTryClock = __rdtsc();
                    }
                }
            }
        }

        if (forceNextTick)
        {
            targetNextTickDataDigest = m256i::zero();
            targetNextTickDataDigestIsKnown = true;
        }
    }
    forceNextTick = false;
}

static bool isTickTimeOut()
{
    return (__rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] > TARGET_TICK_DURATION * NEXT_TICK_TIMEOUT_THRESHOLD * frequency / 1000);
}

// Disabling the optimizer for tickProcessor() is a workaround introduced to solve an issue
// that has been observed in testnets/2025-04-30-profiling.
// In this test, the processor calling tickProcessor() was stuck before entering the function.
// Probably, this was caused by a bug in the optimizer, because disabling the optimizer solved the
// problem.
OPTIMIZE_OFF()
static void tickProcessor(void*)
{
    enableAVX();
    const unsigned long long processorNumber = getRunningProcessorID();

#if !START_NETWORK_FROM_SCRATCH
    // only init first tick if it doesn't load all node states from file
    if (!loadAllNodeStateFromFile)
    {
        initializeFirstTick();
    }
#endif

    loadAllNodeStateFromFile = false;
    unsigned int latestProcessedTick = 0;
    while (!shutDownNode)
    {
        PROFILE_NAMED_SCOPE("tickProcessor(): loop iteration");

        checkinTime(processorNumber);

        const unsigned long long curTimeTick = __rdtsc();
        const unsigned int nextTick = system.tick + 1;

        if (broadcastedComputors.computors.epoch == system.epoch
            && ts.tickInCurrentEpochStorage(nextTick))
        {
            const unsigned int currentTickIndex = ts.tickToIndexCurrentEpoch(system.tick);
            const unsigned int nextTickIndex = ts.tickToIndexCurrentEpoch(nextTick);

            updateFutureTickCount();

            if (system.tick > latestProcessedTick)
            {
                // State persist: if it can reach to this point that means we already have all necessary data to process tick `system.tick`
                // thus, pausing here and doing the state persisting is the best choice.
                if (requestPersistingNodeState)
                {
                    persistingNodeStateTickProcWaiting = 1;
                    WAIT_WHILE(requestPersistingNodeState);
                    persistingNodeStateTickProcWaiting = 0;
                }
                processTick(processorNumber);
                latestProcessedTick = system.tick;
            }

            if (gFutureTickTotalNumberOfComputors > NUMBER_OF_COMPUTORS - QUORUM)
            {
                findNextTickDataDigestFromNextTickVotes();
            }

            if (!targetNextTickDataDigestIsKnown)
            {
                findNextTickDataDigestFromCurrentTickVotes();
            }

            ts.tickData.acquireLock();
            copyMem(&nextTickData, &ts.tickData[nextTickIndex], sizeof(TickData));
            ts.tickData.releaseLock();

            // This time lock ensures tickData is crafted 2 ticks "ago"
            if (nextTickData.epoch == system.epoch)
            {
                m256i timelockPreimage[3];
                timelockPreimage[0] = etalonTick.prevSpectrumDigest;
                timelockPreimage[1] = etalonTick.prevUniverseDigest;
                timelockPreimage[2] = etalonTick.prevComputerDigest;
                m256i timelock;
                KangarooTwelve(timelockPreimage, sizeof(timelockPreimage), &timelock, sizeof(timelock));
                if (nextTickData.timelock != timelock)
                {
                    ts.tickData.acquireLock();
                    ts.tickData[nextTickIndex].epoch = 0;
                    ts.tickData.releaseLock();
                    nextTickData.epoch = 0;
                }
            }

            bool tickDataSuits; // a flag to tell if tickData is suitable to be included with this node states
            if (!targetNextTickDataDigestIsKnown) // Next tick digest is still unknown
            {
                if (nextTickData.epoch != system.epoch // tick data is valid (not yet invalidated)
                    && gFutureTickTotalNumberOfComputors <= NUMBER_OF_COMPUTORS - QUORUM // future tick vote less than 225
                    && __rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] < TARGET_TICK_DURATION * frequency / 1000) // tick duration not exceed TARGET_TICK_DURATION
                {
                    tickDataSuits = false;
                }
                else
                {
                    tickDataSuits = true;
                }
            }
            else
            {
                if (isZero(targetNextTickDataDigest))
                {
                    // if target next tickdata digest is zero(empty tick) then invalidate the tickData in tickStorage
                    ts.tickData.acquireLock();
                    ts.tickData[nextTickIndex].epoch = INVALIDATED_TICK_DATA; // invalidate it and also tell request processor to not update it again
                    ts.tickData.releaseLock();
                    nextTickData.epoch = 0;
                    tickDataSuits = true;
                }
                else
                {
                    // Non-empty tick
                    if (nextTickData.epoch != system.epoch)
                    {
                        // not yet received or malformed next tick data
                        // or the tick data has been discarded because of timeout
                        tickDataSuits = false;
                    }
                    else
                    {
                        KangarooTwelve(&nextTickData, sizeof(TickData), &etalonTick.expectedNextTickTransactionDigest, 32);
                        tickDataSuits = (etalonTick.expectedNextTickTransactionDigest == targetNextTickDataDigest); // make sure the digests are matched
                    }
                }
            }

            // operator opt to force this node to switch to new epoch
            // this can fix the problem of weak nodes getting stuck and can't automatically switch to new epoch
            // due to lack of TRANSACTION data. Below is the explanation:
            // Assume the network is on transition state, usually we would have 2 set, set A: nodes that successfully
            // switched the epoch. And set B: nodes that stuck at the last tick of epoch, cannot switch to new epoch.
            // Let say set A is at (epoch E, tick N) and set B is at (epoch E-1, tick N-1)
            // Problem with current logic & implementation: set B would never switch to new epoch if operators not pressing F7.
            // (1) Because of the protocol design, when both set A & B are at epoch E-1, they receive tickData for tickN that is still marked at epoch E-1 (ie: tickData[N].epoch == E-1)
            // (2) All transactions on tickData[N] are also marked as epoch E-1 (ie: tickData[N].transaction[i].epoch == E-1)
            // (3) When set A successfully switch to epoch E, they clear that tick data of tick N and all relevant transactions, because they are invalid (wrong epoch number). Tick N and tick N+1 will be empty on new epoch.
            // (4) set B either can't get tickData of tick N or can't get all transactions of tick N, thus they will get stuck
            // (*) a node needs to know data and transactions of next tick to continue processing
            // From (1) (2) (3) (4) => operators of "stuck" nodes must press F7 on this case.
            // In other words, here is the explanation in timeline:
            //  - at (epoch E-1, tick N-2), tick leader issues tick data for tick N that's still on epoch E-1
            //  - at (epoch E-1, tick N-1), network decides to switch epoch. Thus, tickData for (epoch E-1, tick N) is invalid, because tick N is on epoch E.
            //  - Fast (internet) nodes that get all the data before tick N-1 can switch the epoch, then invalidate and clear tick N data
            //  - Slow (internet) nodes failed to get the data. Thus, they get stuck and constantly querying other nodes around.
            // [TODO] possible solutions:
            // Since we know the first and second ticks of new epoch are always empty, we can pre-check tickData here if it reaches the transition point,
            // then we can ignore the checking of next tick data/txs and go directly to epoch transition procedure.
            if (forceSwitchEpoch)
            {
                nextTickData.epoch = 0;
                setMem(nextTickData.transactionDigests, NUMBER_OF_TRANSACTIONS_PER_TICK * sizeof(m256i), 0);
                // first and second tick of an epoch are always empty tick
                targetNextTickDataDigest = m256i::zero();
                targetNextTickDataDigestIsKnown = true;
                tickDataSuits = true;
            }

            if (!tickDataSuits)
            {
                // if we have problem regarding lacking of tickData, then wait for MAIN loop to fetch those missing data
                // Here only need to update the stats and rerun the loop again
                gTickNumberOfComputors = 0;
                gTickTotalNumberOfComputors = countCurrentTickVote();
            }
            else
            {
                // tickData is suitable to be included (either non-empty or empty), now we need to verify all transactions in that tickData
                // The node needs to have all of transactions data
                numberOfNextTickTransactions = 0;
                numberOfKnownNextTickTransactions = 0;

                if (nextTickData.epoch == system.epoch)
                {
                    prepareNextTickTransactions();
                }

                if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
                {
                    if (!targetNextTickDataDigestIsKnown
                        && isTickTimeOut())
                    {
                        // If we don't have enough txs data for the next tick, and current/next tick votes not reach quorum
                        // and tick duration exceed 5*TARGET_TICK_DURATION, then it will temporarily discard next tickData, that will lead to zero expectedNextTickTransactionDigest
                        // That doesn't mean next tick has to be empty. Node is still waiting for votes from others to reach quorum
                        // and "refresh" this tickData again
                        // [dkat]: should we add a flag here to discard it once? Is updating ts.tickData needed?
                        ts.tickData.acquireLock();
                        ts.tickData[nextTickIndex].epoch = 0;
                        ts.tickData.releaseLock();
                        nextTickData.epoch = 0;

                        numberOfNextTickTransactions = 0;
                        numberOfKnownNextTickTransactions = 0;

#if !defined(NDEBUG)
                        {
                            CHAR16 dbgMsg[200];
                            setText(dbgMsg, L"Consensus: TIMEOUT at tick ");
                            appendNumber(dbgMsg, system.tick, true);
                            addDebugMessage(dbgMsg);
                        }
#endif
                    }
                }

                if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
                {
                    requestedTickTransactions.requestedTickTransactions.tick = nextTick;
                }
                else
                {
                    // This node has all required transactions
                    requestedTickTransactions.requestedTickTransactions.tick = 0;

                    if (ts.tickData[currentTickIndex].epoch == system.epoch)
                    {
                        KangarooTwelve(&ts.tickData[currentTickIndex], sizeof(TickData), &etalonTick.transactionDigest, 32);
                    }
                    else
                    {
                        etalonTick.transactionDigest = m256i::zero();
                        etalonTick.prevTransactionBodyDigest = 0;
                    }

                    if (nextTickData.epoch == system.epoch)
                    {
                        if (!targetNextTickDataDigestIsKnown)
                        {
                            // if this node is faster than most, targetNextTickDataDigest is unknown at this point because of lack of votes
                            // Thus, expectedNextTickTransactionDigest it not updated yet
                            // If targetNextTickDataDigest is known, expectedNextTickTransactionDigest is set above already.
                            KangarooTwelve(&nextTickData, sizeof(TickData), &etalonTick.expectedNextTickTransactionDigest, 32);
                        }

                        // Compute the txBodyDigest if expectedNextTickTransactionDigest changed
                        if (lastExpectedTickTransactionDigest != etalonTick.expectedNextTickTransactionDigest)
                        {
                            computeTxBodyDigestBase(nextTick);
                            lastExpectedTickTransactionDigest = etalonTick.expectedNextTickTransactionDigest;
                        }
                    }
                    else
                    {
                        etalonTick.expectedNextTickTransactionDigest = m256i::zero();
                        etalonTick.saltedTransactionBodyDigest = 0;
                        lastExpectedTickTransactionDigest = etalonTick.expectedNextTickTransactionDigest;
                    }


                    if (system.tick > system.latestCreatedTick || system.tick == system.initialTick)
                    {
                        if (isMainMode())
                        {
                            broadcastTickVotes();
                        }

                        if (system.tick != system.initialTick)
                        {
                            system.latestCreatedTick = system.tick;
                        }
                    }

                    unsigned int tickNumberOfComputors = 0, tickTotalNumberOfComputors = 0;
                    updateVotesCount(tickNumberOfComputors, tickTotalNumberOfComputors);

                    gTickNumberOfComputors = tickNumberOfComputors;
                    gTickTotalNumberOfComputors = tickTotalNumberOfComputors;

                    if (tickNumberOfComputors >= QUORUM)
                    {
                        tryForceEmptyNextTick();

                        if (targetNextTickDataDigestIsKnown)
                        {
                            tickDataSuits = false;
                            if (isZero(targetNextTickDataDigest))
                            {
                                // Empty tick
                                ts.tickData.acquireLock();
                                ts.tickData[nextTickIndex].epoch = INVALIDATED_TICK_DATA;
                                ts.tickData.releaseLock();
                                nextTickData.epoch = 0;
                                tickDataSuits = true;
                            }
                            else
                            {
                                if (nextTickData.epoch != system.epoch)
                                {
                                    tickDataSuits = false;
                                }
                                else
                                {
                                    KangarooTwelve(&nextTickData, sizeof(TickData), &etalonTick.expectedNextTickTransactionDigest, 32);
                                    tickDataSuits = (etalonTick.expectedNextTickTransactionDigest == targetNextTickDataDigest);
                                }
                            }
                            if (tickDataSuits)
                            {
                                const int dayIndex = ::dayIndex(etalonTick.year, etalonTick.month, etalonTick.day);
                                if ((dayIndex == 738570 + system.epoch * 7 && etalonTick.hour >= 12)
                                    || dayIndex > 738570 + system.epoch * 7)
                                {
                                    // start seamless epoch transition
                                    epochTransitionState = 1;
                                    forceSwitchEpoch = false;
                                }
                                else
                                {
                                    // update etalonTick
                                    etalonTick.tick++;
                                    ts.tickData.acquireLock();
                                    const TickData& td = ts.tickData[currentTickIndex];
                                    if (td.epoch == system.epoch
                                        && (td.year > etalonTick.year
                                            || (td.year == etalonTick.year && (td.month > etalonTick.month
                                                || (td.month == etalonTick.month && (td.day > etalonTick.day
                                                    || (td.day == etalonTick.day && (td.hour > etalonTick.hour
                                                        || (td.hour == etalonTick.hour && (td.minute > etalonTick.minute
                                                            || (td.minute == etalonTick.minute && (td.second > etalonTick.second
                                                                || (td.second == etalonTick.second && td.millisecond > etalonTick.millisecond)))))))))))))
                                    {
                                        etalonTick.millisecond = td.millisecond;
                                        etalonTick.second = td.second;
                                        etalonTick.minute = td.minute;
                                        etalonTick.hour = td.hour;
                                        etalonTick.day = td.day;
                                        etalonTick.month = td.month;
                                        etalonTick.year = td.year;
                                    }
                                    else
                                    {
                                        if (++etalonTick.millisecond > 999)
                                        {
                                            etalonTick.millisecond = 0;

                                            if (++etalonTick.second > 59)
                                            {
                                                etalonTick.second = 0;

                                                if (++etalonTick.minute > 59)
                                                {
                                                    etalonTick.minute = 0;

                                                    if (++etalonTick.hour > 23)
                                                    {
                                                        etalonTick.hour = 0;

                                                        if (++etalonTick.day > ((etalonTick.month == 1 || etalonTick.month == 3 || etalonTick.month == 5 || etalonTick.month == 7 || etalonTick.month == 8 || etalonTick.month == 10 || etalonTick.month == 12) ? 31 : ((etalonTick.month == 4 || etalonTick.month == 6 || etalonTick.month == 9 || etalonTick.month == 11) ? 30 : ((etalonTick.year & 3) ? 28 : 29))))
                                                        {
                                                            etalonTick.day = 1;

                                                            if (++etalonTick.month > 12)
                                                            {
                                                                etalonTick.month = 1;

                                                                ++etalonTick.year;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    ts.tickData.releaseLock();
                                }

                                system.tick++;

                                updateNumberOfTickTransactions();

                                bool isBeginEpoch = false;
                                if (epochTransitionState == 1)
                                {

                                    // wait until all request processors are in waiting state
                                    WAIT_WHILE(epochTransitionWaitingRequestProcessors < nRequestProcessorIDs);

                                    // end current epoch
                                    endEpoch();

                                    // Save the file of revenue. This blocking save can be called from any thread
                                    saveRevenueComponents(NULL);

                                    // instruct main loop to save system and wait until it is done
                                    systemMustBeSaved = true;
                                    WAIT_WHILE(systemMustBeSaved);
                                    epochTransitionState = 2;

                                    beginEpoch();
                                    isBeginEpoch = true;

                                    // Some debug checks that we are ready for the next epoch
                                    ASSERT(system.numberOfSolutions == 0);
                                    ASSERT(numberOfMiners == NUMBER_OF_COMPUTORS);
                                    ASSERT(isZero(system.solutions, sizeof(system.solutions)));
                                    ASSERT(isZero(solutionPublicationTicks, sizeof(solutionPublicationTicks)));
                                    ASSERT(isZero(minerSolutionFlags, NUMBER_OF_MINER_SOLUTION_FLAGS / 8));
                                    ASSERT(isZero((void*)minerScores, sizeof(minerScores)));
                                    ASSERT(isZero((void*)minerPublicKeys, sizeof(minerPublicKeys)));
                                    ASSERT(isZero(competitorScores, sizeof(competitorScores)));
                                    ASSERT(isZero(competitorPublicKeys, sizeof(competitorPublicKeys)));
                                    ASSERT(isZero(competitorComputorStatuses, sizeof(competitorComputorStatuses)));
                                    ASSERT(minimumComputorScore == 0 && minimumCandidateScore == 0);

                                    // instruct main loop to save files and wait until it is done
                                    spectrumMustBeSaved = true;
                                    universeMustBeSaved = true;
                                    computerMustBeSaved = true;
                                    WAIT_WHILE(computerMustBeSaved || universeMustBeSaved || spectrumMustBeSaved);

                                    // update etalon tick
                                    etalonTick.epoch++;
                                    etalonTick.tick++;
                                    etalonTick.saltedSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
                                    getUniverseDigest(etalonTick.saltedUniverseDigest);
                                    getComputerDigest(etalonTick.saltedComputerDigest);

                                    epochTransitionState = 0;
                                }
                                ASSERT(epochTransitionWaitingRequestProcessors >= 0 && epochTransitionWaitingRequestProcessors <= nRequestProcessorIDs);

                                short tickEpoch = 0;
                                TimeDate currentTickDate;
                                ts.tickData.acquireLock();
                                const TickData& td = ts.tickData[currentTickIndex];
                                currentTickDate.millisecond = td.millisecond;
                                currentTickDate.second = td.second;
                                currentTickDate.minute = td.minute;
                                currentTickDate.hour = td.hour;
                                currentTickDate.day = td.day;
                                currentTickDate.month = td.month;
                                currentTickDate.year = td.year;
                                tickEpoch = td.epoch == system.epoch ? system.epoch : 0;
                                ts.tickData.releaseLock();

                                checkAndSwitchMiningPhase(tickEpoch, currentTickDate, isBeginEpoch);

                                gTickNumberOfComputors = 0;
                                gTickTotalNumberOfComputors = 0;
                                targetNextTickDataDigestIsKnown = false;
                                numberOfNextTickTransactions = 0;
                                numberOfKnownNextTickTransactions = 0;

                                for (unsigned int i = 0; i < sizeof(tickTicks) / sizeof(tickTicks[0]) - 1; i++)
                                {
                                    tickTicks[i] = tickTicks[i + 1];
                                }
                                tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] = __rdtsc();
                            }
                        }
                    }
                }
            }
        }
        tickerLoopNumerator += __rdtsc() - curTimeTick;
        tickerLoopDenominator++;
    }
}
OPTIMIZE_ON()

static void emptyCallback(EFI_EVENT Event, void* Context)
{
}

static void shutdownCallback(EFI_EVENT Event, void* Context)
{
    closeEvent(Event);
}

// Required on timeout of callback processor
static void contractProcessorShutdownCallback(EFI_EVENT Event, void* Context)
{
    closeEvent(Event);
    if (isVirtualMachine)
    {
        // This must be called on VM
        contractProcessorState = 0;
    }
    // Timeout is disabled so far, because timeout recovery is not implemented yet.
    // So `contractProcessorState = 0` has been moved to the end of contractProcessor() to prevent unnecessary delay
    // in the tick processor, waiting for contract processor to finish.
    //contractProcessorState = 0;

    // TODO: If timeout is enabled, a solution is needed that properly handles both cases, regular end and timeout of
    // contractProcessor(). It also must prevent race conditions between different contract processor runs, because
    // the time between leaving contractProcessor() and entering contractProcessorShutdownCallback() is very high.
    // So the shutdown callback may be executed during the following contract processor run.
    // The reason of the delay probably is that the event callback execution is triggered by a timer. Test results
    // suggest that the timer interval is 0.1 second.
}

// directory: source directory to load the file. Default: NULL - load from root dir /
// forceLoadFromFile: when loading node states from file, we want to make sure it load from file and ignore constructionEpoch == system.epoch case
static bool loadComputer(CHAR16* directory, bool forceLoadFromFile)
{
    logToConsole(L"Loading contract files ...");
    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 9] = contractIndex / 1000 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 8] = (contractIndex % 1000) / 100 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 7] = (contractIndex % 100) / 10 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 6] = contractIndex % 10 + L'0';
        if (contractDescriptions[contractIndex].constructionEpoch == system.epoch && !forceLoadFromFile)
        {
            setText(message, L" -> ");
            appendText(message, CONTRACT_FILE_NAME);
            setMem(contractStates[contractIndex], contractDescriptions[contractIndex].stateSize, 0);
            appendText(message, L" not loaded but initialized with zeros for construction");
            logToConsole(message);
        }
        else
        {
            long long loadedSize = load(CONTRACT_FILE_NAME, contractDescriptions[contractIndex].stateSize, contractStates[contractIndex], directory);
            setText(message, L" -> "); // set the message after loading otherwise `message` will contain potential messages from load()
            appendText(message, CONTRACT_FILE_NAME);
            if (loadedSize != contractDescriptions[contractIndex].stateSize)
            {
                if (system.epoch < contractDescriptions[contractIndex].constructionEpoch && contractDescriptions[contractIndex].stateSize >= sizeof(IPO))
                {
                    setMem(contractStates[contractIndex], contractDescriptions[contractIndex].stateSize, 0);
                    appendText(message, L" not loaded but initialized with zeros for IPO");
                }
                else
                {
                    appendText(message, L" cannot be read successfully");
                    logToConsole(message);
                    logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);
                    return false;
                }
            }
            logToConsole(message);
        }
    }
    return true;
}

static bool saveComputer(CHAR16* directory)
{
    logToConsole(L"Saving contract files...");

    const unsigned long long beginningTick = __rdtsc();

    bool ok = true;
    unsigned long long totalSize = 0;

    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 9] = contractIndex / 1000 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 8] = (contractIndex % 1000) / 100 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 7] = (contractIndex % 100) / 10 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 6] = contractIndex % 10 + L'0';
        contractStateLock[contractIndex].acquireRead();
        long long savedSize = save(CONTRACT_FILE_NAME, contractDescriptions[contractIndex].stateSize, contractStates[contractIndex], directory);
        contractStateLock[contractIndex].releaseRead();
        totalSize += savedSize;
        if (savedSize != contractDescriptions[contractIndex].stateSize)
        {
            ok = false;

            break;
        }
    }

    if (ok)
    {
        setNumber(message, totalSize, TRUE);
        appendText(message, L" bytes of the computer data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
        return true;
    }
    return false;
}

static bool saveSystem(CHAR16* directory)
{
    logToConsole(L"Saving system file...");

    const unsigned long long beginningTick = __rdtsc();
    CHAR16* fn = (epochTransitionState == 1) ? SYSTEM_END_OF_EPOCH_FILE_NAME : SYSTEM_FILE_NAME;
    long long savedSize = save(fn, sizeof(system), (unsigned char*)&system, directory);
    if (savedSize == sizeof(system))
    {
        setNumber(message, savedSize, TRUE);
        appendText(message, L" bytes of the system data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
        return true;
    }
    return false;
}

static bool saveRevenueComponents(CHAR16* directory)
{
    CHAR16* fn = CUSTOM_MINING_REVENUE_END_OF_EPOCH_FILE_NAME;
    long long savedSize = asyncSave(fn, sizeof(gRevenueComponents), (unsigned char*)&gRevenueComponents, directory);
    if (savedSize == sizeof(gRevenueComponents))
    {
        return true;
    }
    return false;
}

static bool initialize()
{
    enableAVX();

#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
#if defined (__AVX512F__)
    initAVX512FourQConstants();
#endif

    if (!initSpecialEntities())
        return false;

    initTimeStampCounter();

#ifdef ENABLE_PROFILING
    if (!gProfilingDataCollector.init(1024))
    {
        logToConsole(L"gProfilingDataCollector.init() failed!");
        return false;
    }
#endif

    setMem(&tickTicks, sizeof(tickTicks), 0);

    setMem(processors, sizeof(processors), 0);
    setMem(peers, sizeof(peers), 0);
    setMem(publicPeers, sizeof(publicPeers), 0);

    requestedComputors.header.setSize<sizeof(requestedComputors)>();
    requestedComputors.header.setType(RequestComputors::type);
    requestedQuorumTick.header.setSize<sizeof(requestedQuorumTick)>();
    requestedQuorumTick.header.setType(RequestQuorumTick::type);
    requestedTickData.header.setSize<sizeof(requestedTickData)>();
    requestedTickData.header.setType(RequestTickData::type);
    requestedTickTransactions.header.setSize<sizeof(requestedTickTransactions)>();
    requestedTickTransactions.header.setType(REQUEST_TICK_TRANSACTIONS);
    requestedTickTransactions.requestedTickTransactions.tick = 0;

    if (!initFilesystem())
        return false;

    EFI_STATUS status;
    {
        if (!ts.init())
            return false;
        if (!allocPoolWithErrorLog(L"entityPendingTransaction buffer", SPECTRUM_CAPACITY * MAX_TRANSACTION_SIZE,(void**)&entityPendingTransactions, __LINE__) ||
            !allocPoolWithErrorLog(L"entityPendingTransaction buffer", SPECTRUM_CAPACITY * 32ULL,(void**)&entityPendingTransactionDigests , __LINE__))
        {
            return false;
        }

        if (!allocPoolWithErrorLog(L"computorPendingTransactions buffer", NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * MAX_TRANSACTION_SIZE, (void**)&computorPendingTransactions, __LINE__) ||
            !allocPoolWithErrorLog(L"computorPendingTransactions buffer", NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * 32ULL, (void**)&computorPendingTransactionDigests, __LINE__))
        {
            return false;
        }
        

        setMem(spectrumChangeFlags, sizeof(spectrumChangeFlags), 0);


        if (!initSpectrum())
            return false;

        if (!initCommonBuffers())
            return false;

        if (!initAssets())
            return false;

        initContractExec();
        for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
        {
            unsigned long long size = contractDescriptions[contractIndex].stateSize;
            if (!allocPoolWithErrorLog(L"contractStates",  size, (void**)&contractStates[contractIndex], __LINE__))
            {
                return false;
            }
        }

        if (!allocPoolWithErrorLog(L"score", sizeof(*score), (void**)&score, __LINE__))
        {
            return false;
        }
        setMem(score, sizeof(*score), 0);

        if (!allocPoolWithErrorLog(L"score", sizeof(*score_qpi), (void**)&score_qpi, __LINE__))
        {
            return false;
        }
        setMem(score_qpi, sizeof(*score_qpi), 0);

        setMem(solutionThreshold, sizeof(int) * MAX_NUMBER_EPOCH, 0);
        if (!allocPoolWithErrorLog(L"minserSolutionFlag", NUMBER_OF_MINER_SOLUTION_FLAGS / 8, (void**)&minerSolutionFlags, __LINE__))
        {
            return false;
        }

        if (!logger.initLogging())
        {
            return false;
        }
            

#if ADDON_TX_STATUS_REQUEST
        if (!initTxStatusRequestAddOn())
        {
            logToConsole(L"initTxStatusRequestAddOn() failed!");
            return false;
        }
#endif

        logToConsole(L"Loading system file ...");
        setMem(&system, sizeof(system), 0);
        load(SYSTEM_FILE_NAME, sizeof(system), (unsigned char*)&system);
        system.version = VERSION_B;
        system.epoch = EPOCH;
        system.initialMillisecond = 0;
        system.initialSecond = 0;
        system.initialMinute = 0;
        system.initialHour = 12;
        system.initialDay = 13;
        system.initialMonth = 4;
        system.initialYear = 22;
        if (system.epoch == EPOCH)
        {
            system.initialTick = TICK;
        }
        system.tick = system.initialTick;

        lastExpectedTickTransactionDigest = m256i::zero();

        //Init custom mining data. Reset function will be called in beginEpoch()
        customMiningInitialize();

        beginEpoch();

        // needs to be called after ts.beginEpoch() because it looks up tickIndex, which requires to setup begin of epoch in ts
        updateNumberOfTickTransactions();

#if TICK_STORAGE_AUTOSAVE_MODE
        bool canLoadFromFile = loadAllNodeStates();
#else
        bool canLoadFromFile = false;
#endif

        // if failed to load snapshot, load all data and init variables from scratch
        if (!canLoadFromFile)
        {
            etalonTick.epoch = system.epoch;
            etalonTick.tick = system.initialTick;
            etalonTick.millisecond = system.initialMillisecond;
            etalonTick.second = system.initialSecond;
            etalonTick.minute = system.initialMinute;
            etalonTick.hour = system.initialHour;
            etalonTick.day = system.initialDay;
            etalonTick.month = system.initialMonth;
            etalonTick.year = system.initialYear;

            loadSpectrum();
            {
                const unsigned long long beginningTick = __rdtsc();

                unsigned int digestIndex;
                for (digestIndex = 0; digestIndex < SPECTRUM_CAPACITY; digestIndex++)
                {
                    KangarooTwelve64To32(&spectrum[digestIndex], &spectrumDigests[digestIndex]);
                }
                unsigned int previousLevelBeginning = 0;
                unsigned int numberOfLeafs = SPECTRUM_CAPACITY;
                while (numberOfLeafs > 1)
                {
                    for (unsigned int i = 0; i < numberOfLeafs; i += 2)
                    {
                        KangarooTwelve64To32(&spectrumDigests[previousLevelBeginning + i], &spectrumDigests[digestIndex++]);
                    }

                    previousLevelBeginning += numberOfLeafs;
                    numberOfLeafs >>= 1;
                }

                setNumber(message, SPECTRUM_CAPACITY * sizeof(EntityRecord), TRUE);
                appendText(message, L" bytes of the spectrum data are hashed (");
                appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
                appendText(message, L" microseconds).");
                logToConsole(message);

                CHAR16 digestChars[60 + 1];
                getIdentity((unsigned char*)&spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1], digestChars, true);
                updateSpectrumInfo();

                setNumber(message, spectrumInfo.totalAmount, TRUE);
                appendText(message, L" qus in ");
                appendNumber(message, spectrumInfo.numberOfEntities, TRUE);
                appendText(message, L" entities (digest = ");
                appendText(message, digestChars);
                appendText(message, L").");
                logToConsole(message);
            }
            logToConsole(L"Loading universe file ...");
            if (!loadUniverse())
                return false;
            m256i universeDigest;
            {
                setText(message, L"Universe digest = ");
                getUniverseDigest(universeDigest);
                CHAR16 digestChars[60 + 1];
                getIdentity(universeDigest.m256i_u8, digestChars, true);
                appendText(message, digestChars);
                appendText(message, L".");
                logToConsole(message);
            }
            if (!loadComputer())
                return false;
            m256i computerDigest;
            {
                setText(message, L"Computer digest = ");
                getComputerDigest(computerDigest);
                CHAR16 digestChars[60 + 1];
                getIdentity(computerDigest.m256i_u8, digestChars, true);
                appendText(message, digestChars);
                appendText(message, L".");
                logToConsole(message);
            }

            // initialize salted digests of etalonTick, otherwise F2 key would output invalid digests
            // before ticking begins
            etalonTick.saltedSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
            etalonTick.saltedUniverseDigest = universeDigest;
            etalonTick.saltedComputerDigest = computerDigest;
        }
        else
        {
            loadAllNodeStateFromFile = true;
            logToConsole(L"Loaded node state from snapshot, if you want to start from scratch please delete all snapshot files.");
        }
    }

    initializeContracts();

    if (loadMiningSeedFromFile)
    {
        score->initMiningData(initialRandomSeedFromPersistingState);
        loadMiningSeedFromFile = false;;
    }
    else
    {
        short tickEpoch = -1; 
        TimeDate tickDate;
        setMem((void*)&tickDate, sizeof(TimeDate), 0);
        checkAndSwitchMiningPhase(tickEpoch, tickDate, true);
    }    
    score->loadScoreCache(system.epoch);

    loadCustomMiningCache(system.epoch);

    logToConsole(L"Allocating buffers ...");
    if ((!allocPoolWithErrorLog(L"dejavu0", 536870912, (void**)&dejavu0, __LINE__)) ||
        (!allocPoolWithErrorLog(L"dejavu1", 536870912, (void**)&dejavu1, __LINE__)))
    {
        return false;
    }
    setMem((void*)dejavu0, 536870912, 0);
    setMem((void*)dejavu1, 536870912, 0);

    if ((!allocPoolWithErrorLog(L"requestQueueBuffer", REQUEST_QUEUE_BUFFER_SIZE, (void**)&requestQueueBuffer, __LINE__)) ||
        (!allocPoolWithErrorLog(L"respondQueueBuffer", RESPONSE_QUEUE_BUFFER_SIZE, (void**)&responseQueueBuffer, __LINE__)))
    {
        return false;
    }

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        peers[i].receiveData.FragmentCount = 1;
        peers[i].transmitData.FragmentCount = 1;

        if ((!allocPoolWithErrorLog(L"receiveBuffer", BUFFER_SIZE, &peers[i].receiveBuffer, __LINE__))  ||
            (!allocPoolWithErrorLog(L"FragmentBuffer", BUFFER_SIZE, &peers[i].transmitData.FragmentTable[0].FragmentBuffer, __LINE__)) ||
            (!allocPoolWithErrorLog(L"dataToTransmit", BUFFER_SIZE, (void**)&peers[i].dataToTransmit, __LINE__)))
        {
            return false;
        }

        if ((status = createEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].connectAcceptToken.CompletionToken.Event))
            || (status = createEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].receiveToken.CompletionToken.Event))
            || (status = createEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].transmitToken.CompletionToken.Event)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.CreateEvent() fails", status, __LINE__);

            return false;
        }
        peers[i].connectAcceptToken.CompletionToken.Status = -1;
        peers[i].receiveToken.CompletionToken.Status = -1;
        peers[i].receiveToken.Packet.RxData = &peers[i].receiveData;
        peers[i].transmitToken.CompletionToken.Status = -1;
        peers[i].transmitToken.Packet.TxData = &peers[i].transmitData;

        // Init the connection type as 
        if (i < NUMBER_OF_OUTGOING_CONNECTIONS)
        {
            peers[i].isIncommingConnection = FALSE;
        }
        else
        {
            peers[i].isIncommingConnection = TRUE;
        }
    }

    // add knownPublicPeers to list of peers (all with verified status)
    logToConsole(L"Populating publicPeers ...");
    for (unsigned int i = 0; i < sizeof(knownPublicPeers) / sizeof(knownPublicPeers[0]) && numberOfPublicPeers < MAX_NUMBER_OF_PUBLIC_PEERS; i++)
    {
        const IPv4Address& peer_ip = *reinterpret_cast<const IPv4Address*>(knownPublicPeers[i]);
        addPublicPeer(peer_ip);
        if (numberOfPublicPeers > 0)
        {
            publicPeers[numberOfPublicPeers - 1].isHandshaked = true;
            publicPeers[numberOfPublicPeers - 1].isFullnode = true;
        }
    }
    if (numberOfPublicPeers < 4)
    {
        setText(message, L"WARNING: Only ");
        appendNumber(message, numberOfPublicPeers, FALSE);
        appendText(message, L" knownPublicPeers were given. It is recommendended to enter at least 4!");
        logToConsole(message);
    }

    logToConsole(L"Init TCP...");
    if (!initTcp4(PORT))
        return false;
    

    auto& addr = nodeAddress.Addr;
    if ((!addr[0]) || (addr[0] == 127) || (addr[0] == 10)
        || (addr[0] == 172 && addr[1] >= 16 && addr[1] <= 31)
        || (addr[0] == 192 && addr[1] == 168) || (addr[0] == 255))
    {
        logToConsole(L"Detected node running on virtual machine");
        isVirtualMachine = 1;
    }
    else
    {
        logToConsole(L"Detected node running on Bare Metal");
        isVirtualMachine = 0;
    }

    emptyTickResolver.clock = 0;
    emptyTickResolver.tick = 0;
    emptyTickResolver.lastTryClock = 0;

    // Convert time parameters for full custom mining time
    if (gNumberOfFullExternalMiningEvents > 0)
    {
        if ((!allocPoolWithErrorLog(L"gFullExternalEventTime", gNumberOfFullExternalMiningEvents * sizeof(FullExternallEvent), (void**)&gFullExternalEventTime, __LINE__)))
        {
            return false;
        }
        for (int i = 0; i < gNumberOfFullExternalMiningEvents; i++)
        {
            gFullExternalEventTime[i].startTime = convertWeekTimeFromPackedData(gFullExternalComputationTimes[i][0]);
            gFullExternalEventTime[i].endTime = convertWeekTimeFromPackedData(gFullExternalComputationTimes[i][1]);
        }
    }

    return true;
}

static void deinitialize()
{
    deinitTcp4();

    deinitSpecialEntities();

    if (root)
    {
        root->Close(root);
    }

    deinitAssets();
    deinitSpectrum();
    deinitCommonBuffers();

    logger.deinitLogging();

    deInitFileSystem();

#if ADDON_TX_STATUS_REQUEST
    deinitTxStatusRequestAddOn();
#endif

    deinitContractExec();
    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        if (contractStates[contractIndex])
        {
            freePool(contractStates[contractIndex]);
        }
    }

    if (computorPendingTransactionDigests)
    {
        freePool(computorPendingTransactionDigests);
    }
    if (computorPendingTransactions)
    {
        freePool(computorPendingTransactions);
    }
    if (entityPendingTransactionDigests)
    {
        freePool(entityPendingTransactionDigests);
    }
    if (entityPendingTransactions)
    {
        freePool(entityPendingTransactions);
    }
    ts.deinit();

    if (score)
    {
        freePool(score);
    }
    if (minerSolutionFlags)
    {
        freePool(minerSolutionFlags);
    }

    if (dejavu0)
    {
        freePool((void*)dejavu0);
    }
    if (dejavu1)
    {
        freePool((void*)dejavu1);
    }

    if (requestQueueBuffer)
    {
        freePool(requestQueueBuffer);
    }
    if (responseQueueBuffer)
    {
        freePool(responseQueueBuffer);
    }

    for (unsigned int processorIndex = 0; processorIndex < MAX_NUMBER_OF_PROCESSORS; processorIndex++)
    {
        if (processors[processorIndex].buffer)
        {
            freePool(processors[processorIndex].buffer);
        }
    }

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].receiveBuffer)
        {
            freePool(peers[i].receiveBuffer);
        }
        if (peers[i].transmitData.FragmentTable[0].FragmentBuffer)
        {
            freePool(peers[i].transmitData.FragmentTable[0].FragmentBuffer);
        }
        if (peers[i].dataToTransmit)
        {
            freePool(peers[i].dataToTransmit);

            closeEvent(peers[i].connectAcceptToken.CompletionToken.Event);
            closeEvent(peers[i].receiveToken.CompletionToken.Event);
            closeEvent(peers[i].transmitToken.CompletionToken.Event);
        }
    }

    customMiningDeinitialize();
}

static void logInfo()
{
    if (consoleLoggingLevel == 0)
    {
        return;
    }

    unsigned long long numberOfWaitingBytes = 0;

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].tcp4Protocol)
        {
            numberOfWaitingBytes += peers[i].dataToTransmitSize;
        }
    }

    unsigned int numberOfHandshakedPublicPeers = 0;
    unsigned int numberOfFullnodePublicPeers = 0;

    for (unsigned int i = 0; i < numberOfPublicPeers; i++)
    {
        if (publicPeers[i].isHandshaked)
        {
            numberOfHandshakedPublicPeers++;
        }

        if (publicPeers[i].isFullnode)
        {
            numberOfFullnodePublicPeers++;
        }
    }

    setText(message, L"[+");
    appendNumber(message, numberOfProcessedRequests - prevNumberOfProcessedRequests, TRUE);
    appendText(message, L" -");
    appendNumber(message, numberOfDiscardedRequests - prevNumberOfDiscardedRequests, TRUE);
    appendText(message, L" *");
    appendNumber(message, numberOfDuplicateRequests - prevNumberOfDuplicateRequests, TRUE);
    appendText(message, L" /");
    appendNumber(message, numberOfDisseminatedRequests - prevNumberOfDisseminatedRequests, TRUE);
    appendText(message, L"] ");

    unsigned int numberOfConnectingSlots = 0, numberOfConnectedSlots = 0;
    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].tcp4Protocol)
        {
            if (!peers[i].isConnectedAccepted)
            {
                numberOfConnectingSlots++;
            }
            else
            {
                numberOfConnectedSlots++;
            }
        }
    }
    appendNumber(message, numberOfConnectingSlots, FALSE);
    appendText(message, L"|");
    appendNumber(message, numberOfConnectedSlots, FALSE);

    appendText(message, L" ");
    appendNumber(message, numberOfHandshakedPublicPeers, TRUE);
    appendText(message, L"/");
    appendNumber(message, numberOfFullnodePublicPeers, TRUE);
    appendText(message, L"/");    
    appendNumber(message, numberOfPublicPeers, TRUE);
    appendText(message, listOfPeersIsStatic ? L" Static" : L" Dynamic");
    appendText(message, L" (+");
    appendNumber(message, numberOfReceivedBytes - prevNumberOfReceivedBytes, TRUE);
    appendText(message, L" -");
    appendNumber(message, numberOfTransmittedBytes - prevNumberOfTransmittedBytes, TRUE);
    appendText(message, L" ..."); appendNumber(message, numberOfWaitingBytes, TRUE);
    appendText(message, L").");
#if USE_SCORE_CACHE
    appendText(message, L" Score cache: Hit ");
    appendNumber(message, score->scoreCache.hitCount(), TRUE);
    appendText(message, L" | Collision ");
    appendNumber(message, score->scoreCache.collisionCount(), TRUE);
    appendText(message, L" | Miss ");
    appendNumber(message, score->scoreCache.missCount(), TRUE);
#endif
    logToConsole(message);
    prevNumberOfProcessedRequests = numberOfProcessedRequests;
    prevNumberOfDiscardedRequests = numberOfDiscardedRequests;
    prevNumberOfDuplicateRequests = numberOfDuplicateRequests;
    prevNumberOfDisseminatedRequests = numberOfDisseminatedRequests;
    prevNumberOfReceivedBytes = numberOfReceivedBytes;
    prevNumberOfTransmittedBytes = numberOfTransmittedBytes;

    setNumber(message, numberOfProcessors - 2, TRUE);

    appendText(message, L" | Tick = ");
    unsigned long long tickDuration = (tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] - tickTicks[0]) / (sizeof(tickTicks) / sizeof(tickTicks[0]) - 1);
    appendNumber(message, tickDuration / frequency, FALSE);
    appendText(message, L".");
    appendNumber(message, (tickDuration % frequency) * 10 / frequency, FALSE);
    appendText(message, L" s");
    

    if (consoleLoggingLevel < 2)
    {
        logToConsole(message);
        return;
    }

    appendText(message, L" | Indices = ");
    if (!numberOfOwnComputorIndices)
    {
        appendText(message, L"?.");
    }
    else
    {
        const CHAR16 alphabet[26][2] = { L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z" };
        for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
        {
            appendText(message, alphabet[ownComputorIndices[i] / 26]);
            appendText(message, alphabet[ownComputorIndices[i] % 26]);
            if (i < (unsigned int)(numberOfOwnComputorIndices - 1))
            {
                appendText(message, L"+");
            }
            else
            {
                appendText(message, L".");
            }
        }
    }
    logToConsole(message);

    unsigned int numberOfPendingTransactions = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR; i++)
    {
        if (((Transaction*)&computorPendingTransactions[i * MAX_TRANSACTION_SIZE])->tick > system.tick)
        {
            numberOfPendingTransactions++;
        }
    }
    for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
    {
        if (((Transaction*)&entityPendingTransactions[i * MAX_TRANSACTION_SIZE])->tick > system.tick)
        {
            numberOfPendingTransactions++;
        }
    }
    if (nextTickTransactionsSemaphore)
    {
        setText(message, L"?");
    }
    else
    {
        setNumber(message, numberOfKnownNextTickTransactions, TRUE);
    }
    appendText(message, L"/");
    if (nextTickTransactionsSemaphore)
    {
        appendText(message, L"?");
    }
    else
    {
        appendNumber(message, numberOfNextTickTransactions, TRUE);
    }
    appendText(message, L" next tick transactions are known. ");
    const TickData& td = ts.tickData.getByTickInCurrentEpoch(system.tick + 1);
    if (td.epoch == system.epoch)
    {
        appendText(message, L"(");
        appendNumber(message, td.year / 10, FALSE);
        appendNumber(message, td.year % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, td.month / 10, FALSE);
        appendNumber(message, td.month % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, td.day / 10, FALSE);
        appendNumber(message, td.day % 10, FALSE);
        appendText(message, L" ");
        appendNumber(message, td.hour / 10, FALSE);
        appendNumber(message, td.hour % 10, FALSE);
        appendText(message, L":");
        appendNumber(message, td.minute / 10, FALSE);
        appendNumber(message, td.minute % 10, FALSE);
        appendText(message, L":");
        appendNumber(message, td.second / 10, FALSE);
        appendNumber(message, td.second % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, td.millisecond / 100, FALSE);
        appendNumber(message, (td.millisecond % 100) / 10, FALSE);
        appendNumber(message, td.millisecond % 10, FALSE);
        appendText(message, L".) ");
    }
    appendNumber(message, numberOfPendingTransactions, TRUE);
    appendText(message, L" pending transactions.");
    logToConsole(message);

    unsigned int filledRequestQueueBufferSize = (requestQueueBufferHead >= requestQueueBufferTail) ? (requestQueueBufferHead - requestQueueBufferTail) : (REQUEST_QUEUE_BUFFER_SIZE - (requestQueueBufferTail - requestQueueBufferHead));
    unsigned int filledResponseQueueBufferSize = (responseQueueBufferHead >= responseQueueBufferTail) ? (responseQueueBufferHead - responseQueueBufferTail) : (RESPONSE_QUEUE_BUFFER_SIZE - (responseQueueBufferTail - responseQueueBufferHead));
    unsigned int filledRequestQueueLength = (requestQueueElementHead >= requestQueueElementTail) ? (requestQueueElementHead - requestQueueElementTail) : (REQUEST_QUEUE_LENGTH - (requestQueueElementTail - requestQueueElementHead));
    unsigned int filledResponseQueueLength = (responseQueueElementHead >= responseQueueElementTail) ? (responseQueueElementHead - responseQueueElementTail) : (RESPONSE_QUEUE_LENGTH - (responseQueueElementTail - responseQueueElementHead));
    setNumber(message, filledRequestQueueBufferSize, TRUE);
    appendText(message, L" (");
    appendNumber(message, filledRequestQueueLength, TRUE);
    appendText(message, L") :: ");
    appendNumber(message, filledResponseQueueBufferSize, TRUE);
    appendText(message, L" (");
    appendNumber(message, filledResponseQueueLength, TRUE);
    appendText(message, L") | Average processing time = ");
    if (queueProcessingDenominator)
    {
        appendNumber(message, (queueProcessingNumerator / queueProcessingDenominator) * 1000000 / frequency, TRUE);
    }
    else
    {
        appendText(message, L"?");
    }
    appendText(message, L" mcs | Total Qx execution time = ");
    appendNumber(message, contractTotalExecutionTicks[QX_CONTRACT_INDEX] * 1000 / frequency, TRUE);
    appendText(message, L" ms | Solution process time = ");
    appendNumber(message, solutionTotalExecutionTicks * 1000 / frequency, TRUE);
    appendText(message, L" ms | Spectrum reorg time = ");
    appendNumber(message, spectrumReorgTotalExecutionTicks * 1000 / frequency, TRUE);
    appendText(message, L" ms.");
    logToConsole(message);

    // Log infomation about custom mining
    setText(message, L"CustomMining: ");

    // System: Active status | Overflow | Collision
    char isCustomMiningStateActive = 0;
    ACQUIRE(gIsInCustomMiningStateLock);
    isCustomMiningStateActive = gIsInCustomMiningState;
    RELEASE(gIsInCustomMiningStateLock);

    if (isCustomMiningStateActive)
    {
        appendText(message, L"Active. ");
    }
    else
    {
        appendText(message, L"Inactive. ");
    }

    long long customMiningShareMaxOFCount = ATOMIC_LOAD64(gCustomMiningStats.maxOverflowShareCount);
    long long customMiningSharesMaxCollision = ATOMIC_LOAD64(gCustomMiningStats.maxCollisionShareCount);
    appendText(message, L" Overflow: ");
    appendNumber(message, customMiningShareMaxOFCount, false);
    appendText(message, L" | Collision: ");
    appendNumber(message, customMiningSharesMaxCollision, false);
    appendText(message, L".");

    logToConsole(message);

}

static void logHealthStatus()
{
    setText(message, (isMainMode()) ? L"MAIN" : L"aux");
    appendText(message, L"&");
    appendText(message, (mainAuxStatus & 2) ? L"MAIN" : L"aux");
    logToConsole(message);

    // print statuses of thread
    // this accepts a small error when switching day to first day of the next month
    bool allThreadsAreGood = true;
    setText(message, L"Thread status: ");
    for (int i = 0; i < nTickProcessorIDs; i++)
    {
        unsigned long long tid = tickProcessorIDs[i];
        long long diffInSecond = 86400 * (utcTime.Day - threadTimeCheckin[tid].day) + 3600 * (utcTime.Hour - threadTimeCheckin[tid].hour)
            + 60 * (utcTime.Minute - threadTimeCheckin[tid].minute) + (utcTime.Second - threadTimeCheckin[tid].second);
        if (diffInSecond > 120) // if they don't check in in 2 minutes, we can assume the thread is already crashed
        {
            allThreadsAreGood = false;
            appendText(message, L"Tick Processor #");
            appendNumber(message, tid, false);
            appendText(message, L" is not responsive | ");
        }
    }

    for (int i = 0; i < nRequestProcessorIDs; i++)
    {
        unsigned long long tid = requestProcessorIDs[i];
        long long diffInSecond = 86400 * (utcTime.Day - threadTimeCheckin[tid].day) + 3600 * (utcTime.Hour - threadTimeCheckin[tid].hour)
            + 60 * (utcTime.Minute - threadTimeCheckin[tid].minute) + (utcTime.Second - threadTimeCheckin[tid].second);
        if (diffInSecond > 120) // if they don't check in in 2 minutes, we can assume the thread is already crashed
        {
            allThreadsAreGood = false;
            appendText(message, L"Request Processor #");
            appendNumber(message, tid, false);
            appendText(message, L" is not responsive | ");
        }
    }
    if (allThreadsAreGood)
    {
        appendText(message, L"All threads are healthy.");
    }
    logToConsole(message);

    // Print used function call stack size
    setText(message, L"Function call stack usage: ");
    unsigned int maxStackUsageTick = 0;
    unsigned int maxStackUsageContract = 0;
    unsigned int maxStackUsageRequest = 0;
    for (int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
    {
        const Processor& processor = processors[i];
        unsigned int used = processor.maxStackUsed();
        switch (processor.type)
        {
        case Processor::TickProcessor:
            if (maxStackUsageTick < used)
                maxStackUsageTick = used;
            break;
        case Processor::ContractProcessor:
            if (maxStackUsageContract < used)
                maxStackUsageContract = used;
            break;
        case Processor::RequestProcessor:
            if (maxStackUsageRequest < used)
                maxStackUsageRequest = used;
            break;
        }
    }
    appendText(message, L"Contract Processor ");
    appendNumber(message, maxStackUsageContract, TRUE);
    appendText(message, L" | Tick Processor ");
    appendNumber(message, maxStackUsageTick, TRUE);
    appendText(message, L" | Request Processor ");
    appendNumber(message, maxStackUsageRequest, TRUE);
    appendText(message, L" | Capacity ");
    appendNumber(message, STACK_SIZE, TRUE);
    logToConsole(message);
    if (maxStackUsageContract > STACK_SIZE / 2 || maxStackUsageTick > STACK_SIZE / 2 || maxStackUsageRequest > STACK_SIZE / 2)
    {
        logToConsole(L"WARNING: Developers should increase stack size!");
    }

    setText(message, L"Contract status: ");
    bool anyContractError = false;
    for (int i = 0; i < contractCount; i++)
    {
        if (contractError[i])
        {
            if (anyContractError)
                appendText(message, L" | ");
            anyContractError = true;
            appendText(message, L"Contract #");
            appendNumber(message, i, FALSE);
            appendText(message, L": ");
            const CHAR16* errorMsg = L"Unknown error";
            switch (contractError[i])
            {
            // The alloc failures can be fixed by increasing the size of ContractLocalsStack
            case ContractErrorAllocInputOutputFailed: errorMsg = L"AllocInputOutputFailed"; break;
            case ContractErrorAllocLocalsFailed: errorMsg = L"AllocLocalsFailed"; break;
            case ContractErrorAllocContextOtherFunctionCallFailed: errorMsg = L"AllocContextOtherFunctionCallFailed"; break;
            case ContractErrorAllocContextOtherProcedureCallFailed: errorMsg = L"AllocContextOtherProcedureCallFailed"; break;
            // TooManyActions can be fixed by calling less actions or increasing the size of ContractActionTracker
            case ContractErrorTooManyActions: errorMsg = L"TooManyActions"; break;
            // Timeout requires to remove endless loop, speed-up code, or change the timeout
            case ContractErrorTimeout: errorMsg = L"Timeout"; break;
            }
            appendText(message, errorMsg);
        }
    }
    if (!anyContractError)
    {
        appendText(message, L"all healthy");
    }
    logToConsole(message);

    // Print info about stack buffers used to run contracts
    setText(message, L"Contract stack buffer usage: ");
    for (int i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS; ++i)
    {
        appendText(message, L"buf ");
        appendNumber(message, i, FALSE);
        if (contractLocalsStackLock[i])
            appendText(message, L" (locked)");
        appendText(message, L" current ");
        appendNumber(message, contractLocalsStack[i].size(), TRUE);
#ifdef TRACK_MAX_STACK_BUFFER_SIZE
        appendText(message, L", max ");
        appendNumber(message, contractLocalsStack[i].maxSizeObserved(), TRUE);
        appendText(message, L", failed alloc ");
        appendNumber(message, contractLocalsStack[i].failedAllocAttempts(), TRUE);
#endif
        appendText(message, L" | ");
    }
    appendText(message, L"capacity per buf ");
    appendNumber(message, contractLocalsStack[0].capacity(), TRUE);
    appendText(message, L" | max processors waiting ");
    appendNumber(message, contractLocalsStackLockWaitingCountMax, TRUE);
    logToConsole(message);

    setText(message, L"Connections:");
    for (int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; ++i)
    {
        unsigned long long connectionStatusIndicator = (unsigned long long)peers[i].tcp4Protocol;
        if (connectionStatusIndicator > 1)
        {
            appendText(message, L" [");
            appendIPv4Address(message, peers[i].address);
            appendText(message, L":");
            appendText(message, peers[i].isIncommingConnection ? L"i" : L"o");
            if (peers[i].isClosing)
            {
                appendText(message, L"c");
            }
            if (peers[i].isReceiving)
            {
                appendText(message, L"r");
            }
            if (peers[i].isTransmitting)
            {
                appendText(message, L"t");
                appendNumber(message, peers[i].dataToTransmitSize, FALSE);
            }
            appendText(message, L"]");
        }
    }
    logToConsole(message);
}

static void processKeyPresses()
{
    EFI_INPUT_KEY key;
    if (!st->ConIn->ReadKeyStroke(st->ConIn, &key))
    {

        
        // map normal key strokes or execute actions
        switch (key.UnicodeChar) {
            /*
            * maps the key.ScanCode to the pause key
            * on some mainboards the PAUSE key does real pause and on Apple is no PAUSE key
            */
        case L'p':
            key.ScanCode = 0x48; // map to pause key; action defined below
            break;
            /*
            * Just prints QUBIC QUBIC QUBIC QUBIC QUBIC to the screen
            * An example how to use other keys and directly return to not continue with the ScanCode
            * Uncomment it for testing
            */
            // case L'q':
            // setText(message, L"QUBIC QUBIC QUBIC QUBIC QUBIC");
            // logToConsole(message);
            // return;
        }


        switch (key.ScanCode)
        {
        /*
        *
        * F2 Key
        * By pressing the F2 Key the node will display the current status.
        * The status includes:
        * Version, faulty Computors, Last Tick Date,
        * Digest of spectrum, universe, and computer, number of transactions and solutions processed
        */
        case 0x0C:
        {
#ifndef NDEBUG
            forceLogToConsoleAsAddDebugMessage = true;
#endif

            setText(message, L"Qubic ");
            appendQubicVersion(message);
            appendText(message, L".");
            logToConsole(message);

            unsigned int numberOfFaultyComputors = 0;
            for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                if (faultyComputorFlags[i >> 6] & (1ULL << (i & 63)))
                {
                    getIdentity(broadcastedComputors.computors.publicKeys[i].m256i_u8, message, false);
                    appendText(message, L" = ");
                    long long amount = 0;
                    const int spectrumIndex = ::spectrumIndex(broadcastedComputors.computors.publicKeys[i]);
                    if (spectrumIndex >= 0)
                    {
                        amount = energy(spectrumIndex);
                    }
                    appendNumber(message, amount, TRUE);
                    appendText(message, L" qus");
                    logToConsole(message);

                    numberOfFaultyComputors++;
                }
            }
            setNumber(message, numberOfFaultyComputors, TRUE);
            appendText(message, L" faulty computors.");
            logToConsole(message);

            setText(message, L"Tick time was set to ");
            appendNumber(message, etalonTick.year / 10, FALSE);
            appendNumber(message, etalonTick.year % 10, FALSE);
            appendText(message, L".");
            appendNumber(message, etalonTick.month / 10, FALSE);
            appendNumber(message, etalonTick.month % 10, FALSE);
            appendText(message, L".");
            appendNumber(message, etalonTick.day / 10, FALSE);
            appendNumber(message, etalonTick.day % 10, FALSE);
            appendText(message, L" ");
            appendNumber(message, etalonTick.hour / 10, FALSE);
            appendNumber(message, etalonTick.hour % 10, FALSE);
            appendText(message, L":");
            appendNumber(message, etalonTick.minute / 10, FALSE);
            appendNumber(message, etalonTick.minute % 10, FALSE);
            appendText(message, L":");
            appendNumber(message, etalonTick.second / 10, FALSE);
            appendNumber(message, etalonTick.second % 10, FALSE);
            appendText(message, L".");
            appendNumber(message, etalonTick.millisecond / 100, FALSE);
            appendNumber(message, etalonTick.millisecond % 100 / 10, FALSE);
            appendNumber(message, etalonTick.millisecond % 10, FALSE);
            appendText(message, L".");
            logToConsole(message);

            CHAR16 digestChars[60 + 1];
            getIdentity(etalonTick.saltedSpectrumDigest.m256i_u8, digestChars, true);

            setNumber(message, spectrumInfo.totalAmount, TRUE);
            appendText(message, L" qus in ");
            appendNumber(message, spectrumInfo.numberOfEntities, TRUE);
            appendText(message, L" entities (digest = ");
            appendText(message, digestChars);
            appendText(message, L"); ");
            appendNumber(message, numberOfTransactions, TRUE);
            appendText(message, L" transactions.");
            logToConsole(message);

            setText(message, L"Universe digest = ");
            getIdentity(etalonTick.saltedUniverseDigest.m256i_u8, digestChars, true);
            appendText(message, digestChars);
            appendText(message, L".");
            logToConsole(message);

            setText(message, L"Computer digest = ");
            getIdentity(etalonTick.saltedComputerDigest.m256i_u8, digestChars, true);
            appendText(message, digestChars);
            appendText(message, L".");
            logToConsole(message);

            setText(message, L"TxBody digest = ");
            appendNumber(message, etalonTick.saltedTransactionBodyDigest, false);
            appendText(message, L".");
            logToConsole(message);

            setText(message, L"resourceTestingDigest = ");
            appendNumber(message, resourceTestingDigest, false);
            appendText(message, L".");
            logToConsole(message);

            unsigned int numberOfPublishedSolutions = 0, numberOfRecordedSolutions = 0, numberOfObsoleteSolutions = 0;
            for (unsigned int i = 0; i < system.numberOfSolutions; i++)
            {
                if (solutionPublicationTicks[i])
                {
                    numberOfPublishedSolutions++;

                    if (solutionPublicationTicks[i] == SOLUTION_RECORDED_FLAG)
                    {
                        numberOfRecordedSolutions++;
                    }
                    else if (solutionPublicationTicks[i] == SOLUTION_OBSOLETE_FLAG)
                    {
                        numberOfObsoleteSolutions++;
                    }
                }
            }
            setNumber(message, numberOfRecordedSolutions, TRUE);
            appendText(message, L"/");
            appendNumber(message, numberOfPublishedSolutions, TRUE);
            appendText(message, L"/");
            appendNumber(message, numberOfObsoleteSolutions, TRUE);
            appendText(message, L"/");            
            appendNumber(message, system.numberOfSolutions, TRUE);
            appendText(message, L" solutions.");
            logToConsole(message);

            logHealthStatus();

            setText(message, L"Entity balance dust threshold: ");
            appendNumber(message, (dustThresholdBurnAll > dustThresholdBurnHalf) ? dustThresholdBurnAll : dustThresholdBurnHalf, TRUE);
            logToConsole(message);

#if TICK_STORAGE_AUTOSAVE_MODE
#if TICK_STORAGE_AUTOSAVE_MODE == 2
            setText(message, L"Auto-save disabled, use F8 key to trigger save");
#else
            setText(message, L"Auto-save enabled in AUX mode: every ");
            appendNumber(message, TICK_STORAGE_AUTOSAVE_TICK_PERIOD, FALSE);
            appendText(message, L" ticks, next at tick ");
            appendNumber(message, nextPersistingNodeStateTick, FALSE);
#endif
            logToConsole(message);
#endif

            setText(message, L"Average K12 duration for  ");
#if defined (__AVX512F__) && !GENERIC_K12
            appendText(message, L"AVX512 implementation is ");
#else
            appendText(message, L"Generic implementation is ");
#endif
            appendNumber(message, QPI::div(K12MeasurementsSum, K12MeasurementsCount), TRUE);
            appendText(message, L" ticks.");
            logToConsole(message);

#ifndef NDEBUG
            forceLogToConsoleAsAddDebugMessage = false;
#endif
        }
        break;

        /*
        *
        * F3 Key
        * By Pressing the F3 Key the node will display the current state of the mining race
        * You can see which of your ID's is at which position.
        */
        case 0x0D:
        {
            unsigned int numberOfSolutions = 0;
            for (unsigned int i = 0; i < numberOfMiners; i++)
            {
                numberOfSolutions += minerScores[i];
            }
            setNumber(message, numberOfMiners, TRUE);
            appendText(message, L" miners with ");
            appendNumber(message, numberOfSolutions, TRUE);
            appendText(message, L" solutions (min computor score = ");
            appendNumber(message, minimumComputorScore, TRUE);
            appendText(message, L", min candidate score = ");
            appendNumber(message, minimumCandidateScore, TRUE);
            appendText(message, L").");
            logToConsole(message);

            setText(message, L"CustomMining: ");
            gCustomMiningStats.appendLog(message);
            logToConsole(message);
        }
        break;

        /*
        * F4 Key
        * By Pressing the F4 Key the node will drop all currently active connections.
        * This forces the node to reconnect to known peers and can help to recover stuck situations.
        */
        case 0x0E:
        {
            logToConsole(L"Pressed F4 key");
            for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
            {
                closePeer(&peers[i]);
            }
        }
        break;

        /*
        * F5 Key
        * By Pressing the F5 Key the node will issue new votes for it's COMPUTORS.
        * By issuing new "empty" votes a tick can by bypassed if there is no consensus. (to few computors which voted)
        */
        case 0x0F:
        {
            logToConsole(L"Pressed F5 key");
            forceNextTick = true;
        }
        break;

        /*
        * F6 Key
        * By Pressing the F6 Key the current state of Qubic is saved to the disk.
        * The files generated will be appended by .000
        */
        case 0x10:
        {
            logToConsole(L"Pressed F6 key");
            SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 4] = L'0';
            SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 3] = L'0';
            SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 2] = L'0';
            saveSpectrum();

            UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 4] = L'0';
            UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 3] = L'0';
            UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 2] = L'0';
            saveUniverse();

            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 4] = L'0';
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 3] = L'0';
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 2] = L'0';
            saveComputer();

#ifdef ENABLE_PROFILING
            gProfilingDataCollector.writeToFile();
#endif
        }
        break;

        /*
        * F7 Key
        * Force switching epoch
        * By pressing F7 key, next tick digest will be zeroes to fix a corner case where weak nodes cannot get through new epoch
        * due to missing data. This flag will be reset only when the seamless transition procedure happen.
        */
        case 0x11:
        {
            logToConsole(L"Pressed F7 key");
            forceSwitchEpoch = true;
        }
        break;

        /*
        * F8 Key
        * Takes a snapshot of tick storage and save it to disk
        */
        case 0x12:
        {
            logToConsole(L"Pressed F8 key");
            requestPersistingNodeState = 1;
        }
        break;

        /*
        * F9 Key
        * By Pressing the F9 Key the latestCreatedTick got's decreased by one.
        * By decreasing this by one, the Node will resend the issued votes for its Computors.
        */
        case 0x13:
        {
            logToConsole(L"Pressed F9 key");
            if (system.latestCreatedTick > 0) system.latestCreatedTick--;
        }
        break;

        /*
        * F10 Key
        * By Pressing the F10 Key epochTransitionCleanMemoryFlag is set to 1
        * Allowing the node to clean memory and continue switching to new epoch
        */
        case 0x14:
        {
            logToConsole(L"Pressed F10 key: epochTransitionCleanMemoryFlag => 1");
            epochTransitionCleanMemoryFlag = 1;
        }
        break;

        /*
        * F11 Key
        * By Pressing the F11 Key the node can switch between static and dynamic network mode
        * static: incoming connections are blocked and peer list will not be altered
        * dynamic: all connections are open, peers are added and removed dynamically
        */
        case 0x15:
        {
            listOfPeersIsStatic = !listOfPeersIsStatic;
        }
        break;

        /*
        * F12 Key
        * By Pressing the F12 Key the node can switch between aux&aux, aux&MAIN, MAIN&aux, and MAIN&MAIN mode.
        * MAIN: the node is issuing ticks and participate as "COMPUTOR" in the network
        * aux: the node is running without participating active as "COMPUTOR" in the network
        * Upon epoch change aux&aux, and MAIN&MAIN statuses are retained, aux&MAIN, and MAIN&aux are reverted.
        * !! IMPORTANT !! only one MAIN instance per COMPUTOR is allowed.
        */
        case 0x16:
        {
            if (requestPersistingNodeState == 1 || persistingNodeStateTickProcWaiting == 1)
            {
                logToConsole(L"Unable to switch mode because node is saving states.");
            }
            else
            {
                mainAuxStatus = (mainAuxStatus + 1) & 3;
                setText(message, (isMainMode()) ? L"MAIN" : L"aux");
                appendText(message, L"&");
                appendText(message, (mainAuxStatus & 2) ? L"MAIN" : L"aux");
                logToConsole(message);
            }
        }
        break;

        /*
        * ESC Key
        * By Pressing the ESC Key the node will stop
        */
        case 0x17:
        {
            shutDownNode = 1;
        }
        break;

        /*
        * PAUSE key
        * By pressing the PAUSE key you can cycle through the log output verbosity level
        */
        case 0x48:
        {
            consoleLoggingLevel = (consoleLoggingLevel + 1) % 3;
        }
        break;
        }
    }
}

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    ih = imageHandle;
    st = systemTable;
    rs = st->RuntimeServices;
    bs = st->BootServices;

    bs->SetWatchdogTimer(0, 0, 0, NULL);

    initTime();

    st->ConOut->ClearScreen(st->ConOut);
    setText(message, L"Qubic ");
    appendQubicVersion(message);
    appendText(message, L" is launched.");
    logToConsole(message);

    if (initialize())
    {
        logToConsole(L"Setting up multiprocessing ...");

        unsigned int computingProcessorNumber;
        EFI_GUID mpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
        bs->LocateProtocol(&mpServiceProtocolGuid, NULL, (void**)&mpServicesProtocol);
        unsigned long long numberOfAllProcessors, numberOfEnabledProcessors;
        mpServicesProtocol->GetNumberOfProcessors(mpServicesProtocol, &numberOfAllProcessors, &numberOfEnabledProcessors);
        mpServicesProtocol->WhoAmI(mpServicesProtocol, &mainThreadProcessorID); // get the proc Id of main thread (for later use)

        registerAsynFileIO(mpServicesProtocol);
        
        // Initialize resource management
        // ASSUMPTION: - each processor (CPU core) is bound to different functional thread.
        //             - there are potentially 2+ tick processors in the future
        // procId is guaranteed lower than MAX_NUMBER_OF_PROCESSORS (https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/MpService.h#L615)
        // First part: tick processors always process solutions         
        nTickProcessorIDs = 0;
        nRequestProcessorIDs = 0;
        nContractProcessorIDs = 0;
        nSolutionProcessorIDs = 0;
        
        for (int i = 0; i < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            solutionProcessorFlags[i] = false;
        }

        for (unsigned int i = 0; i < numberOfAllProcessors && numberOfProcessors < MAX_NUMBER_OF_PROCESSORS; i++)
        {
            EFI_PROCESSOR_INFORMATION processorInformation;
            mpServicesProtocol->GetProcessorInfo(mpServicesProtocol, i, &processorInformation);
            if (processorInformation.StatusFlag == (PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT))
            {
                if (!allocPoolWithErrorLog(L"processor[i]", BUFFER_SIZE, &processors[numberOfProcessors].buffer, __LINE__))
                {
                    numberOfProcessors = 0;

                    break;
                }
                if (!processors[numberOfProcessors].alloc(STACK_SIZE))
                {
                    logToConsole(L"Failed to allocate stack for processor!");
                    numberOfProcessors = 0;
                    break;
                }

                if (numberOfProcessors == 2)
                {
                    processors[numberOfProcessors].type = Processor::ContractProcessor;
                    processors[numberOfProcessors].setupFunction(contractProcessor, 0);
                    computingProcessorNumber = numberOfProcessors;
                    contractProcessorIDs[nContractProcessorIDs++] = i;
                }
                else
                {
                    if (numberOfProcessors == 1)
                    {
                        processors[numberOfProcessors].type = Processor::TickProcessor;
                        processors[numberOfProcessors].setupFunction(tickProcessor, &processors[numberOfProcessors]);
                        tickProcessorIDs[nTickProcessorIDs++] = i;
                    }
                    else
                    {
                        processors[numberOfProcessors].type = Processor::RequestProcessor;
                        processors[numberOfProcessors].setupFunction(requestProcessor, &processors[numberOfProcessors]);
                        requestProcessorIDs[nRequestProcessorIDs++] = i;
                    }

                    createEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, shutdownCallback, NULL, &processors[numberOfProcessors].event);
                    mpServicesProtocol->StartupThisAP(mpServicesProtocol, Processor::runFunction, i, processors[numberOfProcessors].event, 0, &processors[numberOfProcessors], NULL);

                    #if !defined(NDEBUG)
                    debugLogOnlyMainProcessorRunning = false;
                    #endif

                    if (!solutionProcessorFlags[i % NUMBER_OF_SOLUTION_PROCESSORS]
                        && !solutionProcessorFlags[i])
                    {
                        solutionProcessorFlags[i % NUMBER_OF_SOLUTION_PROCESSORS] = true;
                        solutionProcessorFlags[i] = true;
                        solutionProcessorIDs[nSolutionProcessorIDs++] = i;
                    }
                }
                numberOfProcessors++;
            }
        }
        if (numberOfProcessors < 3)
        {
            logToConsole(L"At least 4 healthy enabled processors are required! Exiting...");
        }
        else
        {
            setNumber(message, 1 + numberOfProcessors, TRUE);
            appendText(message, L"/");
            appendNumber(message, numberOfAllProcessors, TRUE);
            appendText(message, L" processors are being used.");
            logToConsole(message);

            setText(message, L"Main: Processor #");
            appendNumber(message, mainThreadProcessorID, false);
            logToConsole(message);

            setText(message, L"Tick processors: ");
            for (int i = 0; i < nTickProcessorIDs; i++)
            {
                appendText(message, L"Processor #");
                appendNumber(message, tickProcessorIDs[i], false);
                if (i != nTickProcessorIDs -1) appendText(message, L" | ");
            }
            logToConsole(message);

            setText(message, L"Request processors: ");
            for (int i = 0; i < nRequestProcessorIDs; i++)
            {
                appendText(message, L"Processor #");
                appendNumber(message, requestProcessorIDs[i], false);
                if (i != nRequestProcessorIDs - 1) appendText(message, L" | ");
            }
            logToConsole(message);

            setText(message, L"Solution processors: ");
            for (int i = 0; i < nSolutionProcessorIDs; i++)
            {
                appendText(message, L"Processor #");
                appendNumber(message, solutionProcessorIDs[i], false);
                if (i != nSolutionProcessorIDs - 1) appendText(message, L" | ");
            }
            logToConsole(message);

            if (NUMBER_OF_SOLUTION_PROCESSORS * 2 > numberOfProcessors)
            {
                logToConsole(L"WARNING: NUMBER_OF_SOLUTION_PROCESSORS should not be greater than half of the total processor number!");
            }

            // -----------------------------------------------------
            // Main loop
            unsigned int salt;
            _rdrand32_step(&salt);

#if TICK_STORAGE_AUTOSAVE_MODE == 1
            // Use random tick offset to reduce risk of several nodes doing auto-save in parallel (which can lead to bad topology and misalignment)
            nextPersistingNodeStateTick = system.tick + random(TICK_STORAGE_AUTOSAVE_TICK_PERIOD) + TICK_STORAGE_AUTOSAVE_TICK_PERIOD / 10;
#endif
            
            unsigned long long clockTick = 0, systemDataSavingTick = 0, loggingTick = 0, peerRefreshingTick = 0, tickRequestingTick = 0;
            unsigned int tickRequestingIndicator = 0, futureTickRequestingIndicator = 0;
            autoResendTickVotes.lastTick = system.initialTick;
            autoResendTickVotes.lastCheck = __rdtsc();
            logToConsole(L"Init complete! Entering main loop ...");
            while (!shutDownNode)
            {
                if (criticalSituation == 1)
                {
                    logToConsole(L"CRITICAL SITUATION #1!!!");
                }
                if (ts.nextTickTransactionOffset + MAX_TRANSACTION_SIZE > ts.tickTransactions.storageSpaceCurrentEpoch)
                {
                    logToConsole(L"Transaction storage is full!!!");
                }

                const unsigned long long curTimeTick = __rdtsc();

                if (curTimeTick - clockTick >= (frequency >> 1))
                {
                    clockTick = curTimeTick;

                    PROFILE_NAMED_SCOPE("main loop: updateTime()");
                    updateTime();
                }

                if (contractProcessorState == 1)
                {
                    contractProcessorState = 2;
                    createEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, contractProcessorShutdownCallback, NULL, &contractProcessorEvent);
                    mpServicesProtocol->StartupThisAP(mpServicesProtocol, Processor::runFunction, contractProcessorIDs[0], contractProcessorEvent, MAX_CONTRACT_ITERATION_DURATION * 1000, &processors[computingProcessorNumber], NULL);
                }
                /*if (!computationProcessorState && (computation || __computation))
                {
                    numberOfAllSCs++;
                    computationProcessorState = 1;
                    createEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, shutdownCallback, NULL, &computationProcessorEvent);
                    if (status = mpServicesProtocol->StartupThisAP(mpServicesProtocol, computationProcessor, computingProcessorNumber, computationProcessorEvent, MAX_CONTRACT_ITERATION_DURATION * 1000, NULL, NULL))
                    {
                        numberOfNonLaunchedSCs++;
                        logStatusToConsole(L"EFI_MP_SERVICES_PROTOCOL.StartupThisAP() fails", status, __LINE__);
                    }
                }*/
                peerTcp4Protocol->Poll(peerTcp4Protocol);

                for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
                {
                    // handle new connections
                    if (peerConnectionNewlyEstablished(i))
                    {
                        // new connection established:
                        // prepare and send ExchangePublicPeers message
                        ExchangePublicPeers* request = (ExchangePublicPeers*)&peers[i].dataToTransmit[sizeof(RequestResponseHeader)];
                        bool noVerifiedPublicPeers = true;
                        // Only check non-private peers for handshake status
                        for (unsigned int k = NUMBER_OF_PRIVATE_IP; k < numberOfPublicPeers; k++)
                        {
                            if (publicPeers[k].isHandshaked /*&& publicPeers[k].isFullnode*/)
                            {
                                noVerifiedPublicPeers = false;

                                break;
                            }
                        }
                        for (unsigned int j = 0; j < NUMBER_OF_EXCHANGED_PEERS; j++)
                        {
                            if (noVerifiedPublicPeers)
                            {
                                // no verified public peers -> send 0.0.0.0
                                request->peers[j].u32 = 0;
                            }
                            else
                            {
                                if (NUMBER_OF_PRIVATE_IP < numberOfPublicPeers)
                                {
                                    // randomly select verified public peers and discard private IPs
                                    // first NUMBER_OF_PRIVATE_IP ips are same on both array publicPeers and knownPublicPeers
                                    const unsigned int publicPeerIndex = NUMBER_OF_PRIVATE_IP + random(numberOfPublicPeers - NUMBER_OF_PRIVATE_IP);
                                    // share the peer if it's not our private IPs and is handshaked
                                    if (publicPeers[publicPeerIndex].isHandshaked)
                                    {
                                        request->peers[j] = publicPeers[publicPeerIndex].address;
                                    }
                                    else
                                    {
                                        j--;
                                    }
                                }
                                else
                                {
                                    request->peers[j].u32 = 0;
                                }
                            }
                        }

                        RequestResponseHeader* requestHeader = (RequestResponseHeader*)peers[i].dataToTransmit;
                        requestHeader->setSize<sizeof(RequestResponseHeader) + sizeof(ExchangePublicPeers)>();
                        requestHeader->randomizeDejavu();
                        requestHeader->setType(ExchangePublicPeers::type);
                        peers[i].dataToTransmitSize = requestHeader->size();
                        _InterlockedIncrement64(&numberOfDisseminatedRequests);

                        // send RequestComputors message at beginning of epoch
                        if (!broadcastedComputors.computors.epoch
                            || broadcastedComputors.computors.epoch != system.epoch)
                        {
                            requestedComputors.header.randomizeDejavu();
                            copyMem(&peers[i].dataToTransmit[peers[i].dataToTransmitSize], &requestedComputors, requestedComputors.header.size());
                            peers[i].dataToTransmitSize += requestedComputors.header.size();
                            _InterlockedIncrement64(&numberOfDisseminatedRequests);
                        }
                    }

                    // receive and transmit on active connections
                    peerReceiveAndTransmit(i, salt);

                    // reconnect if this peer slot has no active connection
                    peerReconnectIfInactive(i, PORT);
                }

#if !TICK_STORAGE_AUTOSAVE_MODE
                // Only save system + score cache to file regularly here if on AUX and snapshot auto-save is disabled
                if ((!isMainMode())
                    && curTimeTick - systemDataSavingTick >= SYSTEM_DATA_SAVING_PERIOD * frequency / 1000)
                {
                    systemDataSavingTick = curTimeTick;

                    saveSystem();
                    score->saveScoreCache(system.epoch);
                    saveCustomMiningCache(system.epoch);
                }
#endif
                tryResendTickVotes();

                if (curTimeTick - peerRefreshingTick >= PEER_REFRESHING_PERIOD * frequency / 1000)
                {
                    peerRefreshingTick = curTimeTick;

                    unsigned short suitablePeerIndices[NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS];
                    setMem(suitablePeerIndices, sizeof(suitablePeerIndices), 0);
                    unsigned short numberOfSuitablePeers = 0;
                    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
                    {
                        if (peers[i].tcp4Protocol && peers[i].isConnectedAccepted && !peers[i].isClosing)
                        {
                            if (!peers[i].isFullNode())
                            {
                                suitablePeerIndices[numberOfSuitablePeers++] = i;
                            }
                        }
                    }

                    // disconnect 25% of current connections that are not **active fullnode**
                    for (unsigned short i = 0; i < numberOfSuitablePeers / 4; i++)
                    {
                        closePeer(&peers[suitablePeerIndices[random(numberOfSuitablePeers)]]);
                    }
                    logToConsole(L"Refreshed connection...");
                }

                if (curTimeTick - tickRequestingTick >= TICK_REQUESTING_PERIOD * frequency / 1000
                    && ts.tickInCurrentEpochStorage(system.tick + 1)
                    && !epochTransitionState)
                {
                    // Request ticks
                    tickRequestingTick = curTimeTick;
#if TICK_STORAGE_AUTOSAVE_MODE
                    const bool isNewTick = system.tick >= ts.getPreloadTick();
                    const bool isNewTickPlus1 = system.tick + 1 >= ts.getPreloadTick();
                    const bool isNewTickPlus2 = system.tick + 2 >= ts.getPreloadTick();
#else
                    const bool isNewTick = true;
                    const bool isNewTickPlus1 = true;
                    const bool isNewTickPlus2 = true;
#endif
                    if (tickRequestingIndicator == gTickTotalNumberOfComputors
                        && isNewTick)
                    {
                        requestedQuorumTick.header.randomizeDejavu();
                        requestedQuorumTick.requestQuorumTick.quorumTick.tick = system.tick;
                        setMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const Tick* tsCompTicks = ts.ticks.getByTickInCurrentEpoch(system.tick);
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            if (tsCompTicks[i].epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
                        pushToAnyFullNode(&requestedQuorumTick.header);
                    }
                    tickRequestingIndicator = gTickTotalNumberOfComputors;
                    if (futureTickRequestingIndicator == gFutureTickTotalNumberOfComputors
                        && isNewTickPlus1)
                    {
                        requestedQuorumTick.header.randomizeDejavu();
                        requestedQuorumTick.requestQuorumTick.quorumTick.tick = system.tick + 1;
                        setMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const Tick* tsCompTicks = ts.ticks.getByTickInCurrentEpoch(system.tick + 1);
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            if (tsCompTicks[i].epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
                        pushToAnyFullNode(&requestedQuorumTick.header);
                    }
                    futureTickRequestingIndicator = gFutureTickTotalNumberOfComputors;

                    if ((ts.tickData[system.tick + 1 - system.initialTick].epoch != system.epoch
                        || targetNextTickDataDigestIsKnown)
                        && isNewTickPlus1)
                    {
                        // Request tick data of next tick when it is not stored yet or should be updated,
                        // for example because next tick data digest of the quorum differs from the one of this node.
                        // targetNextTickDataDigestIsKnown == true signals that we need to fetch TickData
                        // targetNextTickDataDigestIsKnown == false means there is no consensus on next tick data yet
                        requestedTickData.header.randomizeDejavu();
                        requestedTickData.requestTickData.requestedTickData.tick = system.tick + 1;
                        pushToAny(&requestedTickData.header);
                        pushToAnyFullNode(&requestedTickData.header);
                    }
                    if (ts.tickData[system.tick + 2 - system.initialTick].epoch != system.epoch && isNewTickPlus2)
                    {
                        requestedTickData.header.randomizeDejavu();
                        requestedTickData.requestTickData.requestedTickData.tick = system.tick + 2;
                        pushToAny(&requestedTickData.header);
                        pushToAnyFullNode(&requestedTickData.header);
                    }

                    if (requestedTickTransactions.requestedTickTransactions.tick)
                    {
                        requestedTickTransactions.header.randomizeDejavu();
                        pushToAny(&requestedTickTransactions.header);
                        pushToAnyFullNode(&requestedTickTransactions.header);

                        requestedTickTransactions.requestedTickTransactions.tick = 0;
                    }
                }

                // Add messages from response queue to sending buffer
                const unsigned short responseQueueElementHead = ::responseQueueElementHead;
                if (responseQueueElementTail != responseQueueElementHead)
                {
                    while (responseQueueElementTail != responseQueueElementHead)
                    {
                        RequestResponseHeader* responseHeader = (RequestResponseHeader*)&responseQueueBuffer[responseQueueElements[responseQueueElementTail].offset];
                        if (responseQueueElements[responseQueueElementTail].peer)
                        {
                            push(responseQueueElements[responseQueueElementTail].peer, responseHeader);
                        }
                        else
                        {
                            pushToSeveral(responseHeader);
                        }
                        responseQueueBufferTail += responseHeader->size();
                        if (responseQueueBufferTail > RESPONSE_QUEUE_BUFFER_SIZE - BUFFER_SIZE)
                        {
                            responseQueueBufferTail = 0;
                        }
                        // TODO: Place a fence
                        responseQueueElementTail++;
                    }
                }

                if (systemMustBeSaved)
                {
                    systemDataSavingTick = curTimeTick; // set last save tick to avoid overwrite in main loop
                    saveSystem();
                    systemMustBeSaved = false;
                }
                if (spectrumMustBeSaved)
                {
                    saveSpectrum();
                    spectrumMustBeSaved = false;
                }
                if (universeMustBeSaved)
                {
                    saveUniverse();
                    universeMustBeSaved = false;
                }
                if (computerMustBeSaved)
                {
                    saveComputer();
                    computerMustBeSaved = false;
                }

                if (forceRefreshPeerList)
                {
                    forceRefreshPeerList = false;
                    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
                    {
                        closePeer(&peers[i]);
                    }
                }

                processKeyPresses();

#if TICK_STORAGE_AUTOSAVE_MODE
#if TICK_STORAGE_AUTOSAVE_MODE == 1
                bool nextAutoSaveTickUpdated = false;
                if (isMainMode())
                {
                    // MAIN mode: update auto-save schedule (only run save when switched to AUX mode)
                    while (system.tick >= nextPersistingNodeStateTick)
                    {
                        nextPersistingNodeStateTick += TICK_STORAGE_AUTOSAVE_TICK_PERIOD;
                        nextAutoSaveTickUpdated = true;
                    }
                }
                else
                {
                    // AUX mode
                    if (system.tick > ts.getPreloadTick()) // check the last saved tick
                    {
                        // Start auto save if nextAutoSaveTick == system.tick (or if the main loop has missed nextAutoSaveTick)
                        if (system.tick >= nextPersistingNodeStateTick)
                        {
                            requestPersistingNodeState = 1;
                            while (system.tick >= nextPersistingNodeStateTick)
                            {
                                nextPersistingNodeStateTick += TICK_STORAGE_AUTOSAVE_TICK_PERIOD;
                            }
                            nextAutoSaveTickUpdated = true;
                        }
                    }
                }
#endif
                if (requestPersistingNodeState == 1 && persistingNodeStateTickProcWaiting == 1)
                {
                    // Saving node state takes a lot of time -> Close peer connections before to signal that
                    // the peers should connect to another node.
                    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
                    {
                        closePeer(&peers[i]);
                    }

                    logToConsole(L"Saving node state...");
                    saveAllNodeStates();
#ifdef ENABLE_PROFILING
                    gProfilingDataCollector.writeToFile();
#endif
                    requestPersistingNodeState = 0;
                    logToConsole(L"Complete saving all node states");
                }
#if TICK_STORAGE_AUTOSAVE_MODE == 1
                if (nextAutoSaveTickUpdated)
                {
                    setText(message, L"Auto-save in AUX mode scheduled for tick ");
                    appendNumber(message, nextPersistingNodeStateTick, FALSE);
                    logToConsole(message);
                }
#endif
#endif

                if (curTimeTick - loggingTick >= frequency)
                {
                    loggingTick = curTimeTick;

                    logInfo();

                    if (mainLoopDenominator)
                    {
                        setText(message, L"Main loop duration = ");
                        appendNumber(message, (mainLoopNumerator / mainLoopDenominator) * 1000000 / frequency, TRUE);
                        appendText(message, L" mcs.");
                        logToConsole(message);
                    }
                    mainLoopNumerator = 0;
                    mainLoopDenominator = 0;

                    if (tickerLoopDenominator)
                    {
                        setText(message, L"Ticker loop duration = ");
                        appendNumber(message, (tickerLoopNumerator / tickerLoopDenominator) * 1000000 / frequency, TRUE);
                        appendText(message, L" microseconds. Latest created tick = ");
                        appendNumber(message, system.latestCreatedTick, TRUE);
                        appendText(message, L".");
                        logToConsole(message);
                    }
                    tickerLoopNumerator = 0;
                    tickerLoopDenominator = 0;

                    // output if misalignment happened
                    if (gTickTotalNumberOfComputors - gTickNumberOfComputors >= QUORUM && numberOfKnownNextTickTransactions == numberOfNextTickTransactions)
                    {
                        if (misalignedState == 0)
                        {
                            // misaligned state detected the first time
                            misalignedState = 1;
                        }
                        else if (misalignedState == 1)
                        {
                            // state persisting for at least a second -> also log to debug.log
                            misalignedState = 2;
                        }
                        logToConsole(L"MISALIGNED STATE DETECTED");
                        if (misalignedState == 2)
                        {
                            // print health status and stop repeated logging to debug.log
                            logHealthStatus();
                            misalignedState = 3;
                        }
                    }
                    else
                    {
                        misalignedState = 0;
                    }

#if PAUSE_BEFORE_CLEAR_MEMORY
                    if (epochTransitionCleanMemoryFlag == 0)
                    {
                        logToConsole(L"Please press F10 to clear all memory on RAM and continue epoch transition procedure!");
                    }
#endif

#if !defined(NDEBUG)
                    if (system.tick % 1000 == 0)
                    {
                        logHealthStatus();
                    }
#endif
                }
                else
                {
                    mainLoopNumerator += __rdtsc() - curTimeTick;
                    mainLoopDenominator++;
                }

#if !defined(NDEBUG)
                printDebugMessages();
#endif
                // Flush the file system. Only flush one item at a time to avoid the main loop stay too long
                // Even if the time is not satisfied, when still flush at least some items to make sure the save/load not stuck forever
                // TODO: profile the read/write speed of the file system at the begining and adjust the number of items to flush
                int remainedItem = 1;
                do
                {
                    remainedItem = flushAsyncFileIOBuffer(1);
                }
                while (remainedItem > 0 && ((__rdtsc() - curTimeTick) * 1000000 / frequency < TARGET_MAINTHREAD_LOOP_DURATION));
            }

            saveSystem();
            score->saveScoreCache(system.epoch);
            saveCustomMiningCache(system.epoch);
#ifdef ENABLE_PROFILING
            gProfilingDataCollector.writeToFile();
#endif

            setText(message, L"Qubic ");
            appendQubicVersion(message);
            appendText(message, L" is shut down.");
            logToConsole(message);
        }
    }
    else
    {
        logToConsole(L"Initialization fails!");
    }

    deinitialize();

    bs->Stall(1000000);
    if (!shutDownNode)
    {
        st->ConIn->Reset(st->ConIn, FALSE);
        unsigned long long eventIndex;
        bs->WaitForEvent(1, &st->ConIn->WaitForKey, &eventIndex);
    }

    return EFI_SUCCESS;
}
