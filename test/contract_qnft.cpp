#define NO_UEFI
#include "private_settings.h"
#undef LOG_CONTRACT_ERROR_MESSAGES
#define LOG_CONTRACT_ERROR_MESSAGES 1
#undef LOG_CONTRACT_INFO_MESSAGES
#ifdef QNFT_TEST_DISABLE_INFO_LOG
#define LOG_CONTRACT_INFO_MESSAGES 0
static_assert(LOG_CONTRACT_INFO_MESSAGES == 0);
#else
#define LOG_CONTRACT_INFO_MESSAGES 1
#endif
#include "contract_testing.h"
#include <chrono>
#include <cstdlib>
#include <fstream>

namespace
{
	// Test-only request builder; run() invokes each dedicated public ABI.
	struct OperationCommand
	{
		id owner;                  // Account whose holdings, authorization or credit are addressed.
		id receiver;               // Destination account for this operation.
		id editor;                 // Account authorized to edit metadata while policy permits.
		id royaltyReceiver;        // Account paid creator royalties on shop sales.
		id commitment;             // Committed metadata digest checked during reveal.
		QNFT::Metadata metadata;   // URI and content digest for this object.
		sint64 quantity;           // Number of native asset units requested or returned.
		sint64 maxUnits;           // Lifetime edition-unit issuance cap for the collection.
		sint64 offeredFee;         // Maximum QU offered to the receiving asset manager.
		uint32 itemId;             // Zero-based item identifier; meaningful in item operations.
		uint32 collectionId;       // Zero-based collection identifier.
		uint32 maxItems;           // Maximum number of distinct items allowed.
		uint32 qbayId;             // Identifier of the underlying QBay NFT.
		QNFT::Operation operation; // Public NFT operation selected by the test.
		uint16 royaltyBps;         // Creator royalty in basis points; 10000 means 100%.
		uint16 destination;        // Contract index receiving asset management rights.
		QNFT::Policy policy;       // Rules governing metadata changes and reveal.
	};

	constexpr sint64 TEST_MINT_FEE = 1000000;
	const id alice(1001, 1, 2, 3), bob(1002, 1, 2, 3), carol(1003, 1, 2, 3);
	const id nftId(QNFT_CONTRACT_INDEX, 0, 0, 0);
	QNFT::Metadata metadata(const char* uri = "ipfs://example/metadata.json", uint64 hash = 1)
	{
		QNFT::Metadata m{};
		for (unsigned i = 0; uri[i]; ++i)
		{
			m.uri.set(i, uri[i]);
		}
		m.contentHash = id(hash, 2, 3, 4);
		return m;
	}
	constexpr uint64 nextNameForIssuerAttack = 0x3130303030304eULL; // N000001
	void forgeFromManagementCallback(const QpiContextProcedureCall& qpi, void* state, void*, void* output, void*)
	{
		// Adversarial SC behavior; use real QPI issuance and callback invocator semantics.
		static_cast<TESTEXA::StateData*>(state)->heavyComputationResult = qpi.issueAsset(nextNameForIssuerAttack, qpi.invocator(), 0, 1, 0);
		static_cast<PreManagementRightsTransfer_output*>(output)->allowTransfer = true;
	}
	void countRevenueCallback(const QpiContextProcedureCall&, void* state, void*, void*, void*)
	{
		++static_cast<TESTEXA::StateData*>(state)->heavyComputationResult;
	}
	uint64 observedSaleListingId = 0;
	void observeRoyaltyPayment(const QpiContextProcedureCall& qpi, void* state, void* input, void*, void*)
	{
		const auto& payment = *static_cast<QPI::PostIncomingTransfer_input*>(input);
		const auto* nft = reinterpret_cast<const QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
		QNFT::Listing listing{};
		unsigned observed = 0;
		if (qpi.invocationReward() == 0)
		{
			observed |= 1;
		}
		if (nft->listings.get(observedSaleListingId, listing) && listing.quantity == 7)
		{
			observed |= 2;
		}
		if (qpi.numberOfPossessedShares(nft->items.get(0).assetName, nftId, bob, bob, QNFT_CONTRACT_INDEX, QNFT_CONTRACT_INDEX) == 3)
		{
			observed |= 4;
		}
		if (payment.sourceId == nftId && payment.amount == 1500)
		{
			observed |= 8;
		}
		static_cast<TESTEXA::StateData*>(state)->heavyComputationResult = observed;
	}

} // namespace

class NftTest : public ContractTesting, public ::testing::Test
{
public:
	NftTest()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(QNFT);
		INIT_CONTRACT(QX);
		INIT_CONTRACT(QBAY);
		INIT_CONTRACT(NOST);
		system.epoch = 9999;
		system.initialTick = system.tick;
		utcTime.Year = 2026;
		utcTime.Month = 10;
		utcTime.Day = 17;
		utcTime.Hour = 12;
		utcTime.Minute = 0;
		utcTime.Second = 0;
		utcTime.Nanosecond = 0;
		updateQpiTime();
		callSystemProcedure(QNFT_CONTRACT_INDEX, INITIALIZE);
		// Isolate pre-existing lifecycle/accounting scenarios from the new default tariff schedule.
		auto* tariffState = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
		tariffState->mintFee = TEST_MINT_FEE;
		tariffState->nominateAdminFee = 0;
		tariffState->transferFee = 0;
		tariffState->approveFee = 0;
		tariffState->updateMetadataFee = 0;
		tariffState->exportFee = 0;
		tariffState->importFee = TEST_MINT_FEE;
		tariffState->listingFee = 0;
		tariffState->proposalFee = 0;

		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
		callSystemProcedure(QBAY_CONTRACT_INDEX, INITIALIZE);
		callSystemProcedure(NOST_CONTRACT_INDEX, INITIALIZE);
		increaseEnergy(alice, 100000000000LL);
		increaseEnergy(bob, 100000000000LL);
		increaseEnergy(carol, 100000000000LL);
	}
	template<typename Input>
	QNFT::OperationResult action(unsigned short procedure, const Input& input, const id& caller = alice, sint64 reward = 0)
	{
		QNFT::OperationResult out{};
		EXPECT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][procedure], nullptr);
		if (!contractUserProcedures[QNFT_CONTRACT_INDEX][procedure])
		{
			return out;
		}
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, procedure, input, out, caller, reward));
		return out;
	}
	// Adapt the request-specific response for the shared command test driver only.
	QNFT::OperationResult action(unsigned short procedure, const QNFT::PrepareImport_input& input, const id& caller = alice, sint64 reward = 0)
	{
		QNFT::PrepareImport_output prepared{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, procedure, input, prepared, caller, reward));
		QNFT::OperationResult result{};
		result.itemId = prepared.requestId;
		result.code = prepared.code;
		return result;
	}
	QNFT::OperationResult run(const OperationCommand& in, const id& caller = alice, sint64 reward = 0)
	{
		switch (in.operation)
		{
			case QNFT::Operation::CreateCollection:
			{
				QNFT::CreateCollection_input request{};
				request.metadata = in.metadata;
				request.maxUnits = in.maxUnits;
				request.maxItems = in.maxItems;
				return action(1, request, caller, reward);
			}
			case QNFT::Operation::NominateAdmin:
			{
				QNFT::NominateAdmin_input request{};
				request.receiver = in.receiver;
				request.collectionId = in.collectionId;
				return action(4, request, caller, reward);
			}
			case QNFT::Operation::AcceptAdmin:
			{
				QNFT::AcceptAdmin_input request{};
				request.collectionId = in.collectionId;
				return action(5, request, caller, reward);
			}
			case QNFT::Operation::Mint:
			{
				QNFT::Mint_input request{};
				request.receiver = in.receiver;
				request.editor = in.editor;
				request.royaltyReceiver = in.royaltyReceiver;
				request.commitment = in.commitment;
				request.metadata = in.metadata;
				request.quantity = in.quantity;
				request.collectionId = in.collectionId;
				request.royaltyBps = in.royaltyBps;
				request.policy = in.policy;
				return action(2, request, caller, reward);
			}
			case QNFT::Operation::Transfer:
			{
				QNFT::Transfer_input request{};
				request.owner = in.owner;
				request.receiver = in.receiver;
				request.quantity = in.quantity;
				request.itemId = in.itemId;
				return action(9, request, caller, reward);
			}
			case QNFT::Operation::Approve:
			{
				QNFT::Approve_input request{};
				request.receiver = in.receiver;
				request.itemId = in.itemId;
				return action(10, request, caller, reward);
			}
			case QNFT::Operation::Revoke:
			{
				QNFT::Revoke_input request{};
				request.itemId = in.itemId;
				return action(11, request, caller, reward);
			}
			case QNFT::Operation::UpdateMetadata:
			{
				QNFT::UpdateMetadata_input request{};
				request.metadata = in.metadata;
				request.itemId = in.itemId;
				return action(6, request, caller, reward);
			}
			case QNFT::Operation::FreezeMetadata:
			{
				QNFT::FreezeMetadata_input request{};
				request.itemId = in.itemId;
				return action(8, request, caller, reward);
			}
			case QNFT::Operation::RevealMetadata:
			{
				QNFT::RevealMetadata_input request{};
				request.metadata = in.metadata;
				request.itemId = in.itemId;
				return action(7, request, caller, reward);
			}
			case QNFT::Operation::Export:
			{
				QNFT::Export_input request{};
				request.owner = in.owner == NULL_ID ? caller : in.owner;
				request.quantity = in.quantity;
				request.offeredFee = in.offeredFee;
				request.itemId = in.itemId;
				request.destination = in.destination;
				return action(13, request, caller, reward);
			}
			case QNFT::Operation::Burn:
			{
				QNFT::Burn_input request{};
				request.owner = in.owner == NULL_ID ? caller : in.owner;
				request.quantity = in.quantity;
				request.itemId = in.itemId;
				return action(12, request, caller, reward);
			}
			case QNFT::Operation::ClaimMint:
			{
				QNFT::ClaimMint_input request{};
				request.itemId = in.itemId;
				return action(3, request, caller, reward);
			}
			case QNFT::Operation::PrepareImport:
			{
				QNFT::PrepareImport_input request{};
				request.receiver = in.receiver;
				request.collectionId = in.collectionId;
				request.qbayId = in.qbayId;
				return action(14, request, caller, reward);
			}
			case QNFT::Operation::CancelImport:
			{
				QNFT::CancelImport_input request{};
				request.qbayId = in.qbayId;
				request.requestId = in.itemId;
				return action(16, request, caller, reward);
			}
			case QNFT::Operation::FinalizeImport:
			{
				QNFT::FinalizeImport_input request{};
				request.qbayId = in.qbayId;
				request.requestId = in.itemId;
				return action(15, request, caller, reward);
			}
			case QNFT::Operation::FundReserve:
			{
				QNFT::FundReserve_input request{};
				return action(20, request, caller, reward);
			}
			default: ADD_FAILURE() << "Unknown test operation"; return QNFT::OperationResult{.code = QNFT::Code::InvalidInput};
		}
	}
	uint32 collection(sint64 units = 1000, uint32 items = 10)
	{
		OperationCommand in{};
		in.operation = QNFT::Operation::CreateCollection;
		in.maxItems = items;
		in.maxUnits = units;
		in.metadata = metadata();
		auto out = run(in, alice, QNFT_COLLECTION_FEE);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.itemId;
	}
	QNFT::OperationResult mint(sint64 quantity = 100, QNFT::Policy policy = QNFT::Policy::Immutable, id commitment = NULL_ID)
	{
		OperationCommand in{};
		in.operation = QNFT::Operation::Mint;
		in.quantity = quantity;
		in.receiver = alice;
		in.editor = alice;
		in.royaltyReceiver = carol;
		in.metadata = metadata();
		in.policy = policy;
		in.commitment = commitment;
		in.royaltyBps = 500;
		return run(in, alice, TEST_MINT_FEE);
	}
	sint64 balance(uint32 item, const id& owner, uint16 manager = 0)
	{
		QNFT::GetBalance_input in{owner, item, manager, 0};
		QNFT::GetBalance_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 9, in, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.quantity;
	}
	QNFT::Item item(uint32 itemId = 0)
	{
		QNFT::GetItem_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetItem_input{itemId}, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.item;
	}
	uint16 propose(uint64 variable, sint64 value, const id& proposer = alice, sint64 reward = 0)
	{
		QNFT::SetShareholderProposal_input in{};
		in.epoch = system.epoch;
		in.type = ProposalTypes::VariableYesNo;
		in.data.variableOptions.variable = variable;
		in.data.variableOptions.value = value;
		uint16 out = INVALID_PROPOSAL_INDEX;
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65534, in, out, proposer, reward));
		return out;
	}
	QNFT::ProposalDataT proposal(uint16 slot)
	{
		QNFT::GetShareholderProposal_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 65533, QNFT::GetShareholderProposal_input{slot}, out);
		return out.proposal;
	}
	bool ballot(uint16 slot, const QNFT::ProposalDataT& proposal, const id& voter = alice, uint32 yes = 0, uint32 no = 0, sint64 reward = 0)
	{
		QNFT::SetShareholderVotes_input in{};
		in.proposalIndex = slot;
		in.proposalType = proposal.type;
		in.proposalTick = proposal.tick;
		in.voteValues.set(0, 1);
		in.voteCounts.set(0, yes);
		in.voteValues.set(1, 0);
		in.voteCounts.set(1, no);
		QNFT::SetShareholderVotes_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65535, in, out, voter, reward));
		return out;
	}
	void expectQuCollateral()
	{
		const auto* state = reinterpret_cast<const QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
		sint64 liabilities = state->epochFees + state->developerPending + state->shareholderCarry;
		for (sint64 slot = state->contractRefunds.nextElementIndex(NULL_INDEX); slot != NULL_INDEX;
		     slot = state->contractRefunds.nextElementIndex(slot))
		{
			liabilities += state->contractRefunds.value(slot);
		}
		EXPECT_EQ(getBalance(nftId), liabilities);
	}
};

namespace
{
	// Literal wire layouts independently check the public shop ABI.
	struct ShopListInput
	{
		sint64 unitPrice;
		sint64 quantity;
		uint32 itemId;
		uint32 reserved;
	};
	struct ShopListOutput
	{
		uint64 listingId;
		QNFT::Code code;
	};
	struct ShopBuyInput
	{
		id receiver;
		uint64 listingId;
		sint64 quantity;
		sint64 unitPrice;
		uint32 metadataVersion;
		uint32 reserved;
	};
	struct ShopBuyOutput
	{
		sint64 paid;
		sint64 fee;
		sint64 royalty;
		QNFT::Code code;
	};
} // namespace

// Exercise deployment tariffs separately from lifecycle/accounting scenarios.
class NftFeeTest : public NftTest
{
public:
	NftFeeTest()
	{
		callSystemProcedure(QNFT_CONTRACT_INDEX, INITIALIZE);
	}
};

namespace
{
	// Independent wire fixtures keep the new ABI testable before contract implementation.
	struct FeeBatchEntryWire
	{
		uint64 variable;
		sint64 value;
	};
	struct FeeBatchInputWire
	{
		uint8 url[256];
		FeeBatchEntryWire changes[16];
		uint16 epoch;
		uint16 count;
		uint32 reserved;
	};
	struct FeeBatchOutputWire
	{
		FeeBatchEntryWire changes[16];
		uint32 count;
		uint32 reserved;
		uint32 proposalTick;
		QNFT::Code code;
	};
	static_assert(sizeof(FeeBatchInputWire) == 520);
	static_assert(sizeof(FeeBatchOutputWire) == 272);
} // namespace

TEST_F(NftFeeTest, BatchProposalChargesOnceAndAppliesAllValuesAfterEpochPayout)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][21], nullptr);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 4;
	input.changes[0] = {0, 2000000};
	input.changes[1] = {6, 20000};
	input.changes[2] = {3, 7000};
	input.changes[3] = {4, 1000};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	const auto before = getBalance(alice);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000017));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), before - 1000000);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->epochFees, 1000000);
	EXPECT_EQ(state->transferFee, 10000);
	const auto stored = proposal(slot);
	EXPECT_EQ(static_cast<uint32>(stored.type), static_cast<uint32>(ProposalTypes::MultiVariablesYesNo));
	FeeBatchOutputWire batch{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, batch);
	ASSERT_EQ(batch.code, QNFT::Code::Ok);
	EXPECT_EQ(batch.count, 4U);
	EXPECT_EQ(batch.changes[1].value, 20000);
	EXPECT_EQ(batch.proposalTick, stored.tick);
	ASSERT_TRUE(ballot(slot, stored));
	const auto reserve = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 200000);
	EXPECT_EQ(state->collectionFee, 2000000);
	EXPECT_EQ(state->transferFee, 20000);
	EXPECT_EQ(state->shareholderFeeBPS, 7000U);
	EXPECT_EQ(state->burnFeeBPS, 1000U);
	EXPECT_EQ(state->developerFeeBPS, 2000U);
	EXPECT_EQ(state->mintFee, 100000);
	expectQuCollateral();
}

TEST_F(NftFeeTest, BatchValidationRejectsEntireProposalWithoutCharging)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][21], nullptr);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto before = getBalance(alice);
	for (unsigned n = 0; n < 9; ++n)
	{
		FeeBatchInputWire input{};
		input.epoch = system.epoch;
		input.count = 2;
		input.changes[0] = {0, 77};
		input.changes[1] = {6, 88};
		switch (n)
		{
			case 0: input.count = 0; break;
			case 1: input.count = 14; break;
			case 2: input.changes[1].variable = 0; break;
			case 3: input.changes[1].variable = 13; break;
			case 4: input.changes[1].value = -1; break;
			case 5: input.changes[1].value = MAX_AMOUNT + 1; break;
			case 6:
				input.changes[0] = {3, 7000};
				input.changes[1] = {4, 2000};
				break;
			case 7: input.reserved = 1; break;
			case 8: input.changes[13].value = 1; break;
		}
		uint16 slot = INVALID_PROPOSAL_INDEX;
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000017));
		EXPECT_EQ(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_EQ(getBalance(alice), before);
		auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
		EXPECT_EQ(state->epochFees, 0);
		EXPECT_EQ(state->collectionFee, 1000000);
		EXPECT_EQ(state->transferFee, 10000);
	}
	expectQuCollateral();
}

TEST_F(NftFeeTest, BatchReplacementInvalidatesVotesAndCancellationClearsPayload)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 2;
	input.changes[0] = {0, 77};
	input.changes[1] = {6, 88};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto old = proposal(slot);
	ASSERT_TRUE(ballot(slot, old));
	++system.tick;
	input.changes[1].value = 99;
	uint16 replacement = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, replacement, alice, 1000000));
	EXPECT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(slot));
	EXPECT_FALSE(ballot(slot, old));
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, out);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.changes[1].value, 99);
	// Rejected replacement must preserve the previous proposal, its payload and all QU.
	const auto before = getBalance(alice);
	input.changes[1].variable = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, replacement, alice, 1000017));
	EXPECT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), before);
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.changes[1].value, 99);
	QNFT::SetShareholderProposal_input cancel{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65534, cancel, replacement, alice, 17));
	EXPECT_EQ(getBalance(alice), before);
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, out);
	EXPECT_EQ(out.code, QNFT::Code::NotFound);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->feeBatches.get(slot).count, 0U);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->collectionFee, 1000000);
	EXPECT_EQ(state->transferFee, 10000);
	expectQuCollateral();
}

TEST_F(NftFeeTest, BatchRejectsUnfundedAndUnauthorizedCallsAndAllowsZeroProposalFee)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 2;
	input.changes[0] = {12, 0};
	input.changes[1] = {6, MAX_AMOUNT};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	const auto aliceBefore = getBalance(alice), bobBefore = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 999999));
	EXPECT_EQ(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, bob, 1000000));
	EXPECT_EQ(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), aliceBefore);
	EXPECT_EQ(getBalance(bob), bobBefore);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->proposalFee, 0);
	EXPECT_EQ(state->transferFee, MAX_AMOUNT);
	++system.epoch;
	++system.tick;
	input.changes[1].value = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 0));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	expectQuCollateral();
}

TEST_F(NftFeeTest, BatchDigestMismatchCannotApplyAnyEntry)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 2;
	input.changes[0] = {0, 77};
	input.changes[1] = {6, 88};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto corrupted = state->feeBatches.get(slot);
	corrupted.changes.set(1, QNFT::FeeChange{QNFT::FeeVariable::TransferFee, 99});
	state->feeBatches.set(slot, corrupted);
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, out);
	EXPECT_EQ(out.code, QNFT::Code::NotFound);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->collectionFee, 1000000);
	EXPECT_EQ(state->transferFee, 10000);
	expectQuCollateral();
}

TEST_F(NftFeeTest, ScalarReplacementCannotApplyAnOldBatch)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 2;
	input.changes[0] = {0, 77};
	input.changes[1] = {6, 88};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 21, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	++system.tick;
	const auto replacement = propose(1, 123, alice, 1000000);
	ASSERT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(slot));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::GetShareholderProposal_input{slot}, out);
	EXPECT_EQ(out.code, QNFT::Code::NotFound);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->mintFee, 123);
	EXPECT_EQ(state->collectionFee, 1000000);
	EXPECT_EQ(state->transferFee, 10000);
	expectQuCollateral();
}

