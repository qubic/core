#define NO_UEFI

#include <thread>
#include <chrono>

#include "contract_testing.h"

static const id TESTEXA_CONTRACT_ID(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
static const id TESTEXB_CONTRACT_ID(TESTEXB_CONTRACT_INDEX, 0, 0, 0);
static const id TESTEXC_CONTRACT_ID(TESTEXC_CONTRACT_INDEX, 0, 0, 0);
static const id USER1(123, 456, 789, 876);
static const id USER2(42, 424, 4242, 42424);
static const id USER3(98, 76, 54, 3210);
static const id USER4(9878, 7645, 541, 3210);

void checkPreManagementRightsTransferInput(const PreManagementRightsTransfer_input& observed, const PreManagementRightsTransfer_input& expected)
{
    EXPECT_EQ(observed.asset.assetName, expected.asset.assetName);
    EXPECT_EQ(observed.asset.issuer, expected.asset.issuer);
    EXPECT_EQ(observed.numberOfShares, expected.numberOfShares);
    EXPECT_EQ(observed.offeredFee, expected.offeredFee);
    EXPECT_EQ((int)observed.otherContractIndex, (int)expected.otherContractIndex);
    EXPECT_EQ(observed.owner, expected.owner);
    EXPECT_EQ(observed.possessor, expected.possessor);
}

void checkPostManagementRightsTransferInput(const PostManagementRightsTransfer_input& observed, const PostManagementRightsTransfer_input& expected)
{
    EXPECT_EQ(observed.asset.assetName, expected.asset.assetName);
    EXPECT_EQ(observed.asset.issuer, expected.asset.issuer);
    EXPECT_EQ(observed.numberOfShares, expected.numberOfShares);
    EXPECT_EQ(observed.receivedFee, expected.receivedFee);
    EXPECT_EQ((int)observed.otherContractIndex, (int)expected.otherContractIndex);
    EXPECT_EQ(observed.owner, expected.owner);
    EXPECT_EQ(observed.possessor, expected.possessor);
}

class StateCheckerTestExampleA : public TESTEXA
{
public:
    void checkPostReleaseCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postReleaseSharesCounter, expectedCount);
    }
    
    const PreManagementRightsTransfer_input& getPreReleaseInput() const
    {
        return this->prevPreReleaseSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostReleaseInput() const
    {
        return this->prevPostReleaseSharesInput;
    }

    void checkPostAcquireCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postAcquireShareCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreAcquireInput() const
    {
        return this->prevPreAcquireSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostAcquireInput() const
    {
        return this->prevPostAcquireSharesInput;
    }

    void checkVariablesSetByProposal(
        uint64 expectedVariable1,
        uint32 expectedVariable2,
        sint8 expectedVariable3) const
    {
        EXPECT_EQ(this->dummyStateVariable1, expectedVariable1);
        EXPECT_EQ(this->dummyStateVariable2, expectedVariable2);
        EXPECT_EQ(this->dummyStateVariable3, expectedVariable3);
    }
};

class StateCheckerTestExampleB : public TESTEXB
{
public:
    void checkPostReleaseCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postReleaseSharesCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreReleaseInput() const
    {
        return this->prevPreReleaseSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostReleaseInput() const
    {
        return this->prevPostReleaseSharesInput;
    }

    void checkPostAcquireCounter(uint32 expectedCount)
    {
        EXPECT_EQ(this->postAcquireShareCounter, expectedCount);
    }

    const PreManagementRightsTransfer_input& getPreAcquireInput() const
    {
        return this->prevPreAcquireSharesInput;
    }

    const PostManagementRightsTransfer_input& getPostAcquireInput() const
    {
        return this->prevPostAcquireSharesInput;
    }

    void checkVariablesSetByProposal(
        sint64 expectedVariable1,
        sint64 expectedVariable2,
        sint64 expectedVariable3) const
    {
        EXPECT_EQ(this->fee1, expectedVariable1);
        EXPECT_EQ(this->fee2, expectedVariable2);
        EXPECT_EQ(this->fee3, expectedVariable3);
    }
};

class ContractTestingTestEx : protected ContractTesting
{
public:
    QX::Fees_output qxFees;

    ContractTestingTestEx()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        INIT_CONTRACT(TESTEXA);
        callSystemProcedure(TESTEXA_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(TESTEXB);
        callSystemProcedure(TESTEXB_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(TESTEXC);
        callSystemProcedure(TESTEXC_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(TESTEXD);
        callSystemProcedure(TESTEXD_CONTRACT_INDEX, INITIALIZE);
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);

        checkContractExecCleanup();

        // query QX fees
        callFunction(QX_CONTRACT_INDEX, 1, QX::Fees_input(), qxFees);
    }

    ~ContractTestingTestEx()
    {
        checkContractExecCleanup();
    }

    StateCheckerTestExampleA* getStateTestExampleA()
    {
        return (StateCheckerTestExampleA*)contractStates[TESTEXA_CONTRACT_INDEX];
    }

    StateCheckerTestExampleB* getStateTestExampleB()
    {
        return (StateCheckerTestExampleB*)contractStates[TESTEXB_CONTRACT_INDEX];
    }

    sint64 issueAssetQx(const Asset& asset, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        QX::IssueAsset_input input{ asset.assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        QX::IssueAsset_output output;
        invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, asset.issuer, qxFees.assetIssuanceFee);
        return output.issuedNumberOfShares;
    }

    sint64 issueAssetTestExA(const Asset& asset, sint64 numberOfShares, uint64 unitOfMeasurement, sint8 numberOfDecimalPlaces)
    {
        TESTEXA::IssueAsset_input input{ asset.assetName, numberOfShares, unitOfMeasurement, numberOfDecimalPlaces };
        TESTEXA::IssueAsset_output output;
        invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 1, input, output, asset.issuer, 0);
        return output.issuedNumberOfShares;
    }

    sint64 transferShareOwnershipAndPossessionQx(const Asset& asset, const id& currentOwnerAndPossessor, const id& newOwnerAndPossessor, sint64 numberOfShares)
    {
        QX::TransferShareOwnershipAndPossession_input input;
        QX::TransferShareOwnershipAndPossession_output output;
        
        input.assetName = asset.assetName;
        input.issuer = asset.issuer;
        input.newOwnerAndPossessor = newOwnerAndPossessor;
        input.numberOfShares = numberOfShares;
        
        invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, currentOwnerAndPossessor, qxFees.transferFee);
        
        return output.transferredNumberOfShares;
    }

    template <typename StateStruct>
    sint64 transferShareOwnershipAndPossession(const Asset& asset, const id& currentOwnerAndPossessor, const id& newOwnerAndPossessor, sint64 numberOfShares)
    {
        typename StateStruct::TransferShareOwnershipAndPossession_input input;
        typename StateStruct::TransferShareOwnershipAndPossession_output output;

        input.asset = asset;
        input.newOwnerAndPossessor = newOwnerAndPossessor;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 2, input, output, currentOwnerAndPossessor, 0);

        return output.transferredNumberOfShares;
    }

    sint64 transferShareManagementRightsQx(const Asset& asset, const id& currentOwnerAndPossessor, sint64 numberOfShares, unsigned int newManagingContractIndex, sint64 fee = 0)
    {
        QX::TransferShareManagementRights_input input;
        QX::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, currentOwnerAndPossessor, fee);

        return output.transferredNumberOfShares;
    }

    template <typename StateStruct>
    sint64 transferShareManagementRights(const Asset& asset, const id& currentOwnerAndPossessor, sint64 numberOfShares, unsigned int newManagingContractIndex, sint64 fee = 0)
    {
        typename StateStruct::TransferShareManagementRights_input input;
        typename StateStruct::TransferShareManagementRights_output output;

        input.asset = asset;
        input.newManagingContractIndex = newManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 3, input, output, currentOwnerAndPossessor, fee);

        return output.transferredNumberOfShares;
    }

    template <typename StateStruct>
    void setPreReleaseSharesOutput(bool allowTransfer, sint64 requestedFee)
    {
        typename StateStruct::SetPreReleaseSharesOutput_input input{ allowTransfer, requestedFee };
        typename StateStruct::SetPreReleaseSharesOutput_output output;
        invokeUserProcedure(StateStruct::__contract_index, 4, input, output, USER1, 0);
    }

    template <typename StateStruct>
    void setPreAcquireSharesOutput(bool allowTransfer, sint64 requestedFee)
    {
        typename StateStruct::SetPreReleaseSharesOutput_input input{ allowTransfer, requestedFee };
        typename StateStruct::SetPreReleaseSharesOutput_output output;
        invokeUserProcedure(StateStruct::__contract_index, 5, input, output, USER1, 0);
    }

    
    template <typename StateStruct>
    sint64 acquireShareManagementRights(const Asset& asset, const id& currentOwnerAndPossessor, sint64 numberOfShares, unsigned int prevManagingContractIndex, sint64 fee = 0, const id& originator = NULL_ID)
    {
        typename StateStruct::AcquireShareManagementRights_input input;
        typename StateStruct::AcquireShareManagementRights_output output;

        input.asset = asset;
        input.ownerAndPossessor = currentOwnerAndPossessor;
        input.oldManagingContractIndex = prevManagingContractIndex;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(StateStruct::__contract_index, 6, input, output,
            (isZero(originator)) ? currentOwnerAndPossessor : originator, fee);

        return output.transferredNumberOfShares;
    }

    sint64 getTestExAsShareManagementRightsByInvokingTestExB(const Asset& asset, const id& currentOwnerAndPossessor, sint64 numberOfShares, sint64 fee = 0)
    {
        TESTEXB::GetTestExampleAShareManagementRights_input input;
        TESTEXB::GetTestExampleAShareManagementRights_output output;

        input.asset = asset;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(TESTEXB::__contract_index, 7, input, output, currentOwnerAndPossessor, fee);

        return output.transferredNumberOfShares;
    }

    sint64 getTestExAsShareManagementRightsByInvokingTestExC(const Asset& asset, const id& currentOwnerAndPossessor, sint64 numberOfShares, sint64 fee = 0)
    {
        TESTEXC::GetTestExampleAShareManagementRights_input input;
        TESTEXC::GetTestExampleAShareManagementRights_output output;

        input.asset = asset;
        input.numberOfShares = numberOfShares;

        invokeUserProcedure(TESTEXC::__contract_index, 7, input, output, currentOwnerAndPossessor, fee);

        return output.transferredNumberOfShares;
    }

    TESTEXA::QueryQpiFunctions_output queryQpiFunctions(const TESTEXA::QueryQpiFunctions_input& input)
    {
        TESTEXA::QueryQpiFunctions_output output;
        callFunction(TESTEXA_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    unsigned int callFunctionOfTestExampleAFromTextExampleB(const TESTEXA::QueryQpiFunctions_input& input, TESTEXA::QueryQpiFunctions_output& output, bool expectSuccess)
    {
        return callFunction(TESTEXB_CONTRACT_INDEX, 1, input, output, true, expectSuccess);
    }

    unsigned int callErrorTriggerFunction()
    {
        TESTEXA::ErrorTriggerFunction_input input;
        TESTEXA::ErrorTriggerFunction_output output;
        return callFunction(TESTEXA_CONTRACT_INDEX, 5, input, output, true, false);
    }

    template <typename StateStruct>
    typename StateStruct::IncomingTransferAmounts_output getIncomingTransferAmounts()
    {
        typename StateStruct::IncomingTransferAmounts_input input;
        typename StateStruct::IncomingTransferAmounts_output output;
        EXPECT_EQ(callFunction(StateStruct::__contract_index, 20, input, output), NoContractError);
        return output;
    }

    template <typename StateStruct>
    bool qpiTransfer(const id& destinationPublicKey, sint64 amount, sint64 fee = 0, const id& originator = USER1)
    {
        typename StateStruct::QpiTransfer_input input{ destinationPublicKey, amount };
        typename StateStruct::QpiTransfer_output output;
        return invokeUserProcedure(StateStruct::__contract_index, 20, input, output, originator, fee);
    }

    template <typename StateStruct>
    bool qpiDistributeDividends(sint64 amountPerShare, sint64 fee = 0, const id& originator = USER1)
    {
        typename StateStruct::QpiDistributeDividends_input input{ amountPerShare };
        typename StateStruct::QpiDistributeDividends_output output;
        return invokeUserProcedure(StateStruct::__contract_index, 21, input, output, originator, fee);
    }

    template <typename StateStruct>
    typename StateStruct::GetIpoBid_output getIpoBid(unsigned int ipoContractIndex, unsigned int bidIndex)
    {
        typename StateStruct::GetIpoBid_input input{ ipoContractIndex, bidIndex };
        typename StateStruct::GetIpoBid_output output;
        EXPECT_EQ(callFunction(StateStruct::__contract_index, 30, input, output), NoContractError);
        return output;
    }

    template <typename StateStruct>
    long long qpiBidInIpo(unsigned int ipoContractIndex, long long pricePerShare, unsigned short numberOfShares, sint64 fee = 0, const id& originator = USER1)
    {
        typename StateStruct::QpiBidInIpo_input input{ ipoContractIndex, pricePerShare, numberOfShares };
        typename StateStruct::QpiBidInIpo_output output;
        if (invokeUserProcedure(StateStruct::__contract_index, 30, input, output, originator, fee))
            return output;
        else
            return -2;
    }

    template <typename StateStruct>
    uint16 setShareholderProposal(const id& originator, const typename StateStruct::SetShareholderProposal_input& input)
    {
        typename StateStruct::SetShareholderProposal_output output;
        EXPECT_TRUE(invokeUserProcedure(StateStruct::__contract_index, 65534, input, output, originator, 0));
        return output;
    }

    template <typename StateStruct>
    bool setShareholderVotes(const id& originator, uint16 proposalIndex, const typename StateStruct::ProposalDataT& proposalData, sint64 voteValue)
    {
        // Contract procedure expects ProposalMultiVoteDataV1, but ProposalSingleVoteDataV1 is compatible
        ProposalSingleVoteDataV1 input{ proposalIndex, proposalData.type, proposalData.tick, voteValue };
        typename StateStruct::SetShareholderVotes_output output;
        invokeUserProcedure(StateStruct::__contract_index, 65535, input, output, originator, 0, false);
        return output;
    }

    template <typename StateStruct>
    bool setShareholderVotes(const id& originator, uint16 proposalIndex, const typename StateStruct::ProposalDataT& proposalData,
        const std::vector<std::pair<sint64, uint32>>& voteValueCountPairs)
    {
        ASSERT(voteValueCountPairs.size() <= 8);
        ProposalMultiVoteDataV1 input{ proposalIndex, proposalData.type, proposalData.tick };
        input.voteValues.set(0, NO_VOTE_VALUE); // default with no voteValueCountPairs (vote count 0): set all to no votes
        for (size_t i = 0; i < voteValueCountPairs.size(); ++i)
        {
            input.voteValues.set(i, voteValueCountPairs[i].first);
            input.voteCounts.set(i, voteValueCountPairs[i].second);
        }
        typename StateStruct::SetShareholderVotes_output output;
        invokeUserProcedure(StateStruct::__contract_index, 65535, input, output, originator, 0);
        return output;
    }

    TESTEXB::TestInterContractCallError_output testInterContractCallError()
    {
        TESTEXB::TestInterContractCallError_input input;
        input.dummy = 0;
        TESTEXB::TestInterContractCallError_output output;
        invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 50, input, output, USER1, 0);
        return output;
    }

    template <typename StateStruct>
    std::vector<uint16> getShareholderProposalIndices(bit activeProposals)
    {
        typename StateStruct::GetShareholderProposalIndices_input input{ activeProposals, -1 };
        typename StateStruct::GetShareholderProposalIndices_output output;
        std::vector<uint16> indices;
        do
        {
            callFunction(StateStruct::__contract_index, 65532, input, output);
            for (uint16 i = 0; i < output.numOfIndices; ++i)
                indices.push_back(output.indices.get(i));
        } while (output.numOfIndices == output.indices.capacity());
        return indices;
    }

    template <typename StateStruct>
    StateStruct::GetShareholderProposal_output getShareholderProposal(uint16 proposalIndex)
    {
        typename StateStruct::GetShareholderProposal_input input{ proposalIndex };
        typename StateStruct::GetShareholderProposal_output output;
        callFunction(StateStruct::__contract_index, 65533, input, output);
        return output;
    }

    template <typename StateStruct>
    ProposalMultiVoteDataV1 getShareholderVotes(uint16 proposalIndex, const	id& voter)
    {
        typename StateStruct::GetShareholderVotes_input input{ voter, proposalIndex };
        typename StateStruct::GetShareholderVotes_output output;
        callFunction(StateStruct::__contract_index, 65534, input, output);
        return output;
    }

    template <typename StateStruct>
    ProposalSummarizedVotingDataV1 getShareholderVotingResults(uint16 proposalIndex)
    {
        typename StateStruct::GetShareholderVotingResults_input input{ proposalIndex };
        typename StateStruct::GetShareholderVotingResults_output output;
        callFunction(StateStruct::__contract_index, 65535, input, output);
        return output;
    }

    uint16 setupShareholderProposalTestExA(
        const id& proposer, uint16 type,
        bool setVar1 = false, uint64 valueVar1 = 0,
        bool setVar2 = false, uint32 valueVar2 = 0,
        bool setVar3 = false, sint8 valueVar3 = 0,
        bool expectSuccess = true)
    {
        TESTEXA::SetShareholderProposal_input input;
        setMemory(input, 0);
        input.proposalData.epoch = system.epoch;
        input.proposalData.type = type;
        switch (ProposalTypes::cls(type))
        {
        case ProposalTypes::Class::Variable:
        {
            if (setVar1)
            {
                EXPECT_FALSE(setVar2);
                EXPECT_FALSE(setVar3);
                input.proposalData.variableOptions.variable = 0;
                input.proposalData.variableOptions.value = valueVar1;
            }
            else if (setVar2)
            {
                EXPECT_FALSE(setVar1);
                EXPECT_FALSE(setVar3);
                input.proposalData.variableOptions.variable = 1;
                input.proposalData.variableOptions.value = valueVar2;
            }
            else if (setVar3)
            {
                EXPECT_FALSE(setVar1);
                EXPECT_FALSE(setVar2);
                input.proposalData.variableOptions.variable = 2;
                input.proposalData.variableOptions.value = valueVar3;
            }
            break;
        }
        case ProposalTypes::Class::MultiVariables:
            input.multiVarData.hasValueDummyStateVariable1 = setVar1;
            input.multiVarData.hasValueDummyStateVariable2 = setVar2;
            input.multiVarData.hasValueDummyStateVariable3 = setVar3;
            input.multiVarData.optionYesValues.dummyStateVariable1 = valueVar1;
            input.multiVarData.optionYesValues.dummyStateVariable2 = valueVar2;
            input.multiVarData.optionYesValues.dummyStateVariable3 = valueVar3;
            break;
        }
        uint16 proposalIdx = this->setShareholderProposal<TESTEXA>(proposer, input);
        if (expectSuccess)
            EXPECT_NE((int)proposalIdx, (int)INVALID_PROPOSAL_INDEX);
        else
            EXPECT_EQ((int)proposalIdx, (int)INVALID_PROPOSAL_INDEX);
        return proposalIdx;
    }

    template <typename StateStruct, typename FullProposalDataT>
    uint16 setProposalInOtherContractAsShareholder(const id& originator, uint16 otherContractIndex, const FullProposalDataT& fullProposalData)
    {
        typename StateStruct::SetProposalInOtherContractAsShareholder_input input;
        copyToBuffer(input, fullProposalData);
        input.otherContractIndex = otherContractIndex;
        typename StateStruct::SetProposalInOtherContractAsShareholder_output output;
        invokeUserProcedure(StateStruct::__contract_index, 40, input, output, originator, 0);
        return output.proposalIndex;
    }

    template <typename StateStruct, typename ProposalDataT>
    bool setVotesInOtherContractAsShareholder(const id& originator, uint16 otherContractIndex, uint16 proposalIndex, const ProposalDataT& proposalData,
        const std::vector<std::pair<sint64, uint32>>& voteValueCountPairs)
    {
        ASSERT(voteValueCountPairs.size() <= 8);
        typename StateStruct::SetVotesInOtherContractAsShareholder_input input{ {proposalIndex, proposalData.type, proposalData.tick} };
        input.otherContractIndex = otherContractIndex;
        input.voteData.voteValues.set(0, NO_VOTE_VALUE); // default with no voteValueCountPairs (vote count 0): set all to no votes
        for (size_t i = 0; i < voteValueCountPairs.size(); ++i)
        {
            input.voteData.voteValues.set(i, voteValueCountPairs[i].first);
            input.voteData.voteCounts.set(i, voteValueCountPairs[i].second);
        }
        typename StateStruct::SetVotesInOtherContractAsShareholder_output output;
        invokeUserProcedure(StateStruct::__contract_index, 41, input, output, originator, 0);
        return output.success;
    }

    void endEpoch(bool expectSuccess = true)
    {
        callSystemProcedure(TESTEXD_CONTRACT_INDEX, END_EPOCH, expectSuccess);
        callSystemProcedure(TESTEXC_CONTRACT_INDEX, END_EPOCH, expectSuccess);
        callSystemProcedure(TESTEXB_CONTRACT_INDEX, END_EPOCH, expectSuccess);
        callSystemProcedure(TESTEXA_CONTRACT_INDEX, END_EPOCH, expectSuccess);
        callSystemProcedure(QX_CONTRACT_INDEX, END_EPOCH, expectSuccess);
    }
};

