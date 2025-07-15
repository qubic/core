#pragma once

#include "platform/memory.h"
#include "platform/assert.h"
#include "network_messages/common_def.h"
#include "vote_counter.h"
#include "public_settings.h"


static unsigned long long gVoteScoreBuffer[NUMBER_OF_COMPUTORS];
static unsigned long long gCustomMiningScoreBuffer[NUMBER_OF_COMPUTORS];

static unsigned long long gTxScoreFactor[NUMBER_OF_COMPUTORS];
static constexpr unsigned long long gTxScoreScalingThreshold = (1ULL << 10);

static unsigned long long gVoteScoreFactor[NUMBER_OF_COMPUTORS];
static constexpr unsigned long long gVoteScoreScalingThreshold = (1ULL << 10);

static unsigned long long gCustomMiningScoreFactor[NUMBER_OF_COMPUTORS];
static constexpr unsigned long long gCustomMiningScoreScalingThreshold = (1ULL << 10);

static constexpr unsigned short gTxRevenuePoints[1 + 1024] = { 0, 710, 1125, 1420, 1648, 1835, 1993, 2129, 2250, 2358, 2455, 2545, 2627, 2702, 2773, 2839, 2901, 2960, 3015, 3068, 3118, 3165, 3211, 3254, 3296, 3336, 3375, 3412, 3448, 3483, 3516, 3549, 3580, 3611, 3641, 3670, 3698, 3725, 3751, 3777, 3803, 3827, 3851, 3875, 3898, 3921, 3943, 3964, 3985, 4006, 4026, 4046, 4066, 4085, 4104, 4122, 4140, 4158, 4175, 4193, 4210, 4226, 4243, 4259, 4275, 4290, 4306, 4321, 4336, 4350, 4365, 4379, 4393, 4407, 4421, 4435, 4448, 4461, 4474, 4487, 4500, 4512, 4525, 4537, 4549, 4561, 4573, 4585, 4596, 4608, 4619, 4630, 4641, 4652, 4663, 4674, 4685, 4695, 4705, 4716, 4726, 4736, 4746, 4756, 4766, 4775, 4785, 4795, 4804, 4813, 4823, 4832, 4841, 4850, 4859, 4868, 4876, 4885, 4894, 4902, 4911, 4919, 4928, 4936, 4944, 4952, 4960, 4968, 4976, 4984, 4992, 5000, 5008, 5015, 5023, 5031, 5038, 5046, 5053, 5060, 5068, 5075, 5082, 5089, 5096, 5103, 5110, 5117, 5124, 5131, 5138, 5144, 5151, 5158, 5164, 5171, 5178, 5184, 5191, 5197, 5203, 5210, 5216, 5222, 5228, 5235, 5241, 5247, 5253, 5259, 5265, 5271, 5277, 5283, 5289, 5295, 5300, 5306, 5312, 5318, 5323, 5329, 5335, 5340, 5346, 5351, 5357, 5362, 5368, 5373, 5378, 5384, 5389, 5394, 5400, 5405, 5410, 5415, 5420, 5425, 5431, 5436, 5441, 5446, 5451, 5456, 5461, 5466, 5471, 5475, 5480, 5485, 5490, 5495, 5500, 5504, 5509, 5514, 5518, 5523, 5528, 5532, 5537, 5542, 5546, 5551, 5555, 5560, 5564, 5569, 5573, 5577, 5582, 5586, 5591, 5595, 5599, 5604, 5608, 5612, 5616, 5621, 5625, 5629, 5633, 5637, 5642, 5646, 5650, 5654, 5658, 5662, 5666, 5670, 5674, 5678, 5682, 5686, 5690, 5694, 5698, 5702, 5706, 5710, 5714, 5718, 5721, 5725, 5729, 5733, 5737, 5740, 5744, 5748, 5752, 5755, 5759, 5763, 5766, 5770, 5774, 5777, 5781, 5785, 5788, 5792, 5795, 5799, 5802, 5806, 5809, 5813, 5816, 5820, 5823, 5827, 5830, 5834, 5837, 5841, 5844, 5847, 5851, 5854, 5858, 5861, 5864, 5868, 5871, 5874, 5878, 5881, 5884, 5887, 5891, 5894, 5897, 5900, 5904, 5907, 5910, 5913, 5916, 5919, 5923, 5926, 5929, 5932, 5935, 5938, 5941, 5944, 5948, 5951, 5954, 5957, 5960, 5963, 5966, 5969, 5972, 5975, 5978, 5981, 5984, 5987, 5990, 5993, 5996, 5999, 6001, 6004, 6007, 6010, 6013, 6016, 6019, 6022, 6025, 6027, 6030, 6033, 6036, 6039, 6041, 6044, 6047, 6050, 6053, 6055, 6058, 6061, 6064, 6066, 6069, 6072, 6075, 6077, 6080, 6083, 6085, 6088, 6091, 6093, 6096, 6099, 6101, 6104, 6107, 6109, 6112, 6115, 6117, 6120, 6122, 6125, 6128, 6130, 6133, 6135, 6138, 6140, 6143, 6145, 6148, 6151, 6153, 6156, 6158, 6161, 6163, 6166, 6168, 6170, 6173, 6175, 6178, 6180, 6183, 6185, 6188, 6190, 6193, 6195, 6197, 6200, 6202, 6205, 6207, 6209, 6212, 6214, 6216, 6219, 6221, 6224, 6226, 6228, 6231, 6233, 6235, 6238, 6240, 6242, 6244, 6247, 6249, 6251, 6254, 6256, 6258, 6260, 6263, 6265, 6267, 6269, 6272, 6274, 6276, 6278, 6281, 6283, 6285, 6287, 6289, 6292, 6294, 6296, 6298, 6300, 6303, 6305, 6307, 6309, 6311, 6313, 6316, 6318, 6320, 6322, 6324, 6326, 6328, 6330, 6333, 6335, 6337, 6339, 6341, 6343, 6345, 6347, 6349, 6351, 6353, 6356, 6358, 6360, 6362, 6364, 6366, 6368, 6370, 6372, 6374, 6376, 6378, 6380, 6382, 6384, 6386, 6388, 6390, 6392, 6394, 6396, 6398, 6400, 6402, 6404, 6406, 6408, 6410, 6412, 6414, 6416, 6418, 6420, 6421, 6423, 6425, 6427, 6429, 6431, 6433, 6435, 6437, 6439, 6441, 6443, 6444, 6446, 6448, 6450, 6452, 6454, 6456, 6458, 6459, 6461, 6463, 6465, 6467, 6469, 6471, 6472, 6474, 6476, 6478, 6480, 6482, 6483, 6485, 6487, 6489, 6491, 6493, 6494, 6496, 6498, 6500, 6502, 6503, 6505, 6507, 6509, 6510, 6512, 6514, 6516, 6518, 6519, 6521, 6523, 6525, 6526, 6528, 6530, 6532, 6533, 6535, 6537, 6538, 6540, 6542, 6544, 6545, 6547, 6549, 6550, 6552, 6554, 6556, 6557, 6559, 6561, 6562, 6564, 6566, 6567, 6569, 6571, 6572, 6574, 6576, 6577, 6579, 6581, 6582, 6584, 6586, 6587, 6589, 6591, 6592, 6594, 6596, 6597, 6599, 6600, 6602, 6604, 6605, 6607, 6609, 6610, 6612, 6613, 6615, 6617, 6618, 6620, 6621, 6623, 6625, 6626, 6628, 6629, 6631, 6632, 6634, 6636, 6637, 6639, 6640, 6642, 6643, 6645, 6647, 6648, 6650, 6651, 6653, 6654, 6656, 6657, 6659, 6660, 6662, 6663, 6665, 6667, 6668, 6670, 6671, 6673, 6674, 6676, 6677, 6679, 6680, 6682, 6683, 6685, 6686, 6688, 6689, 6691, 6692, 6694, 6695, 6697, 6698, 6699, 6701, 6702, 6704, 6705, 6707, 6708, 6710, 6711, 6713, 6714, 6716, 6717, 6718, 6720, 6721, 6723, 6724, 6726, 6727, 6729, 6730, 6731, 6733, 6734, 6736, 6737, 6739, 6740, 6741, 6743, 6744, 6746, 6747, 6748, 6750, 6751, 6753, 6754, 6755, 6757, 6758, 6760, 6761, 6762, 6764, 6765, 6767, 6768, 6769, 6771, 6772, 6773, 6775, 6776, 6778, 6779, 6780, 6782, 6783, 6784, 6786, 6787, 6788, 6790, 6791, 6793, 6794, 6795, 6797, 6798, 6799, 6801, 6802, 6803, 6805, 6806, 6807, 6809, 6810, 6811, 6813, 6814, 6815, 6816, 6818, 6819, 6820, 6822, 6823, 6824, 6826, 6827, 6828, 6830, 6831, 6832, 6833, 6835, 6836, 6837, 6839, 6840, 6841, 6842, 6844, 6845, 6846, 6848, 6849, 6850, 6851, 6853, 6854, 6855, 6856, 6858, 6859, 6860, 6862, 6863, 6864, 6865, 6867, 6868, 6869, 6870, 6872, 6873, 6874, 6875, 6877, 6878, 6879, 6880, 6882, 6883, 6884, 6885, 6886, 6888, 6889, 6890, 6891, 6893, 6894, 6895, 6896, 6897, 6899, 6900, 6901, 6902, 6904, 6905, 6906, 6907, 6908, 6910, 6911, 6912, 6913, 6914, 6916, 6917, 6918, 6919, 6920, 6921, 6923, 6924, 6925, 6926, 6927, 6929, 6930, 6931, 6932, 6933, 6934, 6936, 6937, 6938, 6939, 6940, 6941, 6943, 6944, 6945, 6946, 6947, 6948, 6950, 6951, 6952, 6953, 6954, 6955, 6957, 6958, 6959, 6960, 6961, 6962, 6963, 6965, 6966, 6967, 6968, 6969, 6970, 6971, 6972, 6974, 6975, 6976, 6977, 6978, 6979, 6980, 6981, 6983, 6984, 6985, 6986, 6987, 6988, 6989, 6990, 6991, 6993, 6994, 6995, 6996, 6997, 6998, 6999, 7000, 7001, 7003, 7004, 7005, 7006, 7007, 7008, 7009, 7010, 7011, 7012, 7013, 7015, 7016, 7017, 7018, 7019, 7020, 7021, 7022, 7023, 7024, 7025, 7026, 7027, 7029, 7030, 7031, 7032, 7033, 7034, 7035, 7036, 7037, 7038, 7039, 7040, 7041, 7042, 7043, 7044, 7046, 7047, 7048, 7049, 7050, 7051, 7052, 7053, 7054, 7055, 7056, 7057, 7058, 7059, 7060, 7061, 7062, 7063, 7064, 7065, 7066, 7067, 7068, 7069, 7070, 7071, 7073, 7074, 7075, 7076, 7077, 7078, 7079, 7080, 7081, 7082, 7083, 7084, 7085, 7086, 7087, 7088, 7089, 7090, 7091, 7092, 7093, 7094, 7095, 7096, 7097, 7098, 7099 };

