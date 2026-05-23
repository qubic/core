#define NO_UEFI

#include "contract_testing.h"

#include <set>
#include <vector>

// Generous per-user energy so contract calls never run dry.
static constexpr sint64 QPORTAL_USER_ENERGY = 5000000000ll;

static const id QPORTAL_CONTRACT_ID(QPORTAL_CONTRACT_INDEX, 0, 0, 0);

// Ordering for std::set<id, IdLess> (m256i has no operator<).
struct IdLess
{
    bool operator()(const id& a, const id& b) const
    {
        for (int i = 0; i < 4; i++)
            if (a.m256i_u64[i] != b.m256i_u64[i])
                return a.m256i_u64[i] < b.m256i_u64[i];
        return false;
    }
};

// Exposes QPORTAL internal state for assertions.
class QPORTALChecker : public QPORTAL, public QPORTAL::StateData
{
public:
    id getPortalIssuer() const { return PORTAL_Issuer; }
    uint32 getNumberOfRegisters() const { return numberOfRegisters; }
    uint32 getNumberOfProposals() const { return numberOfProposals; }
    uint32 getNumberOfCurrentEpochProposals() const { return numberOfCurrentEpochProposals; }
    bool isRegistered(const id& u) const { return registers.contains(u); }
    bool hasProposal(const id& p) const { return submittedProposals.contains(p); }

    uint8 proposalStatusOf(const id& p) const
    {
        sint64 idx = submittedProposals.getElementIndex(p);
        EXPECT_NE(idx, NULL_INDEX);
        return submittedProposals.value(idx).proposalStatus;
    }

    uint64 lockedOf(const id& u) const
    {
        return lockedAmount.contains(u)
            ? lockedAmount.value(lockedAmount.getElementIndex(u))
            : 0ull;
    }

    sint64 getBurnAmt() const { return burnAmt; }
};

class ContractTestingQPortal : protected ContractTesting
{
public:
    id portalIssuer;
    int portalOwnershipIdx = -1;
    int portalPossessionIdx = -1;

    ContractTestingQPortal()
    {
        initEmptySpectrum();
        initEmptyUniverse();

        // QX is needed as a destination managing contract for transferShareManagementRights.
        INIT_CONTRACT(QX);
        callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);

        INIT_CONTRACT(QPORTAL);
        callSystemProcedure(QPORTAL_CONTRACT_INDEX, INITIALIZE);

        // The contract is only callable from its construction epoch onwards.
        system.epoch = contractDescriptions[QPORTAL_CONTRACT_INDEX].constructionEpoch;

        portalIssuer = getState()->getPortalIssuer();