// Expose execution helpers only to verify that internal calls cannot bypass payment validation.
struct NftExecutionProbe : QNFT
{
	using QNFT::calculateFeeShare;
	using QNFT::effectiveRoyaltyBps;
	using QNFT::ExecuteApprove;
	using QNFT::ExecuteNominateAdmin;
	using QNFT::ExecuteTransfer;
	using QNFT::ExecuteUpdateMetadata;
};

TEST_F(NftFeeTest, ExecutionHelpersRejectInsufficientPaymentBeforeOtherValidation)
{
	auto& state = *reinterpret_cast<QPI::ContractState<QNFT::StateData, QNFT_CONTRACT_INDEX>*>(contractStates[QNFT_CONTRACT_INDEX]);
	QpiContextUserProcedureCall qpi(QNFT_CONTRACT_INDEX, alice, 9999);
	QNFT::OperationResult output{};
	QNFT::NominateAdmin_input nominate{};
	QNFT::ExecuteNominateAdmin_locals nominateLocals{};
	NftExecutionProbe::ExecuteNominateAdmin(qpi, state, nominate, output, nominateLocals);
	EXPECT_EQ(output.code, QNFT::Code::InsufficientFee);

	QNFT::Transfer_input transfer{};
	QNFT::ExecuteTransfer_locals transferLocals{};
	NftExecutionProbe::ExecuteTransfer(qpi, state, transfer, output, transferLocals);
	EXPECT_EQ(output.code, QNFT::Code::InsufficientFee);

	QNFT::Approve_input approve{};
	QNFT::ExecuteApprove_locals approveLocals{};
	NftExecutionProbe::ExecuteApprove(qpi, state, approve, output, approveLocals);
	EXPECT_EQ(output.code, QNFT::Code::InsufficientFee);

	QpiContextUserProcedureCall metadataQpi(QNFT_CONTRACT_INDEX, alice, 49999);
	QNFT::UpdateMetadata_input update{};
	QNFT::ExecuteUpdateMetadata_locals updateLocals{};
	NftExecutionProbe::ExecuteUpdateMetadata(metadataQpi, state, update, output, updateLocals);
	EXPECT_EQ(output.code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(state.get().epochFees, 0);
	EXPECT_EQ(state.get().itemCount, 0U);
	EXPECT_EQ(state.get().collectionCount, 0U);
}

TEST_F(NftFeeTest, DefaultsAndSuccessfulCallsChargeOnceWhileFailuresRefund)
{
	QNFT::GetFeeSchedule_output fees{};
	callFunction(QNFT_CONTRACT_INDEX, 18, NoData{}, fees);
	EXPECT_EQ(fees.collectionFee, 1000000);
	EXPECT_EQ(fees.mintFee, 100000);
	EXPECT_EQ(fees.transferFee, 10000);
	EXPECT_EQ(fees.updateMetadataFee, 50000);
	EXPECT_EQ(fees.proposalFee, 1000000);
	EXPECT_EQ(fees.shopFeeBPS, 200U);
	collection();
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto before = getBalance(alice), pool = state->epochFees;
	QNFT::Transfer_input transfer{alice, bob, 3, 0};
	EXPECT_EQ(action(9, transfer, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(action(9, transfer, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - 10000);
	EXPECT_EQ(state->epochFees, pool + 10000);
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(action(9, transfer, bob, 10017).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->epochFees, pool + 10000);
	EXPECT_EQ(action(6, QNFT::UpdateMetadata_input{metadata("ipfs://new"), 0}, alice, 50017).code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 60000);
	EXPECT_EQ(action(8, QNFT::FreezeMetadata_input{0}, alice, 0).code, QNFT::Code::Ok);
	EXPECT_EQ(action(11, QNFT::Revoke_input{0}, alice, 0).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->epochFees, pool + 60000);
	expectQuCollateral();
}

TEST_F(NftFeeTest, ListingAndExportKeepProtocolAndExternalPaymentsSeparate)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto before = getBalance(alice), pool = state->epochFees;
	QNFT::ListForSale_output listing{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 2, 0, 0}, listing, alice, 9999));
	EXPECT_EQ(listing.code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 2, 0, 0}, listing, alice, 10017));
	ASSERT_EQ(listing.code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 10000);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listing.listingId}, cancelled, alice, 0));
	EXPECT_EQ(cancelled.code, QNFT::Code::Ok);
	QNFT::Export_input exportInput{alice, 3, 1000000, 0, QX_CONTRACT_INDEX};
	EXPECT_EQ(action(13, exportInput, alice, 1009999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(action(13, exportInput, alice, 1010017).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - 10000 - 10000 - 100);
	EXPECT_EQ(state->epochFees, pool + 20000);
	EXPECT_EQ(balance(0, alice, QX_CONTRACT_INDEX), 3);
	expectQuCollateral();
}

TEST_F(NftFeeTest, AdministrationApprovalsAndQbayImportRespectPaidAndFreeBoundaries)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(action(4, QNFT::NominateAdmin_input{bob, 0}, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(action(4, QNFT::NominateAdmin_input{bob, 0}, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(action(5, QNFT::AcceptAdmin_input{0}, bob, 0).code, QNFT::Code::Ok);
	QNFT::Approve_input approval{};
	approval.receiver = bob;
	EXPECT_EQ(action(10, approval, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(action(10, approval, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(action(11, QNFT::Revoke_input{0}, alice, 0).code, QNFT::Code::Ok);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = bob;
	original.royalty = 5;
	original.URI.set(0, 'Q');
	original.URI.set(1, 'm');
	original.URI.set(2, '1');
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	const auto pool = state->epochFees;
	QNFT::PrepareImport_input request{bob, 0, 0};
	EXPECT_EQ(action(14, request, bob, 99999).code, QNFT::Code::InsufficientFee);
	const auto prepared = action(14, request, bob, 100017);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 100000);
	EXPECT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, bob, 0).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, request, bob, 0).code, QNFT::Code::InsufficientFee);
	const auto replacement = action(14, request, bob, 100017);
	ASSERT_EQ(replacement.code, QNFT::Code::Ok);
	EXPECT_NE(replacement.itemId, prepared.itemId);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, bob, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	const auto finalized = action(15, QNFT::FinalizeImport_input{0, replacement.itemId}, alice, 0);
	ASSERT_EQ(finalized.code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 200000);
	EXPECT_EQ(item(finalized.itemId).bridge, QNFT::Bridge::Permanent);
	expectQuCollateral();
}

TEST_F(NftFeeTest, EveryNewTariffIsGovernedButRecoveryAndDeveloperRatesAreNot)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint64 key = 5; key <= 12; ++key)
	{
		const auto charge = state->proposalFee, before = getBalance(alice);
		const auto slot = propose(key, 321, alice, charge + 17);
		ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_EQ(getBalance(alice), before - charge);
		ASSERT_TRUE(ballot(slot, proposal(slot)));
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
		QNFT::GetFeeSchedule_output fees{};
		callFunction(QNFT_CONTRACT_INDEX, 18, NoData{}, fees);
		const sint64 values[] = {fees.nominateAdminFee, fees.transferFee, fees.approveFee, fees.updateMetadataFee,
		                         fees.exportFee,        fees.importFee,   fees.listingFee, fees.proposalFee};
		EXPECT_EQ(values[key - 5], 321);
		EXPECT_EQ(state->developerFeeBPS, 2000U);
		++system.epoch;
		++system.tick;
	}
	EXPECT_EQ(static_cast<uint32>(propose(13, 1, alice, 10000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(propose(5, -1, alice, 10000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(propose(5, MAX_AMOUNT + 1, alice, 10000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto unrestricted = propose(6, MAX_AMOUNT, alice, 10000);
	ASSERT_NE(static_cast<uint32>(unrestricted), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto beforeCancel = getBalance(alice), poolBeforeCancel = state->epochFees;
	QNFT::SetShareholderProposal_input cancel{};
	uint16 cancelled = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65534, cancel, cancelled, alice, 17));
	EXPECT_NE(static_cast<uint32>(cancelled), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), beforeCancel);
	EXPECT_EQ(state->epochFees, poolBeforeCancel);
	QNFT::GetShareholderProposalFees_output votingFees{};
	callFunction(QNFT_CONTRACT_INDEX, 65531, NoData{}, votingFees);
	EXPECT_EQ(votingFees.setProposalFee, 321);
	EXPECT_EQ(votingFees.setVoteFee, 0);
	expectQuCollateral();
}

TEST_F(NftTest, ChangeDeveloperTransfersExclusiveAuthorityAndRefundsReward)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const id original = state->developer;
	increaseEnergy(original, 1000);
	const auto balanceBefore = getBalance(original);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{bob}, alice, 100).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->developer, original);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{bob}, original, 100).code, QNFT::Code::Ok);
	EXPECT_EQ(state->developer, bob);
	EXPECT_EQ(getBalance(original), balanceBefore);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{carol}, original).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->developer, bob);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{carol}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(state->developer, carol);
}

TEST_F(NftTest, ChangeDeveloperRejectsInvalidRecipientsAndPreservesPendingRevenue)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = alice;
	state->developerPending = 123;
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{NULL_ID}).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->developer, alice);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{nftId}).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->developer, alice);
	EXPECT_EQ(action(22, QNFT::ChangeDeveloper_input{bob}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->developerPending, 123);
}

TEST_F(NftTest, StandardQpiShareholderVotingInterfaceIsRegistered)
{
	EXPECT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][65534], nullptr);
	EXPECT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][65535], nullptr);
	for (unsigned i = 65531; i <= 65535; ++i)
	{
		EXPECT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][i], nullptr);
	}
}

TEST_F(NftTest, ShopUsesConfiguredRateAndRejectsRoyaltyOverflowOnExistingListings)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->shopFeeBPS = 300;
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{5000, 3, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1, 0}, bought, bob, 5000));
	EXPECT_EQ(bought.fee, 150);
	state->shopFeeBPS = 9600; // Existing item's 5% royalty no longer fits.
	const auto before = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1, 0}, bought, bob, 5000));
	EXPECT_EQ(bought.code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(bob), before);
	state->shopFeeBPS = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1, 0}, bought, bob, 5000));
	EXPECT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.fee, 0);
	expectQuCollateral();
}

TEST_F(NftTest, GovernanceChangesAllAllowedFeesButNeverDeveloperShare)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const sint64 values[] = {1234, 5678, 375, 4500, 1000};
	for (uint64 key = 0; key < 5; ++key)
	{
		const auto before = getBalance(alice);
		const auto slot = propose(key, values[key], alice, 17);
		ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_EQ(getBalance(alice), before);
		ASSERT_TRUE(ballot(slot, proposal(slot), alice, 0, 0, 29));
		EXPECT_EQ(getBalance(alice), before);
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
		EXPECT_EQ(state->developerFeeBPS, 2000U);
		EXPECT_EQ(state->developerFeeBPS + state->shareholderFeeBPS + state->burnFeeBPS, 10000U);
		if (key == 0)
		{
			EXPECT_EQ(state->collectionFee, 1234);
		}
		if (key == 1)
		{
			EXPECT_EQ(state->mintFee, 5678);
		}
		if (key == 2)
		{
			EXPECT_EQ(state->shopFeeBPS, 375U);
		}
		if (key == 3)
		{
			EXPECT_EQ(state->shareholderFeeBPS, 4500U);
			EXPECT_EQ(state->burnFeeBPS, 3500U);
		}
		if (key == 4)
		{
			EXPECT_EQ(state->burnFeeBPS, 1000U);
			EXPECT_EQ(state->shareholderFeeBPS, 7000U);
		}
		EXPECT_FALSE(ballot(slot, proposal(slot)));
		EXPECT_EQ(static_cast<uint32>(propose(0, 1)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		++system.epoch;
		++system.tick;
	}
	QNFT::GetGovernanceConfig_output cfg{};
	callFunction(QNFT_CONTRACT_INDEX, 17, NoData{}, cfg);
	EXPECT_EQ(cfg.developerFeeBPS, 2000U);
	EXPECT_EQ(cfg.shareholderFeeBPS, 7000U);
	EXPECT_EQ(cfg.burnFeeBPS, 1000U);
	EXPECT_EQ(cfg.proposalSlots, 8U);
	QNFT::GetShareholderProposalFees_output fees{};
	callFunction(QNFT_CONTRACT_INDEX, 65531, NoData{}, fees);
	EXPECT_EQ(fees.setProposalFee, 0);
	EXPECT_EQ(fees.setVoteFee, 0);
	expectQuCollateral();
}

TEST_F(NftTest, GovernanceRejectsNonShareholdersAndUnsafeOrProtectedVariables)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto before = getBalance(bob);
	EXPECT_EQ(static_cast<uint32>(propose(0, 123, bob, 99)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(bob), before);
	struct Invalid
	{
		uint64 key;
		sint64 value;
	};
	for (const auto row : {Invalid{0, -1}, Invalid{1, MAX_AMOUNT + 1}, Invalid{2, 10001}, Invalid{3, 8001}, Invalid{4, 8001}, Invalid{13, 2000},
	                       Invalid{0xffffffffffffffffULL, 0}})
	{
		EXPECT_EQ(static_cast<uint32>(propose(row.key, row.value)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	}
	const auto slot = propose(0, 123);
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_FALSE(ballot(slot, proposal(slot), bob, 0, 0, 55));
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_FALSE(ballot(slot, proposal(slot), alice, 677));
	EXPECT_FALSE(ballot(slot, proposal(slot), alice, 0xffffffffU, 677));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	const auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->collectionFee, 1000000);
	EXPECT_EQ(state->developerFeeBPS, 2000U);
	expectQuCollateral();
}

TEST_F(NftTest, GovernanceUsesNativeQuorumAndRejectsTiesOrNoMajority)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	struct Row
	{
		uint32 yes;
		uint32 no;
		bool accepted;
	};
	for (const auto row : {Row{450, 0, false}, Row{451, 0, true}, Row{226, 225, true}, Row{225, 226, false}, Row{338, 338, false}})
	{
		state->collectionFee = 1000000;
		const auto slot = propose(0, 123);
		ASSERT_TRUE(ballot(slot, proposal(slot), alice, row.yes, row.no));
		QNFT::GetShareholderVotingResults_output summary{};
		callFunction(QNFT_CONTRACT_INDEX, 65535, QNFT::GetShareholderVotingResults_input{slot}, summary);
		EXPECT_EQ(summary.totalVotesAuthorized, 676U);
		EXPECT_EQ(summary.totalVotesCasted, row.yes + row.no);
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
		EXPECT_EQ(state->collectionFee, row.accepted ? 123 : 1000000);
		++system.epoch;
		++system.tick;
	}
}

TEST_F(NftTest, GovernanceSnapshotsSharesAndRejectsStaleProposalVersions)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto slot = propose(0, 123);
	const auto original = proposal(slot);
	ASSERT_TRUE(ballot(slot, original));
	++system.tick;
	EXPECT_EQ(static_cast<uint32>(propose(0, 456)), static_cast<uint32>(slot));
	EXPECT_FALSE(ballot(slot, original));
	QX::TransferShareOwnershipAndPossession_input move{NULL_ID, bob, QNFT_SHARE_NAME, 676};
	QX::TransferShareOwnershipAndPossession_output moved{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 2, move, moved, alice, 100));
	const auto replacement = proposal(slot);
	EXPECT_FALSE(ballot(slot, replacement, bob));
	EXPECT_TRUE(ballot(slot, replacement, alice));
	QNFT::GetShareholderVotes_output votes{};
	callFunction(QNFT_CONTRACT_INDEX, 65534, QNFT::GetShareholderVotes_input{alice, slot}, votes);
	EXPECT_EQ(votes.proposalTick, replacement.tick);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->collectionFee, 456);
	++system.epoch;
	EXPECT_FALSE(ballot(slot, replacement, alice));
	QNFT::GetShareholderProposalIndices_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 65532, QNFT::GetShareholderProposalIndices_input{false, -1}, page);
	EXPECT_EQ(static_cast<uint32>(page.numOfIndices), 1U);
}

TEST_F(NftTest, GovernancePaysClosingEpochBeforeApplyingRevenueSplit)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	const auto slot = propose(3, 3000);
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	const auto before = getBalance(alice);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(alice), before + 599612); // 60% of 1m, rounded per share.
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->shareholderFeeBPS, 3000U);
	EXPECT_EQ(state->burnFeeBPS, 5000U);
	EXPECT_EQ(state->shareholderCarry, 388);
	state->shareholderFeeBPS = 4000;
	state->burnFeeBPS = 4000; // Detect accidental reapplication.
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->shareholderFeeBPS, 4000U);
	EXPECT_EQ(getBalance(alice), before + 599612);
	expectQuCollateral();
}

TEST_F(NftTest, GovernanceCapacityCancellationAndDeterministicConflictingUpdates)
{
	std::vector<std::pair<m256i, unsigned int>> owners;
	for (unsigned i = 0; i < 9; ++i)
	{
		owners.push_back({id(9000 + i, 1, 2, 3), i == 0 ? 76U : 75U});
	}
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	for (const auto& owner : owners)
	{
		increaseEnergy(owner.first, 1000);
	}
	for (unsigned i = 0; i < 8; ++i)
	{
		EXPECT_NE(static_cast<uint32>(propose(0, 100 + i, owners[i].first)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	}
	EXPECT_EQ(static_cast<uint32>(propose(0, 999, owners[8].first)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	QNFT::SetShareholderProposal_input cancel{};
	uint16 cancelled = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65534, cancel, cancelled, owners[0].first, 0));
	EXPECT_EQ(static_cast<uint32>(cancelled), 0U);
	const auto slot = propose(4, 1000, owners[8].first);
	EXPECT_EQ(static_cast<uint32>(slot), 0U);
	const auto other = propose(3, 4000, owners[1].first);
	EXPECT_EQ(static_cast<uint32>(other), 1U);
	for (const auto& owner : owners)
	{
		ASSERT_TRUE(ballot(slot, proposal(slot), owner.first));
		ASSERT_TRUE(ballot(other, proposal(other), owner.first));
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	const auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->developerFeeBPS, 2000U);
	EXPECT_EQ(state->shareholderFeeBPS, 4000U);
	EXPECT_EQ(state->burnFeeBPS, 4000U);
}

TEST_F(NftTest, GovernanceSupportsContractShareholdersThroughQpiNotifications)
{
	INIT_CONTRACT(TESTEXA);
	const id shareholder(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	std::vector<std::pair<m256i, unsigned int>> owners{{shareholder, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	QNFT::ProposalDataT data{};
	data.epoch = system.epoch;
	data.type = ProposalTypes::VariableYesNo;
	data.data.variableOptions.variable = 2;
	data.data.variableOptions.value = 321;
	TESTEXA::SetProposalInOtherContractAsShareholder_input input{};
	copyToBuffer(input, data);
	input.otherContractIndex = QNFT_CONTRACT_INDEX;
	TESTEXA::SetProposalInOtherContractAsShareholder_output output{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 40, input, output, alice, 7));
	ASSERT_NE(static_cast<uint32>(output.proposalIndex), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto stored = proposal(output.proposalIndex);
	TESTEXA::SetVotesInOtherContractAsShareholder_input vote{};
	vote.otherContractIndex = QNFT_CONTRACT_INDEX;
	vote.voteData.proposalIndex = output.proposalIndex;
	vote.voteData.proposalType = stored.type;
	vote.voteData.proposalTick = stored.tick;
	vote.voteData.voteValues.set(0, 1);
	TESTEXA::SetVotesInOtherContractAsShareholder_output voted{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 41, vote, voted, alice, 11));
	EXPECT_TRUE(voted.success);
	QNFT::ListRefunds_output refund{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, refund);
	EXPECT_EQ(refund.count, 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->shopFeeBPS, 321U);
	expectQuCollateral();
}

TEST_F(NftTest, CollectionCreationAlwaysSettlesSuccessAndValidationFailures)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto initialBalance = getBalance(alice);
	QNFT::CreateCollection_input request{metadata(), 1000, 10};
	request.maxItems = 0;
	EXPECT_EQ(action(1, request, alice, QNFT_COLLECTION_FEE + 17).code, QNFT::Code::InvalidInput);
	request.maxItems = 10;
	request.metadata.uri.set(0, 0);
	EXPECT_EQ(action(1, request, alice, QNFT_COLLECTION_FEE + 17).code, QNFT::Code::InvalidInput);
	request.metadata = metadata();
	EXPECT_EQ(action(1, request, alice, QNFT_COLLECTION_FEE - 1).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(state->collectionCount, 0U);
	state->collectionCount = QNFT_MAX_COLLECTIONS;
	EXPECT_EQ(action(1, request, alice, QNFT_COLLECTION_FEE + 17).code, QNFT::Code::Capacity);
	EXPECT_EQ(state->collectionCount, QNFT_MAX_COLLECTIONS);
	EXPECT_EQ(state->epochFees, 0);
	EXPECT_EQ(getBalance(alice), initialBalance);
	state->collectionCount = 0;
	const auto created = action(1, request, alice, QNFT_COLLECTION_FEE + 17);
	ASSERT_EQ(created.code, QNFT::Code::Ok);
	EXPECT_EQ(created.itemId, 0U);
	EXPECT_EQ(state->collectionCount, 1U);
	EXPECT_EQ(state->epochFees, QNFT_COLLECTION_FEE);
	EXPECT_EQ(getBalance(alice), initialBalance - QNFT_COLLECTION_FEE);
	EXPECT_EQ(state->collections.get(0).administrator, alice);
	expectQuCollateral();
}

TEST_F(NftTest, OnlyDedicatedProceduresAreRegistered)
{
	for (unsigned short procedure = 1; procedure <= 21; ++procedure)
	{
		EXPECT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][procedure], nullptr);
	}
	EXPECT_EQ(contractUserProcedures[QNFT_CONTRACT_INDEX][22], nullptr);
	for (unsigned short function = 1; function <= 19; ++function)
	{
		EXPECT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][function], nullptr);
	}
	EXPECT_EQ(contractUserFunctions[QNFT_CONTRACT_INDEX][20], nullptr);
}

