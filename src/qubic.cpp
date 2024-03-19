#include <intrin.h>

#include "network_messages/all.h"

#include "smart_contracts.h"

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

#include "text_output.h"

#include "kangaroo_twelve.h"
#include "four_q.h"
#include "score.h"

#include "network_core/tcp4.h"
#include "network_core/peers.h"

#include "system.h"
#include "assets.h"
#include "logging.h"



////////// Qubic \\\\\\\\\\

#define CONTRACT_STATES_DEPTH 10 // Is derived from MAX_NUMBER_OF_CONTRACTS (=N)
#define TARGET_TICK_DURATION 2500
#define TICK_REQUESTING_PERIOD 500ULL
#define FIRST_TICK_TRANSACTION_OFFSET sizeof(unsigned long long)
#define ISSUANCE_RATE 1000000000000LL
#define MAX_NUMBER_EPOCH 1000ULL
#define MAX_AMOUNT (ISSUANCE_RATE * 1000ULL)
#define MAX_INPUT_SIZE 1024ULL
#define MAX_NUMBER_OF_MINERS 8192
#define NUMBER_OF_MINER_SOLUTION_FLAGS 0x100000000
#define MAX_TRANSACTION_SIZE (MAX_INPUT_SIZE + sizeof(Transaction) + SIGNATURE_SIZE)
#define MAX_MESSAGE_PAYLOAD_SIZE MAX_TRANSACTION_SIZE
#define MAX_NUMBER_OF_TICKS_PER_EPOCH (((((60 * 60 * 24 * 7) / (TARGET_TICK_DURATION / 1000)) + NUMBER_OF_COMPUTORS - 1) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS)
#define MAX_CONTRACT_STATE_SIZE 1073741824
#define MAX_UNIVERSE_SIZE 1073741824
#define MESSAGE_DISSEMINATION_THRESHOLD 1000000000
#define PEER_REFRESHING_PERIOD 120000ULL
#define PORT 21841
#define QUORUM (NUMBER_OF_COMPUTORS * 2 / 3 + 1)
#define SPECTRUM_CAPACITY (1ULL << SPECTRUM_DEPTH) // Must be 2^N
#define SYSTEM_DATA_SAVING_PERIOD 300000ULL
#define TICK_TRANSACTIONS_PUBLICATION_OFFSET 2 // Must be only 2
#define MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET 3 // Must be 3+
#define TIME_ACCURACY 5000
#define TRANSACTION_SPARSENESS 4



typedef struct
{
    EFI_EVENT event;
    Peer* peer;
    void* buffer;
} Processor;




static const unsigned short revenuePoints[1 + 1024] = { 0, 710, 1125, 1420, 1648, 1835, 1993, 2129, 2250, 2358, 2455, 2545, 2627, 2702, 2773, 2839, 2901, 2960, 3015, 3068, 3118, 3165, 3211, 3254, 3296, 3336, 3375, 3412, 3448, 3483, 3516, 3549, 3580, 3611, 3641, 3670, 3698, 3725, 3751, 3777, 3803, 3827, 3851, 3875, 3898, 3921, 3943, 3964, 3985, 4006, 4026, 4046, 4066, 4085, 4104, 4122, 4140, 4158, 4175, 4193, 4210, 4226, 4243, 4259, 4275, 4290, 4306, 4321, 4336, 4350, 4365, 4379, 4393, 4407, 4421, 4435, 4448, 4461, 4474, 4487, 4500, 4512, 4525, 4537, 4549, 4561, 4573, 4585, 4596, 4608, 4619, 4630, 4641, 4652, 4663, 4674, 4685, 4695, 4705, 4716, 4726, 4736, 4746, 4756, 4766, 4775, 4785, 4795, 4804, 4813, 4823, 4832, 4841, 4850, 4859, 4868, 4876, 4885, 4894, 4902, 4911, 4919, 4928, 4936, 4944, 4952, 4960, 4968, 4976, 4984, 4992, 5000, 5008, 5015, 5023, 5031, 5038, 5046, 5053, 5060, 5068, 5075, 5082, 5089, 5096, 5103, 5110, 5117, 5124, 5131, 5138, 5144, 5151, 5158, 5164, 5171, 5178, 5184, 5191, 5197, 5203, 5210, 5216, 5222, 5228, 5235, 5241, 5247, 5253, 5259, 5265, 5271, 5277, 5283, 5289, 5295, 5300, 5306, 5312, 5318, 5323, 5329, 5335, 5340, 5346, 5351, 5357, 5362, 5368, 5373, 5378, 5384, 5389, 5394, 5400, 5405, 5410, 5415, 5420, 5425, 5431, 5436, 5441, 5446, 5451, 5456, 5461, 5466, 5471, 5475, 5480, 5485, 5490, 5495, 5500, 5504, 5509, 5514, 5518, 5523, 5528, 5532, 5537, 5542, 5546, 5551, 5555, 5560, 5564, 5569, 5573, 5577, 5582, 5586, 5591, 5595, 5599, 5604, 5608, 5612, 5616, 5621, 5625, 5629, 5633, 5637, 5642, 5646, 5650, 5654, 5658, 5662, 5666, 5670, 5674, 5678, 5682, 5686, 5690, 5694, 5698, 5702, 5706, 5710, 5714, 5718, 5721, 5725, 5729, 5733, 5737, 5740, 5744, 5748, 5752, 5755, 5759, 5763, 5766, 5770, 5774, 5777, 5781, 5785, 5788, 5792, 5795, 5799, 5802, 5806, 5809, 5813, 5816, 5820, 5823, 5827, 5830, 5834, 5837, 5841, 5844, 5847, 5851, 5854, 5858, 5861, 5864, 5868, 5871, 5874, 5878, 5881, 5884, 5887, 5891, 5894, 5897, 5900, 5904, 5907, 5910, 5913, 5916, 5919, 5923, 5926, 5929, 5932, 5935, 5938, 5941, 5944, 5948, 5951, 5954, 5957, 5960, 5963, 5966, 5969, 5972, 5975, 5978, 5981, 5984, 5987, 5990, 5993, 5996, 5999, 6001, 6004, 6007, 6010, 6013, 6016, 6019, 6022, 6025, 6027, 6030, 6033, 6036, 6039, 6041, 6044, 6047, 6050, 6053, 6055, 6058, 6061, 6064, 6066, 6069, 6072, 6075, 6077, 6080, 6083, 6085, 6088, 6091, 6093, 6096, 6099, 6101, 6104, 6107, 6109, 6112, 6115, 6117, 6120, 6122, 6125, 6128, 6130, 6133, 6135, 6138, 6140, 6143, 6145, 6148, 6151, 6153, 6156, 6158, 6161, 6163, 6166, 6168, 6170, 6173, 6175, 6178, 6180, 6183, 6185, 6188, 6190, 6193, 6195, 6197, 6200, 6202, 6205, 6207, 6209, 6212, 6214, 6216, 6219, 6221, 6224, 6226, 6228, 6231, 6233, 6235, 6238, 6240, 6242, 6244, 6247, 6249, 6251, 6254, 6256, 6258, 6260, 6263, 6265, 6267, 6269, 6272, 6274, 6276, 6278, 6281, 6283, 6285, 6287, 6289, 6292, 6294, 6296, 6298, 6300, 6303, 6305, 6307, 6309, 6311, 6313, 6316, 6318, 6320, 6322, 6324, 6326, 6328, 6330, 6333, 6335, 6337, 6339, 6341, 6343, 6345, 6347, 6349, 6351, 6353, 6356, 6358, 6360, 6362, 6364, 6366, 6368, 6370, 6372, 6374, 6376, 6378, 6380, 6382, 6384, 6386, 6388, 6390, 6392, 6394, 6396, 6398, 6400, 6402, 6404, 6406, 6408, 6410, 6412, 6414, 6416, 6418, 6420, 6421, 6423, 6425, 6427, 6429, 6431, 6433, 6435, 6437, 6439, 6441, 6443, 6444, 6446, 6448, 6450, 6452, 6454, 6456, 6458, 6459, 6461, 6463, 6465, 6467, 6469, 6471, 6472, 6474, 6476, 6478, 6480, 6482, 6483, 6485, 6487, 6489, 6491, 6493, 6494, 6496, 6498, 6500, 6502, 6503, 6505, 6507, 6509, 6510, 6512, 6514, 6516, 6518, 6519, 6521, 6523, 6525, 6526, 6528, 6530, 6532, 6533, 6535, 6537, 6538, 6540, 6542, 6544, 6545, 6547, 6549, 6550, 6552, 6554, 6556, 6557, 6559, 6561, 6562, 6564, 6566, 6567, 6569, 6571, 6572, 6574, 6576, 6577, 6579, 6581, 6582, 6584, 6586, 6587, 6589, 6591, 6592, 6594, 6596, 6597, 6599, 6600, 6602, 6604, 6605, 6607, 6609, 6610, 6612, 6613, 6615, 6617, 6618, 6620, 6621, 6623, 6625, 6626, 6628, 6629, 6631, 6632, 6634, 6636, 6637, 6639, 6640, 6642, 6643, 6645, 6647, 6648, 6650, 6651, 6653, 6654, 6656, 6657, 6659, 6660, 6662, 6663, 6665, 6667, 6668, 6670, 6671, 6673, 6674, 6676, 6677, 6679, 6680, 6682, 6683, 6685, 6686, 6688, 6689, 6691, 6692, 6694, 6695, 6697, 6698, 6699, 6701, 6702, 6704, 6705, 6707, 6708, 6710, 6711, 6713, 6714, 6716, 6717, 6718, 6720, 6721, 6723, 6724, 6726, 6727, 6729, 6730, 6731, 6733, 6734, 6736, 6737, 6739, 6740, 6741, 6743, 6744, 6746, 6747, 6748, 6750, 6751, 6753, 6754, 6755, 6757, 6758, 6760, 6761, 6762, 6764, 6765, 6767, 6768, 6769, 6771, 6772, 6773, 6775, 6776, 6778, 6779, 6780, 6782, 6783, 6784, 6786, 6787, 6788, 6790, 6791, 6793, 6794, 6795, 6797, 6798, 6799, 6801, 6802, 6803, 6805, 6806, 6807, 6809, 6810, 6811, 6813, 6814, 6815, 6816, 6818, 6819, 6820, 6822, 6823, 6824, 6826, 6827, 6828, 6830, 6831, 6832, 6833, 6835, 6836, 6837, 6839, 6840, 6841, 6842, 6844, 6845, 6846, 6848, 6849, 6850, 6851, 6853, 6854, 6855, 6856, 6858, 6859, 6860, 6862, 6863, 6864, 6865, 6867, 6868, 6869, 6870, 6872, 6873, 6874, 6875, 6877, 6878, 6879, 6880, 6882, 6883, 6884, 6885, 6886, 6888, 6889, 6890, 6891, 6893, 6894, 6895, 6896, 6897, 6899, 6900, 6901, 6902, 6904, 6905, 6906, 6907, 6908, 6910, 6911, 6912, 6913, 6914, 6916, 6917, 6918, 6919, 6920, 6921, 6923, 6924, 6925, 6926, 6927, 6929, 6930, 6931, 6932, 6933, 6934, 6936, 6937, 6938, 6939, 6940, 6941, 6943, 6944, 6945, 6946, 6947, 6948, 6950, 6951, 6952, 6953, 6954, 6955, 6957, 6958, 6959, 6960, 6961, 6962, 6963, 6965, 6966, 6967, 6968, 6969, 6970, 6971, 6972, 6974, 6975, 6976, 6977, 6978, 6979, 6980, 6981, 6983, 6984, 6985, 6986, 6987, 6988, 6989, 6990, 6991, 6993, 6994, 6995, 6996, 6997, 6998, 6999, 7000, 7001, 7003, 7004, 7005, 7006, 7007, 7008, 7009, 7010, 7011, 7012, 7013, 7015, 7016, 7017, 7018, 7019, 7020, 7021, 7022, 7023, 7024, 7025, 7026, 7027, 7029, 7030, 7031, 7032, 7033, 7034, 7035, 7036, 7037, 7038, 7039, 7040, 7041, 7042, 7043, 7044, 7046, 7047, 7048, 7049, 7050, 7051, 7052, 7053, 7054, 7055, 7056, 7057, 7058, 7059, 7060, 7061, 7062, 7063, 7064, 7065, 7066, 7067, 7068, 7069, 7070, 7071, 7073, 7074, 7075, 7076, 7077, 7078, 7079, 7080, 7081, 7082, 7083, 7084, 7085, 7086, 7087, 7088, 7089, 7090, 7091, 7092, 7093, 7094, 7095, 7096, 7097, 7098, 7099 };

static volatile int shutDownNode = 0;
static volatile unsigned char mainAuxStatus = 0;
static volatile bool forceRefreshPeerList = false;
static volatile bool forceNextTick = false;
static volatile char criticalSituation = 0;
static volatile bool systemMustBeSaved = false, spectrumMustBeSaved = false, universeMustBeSaved = false, computerMustBeSaved = false;
static volatile unsigned char epochTransitionState = 0;

static m256i operatorPublicKey;
static m256i computorSubseeds[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i computorPrivateKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i computorPublicKeys[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static m256i arbitratorPublicKey;

static struct
{
    RequestResponseHeader header;
    BroadcastComputors broadcastComputors;
} broadcastedComputors;

// data closely related to system
static int solutionPublicationTicks[MAX_NUMBER_OF_SOLUTIONS];
static unsigned long long faultyComputorFlags[(NUMBER_OF_COMPUTORS + 63) / 64];
static unsigned int tickNumberOfComputors = 0, tickTotalNumberOfComputors = 0, futureTickTotalNumberOfComputors = 0;
static unsigned int nextTickTransactionsSemaphore = 0, numberOfNextTickTransactions = 0, numberOfKnownNextTickTransactions = 0;
static unsigned short numberOfOwnComputorIndices;
static unsigned short ownComputorIndices[sizeof(computorSeeds) / sizeof(computorSeeds[0])];
static unsigned short ownComputorIndicesMapping[sizeof(computorSeeds) / sizeof(computorSeeds[0])];

static Tick* ticks = NULL;
static TickData* tickData = NULL;
static volatile char tickDataLock = 0;
static Tick etalonTick;
static TickData nextTickData;
static volatile char tickTransactionsLock = 0;
static unsigned char* tickTransactions = NULL;
static unsigned long long* tickTransactionOffsetsPtr = nullptr;
static unsigned long long nextTickTransactionOffset = FIRST_TICK_TRANSACTION_OFFSET;

static m256i uniqueNextTickTransactionDigests[NUMBER_OF_COMPUTORS];
static unsigned int uniqueNextTickTransactionDigestCounters[NUMBER_OF_COMPUTORS];

static void* reorgBuffer = NULL; // Must be large enough to fit any contract!

static unsigned long long resourceTestingDigest = 0;

static volatile char spectrumLock = 0;
static ::Entity* spectrum = NULL;
static unsigned int numberOfEntities = 0;
static unsigned int numberOfTransactions = 0;
static volatile char entityPendingTransactionsLock = 0;
static unsigned char* entityPendingTransactions = NULL;
static unsigned char* entityPendingTransactionDigests = NULL;
static unsigned int entityPendingTransactionIndices[SPECTRUM_CAPACITY];
static unsigned long long spectrumChangeFlags[SPECTRUM_CAPACITY / (sizeof(unsigned long long) * 8)];
static m256i* spectrumDigests = NULL;

static volatile char computerLock = 0;
static unsigned long long mainLoopNumerator = 0, mainLoopDenominator = 0;
static unsigned char contractProcessorState = 0;
static unsigned int contractProcessorPhase;
static EFI_EVENT contractProcessorEvent;
static m256i originator, invocator;
static long long invocationReward;
static m256i currentContract;
static unsigned char* contractStates[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])];
static m256i contractStateDigests[MAX_NUMBER_OF_CONTRACTS * 2 - 1];
static unsigned long long* contractStateChangeFlags = NULL;
static unsigned long long contractTotalExecutionTicks[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])] = { 0 };
static volatile char contractStateCopyLock = 0;
static char* contractStateCopy = NULL;
static char contractFunctionInputs[MAX_NUMBER_OF_PROCESSORS][65536];
static char* contractFunctionOutputs[MAX_NUMBER_OF_PROCESSORS];
static char executedContractInput[65536];
static char executedContractOutput[RequestResponseHeader::max_size + 1];