        // Default to a Thursday (outside the Mon 00:00 -> end-of-epoch freeze window)
        // so the rest of the suite is not blocked by the freeze.
        setDateTime(2025, 1, 2, 12);
    }

    // Sets the simulated UTC date/time visible to the contract via qpi.year/month/day.
    void setDateTime(uint16 year, uint8 month, uint8 day, uint8 hour)
    {
        utcTime.Year = year;
        utcTime.Month = month;
        utcTime.Day = day;
        utcTime.Hour = hour;
        utcTime.Minute = 0;
        utcTime.Second = 0;
        utcTime.Nanosecond = 0;
        updateQpiTime();
    }

    QPORTALChecker* getState()
    {
        return (QPORTALChecker*)contractStates[QPORTAL_CONTRACT_INDEX];
    }

    // ---- Asset / spectrum setup helpers -----------------------------------
    //
    // The PORTAL asset is issued directly with its management rights already on
    // QPORTAL. This deliberately bypasses QX::TransferShareManagementRights: that
    // QX path crashes in the current build (the pre-existing QIP test, which
    // exercises the same path, crashes identically). Issuing the asset already
    // managed by QPORTAL reproduces the same end state a real user reaches after
    // transferring management rights, so register / vote / refund behave normally.

    // Issues the whole PORTAL supply to the hardcoded issuer, managed by QPORTAL.
    void issuePortal(sint64 totalSupply)
    {
        char name[7] = { 'P', 'O', 'R', 'T', 'A', 'L', 0 };
        char unit[7] = { 0, 0, 0, 0, 0, 0, 0 };
        int issuanceIdx = -1;
        EXPECT_EQ(issueAsset(portalIssuer, name, 0, unit, totalSupply,
                             (unsigned short)QPORTAL_CONTRACT_INDEX,
                             &issuanceIdx, &portalOwnershipIdx, &portalPossessionIdx),
                  totalSupply);
    }

    // Gives `user` `amount` PORTAL shares (already managed by QPORTAL) plus energy.
    void fundUser(const id& user, sint64 amount)
    {
        increaseEnergy(user, QPORTAL_USER_ENERGY);
        int dstO = -1, dstP = -1;
        EXPECT_TRUE(transferShareOwnershipAndPossession(
            portalOwnershipIdx, portalPossessionIdx, user, amount, &dstO, &dstP, true));
    }

    sint64 portalBalanceOnQPortal(const id& owner)
    {
        return numberOfPossessedShares(
            QPORTAL_PORTAL_ASSET_NAME, portalIssuer, owner, owner,
            QPORTAL_CONTRACT_INDEX, QPORTAL_CONTRACT_INDEX);
    }

    // ---- QPORTAL procedures -----------------------------------------------

    sint32 registerInPortalDAO(const id& user)
    {
        QPORTAL::registerInPortalDAO_input input;
        QPORTAL::registerInPortalDAO_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 1, input, output, user, 0);
        return output.returnCode;
    }

    sint32 submitProposal(const id& user, const id& proposalId)
    {
        QPORTAL::submitProposal_input input;
        input.proposalId = proposalId;
        QPORTAL::submitProposal_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 2, input, output, user, 0);
        return output.returnCode;
    }

    sint32 submitInVote(const id& user, const id& proposalId, int vote, uint64 amount)
    {
        QPORTAL::submitInVote_input input;
        input.proposalId = proposalId;
        input.vote = (vote != 0);
        input.votingPortalAmount = amount;
        QPORTAL::submitInVote_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 3, input, output, user, 0);
        return output.returnCode;
    }

    sint32 submitOutVote(const id& user, const id& proposalId)
    {
        QPORTAL::submitOutVote_input input;
        input.proposalId = proposalId;
        QPORTAL::submitOutVote_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 4, input, output, user, 0);
        return output.returnCode;
    }

    sint32 requestRefund(const id& user, uint64 amount)
    {
        QPORTAL::requestRefund_input input;
        input.amount = amount;
        QPORTAL::requestRefund_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 5, input, output, user, 0);
        return output.returnCode;
    }

    QPORTAL::transferShareManagementRights_output transferShareMgmtRights(
        const id& user, uint64 numberOfShares, uint32 newMgmtIdx, sint64 invocationReward)
    {
        QPORTAL::transferShareManagementRights_input input;
        input.asset.assetName = QPORTAL_PORTAL_ASSET_NAME;
        input.asset.issuer = portalIssuer;
        input.numberOfShares = numberOfShares;
        input.newManagementContractIndex = newMgmtIdx;
        QPORTAL::transferShareManagementRights_output output;
        invokeUserProcedure(QPORTAL_CONTRACT_INDEX, 6, input, output, user, invocationReward);
        return output;
    }

    sint64 portalBalanceOnQX(const id& owner)
    {
        return numberOfPossessedShares(
            QPORTAL_PORTAL_ASSET_NAME, portalIssuer, owner, owner,
            QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
    }

    // ---- QPORTAL functions ------------------------------------------------

    QPORTAL::getRegisters_output getRegisters(uint32 offset, uint32 limit)
    {
        QPORTAL::getRegisters_input input;
        input.offset = offset;
        input.limit = limit;
        QPORTAL::getRegisters_output output;
        callFunction(QPORTAL_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    // Collects the non-null member ids returned by getRegisters into a vector.
    std::vector<id> getRegistersVec(uint32 offset, uint32 limit)
    {
        QPORTAL::getRegisters_output o = getRegisters(offset, limit);
        EXPECT_EQ(o.returnCode, (sint32)QPORTAL_SUCCESS);
        std::vector<id> result;
        const id slots[10] = { o.register1, o.register2, o.register3, o.register4, o.register5,
                               o.register6, o.register7, o.register8, o.register9, o.register10 };
        for (int i = 0; i < 10; i++)
            if (slots[i] != NULL_ID)
                result.push_back(slots[i]);
        return result;
    }

    uint32 getCurrentNumberOfRegisters()
    {
        QPORTAL::getCurrentNumberOfRegisters_input input;
        QPORTAL::getCurrentNumberOfRegisters_output output;
        callFunction(QPORTAL_CONTRACT_INDEX, 2, input, output);
        EXPECT_EQ(output.returnCode, (sint32)QPORTAL_SUCCESS);
        return output.numberOfRegisters;
    }

    QPORTAL::getCurrentEpochProposals_output getCurrentEpochProposals()
    {
        QPORTAL::getCurrentEpochProposals_input input;
        QPORTAL::getCurrentEpochProposals_output output;
        callFunction(QPORTAL_CONTRACT_INDEX, 3, input, output);
        return output;
    }

    QPORTAL::getProposalStatus_output getProposalStatus(const id& proposalId)
    {
        QPORTAL::getProposalStatus_input input;
        input.proposalId = proposalId;
        QPORTAL::getProposalStatus_output output;
        callFunction(QPORTAL_CONTRACT_INDEX, 4, input, output);
        return output;
    }

    QPORTAL::getProposalInfo_output getProposalInfo(const id& proposalId)
    {
        QPORTAL::getProposalInfo_input input;
        input.proposalId = proposalId;
        QPORTAL::getProposalInfo_output output;
        callFunction(QPORTAL_CONTRACT_INDEX, 6, input, output);
        return output;
    }

    void endEpoch()
    {
        callSystemProcedure(QPORTAL_CONTRACT_INDEX, END_EPOCH);
    }
};

// Deterministic, well-scattered distinct ids for test members / proposals.
static id qpMember(int n)
{
    return id(n * 0x9E3779B97F4A7C15ull + 1, n * 7ull + 3, n * 13ull + 5, n * 17ull + 9);
}
static id qpProposal(int n)
{
    return id(n * 0xC2B2AE3D27D4EB4Full + 7, n * 11ull + 1, n * 19ull + 2, n * 23ull + 4);
}

// ===========================================================================
// Registration
// ===========================================================================

TEST(ContractQPortal, Register_Success)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    id user = qpMember(1);
    qp.fundUser(user, 1000);

    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    EXPECT_TRUE(qp.getState()->isRegistered(user));
    EXPECT_EQ(qp.getState()->getNumberOfRegisters(), 1u);

    // Register fee (5 PORTAL) moved from the user to the contract.
    EXPECT_EQ(qp.portalBalanceOnQPortal(user), 1000 - (sint64)QPORTAL_REGISTER_FEE);
    EXPECT_EQ(qp.portalBalanceOnQPortal(QPORTAL_CONTRACT_ID), (sint64)QPORTAL_REGISTER_FEE);
}

TEST(ContractQPortal, Register_AlreadyRegistered)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    id user = qpMember(1);
    qp.fundUser(user, 1000);

    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_ALREADY_REGISTERED);
    EXPECT_EQ(qp.getState()->getNumberOfRegisters(), 1u);
}