void checkVoteCounts(const ProposalMultiVoteDataV1& votes, const std::vector<std::pair<sint64, uint32>>& expectedVoteValueCountPairs)
{
    std::vector<std::pair<sint64, uint32>> expectedPairsNotFound = expectedVoteValueCountPairs;
    for (int i = 0; i < votes.voteCounts.capacity(); ++i)
    {
        sint64 value = votes.voteValues.get(i);
        uint32 count = votes.voteCounts.get(i);
        std::pair<sint64, uint32> pair(value, count);
        auto it = std::find(expectedPairsNotFound.begin(), expectedPairsNotFound.end(), pair);
        if (it != expectedPairsNotFound.end())
        {
            // value-count pair found
            expectedPairsNotFound.erase(it);
        }
        else if (count)
        {
            FAIL() << "Error: unexpected vote value/count pair " << value << "/" << count;
        }
    }
    for (const auto& it : expectedPairsNotFound)
    {
        FAIL() << "Error: missing vote value/count pair " << it.first << "/" << it.second;
    }
}

bool operator==(const TESTEXA::MultiVariablesProposalExtraData& p1, const TESTEXA::MultiVariablesProposalExtraData& p2)
{
    return memcmp(&p1, &p2, sizeof(p1)) == 0;
}


TEST(ContractTestEx, QpiReleaseShares)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLOB") };
    const sint64 totalShareCount = 1000000000;
    const sint64 transferShareCount = totalShareCount/4;

    // make sure the entities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with QX
    EXPECT_EQ(test.issueAssetQx(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount);

    // run ownership/possession transfer in QX -> should work (has management rights from issueAsset)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, asset1.issuer, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);

    // run ownership/possession transfer in TESTEXA -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);

    ////////////////////////////////////
    // RELEASE FROM QX TO TESTEXA 

    // invoke release of shares in QX to TESTEXA -> fails because default response of TESTEXA is to reject
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(0);

    // enable that TESTEXA accepts transfer for 0 fee
    test.setPreAcquireSharesOutput<TESTEXA>(true, 0);

    // invoke release of shares in QX to TESTEXA -> succeed
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreAcquireInput(),
        { asset1, USER2, USER2, transferShareCount, 0, QX_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostAcquireInput(),
        { asset1, USER2, USER2, transferShareCount, 0, QX_CONTRACT_INDEX });

    // run ownership/possession transfer in TESTEXA -> should work
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // release shares error case: too few shares -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, 0, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: more shares than available -> fail
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, totalShareCount + 1, TESTEXA_CONTRACT_INDEX, 100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: fee requested by TESTEXA is too high to release shares (QX expects 0) -> fail
    test.setPreAcquireSharesOutput<TESTEXA>(true, 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    // release shares error case: fee requested by TESTEXA is invalid
    test.setPreAcquireSharesOutput<TESTEXA>(true, -100);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    test.setPreAcquireSharesOutput<TESTEXA>(true, MAX_AMOUNT + 1000);
    EXPECT_EQ(test.transferShareManagementRightsQx(asset1, USER1, transferShareCount, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostAcquireCounter(1);

    ////////////////////////////////////
    // RELEASE FROM TESTEXA TO TESTEXB

    // invoke release of shares in TESTEXA to TESTEXB with fee -> succeed
    test.getStateTestExampleB()->checkPostAcquireCounter(0);
    sint64 balanceUser3 = getBalance(USER3);
    sint64 balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    sint64 balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    test.setPreAcquireSharesOutput<TESTEXB>(true, 1000);
    EXPECT_EQ(test.transferShareManagementRights<TESTEXA>(asset1, USER3, transferShareCount, TESTEXB_CONTRACT_INDEX, 1500), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);
    test.getStateTestExampleB()->checkPostAcquireCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreAcquireInput(),
        { asset1, USER3, USER3, transferShareCount, 1500, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostAcquireInput(),
        { asset1, USER3, USER3, transferShareCount, 1000, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER3), balanceUser3 - 1500);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 1500 - 1000);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 1000);

    ////////////////////////////////////
    // RELEASE FROM TESTEXB TO QX

    // run ownership/possession transfer in QX -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with 0 qu -> fails because transfer fee is required by QX
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with sufficient amount but too many shares -> should fail
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, totalShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount);

    // invoke release of shares from TESTEXB to QX with sufficient amount and correct shares -> should work
    EXPECT_EQ(test.transferShareManagementRights<TESTEXB>(asset1, USER3, transferShareCount, QX_CONTRACT_INDEX, test.qxFees.transferFee), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), 0);

    // run ownership/possession transfer in QX -> should work again
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER3, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);
}

