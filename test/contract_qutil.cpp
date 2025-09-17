#define NO_UEFI

#include "contract_testing.h"

#include <random>

class ContractTestingQUtil : public ContractTesting {
public:
    ContractTestingQUtil() {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUTIL);
        callSystemProcedure(QUTIL_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUTIL_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
        callSystemProcedure(QX_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUTIL_CONTRACT_INDEX, END_EPOCH, expectSuccess);
        callSystemProcedure(QX_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }


    QX::IssueAsset_output issueAsset(const id& issuer, uint64_t assetName, uint64_t numberOfShares) {
        QX::IssueAsset_input input;
        input.assetName = assetName;
        input.numberOfShares = numberOfShares;
        input.unitOfMeasurement = 0;
        input.numberOfDecimalPlaces = 0;
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, 1000000000);
        return output;
    }

    QX::TransferShareOwnershipAndPossession_output transferAsset(const id& from, const id& to, const Asset& asset, uint64_t amount) {
        QX::TransferShareOwnershipAndPossession_input input;
        input.issuer = asset.issuer;
        input.newOwnerAndPossessor = to;
        input.assetName = asset.assetName;
        input.numberOfShares = amount;
        QX::TransferShareOwnershipAndPossession_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, from, 1000000);
        return output;
    }

    QUTIL::CreatePoll_output createPoll(const id& creator, const QUTIL::CreatePoll_input& input, uint64_t fee) {
        QUTIL::CreatePoll_output output;
        memset(&output, 0, sizeof(output));
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, 4, input, output, creator, fee);
        return output;
    }

    QUTIL::Vote_output vote(const id& voter, const QUTIL::Vote_input& input, uint64_t fee) {
        QUTIL::Vote_output output;
        memset(&output, 0, sizeof(output));
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, 5, input, output, voter, fee);
        return output;
    }

    QUTIL::GetCurrentResult_output getCurrentResult(uint64_t poll_id) {
        QUTIL::GetCurrentResult_input input;
        input.poll_id = poll_id;
        QUTIL::GetCurrentResult_output output;
        memset(&output, 0, sizeof(output));
        callFunction(QUTIL_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QUTIL::GetPollsByCreator_output getPollsByCreator(const id& creator) {
        QUTIL::GetPollsByCreator_input input;
        input.creator = creator;
        QUTIL::GetPollsByCreator_output output;
        memset(&output, 0, sizeof(output));
        callFunction(QUTIL_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QUTIL::GetCurrentPollId_output getCurrentPollId()
    {
        QUTIL::GetCurrentPollId_input input;
        QUTIL::GetCurrentPollId_output output;
        memset(&output, 0, sizeof(output));
        callFunction(QUTIL_CONTRACT_INDEX, 5, input, output);
        return output;
    }

    QUTIL::GetPollInfo_output getPollInfo(uint64_t poll_id)
    {
        QUTIL::GetPollInfo_input input;
        input.poll_id = poll_id;
        QUTIL::GetPollInfo_output output;
        memset(&output, 0, sizeof(output));
        callFunction(QUTIL_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    QUTIL::DistributeQuToShareholders_output distributeQuToShareholders(const id& invocator, const Asset& asset, sint64 amount) {
        QUTIL::DistributeQuToShareholders_input input{ asset };
        QUTIL::DistributeQuToShareholders_output output;
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, 7, input, output, invocator, amount);
        return output;
    }
};

// Helper function to generate random ID
id generateRandomId() {
    return id::randomValue();
}

// Helper function to convert string to Array<uint8, 256>
Array<uint8, 256> stringToArray(const std::string& str) {
    Array<uint8, 256> arr;
    size_t len = std::min(str.size(), static_cast<size_t>(256));
    for (size_t i = 0; i < len; ++i) {
        arr.set(i, static_cast<uint8>(str[i]));
    }
    for (size_t i = len; i < 256; ++i) {
        arr.set(i, 0);
    }
    return arr;
}

// Helper function to generate a dummy Asset
Asset generateAsset() {
    Asset asset;
    asset.issuer = generateRandomId();
    asset.assetName = 12345; // Simple name for testing
    return asset;
}

TEST(QUtilTest, CreateMultiplePolls_CheckIds)
{
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    uint64_t num_polls = 16;
    std::vector<uint64_t> created_poll_ids;

    for (uint64_t i = 0; i < num_polls; ++i)
    {
        id poll_name = generateRandomId();
        uint64_t min_amount = 1000;
        Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/test" + std::to_string(i));
        uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

        QUTIL::CreatePoll_input input;
        input.poll_name = poll_name;
        input.poll_type = poll_type;
        input.min_amount = min_amount;
        input.github_link = github_link;
        input.num_assets = 0;

        increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
        auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
        created_poll_ids.push_back(output.poll_id);
    }

    auto current_poll_info = qutil.getCurrentPollId();
    EXPECT_EQ(current_poll_info.current_poll_id, num_polls);
    EXPECT_EQ(current_poll_info.active_count, num_polls);

    std::set<uint64_t> expected_ids(created_poll_ids.begin(), created_poll_ids.end());
    std::set<uint64_t> active_ids;
    for (uint64_t i = 0; i < current_poll_info.active_count; ++i)
    {
        active_ids.insert(current_poll_info.active_poll_ids.get(i));
    }
    EXPECT_EQ(active_ids, expected_ids);
}

TEST(QUtilTest, CreateMultiplePolls_CheckNames)
{
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    uint64_t num_polls = 5;
    std::vector<id> poll_names;
    std::vector<uint64_t> poll_ids;

    for (uint64_t i = 0; i < num_polls; ++i)
    {
        id poll_name = generateRandomId();
        poll_names.push_back(poll_name);
        uint64_t min_amount = 1000;
        Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/test" + std::to_string(i));
        uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

        QUTIL::CreatePoll_input input;
        input.poll_name = poll_name;
        input.poll_type = poll_type;
        input.min_amount = min_amount;
        input.github_link = github_link;
        input.num_assets = 0;

        increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
        auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
        poll_ids.push_back(output.poll_id);
    }

    for (uint64_t i = 0; i < num_polls; ++i)
    {
        auto poll_info = qutil.getPollInfo(poll_ids[i]);
        EXPECT_EQ(poll_info.found, 1);
        EXPECT_EQ(poll_info.poll_info.poll_name, poll_names[i]);
    }
}

TEST(QUtilTest, CreatePollsMoreThanMax_CheckActiveIds)
{
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    uint64_t num_polls_per_epoch = QUTIL_MAX_NEW_POLL; // 16 polls per epoch
    uint64_t num_epochs = 2;
    std::vector<uint64_t> created_poll_ids;

    for (uint64_t epoch = 0; epoch < num_epochs; ++epoch)
    {
        for (uint64_t i = 0; i < num_polls_per_epoch; ++i)
        {
            id poll_name = generateRandomId();
            uint64_t min_amount = 1000;
            Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/test" + std::to_string(epoch * num_polls_per_epoch + i));
            uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

            QUTIL::CreatePoll_input input;
            input.poll_name = poll_name;
            input.poll_type = poll_type;
            input.min_amount = min_amount;
            input.github_link = github_link;
            input.num_assets = 0;

            increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
            auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
            created_poll_ids.push_back(output.poll_id);
        }
        if (epoch < num_epochs - 1)
        {
            qutil.endEpoch();
            qutil.beginEpoch();
        }
    }

    auto current_poll_info = qutil.getCurrentPollId();
    EXPECT_EQ(current_poll_info.current_poll_id, num_polls_per_epoch * num_epochs); // Total polls created: 32
    EXPECT_EQ(current_poll_info.active_count, num_polls_per_epoch); // Only 16 active in current epoch

    std::set<uint64_t> expected_active_ids;
    for (uint64_t i = (num_epochs - 1) * num_polls_per_epoch; i < num_epochs * num_polls_per_epoch; ++i)
    {
        expected_active_ids.insert(i); // IDs 16 to 31 should be active
    }

    std::set<uint64_t> active_ids;
    for (uint64_t i = 0; i < current_poll_info.active_count; ++i)
    {
        active_ids.insert(current_poll_info.active_poll_ids.get(i));
    }
    EXPECT_EQ(active_ids, expected_active_ids);
}

TEST(QUtilTest, CreatePollsMoreThanMax_CheckPollInfo)
{
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    uint64_t num_polls_per_epoch = QUTIL_MAX_NEW_POLL; // 16 polls per epoch
    uint64_t num_epochs = 2;
    std::map<uint64_t, id> poll_id_to_name;

    for (uint64_t epoch = 0; epoch < num_epochs; ++epoch)
    {
        for (uint64_t i = 0; i < num_polls_per_epoch; ++i)
        {
            id poll_name = generateRandomId();
            uint64_t poll_id = epoch * num_polls_per_epoch + i;
            poll_id_to_name[poll_id] = poll_name;
            uint64_t min_amount = 1000;
            Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/test" + std::to_string(poll_id));
            uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

            QUTIL::CreatePoll_input input;
            input.poll_name = poll_name;
            input.poll_type = poll_type;
            input.min_amount = min_amount;
            input.github_link = github_link;
            input.num_assets = 0;

            increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
            auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
            EXPECT_EQ(output.poll_id, poll_id);
        }
        if (epoch < num_epochs - 1)
        {
            qutil.endEpoch();
            qutil.beginEpoch();
        }
    }

    // Check polls from the first epoch (IDs 0-15, should be deactivated)
    for (uint64_t i = 0; i < num_polls_per_epoch; ++i)
    {
        auto poll_info = qutil.getPollInfo(i);
        EXPECT_EQ(poll_info.found, 1); // Poll exists but is inactive
        EXPECT_EQ(poll_info.poll_info.is_active, 0);
    }

    // Check polls from the second epoch (IDs 16-31, should be active)
    for (uint64_t i = num_polls_per_epoch; i < num_polls_per_epoch * 2; ++i)
    {
        auto poll_info = qutil.getPollInfo(i);
        EXPECT_EQ(poll_info.found, 1);
        EXPECT_EQ(poll_info.poll_info.is_active, 1);
        EXPECT_EQ(poll_info.poll_info.poll_name, poll_id_to_name[i]);
    }
}

TEST(QUtilTest, CreatePolls_Vote_PassEpoch_CreateNewPolls_Vote_CheckResults) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    uint64_t min_amount = 1000;

    // Create poll 0
    id poll_name0 = generateRandomId();
    QUTIL::CreatePoll_input create_input0;
    create_input0.poll_name = poll_name0;
    create_input0.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input0.min_amount = min_amount;
    create_input0.github_link = stringToArray("https://github.com/qubic/proposal/poll0");
    create_input0.num_assets = 0;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output0 = qutil.createPoll(creator, create_input0, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id0 = create_output0.poll_id;

    // Create poll 1
    id poll_name1 = generateRandomId();
    QUTIL::CreatePoll_input create_input1;
    create_input1.poll_name = poll_name1;
    create_input1.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input1.min_amount = min_amount;
    create_input1.github_link = stringToArray("https://github.com/qubic/proposal/poll1");
    create_input1.num_assets = 0;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output1 = qutil.createPoll(creator, create_input1, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id1 = create_output1.poll_id;

    // Vote on poll 0: 2 votes for option 0, 1 for option 1
    id voter0 = generateRandomId();
    increaseEnergy(voter0, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input0;
    vote_input0.poll_id = poll_id0;
    vote_input0.address = voter0;
    vote_input0.amount = min_amount;
    vote_input0.chosen_option = 0;
    qutil.vote(voter0, vote_input0, QUTIL_VOTE_FEE);

    id voter1 = generateRandomId();
    increaseEnergy(voter1, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input1;
    vote_input1.poll_id = poll_id0;
    vote_input1.address = voter1;
    vote_input1.amount = min_amount;
    vote_input1.chosen_option = 0;
    qutil.vote(voter1, vote_input1, QUTIL_VOTE_FEE);

    id voter2 = generateRandomId();
    increaseEnergy(voter2, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input2;
    vote_input2.poll_id = poll_id0;
    vote_input2.address = voter2;
    vote_input2.amount = min_amount;
    vote_input2.chosen_option = 1;
    qutil.vote(voter2, vote_input2, QUTIL_VOTE_FEE);

    // Vote on poll 1: 1 vote for option 0, 2 for option 1
    id voter3 = generateRandomId();
    increaseEnergy(voter3, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input3;
    vote_input3.poll_id = poll_id1;
    vote_input3.address = voter3;
    vote_input3.amount = min_amount;
    vote_input3.chosen_option = 0;
    qutil.vote(voter3, vote_input3, QUTIL_VOTE_FEE);

    id voter4 = generateRandomId();
    increaseEnergy(voter4, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input4;
    vote_input4.poll_id = poll_id1;
    vote_input4.address = voter4;
    vote_input4.amount = min_amount;
    vote_input4.chosen_option = 1;
    qutil.vote(voter4, vote_input4, QUTIL_VOTE_FEE);

    id voter5 = generateRandomId();
    increaseEnergy(voter5, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input5;
    vote_input5.poll_id = poll_id1;
    vote_input5.address = voter5;
    vote_input5.amount = min_amount;
    vote_input5.chosen_option = 1;
    qutil.vote(voter5, vote_input5, QUTIL_VOTE_FEE);

    // Pass the epoch
    qutil.endEpoch();

    // Create poll 2
    id poll_name2 = generateRandomId();
    QUTIL::CreatePoll_input create_input2;
    create_input2.poll_name = poll_name2;
    create_input2.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input2.min_amount = min_amount;
    create_input2.github_link = stringToArray("https://github.com/qubic/proposal/poll2");
    create_input2.num_assets = 0;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output2 = qutil.createPoll(creator, create_input2, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id2 = create_output2.poll_id;

    // Create poll 3
    id poll_name3 = generateRandomId();
    QUTIL::CreatePoll_input create_input3;
    create_input3.poll_name = poll_name3;
    create_input3.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input3.min_amount = min_amount;
    create_input3.github_link = stringToArray("https://github.com/qubic/proposal/poll3");
    create_input3.num_assets = 0;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output3 = qutil.createPoll(creator, create_input3, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id3 = create_output3.poll_id;

    // Vote on poll 2: 1 vote for option 2
    id voter6 = generateRandomId();
    increaseEnergy(voter6, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input6;
    vote_input6.poll_id = poll_id2;
    vote_input6.address = voter6;
    vote_input6.amount = min_amount;
    vote_input6.chosen_option = 2;
    qutil.vote(voter6, vote_input6, QUTIL_VOTE_FEE);

    // Vote on poll 3: 1 vote for option 3
    id voter7 = generateRandomId();
    increaseEnergy(voter7, min_amount + QUTIL_VOTE_FEE);
    QUTIL::Vote_input vote_input7;
    vote_input7.poll_id = poll_id3;
    vote_input7.address = voter7;
    vote_input7.amount = min_amount;
    vote_input7.chosen_option = 3;
    qutil.vote(voter7, vote_input7, QUTIL_VOTE_FEE);

    // Check results for old polls
    auto result0 = qutil.getCurrentResult(poll_id0);
    EXPECT_EQ(result0.is_active, 0);
    EXPECT_EQ(result0.result.get(0), 2 * min_amount);
    EXPECT_EQ(result0.result.get(1), min_amount);
    EXPECT_EQ(result0.voter_count.get(0), 2);
    EXPECT_EQ(result0.voter_count.get(1), 1);

    auto result1 = qutil.getCurrentResult(poll_id1);
    EXPECT_EQ(result1.is_active, 0);
    EXPECT_EQ(result1.result.get(0), min_amount); // One vote for option 0
    EXPECT_EQ(result1.result.get(1), 2 * min_amount); // Two votes for option 1
    EXPECT_EQ(result1.voter_count.get(0), 1);
    EXPECT_EQ(result1.voter_count.get(1), 2);

    // Check results for new polls
    auto result2 = qutil.getCurrentResult(poll_id2);
    EXPECT_EQ(result2.is_active, 1);
    EXPECT_EQ(result2.result.get(2), min_amount);
    EXPECT_EQ(result2.voter_count.get(2), 1);
    for (uint64_t i = 0; i < QUTIL_MAX_OPTIONS; ++i)
    {
        if (i != 2)
        {
            EXPECT_EQ(result2.result.get(i), 0);
            EXPECT_EQ(result2.voter_count.get(i), 0);
        }
    }

    auto result3 = qutil.getCurrentResult(poll_id3);
    EXPECT_EQ(result3.is_active, 1);
    EXPECT_EQ(result3.result.get(3), min_amount);
    EXPECT_EQ(result3.voter_count.get(3), 1);
    for (uint64_t i = 0; i < QUTIL_MAX_OPTIONS; ++i)
    {
        if (i != 3)
        {
            EXPECT_EQ(result3.result.get(i), 0);
            EXPECT_EQ(result3.voter_count.get(i), 0);
        }
    }
}

TEST(QUtilTest, VoterListUpdateAndCompaction) {
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    uint64_t min_amount = 1000;
    id poll_name = generateRandomId();
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/test");
    uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = poll_type;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id0 = create_output.poll_id;

    id voterA = generateRandomId();
    id voterB = generateRandomId();
    id voterC = generateRandomId();
    id voterD = generateRandomId();
    id voterE = generateRandomId();
    id voterF = generateRandomId();
    id voterG = generateRandomId();

    // Give each voter enough energy for voting (min_amount + fee) for two polls
    increaseEnergy(voterA, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterB, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterC, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterD, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterE, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterF, min_amount + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterG, min_amount + 2 * QUTIL_VOTE_FEE);

    int voterA_index = spectrumIndex(voterA);
    int voterB_index = spectrumIndex(voterB);
    int voterC_index = spectrumIndex(voterC);
    int voterD_index = spectrumIndex(voterD);
    int voterE_index = spectrumIndex(voterE);
    int voterF_index = spectrumIndex(voterF);
    int voterG_index = spectrumIndex(voterG);

    // Scenario 1: All voters valid for Poll 0
    QUTIL::Vote_input vote_inputA;
    vote_inputA.poll_id = poll_id0;
    vote_inputA.address = voterA;
    vote_inputA.amount = min_amount;
    vote_inputA.chosen_option = 0;
    qutil.vote(voterA, vote_inputA, QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_inputB;
    vote_inputB.poll_id = poll_id0;
    vote_inputB.address = voterB;
    vote_inputB.amount = min_amount;
    vote_inputB.chosen_option = 0;
    qutil.vote(voterB, vote_inputB, QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_inputC;
    vote_inputC.poll_id = poll_id0;
    vote_inputC.address = voterC;
    vote_inputC.amount = min_amount;
    vote_inputC.chosen_option = 0;
    qutil.vote(voterC, vote_inputC, QUTIL_VOTE_FEE);

    auto result = qutil.getCurrentResult(poll_id0);
    EXPECT_EQ(result.result.get(0), 3000); // A, B, C: 1000 each
    EXPECT_EQ(result.voter_count.get(0), 3);

    auto balance_b = getBalance(voterB);
    // Scenario 2: Invalidate voterB by decreasing energy below min_amount
    decreaseEnergy(voterB_index, min_amount + QUTIL_VOTE_FEE - 500); // Leave 500, below min_amount
    balance_b = getBalance(voterB);

    QUTIL::Vote_input vote_inputD;
    vote_inputD.poll_id = poll_id0;
    vote_inputD.address = voterD;
    vote_inputD.amount = min_amount;
    vote_inputD.chosen_option = 0;
    qutil.vote(voterD, vote_inputD, QUTIL_VOTE_FEE);

    result = qutil.getCurrentResult(poll_id0);
    EXPECT_EQ(result.result.get(0), 3000); // A, C, D: 1000 each, B invalid
    EXPECT_EQ(result.voter_count.get(0), 3);

    // Scenario 3: Invalidate voterA and voterC
    decreaseEnergy(voterA_index, min_amount + QUTIL_VOTE_FEE - 500); // Leave 500
    decreaseEnergy(voterC_index, min_amount + QUTIL_VOTE_FEE - 500); // Leave 500

    QUTIL::Vote_input vote_inputE;
    vote_inputE.poll_id = poll_id0;
    vote_inputE.address = voterE;
    vote_inputE.amount = min_amount;
    vote_inputE.chosen_option = 0;
    qutil.vote(voterE, vote_inputE, QUTIL_VOTE_FEE);

    result = qutil.getCurrentResult(poll_id0);
    EXPECT_EQ(result.result.get(0), 2000); // D, E: 1000 each, others invalid
    EXPECT_EQ(result.voter_count.get(0), 2);

    // Scenario 4: Single voter with new poll
    id poll_name2 = generateRandomId();
    QUTIL::CreatePoll_input create_input2;
    create_input2.poll_name = poll_name2;
    create_input2.poll_type = poll_type;
    create_input2.min_amount = min_amount;
    create_input2.github_link = github_link;
    create_input2.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output2 = qutil.createPoll(creator, create_input2, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id2 = create_output2.poll_id;

    id voterH = generateRandomId();
    increaseEnergy(voterH, min_amount + QUTIL_VOTE_FEE);
    int voterH_index = spectrumIndex(voterH);
    QUTIL::Vote_input vote_inputH;
    vote_inputH.poll_id = poll_id2;
    vote_inputH.address = voterH;
    vote_inputH.amount = min_amount;
    vote_inputH.chosen_option = 0;
    qutil.vote(voterH, vote_inputH, QUTIL_VOTE_FEE);

    result = qutil.getCurrentResult(poll_id2);
    EXPECT_EQ(result.result.get(0), 1000); // H: 1000
    EXPECT_EQ(result.voter_count.get(0), 1);

    // Scenario 5: Multiple polls with voter invalidation and new votes
    // Create a new poll (Poll 1)
    id poll_name1 = generateRandomId();
    QUTIL::CreatePoll_input create_input1;
    create_input1.poll_name = poll_name1;
    create_input1.poll_type = poll_type;
    create_input1.min_amount = min_amount;
    create_input1.github_link = github_link;
    create_input1.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output1 = qutil.createPoll(creator, create_input1, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id1 = create_output1.poll_id;

    // Voters D, E voted for Poll 0, already did above. Only A, B, C need to revote again. F now votes for Poll 0
    increaseEnergy(voterA, 500 + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterB, 500 + 2 * QUTIL_VOTE_FEE);
    increaseEnergy(voterC, 500 + 2 * QUTIL_VOTE_FEE);

    EXPECT_EQ(getBalance(voterA), 1200);
    EXPECT_EQ(getBalance(voterB), 1200);
    EXPECT_EQ(getBalance(voterC), 1200);
    EXPECT_EQ(getBalance(voterD), 1100);
    EXPECT_EQ(getBalance(voterE), 1100);
    EXPECT_EQ(getBalance(voterF), 1200);
    EXPECT_EQ(getBalance(voterG), 1200);

    vote_inputB.poll_id = poll_id0;
    qutil.vote(voterB, vote_inputB, QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_inputF;
    vote_inputF.poll_id = poll_id0;
    vote_inputF.address = voterF;
    vote_inputF.amount = min_amount;
    vote_inputF.chosen_option = 0;
    qutil.vote(voterF, vote_inputF, QUTIL_VOTE_FEE);

    // Add A and C voting for Poll 0 again
    vote_inputA.poll_id = poll_id0;
    qutil.vote(voterA, vote_inputA, QUTIL_VOTE_FEE);
    vote_inputC.poll_id = poll_id0;
    qutil.vote(voterC, vote_inputC, QUTIL_VOTE_FEE);

    // Voters A, B, C, D, E, F vote for Poll 1
    vote_inputA.poll_id = poll_id1;
    qutil.vote(voterA, vote_inputA, QUTIL_VOTE_FEE);
    vote_inputB.poll_id = poll_id1;
    qutil.vote(voterB, vote_inputB, QUTIL_VOTE_FEE);
    vote_inputC.poll_id = poll_id1;
    qutil.vote(voterC, vote_inputC, QUTIL_VOTE_FEE);
    vote_inputD.poll_id = poll_id1;
    qutil.vote(voterD, vote_inputD, QUTIL_VOTE_FEE);
    vote_inputE.poll_id = poll_id1;
    qutil.vote(voterE, vote_inputE, QUTIL_VOTE_FEE);
    vote_inputF.poll_id = poll_id1;
    qutil.vote(voterF, vote_inputF, QUTIL_VOTE_FEE);

    EXPECT_EQ(getBalance(voterA), 1000);
    EXPECT_EQ(getBalance(voterB), 1000);
    EXPECT_EQ(getBalance(voterC), 1000);
    EXPECT_EQ(getBalance(voterD), 1000);
    EXPECT_EQ(getBalance(voterE), 1000);
    EXPECT_EQ(getBalance(voterF), 1000);
    EXPECT_EQ(getBalance(voterG), 1200);

    // Decrease energy for voters B and D below min_amount for both polls
    decreaseEnergy(voterB_index, 500);
    decreaseEnergy(voterD_index, 500);

    EXPECT_EQ(getBalance(voterA), 1000);
    EXPECT_EQ(getBalance(voterB), 500);
    EXPECT_EQ(getBalance(voterC), 1000);
    EXPECT_EQ(getBalance(voterD), 500);
    EXPECT_EQ(getBalance(voterE), 1000);
    EXPECT_EQ(getBalance(voterF), 1000);
    EXPECT_EQ(getBalance(voterG), 1200);

    // Voter G votes for both Poll 0 and Poll 1
    QUTIL::Vote_input vote_inputG;
    vote_inputG.address = voterG;
    vote_inputG.amount = min_amount;
    vote_inputG.chosen_option = 0;

    vote_inputG.poll_id = poll_id0;
    qutil.vote(voterG, vote_inputG, QUTIL_VOTE_FEE);
    vote_inputG.poll_id = poll_id1;
    qutil.vote(voterG, vote_inputG, QUTIL_VOTE_FEE);

    // Get and verify results for Poll 0
    result = qutil.getCurrentResult(poll_id0);
    EXPECT_EQ(result.result.get(0), 5000); // A, C, E, F, G: 1000 each, B and D invalid
    EXPECT_EQ(result.voter_count.get(0), 5);

    // Get and verify results for Poll 1
    result = qutil.getCurrentResult(poll_id1);
    EXPECT_EQ(result.result.get(0), 5000); // A, C, E, F, G: 1000 each, B and D invalid
    EXPECT_EQ(result.voter_count.get(0), 5);
}

// Test successful Qubic poll creation
TEST(QUtilTest, CreatePoll_Success_Qubic) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    uint64_t num_assets = 0;
    uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = poll_type;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.allowed_assets = allowed_assets;
    input.num_assets = num_assets;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = output.poll_id;

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 1);
    EXPECT_EQ(polls.poll_ids.get(0), poll_id);

    auto result = qutil.getCurrentResult(poll_id);
    for (uint64_t i = 0; i < QUTIL_MAX_OPTIONS; ++i) {
        EXPECT_EQ(result.result.get(i), 0); // No votes yet
    }
}

// Test successful Asset poll creation
TEST(QUtilTest, CreatePoll_Success_Asset) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, generateAsset());
    uint64_t num_assets = 1;
    uint64_t poll_type = QUTIL_POLL_TYPE_ASSET;

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = poll_type;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.allowed_assets = allowed_assets;
    input.num_assets = num_assets;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = output.poll_id;

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 1);
    EXPECT_EQ(polls.poll_ids.get(0), poll_id);
}

// Test CreatePoll failure due to insufficient funds
TEST(QUtilTest, CreatePoll_InsufficientFunds) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist
    uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = poll_type;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE - 1);
    qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE - 1);

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Test CreatePoll failure due to invalid poll type
TEST(QUtilTest, CreatePoll_InvalidPollType) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = 3; // Invalid type
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Test CreatePoll failure due to invalid num_assets for Qubic poll
TEST(QUtilTest, CreatePoll_InvalidNumAssetsQubic) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.num_assets = 1;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Test CreatePoll failure due to invalid num_assets for Asset poll
TEST(QUtilTest, CreatePoll_InvalidNumAssetsAsset) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = QUTIL_POLL_TYPE_ASSET;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Test successful voting
TEST(QUtilTest, Vote_Success) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist
    uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = poll_type;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    id voter = generateRandomId();
    increaseEnergy(voter, min_amount + QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = poll_id;
    vote_input.address = voter;
    vote_input.amount = min_amount;
    vote_input.chosen_option = 0;

    auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_output.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), min_amount);
    EXPECT_EQ(result.voter_count.get(0), 1);
}

// Test Vote failure due to insufficient fee
TEST(QUtilTest, Vote_InsufficientFee) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    id voter = generateRandomId();
    increaseEnergy(voter, min_amount + QUTIL_VOTE_FEE - 1);

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = poll_id;
    vote_input.address = voter;
    vote_input.amount = min_amount;
    vote_input.chosen_option = 0;

    auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE - 1);
    EXPECT_FALSE(vote_output.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.voter_count.get(0), 0);
}

// Test Vote failure due to invalid poll ID
TEST(QUtilTest, Vote_InvalidPollId) {
    ContractTestingQUtil qutil;
    id voter = generateRandomId();
    increaseEnergy(voter, 1000 + QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = 999; // Non-existent poll
    vote_input.address = voter;
    vote_input.amount = 1000;
    vote_input.chosen_option = 0;

    auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_output.success);
}

// Test Vote failure due to insufficient balance
TEST(QUtilTest, Vote_InsufficientBalance) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    id voter = generateRandomId();
    increaseEnergy(voter, min_amount - 1 + QUTIL_VOTE_FEE); // Less than min_amount

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = poll_id;
    vote_input.address = voter;
    vote_input.amount = min_amount;
    vote_input.chosen_option = 0;

    auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_output.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.voter_count.get(0), 0);
}

// Test Vote failure due to invalid option
TEST(QUtilTest, Vote_InvalidOption) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    id voter = generateRandomId();
    increaseEnergy(voter, min_amount + QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = poll_id;
    vote_input.address = voter;
    vote_input.amount = min_amount;
    vote_input.chosen_option = QUTIL_MAX_OPTIONS; // 64 is invalid

    auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_output.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.voter_count.get(0), 0);
}

// Test GetCurrentResult success
TEST(QUtilTest, GetCurrentResult_Success) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    id voter = generateRandomId();
    increaseEnergy(voter, min_amount + QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_input;
    vote_input.poll_id = poll_id;
    vote_input.address = voter;
    vote_input.amount = min_amount;
    vote_input.chosen_option = 1;

    qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(1), min_amount);
    EXPECT_EQ(result.voter_count.get(1), 1);
}

// Test GetCurrentResult failure due to invalid poll ID
TEST(QUtilTest, GetCurrentResult_InvalidPollId) {
    ContractTestingQUtil qutil;
    auto result = qutil.getCurrentResult(999); // Non-existent poll
    for (uint64_t i = 0; i < QUTIL_MAX_OPTIONS; ++i) {
        EXPECT_EQ(result.result.get(i), 0); // Should return default values
    }
}

// Test GetPollsByCreator with polls
TEST(QUtilTest, GetPollsByCreator_WithPolls) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name1 = generateRandomId();
    id poll_name2 = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input input;
    input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    input.min_amount = min_amount;
    input.github_link = github_link;
    input.num_assets = 0;

    input.poll_name = poll_name1;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto output1 = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);

    input.poll_name = poll_name2;
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto output2 = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 2);
    EXPECT_TRUE(polls.poll_ids.get(0) == output1.poll_id || polls.poll_ids.get(1) == output1.poll_id);
    EXPECT_TRUE(polls.poll_ids.get(0) == output2.poll_id || polls.poll_ids.get(1) == output2.poll_id);
}

// Test GetPollsByCreator with no polls
TEST(QUtilTest, GetPollsByCreator_NoPolls) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Scenario ID 1: Create a poll and have 10 random IDs vote
TEST(QUtilTest, ScenarioID1_CreatePoll_10Voters) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    Array<uint8, 256> github_link = stringToArray("https://github.com/qubic/proposal/abc");  // Test link, does not exist

    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = poll_name;
    create_input.poll_type = QUTIL_POLL_TYPE_QUBIC;
    create_input.min_amount = min_amount;
    create_input.github_link = github_link;
    create_input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    std::vector<id> voters;
    for (int i = 0; i < 10; ++i) {
        id voter = generateRandomId();
        voters.push_back(voter);
        increaseEnergy(voter, min_amount + QUTIL_VOTE_FEE);

        QUTIL::Vote_input vote_input;
        vote_input.poll_id = poll_id;
        vote_input.address = voter;
        vote_input.amount = min_amount;
        vote_input.chosen_option = i % QUTIL_MAX_OPTIONS; // Options 0-9

        auto vote_output = qutil.vote(voter, vote_input, QUTIL_VOTE_FEE);
        EXPECT_TRUE(vote_output.success);
    }

    auto result = qutil.getCurrentResult(poll_id);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(result.result.get(i), min_amount);
        EXPECT_EQ(result.voter_count.get(i), 1);
    }

    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 1);
    EXPECT_EQ(polls.poll_ids.get(0), poll_id);
}