static volatile char tickLocks[NUMBER_OF_COMPUTORS];
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
    DATA_LENGTH, INFO_LENGTH,
    NUMBER_OF_INPUT_NEURONS, NUMBER_OF_OUTPUT_NEURONS,
    MAX_INPUT_DURATION, MAX_OUTPUT_DURATION,
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

BroadcastFutureTickData broadcastedFutureTickData;

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




unsigned long long& tickTransactionOffsets(unsigned int tick, unsigned int transaction)
{
    const unsigned int tickIndex = tick - system.initialTick;
    return tickTransactionOffsetsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + transaction];
}

static void logToConsole(const CHAR16* message)
{
    if (disableConsoleLogging)
    {
        return;
    }

    timestampedMessage[0] = (time.Year % 100) / 10 + L'0';
    timestampedMessage[1] = time.Year % 10 + L'0';
    timestampedMessage[2] = time.Month / 10 + L'0';
    timestampedMessage[3] = time.Month % 10 + L'0';
    timestampedMessage[4] = time.Day / 10 + L'0';
    timestampedMessage[5] = time.Day % 10 + L'0';
    timestampedMessage[6] = time.Hour / 10 + L'0';
    timestampedMessage[7] = time.Hour % 10 + L'0';
    timestampedMessage[8] = time.Minute / 10 + L'0';
    timestampedMessage[9] = time.Minute % 10 + L'0';
    timestampedMessage[10] = time.Second / 10 + L'0';
    timestampedMessage[11] = time.Second % 10 + L'0';
    timestampedMessage[12] = ' ';
    timestampedMessage[13] = 0;

    appendNumber(timestampedMessage, tickNumberOfComputors / 100, FALSE);
    appendNumber(timestampedMessage, (tickNumberOfComputors % 100) / 10, FALSE);
    appendNumber(timestampedMessage, tickNumberOfComputors % 10, FALSE);
    appendText(timestampedMessage, L":");
    appendNumber(timestampedMessage, (tickTotalNumberOfComputors - tickNumberOfComputors) / 100, FALSE);
    appendNumber(timestampedMessage, ((tickTotalNumberOfComputors - tickNumberOfComputors) % 100) / 10, FALSE);
    appendNumber(timestampedMessage, (tickTotalNumberOfComputors - tickNumberOfComputors) % 10, FALSE);
    appendText(timestampedMessage, L"(");
    appendNumber(timestampedMessage, futureTickTotalNumberOfComputors / 100, FALSE);
    appendNumber(timestampedMessage, (futureTickTotalNumberOfComputors % 100) / 10, FALSE);
    appendNumber(timestampedMessage, futureTickTotalNumberOfComputors % 10, FALSE);
    appendText(timestampedMessage, L").");
    appendNumber(timestampedMessage, system.tick, FALSE);
    appendText(timestampedMessage, L".");
    appendNumber(timestampedMessage, system.epoch, FALSE);
    appendText(timestampedMessage, L" ");

    appendText(timestampedMessage, message);
    appendText(timestampedMessage, L"\r\n");

    outputStringToConsole(timestampedMessage);
}


static int spectrumIndex(const m256i& publicKey)
{
    if (isZero(publicKey))
    {
        return -1;
    }

    unsigned int index = publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

    ACQUIRE(spectrumLock);

iteration:
    if (spectrum[index].publicKey == publicKey)
    {
        RELEASE(spectrumLock);

        return index;
    }
    else
    {
        if (isZero(spectrum[index].publicKey))
        {
            RELEASE(spectrumLock);

            return -1;
        }
        else
        {
            index = (index + 1) & (SPECTRUM_CAPACITY - 1);

            goto iteration;
        }
    }
}

static long long energy(const int index)
{
    return spectrum[index].incomingAmount - spectrum[index].outgoingAmount;
}

static void increaseEnergy(const m256i& publicKey, long long amount)
{
    if (!isZero(publicKey) && amount >= 0)
    {
        // TODO: numberOfEntities!

        unsigned int index = publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

        ACQUIRE(spectrumLock);

    iteration:
        if (spectrum[index].publicKey == publicKey)
        {
            spectrum[index].incomingAmount += amount;
            spectrum[index].numberOfIncomingTransfers++;
            spectrum[index].latestIncomingTransferTick = system.tick;
        }
        else
        {
            if (isZero(spectrum[index].publicKey))
            {
                spectrum[index].publicKey = publicKey;
                spectrum[index].incomingAmount = amount;
                spectrum[index].numberOfIncomingTransfers = 1;
                spectrum[index].latestIncomingTransferTick = system.tick;
            }
            else
            {
                index = (index + 1) & (SPECTRUM_CAPACITY - 1);

                goto iteration;
            }
        }

        RELEASE(spectrumLock);
    }
}

static bool decreaseEnergy(const int index, long long amount)
{
    if (amount >= 0)
    {
        ACQUIRE(spectrumLock);

        if (energy(index) >= amount)
        {
            spectrum[index].outgoingAmount += amount;
            spectrum[index].numberOfOutgoingTransfers++;
            spectrum[index].latestOutgoingTransferTick = system.tick;

            RELEASE(spectrumLock);

            return true;
        }

        RELEASE(spectrumLock);
    }

    return false;
}

static void enableAVX()
{
    __writecr4(__readcr4() | 0x40000);
    _xsetbv(_XCR_XFEATURE_ENABLED_MASK, _xgetbv(_XCR_XFEATURE_ENABLED_MASK) | (7
#ifdef __AVX512F__
        | 224
#endif
        ));
}