TEST(ContractTestEx, QpiAcquireShares)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLURB") };
    const sint64 totalShareCount = 100000000;
    const sint64 transferShareCount = totalShareCount / 4;
    
    // make sure the entities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with TestExampleA
    EXPECT_EQ(test.issueAssetTestExA(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount);

    // run ownership/possession transfer in TestExampleA -> should work (has management rights from issueAsset)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, USER2, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // run ownership/possession transfer in QX -> should fail (requires management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset1, USER2, USER3, transferShareCount), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), 0);


    //////////////////////////////////////////////
    // TESTEXB ACQUIRES FROM TESTEXA

    // TestExampleB tries / fails to acquire management rights (negative shares count)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, -100, TESTEXA_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (more shares than available)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount + 1, TESTEXA_CONTRACT_INDEX, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (negative offered fee)
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, -100), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (rejected by TESTEXA)
    test.setPreReleaseSharesOutput<TESTEXA>(false, 0);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 4999), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (requested fee is negative)
    test.setPreReleaseSharesOutput<TESTEXA>(true, -5000);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 5000), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (offered fee lower than requested fee)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 5000);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 4999), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (not enough QU owned to pay fee)
    test.setPreReleaseSharesOutput<TESTEXA>(true, test.qxFees.assetIssuanceFee * 11);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, test.qxFees.assetIssuanceFee * 11), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB tries / fails to acquire management rights (with wrong originator USER3)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 1234);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount, TESTEXA_CONTRACT_INDEX, 1234, USER3), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), 0);
    test.getStateTestExampleA()->checkPostReleaseCounter(0);

    // TestExampleB acquires management rights (success)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 1234);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount / 4, TESTEXA_CONTRACT_INDEX, 1239), transferShareCount / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount * 3 / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount / 4);
    test.getStateTestExampleA()->checkPostReleaseCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 4, 1239, TESTEXB_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 4, 1234, TESTEXB_CONTRACT_INDEX });

    // TestExampleB acquires management rights (success)
    test.setPreReleaseSharesOutput<TESTEXA>(true, 10);
    sint64 balanceUser2 = getBalance(USER2);
    sint64 balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    sint64 balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXB>(asset1, USER2, transferShareCount * 3 / 4, TESTEXA_CONTRACT_INDEX, 15), transferShareCount * 3 / 4);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount);
    test.getStateTestExampleA()->checkPostReleaseCounter(2);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleA()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount * 3 / 4, 15, TESTEXB_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleA()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount * 3 / 4, 10, TESTEXB_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER2), balanceUser2 - 15);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 10);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 15 - 10);

    // run ownership/possession transfer in TestExampleB -> should work now (after acquiring management rights)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXB>(asset1, USER2, USER3, transferShareCount / 2), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount / 2);

    //////////////////////////////////////////////
    // TESTEXA ACQUIRES FROM TESTEXB

    // TestExampleA acquires management rights from TestExampleB of shares owned by USER2 (success)
    test.getStateTestExampleB()->checkPostReleaseCounter(0);
    test.setPreReleaseSharesOutput<TESTEXB>(true, 13);
    balanceUser2 = getBalance(USER2);
    balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXA>(asset1, USER2, transferShareCount / 10, TESTEXB_CONTRACT_INDEX, 42), transferShareCount / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), transferShareCount / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER2, TESTEXB_CONTRACT_INDEX }, { USER2, TESTEXB_CONTRACT_INDEX }), transferShareCount * 4 / 10);
    test.getStateTestExampleB()->checkPostReleaseCounter(1);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 10, 42, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostReleaseInput(),
        { asset1, USER2, USER2, transferShareCount / 10, 13, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER2), balanceUser2 - 42);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 42 - 13);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 13);

    // TestExampleA acquires management rights from TestExampleB of shares owned by USER3 (success)
    test.setPreReleaseSharesOutput<TESTEXB>(true, 123);
    sint64 balanceUser3 = getBalance(USER3);
    balanceTestExA = getBalance(TESTEXA_CONTRACT_ID);
    balanceTestExB = getBalance(TESTEXB_CONTRACT_ID);
    EXPECT_EQ(test.acquireShareManagementRights<TESTEXA>(asset1, USER3, transferShareCount * 2 / 10, TESTEXB_CONTRACT_INDEX, 124), transferShareCount * 2 / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXA_CONTRACT_INDEX }, { USER3, TESTEXA_CONTRACT_INDEX }), transferShareCount * 2 / 10);
    EXPECT_EQ(numberOfShares(asset1, { USER3, TESTEXB_CONTRACT_INDEX }, { USER3, TESTEXB_CONTRACT_INDEX }), transferShareCount * 3 / 10);
    test.getStateTestExampleB()->checkPostReleaseCounter(2);
    checkPreManagementRightsTransferInput(
        test.getStateTestExampleB()->getPreReleaseInput(),
        { asset1, USER3, USER3, transferShareCount * 2 / 10, 124, TESTEXA_CONTRACT_INDEX });
    checkPostManagementRightsTransferInput(
        test.getStateTestExampleB()->getPostReleaseInput(),
        { asset1, USER3, USER3, transferShareCount * 2 / 10, 123, TESTEXA_CONTRACT_INDEX });
    EXPECT_EQ(getBalance(USER3), balanceUser3 - 124);
    EXPECT_EQ(getBalance(TESTEXA_CONTRACT_ID), balanceTestExA + 124 - 123);
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), balanceTestExB + 123);

    // Some count final checks
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(QX_CONTRACT_INDEX)), 0);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(TESTEXA_CONTRACT_INDEX)), totalShareCount - transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byManagingContract(TESTEXB_CONTRACT_INDEX)), transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER1)), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER2)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::byOwner(USER3)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(QX_CONTRACT_INDEX)), 0);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(TESTEXA_CONTRACT_INDEX)), totalShareCount - transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byManagingContract(TESTEXB_CONTRACT_INDEX)), transferShareCount * 7 / 10);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER1)), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER2)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1, AssetOwnershipSelect::any(), AssetPossessionSelect::byPossessor(USER3)), transferShareCount / 2);
    EXPECT_EQ(numberOfShares(asset1), totalShareCount);
}

TEST(ContractTestEx, GetManagementRightsByInvokingOtherContractsRelease)
{
    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("BLURB") };
    const sint64 totalShareCount = 1000000;
    const sint64 transferShareCount = totalShareCount / 5;

    // make sure the entities have enough qu
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER2, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(USER3, test.qxFees.assetIssuanceFee * 10);

    // issueAsset with TestExampleA
    EXPECT_EQ(test.issueAssetTestExA(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount);

    ///////////////////////////////////////////////////////////////////////////
    // TESTEXB ACQUIRES FROM TESTEXA BY INVOKING TESTEXA PROCEDURE (QX PATTERN)

    // run ownership/possession transfer to TestExB in TestExampleA -> should work (has management rights from issueAsset)
    // (TestExB needs to own/possess shares in order to invoke TestExA for transferring rights to TestExB in the next step)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, TESTEXB_CONTRACT_ID, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Transfer rights to TestExB using the QX approach
    // -> Test that we don't get a deadlock in the following case:
    //    invoke procedure of TestExB, which invokes procedure of TestExA for calling qpi.releaseShares(), which runs
    //    callback PRE_ACQUIRE_SHARES of TestExB
    // Attempt 1: fail due to forbidding by default
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Attempt 2: allow -> fail because requested fee > offered fee
    test.setPreAcquireSharesOutput<TESTEXB>(true, 13);
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 0), 0);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // Attempt 2: allow -> success
    EXPECT_EQ(test.getTestExAsShareManagementRightsByInvokingTestExB(asset1, USER1, transferShareCount, 15), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXB_CONTRACT_ID, TESTEXB_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, TESTEXB_CONTRACT_INDEX }), transferShareCount);
}

// Test stopping + cleanup of contract functions execution for recursive function leading to stack overflow
TEST(ContractTestEx, AbortFunction)
{
    ContractTestingTestEx test;

    // Successfully run function
    TESTEXA::QueryQpiFunctions_input input{};
    TESTEXA::QueryQpiFunctions_output output{};
    EXPECT_EQ(test.callFunctionOfTestExampleAFromTextExampleB(input, output, true), NoContractError);

    // Check that error handling works when error is supposed to happen
    EXPECT_EQ(test.callErrorTriggerFunction(), ContractErrorAllocLocalsFailed);
}

static id getUser(unsigned long long i)
{
    return id(i, i / 2 + 4, i + 10, i * 3 + 8);
}

static void concurrentContractFunctionCall(ContractTestingTestEx* test)
{
    // This calls a user function in contract TestExampleA that calls a function in TestExampleB.
    // When running concurrently with a management rights transfer, this may trigger a deadlock
    // that needs to be resolved.
    for (int i = 0; i < 3; ++i)
    {
        TESTEXA::QueryQpiFunctions_input queryQpiFuncInput;
        TESTEXA::QueryQpiFunctions_output qpiReturned;
        setMemory(utcTime, 0);
        utcTime.Year = 2022;
        utcTime.Month = 4;
        utcTime.Day = 13;
        utcTime.Hour = 12;
        updateQpiTime();
        bool expectSuccess = false;
        unsigned int errorCode = test->callFunctionOfTestExampleAFromTextExampleB(queryQpiFuncInput, qpiReturned, expectSuccess);
        ASSERT(errorCode == ContractErrorStoppedToResolveDeadlock || errorCode == NoContractError);
        if (errorCode == NoContractError)
        {
            EXPECT_EQ(qpiReturned.qpiFunctionsOutput.year, 22);
            EXPECT_EQ(qpiReturned.qpiFunctionsOutput.month, 4);
            EXPECT_EQ(qpiReturned.qpiFunctionsOutput.day, 13);
            EXPECT_EQ(qpiReturned.qpiFunctionsOutput.hour, 12);
        }
    }
}

