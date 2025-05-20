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
    }

    void beginEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, BEGIN_EPOCH, expectSuccess);
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(MSVAULT_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }

    QUTIL::CreatePoll_output createPoll(const id& creator, const QUTIL::CreatePoll_input& input, uint64_t fee) {
        QUTIL::CreatePoll_output output;
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, 4, input, output, creator, fee);
        return output;
    }

    QUTIL::Vote_output vote(const id& voter, const QUTIL::Vote_input& input, uint64_t fee) {
        QUTIL::Vote_output output;
        invokeUserProcedure(QUTIL_CONTRACT_INDEX, 5, input, output, voter, fee);
        return output;
    }

    QUTIL::GetCurrentResult_output getCurrentResult(uint64_t poll_id) {
        QUTIL::GetCurrentResult_input input;
        input.poll_id = poll_id;
        QUTIL::GetCurrentResult_output output;
        callFunction(QUTIL_CONTRACT_INDEX, 2, input, output);
        return output;
    }

    QUTIL::GetPollsByCreator_output getPollsByCreator(const id& creator) {
        QUTIL::GetPollsByCreator_input input;
        input.creator = creator;
        QUTIL::GetPollsByCreator_output output;
        callFunction(QUTIL_CONTRACT_INDEX, 3, input, output);
        return output;
    }
};

// Helper function to generate random ID
id generateRandomId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(_A, _Z);

    return ID(
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen),
        dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen), dis(gen)
    );
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