TEST(ContractQPortal, Register_InsufficientPortal)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    id user = qpMember(1);
    qp.fundUser(user, QPORTAL_REGISTER_FEE - 1); // below the register fee

    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_INSUFFICIENT_PORTAL);
    EXPECT_FALSE(qp.getState()->isRegistered(user));
    EXPECT_EQ(qp.getState()->getNumberOfRegisters(), 0u);
}

TEST(ContractQPortal, Register_NoPortalShares)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    // User has a spectrum entry but holds no PORTAL shares.
    id user = qpMember(1);
    increaseEnergy(user, QPORTAL_USER_ENERGY);

    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_INSUFFICIENT_PORTAL);
    EXPECT_FALSE(qp.getState()->isRegistered(user));
}

TEST(ContractQPortal, Register_MultipleMembers)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    for (int i = 0; i < 6; i++)
    {
        id user = qpMember(i);
        qp.fundUser(user, 100);
        EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    }
    EXPECT_EQ(qp.getState()->getNumberOfRegisters(), 6u);
    EXPECT_EQ(qp.getCurrentNumberOfRegisters(), 6u);
}

// ===========================================================================
// getRegisters
// ===========================================================================

TEST(ContractQPortal, GetRegisters_InvalidLimit)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.getRegisters(0, 0).returnCode, (sint32)QPORTAL_INVALID_OFFSET_OR_LIMIT);
    EXPECT_EQ(qp.getRegisters(0, 11).returnCode, (sint32)QPORTAL_INVALID_OFFSET_OR_LIMIT);
}

TEST(ContractQPortal, GetRegisters_OffsetOutOfRange)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    // offset == numberOfRegisters is out of range
    EXPECT_EQ(qp.getRegisters(1, 5).returnCode, (sint32)QPORTAL_INVALID_OFFSET_OR_LIMIT);
    EXPECT_EQ(qp.getRegisters(5, 5).returnCode, (sint32)QPORTAL_INVALID_OFFSET_OR_LIMIT);
}

TEST(ContractQPortal, GetRegisters_ReturnsAllMembers)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    std::set<id, IdLess> registered;
    for (int i = 0; i < 7; i++)
    {
        id user = qpMember(i);
        qp.fundUser(user, 100);
        EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
        registered.insert(user);
    }

    std::vector<id> page = qp.getRegistersVec(0, 10);
    EXPECT_EQ(page.size(), 7u);
    std::set<id, IdLess> returned(page.begin(), page.end());
    EXPECT_EQ(returned.size(), 7u);        // no duplicates
    EXPECT_TRUE(returned == registered);   // exactly the registered set
}

TEST(ContractQPortal, GetRegisters_Pagination)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);

    std::set<id, IdLess> registered;
    const int total = 23;
    for (int i = 0; i < total; i++)
    {
        id user = qpMember(i);
        qp.fundUser(user, 100);
        EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
        registered.insert(user);
    }
    EXPECT_EQ(qp.getCurrentNumberOfRegisters(), (uint32)total);

    // Walk every page of 10 and verify the union is exactly the member set,
    // with no overlap between pages.
    std::set<id, IdLess> seen;
    for (uint32 offset = 0; offset < (uint32)total; offset += 10)
    {
        std::vector<id> page = qp.getRegistersVec(offset, 10);
        uint32 expectedCount = ((uint32)total - offset) < 10 ? ((uint32)total - offset) : 10;
        EXPECT_EQ(page.size(), expectedCount);
        for (const id& m : page)
        {
            EXPECT_TRUE(registered.count(m) == 1); // belongs to the member set
            EXPECT_TRUE(seen.insert(m).second);    // not seen on a previous page
        }
    }
    EXPECT_TRUE(seen == registered);
}