TEST(ContractTestEx, ResolveDeadlockCallbackProcedureAndConcurrentFunction)
{
    // deadlock pattern (with index of TestExC > TestExB > TestExA):
    // 1. TestExC procedure invokes TestExA procedure, which runs qpi.releaseShares() to TestExC leading to a call of
    //    PRE_ACQUIRE_SHARES in TestExC
    //    -> solution to resolve deadlock: reusing lock
    // 2. PRE_ACQUIRE_SHARES in TestExC invokes a TestExA procedure (this runs a lot of computation to wait for
    //    concurrent execution of contract function needed for test 3)
    //    -> solution to resolve deadlock: reusing lock
    // 3. PRE_ACQUIRE_SHARES in TestExC tries to invoke a procedure of TestExB; this leads to a deadlock if a contract
    //    function of TextExB is running in parallel that tries to run a function of TextExA:
    //    -> contract function of TestExB running in request processor is waiting for a read lock of TestExA
    //    -> read lock of TestExA cannot be acquired before qpi.releaseShares() returns, which it doesn't because
    //       PRE_ACQUIRE_SHARES is waiting for a write lock of TestExB, which cannot be acquired before the contract
    //       function of TestExB is finished
    //    -> solution to resolve deadlock: cancel contract function of TestExB

    ContractTestingTestEx test;

    const Asset asset1{ USER1, assetNameFromString("WOBBL") };
    const sint64 totalShareCount = 100000000;
    const int numberOfUsers = 500;
    const sint64 transferShareCount = totalShareCount / numberOfUsers / 10;

    // populate spectrum
    increaseEnergy(USER1, test.qxFees.assetIssuanceFee * 10);
    increaseEnergy(TESTEXC_CONTRACT_ID, test.qxFees.assetIssuanceFee * 10);
    for (int i = 0; i < numberOfUsers; ++i)
        increaseEnergy(getUser(i), test.qxFees.assetIssuanceFee * (i % 1000 + 1));

    // issueAsset with TestExampleA
    EXPECT_EQ(test.issueAssetTestExA(asset1, totalShareCount, 0, 0), totalShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount);

    // run ownership/possession transfer to TestExC in TestExA (needed to run deadlock test below)
    EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, TESTEXC_CONTRACT_ID, transferShareCount), transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { USER1, TESTEXA_CONTRACT_INDEX }, { USER1, TESTEXA_CONTRACT_INDEX }), totalShareCount - transferShareCount);
    EXPECT_EQ(numberOfShares(asset1, { TESTEXC_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }, { TESTEXC_CONTRACT_ID, TESTEXA_CONTRACT_INDEX }), transferShareCount);

    // populate universe
    for (int i = 0; i < numberOfUsers; ++i)
        EXPECT_EQ(test.transferShareOwnershipAndPossession<TESTEXA>(asset1, asset1.issuer, getUser(i), transferShareCount), transferShareCount);

    // start procedure call for deadlock test (baseline without concurrent function call)
    {
        std::cout << "Test callback procedure without concurrent function ..." << std::endl;
        auto startTime = std::chrono::high_resolution_clock::now();
        test.getTestExAsShareManagementRightsByInvokingTestExC(asset1, TESTEXC_CONTRACT_ID, 1, 0);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "Run-time of procedure without concurrent function: " << durationMilliSec << " milliseconds\n" << std::endl;
    }

    // start function call (baseline without concurrent procedure call)
    {
        std::cout << "Test function without concurrent procedure ..." << std::endl;
        auto startTime = std::chrono::high_resolution_clock::now();
        TESTEXA::QueryQpiFunctions_input input;
        TESTEXA::QueryQpiFunctions_output output;
        test.callFunctionOfTestExampleAFromTextExampleB(input, output, true);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "Run-time of function without concurrent procedure: " << durationMilliSec << " milliseconds\n" << std::endl;
    }

    // deadlock test with procedure and concurrent function call
    for (int i = 0; i < 3; ++i)
    {
        std::cout << "Test callback procedure with concurrent function ..." << std::endl;
        auto startTime = std::chrono::high_resolution_clock::now();

        auto lambda = [](ContractTestingTestEx* test, const Asset& asset1) { test->getTestExAsShareManagementRightsByInvokingTestExC(asset1, TESTEXC_CONTRACT_ID, 1, 0); };

        auto userProcedureThread = std::thread(lambda, &test, asset1);
        auto userFunctionThread = std::thread(concurrentContractFunctionCall, &test);

        userProcedureThread.join();
        userFunctionThread.join();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationMilliSec = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "Run-time of procedure with concurrent function: " << durationMilliSec << " milliseconds\n" << std::endl;
    }
}

TEST(ContractTestEx, QueryBasicQpiFunctions)
{
    ContractTestingTestEx test;

    id arbitratorPubKey;
    getPublicKeyFromIdentity((const unsigned char*)ARBITRATOR, arbitratorPubKey.m256i_u8);

    // prepare data for qpi.K12() and qpi.signatureValidity()
    TESTEXA::QueryQpiFunctions_input queryQpiFuncInput1;
    id subseed1(123456789, 987654321, 1357986420, 0xabcdef);
    id privateKey1, digest1;
    getPrivateKey(subseed1.m256i_u8, privateKey1.m256i_u8);
    getPublicKey(privateKey1.m256i_u8, queryQpiFuncInput1.entity.m256i_u8);
    for (uint64 i = 0; i < queryQpiFuncInput1.data.capacity(); ++i)
        queryQpiFuncInput1.data.set(i, static_cast<sint8>(i - 50));
    KangarooTwelve(&queryQpiFuncInput1.data, sizeof(queryQpiFuncInput1.data), &digest1, sizeof(digest1));
    sign(subseed1.m256i_u8, queryQpiFuncInput1.entity.m256i_u8, digest1.m256i_u8, (unsigned char*)&queryQpiFuncInput1.signature);

    // Test 1 with initial time
    setMemory(utcTime, 0);
    utcTime.Year = 2022;
    utcTime.Month = 4;
    utcTime.Day = 13;
    utcTime.Hour = 12;
    updateQpiTime();
    numberTickTransactions = 123;
    system.tick = 4567890;
    system.epoch = 987;
    auto qpiReturned1 = test.queryQpiFunctions(queryQpiFuncInput1);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.year, 22);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.month, 4);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.day, 13);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.hour, 12);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.minute, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.second, 0);
    EXPECT_EQ((int)qpiReturned1.qpiFunctionsOutput.millisecond, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.dayOfWeek, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.arbitrator, arbitratorPubKey);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.computor0, id::zero());
    EXPECT_EQ((int)qpiReturned1.qpiFunctionsOutput.epoch, (int)system.epoch);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.invocationReward, 0);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.invocator, id::zero());
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.numberOfTickTransactions, numberTickTransactions);
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.originator, id::zero());
    EXPECT_EQ(qpiReturned1.qpiFunctionsOutput.tick, system.tick);
    EXPECT_EQ(qpiReturned1.inputDataK12, digest1);
    EXPECT_TRUE(qpiReturned1.inputSignatureValid);

    // K12 test for wrong signature: change data but not signature
    TESTEXA::QueryQpiFunctions_input queryQpiFuncInput2 = queryQpiFuncInput1;
    id digest2;
    queryQpiFuncInput2.data.set(0, 0);
    KangarooTwelve(&queryQpiFuncInput2.data, sizeof(queryQpiFuncInput2.data), &digest2, sizeof(digest2));
    EXPECT_NE(digest1, digest2);

    // Test 2 with current time
    updateTime();
    updateQpiTime();
    numberTickTransactions = 42;
    system.tick = 4567891;
    system.epoch = 988;
    broadcastedComputors.computors.publicKeys[0] = id(12, 34, 56, 78);
    auto qpiReturned2 = test.queryQpiFunctions(queryQpiFuncInput2);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.year, utcTime.Year - 2000);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.month, utcTime.Month);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.day, utcTime.Day);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.hour, utcTime.Hour);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.minute, utcTime.Minute);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.second, utcTime.Second);
    EXPECT_EQ((int)qpiReturned2.qpiFunctionsOutput.millisecond, utcTime.Nanosecond / 1000000);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.dayOfWeek, dayIndex(qpiReturned2.qpiFunctionsOutput.year, qpiReturned2.qpiFunctionsOutput.month, qpiReturned2.qpiFunctionsOutput.day) % 7);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.arbitrator, arbitratorPubKey);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.computor0, id(12, 34, 56, 78));
    EXPECT_EQ((int)qpiReturned2.qpiFunctionsOutput.epoch, (int)system.epoch);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.invocationReward, 0);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.invocator, id::zero());
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.numberOfTickTransactions, numberTickTransactions);
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.originator, id::zero());
    EXPECT_EQ(qpiReturned2.qpiFunctionsOutput.tick, system.tick);
    EXPECT_EQ(qpiReturned2.inputDataK12, digest2);
    EXPECT_FALSE(qpiReturned2.inputSignatureValid);
}

TEST(ContractTestEx, QpiFunctionsIPO)
{
    // test IPO functions with IPO of TESTEXD
    ContractTestingTestEx test;
    system.epoch = contractDescriptions[TESTEXD_CONTRACT_INDEX].constructionEpoch - 1;
    constexpr long long initialBalance = 12345678;
    increaseEnergy(USER1, initialBalance);
    increaseEnergy(TESTEXB_CONTRACT_ID, initialBalance);
    increaseEnergy(TESTEXC_CONTRACT_ID, initialBalance);

    // Test output of qpi functions for invalid contract index
    for (int i = 0; i < NUMBER_OF_COMPUTORS + 2; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(contractCount, i);
        EXPECT_TRUE(isZero(bid.publicKey));
        EXPECT_EQ(bid.price, -1);
    }

    // Test output of qpi functions for contract that is not in IPO
    for (int i = 0; i < NUMBER_OF_COMPUTORS + 2; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXB_CONTRACT_INDEX, i);
        EXPECT_TRUE(isZero(bid.publicKey));
        EXPECT_EQ(bid.price, -2);
    }

    // Test output of qpi functions without any bids
    for (int i = 0; i < NUMBER_OF_COMPUTORS + 2; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        EXPECT_TRUE(isZero(bid.publicKey));
        EXPECT_EQ(bid.price, (i < NUMBER_OF_COMPUTORS) ? 0 : -3);
    }

    // Test bids with invalid contract, price, and quantity
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(contractCount, 10, 100), -1);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXC_CONTRACT_INDEX, 10, 100), -1);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 0, 100), -1);
    EXPECT_EQ(test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, 0).price, 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, MAX_AMOUNT, 100), -1);
    EXPECT_EQ(test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, 0).price, 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 10, 0), -1);
    EXPECT_EQ(test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, 0).price, 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 10, NUMBER_OF_COMPUTORS + 1), -1);
    EXPECT_EQ(test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, 0).price, 0);

    // Successfully bid in IPO
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 10, 100), 100);
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        EXPECT_EQ(bid.publicKey, (i < 100) ? TESTEXC_CONTRACT_ID : NULL_ID);
        EXPECT_EQ(bid.price, (i < 100) ? 10 : 0);
    }
    EXPECT_EQ(test.qpiBidInIpo<TESTEXB>(TESTEXD_CONTRACT_INDEX, 100, 600), 600);
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        EXPECT_EQ(bid.publicKey, (i < 600) ? TESTEXB_CONTRACT_ID : TESTEXC_CONTRACT_ID);
        EXPECT_EQ(bid.price, (i < 600) ? 100 : 10);
    }
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 1000, 10), 10);
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        if (i < 10)
        {
            EXPECT_EQ(bid.publicKey, TESTEXC_CONTRACT_ID);
            EXPECT_EQ(bid.price, 1000);
        }
        else if (i < 10 + 600)
        {
            EXPECT_EQ(bid.publicKey, TESTEXB_CONTRACT_ID);
            EXPECT_EQ(bid.price, 100);
        }
        else
        {
            EXPECT_EQ(bid.publicKey, TESTEXC_CONTRACT_ID);
            EXPECT_EQ(bid.price, 10);
        }
    }

    // Test too low bids
    EXPECT_EQ(test.qpiBidInIpo<TESTEXB>(TESTEXD_CONTRACT_INDEX, 1, 10), 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 2, 10), 0);

    // Simulate end of IPO
    finishIPOs();

    // Check contract shares
    Asset asset{NULL_ID, assetNameFromString("TESTEXD")};
    EXPECT_EQ(600, numberOfShares(asset, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }));
    EXPECT_EQ(76, numberOfShares(asset, { TESTEXC_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXC_CONTRACT_ID, QX_CONTRACT_INDEX }));

    // Check balances
    const long long finalPrice = 10;
    EXPECT_EQ(getBalance(TESTEXB_CONTRACT_ID), initialBalance - 600 * finalPrice);
    EXPECT_EQ(getBalance(TESTEXC_CONTRACT_ID), initialBalance - 76 * finalPrice);
}

//-------------------------------------------------------------------
// Test CallbackPostIncomingTransfer