TEST(QUtilTest, CreatePoll_InvalidGithubLink) {
    ContractTestingQUtil qutil;
    id creator = generateRandomId();
    id poll_name = generateRandomId();
    uint64_t min_amount = 1000;
    // Invalid GitHub link (does not start with "https://github.com/qubic")
    Array<uint8, 256> invalid_github_link = stringToArray("https://github.com/invalidorg/proposal/abc");
    uint64_t poll_type = QUTIL_POLL_TYPE_QUBIC;

    QUTIL::CreatePoll_input input;
    input.poll_name = poll_name;
    input.poll_type = poll_type;
    input.min_amount = min_amount;
    input.github_link = invalid_github_link;
    input.num_assets = 0;

    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    // Attempt to create the poll with invalid GitHub link
    auto output = qutil.createPoll(creator, input, QUTIL_POLL_CREATION_FEE);
    // Expect poll_id to be 0 indicating failure
    EXPECT_EQ(output.poll_id, 0);

    // Verify that no poll was created for the creator
    auto polls = qutil.getPollsByCreator(creator);
    EXPECT_EQ(polls.count, 0);
}

// Create an asset poll and have voters with allowed assets vote
TEST(QUtilTest, CreateAssetPoll_VotersWithAssets)
{
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    id issuer = generateRandomId();
    id voterX = generateRandomId();
    id voterY = generateRandomId();
    id voterZ = generateRandomId();
    id voterW = generateRandomId();

    increaseEnergy(issuer, 2000000000 + 1000000 * 10); // For issuance and transfers
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    increaseEnergy(voterX, 10000000); // for votes and transfers
    increaseEnergy(voterY, 10000000);
    increaseEnergy(voterZ, 10000000);
    increaseEnergy(voterW, 10000000);

    // Issue assets with valid names
    unsigned long long assetNameA = assetNameFromString("ASSETA");
    unsigned long long assetNameB = assetNameFromString("ASSETB");
    Asset assetA = { issuer, assetNameA };
    Asset assetB = { issuer, assetNameB };
    qutil.issueAsset(issuer, assetNameA, 1000000);
    qutil.issueAsset(issuer, assetNameB, 1000000);

    // Transfer assets
    auto transferred_amount1 = qutil.transferAsset(issuer, voterX, assetA, 2000);
    auto transferred_amount2 = qutil.transferAsset(issuer, voterX, assetB, 1500);
    auto transferred_amount3 = qutil.transferAsset(issuer, voterY, assetA, 1000);
    auto transferred_amount4 = qutil.transferAsset(issuer, voterZ, assetB, 1200);

    // Create asset poll
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, assetA);
    allowed_assets.set(1, assetB);
    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = generateRandomId();
    create_input.poll_type = QUTIL_POLL_TYPE_ASSET;
    create_input.min_amount = 1000;
    create_input.github_link = stringToArray("https://github.com/qubic/proposal/test");
    create_input.allowed_assets = allowed_assets;
    create_input.num_assets = 2;

    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    auto current_poll_id = qutil.getCurrentPollId();

    // Voters vote
    QUTIL::Vote_input vote_inputX;
    vote_inputX.poll_id = poll_id;
    vote_inputX.address = voterX;
    vote_inputX.amount = 2000; // Max of A and B
    vote_inputX.chosen_option = 0;
    auto vote_outputX = qutil.vote(voterX, vote_inputX, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_outputX.success);

    QUTIL::Vote_input vote_inputY;
    vote_inputY.poll_id = poll_id;
    vote_inputY.address = voterY;
    vote_inputY.amount = 1000; // Holding of A
    vote_inputY.chosen_option = 1;
    auto vote_outputY = qutil.vote(voterY, vote_inputY, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_outputY.success);

    QUTIL::Vote_input vote_inputZ;
    vote_inputZ.poll_id = poll_id;
    vote_inputZ.address = voterZ;
    vote_inputZ.amount = 1200; // Holding of B
    vote_inputZ.chosen_option = 0;
    auto vote_outputZ = qutil.vote(voterZ, vote_inputZ, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_outputZ.success);

    QUTIL::Vote_input vote_inputW;
    vote_inputW.poll_id = poll_id;
    vote_inputW.address = voterW;
    vote_inputW.amount = 1000; // No assets
    vote_inputW.chosen_option = 0;
    auto vote_outputW = qutil.vote(voterW, vote_inputW, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_outputW.success);

    // Check results
    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), 2000 + 1200); // VoterX + VoterZ
    EXPECT_EQ(result.result.get(1), 1000); // VoterY
    EXPECT_EQ(result.voter_count.get(0), 2);
    EXPECT_EQ(result.voter_count.get(1), 1);
}

