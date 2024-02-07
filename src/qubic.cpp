#define SINGLE_COMPILE_UNIT

// contract_def.h needs to be included first to make sure that contracts have minimal access
#include "contract_core/contract_def.h"
#include "contract_core/contract_exec.h"

#include <intrin.h>

#include "network_messages/all.h"

#include "private_settings.h"
#include "public_settings.h"



////////// C++ helpers \\\\\\\\\\

#include "platform/m256.h"
#include "platform/concurrency.h"
// TODO: Use "long long" instead of "int" for DB indices


#include "platform/uefi.h"
#include "platform/time.h"
#include "platform/file_io.h"
#include "platform/time_stamp_counter.h"

#include "platform/custom_stack.h"

#include "text_output.h"

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

#include "spectrum.h"
#include "contract_core/qpi_spectrum_impl.h"

#include "logging/logging.h"
#include "logging/net_msg_impl.h"

#include "tick_storage.h"
#include "vote_counter.h"

#include "addons/tx_status_request.h"

#include "files/files.h"
#include "mining/mining.h"
#include "oracles/oracle_machines.h"

////////// Qubic \\\\\\\\\\

#define CONTRACT_STATES_DEPTH 10 // Is derived from MAX_NUMBER_OF_CONTRACTS (=N)
#define TICK_REQUESTING_PERIOD 500ULL
#define MAX_NUMBER_EPOCH 1000ULL
#define INVALIDATED_TICK_DATA (MAX_NUMBER_EPOCH+1)
#define MAX_NUMBER_OF_MINERS 8192
#define NUMBER_OF_MINER_SOLUTION_FLAGS 0x100000000
#define MAX_MESSAGE_PAYLOAD_SIZE MAX_TRANSACTION_SIZE
#define MAX_UNIVERSE_SIZE 1073741824
#define MESSAGE_DISSEMINATION_THRESHOLD 1000000000
#define PEER_REFRESHING_PERIOD 120000ULL
#define PORT 31841
#define SYSTEM_DATA_SAVING_PERIOD 300000ULL
#define TICK_TRANSACTIONS_PUBLICATION_OFFSET 2 // Must be only 2
#define TICK_VOTE_COUNTER_PUBLICATION_OFFSET 4 // Must be at least 3+: 1+ for tx propagration + 1 for tickData propagration + 1 for vote propagration
#define MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET 3 // Must be 3+
#define TIME_ACCURACY 5000


struct Processor : public CustomStack
{
    enum Type { Unused = 0, RequestProcessor, TickProcessor, ContractProcessor };
    Type type;
    EFI_EVENT event;
    Peer* peer;
    void* buffer;
};




static const unsigned short revenuePoints[1 + 1024] = { 0, 710, 1125, 1420, 1648, 1835, 1993, 2129, 2250, 2358, 2455, 2545, 2627, 2702, 2773, 2839, 2901, 2960, 3015, 3068, 3118, 3165, 3211, 3254, 3296, 3336, 3375, 3412, 3448, 3483, 3516, 3549, 3580, 3611, 3641, 3670, 3698, 3725, 3751, 3777, 3803, 3827, 3851, 3875, 3898, 3921, 3943, 3964, 3985, 4006, 4026, 4046, 4066, 4085, 4104, 4122, 4140, 4158, 4175, 4193, 4210, 4226, 4243, 4259, 4275, 4290, 4306, 4321, 4336, 4350, 4365, 4379, 4393, 4407, 4421, 4435, 4448, 4461, 4474, 4487, 4500, 4512, 4525, 4537, 4549, 4561, 4573, 4585, 4596, 4608, 4619, 4630, 4641, 4652, 4663, 4674, 4685, 4695, 4705, 4716, 4726, 4736, 4746, 4756, 4766, 4775, 4785, 4795, 4804, 4813, 4823, 4832, 4841, 4850, 4859, 4868, 4876, 4885, 4894, 4902, 4911, 4919, 4928, 4936, 4944, 4952, 4960, 4968, 4976, 4984, 4992, 5000, 5008, 5015, 5023, 5031, 5038, 5046, 5053, 5060, 5068, 5075, 5082, 5089, 5096, 5103, 5110, 5117, 5124, 5131, 5138, 5144, 5151, 5158, 5164, 5171, 5178, 5184, 5191, 5197, 5203, 5210, 5216, 5222, 5228, 5235, 5241, 5247, 5253, 5259, 5265, 5271, 5277, 5283, 5289, 5295, 5300, 5306, 5312, 5318, 5323, 5329, 5335, 5340, 5346, 5351, 5357, 5362, 5368, 5373, 5378, 5384, 5389, 5394, 5400, 5405, 5410, 5415, 5420, 5425, 5431, 5436, 5441, 5446, 5451, 5456, 5461, 5466, 5471, 5475, 5480, 5485, 5490, 5495, 5500, 5504, 5509, 5514, 5518, 5523, 5528, 5532, 5537, 5542, 5546, 5551, 5555, 5560, 5564, 5569, 5573, 5577, 5582, 5586, 5591, 5595, 5599, 5604, 5608, 5612, 5616, 5621, 5625, 5629, 5633, 5637, 5642, 5646, 5650, 5654, 5658, 5662, 5666, 5670, 5674, 5678, 5682, 5686, 5690, 5694, 5698, 5702, 5706, 5710, 5714, 5718, 5721, 5725, 5729, 5733, 5737, 5740, 5744, 5748, 5752, 5755, 5759, 5763, 5766, 5770, 5774, 5777, 5781, 5785, 5788, 5792, 5795, 5799, 5802, 5806, 5809, 5813, 5816, 5820, 5823, 5827, 5830, 5834, 5837, 5841, 5844, 5847, 5851, 5854, 5858, 5861, 5864, 5868, 5871, 5874, 5878, 5881, 5884, 5887, 5891, 5894, 5897, 5900, 5904, 5907, 5910, 5913, 5916, 5919, 5923, 5926, 5929, 5932, 5935, 5938, 5941, 5944, 5948, 5951, 5954, 5957, 5960, 5963, 5966, 5969, 5972, 5975, 5978, 5981, 5984, 5987, 5990, 5993, 5996, 5999, 6001, 6004, 6007, 6010, 6013, 6016, 6019, 6022, 6025, 6027, 6030, 6033, 6036, 6039, 6041, 6044, 6047, 6050, 6053, 6055, 6058, 6061, 6064, 6066, 6069, 6072, 6075, 6077, 6080, 6083, 6085, 6088, 6091, 6093, 6096, 6099, 6101, 6104, 6107, 6109, 6112, 6115, 6117, 6120, 6122, 6125, 6128, 6130, 6133, 6135, 6138, 6140, 6143, 6145, 6148, 6151, 6153, 6156, 6158, 6161, 6163, 6166, 6168, 6170, 6173, 6175, 6178, 6180, 6183, 6185, 6188, 6190, 6193, 6195, 6197, 6200, 6202, 6205, 6207, 6209, 6212, 6214, 6216, 6219, 6221, 6224, 6226, 6228, 6231, 6233, 6235, 6238, 6240, 6242, 6244, 6247, 6249, 6251, 6254, 6256, 6258, 6260, 6263, 6265, 6267, 6269, 6272, 6274, 6276, 6278, 6281, 6283, 6285, 6287, 6289, 6292, 6294, 6296, 6298, 6300, 6303, 6305, 6307, 6309, 6311, 6313, 6316, 6318, 6320, 6322, 6324, 6326, 6328, 6330, 6333, 6335, 6337, 6339, 6341, 6343, 6345, 6347, 6349, 6351, 6353, 6356, 6358, 6360, 6362, 6364, 6366, 6368, 6370, 6372, 6374, 6376, 6378, 6380, 6382, 6384, 6386, 6388, 6390, 6392, 6394, 6396, 6398, 6400, 6402, 6404, 6406, 6408, 6410, 6412, 6414, 6416, 6418, 6420, 6421, 6423, 6425, 6427, 6429, 6431, 6433, 6435, 6437, 6439, 6441, 6443, 6444, 6446, 6448, 6450, 6452, 6454, 6456, 6458, 6459, 6461, 6463, 6465, 6467, 6469, 6471, 6472, 6474, 6476, 6478, 6480, 6482, 6483, 6485, 6487, 6489, 6491, 6493, 6494, 6496, 6498, 6500, 6502, 6503, 6505, 6507, 6509, 6510, 6512, 6514, 6516, 6518, 6519, 6521, 6523, 6525, 6526, 6528, 6530, 6532, 6533, 6535, 6537, 6538, 6540, 6542, 6544, 6545, 6547, 6549, 6550, 6552, 6554, 6556, 6557, 6559, 6561, 6562, 6564, 6566, 6567, 6569, 6571, 6572, 6574, 6576, 6577, 6579, 6581, 6582, 6584, 6586, 6587, 6589, 6591, 6592, 6594, 6596, 6597, 6599, 6600, 6602, 6604, 6605, 6607, 6609, 6610, 6612, 6613, 6615, 6617, 6618, 6620, 6621, 6623, 6625, 6626, 6628, 6629, 6631, 6632, 6634, 6636, 6637, 6639, 6640, 6642, 6643, 6645, 6647, 6648, 6650, 6651, 6653, 6654, 6656, 6657, 6659, 6660, 6662, 6663, 6665, 6667, 6668, 6670, 6671, 6673, 6674, 6676, 6677, 6679, 6680, 6682, 6683, 6685, 6686, 6688, 6689, 6691, 6692, 6694, 6695, 6697, 6698, 6699, 6701, 6702, 6704, 6705, 6707, 6708, 6710, 6711, 6713, 6714, 6716, 6717, 6718, 6720, 6721, 6723, 6724, 6726, 6727, 6729, 6730, 6731, 6733, 6734, 6736, 6737, 6739, 6740, 6741, 6743, 6744, 6746, 6747, 6748, 6750, 6751, 6753, 6754, 6755, 6757, 6758, 6760, 6761, 6762, 6764, 6765, 6767, 6768, 6769, 6771, 6772, 6773, 6775, 6776, 6778, 6779, 6780, 6782, 6783, 6784, 6786, 6787, 6788, 6790, 6791, 6793, 6794, 6795, 6797, 6798, 6799, 6801, 6802, 6803, 6805, 6806, 6807, 6809, 6810, 6811, 6813, 6814, 6815, 6816, 6818, 6819, 6820, 6822, 6823, 6824, 6826, 6827, 6828, 6830, 6831, 6832, 6833, 6835, 6836, 6837, 6839, 6840, 6841, 6842, 6844, 6845, 6846, 6848, 6849, 6850, 6851, 6853, 6854, 6855, 6856, 6858, 6859, 6860, 6862, 6863, 6864, 6865, 6867, 6868, 6869, 6870, 6872, 6873, 6874, 6875, 6877, 6878, 6879, 6880, 6882, 6883, 6884, 6885, 6886, 6888, 6889, 6890, 6891, 6893, 6894, 6895, 6896, 6897, 6899, 6900, 6901, 6902, 6904, 6905, 6906, 6907, 6908, 6910, 6911, 6912, 6913, 6914, 6916, 6917, 6918, 6919, 6920, 6921, 6923, 6924, 6925, 6926, 6927, 6929, 6930, 6931, 6932, 6933, 6934, 6936, 6937, 6938, 6939, 6940, 6941, 6943, 6944, 6945, 6946, 6947, 6948, 6950, 6951, 6952, 6953, 6954, 6955, 6957, 6958, 6959, 6960, 6961, 6962, 6963, 6965, 6966, 6967, 6968, 6969, 6970, 6971, 6972, 6974, 6975, 6976, 6977, 6978, 6979, 6980, 6981, 6983, 6984, 6985, 6986, 6987, 6988, 6989, 6990, 6991, 6993, 6994, 6995, 6996, 6997, 6998, 6999, 7000, 7001, 7003, 7004, 7005, 7006, 7007, 7008, 7009, 7010, 7011, 7012, 7013, 7015, 7016, 7017, 7018, 7019, 7020, 7021, 7022, 7023, 7024, 7025, 7026, 7027, 7029, 7030, 7031, 7032, 7033, 7034, 7035, 7036, 7037, 7038, 7039, 7040, 7041, 7042, 7043, 7044, 7046, 7047, 7048, 7049, 7050, 7051, 7052, 7053, 7054, 7055, 7056, 7057, 7058, 7059, 7060, 7061, 7062, 7063, 7064, 7065, 7066, 7067, 7068, 7069, 7070, 7071, 7073, 7074, 7075, 7076, 7077, 7078, 7079, 7080, 7081, 7082, 7083, 7084, 7085, 7086, 7087, 7088, 7089, 7090, 7091, 7092, 7093, 7094, 7095, 7096, 7097, 7098, 7099 };