class ContractTestCallbackPostIncomingTransfer : public ContractTestingTestEx
{
public:
    // test qpi.transfer() on contract SrcStateStruct. DstStateStruct is another contract to check.
    template <typename SrcStateStruct, typename DstStateStruct>
    void testQpiTransfer(const id& dstPublicKey, sint64 amount, sint64 fee = 0, const id& originator = USER1)
    {
        const id srcPublicKey(SrcStateStruct::__contract_index, 0, 0, 0);
        const sint64 originatorBalanceBefore = getBalance(originator);
        const sint64 srcBalanceBefore = getBalance(srcPublicKey);
        const sint64 dstBalanceBefore = getBalance(dstPublicKey);
        const auto srcBefore = getIncomingTransferAmounts<SrcStateStruct>();
        const auto dstBefore = getIncomingTransferAmounts<DstStateStruct>();

        EXPECT_GE(originatorBalanceBefore, fee);
        bool success = qpiTransfer<SrcStateStruct>(dstPublicKey, amount, fee, originator);
        EXPECT_TRUE(success);

        if (success)
        {
            const sint64 originatorBalanceAfter = getBalance(originator);
            const sint64 srcBalanceAfter = getBalance(srcPublicKey);
            const sint64 dstBalanceAfter = getBalance(dstPublicKey);
            EXPECT_EQ(originatorBalanceAfter, originatorBalanceBefore - fee);
            if (srcPublicKey != dstPublicKey)
            {
                EXPECT_EQ(srcBalanceAfter, srcBalanceBefore + fee - amount);
                EXPECT_EQ(dstBalanceAfter, dstBalanceBefore + amount);
            }
            else
            {
                EXPECT_EQ(srcBalanceAfter, srcBalanceBefore + fee);
            }

            const auto srcAfter = getIncomingTransferAmounts<SrcStateStruct>();
            const auto dstAfter = getIncomingTransferAmounts<DstStateStruct>();
            EXPECT_EQ(srcAfter.procedureTransactionAmount, srcBefore.procedureTransactionAmount + fee);
            if (srcPublicKey != dstPublicKey)
            {
                EXPECT_EQ(dstAfter.procedureTransactionAmount, dstBefore.procedureTransactionAmount);
                EXPECT_EQ(srcAfter.qpiTransferAmount, srcBefore.qpiTransferAmount);
            }
            if (dstPublicKey == id(DstStateStruct::__contract_index, 0, 0, 0))
            {
                EXPECT_EQ(dstAfter.qpiTransferAmount, dstBefore.qpiTransferAmount + amount);
            }
            else
            {
                EXPECT_EQ(dstAfter.qpiTransferAmount, dstBefore.qpiTransferAmount);
            }
            EXPECT_EQ(srcAfter.standardTransactionAmount, srcBefore.standardTransactionAmount);
            EXPECT_EQ(dstAfter.standardTransactionAmount, dstBefore.standardTransactionAmount);
            EXPECT_EQ(srcAfter.qpiDistributeDividendsAmount, srcBefore.qpiDistributeDividendsAmount);
            EXPECT_EQ(dstAfter.qpiDistributeDividendsAmount, dstBefore.qpiDistributeDividendsAmount);
            EXPECT_EQ(srcAfter.revenueDonationAmount, srcBefore.revenueDonationAmount);
            EXPECT_EQ(dstAfter.revenueDonationAmount, dstBefore.revenueDonationAmount);
            EXPECT_EQ(srcAfter.ipoBidRefundAmount, srcBefore.ipoBidRefundAmount);
            EXPECT_EQ(dstAfter.ipoBidRefundAmount, dstBefore.ipoBidRefundAmount);
        }
    }

    // test qpi.distributeDividends() on contract SrcStateStruct. DstStateStruct is another contract to check.
    template <typename SrcStateStruct, typename DstStateStruct>
    void testQpiDistributeDividends(sint64 amountPerShare, const std::vector<std::pair<m256i, unsigned int>>& shareholders, sint64 fee = 0, const id& originator = USER1)
    {
        // check number of shares
        unsigned int totalShareCount = 0;
        for (const auto& ownerShareCountPair : shareholders)
            totalShareCount += ownerShareCountPair.second;
        EXPECT_EQ(totalShareCount, NUMBER_OF_COMPUTORS);

        // get state before call and compute state expected after call
        const id srcPublicKey(SrcStateStruct::__contract_index, 0, 0, 0);
        const id dstPublicKey(DstStateStruct::__contract_index, 0, 0, 0);
        std::map<id, sint64> expectedBalances;
        expectedBalances[originator] = getBalance(originator);
        EXPECT_GE(expectedBalances[originator], fee);
        expectedBalances[srcPublicKey] = getBalance(srcPublicKey);
        expectedBalances[dstPublicKey] = getBalance(dstPublicKey);
        for (const auto& ownerShareCountPair : shareholders)
            expectedBalances[ownerShareCountPair.first] = getBalance(ownerShareCountPair.first);
        expectedBalances[originator] -= fee;
        expectedBalances[srcPublicKey] += fee - amountPerShare * NUMBER_OF_COMPUTORS;
        auto expectedIncomingSrc = getIncomingTransferAmounts<SrcStateStruct>();
        auto expectedIncomingDst = getIncomingTransferAmounts<DstStateStruct>();
        expectedIncomingSrc.procedureTransactionAmount += fee;
        if (srcPublicKey == dstPublicKey)
            expectedIncomingDst.procedureTransactionAmount += fee;
        for (const auto& ownerShareCountPair : shareholders)
        {
            const sint64 dividend = amountPerShare * ownerShareCountPair.second;
            expectedBalances[ownerShareCountPair.first] += dividend;
            if (ownerShareCountPair.first == srcPublicKey)
                expectedIncomingSrc.qpiDistributeDividendsAmount += dividend;
            if (ownerShareCountPair.first == dstPublicKey)
                expectedIncomingDst.qpiDistributeDividendsAmount += dividend;
        }

        bool success = qpiDistributeDividends<SrcStateStruct>(amountPerShare, fee, originator);
        EXPECT_TRUE(success);

        if (success)
        {
            for (const auto& idBalancePair : expectedBalances)
            {
                EXPECT_EQ(getBalance(idBalancePair.first), idBalancePair.second);
            }

            const auto observedIncomingSrc = getIncomingTransferAmounts<SrcStateStruct>();
            const auto observedIncomingDst = getIncomingTransferAmounts<DstStateStruct>();
            EXPECT_EQ(expectedIncomingSrc.standardTransactionAmount, observedIncomingSrc.standardTransactionAmount);
            EXPECT_EQ(expectedIncomingDst.standardTransactionAmount, observedIncomingDst.standardTransactionAmount);
            EXPECT_EQ(expectedIncomingSrc.procedureTransactionAmount, observedIncomingSrc.procedureTransactionAmount);
            EXPECT_EQ(expectedIncomingDst.procedureTransactionAmount, observedIncomingDst.procedureTransactionAmount);
            EXPECT_EQ(expectedIncomingSrc.qpiTransferAmount, observedIncomingSrc.qpiTransferAmount);
            EXPECT_EQ(expectedIncomingDst.qpiTransferAmount, observedIncomingDst.qpiTransferAmount);
            EXPECT_EQ(expectedIncomingSrc.qpiDistributeDividendsAmount, observedIncomingSrc.qpiDistributeDividendsAmount);
            EXPECT_EQ(expectedIncomingDst.qpiDistributeDividendsAmount, observedIncomingDst.qpiDistributeDividendsAmount);
            EXPECT_EQ(expectedIncomingSrc.revenueDonationAmount, observedIncomingSrc.revenueDonationAmount);
            EXPECT_EQ(expectedIncomingDst.revenueDonationAmount, observedIncomingDst.revenueDonationAmount);
            EXPECT_EQ(expectedIncomingSrc.ipoBidRefundAmount, observedIncomingSrc.ipoBidRefundAmount);
            EXPECT_EQ(expectedIncomingDst.ipoBidRefundAmount, observedIncomingDst.ipoBidRefundAmount);
        }
    }
};