static constexpr unsigned long long maxTxRevPoints = gTxRevenuePoints[sizeof(gTxRevenuePoints) / sizeof(gTxRevenuePoints[0]) - 1];
// Assert checkout for tx
static_assert(maxTxRevPoints* MAX_NUMBER_OF_TICKS_PER_EPOCH / 676 <= 0xFFFFFFFFFFFFFFFFULL / gTxScoreScalingThreshold,
    "Max value of tx score can make score overflow");
// Assert check for vote
static_assert(((1ULL << VOTE_COUNTER_NUM_BIT_PER_COMP) - 1)* NUMBER_OF_COMPUTORS* MAX_NUMBER_OF_TICKS_PER_EPOCH <= 0xFFFFFFFFFFFFFFFFULL / gVoteScoreScalingThreshold,
    "Max value of vote score can make score overflow");
// Assert check for custom mining score. Custom mining only happen in idle phase so for all epoch need to divided by 2
static_assert(((1ULL << VOTE_COUNTER_NUM_BIT_PER_COMP) - 1)* NUMBER_OF_COMPUTORS* MAX_NUMBER_OF_TICKS_PER_EPOCH / 2 <= 0xFFFFFFFFFFFFFFFFULL / gCustomMiningScoreScalingThreshold,
    "Max value of custom mininng score can make score overflow");