// Voters with no allowed assets cannot vote
TEST(QUtilTest, VotersNoAllowedAssets)
{
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    id issuer = generateRandomId();
    id voterW = generateRandomId();

    increaseEnergy(issuer, 2000000000);
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    increaseEnergy(voterW, QUTIL_VOTE_FEE);

    unsigned long long assetNameA = assetNameFromString("ASSETA");
    Asset assetA = { issuer, assetNameA };
    qutil.issueAsset(issuer, assetNameA, 1000000);

    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, assetA);
    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = generateRandomId();
    create_input.poll_type = QUTIL_POLL_TYPE_ASSET;
    create_input.min_amount = 1000;
    create_input.github_link = stringToArray("https://github.com/qubic/proposal/test");
    create_input.allowed_assets = allowed_assets;
    create_input.num_assets = 1;

    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    QUTIL::Vote_input vote_inputW;
    vote_inputW.poll_id = poll_id;
    vote_inputW.address = voterW;
    vote_inputW.amount = 1000;
    vote_inputW.chosen_option = 0;
    auto vote_outputW = qutil.vote(voterW, vote_inputW, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_outputW.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), 0);
    EXPECT_EQ(result.voter_count.get(0), 0);
}

// Voters with one allowed asset can vote
TEST(QUtilTest, VotersWithOneAllowedAsset)
{
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    id issuer = generateRandomId();
    id voterY = generateRandomId();

    increaseEnergy(issuer, 2000000000 + 1000000);
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    increaseEnergy(voterY, QUTIL_VOTE_FEE);

    unsigned long long assetNameA = assetNameFromString("ASSETA");
    unsigned long long assetNameB = assetNameFromString("ASSETB");
    Asset assetA = { issuer, assetNameA };
    Asset assetB = { issuer, assetNameB };
    qutil.issueAsset(issuer, assetNameA, 1000000);
    qutil.issueAsset(issuer, assetNameB, 1000000);
    qutil.transferAsset(issuer, voterY, assetA, 1000);

    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, assetA);
    allowed_assets.set(1, assetB);
    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = generateRandomId();
    create_input.poll_type = QUTIL_POLL_TYPE_ASSET;
    create_input.min_amount = 1000;
    create_input.github_link = stringToArray("https://github.com/qubic/proposal/test");
    create_input.allowed_assets = allowed_assets;
    create_input.num_assets = 2;

    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    QUTIL::Vote_input vote_inputY;
    vote_inputY.poll_id = poll_id;
    vote_inputY.address = voterY;
    vote_inputY.amount = 1000;
    vote_inputY.chosen_option = 1;
    auto vote_outputY = qutil.vote(voterY, vote_inputY, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_outputY.success);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(1), 1000);
    EXPECT_EQ(result.voter_count.get(1), 1);
}