static volatile int shutDownNode = 0;
static volatile unsigned char mainAuxStatus = 0;
static volatile bool forceRefreshPeerList = false;
static volatile bool forceNextTick = false;
static volatile bool forceSwitchEpoch = false;
static volatile char criticalSituation = 0;
static volatile bool systemMustBeSaved = false, spectrumMustBeSaved = false, universeMustBeSaved = false, computerMustBeSaved = false;

static int misalignedState = 0;

static volatile unsigned char epochTransitionState = 0;
static volatile long epochTransitionWaitingRequestProcessors = 0;

static m256i operatorPublicKey;
static m256i computorSubseeds[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i computorPrivateKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i computorPublicKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i arbitratorPublicKey;

BroadcastComputors broadcastedComputors;

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
static Tick etalonTick;
static TickData nextTickData;

static m256i uniqueNextTickTransactionDigests[NUMBER_OF_COMPUTORS];
static unsigned int uniqueNextTickTransactionDigestCounters[NUMBER_OF_COMPUTORS];

static unsigned long long resourceTestingDigest = 0;

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
static const Transaction* contractProcessorTransaction = 0;
static int contractProcessorTransactionMoneyflew = 0;
static EFI_EVENT contractProcessorEvent;
static m256i contractStateDigests[MAX_NUMBER_OF_CONTRACTS * 2 - 1];
const unsigned long long contractStateDigestsSizeInBytes = sizeof(contractStateDigests);

// targetNextTickDataDigestIsKnown == true signals that we need to fetch TickData (update the version in this node)
// targetNextTickDataDigestIsKnown == false means there is no consensus on next tick data yet
static bool targetNextTickDataDigestIsKnown = false;
static m256i targetNextTickDataDigest;
static unsigned long long tickTicks[11];

static m256i releasedPublicKeys[NUMBER_OF_COMPUTORS];
static long long releasedAmounts[NUMBER_OF_COMPUTORS];
static unsigned int numberOfReleasedEntities;

static EFI_MP_SERVICES_PROTOCOL* mpServicesProtocol;
static unsigned int numberOfProcessors = 0;
static Processor processors[MAX_NUMBER_OF_PROCESSORS];

// Variables for tracking the detail of processors(CPU core) and function, this is useful for resource management and debugging
static unsigned long long tickProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function tickProcessor
static unsigned long long requestProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function requestProcessor
static unsigned long long contractProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that run function contractProcessor

static unsigned long long solutionProcessorIDs[MAX_NUMBER_OF_PROCESSORS]; // a list of proc id that will process solution
static bool solutionProcessorFlags[MAX_NUMBER_OF_PROCESSORS]; // flag array to indicate that whether a procId should help processing solutions or not
static unsigned long long mainThreadProcessorID = -1;
static int nTickProcessorIDs = 0;
static int nRequestProcessorIDs = 0;
static int nContractProcessorIDs = 0;
static int nSolutionProcessorIDs = 0;

static ScoreFunction<
    DATA_LENGTH,
    NUMBER_OF_HIDDEN_NEURONS, 
    NUMBER_OF_NEIGHBOR_NEURONS,
    MAX_DURATION,
    NUMBER_OF_OPTIMIZATION_STEPS,
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
    unsigned long long resourceTestingDigest;
    unsigned int numberOfMiners;
    unsigned int numberOfTransactions;
    unsigned long long lastLogId;
} nodeStateBuffer;
#endif
static bool saveComputer(CHAR16* directory = NULL);
static bool saveSystem(CHAR16* directory = NULL);
static bool loadComputer(CHAR16* directory = NULL, bool forceLoadFromFile = false);

BroadcastFutureTickData broadcastedFutureTickData;