TEST(ContractQPortal, GetRegisters_LimitCapsResult)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    for (int i = 0; i < 8; i++)
    {
        id user = qpMember(i);
        qp.fundUser(user, 100);
        EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    }

    EXPECT_EQ(qp.getRegistersVec(0, 3).size(), 3u);   // limited by limit
    EXPECT_EQ(qp.getRegistersVec(6, 10).size(), 2u);  // limited by remaining
    EXPECT_EQ(qp.getRegistersVec(7, 10).size(), 1u);
}

// ===========================================================================
// submitProposal
// ===========================================================================

TEST(ContractQPortal, SubmitProposal_Success)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(user, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_TRUE(qp.getState()->hasProposal(proposal));
    EXPECT_EQ(qp.getState()->getNumberOfProposals(), 1u);
    EXPECT_EQ(qp.getState()->getNumberOfCurrentEpochProposals(), 1u);
    EXPECT_EQ(qp.getProposalStatus(proposal).status, 0u); // voting
}

TEST(ContractQPortal, SubmitProposal_NotRegistered)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    increaseEnergy(user, QPORTAL_USER_ENERGY);

    EXPECT_EQ(qp.submitProposal(user, qpProposal(1)), (sint32)QPORTAL_NOT_REGISTERED);
}

TEST(ContractQPortal, SubmitProposal_InvalidInput)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitProposal(user, NULL_ID), (sint32)QPORTAL_INVALID_INPUT);
}

TEST(ContractQPortal, SubmitProposal_AlreadyExists)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id userA = qpMember(1), userB = qpMember(2);
    qp.fundUser(userA, 100);
    qp.fundUser(userB, 100);
    EXPECT_EQ(qp.registerInPortalDAO(userA), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(userB), (sint32)QPORTAL_SUCCESS);

    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(userA, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(userB, proposal), (sint32)QPORTAL_ALREADY_EXISTED_PROPOSAL);
}

TEST(ContractQPortal, SubmitProposal_MaxPerUser)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitProposal(user, qpProposal(1)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, qpProposal(2)), (sint32)QPORTAL_SUCCESS);
    // QPORTAL_MAX_PROPOSAL_USER == 2
    EXPECT_EQ(qp.submitProposal(user, qpProposal(3)), (sint32)QPORTAL_REACHED_PROPOSAL);
    EXPECT_EQ(qp.getState()->getNumberOfProposals(), 2u);
}

TEST(ContractQPortal, SubmitProposal_MaxPerEpoch)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);

    // QPORTAL_MAX_PROPOSAL_EPOCH == 5; with 2 proposals/user we need 3 members.
    for (int i = 0; i < 3; i++)
    {
        qp.fundUser(qpMember(i), 100);
        EXPECT_EQ(qp.registerInPortalDAO(qpMember(i)), (sint32)QPORTAL_SUCCESS);
    }
    EXPECT_EQ(qp.submitProposal(qpMember(0), qpProposal(1)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(qpMember(0), qpProposal(2)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(qpMember(1), qpProposal(3)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(qpMember(1), qpProposal(4)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(qpMember(2), qpProposal(5)), (sint32)QPORTAL_SUCCESS);
    // 6th proposal of the epoch is rejected
    EXPECT_EQ(qp.submitProposal(qpMember(2), qpProposal(6)), (sint32)QPORTAL_REACHED_PROPOSAL);
    EXPECT_EQ(qp.getState()->getNumberOfCurrentEpochProposals(), 5u);
}

TEST(ContractQPortal, GetCurrentEpochProposals_ReflectsSubmissions)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);

    id p1 = qpProposal(1), p2 = qpProposal(2);
    EXPECT_EQ(qp.submitProposal(user, p1), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, p2), (sint32)QPORTAL_SUCCESS);

    QPORTAL::getCurrentEpochProposals_output o = qp.getCurrentEpochProposals();
    EXPECT_EQ(o.returnCode, (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(o.proposal1, p1);
    EXPECT_EQ(o.proposal2, p2);
    EXPECT_EQ(o.proposal3, NULL_ID);
}

// ===========================================================================
// submitInVote
// ===========================================================================

TEST(ContractQPortal, SubmitInVote_SuccessYes)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);

    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 30000), (sint32)QPORTAL_SUCCESS);

    // Voting PORTAL is locked and transferred to the contract.
    EXPECT_EQ(qp.getState()->lockedOf(voter), 30000ull);
    EXPECT_EQ(qp.portalBalanceOnQPortal(voter), 50000 - (sint64)QPORTAL_REGISTER_FEE - 30000);

    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.returnCode, (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(info.yesVotes, 1u);
    EXPECT_EQ(info.noVotes, 0u);
    EXPECT_EQ(info.yesPortal, 30000ull);
    EXPECT_EQ(info.noPortal, 0ull);
    EXPECT_EQ(info.proposer, proposer);
}