TEST_F(NftTest, CompactRecoveryProceduresRefundMissingItemsWithoutChangingBalances)
{
	const auto before = getBalance(alice);
	QNFT::ClaimMint_input request{};
	for (unsigned short procedure : {8, 3, 11, 16, 15})
	{
		const auto out = procedure == 16   ? action(procedure, QNFT::CancelImport_input{0, 0}, alice, 123)
		                 : procedure == 15 ? action(procedure, QNFT::FinalizeImport_input{0, 0}, alice, 123)
		                                   : action(procedure, request, alice, 123);
		EXPECT_EQ(out.code, QNFT::Code::NotFound);
		EXPECT_EQ(getBalance(alice), before);
		EXPECT_EQ(getBalance(nftId), 0);
	}
}

TEST(NftAbi, GovernanceWireLayoutsMatchStandardQpiAndSdk)
{
	EXPECT_EQ(sizeof(QNFT::SetShareholderProposal_input), 304U);
	EXPECT_EQ(sizeof(QNFT::SetShareholderProposal_output), 2U);
	EXPECT_EQ(sizeof(QNFT::SetShareholderVotes_input), 104U);
	EXPECT_EQ(sizeof(QNFT::SetShareholderVotes_output), 1U);
	EXPECT_EQ(sizeof(QNFT::GetShareholderProposal_output), 336U);
	EXPECT_EQ(offsetof(QNFT::GetShareholderProposal_output, proposerPubicKey), 304U);
	EXPECT_EQ(sizeof(QNFT::GetShareholderProposalIndices_input), 8U);
	EXPECT_EQ(sizeof(QNFT::GetShareholderProposalIndices_output), 130U);
	EXPECT_EQ(sizeof(QNFT::GetShareholderVotes_input), 40U);
	EXPECT_EQ(sizeof(QNFT::GetShareholderVotingResults_output), 48U);
	EXPECT_EQ(sizeof(QNFT::GetGovernanceConfig_output), 20U);
	EXPECT_EQ(offsetof(QNFT::GovernanceEvent, _terminator), 64U);
}

TEST(NftAbi, CompactWireLayoutMatchesSdk)
{
	{
		QNFT::CreateCollection_input input{};
		input.metadata = metadata();
		input.maxUnits = 1000;
		input.maxItems = 10;
		EXPECT_EQ(sizeof(input), 304U);
		std::ofstream file("abi-compact-CreateCollection.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::NominateAdmin_input input{};
		setMem(&input.receiver, 32, 2);
		input.collectionId = 2;
		EXPECT_EQ(sizeof(input), 40U);
		std::ofstream file("abi-compact-NominateAdmin.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::AcceptAdmin_input input{};
		input.collectionId = 2;
		EXPECT_EQ(sizeof(input), 4U);
		std::ofstream file("abi-compact-AcceptAdmin.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Mint_input input{};
		setMem(&input.receiver, 32, 2);
		setMem(&input.editor, 32, 3);
		setMem(&input.royaltyReceiver, 32, 4);
		setMem(&input.commitment, 32, 5);
		input.metadata = metadata();
		input.quantity = 3;
		input.collectionId = 2;
		input.royaltyBps = 500;
		input.policy = QNFT::Policy::Reveal;
		EXPECT_EQ(sizeof(input), 432U);
		std::ofstream file("abi-compact-Mint.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Transfer_input input{};
		setMem(&input.owner, 32, 1);
		setMem(&input.receiver, 32, 2);
		input.quantity = 3;
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 80U);
		std::ofstream file("abi-compact-Transfer.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Approve_input input{};
		setMem(&input.receiver, 32, 2);
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 40U);
		std::ofstream file("abi-compact-Approve.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Revoke_input input{};
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 4U);
		std::ofstream file("abi-compact-Revoke.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::UpdateMetadata_input input{};
		input.metadata = metadata();
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 296U);
		std::ofstream file("abi-compact-UpdateMetadata.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::FreezeMetadata_input input{};
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 4U);
		std::ofstream file("abi-compact-FreezeMetadata.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::RevealMetadata_input input{};
		input.metadata = metadata();
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 296U);
		std::ofstream file("abi-compact-RevealMetadata.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Export_input input{};
		input.owner = id(0x0101010101010101ULL, 0x0101010101010101ULL, 0x0101010101010101ULL, 0x0101010101010101ULL);
		input.quantity = 3;
		input.offeredFee = 7;
		input.itemId = 36;
		input.destination = 1;
		EXPECT_EQ(sizeof(input), 56U);
		std::ofstream file("abi-compact-Export.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::Burn_input input{};
		input.owner = id(0x0101010101010101ULL, 0x0101010101010101ULL, 0x0101010101010101ULL, 0x0101010101010101ULL);
		input.quantity = 3;
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 48U);
		std::ofstream file("abi-compact-Burn.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::ClaimMint_input input{};
		input.itemId = 36;
		EXPECT_EQ(sizeof(input), 4U);
		std::ofstream file("abi-compact-ClaimMint.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::PrepareImport_input input{};
		setMem(&input.receiver, 32, 2);
		input.collectionId = 2;
		input.qbayId = 7;
		EXPECT_EQ(sizeof(input), 40U);
		std::ofstream file("abi-compact-PrepareImport.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::CancelImport_input input{};
		input.qbayId = 7;
		input.requestId = 36;
		EXPECT_EQ(sizeof(input), 8U);
		std::ofstream file("abi-compact-CancelImport.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::FinalizeImport_input input{};
		input.qbayId = 7;
		input.requestId = 36;
		EXPECT_EQ(sizeof(input), 8U);
		std::ofstream file("abi-compact-FinalizeImport.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::FundReserve_input input{};
		std::ofstream file("abi-compact-FundReserve.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
}

TEST_F(NftTest, ShopEscrowsPartialSaleAndChargesExactlyTwoPercent)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][17], nullptr);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ShopListOutput listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	ASSERT_NE(listed.listingId, 0);
	EXPECT_EQ(balance(0, alice), 90);
	EXPECT_EQ(balance(0, nftId), 10);
	const auto buyerBalance = getBalance(bob), sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	const auto reserve = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	ShopBuyOutput bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.paid, 30000);
	EXPECT_EQ(bought.fee, 600);
	EXPECT_EQ(bought.royalty, 1500);
	EXPECT_EQ(getBalance(bob), buyerBalance - 30000);
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(balance(0, nftId), 7);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve); // No burn before END_EPOCH.
	EXPECT_EQ(getBalance(alice), sellerBefore + 27900);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 1500);
	expectQuCollateral();
}

TEST_F(NftTest, ShopRejectsUnfundedStaleAndUnauthorizedPurchasesWithoutLosingEscrow)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][17], nullptr);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ShopListOutput listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto buyerBalance = getBalance(bob);
	for (const auto& command : {ShopBuyInput{bob, listed.listingId, 3, 9999, 1, 0}, ShopBuyInput{bob, listed.listingId, 3, 10000, 2, 0},
	                            ShopBuyInput{bob, listed.listingId, 11, 10000, 1, 0}})
	{
		ShopBuyOutput out{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, command, out, bob, 30000));
		EXPECT_NE(out.code, QNFT::Code::Ok);
		EXPECT_EQ(getBalance(bob), buyerBalance);
		EXPECT_EQ(balance(0, nftId), 10);
	}
	ShopBuyOutput out{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1, 0}, out, bob, 29999));
	EXPECT_EQ(out.code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	EXPECT_EQ(balance(0, nftId), 10);
}

TEST_F(NftTest, ShopEpochDistributionExcludesCollateralAndCannotRunTwice)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][17], nullptr);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto reserve = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve);
	ShopListOutput listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0, 0}, listed, alice, 0));
	ShopBuyOutput bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	const auto shareholderBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 400120);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 400120);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	EXPECT_EQ(getBalance(carol), royaltyBefore);
	expectQuCollateral();
}

TEST_F(NftTest, ShopCancellationReturnsOnlyUnsoldUnitsAndListingIdsNeverRepeat)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0, 0}, listed, alice, 0));
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, bob, 0));
	EXPECT_EQ(cancelled.code, QNFT::Code::Unauthorized);
	EXPECT_EQ(balance(0, nftId), 7);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 97);
	EXPECT_EQ(balance(0, nftId), 0);
	const auto oldId = listed.listingId;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{20000, 7, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	EXPECT_GT(listed.listingId, oldId);
	const auto buyerBalance = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, oldId, 1, 10000, 1, 0}, bought, bob, 10000));
	EXPECT_EQ(bought.code, QNFT::Code::NotFound);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	QNFT::GetListing_output listing{};
	callFunction(QNFT_CONTRACT_INDEX, 11, QNFT::GetListing_input{listed.listingId}, listing);
	EXPECT_EQ(listing.code, QNFT::Code::Ok);
	EXPECT_EQ(listing.listing.quantity, 7);
	QNFT::ListListings_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::ListListings_input{0, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1);
	EXPECT_EQ(page.listingIds.get(0), listed.listingId);
}

TEST_F(NftTest, ShopMetadataChangesInvalidateListingsAndOverflowNeverChargesBuyer)
{
	collection();
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{MAX_AMOUNT, 2, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	const auto buyerBalance = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 2, MAX_AMOUNT, 1, 0}, bought, bob, 123));
	EXPECT_EQ(bought.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	OperationCommand update{};
	update.operation = QNFT::Operation::UpdateMetadata;
	update.metadata = metadata("ipfs://changed");
	ASSERT_EQ(run(update).code, QNFT::Code::Ok);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, MAX_AMOUNT, 1, 0}, bought, bob, 123));
	EXPECT_EQ(bought.code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	EXPECT_EQ(balance(0, nftId), 2);
}

TEST_F(NftTest, ShopPaysRoyaltyContractsBeforeEpochRevenue)
{
	INIT_CONTRACT(TESTEXA);
	const id malicious(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	std::vector<std::pair<m256i, unsigned int>> owners{{malicious, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][POST_INCOMING_TRANSFER] = countRevenueCallback;
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = malicious; // Deployment recipient, also the royalty recipient and shareholder.
	collection();
	OperationCommand mintInput{};
	mintInput.operation = QNFT::Operation::Mint;
	mintInput.quantity = 100;
	mintInput.receiver = alice;
	mintInput.metadata = metadata();
	mintInput.royaltyReceiver = malicious;
	mintInput.royaltyBps = 500;
	ASSERT_EQ(run(mintInput, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0, 0}, listed, alice, 0));
	increaseEnergy(malicious, 100000);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1, 0}, bought, malicious, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::ListRefunds_output refund{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, refund);
	EXPECT_EQ(refund.count, 0);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 2);
	const auto paidBefore = getBalance(malicious);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(malicious), paidBefore + 1600020);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 4);
	EXPECT_EQ(issuanceIndex(nftId, nextNameForIssuerAttack), NO_ASSET_INDEX);
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
	expectQuCollateral();
}

namespace
{
	struct TransferLogWire
	{
		uint32 contractIndex, type;
		id operatorId, from, to;
		sint64 quantity;
		uint32 itemId;
	};

	std::vector<TransferLogWire> transferLogs()
	{
		const auto range = qLogger::tx.getLogIdInfo(0, system.tick, 0);
		std::vector<TransferLogWire> result;
		for (long long i = 0; i < range.length; ++i)
		{
			char buffer[1024]{};
			qLogger::logBuf.get(buffer, range.startIndex + i);
			uint32 sizeAndType = 0;
			memcpy(&sizeAndType, buffer + 6, 4);
			if ((sizeAndType >> 24) != CONTRACT_INFORMATION_MESSAGE || (sizeAndType & 0xffffff) != 116)
			{
				continue;
			}
			TransferLogWire event{};
			memcpy(&event, buffer + LOG_HEADER_SIZE, 116);
			if (event.contractIndex == QNFT_CONTRACT_INDEX && event.type == 0x10000)
			{
				result.push_back(event);
			}
		}
		return result;
	}
} // namespace

std::vector<std::string> uriLogs()
{
	const auto range = qLogger::tx.getLogIdInfo(0, system.tick, 0);
	std::vector<std::string> result;
	for (long long i = 0; i < range.length; ++i)
	{
		char buffer[1024]{};
		qLogger::logBuf.get(buffer, range.startIndex + i);
		uint32 sizeAndType = 0, contract = 0, type = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		memcpy(&contract, buffer + LOG_HEADER_SIZE, 4);
		memcpy(&type, buffer + LOG_HEADER_SIZE + 4, 4);
		if ((sizeAndType >> 24) == CONTRACT_INFORMATION_MESSAGE && (sizeAndType & 0xffffff) == 268 && contract == QNFT_CONTRACT_INDEX &&
		    type == 0x10001)
		{
			result.emplace_back(buffer + LOG_HEADER_SIZE + 8);
		}
	}

	return result;
}

// Independent query wire fixtures keep the API contract explicit.
struct JournalRangeWire
{
	uint64 first, next;
	uint32 capacity, count;
};

TEST_F(NftTest, JournalQueriesArePublicAndRecordMintWithoutLogReader)
{
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][4], nullptr);
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][5], nullptr);
	JournalRangeWire range{};
	callFunction(QNFT_CONTRACT_INDEX, 4, QPI::NoData{}, range);
	EXPECT_EQ(range.first, 1U);
	EXPECT_EQ(range.next, 1U);
	EXPECT_EQ(range.count, 0U);
	EXPECT_EQ(range.capacity, 65536U);
	collection();
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 4, QPI::NoData{}, range);
	EXPECT_EQ(range.first, 1U);
	EXPECT_EQ(range.next, 6U);
	EXPECT_EQ(range.count, 5U);
}

TEST_F(NftTest, JournalPagesPreserveOrderPayloadAndRejectBadCursors)
{
	EXPECT_EQ(sizeof(QNFT::JournalEvent), 408U);
	EXPECT_EQ(sizeof(QNFT::GetEvents_output), 6560U);
	collection();
	ASSERT_EQ(mint(3, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	QNFT::GetEvents_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{0, 2, 0}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 2U);
	EXPECT_EQ(page.cursor, 3U);
	EXPECT_EQ(page.events.get(0).eventId, 1U);
	EXPECT_EQ(page.events.get(0).kind, QNFT::EventKind::Command);
	EXPECT_EQ(page.events.get(1).eventId, 2U);
	EXPECT_EQ(page.events.get(1).kind, QNFT::EventKind::TransferSingle);
	EXPECT_EQ(page.events.get(1).actor, alice);
	EXPECT_EQ(page.events.get(1).from, NULL_ID);
	EXPECT_EQ(page.events.get(1).to, nftId);
	EXPECT_EQ(page.events.get(1).quantity, 3);
	EXPECT_EQ(page.events.get(1).tick, system.tick);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{page.cursor, 16, 0}, page);
	ASSERT_EQ(page.count, 3U);
	EXPECT_EQ(page.cursor, 6U);
	EXPECT_EQ(page.events.get(0).kind, QNFT::EventKind::URI);
	const auto minted = item();
	EXPECT_EQ(memcmp(&page.events.get(0).uri, &minted.metadata.uri, 256), 0);
	EXPECT_EQ(page.events.get(1).from, nftId);
	EXPECT_EQ(page.events.get(1).to, alice);
	const auto firstUri = page.events.get(0).uri;
	ASSERT_EQ(action(6, QNFT::UpdateMetadata_input{metadata("ipfs://new"), 0}).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{3, 1, 0}, page);
	EXPECT_EQ(memcmp(&page.events.get(0).uri, &firstUri, 256), 0);
	const auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto last = state->lastEventId;
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, bob, 99, 0}).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(state->lastEventId, last);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{last + 1, 16, 0}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	for (auto request :
	     {QNFT::GetEvents_input{0, 0, 0}, QNFT::GetEvents_input{0, 17, 0}, QNFT::GetEvents_input{0, 1, 1}, QNFT::GetEvents_input{last + 2, 1, 0}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 5, request, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
}

TEST_F(NftTest, JournalRingOverwritesOldestAndNeverResetsAtEpoch)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	// Seed a full retained interval to test wrap without issuing 65536 transactions.
	for (uint64 i = 1; i <= QNFT_EVENT_CAPACITY; ++i)
	{
		QNFT::JournalEvent record{};
		record.eventId = i;
		record.kind = QNFT::EventKind::Command;
		state->journal.set(i - 1, record);
	}

	state->lastEventId = QNFT_EVENT_CAPACITY;
	ASSERT_EQ(action(4, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_EVENT_CAPACITY + 1ULL);
	EXPECT_EQ(state->journal.get(0).eventId, QNFT_EVENT_CAPACITY + 1ULL);
	QNFT::GetEvents_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{1, 16, 0}, page);
	EXPECT_EQ(page.code, QNFT::Code::NotFound);
	EXPECT_EQ(page.firstEventId, 2U);
	EXPECT_EQ(page.count, 0U);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{QNFT_EVENT_CAPACITY, 16, 0}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 2U);
	EXPECT_EQ(page.events.get(0).eventId, QNFT_EVENT_CAPACITY);
	EXPECT_EQ(page.events.get(1).eventId, QNFT_EVENT_CAPACITY + 1ULL);
	EXPECT_EQ(page.events.get(1).actor, alice);
	EXPECT_EQ(page.events.get(1).to, bob);
	state->epochFees = 0;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_GE(state->lastEventId, QNFT_EVENT_CAPACITY + 1ULL);
}

TEST_F(NftTest, JournalIncludesMarketAndGovernanceWithOriginalActors)
{
	collection();
	ASSERT_EQ(mint(5).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::GetEvents_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{6, 16, 0}, page);
	ASSERT_EQ(page.count, 2U);
	EXPECT_EQ(page.events.get(0).kind, QNFT::EventKind::TransferSingle);
	const auto market = page.events.get(1);
	EXPECT_EQ(market.kind, QNFT::EventKind::Market);
	EXPECT_EQ(market.actor, alice);
	EXPECT_EQ(market.reference, listed.listingId);
	EXPECT_EQ(market.amount, 100);
	EXPECT_EQ(market.quantity, 2);
	EXPECT_EQ(market.from, NULL_ID);
	EXPECT_EQ(market.to, nftId);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto slot = propose(2, 250);
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::GetEvents_input{8, 16, 0}, page);
	ASSERT_EQ(page.count, 2U);
	EXPECT_EQ(page.events.get(0).kind, QNFT::EventKind::Governance);
	EXPECT_EQ(page.events.get(0).actor, alice);
	EXPECT_EQ(page.events.get(0).amount, 250);
	EXPECT_EQ(page.events.get(0).reference, 2U);
	EXPECT_EQ(page.events.get(0).version, static_cast<uint32>(slot));
	EXPECT_EQ(page.events.get(1).action, static_cast<uint32>(QNFT::GovernanceAction::Vote));
	EXPECT_EQ(page.events.get(1).amount, 0);
	EXPECT_EQ(page.events.get(1).reference, 0U);
}

TEST_F(NftTest, JournalCounterExhaustionDoesNotWrapOrBlockOperations)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->lastEventId = QNFT_LAST_EVENT_ID - 1;
	ASSERT_EQ(action(4, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_LAST_EVENT_ID);
	ASSERT_EQ(action(4, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_LAST_EVENT_ID);
	JournalRangeWire range{};
	callFunction(QNFT_CONTRACT_INDEX, 4, QPI::NoData{}, range);
	EXPECT_EQ(range.next, 0xffffffffffffffffULL);
}

TEST_F(NftTest, ShopEventsTrackEscrowAndExportDoesNotChangeOwnership)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 7, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{carol, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 1;
	ex.destination = QX_CONTRACT_INDEX;
	ex.offeredFee = 1000000;
	ASSERT_EQ(run(ex, alice, 1000000).code, QNFT::Code::Ok);
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 3U);
	EXPECT_EQ(logs[0].operatorId, alice);
	EXPECT_EQ(logs[0].from, alice);
	EXPECT_EQ(logs[0].to, nftId);
	EXPECT_EQ(logs[0].quantity, 7);
	EXPECT_EQ(logs[1].operatorId, bob);
	EXPECT_EQ(logs[1].from, nftId);
	EXPECT_EQ(logs[1].to, carol);
	EXPECT_EQ(logs[1].quantity, 3);
	EXPECT_EQ(logs[2].operatorId, alice);
	EXPECT_EQ(logs[2].from, nftId);
	EXPECT_EQ(logs[2].to, alice);
	EXPECT_EQ(logs[2].quantity, 4);
	EXPECT_TRUE(uriLogs().empty());
}