TEST(ContractTestEx, CallbackPostIncomingTransfer)
{
    // Tested types of incoming transfers (should be also tested in testnet):
    // - TransferType::qpiTransfer (including transfer to oneself)
    // - TransferType::qpiDistributeDividends (including dividends to oneself)
    // - TransferType::procedureTransaction
    // - TransferType::ipoBidRefund through qpi.bidInIpo()
    //
    // Important test: triggering callback from callback must be prevented (checked by ASSERTs in contracts)
    //
    // The following cannot be tested with Google Test at the moment and have to be tested in the testnet.
    // - TransferType::standardTransaction
    // - TransferType::revenueDonation
    // - TransferType::ipoBidRefund through transaction
    ContractTestCallbackPostIncomingTransfer test;

    increaseEnergy(USER1, 12345678);
    increaseEnergy(USER2, 31427);
    increaseEnergy(USER3, 218000);
    increaseEnergy(TESTEXB_CONTRACT_ID, 19283764);
    increaseEnergy(TESTEXC_CONTRACT_ID, 987654321);

    // qpi.transfer() to other contract
    test.testQpiTransfer<TESTEXB, TESTEXC>(TESTEXC_CONTRACT_ID, 100, 1000, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXB>(TESTEXB_CONTRACT_ID, 2000, 200, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXB>(TESTEXB_CONTRACT_ID, 300, 3000, USER1);
    test.testQpiTransfer<TESTEXB, TESTEXC>(TESTEXC_CONTRACT_ID, 4000, 400, USER1);

    // qpi.transfer() to self
    test.testQpiTransfer<TESTEXB, TESTEXB>(TESTEXB_CONTRACT_ID, 50, 500, USER1);
    test.testQpiTransfer<TESTEXB, TESTEXB>(TESTEXB_CONTRACT_ID, 600, 60, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXC>(TESTEXC_CONTRACT_ID, 700, 7000, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXC>(TESTEXC_CONTRACT_ID, 8000, 800, USER1);

    // qpi.transfer() to non-contract entity
    test.testQpiTransfer<TESTEXB, TESTEXC>(getUser(0), 900, 9000, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXB>(getUser(1), 10000, 1000, USER1);
    test.testQpiTransfer<TESTEXC, TESTEXB>(getUser(2), 11000, 1100, USER1);
    test.testQpiTransfer<TESTEXB, TESTEXC>(getUser(3), 12000, 1200, USER1);

    // issue contract shares
    std::vector<std::pair<m256i, unsigned int>> sharesTestExB{ {USER1, 356}, {TESTEXC_CONTRACT_ID, 200}, {TESTEXB_CONTRACT_ID, 100}, {TESTEXA_CONTRACT_ID, 20} };
    issueContractShares(TESTEXB_CONTRACT_INDEX, sharesTestExB);
    std::vector<std::pair<m256i, unsigned int>> sharesTestExC{ {USER2, 576}, {USER3, 40}, {TESTEXC_CONTRACT_ID, 30}, {TESTEXB_CONTRACT_ID, 20}, {TESTEXA_CONTRACT_ID, 10} };
    issueContractShares(TESTEXC_CONTRACT_INDEX, sharesTestExC);

    // test qpi.distributeDividends()
    test.testQpiDistributeDividends<TESTEXB, TESTEXC>(1, sharesTestExB, 1234, USER1);
    test.testQpiDistributeDividends<TESTEXB, TESTEXC>(11, sharesTestExB, 9764, USER2);
    test.testQpiDistributeDividends<TESTEXB, TESTEXB>(3, sharesTestExB, 42, USER3);
    test.testQpiDistributeDividends<TESTEXC, TESTEXB>(2, sharesTestExC, 12345, USER1);
    test.testQpiDistributeDividends<TESTEXC, TESTEXB>(13, sharesTestExC, 98, USER2);
    test.testQpiDistributeDividends<TESTEXC, TESTEXC>(4, sharesTestExC, 9, USER3);

    // test refund in qpi.bidInIPO() and finalizeIpo()
    system.epoch = contractDescriptions[TESTEXD_CONTRACT_INDEX].constructionEpoch - 1;
    auto itaB1 = test.getIncomingTransferAmounts<TESTEXB>();
    auto itaC1 = test.getIncomingTransferAmounts<TESTEXC>();
    EXPECT_EQ(itaB1.ipoBidRefundAmount, 0);
    EXPECT_EQ(itaC1.ipoBidRefundAmount, 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXB>(TESTEXD_CONTRACT_INDEX, 20, NUMBER_OF_COMPUTORS, 42), NUMBER_OF_COMPUTORS);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 30, NUMBER_OF_COMPUTORS * 3 / 4, 13), NUMBER_OF_COMPUTORS * 3 / 4);
    auto itaB2 = test.getIncomingTransferAmounts<TESTEXB>();
    auto itaC2 = test.getIncomingTransferAmounts<TESTEXC>();
    // -> in 75% 30 (C), in 25% 20 (B), refund 75% 20 (B)
    EXPECT_EQ(itaB2.ipoBidRefundAmount, 20 * NUMBER_OF_COMPUTORS * 3 / 4);
    EXPECT_EQ(itaC2.ipoBidRefundAmount, 0);
    EXPECT_EQ(itaB2.procedureTransactionAmount, itaB1.procedureTransactionAmount + 42);
    EXPECT_EQ(itaC2.procedureTransactionAmount, itaC1.procedureTransactionAmount + 13);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 50, NUMBER_OF_COMPUTORS / 2, 3), NUMBER_OF_COMPUTORS / 2);
    auto itaB3 = test.getIncomingTransferAmounts<TESTEXB>();
    auto itaC3 = test.getIncomingTransferAmounts<TESTEXC>();
    // -> in 50% 50 (C), in 50% 30 (C), ex 25% 30 (C), ex 25% 20 (B)
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        EXPECT_EQ(bid.publicKey, TESTEXC_CONTRACT_ID);
        EXPECT_EQ(bid.price, (i < NUMBER_OF_COMPUTORS / 2) ? 50 : 30);
    }
    EXPECT_EQ(itaB3.ipoBidRefundAmount, itaB2.ipoBidRefundAmount + 20 * NUMBER_OF_COMPUTORS / 4);
    EXPECT_EQ(itaC3.ipoBidRefundAmount, itaC2.ipoBidRefundAmount + 30 * NUMBER_OF_COMPUTORS / 4);
    EXPECT_EQ(itaB3.procedureTransactionAmount, itaB2.procedureTransactionAmount);
    EXPECT_EQ(itaC3.procedureTransactionAmount, itaC2.procedureTransactionAmount + 3);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXB>(TESTEXD_CONTRACT_INDEX, 99, NUMBER_OF_COMPUTORS * 3 / 4, 14), NUMBER_OF_COMPUTORS * 3 / 4);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXB>(TESTEXD_CONTRACT_INDEX, 9, NUMBER_OF_COMPUTORS / 2, 123), 0);
    EXPECT_EQ(test.qpiBidInIpo<TESTEXC>(TESTEXD_CONTRACT_INDEX, 60, NUMBER_OF_COMPUTORS, 654), NUMBER_OF_COMPUTORS / 4);
    auto itaB4 = test.getIncomingTransferAmounts<TESTEXB>();
    auto itaC4 = test.getIncomingTransferAmounts<TESTEXC>();
    // -> in 75% 99 (B), in 25% 60 (C), ex 75% 60 (C), ex 50% 50 (C), ex 50% 30 (C), ex 50% 9 (B)
    for (int i = 0; i < NUMBER_OF_COMPUTORS; ++i)
    {
        const auto bid = test.getIpoBid<TESTEXC>(TESTEXD_CONTRACT_INDEX, i);
        EXPECT_EQ(bid.publicKey, (i < NUMBER_OF_COMPUTORS * 3 / 4) ? TESTEXB_CONTRACT_ID : TESTEXC_CONTRACT_ID);
        EXPECT_EQ(bid.price, (i < NUMBER_OF_COMPUTORS * 3 / 4) ? 99 : 60);
    }
    EXPECT_EQ(itaB4.ipoBidRefundAmount, itaB3.ipoBidRefundAmount + 9 * NUMBER_OF_COMPUTORS / 2);
    EXPECT_EQ(itaC4.ipoBidRefundAmount, itaC3.ipoBidRefundAmount + 60 * NUMBER_OF_COMPUTORS * 3 / 4 + 50 * NUMBER_OF_COMPUTORS / 2 + 30 * NUMBER_OF_COMPUTORS / 2);
    EXPECT_EQ(itaB4.procedureTransactionAmount, itaB3.procedureTransactionAmount + 14 + 123);
    EXPECT_EQ(itaC4.procedureTransactionAmount, itaC3.procedureTransactionAmount + 654);

    // simulate end of IPO
    finishIPOs();

    // check contract shares
    Asset asset{ NULL_ID, assetNameFromString("TESTEXD") };
    EXPECT_EQ(NUMBER_OF_COMPUTORS * 3 / 4, numberOfShares(asset, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }));
    EXPECT_EQ(NUMBER_OF_COMPUTORS * 1 / 4, numberOfShares(asset, { TESTEXC_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXC_CONTRACT_ID, QX_CONTRACT_INDEX }));

    // check refunds (finalPrice = 60)
    auto itaB5 = test.getIncomingTransferAmounts<TESTEXB>();
    auto itaC5 = test.getIncomingTransferAmounts<TESTEXC>();
    EXPECT_EQ(itaB5.ipoBidRefundAmount, itaB4.ipoBidRefundAmount + NUMBER_OF_COMPUTORS * 3 / 4 * (99 - 60));
    EXPECT_EQ(itaC5.ipoBidRefundAmount, itaC4.ipoBidRefundAmount);
    EXPECT_EQ(itaB5.procedureTransactionAmount, itaB4.procedureTransactionAmount);
    EXPECT_EQ(itaC5.procedureTransactionAmount, itaC4.procedureTransactionAmount);
    EXPECT_EQ(itaB5.standardTransactionAmount, itaB1.standardTransactionAmount);
    EXPECT_EQ(itaC5.standardTransactionAmount, itaC1.standardTransactionAmount);
    EXPECT_EQ(itaB5.qpiDistributeDividendsAmount, itaB1.qpiDistributeDividendsAmount);
    EXPECT_EQ(itaC5.qpiDistributeDividendsAmount, itaC1.qpiDistributeDividendsAmount);
    EXPECT_EQ(itaB5.qpiTransferAmount, itaB1.qpiTransferAmount);
    EXPECT_EQ(itaC5.qpiTransferAmount, itaC1.qpiTransferAmount);
    EXPECT_EQ(itaB5.revenueDonationAmount, itaB1.revenueDonationAmount);
    EXPECT_EQ(itaC5.revenueDonationAmount, itaC1.revenueDonationAmount);
}

TEST(ContractTestEx, BurnAssets)
{
    ContractTestCallbackPostIncomingTransfer test;

    increaseEnergy(USER1, 1234567890123llu);
    increaseEnergy(TESTEXB_CONTRACT_ID, 19283764);

    {
        // issue contract shares
        Asset asset{ NULL_ID, assetNameFromString("TESTEXB") };
        std::vector<std::pair<m256i, unsigned int>> sharesTestExB{ {USER1, 356}, {TESTEXC_CONTRACT_ID, 200}, {TESTEXB_CONTRACT_ID, 100}, {TESTEXA_CONTRACT_ID, 20} };
        issueContractShares(TESTEXB_CONTRACT_INDEX, sharesTestExB);
        EXPECT_EQ(356, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }));

        // burning contract shares is supposed to fail
        EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset, USER1, NULL_ID, 100), 0);
        EXPECT_EQ(356, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }));
    }

    {
        // issue non-contract asset shares
        Asset asset{ USER1, assetNameFromString("BLOB") };
        EXPECT_EQ(test.issueAssetQx(asset, 1000000, 0, 0), 1000000);
        EXPECT_EQ(1000000, numberOfShares(asset));
        EXPECT_EQ(1000000, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }));
        EXPECT_EQ(1000000, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }));

        // burn non-contract shares
        EXPECT_EQ(test.transferShareOwnershipAndPossessionQx(asset, USER1, NULL_ID, 100), 100);
        EXPECT_EQ(1000000 - 100, numberOfShares(asset));
        EXPECT_EQ(1000000 - 100, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }));
        EXPECT_EQ(1000000 - 100, numberOfShares(asset, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }));
    }
}