TEST(ContractQPortal, SubmitInVote_SuccessNo)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 12345), (sint32)QPORTAL_SUCCESS);

    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.yesVotes, 0u);
    EXPECT_EQ(info.noVotes, 1u);
    EXPECT_EQ(info.yesPortal, 0ull);
    EXPECT_EQ(info.noPortal, 12345ull);
}

TEST(ContractQPortal, SubmitInVote_NotRegistered)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_NOT_REGISTERED);
}

TEST(ContractQPortal, SubmitInVote_InvalidInput)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, NULL_ID, 1, 1000), (sint32)QPORTAL_INVALID_INPUT);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 0), (sint32)QPORTAL_INVALID_INPUT);
}

TEST(ContractQPortal, SubmitInVote_ProposalNotExist)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id voter = qpMember(2);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, qpProposal(99), 1, 1000), (sint32)QPORTAL_NOT_EXISTED_PROPOSAL);
}

TEST(ContractQPortal, SubmitInVote_AlreadyVoted)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_ALREADY_VOTED_PROPOSAL);
    // Only the first vote locked PORTAL.
    EXPECT_EQ(qp.getState()->lockedOf(voter), 1000ull);
}

TEST(ContractQPortal, SubmitInVote_InsufficientPortal)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 1000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    // voter only has 1000 - 5 = 995 PORTAL left after registration
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 5000), (sint32)QPORTAL_INSUFFICIENT_PORTAL);
    EXPECT_EQ(qp.getState()->lockedOf(voter), 0ull);
}

TEST(ContractQPortal, SubmitInVote_MultipleVotersTally)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    qp.fundUser(proposer, 100);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    for (int i = 1; i <= 3; i++)
    {
        id v = qpMember(i);
        qp.fundUser(v, 100000);
        EXPECT_EQ(qp.registerInPortalDAO(v), (sint32)QPORTAL_SUCCESS);
        EXPECT_EQ(qp.submitInVote(v, proposal, 1, 10000), (sint32)QPORTAL_SUCCESS);
    }
    id vno = qpMember(4);
    qp.fundUser(vno, 100000);
    EXPECT_EQ(qp.registerInPortalDAO(vno), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(vno, proposal, 0, 7000), (sint32)QPORTAL_SUCCESS);

    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.yesVotes, 3u);
    EXPECT_EQ(info.noVotes, 1u);
    EXPECT_EQ(info.yesPortal, 30000ull);
    EXPECT_EQ(info.noPortal, 7000ull);
}

// ===========================================================================
// submitOutVote
// ===========================================================================

TEST(ContractQPortal, SubmitOutVote_Success)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 8000), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_SUCCESS);

    // The vote is removed from the tally.
    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.yesVotes, 0u);
    EXPECT_EQ(info.yesPortal, 0ull);
}

TEST(ContractQPortal, SubmitOutVote_NotVoted)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_NOT_VOTED_PROPOSAL);
}

TEST(ContractQPortal, SubmitOutVote_ProposalNotExist)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id voter = qpMember(2);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitOutVote(voter, qpProposal(99)), (sint32)QPORTAL_NOT_EXISTED_PROPOSAL);
}

TEST(ContractQPortal, SubmitOutVote_ThenRevoteAllowed)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 5000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_SUCCESS);
    // Re-voting after withdrawing is allowed.
    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 3000), (sint32)QPORTAL_SUCCESS);

    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.noVotes, 1u);
    EXPECT_EQ(info.noPortal, 3000ull);
}

// ===========================================================================
// requestRefund
// ===========================================================================

TEST(ContractQPortal, RequestRefund_BlockedWhileVoteActive)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 10000), (sint32)QPORTAL_SUCCESS);

    // The locked PORTAL backs an active vote, so a refund is blocked.
    EXPECT_EQ(qp.requestRefund(voter, 5000), (sint32)QPORTAL_EXISTED_PROPOSAL);
}

TEST(ContractQPortal, RequestRefund_InvalidAmount)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id voter = qpMember(2);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);

    // amount must be strictly greater than the refund fee
    EXPECT_EQ(qp.requestRefund(voter, QPORTAL_REFUND_FEE), (sint32)QPORTAL_INVALID_INPUT);
}

TEST(ContractQPortal, RequestRefund_InsufficientLocked)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id voter = qpMember(2);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);

    // Nothing locked yet.
    EXPECT_EQ(qp.requestRefund(voter, 1000), (sint32)QPORTAL_INSUFFICIENT_PORTAL);
}

TEST(ContractQPortal, RequestRefund_SuccessAfterOutVote)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 10000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_SUCCESS);

    sint64 before = qp.portalBalanceOnQPortal(voter);
    EXPECT_EQ(qp.requestRefund(voter, 10000), (sint32)QPORTAL_SUCCESS);

    // Refund returns amount - fee; lockedAmount is fully cleared.
    EXPECT_EQ(qp.portalBalanceOnQPortal(voter), before + 10000 - (sint64)QPORTAL_REFUND_FEE);
    EXPECT_EQ(qp.getState()->lockedOf(voter), 0ull);
}