// Decrease voter shares and update voting power
TEST(QUtilTest, DecreaseShares_UpdateVotingPower)
{
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    id issuer = generateRandomId();
    id voterX = generateRandomId();
    id voterY = generateRandomId();

    increaseEnergy(issuer, 2000000000 + 1000000 * 10);
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    increaseEnergy(voterX, 10000000);
    increaseEnergy(voterY, 10000000);

    unsigned long long assetNameA = assetNameFromString("ASSETA");
    Asset assetA = { issuer, assetNameA };
    qutil.issueAsset(issuer, assetNameA, 1000000);
    qutil.transferAsset(issuer, voterX, assetA, 2000);
    qutil.transferAsset(issuer, voterY, assetA, 1000);

    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, assetA);
    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = generateRandomId();
    create_input.poll_type = QUTIL_POLL_TYPE_ASSET;
    create_input.min_amount = 1000;
    create_input.github_link = stringToArray("https://github.com/qubic/proposal/test");
    create_input.allowed_assets = allowed_assets;
    create_input.num_assets = 1;

    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    // Initial votes
    QUTIL::Vote_input vote_inputX;
    vote_inputX.poll_id = poll_id;
    vote_inputX.address = voterX;
    vote_inputX.amount = 2000;
    vote_inputX.chosen_option = 0;
    qutil.vote(voterX, vote_inputX, QUTIL_VOTE_FEE);

    QUTIL::Vote_input vote_inputY;
    vote_inputY.poll_id = poll_id;
    vote_inputY.address = voterY;
    vote_inputY.amount = 1000;
    vote_inputY.chosen_option = 1;
    qutil.vote(voterY, vote_inputY, QUTIL_VOTE_FEE);

    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), 2000);
    EXPECT_EQ(result.result.get(1), 1000);
    EXPECT_EQ(result.voter_count.get(0), 1);
    EXPECT_EQ(result.voter_count.get(1), 1);

    // Decrease voterX's shares below min_amount
    qutil.transferAsset(voterX, issuer, assetA, 1900); // Leaves 100

    // VoterY votes again to trigger update
    qutil.vote(voterY, vote_inputY, QUTIL_VOTE_FEE);

    // Check updated results
    result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), 0); // VoterX removed
    EXPECT_EQ(result.result.get(1), 1000); // VoterY remains
    EXPECT_EQ(result.voter_count.get(0), 0);
    EXPECT_EQ(result.voter_count.get(1), 1);

    // VoterX tries to vote again with insufficient shares
    auto vote_outputX = qutil.vote(voterX, vote_inputX, QUTIL_VOTE_FEE);
    EXPECT_FALSE(vote_outputX.success);
}