struct RevenueComponents
{
    unsigned long long txScore[NUMBER_OF_COMPUTORS];    // revenue score with txs
    unsigned long long voteScore[NUMBER_OF_COMPUTORS];  // vote count
    unsigned long long customMiningScore[NUMBER_OF_COMPUTORS]; // the shares count with custom mining

    unsigned long long txScoreFactor[NUMBER_OF_COMPUTORS];
    unsigned long long voteScoreFactor[NUMBER_OF_COMPUTORS];
    unsigned long long customMiningScoreFactor[NUMBER_OF_COMPUTORS];

    long long conservativeRevenue[NUMBER_OF_COMPUTORS];
    long long revenue[NUMBER_OF_COMPUTORS];
} gRevenueComponents;

// Get the lower bound that start to separate the QUORUM region of score
unsigned long long getQuorumScore(const unsigned long long* score)
{
    unsigned long long sortedScore[QUORUM + 1];
    // Sort revenue scores to get lowest score of quorum
    setMem(sortedScore, sizeof(sortedScore), 0);
    for (unsigned short computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        sortedScore[QUORUM] = score[computorIndex];
        unsigned int i = QUORUM;
        while (i
            && sortedScore[i - 1] < sortedScore[i])
        {
            const unsigned long long tmp = sortedScore[i - 1];
            sortedScore[i - 1] = sortedScore[i];
            sortedScore[i--] = tmp;
        }
    }
    if (!sortedScore[QUORUM - 1])
    {
        sortedScore[QUORUM - 1] = 1;
    }
    return sortedScore[QUORUM - 1];
}