static struct
{
	Transaction transaction;
	unsigned char data[VOTE_COUNTER_DATA_SIZE_IN_BYTES];
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

static void logToConsole(const CHAR16* message)
{
    if (disableConsoleLogging)
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
                                || misalignedState == 1
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

        // Set isVerified if sExchangePublicPeers was received on outgoing connection
        if (peer->address.u32)
        {
            for (unsigned int j = 0; j < numberOfPublicPeers; j++)
            {
                if (peer->address == publicPeers[j].address)
                {
                    publicPeers[j].isVerified = true;

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
            if ((hasEnoughBalance || computorIndex(request->sourcePublicKey) >=0 ) && header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            if (isZero(request->destinationPublicKey))
            {
                if (request->sourcePublicKey == arbitratorPublicKey)
                {
                    // See CustomMiningTaskMessage structure
                }
                else
                {
                    for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
                    {
                        if (request->destinationPublicKey == computorPublicKeys[i])
                        {
                            // See CustomMiningSolutionMessage structure

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
                                    bs->SetMem(sharedKeyAndGammingNonce, 32, 0);
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
                                bs->CopyMem(&sharedKeyAndGammingNonce[32], &request->gammingNonce, 32);
                                unsigned char gammingKey[32];
                                KangarooTwelve64To32(sharedKeyAndGammingNonce, gammingKey);
                                bs->SetMem(sharedKeyAndGammingNonce, 32, 0); // Zero the shared key in case stack content could be leaked later
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
            bs->CopyMem(&broadcastedComputors.computors, &request->computors, sizeof(Computors));

            // Update ownComputorIndices and minerPublicKeys
            if (request->computors.epoch == system.epoch)
            {
                numberOfOwnComputorIndices = 0;
                ACQUIRE(minerScoreArrayLock);
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
        if (verify(broadcastedComputors.computors.publicKeys[request->tick.computorIndex].m256i_u8, digest, request->tick.signature))
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
                    || request->tick.expectedNextTickTransactionDigest != tsTick->expectedNextTickTransactionDigest)
                {
                    faultyComputorFlags[request->tick.computorIndex >> 6] |= (1ULL << (request->tick.computorIndex & 63));
                }
            }
            else
            {
                // Copy the sent tick to the tick storage
                bs->CopyMem(tsTick, &request->tick, sizeof(Tick));
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
                                bs->CopyMem(&td, &request->tickData, sizeof(TickData));
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
                            bs->CopyMem(&td, &request->tickData, sizeof(TickData));
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
                    bs->CopyMem(&computorPendingTransactions[computorIndex * offset * MAX_TRANSACTION_SIZE], request, transactionSize);
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
                        bs->CopyMem(&entityPendingTransactions[spectrumIndex * MAX_TRANSACTION_SIZE], request, transactionSize);
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
                                bs->CopyMem(ts.tickTransactions(ts.nextTickTransactionOffset), request, transactionSize);
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
        // The order of the computors is randomized.
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
                    enqueueResponse(peer, sizeof(Tick), BroadcastTick::type, header->dejavu(), tsTick);
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
        unsigned short tickTransactionIndices[NUMBER_OF_TRANSACTIONS_PER_TICK];
        unsigned short numberOfTickTransactions;
        for (numberOfTickTransactions = 0; numberOfTickTransactions < NUMBER_OF_TRANSACTIONS_PER_TICK; numberOfTickTransactions++)
        {
            tickTransactionIndices[numberOfTickTransactions] = numberOfTickTransactions;
        }
        while (numberOfTickTransactions)
        {
            const unsigned short index = random(numberOfTickTransactions);

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
        bs->SetMem(&currentTickInfo, sizeof(CurrentTickInfo), 0);
    }

    enqueueResponse(peer, sizeof(currentTickInfo), RESPOND_CURRENT_TICK_INFO, header->dejavu(), &currentTickInfo);
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

        bs->SetMem(respondedEntity.siblings, sizeof(respondedEntity.siblings), 0);
    }
    else
    {
        bs->CopyMem(&respondedEntity.entity, &spectrum[respondedEntity.spectrumIndex], sizeof(::Entity));
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
        || system.epoch >= contractDescriptions[request->contractIndex].constructionEpoch)
    {
        bs->SetMem(respondContractIPO.publicKeys, sizeof(respondContractIPO.publicKeys), 0);
        bs->SetMem(respondContractIPO.prices, sizeof(respondContractIPO.prices), 0);
    }
    else
    {
        contractStateLock[request->contractIndex].acquireRead();
        IPO* ipo = (IPO*)contractStates[request->contractIndex];
        bs->CopyMem(respondContractIPO.publicKeys, ipo->publicKeys, sizeof(respondContractIPO.publicKeys));
        bs->CopyMem(respondContractIPO.prices, ipo->prices, sizeof(respondContractIPO.prices));
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
        qpiContext.call(request->inputType, (((unsigned char*)request) + sizeof(RequestContractFunction)), request->inputSize);
        enqueueResponse(peer, qpiContext.outputSize, RespondContractFunction::type, header->dejavu(), qpiContext.outputBuffer);
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

    enqueueResponse(peer, sizeof(respondedSystemInfo), RESPOND_SYSTEM_INFO, header->dejavu(), &respondedSystemInfo);
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
                if (status != EFI_SUCCESS)
                {
                    logStatusToConsole(L"SetTime() failed!", status, __LINE__);
                }
            }
            // this has no break by intention, because SPECIAL_COMMAND_SEND_TIME responds the same way as SPECIAL_COMMAND_QUERY_TIME
            case SPECIAL_COMMAND_QUERY_TIME:
            {
                // send back current time
                SpecialCommandSendTime response;
                response.everIncreasingNonceAndCommandType = (request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) | (SPECIAL_COMMAND_SEND_TIME << 56);
                updateTime();
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

static unsigned int getTickInMiningPhaseCycle()
{
    return (system.tick - system.initialTick) % (INTERNAL_COMPUTATIONS_INTERVAL + EXTERNAL_COMPUTATIONS_INTERVAL);
}

static void checkAndSwitchMiningPhase()
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
    }
}

// Disabling the optimizer for requestProcessor() is a workaround introduced to solve an issue
// that has been observed in testnets/2024-11-23-release-227-qvault.
// In this test, the processors calling requestProcessor() were stuck before entering the function.
// Probably, this was caused by a bug in the optimizer, because disabling the optimizer solved the
// problem.
#pragma optimize("", off)
static void requestProcessor(void* ProcedureArgument)
{
    enableAVX();

    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);

    Processor* processor = (Processor*)ProcedureArgument;
    RequestResponseHeader* header = (RequestResponseHeader*)processor->buffer;
    while (!shutDownNode)
    {
        checkinTime(processorNumber);
        // in epoch transition, wait here
        if (epochTransitionState)
        {
            _InterlockedIncrement(&epochTransitionWaitingRequestProcessors);
            while (epochTransitionState)
            {
                _mm_pause();
            }
            _InterlockedDecrement(&epochTransitionWaitingRequestProcessors);
        }

        // try to compute a solution if any is queued and this thread is assigned to compute solution
        if (solutionProcessorFlags[processorNumber])
        {
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
                const unsigned long long beginningTick = __rdtsc();

                {
                    RequestResponseHeader* requestHeader = (RequestResponseHeader*)&requestQueueBuffer[requestQueueElements[requestQueueElementTail].offset];
                    bs->CopyMem(header, requestHeader, requestHeader->size());
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
                    logger.processRequestLog(peer, header);
                }
                break;

                case RequestLogIdRangeFromTx::type:
                {
                    logger.processRequestTxLogInfo(peer, header);
                }
                break;

                case RequestAllLogIdRangesFromTick::type:
                {
                    logger.processRequestTickTxLogInfo(peer, header);
                }
                break;

                case REQUEST_SYSTEM_INFO:
                {
                    processRequestSystemInfo(peer, header);
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
#pragma optimize("", on)

QPI::id QPI::QpiContextFunctionCall::arbitrator() const
{
    return arbitratorPublicKey;
}

QPI::id QPI::QpiContextFunctionCall::computor(unsigned short computorIndex) const
{
    return broadcastedComputors.computors.publicKeys[computorIndex % NUMBER_OF_COMPUTORS];
}

unsigned char QPI::QpiContextFunctionCall::day() const
{
    return etalonTick.day;
}

unsigned char QPI::QpiContextFunctionCall::dayOfWeek(unsigned char year, unsigned char month, unsigned char day) const
{
    return dayIndex(year, month, day) % 7;
}

unsigned char QPI::QpiContextFunctionCall::hour() const
{
    return etalonTick.hour;
}

unsigned short QPI::QpiContextFunctionCall::millisecond() const
{
    return etalonTick.millisecond;
}

unsigned char QPI::QpiContextFunctionCall::minute() const
{
    return etalonTick.minute;
}

unsigned char QPI::QpiContextFunctionCall::month() const
{
    return etalonTick.month;
}


int QPI::QpiContextFunctionCall::numberOfTickTransactions() const
{
    return -1; // TODO: Return -1 if the current tick is empty, return the number of the transactions in the tick otherwise, including 0
}

unsigned char QPI::QpiContextFunctionCall::second() const
{
    return etalonTick.second;
}

bool QPI::QpiContextFunctionCall::signatureValidity(const m256i& entity, const m256i& digest, const array<signed char, 64>& signature) const
{
    return verify(entity.m256i_u8, digest.m256i_u8, reinterpret_cast<const unsigned char*>(&signature));
}

unsigned char QPI::QpiContextFunctionCall::year() const
{
    return etalonTick.year;
}

template <typename T>
m256i QPI::QpiContextFunctionCall::K12(const T& data) const
{
    m256i digest;

    KangarooTwelve(&data, sizeof(data), &digest, sizeof(digest));

    return digest;
}

static void contractProcessor(void*)
{
    enableAVX();

    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);

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
                QpiContextSystemProcedureCall qpiContext(executedContractIndex);
                qpiContext.call(INITIALIZE);
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
                QpiContextSystemProcedureCall qpiContext(executedContractIndex);
                qpiContext.call(BEGIN_EPOCH);
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
                QpiContextSystemProcedureCall qpiContext(executedContractIndex);
                qpiContext.call(BEGIN_TICK);
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
                QpiContextSystemProcedureCall qpiContext(executedContractIndex);
                qpiContext.call(END_TICK);
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
                QpiContextSystemProcedureCall qpiContext(executedContractIndex);
                qpiContext.call(END_EPOCH);
            }
        }
    }
    break;

    // TODO: rename to invoke (with option to have amount)
    case USER_PROCEDURE_CALL:
    {
        const Transaction* transaction = contractProcessorTransaction;
        ASSERT(transaction && transaction->checkValidity());

        unsigned int contractIndex = (unsigned int)transaction->destinationPublicKey.m256i_u64[0];
        ASSERT(system.epoch >= contractDescriptions[contractIndex].constructionEpoch);
        ASSERT(system.epoch < contractDescriptions[contractIndex].destructionEpoch);
        ASSERT(contractUserProcedures[contractIndex][transaction->inputType]);

        QpiContextUserProcedureCall qpiContext(contractIndex, transaction->sourcePublicKey, transaction->amount);
        qpiContext.call(transaction->inputType, transaction->inputPtr(), transaction->inputSize);

        if (contractActionTracker.getOverallQuTransferBalance(transaction->sourcePublicKey) == 0)
            contractProcessorTransactionMoneyflew = 0;
        else
            contractProcessorTransactionMoneyflew = 1;
        contractProcessorTransaction = 0;
    }
    break;
    }
}

static void processTickTransactionContractIPO(const Transaction* transaction, const int spectrumIndex, const unsigned int contractIndex)
{
    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(transaction->tick == system.tick);
    ASSERT(!transaction->amount && transaction->inputSize == sizeof(ContractIPOBid));
    ASSERT(spectrumIndex >= 0);
    ASSERT(contractIndex < contractCount);
    ASSERT(system.epoch < contractDescriptions[contractIndex].constructionEpoch);

    ContractIPOBid* contractIPOBid = (ContractIPOBid*)transaction->inputPtr();
    if (contractIPOBid->price > 0 && contractIPOBid->price <= MAX_AMOUNT / NUMBER_OF_COMPUTORS
        && contractIPOBid->quantity > 0 && contractIPOBid->quantity <= NUMBER_OF_COMPUTORS)
    {
        const long long amount = contractIPOBid->price * contractIPOBid->quantity;
        if (decreaseEnergy(spectrumIndex, amount))
        {
            const QuTransfer quTransfer = { transaction->sourcePublicKey, m256i::zero(), amount };
            logger.logQuTransfer(quTransfer);

            numberOfReleasedEntities = 0;
            contractStateLock[contractIndex].acquireWrite();
            IPO* ipo = (IPO*)contractStates[contractIndex];
            for (unsigned int i = 0; i < contractIPOBid->quantity; i++)
            {
                if (contractIPOBid->price <= ipo->prices[NUMBER_OF_COMPUTORS - 1])
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (transaction->sourcePublicKey == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = transaction->sourcePublicKey;
                        releasedAmounts[numberOfReleasedEntities++] = contractIPOBid->price;
                    }
                    else
                    {
                        releasedAmounts[j] += contractIPOBid->price;
                    }
                }
                else
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (ipo->publicKeys[NUMBER_OF_COMPUTORS - 1] == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = ipo->publicKeys[NUMBER_OF_COMPUTORS - 1];
                        releasedAmounts[numberOfReleasedEntities++] = ipo->prices[NUMBER_OF_COMPUTORS - 1];
                    }
                    else
                    {
                        releasedAmounts[j] += ipo->prices[NUMBER_OF_COMPUTORS - 1];
                    }

                    ipo->publicKeys[NUMBER_OF_COMPUTORS - 1] = transaction->sourcePublicKey;
                    ipo->prices[NUMBER_OF_COMPUTORS - 1] = contractIPOBid->price;
                    j = NUMBER_OF_COMPUTORS - 1;
                    while (j
                        && ipo->prices[j - 1] < ipo->prices[j])
                    {
                        const m256i tmpPublicKey = ipo->publicKeys[j - 1];
                        const long long tmpPrice = ipo->prices[j - 1];
                        ipo->publicKeys[j - 1] = ipo->publicKeys[j];
                        ipo->prices[j - 1] = ipo->prices[j];
                        ipo->publicKeys[j] = tmpPublicKey;
                        ipo->prices[j--] = tmpPrice;
                    }

                    contractStateChangeFlags[contractIndex >> 6] |= (1ULL << (contractIndex & 63));
                }
            }
            contractStateLock[contractIndex].releaseWrite();

            for (unsigned int i = 0; i < numberOfReleasedEntities; i++)
            {
                increaseEnergy(releasedPublicKeys[i], releasedAmounts[i]);
                const QuTransfer quTransfer = { m256i::zero(), releasedPublicKeys[i], releasedAmounts[i] };
                logger.logQuTransfer(quTransfer);
            }
        }
    }
}

// Return if money flew
static bool processTickTransactionContractProcedure(const Transaction* transaction, const int spectrumIndex, const unsigned int contractIndex)
{
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
        contractProcessorTransaction = transaction;
        contractProcessorPhase = USER_PROCEDURE_CALL;
        contractProcessorState = 1;
        while (contractProcessorState)
        {
            _mm_pause();
        }

        return contractProcessorTransactionMoneyflew;
    }

    // if transaction tries to invoke non-registered procedure, transaction amount is not reimbursed
    return transaction->amount > 0;
}

static void processTickTransactionSolution(const MiningSolutionTransaction* transaction, const unsigned long long processorNumber)
{
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
            resourceTestingDigest ^= (unsigned long long)(solutionScore);
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
    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(isZero(transaction->destinationPublicKey));
    ASSERT(transaction->tick == system.tick);

    // TODO
}

static void processTickTransactionOracleReplyReveal(const OracleReplyRevealTransactionPrefix* transaction)
{
    ASSERT(nextTickData.epoch == system.epoch);
    ASSERT(transaction != nullptr);
    ASSERT(transaction->checkValidity());
    ASSERT(isZero(transaction->destinationPublicKey));
    ASSERT(transaction->tick == system.tick);

    // TODO
}

static void processTickTransaction(const Transaction* transaction, const m256i& transactionDigest, unsigned long long processorNumber)
{
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

            if (transaction->amount)
            {
                moneyFlew = true;
                const QuTransfer quTransfer = { transaction->sourcePublicKey , transaction->destinationPublicKey , transaction->amount };
                logger.logQuTransfer(quTransfer);
            }

            if (isZero(transaction->destinationPublicKey))
            {
                switch (transaction->inputType)
                {
                case VOTE_COUNTER_INPUT_TYPE:
                {
                    int computorIndex = transaction->tick % NUMBER_OF_COMPUTORS;
                    if (transaction->sourcePublicKey == broadcastedComputors.computors.publicKeys[computorIndex]) // this tx was sent by the tick leader of this tick
                    {
                        if (!transaction->amount
                            && transaction->inputSize == VOTE_COUNTER_DATA_SIZE_IN_BYTES)
                        {
                            voteCounter.addVotes(transaction->inputPtr(), computorIndex);
                        }
                    }
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
                }
            }
            else
            {
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
                    if (system.epoch < contractDescriptions[contractIndex].constructionEpoch)
                    {
                        // IPO
                        if (!transaction->amount
                            && transaction->inputSize == sizeof(ContractIPOBid))
                        {
                            processTickTransactionContractIPO(transaction, spectrumIndex, contractIndex);
                        }
                    }
                    else if (system.epoch < contractDescriptions[contractIndex].destructionEpoch)
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

#pragma optimize("", off)
static void processTick(unsigned long long processorNumber)
{
    if (system.tick > system.initialTick)
    {
        etalonTick.prevResourceTestingDigest = resourceTestingDigest;
        etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
        getUniverseDigest(etalonTick.prevUniverseDigest);
        getComputerDigest(etalonTick.prevComputerDigest);
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
        }
#endif
    }
    else
    {
        // it should never go here
    }

    if (system.tick == system.initialTick)
    {
        logger.reset(system.initialTick, system.initialTick); // clear logs here to give more time for querying and persisting the data when we do seamless transition
        logger.registerNewTx(system.tick, logger.SC_INITIALIZE_TX);
        contractProcessorPhase = INITIALIZE;
        contractProcessorState = 1;
        while (contractProcessorState)
        {
            _mm_pause();
        }

        logger.registerNewTx(system.tick, logger.SC_BEGIN_EPOCH_TX);
        contractProcessorPhase = BEGIN_EPOCH;
        contractProcessorState = 1;
        while (contractProcessorState)
        {
            _mm_pause();
        }
    }

    logger.registerNewTx(system.tick, logger.SC_BEGIN_TICK_TX);
    contractProcessorPhase = BEGIN_TICK;
    contractProcessorState = 1;
    while (contractProcessorState)
    {
        _mm_pause();
    }

    unsigned int tickIndex = ts.tickToIndexCurrentEpoch(system.tick);
    ts.tickData.acquireLock();
    bs->CopyMem(&nextTickData, &ts.tickData[tickIndex], sizeof(TickData));
    ts.tickData.releaseLock();
    unsigned long long solutionProcessStartTick = __rdtsc(); // for tracking the time processing solutions
    if (nextTickData.epoch == system.epoch)
    {
        auto* tsCurrentTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(tickIndex);
#if ADDON_TX_STATUS_REQUEST
        txStatusData.tickTxIndexStart[system.tick - system.initialTick] = numberOfTransactions; // qli: part of tx_status_request add-on
#endif
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

        {
            // Process solutions in this tick and store in cache. In parallel, score->tryProcessSolution() is called by
            // request processors to speed up solution processing.
            score->startProcessTaskQueue();
            while (!score->isTaskQueueProcessed())
            {
                score->tryProcessSolution(processorNumber);
            }
            score->stopProcessTaskQueue();
        }
        solutionTotalExecutionTicks = __rdtsc() - solutionProcessStartTick; // for tracking the time processing solutions

        // Process all transaction of the tick
        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(nextTickData.transactionDigests[transactionIndex]))
            {
                if (tsCurrentTickTransactionOffsets[transactionIndex])
                {
                    Transaction* transaction = ts.tickTransactions(tsCurrentTickTransactionOffsets[transactionIndex]);
                    logger.registerNewTx(transaction->tick, transactionIndex);
                    processTickTransaction(transaction, nextTickData.transactionDigests[transactionIndex], processorNumber);
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
    }

    logger.registerNewTx(system.tick, logger.SC_END_TICK_TX);
    contractProcessorPhase = END_TICK;
    contractProcessorState = 1;
    while (contractProcessorState)
    {
        _mm_pause();
    }

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

    getUniverseDigest(etalonTick.saltedUniverseDigest);
    getComputerDigest(etalonTick.saltedComputerDigest);

    for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
    {
        if ((system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET) % NUMBER_OF_COMPUTORS == ownComputorIndices[i])
        {
            if (system.tick > system.latestLedTick)
            {
                if (mainAuxStatus & 1)
                {
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

                    unsigned int numberOfEntityPendingTransactionIndices;
                    for (numberOfEntityPendingTransactionIndices = 0; numberOfEntityPendingTransactionIndices < NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR; numberOfEntityPendingTransactionIndices++)
                    {
                        entityPendingTransactionIndices[numberOfEntityPendingTransactionIndices] = numberOfEntityPendingTransactionIndices;
                    }
                    while (j < NUMBER_OF_TRANSACTIONS_PER_TICK && numberOfEntityPendingTransactionIndices)
                    {
                        const unsigned int index = random(numberOfEntityPendingTransactionIndices);

                        const Transaction* pendingTransaction = ((Transaction*)&computorPendingTransactions[entityPendingTransactionIndices[index] * MAX_TRANSACTION_SIZE]);
                        if (pendingTransaction->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET)
                        {
                            ASSERT(pendingTransaction->checkValidity());
                            const unsigned int transactionSize = pendingTransaction->totalSize();
                            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                            {
                                ts.tickTransactions.acquireLock();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    ts.tickTransactionOffsets(pendingTransaction->tick, j) = ts.nextTickTransactionOffset;
                                    bs->CopyMem(ts.tickTransactions(ts.nextTickTransactionOffset), (void*)pendingTransaction, transactionSize);
                                    broadcastedFutureTickData.tickData.transactionDigests[j] = &computorPendingTransactionDigests[entityPendingTransactionIndices[index] * 32ULL];
                                    j++;
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                                ts.tickTransactions.releaseLock();
                            }
                        }

                        entityPendingTransactionIndices[index] = entityPendingTransactionIndices[--numberOfEntityPendingTransactionIndices];
                    }

                    for (numberOfEntityPendingTransactionIndices = 0; numberOfEntityPendingTransactionIndices < SPECTRUM_CAPACITY; numberOfEntityPendingTransactionIndices++)
                    {
                        entityPendingTransactionIndices[numberOfEntityPendingTransactionIndices] = numberOfEntityPendingTransactionIndices;
                    }
                    while (j < NUMBER_OF_TRANSACTIONS_PER_TICK && numberOfEntityPendingTransactionIndices)
                    {
                        const unsigned int index = random(numberOfEntityPendingTransactionIndices);

                        const Transaction* pendingTransaction = ((Transaction*)&entityPendingTransactions[entityPendingTransactionIndices[index] * MAX_TRANSACTION_SIZE]);
                        if (pendingTransaction->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET)
                        {
                            ASSERT(pendingTransaction->checkValidity());
                            const unsigned int transactionSize = pendingTransaction->totalSize();
                            if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                            {
                                ts.tickTransactions.acquireLock();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    ts.tickTransactionOffsets(pendingTransaction->tick, j) = ts.nextTickTransactionOffset;
                                    bs->CopyMem(ts.tickTransactions(ts.nextTickTransactionOffset), (void*)pendingTransaction, transactionSize);
                                    broadcastedFutureTickData.tickData.transactionDigests[j] = &entityPendingTransactionDigests[entityPendingTransactionIndices[index] * 32ULL];
                                    j++;
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                                ts.tickTransactions.releaseLock();
                            }
                        }

                        entityPendingTransactionIndices[index] = entityPendingTransactionIndices[--numberOfEntityPendingTransactionIndices];
                    }

                    for (; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                    {
                        broadcastedFutureTickData.tickData.transactionDigests[j] = m256i::zero();
                    }

                    bs->SetMem(broadcastedFutureTickData.tickData.contractFees, sizeof(broadcastedFutureTickData.tickData.contractFees), 0);

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

    for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
    {
        if ((system.tick + TICK_VOTE_COUNTER_PUBLICATION_OFFSET) % NUMBER_OF_COMPUTORS == ownComputorIndices[i])
        {
            if (mainAuxStatus & 1)
            {
                auto& payload = voteCounterPayload; // note: not thread-safe
                payload.transaction.sourcePublicKey = computorPublicKeys[ownComputorIndicesMapping[i]];
                payload.transaction.destinationPublicKey = m256i::zero();
                payload.transaction.amount = 0;
                payload.transaction.tick = system.tick + TICK_VOTE_COUNTER_PUBLICATION_OFFSET;
                payload.transaction.inputType = VOTE_COUNTER_INPUT_TYPE;
                payload.transaction.inputSize = sizeof(payload.data);
                voteCounter.compressNewVotesPacket(system.tick - 675, system.tick + 1, ownComputorIndices[i], payload.data);
                unsigned char digest[32];
                KangarooTwelve(&payload.transaction, sizeof(payload.transaction) + sizeof(payload.data), digest, sizeof(digest));
                sign(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, payload.signature);
                enqueueResponse(NULL, sizeof(payload), BROADCAST_TRANSACTION, 0, &payload);
            }
        }
    }

    if (mainAuxStatus & 1)
    {
        // Publish solutions that were sent via BroadcastMessage as MiningSolutionTransaction
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
    // Check that continous updating of spectrum info is consistent with counting from scratch
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
}

#pragma optimize("", on)

static void beginEpoch()
{
    // This version doesn't support migration from contract IPO to contract operation!

    numberOfOwnComputorIndices = 0;

    broadcastedComputors.computors.epoch = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        broadcastedComputors.computors.publicKeys[i].setRandomValue();
    }
    bs->SetMem(&broadcastedComputors.computors.signature, sizeof(broadcastedComputors.computors.signature), 0);

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

    bs->SetMem(solutionPublicationTicks, sizeof(solutionPublicationTicks), 0);
    bs->SetMem(faultyComputorFlags, sizeof(faultyComputorFlags), 0);

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
    bs->SetMem(minerSolutionFlags, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, 0);
    bs->SetMem((void*)minerPublicKeys, sizeof(minerPublicKeys), 0);
    bs->SetMem((void*)minerScores, sizeof(minerScores), 0);
    numberOfMiners = NUMBER_OF_COMPUTORS;
    bs->SetMem(competitorPublicKeys, sizeof(competitorPublicKeys), 0);
    bs->SetMem(competitorScores, sizeof(competitorScores), 0);
    bs->SetMem(competitorComputorStatuses, sizeof(competitorComputorStatuses), 0);
    minimumComputorScore = 0;
    minimumCandidateScore = 0;

    if (system.epoch < MAX_NUMBER_EPOCH && (solutionThreshold[system.epoch] <= 0 || solutionThreshold[system.epoch] > DATA_LENGTH)) { // invalid threshold
        solutionThreshold[system.epoch] = SOLUTION_THRESHOLD_DEFAULT;
    }

    system.latestOperatorNonce = 0;
    system.numberOfSolutions = 0;
    bs->SetMem(system.solutions, sizeof(system.solutions), 0);
    bs->SetMem(system.futureComputors, sizeof(system.futureComputors), 0);

    // Reset resource testing digest at beginning of the epoch
    // there are many global variables that were init at declaration, may need to re-check all of them again
    resourceTestingDigest = 0;

    numberOfTransactions = 0;
#if TICK_STORAGE_AUTOSAVE_MODE
    ts.initMetaData(system.epoch); // for save/load state
#endif
}


// called by tickProcessor() after system.tick has been incremented
static void endEpoch()
{
    logger.registerNewTx(system.tick, logger.SC_END_EPOCH_TX);
    contractProcessorPhase = END_EPOCH;
    contractProcessorState = 1;
    while (contractProcessorState)
    {
        _mm_pause();
    }

    // treating endEpoch as a tick, start updating etalonTick:
    // this is the last tick of an epoch, should we set prevResourceTestingDigest to zero? nodes that start from scratch (for the new epoch)
    // would be unable to compute this value(!?)
    etalonTick.prevResourceTestingDigest = resourceTestingDigest; 
    etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
    getUniverseDigest(etalonTick.prevUniverseDigest);
    getComputerDigest(etalonTick.prevComputerDigest);

    // Handle IPO
    for (unsigned int contractIndex = 1; contractIndex < contractCount; contractIndex++)
    {
        if (system.epoch < contractDescriptions[contractIndex].constructionEpoch)
        {
            contractStateLock[contractIndex].acquireRead();
            IPO* ipo = (IPO*)contractStates[contractIndex];
            long long finalPrice = ipo->prices[NUMBER_OF_COMPUTORS - 1];
            int issuanceIndex, ownershipIndex, possessionIndex;
            if (finalPrice)
            {
                if (!issueAsset(m256i::zero(), (char*)contractDescriptions[contractIndex].assetName, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex))
                {
                    finalPrice = 0;
                }
            }
            numberOfReleasedEntities = 0;
            for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                if (ipo->prices[i] > finalPrice)
                {
                    unsigned int j;
                    for (j = 0; j < numberOfReleasedEntities; j++)
                    {
                        if (ipo->publicKeys[i] == releasedPublicKeys[j])
                        {
                            break;
                        }
                    }
                    if (j == numberOfReleasedEntities)
                    {
                        releasedPublicKeys[numberOfReleasedEntities] = ipo->publicKeys[i];
                        releasedAmounts[numberOfReleasedEntities++] = ipo->prices[i] - finalPrice;
                    }
                    else
                    {
                        releasedAmounts[j] += (ipo->prices[i] - finalPrice);
                    }
                }
                if (finalPrice)
                {
                    int destinationOwnershipIndex, destinationPossessionIndex;
                    transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, ipo->publicKeys[i], 1, &destinationOwnershipIndex, &destinationPossessionIndex, true);
                }
            }
            for (unsigned int i = 0; i < numberOfReleasedEntities; i++)
            {
                increaseEnergy(releasedPublicKeys[i], releasedAmounts[i]);
                const QuTransfer quTransfer = { m256i::zero(), releasedPublicKeys[i], releasedAmounts[i] };
                logger.logQuTransfer(quTransfer);
            }
            contractStateLock[contractIndex].releaseRead();

            contractStateLock[0].acquireWrite();
            contractFeeReserve(contractIndex) = finalPrice * NUMBER_OF_COMPUTORS;
            contractStateLock[0].releaseWrite();
        }
    }

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
        bs->SetMem(revenueScore, sizeof(revenueScore), 0);
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
                revenueScore[tick % NUMBER_OF_COMPUTORS] += revenuePoints[numberOfTransactions];
            }
            ts.tickData.releaseLock();
        }

        // Merge votecount to final rev score
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            unsigned long long vote_count = voteCounter.getVoteCount(i);
            if (system.tick - system.initialTick <= NUMBER_OF_COMPUTORS)
            {
                // Due to the validity check requring 675*451 votes, which assumes at least 676 ticks in the epoch, all vote counts are 0 if the epoch has less ticks.
                // This is a workaround to prevent that no computor gets revenue in this case.
                vote_count = 1;
            }
            if (vote_count != 0)
            {
                unsigned long long final_score = vote_count * revenueScore[i];
                if ((final_score / vote_count) != revenueScore[i]) // detect overflow
                {
                    revenueScore[i] = 0xFFFFFFFFFFFFFFFFULL; // maximum score
                }
                else
                {
                    revenueScore[i] = final_score;
                }
            }
            else
            {
                revenueScore[i] = 0;
            }
        }

        // Sort revenue scores to get lowest score of quorum
        unsigned long long sortedRevenueScore[QUORUM + 1];
        bs->SetMem(sortedRevenueScore, sizeof(sortedRevenueScore), 0);
        for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
        {
            sortedRevenueScore[QUORUM] = revenueScore[computorIndex];
            unsigned int i = QUORUM;
            while (i
                && sortedRevenueScore[i - 1] < sortedRevenueScore[i])
            {
                const unsigned long long tmp = sortedRevenueScore[i - 1];
                sortedRevenueScore[i - 1] = sortedRevenueScore[i];
                sortedRevenueScore[i--] = tmp;
            }
        }
        if (!sortedRevenueScore[QUORUM - 1])
        {
            sortedRevenueScore[QUORUM - 1] = 1;
        }

        // Get revenue donation data by calling contract GQMPROP::GetRevenueDonation()
        QpiContextUserFunctionCall qpiContext(GQMPROP::__contract_index);
        qpiContext.call(5, "", 0);
        ASSERT(qpiContext.outputSize == sizeof(GQMPROP::RevenueDonationT));
        const GQMPROP::RevenueDonationT* emissionDist = (GQMPROP::RevenueDonationT*)qpiContext.outputBuffer;

        // Compute revenue of computors and arbitrator
        long long arbitratorRevenue = ISSUANCE_RATE;
        constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;
        constexpr long long scalingThreshold = 0xFFFFFFFFFFFFFFFFULL / issuancePerComputor;
        static_assert(MAX_NUMBER_OF_TICKS_PER_EPOCH <= 605020, "Redefine scalingFactor");
        // maxRevenueScore for 605020 ticks = ((7099 * 605020) / 676) * 605020 * 675
        constexpr unsigned scalingFactor = 208100; // >= (maxRevenueScore600kTicks / 0xFFFFFFFFFFFFFFFFULL) * issuancePerComputor =(approx)= 208078.5
        for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
        {
            // Compute initial computor revenue, reducing arbitrator revenue
            long long revenue;
            if (revenueScore[computorIndex] >= sortedRevenueScore[QUORUM - 1])
                revenue = issuancePerComputor;
            else
            {
                if (revenueScore[computorIndex] > scalingThreshold)
                {
                    // scale down to prevent overflow, then scale back up after division
                    unsigned long long scaledRev = revenueScore[computorIndex] / scalingFactor;
                    revenue = ((issuancePerComputor * scaledRev) / sortedRevenueScore[QUORUM - 1]);
                    revenue *= scalingFactor;
                }
                else
                {
                    revenue = ((issuancePerComputor * ((unsigned long long)revenueScore[computorIndex])) / sortedRevenueScore[QUORUM - 1]);
                }
            }
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
                    if (revenue)
                    {
                        const QuTransfer quTransfer = { m256i::zero(), rdEntry.destinationPublicKey, donation };
                        logger.logQuTransfer(quTransfer);
                    }
                }
            }

            // Generate computor revenue
            increaseEnergy(broadcastedComputors.computors.publicKeys[computorIndex], revenue);
            if (revenue)
            {
                const QuTransfer quTransfer = { m256i::zero(), broadcastedComputors.computors.publicKeys[computorIndex], revenue };
                logger.logQuTransfer(quTransfer);
            }
        }
        emissionDist = nullptr; qpiContext.freeBuffer(); // Free buffer holding revenue donation table, because we don't need it anymore

        // Generate arbitrator revenue
        increaseEnergy((unsigned char*)&arbitratorPublicKey, arbitratorRevenue);
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

    system.epoch++;
    system.initialTick = system.tick;

    mainAuxStatus = ((mainAuxStatus & 1) << 1) | ((mainAuxStatus & 2) >> 1);
}


#if !START_NETWORK_FROM_SCRATCH

static bool haveSamePrevDigestsAndTime(const Tick& A, const Tick& B)
{
    return A.prevComputerDigest == B.prevComputerDigest &&
        A.prevResourceTestingDigest == B.prevResourceTestingDigest &&
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
    nodeStateBuffer.lastLogId = logger.logId;
    voteCounter.saveAllDataToArray(nodeStateBuffer.voteCounterData);

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
    logger.logId = nodeStateBuffer.lastLogId;
    loadMiningSeedFromFile = true;
    voteCounter.loadAllDataFromArray(nodeStateBuffer.voteCounterData);

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
    logToConsole(L"Initializing logger");
    logger.reset(system.initialTick, system.tick); // initialize the logger
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
// and look for those txs in local memory (pending txs). If a transaction doesn't exist, it will try to update requestedTickTransactions
// I main loop (MAIN thread), it will try to fetch missing txs based on the data inside requestedTickTransactions
// Current code assumes the limits:
// - 1 tx per source publickey per tick
// - 128 txs per computor publickey per tick
static void prepareNextTickTransactions()
{
    const unsigned int nextTick = system.tick + 1;
    const unsigned int nextTickIndex = ts.tickToIndexCurrentEpoch(nextTick);

    nextTickTransactionsSemaphore = 1; // signal a flag for displaying on the console log
    bs->SetMem(requestedTickTransactions.requestedTickTransactions.transactionFlags, sizeof(requestedTickTransactions.requestedTickTransactions.transactionFlags), 0);
    unsigned long long unknownTransactions[NUMBER_OF_TRANSACTIONS_PER_TICK / 64];
    bs->SetMem(unknownTransactions, sizeof(unknownTransactions), 0);
    const auto* tsNextTickTransactionOffsets = ts.tickTransactionOffsets.getByTickIndex(nextTickIndex);
    
    // This function maybe called multiple times per tick due to lack of data (txs or votes)
    // Here we do a simple pre scan to check txs via tsNextTickTransactionOffsets (already processed - aka already copying from pendingTransaction array to tickTransaction)
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
            ts.tickTransactions.releaseLock();
        }
    }
        
    if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
    {
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
                            if (!tsPendingTransactionOffsets[j])
                            {
                                const unsigned int transactionSize = pendingTransaction->totalSize();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    tsPendingTransactionOffsets[j] = ts.nextTickTransactionOffset;
                                    bs->CopyMem(ts.tickTransactions(ts.nextTickTransactionOffset), pendingTransaction, transactionSize);
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                            }
                            ts.tickTransactions.releaseLock();

                            numberOfKnownNextTickTransactions++;
                            unknownTransactions[j >> 6] &= ~(1ULL << (j & 63));

                            break;
                        }
                    }
                }

                RELEASE(computorPendingTransactionsLock);
            }
        }
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
                            if (!tsPendingTransactionOffsets[j])
                            {
                                const unsigned int transactionSize = pendingTransaction->totalSize();
                                if (ts.nextTickTransactionOffset + transactionSize <= ts.tickTransactions.storageSpaceCurrentEpoch)
                                {
                                    tsPendingTransactionOffsets[j] = ts.nextTickTransactionOffset;
                                    bs->CopyMem(ts.tickTransactions(ts.nextTickTransactionOffset), pendingTransaction, transactionSize);
                                    ts.nextTickTransactionOffset += transactionSize;
                                }
                            }
                            ts.tickTransactions.releaseLock();

                            numberOfKnownNextTickTransactions++;
                            unknownTransactions[j >> 6] &= ~(1ULL << (j & 63));

                            break;
                        }
                    }
                }

                RELEASE(entityPendingTransactionsLock);
            }
        }

        // Update requestedTickTransactions the list of txs that not exist in memory so the MAIN loop can try to fetch them from peers
        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
        {
            if (!(unknownTransactions[i >> 6] & (1ULL << (i & 63))))
            {
                requestedTickTransactions.requestedTickTransactions.transactionFlags[i >> 3] |= (1 << (i & 7));
            }
        }
    }
    nextTickTransactionsSemaphore = 0;
}