TEST(QUtilTest, MultipleVoters_ShareTransfers_EligibilityTest)
{
    ContractTestingQUtil qutil;

    id creator = generateRandomId();
    id issuer = generateRandomId();
    std::vector<id> voters(10);
    for (auto& v : voters) 
    {
        v = generateRandomId();
    }

    increaseEnergy(issuer, 4000000000); // for issuance and transfers
    increaseEnergy(creator, QUTIL_POLL_CREATION_FEE);
    for (const auto& v : voters)
    {
        increaseEnergy(v, 10000000); // for votes and transfers
    }

    // Issue 3 asset
    unsigned long long assetNameA = assetNameFromString("ASSETA");
    unsigned long long assetNameB = assetNameFromString("ASSETB");
    unsigned long long assetNameC = assetNameFromString("ASSETC");
    Asset assetA = { issuer, assetNameA };
    Asset assetB = { issuer, assetNameB };
    Asset assetC = { issuer, assetNameC };
    qutil.issueAsset(issuer, assetNameA, 1000000);
    qutil.issueAsset(issuer, assetNameB, 1000000);
    qutil.issueAsset(issuer, assetNameC, 1000000);

    // Distribute assets
    // Voters 0-2: 2000 A, 500 B, 500 C
    for (int i = 0; i < 3; i++)
    {
        qutil.transferAsset(issuer, voters[i], assetA, 2000);
        qutil.transferAsset(issuer, voters[i], assetB, 500);
        qutil.transferAsset(issuer, voters[i], assetC, 500);
    }
    // Voters 3-5: 500 A, 2000 B, 500 C
    for (int i = 3; i < 6; i++)
    {
        qutil.transferAsset(issuer, voters[i], assetA, 500);
        qutil.transferAsset(issuer, voters[i], assetB, 2000);
        qutil.transferAsset(issuer, voters[i], assetC, 500);
    }
    // Voters 6-9: 500 A, 500 B, 2000 C
    for (int i = 6; i < 10; i++)
    {
        qutil.transferAsset(issuer, voters[i], assetA, 500);
        qutil.transferAsset(issuer, voters[i], assetB, 500);
        qutil.transferAsset(issuer, voters[i], assetC, 2000);
    }

    // Create asset poll
    Array<Asset, QUTIL_MAX_ASSETS_PER_POLL> allowed_assets;
    allowed_assets.set(0, assetA);
    allowed_assets.set(1, assetB);
    allowed_assets.set(2, assetC);
    QUTIL::CreatePoll_input create_input;
    create_input.poll_name = generateRandomId();
    create_input.poll_type = QUTIL_POLL_TYPE_ASSET;
    create_input.min_amount = 1000;
    create_input.github_link = stringToArray("https://github.com/qubic/proposal/test");
    create_input.allowed_assets = allowed_assets;
    create_input.num_assets = 3;
    auto create_output = qutil.createPoll(creator, create_input, QUTIL_POLL_CREATION_FEE);
    uint64_t poll_id = create_output.poll_id;

    for (int i = 0; i < 10; i++)
    {
        uint64_t option = (i < 3) ? 0 : (i < 6 ? 1 : 2);
        QUTIL::Vote_input vote_input;
        vote_input.poll_id = poll_id;
        vote_input.address = voters[i];
        vote_input.amount = 2000; // Max holding
        vote_input.chosen_option = option;
        auto vote_output = qutil.vote(voters[i], vote_input, QUTIL_VOTE_FEE);
        EXPECT_TRUE(vote_output.success);
    }

    // Check initial results
    auto result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.is_active, 1);
    EXPECT_EQ(result.result.get(0), 3 * 2000); // Voters 0-2
    EXPECT_EQ(result.result.get(1), 3 * 2000); // Voters 3-5
    EXPECT_EQ(result.result.get(2), 4 * 2000); // Voters 6-9
    EXPECT_EQ(result.voter_count.get(0), 3);
    EXPECT_EQ(result.voter_count.get(1), 3);
    EXPECT_EQ(result.voter_count.get(2), 4);

    // Make 3 voters ineligible
    // Voter 0: All assets to 999
    qutil.transferAsset(voters[0], issuer, assetA, 1001); // A: 2000 - 1001 = 999
    qutil.transferAsset(issuer, voters[0], assetB, 499); // B: 500 + 499 = 999
    qutil.transferAsset(issuer, voters[0], assetC, 499); // C: 500 + 499 = 999
    // Voter 3: Reduce B to 999
    qutil.transferAsset(voters[3], issuer, assetB, 1001); // B: 2000 - 1001 = 999
    // Voter 6: Reduce C to 999
    qutil.transferAsset(voters[6], issuer, assetC, 1001); // C: 2000 - 1001 = 999

    // Trigger voter list to update
    QUTIL::Vote_input vote_input1;
    vote_input1.poll_id = poll_id;
    vote_input1.address = voters[1];
    vote_input1.amount = 2000;
    vote_input1.chosen_option = 0;
    auto vote_output1 = qutil.vote(voters[1], vote_input1, QUTIL_VOTE_FEE);
    EXPECT_TRUE(vote_output1.success);

    // Check updated results
    result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.result.get(0), 2 * 2000); // Voters 1,2
    EXPECT_EQ(result.result.get(1), 2 * 2000); // Voters 4,5
    EXPECT_EQ(result.result.get(2), 3 * 2000); // Voters 7-9
    EXPECT_EQ(result.voter_count.get(0), 2);
    EXPECT_EQ(result.voter_count.get(1), 2);
    EXPECT_EQ(result.voter_count.get(2), 3);

    // Verify ineligible voters cannot vote
    for (int i : {0, 3, 6})
    {
        QUTIL::Vote_input vote_input;
        vote_input.poll_id = poll_id;
        vote_input.address = voters[i];
        vote_input.amount = 2000;
        vote_input.chosen_option = (i < 3) ? 0 : (i < 6 ? 1 : 2);
        auto vote_output = qutil.vote(voters[i], vote_input, QUTIL_VOTE_FEE);
        EXPECT_FALSE(vote_output.success);
    }

    qutil.endEpoch();
    qutil.beginEpoch();

    // Check updated results
    result = qutil.getCurrentResult(poll_id);
    EXPECT_EQ(result.is_active, 0);
    EXPECT_EQ(result.result.get(0), 2 * 2000); // Voters 1,2
    EXPECT_EQ(result.result.get(1), 2 * 2000); // Voters 4,5
    EXPECT_EQ(result.result.get(2), 3 * 2000); // Voters 7-9
    EXPECT_EQ(result.voter_count.get(0), 2);
    EXPECT_EQ(result.voter_count.get(1), 2);
    EXPECT_EQ(result.voter_count.get(2), 3);
}