TEST_F(NftTest, TransferEventsDescribeMintOperatorTransferAndBurn)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, carol, 2, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 3, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 6, 0}, bob).code, QNFT::Code::InsufficientBalance);
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 4U);
	EXPECT_EQ(logs[0].operatorId, alice);
	EXPECT_EQ(logs[0].from, NULL_ID);
	EXPECT_EQ(logs[0].to, nftId);
	EXPECT_EQ(logs[0].quantity, 10);
	EXPECT_EQ(logs[1].from, nftId);
	EXPECT_EQ(logs[1].to, alice);
	EXPECT_EQ(logs[1].quantity, 10);
	EXPECT_EQ(logs[2].operatorId, bob);
	EXPECT_EQ(logs[2].from, alice);
	EXPECT_EQ(logs[2].to, carol);
	EXPECT_EQ(logs[2].quantity, 2);
	EXPECT_EQ(logs[3].operatorId, bob);
	EXPECT_EQ(logs[3].from, alice);
	EXPECT_EQ(logs[3].to, NULL_ID);
	EXPECT_EQ(logs[3].quantity, 3);
	for (const auto& log : logs)
	{
		EXPECT_EQ(log.itemId, 0U);
	}
	EXPECT_EQ(balance(0, alice), 5);
	EXPECT_EQ(balance(0, carol), 2);
}

TEST_F(NftTest, MetadataQueryAndUriEventsFollowCommittedMetadata)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	ASSERT_EQ(mint(1, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	const auto changed = metadata("ipfs://collection/{id}.json", 9);
	ASSERT_EQ(action(6, QNFT::UpdateMetadata_input{changed, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(8, QNFT::FreezeMetadata_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(6, QNFT::UpdateMetadata_input{metadata("ipfs://rejected"), 0}).code, QNFT::Code::Frozen);
	QNFT::GetItem_output result{};
	EXPECT_EQ(sizeof(result), 512U);
	EXPECT_EQ(offsetof(QNFT::GetItem_output, code), 504U);
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][3], nullptr);
	callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetItem_input{0}, result);
	EXPECT_EQ(result.code, QNFT::Code::Ok);
	EXPECT_EQ(result.item.metadata.contentHash, changed.contentHash);
	EXPECT_EQ(memcmp(&result.item.metadata.uri, &changed.uri, 256), 0);
	qLogger::updateTick(system.tick);
	const auto range = qLogger::tx.getLogIdInfo(0, system.tick, 0);
	unsigned count = 0;
	for (long long i = 0; i < range.length; ++i)
	{
		char buffer[1024]{};
		qLogger::logBuf.get(buffer, range.startIndex + i);
		uint32 sizeAndType = 0, contract = 0, type = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		memcpy(&contract, buffer + LOG_HEADER_SIZE, 4);
		memcpy(&type, buffer + LOG_HEADER_SIZE + 4, 4);
		if ((sizeAndType >> 24) != CONTRACT_INFORMATION_MESSAGE || contract != QNFT_CONTRACT_INDEX || type != 0x10001)
		{
			continue;
		}
		ASSERT_EQ(sizeAndType & 0xffffff, 268U);
		if (++count == 2)
		{
			EXPECT_EQ(memcmp(buffer + LOG_HEADER_SIZE + 8, &result.item.metadata.uri, 256), 0);
		}
	}
	EXPECT_EQ(count, 2U);
	callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetItem_input{999}, result);
	EXPECT_EQ(result.code, QNFT::Code::NotFound);
}

TEST_F(NftTest, RejectedCommandsAndShopActionsEmitOneErrorEach)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	qLogger::registerNewTx(system.tick, 1);
	QNFT::Transfer_input transfer{};
	transfer.owner = alice;
	transfer.receiver = bob;
	transfer.quantity = 1;
	transfer.itemId = 999;
	EXPECT_EQ(action(9, transfer).code, QNFT::Code::NotFound);
	QNFT::CancelSale_input cancel{};
	cancel.listingId = 987;
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, cancel, cancelled, alice, 0));
	EXPECT_EQ(cancelled.code, QNFT::Code::NotFound);
	QNFT::ListForSale_input list{};
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, list, listed, alice, 0));
	QNFT::Buy_input buy{};
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, buy, bought, bob, 0));
	QNFT::Export_input exportInput{};
	exportInput.owner = alice;
	exportInput.quantity = 1;
	exportInput.destination = 999;
	EXPECT_EQ(action(13, exportInput).code, QNFT::Code::ExternalFailure);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	qLogger::updateTick(system.tick);
	const auto range = qLogger::tx.getLogIdInfo(0, system.tick, 1);
	unsigned errors = 0;
	for (long long i = 0; i < range.length; ++i)
	{
		char buffer[1024]{};
		qLogger::logBuf.get(buffer, range.startIndex + i);
		unsigned sizeAndType = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		if ((sizeAndType >> 24) != CONTRACT_ERROR_MESSAGE)
		{
			continue;
		}
		unsigned contract = 0;
		memcpy(&contract, buffer + LOG_HEADER_SIZE, 4);
		EXPECT_EQ(contract, QNFT_CONTRACT_INDEX);
		ASSERT_EQ(sizeAndType & 0xffffffU, 68U);
		QNFT::FailureEvent event{};
		memcpy(&event, buffer + LOG_HEADER_SIZE, 68);
		const QNFT::Code expected[] = {QNFT::Code::NotFound,     QNFT::Code::NotFound,        QNFT::Code::InvalidInput,
		                               QNFT::Code::InvalidInput, QNFT::Code::ExternalFailure, QNFT::Code::Conflict};
		ASSERT_LT(errors, 6U);
		EXPECT_EQ(event.code, expected[errors]);
		if (errors == 0)
		{
			EXPECT_EQ(event.operation, QNFT::Operation::Transfer);
			EXPECT_EQ(event.itemId, 999U);
		}
		if (errors == 1)
		{
			EXPECT_EQ(event.marketAction, QNFT::MarketAction::Cancel);
			EXPECT_EQ(event.listingId, 987U);
		}
		if (errors == 4)
		{
			EXPECT_EQ(event.operation, QNFT::Operation::Export);
		}
		if (errors == 5)
		{
			EXPECT_EQ(event.marketAction, QNFT::MarketAction::Distribution);
		}
		if (errors < 5)
		{
			EXPECT_EQ(event.actor, errors == 3 ? bob : alice);
		}
		++errors;
	}
	EXPECT_EQ(errors, 6U);
	const auto successRange = qLogger::tx.getLogIdInfo(0, system.tick, 0);
	for (long long i = 0; i < successRange.length; ++i)
	{
		char buffer[1024]{};
		qLogger::logBuf.get(buffer, successRange.startIndex + i);
		unsigned sizeAndType = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		EXPECT_NE(sizeAndType >> 24, CONTRACT_ERROR_MESSAGE);
	}
}

TEST_F(NftTest, EpochDistributionUsesInitializedStatePercentagesAndPreservesRounding)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->developerFeeBPS, 2000U);
	EXPECT_EQ(state->shareholderFeeBPS, 6000U);
	EXPECT_EQ(state->burnFeeBPS, 2000U);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	state->developerFeeBPS = 2500;
	state->shareholderFeeBPS = 2500;
	state->burnFeeBPS = 5000;
	state->collectionFee = 10003;
	QNFT::CreateCollection_input request{metadata(), 1000, 10};
	ASSERT_EQ(action(1, request, alice, 10003).code, QNFT::Code::Ok);
	const auto shareholderBefore = getBalance(alice);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->epochFees, 0);
	EXPECT_EQ(state->developerPending, 2500);
	EXPECT_EQ(state->shareholderCarry, 472);

	EXPECT_EQ(getBalance(alice), shareholderBefore + 2028);
	EXPECT_EQ(getBalance(nftId), 2972); // 5003 burned, 2028 dividends paid.
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(nftId), 2972);
}

TEST_F(NftTest, InvalidEpochPercentagesKeepAllFundsAvailableForRetry)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	const uint32 invalid[][3] = {{0, 0, 0}, {2000, 5000, 2999}, {2000, 5000, 3001}, {10001, 0, 0}, {0xffffffffU, 5001, 5000}};
	for (const auto& split : invalid)
	{
		state->developerFeeBPS = split[0];
		state->shareholderFeeBPS = split[1];
		state->burnFeeBPS = split[2];
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
		EXPECT_EQ(state->epochFees, 1000000);
		EXPECT_EQ(state->developerPending, 0);
		EXPECT_EQ(state->shareholderCarry, 0);
		EXPECT_EQ(state->lastDistributionEpoch, 0U);

		EXPECT_EQ(getBalance(nftId), 1000000);
	}
	state->developerFeeBPS = 0;
	state->shareholderFeeBPS = 0;
	state->burnFeeBPS = 10000;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->epochFees, 0);
	EXPECT_EQ(getBalance(nftId), 0);
	EXPECT_EQ(state->lastDistributionEpoch, static_cast<uint32>(system.epoch));
	expectQuCollateral();
}

TEST_F(NftTest, ShopEpochCarriesRoundingAndSnapshotsCurrentShareholders)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	const auto aliceBefore = getBalance(alice), bobBefore = getBalance(bob);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	QNFT::GetMarket_output market{};
	callFunction(QNFT_CONTRACT_INDEX, 13, QNFT::GetMarket_input{}, market);
	EXPECT_EQ(market.epochFees, 0);
	EXPECT_EQ(market.developerPending, 400000);
	EXPECT_EQ(market.shareholderCarry, 100);
	QX::TransferShareOwnershipAndPossession_input move{NULL_ID, bob, QNFT_SHARE_NAME, 100};
	QX::TransferShareOwnershipAndPossession_output moved{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 2, move, moved, alice, 100));
	ASSERT_EQ(moved.transferredNumberOfShares, 100);
	++system.epoch;
	collection();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	callFunction(QNFT_CONTRACT_INDEX, 13, QNFT::GetMarket_input{}, market);
	EXPECT_EQ(market.developerPending, 600000);
	EXPECT_EQ(market.shareholderCarry, 488);
	EXPECT_EQ(getBalance(alice), aliceBefore - 100 - 1000000 + 1710812);
	EXPECT_EQ(getBalance(bob), bobBefore + 88700);
}

TEST_F(NftTest, ShopPaysRoyaltyWhenSellerProceedsAreZero)
{
	collection();
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.quantity = 1;
	in.receiver = alice;
	in.metadata = metadata();
	in.royaltyBps = 9800;
	in.royaltyReceiver = carol;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 1, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 100, 1, 0}, bought, bob, 100));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), sellerBefore);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 98);
	EXPECT_EQ(bought.fee, 2);
	EXPECT_EQ(bought.royalty, 98);
	expectQuCollateral();
}

TEST_F(NftTest, EpochPaysDeveloperAndShareholdersDirectlyWithoutWithdrawals)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = carol;
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	const auto developerBefore = getBalance(carol), shareholderBefore = getBalance(alice);
	const auto reserveBefore = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(carol), developerBefore + 200000);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 599612);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserveBefore + 200000);

	EXPECT_EQ(state->developerPending, 0);
	EXPECT_EQ(state->shareholderCarry, 388);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(carol), developerBefore + 200000);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 599612);
	expectQuCollateral();
}

TEST_F(NftTest, FailedNativePayoutsRetryNextEpochWithoutRepeatingBurn)
{
	INIT_CONTRACT(TESTEXA);
	// Keep the recipient alive in both the failed-payment epoch and the retry epoch.
	system.epoch = contractDescriptions[TESTEXA_CONTRACT_INDEX].destructionEpoch - 2;
	const id developer(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = developer;
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	const auto reserveBefore = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	const auto shareholderBefore = getBalance(alice), developerBefore = getBalance(developer);
	// Exercise real QPI rejection of dividends and SC transfers in incoming-transfer context.
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	contractCallbacksRunning = 0;
	EXPECT_EQ(state->epochFees, 0);
	EXPECT_EQ(state->developerPending, 200000);
	EXPECT_EQ(state->shareholderCarry, 600000);
	EXPECT_EQ(getBalance(alice), shareholderBefore);
	EXPECT_EQ(getBalance(developer), developerBefore);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserveBefore + 200000);
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(developer), developerBefore);
	++system.epoch;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(developer), developerBefore + 200000);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 599612);
	EXPECT_EQ(state->developerPending, 0);
	EXPECT_EQ(state->shareholderCarry, 388);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserveBefore + 200000);
	expectQuCollateral();
}

TEST_F(NftTest, DividendRetryDoesNotPaySuccessfulDeveloperAgain)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = carol;
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	const auto developerBefore = getBalance(carol), shareholderBefore = getBalance(alice);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	contractCallbacksRunning = 0;
	EXPECT_EQ(getBalance(carol), developerBefore + 200000);
	EXPECT_EQ(state->developerPending, 0);
	EXPECT_EQ(state->shareholderCarry, 600000);
	++system.epoch;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(carol), developerBefore + 200000);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 599612);
	EXPECT_EQ(state->shareholderCarry, 388);
	expectQuCollateral();
}

TEST_F(NftTest, DeferredDeveloperRevenueIsPaidAutomaticallyOnceRecipientIsConfigured)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->developerPending, 400000);
	const auto developerBefore = getBalance(carol), shareholderBefore = getBalance(alice);
	state->developer = carol; // Model deployment configuration, not a public setter.
	++system.epoch;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(carol), developerBefore + 400000);
	EXPECT_EQ(getBalance(alice), shareholderBefore);
	EXPECT_EQ(state->developerPending, 0);

	expectQuCollateral();
}

TEST_F(NftTest, ShopSmallPricesRoundFeesAndRoyaltiesDownWithoutLosingQu)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	struct Row
	{
		sint64 price;
		sint64 fee;
		sint64 royalty;
	};
	for (const Row row : {Row{1, 0, 0}, Row{49, 0, 2}, Row{50, 1, 2}, Row{51, 1, 2}, Row{99, 1, 4}, Row{100, 2, 5}, Row{101, 2, 5}})
	{
		QNFT::ListForSale_output listed{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{row.price, 1, 0, 0}, listed, alice, 0));
		ASSERT_EQ(listed.code, QNFT::Code::Ok);
		QNFT::Buy_output bought{};
		ASSERT_TRUE(
		    invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, row.price, 1, 0}, bought, bob, row.price + 13));
		ASSERT_EQ(bought.code, QNFT::Code::Ok);
		EXPECT_EQ(bought.fee, row.fee);
		EXPECT_EQ(bought.royalty, row.royalty);
		QNFT::GetListing_output closed{};
		callFunction(QNFT_CONTRACT_INDEX, 11, QNFT::GetListing_input{listed.listingId}, closed);
		EXPECT_EQ(closed.code, QNFT::Code::NotFound);
	}
	EXPECT_EQ(getBalance(alice), sellerBefore + 424);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 20);
	QNFT::GetMarket_output market{};
	callFunction(QNFT_CONTRACT_INDEX, 13, QNFT::GetMarket_input{}, market);
	EXPECT_EQ(market.epochFees, 2000007);
	EXPECT_EQ(getBalance(nftId), 2000007);
	EXPECT_EQ(balance(0, bob), 7);
	EXPECT_EQ(balance(0, nftId), 0);
}

TEST_F(NftTest, OperatorBurnUsesOwnerBalanceAndPreservesApproval)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 11, 0}, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 3, 0}, carol).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 3, 0}, bob, 17).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 2, 0}, bob, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 5);
	EXPECT_EQ(item().burned, 5);
	EXPECT_EQ(getBalance(bob), before);
	ASSERT_EQ(action(11, QNFT::Revoke_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Unauthorized);
	expectQuCollateral();
}

TEST_F(NftTest, OperatorExportPreservesOwnershipAndChargesOperator)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	manager->preAcquireSharesOutput.requestedFee = 7;
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->exportFee = 10;
	OperationCommand approval{};
	approval.operation = QNFT::Operation::Approve;
	approval.receiver = bob;
	ASSERT_EQ(run(approval).code, QNFT::Code::Ok);
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 11;
	ex.destination = TESTEXA_CONTRACT_INDEX;
	ex.offeredFee = 7;
	const auto ownerBefore = getBalance(alice), operatorBefore = getBalance(bob), pool = state->epochFees;
	EXPECT_EQ(run(ex, bob, 20).code, QNFT::Code::InsufficientBalance);
	ex.quantity = 3;
	EXPECT_EQ(run(ex, carol, 20).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(run(ex, bob, 16).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(manager->postAcquireShareCounter, 0);
	ASSERT_EQ(run(ex, bob, 20).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 7);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 3);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(getBalance(alice), ownerBefore);
	EXPECT_EQ(getBalance(bob), operatorBefore - 17);
	EXPECT_EQ(state->epochFees, pool + 10);
	EXPECT_EQ(run(ex, bob, 20).code, QNFT::Code::Ok);
	EXPECT_EQ(manager->postAcquireShareCounter, 2);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 6);
	EXPECT_EQ(getBalance(bob), operatorBefore - 34);
	expectQuCollateral();
}

TEST_F(NftTest, ItemApprovalPersistsAcrossTimeAndRequiresCurrentOwnerBalance)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 8, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 3, 0}, bob).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(13, QNFT::Export_input{alice, 3, 0, 0, TESTEXA_CONTRACT_INDEX}, bob).code, QNFT::Code::InsufficientBalance);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Ok);
	utcTime.Year = 2030;
	updateQpiTime();
	EXPECT_EQ(action(13, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 1);
	EXPECT_EQ(item().burned, 9);
	expectQuCollateral();
}

TEST_F(NftTest, FailedDelegatedExportPreservesApprovalAndRefundsUnspentReward)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	OperationCommand approval{};
	approval.operation = QNFT::Operation::Approve;
	approval.receiver = bob;
	ASSERT_EQ(run(approval).code, QNFT::Code::Ok);
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 3;
	ex.destination = TESTEXA_CONTRACT_INDEX;
	const auto before = getBalance(bob);
	EXPECT_EQ(run(ex, bob, 17).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 10);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(run(ex, bob, 17).code, QNFT::Code::ExternalFailure);
	QNFT::GetApproval_output approvalAfter{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approvalAfter);
	EXPECT_EQ(approvalAfter.approval.operatorId, bob);
	expectQuCollateral();
}

// Burning does not consume a transfer permission or bypass the remaining balance.
TEST_F(NftTest, PartialBurnPreservesApprovalAndOperatorStillNeedsBalance)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	QNFT::Approve_input approval{bob, 0};
	ASSERT_EQ(action(10, approval).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 11, 0}).code, QNFT::Code::InsufficientBalance);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 4, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 6);
	EXPECT_EQ(item().burned, 4);
	QNFT::GetApproval_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, remaining);
	EXPECT_EQ(remaining.approval.operatorId, bob);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, carol, 7, 0}, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice), 6);
	EXPECT_EQ(balance(0, carol), 0);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, carol, 3, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 3);
	EXPECT_EQ(balance(0, carol), 3);
	expectQuCollateral();
}

TEST_F(NftTest, FullBalanceBurnPreservesApprovalForSubsequentlyReceivedUnits)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 4, 0}).code, QNFT::Code::Ok);
	QNFT::Approve_input approval{carol, 0};
	ASSERT_EQ(action(10, approval).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 6, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 0);
	QNFT::GetApproval_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, remaining);
	EXPECT_EQ(remaining.approval.operatorId, carol);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, bob, 1, 0}, carol).code, QNFT::Code::InsufficientBalance);
	ASSERT_EQ(action(9, QNFT::Transfer_input{bob, alice, 2, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 1, 0}, carol).code, QNFT::Code::Ok);
	// A fresh approval remains explicitly revocable after the final unit is burned.
	ASSERT_EQ(action(10, QNFT::Approve_input{carol, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::Revoke_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().burned, 7);
	EXPECT_EQ(balance(0, alice), 0);
	EXPECT_EQ(balance(0, bob), 3);
	expectQuCollateral();
}

// Owners can approve, burn and export their own holdings without delegated permission.
TEST_F(NftTest, OwnerOperationsUseSelectedHoldings)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	QNFT::Approve_input approval{};
	approval.receiver = carol;
	QNFT::Burn_input burn{};
	burn.owner = bob;
	burn.quantity = 1;
	QNFT::Export_input exportInput{};
	exportInput.owner = bob;
	exportInput.quantity = 1;
	exportInput.destination = QX_CONTRACT_INDEX;
	exportInput.offeredFee = 1000000;
	const auto bobBefore = getBalance(bob);
	EXPECT_EQ(action(10, approval, bob, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, burn, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(13, exportInput, bob, 1000000).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(getBalance(bob), bobBefore);
	EXPECT_EQ(balance(0, alice), 100);

	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 5, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, approval, bob).code, QNFT::Code::Ok);
	QNFT::GetApproval_output approved{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{bob, 0}, approved);
	EXPECT_EQ(approved.approval.operatorId, carol);
	ASSERT_EQ(action(12, burn, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(13, exportInput, bob, 1000000).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 95);
	EXPECT_EQ(balance(0, bob), 4);
	EXPECT_EQ(balance(0, bob, QNFT_CONTRACT_INDEX), 3);
	EXPECT_EQ(balance(0, bob, QX_CONTRACT_INDEX), 1);
	EXPECT_EQ(item().burned, 1);
	expectQuCollateral();
}

TEST_F(NftTest, FixedEditionSplitsWithoutIssuingAnotherAsset)
{
	collection();
	auto out = mint();
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.assetName, 0x3030303030304eULL);
	EXPECT_EQ(balance(0, alice), 100);
	OperationCommand in{};
	in.operation = QNFT::Operation::Transfer;
	in.itemId = 0;
	in.quantity = 3;
	in.owner = alice;
	in.receiver = bob;
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 97);
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(item().supply, 100);
	EXPECT_EQ(item().creator, alice);
}