// broadcast all tickVotes from all IDs in this node
static void broadcastTickVotes()
{
    BroadcastTick broadcastTick;
    bs->CopyMem(&broadcastTick.tick, &etalonTick, sizeof(Tick));
    for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
    {
        broadcastTick.tick.computorIndex = ownComputorIndices[i] ^ BroadcastTick::type;
        broadcastTick.tick.epoch = system.epoch;
        m256i saltedData[2];
        saltedData[0] = computorPublicKeys[ownComputorIndicesMapping[i]];
        saltedData[1].m256i_u64[0] = resourceTestingDigest;
        KangarooTwelve(saltedData, 32 + sizeof(resourceTestingDigest), &broadcastTick.tick.saltedResourceTestingDigest, sizeof(broadcastTick.tick.saltedResourceTestingDigest));
        saltedData[1] = etalonTick.saltedSpectrumDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedSpectrumDigest);
        saltedData[1] = etalonTick.saltedUniverseDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedUniverseDigest);
        saltedData[1] = etalonTick.saltedComputerDigest;
        KangarooTwelve64To32(saltedData, &broadcastTick.tick.saltedComputerDigest);

        unsigned char digest[32];
        KangarooTwelve(&broadcastTick.tick, sizeof(Tick) - SIGNATURE_SIZE, digest, sizeof(digest));
        broadcastTick.tick.computorIndex ^= BroadcastTick::type;
        sign(computorSubseeds[ownComputorIndicesMapping[i]].m256i_u8, computorPublicKeys[ownComputorIndicesMapping[i]].m256i_u8, digest, broadcastTick.tick.signature);

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
                saltedData[1].m256i_u64[0] = resourceTestingDigest;
                KangarooTwelve(saltedData, 32 + sizeof(resourceTestingDigest), &saltedDigest, sizeof(resourceTestingDigest));
                if (tick->saltedResourceTestingDigest == saltedDigest.m256i_u64[0])
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
                                tickNumberOfComputors++;
                                // to avoid submitting invalid votes (eg: all zeroes with valid signature)
                                // only count votes that matched etalonTick
                                voteCounter.registerNewVote(tick->tick, tick->computorIndex);
                            }
                        }
                    }
                }
            }
        }
        ts.ticks.releaseLock(i);
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
        // - refresh the network (try to resolve bad topology)
        if ((mainAuxStatus & 1) && (AUTO_FORCE_NEXT_TICK_THRESHOLD != 0))
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