TEST(ContractTestEx, ShareholderProposals)
{
    ContractTestingTestEx test;
    uint16 proposalIdx = 0;

    system.epoch = 200;

    increaseEnergy(USER1, 12345678);
    increaseEnergy(USER2, 31427);
    increaseEnergy(USER3, 218000);
    increaseEnergy(USER4, 218000);
    increaseEnergy(TESTEXA_CONTRACT_ID, 987654321);
    increaseEnergy(TESTEXB_CONTRACT_ID, 19283764);

    // issue contract shares
    std::vector<std::pair<m256i, unsigned int>> sharesTestExA{
        {USER1, 356},
        {USER2, 200},
        {TESTEXB_CONTRACT_ID, 100},
        {USER3, 20}
    };
    issueContractShares(TESTEXA_CONTRACT_INDEX, sharesTestExA);

    // enable that TESTEXA accepts transfer for 0 fee
    test.setPreAcquireSharesOutput<TESTEXA>(true, 0);

    // transfer management rights of some shares to other contract to cover case of multiple asset records of single possessor
    const Asset TESTEXA_ASSET{ NULL_ID, assetNameFromString("TESTEXA") };
    EXPECT_EQ(test.transferShareManagementRightsQx(TESTEXA_ASSET, USER2, 50, TESTEXA_CONTRACT_INDEX), 50);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), 356);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 150);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { USER2, TESTEXA_CONTRACT_INDEX }, { USER2, TESTEXA_CONTRACT_INDEX }), 50);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXB_CONTRACT_ID, QX_CONTRACT_INDEX }), 100);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 20);
    EXPECT_EQ(numberOfShares(TESTEXA_ASSET, { USER4, QX_CONTRACT_INDEX }, { USER4, QX_CONTRACT_INDEX }), 0);

    // fail: 4 options not supported with Yes/No proposals
    test.setupShareholderProposalTestExA(USER2, ProposalTypes::FourOptions, false, 0, false, 0, false, 0, false);

    // fail: no right, because no shareholder
    test.setupShareholderProposalTestExA(USER4, ProposalTypes::ThreeOptions, false, 0, false, 0, false, 0, false);

    // fail: transfer not allowed
    test.setupShareholderProposalTestExA(USER2, ProposalTypes::TransferYesNo, false, 0, false, 0, false, 0, false);

    // fail: invalid value of variable
    test.setupShareholderProposalTestExA(USER2, ProposalTypes::VariableYesNo, false, 0, false, 0, true, 120, false);

    // check that no active/inactive proposals
    EXPECT_EQ(test.getShareholderProposalIndices<TESTEXA>(true).size(), 0);
    EXPECT_EQ(test.getShareholderProposalIndices<TESTEXA>(false).size(), 0);

    // success: set var3 with single-var proposal
    proposalIdx = test.setupShareholderProposalTestExA(USER2, ProposalTypes::VariableYesNo, false, 0, false, 0, true, 100, true);

    // check that no active/inactive proposals
    auto proposalIndices = test.getShareholderProposalIndices<TESTEXA>(true);
    EXPECT_TRUE(proposalIndices.size() == 1 && proposalIndices[0] == proposalIdx);
    EXPECT_EQ(test.getShareholderProposalIndices<TESTEXA>(false).size(), 0);

    // fail: try to get non-existing proposal
    auto fullProposalData = test.getShareholderProposal<TESTEXA>(proposalIdx + 1);
    EXPECT_EQ(fullProposalData.proposerPubicKey, NULL_ID);
    EXPECT_EQ((int)fullProposalData.proposal.type, 0);

    // success: get existing proposal
    fullProposalData = test.getShareholderProposal<TESTEXA>(proposalIdx);
    EXPECT_EQ(fullProposalData.proposerPubicKey, USER2);
    EXPECT_EQ((int)fullProposalData.proposal.type, (int)ProposalTypes::VariableYesNo);
    auto proposal = fullProposalData.proposal;

    // fail: try to get shareholder votes of user who is no shareholder
    auto votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER4);
    EXPECT_EQ((int)votes.proposalType, 0);

    // fail: try to get shareholder votes of non-existing proposal
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx + 1, USER1);
    EXPECT_EQ((int)votes.proposalType, 0);

    // success: get shareholder votes of user who is no shareholder
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER1);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    EXPECT_EQ((int)votes.proposalIndex, (int)proposalIdx);
    EXPECT_EQ(votes.proposalTick, proposal.tick);
    checkVoteCounts(votes, {});

    // set all votes of USER1 to option 0 with single-vote struct
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdx, proposal, 0));

    // get shareholder votes of user who is no shareholder and check that they are correct
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER1);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    checkVoteCounts(votes, { {0, 356} });

    // set 50 votes of USER2 to option 0 and 150 to option 1
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { {0, 50}, {1, 150} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    checkVoteCounts(votes, { {0, 50}, {1, 150} });

    // fail: set 51 votes of USER2 to option 1 and 150 to option 0 (more votes than shares)
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { {1, 51}, {0, 150} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    checkVoteCounts(votes, { {0, 50}, {1, 150} });

    // set 20 votes of USER2 to option 0 and 30 to option 1 (some votes unused)
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { {0, 20}, {1, 30} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    checkVoteCounts(votes, { {0, 20}, {1, 30} });

    // fail: try to get voting results of invalid proposal
    auto results = test.getShareholderVotingResults<TESTEXA>(proposalIdx + 1);
    EXPECT_EQ(results.totalVotesAuthorized, 0);

    // check voting results
    results = test.getShareholderVotingResults<TESTEXA>(proposalIdx);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ((int)results.optionCount, 2);
    EXPECT_EQ(results.optionVoteCount.get(0), 20 + 356);
    EXPECT_EQ(results.optionVoteCount.get(1), 30);
    EXPECT_EQ(results.totalVotesCasted, 20 + 356 + 30);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.getMostVotedOption(), 0);

    // set 1 vote of USER3 to option 0 and 19 to option 1
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER3, proposalIdx, proposal, { {0, 1}, {1, 19} }));

    // change votes of USER1
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdx, proposal, { {0, 300}, {1, 50} }));

    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER3);
    EXPECT_EQ((int)votes.proposalType, (int)proposal.type);
    checkVoteCounts(votes, { {0, 1}, {1, 19} });
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    checkVoteCounts(votes, { {0, 20}, {1, 30} });
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER1);
    checkVoteCounts(votes, { {0, 300}, {1, 50} });

    results = test.getShareholderVotingResults<TESTEXA>(proposalIdx);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ((int)results.optionCount, 2);
    EXPECT_EQ(results.optionVoteCount.get(0), 1 + 20 + 300);
    EXPECT_EQ(results.optionVoteCount.get(1), 19 + 30 + 50);
    EXPECT_EQ(results.totalVotesCasted, 1 + 20 + 300 + 19 + 30 + 50);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.getMostVotedOption(), 0);

    // withdraw votes of USER1 and USER3
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdx, proposal, std::vector<std::pair<sint64, uint32>>()));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER3, proposalIdx, proposal, NO_VOTE_VALUE));

    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER3);
    checkVoteCounts(votes, {});
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    checkVoteCounts(votes, { {0, 20}, {1, 30} });
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER1);
    checkVoteCounts(votes, {});

    results = test.getShareholderVotingResults<TESTEXA>(proposalIdx);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 20);
    EXPECT_EQ(results.optionVoteCount.get(1), 30);
    EXPECT_EQ(results.totalVotesCasted, 20 + 30);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.getMostVotedOption(), 1);

    // fail: try to set all votes of USER2 to invalid value with single-vote struct
    // (uses Multi-Vote internally for testing compatibility, so votes of the user are reset)
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, 4));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    checkVoteCounts(votes, {});

    // fail: try to set votes of invalid proposal index
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, 0xffff, proposal, { { 0, 111 } }));

    // fail: try to set votes of inactive proposal
    ++system.epoch;
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { { 0, 111 } }));
    --system.epoch;

    // fail: try to set votes of with wrong proposal type
    proposal.type = ProposalTypes::VariableThreeValues;
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { { 0, 111 } }));
    proposal.type = ProposalTypes::VariableYesNo;

    // fail: try to set votes of with wrong proposal tick
    ++proposal.tick;
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { { 0, 111 } }));
    --proposal.tick;

    // fail: try to set votes for USER4 who is no shareholder
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER4, proposalIdx, proposal, { { 0, 111 } }));

    // success: set votes with duplicate values in array
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { { 0, 111 }, {1, 10}, {0, 22}, {1, 3} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    checkVoteCounts(votes, { {0, 133}, {1, 13} });

    // fail: try to set votes of USER2 to invalid value with multi-vote struct
    // (votes of the user are reset)
    EXPECT_FALSE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx, proposal, { {0, 12}, {1, 23}, {2, 34} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, USER2);
    checkVoteCounts(votes, {});

    // voting of TESTEXB as shareholder of TESTEXA (originator not checked by procedure)
    // user procedure TESTEXB::setVotesInOtherContractAsShareholder
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, proposalIdx, proposal, { {0, 10}, {1, 20}, {0, 70} }));
    votes = test.getShareholderVotes<TESTEXA>(proposalIdx, TESTEXB_CONTRACT_ID);
    checkVoteCounts(votes, { {0, 80}, {1, 20} });

    results = test.getShareholderVotingResults<TESTEXA>(proposalIdx);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 80);
    EXPECT_EQ(results.optionVoteCount.get(1), 20);
    EXPECT_EQ(results.totalVotesCasted, 100);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.getMostVotedOption(), 0);

    //////////////////////////////////////////////////////
    // create new shareholder proposal in TESTEXA as shareholder TESTEXB
    TESTEXA::SetShareholderProposal_input setShareholderProposalInput2;
    setShareholderProposalInput2.proposalData.type = ProposalTypes::MultiVariablesYesNo;
    setShareholderProposalInput2.proposalData.epoch = system.epoch;
    setMemory(setShareholderProposalInput2.multiVarData, 0);

    // fails to create proposal, because multiVarData is invalid (originator not checked by procedure)
    uint16 proposalIdx2 = test.setProposalInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, setShareholderProposalInput2);
    EXPECT_EQ((int)proposalIdx2, (int)INVALID_PROPOSAL_INDEX);

    // create proposal (originator not checked by procedure)
    setShareholderProposalInput2.multiVarData.hasValueDummyStateVariable1 = true;
    setShareholderProposalInput2.multiVarData.hasValueDummyStateVariable2 = true;
    setShareholderProposalInput2.multiVarData.hasValueDummyStateVariable3 = true;
    setShareholderProposalInput2.multiVarData.optionYesValues.dummyStateVariable1 = 1;
    setShareholderProposalInput2.multiVarData.optionYesValues.dummyStateVariable2 = 2;
    setShareholderProposalInput2.multiVarData.optionYesValues.dummyStateVariable3 = 3;
    proposalIdx2 = test.setProposalInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, setShareholderProposalInput2);

    // get and check new proposal
    auto fullProposalData2 = test.getShareholderProposal<TESTEXA>(proposalIdx2);
    EXPECT_EQ(fullProposalData2.proposerPubicKey, TESTEXB_CONTRACT_ID);
    EXPECT_EQ((int)fullProposalData2.proposal.type, (int)ProposalTypes::MultiVariablesYesNo);
    auto proposal2 = fullProposalData2.proposal;
    EXPECT_EQ(fullProposalData2.multiVarData, setShareholderProposalInput2.multiVarData);

    // cast votes
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, proposalIdx2, proposal2, { {1, 90} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdx2, proposal2, { {0, 50}, {1, 260} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdx2, proposal2, { {0, 10}, {1, 160} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER3, proposalIdx2, proposal2, { {0, 1}, {1, 15} }));
    results = test.getShareholderVotingResults<TESTEXA>(proposalIdx2);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 61);
    EXPECT_EQ(results.optionVoteCount.get(1), 525);
    EXPECT_EQ(results.getAcceptedOption(), 1);
    EXPECT_EQ(results.totalVotesCasted, 61 + 525);

    // test proposal listing function (2 active, 0 inactive)
    proposalIndices = test.getShareholderProposalIndices<TESTEXA>(true);
    EXPECT_TRUE(proposalIndices.size() == 2 && proposalIndices[0] == proposalIdx && proposalIndices[1] == proposalIdx2);
    EXPECT_EQ(test.getShareholderProposalIndices<TESTEXA>(false).size(), 0);

    // test that variables are set correctly after epoch switch
    test.getStateTestExampleA()->checkVariablesSetByProposal(0, 0, 0);
    test.endEpoch();
    ++system.epoch;
    test.getStateTestExampleA()->checkVariablesSetByProposal(1, 2, 3);

    // test proposal listing function (2 inactive by USER2/TESTEXB, 0 active)
    proposalIndices = test.getShareholderProposalIndices<TESTEXA>(false);
    EXPECT_TRUE(proposalIndices.size() == 2 && proposalIndices[0] == proposalIdx && proposalIndices[1] == proposalIdx2);
    EXPECT_EQ(test.getShareholderProposalIndices<TESTEXA>(true).size(), 0);

    // Setup proposal to change variable 1
    uint16 proposalIdxA1 = test.setupShareholderProposalTestExA(USER1, ProposalTypes::VariableYesNo, true, 13);
    EXPECT_NE((int)proposalIdxA1, (int)INVALID_PROPOSAL_INDEX);
    auto proposalDataA1 = test.getShareholderProposal<TESTEXA>(proposalIdxA1);
    auto proposalA1 = proposalDataA1.proposal;
    EXPECT_EQ((int)proposalA1.type, (int)ProposalTypes::VariableYesNo);

    // Setup proposal to change variable 2 and 3
    uint16 proposalIdxA2 = test.setupShareholderProposalTestExA(USER2, ProposalTypes::MultiVariablesYesNo, false, 0, true, 4, true, 5);
    EXPECT_NE((int)proposalIdxA2, (int)INVALID_PROPOSAL_INDEX);
    auto proposalDataA2 = test.getShareholderProposal<TESTEXA>(proposalIdxA2);
    auto proposalA2 = proposalDataA2.proposal;
    EXPECT_EQ((int)proposalA2.type, (int)ProposalTypes::MultiVariablesYesNo);
    EXPECT_EQ(proposalDataA2.proposerPubicKey, USER2);
    EXPECT_EQ(proposalDataA2.multiVarData.optionYesValues.dummyStateVariable2, 4);
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdxA2, proposalA2, { {0, 3} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXA>(proposalIdxA2, USER2), { {0, 3} });

    // Overwrite proposal to change variable 2 and 3
    proposalIdxA2 = test.setupShareholderProposalTestExA(USER2, ProposalTypes::MultiVariablesYesNo, false, 0, true, 1337, true, 42);
    EXPECT_NE((int)proposalIdxA2, (int)INVALID_PROPOSAL_INDEX);
    checkVoteCounts(test.getShareholderVotes<TESTEXA>(proposalIdxA2, USER2), {});

    ///////////////////////////////////////////////////////////////
    // Proposals in TestExB

    // issue contract shares
    std::vector<std::pair<m256i, unsigned int>> sharesTestExB{
        {TESTEXA_CONTRACT_ID, 256},
        {USER2, 200},
        {USER3, 100},
        {USER4, 120}
    };
    issueContractShares(TESTEXB_CONTRACT_INDEX, sharesTestExB);
    const Asset TESTEXB_ASSET{ NULL_ID, assetNameFromString("TESTEXB") };
    EXPECT_EQ(numberOfShares(TESTEXB_ASSET, { TESTEXA_CONTRACT_ID, QX_CONTRACT_INDEX }, { TESTEXA_CONTRACT_ID, QX_CONTRACT_INDEX }), 256);
    EXPECT_EQ(numberOfShares(TESTEXB_ASSET, { USER2, QX_CONTRACT_INDEX }, { USER2, QX_CONTRACT_INDEX }), 200);
    EXPECT_EQ(numberOfShares(TESTEXB_ASSET, { USER3, QX_CONTRACT_INDEX }, { USER3, QX_CONTRACT_INDEX }), 100);
    EXPECT_EQ(numberOfShares(TESTEXB_ASSET, { USER4, QX_CONTRACT_INDEX }, { USER4, QX_CONTRACT_INDEX }), 120);
    EXPECT_EQ(numberOfShares(TESTEXB_ASSET, { USER1, QX_CONTRACT_INDEX }, { USER1, QX_CONTRACT_INDEX }), 0);

    // Create scalar variable proposal
    TESTEXB::ProposalDataT proposalB1;
    proposalB1.epoch = system.epoch;
    proposalB1.type = ProposalTypes::VariableScalarMean;
    proposalB1.variableScalar.variable = 0;
    proposalB1.variableScalar.minValue = 0;
    proposalB1.variableScalar.maxValue = MAX_AMOUNT;
    proposalB1.variableScalar.proposedValue = 1000;
    uint16 proposalIdxB1 = test.setShareholderProposal<TESTEXB>(USER2, { proposalB1 });
    EXPECT_NE((int)proposalIdxB1, (int)INVALID_PROPOSAL_INDEX);
    auto proposalDataB1 = test.getShareholderProposal<TESTEXB>(proposalIdxB1);
    proposalB1 = proposalDataB1.proposal; // needed to set tick
    EXPECT_EQ((int)proposalDataB1.proposal.type, (int)ProposalTypes::VariableScalarMean);
    EXPECT_EQ(proposalDataB1.proposerPubicKey, USER2);
    EXPECT_EQ(proposalDataB1.proposal.variableScalar.maxValue, MAX_AMOUNT);
    EXPECT_EQ(proposalDataB1.proposal.variableScalar.proposedValue, 1000);

    // Create multi-option variable proposal as shareholder TESTEXA
    TESTEXB::ProposalDataT proposalB2;
    proposalB2.epoch = system.epoch;
    proposalB2.type = ProposalTypes::VariableFourValues;
    proposalB2.variableOptions.variable = 1;
    proposalB2.variableOptions.values.set(0, 100);
    proposalB2.variableOptions.values.set(1, 1000);
    proposalB2.variableOptions.values.set(2, 10000);
    proposalB2.variableOptions.values.set(3, 100000);
    uint16 proposalIdxB2 = test.setProposalInOtherContractAsShareholder<TESTEXA>(USER1, TESTEXB_CONTRACT_INDEX, TESTEXB::SetShareholderProposal_input{ proposalB2 });
    EXPECT_NE((int)proposalIdxB2, (int)INVALID_PROPOSAL_INDEX);
    auto proposalDataB2 = test.getShareholderProposal<TESTEXB>(proposalIdxB2);
    proposalB2 = proposalDataB2.proposal; // needed to set tick
    EXPECT_EQ((int)proposalDataB2.proposal.type, (int)ProposalTypes::VariableFourValues);
    EXPECT_EQ(proposalDataB2.proposerPubicKey, TESTEXA_CONTRACT_ID);
    EXPECT_EQ(proposalDataB2.proposal.variableOptions.variable, 1);
    EXPECT_EQ(proposalDataB2.proposal.variableOptions.values.get(0), 100);
    EXPECT_EQ(proposalDataB2.proposal.variableOptions.values.get(1), 1000);
    EXPECT_EQ(proposalDataB2.proposal.variableOptions.values.get(2), 10000);
    EXPECT_EQ(proposalDataB2.proposal.variableOptions.values.get(3), 100000);

    // cast votes in A1
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdxA1, proposalA1, { {0, 60}, {1, 270} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdxA1, proposalA1, { {0, 15}, {1, 180} }));
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, proposalIdxA1, proposalA1, { {1, 80}, {0, 15} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER3, proposalIdxA1, proposalA1, { {0, 9}, {1, 11} }));
    results = test.getShareholderVotingResults<TESTEXA>(proposalIdxA1);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 99);
    EXPECT_EQ(results.optionVoteCount.get(1), 541);
    EXPECT_EQ(results.getAcceptedOption(), 1);
    EXPECT_EQ(results.totalVotesCasted, 99 + 541);

    // cast votes in A2
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdxA2, proposalA2, { {0, 150}, {1, 150} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER2, proposalIdxA2, proposalA2, { {0, 100}, {1, 100} }));
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXB>(USER4, TESTEXA_CONTRACT_INDEX, proposalIdxA2, proposalA2, { {1, 50}, {0, 50} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER3, proposalIdxA2, proposalA2, { {0, 10}, {1, 10} }));
    results = test.getShareholderVotingResults<TESTEXA>(proposalIdxA2);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 310);
    EXPECT_EQ(results.optionVoteCount.get(1), 310);
    EXPECT_EQ(results.getAcceptedOption(), 0);
    EXPECT_EQ(results.totalVotesCasted, 620);
    EXPECT_TRUE(test.setShareholderVotes<TESTEXA>(USER1, proposalIdxA2, proposalA2, { {0, 0}, {1, 350} }));
    results = test.getShareholderVotingResults<TESTEXA>(proposalIdxA2);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 160);
    EXPECT_EQ(results.optionVoteCount.get(1), 510);
    EXPECT_EQ(results.getAcceptedOption(), 1);
    EXPECT_EQ(results.totalVotesCasted, 670);

    // cast votes in B1
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXA>(USER1, TESTEXB_CONTRACT_INDEX, proposalIdxB1, proposalB1, { {0, 10}, {100, 20}, {1000, 200}, {10000, 10}, {100000, 5}, {1000000, 5}, {10000000, 2}, {100000000, 2} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXB>(proposalIdxB1, TESTEXA_CONTRACT_ID), { {0, 10}, {100, 20}, {1000, 200}, {10000, 10}, {100000, 5}, {1000000, 5}, {10000000, 2}, {100000000, 2} });
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER2, proposalIdxB1, proposalB1, { {100, 200} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXB>(proposalIdxB1, USER2), { {100, 200} });
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER3, proposalIdxB1, proposalB1, { {150, 90}, {200, 10} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXB>(proposalIdxB1, USER3), { {150, 90}, {200, 10} });
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER4, proposalIdxB1, proposalB1, { {300, 99}, {11974, 1} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXB>(proposalIdxB1, USER4), { {300, 99}, {11974, 1} });
    results = test.getShareholderVotingResults<TESTEXB>(proposalIdxB1);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ((int)results.optionCount, 0);
    EXPECT_EQ(results.scalarVotingResult, 345381);
    EXPECT_EQ(results.totalVotesCasted, 654);

    // cast votes in B2
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXA>(USER1, TESTEXB_CONTRACT_INDEX, proposalIdxB2, proposalB2, { {0, 10}, {1, 20}, {2, 30}, {3, 40} }));
    checkVoteCounts(test.getShareholderVotes<TESTEXB>(proposalIdxB2, TESTEXA_CONTRACT_ID), { {0, 10}, {1, 20}, {2, 30}, {3, 40} });
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER2, proposalIdxB2, proposalB2, { {0, 20}, {1, 30}, {2, 40}, {3, 50}, {4, 3} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER3, proposalIdxB2, proposalB2, { {0, 5}, {1, 10}, {2, 15}, {3, 20}, {4, 2} }));
    EXPECT_TRUE(test.setShareholderVotes<TESTEXB>(USER4, proposalIdxB2, proposalB2, { {0, 25}, {1, 20}, {2, 15}, {3, 10} }));
    results = test.getShareholderVotingResults<TESTEXB>(proposalIdxB2);
    EXPECT_EQ(results.totalVotesAuthorized, 676);
    EXPECT_EQ(results.optionVoteCount.get(0), 60);
    EXPECT_EQ(results.optionVoteCount.get(1), 80);
    EXPECT_EQ(results.optionVoteCount.get(2), 100);
    EXPECT_EQ(results.optionVoteCount.get(3), 120);
    EXPECT_EQ(results.optionVoteCount.get(4), 5);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.totalVotesCasted, 365);
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXA>(USER1, TESTEXB_CONTRACT_INDEX, proposalIdxB2, proposalB2, { {0, 45}, {1, 50}, {2, 55}, {3, 50}, {4, 5} }));
    results = test.getShareholderVotingResults<TESTEXB>(proposalIdxB2);
    EXPECT_EQ(results.optionVoteCount.get(0), 95);
    EXPECT_EQ(results.optionVoteCount.get(1), 110);
    EXPECT_EQ(results.optionVoteCount.get(2), 125);
    EXPECT_EQ(results.optionVoteCount.get(3), 130);
    EXPECT_EQ(results.optionVoteCount.get(4), 10);
    EXPECT_EQ(results.getAcceptedOption(), -1);
    EXPECT_EQ(results.totalVotesCasted, 470);
    EXPECT_TRUE(test.setVotesInOtherContractAsShareholder<TESTEXA>(USER1, TESTEXB_CONTRACT_INDEX, proposalIdxB2, proposalB2, { {0, 5}, {1, 5}, {2, 5}, {3, 240} }));
    results = test.getShareholderVotingResults<TESTEXB>(proposalIdxB2);
    EXPECT_EQ(results.optionVoteCount.get(0), 55);
    EXPECT_EQ(results.optionVoteCount.get(1), 65);
    EXPECT_EQ(results.optionVoteCount.get(2), 75);
    EXPECT_EQ(results.optionVoteCount.get(3), 320);
    EXPECT_EQ(results.optionVoteCount.get(4), 5);
    EXPECT_EQ(results.getAcceptedOption(), 3);
    EXPECT_EQ(results.totalVotesCasted, 520);

    // test proposal listing function in TESTEXA: 1 inactive by TESTEXB, 2 active by USER2/USER1
    proposalIndices = test.getShareholderProposalIndices<TESTEXA>(false);
    EXPECT_TRUE(proposalIndices.size() == 1 && proposalIndices[0] == proposalIdx2);
    proposalIndices = test.getShareholderProposalIndices<TESTEXA>(true);
    EXPECT_TRUE(proposalIndices.size() == 2 && proposalIndices[0] == proposalIdxA2 && proposalIndices[1] == proposalIdxA1);

    // test proposal listing function in TESTEXB: 0 inactive, 2 active by USER1/TESTEXA
    proposalIndices = test.getShareholderProposalIndices<TESTEXB>(false);
    EXPECT_TRUE(proposalIndices.size() == 0);
    proposalIndices = test.getShareholderProposalIndices<TESTEXB>(true);
    EXPECT_TRUE(proposalIndices.size() == 2 && proposalIndices[0] == proposalIdxB1 && proposalIndices[1] == proposalIdxB2);

    // test that variables are set correctly after epoch switch
    test.getStateTestExampleA()->checkVariablesSetByProposal(1, 2, 3);
    test.getStateTestExampleB()->checkVariablesSetByProposal(0, 0, 0);
    test.endEpoch();
    ++system.epoch;
    test.getStateTestExampleA()->checkVariablesSetByProposal(13, 1337, 42);
    test.getStateTestExampleB()->checkVariablesSetByProposal(345381, 10000, 0);

    // test proposal listing function in TESTEXA: 3 inactive by TESTEXB/USER2/USER1, 0 active
    EXPECT_TRUE(test.getShareholderProposalIndices<TESTEXA>(false).size() == 3);
    EXPECT_TRUE(test.getShareholderProposalIndices<TESTEXA>(true).size() == 0);

    // test proposal listing function in TESTEXB: 2 inactive by USER1/TESTEXA, 0 active
    EXPECT_TRUE(test.getShareholderProposalIndices<TESTEXB>(false).size() == 2);
    EXPECT_TRUE(test.getShareholderProposalIndices<TESTEXB>(true).size() == 0);
}