TEST(ContractQPortal, RequestRefund_SuccessAfterEpoch)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(1), voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 20000), (sint32)QPORTAL_SUCCESS);

    // After the epoch ends, votes are cleared and the lock can be withdrawn.
    qp.endEpoch();
    EXPECT_EQ(qp.getState()->lockedOf(voter), 20000ull);

    sint64 before = qp.portalBalanceOnQPortal(voter);
    EXPECT_EQ(qp.requestRefund(voter, 20000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.portalBalanceOnQPortal(voter), before + 20000 - (sint64)QPORTAL_REFUND_FEE);
    EXPECT_EQ(qp.getState()->lockedOf(voter), 0ull);
}

// ===========================================================================
// END_EPOCH resolution
// ===========================================================================

// Helper: register a proposer and submit one proposal.
static void setupProposal(ContractTestingQPortal& qp, const id& proposer, const id& proposal)
{
    qp.fundUser(proposer, 100);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
}

TEST(ContractQPortal, EndEpoch_ProposalAccepted)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    id proposal = qpProposal(1);
    setupProposal(qp, proposer, proposal);

    // yesVotes > noVotes, yesPortal > noPortal, total >= QPORTAL_MIN_HOLDING_PORTAL (100000)
    id v1 = qpMember(1), v2 = qpMember(2);
    qp.fundUser(v1, 200000);
    qp.fundUser(v2, 200000);
    EXPECT_EQ(qp.registerInPortalDAO(v1), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(v2), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(v1, proposal, 1, 70000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(v2, proposal, 1, 40000), (sint32)QPORTAL_SUCCESS);

    qp.endEpoch();

    EXPECT_EQ(qp.getState()->proposalStatusOf(proposal), 1); // accepted
    EXPECT_EQ(qp.getProposalStatus(proposal).status, 1u);
}

TEST(ContractQPortal, EndEpoch_ProposalRejectedLowHolding)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    id proposal = qpProposal(1);
    setupProposal(qp, proposer, proposal);

    // Total voting PORTAL below QPORTAL_MIN_HOLDING_PORTAL (100000).
    id v1 = qpMember(1);
    qp.fundUser(v1, 200000);
    EXPECT_EQ(qp.registerInPortalDAO(v1), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(v1, proposal, 1, 30000), (sint32)QPORTAL_SUCCESS);

    qp.endEpoch();
    EXPECT_EQ(qp.getState()->proposalStatusOf(proposal), 2); // rejected
}

TEST(ContractQPortal, EndEpoch_ProposalRejectedNoVotes)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    id proposal = qpProposal(1);
    setupProposal(qp, proposer, proposal);

    qp.endEpoch();
    EXPECT_EQ(qp.getState()->proposalStatusOf(proposal), 2); // rejected
}

TEST(ContractQPortal, EndEpoch_ProposalRejectedNoMajority)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    id proposal = qpProposal(1);
    setupProposal(qp, proposer, proposal);

    // total >= 100000 but noPortal > yesPortal
    id v1 = qpMember(1), v2 = qpMember(2);
    qp.fundUser(v1, 200000);
    qp.fundUser(v2, 200000);
    EXPECT_EQ(qp.registerInPortalDAO(v1), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(v2), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(v1, proposal, 1, 40000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(v2, proposal, 0, 70000), (sint32)QPORTAL_SUCCESS);

    qp.endEpoch();
    EXPECT_EQ(qp.getState()->proposalStatusOf(proposal), 2); // rejected
}

TEST(ContractQPortal, EndEpoch_ResetsEpochState)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id user = qpMember(0);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, qpProposal(1)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, qpProposal(2)), (sint32)QPORTAL_SUCCESS);

    qp.endEpoch();

    // Current-epoch counters reset; lifetime proposal count preserved.
    EXPECT_EQ(qp.getState()->getNumberOfCurrentEpochProposals(), 0u);
    EXPECT_EQ(qp.getState()->getNumberOfProposals(), 2u);

    // currentEpochProposals array is cleared.
    QPORTAL::getCurrentEpochProposals_output o = qp.getCurrentEpochProposals();
    EXPECT_EQ(o.proposal1, NULL_ID);
    EXPECT_EQ(o.proposal2, NULL_ID);

    // userProposalStatus reset: the user may submit the per-user max again.
    EXPECT_EQ(qp.submitProposal(user, qpProposal(3)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, qpProposal(4)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitProposal(user, qpProposal(5)), (sint32)QPORTAL_REACHED_PROPOSAL);
}

TEST(ContractQPortal, EndEpoch_ClosedProposalRejectsVotes)
{
    ContractTestingQPortal qp;
    qp.issuePortal(10000000);
    id proposer = qpMember(0);
    id proposal = qpProposal(1);
    setupProposal(qp, proposer, proposal);
    qp.endEpoch(); // proposal resolved (rejected, no votes)

    id voter = qpMember(1);
    qp.fundUser(voter, 50000);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    // Voting on a resolved proposal is rejected.
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_CLOSED_PROPOSAL);
}

// ===========================================================================
// getProposalInfo / getProposalStatus edge cases
// ===========================================================================