static void tickProcessor(void*)
{
    enableAVX();
    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);

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
                    while (requestPersistingNodeState) _mm_pause();
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
            bs->CopyMem(&nextTickData, &ts.tickData[nextTickIndex], sizeof(TickData));
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
                        && __rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] > TARGET_TICK_DURATION * 5 * frequency / 1000)
                    {
                        // If we don't have enough txs data for the next tick, and next tick digest is unknown (not reach quorum)
                        // and tick duration exceed 5*TARGET_TICK_DURATION, then this tick is forced to be empty.
                        ts.tickData.acquireLock();
                        ts.tickData[nextTickIndex].epoch = 0;
                        ts.tickData.releaseLock();
                        nextTickData.epoch = 0;

                        numberOfNextTickTransactions = 0;
                        numberOfKnownNextTickTransactions = 0;
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
                    }

                    if (nextTickData.epoch == system.epoch)
                    {
                        if (!targetNextTickDataDigestIsKnown)
                        {
                            // if this node is faster than most, targetNextTickDataDigest is unknown at this point because of lack of votes
                            // Thus, expectedNextTickTransactionDigest it not updated yet
                            KangarooTwelve(&nextTickData, sizeof(TickData), &etalonTick.expectedNextTickTransactionDigest, 32);
                        }
                    }
                    else
                    {
                        etalonTick.expectedNextTickTransactionDigest = m256i::zero();
                    }

                    if (system.tick > system.latestCreatedTick || system.tick == system.initialTick)
                    {
                        if (mainAuxStatus & 1)
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
                                ts.tickData[nextTickIndex].epoch = 0;
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
                                // if ((dayIndex == 738570 + system.epoch * 7 && etalonTick.hour >= 12)
                                //     || dayIndex > 738570 + system.epoch * 7)
                                if (system.tick - system.initialTick >= TESTNET_EPOCH_DURATION)
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

                                checkAndSwitchMiningPhase();

                                if (epochTransitionState == 1)
                                {
                                    // seamless epoch transistion
#ifndef NDEBUG
                                    addDebugMessage(L"Starting epoch transition");
                                    {
                                        CHAR16 dbgMsgBuf[300];
                                        CHAR16 digestChars[60 + 1];
                                        getIdentity(score->currentRandomSeed.m256i_u8, digestChars, true);
                                        setText(dbgMsgBuf, L"Old mining seed: ");
                                        appendText(dbgMsgBuf, digestChars);
                                        addDebugMessage(dbgMsgBuf);
                                    }
#endif

                                    // wait until all request processors are in waiting state
                                    while (epochTransitionWaitingRequestProcessors < nRequestProcessorIDs)
                                    {
                                        _mm_pause();
                                    }

                                    // end current epoch
                                    endEpoch();

                                    // instruct main loop to save system and wait until it is done
                                    systemMustBeSaved = true;
                                    while (systemMustBeSaved)
                                    {
                                        _mm_pause();
                                    }
                                    epochTransitionState = 2;

#ifndef NDEBUG
                                    addDebugMessage(L"Calling beginEpoch1of2()"); // TODO: remove after testing
#endif
                                    beginEpoch();
                                    setNewMiningSeed();
#ifndef NDEBUG
                                    addDebugMessage(L"Finished beginEpoch2of2()"); // TODO: remove after testing
                                    {
                                        CHAR16 dbgMsgBuf[300];
                                        CHAR16 digestChars[60 + 1];
                                        getIdentity(score->currentRandomSeed.m256i_u8, digestChars, true);
                                        setText(dbgMsgBuf, L"New mining seed: ");
                                        appendText(dbgMsgBuf, digestChars);
                                        addDebugMessage(dbgMsgBuf);
                                    }
#endif

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
                                    while (computerMustBeSaved || universeMustBeSaved || spectrumMustBeSaved)
                                    {
                                        _mm_pause();
                                    }

                                    // update etalon tick
                                    etalonTick.epoch++;
                                    etalonTick.tick++;
                                    etalonTick.saltedSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
                                    getUniverseDigest(etalonTick.saltedUniverseDigest);
                                    getComputerDigest(etalonTick.saltedComputerDigest);

                                    epochTransitionState = 0;

#ifndef NDEBUG
                                    addDebugMessage(L"Finished epoch transition");
#endif
                                }
                                ASSERT(epochTransitionWaitingRequestProcessors >= 0 && epochTransitionWaitingRequestProcessors <= nRequestProcessorIDs);

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

static void emptyCallback(EFI_EVENT Event, void* Context)
{
}

static void shutdownCallback(EFI_EVENT Event, void* Context)
{
    bs->CloseEvent(Event);
}

static void contractProcessorShutdownCallback(EFI_EVENT Event, void* Context)
{
    bs->CloseEvent(Event);

    contractProcessorState = 0;
}

// directory: source directory to load the file. Default: NULL - load from root dir /
// forceLoadFromFile: when loading node states from file, we want to make sure it load from file and ignore constructionEpoch == system.epoch case
static bool loadComputer(CHAR16* directory, bool forceLoadFromFile)
{
    logToConsole(L"Loading contract files ...");
    setText(message, L"Loaded SC: ");
    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        if (contractDescriptions[contractIndex].constructionEpoch == system.epoch && !forceLoadFromFile)
        {
            bs->SetMem(contractStates[contractIndex], contractDescriptions[contractIndex].stateSize, 0);
        }
        else
        {
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 9] = contractIndex / 1000 + L'0';
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 8] = (contractIndex % 1000) / 100 + L'0';
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 7] = (contractIndex % 100) / 10 + L'0';
            CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 6] = contractIndex % 10 + L'0';
            long long loadedSize = load(CONTRACT_FILE_NAME, contractDescriptions[contractIndex].stateSize, contractStates[contractIndex], directory);
            if (loadedSize != contractDescriptions[contractIndex].stateSize)
            {
                if (system.epoch < contractDescriptions[contractIndex].constructionEpoch && contractDescriptions[contractIndex].stateSize >= sizeof(IPO))
                {
                    setMem(contractStates[contractIndex], contractDescriptions[contractIndex].stateSize, 0);
                    appendText(message, L"(");
                    appendText(message, CONTRACT_FILE_NAME);
                    appendText(message, L" not loaded but initialized with zeros for IPO) ");
                }
                else
                {
                    logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);
                    return false;
                }
            }
            else
            {
                appendText(message, CONTRACT_FILE_NAME);
                appendText(message, L" ");
            }
        }
    }
    logToConsole(message);
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