TEST(ContractTestEx, InterContractCallInsufficientFees)
{
    ContractTestingTestEx test;
    increaseEnergy(USER1, 1000000);

    // First verify call works normally (TestExampleA has fees from constructor)
    auto output1 = test.testInterContractCallError();
    EXPECT_EQ(output1.errorCode, QPI::NoCallError);
    EXPECT_EQ(output1.callSucceeded, 1);

    // Save original fee reserve
    long long originalFeeReserve = getContractFeeReserve(TESTEXA_CONTRACT_INDEX);

    // Drain TestExampleA's fee reserve
    setContractFeeReserve(TESTEXA_CONTRACT_INDEX, 0);

    // Verify fee reserve is now 0
    EXPECT_EQ(getContractFeeReserve(TESTEXA_CONTRACT_INDEX), 0);

    // Try the call again - should fail with insufficient fees
    auto output2 = test.testInterContractCallError();
    EXPECT_EQ(output2.errorCode, QPI::CallErrorInsufficientFees);
    EXPECT_EQ(output2.callSucceeded, 0);

    // Restore fee reserve for other tests
    setContractFeeReserve(TESTEXA_CONTRACT_INDEX, originalFeeReserve);
}

TEST(ContractTestEx, SystemCallbacksWithNegativeFeeReserve)
{
    ContractTestingTestEx test;

    // Set TESTEXC fee reserve to negative value
    setContractFeeReserve(TESTEXC_CONTRACT_INDEX, -1000);
    EXPECT_EQ(getContractFeeReserve(TESTEXC_CONTRACT_INDEX), -1000);

    const auto initialIncomingC = test.getIncomingTransferAmounts<TESTEXC>();
    const sint64 initialBalanceC = getBalance(TESTEXC_CONTRACT_ID);

    // Give TESTEXB balance to make the transfer
    increaseEnergy(TESTEXB_CONTRACT_ID, 10000);
    increaseEnergy(USER1, 10000);
    const sint64 transferAmount = 5000;
    EXPECT_TRUE(test.qpiTransfer<TESTEXB>(TESTEXC_CONTRACT_ID, transferAmount, 1000, USER1));

    // Verify callback executed and modified state
    const auto afterIncomingC = test.getIncomingTransferAmounts<TESTEXC>();
    EXPECT_EQ(afterIncomingC.qpiTransferAmount, initialIncomingC.qpiTransferAmount + transferAmount);
    EXPECT_EQ(getBalance(TESTEXC_CONTRACT_ID), initialBalanceC + transferAmount);

    // Verify TESTEXB not in error state
    EXPECT_EQ(contractError[TESTEXB_CONTRACT_INDEX], NoContractError);

    // Verify TESTEXC fee reserve is still negative
    EXPECT_LT(getContractFeeReserve(TESTEXC_CONTRACT_INDEX), 0);
}