TEST_F(NftTest, RejectsUnauthorizedOverdrawAndRefundsFees)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::Transfer;
	in.quantity = 1;
	in.owner = alice;
	in.receiver = bob;
	auto before = getBalance(carol);
	EXPECT_EQ(run(in, carol, 123).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(carol), before);
	in.quantity = 101;
	EXPECT_EQ(run(in).code, QNFT::Code::InsufficientBalance);
	in.quantity = 0;
	EXPECT_EQ(run(in).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(balance(0, bob), 0);
}

TEST_F(NftTest, CollectionLimitsAreCumulativeAndAdminTransferRequiresAcceptance)
{
	collection(100, 2);
	ASSERT_EQ(mint(60).code, QNFT::Code::Ok);
	EXPECT_EQ(mint(41).code, QNFT::Code::Capacity);
	OperationCommand in{};
	in.operation = QNFT::Operation::NominateAdmin;
	in.receiver = bob;
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::AcceptAdmin;
	EXPECT_EQ(run(in, carol).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Unauthorized);
}

TEST_F(NftTest, MetadataFreezeAndRoyaltyOnTotalSalePrice)
{
	collection();
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::UpdateMetadata;
	in.metadata = metadata("ipfs://changed", 9);
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(item().metadataVersion, 2);
	in.operation = QNFT::Operation::FreezeMetadata;
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::UpdateMetadata;
	EXPECT_EQ(run(in).code, QNFT::Code::Frozen);
	EXPECT_EQ(item().royaltyReceiver, carol);
	EXPECT_EQ(static_cast<unsigned>(item().royaltyBps), 500U);
}

TEST_F(NftTest, ExpandedRegistryLastSlotsAndListingCursorBoundaries)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->collectionCount = QNFT_MAX_COLLECTIONS - 1;
	QNFT::CreateCollection_input request{metadata(), MAX_AMOUNT, QNFT_MAX_ITEMS};
	const auto created = action(1, request, alice, QNFT_COLLECTION_FEE);
	ASSERT_EQ(created.code, QNFT::Code::Ok);
	EXPECT_EQ(created.itemId, QNFT_MAX_COLLECTIONS - 1);
	EXPECT_EQ(action(1, request, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Capacity);
	state->itemCount = QNFT_MAX_ITEMS - 1;
	OperationCommand mintInput{};
	mintInput.operation = QNFT::Operation::Mint;
	mintInput.quantity = 100;
	mintInput.receiver = alice;
	mintInput.collectionId = QNFT_MAX_COLLECTIONS - 1;
	mintInput.metadata = metadata();
	const auto minted = run(mintInput, alice, TEST_MINT_FEE);
	ASSERT_EQ(minted.code, QNFT::Code::Ok);
	EXPECT_EQ(minted.itemId, QNFT_MAX_ITEMS - 1);
	EXPECT_EQ(balance(QNFT_MAX_ITEMS - 1, alice), 100);
	EXPECT_EQ(run(mintInput, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	QNFT::ListListings_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::ListListings_input{QNFT_LISTING_SLOTS, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, QNFT_LISTING_SLOTS);
	callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::ListListings_input{QNFT_LISTING_SLOTS + 1, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
	expectQuCollateral();
}

TEST_F(NftTest, StateFitsApprovedBudget)
{
	EXPECT_LT(sizeof(QNFT::StateData), 128ULL * 1024 * 1024);
}

TEST_F(NftTest, ForeignIssuerWithRegisteredNftNameCannotEnterQnft)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	const auto name = item().assetName;
	OperationCommand approve{};
	approve.operation = QNFT::Operation::Approve;
	approve.owner = alice;
	approve.receiver = bob;
	approve.quantity = 5;

	ASSERT_EQ(run(approve).code, QNFT::Code::Ok);
	// Issue a real, distinct asset with an identical name and alice as its issuer/owner.
	QX::IssueAsset_input issue{name, 7, 0, 0};
	QX::IssueAsset_output issued{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 1, issue, issued, alice, 1000000000));
	ASSERT_EQ(issued.issuedNumberOfShares, 7);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto fees = state->epochFees;
	const auto ownerQu = getBalance(alice), contractQu = getBalance(nftId);
	QX::TransferShareManagementRights_input incoming{Asset{alice, name}, 7, QNFT_CONTRACT_INDEX};
	QX::TransferShareManagementRights_output result{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 9, incoming, result, alice, 0));
	EXPECT_EQ(result.transferredNumberOfShares, 0);
	EXPECT_EQ(numberOfPossessedShares(name, alice, alice, alice, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 7);
	EXPECT_EQ(numberOfPossessedShares(name, alice, alice, alice, QNFT_CONTRACT_INDEX, QNFT_CONTRACT_INDEX), 0);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(item().supply, 100);
	EXPECT_EQ(item().burned, 0);
	EXPECT_EQ(state->itemCount, 1);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(alice), ownerQu);
	EXPECT_EQ(getBalance(nftId), contractQu);
	QNFT::GetApproval_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approval);
	EXPECT_EQ(approval.approval.operatorId, bob);
}

TEST_F(NftTest, ApprovalSurvivesTransfersAndQxRoundTrip)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::Approve;
	in.owner = alice;
	in.receiver = bob;
	in.quantity = 5;

	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::Transfer;
	in.receiver = carol;
	in.quantity = 101;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::InsufficientBalance);
	in.quantity = 2;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, carol), 2);
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::Approve;
	in.receiver = bob;
	in.quantity = 5;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::Export;
	in.destination = QX_CONTRACT_INDEX;
	in.offeredFee = 1000000;
	ASSERT_EQ(run(in, alice, 1000000).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, QX_CONTRACT_INDEX), 5);
	QX::TransferShareManagementRights_input back{Asset{nftId, item().assetName}, 5, QNFT_CONTRACT_INDEX};
	QX::TransferShareManagementRights_output out{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 9, back, out, alice, 1000000));
	ASSERT_EQ(out.transferredNumberOfShares, 5);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 96);
	in.operation = QNFT::Operation::Transfer;
	in.receiver = carol;
	in.quantity = 1;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
}

TEST_F(NftTest, QxSellsOnlyRequestedEditionQuantity)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::Export;
	in.owner = alice;
	in.quantity = 10;
	in.destination = QX_CONTRACT_INDEX;
	in.offeredFee = 1000000;
	ASSERT_EQ(run(in, alice, 1000000).code, QNFT::Code::Ok);
	QX::AddToAskOrder_input ask{nftId, item().assetName, 100, 10};
	QX::AddToAskOrder_output askOut{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 5, ask, askOut, alice, 0));
	ASSERT_EQ(askOut.addedNumberOfShares, 10);
	QX::AddToBidOrder_input bid{nftId, item().assetName, 100, 3};
	QX::AddToBidOrder_output bidOut{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 6, bid, bidOut, bob, 300));
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 90);
}

TEST_F(NftTest, RevealChecksCommitmentAndBurnDoesNotRestoreMintQuota)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection(100, 1);
	auto revealed = metadata("ipfs://revealed", 123);
	id commitment{};
	KangarooTwelve(&revealed, sizeof(revealed), &commitment, sizeof(commitment));
	ASSERT_EQ(mint(100, QNFT::Policy::Reveal, commitment).code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::RevealMetadata;
	in.metadata = metadata("ipfs://wrong");
	EXPECT_EQ(run(in).code, QNFT::Code::InvalidInput);
	in.metadata = revealed;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(item().frozen, 1);
	in.operation = QNFT::Operation::Burn;
	in.owner = alice;
	in.quantity = 4;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(item().burned, 4);
	EXPECT_EQ(balance(0, alice), 96);
	EXPECT_EQ(mint(1).code, QNFT::Code::Capacity);
	qLogger::updateTick(system.tick);
	const auto uris = uriLogs();
	ASSERT_EQ(uris.size(), 2U);
	EXPECT_EQ(uris[1], "ipfs://revealed");
}

TEST_F(NftTest, QbayImportIsPermanentAndCannotReturnOriginal)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	// Seed a pre-existing QBay original; all custody changes use real public procedures.
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.royalty = 5;
	original.URI.set(0, 'Q');
	original.URI.set(1, 'm');
	original.URI.set(2, '1');
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	OperationCommand in{};
	in.operation = QNFT::Operation::PrepareImport;
	in.receiver = alice;
	in.qbayId = 0;
	auto prepared = run(in, alice, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Unauthorized);
	QBAY::transfer_output dep{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, dep, alice, 0));
	ASSERT_EQ(dep.returnCode, QBAY::LogInfo::success);
	in.operation = QNFT::Operation::CancelImport;
	in.itemId = prepared.itemId;
	EXPECT_EQ(run(in).code, QNFT::Code::Conflict);
	in.operation = QNFT::Operation::FinalizeImport;
	ASSERT_EQ(run(in, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto wrapped = item();
	EXPECT_EQ(wrapped.creator, carol);
	EXPECT_EQ(wrapped.supply, 1);
	EXPECT_EQ(balance(0, alice), 1);
	EXPECT_EQ(run(in, bob, TEST_MINT_FEE).code, QNFT::Code::NotFound);
	in.operation = QNFT::Operation::Transfer;
	in.owner = alice;
	in.receiver = bob;
	in.quantity = 1;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(item().bridge, QNFT::Bridge::Permanent);
	EXPECT_EQ(qbay->NFTs.get(0).possessor, nftId);
	EXPECT_EQ(balance(0, bob), 1);

	in.operation = QNFT::Operation::PrepareImport;
	in.receiver = bob;
	EXPECT_NE(run(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(item().assetName, wrapped.assetName);
	EXPECT_EQ(item().supply, 1);
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 3U);
	EXPECT_EQ(logs[0].operatorId, bob);
	EXPECT_EQ(logs[0].from, NULL_ID);
	EXPECT_EQ(logs[0].to, nftId);
	EXPECT_EQ(logs[1].from, nftId);
	EXPECT_EQ(logs[1].to, alice);
	EXPECT_EQ(uriLogs(), std::vector<std::string>{"Qm1"});
}

TEST_F(NftTest, NostStandardAuctionDeliversEditionLot)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	callSystemProcedure(NOST_CONTRACT_INDEX, END_TICK);
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 3;
	ex.destination = NOST_CONTRACT_INDEX;
	ASSERT_EQ(run(ex).code, QNFT::Code::Ok);
	NOST::CreateAuction_input auction{};
	const char* cid = "bafybeigdyrzt2a3x4m5n6p7qrstuvwx234567abcdefghijklmnopqrst";
	for (uint32 i = 0; cid[i]; ++i)
	{
		auction.metadataIpfsCid.set(i, cid[i]);
	}
	auction.auctionLotItems.set(0, NOST::AuctionAssetEntry{Asset{nftId, item().assetName}, 3});
	auction.minimumPurchaseQuantity = 1;
	auction.initialPrice = NOST_STANDARD_MIN_PRICE;
	auction.salePrice = NOST_STANDARD_MIN_PRICE;
	auction.minimumBidIncrement = NOST_STANDARD_MIN_BID_INCREMENT;
	auction.buyNowPrice = NOST_STANDARD_MIN_PRICE;
	auction.durationDays = 1;
	auction.auctionType = static_cast<uint8>(NOST::EAuctionType::Standard);
	auction.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Public);
	NOST::CreateAuction_output created{};
	ASSERT_TRUE(invokeUserProcedure(NOST_CONTRACT_INDEX, 1, auction, created, alice, NOST_PUBLIC_AUCTION_CREATION_FEE));
	ASSERT_EQ(created.errorCode, NOST::EAuctionError::Success);
	NOST::PlaceBid_input bid{created.auctionIndex, 1, NOST_STANDARD_MIN_PRICE};
	NOST::PlaceBid_output bought{};
	ASSERT_TRUE(invokeUserProcedure(NOST_CONTRACT_INDEX, 2, bid, bought, bob, NOST_STANDARD_MIN_PRICE));
	ASSERT_EQ(bought.errorCode, NOST::EAuctionError::Success);
	EXPECT_EQ(balance(0, bob, NOST_CONTRACT_INDEX), 3);
	EXPECT_EQ(balance(0, alice), 97);
}

TEST_F(NftTest, InvalidArgumentsTakePrecedenceOverMissingRecords)
{
	const auto before = getBalance(alice);
	for (auto operation : {QNFT::Operation::Transfer, QNFT::Operation::Burn, QNFT::Operation::Approve, QNFT::Operation::Export,
	                       QNFT::Operation::NominateAdmin, QNFT::Operation::Mint, QNFT::Operation::UpdateMetadata, QNFT::Operation::RevealMetadata})
	{
		OperationCommand in{};
		in.operation = operation;
		in.itemId = 0xffffffffU;
		in.collectionId = 0xffffffffU;
		EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput) << static_cast<unsigned>(operation);
		EXPECT_EQ(getBalance(alice), before);
	}

	expectQuCollateral();
}

TEST_F(NftTest, InvalidMetadataFeeAndCapacityLeaveStateAndBalancesUnchanged)
{
	collection();
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.quantity = 1;
	in.receiver = alice;
	in.metadata = metadata();
	in.metadata.uri.set(250, 'X');
	auto before = getBalance(alice);
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(alice), before);
	in.metadata = metadata();
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE - 1).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(getBalance(alice), before);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->itemCount, 0);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
	state->itemCount = QNFT_MAX_ITEMS;
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	EXPECT_EQ(getBalance(alice), before);
}

TEST_F(NftTest, PersistentAuthorizationAndInvalidManagementDestination)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::Approve;
	in.owner = alice;
	in.receiver = bob;
	in.quantity = 1;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	utcTime.Day = 18;
	updateQpiTime();
	in.operation = QNFT::Operation::Transfer;
	in.receiver = carol;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::Revoke;
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
	// A nonexistent manager is rejected by core without moving the asset.
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 1;
	ex.destination = 999;
	EXPECT_EQ(run(ex).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(balance(0, alice), 99);
}

TEST(NftAbi, WireLayoutIsStable)
{
	EXPECT_EQ(sizeof(QNFT::ListForSale_input), 24);
	EXPECT_EQ(sizeof(QNFT::ListForSale_output), 16);
	EXPECT_EQ(sizeof(QNFT::Buy_input), 64);
	EXPECT_EQ(sizeof(QNFT::Buy_output), 32);
	EXPECT_EQ(sizeof(QNFT::GetListing_output), 72);
	EXPECT_EQ(sizeof(QNFT::GetMarket_output), 80);
	EXPECT_EQ(sizeof(QNFT::ListListings_output), 272);
	EXPECT_EQ(offsetof(QNFT::StateData, listings), offsetof(QNFT::StateData, contractRefunds) + sizeof(QNFT::StateData::contractRefunds));
	EXPECT_EQ(offsetof(QNFT::MarketEvent, _terminator), 100);
	EXPECT_EQ(sizeof(QNFT::Operation), 2);
	EXPECT_EQ(sizeof(QNFT::Code), 4);
	EXPECT_EQ(sizeof(QNFT::Policy), 1);
	EXPECT_EQ(sizeof(QNFT::Origin), 1);
	EXPECT_EQ(sizeof(QNFT::Bridge), 1);
	EXPECT_EQ(static_cast<uint32>(QNFT::Operation::CreateCollection), 1U);
	EXPECT_EQ(offsetof(QNFT::Event, type), 4);
	EXPECT_EQ(offsetof(QNFT::Event, typeReserved), 6);
	EXPECT_EQ(offsetof(QNFT::Event, actor), 8);
	EXPECT_EQ(sizeof(QNFT::Metadata), 288);
	EXPECT_EQ(sizeof(QNFT::OperationResult), 24);
	EXPECT_EQ(offsetof(QNFT::Event, _terminator), 88);
	EXPECT_EQ(sizeof(QNFT::Item), 504);
	EXPECT_EQ(sizeof(QNFT::GetItem_output), 512);
	EXPECT_EQ(sizeof(QNFT::GetCollection_output), 416);
	EXPECT_EQ(sizeof(QNFT::GetApproval_input), 40);
	EXPECT_EQ(sizeof(QNFT::ListRefunds_input), 8);
	EXPECT_EQ(sizeof(QNFT::ListRefunds_output), 656);
	EXPECT_EQ(offsetof(QNFT::StateData, contractRefunds), 79118376);
	EXPECT_EQ(sizeof(NOST::CreateAuction_input), 1008);
	QNFT::GetItem_output reply{};
	reply.item.creator = alice;
	reply.item.supply = 100;
	reply.item.metadata = metadata();
	std::ofstream("abi-item.bin", std::ios::binary).write(reinterpret_cast<const char*>(&reply), sizeof(reply));
	EXPECT_EQ(sizeof(QNFT::GetFeeSchedule_output), 88);
	EXPECT_EQ(offsetof(QNFT::GetFeeSchedule_output, shopFeeBPS), 80);
	std::cout << "QNFT state bytes: " << sizeof(QNFT::StateData) << '\n';
}

TEST_F(NftTest, ImportCancellationDeletesBindingAndNewOwnerCreatesFreshRequest)
{
	collection();
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	OperationCommand in{};
	in.operation = QNFT::Operation::PrepareImport;
	in.receiver = alice;
	EXPECT_EQ(run(in).code, QNFT::Code::InsufficientFee);
	auto before = getBalance(alice);
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - TEST_MINT_FEE);
	QBAY::transfer_output transferred{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{bob, 0}, transferred, alice, 0));
	ASSERT_EQ(transferred.returnCode, QBAY::LogInfo::success);
	in.operation = QNFT::Operation::CancelImport;
	EXPECT_EQ(run(in, carol).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(run(in, bob).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_FALSE(state->imports.contains(0));
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
	EXPECT_EQ(state->items.get(0).reservedRecipient, NULL_ID);
	OperationCommand create{};
	create.operation = QNFT::Operation::CreateCollection;
	create.metadata = metadata();
	create.maxItems = 1;
	create.maxUnits = 1;
	const auto created = run(create, bob, TEST_MINT_FEE);
	ASSERT_EQ(created.code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::PrepareImport;
	in.receiver = bob;
	in.collectionId = created.itemId;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::InsufficientFee);
	const auto prepared = run(in, bob, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	EXPECT_EQ(prepared.itemId, 1U);
	EXPECT_EQ(state->itemCount, 0U);
	in.operation = QNFT::Operation::CancelImport;
	in.itemId = 0;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->importRequests.population(), 1U);
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, transferred, bob, 0));
	ASSERT_EQ(transferred.returnCode, QBAY::LogInfo::success);
	in.operation = QNFT::Operation::FinalizeImport;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::NotFound);
	in.itemId = prepared.itemId;
	const auto finalized = run(in, bob);
	EXPECT_EQ(finalized.code, QNFT::Code::Ok);
	EXPECT_EQ(finalized.itemId, 0U);
	EXPECT_EQ(balance(0, bob), 1);
	EXPECT_EQ(state->itemCount, 1U);
	EXPECT_EQ(state->importRequests.population(), 0U);
}

TEST_F(NftTest, ReserveCanBeFundedWithoutMintingOrAdminPrivileges)
{
	const auto reserve = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	const auto userBalance = getBalance(bob);
	OperationCommand in{};
	in.operation = QNFT::Operation::FundReserve;
	ASSERT_EQ(run(in, bob, 12345).code, QNFT::Code::Ok);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 12345);
	EXPECT_EQ(getBalance(bob), userBalance - 12345);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->itemCount, 0);
}

TEST_F(NftTest, ExternalBurnsAreReflectedInSupplyBeforeAndAfterEpochRebuild)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	const uint64 name = item().assetName;
	OperationCommand in{};
	in.operation = QNFT::Operation::Export;
	in.owner = alice;
	in.quantity = 10;
	in.destination = QX_CONTRACT_INDEX;
	in.offeredFee = 1000000;
	ASSERT_EQ(run(in, alice, 1000000).code, QNFT::Code::Ok);
	QX::TransferShareOwnershipAndPossession_input burn{nftId, NULL_ID, name, 3};
	QX::TransferShareOwnershipAndPossession_output burned{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 2, burn, burned, alice, 1000000));
	ASSERT_EQ(burned.transferredNumberOfShares, 3);
	EXPECT_EQ(item().burned, 3);
	EXPECT_EQ(item().supply - item().burned - item().reserved, 97);
	in.operation = QNFT::Operation::Burn;
	in.quantity = 2;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	EXPECT_EQ(item().burned, 5);
	burn.numberOfShares = 7;
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 2, burn, burned, alice, 1000000));
	ASSERT_EQ(burned.transferredNumberOfShares, 7);
	EXPECT_EQ(item().burned, 12);
	in.quantity = 88;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	assetsEndEpoch();
	EXPECT_EQ(item().burned, 100);
	EXPECT_EQ(balance(0, alice), 0);
	ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
	EXPECT_NE(item(1).assetName, name);
}