static bool initialize()
{
    enableAVX();

#if defined (__AVX512F__) && !GENERIC_K12
    initAVX512KangarooTwelveConstants();
#endif
#if defined (__AVX512F__)
    initAVX512FourQConstants();
#endif

    getPublicKeyFromIdentity((const unsigned char*)OPERATOR, operatorPublicKey.m256i_u8);
    if (isZero(operatorPublicKey))
    {
        operatorPublicKey.setRandomValue();
    }

    for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
    {
        if (!getSubseed(computorSeeds[i], computorSubseeds[i].m256i_u8))
        {
            return false;
        }
        getPrivateKey(computorSubseeds[i].m256i_u8, computorPrivateKeys[i].m256i_u8);
        getPublicKey(computorPrivateKeys[i].m256i_u8, computorPublicKeys[i].m256i_u8);
    }

    getPublicKeyFromIdentity((const unsigned char*)ARBITRATOR, (unsigned char*)&arbitratorPublicKey);

    initTimeStampCounter();

    bs->SetMem(&tickTicks, sizeof(tickTicks), 0);

    bs->SetMem(processors, sizeof(processors), 0);
    bs->SetMem(peers, sizeof(peers), 0);
    bs->SetMem(publicPeers, sizeof(publicPeers), 0);

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
        if (status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * MAX_TRANSACTION_SIZE, (void**)&entityPendingTransactions))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, SPECTRUM_CAPACITY * MAX_TRANSACTION_SIZE);
            return false;
        }
        else if (status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * 32ULL, (void**)&entityPendingTransactionDigests))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, SPECTRUM_CAPACITY * 32ULL);

            return false;
        }
        if (status = bs->AllocatePool(EfiRuntimeServicesData, NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * MAX_TRANSACTION_SIZE, (void**)&computorPendingTransactions))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * MAX_TRANSACTION_SIZE);
            return false;
        }
        else if (status = bs->AllocatePool(EfiRuntimeServicesData, NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * 32ULL, (void**)&computorPendingTransactionDigests))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, NUMBER_OF_COMPUTORS * MAX_NUMBER_OF_PENDING_TRANSACTIONS_PER_COMPUTOR * 32ULL);

            return false;
        }
        bs->SetMem(spectrumChangeFlags, sizeof(spectrumChangeFlags), 0);

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
            if (status = bs->AllocatePool(EfiRuntimeServicesData, size, (void**)&contractStates[contractIndex]))
            {
                logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, size);

                return false;
            }
        }

        if (status = bs->AllocatePool(EfiRuntimeServicesData, sizeof(*score), (void**)&score))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, sizeof(*score));
            return false;
        }
        setMem(score, sizeof(*score), 0);

        bs->SetMem(solutionThreshold, sizeof(int) * MAX_NUMBER_EPOCH, 0);
        if (status = bs->AllocatePool(EfiRuntimeServicesData, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, (void**)&minerSolutionFlags))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, NUMBER_OF_MINER_SOLUTION_FLAGS / 8);

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
        bs->SetMem(&system, sizeof(system), 0);
        load(SYSTEM_FILE_NAME, sizeof(system), (unsigned char*)&system);
        system.version = VERSION_B;
        system.epoch = EPOCH;
        system.initialHour = 12;
        system.initialDay = 13;
        system.initialMonth = 4;
        system.initialYear = 22;
        if (system.epoch == EPOCH)
        {
            system.initialTick = TICK;
        }
        system.tick = system.initialTick;

        beginEpoch();
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

                setNumber(message, SPECTRUM_CAPACITY * sizeof(::Entity), TRUE);
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
            loadComputer();
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
        setNewMiningSeed();
    }    
    score->loadScoreCache(system.epoch);

    logToConsole(L"Allocating buffers ...");
    if ((status = bs->AllocatePool(EfiRuntimeServicesData, 536870912, (void**)&dejavu0))
        || (status = bs->AllocatePool(EfiRuntimeServicesData, 536870912, (void**)&dejavu1)))
    {
        logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, 536870912);

        return false;
    }
    bs->SetMem((void*)dejavu0, 536870912, 0);
    bs->SetMem((void*)dejavu1, 536870912, 0);

    if (status = bs->AllocatePool(EfiRuntimeServicesData, REQUEST_QUEUE_BUFFER_SIZE, (void**)&requestQueueBuffer))
    {
        logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, REQUEST_QUEUE_BUFFER_SIZE);
        return false;
    }
    else if (status = bs->AllocatePool(EfiRuntimeServicesData, RESPONSE_QUEUE_BUFFER_SIZE, (void**)&responseQueueBuffer))
    {
        logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, RESPONSE_QUEUE_BUFFER_SIZE);

        return false;
    }

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        peers[i].receiveData.FragmentCount = 1;
        peers[i].transmitData.FragmentCount = 1;
        if (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, &peers[i].receiveBuffer))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, BUFFER_SIZE);

            return false;
        }
        else if (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, &peers[i].transmitData.FragmentTable[0].FragmentBuffer))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, BUFFER_SIZE);

            return false;
        }
        else if (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, (void**)&peers[i].dataToTransmit))
        {
            logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, BUFFER_SIZE);

            return false;
        }
        if ((status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].connectAcceptToken.CompletionToken.Event))
            || (status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].receiveToken.CompletionToken.Event))
            || (status = bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, emptyCallback, NULL, &peers[i].transmitToken.CompletionToken.Event)))
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
            publicPeers[numberOfPublicPeers - 1].isVerified = true;
    }

    logToConsole(L"Init TCP...");
    if (!initTcp4(PORT))
        return false;

    emptyTickResolver.clock = 0;
    emptyTickResolver.tick = 0;
    emptyTickResolver.lastTryClock = 0;
    
    return true;
}