TEST(ContractQPortal, GetProposalInfo_NotExist)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    EXPECT_EQ(qp.getProposalInfo(qpProposal(123)).returnCode, (sint32)QPORTAL_NOT_EXISTED_PROPOSAL);
}

TEST(ContractQPortal, GetProposalInfo_VotelessProposalHasZeroCounts)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(0);
    qp.fundUser(user, 100);
    EXPECT_EQ(qp.registerInPortalDAO(user), (sint32)QPORTAL_SUCCESS);
    id proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(user, proposal), (sint32)QPORTAL_SUCCESS);

    // A submitted-but-not-yet-voted proposal must report zeroed counts, not garbage.
    QPORTAL::getProposalInfo_output info = qp.getProposalInfo(proposal);
    EXPECT_EQ(info.returnCode, (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(info.proposer, user);
    EXPECT_EQ(info.status, 0u);
    EXPECT_EQ(info.yesVotes, 0u);
    EXPECT_EQ(info.noVotes, 0u);
    EXPECT_EQ(info.yesPortal, 0ull);
    EXPECT_EQ(info.noPortal, 0ull);
}

TEST(ContractQPortal, GetProposalStatus_InvalidAndMissing)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    EXPECT_EQ(qp.getProposalStatus(NULL_ID).returnCode, (sint32)QPORTAL_INVALID_INPUT);
    EXPECT_EQ(qp.getProposalStatus(qpProposal(7)).returnCode, (sint32)QPORTAL_NOT_EXISTED_PROPOSAL);
}

// ===========================================================================
// transferShareManagementRights
// ===========================================================================

TEST(ContractQPortal, TransferShareMgmtRights_InsufficientReward)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 1000);

    // invocationReward below QPORTAL_EXECUTION_FEE (100) is rejected.
    QPORTAL::transferShareManagementRights_output o =
        qp.transferShareMgmtRights(user, 500, QX_CONTRACT_INDEX, QPORTAL_EXECUTION_FEE - 1);
    EXPECT_EQ(o.returnCode, (sint32)QPORTAL_INSUFFICIENT_PORTAL);
    // Management is unchanged: all shares still on QPORTAL.
    EXPECT_EQ(qp.portalBalanceOnQPortal(user), 1000);
}

TEST(ContractQPortal, TransferShareMgmtRights_InsufficientShares)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 50);

    // Requesting more shares than the user holds is rejected.
    QPORTAL::transferShareManagementRights_output o =
        qp.transferShareMgmtRights(user, 1000, QX_CONTRACT_INDEX, QPORTAL_EXECUTION_FEE);
    EXPECT_EQ(o.returnCode, (sint32)QPORTAL_INSUFFICIENT_PORTAL);
    EXPECT_EQ(o.transferredNumberOfShares, 0ll);
    EXPECT_EQ(qp.portalBalanceOnQPortal(user), 50);
}

TEST(ContractQPortal, TransferShareMgmtRights_SelfDestinationRejected)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 1000);

    // The contract's input-validation guard rejects destination == SELF.
    QPORTAL::transferShareManagementRights_output o =
        qp.transferShareMgmtRights(user, 500, QPORTAL_CONTRACT_INDEX, 5000000);
    EXPECT_EQ(o.returnCode, (sint32)QPORTAL_INVALID_INPUT);
    EXPECT_EQ(qp.portalBalanceOnQPortal(user), 1000);
}

// ===========================================================================
// Freeze window: Monday 00:00 UTC -> Wednesday 12:00 UTC
//
// One test per row of the required-tests matrix:
//   Sunday 23:59         -> proposal/vote/cancel allowed
//   Monday 00:00         -> QPORTAL_EPOCH_FROZEN
//   Monday 12:00         -> QPORTAL_EPOCH_FROZEN
//   Tuesday 23:59        -> QPORTAL_EPOCH_FROZEN
//   Wednesday 11:59      -> QPORTAL_EPOCH_FROZEN
//   Wednesday 12:00      -> proposal/vote/cancel allowed for new epoch
//   During freeze, requestRefund() with active vote -> rejected
//   After END_EPOCH, requestRefund() -> allowed
//
// Reference calendar: 2026-05-17 Sun, -18 Mon, -19 Tue, -20 Wed.
// setDateTime() has hour granularity, so 23:59 is exercised with hour=23.
// ===========================================================================

// Helper: build a fresh fixture with a registered proposer/voter and one
// proposal, the voter holding `voterFund` PORTAL ready to vote. All setup is
// performed on the fixture's default Thursday (outside the freeze window).
static void freezeSetup(ContractTestingQPortal& qp, id& proposer, id& voter, id& proposal,
                        sint64 voterFund = 50000)
{
    qp.issuePortal(10000000);
    proposer = qpMember(1);
    voter = qpMember(2);
    qp.fundUser(proposer, 100);
    qp.fundUser(voter, voterFund);
    EXPECT_EQ(qp.registerInPortalDAO(proposer), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.registerInPortalDAO(voter), (sint32)QPORTAL_SUCCESS);
    proposal = qpProposal(1);
    EXPECT_EQ(qp.submitProposal(proposer, proposal), (sint32)QPORTAL_SUCCESS);
}