TEST_F(NftTest, ApprovalWorksAtBootstrapAndRemainsScopedToOneItem)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	QNFT::GetApproval_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approval);
	EXPECT_EQ(approval.approval.operatorId, bob);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, carol, 2, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, carol, 2, 1}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 2, 1}, bob).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(action(10, QNFT::Approve_input{carol, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, carol).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftTest, GenericExportSupportsAnyRegisteredManagerAndItsRequestedFee)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	manager->preAcquireSharesOutput.requestedFee = 7;
	OperationCommand in{};
	in.operation = QNFT::Operation::Export;
	in.owner = alice;
	in.quantity = 2;
	in.destination = TESTEXA_CONTRACT_INDEX;
	in.offeredFee = 7;
	const auto before = getBalance(alice);
	EXPECT_EQ(run(in, bob, 7).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(manager->postAcquireShareCounter, 0);
	in.offeredFee = 6;
	EXPECT_EQ(run(in, alice, 6).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 100);
	in.offeredFee = 7;
	EXPECT_EQ(run(in, alice, 20).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - 7);
	EXPECT_EQ(getBalance(id(TESTEXA_CONTRACT_INDEX, 0, 0, 0)), 7);
	EXPECT_EQ(manager->postAcquireShareCounter, 1);
	EXPECT_EQ(uint32(manager->prevPostAcquireSharesInput.otherContractIndex), uint32(QNFT_CONTRACT_INDEX));
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 98);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 2);
	EXPECT_EQ(issuanceIndex(nftId, nextNameForIssuerAttack), NO_ASSET_INDEX);
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
}

TEST_F(NftTest, OwnerCanForwardThroughQxWithoutDelegatingNftIssuer)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.operation = QNFT::Operation::Export;
	in.owner = alice;
	in.quantity = 1;
	in.destination = QX_CONTRACT_INDEX;
	in.offeredFee = 1000000;
	ASSERT_EQ(run(in, alice, 1000000).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] = forgeFromManagementCallback;
	QX::TransferShareManagementRights_input forward{};
	forward.asset = Asset{nftId, item().assetName};
	forward.numberOfShares = 1;
	forward.newManagingContractIndex = TESTEXA_CONTRACT_INDEX;
	QX::TransferShareManagementRights_output out{};
	ASSERT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 9, forward, out, alice, 1000000));
	EXPECT_EQ(out.transferredNumberOfShares, 1);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 1);
	EXPECT_EQ(issuanceIndex(nftId, nextNameForIssuerAttack), NO_ASSET_INDEX);
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
}

TEST_F(NftTest, LateQbayDepositCannotReviveDeletedImport)
{
	collection();
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	OperationCommand in{};
	in.operation = QNFT::Operation::PrepareImport;
	in.receiver = alice;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::CancelImport;
	ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	QBAY::transfer_output dep{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, dep, alice, 0));
	ASSERT_EQ(dep.returnCode, QBAY::LogInfo::success);
	in.operation = QNFT::Operation::FinalizeImport;
	ASSERT_EQ(run(in, bob).code, QNFT::Code::NotFound);
	QNFT::GetItem_output missing{};
	callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetItem_input{0}, missing);
	EXPECT_EQ(missing.code, QNFT::Code::NotFound);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->itemCount, 0U);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->importRequests.population(), 0);
}

// Regression intent: a claim must release only reserved delivery, once, to its recorded beneficiary.
TEST_F(NftTest, ClaimMintConsumesOnlyRecordedReservationOnce)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	ASSERT_EQ(mint(7).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 7, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	// Construct the persisted post-issuance/pre-delivery snapshot with real ledger escrow.
	state->listings.removeByKey(listed.listingId);
	auto record = state->items.get(0);
	record.reserved = 7;
	record.reservedRecipient = alice;
	state->items.set(0, record);
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	const auto fees = state->epochFees, before = getBalance(alice);
	EXPECT_EQ(action(3, QNFT::ClaimMint_input{0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(balance(0, nftId), 7);
	// Temporarily make the recorded escrow unavailable; the real QPI transfer must fail safely.
	record.reserved = 8;
	state->items.set(0, record);
	EXPECT_EQ(action(3, QNFT::ClaimMint_input{0}, alice, 17).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(item().reserved, 8);
	EXPECT_EQ(item().reservedRecipient, alice);
	EXPECT_EQ(balance(0, nftId), 7);
	EXPECT_EQ(getBalance(alice), before);
	record.reserved = 7;
	state->items.set(0, record);
	const auto claimed = action(3, QNFT::ClaimMint_input{0}, alice, 17);
	EXPECT_EQ(claimed.code, QNFT::Code::Ok);
	EXPECT_EQ(claimed.quantity, 7);
	EXPECT_EQ(balance(0, alice), 7);
	EXPECT_EQ(balance(0, nftId), 0);
	EXPECT_EQ(item().reserved, 0);
	EXPECT_EQ(item().reservedRecipient, NULL_ID);
	EXPECT_EQ(action(3, QNFT::ClaimMint_input{0}, alice).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(alice), before);
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 1U);
	EXPECT_EQ(logs[0].from, nftId);
	EXPECT_EQ(logs[0].to, alice);
	EXPECT_EQ(logs[0].quantity, 7);
	EXPECT_TRUE(uriLogs().empty());
	expectQuCollateral();
}

// Regression intent: wrapping a metadata version must not revive a stale listing.
TEST_F(NftTest, MetadataVersionExhaustionPreservesDocumentAndRefund)
{
	collection();
	ASSERT_EQ(mint(3, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto record = state->items.get(0);
	record.metadataVersion = 0xffffffffU;
	state->items.set(0, record);
	const auto before = getBalance(alice), fees = state->epochFees;
	EXPECT_EQ(action(6, QNFT::UpdateMetadata_input{metadata("ipfs://changed", 99), 0}, alice, 50017).code, QNFT::Code::Capacity);
	EXPECT_EQ(item().metadataVersion, 0xffffffffU);
	EXPECT_EQ(item().metadata.contentHash, record.metadata.contentHash);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(action(8, QNFT::FreezeMetadata_input{0}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(item().frozen, 1);
	expectQuCollateral();
}

// Regression intent: at the allowance cap, replacement remains possible and revocation frees admission.
TEST_F(NftTest, ApprovalCapacityAllowsReplacementAndReusesRevokedSlot)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::Approve_input request{bob, 0};
	ASSERT_EQ(action(10, request).code, QNFT::Code::Ok);
	QNFT::Approval filler{bob};
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->approvals.set(id(n, 41, 42, 43), filler);
	}
	request.itemId = 1;
	const auto before = getBalance(alice);
	EXPECT_EQ(action(10, request, alice, 17).code, QNFT::Code::Capacity);
	EXPECT_EQ(getBalance(alice), before);
	request.itemId = 0;
	request.receiver = carol;
	ASSERT_EQ(action(10, request).code, QNFT::Code::Ok);
	QNFT::GetApproval_output approved{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approved);
	EXPECT_EQ(approved.approval.operatorId, carol);
	EXPECT_EQ(state->approvals.population(), QNFT_MAX_APPROVALS);
	EXPECT_EQ(action(11, QNFT::Revoke_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->approvals.population(), QNFT_MAX_APPROVALS - 1);
	EXPECT_EQ(action(10, request).code, QNFT::Code::Ok);
	expectQuCollateral();
}

// Revocation always addresses the caller and is independent of network time.
TEST_F(NftTest, RevokeOnlyTouchesInvocatorApprovalRegardlessOfTime)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 5, 0}).code, QNFT::Code::Ok);
	QNFT::Approve_input approval{carol, 0};
	ASSERT_EQ(action(10, approval, alice).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, approval, bob).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::Revoke_input request{};
	ASSERT_EQ(action(11, request, alice, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(state->approvals.population(), 1);
	QNFT::GetApproval_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{bob, 0}, remaining);
	EXPECT_EQ(remaining.approval.operatorId, carol);
	// A missing approval reports NotFound, refunds the reward, and preserves other owners' entries.
	const auto before = getBalance(alice), fees = state->epochFees;
	ASSERT_EQ(action(11, request, alice, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(state->approvals.population(), 1);
	utcTime.Year = 2027;
	updateQpiTime();
	ASSERT_EQ(action(11, request, alice).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->approvals.population(), 1);
	ASSERT_EQ(action(11, request, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(state->approvals.population(), 0);
	utcTime.Year = 2026;
	updateQpiTime();
	ASSERT_EQ(action(10, approval, alice).code, QNFT::Code::Ok);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	ASSERT_EQ(action(11, request, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(state->approvals.population(), 0);
	expectQuCollateral();
}

// Epoch maintenance compacts revoked entries without removing live permissions.
TEST_F(NftTest, EpochCompactsRevokedApprovalsWithoutRemovingLivePermissions)
{
	collection();
	ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->approvals.set(id(n, 41, 42, 43), QNFT::Approval{bob});
	}
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->approvals.removeByKey(id(n, 41, 42, 43));
	}
	ASSERT_TRUE(state->approvals.needsCleanup(QNFT_MAP_CLEANUP_PERCENT));
	utcTime.Year = 2030;
	updateQpiTime();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->approvals.population(), 1);
	EXPECT_FALSE(state->approvals.needsCleanup(0));
	QNFT::GetApproval_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approval);
	EXPECT_EQ(approval.approval.operatorId, bob);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Ok);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->approvals.population(), 1);
	expectQuCollateral();
}

TEST_F(NftTest, ApprovalCanBeGrantedAtZeroBalanceAndSurvivesListing)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::Approve_input{carol, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listing{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{7, 2, 0, 0}, listing, alice, 0));
	ASSERT_EQ(listing.code, QNFT::Code::Ok);
	QNFT::GetApproval_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::GetApproval_input{alice, 0}, approval);
	EXPECT_EQ(approval.approval.operatorId, bob);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::InsufficientBalance);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listing.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 2, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::Burn_input{bob, 1, 0}, carol).code, QNFT::Code::Ok);
	expectQuCollateral();
}

// Epoch cleanup must preserve the active listings and their escrowed units.
TEST_F(NftTest, EpochCompactsShopListingsWithoutLosingEscrow)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{7, 2, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	// Seed historical churn using real map inserts/removes, without thousands of redundant transactions.
	// Insert all entries before removing so tombstone reuse cannot hide the cleanup threshold.
	QNFT::Listing historical{};
	for (uint64 i = 10000; i < 30000; ++i)
	{
		historical.listingId = i;
		state->listings.set(i, historical);
	}

	for (uint64 i = 10000; i < 30000; ++i)
	{
		state->listings.removeByKey(i);
	}

	ASSERT_TRUE(state->listings.needsCleanup(25));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_FALSE(state->listings.needsCleanup(0));
	EXPECT_EQ(state->listings.population(), 1U);
	EXPECT_EQ(balance(0, nftId), 2);
	QNFT::GetListing_output live{};
	callFunction(QNFT_CONTRACT_INDEX, 11, QNFT::GetListing_input{listed.listingId}, live);
	ASSERT_EQ(live.code, QNFT::Code::Ok);
	EXPECT_EQ(live.listing.seller, alice);
	EXPECT_EQ(live.listing.quantity, 2);
	EXPECT_EQ(live.listing.unitPrice, 7);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	EXPECT_EQ(cancelled.code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 2);
	expectQuCollateral();
}

// Regression intent: sparse listing pagination must return entries beyond the former 8,192-slot ceiling.
TEST_F(NftTest, ListingPaginationReturnsUpperSlotsExactlyOnce)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::Listing listing{};
	listing.seller = alice;
	listing.quantity = 1;
	listing.unitPrice = 7;
	uint64 chosen = 0;
	sint64 slot = 0;
	for (uint64 candidate = 1; candidate < 10000; ++candidate)
	{
		listing.listingId = candidate;
		state->listings.set(candidate, listing);
		slot = state->listings.nextElementIndex(NULL_INDEX);
		if (slot >= 8192)
		{
			chosen = candidate;
			break;
		}
		state->listings.removeByKey(candidate);
	}
	ASSERT_NE(chosen, 0ULL);
	QNFT::ListListings_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::ListListings_input{8192, 1}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.listingIds.get(0), chosen);
	EXPECT_EQ(page.nextOffset, slot + 1);
	callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::ListListings_input{page.nextOffset, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, QNFT_LISTING_SLOTS);
}

// Regression intent: failure after purchase preflight must not retain payment or create seller credits.
TEST_F(NftTest, FailedAssetDeliveryRefundsPurchaseAndPreservesListing)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::CancelSale_output cancelled{};
	QNFT::GetListing_output original{};
	callFunction(QNFT_CONTRACT_INDEX, 11, QNFT::GetListing_input{listed.listingId}, original);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	// Seed a listing whose native escrow is unavailable to exercise the real QPI failure branch.
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->listings.set(listed.listingId, original.listing);
	const auto before = getBalance(bob), fees = state->epochFees;
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 100, 1, 0}, bought, bob, 117));
	EXPECT_EQ(bought.code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(bought.paid, 0);
	EXPECT_EQ(bought.fee, 0);
	EXPECT_EQ(bought.royalty, 0);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(state->epochFees, fees);
	QNFT::Listing retained{};
	ASSERT_TRUE(state->listings.get(listed.listingId, retained));
	EXPECT_EQ(retained.quantity, 2);
	expectQuCollateral();
}

// Regression intent: royalty arithmetic must not overflow at MAX_AMOUNT or round sub-QU royalties upward.
TEST_F(NftTest, RoyaltyBoundariesUseTotalPriceWithoutOverflow)
{
	struct Case
	{
		sint64 price;
		uint32 bps;
		sint64 expected;
	};
	const Case cases[] = {{0, 10000, 0}, {1, 9999, 0}, {10001, 1, 1}, {19999, 5000, 9999}, {MAX_AMOUNT, 10000, MAX_AMOUNT}, {MAX_AMOUNT, 0, 0}};
	for (const auto row : cases)
	{
		EXPECT_EQ(NftExecutionProbe::calculateFeeShare(row.price, row.bps), row.expected);
	}
}

// Persisted post-issuance snapshots must retry delivery without issuing a second asset or changing beneficiary.
TEST_F(NftTest, IssuedImportRejectsManualFinalizationAndCompletesOnTick)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	ASSERT_EQ(action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	QNFT::ImportReservation request{};
	ASSERT_TRUE(state->importRequests.get(0, request));
	QNFT::Item record{};
	const char assetName[8] = "N000000";
	copyMem(&record.assetName, assetName, 8);
	record.origin = QNFT::Origin::Qbay;
	record.bridge = QNFT::Bridge::Requested;
	record.reservedRecipient = bob;
	record.creator = carol;
	record.metadata = request.metadata;
	record.metadataVersion = 1;
	state->imports.set(0, 0); // Persisted issuance binds the original before delivery is attempted.
	state->itemCount = 1;
	state->importRequests.removeByKey(0);
	auto issuedCollection = state->collections.get(0);
	++issuedCollection.issuedItems;
	++issuedCollection.issuedUnits;
	state->collections.set(0, issuedCollection);
	int issuance, owner, possession, displacedOwner, displacedPossession, restoredOwner, restoredPossession;
	const char unit[7]{};
	ASSERT_EQ(issueAsset(nftId, reinterpret_cast<const char*>(&record.assetName), 0, unit, 1, QNFT_CONTRACT_INDEX, &issuance, &owner, &possession),
	          1);
	record.supply = 1;
	record.reserved = 1;
	state->items.set(0, record);
	state->importDeliveries.addTail(0); // Reproduce the queued post-issuance snapshot.
	QNFT::GetImport_output progress{};
	callFunction(QNFT_CONTRACT_INDEX, 6, QNFT::GetImport_input{0}, progress);
	ASSERT_EQ(progress.code, QNFT::Code::NotFound);
	// Fault injection into ledger custody after issuance; QNFT must preserve its recoverable snapshot.
	ASSERT_TRUE(transferShareOwnershipAndPossession(owner, possession, carol, 1, &displacedOwner, &displacedPossession, true));
	const auto before = getBalance(alice), fees = state->epochFees;
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 41}, alice).code, QNFT::Code::NotFound);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 42}, alice, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(item().supply, 1);
	EXPECT_EQ(item().reserved, 1);
	EXPECT_EQ(item().reservedRecipient, bob);
	uint32 boundItem = 99;
	ASSERT_TRUE(state->imports.get(0, boundItem));
	EXPECT_EQ(boundItem, 0U);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(qbay->NFTs.get(0).possessor, nftId);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	ASSERT_TRUE(transferShareOwnershipAndPossession(displacedOwner, displacedPossession, nftId, 1, &restoredOwner, &restoredPossession, true));
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 42}, carol, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->importDeliveries.population(), 1U);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(balance(0, bob), 1);
	EXPECT_EQ(balance(0, alice), 0);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(item().bridge, QNFT::Bridge::Permanent);
	EXPECT_EQ(item().reserved, 0);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 42}, alice, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(item().supply, 1);
	expectQuCollateral();
	// A repeated tick cannot deliver a completed import again.
	const auto completedEventId = state->lastEventId;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->importDeliveries.population(), 0U);
	EXPECT_EQ(state->lastEventId, completedEventId);
	EXPECT_EQ(balance(0, bob), 1);
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 1U);
	EXPECT_EQ(logs[0].operatorId, NULL_ID);
	EXPECT_EQ(logs[0].from, nftId);
	EXPECT_EQ(logs[0].to, bob);
	EXPECT_EQ(logs[0].quantity, 1);
	EXPECT_TRUE(uriLogs().empty());
	EXPECT_EQ(state->collections.get(0).issuedItems, 1U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 1);
}

TEST_F(NftTest, PendingImportDeliveryCompletesOnTickWithoutUserRetry)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	ASSERT_EQ(action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	QNFT::ImportReservation request{};
	ASSERT_TRUE(state->importRequests.get(0, request));
	QNFT::Item record{};
	const char assetName[8] = "N000000";
	copyMem(&record.assetName, assetName, 8);
	record.origin = QNFT::Origin::Qbay;
	record.bridge = QNFT::Bridge::Requested;
	record.reservedRecipient = bob;
	record.creator = carol;
	record.metadata = request.metadata;
	record.metadataVersion = 1;
	state->imports.set(0, 0); // Persisted issuance binds the original before delivery is attempted.
	state->itemCount = 1;
	state->importRequests.removeByKey(0);
	auto issuedCollection = state->collections.get(0);
	++issuedCollection.issuedItems;
	++issuedCollection.issuedUnits;
	state->collections.set(0, issuedCollection);
	int issuance, owner, possession, displacedOwner, displacedPossession, restoredOwner, restoredPossession;
	const char unit[7]{};
	ASSERT_EQ(issueAsset(nftId, reinterpret_cast<const char*>(&record.assetName), 0, unit, 1, QNFT_CONTRACT_INDEX, &issuance, &owner, &possession),
	          1);
	record.supply = 1;
	record.reserved = 1;
	state->items.set(0, record);
	state->importDeliveries.addTail(0); // Reproduce the queued post-issuance snapshot.
	QNFT::GetImport_output progress{};
	callFunction(QNFT_CONTRACT_INDEX, 6, QNFT::GetImport_input{0}, progress);
	ASSERT_EQ(progress.code, QNFT::Code::NotFound);
	// Fault injection into ledger custody after issuance; QNFT must preserve its recoverable snapshot.
	ASSERT_TRUE(transferShareOwnershipAndPossession(owner, possession, carol, 1, &displacedOwner, &displacedPossession, true));
	const auto before = getBalance(alice), fees = state->epochFees;
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 41}, alice).code, QNFT::Code::NotFound);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 42}, alice, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(item().supply, 1);
	EXPECT_EQ(item().reserved, 1);
	EXPECT_EQ(item().reservedRecipient, bob);
	uint32 boundItem = 99;
	ASSERT_TRUE(state->imports.get(0, boundItem));
	EXPECT_EQ(boundItem, 0U);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(qbay->NFTs.get(0).possessor, nftId);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	// Failed ticks retain exactly one entry and do not change counters, fees or events.
	const auto pendingEventId = state->lastEventId;
	for (unsigned tick = 0; tick < 3; ++tick)
	{
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
		EXPECT_EQ(state->importDeliveries.population(), 1U);
		EXPECT_EQ(state->lastEventId, pendingEventId);
		EXPECT_EQ(state->itemCount, 1U);
		EXPECT_EQ(state->epochFees, fees);
	}
	ASSERT_TRUE(transferShareOwnershipAndPossession(displacedOwner, displacedPossession, nftId, 1, &restoredOwner, &restoredPossession, true));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->importDeliveries.population(), 0U);
	const auto completedEventId = state->lastEventId;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->lastEventId, completedEventId);
	EXPECT_EQ(balance(0, bob), 1);
	EXPECT_EQ(balance(0, alice), 0);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(item().bridge, QNFT::Bridge::Permanent);
	EXPECT_EQ(item().reserved, 0);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, 42}, alice, 17).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(item().supply, 1);
	expectQuCollateral();
	qLogger::updateTick(system.tick);
	const auto logs = transferLogs();
	ASSERT_EQ(logs.size(), 1U);
	EXPECT_EQ(logs[0].operatorId, NULL_ID);
	EXPECT_EQ(logs[0].from, nftId);
	EXPECT_EQ(logs[0].to, bob);
	EXPECT_EQ(logs[0].quantity, 1);
	EXPECT_TRUE(uriLogs().empty());
	EXPECT_EQ(state->collections.get(0).issuedItems, 1U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 1);
}