static void deinitialize()
{
    deinitTcp4();

    bs->SetMem(computorSeeds, sizeof(computorSeeds), 0);
    bs->SetMem(computorSubseeds, sizeof(computorSubseeds), 0);
    bs->SetMem(computorPrivateKeys, sizeof(computorPrivateKeys), 0);
    bs->SetMem(computorPublicKeys, sizeof(computorPublicKeys), 0);

    if (root)
    {
        root->Close(root);
    }

    deinitAssets();
    deinitSpectrum();
    deinitCommonBuffers();

    logger.deinitLogging();

#if ADDON_TX_STATUS_REQUEST
    deinitTxStatusRequestAddOn();
#endif

    if (contractStateChangeFlags)
    {
        bs->FreePool(contractStateChangeFlags);
    }
    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        if (contractStates[contractIndex])
        {
            bs->FreePool(contractStates[contractIndex]);
        }
    }

    if (computorPendingTransactionDigests)
    {
        bs->FreePool(computorPendingTransactionDigests);
    }
    if (computorPendingTransactions)
    {
        bs->FreePool(computorPendingTransactions);
    }
    if (entityPendingTransactionDigests)
    {
        bs->FreePool(entityPendingTransactionDigests);
    }
    if (entityPendingTransactions)
    {
        bs->FreePool(entityPendingTransactions);
    }
    ts.deinit();

    if (score)
    {
        bs->FreePool(score);
    }
    if (minerSolutionFlags)
    {
        bs->FreePool(minerSolutionFlags);
    }

    if (dejavu0)
    {
        bs->FreePool((void*)dejavu0);
    }
    if (dejavu1)
    {
        bs->FreePool((void*)dejavu1);
    }

    if (requestQueueBuffer)
    {
        bs->FreePool(requestQueueBuffer);
    }
    if (responseQueueBuffer)
    {
        bs->FreePool(responseQueueBuffer);
    }

    for (unsigned int processorIndex = 0; processorIndex < MAX_NUMBER_OF_PROCESSORS; processorIndex++)
    {
        if (processors[processorIndex].buffer)
        {
            bs->FreePool(processors[processorIndex].buffer);
        }
    }

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].receiveBuffer)
        {
            bs->FreePool(peers[i].receiveBuffer);
        }
        if (peers[i].transmitData.FragmentTable[0].FragmentBuffer)
        {
            bs->FreePool(peers[i].transmitData.FragmentTable[0].FragmentBuffer);
        }
        if (peers[i].dataToTransmit)
        {
            bs->FreePool(peers[i].dataToTransmit);

            bs->CloseEvent(peers[i].connectAcceptToken.CompletionToken.Event);
            bs->CloseEvent(peers[i].receiveToken.CompletionToken.Event);
            bs->CloseEvent(peers[i].transmitToken.CompletionToken.Event);
        }
    }
}