static void getComputerDigest(m256i& digest)
{
    unsigned int digestIndex;
    for (digestIndex = 0; digestIndex < MAX_NUMBER_OF_CONTRACTS; digestIndex++)
    {
        if (contractStateChangeFlags[digestIndex >> 6] & (1ULL << (digestIndex & 63)))
        {
            const unsigned long long size = digestIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]) ? contractDescriptions[digestIndex].stateSize : 0;
            if (!size)
            {
                contractStateDigests[digestIndex] = _mm256_setzero_si256();
            }
            else
            {
                KangarooTwelve(contractStates[digestIndex], (unsigned int)size, &contractStateDigests[digestIndex], 32);
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
        && header->size() >= sizeof(RequestResponseHeader) + sizeof(BroadcastMessage) + SIGNATURE_SIZE)
    {
        const unsigned int messageSize = header->size() - sizeof(RequestResponseHeader);

        bool ok;
        if (isZero(request->sourcePublicKey))
        {
            ok = true;
        }
        else
        {
            m256i digest;
            KangarooTwelve(request, messageSize - SIGNATURE_SIZE, &digest, sizeof(digest));
            ok = verify(request->sourcePublicKey.m256i_u8, digest.m256i_u8, (((const unsigned char*)request) + (messageSize - SIGNATURE_SIZE)));
        }
        if (ok)
        {
            if (header->isDejavuZero())
            {
                const int spectrumIndex = ::spectrumIndex(request->sourcePublicKey);
                if (spectrumIndex >= 0 && energy(spectrumIndex) >= MESSAGE_DISSEMINATION_THRESHOLD)
                {
                    enqueueResponse(NULL, header);
                }
            }

            for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
            {
                if (request->destinationPublicKey == computorPublicKeys[i])
                {
                    const unsigned int messagePayloadSize = messageSize - sizeof(BroadcastMessage) - SIGNATURE_SIZE;
                    if (messagePayloadSize)
                    {
                        unsigned char sharedKeyAndGammingNonce[64];

                        if (isZero(request->sourcePublicKey))
                        {
                            bs->SetMem(sharedKeyAndGammingNonce, 32, 0);
                        }
                        else
                        {
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
                                if (messagePayloadSize >= 32)
                                {
                                    const m256i& solution_nonce = *(m256i*)((unsigned char*)request + sizeof(BroadcastMessage));
                                    unsigned int k;
                                    for (k = 0; k < system.numberOfSolutions; k++)
                                    {
                                        if (solution_nonce == system.solutions[k].nonce
                                            && request->destinationPublicKey == system.solutions[k].computorPublicKey)
                                        {
                                            break;
                                        }
                                    }
                                    if (k == system.numberOfSolutions)
                                    {
                                        unsigned long long solutionScore = (*score)(processorNumber, request->destinationPublicKey, solution_nonce);
                                        const int threshold = (system.epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[system.epoch] : SOLUTION_THRESHOLD_DEFAULT;
                                        if (system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS
                                            && ((solutionScore >= (DATA_LENGTH / 2) + threshold) || (solutionScore <= (DATA_LENGTH / 2) - threshold)))
                                        {
                                            ACQUIRE(solutionsLock);

                                            for (k = 0; k < system.numberOfSolutions; k++)
                                            {
                                                if (solution_nonce == system.solutions[k].nonce
                                                    && request->destinationPublicKey == system.solutions[k].computorPublicKey)
                                                {
                                                    break;
                                                }
                                            }
                                            if (k == system.numberOfSolutions)
                                            {
                                                system.solutions[system.numberOfSolutions].computorPublicKey = request->destinationPublicKey;
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

static void processBroadcastComputors(Peer* peer, RequestResponseHeader* header)
{
    BroadcastComputors* request = header->getPayload<BroadcastComputors>();
    // verify that all address are non-zeroes
    // discard it even if ARB broadcast it
    if (request->computors.epoch > broadcastedComputors.broadcastComputors.computors.epoch)
    {
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            if (request->computors.publicKeys[i] == NULL_ID) // NULL_ID is _mm256_setzero_si256()
            {
                return;
            }
        }
    }
    if (request->computors.epoch > broadcastedComputors.broadcastComputors.computors.epoch)
    {
        unsigned char digest[32];
        KangarooTwelve(request, sizeof(BroadcastComputors) - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify((unsigned char*)&arbitratorPublicKey, digest, request->computors.signature))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            bs->CopyMem(&broadcastedComputors.broadcastComputors.computors, &request->computors, sizeof(Computors));

            if (request->computors.epoch == system.epoch)
            {
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
            }
        }
    }
}

static void processBroadcastTick(Peer* peer, RequestResponseHeader* header)
{
    BroadcastTick* request = header->getPayload<BroadcastTick>();
    if (request->tick.computorIndex < NUMBER_OF_COMPUTORS
        && request->tick.epoch == system.epoch
        && request->tick.tick >= system.tick && request->tick.tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH
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
        if (verify(broadcastedComputors.broadcastComputors.computors.publicKeys[request->tick.computorIndex].m256i_u8, digest, request->tick.signature))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

            ACQUIRE(tickLocks[request->tick.computorIndex]);

            const unsigned int offset = ((request->tick.tick - system.initialTick) * NUMBER_OF_COMPUTORS) + request->tick.computorIndex;
            if (ticks[offset].epoch == system.epoch)
            {
                if (*((unsigned long long*)&request->tick.millisecond) != *((unsigned long long*)&ticks[offset].millisecond)
                    || request->tick.prevSpectrumDigest != ticks[offset].prevSpectrumDigest
                    || request->tick.prevUniverseDigest != ticks[offset].prevUniverseDigest
                    || request->tick.prevComputerDigest != ticks[offset].prevComputerDigest
                    || request->tick.saltedSpectrumDigest != ticks[offset].saltedSpectrumDigest
                    || request->tick.saltedUniverseDigest != ticks[offset].saltedUniverseDigest
                    || request->tick.saltedComputerDigest != ticks[offset].saltedComputerDigest
                    || request->tick.transactionDigest != ticks[offset].transactionDigest
                    || request->tick.expectedNextTickTransactionDigest != ticks[offset].expectedNextTickTransactionDigest)
                {
                    faultyComputorFlags[request->tick.computorIndex >> 6] |= (1ULL << (request->tick.computorIndex & 63));
                }
            }
            else
            {
                bs->CopyMem(&ticks[offset], &request->tick, sizeof(Tick));
            }

            RELEASE(tickLocks[request->tick.computorIndex]);
        }
    }
}

static void processBroadcastFutureTickData(Peer* peer, RequestResponseHeader* header)
{
    BroadcastFutureTickData* request = header->getPayload<BroadcastFutureTickData>();
    if (request->tickData.epoch == system.epoch
        && request->tickData.tick > system.tick && request->tickData.tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH
        && request->tickData.tick % NUMBER_OF_COMPUTORS == request->tickData.computorIndex
        && request->tickData.month >= 1 && request->tickData.month <= 12
        && request->tickData.day >= 1 && request->tickData.day <= ((request->tickData.month == 1 || request->tickData.month == 3 || request->tickData.month == 5 || request->tickData.month == 7 || request->tickData.month == 8 || request->tickData.month == 10 || request->tickData.month == 12) ? 31 : ((request->tickData.month == 4 || request->tickData.month == 6 || request->tickData.month == 9 || request->tickData.month == 11) ? 30 : ((request->tickData.year & 3) ? 28 : 29)))
        && request->tickData.hour <= 23
        && request->tickData.minute <= 59
        && request->tickData.second <= 59
        && request->tickData.millisecond <= 999
        && ms(request->tickData.year, request->tickData.month, request->tickData.day, request->tickData.hour, request->tickData.minute, request->tickData.second, request->tickData.millisecond) <= ms(time.Year - 2000, time.Month, time.Day, time.Hour, time.Minute, time.Second, time.Nanosecond / 1000000) + TIME_ACCURACY)
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
            if (verify(broadcastedComputors.broadcastComputors.computors.publicKeys[request->tickData.computorIndex].m256i_u8, digest, request->tickData.signature))
            {
                if (header->isDejavuZero())
                {
                    enqueueResponse(NULL, header);
                }

                ACQUIRE(tickDataLock);
                if (request->tickData.tick == system.tick + 1 && targetNextTickDataDigestIsKnown)
                {
                    if (!isZero(targetNextTickDataDigest))
                    {
                        unsigned char digest[32];
                        KangarooTwelve(&request->tickData, sizeof(TickData), digest, 32);
                        if (digest == targetNextTickDataDigest)
                        {
                            bs->CopyMem(&tickData[request->tickData.tick - system.initialTick], &request->tickData, sizeof(TickData));
                        }
                    }
                }
                else
                {
                    if (tickData[request->tickData.tick - system.initialTick].epoch == system.epoch)
                    {
                        if (*((unsigned long long*)&request->tickData.millisecond) != *((unsigned long long*)&tickData[request->tickData.tick - system.initialTick].millisecond))
                        {
                            faultyComputorFlags[request->tickData.computorIndex >> 6] |= (1ULL << (request->tickData.computorIndex & 63));
                        }
                        else
                        {
                            for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                            {
                                if (request->tickData.transactionDigests[i] != tickData[request->tickData.tick - system.initialTick].transactionDigests[i])
                                {
                                    faultyComputorFlags[request->tickData.computorIndex >> 6] |= (1ULL << (request->tickData.computorIndex & 63));

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        bs->CopyMem(&tickData[request->tickData.tick - system.initialTick], &request->tickData, sizeof(TickData));
                    }
                }
                RELEASE(tickDataLock);
            }
        }
    }
}

static void processBroadcastTransaction(Peer* peer, RequestResponseHeader* header)
{
    Transaction* request = header->getPayload<Transaction>();
    if (request->amount >= 0 && request->amount <= MAX_AMOUNT
        && request->inputSize <= MAX_INPUT_SIZE && request->inputSize == header->size() - sizeof(RequestResponseHeader) - sizeof(Transaction) - SIGNATURE_SIZE)
    {
        const unsigned int transactionSize = sizeof(Transaction) + request->inputSize + SIGNATURE_SIZE;
        unsigned char digest[32];
        KangarooTwelve(request, transactionSize - SIGNATURE_SIZE, digest, sizeof(digest));
        if (verify(request->sourcePublicKey.m256i_u8, digest, (((const unsigned char*)request) + sizeof(Transaction) + request->inputSize)))
        {
            if (header->isDejavuZero())
            {
                enqueueResponse(NULL, header);
            }

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

            ACQUIRE(tickDataLock);
            if (request->tick == system.tick + 1
                && tickData[request->tick - system.initialTick].epoch == system.epoch)
            {
                KangarooTwelve(request, transactionSize, digest, sizeof(digest));
                for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                {
                    if (digest == tickData[request->tick - system.initialTick].transactionDigests[i])
                    {
                        ACQUIRE(tickTransactionsLock);
                        if (!tickTransactionOffsets(request->tick, i))
                        {
                            if (nextTickTransactionOffset + transactionSize <= FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS))
                            {
                                tickTransactionOffsets(request->tick, i) = nextTickTransactionOffset;
                                bs->CopyMem(&tickTransactions[nextTickTransactionOffset], request, transactionSize);
                                nextTickTransactionOffset += transactionSize;
                            }
                        }
                        RELEASE(tickTransactionsLock);

                        break;
                    }
                }
            }
            RELEASE(tickDataLock);
        }
    }
}

static void processRequestComputors(Peer* peer, RequestResponseHeader* header)
{
    if (broadcastedComputors.broadcastComputors.computors.epoch)
    {
        enqueueResponse(peer, sizeof(broadcastedComputors.broadcastComputors), BroadcastComputors::type, header->dejavu(), &broadcastedComputors.broadcastComputors);
    }
    else
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
}

static void processRequestQuorumTick(Peer* peer, RequestResponseHeader* header)
{
    RequestQuorumTick* request = header->getPayload<RequestQuorumTick>();
    if (request->quorumTick.tick >= system.initialTick && request->quorumTick.tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
    {
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
                const unsigned int offset = ((request->quorumTick.tick - system.initialTick) * NUMBER_OF_COMPUTORS) + computorIndices[index];
                if (ticks[offset].epoch == system.epoch)
                {
                    enqueueResponse(peer, sizeof(Tick), BroadcastTick::type, header->dejavu(), &ticks[offset]);
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
    if (request->requestedTickData.tick > system.initialTick && request->requestedTickData.tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH
        && tickData[request->requestedTickData.tick - system.initialTick].epoch == system.epoch)
    {
        enqueueResponse(peer, sizeof(TickData), BroadcastFutureTickData::type, header->dejavu(), &tickData[request->requestedTickData.tick - system.initialTick]);
    }
    else
    {
        enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
    }
}

static void processRequestTickTransactions(Peer* peer, RequestResponseHeader* header)
{
    RequestedTickTransactions* request = header->getPayload<RequestedTickTransactions>();
    if (request->tick >= system.initialTick && request->tick < system.initialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH)
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

            if (!(request->transactionFlags[tickTransactionIndices[index] >> 3] & (1 << (tickTransactionIndices[index] & 7)))
                && tickTransactionOffsets(request->tick, tickTransactionIndices[index]))
            {
                const Transaction* transaction = (Transaction*)&tickTransactions[tickTransactionOffsets(request->tick, tickTransactionIndices[index])];
                enqueueResponse(peer, sizeof(Transaction) + transaction->inputSize + SIGNATURE_SIZE, BROADCAST_TRANSACTION, header->dejavu(), (void*)transaction);
            }

            tickTransactionIndices[index] = tickTransactionIndices[--numberOfTickTransactions];
        }
    }
    enqueueResponse(peer, 0, EndResponse::type, header->dejavu(), NULL);
}

static void processRequestCurrentTickInfo(Peer* peer, RequestResponseHeader* header)
{
    CurrentTickInfo currentTickInfo;

    if (broadcastedComputors.broadcastComputors.computors.epoch)
    {
        unsigned long long tickDuration = (__rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1]) / frequency;
        if (tickDuration > 0xFFFF)
        {
            tickDuration = 0xFFFF;
        }
        currentTickInfo.tickDuration = (unsigned short)tickDuration;

        currentTickInfo.epoch = system.epoch;
        currentTickInfo.tick = system.tick;
        currentTickInfo.numberOfAlignedVotes = tickNumberOfComputors;
        currentTickInfo.numberOfMisalignedVotes = (tickTotalNumberOfComputors - tickNumberOfComputors);
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

        int sibling = respondedEntity.spectrumIndex;
        unsigned int spectrumDigestInputOffset = 0;
        for (unsigned int j = 0; j < SPECTRUM_DEPTH; j++)
        {
            respondedEntity.siblings[j] = spectrumDigests[spectrumDigestInputOffset + (sibling ^ 1)];
            spectrumDigestInputOffset += (SPECTRUM_CAPACITY >> j);
            sibling >>= 1;
        }
    }

    enqueueResponse(peer, sizeof(respondedEntity), RESPOND_ENTITY, header->dejavu(), &respondedEntity);
}

static void processRequestContractIPO(Peer* peer, RequestResponseHeader* header)
{
    RespondContractIPO respondContractIPO;

    RequestContractIPO* request = header->getPayload<RequestContractIPO>();
    respondContractIPO.contractIndex = request->contractIndex;
    respondContractIPO.tick = system.tick;
    if (request->contractIndex >= sizeof(contractDescriptions) / sizeof(contractDescriptions[0])
        || system.epoch >= contractDescriptions[request->contractIndex].constructionEpoch)
    {
        bs->SetMem(respondContractIPO.publicKeys, sizeof(respondContractIPO.publicKeys), 0);
        bs->SetMem(respondContractIPO.prices, sizeof(respondContractIPO.prices), 0);
    }
    else
    {
        IPO* ipo = (IPO*)contractStates[request->contractIndex];
        bs->CopyMem(respondContractIPO.publicKeys, ipo->publicKeys, sizeof(respondContractIPO.publicKeys));
        bs->CopyMem(respondContractIPO.prices, ipo->prices, sizeof(respondContractIPO.prices));
    }

    enqueueResponse(peer, sizeof(respondContractIPO), RespondContractIPO::type, header->dejavu(), &respondContractIPO);
}

static void processRequestContractFunction(Peer* peer, const unsigned long long processorNumber, RequestResponseHeader* header)
{
    // TODO: Invoked function may enter endless loop, so a timeout (and restart) is required for request processing threads
    // TODO: Enable parallel execution of contract functions

    RespondContractFunction* response = (RespondContractFunction*)contractFunctionOutputs[processorNumber];

    RequestContractFunction* request = header->getPayload<RequestContractFunction>();
    executedContractIndex = request->contractIndex;
    if (header->size() != sizeof(RequestResponseHeader) + sizeof(RequestContractFunction) + request->inputSize
        || !executedContractIndex || executedContractIndex >= sizeof(contractDescriptions) / sizeof(contractDescriptions[0])
        || system.epoch < contractDescriptions[executedContractIndex].constructionEpoch
        || !contractUserFunctions[executedContractIndex][request->inputType])
    {
        enqueueResponse(peer, 0, response->type, header->dejavu(), NULL);
    }
    else
    {
        ::originator = _mm256_setzero_si256();
        ::invocator = ::originator;
        ::invocationReward = 0;
        currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

        bs->SetMem(&contractFunctionInputs[processorNumber], sizeof(contractFunctionInputs[processorNumber]), 0);
        bs->CopyMem(&contractFunctionInputs[processorNumber], (((unsigned char*)request) + sizeof(RequestContractFunction)), request->inputSize);
        ACQUIRE(contractStateCopyLock); // A single contract state buffer is used because of lack of memory, if we could afford we would use a buffer per request processing thread
        bs->CopyMem(contractStateCopy, contractStates[executedContractIndex], contractDescriptions[executedContractIndex].stateSize);
        contractUserFunctions[executedContractIndex][request->inputType](contractStateCopy, &contractFunctionInputs[processorNumber], response);
        RELEASE(contractStateCopyLock);

        enqueueResponse(peer, contractUserFunctionOutputSizes[executedContractIndex][request->inputType], response->type, header->dejavu(), response);
    }
}

static void processRequestSystemInfo(Peer* peer, RequestResponseHeader* header)
{
    RespondSystemInfo respondedSystemInfo;

    respondedSystemInfo.version = system.version;
    respondedSystemInfo.epoch = system.epoch;
    respondedSystemInfo.tick = system.tick;
    respondedSystemInfo.initialTick = system.initialTick;
    respondedSystemInfo.latestCreatedTick = system.latestLedTick;

    respondedSystemInfo.initialMillisecond = system.initialMillisecond;
    respondedSystemInfo.initialSecond = system.initialSecond;
    respondedSystemInfo.initialMinute = system.initialMinute;
    respondedSystemInfo.initialHour = system.initialHour;
    respondedSystemInfo.initialDay = system.initialDay;
    respondedSystemInfo.initialMonth = system.initialMonth;
    respondedSystemInfo.initialYear = system.initialYear;

    respondedSystemInfo.numberOfEntities = numberOfEntities;
    respondedSystemInfo.numberOfTransactions = numberOfTransactions;

    respondedSystemInfo.randomMiningSeed = score->initialRandomSeed;

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

            case SPECIAL_COMMAND_GET_PROPOSAL_AND_BALLOT_REQUEST:
            {
                SpecialCommandGetProposalAndBallotRequest* _request = header->getPayload<SpecialCommandGetProposalAndBallotRequest>();
                if (_request->computorIndex < NUMBER_OF_COMPUTORS)
                {
                    SpecialCommandGetProposalAndBallotResponse response;

                    response.everIncreasingNonceAndCommandType = (_request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) | (SPECIAL_COMMAND_GET_PROPOSAL_AND_BALLOT_RESPONSE << 56);
                    response.computorIndex = _request->computorIndex;
                    *((short*)response.padding) = 0;
                    bs->CopyMem(&response.proposal, &system.proposals[_request->computorIndex], sizeof(ComputorProposal));
                    bs->CopyMem(&response.ballot, &system.ballots[_request->computorIndex], sizeof(ComputorBallot));

                    enqueueResponse(peer, sizeof(response), SpecialCommand::type, header->dejavu(), &response);
                }
            }
            break;

            case SPECIAL_COMMAND_SET_PROPOSAL_AND_BALLOT_REQUEST:
            {
                SpecialCommandSetProposalAndBallotRequest* _request = header->getPayload<SpecialCommandSetProposalAndBallotRequest>();
                if (_request->computorIndex < NUMBER_OF_COMPUTORS)
                {
                    bs->CopyMem(&system.proposals[_request->computorIndex], &_request->proposal, sizeof(ComputorProposal));
                    bs->CopyMem(&system.ballots[_request->computorIndex], &_request->ballot, sizeof(ComputorBallot));

                    SpecialCommandSetProposalAndBallotResponse response;

                    response.everIncreasingNonceAndCommandType = (_request->everIncreasingNonceAndCommandType & 0xFFFFFFFFFFFFFF) | (SPECIAL_COMMAND_SET_PROPOSAL_AND_BALLOT_RESPONSE << 56);
                    response.computorIndex = _request->computorIndex;
                    *((short*)response.padding) = 0;

                    enqueueResponse(peer, sizeof(response), SpecialCommand::type, header->dejavu(), &response);
                }
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
                mainAuxStatus = _request->mainModeFlag;
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
                copyMem(&newTime, &_request->utcTime, sizeof(_request->utcTime)); // caution: response.utcTime is subset of time (smaller size)
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
                copyMem(&response.utcTime, &time, sizeof(response.utcTime)); // caution: response.utcTime is subset of time (smaller size)
                enqueueResponse(peer, sizeof(SpecialCommandSendTime), SpecialCommand::type, header->dejavu(), &response);
            }
            break;
            }
        }
    }
}

static void requestProcessor(void* ProcedureArgument)
{
    enableAVX();

    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);

    Processor* processor = (Processor*)ProcedureArgument;
    RequestResponseHeader* header = (RequestResponseHeader*)processor->buffer;
    while (!shutDownNode)
    {
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
                    processRequestLog(peer, header);
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
                }

                queueProcessingNumerator += __rdtsc() - beginningTick;
                queueProcessingDenominator++;

                _InterlockedIncrement64(&numberOfProcessedRequests);
            }
        }
    }
}

static void __beginFunctionOrProcedure(const unsigned int functionOrProcedureId)
{
    // TODO
}

static void __endFunctionOrProcedure(const unsigned int functionOrProcedureId)
{
    contractStateChangeFlags[functionOrProcedureId >> (22 + 6)] |= (1ULL << ((functionOrProcedureId >> 22) & 63));
}

static void __registerUserFunction(USER_FUNCTION userFunction, unsigned short inputType, unsigned short inputSize, unsigned short outputSize)
{
    contractUserFunctions[executedContractIndex][inputType] = userFunction;
    contractUserFunctionInputSizes[executedContractIndex][inputType] = inputSize;
    contractUserFunctionOutputSizes[executedContractIndex][inputType] = outputSize;
}

static void __registerUserProcedure(USER_PROCEDURE userProcedure, unsigned short inputType, unsigned short inputSize, unsigned short outputSize)
{
    contractUserProcedures[executedContractIndex][inputType] = userProcedure;
    contractUserProcedureInputSizes[executedContractIndex][inputType] = inputSize;
    contractUserFunctionOutputSizes[executedContractIndex][inputType] = outputSize;
}

static const m256i& __arbitrator()
{
    return arbitratorPublicKey;
}

static long long __burn(long long amount)
{
    if (amount < 0 || amount > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    const int index = spectrumIndex(currentContract);

    if (index < 0)
    {
        return -amount;
    }

    const long long remainingAmount = energy(index) - amount;

    if (remainingAmount < 0)
    {
        return remainingAmount;
    }

    if (decreaseEnergy(index, amount))
    {
        Contract0State* contract0State = (Contract0State*)contractStates[0];
        contract0State->contractFeeReserves[executedContractIndex] += amount;

        const Burning burning = { currentContract , amount };
        logBurning(burning);
    }

    return remainingAmount;
}

static const m256i& __computor(unsigned short computorIndex)
{
    return broadcastedComputors.broadcastComputors.computors.publicKeys[computorIndex % NUMBER_OF_COMPUTORS];
}

static unsigned char __day()
{
    return etalonTick.day;
}

static unsigned char __dayOfWeek(unsigned char year, unsigned char month, unsigned char day)
{
    return dayIndex(year, month, day) % 7;
}

static unsigned short __epoch()
{
    return system.epoch;
}

static bool __getEntity(const m256i& id, ::Entity& entity)
{
    int index = spectrumIndex(id);
    if (index < 0)
    {
        entity.publicKey = id;
        entity.incomingAmount = 0;
        entity.outgoingAmount = 0;
        entity.numberOfIncomingTransfers = 0;
        entity.numberOfOutgoingTransfers = 0;
        entity.latestIncomingTransferTick = 0;
        entity.latestOutgoingTransferTick = 0;

        return false;
    }
    else
    {
        entity.publicKey = spectrum[index].publicKey;
        entity.incomingAmount = spectrum[index].incomingAmount;
        entity.outgoingAmount = spectrum[index].outgoingAmount;
        entity.numberOfIncomingTransfers = spectrum[index].numberOfIncomingTransfers;
        entity.numberOfOutgoingTransfers = spectrum[index].numberOfOutgoingTransfers;
        entity.latestIncomingTransferTick = spectrum[index].latestIncomingTransferTick;
        entity.latestOutgoingTransferTick = spectrum[index].latestOutgoingTransferTick;

        return true;
    }
}

static unsigned char __hour()
{
    return etalonTick.hour;
}

static long long __invocationReward()
{
    return ::invocationReward;
}

static const m256i& __invocator()
{
    return ::invocator;
}

static long long __issueAsset(unsigned long long name, const m256i& issuer, char numberOfDecimalPlaces, long long numberOfShares, unsigned long long unitOfMeasurement)
{
    if (((unsigned char)name) < 'A' || ((unsigned char)name) > 'Z'
        || name > 0xFFFFFFFFFFFFFF)
    {
        return 0;
    }
    for (unsigned int i = 1; i < 7; i++)
    {
        if (!((unsigned char)(name >> (i * 8))))
        {
            while (++i < 7)
            {
                if ((unsigned char)(name >> (i * 8)))
                {
                    return 0;
                }
            }

            break;
        }
    }
    for (unsigned int i = 1; i < 7; i++)
    {
        if (!((unsigned char)(name >> (i * 8)))
            || (((unsigned char)(name >> (i * 8))) >= '0' && ((unsigned char)(name >> (i * 8))) <= '9')
            || (((unsigned char)(name >> (i * 8))) >= 'A' && ((unsigned char)(name >> (i * 8))) <= 'Z'))
        {
            // Do nothing
        }
        else
        {
            return 0;
        }
    }

    if (issuer != currentContract && issuer != __invocator())
    {
        return 0;
    }

    if (numberOfShares <= 0 || numberOfShares > MAX_AMOUNT)
    {
        return 0;
    }

    if (unitOfMeasurement > 0xFFFFFFFFFFFFFF)
    {
        return 0;
    }

    char nameBuffer[7] = { char(name), char(name >> 8), char(name >> 16), char(name >> 24), char(name >> 32), char(name >> 40), char(name >> 48) };
    char unitOfMeasurementBuffer[7] = { char(unitOfMeasurement), char(unitOfMeasurement >> 8), char(unitOfMeasurement >> 16), char(unitOfMeasurement >> 24), char(unitOfMeasurement >> 32), char(unitOfMeasurement >> 40), char(unitOfMeasurement >> 48) };
    int issuanceIndex, ownershipIndex, possessionIndex;
    numberOfShares = issueAsset(issuer, nameBuffer, numberOfDecimalPlaces, unitOfMeasurementBuffer, numberOfShares, executedContractIndex, &issuanceIndex, &ownershipIndex, &possessionIndex);

    return numberOfShares;
}

static unsigned short __millisecond()
{
    return etalonTick.millisecond;
}

static unsigned char __minute()
{
    return etalonTick.minute;
}

static unsigned char __month()
{
    return etalonTick.month;
}

static m256i __nextId(const m256i& currentId)
{
    int index = spectrumIndex(currentId);
    while (++index < SPECTRUM_CAPACITY)
    {
        const m256i& nextId = spectrum[index].publicKey;
        if (!isZero(nextId))
        {
            return nextId;
        }
    }

    return _mm256_setzero_si256();
}

static m256i __originator()
{
    return ::originator;
}

static unsigned char __second()
{
    return etalonTick.second;
}

static void* __scratchpad()
{
    return reorgBuffer;
}

static unsigned int __tick()
{
    return system.tick;
}

static long long __transfer(const m256i& destination, long long amount)
{
    if (amount < 0 || amount > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    const int index = spectrumIndex(currentContract);

    if (index < 0)
    {
        return -amount;
    }

    const long long remainingAmount = energy(index) - amount;

    if (remainingAmount < 0)
    {
        return remainingAmount;
    }

    if (decreaseEnergy(index, amount))
    {
        increaseEnergy(destination, amount);

        const QuTransfer quTransfer = { currentContract , destination , amount };
        logQuTransfer(quTransfer);
    }

    return remainingAmount;
}

static long long __transferShareOwnershipAndPossession(unsigned long long assetName, const m256i& issuer, const m256i& owner, const m256i& possessor, long long numberOfShares, const m256i& newOwnerAndPossessor)
{
    if (numberOfShares <= 0 || numberOfShares > MAX_AMOUNT)
    {
        return -((long long)(MAX_AMOUNT + 1));
    }

    ACQUIRE(universeLock);

    int issuanceIndex = issuer.m256i_u32[0] & (ASSETS_CAPACITY - 1);
iteration:
    if (assets[issuanceIndex].varStruct.issuance.type == EMPTY)
    {
        RELEASE(universeLock);

        return -numberOfShares;
    }
    else
    {
        if (assets[issuanceIndex].varStruct.issuance.type == ISSUANCE
            && ((*((unsigned long long*)assets[issuanceIndex].varStruct.issuance.name)) & 0xFFFFFFFFFFFFFF) == assetName
            && assets[issuanceIndex].varStruct.issuance.publicKey == issuer)
        {
            int ownershipIndex = owner.m256i_u32[0] & (ASSETS_CAPACITY - 1);
        iteration2:
            if (assets[ownershipIndex].varStruct.ownership.type == EMPTY)
            {
                RELEASE(universeLock);

                return -numberOfShares;
            }
            else
            {
                if (assets[ownershipIndex].varStruct.ownership.type == OWNERSHIP
                    && assets[ownershipIndex].varStruct.ownership.issuanceIndex == issuanceIndex
                    && assets[ownershipIndex].varStruct.ownership.publicKey == owner
                    && assets[ownershipIndex].varStruct.ownership.managingContractIndex == executedContractIndex) // TODO: This condition needs extra attention during refactoring!
                {
                    int possessionIndex = possessor.m256i_u32[0] & (ASSETS_CAPACITY - 1);
                iteration3:
                    if (assets[possessionIndex].varStruct.possession.type == EMPTY)
                    {
                        RELEASE(universeLock);

                        return -numberOfShares;
                    }
                    else
                    {
                        if (assets[possessionIndex].varStruct.possession.type == POSSESSION
                            && assets[possessionIndex].varStruct.possession.ownershipIndex == ownershipIndex
                            && assets[possessionIndex].varStruct.possession.publicKey == possessor)
                        {
                            if (assets[possessionIndex].varStruct.possession.managingContractIndex == executedContractIndex) // TODO: This condition needs extra attention during refactoring!
                            {
                                if (assets[possessionIndex].varStruct.possession.numberOfShares >= numberOfShares)
                                {
                                    int destinationOwnershipIndex, destinationPossessionIndex;
                                    transferShareOwnershipAndPossession(ownershipIndex, possessionIndex, newOwnerAndPossessor, numberOfShares, &destinationOwnershipIndex, &destinationPossessionIndex, false);

                                    RELEASE(universeLock);

                                    return assets[possessionIndex].varStruct.possession.numberOfShares;
                                }
                                else
                                {
                                    RELEASE(universeLock);

                                    return assets[possessionIndex].varStruct.possession.numberOfShares - numberOfShares;
                                }
                            }
                            else
                            {
                                RELEASE(universeLock);

                                return -numberOfShares;
                            }
                        }
                        else
                        {
                            possessionIndex = (possessionIndex + 1) & (ASSETS_CAPACITY - 1);

                            goto iteration3;
                        }
                    }
                }
                else
                {
                    ownershipIndex = (ownershipIndex + 1) & (ASSETS_CAPACITY - 1);

                    goto iteration2;
                }
            }
        }
        else
        {
            issuanceIndex = (issuanceIndex + 1) & (ASSETS_CAPACITY - 1);

            goto iteration;
        }
    }

    return 0;
}

static unsigned char __year()
{
    return etalonTick.year;
}

template <typename T>
static m256i __K12(T data)
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

    switch (contractProcessorPhase)
    {
    case INITIALIZE:
    {
        for (executedContractIndex = 1; executedContractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); executedContractIndex++)
        {
            if (system.epoch == contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                ::originator = _mm256_setzero_si256();
                ::invocator = ::originator;
                ::invocationReward = 0;
                currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                const unsigned long long startTick = __rdtsc();
                contractSystemProcedures[executedContractIndex][INITIALIZE](contractStates[executedContractIndex]);
                contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
            }
        }
    }
    break;

    case BEGIN_EPOCH:
    {
        for (executedContractIndex = 1; executedContractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); executedContractIndex++)
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                ::originator = _mm256_setzero_si256();
                ::invocator = ::originator;
                ::invocationReward = 0;
                currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                const unsigned long long startTick = __rdtsc();
                contractSystemProcedures[executedContractIndex][BEGIN_EPOCH](contractStates[executedContractIndex]);
                contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
            }
        }
    }
    break;

    case BEGIN_TICK:
    {
        for (executedContractIndex = 1; executedContractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); executedContractIndex++)
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                ::originator = _mm256_setzero_si256();
                ::invocator = ::originator;
                ::invocationReward = 0;
                currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                const unsigned long long startTick = __rdtsc();
                contractSystemProcedures[executedContractIndex][BEGIN_TICK](contractStates[executedContractIndex]);
                contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
            }
        }
    }
    break;

    case END_TICK:
    {
        for (executedContractIndex = sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); executedContractIndex-- > 1; )
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                ::originator = _mm256_setzero_si256();
                ::invocator = ::originator;
                ::invocationReward = 0;
                currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                const unsigned long long startTick = __rdtsc();
                contractSystemProcedures[executedContractIndex][END_TICK](contractStates[executedContractIndex]);
                contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
            }
        }
    }
    break;

    case END_EPOCH:
    {
        for (executedContractIndex = sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); executedContractIndex-- > 1; )
        {
            if (system.epoch >= contractDescriptions[executedContractIndex].constructionEpoch
                && system.epoch < contractDescriptions[executedContractIndex].destructionEpoch)
            {
                ::originator = _mm256_setzero_si256();
                ::invocator = ::originator;
                ::invocationReward = 0;
                currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                const unsigned long long startTick = __rdtsc();
                contractSystemProcedures[executedContractIndex][END_EPOCH](contractStates[executedContractIndex]);
                contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
            }
        }
    }
    break;
    }
}

