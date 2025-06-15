#define NO_UEFI

#include "contract_testing.h"

#include <random>

class ContractTestingQX : public ContractTesting {
public:
    ContractTestingQX() {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
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
};

class ContractTestingQUtil : public ContractTesting {
public:
    ContractTestingQUtil() {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(QUTIL);
        callSystemProcedure(QUTIL_CONTRACT_INDEX, INITIALIZE);
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUTIL_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(QUTIL_CONTRACT_INDEX, END_EPOCH, expectSuccess);
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
    uint64_t num_polls = 30;
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
    uint64_t num_polls = 70;
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
    EXPECT_EQ(current_poll_info.active_count, QUTIL_MAX_POLL);

    std::set<uint64_t> expected_active_ids;
    for (uint64_t i = num_polls - QUTIL_MAX_POLL; i < num_polls; ++i)
    {
        expected_active_ids.insert(i);
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
    uint64_t num_polls = 70;
    std::map<uint64_t, id> poll_id_to_name;

    for (uint64_t i = 0; i < num_polls; ++i)
    {
        id poll_name = generateRandomId();
        poll_id_to_name[i] = poll_name;
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
        EXPECT_EQ(output.poll_id, i);
    }

    // Check polls 0 to 5 (should be overwritten)
    for (uint64_t i = 0; i < 6; ++i)
    {
        auto poll_info = qutil.getPollInfo(i);
        EXPECT_EQ(poll_info.found, 0);
    }

    // Check polls 6 to 69
    for (uint64_t i = 6; i < num_polls; ++i)
    {
        auto poll_info = qutil.getPollInfo(i);
        EXPECT_EQ(poll_info.found, 1);
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