TEST(QUtilTest, DistributeQuToShareholders)
{
    ContractTestingQUtil qutil;
 
    id distributor = generateRandomId();
    id issuer = generateRandomId();
    std::vector<id> shareholder(10);
    for (auto& v : shareholder)
    {
        v = generateRandomId();
    }

    increaseEnergy(issuer, 4000000000); // for issuance and transfers
    increaseEnergy(distributor, 10000000000);

    // Issue 3 asset
    unsigned long long assetNameA = assetNameFromString("ASSETA");
    unsigned long long assetNameB = assetNameFromString("ASSETB");
    unsigned long long assetNameC = assetNameFromString("ASSETC");
    Asset assetA = { issuer, assetNameA };
    Asset assetB = { issuer, assetNameB };
    Asset assetC = { issuer, assetNameC };
    qutil.issueAsset(issuer, assetNameA, 10);
    qutil.issueAsset(issuer, assetNameB, 10000);
    qutil.issueAsset(issuer, assetNameC, 10000000);

    // Distribute assets
    // shareholder 0-2: 1 A, 500 B, 500 C
    for (int i = 0; i < 3; i++)
    {
        qutil.transferAsset(issuer, shareholder[i], assetA, 1);
        qutil.transferAsset(issuer, shareholder[i], assetB, 500);
        qutil.transferAsset(issuer, shareholder[i], assetC, 600);
    }
    // shareholder 3-5: 0 A, 2000 B, 500 C
    for (int i = 3; i < 6; i++)
    {
        qutil.transferAsset(issuer, shareholder[i], assetB, 2000);
        qutil.transferAsset(issuer, shareholder[i], assetC, 500);
    }
    // shareholder 6-9: 1 A, 500 B, 0 C
    for (int i = 6; i < 10; i++)
    {
        qutil.transferAsset(issuer, shareholder[i], assetA, 1);
        qutil.transferAsset(issuer, shareholder[i], assetB, 500);
    }

    QUTIL::DistributeQuToShareholders_output output;
    sint64 distributorBalanceBefore, shareholderBalanceBefore;

    // Error case 1: asset without shareholders
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, { distributor, assetNameA }, 10000000);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore);
    EXPECT_EQ(output.shareholders, 0);
    EXPECT_EQ(output.totalShares, 0);
    EXPECT_EQ(output.amountPerShare, 0);
    EXPECT_EQ(output.fees, 0);

    // Error case 2: amount too low to pay fee
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetA, 1);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore);
    EXPECT_EQ(output.shareholders, 8);
    EXPECT_EQ(output.totalShares, 10);
    EXPECT_LE(output.amountPerShare, 0);
    EXPECT_EQ(output.fees, 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Error case 3: amount too low to pay 1 QU per share
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetA, 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER + 9);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore);
    EXPECT_EQ(output.shareholders, 8);
    EXPECT_EQ(output.totalShares, 10);
    EXPECT_EQ(output.amountPerShare, 0);
    EXPECT_EQ(output.fees, 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetA + exactly calculated amount
    sint64 amountPerShare = 50;
    sint64 totalAmount = 10 * amountPerShare + 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetA, totalAmount);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + amountPerShare);
    EXPECT_EQ(output.shareholders, 8);
    EXPECT_EQ(output.totalShares, 10);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetA + amount with some QUs that cannot be evenly distributed and are refundet
    amountPerShare = 100;
    totalAmount = 10 * amountPerShare + 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetA, totalAmount + 7);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + amountPerShare);
    EXPECT_EQ(output.shareholders, 8);
    EXPECT_EQ(output.totalShares, 10);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 8 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetB + exactly calculated amount
    amountPerShare = 1000;
    totalAmount = 10000 * amountPerShare + 11 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetB, totalAmount);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + 500 * amountPerShare);
    EXPECT_EQ(output.shareholders, 11);
    EXPECT_EQ(output.totalShares, 10000);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 11 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetB + amount with some QUs that cannot be evenly distributed and are refundet
    amountPerShare = 42;
    totalAmount = 10000 * amountPerShare + 11 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetB, totalAmount + 9999);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + 500 * amountPerShare);
    EXPECT_EQ(output.shareholders, 11);
    EXPECT_EQ(output.totalShares, 10000);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 11 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetC + exactly calculated amount (fee is minimal)
    amountPerShare = 123;
    totalAmount = 10000000 * amountPerShare + 7 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetC, totalAmount);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + 600 * amountPerShare);
    EXPECT_EQ(output.shareholders, 7);
    EXPECT_EQ(output.totalShares, 10000000);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 7 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);

    // Success case with assetC + non-minimal fee (fee payed too much is donation for running QUTIL -> burned with fee)
    amountPerShare = 654;
    totalAmount = 10000000 * amountPerShare + 7 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER;
    distributorBalanceBefore = getBalance(distributor);
    shareholderBalanceBefore = getBalance(shareholder[0]);
    output = qutil.distributeQuToShareholders(distributor, assetC, totalAmount + 123456);
    EXPECT_EQ(getBalance(distributor), distributorBalanceBefore - totalAmount);
    EXPECT_EQ(getBalance(shareholder[0]), shareholderBalanceBefore + 600 * amountPerShare);
    EXPECT_EQ(output.shareholders, 7);
    EXPECT_EQ(output.totalShares, 10000000);
    EXPECT_EQ(output.amountPerShare, amountPerShare);
    EXPECT_EQ(output.fees, 7 * QUTIL_DISTRIBUTE_QU_TO_SHAREHOLDER_FEE_PER_SHAREHOLDER);
}