// Invalid imports must not consume identities, collection quotas, pending slots or QU.
TEST_F(NftTest, ImportRejectionMatrixPreservesRegistryAndFunds)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT clean{};
	clean.creator = carol;
	clean.possessor = alice;
	clean.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	clean.URI.set(0, 'Q');
	qbay->numberOfNFT = 1;
	const auto before = getBalance(alice), fees = state->epochFees;
	for (unsigned n = 0; n < 8; ++n)
	{
		state->importRequests.reset();
		auto original = clean;
		QNFT::PrepareImport_input request{alice, 0, 0};
		QNFT::Code expected = QNFT::Code::Conflict;
		switch (n)
		{
			case 0: original.statusOfSale = true; break;
			case 1: original.statusOfAsk = true; break;
			case 2: original.statusOfExchange = true; break;
			case 3:
				original.possessor = bob;
				expected = QNFT::Code::Unauthorized;
				break;
			case 4:
				original.royalty = 101;
				expected = QNFT::Code::InvalidInput;
				break;
			case 5:
				request.collectionId = 1;
				expected = QNFT::Code::NotFound;
				break;
			case 6:
				for (uint32 i = 0; i < QNFT_MAX_IMPORTS; ++i)
				{
					QNFT::ImportReservation reserved{};
					reserved.qbayId = i + 1;
					state->importRequests.set(i, reserved);
				}
				expected = QNFT::Code::Capacity;
				break;
			case 7:
				state->nextImportRequestId = QNFT_REQUEST_ID_LIMIT;
				expected = QNFT::Code::Capacity;
				break;
		}
		qbay->NFTs.set(0, original);
		EXPECT_EQ(action(14, request, alice, TEST_MINT_FEE + 17).code, expected);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(state->imports.population(), 0);
		EXPECT_EQ(state->importRequests.population(), n == 6 ? QNFT_MAX_IMPORTS : 0U);
		EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
		EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
		EXPECT_EQ(state->importRequests.population(), n == 6 ? QNFT_MAX_IMPORTS : 0U);
		EXPECT_EQ(getBalance(alice), before);
		EXPECT_EQ(state->epochFees, fees);
		expectQuCollateral();
	}
}

TEST_F(NftTest, DISABLED_WorkloadBenchmark)
{
	const char* sizeText = std::getenv("QNFT_BENCH_ITEMS");
	const uint32 size = sizeText ? static_cast<uint32>(std::strtoul(sizeText, nullptr, 10)) : 1024;
	ASSERT_GT(size, 0U);
	ASSERT_LE(size, QNFT_MAX_ITEMS);
	collection(MAX_AMOUNT, QNFT_MAX_ITEMS);
	const auto start = std::chrono::steady_clock::now();
	for (uint32 i = 0; i < size; ++i)
	{
		auto out = mint(1);
		ASSERT_EQ(out.code, QNFT::Code::Ok);
		ASSERT_EQ(out.itemId, i);
		if (i + 1 == 1024 || i + 1 == 4096 || i + 1 == 16384)
		{
			std::cout << "BENCH mint_count=" << i + 1
			          << " cumulative_ms=" << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() << '\n';
		}
	}
	auto before = std::chrono::steady_clock::now();
	for (unsigned i = 0; i < 100; ++i)
	{
		EXPECT_EQ(balance(size - 1, alice), 1);
	}
	std::cout << "BENCH tail_100_balance_ms=" << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - before).count() << '\n';
	before = std::chrono::steady_clock::now();
	assetsEndEpoch();
	std::cout << "BENCH epoch_rebuild_ms=" << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - before).count() << '\n';
	EXPECT_EQ(balance(size - 1, alice), 1);
}

TEST_F(NftTest, DISABLED_DistributedEditionBenchmark)
{
	constexpr uint32 ownerCount = 16384;
	collection(ownerCount, 1);
	ASSERT_EQ(mint(ownerCount).code, QNFT::Code::Ok);
	const auto start = std::chrono::steady_clock::now();
	OperationCommand in{};
	in.operation = QNFT::Operation::Transfer;
	in.owner = alice;
	in.quantity = 1;
	for (uint32 i = 0; i < ownerCount; ++i)
	{
		in.receiver = id(10000 + i, 10, 11, 12);
		ASSERT_EQ(run(in).code, QNFT::Code::Ok);
	}
	std::cout << "BENCH edition_16384_transfers_ms=" << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
	          << '\n';
	EXPECT_EQ(balance(0, alice), 0);
	EXPECT_EQ(balance(0, in.receiver), 1);
}
// Preparing and cancelling a request must never allocate an NFT identity.
TEST_F(NftTest, ImportPreparationAndCancellationDoNotAllocateItems)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	for (unsigned i = 0; i < 3; ++i)
	{
		const auto prepared = action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE);
		ASSERT_EQ(prepared.code, QNFT::Code::Ok);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
		EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
		EXPECT_EQ(state->collectionCount, 1U);
		EXPECT_EQ(state->items.get(0).assetName, 0U);
		EXPECT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, alice).code, QNFT::Code::Ok);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(state->importRequests.population(), 0U);
		EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
	}
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	EXPECT_EQ(state->itemCount, 1U);
	EXPECT_EQ(balance(0, alice), 100);
}

TEST_F(NftTest, ImportFinalizationAllocatesAfterInterveningMintAndPreservesSnapshot)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.royalty = 5;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	QNFT::PrepareImport_output prepared{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 14, QNFT::PrepareImport_input{bob, 0, 0}, prepared, alice, TEST_MINT_FEE));
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	EXPECT_EQ(sizeof(prepared), 8U);
	EXPECT_EQ(state->itemCount, 0U);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, prepared.requestId}, carol).code, QNFT::Code::Conflict);
	EXPECT_EQ(state->itemCount, 0U);
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	const auto finalized = action(15, QNFT::FinalizeImport_input{0, prepared.requestId}, carol);
	ASSERT_EQ(finalized.code, QNFT::Code::Ok);
	EXPECT_EQ(finalized.itemId, 1U);
	EXPECT_EQ(state->itemCount, 2U);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(balance(1, bob), 1);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(item(1).creator, carol);
	EXPECT_EQ(item(1).metadata.uri.get(0), 'Q');
	EXPECT_EQ(static_cast<unsigned>(item(1).royaltyBps), 500U);
	uint32 binding = 0;
	ASSERT_TRUE(state->imports.get(0, binding));
	EXPECT_EQ(binding, 1U);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, prepared.requestId}, carol).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->itemCount, 2U);
	expectQuCollateral();
}

TEST_F(NftTest, ImportReservationProtectsLastItemSlotAndCancellationReleasesIt)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	state->itemCount = QNFT_MAX_ITEMS - 1;
	const auto prepared = action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	EXPECT_EQ(state->itemCount, QNFT_MAX_ITEMS - 1);
	EXPECT_EQ(mint().code, QNFT::Code::Capacity);
	ASSERT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, alice).code, QNFT::Code::Ok);

	EXPECT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, alice).code, QNFT::Code::NotFound);

	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	EXPECT_EQ(state->itemCount, QNFT_MAX_ITEMS);
}

TEST_F(NftTest, ImportRequestQueryExposesReservationAndReportsDeletion)
{
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][6], nullptr);
	collection();
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	const auto prepared = action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	struct QueryOutput
	{
		QNFT::ImportRequest request;
		QNFT::Code code;
	} queried{};
	static_assert(sizeof(QueryOutput) == 408);
	callFunction(QNFT_CONTRACT_INDEX, 6, prepared.itemId, queried);
	ASSERT_EQ(queried.code, QNFT::Code::Ok);
	EXPECT_EQ(queried.request.requester, alice);
	EXPECT_EQ(queried.request.recipient, bob);
	EXPECT_EQ(queried.request.qbayId, 0U);
	EXPECT_EQ(queried.request.requestId, prepared.itemId);
	EXPECT_EQ(queried.request.metadata.uri.get(0), 'Q');
	ASSERT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, alice).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 6, prepared.itemId, queried);
	EXPECT_EQ(queried.code, QNFT::Code::NotFound);
}

// A failed core issuance must leave the reservation retryable and consume no item ID.
TEST_F(NftTest, ImportIssuanceFailureDoesNotAllocateItemOrConsumeReservation)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	const auto prepared = action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	// Seed a conflicting ledger issuance to exercise the real core failure branch.
	const char name[8] = "N000000", unit[7]{};
	int issuance, owner, possession;
	ASSERT_EQ(issueAsset(nftId, name, 0, unit, 1, QNFT_CONTRACT_INDEX, &issuance, &owner, &possession), 1);
	const auto before = getBalance(alice), fees = state->epochFees;
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{0, prepared.itemId}, alice, 17).code, QNFT::Code::ExternalFailure);
	EXPECT_FALSE(state->imports.contains(0));
	EXPECT_EQ(state->itemCount, 0U);
	EXPECT_EQ(state->items.get(0).assetName, 0U);
	EXPECT_EQ(state->importRequests.population(), 1U);
	QNFT::ImportReservation request{};
	ASSERT_TRUE(state->importRequests.get(0, request));
	EXPECT_FALSE(state->imports.contains(request.qbayId));
	EXPECT_EQ(request.recipient, bob);
	EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(action(16, QNFT::CancelImport_input{0, prepared.itemId}, alice).code, QNFT::Code::Conflict);
	expectQuCollateral();
}

TEST_F(NftTest, EpochCompactsImportRequestsWithoutLosingLiveReservations)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::ImportReservation request{};
	request.requester = alice;
	request.recipient = bob;
	for (uint32 i = 0; i < 200; ++i)
	{
		state->importRequests.set(i, request);
	}
	for (uint32 i = 1; i < 200; ++i)
	{
		state->importRequests.removeByKey(i);
	}
	ASSERT_TRUE(state->importRequests.needsCleanup(QNFT_MAP_CLEANUP_PERCENT));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_FALSE(state->importRequests.needsCleanup(0));
	ASSERT_TRUE(state->importRequests.get(0, request));
	EXPECT_EQ(request.recipient, bob);
	EXPECT_EQ(state->importRequests.population(), 1);
}

TEST_F(NftTest, ImportRegistryContainsOnlyIssuedItemsAndPendingRequestsStayUnique)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(7, original);
	qbay->NFTs.set(9, original);
	qbay->numberOfNFT = 10;
	const auto first = action(14, QNFT::PrepareImport_input{bob, 0, 7}, alice, TEST_MINT_FEE);
	ASSERT_EQ(first.code, QNFT::Code::Ok);
	EXPECT_EQ(state->imports.population(), 0);
	const auto second = action(14, QNFT::PrepareImport_input{alice, 0, 9}, alice, TEST_MINT_FEE);
	ASSERT_EQ(second.code, QNFT::Code::Ok);
	const auto fees = state->epochFees, before = getBalance(alice);
	EXPECT_EQ(action(14, QNFT::PrepareImport_input{carol, 0, 7}, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(14, QNFT::PrepareImport_input{carol, 0, 9}, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
	EXPECT_EQ(state->importRequests.population(), 2U);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->imports.population(), 0);
	ASSERT_EQ(action(16, QNFT::CancelImport_input{7, first.itemId}).code, QNFT::Code::Ok);
	const auto replacement = action(14, QNFT::PrepareImport_input{bob, 0, 7}, alice, TEST_MINT_FEE);
	ASSERT_EQ(replacement.code, QNFT::Code::Ok);
	EXPECT_NE(replacement.itemId, first.itemId);
	QNFT::ImportReservation pending{};
	ASSERT_TRUE(state->importRequests.get(7, pending));
	EXPECT_EQ(pending.requestId, replacement.itemId);
	EXPECT_EQ(action(16, QNFT::CancelImport_input{7, first.itemId}).code, QNFT::Code::NotFound);
	EXPECT_EQ(action(15, QNFT::FinalizeImport_input{7, first.itemId}).code, QNFT::Code::NotFound);
	EXPECT_EQ(action(16, QNFT::CancelImport_input{9, replacement.itemId}).code, QNFT::Code::NotFound);
	EXPECT_EQ(state->importRequests.population(), 2U);
	QNFT::GetImport_output snapshot{};
	callFunction(QNFT_CONTRACT_INDEX, 6, QNFT::GetImport_input{7}, snapshot);
	ASSERT_EQ(snapshot.code, QNFT::Code::Ok);
	EXPECT_EQ(snapshot.request.requestId, replacement.itemId);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 7}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	const auto finalized = action(15, QNFT::FinalizeImport_input{7, replacement.itemId}, carol);
	ASSERT_EQ(finalized.code, QNFT::Code::Ok);
	EXPECT_EQ(finalized.itemId, 0U);
	EXPECT_FALSE(state->importRequests.contains(7));
	EXPECT_TRUE(state->importRequests.contains(9));
	EXPECT_EQ(state->collections.get(0).issuedItems, 1U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 1);
	EXPECT_EQ(item().qbayId, 7U);
	uint32 itemId = 99;
	ASSERT_TRUE(state->imports.get(7, itemId));
	EXPECT_EQ(itemId, 0U);
	EXPECT_FALSE(state->imports.contains(9));
	EXPECT_EQ(state->imports.population(), 1);
	ASSERT_EQ(action(16, QNFT::CancelImport_input{9, second.itemId}).code, QNFT::Code::Ok);
	ASSERT_TRUE(state->imports.get(7, itemId));
	EXPECT_EQ(itemId, 0U);
	EXPECT_EQ(state->importRequests.population(), 0U);
	EXPECT_EQ(balance(0, bob), 1);
	expectQuCollateral();
}

TEST_F(NftTest, ImportAdmissionUsesActualRequestMapPopulation)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	QNFT::ImportReservation reservation{};
	reservation.requester = alice;
	reservation.recipient = bob;
	for (uint32 i = 0; i < QNFT_MAX_IMPORTS; ++i)
	{
		reservation.qbayId = i + 1;
		state->importRequests.set(i, reservation);
	}
	state->nextImportRequestId = QNFT_MAX_IMPORTS;
	const auto balanceBefore = getBalance(alice), fees = state->epochFees;
	EXPECT_EQ(action(14, QNFT::PrepareImport_input{alice, 0, 0}, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	EXPECT_EQ(state->importRequests.population(), QNFT_MAX_IMPORTS);
	EXPECT_EQ(state->itemCount, 0U);
	EXPECT_EQ(state->nextImportRequestId, QNFT_MAX_IMPORTS);
	EXPECT_EQ(getBalance(alice), balanceBefore);
	EXPECT_EQ(state->epochFees, fees);
}

// Reservations protect both collection limits without claiming that an NFT has already been issued.
TEST_F(NftTest, PendingImportsReserveCollectionItemsAndUnits)
{
	collection(2, 3);
	collection(1000, 1);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.creator = carol;
	original.possessor = alice;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	for (uint32 i = 7; i < 11; ++i)
	{
		qbay->NFTs.set(i, original);
	}
	qbay->numberOfNFT = 11;
	const auto first = action(14, QNFT::PrepareImport_input{bob, 0, 7}, alice, TEST_MINT_FEE);
	ASSERT_EQ(first.code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::PrepareImport_input{bob, 0, 8}, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
	EXPECT_EQ(action(14, QNFT::PrepareImport_input{bob, 0, 9}, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.receiver = alice;
	in.metadata = metadata();
	in.quantity = 1;
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	// The second collection has its own independent reservation budget and only one item slot.
	ASSERT_EQ(action(14, QNFT::PrepareImport_input{bob, 1, 9}, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::PrepareImport_input{bob, 1, 10}, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	in.collectionId = 1;
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	ASSERT_EQ(action(16, QNFT::CancelImport_input{7, first.itemId}, alice).code, QNFT::Code::Ok);
	in.collectionId = 0;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(state->collections.get(0).issuedItems, 1U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 1);
	EXPECT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 8}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	ASSERT_EQ(action(15, QNFT::FinalizeImport_input{8, 1}, carol).code, QNFT::Code::Ok);
	EXPECT_EQ(state->collections.get(0).issuedItems, 2U);
	EXPECT_EQ(state->collections.get(0).issuedUnits, 2);
	EXPECT_EQ(state->importRequests.population(), 1U);
}

namespace
{
	struct RegistryPageQueryWire
	{
		uint32 offset;
		uint32 limit;
	};
	struct ImportRequestPageWire
	{
		QPI::Array<QNFT::ImportRequest, 16> requests;
		uint32 count, nextOffset, totalCount;
		QNFT::Code code;
	};
	struct CollectionEntryWire
	{
		QNFT::Collection collection;
		uint32 collectionId;
	};
	struct CollectionPageWire
	{
		QPI::Array<CollectionEntryWire, 16> collections;
		uint32 count, nextOffset, totalCount;
		QNFT::Code code;
	};
	static_assert(sizeof(ImportRequestPageWire) == 6416);
	static_assert(sizeof(CollectionPageWire) == 6672);
} // namespace

TEST_F(NftTest, ListCollectionsReturnsCompleteRecordsAndPageBoundaries)
{
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][2], nullptr);
	CollectionPageWire page{};
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, 0U);
	for (uint32 i = 0; i < 17; ++i)
	{
		ASSERT_EQ(collection(100 + i, 10 + i), i);
	}
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 16U);
	EXPECT_EQ(page.totalCount, 17U);
	EXPECT_EQ(page.nextOffset, 16U);
	for (uint32 i = 0; i < page.count; ++i)
	{
		const auto& entry = page.collections.get(i);
		EXPECT_EQ(entry.collectionId, i);
		EXPECT_EQ(entry.collection.creator, alice);
		EXPECT_EQ(entry.collection.maxUnits, 100 + i);
		EXPECT_EQ(entry.collection.maxItems, 10 + i);
		EXPECT_EQ(entry.collection.metadata.contentHash, metadata().contentHash);
	}
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{page.nextOffset, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.collections.get(0).collectionId, 16U);
	EXPECT_EQ(page.nextOffset, 17U);
	EXPECT_EQ(page.collections.get(1).collection.maxUnits, 0);
	std::ofstream("abi-collection-page.bin", std::ios::binary).write(reinterpret_cast<const char*>(&page), sizeof(page));
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{17, 1}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	for (const auto query : {RegistryPageQueryWire{18, 1}, RegistryPageQueryWire{0, 0}, RegistryPageQueryWire{0, 17}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 2, query, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->collections.set(QNFT_MAX_COLLECTIONS - 1, state->collections.get(16));
	state->collectionCount = QNFT_MAX_COLLECTIONS;
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{QNFT_MAX_COLLECTIONS - 1, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.collections.get(0).collectionId, QNFT_MAX_COLLECTIONS - 1);
	EXPECT_EQ(page.nextOffset, QNFT_MAX_COLLECTIONS);
}

TEST_F(NftTest, ListImportRequestsUsesStableVersionCursorAcrossRemovalAndCompaction)
{
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][7], nullptr);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	ImportRequestPageWire page{};
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, QNFT_REQUEST_ID_LIMIT);
	for (uint32 i = 0; i < 19; ++i)
	{
		QNFT::ImportReservation request{};
		request.qbayId = 500 - i;
		request.requestId = i * 3;
		request.requester = alice;
		request.recipient = bob;
		request.creator = carol;
		request.metadata = metadata();
		request.royaltyBps = 500;
		state->importRequests.set(request.qbayId, request);
	}
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 16U);
	EXPECT_EQ(page.totalCount, 19U);
	EXPECT_EQ(page.nextOffset, 46U);
	for (uint32 i = 0; i < 16; ++i)
	{
		EXPECT_EQ(page.requests.get(i).requestId, i * 3);
		EXPECT_EQ(page.requests.get(i).qbayId, 500 - i);
		EXPECT_EQ(page.requests.get(i).recipient, bob);
		EXPECT_EQ(page.requests.get(i).metadata.contentHash, metadata().contentHash);
	}
	const auto cursor = page.nextOffset;
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{1, 1}, page);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.requests.get(0).requestId, 3U);
	EXPECT_EQ(page.nextOffset, 4U);
	state->importRequests.removeByKey(500); // A previously returned request is cancelled.
	state->importRequests.removeByKey(483); // A not-yet-returned request is cancelled.
	state->importRequests.cleanup();
	QNFT::ImportReservation replacement{};
	replacement.qbayId = 500;
	replacement.requestId = 57;
	replacement.recipient = carol;
	state->importRequests.set(500, replacement);
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{cursor, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 3U);
	EXPECT_EQ(page.requests.get(0).requestId, 48U);
	EXPECT_EQ(page.requests.get(1).requestId, 54U);
	EXPECT_EQ(page.requests.get(2).requestId, 57U);
	EXPECT_EQ(page.requests.get(2).recipient, carol);
	EXPECT_EQ(page.nextOffset, QNFT_REQUEST_ID_LIMIT);
	EXPECT_EQ(page.totalCount, 18U);
	std::ofstream("abi-import-page.bin", std::ios::binary).write(reinterpret_cast<const char*>(&page), sizeof(page));
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{QNFT_REQUEST_ID_LIMIT, 16}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	for (uint32 limit : {0U, 17U})
	{
		callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{0, limit}, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	state->importRequests.reset();
	replacement.requestId = QNFT_REQUEST_ID_LIMIT - 1;
	state->importRequests.set(replacement.qbayId, replacement);
	callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{QNFT_REQUEST_ID_LIMIT - 1, 1}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.requests.get(0).requestId, QNFT_REQUEST_ID_LIMIT - 1);
	EXPECT_EQ(page.nextOffset, QNFT_REQUEST_ID_LIMIT);
}