// This function will sort the score take the 450th score at reference score
// Score greater than or equal this will be assigned as scalingThreshold value
// Score is zero will become zero
// Score lower than this value will be scaled at score * scalingThreshold / value
static void computeRevFactor(
    const unsigned long long* score,
    const unsigned long long scalingThreshold,
    unsigned long long* outputScoreFactor)
{
    ASSERT(scalingThreshold > 0);

    // Sort revenue scores to get lowest score of quorum
    unsigned long long quorumScore = getQuorumScore(score);

    for (unsigned int computorIndex = 0; computorIndex < NUMBER_OF_COMPUTORS; computorIndex++)
    {
        unsigned long long scoreFactor = 0;
        if (score[computorIndex] == 0)
        {
            scoreFactor = 0;
        }
        else if (score[computorIndex] >= quorumScore)
        {
            scoreFactor = scalingThreshold;
        }
        else // Lower than QUORUM score.
        {
            // Checking overflow. We don't expect falling
            ASSERT(0xFFFFFFFFFFFFFFFFULL / scalingThreshold >= score[computorIndex]);
            scoreFactor = scalingThreshold * score[computorIndex] / quorumScore;
        }

        outputScoreFactor[computorIndex] = scoreFactor;
    }
}

static void computeRevenue(
    const unsigned long long* txScore,
    const unsigned long long* voteScore,
    const unsigned long long* customMiningScore,
    long long* revenue = NULL)
{
    // Transaction score
    copyMem(gRevenueComponents.txScore, txScore, sizeof(gRevenueComponents.txScore));
    computeRevFactor(gRevenueComponents.txScore, gTxScoreScalingThreshold, gRevenueComponents.txScoreFactor);

    // Vote score
    copyMem(gRevenueComponents.voteScore, voteScore, sizeof(gRevenueComponents.voteScore));
    computeRevFactor(gRevenueComponents.voteScore, gVoteScoreScalingThreshold, gRevenueComponents.voteScoreFactor);

    // Custom mining score
    copyMem(gRevenueComponents.customMiningScore, customMiningScore, sizeof(gRevenueComponents.customMiningScore));
    computeRevFactor(gRevenueComponents.customMiningScore, gCustomMiningScoreScalingThreshold, gRevenueComponents.customMiningScoreFactor);

    long long arbitratorRevenue = ISSUANCE_RATE;
    constexpr long long issuancePerComputor = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;
    constexpr long long scalingThreshold = 0xFFFFFFFFFFFFFFFFULL / issuancePerComputor;
    static_assert(gTxScoreScalingThreshold * gVoteScoreScalingThreshold * gCustomMiningScoreScalingThreshold <= scalingThreshold, "Normalize factor can cause overflow");

    // Save data of custom mining. But not apply yet
    {
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            unsigned long long txFactor = gRevenueComponents.txScoreFactor[i];
            unsigned long long voteFactor = gRevenueComponents.voteScoreFactor[i];
            unsigned long long customFactor = gRevenueComponents.customMiningScoreFactor[i];
            ASSERT(txFactor <= gTxScoreScalingThreshold);
            ASSERT(voteFactor <= gVoteScoreScalingThreshold);
            ASSERT(customFactor <= gCustomMiningScoreScalingThreshold);
            static_assert(gTxScoreScalingThreshold * gVoteScoreScalingThreshold * gCustomMiningScoreScalingThreshold < 0xFFFFFFFFFFFFFFFFULL / issuancePerComputor);

            unsigned long long combinedScoreFactor = txFactor * voteFactor * customFactor;

            gRevenueComponents.revenue[i] =
                (long long)(combinedScoreFactor * issuancePerComputor / gTxScoreScalingThreshold / gVoteScoreScalingThreshold / gCustomMiningScoreScalingThreshold);
        }
    }

    // Apply the new revenue formula
    if (NULL != revenue)
    {
        copyMem(revenue, gRevenueComponents.revenue, sizeof(gRevenueComponents.revenue));
    }
}