static void processTick(unsigned long long processorNumber)
{
    etalonTick.prevResourceTestingDigest = resourceTestingDigest;
    etalonTick.prevSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
    getUniverseDigest(etalonTick.prevUniverseDigest);
    getComputerDigest(etalonTick.prevComputerDigest);

    if (system.tick == system.initialTick)
    {
        contractProcessorPhase = INITIALIZE;
        contractProcessorState = 1;
        while (contractProcessorState)
        {
            _mm_pause();
        }

        contractProcessorPhase = BEGIN_EPOCH;
        contractProcessorState = 1;
        while (contractProcessorState)
        {
            _mm_pause();
        }
    }

    contractProcessorPhase = BEGIN_TICK;
    contractProcessorState = 1;
    while (contractProcessorState)
    {
        _mm_pause();
    }

    ACQUIRE(tickDataLock);
    bs->CopyMem(&nextTickData, &tickData[system.tick - system.initialTick], sizeof(TickData));
    RELEASE(tickDataLock);
    if (nextTickData.epoch == system.epoch)
    {
        bs->SetMem(entityPendingTransactionIndices, sizeof(entityPendingTransactionIndices), 0);
        // reset solution task queue
        score->resetTaskQueue();
        // pre-scan any solution tx and add them to solution task queue
        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(nextTickData.transactionDigests[transactionIndex]))
            {
                if (tickTransactionOffsets(system.tick, transactionIndex))
                {
                    Transaction* transaction = (Transaction*)&tickTransactions[tickTransactionOffsets(system.tick, transactionIndex)];
                    const int spectrumIndex = ::spectrumIndex(transaction->sourcePublicKey);
                    if (spectrumIndex >= 0
                        && !entityPendingTransactionIndices[spectrumIndex])
                    {
                        if (transaction->destinationPublicKey == arbitratorPublicKey)
                        {
                            if (!transaction->amount
                                && transaction->inputSize == 32
                                && !transaction->inputType)
                            {
                                const m256i& solution_nonce = *(m256i*)((unsigned char*)transaction + sizeof(Transaction));
                                m256i data[2] = { transaction->sourcePublicKey, solution_nonce };
                                static_assert(sizeof(data) == 2 * 32, "Unexpected array size");
                                unsigned int flagIndex;
                                KangarooTwelve(data, sizeof(data), &flagIndex, sizeof(flagIndex));
                                if (!(minerSolutionFlags[flagIndex >> 6] & (1ULL << (flagIndex & 63))))
                                {
                                    score->addTask(transaction->sourcePublicKey, solution_nonce);
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

        for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
        {
            if (!isZero(nextTickData.transactionDigests[transactionIndex]))
            {
                if (tickTransactionOffsets(system.tick, transactionIndex))
                {
                    Transaction* transaction = (Transaction*)&tickTransactions[tickTransactionOffsets(system.tick, transactionIndex)];
                    const int spectrumIndex = ::spectrumIndex(transaction->sourcePublicKey);
                    if (spectrumIndex >= 0
                        && !entityPendingTransactionIndices[spectrumIndex])
                    {
                        entityPendingTransactionIndices[spectrumIndex] = 1;

                        numberOfTransactions++;
                        if (decreaseEnergy(spectrumIndex, transaction->amount))
                        {
                            increaseEnergy(transaction->destinationPublicKey, transaction->amount);
                            if (transaction->amount)
                            {
                                const QuTransfer quTransfer = { transaction->sourcePublicKey , transaction->destinationPublicKey , transaction->amount };
                                logQuTransfer(quTransfer);
                            }

                            if (isZero(transaction->destinationPublicKey))
                            {
                                // Nothing to do
                            }
                            else
                            {
                                // Contracts are identified by their index stored in the first 64 bits of the id, all
                                // other bits are zeroed. However, the max number of contracts is limited to 2^32 - 1,
                                // only 32 bits are used for the contract index.
                                m256i maskedDestinationPublicKey = transaction->destinationPublicKey;
                                maskedDestinationPublicKey.m256i_u64[0] &= ~(MAX_NUMBER_OF_CONTRACTS - 1ULL);
                                executedContractIndex = (unsigned int)transaction->destinationPublicKey.m256i_u64[0];
                                if (isZero(maskedDestinationPublicKey)
                                    && executedContractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]))
                                {
                                    if (system.epoch < contractDescriptions[executedContractIndex].constructionEpoch)
                                    {
                                        if (!transaction->amount
                                            && transaction->inputSize == sizeof(ContractIPOBid))
                                        {
                                            ContractIPOBid* contractIPOBid = (ContractIPOBid*)(((unsigned char*)transaction) + sizeof(Transaction));
                                            if (contractIPOBid->price > 0 && contractIPOBid->price <= MAX_AMOUNT / NUMBER_OF_COMPUTORS
                                                && contractIPOBid->quantity > 0 && contractIPOBid->quantity <= NUMBER_OF_COMPUTORS)
                                            {
                                                const long long amount = contractIPOBid->price * contractIPOBid->quantity;
                                                if (decreaseEnergy(spectrumIndex, amount))
                                                {
                                                    const QuTransfer quTransfer = { transaction->sourcePublicKey , _mm256_setzero_si256() , amount };
                                                    logQuTransfer(quTransfer);

                                                    numberOfReleasedEntities = 0;
                                                    IPO* ipo = (IPO*)contractStates[executedContractIndex];
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

                                                            contractStateChangeFlags[executedContractIndex >> 6] |= (1ULL << (executedContractIndex & 63));
                                                        }
                                                    }
                                                    for (unsigned int i = 0; i < numberOfReleasedEntities; i++)
                                                    {
                                                        increaseEnergy(releasedPublicKeys[i], releasedAmounts[i]);
                                                        const QuTransfer quTransfer = { _mm256_setzero_si256() , releasedPublicKeys[i] , releasedAmounts[i] };
                                                        logQuTransfer(quTransfer);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (contractUserProcedures[executedContractIndex][transaction->inputType])
                                        {
                                            ::originator = transaction->sourcePublicKey;
                                            ::invocator = ::originator;
                                            ::invocationReward = transaction->amount;
                                            currentContract = _mm256_set_epi64x(0, 0, 0, executedContractIndex);

                                            bs->SetMem(&executedContractInput, sizeof(executedContractInput), 0);
                                            bs->CopyMem(&executedContractInput, (((unsigned char*)transaction) + sizeof(Transaction)), transaction->inputSize);
                                            const unsigned long long startTick = __rdtsc();
                                            contractUserProcedures[executedContractIndex][transaction->inputType](contractStates[executedContractIndex], &executedContractInput, &executedContractOutput);
                                            contractTotalExecutionTicks[executedContractIndex] += __rdtsc() - startTick;
                                        }
                                    }
                                }
                                else
                                {
                                    if (transaction->destinationPublicKey == arbitratorPublicKey)
                                    {
                                        if (!transaction->amount
                                            && transaction->inputSize == 32
                                            && !transaction->inputType)
                                        {
                                            const m256i & solution_nonce = *(m256i*)((unsigned char*)transaction + sizeof(Transaction));
                                            m256i data[2] = { transaction->sourcePublicKey, solution_nonce };
                                            static_assert(sizeof(data) == 2 * 32, "Unexpected array size");
                                            unsigned int flagIndex;
                                            KangarooTwelve(data, sizeof(data), &flagIndex, sizeof(flagIndex));
                                            if (!(minerSolutionFlags[flagIndex >> 6] & (1ULL << (flagIndex & 63))))
                                            {
                                                minerSolutionFlags[flagIndex >> 6] |= (1ULL << (flagIndex & 63));

                                                unsigned long long solutionScore = (*::score)(processorNumber, transaction->sourcePublicKey, solution_nonce);

                                                resourceTestingDigest ^= solutionScore;
                                                KangarooTwelve(&resourceTestingDigest, sizeof(resourceTestingDigest), &resourceTestingDigest, sizeof(resourceTestingDigest));

                                                const int threshold = (system.epoch < MAX_NUMBER_EPOCH) ? solutionThreshold[system.epoch] : SOLUTION_THRESHOLD_DEFAULT;
                                                if (((solutionScore >= (DATA_LENGTH / 2) + threshold) || (solutionScore <= (DATA_LENGTH / 2) - threshold)))
                                                {
                                                    for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
                                                    {
                                                        if (transaction->sourcePublicKey == computorPublicKeys[i])
                                                        {
                                                            ACQUIRE(solutionsLock);

                                                            unsigned int j;
                                                            for (j = 0; j < system.numberOfSolutions; j++)
                                                            {
                                                                if (solution_nonce == system.solutions[j].nonce
                                                                    && transaction->sourcePublicKey == system.solutions[j].computorPublicKey)
                                                                {
                                                                    solutionPublicationTicks[j] = -1;

                                                                    break;
                                                                }
                                                            }
                                                            if (j == system.numberOfSolutions
                                                                && system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS)
                                                            {
                                                                system.solutions[system.numberOfSolutions].computorPublicKey = transaction->sourcePublicKey;
                                                                system.solutions[system.numberOfSolutions].nonce = solution_nonce;
                                                                solutionPublicationTicks[system.numberOfSolutions++] = -1;
                                                            }

                                                            RELEASE(solutionsLock);

                                                            break;
                                                        }
                                                    }

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

                                                    for (unsigned int i = 0; i < QUORUM; i++)
                                                    {
                                                        system.futureComputors[i] = minerPublicKeys[i];
                                                    }
                                                    for (unsigned int i = QUORUM; i < NUMBER_OF_COMPUTORS; i++)
                                                    {
                                                        system.futureComputors[i] = competitorPublicKeys[i - QUORUM];
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
                                                            if (solution_nonce == system.solutions[j].nonce
                                                                && transaction->sourcePublicKey == system.solutions[j].computorPublicKey)
                                                            {
                                                                solutionPublicationTicks[j] = -1;

                                                                break;
                                                            }
                                                        }
                                                        if (j == system.numberOfSolutions
                                                            && system.numberOfSolutions < MAX_NUMBER_OF_SOLUTIONS)
                                                        {
                                                            system.solutions[system.numberOfSolutions].computorPublicKey = transaction->sourcePublicKey;
                                                            system.solutions[system.numberOfSolutions].nonce = solution_nonce;
                                                            solutionPublicationTicks[system.numberOfSolutions++] = -1;
                                                        }

                                                        RELEASE(solutionsLock);

                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
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

    contractProcessorPhase = END_TICK;
    contractProcessorState = 1;
    while (contractProcessorState)
    {
        _mm_pause();
    }

    unsigned int digestIndex;
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

                    broadcastedFutureTickData.tickData.millisecond = time.Nanosecond / 1000000;
                    broadcastedFutureTickData.tickData.second = time.Second;
                    broadcastedFutureTickData.tickData.minute = time.Minute;
                    broadcastedFutureTickData.tickData.hour = time.Hour;
                    broadcastedFutureTickData.tickData.day = time.Day;
                    broadcastedFutureTickData.tickData.month = time.Month;
                    broadcastedFutureTickData.tickData.year = time.Year - 2000;

                    if (system.proposals[ownComputorIndices[i]].uriSize)
                    {
                        bs->CopyMem(&broadcastedFutureTickData.tickData.varStruct.proposal, &system.proposals[ownComputorIndices[i]], sizeof(ComputorProposal));
                    }
                    else
                    {
                        bs->CopyMem(&broadcastedFutureTickData.tickData.varStruct.ballot, &system.ballots[ownComputorIndices[i]], sizeof(ComputorBallot));
                    }

                    m256i timelockPreimage[3];
                    static_assert(sizeof(timelockPreimage) == 3 * 32, "Unexpected array size");
                    timelockPreimage[0] = etalonTick.saltedSpectrumDigest;
                    timelockPreimage[1] = etalonTick.saltedUniverseDigest;
                    timelockPreimage[2] = etalonTick.saltedComputerDigest;
                    KangarooTwelve(timelockPreimage, sizeof(timelockPreimage), &broadcastedFutureTickData.tickData.timelock, sizeof(broadcastedFutureTickData.tickData.timelock));

                    unsigned int numberOfEntityPendingTransactionIndices;
                    for (numberOfEntityPendingTransactionIndices = 0; numberOfEntityPendingTransactionIndices < SPECTRUM_CAPACITY; numberOfEntityPendingTransactionIndices++)
                    {
                        entityPendingTransactionIndices[numberOfEntityPendingTransactionIndices] = numberOfEntityPendingTransactionIndices;
                    }
                    unsigned int j = 0;
                    while (j < NUMBER_OF_TRANSACTIONS_PER_TICK && numberOfEntityPendingTransactionIndices)
                    {
                        const unsigned int index = random(numberOfEntityPendingTransactionIndices);

                        const Transaction* pendingTransaction = ((Transaction*)&entityPendingTransactions[entityPendingTransactionIndices[index] * MAX_TRANSACTION_SIZE]);
                        if (pendingTransaction->tick == system.tick + TICK_TRANSACTIONS_PUBLICATION_OFFSET)
                        {
                            const unsigned int transactionSize = sizeof(Transaction) + pendingTransaction->inputSize + SIGNATURE_SIZE;
                            if (nextTickTransactionOffset + transactionSize <= FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS))
                            {
                                ACQUIRE(tickTransactionsLock);
                                if (nextTickTransactionOffset + transactionSize <= FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS))
                                {
                                    tickTransactionOffsets(pendingTransaction->tick, j) = nextTickTransactionOffset;
                                    bs->CopyMem(&tickTransactions[nextTickTransactionOffset], (void*)pendingTransaction, transactionSize);
                                    broadcastedFutureTickData.tickData.transactionDigests[j] = &entityPendingTransactionDigests[entityPendingTransactionIndices[index] * 32ULL];
                                    j++;
                                    nextTickTransactionOffset += transactionSize;
                                }
                                RELEASE(tickTransactionsLock);
                            }
                        }

                        entityPendingTransactionIndices[index] = entityPendingTransactionIndices[--numberOfEntityPendingTransactionIndices];
                    }
                    for (; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                    {
                        broadcastedFutureTickData.tickData.transactionDigests[j] = _mm256_setzero_si256();
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

    if (mainAuxStatus & 1)
    {
        for (unsigned int i = 0; i < sizeof(computorSeeds) / sizeof(computorSeeds[0]); i++)
        {
            int solutionIndexToPublish = -1;

            unsigned int j;
            for (j = 0; j < system.numberOfSolutions; j++)
            {
                if (solutionPublicationTicks[j] > 0
                    && system.solutions[j].computorPublicKey == computorPublicKeys[i])
                {
                    if (solutionPublicationTicks[j] <= (int)system.tick)
                    {
                        solutionIndexToPublish = j;
                    }

                    break;
                }
            }
            if (j == system.numberOfSolutions)
            {
                for (j = 0; j < system.numberOfSolutions; j++)
                {
                    if (!solutionPublicationTicks[j]
                        && system.solutions[j].computorPublicKey == computorPublicKeys[i])
                    {
                        solutionIndexToPublish = j;

                        break;
                    }
                }
            }

            if (solutionIndexToPublish >= 0)
            {
                struct
                {
                    Transaction transaction;
                    m256i nonce;
                    unsigned char signature[SIGNATURE_SIZE];
                } payload;
                static_assert(sizeof(payload) == sizeof(Transaction) + 32 + SIGNATURE_SIZE, "Unexpected struct size!");
                payload.transaction.sourcePublicKey = computorPublicKeys[i];
                payload.transaction.destinationPublicKey = arbitratorPublicKey;
                payload.transaction.amount = 0;
                unsigned int random;
                _rdrand32_step(&random);
                solutionPublicationTicks[solutionIndexToPublish] = payload.transaction.tick = system.tick + MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET + random % MIN_MINING_SOLUTIONS_PUBLICATION_OFFSET;
                payload.transaction.inputType = 0;
                payload.transaction.inputSize = sizeof(payload.nonce);
                payload.nonce = system.solutions[solutionIndexToPublish].nonce;

                unsigned char digest[32];
                KangarooTwelve(&payload.transaction, sizeof(payload.transaction) + sizeof(payload.nonce), digest, sizeof(digest));
                sign(computorSubseeds[i].m256i_u8, computorPublicKeys[i].m256i_u8, digest, payload.signature);

                enqueueResponse(NULL, sizeof(payload), BROADCAST_TRANSACTION, 0, &payload);
            }
        }
    }
}

static void beginEpoch1of2()
{
    // This version doesn't support migration from contract IPO to contract operation!

    numberOfOwnComputorIndices = 0;

    broadcastedComputors.header.setSize<sizeof(broadcastedComputors.header) + sizeof(broadcastedComputors.broadcastComputors)>();
    broadcastedComputors.header.setType(BroadcastComputors::type);
    broadcastedComputors.broadcastComputors.computors.epoch = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        broadcastedComputors.broadcastComputors.computors.publicKeys[i].setRandomValue();
    }
    bs->SetMem(&broadcastedComputors.broadcastComputors.computors.signature, sizeof(broadcastedComputors.broadcastComputors.computors.signature), 0);

    bs->SetMem(ticks, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_COMPUTORS * sizeof(Tick), 0);
    bs->SetMem(tickData, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * sizeof(TickData), 0);
    bs->SetMem(tickTransactions, FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS), 0);
    bs->SetMem(tickTransactionOffsetsPtr, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * sizeof(*tickTransactionOffsetsPtr), 0);

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

    bs->SetMem(score, sizeof(*score), 0);
    score->resetTaskQueue();
    score->loadScoreCache(system.epoch);
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
    bs->SetMem(system.proposals, sizeof(system.proposals), 0);
    bs->SetMem(system.ballots, sizeof(system.ballots), 0);
    system.numberOfSolutions = 0;
    bs->SetMem(system.solutions, sizeof(system.solutions), 0);
    bs->SetMem(system.futureComputors, sizeof(system.futureComputors), 0);

    // Reset resource testing digest at beginning of the epoch
    // there are many global variables that were init at declaration, may need to re-check all of them again
    resourceTestingDigest = 0;


#if LOG_QU_TRANSFERS && LOG_QU_TRANSFERS_TRACK_TRANSFER_ID
    CurrentTransferId = 0;
#endif
}

static void beginEpoch2of2()
{
    score->initMiningData(spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1]);
}

// called by tickProcessor() after system.tick has been incremented
static void endEpoch()
{
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

    Contract0State* contract0State = (Contract0State*)contractStates[0];
    for (unsigned int contractIndex = 1; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
    {
        if (system.epoch < contractDescriptions[contractIndex].constructionEpoch)
        {
            IPO* ipo = (IPO*)contractStates[contractIndex];
            long long finalPrice = ipo->prices[NUMBER_OF_COMPUTORS - 1];
            int issuanceIndex, ownershipIndex, possessionIndex;
            if (finalPrice)
            {
                if (!issueAsset(_mm256_setzero_si256(), (char*)contractDescriptions[contractIndex].assetName, 0, CONTRACT_ASSET_UNIT_OF_MEASUREMENT, NUMBER_OF_COMPUTORS, QX_CONTRACT_INDEX, &issuanceIndex, &ownershipIndex, &possessionIndex))
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
                const QuTransfer quTransfer = { _mm256_setzero_si256() , releasedPublicKeys[i] , releasedAmounts[i] };
                logQuTransfer(quTransfer);
            }

            contract0State->contractFeeReserves[contractIndex] = finalPrice * NUMBER_OF_COMPUTORS;
        }
    }

    system.initialMillisecond = etalonTick.millisecond;
    system.initialSecond = etalonTick.second;
    system.initialMinute = etalonTick.minute;
    system.initialHour = etalonTick.hour;
    system.initialDay = etalonTick.day;
    system.initialMonth = etalonTick.month;
    system.initialYear = etalonTick.year;

    long long arbitratorRevenue = ISSUANCE_RATE;

    unsigned long long transactionCounters[NUMBER_OF_COMPUTORS];
    bs->SetMem(transactionCounters, sizeof(transactionCounters), 0);
    for (unsigned int tick = system.initialTick; tick < system.tick; tick++)
    {
        ACQUIRE(tickDataLock);
        if (tickData[tick - system.initialTick].epoch == system.epoch)
        {
            unsigned int numberOfTransactions = 0;
            for (unsigned int transactionIndex = 0; transactionIndex < NUMBER_OF_TRANSACTIONS_PER_TICK; transactionIndex++)
            {
                if (!isZero(tickData[tick - system.initialTick].transactionDigests[transactionIndex]))
                {
                    numberOfTransactions++;
                }
            }
            transactionCounters[tick % NUMBER_OF_COMPUTORS] += revenuePoints[numberOfTransactions];
        }
        RELEASE(tickDataLock);
    }
    unsigned long long sortedTransactionCounters[QUORUM + 1];
    bs->SetMem(sortedTransactionCounters, sizeof(sortedTransactionCounters), 0);
    for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        sortedTransactionCounters[QUORUM] = transactionCounters[computorIndex];
        unsigned int i = QUORUM;
        while (i
            && sortedTransactionCounters[i - 1] < sortedTransactionCounters[i])
        {
            const unsigned long long tmp = sortedTransactionCounters[i - 1];
            sortedTransactionCounters[i - 1] = sortedTransactionCounters[i];
            sortedTransactionCounters[i--] = tmp;
        }
    }
    if (!sortedTransactionCounters[QUORUM - 1])
    {
        sortedTransactionCounters[QUORUM - 1] = 1;
    }
    for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        const long long revenue = (transactionCounters[computorIndex] >= sortedTransactionCounters[QUORUM - 1]) ? (ISSUANCE_RATE / NUMBER_OF_COMPUTORS) : (((ISSUANCE_RATE / NUMBER_OF_COMPUTORS) * ((unsigned long long)transactionCounters[computorIndex])) / sortedTransactionCounters[QUORUM - 1]);
        increaseEnergy(broadcastedComputors.broadcastComputors.computors.publicKeys[computorIndex], revenue);
        if (revenue)
        {
            const QuTransfer quTransfer = { _mm256_setzero_si256() , broadcastedComputors.broadcastComputors.computors.publicKeys[computorIndex] , revenue };
            logQuTransfer(quTransfer);
        }
        arbitratorRevenue -= revenue;
    }

    increaseEnergy((unsigned char*)&arbitratorPublicKey, arbitratorRevenue);
    const QuTransfer quTransfer = { _mm256_setzero_si256() , arbitratorPublicKey , arbitratorRevenue };
    logQuTransfer(quTransfer);

    {
        ACQUIRE(spectrumLock);

        ::Entity* reorgSpectrum = (::Entity*)reorgBuffer;
        bs->SetMem(reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity), 0);
        for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
        {
            if (spectrum[i].incomingAmount - spectrum[i].outgoingAmount)
            {
                unsigned int index = spectrum[i].publicKey.m256i_u32[0] & (SPECTRUM_CAPACITY - 1);

            iteration:
                if (isZero(reorgSpectrum[index].publicKey))
                {
                    bs->CopyMem(&reorgSpectrum[index], &spectrum[i], sizeof(::Entity));
                }
                else
                {
                    index = (index + 1) & (SPECTRUM_CAPACITY - 1);

                    goto iteration;
                }
            }
        }
        bs->CopyMem(spectrum, reorgSpectrum, SPECTRUM_CAPACITY * sizeof(::Entity));

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

        numberOfEntities = 0;
        for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
        {
            if (spectrum[i].incomingAmount - spectrum[i].outgoingAmount)
            {
                numberOfEntities++;
            }
        }

        RELEASE(spectrumLock);
    }

    assetsEndEpoch(reorgBuffer);

    system.epoch++;
    system.initialTick = system.tick;

    mainAuxStatus = ((mainAuxStatus & 1) << 1) | ((mainAuxStatus & 2) >> 1);
}

static void tickProcessor(void*)
{
    enableAVX();
    unsigned long long processorNumber;
    mpServicesProtocol->WhoAmI(mpServicesProtocol, &processorNumber);

    unsigned int latestProcessedTick = 0;
    while (!shutDownNode)
    {
        const unsigned long long curTimeTick = __rdtsc();

        if (broadcastedComputors.broadcastComputors.computors.epoch == system.epoch)
        {
            {
                const unsigned int baseOffset = (system.tick + 1 - system.initialTick) * NUMBER_OF_COMPUTORS;
                unsigned int futureTickTotalNumberOfComputors = 0;
                for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                {
                    if (ticks[baseOffset + i].epoch == system.epoch)
                    {
                        futureTickTotalNumberOfComputors++;
                    }
                }
                ::futureTickTotalNumberOfComputors = futureTickTotalNumberOfComputors;
            }

            if (system.tick - system.initialTick < MAX_NUMBER_OF_TICKS_PER_EPOCH - 1)
            {
                if (system.tick > latestProcessedTick)
                {
                    processTick(processorNumber);

                    latestProcessedTick = system.tick;
                }

                if (futureTickTotalNumberOfComputors > NUMBER_OF_COMPUTORS - QUORUM)
                {
                    const unsigned int baseOffset = (system.tick + 1 - system.initialTick) * NUMBER_OF_COMPUTORS;
                    unsigned int numberOfEmptyNextTickTransactionDigest = 0;
                    unsigned int numberOfUniqueNextTickTransactionDigests = 0;
                    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                    {
                        if (ticks[baseOffset + i].epoch == system.epoch)
                        {
                            unsigned int j;
                            for (j = 0; j < numberOfUniqueNextTickTransactionDigests; j++)
                            {
                                if (ticks[baseOffset + i].transactionDigest == uniqueNextTickTransactionDigests[j])
                                {
                                    break;
                                }
                            }
                            if (j == numberOfUniqueNextTickTransactionDigests)
                            {
                                uniqueNextTickTransactionDigests[numberOfUniqueNextTickTransactionDigests] = ticks[baseOffset + i].transactionDigest;
                                uniqueNextTickTransactionDigestCounters[numberOfUniqueNextTickTransactionDigests++] = 1;
                            }
                            else
                            {
                                uniqueNextTickTransactionDigestCounters[j]++;
                            }

                            if (isZero(ticks[baseOffset + i].transactionDigest))
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
                            targetNextTickDataDigest = _mm256_setzero_si256();
                            targetNextTickDataDigestIsKnown = true;
                        }
                    }
                }

                if (!targetNextTickDataDigestIsKnown)
                {
                    const unsigned int baseOffset = (system.tick - system.initialTick) * NUMBER_OF_COMPUTORS;
                    unsigned int numberOfEmptyNextTickTransactionDigest = 0;
                    unsigned int numberOfUniqueNextTickTransactionDigests = 0;
                    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                    {
                        if (ticks[baseOffset + i].epoch == system.epoch)
                        {
                            unsigned int j;
                            for (j = 0; j < numberOfUniqueNextTickTransactionDigests; j++)
                            {
                                if (ticks[baseOffset + i].expectedNextTickTransactionDigest == uniqueNextTickTransactionDigests[j])
                                {
                                    break;
                                }
                            }
                            if (j == numberOfUniqueNextTickTransactionDigests)
                            {
                                uniqueNextTickTransactionDigests[numberOfUniqueNextTickTransactionDigests] = ticks[baseOffset + i].expectedNextTickTransactionDigest;
                                uniqueNextTickTransactionDigestCounters[numberOfUniqueNextTickTransactionDigests++] = 1;
                            }
                            else
                            {
                                uniqueNextTickTransactionDigestCounters[j]++;
                            }

                            if (isZero(ticks[baseOffset + i].expectedNextTickTransactionDigest))
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
                                targetNextTickDataDigest = _mm256_setzero_si256();
                                targetNextTickDataDigestIsKnown = true;
                            }
                        }
                    }
                }

                ACQUIRE(tickDataLock);
                bs->CopyMem(&nextTickData, &tickData[system.tick + 1 - system.initialTick], sizeof(TickData));
                RELEASE(tickDataLock);
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
                        ACQUIRE(tickDataLock);
                        tickData[system.tick + 1 - system.initialTick].epoch = 0;
                        RELEASE(tickDataLock);
                        nextTickData.epoch = 0;
                    }
                }

                bool tickDataSuits;
                if (!targetNextTickDataDigestIsKnown)
                {
                    if (nextTickData.epoch != system.epoch
                        && futureTickTotalNumberOfComputors <= NUMBER_OF_COMPUTORS - QUORUM
                        && __rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] < TARGET_TICK_DURATION * frequency / 1000)
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
                        ACQUIRE(tickDataLock);
                        tickData[system.tick + 1 - system.initialTick].epoch = 0;
                        RELEASE(tickDataLock);
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
                }
                if (!tickDataSuits)
                {
                    unsigned int tickTotalNumberOfComputors = 0;
                    const unsigned int baseOffset = (system.tick - system.initialTick) * NUMBER_OF_COMPUTORS;
                    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                    {
                        ACQUIRE(tickLocks[i]);

                        if (ticks[baseOffset + i].epoch == system.epoch)
                        {
                            tickTotalNumberOfComputors++;
                        }

                        RELEASE(tickLocks[i]);
                    }
                    ::tickNumberOfComputors = 0;
                    ::tickTotalNumberOfComputors = tickTotalNumberOfComputors;
                }
                else
                {
                    numberOfNextTickTransactions = 0;
                    numberOfKnownNextTickTransactions = 0;

                    if (nextTickData.epoch == system.epoch)
                    {
                        nextTickTransactionsSemaphore = 1;
                        bs->SetMem(requestedTickTransactions.requestedTickTransactions.transactionFlags, sizeof(requestedTickTransactions.requestedTickTransactions.transactionFlags), 0);
                        unsigned long long unknownTransactions[NUMBER_OF_TRANSACTIONS_PER_TICK / 64];
                        bs->SetMem(unknownTransactions, sizeof(unknownTransactions), 0);
                        for (unsigned int i = 0; i < NUMBER_OF_TRANSACTIONS_PER_TICK; i++)
                        {
                            if (!isZero(nextTickData.transactionDigests[i]))
                            {
                                numberOfNextTickTransactions++;

                                ACQUIRE(tickTransactionsLock);
                                if (tickTransactionOffsets(system.tick + 1, i))
                                {
                                    const Transaction* transaction = (Transaction*)&tickTransactions[tickTransactionOffsets(system.tick + 1, i)];
                                    unsigned char digest[32];
                                    KangarooTwelve(transaction, sizeof(Transaction) + transaction->inputSize + SIGNATURE_SIZE, digest, sizeof(digest));
                                    if (digest == nextTickData.transactionDigests[i])
                                    {
                                        numberOfKnownNextTickTransactions++;
                                    }
                                    else
                                    {
                                        unknownTransactions[i >> 6] |= (1ULL << (i & 63));
                                    }
                                }
                                RELEASE(tickTransactionsLock);
                            }
                        }
                        if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
                        {
                            const unsigned int nextTick = system.tick + 1;
                            for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
                            {
                                Transaction* pendingTransaction = (Transaction*)&entityPendingTransactions[i * MAX_TRANSACTION_SIZE];
                                if (pendingTransaction->tick == nextTick)
                                {
                                    ACQUIRE(entityPendingTransactionsLock);

                                    for (unsigned int j = 0; j < NUMBER_OF_TRANSACTIONS_PER_TICK; j++)
                                    {
                                        if (unknownTransactions[j >> 6] & (1ULL << (j & 63)))
                                        {
                                            if (&entityPendingTransactionDigests[i * 32ULL] == nextTickData.transactionDigests[j])
                                            {
                                                unsigned char transactionBuffer[MAX_TRANSACTION_SIZE];
                                                const unsigned int transactionSize = sizeof(Transaction) + pendingTransaction->inputSize + SIGNATURE_SIZE;
                                                bs->CopyMem(transactionBuffer, (void*)pendingTransaction, transactionSize);

                                                pendingTransaction = (Transaction*)transactionBuffer;
                                                ACQUIRE(tickTransactionsLock);
                                                if (!tickTransactionOffsets(pendingTransaction->tick, j))
                                                {
                                                    if (nextTickTransactionOffset + transactionSize <= FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS))
                                                    {
                                                        tickTransactionOffsets(pendingTransaction->tick, j) = nextTickTransactionOffset;
                                                        bs->CopyMem(&tickTransactions[nextTickTransactionOffset], pendingTransaction, transactionSize);
                                                        nextTickTransactionOffset += transactionSize;
                                                    }
                                                }
                                                RELEASE(tickTransactionsLock);

                                                numberOfKnownNextTickTransactions++;
                                                unknownTransactions[j >> 6] &= ~(1ULL << (j & 63));
                                            }
                                        }
                                    }

                                    RELEASE(entityPendingTransactionsLock);
                                }
                            }

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

                    if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
                    {
                        if (!targetNextTickDataDigestIsKnown
                            && __rdtsc() - tickTicks[sizeof(tickTicks) / sizeof(tickTicks[0]) - 1] > TARGET_TICK_DURATION * 5 * frequency / 1000)
                        {
                            ACQUIRE(tickDataLock);
                            tickData[system.tick + 1 - system.initialTick].epoch = 0;
                            RELEASE(tickDataLock);
                            nextTickData.epoch = 0;

                            numberOfNextTickTransactions = 0;
                            numberOfKnownNextTickTransactions = 0;
                        }
                    }

                    if (numberOfKnownNextTickTransactions != numberOfNextTickTransactions)
                    {
                        requestedTickTransactions.requestedTickTransactions.tick = system.tick + 1;
                    }
                    else
                    {
                        requestedTickTransactions.requestedTickTransactions.tick = 0;

                        if (tickData[system.tick - system.initialTick].epoch == system.epoch)
                        {
                            KangarooTwelve(&tickData[system.tick - system.initialTick], sizeof(TickData), &etalonTick.transactionDigest, 32);
                        }
                        else
                        {
                            etalonTick.transactionDigest = _mm256_setzero_si256();
                        }

                        if (nextTickData.epoch == system.epoch)
                        {
                            if (!targetNextTickDataDigestIsKnown)
                            {
                                KangarooTwelve(&nextTickData, sizeof(TickData), &etalonTick.expectedNextTickTransactionDigest, 32);
                            }
                        }
                        else
                        {
                            etalonTick.expectedNextTickTransactionDigest = _mm256_setzero_si256();
                        }

                        if (system.tick > system.latestCreatedTick || system.tick == system.initialTick)
                        {
                            if (mainAuxStatus & 1)
                            {
                                BroadcastTick broadcastTick;
                                bs->CopyMem(&broadcastTick.tick, &etalonTick, sizeof(Tick));
                                for (unsigned int i = 0; i < numberOfOwnComputorIndices; i++)
                                {
                                    broadcastTick.tick.computorIndex = ownComputorIndices[i] ^ BroadcastTick::type;
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
                                }
                            }

                            if (system.tick != system.initialTick)
                            {
                                system.latestCreatedTick = system.tick;
                            }
                        }

                        const unsigned int baseOffset = (system.tick - system.initialTick) * NUMBER_OF_COMPUTORS;

                        unsigned int tickNumberOfComputors = 0, tickTotalNumberOfComputors = 0;
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            ACQUIRE(tickLocks[i]);

                            const Tick* tick = &ticks[baseOffset + i];
                            if (tick->epoch == system.epoch)
                            {
                                tickTotalNumberOfComputors++;

                                if (*((unsigned long long*)&tick->millisecond) == *((unsigned long long*)&etalonTick.millisecond)
                                    && tick->prevSpectrumDigest == etalonTick.prevSpectrumDigest
                                    && tick->prevUniverseDigest == etalonTick.prevUniverseDigest
                                    && tick->prevComputerDigest == etalonTick.prevComputerDigest
                                    && tick->transactionDigest == etalonTick.transactionDigest)
                                {
                                    m256i saltedData[2];
                                    m256i saltedDigest;
                                    saltedData[0] = broadcastedComputors.broadcastComputors.computors.publicKeys[tick->computorIndex];
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
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            RELEASE(tickLocks[i]);
                        }
                        ::tickNumberOfComputors = tickNumberOfComputors;
                        ::tickTotalNumberOfComputors = tickTotalNumberOfComputors;

                        if (tickNumberOfComputors >= QUORUM)
                        {
                            if (!targetNextTickDataDigestIsKnown)
                            {
                                if (forceNextTick)
                                {
                                    targetNextTickDataDigest = _mm256_setzero_si256();
                                    targetNextTickDataDigestIsKnown = true;
                                }
                            }
                            forceNextTick = false;

                            if (targetNextTickDataDigestIsKnown)
                            {
                                tickDataSuits = false;
                                if (isZero(targetNextTickDataDigest))
                                {
                                    ACQUIRE(tickDataLock);
                                    tickData[system.tick + 1 - system.initialTick].epoch = 0;
                                    RELEASE(tickDataLock);
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
                                        epochTransitionState = 1;
                                    }
                                    else
                                    {
                                        etalonTick.tick++;
                                        ACQUIRE(tickDataLock);
                                        if (tickData[system.tick - system.initialTick].epoch == system.epoch
                                            && (tickData[system.tick - system.initialTick].year > etalonTick.year
                                                || (tickData[system.tick - system.initialTick].year == etalonTick.year && (tickData[system.tick - system.initialTick].month > etalonTick.month
                                                    || (tickData[system.tick - system.initialTick].month == etalonTick.month && (tickData[system.tick - system.initialTick].day > etalonTick.day
                                                        || (tickData[system.tick - system.initialTick].day == etalonTick.day && (tickData[system.tick - system.initialTick].hour > etalonTick.hour
                                                            || (tickData[system.tick - system.initialTick].hour == etalonTick.hour && (tickData[system.tick - system.initialTick].minute > etalonTick.minute
                                                                || (tickData[system.tick - system.initialTick].minute == etalonTick.minute && (tickData[system.tick - system.initialTick].second > etalonTick.second
                                                                    || (tickData[system.tick - system.initialTick].second == etalonTick.second && tickData[system.tick - system.initialTick].millisecond > etalonTick.millisecond)))))))))))))
                                        {
                                            etalonTick.millisecond = tickData[system.tick - system.initialTick].millisecond;
                                            etalonTick.second = tickData[system.tick - system.initialTick].second;
                                            etalonTick.minute = tickData[system.tick - system.initialTick].minute;
                                            etalonTick.hour = tickData[system.tick - system.initialTick].hour;
                                            etalonTick.day = tickData[system.tick - system.initialTick].day;
                                            etalonTick.month = tickData[system.tick - system.initialTick].month;
                                            etalonTick.year = tickData[system.tick - system.initialTick].year;
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
                                        RELEASE(tickDataLock);
                                    }

                                    system.tick++;

                                    if (epochTransitionState == 1)
                                    {
                                        // seamless epoch transistion
                                        endEpoch();

                                        // instruct main loop to save system and wait until it is done
                                        systemMustBeSaved = true;
                                        while (systemMustBeSaved)
                                        {
                                            _mm_pause();
                                        }

                                        // a temporary fix to set correct filenames because complete beginEpoch1of2() and beginEpoch2of2() is commented out
                                        SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
                                        SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
                                        SPECTRUM_FILE_NAME[sizeof(SPECTRUM_FILE_NAME) / sizeof(SPECTRUM_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

                                        UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
                                        UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
                                        UNIVERSE_FILE_NAME[sizeof(UNIVERSE_FILE_NAME) / sizeof(UNIVERSE_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

                                        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 4] = system.epoch / 100 + L'0';
                                        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 3] = (system.epoch % 100) / 10 + L'0';
                                        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 2] = system.epoch % 10 + L'0';

                                        /*
                                        beginEpoch1of2();
                                        beginEpoch2of2();
                                        */

                                        spectrumMustBeSaved = true;
                                        universeMustBeSaved = true;
                                        computerMustBeSaved = true;

                                        //update etalon tick:
                                        etalonTick.epoch++;
                                        etalonTick.tick++;
                                        etalonTick.saltedSpectrumDigest = spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1];
                                        getUniverseDigest(etalonTick.saltedUniverseDigest);
                                        getComputerDigest(etalonTick.saltedComputerDigest);

                                        epochTransitionState = 0;
                                    }

                                    ::tickNumberOfComputors = 0;
                                    ::tickTotalNumberOfComputors = 0;
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

static void saveSpectrum()
{
    logToConsole(L"Saving spectrum file...");

    const unsigned long long beginningTick = __rdtsc();

    ACQUIRE(spectrumLock);
    long long savedSize = save(SPECTRUM_FILE_NAME, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum);
    RELEASE(spectrumLock);

    if (savedSize == SPECTRUM_CAPACITY * sizeof(::Entity))
    {
        setNumber(message, savedSize, TRUE);
        appendText(message, L" bytes of the spectrum data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
    }
}

static void saveComputer()
{
    logToConsole(L"Saving contract files...");

    const unsigned long long beginningTick = __rdtsc();

    bool ok = true;
    unsigned long long totalSize = 0;

    ACQUIRE(computerLock);

    for (unsigned int contractIndex = 0; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
    {
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 9] = contractIndex / 1000 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 8] = (contractIndex % 1000) / 100 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 7] = (contractIndex % 100) / 10 + L'0';
        CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 6] = contractIndex % 10 + L'0';
        long long savedSize = save(CONTRACT_FILE_NAME, contractDescriptions[contractIndex].stateSize, contractStates[contractIndex]);
        totalSize += savedSize;
        if (savedSize != contractDescriptions[contractIndex].stateSize)
        {
            ok = false;

            break;
        }
    }

    RELEASE(computerLock);

    if (ok)
    {
        setNumber(message, totalSize, TRUE);
        appendText(message, L" bytes of the computer data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
    }
}

static void saveSystem()
{
    logToConsole(L"Saving system file...");

    const unsigned long long beginningTick = __rdtsc();
    long long savedSize = save(SYSTEM_FILE_NAME, sizeof(system), (unsigned char*)&system);
    if (savedSize == sizeof(system))
    {
        setNumber(message, savedSize, TRUE);
        appendText(message, L" bytes of the system data are saved (");
        appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
        appendText(message, L" microseconds).");
        logToConsole(message);
    }
}

static bool initialize()
{
    enableAVX();

#ifdef __AVX512F__
    initAVX512KangarooTwelveConstants();
    initAVX512FourQConstants();
#endif

    for (unsigned int contractIndex = 0; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
    {
        contractStates[contractIndex] = NULL;
    }
    bs->SetMem(contractSystemProcedures, sizeof(contractSystemProcedures), 0);
    bs->SetMem(contractUserFunctions, sizeof(contractUserFunctions), 0);
    bs->SetMem(contractUserProcedures, sizeof(contractUserProcedures), 0);
    for (unsigned int processorIndex = 0; processorIndex < MAX_NUMBER_OF_PROCESSORS; processorIndex++)
    {
        contractFunctionOutputs[processorIndex] = NULL;
    }

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

    bs->SetMem((void*)tickLocks, sizeof(tickLocks), 0);
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
        if (status = bs->AllocatePool(EfiRuntimeServicesData, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_COMPUTORS * sizeof(Tick), (void**)&ticks))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * sizeof(TickData), (void**)&tickData))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, FIRST_TICK_TRANSACTION_OFFSET + (((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * MAX_TRANSACTION_SIZE / TRANSACTION_SPARSENESS), (void**)&tickTransactions))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * MAX_TRANSACTION_SIZE, (void**)&entityPendingTransactions))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * 32ULL, (void**)&entityPendingTransactionDigests)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
        if (status = bs->AllocatePool(EfiRuntimeServicesData, ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK * sizeof(*tickTransactionOffsetsPtr), (void**)&tickTransactionOffsetsPtr))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        if (status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * sizeof(::Entity) >= ASSETS_CAPACITY * sizeof(Asset) ? SPECTRUM_CAPACITY * sizeof(::Entity) : ASSETS_CAPACITY * sizeof(Asset), (void**)&reorgBuffer))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        if ((status = bs->AllocatePool(EfiRuntimeServicesData, SPECTRUM_CAPACITY * sizeof(::Entity), (void**)&spectrum))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, (SPECTRUM_CAPACITY * 2 - 1) * 32ULL, (void**)&spectrumDigests)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
        bs->SetMem(spectrumChangeFlags, sizeof(spectrumChangeFlags), 0);

        if (!initAssets())
            return false;

        for (unsigned int contractIndex = 0; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
        {
            unsigned long long size = contractDescriptions[contractIndex].stateSize;
            if (status = bs->AllocatePool(EfiRuntimeServicesData, size, (void**)&contractStates[contractIndex]))
            {
                logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

                return false;
            }
        }
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, MAX_NUMBER_OF_CONTRACTS / 8, (void**)&contractStateChangeFlags))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, MAX_CONTRACT_STATE_SIZE, (void**)&contractStateCopy)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
        bs->SetMem(contractStateChangeFlags, MAX_NUMBER_OF_CONTRACTS / 8, 0xFF);
        for (unsigned int processorIndex = 0; processorIndex < MAX_NUMBER_OF_PROCESSORS; processorIndex++)
        {
            if (status = bs->AllocatePool(EfiRuntimeServicesData, RequestResponseHeader::max_size - sizeof(RequestResponseHeader), (void**)&contractFunctionOutputs[processorIndex]))
            {
                logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

                return false;
            }
        }

        if (status = bs->AllocatePool(EfiRuntimeServicesData, sizeof(*score), (void**)&score))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);
            return false;
        }

        bs->SetMem(solutionThreshold, sizeof(int) * MAX_NUMBER_EPOCH, 0);
        if (status = bs->AllocatePool(EfiRuntimeServicesData, NUMBER_OF_MINER_SOLUTION_FLAGS / 8, (void**)&minerSolutionFlags))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        if (!initLogging())
            return false;

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

        beginEpoch1of2();

        etalonTick.epoch = system.epoch;
        etalonTick.tick = system.initialTick;
        etalonTick.millisecond = system.initialMillisecond;
        etalonTick.second = system.initialSecond;
        etalonTick.minute = system.initialMinute;
        etalonTick.hour = system.initialHour;
        etalonTick.day = system.initialDay;
        etalonTick.month = system.initialMonth;
        etalonTick.year = system.initialYear;

        logToConsole(L"Loading spectrum file ...");
        long long loadedSize = load(SPECTRUM_FILE_NAME, SPECTRUM_CAPACITY * sizeof(::Entity), (unsigned char*)spectrum);
        if (loadedSize != SPECTRUM_CAPACITY * sizeof(::Entity))
        {
            logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

            return false;
        }
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
            unsigned long long totalAmount = 0;

            getIdentity((unsigned char*)&spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1], digestChars, true);

            for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
            {
                if (spectrum[i].incomingAmount - spectrum[i].outgoingAmount)
                {
                    numberOfEntities++;
                    totalAmount += spectrum[i].incomingAmount - spectrum[i].outgoingAmount;
                }
            }

            setNumber(message, totalAmount, TRUE);
            appendText(message, L" qus in ");
            appendNumber(message, numberOfEntities, TRUE);
            appendText(message, L" entities (digest = ");
            appendText(message, digestChars);
            appendText(message, L").");
            logToConsole(message);
        }

        logToConsole(L"Loading universe file ...");
        if (!loadUniverse())
            return false;

        logToConsole(L"Loading contract files ...");
        for (unsigned int contractIndex = 0; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
        {
            if (contractDescriptions[contractIndex].constructionEpoch == system.epoch)
            {
                bs->SetMem(contractStates[contractIndex], contractDescriptions[contractIndex].stateSize, 0);
            }
            else
            {
                CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 9] = contractIndex / 1000 + L'0';
                CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 8] = (contractIndex % 1000) / 100 + L'0';
                CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 7] = (contractIndex % 100) / 10 + L'0';
                CONTRACT_FILE_NAME[sizeof(CONTRACT_FILE_NAME) / sizeof(CONTRACT_FILE_NAME[0]) - 6] = contractIndex % 10 + L'0';
                loadedSize = load(CONTRACT_FILE_NAME, contractDescriptions[contractIndex].stateSize, contractStates[contractIndex]);
                if (loadedSize != contractDescriptions[contractIndex].stateSize)
                {
                    logStatusToConsole(L"EFI_FILE_PROTOCOL.Read() reads invalid number of bytes", loadedSize, __LINE__);

                    return false;
                }
            }

            initializeContract(contractIndex, contractStates[contractIndex]);
        }
        {
            setText(message, L"Computer digest = ");
            m256i digest;
            getComputerDigest(digest);
            CHAR16 digestChars[60 + 1];
            getIdentity((unsigned char*)&digest, digestChars, true);
            appendText(message, digestChars);
            appendText(message, L".");
            logToConsole(message);
        }
    }

    logToConsole(L"Allocating buffers ...");
    if ((status = bs->AllocatePool(EfiRuntimeServicesData, 536870912, (void**)&dejavu0))
        || (status = bs->AllocatePool(EfiRuntimeServicesData, 536870912, (void**)&dejavu1)))
    {
        logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

        return false;
    }
    bs->SetMem((void*)dejavu0, 536870912, 0);
    bs->SetMem((void*)dejavu1, 536870912, 0);

    if ((status = bs->AllocatePool(EfiRuntimeServicesData, REQUEST_QUEUE_BUFFER_SIZE, (void**)&requestQueueBuffer))
        || (status = bs->AllocatePool(EfiRuntimeServicesData, RESPONSE_QUEUE_BUFFER_SIZE, (void**)&responseQueueBuffer)))
    {
        logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

        return false;
    }

    for (unsigned int i = 0; i < NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS; i++)
    {
        peers[i].receiveData.FragmentCount = 1;
        peers[i].transmitData.FragmentCount = 1;
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, &peers[i].receiveBuffer))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, &peers[i].transmitData.FragmentTable[0].FragmentBuffer))
            || (status = bs->AllocatePool(EfiRuntimeServicesData, BUFFER_SIZE, (void**)&peers[i].dataToTransmit)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

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

    beginEpoch2of2();

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

    if (spectrumDigests)
    {
        bs->FreePool(spectrumDigests);
    }
    if (spectrum)
    {
        bs->FreePool(spectrum);
    }

    deinitAssets();

    if (reorgBuffer)
    {
        bs->FreePool(reorgBuffer);
    }

    deinitLogging();

    for (unsigned int processorIndex = 0; processorIndex < MAX_NUMBER_OF_PROCESSORS; processorIndex++)
    {
        if (contractFunctionOutputs[processorIndex])
        {
            bs->FreePool(contractFunctionOutputs[processorIndex]);
        }
    }
    if (contractStateCopy)
    {
        bs->FreePool(contractStateCopy);
    }
    if (contractStateChangeFlags)
    {
        bs->FreePool(contractStateChangeFlags);
    }
    for (unsigned int contractIndex = 0; contractIndex < sizeof(contractDescriptions) / sizeof(contractDescriptions[0]); contractIndex++)
    {
        if (contractStates[contractIndex])
        {
            bs->FreePool(contractStates[contractIndex]);
        }
    }

    if (entityPendingTransactionDigests)
    {
        bs->FreePool(entityPendingTransactionDigests);
    }
    if (entityPendingTransactions)
    {
        bs->FreePool(entityPendingTransactions);
    }
    if (tickTransactions)
    {
        bs->FreePool(tickTransactions);
    }
    if (tickData)
    {
        bs->FreePool(tickData);
    }
    if (ticks)
    {
        bs->FreePool(ticks);
    }
    if (tickTransactionOffsetsPtr)
    {
        bs->FreePool(tickTransactionOffsetsPtr);
    }

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
    if (tickData[system.tick + 1 - system.initialTick].epoch == system.epoch)
    {
        appendText(message, L"(");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].year / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].year % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].month / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].month % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].day / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].day % 10, FALSE);
        appendText(message, L" ");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].hour / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].hour % 10, FALSE);
        appendText(message, L":");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].minute / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].minute % 10, FALSE);
        appendText(message, L":");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].second / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].second % 10, FALSE);
        appendText(message, L".");
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].millisecond / 100, FALSE);
        appendNumber(message, (tickData[system.tick + 1 - system.initialTick].millisecond % 100) / 10, FALSE);
        appendNumber(message, tickData[system.tick + 1 - system.initialTick].millisecond % 10, FALSE);
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
    appendNumber(message, contractTotalExecutionTicks[QX_CONTRACT_INDEX] / frequency, TRUE);
    appendText(message, L" s.");
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
                    getIdentity(broadcastedComputors.broadcastComputors.computors.publicKeys[i].m256i_u8, message, false);
                    appendText(message, L" = ");
                    long long amount = 0;
                    const int spectrumIndex = ::spectrumIndex(broadcastedComputors.broadcastComputors.computors.publicKeys[i]);
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

            getIdentity((unsigned char*)&spectrumDigests[(SPECTRUM_CAPACITY * 2 - 1) - 1], digestChars, true);
            unsigned int numberOfEntities = 0;
            unsigned long long totalAmount = 0;
            for (unsigned int i = 0; i < SPECTRUM_CAPACITY; i++)
            {
                if (energy(i))
                {
                    numberOfEntities++;
                    totalAmount += energy(i);
                }
            }
            setNumber(message, totalAmount, TRUE);
            appendText(message, L" qus in ");
            appendNumber(message, numberOfEntities, TRUE);
            appendText(message, L" entities (digest = ");
            appendText(message, digestChars);
            appendText(message, L"); ");
            appendNumber(message, numberOfTransactions, TRUE);
            appendText(message, L" transactions.");
            logToConsole(message);

            m256i digest;

            setText(message, L"Universe digest = ");
            getUniverseDigest(digest);
            getIdentity((unsigned char*)&digest, digestChars, true);
            appendText(message, digestChars);
            appendText(message, L".");
            logToConsole(message);

            setText(message, L"Computer digest = ");
            getComputerDigest(digest);
            getIdentity((unsigned char*)&digest, digestChars, true);
            appendText(message, digestChars);
            appendText(message, L".");
            logToConsole(message);

            setText(message, L"resourceTestingDigest = ");
            appendNumber(message, resourceTestingDigest, false);
            appendText(message, L".");
            logToConsole(message);

            unsigned int numberOfPublishedSolutions = 0, numberOfRecordedSolutions = 0;
            for (unsigned int i = 0; i < system.numberOfSolutions; i++)
            {
                if (solutionPublicationTicks[i])
                {
                    numberOfPublishedSolutions++;

                    if (solutionPublicationTicks[i] < 0)
                    {
                        numberOfRecordedSolutions++;
                    }
                }
            }
            setNumber(message, numberOfRecordedSolutions, TRUE);
            appendText(message, L"/");
            appendNumber(message, numberOfPublishedSolutions, TRUE);
            appendText(message, L"/");
            appendNumber(message, system.numberOfSolutions, TRUE);
            appendText(message, L" solutions.");
            logToConsole(message);

            setText(message, (mainAuxStatus & 1) ? L"MAIN" : L"aux");
            appendText(message, L"&");
            appendText(message, (mainAuxStatus & 2) ? L"MAIN" : L"aux");
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
        * F9 Key
        * By Pressing the F9 Key the latestCreatedTick got's decreased by one.
        * By decreasing this by one, the Node will resend the issued votes for its Computors.
        */
        case 0x13:
        {
            system.latestCreatedTick--;
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
            mainAuxStatus = (mainAuxStatus + 1) & 3;
            setText(message, (mainAuxStatus & 1) ? L"MAIN" : L"aux");
            appendText(message, L"&");
            appendText(message, (mainAuxStatus & 2) ? L"MAIN" : L"aux");
            logToConsole(message);
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
                    logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

                    numberOfProcessors = 0;

                    break;
                }

                if (numberOfProcessors == 2)
                {
                    computingProcessorNumber = i;
                    contractProcessorIDs[nContractProcessorIDs++] = i;
                }
                else
                {
                    bs->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, shutdownCallback, NULL, &processors[numberOfProcessors].event);
                    mpServicesProtocol->StartupThisAP(mpServicesProtocol, numberOfProcessors == 1 ? tickProcessor : requestProcessor, i, processors[numberOfProcessors].event, 0, &processors[numberOfProcessors], NULL);

                    if (numberOfProcessors == 1)
                    {
                        tickProcessorIDs[nTickProcessorIDs++] = i;
                    }
                    else
                    {
                        requestProcessorIDs[nRequestProcessorIDs++] = i;
                    }

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


            // -----------------------------------------------------
            // Main loop
            unsigned int salt;
            _rdrand32_step(&salt);

            unsigned long long clockTick = 0, systemDataSavingTick = 0, loggingTick = 0, peerRefreshingTick = 0, tickRequestingTick = 0;
            unsigned int tickRequestingIndicator = 0, futureTickRequestingIndicator = 0;
            logToConsole(L"Init complete! Entering main loop ...");
            while (!shutDownNode)
            {
                if (criticalSituation == 1)
                {
                    logToConsole(L"CRITICAL SITUATION #1!!!");
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
                    mpServicesProtocol->StartupThisAP(mpServicesProtocol, contractProcessor, computingProcessorNumber, contractProcessorEvent, MAX_CONTRACT_ITERATION_DURATION * 1000, NULL, NULL);
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
                        if (!broadcastedComputors.broadcastComputors.computors.epoch
                            || broadcastedComputors.broadcastComputors.computors.epoch != system.epoch)
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
                    score->saveScoreCache();
                }

                if (curTimeTick - peerRefreshingTick >= PEER_REFRESHING_PERIOD * frequency / 1000)
                {
                    peerRefreshingTick = curTimeTick;

                    for (unsigned int i = 0; i < (NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS) / 4; i++)
                    {
                        closePeer(&peers[random(NUMBER_OF_OUTGOING_CONNECTIONS + NUMBER_OF_INCOMING_CONNECTIONS)]);
                    }
                }

                if (curTimeTick - tickRequestingTick >= TICK_REQUESTING_PERIOD * frequency / 1000)
                {
                    tickRequestingTick = curTimeTick;

                    if (tickRequestingIndicator == tickTotalNumberOfComputors)
                    {
                        requestedQuorumTick.header.randomizeDejavu();
                        requestedQuorumTick.requestQuorumTick.quorumTick.tick = system.tick;
                        bs->SetMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const unsigned int baseOffset = (system.tick - system.initialTick) * NUMBER_OF_COMPUTORS;
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            const Tick* tick = &ticks[baseOffset + i];
                            if (tick->epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
                    }
                    tickRequestingIndicator = tickTotalNumberOfComputors;
                    if (futureTickRequestingIndicator == futureTickTotalNumberOfComputors)
                    {
                        requestedQuorumTick.header.randomizeDejavu();
                        requestedQuorumTick.requestQuorumTick.quorumTick.tick = system.tick + 1;
                        bs->SetMem(&requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags, sizeof(requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags), 0);
                        const unsigned int baseOffset = (system.tick + 1 - system.initialTick) * NUMBER_OF_COMPUTORS;
                        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
                        {
                            const Tick* tick = &ticks[baseOffset + i];
                            if (tick->epoch == system.epoch)
                            {
                                requestedQuorumTick.requestQuorumTick.quorumTick.voteFlags[i >> 3] |= (1 << (i & 7));
                            }
                        }
                        pushToAny(&requestedQuorumTick.header);
                    }
                    futureTickRequestingIndicator = futureTickTotalNumberOfComputors;

                    if (tickData[system.tick + 1 - system.initialTick].epoch != system.epoch
                        || targetNextTickDataDigestIsKnown)
                    {
                        requestedTickData.header.randomizeDejavu();
                        requestedTickData.requestTickData.requestedTickData.tick = system.tick + 1;
                        pushToAny(&requestedTickData.header);
                    }
                    if (tickData[system.tick + 2 - system.initialTick].epoch != system.epoch)
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
                    spectrumMustBeSaved = false;
                    saveSpectrum();
                }
                if (universeMustBeSaved)
                {
                    universeMustBeSaved = false;
                    saveUniverse();
                }
                if (computerMustBeSaved)
                {
                    computerMustBeSaved = false;
                    saveComputer();
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
                }
                else
                {
                    mainLoopNumerator += __rdtsc() - curTimeTick;
                    mainLoopDenominator++;
                }
            }

            saveSystem();
            score->saveScoreCache();

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