TEST_F(NftTest, ListImportRequestsTraversesFullCapacityWithoutDuplicates)
{
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][7], nullptr);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 i = 0; i < QNFT_MAX_IMPORTS; ++i)
	{
		QNFT::ImportReservation request{};
		request.qbayId = QNFT_MAX_IMPORTS - i;
		request.requestId = i * 2;
		state->importRequests.set(request.qbayId, request);
	}
	uint32 cursor = 0, seen = 0;
	do
	{
		ImportRequestPageWire page{};
		callFunction(QNFT_CONTRACT_INDEX, 7, RegistryPageQueryWire{cursor, 16}, page);
		ASSERT_EQ(page.code, QNFT::Code::Ok);
		ASSERT_EQ(page.count, 16U);
		for (uint32 i = 0; i < page.count; ++i)
		{
			EXPECT_EQ(page.requests.get(i).requestId, (seen + i) * 2);
		}
		seen += page.count;
		cursor = page.nextOffset;
		ASSERT_LE(seen, QNFT_MAX_IMPORTS);
	} while (cursor != QNFT_REQUEST_ID_LIMIT);
	EXPECT_EQ(seen, QNFT_MAX_IMPORTS);
}

// Failed head entries rotate behind other recipients and every tick has bounded work.
TEST_F(NftTest, ImportDeliveryQueueIsBoundedAndDoesNotStarveOtherRecipients)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK); // The zero-initialized queue is empty.
	constexpr uint32 count = QNFT_IMPORT_DELIVERIES_PER_TICK + 2;
	static_assert(count <= 10);
	int displacedOwner = 0, displacedPossession = 0;
	for (uint32 i = 0; i < count; ++i)
	{
		QNFT::Item record{};
		char name[8] = "N000000";
		name[6] = static_cast<char>('0' + i);
		copyMem(&record.assetName, name, sizeof(name));
		record.origin = QNFT::Origin::Qbay;
		record.bridge = QNFT::Bridge::Requested;
		record.supply = 1;
		record.reserved = 1;
		record.reservedRecipient = bob;
		record.qbayId = i;
		int issuance, owner, possession;
		const char unit[7]{};
		ASSERT_EQ(issueAsset(nftId, name, 0, unit, 1, QNFT_CONTRACT_INDEX, &issuance, &owner, &possession), 1);
		if (i == 0)
		{
			// The first entry cannot deliver until custody is repaired, while later entries can.
			ASSERT_TRUE(transferShareOwnershipAndPossession(owner, possession, carol, 1, &displacedOwner, &displacedPossession, true));
		}
		state->items.set(i, record);
		state->imports.set(i, i);
		ASSERT_NE(state->importDeliveries.addTail(i), NULL_INDEX);
	}
	state->itemCount = count;
	auto col = state->collections.get(0);
	col.issuedItems = count;
	col.issuedUnits = count;
	state->collections.set(0, col);
	const auto fees = state->epochFees, beneficiaryFunds = getBalance(bob);
	const auto eventId = state->lastEventId;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->importDeliveries.population(), 3U);
	EXPECT_EQ(state->lastEventId - eventId, QNFT_IMPORT_DELIVERIES_PER_TICK - 1);
	for (uint32 i = 0; i < count; ++i)
	{
		EXPECT_EQ(balance(i, bob), i > 0 && i < QNFT_IMPORT_DELIVERIES_PER_TICK ? 1 : 0);
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->importDeliveries.population(), 1U);
	EXPECT_EQ(state->lastEventId - eventId, count - 1);
	EXPECT_EQ(state->itemCount, count);
	EXPECT_EQ(state->collections.get(0).issuedItems, count);
	EXPECT_EQ(state->collections.get(0).issuedUnits, count);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(bob), beneficiaryFunds);
	int owner, possession;
	ASSERT_TRUE(transferShareOwnershipAndPossession(displacedOwner, displacedPossession, nftId, 1, &owner, &possession, true));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->importDeliveries.population(), 0U);
	EXPECT_EQ(state->lastEventId - eventId, count);
	for (uint32 i = 0; i < count; ++i)
	{
		EXPECT_EQ(balance(i, bob), 1);
		EXPECT_EQ(state->items.get(i).bridge, QNFT::Bridge::Permanent);
		EXPECT_EQ(state->items.get(i).reserved, 0);
		EXPECT_EQ(state->items.get(i).reservedRecipient, NULL_ID);
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->lastEventId - eventId, count);
}

// A stored rate does not activate royalties without a beneficiary.
TEST_F(NftTest, MintAllowsMissingRoyaltyReceiverAndReportsZeroRoyalty)
{
	collection();
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.quantity = 2;
	in.receiver = alice;
	in.metadata = metadata();
	in.royaltyBps = 10000;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(item().royaltyReceiver, NULL_ID);
	EXPECT_EQ(static_cast<unsigned>(item().royaltyBps), 10000U);
	EXPECT_EQ(static_cast<unsigned>(NftExecutionProbe::effectiveRoyaltyBps(item())), 0U);
}

// Inactive royalties neither block listings nor consume sale proceeds or credit capacity.
TEST_F(NftTest, ShopIgnoresRoyaltyRateWithoutReceiver)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto record = state->items.get(0);
	record.royaltyReceiver = NULL_ID;
	record.royaltyBps = 10000;
	state->items.set(0, record);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto fees = state->epochFees, buyerFunds = getBalance(bob), sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	// The current shop rate still applies to a previously created listing.
	state->shopFeeBPS = 300;
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 2, 100, 1, 0}, bought, bob, 217));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.paid, 200);
	EXPECT_EQ(bought.fee, 6);
	EXPECT_EQ(bought.royalty, 0);
	EXPECT_EQ(getBalance(alice), sellerBefore + 194);
	EXPECT_EQ(getBalance(carol), royaltyBefore);
	EXPECT_EQ(state->epochFees, fees + 6);
	EXPECT_EQ(getBalance(bob), buyerFunds - 200);
	EXPECT_EQ(balance(0, bob), 2);
	expectQuCollateral();
}

TEST_F(NftTest, ImportedItemWithoutCreatorReportsNoRoyalty)
{
	collection();
	auto* qbay = reinterpret_cast<QBAY::StateData*>(contractStates[QBAY_CONTRACT_INDEX]);
	QBAY::InfoOfNFT original{};
	original.possessor = alice;
	original.royalty = 100;
	original.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
	original.URI.set(0, 'Q');
	qbay->NFTs.set(0, original);
	qbay->numberOfNFT = 1;
	const auto prepared = action(14, QNFT::PrepareImport_input{bob, 0, 0}, alice, TEST_MINT_FEE);
	ASSERT_EQ(prepared.code, QNFT::Code::Ok);
	QBAY::transfer_output deposited{};
	ASSERT_TRUE(invokeUserProcedure(QBAY_CONTRACT_INDEX, 5, QBAY::transfer_input{nftId, 0}, deposited, alice, 0));
	ASSERT_EQ(deposited.returnCode, QBAY::LogInfo::success);
	ASSERT_EQ(action(15, QNFT::FinalizeImport_input{0, prepared.itemId}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().royaltyReceiver, NULL_ID);
	EXPECT_EQ(static_cast<unsigned>(NftExecutionProbe::effectiveRoyaltyBps(item())), 0U);
	EXPECT_EQ(static_cast<unsigned>(item().royaltyBps), 10000U);
}

TEST_F(NftTest, BuyPaysSellerAndRoyaltyImmediately)
{
	collection();
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 3, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol), buyerBefore = getBalance(bob);
	const auto contractBefore = getBalance(nftId);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), sellerBefore + 27900);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 1500);
	EXPECT_EQ(getBalance(bob), buyerBefore - 30000);
	EXPECT_EQ(getBalance(nftId), contractBefore + 600);
	EXPECT_EQ(balance(0, bob), 3);
}

TEST_F(NftTest, BuyCombinesSellerAndRoyaltyPayments)
{
	collection();
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.quantity = 1;
	in.receiver = alice;
	in.metadata = metadata();
	in.royaltyBps = 9800;
	in.royaltyReceiver = alice;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 1, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 100, 1, 0}, bought, bob, 100));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.fee, 2);
	EXPECT_EQ(bought.royalty, 98);
	EXPECT_EQ(getBalance(alice), sellerBefore + 98);
}

TEST_F(NftTest, ShopHasNoEarningsWithdrawalOrQuery)
{
	EXPECT_EQ(contractUserProcedures[QNFT_CONTRACT_INDEX][23], nullptr);
	EXPECT_EQ(contractUserFunctions[QNFT_CONTRACT_INDEX][20], nullptr);
}

TEST_F(NftTest, RoyaltyCallbackObservesCommittedSaleAndDeliveredAssets)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][POST_INCOMING_TRANSFER] = observeRoyaltyPayment;
	collection();
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.quantity = 10;
	in.receiver = alice;
	in.metadata = metadata();
	in.royaltyBps = 500;
	in.royaltyReceiver = recipient;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	observedSaleListingId = listed.listingId;
	const auto beneficiaryBefore = getBalance(recipient);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1, 0}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(recipient), beneficiaryBefore + 1500);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 15);
	expectQuCollateral();
}

TEST_F(NftTest, RefundPaysContractImmediatelyWhenTransferIsAllowed)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	increaseEnergy(recipient, 123);
	const auto before = getBalance(recipient);
	EXPECT_EQ(action(9, QNFT::Transfer_input{}, recipient, 123).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(recipient), before);
	QNFT::ListRefunds_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, pending);
	EXPECT_EQ(pending.count, 0);
}

TEST_F(NftTest, FailedRefundRetriesOnTickWithoutManualWithdrawal)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	increaseEnergy(recipient, 123);
	const auto before = getBalance(recipient);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	const auto rejected = action(9, QNFT::Transfer_input{}, recipient, 123);
	contractCallbacksRunning = NoContractCallback;
	EXPECT_EQ(rejected.code, QNFT::Code::InvalidInput);
	QNFT::ListRefunds_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, pending);
	ASSERT_EQ(pending.count, 1U);
	EXPECT_EQ(pending.refunds.get(0).recipient, recipient);
	EXPECT_EQ(pending.refunds.get(0).amount, 123);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), before);
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, pending);
	EXPECT_EQ(pending.count, 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), before);
	EXPECT_EQ(contractUserProcedures[QNFT_CONTRACT_INDEX][25], nullptr);
	expectQuCollateral();
}

TEST_F(NftTest, RefundRetryIsBoundedAndPaysOrdinaryDestinations)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 index = 0; index < QNFT_REFUNDS_PER_TICK + 1; ++index)
	{
		const id recipient(100000 + index, 1, 0, 0);
		state->contractRefunds.set(recipient, 17);
		increaseEnergy(nftId, 17);
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->contractRefunds.population(), 1U);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->contractRefunds.population(), 0U);
	for (uint32 index = 0; index < QNFT_REFUNDS_PER_TICK + 1; ++index)
	{
		EXPECT_EQ(getBalance(id(100000 + index, 1, 0, 0)), 17);
	}
	expectQuCollateral();
}

TEST_F(NftTest, FailedTickRefundPreservesPendingAmountForNextTick)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->contractRefunds.set(recipient, MAX_AMOUNT);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	sint64 pending = 0;
	ASSERT_TRUE(state->contractRefunds.get(recipient, pending));
	EXPECT_EQ(pending, MAX_AMOUNT);
	EXPECT_EQ(getBalance(recipient), 0);
	// Repair the deliberately undercollateralized fixture and retry without a user command.
	state->contractRefunds.set(recipient, 19);
	increaseEnergy(nftId, 19);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 19);
	EXPECT_FALSE(state->contractRefunds.contains(recipient));
}

TEST_F(NftTest, ListItemsReturnsFullSnapshotsAndPageBoundaries)
{
	QNFT::ListItems_output page{};
	EXPECT_EQ(sizeof(QNFT::ItemEntry), 512U);
	EXPECT_EQ(offsetof(QNFT::ItemEntry, itemId), 504U);
	EXPECT_EQ(sizeof(page), 8208U);
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::ListItems_input{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.totalCount, 0U);
	collection(1000, 30);
	for (unsigned i = 0; i < 17; ++i)
	{
		ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	}
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 3, 0}).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::ListItems_input{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 16U);
	EXPECT_EQ(page.totalCount, 17U);
	EXPECT_EQ(page.nextOffset, 16U);
	for (uint32 i = 0; i < page.count; ++i)
	{
		QNFT::GetItem_output single{};
		callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetItem_input{i}, single);
		EXPECT_EQ(page.items.get(i).itemId, i);
		EXPECT_EQ(memcmp(&page.items.get(i).item, &single.item, sizeof(QNFT::Item)), 0);
		EXPECT_EQ(page.items.get(i).item.metadata.contentHash, metadata().contentHash);
	}
	EXPECT_EQ(page.items.get(0).item.burned, 3);
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::ListItems_input{16, 16}, page);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.items.get(0).itemId, 16U);
	EXPECT_EQ(page.nextOffset, 17U);
	EXPECT_EQ(page.items.get(1).item.supply, 0);
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::ListItems_input{17, 1}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, 17U);
	for (const auto input : {QNFT::ListItems_input{18, 1}, QNFT::ListItems_input{0, 0}, QNFT::ListItems_input{0, 17}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 8, input, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->itemCount = QNFT_MAX_ITEMS;
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::ListItems_input{QNFT_MAX_ITEMS - 1, 16}, page);
	EXPECT_EQ(page.count, 1U);
	EXPECT_EQ(page.items.get(0).itemId, QNFT_MAX_ITEMS - 1);
	EXPECT_EQ(page.nextOffset, QNFT_MAX_ITEMS);
}

TEST_F(NftTest, ListRefundsListsAllPositiveCreditsWithBoundedPages)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::ListRefunds_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.totalCount, 0U);
	EXPECT_EQ(page.nextOffset, QNFT_CONTRACT_REFUND_SLOTS);
	for (uint32 i = 0; i < 17; ++i)
	{
		state->contractRefunds.set(id(100000 + i, 1, 0, 0), 100 + i);
		increaseEnergy(nftId, 100 + i);
	}
	const id cleared(200000, 1, 0, 0);
	state->contractRefunds.set(cleared, 0); // A repayment may expose a temporarily cleared entry during its callback.
	const auto before = getBalance(nftId);
	bool seen[17]{};
	uint32 cursor = 0, totalSeen = 0;
	do
	{
		callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{cursor, 16}, page);
		ASSERT_EQ(page.code, QNFT::Code::Ok);
		EXPECT_EQ(page.totalCount, 17U);
		EXPECT_EQ(page.count, totalSeen == 0 ? 16U : 1U);
		for (uint32 i = 0; i < page.count; ++i)
		{
			const auto& entry = page.refunds.get(i);
			const auto index = entry.recipient.u64._0 - 100000;
			ASSERT_LT(index, 17U);
			EXPECT_FALSE(seen[index]);
			seen[index] = true;
			EXPECT_EQ(entry.recipient, id(100000 + index, 1, 0, 0));
			EXPECT_EQ(entry.amount, 100 + index);
		}
		totalSeen += page.count;
		cursor = page.nextOffset;
	} while (cursor != QNFT_CONTRACT_REFUND_SLOTS);
	EXPECT_EQ(totalSeen, 17U);
	EXPECT_EQ(getBalance(nftId), before);
	EXPECT_EQ(state->contractRefunds.population(), 18U);
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{cursor, 16}, page);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.totalCount, 17U);
	for (const auto query :
	     {QNFT::ListRefunds_input{0, 0}, QNFT::ListRefunds_input{0, 17}, QNFT::ListRefunds_input{QNFT_CONTRACT_REFUND_SLOTS + 1, 1}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 16, query, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	state->contractRefunds.removeByKey(cleared);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListRefunds_input{0, 16}, page);
	EXPECT_EQ(page.totalCount, 9U);
	EXPECT_EQ(page.count, 9U);
	expectQuCollateral();
}

TEST_F(NftTest, GetRefundReadsOneAddressIndependentlyOfPages)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->contractRefunds.set(alice, 123);
	state->contractRefunds.set(bob, 456);
	QNFT::GetRefund_output out{};
	EXPECT_EQ(sizeof(QNFT::GetRefund_input), 32U);
	EXPECT_EQ(sizeof(out), 16U);
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{alice}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 123);
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{bob}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 456);
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{carol}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 0);
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{NULL_ID}, out);
	EXPECT_EQ(out.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->contractRefunds.population(), 2U);
}

// A replaced operator must lose all three delegated capabilities immediately.
TEST_F(NftTest, ReplacedOperatorCannotTransferBurnOrExport)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::Approve_input{carol, 0}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, bob, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(13, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice), 10);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(item().burned, 0);
	// The replacement retains exactly the capabilities removed from the old operator.
	ASSERT_EQ(action(9, QNFT::Transfer_input{alice, bob, 1, 0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(13, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, carol).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 7);
	EXPECT_EQ(balance(0, bob), 1);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 1);
	EXPECT_EQ(item().burned, 1);
	expectQuCollateral();
}

// Revocation must affect execution, not merely the approval query.
TEST_F(NftTest, RevokedOperatorCannotTransferBurnOrExport)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(10, QNFT::Approve_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::Revoke_input{0}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(9, QNFT::Transfer_input{alice, bob, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(12, QNFT::Burn_input{alice, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(13, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice), 10);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(item().burned, 0);
	expectQuCollateral();
}

// A second failed refund must add to the first credit, rather than overwrite it.
TEST_F(NftTest, RepeatedFailedRefundsAccumulateAndPayExactlyOnce)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	increaseEnergy(recipient, 1000);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	const auto first = action(9, QNFT::Transfer_input{}, recipient, 123);
	const auto second = action(9, QNFT::Transfer_input{}, recipient, 456);
	contractCallbacksRunning = NoContractCallback;
	ASSERT_EQ(first.code, QNFT::Code::InvalidInput);
	ASSERT_EQ(second.code, QNFT::Code::InvalidInput);
	QNFT::GetRefund_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{recipient}, pending);
	EXPECT_EQ(pending.quantity, 579);
	EXPECT_EQ(getBalance(recipient), 421);
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 1000);
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{recipient}, pending);
	EXPECT_EQ(pending.quantity, 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 1000);
	expectQuCollateral();
}

// Failed recipients must not consume every future tick's retry budget.
TEST_F(NftTest, FailedRefundRecipientsDoNotStarveLaterPayments)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 index = 0; index < 10; ++index)
	{
		state->contractRefunds.set(id(900000 + index, 7, 8, 9), MAX_AMOUNT);
	}
	// Put the sole payable entry last in the map's traversal order.
	auto last = state->contractRefunds.nextElementIndex(NULL_INDEX);
	for (auto slot = last; slot != NULL_INDEX; slot = state->contractRefunds.nextElementIndex(slot))
	{
		last = slot;
	}
	const id recipient = state->contractRefunds.key(last);
	state->contractRefunds.set(recipient, 19);
	increaseEnergy(nftId, 19);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 19);
	EXPECT_FALSE(state->contractRefunds.contains(recipient));
	EXPECT_EQ(state->contractRefunds.population(), 9U);
	// Deliberately unfunded entries exercise failure without altering their liabilities.
	for (auto slot = state->contractRefunds.nextElementIndex(NULL_INDEX); slot != NULL_INDEX; slot = state->contractRefunds.nextElementIndex(slot))
	{
		EXPECT_EQ(state->contractRefunds.value(slot), MAX_AMOUNT);
	}
}

// Epoch revenue distribution and map compaction must leave deferred refunds fully backed.
TEST_F(NftTest, EpochPayoutPreservesAccumulatedRefundForFollowingTick)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = carol;
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	increaseEnergy(recipient, 579);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	const auto rejected = action(9, QNFT::Transfer_input{}, recipient, 579);
	contractCallbacksRunning = NoContractCallback;
	ASSERT_EQ(rejected.code, QNFT::Code::InvalidInput);
	// Force cleanup through churn while preserving the real, funded credit.
	for (uint32 index = 0; index < 1800; ++index)
	{
		const id removed(800000 + index, 7, 8, 9);
		state->contractRefunds.set(removed, 1);
		state->contractRefunds.removeByKey(removed);
	}

	const auto shareholderBefore = getBalance(alice);
	const auto developerBefore = getBalance(carol);
	const auto reserveBefore = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	ASSERT_EQ(state->epochFees, 2000000);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	EXPECT_EQ(getBalance(carol), developerBefore + 400000);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserveBefore + 400000);
	EXPECT_EQ(getBalance(nftId), 679); // 579 refund plus 100 dividend rounding carry.
	QNFT::GetRefund_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 15, QNFT::GetRefund_input{recipient}, pending);
	EXPECT_EQ(pending.quantity, 579);
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 579);
	EXPECT_EQ(getBalance(nftId), 100);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 579);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	expectQuCollateral();
}