// Row 1: Sunday 23:59 -> proposal/vote/cancel allowed.
TEST(ContractQPortal, Freeze_Allowed_Sunday_23_59)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);

    qp.setDateTime(2026, 5, 17, 23); // Sunday 23:xx UTC
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_SUCCESS);
}

// Row 2: Monday 00:00 -> QPORTAL_EPOCH_FROZEN.
TEST(ContractQPortal, Freeze_Frozen_Monday_00_00)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);

    qp.setDateTime(2026, 5, 18, 0); // Monday 00:00 UTC
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 500), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_EPOCH_FROZEN);
}

// Row 3: Monday 12:00 -> QPORTAL_EPOCH_FROZEN.
TEST(ContractQPortal, Freeze_Frozen_Monday_12_00)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);

    qp.setDateTime(2026, 5, 18, 12); // Monday 12:00 UTC
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 500), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_EPOCH_FROZEN);
}

// Row 4: Tuesday 23:59 -> QPORTAL_EPOCH_FROZEN.
TEST(ContractQPortal, Freeze_Frozen_Tuesday_23_59)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);

    qp.setDateTime(2026, 5, 19, 23); // Tuesday 23:xx UTC
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 500), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_EPOCH_FROZEN);
}

// Row 5: Wednesday 11:59 -> QPORTAL_EPOCH_FROZEN (one hour before thaw).
TEST(ContractQPortal, Freeze_Frozen_Wednesday_11_59)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);

    qp.setDateTime(2026, 5, 20, 11); // Wednesday 11:xx UTC
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 0, 500), (sint32)QPORTAL_EPOCH_FROZEN);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_EPOCH_FROZEN);
}

// Row 6: Wednesday 12:00 -> proposal/vote/cancel allowed for the new epoch.
TEST(ContractQPortal, Freeze_Allowed_Wednesday_12_00)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);

    qp.setDateTime(2026, 5, 20, 12); // Wednesday 12:00 UTC -- exact thaw boundary
    EXPECT_EQ(qp.submitProposal(proposer, qpProposal(2)), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 1000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.submitOutVote(voter, proposal), (sint32)QPORTAL_SUCCESS);
}

// Row 7: during the freeze, requestRefund() for an active vote is rejected.
TEST(ContractQPortal, Freeze_RefundRejectedDuringFreezeWithActiveVote)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);

    // Vote on the default Thursday; the vote is active in proposalVotes.
    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 10000), (sint32)QPORTAL_SUCCESS);

    // Move into the freeze window (Tuesday) and request a refund.
    qp.setDateTime(2026, 5, 19, 12);
    // The refund check sees the still-active vote and rejects.
    EXPECT_EQ(qp.requestRefund(voter, 5000), (sint32)QPORTAL_EXISTED_PROPOSAL);
    EXPECT_EQ(qp.getState()->lockedOf(voter), 10000ull); // still locked
}

// Row 8: after END_EPOCH, requestRefund() is allowed.
TEST(ContractQPortal, Freeze_RefundAllowedAfterEndEpoch)
{
    ContractTestingQPortal qp;
    id proposer, voter, proposal;
    freezeSetup(qp, proposer, voter, proposal);

    EXPECT_EQ(qp.submitInVote(voter, proposal, 1, 10000), (sint32)QPORTAL_SUCCESS);

    // END_EPOCH clears proposalVotes; lockedAmount persists.
    qp.endEpoch();
    EXPECT_EQ(qp.getState()->lockedOf(voter), 10000ull);

    // After the epoch transition the refund goes through.
    sint64 before = qp.portalBalanceOnQPortal(voter);
    EXPECT_EQ(qp.requestRefund(voter, 10000), (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(qp.portalBalanceOnQPortal(voter), before + 10000 - (sint64)QPORTAL_REFUND_FEE);
    EXPECT_EQ(qp.getState()->lockedOf(voter), 0ull);
}

TEST(ContractQPortal, TransferShareMgmtRights_SuccessToQX)
{
    ContractTestingQPortal qp;
    qp.issuePortal(1000000);
    id user = qpMember(1);
    qp.fundUser(user, 1000);

    // Move management of 600 shares from QPORTAL to QX (a contract that accepts).
    QPORTAL::transferShareManagementRights_output o =
        qp.transferShareMgmtRights(user, 600, QX_CONTRACT_INDEX, 5000000);
    EXPECT_EQ(o.returnCode, (sint32)QPORTAL_SUCCESS);
    EXPECT_EQ(o.transferredNumberOfShares, 600ll);

    // 600 shares are now managed by QX, 400 remain on QPORTAL.
    EXPECT_EQ(qp.portalBalanceOnQX(user), 600);
    EXPECT_EQ(qp.portalBalanceOnQPortal(user), 400);

    // The transfer fee is accumulated for burning.
    EXPECT_EQ(qp.getState()->getBurnAmt(), (sint64)QPORTAL_EXECUTION_FEE);
}