static void logInfo()
{
    unsigned long long numberOfWaitingBytes = 0;

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        if (peers[i].tcp4Protocol)
        {
            numberOfWaitingBytes += peers[i].dataToTransmitSize;
        }
    }

    unsigned int numberOfVerifiedPublicPeers = 0;

    for (unsigned int i = 0; i < numberOfPublicPeers; i++)
    {
        if (publicPeers[i].isVerified)
        {
            numberOfVerifiedPublicPeers++;
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
    appendNumber(message, numberOfVerifiedPublicPeers, TRUE);
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
    appendText(message, L" s | Indices = ");
    if (!numberOfOwnComputorIndices)
    {
        appendText(message, L"?.");
    }
    else
    {
        appendText(message, L"[Owning ");
        appendNumber(message, numberOfOwnComputorIndices, false);
        appendText(message, L" indices]");
        // const CHAR16 alphabet[26][2] = { L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z" };
        // for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
        // {
        //     appendText(message, alphabet[ownComputorIndices[i] / 26]);
        //     appendText(message, alphabet[ownComputorIndices[i] % 26]);
        //     if (i < (unsigned int)(numberOfOwnComputorIndices - 1))
        //     {
        //         appendText(message, L"+");
        //     }
        //     else
        //     {
        //         appendText(message, L".");
        //     }
        // }
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
}

static void logHealthStatus()
{
    setText(message, (mainAuxStatus & 1) ? L"MAIN" : L"aux");
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
        appendText(message, L"no errors");
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
}

static void processKeyPresses()
{
    EFI_INPUT_KEY key;
    if (!st->ConIn->ReadKeyStroke(st->ConIn, &key))
    {
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
        case 0x0C: // 
        {
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
            setText(message, L"Auto-save enabled in AUX mode: every ");
            appendNumber(message, TICK_STORAGE_AUTOSAVE_TICK_PERIOD, FALSE);
            appendText(message, L" ticks, next at tick ");
            appendNumber(message, nextPersistingNodeStateTick, FALSE);
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
                setText(message, (mainAuxStatus & 1) ? L"MAIN" : L"aux");
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
        * PAUSE Key
        * By Pressing the PAUSE Key you can toggle the log output
        */
        case 0x48:
        {
            disableConsoleLogging = !disableConsoleLogging;
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

        EFI_STATUS status;

        unsigned int computingProcessorNumber;
        EFI_GUID mpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
        bs->LocateProtocol(&mpServiceProtocolGuid, NULL, (void**)&mpServicesProtocol);
        unsigned long long numberOfAllProcessors, numberOfEnabledProcessors;
        mpServicesProtocol->GetNumberOfProcessors(mpServicesProtocol, &numberOfAllProcessors, &numberOfEnabledProcessors);
        mpServicesProtocol->WhoAmI(mpServicesProtocol, &mainThreadProcessorID); // get the proc Id of main thread (for later use)
        
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
                if (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, &processors[numberOfProcessors].buffer))
                {
                    logStatusAndMemInfoToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__, BUFFER_SIZE);

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

                    bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, shutdownCallback, NULL, &processors[numberOfProcessors].event);
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

            // TODO: remove later
            unsigned long long debugDigestOriginal = 0, debugDigestCurrent = 0;
            unsigned int debugTick = 0;

            KangarooTwelve(contractUserProcedureLocalsSizes, sizeof(contractUserProcedureLocalsSizes), &debugDigestOriginal, sizeof(debugDigestOriginal));

#if TICK_STORAGE_AUTOSAVE_MODE
            // Use random tick offset to reduce risk of several nodes doing auto-save in parallel (which can lead to bad topology and misalignment)
            nextPersistingNodeStateTick = system.tick + random(TICK_STORAGE_AUTOSAVE_TICK_PERIOD) + TICK_STORAGE_AUTOSAVE_TICK_PERIOD / 10;
#endif

            unsigned long long clockTick = 0, systemDataSavingTick = 0, loggingTick = 0, peerRefreshingTick = 0, tickRequestingTick = 0;
            unsigned int tickRequestingIndicator = 0, futureTickRequestingIndicator = 0;
            logToConsole(L"Init complete! Entering main loop ...");
            while (!shutDownNode)
            {
                if (criticalSituation == 1)
                {
                    logToConsole(L"CRITICAL SITUATION #1!!!");
                }

                {
                    // TODO: remove later
                    KangarooTwelve(contractUserProcedureLocalsSizes, sizeof(contractUserProcedureLocalsSizes), &debugDigestCurrent, sizeof(debugDigestCurrent));
                    if (debugDigestOriginal != debugDigestCurrent)
                    {
                        if (debugTick == 0)
                            debugTick = system.tick;
                        setText(message, L"REPORT TO DEVS: contractUserProcedureLocalsSizes changed in tick ");
                        appendNumber(message, debugTick, FALSE);
                        logToConsole(message);
                    }
                }

                const unsigned long long curTimeTick = __rdtsc();

                if (curTimeTick - clockTick >= (frequency >> 1))
                {
                    clockTick = curTimeTick;

                    updateTime();
                }

                if (contractProcessorState == 1)
                {
                    contractProcessorState = 2;
                    bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, contractProcessorShutdownCallback, NULL, &contractProcessorEvent);
                    mpServicesProtocol->StartupThisAP(mpServicesProtocol, Processor::runFunction, contractProcessorIDs[0], contractProcessorEvent, MAX_CONTRACT_ITERATION_DURATION * 1000, &processors[computingProcessorNumber], NULL);
                }
                /*if (!computationProcessorState && (computation || __computation))
                {
                    numberOfAllSCs++;
                    computationProcessorState = 1;
                    bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, shutdownCallback, NULL, &computationProcessorEvent);
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
                        for (unsigned int k = 0; k < numberOfPublicPeers; k++)
                        {
                            if (publicPeers[k].isVerified)
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
                                // randomly select verified public peers
                                const unsigned int publicPeerIndex = random(numberOfPublicPeers);
                                if (publicPeers[publicPeerIndex].isVerified)
                                {
                                    request->peers[j] = publicPeers[publicPeerIndex].address;
                                }
                                else
                                {
                                    j--;
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
                            bs->CopyMem(&peers[i].dataToTransmit[peers[i].dataToTransmitSize], &requestedComputors, requestedComputors.header.size());
                            peers[i].dataToTransmitSize += requestedComputors.header.size();
                            _InterlockedIncrement64(&numberOfDisseminatedRequests);
                        }
                    }

                    // receive and transmit on active connections
                    peerReceiveAndTransmit(i, salt);

                    // reconnect if this peer slot has no active connection
                    peerReconnectIfInactive(i, PORT);
                }

                if (curTimeTick - systemDataSavingTick >= SYSTEM_DATA_SAVING_PERIOD * frequency / 1000)
                {
                    systemDataSavingTick = curTimeTick;

                    saveSystem();
                    score->saveScoreCache(system.epoch);
                }

                if (curTimeTick - peerRefreshingTick >= PEER_REFRESHING_PERIOD * frequency / 1000)
                {
                    peerRefreshingTick = curTimeTick;

                    for (unsigned int i = 0; i < (NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS) / 4; i++)
                    {
                        closePeer(&peers[random(NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS)]);
                    }
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
                        bs->SetMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const Tick* tsCompTicks = ts.ticks.getByTickInCurrentEpoch(system.tick);
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            if (tsCompTicks[i].epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
                    }
                    tickRequestingIndicator = gTickTotalNumberOfComputors;
                    if (futureTickRequestingIndicator == gFutureTickTotalNumberOfComputors
                        && isNewTickPlus1)
                    {
                        requestedQuorumTick.header.randomizeDejavu();
                        requestedQuorumTick.requestQuorumTick.quorumTick.tick = system.tick + 1;
                        bs->SetMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const Tick* tsCompTicks = ts.ticks.getByTickInCurrentEpoch(system.tick + 1);
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            if (tsCompTicks[i].epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
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
                    }
                    if (ts.tickData[system.tick + 2 - system.initialTick].epoch != system.epoch && isNewTickPlus2)
                    {
                        requestedTickData.header.randomizeDejavu();
                        requestedTickData.requestTickData.requestedTickData.tick = system.tick + 2;
                        pushToAny(&requestedTickData.header);
                    }

                    if (requestedTickTransactions.requestedTickTransactions.tick)
                    {
                        requestedTickTransactions.header.randomizeDejavu();
                        pushToAny(&requestedTickTransactions.header);

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
                bool nextAutoSaveTickUpdated = false;
                if (mainAuxStatus & 1)
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
                    requestPersistingNodeState = 0;
                    logToConsole(L"Complete saving all node states");
                }
                if (nextAutoSaveTickUpdated)
                {
                    setText(message, L"Auto-save in AUX mode scheduled for tick ");
                    appendNumber(message, nextPersistingNodeStateTick, FALSE);
                    logToConsole(message);
                }
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
                            // also log to debug.log
                            misalignedState = 1;
                        }
                        logToConsole(L"MISALIGNED STATE DETECTED");
                        if (misalignedState == 1)
                        {
                            // print health status and stop repeated logging to debug.log
                            logHealthStatus();
                            misalignedState = 2;
                        }
                    }
                    else
                    {
                        misalignedState = 0;
                    }

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
            }

            saveSystem();
            score->saveScoreCache(system.epoch);

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
