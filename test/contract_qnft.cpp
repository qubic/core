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
#include <set>

namespace
{
template<class T> bool supportsAnyIssuerScan()
{
    return requires(T& iterator, const QPI::id& owner) { iterator.beginAnyIssuer(owner, 0); };
}

template<class T> bool supportsDecimalPlaces()
{
    return requires(const T& iterator) { iterator.numberOfDecimalPlaces(); };
}
}

TEST(QnftRegistrationQpi, RequiredReadOnlyQueriesAreAvailable)
{
    EXPECT_TRUE(supportsAnyIssuerScan<QPI::AssetOwnerIterator>());
    EXPECT_TRUE(supportsDecimalPlaces<QPI::AssetIssuanceIterator>());
}

namespace
{
	// Test-only request builder; run() invokes each dedicated public ABI.
	struct OperationCommand
	{
		id owner;                  // Account whose holdings, authorization or credit are addressed.
		id receiver;               // Destination account for this operation.
		id royaltyReceiver;        // Account paid creator royalties on shop sales.
		QNFT::Metadata metadata;   // URI and content digest for this object.
		sint64 quantity;           // Number of native asset units requested or returned.
		sint64 maxUnits;           // Lifetime edition-unit issuance cap for the collection.
		sint64 offeredFee;         // Maximum QU offered to the receiving asset manager.
		uint32 itemId;             // Zero-based item identifier; meaningful in item operations.
		uint32 collectionId;       // Zero-based collection identifier.
		uint32 maxItems;           // Maximum number of distinct items allowed.
		QNFT::Operation operation; // Public NFT operation selected by the test.
		uint16 royaltyBps;         // Creator royalty in basis points; 10000 means 100%.
		uint16 destination;        // Contract index receiving asset management rights.
		QNFT::Policy policy;       // Rules governing metadata changes.
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

	void forwardNftFromReceiver(const QpiContextProcedureCall& qpi, void*, void* input, void* output, void*)
	{
		auto& result = *static_cast<QNFT::TransferFrom_output*>(output);
		if (qpi.invocator() != alice)
		{
			result.code = QNFT::Code::Unauthorized;
			return;
		}
		QNFT::TransferFrom_input request = *static_cast<QNFT::TransferFrom_input*>(input);
		QPI::InterContractCallError error;
		const auto* context = qpi.__qpiConstructProcedureCallContext(QNFT_CONTRACT_INDEX, qpi.invocationReward(), error);
		if (!context)
		{
			result.code = QNFT::Code::ExternalFailure;
			return;
		}
		auto* state = static_cast<QPI::ContractState<QNFT::StateData, QNFT_CONTRACT_INDEX>*>(qpi.__qpiAcquireStateForWriting(QNFT_CONTRACT_INDEX));
		auto* locals = static_cast<QNFT::TransferFrom_locals*>(qpi.__qpiAllocLocals(sizeof(QNFT::TransferFrom_locals)));
		QNFT::TransferFrom(*context, *state, request, result, *locals);
		qpi.__qpiFreeLocals();
		qpi.__qpiReleaseStateForWriting(QNFT_CONTRACT_INDEX);
		qpi.__qpiFreeContext();
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
		tariffState->listingFee = 0;
		tariffState->proposalFee = 0;

		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
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
				return action(2, request, caller, reward);
			}
			case QNFT::Operation::AcceptAdmin:
			{
				QNFT::AcceptAdmin_input request{};
				request.collectionId = in.collectionId;
				return action(3, request, caller, reward);
			}
			case QNFT::Operation::Mint:
			{
				QNFT::Mint_input request{};
				request.receiver = in.receiver;
				request.royaltyReceiver = in.royaltyReceiver;
				request.metadata = in.metadata;
				request.quantity = in.quantity;
				request.collectionId = in.collectionId;
				request.royaltyBps = in.royaltyBps;
				request.policy = in.policy;
				return action(6, request, caller, reward);
			}
			case QNFT::Operation::Transfer:
			{
				QNFT::TransferFrom_input request{};
				request.owner = in.owner;
				request.receiver = in.receiver;
				request.quantity = in.quantity;
				request.itemId = in.itemId;
				return action(12, request, caller, reward);
			}
			case QNFT::Operation::UpdateMetadata:
			{
				QNFT::UpdateMetadata_input request{};
				request.metadata = in.metadata;
				request.itemId = in.itemId;
				return action(9, request, caller, reward);
			}
			case QNFT::Operation::FreezeMetadata:
			{
				QNFT::FreezeMetadata_input request{};
				request.itemId = in.itemId;
				return action(10, request, caller, reward);
			}
			case QNFT::Operation::Export:
			{
				QNFT::Export_input request{};
				request.owner = in.owner == NULL_ID ? caller : in.owner;
				request.quantity = in.quantity;
				request.offeredFee = in.offeredFee;
				request.itemId = in.itemId;
				request.destination = in.destination;
				return action(16, request, caller, reward);
			}
			case QNFT::Operation::Burn:
			{
				QNFT::Burn_input request{};
				request.owner = in.owner == NULL_ID ? caller : in.owner;
				request.quantity = in.quantity;
				request.itemId = in.itemId;
				return action(14, request, caller, reward);
			}
			case QNFT::Operation::FundReserve:
			{
				QNFT::FundReserve_input request{};
				return action(26, request, caller, reward);
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

	QNFT::OperationResult mint(sint64 quantity = 100, QNFT::Policy policy = QNFT::Policy::Immutable, uint32 collectionId = 0)
	{
		OperationCommand in{};
		in.collectionId = collectionId;
		in.operation = QNFT::Operation::Mint;
		in.quantity = quantity;
		in.receiver = alice;

		in.royaltyReceiver = carol;
		in.metadata = metadata();
		in.policy = policy;
		in.royaltyBps = 500;
		return run(in, alice, TEST_MINT_FEE);
	}

	sint64 balance(uint32 item, const id& owner, uint16 manager = 0)
	{
		QNFT::GetBalance_input in{owner, item, manager};
		QNFT::GetBalance_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 15, in, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.quantity;
	}

	QNFT::Item item(uint32 itemId = 0)
	{
		QNFT::GetItem_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 4, QNFT::GetItem_input{itemId}, out);
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
		sint64 liabilities = state->epochFees + state->developerPending + state->shareholderCarry + state->rentalDeposits + state->rentalCredits;
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
	};

	struct ShopBuyOutput
	{
		sint64 paid;
		sint64 fee;
		sint64 royalty;
		sint64 shareholderAmount;
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
	};

	struct FeeBatchPayloadWire
	{
		FeeBatchEntryWire changes[16];
		uint32 count;
	};

	struct FeeBatchOutputWire
	{
		FeeBatchPayloadWire batch;
		uint32 proposalTick;
		QNFT::Code code;
	};

	static_assert(sizeof(FeeBatchInputWire) == 520);
	static_assert(sizeof(FeeBatchOutputWire) == 272);
} // namespace

TEST_F(NftFeeTest, BatchProposalChargesOnceAndAppliesAllValuesAfterEpochPayout)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][27], nullptr);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000017));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), before - 1000000);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->epochFees, 1000000);
	EXPECT_EQ(state->transferFee, 10000);
	const auto stored = proposal(slot);
	EXPECT_EQ(static_cast<uint32>(stored.type), static_cast<uint32>(ProposalTypes::MultiVariablesYesNo));
	FeeBatchOutputWire batch{};
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, batch);
	ASSERT_EQ(batch.code, QNFT::Code::Ok);
	EXPECT_EQ(batch.batch.count, 4U);
	EXPECT_EQ(batch.batch.changes[1].value, 20000);
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
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][27], nullptr);
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto before = getBalance(alice);
	for (unsigned n = 0; n < 8; ++n)
	{
		FeeBatchInputWire input{};
		input.epoch = system.epoch;
		input.count = 2;
		input.changes[0] = {0, 77};
		input.changes[1] = {6, 88};
		switch (n)
		{
			case 0: input.count = 0; break;
			case 1: input.count = QNFT_FEE_VARIABLE_COUNT + 1; break;
			case 2: input.changes[1].variable = 0; break;
			case 3: input.changes[1].variable = QNFT_FEE_VARIABLE_COUNT; break;
			case 4: input.changes[1].value = -1; break;
			case 5: input.changes[1].value = MAX_AMOUNT + 1; break;
			case 6:
				input.changes[0] = {3, 7000};
				input.changes[1] = {4, 2000};
				break;
			case 7: input.changes[13].value = 1; break;
		}
		uint16 slot = INVALID_PROPOSAL_INDEX;
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000017));
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto old = proposal(slot);
	ASSERT_TRUE(ballot(slot, old));
	++system.tick;
	input.changes[1].value = 99;
	uint16 replacement = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, replacement, alice, 1000000));
	EXPECT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(slot));
	EXPECT_FALSE(ballot(slot, old));
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, out);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.batch.changes[1].value, 99);
	// Rejected replacement must preserve the previous proposal, its payload and all QU.
	const auto before = getBalance(alice);
	input.changes[1].variable = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, replacement, alice, 1000017));
	EXPECT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), before);
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.batch.changes[1].value, 99);
	QNFT::SetShareholderProposal_input cancel{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 65534, cancel, replacement, alice, 17));
	EXPECT_EQ(getBalance(alice), before);
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, out);
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
	input.changes[0] = {11, 0};
	input.changes[1] = {6, MAX_AMOUNT};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	const auto aliceBefore = getBalance(alice), bobBefore = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 999999));
	EXPECT_EQ(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, bob, 1000000));
	EXPECT_EQ(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(alice), aliceBefore);
	EXPECT_EQ(getBalance(bob), bobBefore);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->proposalFee, 0);
	EXPECT_EQ(state->transferFee, MAX_AMOUNT);
	++system.epoch;
	++system.tick;
	input.changes[1].value = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 0));
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto corrupted = state->feeBatches.get(slot);
	corrupted.changes.set(1, QNFT::FeeChange{QNFT::FeeVariable::TransferFee, 99});
	state->feeBatches.set(slot, corrupted);
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, out);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 1000000));
	ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	++system.tick;
	const auto replacement = propose(1, 123, alice, 1000000);
	ASSERT_EQ(static_cast<uint32>(replacement), static_cast<uint32>(slot));
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	FeeBatchOutputWire out{};
	callFunction(QNFT_CONTRACT_INDEX, 33, QNFT::GetShareholderProposal_input{slot}, out);
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
	using QNFT::ExecuteNominateAdmin;
	using QNFT::ExecuteSetApprovalForAll;
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

	QNFT::TransferFrom_input transfer{};
	QNFT::ExecuteTransfer_locals transferLocals{};
	NftExecutionProbe::ExecuteTransfer(qpi, state, transfer, output, transferLocals);
	EXPECT_EQ(output.code, QNFT::Code::InsufficientFee);

	QNFT::SetApprovalForAll_input approve{bob, true};
	QNFT::ExecuteSetApprovalForAll_locals approveLocals{};
	NftExecutionProbe::ExecuteSetApprovalForAll(qpi, state, approve, output, approveLocals);
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
	callFunction(QNFT_CONTRACT_INDEX, 31, NoData{}, fees);
	EXPECT_EQ(fees.collectionFee, 1000000);
	EXPECT_EQ(fees.mintFee, 100000);
	EXPECT_EQ(fees.transferFee, 10000);
	EXPECT_EQ(fees.updateMetadataFee, 50000);
	EXPECT_EQ(fees.proposalFee, 1000000);
	EXPECT_EQ(fees.shopFeeBPS, 30U);
	collection();
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto before = getBalance(alice), pool = state->epochFees;
	QNFT::TransferFrom_input transfer{alice, bob, 3, 0};
	EXPECT_EQ(action(12, transfer, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(action(12, transfer, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - 10000);
	EXPECT_EQ(state->epochFees, pool + 10000);
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(action(12, transfer, bob, 10017).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->epochFees, pool + 10000);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://new"), 0}, alice, 50017).code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 60000);
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{0}, alice, 0).code, QNFT::Code::Ok);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}, alice, 0).code, QNFT::Code::Ok);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 2, 0}, listing, alice, 9999));
	EXPECT_EQ(listing.code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 2, 0}, listing, alice, 10017));
	ASSERT_EQ(listing.code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees, pool + 10000);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listing.listingId}, cancelled, alice, 0));
	EXPECT_EQ(cancelled.code, QNFT::Code::Ok);
	QNFT::Export_input exportInput{alice, 3, 1000000, 0, QX_CONTRACT_INDEX};
	EXPECT_EQ(action(16, exportInput, alice, 1009999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(balance(0, alice), 100);
	EXPECT_EQ(action(16, exportInput, alice, 1010017).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before - 10000 - 10000 - 100);
	EXPECT_EQ(state->epochFees, pool + 20000);
	EXPECT_EQ(balance(0, alice, QX_CONTRACT_INDEX), 3);
	expectQuCollateral();
}

TEST_F(NftFeeTest, AdministrationApprovalsRespectPaidAndFreeBoundaries)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob, 0).code, QNFT::Code::Ok);
	QNFT::SetApprovalForAll_input approval{bob, true};
	EXPECT_EQ(action(11, approval, alice, 9999).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(action(11, approval, alice, 10017).code, QNFT::Code::Ok);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}, alice, 0).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftFeeTest, EveryNewTariffIsGovernedButRecoveryAndDeveloperRatesAreNot)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint64 key : {5ULL, 6ULL, 7ULL, 8ULL, 9ULL, 10ULL, 11ULL})
	{
		const auto charge = state->proposalFee, before = getBalance(alice);
		const auto slot = propose(key, 321, alice, charge + 17);
		ASSERT_NE(static_cast<uint32>(slot), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_EQ(getBalance(alice), before - charge);
		ASSERT_TRUE(ballot(slot, proposal(slot)));
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
		QNFT::GetFeeSchedule_output fees{};
		callFunction(QNFT_CONTRACT_INDEX, 31, NoData{}, fees);
		const sint64 values[] = {fees.nominateAdminFee, fees.transferFee, fees.approveFee, fees.updateMetadataFee,
		                         fees.exportFee,        fees.listingFee,  fees.proposalFee};
		EXPECT_EQ(values[key - 5], 321);
		EXPECT_EQ(state->developerFeeBPS, 2000U);
		++system.epoch;
		++system.tick;
	}
	EXPECT_EQ(static_cast<uint32>(propose(QNFT_FEE_VARIABLE_COUNT, 1, alice, 10000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
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
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{bob}, alice, 100).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->developer, original);
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{bob}, original, 100).code, QNFT::Code::Ok);
	EXPECT_EQ(state->developer, bob);
	EXPECT_EQ(getBalance(original), balanceBefore);
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{carol}, original).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(state->developer, bob);
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{carol}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(state->developer, carol);
}

TEST_F(NftTest, ChangeDeveloperRejectsInvalidRecipientsAndPreservesPendingRevenue)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->developer = alice;
	state->developerPending = 123;
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{NULL_ID}).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->developer, alice);
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{nftId}).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->developer, alice);
	EXPECT_EQ(action(28, QNFT::ChangeDeveloper_input{bob}).code, QNFT::Code::Ok);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{5000, 3, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1}, bought, bob, 5000));
	EXPECT_EQ(bought.fee, 150);
	state->shopFeeBPS = 9600; // Existing item's 5% royalty no longer fits.
	const auto before = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1}, bought, bob, 5000));
	EXPECT_EQ(bought.code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(bob), before);
	state->shopFeeBPS = 0;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 5000, 1}, bought, bob, 5000));
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
	callFunction(QNFT_CONTRACT_INDEX, 32, NoData{}, cfg);
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

	for (const auto row : {Invalid{0, -1}, Invalid{1, MAX_AMOUNT + 1}, Invalid{2, 10001}, Invalid{3, 8001}, Invalid{4, 8001}, Invalid{QNFT_FEE_VARIABLE_COUNT, 2000},
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
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, refund);
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
	request.maxItems = QNFT_MAX_ITEMS + 1;
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
	EXPECT_EQ(state->collections.get(0).creator, alice);
	EXPECT_EQ(state->collections.get(0).administrator, NULL_ID);
	EXPECT_EQ(state->collections.get(0).nominatedAdministrator, NULL_ID);
	expectQuCollateral();
}

TEST_F(NftTest, PublicEntryPointsHaveContiguousIds)
{
	for (unsigned short procedure = 1; procedure <= 29; ++procedure)
	{
		EXPECT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][procedure], nullptr) << procedure;
	}
	EXPECT_EQ(contractUserProcedures[QNFT_CONTRACT_INDEX][30], nullptr);
	for (unsigned short function = 1; function <= 36; ++function)
	{
		EXPECT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][function], nullptr) << function;
	}
	EXPECT_EQ(contractUserFunctions[QNFT_CONTRACT_INDEX][37], nullptr);
}

TEST_F(NftTest, CompactRecoveryProceduresRefundMissingItemsWithoutChangingBalances)
{
	const auto before = getBalance(alice);
	QNFT::GetItem_input request{};
	for (unsigned short procedure : {10})
	{
		const auto out = action(procedure, request, alice, 123);
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
	EXPECT_EQ(offsetof(QNFT::GovernanceEvent, _terminator), 72U);
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
		setMem(&input.royaltyReceiver, 32, 4);
		input.metadata = metadata();
		input.quantity = 3;
		input.collectionId = 2;
		input.royaltyBps = 500;
		input.policy = QNFT::Policy::Mutable;
		EXPECT_EQ(sizeof(input), 384U);
		std::ofstream file("abi-compact-Mint.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
	{
		QNFT::TransferFrom_input input{};
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
		QNFT::FundReserve_input input{};
		std::ofstream file("abi-compact-FundReserve.bin", std::ios::binary);
		file.write(reinterpret_cast<const char*>(&input), sizeof(input));
		ASSERT_TRUE(file.good());
	}
}

TEST_F(NftTest, ShopEscrowsPartialSaleAndChargesExactlyPointThreePercent)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][17], nullptr);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ShopListOutput listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	ASSERT_NE(listed.listingId, 0);
	EXPECT_EQ(balance(0, alice), 90);
	EXPECT_EQ(balance(0, nftId), 10);
	const auto buyerBalance = getBalance(bob), sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	const auto reserve = getContractFeeReserve(QNFT_CONTRACT_INDEX);
	ShopBuyOutput bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1}, bought, bob, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.paid, 30000);
	EXPECT_EQ(bought.fee, 90);
	EXPECT_EQ(bought.royalty, 1500);
	EXPECT_EQ(getBalance(bob), buyerBalance - 30000);
	EXPECT_EQ(balance(0, bob), 3);
	EXPECT_EQ(balance(0, nftId), 7);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve); // No burn before END_EPOCH.
	EXPECT_EQ(getBalance(alice), sellerBefore + 28410);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 1500);
	expectQuCollateral();
}

TEST_F(NftTest, ShopRejectsUnfundedStaleAndUnauthorizedPurchasesWithoutLosingEscrow)
{
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][17], nullptr);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ShopListOutput listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto buyerBalance = getBalance(bob);
	for (const auto& command : {ShopBuyInput{bob, listed.listingId, 3, 9999, 1}, ShopBuyInput{bob, listed.listingId, 3, 10000, 2},
	                            ShopBuyInput{bob, listed.listingId, 11, 10000, 1}})
	{
		ShopBuyOutput out{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, command, out, bob, 30000));
		EXPECT_NE(out.code, QNFT::Code::Ok);
		EXPECT_EQ(getBalance(bob), buyerBalance);
		EXPECT_EQ(balance(0, nftId), 10);
	}
	ShopBuyOutput out{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1}, out, bob, 29999));
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, ShopListInput{10000, 10, 0}, listed, alice, 0));
	ShopBuyOutput bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, ShopBuyInput{bob, listed.listingId, 3, 10000, 1}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	const auto shareholderBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 400018);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getContractFeeReserve(QNFT_CONTRACT_INDEX), reserve + 400018);
	EXPECT_EQ(getBalance(alice), shareholderBefore + 1199900);
	EXPECT_EQ(getBalance(carol), royaltyBefore);
	expectQuCollateral();
}

TEST_F(NftTest, ShopCancellationReturnsOnlyUnsoldUnitsAndListingIdsNeverRepeat)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0}, listed, alice, 0));
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1}, bought, bob, 30000));
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{20000, 7, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	EXPECT_GT(listed.listingId, oldId);
	const auto buyerBalance = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, oldId, 1, 10000, 1}, bought, bob, 10000));
	EXPECT_EQ(bought.code, QNFT::Code::NotFound);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	QNFT::GetListing_output listing{};
	callFunction(QNFT_CONTRACT_INDEX, 20, QNFT::GetListing_input{listed.listingId}, listing);
	EXPECT_EQ(listing.code, QNFT::Code::Ok);
	EXPECT_EQ(listing.listing.quantity, 7);
	QNFT::ListListings_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 21, QNFT::ListListings_input{0, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1);
	EXPECT_EQ(page.listingIds.get(0), listed.listingId);
}

TEST_F(NftTest, ShopMetadataChangesInvalidateListingsAndOverflowNeverChargesBuyer)
{
	collection();
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{MAX_AMOUNT, 2, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	const auto buyerBalance = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 2, MAX_AMOUNT, 1}, bought, bob, 123));
	EXPECT_EQ(bought.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(bob), buyerBalance);
	OperationCommand update{};
	update.operation = QNFT::Operation::UpdateMetadata;
	update.metadata = metadata("ipfs://changed");
	ASSERT_EQ(run(update).code, QNFT::Code::Ok);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, MAX_AMOUNT, 1}, bought, bob, 123));
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0}, listed, alice, 0));
	increaseEnergy(malicious, 100000);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1}, bought, malicious, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::ListRefunds_output refund{};
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, refund);
	EXPECT_EQ(refund.count, 0);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 2);
	const auto paidBefore = getBalance(malicious);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(getBalance(malicious), paidBefore + 1599918);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 4);
	EXPECT_EQ(issuanceIndex(nftId, nextNameForIssuerAttack), NO_ASSET_INDEX);
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
	expectQuCollateral();
}

namespace
{
	struct TransferLogWire
	{
		uint32 contractIndex, kind, type;
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
			if ((sizeAndType >> 24) != CONTRACT_INFORMATION_MESSAGE || (sizeAndType & 0xffffff) != 124)
			{
				continue;
			}
			TransferLogWire event{};
			memcpy(&event, buffer + LOG_HEADER_SIZE, 124);
			if (event.contractIndex == QNFT_CONTRACT_INDEX && event.kind == static_cast<uint32>(QNFT::EventKind::TransferSingle) && event.type == 0)
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
		uint32 sizeAndType = 0, contract = 0, type = 0, kind = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		memcpy(&contract, buffer + LOG_HEADER_SIZE, 4);
		memcpy(&kind, buffer + LOG_HEADER_SIZE + 4, 4);
		memcpy(&type, buffer + LOG_HEADER_SIZE + 8, 4);
		if ((sizeAndType >> 24) == CONTRACT_INFORMATION_MESSAGE && (sizeAndType & 0xffffff) == 272 && contract == QNFT_CONTRACT_INDEX &&
		    kind == static_cast<uint32>(QNFT::EventKind::URI) && type == 1)
		{
			result.emplace_back(buffer + LOG_HEADER_SIZE + 12);
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
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][34], nullptr);
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][35], nullptr);
	JournalRangeWire range{};
	callFunction(QNFT_CONTRACT_INDEX, 34, QPI::NoData{}, range);
	EXPECT_EQ(range.first, 1U);
	EXPECT_EQ(range.next, 1U);
	EXPECT_EQ(range.count, 0U);
	EXPECT_EQ(range.capacity, 65536U);
	collection();
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 34, QPI::NoData{}, range);
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
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{0, 2}, page);
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
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{page.cursor, 16}, page);
	ASSERT_EQ(page.count, 3U);
	EXPECT_EQ(page.cursor, 6U);
	EXPECT_EQ(page.events.get(0).kind, QNFT::EventKind::URI);
	const auto minted = item();
	EXPECT_EQ(memcmp(&page.events.get(0).uri, &minted.metadata.uri, 256), 0);
	EXPECT_EQ(page.events.get(1).from, nftId);
	EXPECT_EQ(page.events.get(1).to, alice);
	const auto firstUri = page.events.get(0).uri;
	ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://new"), 0}).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{3, 1}, page);
	EXPECT_EQ(memcmp(&page.events.get(0).uri, &firstUri, 256), 0);
	const auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto last = state->lastEventId;
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 99, 0}).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(state->lastEventId, last);
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{last + 1, 16}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	for (auto request : {QNFT::GetEvents_input{0, 0}, QNFT::GetEvents_input{0, 17}, QNFT::GetEvents_input{last + 2, 1}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 35, request, page);
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
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_EVENT_CAPACITY + 1ULL);
	EXPECT_EQ(state->journal.get(0).eventId, QNFT_EVENT_CAPACITY + 1ULL);
	QNFT::GetEvents_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{1, 16}, page);
	EXPECT_EQ(page.code, QNFT::Code::NotFound);
	EXPECT_EQ(page.firstEventId, 2U);
	EXPECT_EQ(page.count, 0U);
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{QNFT_EVENT_CAPACITY, 16}, page);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::GetEvents_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{6, 16}, page);
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
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{8, 16}, page);
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
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_LAST_EVENT_ID);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, QNFT_LAST_EVENT_ID);
	JournalRangeWire range{};
	callFunction(QNFT_CONTRACT_INDEX, 34, QPI::NoData{}, range);
	EXPECT_EQ(range.next, 0xffffffffffffffffULL);
}

TEST_F(NftTest, ShopEventsTrackEscrowAndExportDoesNotChangeOwnership)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 7, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{carol, listed.listingId, 3, 10000, 1}, bought, bob, 30000));
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
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 2, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 3, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 6, 0}, bob).code, QNFT::Code::InsufficientBalance);
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
	ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{changed, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::FreezeMetadata_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://rejected"), 0}).code, QNFT::Code::Frozen);
	QNFT::GetItem_output result{};
	EXPECT_EQ(sizeof(result), 480U);
	EXPECT_EQ(offsetof(QNFT::GetItem_output, code), 472U);
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][4], nullptr);
	callFunction(QNFT_CONTRACT_INDEX, 4, QNFT::GetItem_input{0}, result);
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
		uint32 sizeAndType = 0, contract = 0, type = 0, kind = 0;
		memcpy(&sizeAndType, buffer + 6, 4);
		memcpy(&contract, buffer + LOG_HEADER_SIZE, 4);
		memcpy(&kind, buffer + LOG_HEADER_SIZE + 4, 4);
		memcpy(&type, buffer + LOG_HEADER_SIZE + 8, 4);
		if ((sizeAndType >> 24) != CONTRACT_INFORMATION_MESSAGE || contract != QNFT_CONTRACT_INDEX || kind != static_cast<uint32>(QNFT::EventKind::URI) || type != 1)
		{
			continue;
		}
		ASSERT_EQ(sizeAndType & 0xffffff, 272U);
		if (++count == 2)
		{
			EXPECT_EQ(memcmp(buffer + LOG_HEADER_SIZE + 12, &result.item.metadata.uri, 256), 0);
		}
	}
	EXPECT_EQ(count, 2U);
	callFunction(QNFT_CONTRACT_INDEX, 4, QNFT::GetItem_input{999}, result);
	EXPECT_EQ(result.code, QNFT::Code::NotFound);
}

TEST_F(NftTest, TransferSettlesFeesAndReportsRejectedItem)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->transferFee = 7;
	const auto funds = getBalance(alice), fees = state->epochFees;
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	const QNFT::TransferFrom_input rejected{alice, NULL_ID, 2, 1};
	EXPECT_EQ(action(12, rejected, alice, 17).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(balance(1, alice), 10);
	qLogger::updateTick(system.tick);
	const auto range = qLogger::tx.getLogIdInfo(0, system.tick, 0);
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

		ASSERT_EQ(sizeAndType & 0xffffffU, 68U);
		QNFT::FailureEvent event{};
		memcpy(&event, buffer + LOG_HEADER_SIZE, 68);
		EXPECT_EQ(event.contractIndex, QNFT_CONTRACT_INDEX);
		EXPECT_EQ(event.operation, QNFT::Operation::Transfer);
		EXPECT_EQ(event.actor, alice);
		EXPECT_EQ(event.itemId, 1U);
		EXPECT_EQ(event.code, QNFT::Code::InvalidInput);
		++errors;
	}

	EXPECT_EQ(errors, 1U);
	const QNFT::TransferFrom_input accepted{alice, bob, 2, 1};
	EXPECT_EQ(action(12, accepted, alice, 6).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(state->epochFees, fees);
	const auto result = action(12, accepted, alice, 17);
	ASSERT_EQ(result.code, QNFT::Code::Ok);
	EXPECT_EQ(result.itemId, 1U);
	EXPECT_EQ(result.quantity, 2);
	EXPECT_EQ(result.assetName, item(1).assetName);
	EXPECT_EQ(getBalance(alice), funds - 7);
	EXPECT_EQ(state->epochFees, fees + 7);
	EXPECT_EQ(balance(1, alice), 8);
	EXPECT_EQ(balance(1, bob), 2);
	expectQuCollateral();
}

TEST_F(NftTest, RejectedCommandsAndShopActionsEmitOneErrorEach)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	qLogger::registerNewTx(system.tick, 1);
	QNFT::TransferFrom_input transfer{};
	transfer.owner = alice;
	transfer.receiver = bob;
	transfer.quantity = 1;
	transfer.itemId = 999;
	EXPECT_EQ(action(12, transfer).code, QNFT::Code::NotFound);
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
	EXPECT_EQ(action(16, exportInput).code, QNFT::Code::ExternalFailure);
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
	reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->developer = NULL_ID;
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
	reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->developer = NULL_ID;
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	const auto aliceBefore = getBalance(alice), bobBefore = getBalance(bob);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	QNFT::GetMarket_output market{};
	callFunction(QNFT_CONTRACT_INDEX, 22, QNFT::GetMarket_input{}, market);
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
	callFunction(QNFT_CONTRACT_INDEX, 22, QNFT::GetMarket_input{}, market);
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
	in.royaltyBps = 9970;
	in.royaltyReceiver = carol;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 1, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 10000, 1}, bought, bob, 10000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), sellerBefore);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 9970);
	EXPECT_EQ(bought.fee, 30);
	EXPECT_EQ(bought.royalty, 9970);
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
	reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->developer = NULL_ID;
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

	for (const Row row : {Row{1, 0, 0}, Row{333, 0, 16}, Row{334, 1, 16}, Row{666, 1, 33}, Row{667, 2, 33}, Row{1000, 3, 50}})
	{
		QNFT::ListForSale_output listed{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{row.price, 1, 0}, listed, alice, 0));
		ASSERT_EQ(listed.code, QNFT::Code::Ok);
		QNFT::Buy_output bought{};
		ASSERT_TRUE(
		    invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, row.price, 1}, bought, bob, row.price + 13));
		ASSERT_EQ(bought.code, QNFT::Code::Ok);
		EXPECT_EQ(bought.fee, row.fee);
		EXPECT_EQ(bought.royalty, row.royalty);
		QNFT::GetListing_output closed{};
		callFunction(QNFT_CONTRACT_INDEX, 20, QNFT::GetListing_input{listed.listingId}, closed);
		EXPECT_EQ(closed.code, QNFT::Code::NotFound);
	}
	EXPECT_EQ(getBalance(alice), sellerBefore + 2846);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 148);
	QNFT::GetMarket_output market{};
	callFunction(QNFT_CONTRACT_INDEX, 22, QNFT::GetMarket_input{}, market);
	EXPECT_EQ(market.epochFees, 2000007);
	EXPECT_EQ(getBalance(nftId), 2000007);
	EXPECT_EQ(balance(0, bob), 6);
	EXPECT_EQ(balance(0, nftId), 0);
}

TEST_F(NftTest, OperatorBurnUsesOwnerBalanceAndPreservesApproval)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 11, 0}, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 3, 0}, carol).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 3, 0}, bob, 17).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 2, 0}, bob, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 5);
	EXPECT_EQ(item().burned, 5);
	EXPECT_EQ(getBalance(bob), before);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Unauthorized);
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
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
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

TEST_F(NftTest, OperatorApprovalPersistsAcrossTimeAndRequiresCurrentOwnerBalance)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 8, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 3, 0}, bob).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 3, 0, 0, TESTEXA_CONTRACT_INDEX}, bob).code, QNFT::Code::InsufficientBalance);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(balance(0, alice), 2);
	EXPECT_EQ(item().burned, 8);
	utcTime.Year = 2030;
	updateQpiTime();
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 1);
	EXPECT_EQ(item().burned, 9);
	expectQuCollateral();
}

TEST_F(NftTest, FailedDelegatedExportPreservesApprovalAndRefundsUnspentReward)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
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
	QNFT::IsApprovedForAll_output approvalAfter{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approvalAfter);
	EXPECT_TRUE(approvalAfter.approved);
	expectQuCollateral();
}

// Burning does not consume a transfer permission or bypass the remaining balance.
TEST_F(NftTest, PartialBurnPreservesApprovalAndOperatorStillNeedsBalance)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	QNFT::SetApprovalForAll_input approval{bob, true};
	ASSERT_EQ(action(11, approval).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 11, 0}).code, QNFT::Code::InsufficientBalance);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 4, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 6);
	EXPECT_EQ(item().burned, 4);
	QNFT::IsApprovedForAll_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, remaining);
	EXPECT_TRUE(remaining.approved);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 7, 0}, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice), 6);
	EXPECT_EQ(balance(0, carol), 0);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 3, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 3);
	EXPECT_EQ(balance(0, carol), 3);
	expectQuCollateral();
}

TEST_F(NftTest, FullBalanceBurnPreservesApprovalForSubsequentlyReceivedUnits)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 4, 0}).code, QNFT::Code::Ok);
	QNFT::SetApprovalForAll_input approval{carol, true};
	ASSERT_EQ(action(11, approval).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 6, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 0);
	QNFT::IsApprovedForAll_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, carol}, remaining);
	EXPECT_TRUE(remaining.approved);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}, carol).code, QNFT::Code::InsufficientBalance);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{bob, alice, 2, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}, carol).code, QNFT::Code::Ok);
	// A fresh approval remains explicitly revocable after the final unit is burned.
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, false}).code, QNFT::Code::Ok);
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
	QNFT::SetApprovalForAll_input approval{carol, true};
	QNFT::Burn_input burn{};
	burn.owner = bob;
	burn.quantity = 1;
	QNFT::Export_input exportInput{};
	exportInput.owner = bob;
	exportInput.quantity = 1;
	exportInput.destination = QX_CONTRACT_INDEX;
	exportInput.offeredFee = 1000000;
	const auto bobBefore = getBalance(bob);
	EXPECT_EQ(action(11, approval, bob, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, burn, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(action(16, exportInput, bob, 1000000).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(getBalance(bob), bobBefore);
	EXPECT_EQ(balance(0, alice), 100);

	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 5, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, approval, bob).code, QNFT::Code::Ok);
	QNFT::IsApprovedForAll_output approved{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{bob, carol}, approved);
	EXPECT_TRUE(approved.approved);
	ASSERT_EQ(action(14, burn, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(16, exportInput, bob, 1000000).code, QNFT::Code::Ok);
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
	EXPECT_EQ(run(in).code, QNFT::Code::Ok);
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
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
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
	callFunction(QNFT_CONTRACT_INDEX, 21, QNFT::ListListings_input{QNFT_LISTING_SLOTS, 32}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, QNFT_LISTING_SLOTS);
	callFunction(QNFT_CONTRACT_INDEX, 21, QNFT::ListListings_input{QNFT_LISTING_SLOTS + 1, 32}, page);
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
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
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
	QNFT::IsApprovedForAll_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approval);
	EXPECT_TRUE(approval.approved);
}

TEST_F(NftTest, ApprovalSurvivesTransfersAndQxRoundTrip)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	OperationCommand in{};
	in.owner = alice;
	in.receiver = bob;
	in.quantity = 5;

	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	in.operation = QNFT::Operation::Transfer;
	in.receiver = carol;
	in.quantity = 101;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::InsufficientBalance);
	in.quantity = 2;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, carol), 2);
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	in.receiver = bob;
	in.quantity = 5;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
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

TEST_F(NftTest, MetadataUpdatesAndFreezeDoNotRestoreMintQuota)
{
	collection(100, 1);
	ASSERT_EQ(mint(100, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://final", 123), 0}).code, QNFT::Code::Ok);
	EXPECT_FALSE(item().frozen);
	ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://updated", 456), 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().metadataVersion, 3U);
	ASSERT_EQ(action(10, QNFT::FreezeMetadata_input{0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://rejected"), 0}).code, QNFT::Code::Frozen);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 4, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().burned, 4);
	EXPECT_EQ(balance(0, alice), 96);
	EXPECT_EQ(mint(1).code, QNFT::Code::Capacity);
	expectQuCollateral();
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
	for (auto operation : {QNFT::Operation::Transfer, QNFT::Operation::Burn, QNFT::Operation::Export, QNFT::Operation::NominateAdmin,
	                       QNFT::Operation::Mint, QNFT::Operation::UpdateMetadata})
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
	in.owner = alice;
	in.receiver = bob;
	in.quantity = 1;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	utcTime.Day = 18;
	updateQpiTime();
	in.operation = QNFT::Operation::Transfer;
	in.receiver = carol;
	EXPECT_EQ(run(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	// A nonexistent manager is rejected by core without moving the asset.
	OperationCommand ex{};
	ex.operation = QNFT::Operation::Export;
	ex.owner = alice;
	ex.quantity = 1;
	ex.destination = 999;
	EXPECT_EQ(run(ex).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(balance(0, alice), 99);
}

TEST(NftAbi, PublicWireLayoutsAndIdentifiers)
{
	EXPECT_EQ(sizeof(QNFT::ListForSale_input), 24);
	EXPECT_EQ(sizeof(QNFT::ListForSale_output), 16);
	EXPECT_EQ(sizeof(QNFT::Buy_input), 64);
	EXPECT_EQ(sizeof(QNFT::Buy_output), 40);
	EXPECT_EQ(sizeof(QNFT::DividendInfo_input), 16U);
	EXPECT_EQ(sizeof(QNFT::DividendInfo_output), 32U);
	EXPECT_EQ(offsetof(QNFT::DividendInfo_output, code), 24U);
	EXPECT_EQ(offsetof(QNFT::DividendInfo_output, dividendBps), 28U);
	EXPECT_EQ(offsetof(QNFT::Buy_output, shareholderAmount), 24U);
	EXPECT_EQ(offsetof(QNFT::Buy_output, code), 32U);
	EXPECT_EQ(offsetof(QNFT::Mint_input, dividendAssetName), 360U);
	EXPECT_EQ(offsetof(QNFT::Mint_input, dividendBps), 374U);
	EXPECT_EQ(offsetof(QNFT::Item, dividendAssetName), 448U);
	EXPECT_EQ(offsetof(QNFT::Item, dividendBps), 466U);
	EXPECT_EQ(sizeof(QNFT::GetListing_output), 72);
	EXPECT_EQ(sizeof(QNFT::GetMarket_output), 80);
	EXPECT_EQ(sizeof(QNFT::ListListings_output), 272);
	EXPECT_EQ(offsetof(QNFT::StateData, listings), offsetof(QNFT::StateData, contractRefunds) + sizeof(QNFT::StateData::contractRefunds));
	EXPECT_EQ(offsetof(QNFT::MarketEvent, _terminator), 108);
	EXPECT_EQ(sizeof(QNFT::Operation), 4);
	EXPECT_EQ(sizeof(QNFT::Code), 4);
	EXPECT_EQ(sizeof(QNFT::Policy), 1);
	EXPECT_EQ(static_cast<uint32>(QNFT::Operation::CreateCollection), 1U);
	EXPECT_EQ(static_cast<uint32>(QNFT::Operation::FundReserve), 10U);
	EXPECT_EQ(static_cast<uint64>(QNFT::FeeVariable::ListingFee), 10U);
	EXPECT_EQ(static_cast<uint64>(QNFT::FeeVariable::ProposalFee), 11U);
	EXPECT_EQ(QNFT_FEE_VARIABLE_COUNT, 14U);
	EXPECT_EQ(static_cast<uint32>(QNFT::MarketAction::Distribution), 4U);
	EXPECT_EQ(static_cast<uint32>(QNFT::EventKind::MintingStatusChanged), 7U);
	EXPECT_EQ(sizeof(QNFT::Collection), 416U);
	EXPECT_EQ(offsetof(QNFT::CreateCollection_input, externalTransfersDisabled), 300U);
	EXPECT_EQ(offsetof(QNFT::Event, type), 8);
	EXPECT_EQ(offsetof(QNFT::Event, actor), 16);
	EXPECT_EQ(sizeof(QNFT::Metadata), 288);
	EXPECT_EQ(sizeof(QNFT::OperationResult), 24);
	EXPECT_EQ(offsetof(QNFT::Event, _terminator), 96);
	EXPECT_EQ(sizeof(QNFT::Item), 472);
	EXPECT_EQ(sizeof(QNFT::GetItem_output), 480);
	EXPECT_EQ(sizeof(QNFT::GetCollection_output), 424);
	EXPECT_EQ(sizeof(QNFT::IsApprovedForAll_input), 64);
	EXPECT_EQ(sizeof(QNFT::ListRefunds_input), 8);
	EXPECT_EQ(sizeof(QNFT::ListRefunds_output), 656);
	EXPECT_EQ(offsetof(QNFT::StateData, contractRefunds), 88915152);
	EXPECT_EQ(sizeof(NOST::CreateAuction_input), 1008);
	QNFT::GetItem_output reply{};
	reply.item.creator = alice;
	reply.item.supply = 100;
	reply.item.metadata = metadata();
	std::ofstream("abi-item.bin", std::ios::binary).write(reinterpret_cast<const char*>(&reply), sizeof(reply));
	EXPECT_EQ(sizeof(QNFT::GetFeeSchedule_output), 88);
	EXPECT_EQ(offsetof(QNFT::GetFeeSchedule_output, shopFeeBPS), 72);
	EXPECT_EQ(offsetof(QNFT::GetFeeSchedule_output, rentalProtocolBPS), 76);
	EXPECT_EQ(offsetof(QNFT::GetFeeSchedule_output, rentalCreatorBPS), 80);
	std::cout << "QNFT state bytes: " << sizeof(QNFT::StateData) << '\n';
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

TEST_F(NftTest, OperatorApprovalWorksAtBootstrapAndCoversAllItems)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	QNFT::IsApprovedForAll_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approval);
	EXPECT_TRUE(approval.approved);
	// Even an owner without rentals cannot move units while the clock guard rejects the date.
	const auto funds = getBalance(bob);
	for (unsigned hour : {0U, 12U, 23U})
	{
		utcTime.Hour = hour;
		updateQpiTime();
		EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 2, 0}, bob, 17).code, QNFT::Code::ExternalFailure);
		EXPECT_EQ(balance(0, alice), 10);
		EXPECT_EQ(balance(0, carol), 0);
		EXPECT_EQ(getBalance(bob), funds);
	}

	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 2, 1}, bob).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 2, 1}, bob).code, QNFT::Code::ExternalFailure);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, carol).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(item().burned, 0);
	utcTime.Year = 2026;
	updateQpiTime();
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 2, 0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, carol).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 7);
	EXPECT_EQ(balance(0, carol), 2);
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

// Deferred delivery retries on ticks and releases only the recorded reservation once.
TEST_F(NftTest, TickMintDeliveryRetriesAndConsumesOnlyRecordedReservationOnce)
{
	qLogger::reset(system.tick);
	qLogger::registerNewTx(system.tick, 0);
	ASSERT_EQ(action(1, QNFT::CreateCollection_input{metadata(), 1000, 10, 1}, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(7).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 7, 0}, listed, alice, 0));
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
	ASSERT_NE(state->pendingMintDeliveries.addTail(0), NULL_INDEX);
	EXPECT_EQ(balance(0, nftId), 7);
	// Temporarily make the recorded escrow unavailable; the real QPI transfer must fail safely.
	record.reserved = 8;
	state->items.set(0, record);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->pendingMintDeliveries.population(), 1);
	EXPECT_EQ(item().reserved, 8);
	EXPECT_EQ(item().reservedRecipient, alice);
	EXPECT_EQ(balance(0, nftId), 7);
	EXPECT_EQ(getBalance(alice), before);
	record.reserved = 7;
	state->items.set(0, record);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->pendingMintDeliveries.population(), 0);
	EXPECT_EQ(balance(0, alice), 7);
	EXPECT_EQ(balance(0, nftId), 0);
	EXPECT_EQ(item().reserved, 0);
	EXPECT_EQ(item().reservedRecipient, NULL_ID);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
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

TEST_F(NftTest, TickMintDeliveryIsBoundedAndFailedRecipientDoesNotStarveOthers)
{
	constexpr uint32 count = QNFT_MINT_DELIVERIES_PER_TICK + 2;
	collection(count, count);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 i = 0; i < count; ++i)
	{
		ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
		QNFT::ListForSale_output listed{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 1, i}, listed, alice, 0));
		ASSERT_EQ(listed.code, QNFT::Code::Ok);
		state->listings.removeByKey(listed.listingId);
		auto record = state->items.get(i);
		record.reserved = i == 0 ? 2 : 1; // First entry cannot be delivered yet.
		record.reservedRecipient = bob;
		state->items.set(i, record);
		ASSERT_NE(state->pendingMintDeliveries.addTail(i), NULL_INDEX);
	}
	const auto fees = state->epochFees;
	// Delivery does not depend on minting remaining open or on the rental clock guard.
	state->mintingStatus.set(0, QNFT::MintingStatus::Closed);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->pendingMintDeliveries.population(), 3);
	EXPECT_EQ(balance(0, bob), 0);
	for (uint32 i = 1; i < QNFT_MINT_DELIVERIES_PER_TICK; ++i)
	{
		EXPECT_EQ(balance(i, bob), 1);
	}
	EXPECT_EQ(balance(QNFT_MINT_DELIVERIES_PER_TICK, bob), 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->pendingMintDeliveries.population(), 1);
	EXPECT_EQ(balance(count - 1, bob), 1);
	auto record = state->items.get(0);
	record.reserved = 1;
	state->items.set(0, record);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->pendingMintDeliveries.population(), 0);
	const auto journal = state->lastEventId;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->lastEventId, journal);
	EXPECT_EQ(state->epochFees, fees);
	for (uint32 i = 0; i < count; ++i)
	{
		EXPECT_EQ(balance(i, bob), 1);
		EXPECT_EQ(state->items.get(i).reserved, 0);
		EXPECT_EQ(state->items.get(i).reservedRecipient, NULL_ID);
	}
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
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://changed", 99), 0}, alice, 50017).code, QNFT::Code::Capacity);
	EXPECT_EQ(item().metadataVersion, 0xffffffffU);
	EXPECT_EQ(item().metadata.contentHash, record.metadata.contentHash);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{0}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(item().frozen, 1);
	expectQuCollateral();
}

// Regression intent: at the operator cap, reaffirmation remains possible and revocation frees admission.
TEST_F(NftTest, OperatorApprovalCapacityAllowsReaffirmationAndReusesRevokedSlot)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::SetApprovalForAll_input request{bob, true};
	ASSERT_EQ(action(11, request).code, QNFT::Code::Ok);
	QPI::bit filler = true;
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->operatorApprovals.set(id(n, 41, 42, 43), filler);
	}
	request.operatorId = carol;
	const auto before = getBalance(alice);
	EXPECT_EQ(action(11, request, alice, 17).code, QNFT::Code::Capacity);
	EXPECT_EQ(getBalance(alice), before);
	request.operatorId = bob;
	ASSERT_EQ(action(11, request).code, QNFT::Code::Ok);
	QNFT::IsApprovedForAll_output approved{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approved);
	EXPECT_TRUE(approved.approved);
	EXPECT_EQ(state->operatorApprovals.population(), QNFT_MAX_APPROVALS);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	EXPECT_EQ(state->operatorApprovals.population(), QNFT_MAX_APPROVALS - 1);
	EXPECT_EQ(action(11, request).code, QNFT::Code::Ok);
	expectQuCollateral();
}

// Revocation always addresses the caller and is independent of network time.
TEST_F(NftTest, OperatorRevocationOnlyTouchesInvocatorRegardlessOfTime)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 5, 0}).code, QNFT::Code::Ok);
	QNFT::SetApprovalForAll_input approval{carol, true};
	ASSERT_EQ(action(11, approval, alice).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, approval, bob).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::SetApprovalForAll_input request{carol, false};
	ASSERT_EQ(action(11, request, alice, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(state->operatorApprovals.population(), 1);
	QNFT::IsApprovedForAll_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{bob, carol}, remaining);
	EXPECT_TRUE(remaining.approved);
	// Repeated revocation refunds the reward and preserves other owners' entries.
	const auto before = getBalance(alice), fees = state->epochFees;
	ASSERT_EQ(action(11, request, alice, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(state->operatorApprovals.population(), 1);
	utcTime.Year = 2027;
	updateQpiTime();
	ASSERT_EQ(action(11, request, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(state->operatorApprovals.population(), 1);
	ASSERT_EQ(action(11, request, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(state->operatorApprovals.population(), 0);
	utcTime.Year = 2026;
	updateQpiTime();
	ASSERT_EQ(action(11, approval, alice).code, QNFT::Code::Ok);
	utcTime.Year = 2022;
	utcTime.Month = 4;
	utcTime.Day = 13;
	updateQpiTime();
	ASSERT_EQ(action(11, request, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(state->operatorApprovals.population(), 0);
	expectQuCollateral();
}

// Epoch maintenance compacts revoked entries without removing live permissions.
TEST_F(NftTest, EpochCompactsRevokedApprovalsWithoutRemovingLivePermissions)
{
	collection();
	ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->operatorApprovals.set(id(n, 41, 42, 43), true);
	}
	for (uint32 n = 1; n < QNFT_MAX_APPROVALS; ++n)
	{
		state->operatorApprovals.removeByKey(id(n, 41, 42, 43));
	}
	ASSERT_TRUE(state->operatorApprovals.needsCleanup(QNFT_MAP_CLEANUP_PERCENT));
	utcTime.Year = 2030;
	updateQpiTime();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->operatorApprovals.population(), 1);
	EXPECT_FALSE(state->operatorApprovals.needsCleanup(0));
	QNFT::IsApprovedForAll_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approval);
	EXPECT_TRUE(approval.approved);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Ok);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(state->operatorApprovals.population(), 1);
	expectQuCollateral();
}

TEST_F(NftTest, ApprovalCanBeGrantedAtZeroBalanceAndSurvivesListing)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listing{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{7, 2, 0}, listing, alice, 0));
	ASSERT_EQ(listing.code, QNFT::Code::Ok);
	QNFT::IsApprovedForAll_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, bob}, approval);
	EXPECT_TRUE(approval.approved);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::InsufficientBalance);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listing.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 2, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{bob, 1, 0}, carol).code, QNFT::Code::Ok);
	expectQuCollateral();
}

// Epoch cleanup must preserve the active listings and their escrowed units.
TEST_F(NftTest, EpochCompactsShopListingsWithoutLosingEscrow)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{7, 2, 0}, listed, alice, 0));
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
	callFunction(QNFT_CONTRACT_INDEX, 20, QNFT::GetListing_input{listed.listingId}, live);
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
	callFunction(QNFT_CONTRACT_INDEX, 21, QNFT::ListListings_input{8192, 1}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.listingIds.get(0), chosen);
	EXPECT_EQ(page.nextOffset, slot + 1);
	callFunction(QNFT_CONTRACT_INDEX, 21, QNFT::ListListings_input{page.nextOffset, 32}, page);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::CancelSale_output cancelled{};
	QNFT::GetListing_output original{};
	callFunction(QNFT_CONTRACT_INDEX, 20, QNFT::GetListing_input{listed.listingId}, original);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	// Seed a listing whose native escrow is unavailable to exercise the real QPI failure branch.
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->listings.set(listed.listingId, original.listing);
	const auto before = getBalance(bob), fees = state->epochFees;
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 100, 1}, bought, bob, 117));
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

namespace
{
	struct RegistryPageQueryWire
	{
		uint32 offset;
		uint32 limit;
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

	static_assert(sizeof(CollectionPageWire) == 6800);
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

// Failed head entries rotate behind other recipients and every tick has bounded work.

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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto fees = state->epochFees, buyerFunds = getBalance(bob), sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol);
	// The current shop rate still applies to a previously created listing.
	state->shopFeeBPS = 300;
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 2, 100, 1}, bought, bob, 217));
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

TEST_F(NftTest, BuyPaysSellerAndRoyaltyImmediately)
{
	collection();
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 3, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice), royaltyBefore = getBalance(carol), buyerBefore = getBalance(bob);
	const auto contractBefore = getBalance(nftId);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1}, bought, bob, 30123));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), sellerBefore + 28410);
	EXPECT_EQ(getBalance(carol), royaltyBefore + 1500);
	EXPECT_EQ(getBalance(bob), buyerBefore - 30000);
	EXPECT_EQ(getBalance(nftId), contractBefore + 90);
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
	in.royaltyBps = 9970;
	in.royaltyReceiver = alice;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 1, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	const auto sellerBefore = getBalance(alice);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 10000, 1}, bought, bob, 10000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.fee, 30);
	EXPECT_EQ(bought.royalty, 9970);
	EXPECT_EQ(getBalance(alice), sellerBefore + 9970);
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
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 10, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	observedSaleListingId = listed.listingId;
	const auto beneficiaryBefore = getBalance(recipient);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 3, 10000, 1}, bought, bob, 30000));
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
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{}, recipient, 123).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(recipient), before);
	QNFT::ListRefunds_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, pending);
	EXPECT_EQ(pending.count, 0);
}

TEST_F(NftTest, FailedRefundRetriesOnTickWithoutManualWithdrawal)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	increaseEnergy(recipient, 123);
	const auto before = getBalance(recipient);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	const auto rejected = action(12, QNFT::TransferFrom_input{}, recipient, 123);
	contractCallbacksRunning = NoContractCallback;
	EXPECT_EQ(rejected.code, QNFT::Code::InvalidInput);
	QNFT::ListRefunds_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, pending);
	ASSERT_EQ(pending.count, 1U);
	EXPECT_EQ(pending.refunds.get(0).recipient, recipient);
	EXPECT_EQ(pending.refunds.get(0).amount, 123);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), before);
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, pending);
	EXPECT_EQ(pending.count, 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), before);
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
	EXPECT_EQ(sizeof(QNFT::ItemEntry), 480U);
	EXPECT_EQ(offsetof(QNFT::ItemEntry, itemId), 472U);
	EXPECT_EQ(sizeof(page), 7696U);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::ListItems_input{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.totalCount, 0U);
	collection(1000, 30);
	for (unsigned i = 0; i < 17; ++i)
	{
		ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	}
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 3, 0}).code, QNFT::Code::Ok);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::ListItems_input{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 16U);
	EXPECT_EQ(page.totalCount, 17U);
	EXPECT_EQ(page.nextOffset, 16U);
	for (uint32 i = 0; i < page.count; ++i)
	{
		QNFT::GetItem_output single{};
		callFunction(QNFT_CONTRACT_INDEX, 4, QNFT::GetItem_input{i}, single);
		EXPECT_EQ(page.items.get(i).itemId, i);
		EXPECT_EQ(memcmp(&page.items.get(i).item, &single.item, sizeof(QNFT::Item)), 0);
		EXPECT_EQ(page.items.get(i).item.metadata.contentHash, metadata().contentHash);
	}
	EXPECT_EQ(page.items.get(0).item.burned, 3);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::ListItems_input{16, 16}, page);
	ASSERT_EQ(page.count, 1U);
	EXPECT_EQ(page.items.get(0).itemId, 16U);
	EXPECT_EQ(page.nextOffset, 17U);
	EXPECT_EQ(page.items.get(1).item.supply, 0);
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::ListItems_input{17, 1}, page);
	EXPECT_EQ(page.code, QNFT::Code::Ok);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.nextOffset, 17U);
	for (const auto input : {QNFT::ListItems_input{18, 1}, QNFT::ListItems_input{0, 0}, QNFT::ListItems_input{0, 17}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 5, input, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->itemCount = QNFT_MAX_ITEMS;
	callFunction(QNFT_CONTRACT_INDEX, 5, QNFT::ListItems_input{QNFT_MAX_ITEMS - 1, 16}, page);
	EXPECT_EQ(page.count, 1U);
	EXPECT_EQ(page.items.get(0).itemId, QNFT_MAX_ITEMS - 1);
	EXPECT_EQ(page.nextOffset, QNFT_MAX_ITEMS);
}

TEST_F(NftTest, ListRefundsListsAllPositiveCreditsWithBoundedPages)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::ListRefunds_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, page);
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
		callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{cursor, 16}, page);
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
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{cursor, 16}, page);
	EXPECT_EQ(page.count, 0U);
	EXPECT_EQ(page.totalCount, 17U);
	for (const auto query :
	     {QNFT::ListRefunds_input{0, 0}, QNFT::ListRefunds_input{0, 17}, QNFT::ListRefunds_input{QNFT_CONTRACT_REFUND_SLOTS + 1, 1}})
	{
		callFunction(QNFT_CONTRACT_INDEX, 28, query, page);
		EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(page.count, 0U);
	}
	state->contractRefunds.removeByKey(cleared);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	callFunction(QNFT_CONTRACT_INDEX, 28, QNFT::ListRefunds_input{0, 16}, page);
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
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{alice}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 123);
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{bob}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 456);
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{carol}, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.quantity, 0);
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{NULL_ID}, out);
	EXPECT_EQ(out.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(state->contractRefunds.population(), 2U);
}

// Revoking one operator must not affect another operator for the same owner.
TEST_F(NftTest, RevokingOneOperatorPreservesOtherOperatorPermissions)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(balance(0, alice), 10);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(item().burned, 0);
	// The other operator retains all delegated capabilities.
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, carol).code, QNFT::Code::Ok);
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
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, false}).code, QNFT::Code::Ok);
	const auto before = getBalance(bob);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, bob, 17).code, QNFT::Code::Unauthorized);
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
	const auto first = action(12, QNFT::TransferFrom_input{}, recipient, 123);
	const auto second = action(12, QNFT::TransferFrom_input{}, recipient, 456);
	contractCallbacksRunning = NoContractCallback;
	ASSERT_EQ(first.code, QNFT::Code::InvalidInput);
	ASSERT_EQ(second.code, QNFT::Code::InvalidInput);
	QNFT::GetRefund_output pending{};
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{recipient}, pending);
	EXPECT_EQ(pending.quantity, 579);
	EXPECT_EQ(getBalance(recipient), 421);
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), 1000);
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{recipient}, pending);
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
	const auto rejected = action(12, QNFT::TransferFrom_input{}, recipient, 579);
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
	callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{recipient}, pending);
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

class Nft1155Test : public NftTest
{
public:
	QNFT::Mint_input edition(sint64 quantity, const id& receiver = alice)
	{
		QNFT::Mint_input in{};
		in.receiver = receiver;
		in.metadata = metadata();
		in.quantity = quantity;
		in.royaltyReceiver = carol;
		in.royaltyBps = 500;
		return in;
	}

	QNFT::Batch_output mintBatch(const QNFT::MintBatch_input& in, sint64 reward = 2 * TEST_MINT_FEE)
	{
		QNFT::Batch_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 7, in, out, alice, reward));
		return out;
	}

	QNFT::Batch_output transferBatch(const QNFT::BatchTransfer_input& in, const id& caller = alice, sint64 reward = 0)
	{
		QNFT::Batch_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 13, in, out, caller, reward));
		return out;
	}

	QNFT::RoyaltyInfo_output royalty(uint32 token, sint64 price)
	{
		QNFT::RoyaltyInfo_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 11, QNFT::RoyaltyInfo_input{price, token}, out);
		return out;
	}

	sint64 owned(uint32 token, const id& owner)
	{
		QNFT::BalanceOf_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 13, QNFT::BalanceOf_input{owner, token}, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.balance;
	}
};

TEST_F(Nft1155Test, MintBatchCreatesTypesAndChargesPerTypeNotPerUnit)
{
	collection();
	QNFT::MintBatch_input in{};
	in.count = 2;
	in.entries.set(0, edition(100));
	in.entries.set(1, edition(1, bob));
	const auto before = getBalance(alice);
	const auto out = mintBatch(in, 2 * TEST_MINT_FEE + 123);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.completed, 2U);
	EXPECT_EQ(out.attempted, 2U);
	EXPECT_EQ(out.fee, 2 * TEST_MINT_FEE);
	EXPECT_EQ(out.results.get(0).itemId, 0U);
	EXPECT_EQ(out.results.get(1).itemId, 1U);
	EXPECT_EQ(owned(0, alice), 100);
	EXPECT_EQ(owned(1, bob), 1);
	EXPECT_EQ(item(0).supply, 100);
	EXPECT_EQ(item(1).supply, 1);
	EXPECT_EQ(getBalance(alice), before - 2 * TEST_MINT_FEE);
	expectQuCollateral();
}

TEST_F(Nft1155Test, MintBatchPreflightsWholeCollectionQuotaAndFee)
{
	collection(100, 10);
	QNFT::MintBatch_input in{};
	in.count = 2;
	in.entries.set(0, edition(60));
	in.entries.set(1, edition(41));
	const auto before = getBalance(alice);
	EXPECT_EQ(mintBatch(in).code, QNFT::Code::Capacity);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->itemCount, 0U);
	EXPECT_EQ(getBalance(alice), before);
	in.entries.set(1, edition(40));
	EXPECT_EQ(mintBatch(in, TEST_MINT_FEE).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(state->itemCount, 0U);
	EXPECT_EQ(mintBatch(in).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(Nft1155Test, MintBatchReportsIrreversiblePrefixAndRefundsUnissuedFee)
{
	collection();
	const char name[8] = "N000001", unit[7]{};
	int issuance, owner, possession;
	ASSERT_EQ(issueAsset(nftId, name, 0, unit, 1, QNFT_CONTRACT_INDEX, &issuance, &owner, &possession), 1);
	QNFT::MintBatch_input in{};
	in.count = 2;
	in.entries.set(0, edition(7));
	in.entries.set(1, edition(9));
	const auto before = getBalance(alice);
	const auto out = mintBatch(in);
	EXPECT_EQ(out.code, QNFT::Code::Pending);
	EXPECT_EQ(out.completed, 1U);
	EXPECT_EQ(out.attempted, 2U);
	EXPECT_EQ(out.failureIndex, 1U);
	EXPECT_EQ(out.failureCode, QNFT::Code::ExternalFailure);
	EXPECT_EQ(out.fee, TEST_MINT_FEE);
	EXPECT_EQ(getBalance(alice), before - TEST_MINT_FEE);
	EXPECT_EQ(owned(0, alice), 7);
	EXPECT_EQ(out.results.get(0).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(Nft1155Test, BatchTransferChecksDuplicatesBeforeChangingAnyBalance)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input in{};
	in.owner = alice;
	in.receiver = bob;
	in.count = 2;
	in.entries.set(0, QNFT::TokenAmount{60, 0});
	in.entries.set(1, QNFT::TokenAmount{41, 0});
	EXPECT_EQ(transferBatch(in).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(owned(0, alice), 100);
	EXPECT_EQ(owned(0, bob), 0);
	in.entries.set(1, QNFT::TokenAmount{40, 0});
	const auto out = transferBatch(in);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.completed, 2U);
	EXPECT_EQ(owned(0, bob), 100);
	expectQuCollateral();
}

TEST_F(Nft1155Test, OperatorWidePermissionHandlesSeveralTypesAndRevocation)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(50).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input in{};
	in.owner = alice;
	in.receiver = carol;
	in.count = 2;
	in.entries.set(0, QNFT::TokenAmount{30, 0});
	in.entries.set(1, QNFT::TokenAmount{10, 1});
	EXPECT_EQ(transferBatch(in, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, 1}).code, QNFT::Code::Ok);
	EXPECT_EQ(transferBatch(in, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, alice), 70);
	EXPECT_EQ(owned(1, alice), 40);
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(transferBatch(in, bob).code, QNFT::Code::Unauthorized);
	QNFT::BalanceOfBatch_input query{};
	query.count = 3;
	query.entries.set(0, {alice, 0});
	query.entries.set(1, {carol, 1});
	query.entries.set(2, {bob, 999});
	QNFT::BalanceOfBatch_output out{};
	callFunction(QNFT_CONTRACT_INDEX, 14, query, out);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.count, 3U);
	EXPECT_EQ(out.balances.get(0), 70);
	EXPECT_EQ(out.balances.get(1), 10);
	EXPECT_EQ(out.balances.get(2), 0);
	query.entries.set(2, {NULL_ID, 0});
	out = {};
	callFunction(QNFT_CONTRACT_INDEX, 14, query, out);
	EXPECT_EQ(out.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(out.count, 0U);
	EXPECT_EQ(out.balances.get(0), 0);
}

TEST_F(Nft1155Test, SelfAndZeroTransfersKeepSupplyAndChargeOnlyCompletedEntries)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->transferFee = 7;
	QNFT::BatchTransfer_input in{};
	in.owner = alice;
	in.receiver = alice;
	in.count = 2;
	in.entries.set(0, QNFT::TokenAmount{10, 0});
	in.entries.set(1, QNFT::TokenAmount{10, 0});
	const auto before = getBalance(alice);
	EXPECT_EQ(transferBatch(in, alice, 14).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, alice), 10);
	EXPECT_EQ(getBalance(alice), before - 14);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 0, 0}, alice, 7).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, bob), 0);
	expectQuCollateral();
}

TEST_F(Nft1155Test, GlobalBalanceSupplyAndRoyaltiesFollowPartialExportExternalTransferAndBurn)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(16, QNFT::Export_input{alice, 30, 0, 0, TESTEXA_CONTRACT_INDEX}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 70);
	EXPECT_EQ(owned(0, alice), 100);
	const Asset asset{nftId, item().assetName};
	TESTEXA::TransferShareOwnershipAndPossession_output moved{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 2, TESTEXA::TransferShareOwnershipAndPossession_input{asset, 10, bob}, moved, alice, 0));
	EXPECT_EQ(owned(0, alice), 90);
	EXPECT_EQ(owned(0, bob), 10);
	EXPECT_EQ(royalty(0, 10000).royaltyAmount, 500);
	EXPECT_EQ(royalty(0, 10000).receiver, carol);
	ASSERT_TRUE(
	    invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 2, TESTEXA::TransferShareOwnershipAndPossession_input{asset, 10, NULL_ID}, moved, bob, 0));
	QNFT::TotalSupply_output out{};
	callFunction(QNFT_CONTRACT_INDEX, 7, QNFT::TotalSupply_input{0}, out);
	EXPECT_EQ(out.supply, 90);
	EXPECT_EQ(owned(0, bob), 0);
	EXPECT_EQ(item().burned, 10);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 20, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, alice), 70);
	EXPECT_EQ(owned(0, bob), 20);
}

TEST_F(Nft1155Test, RoyaltyQuoteHasCurrencyIndependentFloorAndNoSideEffects)
{
	collection();
	ASSERT_EQ(action(6, edition(100), alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	const auto before = getBalance(carol);
	for (sint64 price : {0LL, 1LL, 19LL, 20LL, 999LL, 10000LL, 9223372036854775807LL})
	{
		const auto out = royalty(0, price);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		EXPECT_EQ(out.royaltyAmount, price / 20);
		EXPECT_EQ(out.receiver, carol);
	}
	EXPECT_EQ(royalty(0, -1).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(royalty(999, 1).code, QNFT::Code::NotFound);
	EXPECT_EQ(getBalance(carol), before);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->shopFeeBPS = 9000;
	EXPECT_EQ(royalty(0, 10000).royaltyAmount, 500);
}

TEST_F(Nft1155Test, EmptyAndOversizedBatchesHaveExplicitResults)
{
	QNFT::MintBatch_input mintIn{};
	const auto before = getBalance(alice);
	EXPECT_EQ(mintBatch(mintIn, 123).code, QNFT::Code::Ok);
	mintIn.count = QNFT_MINT_BATCH_CAPACITY + 1;
	EXPECT_EQ(mintBatch(mintIn, 123).code, QNFT::Code::InvalidInput);
	QNFT::BatchTransfer_input in{};
	in.owner = alice;
	in.receiver = bob;
	EXPECT_EQ(transferBatch(in, alice, 123).code, QNFT::Code::Ok);
	in.count = QNFT_TOKEN_BATCH_CAPACITY + 1;
	EXPECT_EQ(transferBatch(in, alice, 123).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(alice), before);
	expectQuCollateral();
}

TEST_F(Nft1155Test, HolderPagesExposePartialManagementAndRequireStableTick)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(16, QNFT::Export_input{alice, 30, 0, 0, TESTEXA_CONTRACT_INDEX}).code, QNFT::Code::Ok);
	QNFT::GetAssetState_output summary{};
	callFunction(QNFT_CONTRACT_INDEX, 8, QNFT::GetAssetState_input{0}, summary);
	EXPECT_EQ(summary.code, QNFT::Code::Ok);
	EXPECT_EQ(summary.supply, 100);
	EXPECT_EQ(summary.managedSupply, 70);
	sint64 local = 0, external = 0;
	uint32 cursor = 0;
	bool done = false;
	for (unsigned page = 0; page < 16 && !done; ++page)
	{
		QNFT::ListHolders_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListHolders_input{0, cursor, 1, 0}, out);
		ASSERT_EQ(out.code, QNFT::Code::Ok);
		for (uint32 i = 0; i < out.count; ++i)
		{
			EXPECT_EQ(out.entries.get(i).owner, alice);
			if (out.entries.get(i).ownershipManager == QNFT_CONTRACT_INDEX)
			{
				local += out.entries.get(i).quantity;
			}
			else
			{
				external += out.entries.get(i).quantity;
			}
		}
		cursor = out.nextCursor;
		done = out.done != 0;
	}
	EXPECT_TRUE(done);
	EXPECT_EQ(local, 70);
	EXPECT_EQ(external, 30);
	QNFT::ListHolders_output stale{};
	callFunction(QNFT_CONTRACT_INDEX, 16, QNFT::ListHolders_input{0, 0, 16, system.tick + 1}, stale);
	EXPECT_EQ(stale.code, QNFT::Code::Conflict);
	EXPECT_EQ(stale.count, 0U);
}

TEST_F(Nft1155Test, ProtocolAndRoyaltyBoundariesHaveExplicitNativeSemantics)
{
	QNFT::GetProtocolInfo_output protocol{};
	callFunction(QNFT_CONTRACT_INDEX, 30, QNFT::GetProtocolInfo_input{}, protocol);
	EXPECT_EQ(protocol.majorVersion, 1U);
	EXPECT_EQ(protocol.minorVersion, 0U);
	collection();
	for (uint16 rate : {uint16(0), uint16(10000)})
	{
		auto in = edition(1);
		in.royaltyBps = rate;
		const auto minted = action(6, in, alice, TEST_MINT_FEE);
		ASSERT_EQ(minted.code, QNFT::Code::Ok);
		const auto quote = royalty(minted.itemId, 9223372036854775807LL);
		EXPECT_EQ(quote.code, QNFT::Code::Ok);
		EXPECT_EQ(quote.royaltyAmount, rate ? 9223372036854775807LL : 0LL);
	}
	auto in = edition(1);
	in.royaltyReceiver = NULL_ID;
	in.royaltyBps = 10000;
	const auto minted = action(6, in, alice, TEST_MINT_FEE);
	ASSERT_EQ(minted.code, QNFT::Code::Ok);
	EXPECT_EQ(royalty(minted.itemId, 10000).royaltyAmount, 0);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, minted.itemId}).code, QNFT::Code::Ok);
	EXPECT_EQ(royalty(minted.itemId, 10000).code, QNFT::Code::NotFound);
	QNFT::URI_output uri{};
	callFunction(QNFT_CONTRACT_INDEX, 9, QNFT::URI_input{minted.itemId}, uri);
	EXPECT_EQ(uri.code, QNFT::Code::Ok);
}

TEST_F(Nft1155Test, ContractReceivesEditionAndReturnsPartThroughRealQpiCall)
{
	INIT_CONTRACT(TESTEXA);
	contractUserProcedures[TESTEXA_CONTRACT_INDEX][60000] = forwardNftFromReceiver;
	contractUserProcedureInputSizes[TESTEXA_CONTRACT_INDEX][60000] = sizeof(QNFT::TransferFrom_input);
	contractUserProcedureOutputSizes[TESTEXA_CONTRACT_INDEX][60000] = sizeof(QNFT::TransferFrom_output);
	contractUserProcedureLocalsSizes[TESTEXA_CONTRACT_INDEX][60000] = 0;
	const id receiver(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, receiver, 30, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, receiver), 30);
	EXPECT_EQ(owned(0, alice), 70);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{receiver, bob, 10, 0}, bob).code, QNFT::Code::Unauthorized);
	QNFT::TransferFrom_output out{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 60000, QNFT::TransferFrom_input{receiver, carol, 10, 0}, out, alice, 0));
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, receiver), 20);
	EXPECT_EQ(owned(0, carol), 10);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, nftId, 1, 0}).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, id(QX_CONTRACT_INDEX, 0, 0, 0), 1, 0}).code, QNFT::Code::InvalidInput);
}

class Nft1155ExtrasTest : public Nft1155Test
{
public:
	QNFT::Batch_output multi(const QNFT::MultiMint_input& in, sint64 reward, const id& caller = alice)
	{
		QNFT::Batch_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 8, in, out, caller, reward));
		return out;
	}

	QNFT::GetMintingStatus_output mintStatus(uint32 collectionId = 0)
	{
		QNFT::GetMintingStatus_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 3, QNFT::GetMintingStatus_input{collectionId}, out);
		return out;
	}

	QNFT::Code setStatus(QNFT::MintingStatus status, const id& caller = alice)
	{
		return action(4, QNFT::SetMintingStatus_input{0, status}, caller, 123).code;
	}

	QNFT::Batch_output burnBatch(const QNFT::BurnBatch_input& in, const id& caller = alice)
	{
		QNFT::Batch_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 15, in, out, caller, 123));
		return out;
	}

	bool exists(uint32 token)
	{
		QNFT::Exists_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 6, QNFT::Exists_input{token}, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out.exists != 0;
	}

	std::string resolved(uint32 token)
	{
		QNFT::ResolvedURI_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::ResolvedURI_input{token}, out);
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		std::string value;
		for (uint32 i = 0; i < 256 && out.uri.get(i); ++i)
		{
			value += char(out.uri.get(i));
		}
		return value;
	}
};

TEST_F(Nft1155ExtrasTest, CollectionManagersShareIssuanceAndAuthorRemainsCollectionCreator)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->collections.get(0).administrator, NULL_ID);
	QNFT::AcceptAdmin_output rejected{};
	EXPECT_FALSE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 3, QNFT::AcceptAdmin_input{0}, rejected, NULL_ID, 0));
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(6, edition(1), bob, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_FALSE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 3, QNFT::AcceptAdmin_input{0}, rejected, NULL_ID, 0));
	for (const auto& manager : {alice, bob})
	{
		ASSERT_EQ(action(6, edition(1), manager, TEST_MINT_FEE).code, QNFT::Code::Ok);
		QNFT::MintBatch_input batch{};
		batch.count = 1;
		batch.entries.set(0, edition(1));
		QNFT::Batch_output result{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 7, batch, result, manager, TEST_MINT_FEE));
		EXPECT_EQ(result.code, QNFT::Code::Ok);
		QNFT::MultiMint_input multiInput{};
		multiInput.count = 1;
		multiInput.prototype = edition(0);
		multiInput.quantities.set(0, 1);
		EXPECT_EQ(multi(multiInput, TEST_MINT_FEE, manager).code, QNFT::Code::Ok);
		EXPECT_EQ(setStatus(QNFT::MintingStatus::Paused, manager), QNFT::Code::Ok);
		EXPECT_EQ(action(6, edition(1), manager, TEST_MINT_FEE).code, QNFT::Code::Conflict);
		EXPECT_EQ(setStatus(QNFT::MintingStatus::Open, manager), QNFT::Code::Ok);
	}
	ASSERT_EQ(state->itemCount, 7U);
	for (uint32 i = 0; i < 7; ++i)
	{
		EXPECT_EQ(item(i).creator, alice);
		EXPECT_EQ(item(i).royaltyReceiver, carol);
	}
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}, alice).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
	const auto funds = getBalance(bob);
	EXPECT_EQ(action(6, edition(1), bob, TEST_MINT_FEE + 123).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(bob), funds);
	EXPECT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}, carol).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, CollectionManagersEditAndFreezeMetadataButCannotSpendHolderUnits)
{
	collection();
	auto in = edition(10, carol);
	in.policy = QNFT::Policy::Mutable;
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(6, in, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);
	for (const auto& manager : {alice, bob})
	{
		EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://managed", 9), 0}, manager).code, QNFT::Code::Ok);
		EXPECT_EQ(action(12, QNFT::TransferFrom_input{carol, manager, 1, 0}, manager).code, QNFT::Code::Unauthorized);
		EXPECT_EQ(action(14, QNFT::Burn_input{carol, 1, 0}, manager).code, QNFT::Code::Unauthorized);
		EXPECT_EQ(action(16, QNFT::Export_input{carol, 1, 0, 0, QX_CONTRACT_INDEX}, manager).code, QNFT::Code::Unauthorized);
	}
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 0}, alice).code, QNFT::Code::Frozen);
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{1}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 1}, bob).code, QNFT::Code::Frozen);
	EXPECT_EQ(owned(0, carol), 10);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, FormerAdministratorLosesAllMetadataAuthority)
{
	collection();
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	auto in = edition(1);
	in.policy = QNFT::Policy::Mutable;
	ASSERT_EQ(action(6, in, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);

	ASSERT_EQ(action(6, in, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}, alice).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 0}, bob, 123).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 1}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(10, QNFT::FreezeMetadata_input{1}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 0}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata(), 1}, carol).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, MultiMintIssuesSixteenTypesWithSharedSettingsAndResolvedUri)
{
	collection(1000, 32);
	QNFT::MultiMint_input in{};
	in.count = 16;
	in.prototype = edition(0);
	in.prototype.metadata = metadata("ipfs://collection/{id}.json");
	for (uint32 i = 0; i < in.count; ++i)
	{
		in.quantities.set(i, i + 1);
	}
	const auto before = getBalance(alice);
	const auto out = multi(in, 16 * TEST_MINT_FEE + 123);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.completed, 16U);
	EXPECT_EQ(out.fee, 16 * TEST_MINT_FEE);
	for (uint32 i = 0; i < in.count; ++i)
	{
		EXPECT_EQ(out.results.get(i).itemId, i);
		EXPECT_EQ(owned(i, alice), i + 1);
		EXPECT_TRUE(exists(i));
	}
	EXPECT_EQ(resolved(15), "ipfs://collection/" + std::string(63, '0') + "f.json");
	EXPECT_EQ(getBalance(alice), before - 16 * TEST_MINT_FEE);
	const auto status = mintStatus();
	EXPECT_EQ(status.remainingItems, 16U);
	EXPECT_EQ(status.remainingUnits, 864);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, MultiMintPreflightsQuotaAuthorityFeeAndReservedQuantity)
{
	collection(10, 5);
	QNFT::MultiMint_input in{};
	in.count = 3;
	in.prototype = edition(0);
	in.quantities.set(0, 4);
	in.quantities.set(1, 4);
	in.quantities.set(2, 4);
	EXPECT_EQ(multi(in, 3 * TEST_MINT_FEE).code, QNFT::Code::Capacity);
	EXPECT_FALSE(exists(0));
	in.quantities.set(2, 2);
	EXPECT_EQ(multi(in, 3 * TEST_MINT_FEE, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(multi(in, 2 * TEST_MINT_FEE).code, QNFT::Code::InsufficientFee);
	in.prototype.quantity = 1;
	EXPECT_EQ(multi(in, 3 * TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	in.prototype.quantity = 0;
	EXPECT_EQ(multi(in, 3 * TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(mintStatus().remainingUnits, 0);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, PausedAndClosedMintGateAllIssuancePathsWithoutBlockingTransfers)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	const auto before = getBalance(alice);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Paused, bob), QNFT::Code::Unauthorized);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Paused), QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Conflict);
	QNFT::MintBatch_input batch{};
	batch.count = 1;
	batch.entries.set(0, edition(1));
	EXPECT_EQ(mintBatch(batch, TEST_MINT_FEE).code, QNFT::Code::Conflict);
	QNFT::MultiMint_input compact{};
	compact.count = 1;
	compact.prototype = edition(0);
	compact.quantities.set(0, 1);
	EXPECT_EQ(multi(compact, TEST_MINT_FEE).code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 10, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Open), QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Closed), QNFT::Code::Ok);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Open), QNFT::Code::Conflict);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Paused), QNFT::Code::Conflict);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Closed), QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Conflict);
	EXPECT_EQ(mintStatus().status, QNFT::MintingStatus::Closed);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, NewAdministratorCannotReopenPermanentlyClosedCollection)
{
	collection();
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Closed), QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Open, bob), QNFT::Code::Conflict);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Paused, alice), QNFT::Code::Conflict);
}

TEST_F(Nft1155ExtrasTest, MetadataUpdateAndFreezeAfterMultiMintTransferExportAndClose)
{
	collection();
	const auto finalMetadata = metadata("ipfs://final/{id}.json", 777);
	QNFT::MultiMint_input in{};
	in.count = 2;
	in.prototype = edition(0);
	in.prototype.policy = QNFT::Policy::Mutable;
	in.prototype.metadata = metadata("ipfs://placeholder", 111);
	in.quantities.set(0, 100);
	in.quantities.set(1, 50);
	ASSERT_EQ(multi(in, 2 * TEST_MINT_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 10, 0}).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->preAcquireSharesOutput.allowTransfer = true;
	ASSERT_EQ(action(16, QNFT::Export_input{alice, 20, 0, 1, TESTEXA_CONTRACT_INDEX}).code, QNFT::Code::Ok);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Closed), QNFT::Code::Ok);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{finalMetadata, 0}, bob).code, QNFT::Code::Unauthorized);
	for (uint32 i = 0; i < 2; ++i)
	{
		ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{finalMetadata, i}).code, QNFT::Code::Ok);
		EXPECT_FALSE(item(i).frozen);
		EXPECT_EQ(item(i).metadataVersion, 2U);
		ASSERT_EQ(action(10, QNFT::FreezeMetadata_input{i}).code, QNFT::Code::Ok);
		EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{finalMetadata, i}).code, QNFT::Code::Frozen);
	}
	EXPECT_EQ(resolved(1), "ipfs://final/" + std::string(63, '0') + "1.json");
	EXPECT_EQ(owned(0, bob), 10);
	EXPECT_EQ(owned(1, alice), 50);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, BurnBatchPreflightsDuplicatesAndRetainsIssuanceQuotas)
{
	collection(150, 2);
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(50).code, QNFT::Code::Ok);
	QNFT::BurnBatch_input in{};
	in.owner = alice;
	in.count = 2;
	in.entries.set(0, QNFT::TokenAmount{60, 0});
	in.entries.set(1, QNFT::TokenAmount{41, 0});
	EXPECT_EQ(burnBatch(in).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(owned(0, alice), 100);
	in.entries.set(1, QNFT::TokenAmount{40, 0});
	EXPECT_EQ(burnBatch(in, bob).code, QNFT::Code::Unauthorized);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, 1}).code, QNFT::Code::Ok);
	const auto out = burnBatch(in, bob);
	EXPECT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.completed, 2U);
	EXPECT_FALSE(exists(0));
	EXPECT_TRUE(exists(1));
	EXPECT_FALSE(exists(999));
	EXPECT_EQ(mintStatus().remainingItems, 0U);
	EXPECT_EQ(mintStatus().remainingUnits, 0);
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, UriResolutionHandlesRepeatedPlaceholdersAndOverflowWithoutChangingStoredUri)
{
	collection();
	auto input = edition(1);
	input.metadata = metadata("ipfs://{id}/{id}");
	ASSERT_EQ(action(6, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	EXPECT_EQ(resolved(0), "ipfs://" + std::string(64, '0') + "/" + std::string(64, '0'));
	QNFT::URI_output raw{};
	callFunction(QNFT_CONTRACT_INDEX, 9, QNFT::URI_input{0}, raw);
	EXPECT_EQ(raw.uri.get(7), '{');
	const std::string tooLong = "ipfs://" + std::string(200, 'x') + "{id}";
	input.metadata = metadata(tooLong.c_str());
	ASSERT_EQ(action(6, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ResolvedURI_output output{};
	callFunction(QNFT_CONTRACT_INDEX, 10, QNFT::ResolvedURI_input{1}, output);
	EXPECT_EQ(output.code, QNFT::Code::Capacity);
	EXPECT_EQ(output.uri.get(0), 0);
}

TEST_F(NftTest, FailedRoyaltyIsAccumulatedAndRetriedExactlyOnce)
{
	collection();
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.receiver = alice;
	in.quantity = 2;
	in.metadata = metadata();
	in.royaltyReceiver = recipient;
	in.royaltyBps = 500;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output sale{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{1000, 2, 0}, sale, alice, 0));
	ASSERT_EQ(sale.code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto previous = getBalance(recipient);
	contractError[TESTEXA_CONTRACT_INDEX] = ContractErrorIPOFailed;
	for (unsigned i = 0; i < 2; ++i)
	{
		QNFT::Buy_output bought{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, sale.listingId, 1, 1000, 1}, bought, bob, 1000));
		EXPECT_EQ(bought.code, QNFT::Code::Ok);
		EXPECT_EQ(bought.royalty, 50);
		QNFT::GetRefund_output debt{};
		callFunction(QNFT_CONTRACT_INDEX, 27, QNFT::GetRefund_input{recipient}, debt);
		EXPECT_EQ(debt.quantity, i == 0 ? 50 : 117);
		EXPECT_EQ(getBalance(recipient), previous);
		expectQuCollateral();
		if (i == 0)
		{
			// An independent refund shares the same beneficiary entry before the next purchase.
			state->contractRefunds.set(recipient, debt.quantity + 17);
			increaseEnergy(nftId, 17);
		}
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	sint64 pending = 0;
	ASSERT_TRUE(state->contractRefunds.get(recipient, pending));
	EXPECT_EQ(pending, 117);
	contractError[TESTEXA_CONTRACT_INDEX] = NoContractError;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), previous + 117);
	EXPECT_FALSE(state->contractRefunds.contains(recipient));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient), previous + 117);
	EXPECT_EQ(balance(0, bob), 2);
	expectQuCollateral();
}

TEST_F(NftTest, RoyaltyDebtCapacityRejectsBeforeDeliveringAndAllowsExistingRecipient)
{
	collection();
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	OperationCommand in{};
	in.operation = QNFT::Operation::Mint;
	in.receiver = alice;
	in.quantity = 2;
	in.metadata = metadata();
	in.royaltyReceiver = recipient;
	in.royaltyBps = 500;
	ASSERT_EQ(run(in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	QNFT::ListForSale_output sale{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{1000, 2, 0}, sale, alice, 0));
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 i = 0; i < QNFT_CONTRACT_REFUND_SLOTS; ++i)
	{
		state->contractRefunds.set(id(800000 + i, 1, 0, 0), 1);
	}
	increaseEnergy(nftId, QNFT_CONTRACT_REFUND_SLOTS);
	contractError[TESTEXA_CONTRACT_INDEX] = ContractErrorIPOFailed;
	const auto buyer = getBalance(bob), fees = state->epochFees;
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, sale.listingId, 1, 1000, 1}, bought, bob, 1000));
	EXPECT_EQ(bought.code, QNFT::Code::Capacity);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(getBalance(bob), buyer);
	EXPECT_EQ(state->epochFees, fees);
	state->contractRefunds.removeByKey(id(800000, 1, 0, 0));
	state->contractRefunds.set(recipient, 1);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, sale.listingId, 1, 1000, 1}, bought, bob, 1000));
	EXPECT_EQ(bought.code, QNFT::Code::Ok);
	sint64 debt = 0;
	ASSERT_TRUE(state->contractRefunds.get(recipient, debt));
	EXPECT_EQ(debt, 51);
	contractError[TESTEXA_CONTRACT_INDEX] = NoContractError;
	expectQuCollateral();
}

TEST_F(Nft1155Test, MintWrapperValidationPreservesCountAndRefunds)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto before = getBalance(alice);
	const auto fees = state->epochFees;
	for (const uint32 count : {3U, 0xffffffffU})
	{
		QNFT::MintBatch_input input{};
		input.count = count;
		const auto result = mintBatch(input, TEST_MINT_FEE);
		EXPECT_EQ(result.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(result.failureCode, QNFT::Code::InvalidInput);
		EXPECT_EQ(result.failureIndex, count);
		EXPECT_EQ(result.attempted, 0U);
		EXPECT_EQ(result.completed, 0U);
		EXPECT_EQ(result.fee, 0);
		EXPECT_EQ(getBalance(alice), before);
	}
	for (const uint32 count : {0U, 1U, 16U, 17U, 0xffffffffU})
	{
		QNFT::MultiMint_input input{};
		input.count = count;
		input.prototype.quantity = 1;
		QNFT::MultiMint_output result{};
		ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 8, input, result, alice, TEST_MINT_FEE));
		EXPECT_EQ(result.code, QNFT::Code::InvalidInput);
		EXPECT_EQ(result.failureCode, QNFT::Code::InvalidInput);
		EXPECT_EQ(result.failureIndex, count);
		EXPECT_EQ(result.attempted, 0U);
		EXPECT_EQ(result.completed, 0U);
		EXPECT_EQ(result.fee, 0);
		EXPECT_EQ(getBalance(alice), before);
	}
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(state->itemCount, 0U);
	expectQuCollateral();
}

TEST_F(Nft1155Test, GlobalApprovalEmitsOnlyItsOwnEventAndSettlesFees)
{
	collection();
	ASSERT_EQ(mint().code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	QNFT::SetApprovalForAll_input otherOperator{carol, true};
	ASSERT_EQ(action(11, otherOperator, alice).code, QNFT::Code::Ok);
	state->approveFee = 100;
	const auto before = getBalance(alice);
	const auto fees = state->epochFees;
	auto previous = state->lastEventId;
	const auto result = action(11, QNFT::SetApprovalForAll_input{bob, 1}, alice, 150);
	ASSERT_EQ(result.code, QNFT::Code::Ok);
	EXPECT_EQ(result.assetName, 0);
	ASSERT_EQ(state->lastEventId, previous + 1);
	const auto event = state->journal.get(previous & QNFT_EVENT_SLOT_MASK);
	EXPECT_EQ(event.kind, QNFT::EventKind::ApprovalForAll);
	EXPECT_EQ(event.from, alice);
	EXPECT_EQ(event.to, bob);
	EXPECT_EQ(event.quantity, 1);
	EXPECT_EQ(getBalance(alice), before - 100);
	EXPECT_EQ(state->epochFees, fees + 100);
	QNFT::IsApprovedForAll_output approval{};
	callFunction(QNFT_CONTRACT_INDEX, 19, QNFT::IsApprovedForAll_input{alice, carol}, approval);
	EXPECT_TRUE(approval.approved);
	previous = state->lastEventId;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, 0}, alice, 150).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, previous + 1);
	EXPECT_EQ(state->journal.get(previous & QNFT_EVENT_SLOT_MASK).quantity, 0);
	previous = state->lastEventId;
	EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, 1}, alice, 99).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(state->lastEventId, previous);
	EXPECT_EQ(getBalance(alice), before - 100);
	EXPECT_EQ(state->epochFees, fees + 100);
	expectQuCollateral();
}

TEST_F(Nft1155Test, RejectedAndEmptyBatchesPreserveCleanState)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto feeBefore = state->epochFees;
	const auto eventsBefore = state->lastEventId;
	const auto mask = 1ULL << (QNFT_CONTRACT_INDEX & 63);
	for (uint32 count : {0U, 3U})
	{
		contractStateChangeFlags[QNFT_CONTRACT_INDEX >> 6] &= ~mask;
		QNFT::MintBatch_input input{};
		input.count = count;
		const auto result = mintBatch(input, 0);
		EXPECT_EQ(result.code, count == 0 ? QNFT::Code::Ok : QNFT::Code::InvalidInput);
		EXPECT_EQ(result.fee, 0);
		EXPECT_EQ(state->epochFees, feeBefore);
		EXPECT_EQ(state->lastEventId, eventsBefore);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(contractStateChangeFlags[QNFT_CONTRACT_INDEX >> 6] & mask, 0ULL);
	}
	contractStateChangeFlags[QNFT_CONTRACT_INDEX >> 6] &= ~mask;
	QNFT::Mint_input invalid{};
	EXPECT_EQ(action(6, invalid, alice, 0).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(contractStateChangeFlags[QNFT_CONTRACT_INDEX >> 6] & mask, 0ULL);
}

TEST_F(Nft1155ExtrasTest, OptionalCollectionLimitsAreIndependentAndPreserved)
{
	struct Limits
	{
		sint64 units;
		uint32 items;
	};

	uint32 collectionId = 0;
	for (const auto limits : {Limits{0, 0}, Limits{0, 2}, Limits{5, 0}, Limits{5, 2}})
	{
		const auto created = action(1, QNFT::CreateCollection_input{metadata(), limits.units, limits.items}, alice, QNFT_COLLECTION_FEE);
		ASSERT_EQ(created.code, QNFT::Code::Ok);
		ASSERT_EQ(created.itemId, collectionId);
		const auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
		EXPECT_EQ(state->collections.get(collectionId).maxUnits, limits.units);
		EXPECT_EQ(state->collections.get(collectionId).maxItems, limits.items);
		QNFT::GetCollection_output saved{};
		callFunction(QNFT_CONTRACT_INDEX, 1, QNFT::GetCollection_input{collectionId}, saved);
		ASSERT_EQ(saved.code, QNFT::Code::Ok);
		EXPECT_EQ(saved.collection.maxItems, limits.items);
		EXPECT_EQ(saved.collection.maxUnits, limits.units);
		const auto firstItem = state->itemCount;
		QNFT::MintBatch_input batch{};
		batch.count = 2;
		auto request = edition(2);
		request.collectionId = collectionId;
		batch.entries.set(0, request);
		batch.entries.set(1, request);
		ASSERT_EQ(mintBatch(batch, 2 * TEST_MINT_FEE).code, QNFT::Code::Ok);
		auto status = mintStatus(collectionId);
		EXPECT_EQ(status.remainingItems, limits.items ? 0U : QNFT_MAX_ITEMS - 2);
		EXPECT_EQ(status.remainingUnits, limits.units ? 1 : MAX_AMOUNT - 4);
		ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, firstItem}).code, QNFT::Code::Ok);
		EXPECT_EQ(mintStatus(collectionId).remainingUnits, status.remainingUnits);
		EXPECT_EQ(mintStatus(collectionId).remainingItems, status.remainingItems);
		EXPECT_EQ(action(6, request, alice, TEST_MINT_FEE).code, limits.items || limits.units ? QNFT::Code::Capacity : QNFT::Code::Ok);
		++collectionId;
	}
	expectQuCollateral();
}

TEST_F(Nft1155ExtrasTest, OptionalCollectionLimitsKeepBatchPreflightAndTechnicalBounds)
{
	ASSERT_EQ(action(1, QNFT::CreateCollection_input{metadata(), 0, 0}, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Ok);
	QNFT::MultiMint_input multiInput{};
	multiInput.count = 2;
	multiInput.prototype = edition(0);
	multiInput.quantities.set(0, 2);
	multiInput.quantities.set(1, 3);
	ASSERT_EQ(multi(multiInput, 2 * TEST_MINT_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Paused), QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Conflict);
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Open), QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto c = state->collections.get(0);
	// Model a nearly exhausted lifetime counter without issuing MAX_AMOUNT units in the fixture.
	c.issuedUnits = MAX_AMOUNT - 3;
	state->collections.set(0, c);
	QNFT::MintBatch_input batch{};
	batch.count = 2;
	batch.entries.set(0, edition(2));
	batch.entries.set(1, edition(2));
	const auto before = getBalance(alice);
	const auto count = state->itemCount;
	EXPECT_EQ(mintBatch(batch, 2 * TEST_MINT_FEE).code, QNFT::Code::Capacity);
	EXPECT_EQ(state->itemCount, count);
	EXPECT_EQ(getBalance(alice), before);
	EXPECT_EQ(mintStatus().remainingUnits, 3);
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	EXPECT_EQ(mintStatus().remainingUnits, 0);
	EXPECT_EQ(mint(1).code, QNFT::Code::Capacity);
	c = state->collections.get(0);
	c.issuedUnits = 8;
	state->collections.set(0, c);
	state->itemCount = QNFT_MAX_ITEMS;
	EXPECT_EQ(mint(1).code, QNFT::Code::Capacity);
	state->itemCount = count + 1;
	ASSERT_EQ(setStatus(QNFT::MintingStatus::Closed), QNFT::Code::Ok);
	EXPECT_EQ(mint(1).code, QNFT::Code::Conflict);
	EXPECT_EQ(setStatus(QNFT::MintingStatus::Open), QNFT::Code::Conflict);
	expectQuCollateral();
}

TEST_F(NftTest, CollectionPolicyBlocksExternalTransfersAndExport)
{
	static_assert(sizeof(QNFT::CreateCollection_input) == 304);
	QNFT::OperationResult created{};
	ASSERT_TRUE(
	    invokeUserProcedure(QNFT_CONTRACT_INDEX, 1, QNFT::CreateCollection_input{metadata(), 1000, 10, 1}, created, alice, QNFT_COLLECTION_FEE));
	ASSERT_EQ(created.code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	const id receiver(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	const auto funds = getBalance(alice);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}, alice, 123).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, receiver, 1, 0}, alice, 123).code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(balance(0, alice), 10);
}

TEST_F(NftTest, CollectionPolicyIsValidatedExposedAndSurvivesAdministrationTransfer)
{
	const auto funds = getBalance(alice);
	const auto created = action(1, QNFT::CreateCollection_input{metadata(), 1000, 10, true}, alice, QNFT_COLLECTION_FEE);
	EXPECT_EQ(getBalance(alice), funds - QNFT_COLLECTION_FEE);
	ASSERT_EQ(created.code, QNFT::Code::Ok);
	EXPECT_EQ(created.itemId, 0U);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	QNFT::GetCollection_output out{};
	callFunction(QNFT_CONTRACT_INDEX, 1, QNFT::GetCollection_input{0}, out);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.collection.externalTransfersDisabled, 1U);
	EXPECT_EQ(out.collection.administrator, bob);
	collection();
	CollectionPageWire page{};
	callFunction(QNFT_CONTRACT_INDEX, 2, RegistryPageQueryWire{0, 16}, page);
	ASSERT_EQ(page.code, QNFT::Code::Ok);
	ASSERT_EQ(page.count, 2U);
	EXPECT_EQ(page.collections.get(0).collection.externalTransfersDisabled, 1U);
	EXPECT_EQ(page.collections.get(1).collection.externalTransfersDisabled, 0U);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, TESTEXA_CONTRACT_INDEX}).code, QNFT::Code::Conflict);
}

TEST_F(NftTest, CollectionPolicyBlocksOperatorsWithoutCallingDestinationOrChargingFees)
{
	ASSERT_EQ(action(1, QNFT::CreateCollection_input{metadata(), 1000, 10, 1}, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] = forgeFromManagementCallback;
	const id receiver(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->transferFee = 7;
	state->exportFee = 11;
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, 1}).code, QNFT::Code::Ok);
	const auto fees = state->epochFees;
	for (const id caller : {alice, bob, carol})
	{
		const auto funds = getBalance(caller);
		EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 100, 0, TESTEXA_CONTRACT_INDEX}, caller, 123).code, QNFT::Code::Conflict);
		EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, receiver, 1, 0}, caller, 123).code, QNFT::Code::Conflict);
		EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, receiver, 0, 0}, caller, 123).code, QNFT::Code::Conflict);
		EXPECT_EQ(getBalance(caller), funds);
	}
	EXPECT_EQ(manager->heavyComputationResult, 0);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 10);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(balance(0, receiver), 0);
	expectQuCollateral();
}

TEST_F(Nft1155Test, CollectionPolicyPreflightsMixedBatchBeforeMovingAnyUnits)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(1, QNFT::CreateCollection_input{metadata(), 1000, 10, 1}, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10, QNFT::Policy::Immutable, 1).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input in{};
	in.owner = alice;
	in.receiver = id(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	in.count = 2;
	in.entries.set(0, QNFT::TokenAmount{2, 0});
	in.entries.set(1, QNFT::TokenAmount{3, 1});
	const auto funds = getBalance(alice);
	const auto out = transferBatch(in, alice, 123);
	EXPECT_EQ(out.code, QNFT::Code::Conflict);
	EXPECT_EQ(out.completed, 0U);
	EXPECT_EQ(out.attempted, 0U);
	EXPECT_EQ(out.failureIndex, 1U);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(owned(0, alice), 10);
	EXPECT_EQ(owned(1, alice), 10);
	EXPECT_EQ(owned(0, in.receiver), 0);
	EXPECT_EQ(owned(1, in.receiver), 0);
	in.receiver = bob;
	EXPECT_EQ(transferBatch(in).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, bob), 2);
	EXPECT_EQ(owned(1, bob), 3);
	expectQuCollateral();
}

TEST_F(NftTest, CollectionPolicyPreservesUserTransfersBurnAndShopEscrow)
{
	ASSERT_EQ(action(1, QNFT::CreateCollection_input{metadata(), 1000, 10, 1}, alice, QNFT_COLLECTION_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 2, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{bob, 1, 0}, bob).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10000, 7, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{carol, listed.listingId, 3, 10000, 1}, bought, bob, 30000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, alice), 5);
	EXPECT_EQ(balance(0, bob), 1);
	EXPECT_EQ(balance(0, carol), 3);
	EXPECT_EQ(balance(0, nftId), 0);
	expectQuCollateral();
}

TEST_F(NftFeeTest, UnknownFeeVariableCannotBeProposed)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto funds = getBalance(alice);
	EXPECT_EQ(propose(QNFT_FEE_VARIABLE_COUNT, 123, alice, 1000017), INVALID_PROPOSAL_INDEX);
	EXPECT_EQ(getBalance(alice), funds);
	expectQuCollateral();
}

TEST_F(NftTest, ExportToErroredManagerRefundsWithoutAbortingOrMovingAssets)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	const id receiver(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	manager->preAcquireSharesOutput.requestedFee = 7;
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->exportFee = 11;
	const auto userFunds = getBalance(alice), managerFunds = getBalance(receiver), fees = state->epochFees;
	const auto journal = state->lastEventId;
	// Model an already failed destination; no caller is granted permission to set error flags.
	contractError[TESTEXA_CONTRACT_INDEX] = ContractErrorIPOFailed;
	const QNFT::Export_input request{alice, 1, 7, 0, TESTEXA_CONTRACT_INDEX};
	EXPECT_EQ(action(16, request, alice, 31).code, QNFT::Code::ExternalFailure);
	EXPECT_EQ(contractError[QNFT_CONTRACT_INDEX], NoContractError);
	EXPECT_EQ(getBalance(alice), userFunds);
	EXPECT_EQ(getBalance(receiver), managerFunds);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(state->lastEventId, journal);
	EXPECT_FALSE(state->exporting.get(0));
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 10);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
	EXPECT_EQ(manager->prevPreAcquireSharesInput.numberOfShares, 0);
	EXPECT_EQ(manager->postAcquireShareCounter, 0);
	// Unrelated user operations remain usable, and the same export works after destination recovery.
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 1, 0}).code, QNFT::Code::Ok);
	contractError[TESTEXA_CONTRACT_INDEX] = NoContractError;
	EXPECT_EQ(action(16, request, alice, 31).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), userFunds - 18);
	EXPECT_EQ(getBalance(receiver), managerFunds + 7);
	EXPECT_EQ(state->epochFees, fees + 11);
	EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 8);
	EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 1);
	expectQuCollateral();
}

namespace
{
	using RentalPolicyWire = QNFT::SetExternalTransfersDisabled_input;

	struct RentalOfferWire
	{
		sint64 dailyPrice;
		sint64 quantity;
		uint32 itemId;
		uint32 periodDays;
	};

	struct RentalAcceptWire
	{
		uint64 offerId;
		sint64 quantity;
		sint64 expectedDailyPrice;
		uint32 expectedPeriodDays;
	};

	struct RentalResultWire
	{
		uint64 rentalId;
		uint64 offerId;
		sint64 paid;
		QNFT::Code code;
	};
} // namespace

TEST_F(NftTest, RentalPolicyCreatorAndCurrentAdministratorCanChangeRestriction)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][5], nullptr);
	EXPECT_EQ(action(5, RentalPolicyWire{0, 1}, carol, 123).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(5, RentalPolicyWire{0, 1}, alice, 123).code, QNFT::Code::Ok);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 1, 0, 0, QX_CONTRACT_INDEX}).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(5, RentalPolicyWire{0, 0}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(action(5, RentalPolicyWire{0, 1}, bob).code, QNFT::Code::Ok);
	RentalPolicyWire noncanonical{0, true};
	noncanonical.disabled.charValue = 2;
	EXPECT_EQ(action(5, noncanonical, alice).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftTest, RentalChargesPeriodAndLocksOnlyRentedQuantity)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_NE(contractUserProcedures[QNFT_CONTRACT_INDEX][20], nullptr);
	RentalResultWire offered{}, rented{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 20, RentalOfferWire{100, 2, 0, 15}, offered, alice, 0));
	ASSERT_EQ(offered.code, QNFT::Code::Ok);
	const auto ownerBefore = getBalance(alice), renterBefore = getBalance(bob);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 22, RentalAcceptWire{offered.offerId, 2, 100, 15}, rented, bob, 6000));
	ASSERT_EQ(rented.code, QNFT::Code::Ok);
	EXPECT_EQ(rented.paid, 3000);
	EXPECT_EQ(getBalance(bob), renterBefore - 6000);
	// Alice is both lender and immutable collection creator: 2700 + 90.
	EXPECT_EQ(getBalance(alice), ownerBefore + 2790);
	EXPECT_EQ(balance(0, alice), 10);
	EXPECT_EQ(balance(0, bob), 0);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 9, 0}).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 8, 0}).code, QNFT::Code::Ok);
}

class NftRentalTest : public NftTest
{
public:
	uint64 open(sint64 reward = 6000, sint64 units = 2, sint64 daily = 100, uint32 days = 15)
	{
		collection();
		EXPECT_EQ(mint(10).code, QNFT::Code::Ok);
		QNFT::RentalResult offer{}, rental{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 20, QNFT::CreateRentalOffer_input{daily, units, 0, days}, offer, alice, 0));
		EXPECT_EQ(offer.code, QNFT::Code::Ok);
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 22, QNFT::Rent_input{offer.offerId, units, daily, days}, rental, bob, reward));
		EXPECT_EQ(rental.code, QNFT::Code::Ok);
		return rental.rentalId;
	}

	QNFT::GetRental_output rental(uint64 key)
	{
		QNFT::GetRental_output out{};
		callFunction(QNFT_CONTRACT_INDEX, 25, QNFT::GetRental_input{key}, out);
		return out;
	}

	template<class T>
	QNFT::RentalResult change(unsigned short proc, const T& in, const id& user = bob, sint64 reward = 0)
	{
		QNFT::RentalResult out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, proc, in, out, user, reward));
		return out;
	}

	void advance(uint32 days)
	{
		QPI::DateAndTime date(utcTime.Year, utcTime.Month, utcTime.Day, utcTime.Hour, utcTime.Minute, utcTime.Second);
		ASSERT_TRUE(date.addDays(days));
		utcTime.Year = date.getYear();
		utcTime.Month = date.getMonth();
		utcTime.Day = date.getDay();
		++system.tick;
		updateQpiTime();
	}
};

TEST_F(NftRentalTest, ShareholderVotesChangeRentalChargesAndRenewals)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	const auto protocolSlot = propose(12, 1000);
	ASSERT_NE(protocolSlot, INVALID_PROPOSAL_INDEX);
	ASSERT_TRUE(ballot(protocolSlot, proposal(protocolSlot)));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	++system.tick;
	const auto creatorSlot = propose(13, 500);
	ASSERT_NE(creatorSlot, INVALID_PROPOSAL_INDEX);
	ASSERT_TRUE(ballot(creatorSlot, proposal(creatorSlot)));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);

	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 10, 0}).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, carol);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	const auto creatorBefore = getBalance(alice), ownerBefore = getBalance(carol);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto feesBefore = state->epochFees;
	QNFT::GetFeeSchedule_output fees{};
	callFunction(QNFT_CONTRACT_INDEX, 31, NoData{}, fees);
	EXPECT_EQ(fees.rentalProtocolBPS, 1000U);
	EXPECT_EQ(fees.rentalCreatorBPS, 500U);
	const auto rented = change(22, QNFT::Rent_input{offer.offerId, 2, 100, 15}, bob, 6000);
	ASSERT_EQ(rented.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice) - creatorBefore, 150);
	EXPECT_EQ(getBalance(carol) - ownerBefore, 2550);
	EXPECT_EQ(state->epochFees - feesBefore, 300);
	advance(15);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(alice) - creatorBefore, 300);
	EXPECT_EQ(getBalance(carol) - ownerBefore, 5100);
	EXPECT_EQ(state->epochFees - feesBefore, 600);
	EXPECT_EQ(state->developerFeeBPS, 2000U);
	expectQuCollateral();
}

TEST_F(NftRentalTest, RentalFeeGovernanceRejectsOverAllocationAndAcceptsAtomicRebalance)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{alice, 676}};
	issueContractShares(QNFT_CONTRACT_INDEX, owners);
	EXPECT_EQ(propose(12, 9701), INVALID_PROPOSAL_INDEX);
	EXPECT_EQ(propose(13, 9301), INVALID_PROPOSAL_INDEX);
	EXPECT_EQ(propose(12, -1), INVALID_PROPOSAL_INDEX);
	EXPECT_EQ(propose(13, 10001), INVALID_PROPOSAL_INDEX);
	FeeBatchInputWire input{};
	input.epoch = system.epoch;
	input.count = 2;
	input.changes[0] = {12, 9800};
	input.changes[1] = {13, 201};
	uint16 slot = INVALID_PROPOSAL_INDEX;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 0));
	EXPECT_EQ(slot, INVALID_PROPOSAL_INDEX);
	input.changes[1].value = 200;
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 27, input, slot, alice, 0));
	ASSERT_NE(slot, INVALID_PROPOSAL_INDEX);
	ASSERT_TRUE(ballot(slot, proposal(slot)));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_EPOCH);
	const auto before = getBalance(alice);
	open(3000);
	// Creator and lender are Alice: only the 2% creator allocation remains.
	EXPECT_EQ(getBalance(alice) - before, -1000000 - TEST_MINT_FEE + 60);
	expectQuCollateral();
}

TEST_F(NftRentalTest, RentalPeriodProjectionTracksFundedCoverage)
{
	QNFT::Rental rental{};
	rental.paidUntil = QPI::DateAndTime(2026, 11, 1, 12);
	rental.status = QNFT::RentalStatus::Active;
	rental.autoRenew = true;
	rental.periodDays = 15;
	rental.periodPrice = 1500;
	rental.deposit = 3000;
	QNFT::RentalPeriodProjection projection{};
	QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 11, 1, 12), projection);
	EXPECT_TRUE(projection.active);
	EXPECT_EQ(projection.periodsPaid, 1);
	EXPECT_EQ(projection.paidUntil, QPI::DateAndTime(2026, 11, 16, 12));
	QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 11, 20, 12), projection);
	EXPECT_TRUE(projection.active);
	EXPECT_EQ(projection.periodsPaid, 2);
	rental.deposit = 1500;
	QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 11, 20, 12), projection);
	EXPECT_FALSE(projection.active);
	rental.autoRenew = false;
	QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 11, 1, 12), projection);
	EXPECT_FALSE(projection.active);
	EXPECT_EQ(projection.periodsPaid, 0);
	QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 11, 1, 11), projection);
	EXPECT_TRUE(projection.active);
	EXPECT_EQ(rental.deposit, 1500);
	EXPECT_EQ(rental.paidUntil, QPI::DateAndTime(2026, 11, 1, 12));
}

TEST_F(NftRentalTest, BootstrapDateBlocksRentalTimeUseRegardlessOfHour)
{
	const auto key = open();
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 1, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto deadlines = state->rentalDeadlines.population();
	const auto deposits = state->rentalDeposits;
	const auto journal = state->lastEventId;
	const auto funds = getBalance(bob);
	for (unsigned hour : {0U, 12U, 23U})
	{
		utcTime.Year = 2022;
		utcTime.Month = 4;
		utcTime.Day = 13;
		utcTime.Hour = hour;
		updateQpiTime();
		EXPECT_EQ(change(20, QNFT::CreateRentalOffer_input{100, 1, 0, 15}, alice).code, QNFT::Code::Conflict);
		EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, bob, 1500).code, QNFT::Code::Conflict);
		EXPECT_EQ(rental(key).code, QNFT::Code::Conflict);
		callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
		EXPECT_EQ(state->rentalDeadlines.population(), deadlines);
		EXPECT_EQ(state->rentalDeposits, deposits);
		EXPECT_EQ(state->lastEventId, journal);
		EXPECT_EQ(getBalance(bob), funds);
	}
	utcTime.Year = 2026;
	utcTime.Month = 10;
	utcTime.Day = 17;
	utcTime.Hour = 12;
	updateQpiTime();
	EXPECT_EQ(rental(key).code, QNFT::Code::Ok);
	EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, bob, 1500).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftRentalTest, RenewalIsChargedAtBoundaryAndOnlyOnceDespiteRepeatedTick)
{
	const auto key = open();
	const auto first = rental(key).rental.paidUntil;
	// The fixture starts at 2026-10-17 12:00 UTC; the first paid period lasts 15 days.
	EXPECT_EQ(first, 1793534400000000ULL);
	advance(14);
	EXPECT_EQ(rental(key).rental.deposit, 3000);
	EXPECT_EQ(rental(key).rental.paidUntil, first);
	advance(1);
	const auto projected = rental(key);
	EXPECT_EQ(projected.rental.deposit, 0);
	EXPECT_EQ(projected.rental.status, QNFT::RentalStatus::Active);
	const auto before = getBalance(alice);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(alice), before + 2790);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(alice), before + 2790);
	EXPECT_EQ(rental(key).rental.paidUntil, projected.rental.paidUntil);
	expectQuCollateral();
}

TEST_F(NftRentalTest, EmptyDepositEndsAndLateTopupCannotReviveRights)
{
	const auto key = open();
	EXPECT_EQ(change(24, QNFT::RentalDeposit_input{key, 3000}).code, QNFT::Code::Ok);
	advance(15);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Ended);
	const auto before = getBalance(bob);
	EXPECT_EQ(change(23, QNFT::GetRental_input{key}, bob, 3000).code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(bob), before);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 10, 0}).code, QNFT::Code::Ok);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	expectQuCollateral();
}

TEST_F(NftRentalTest, BothPartiesCanStopRenewalWithoutRefundingPaidAccess)
{
	const auto key = open();
	EXPECT_EQ(change(25, QNFT::GetRental_input{key}, carol).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(change(25, QNFT::GetRental_input{key}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(change(25, QNFT::GetRental_input{key}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Active);
	EXPECT_FALSE(rental(key).rental.autoRenew);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 9, 0}).code, QNFT::Code::Conflict);
	advance(15);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Ended);
	EXPECT_EQ(rental(key).rental.deposit, 3000);
	EXPECT_EQ(change(24, QNFT::RentalDeposit_input{key, 3001}).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(change(24, QNFT::RentalDeposit_input{key, 3000}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 10, 0}).code, QNFT::Code::Ok);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	expectQuCollateral();
}

TEST_F(NftRentalTest, WithdrawalAtBoundaryCannotTakeAlreadyConsumedRenewal)
{
	const auto key = open();
	advance(15);
	const auto funds = getBalance(bob);
	EXPECT_EQ(change(24, QNFT::RentalDeposit_input{key, 1}).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(getBalance(bob), funds);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Active);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	expectQuCollateral();
}

TEST_F(NftRentalTest, FullPairTraversalIncludesRentalsBeyondFormerLimit)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{1, 71, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	for (unsigned i = 0; i < 70; ++i)
	{
		ASSERT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 1, 15}, bob, 15).code, QNFT::Code::Ok) << i;
	}

	struct SummaryWire
	{
		sint64 owned, managed, offered, lent, rented, available;
		QNFT::Code code;
	} owner{}, user{};

	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::RentalBalance_input{alice, 0}, owner);
	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::RentalBalance_input{bob, 0}, user);
	ASSERT_EQ(owner.code, QNFT::Code::Ok);
	ASSERT_EQ(user.code, QNFT::Code::Ok);
	EXPECT_EQ(owner.lent, 70);
	EXPECT_EQ(user.rented, 70);
	EXPECT_EQ(owner.available, 29);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 30, 0}).code, QNFT::Code::Conflict);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 29, 0}).code, QNFT::Code::Ok);
	advance(15);
	// New rental sweeps the entire expired list; removals must not skip relocated entries.
	ASSERT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 1, 15}, bob, 15).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->lentRentals.population(), 1);
	EXPECT_EQ(state->takenRentals.population(), 1);
	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::RentalBalance_input{alice, 0}, owner);
	EXPECT_EQ(owner.lent, 1);
	EXPECT_EQ(owner.available, 70);
	expectQuCollateral();
}

TEST_F(NftRentalTest, UserSummarySeparatesOwnershipAndRentedRights)
{
	open();
	ASSERT_NE(contractUserFunctions[QNFT_CONTRACT_INDEX][17], nullptr);

	struct SummaryWire
	{
		sint64 owned, managed, offered, lent, rented, available;
		QNFT::Code code;
	} owner{}, user{};

	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::RentalBalance_input{alice, 0}, owner);
	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::RentalBalance_input{bob, 0}, user);
	EXPECT_EQ(owner.code, QNFT::Code::Ok);
	EXPECT_EQ(user.code, QNFT::Code::Ok);
	EXPECT_EQ(owner.owned, 10);
	EXPECT_EQ(owner.lent, 2);
	EXPECT_EQ(owner.available, 8);
	EXPECT_EQ(user.owned, 0);
	EXPECT_EQ(user.rented, 2);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{bob, carol, 1, 0}, bob).code, QNFT::Code::InsufficientBalance);
}

TEST_F(NftRentalTest, MissedPeriodsUsePerPeriodRoundingAndCannotBePaidTwice)
{
	const auto key = open(150, 1, 1);
	const auto before = getBalance(alice);
	advance(46);
	const auto projected = rental(key);
	EXPECT_EQ(projected.rental.deposit, 90);
	EXPECT_EQ(projected.rental.status, QNFT::RentalStatus::Active);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(alice), before + 42); // Three periods, each 15 - floor(7%) - floor(3%).
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(alice), before + 42);
	EXPECT_EQ(change(23, QNFT::GetRental_input{key}, bob, 15).code, QNFT::Code::Ok);
	EXPECT_EQ(rental(key).rental.deposit, 105);
	expectQuCollateral();
}

TEST_F(NftRentalTest, PartialOfferCancellationReleasesOnlyUnpaidUnits)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 6, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 5, 0}).code, QNFT::Code::Conflict);
	const auto rented = change(22, QNFT::Rent_input{offer.offerId, 2, 100, 15}, bob, 3000);
	ASSERT_EQ(rented.code, QNFT::Code::Ok);
	EXPECT_EQ(change(21, QNFT::CancelRentalOffer_input{offer.offerId}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(change(21, QNFT::CancelRentalOffer_input{offer.offerId}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 9, 0}).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 8, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(rental(rented.rentalId).rental.quantity, 2);
	expectQuCollateral();
}

TEST_F(NftRentalTest, RentalIncomeUsesImmutableCreatorAndCurrentOwner)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 10, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, carol);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	const auto artistFunds = getBalance(alice), ownerFunds = getBalance(carol);
	const auto rented = change(22, QNFT::Rent_input{offer.offerId, 2, 100, 15}, bob, 3000);
	ASSERT_EQ(rented.code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(alice), artistFunds + 90);
	EXPECT_EQ(getBalance(carol), ownerFunds + 2700);
	expectQuCollateral();
}

TEST_F(NftRentalTest, InvalidTermsAndStaleAcceptanceRefundPayment)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	EXPECT_EQ(change(20, QNFT::CreateRentalOffer_input{100, 1, 0, 14}, alice).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(change(20, QNFT::CreateRentalOffer_input{MAX_AMOUNT, 1, 0, 15}, alice).code, QNFT::Code::InvalidInput);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice);
	const auto funds = getBalance(bob);
	EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 101, 15}, bob, 3000).code, QNFT::Code::Conflict);
	EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, bob, 1499).code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(getBalance(bob), funds);
	expectQuCollateral();
}

TEST_F(NftRentalTest, UserPagesIncludeExternalOwnershipAndDistinguishRentalRights)
{
	// Zero disables the API's snapshot check; use an explicit snapshot even in an isolated test run.
	if (!system.tick)
	{
		system.tick = 1;
	}
	const auto key = open();
	INIT_CONTRACT(TESTEXA);
	auto* manager = reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX]);
	manager->preAcquireSharesOutput.allowTransfer = true;
	manager->preAcquireSharesOutput.requestedFee = 0;
	ASSERT_EQ(action(16, QNFT::Export_input{alice, 3, 0, 0, TESTEXA_CONTRACT_INDEX}).code, QNFT::Code::Ok);
	QNFT::GetUserNft_output summary{};
	callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::GetUserNft_input{alice, 0}, summary);
	EXPECT_EQ(summary.ownedQuantity, 10);
	EXPECT_EQ(summary.managedQuantity, 7);
	EXPECT_EQ(summary.availableQuantity, 5);
	for (const auto user : {alice, bob})
	{
		QNFT::ListUserNfts_input in{user, 0, 1, 0, QNFT::UserNftPhase::Ownership};
		sint64 owned = 0, rented = 0, external = 0;
		bool done = false;
		for (unsigned page = 0; page < 200 && !done; ++page)
		{
			QNFT::ListUserNfts_output out{};
			callFunction(QNFT_CONTRACT_INDEX, 18, in, out);
			ASSERT_EQ(out.code, QNFT::Code::Ok);
			for (uint32 i = 0; i < out.count; ++i)
			{
				const auto& row = out.entries.get(i);
				if (row.relation == QNFT::UserNftRelation::Owned)
				{
					owned += row.quantity;
					if (row.managingContract == TESTEXA_CONTRACT_INDEX)
					{
						external += row.quantity;
					}
				}
				else
				{
					rented += row.quantity;
					EXPECT_EQ(row.rentalId, key);
					EXPECT_EQ(row.owner, alice);
				}
			}
			in.cursor = out.nextCursor;
			in.phase = out.phase;
			in.tick = out.tick;
			done = out.done;
		}
		EXPECT_TRUE(done);
		EXPECT_EQ(owned, user == alice ? 10 : 0);
		EXPECT_EQ(external, user == alice ? 3 : 0);
		EXPECT_EQ(rented, user == bob ? 2 : 0);
		QNFT::ListUserNfts_output stale{};
		++system.tick;
		callFunction(QNFT_CONTRACT_INDEX, 18, in, stale);
		EXPECT_EQ(stale.code, QNFT::Code::Conflict);
	}
	expectQuCollateral();
}

TEST_F(NftRentalTest, AssetOwnerIteratorHonorsBudgetAndCursorValidation)
{
	open();
	QPI::AssetOwnerIterator it;
	it.begin(alice, nftId);
	EXPECT_FALSE(it.next(0));
	EXPECT_EQ(it.cursor(), 0U);
	sint64 total = 0;
	for (unsigned calls = 0; calls < 1000 && !it.reachedEnd(); ++calls)
	{
		const auto previous = it.cursor();
		if (it.next(1))
		{
			EXPECT_EQ(it.asset().issuer, nftId);
			total += it.numberOfOwnedShares();
		}
		EXPECT_LE(it.cursor() - previous, 1U);
	}
	EXPECT_TRUE(it.reachedEnd());
	EXPECT_EQ(total, 10);
	it.begin(alice, nftId, ASSETS_CAPACITY + 1);
	EXPECT_FALSE(it.isValidCursor());
	EXPECT_FALSE(it.next(1));
}

TEST_F(NftRentalTest, TickBacklogDoesNotHideFundedUsageOrChargeTwice)
{
	collection();
	ASSERT_EQ(mint(100).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{1, 33, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	uint64 last = 0;
	for (unsigned i = 0; i < 33; ++i)
	{
		const auto rented = change(22, QNFT::Rent_input{offer.offerId, 1, 1, 15}, bob, 30);
		ASSERT_EQ(rented.code, QNFT::Code::Ok);
		last = rented.rentalId;
	}
	// Drain initial payout queue before the simultaneous deadline.
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	advance(15);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->rentalDeposits, 15);
	EXPECT_EQ(rental(last).rental.deposit, 0);
	EXPECT_EQ(rental(last).rental.status, QNFT::RentalStatus::Active);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->rentalDeposits, 0);
	expectQuCollateral();
}

TEST_F(NftRentalTest, OperatorsAndDuplicateBatchesCannotSpendReservedUnits)
{
	open();
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 9, 0}, carol).code, QNFT::Code::Conflict);
	EXPECT_EQ(action(16, QNFT::Export_input{alice, 9, 0, 0, QX_CONTRACT_INDEX}, carol).code, QNFT::Code::Conflict);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 9, 0}, listed, alice, 0));
	EXPECT_EQ(listed.code, QNFT::Code::Conflict);
	QNFT::BatchTransfer_input transfer{};
	transfer.owner = alice;
	transfer.receiver = carol;
	transfer.count = 2;
	transfer.entries.set(0, QNFT::TokenAmount{5, 0});
	transfer.entries.set(1, QNFT::TokenAmount{4, 0});
	QNFT::Batch_output transferred{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 13, transfer, transferred, carol, 0));
	EXPECT_EQ(transferred.code, QNFT::Code::Conflict);
	EXPECT_EQ(transferred.completed, 0U);
	EXPECT_EQ(balance(0, alice), 10);
	EXPECT_EQ(balance(0, carol), 0);
	QNFT::BurnBatch_input burn{};
	burn.owner = alice;
	burn.count = 2;
	burn.entries.set(0, QNFT::TokenAmount{5, 0});
	burn.entries.set(1, QNFT::TokenAmount{4, 0});
	QNFT::Batch_output burned{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 15, burn, burned, carol, 0));
	EXPECT_EQ(burned.code, QNFT::Code::Conflict);
	EXPECT_EQ(burned.completed, 0U);
	expectQuCollateral();
}

TEST_F(NftRentalTest, PolicyRemainsMutableAfterClosingMintAndOldAdminLosesAccess)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}, bob).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
	ASSERT_EQ(action(4, QNFT::SetMintingStatus_input{0, QNFT::MintingStatus::Closed}, carol).code, QNFT::Code::Ok);
	EXPECT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, 1}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, 1}, carol).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto journal = state->lastEventId;
	const auto funds = getBalance(alice);
	EXPECT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, 1}, alice, 123).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, journal);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, 0}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, journal + 1);
	expectQuCollateral();
}

TEST_F(NftRentalTest, FailedRentalPaymentRemainsCollateralAndRetriesExactlyOnce)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	INIT_CONTRACT(TESTEXA);
	const id lender(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	increaseEnergy(lender, 1);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, lender, 2, 0}).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, lender);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	const auto accepted = change(22, QNFT::Rent_input{offer.offerId, 2, 100, 15}, bob, 6000);
	ASSERT_EQ(accepted.code, QNFT::Code::Ok);
	const auto ownerFunds = getBalance(lender), creatorFunds = getBalance(alice);
	advance(15);
	contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	contractCallbacksRunning = 0;
	EXPECT_EQ(getBalance(lender), ownerFunds);
	EXPECT_EQ(getBalance(alice), creatorFunds + 90);
	EXPECT_EQ(rental(accepted.rentalId).rental.ownerCredit, 2700);
	expectQuCollateral();
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(lender), ownerFunds + 2700);
	EXPECT_EQ(rental(accepted.rentalId).rental.ownerCredit, 0);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(lender), ownerFunds + 2700);
	expectQuCollateral();
}

namespace
{
	template<class T>
	bool setSaleDividend(T& value, uint64 name, uint16 bps)
	{
		value.dividendAssetName = name;
		value.dividendBps = bps;
		return true;
	}

	template<class T>
	sint64 saleDividendAmount(const T& value)
	{
		return value.shareholderAmount;
	}
} // namespace

class NftSaleDividendTest : public Nft1155Test
{
public:
	uint64 shares(unsigned index, std::vector<std::pair<m256i, unsigned int>> owners)
	{
		issueContractShares(index, owners);
		return *reinterpret_cast<const uint64*>(contractDescriptions[index].assetName);
	}

	QNFT::ListForSale_output list(sint64 price = 1000000, sint64 quantity = 1, uint32 itemId = 0)
	{
		QNFT::ListForSale_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{price, quantity, itemId}, out, alice, 0));
		EXPECT_EQ(out.code, QNFT::Code::Ok);
		return out;
	}

	QNFT::Buy_output buy(uint64 listing, sint64 price = 1000000, sint64 quantity = 1)
	{
		QNFT::Buy_output out{};
		EXPECT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listing, quantity, price, 1}, out, bob, price * quantity));
		return out;
	}
};

TEST_F(NftSaleDividendTest, PaysSelectedShareholdersAndReturnsDustToSeller)
{
	collection();
	auto in = edition(1);
	ASSERT_TRUE(setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{bob, 100}, {carol, 576}}), 1000));
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	const auto a = getBalance(alice), b = getBalance(bob), c = getBalance(carol);
	auto out = buy(listing.listingId);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(saleDividendAmount(out), 99372);
	EXPECT_EQ(getBalance(alice) - a, 847628);
	EXPECT_EQ(getBalance(bob) - b, -985300);
	EXPECT_EQ(getBalance(carol) - c, 134672);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, AcceptsSharesOfLaterContractAndZeroPerShare)
{
	collection();
	auto in = edition(2);
	ASSERT_GT(TESTEXA_CONTRACT_INDEX, QNFT_CONTRACT_INDEX);
	ASSERT_TRUE(setSaleDividend(in, shares(TESTEXA_CONTRACT_INDEX, {{carol, 676}}), 1000));
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list(100, 2);
	const auto a = getBalance(alice), c = getBalance(carol);
	auto out = buy(listing.listingId, 100);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(saleDividendAmount(out), 0);
	EXPECT_EQ(getBalance(alice) - a, 95);
	EXPECT_EQ(getBalance(carol) - c, 5);
	EXPECT_EQ(balance(0, nftId), 1);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, RejectsMissingSharesAndExcessiveCombinedRatesBeforeMint)
{
	collection();
	auto in = edition(1);
	ASSERT_TRUE(setSaleDividend(in, 12345, 1000));
	EXPECT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	ASSERT_TRUE(setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{carol, 676}}), 9471));
	EXPECT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	ASSERT_TRUE(setSaleDividend(in, 0, 1000));
	EXPECT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->itemCount, 0);
}

TEST_F(NftSaleDividendTest, RechecksRatesAtPurchaseWithoutDeliveringNft)
{
	collection();
	auto in = edition(1);
	ASSERT_TRUE(setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{carol, 676}}), 9470));
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->shopFeeBPS = 31;
	const auto b = getBalance(bob);
	EXPECT_EQ(buy(listing.listingId).code, QNFT::Code::Conflict);
	EXPECT_EQ(getBalance(bob), b);
	EXPECT_EQ(balance(0, nftId), 1);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, RejectsFullDebtMapBeforeDelivery)
{
	collection();
	auto in = edition(1);
	ASSERT_TRUE(setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{bob, 676}}), 1000));
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	for (uint32 i = 0; i < QNFT_CONTRACT_REFUND_SLOTS; ++i)
	{
		state->contractRefunds.set(id(100000 + i, 1, 0, 0), 1);
	}
	increaseEnergy(nftId, QNFT_CONTRACT_REFUND_SLOTS);
	const auto b = getBalance(bob);
	EXPECT_EQ(buy(listing.listingId).code, QNFT::Code::Capacity);
	EXPECT_EQ(getBalance(bob), b);
	EXPECT_EQ(balance(0, nftId), 1);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, CreditsSelfHeldSharesAsProtocolRevenue)
{
	collection();
	auto in = edition(1);
	ASSERT_TRUE(setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{nftId, 676}}), 1000));
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto fees = state->epochFees;
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	EXPECT_EQ(state->epochFees - fees, 102372);
	EXPECT_EQ(state->contractRefunds.population(), 0);
	expectQuCollateral();
}

namespace
{
	void moveDividendShares(uint64 name, const id& from, const id& to, sint64 count)
	{
		QPI::AssetPossessionIterator iter(QPI::Asset{NULL_ID, name}, QPI::AssetOwnershipSelect::any(), QPI::AssetPossessionSelect::byPossessor(from));
		ASSERT_FALSE(iter.reachedEnd());
		int destinationOwnership = 0, destinationPossession = 0;
		ASSERT_TRUE(transferShareOwnershipAndPossession(iter.ownershipIndex(), iter.possessionIndex(), to, count, &destinationOwnership,
		                                                &destinationPossession, true));
	}

	uint64 callbackDividendAsset = 0;
	id callbackDividendDestination;

	void moveSharesDuringDividend(const QpiContextProcedureCall&, void* state, void*, void*, void*)
	{
		moveDividendShares(callbackDividendAsset, id(TESTEXA_CONTRACT_INDEX, 0, 0, 0), callbackDividendDestination, 676);
		++static_cast<TESTEXA::StateData*>(state)->heavyComputationResult;
	}
} // namespace

TEST_F(NftSaleDividendTest, UsesCurrentHoldersForEachPartialPurchase)
{
	collection();
	auto in = edition(2);
	auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list(1000000, 2);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	moveDividendShares(name, carol, bob, 676);
	auto b = getBalance(bob), c = getBalance(carol);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(bob) - b, -900628);
	EXPECT_EQ(getBalance(carol) - c, 50000);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, FailedTransferRemainsOwedToOriginalHolderAfterSharesMove)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	collection();
	auto in = edition(1);
	auto name = shares(QX_CONTRACT_INDEX, {{recipient, 676}});
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	const auto previousError = contractError[TESTEXA_CONTRACT_INDEX];
	contractError[TESTEXA_CONTRACT_INDEX] = ContractErrorIPOFailed;
	auto out = buy(listing.listingId);
	contractError[TESTEXA_CONTRACT_INDEX] = previousError;
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.shareholderAmount, 99372);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	sint64 pending = 0;
	ASSERT_TRUE(state->contractRefunds.get(recipient, pending));
	EXPECT_EQ(pending, 99372);
	expectQuCollateral();
	moveDividendShares(name, recipient, bob, 676);
	const auto b = getBalance(bob), recipientBefore = getBalance(recipient);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient) - recipientBefore, 99372);
	EXPECT_EQ(getBalance(bob), b);
	EXPECT_FALSE(state->contractRefunds.contains(recipient));
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient) - recipientBefore, 99372);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, CallbackCannotRedirectCurrentSaleAndSharedRoyaltyIsPaidOnce)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	collection();
	auto in = edition(1);
	in.royaltyReceiver = recipient;
	callbackDividendAsset = shares(QX_CONTRACT_INDEX, {{recipient, 676}});
	callbackDividendDestination = bob;
	setSaleDividend(in, callbackDividendAsset, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][POST_INCOMING_TRANSFER] = moveSharesDuringDividend;
	const auto r = getBalance(recipient), b = getBalance(bob);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(recipient) - r, 149372);
	EXPECT_EQ(getBalance(bob) - b, -1000000);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 1);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(recipient) - r, 149372);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, PaysAll676HoldersAndPreservesExistingDebt)
{
	collection();
	std::vector<std::pair<m256i, unsigned int>> holders;
	for (uint32 i = 0; i < 676; ++i)
	{
		holders.emplace_back(id(200000 + i, 1, 0, 0), 1);
	}
	auto in = edition(1);
	setSaleDividend(in, shares(QX_CONTRACT_INDEX, holders), 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->contractRefunds.set(holders[0].first, 17);
	increaseEnergy(nftId, 17);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	for (const auto& holder : holders)
	{
		EXPECT_EQ(getBalance(holder.first), 147);
	}
	sint64 pending = 0;
	ASSERT_TRUE(state->contractRefunds.get(holders[0].first, pending));
	EXPECT_EQ(pending, 17);
	EXPECT_EQ(state->contractRefunds.population(), 1);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(getBalance(holders[0].first), 164);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, MintBatchAndMultiMintPersistTermsAndInvalidBatchIssuesNothing)
{
	collection();
	auto in = edition(1);
	auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	setSaleDividend(in, name, 1000);
	QNFT::MintBatch_input batch{};
	batch.count = 2;
	batch.entries.set(0, in);
	auto invalid = in;
	invalid.dividendBps = 10001;
	batch.entries.set(1, invalid);
	EXPECT_EQ(mintBatch(batch).code, QNFT::Code::InvalidInput);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	EXPECT_EQ(state->itemCount, 0);
	batch.entries.set(1, in);
	ASSERT_EQ(mintBatch(batch).code, QNFT::Code::Ok);
	QNFT::MultiMint_input multi{};
	multi.count = 2;
	multi.prototype = in;
	multi.prototype.quantity = 0;
	multi.quantities.set(0, 1);
	multi.quantities.set(1, 2);
	QNFT::MultiMint_output output{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 8, multi, output, alice, 2 * TEST_MINT_FEE));
	ASSERT_EQ(output.code, QNFT::Code::Ok);
	for (uint32 i = 0; i < 4; ++i)
	{
		QNFT::GetItem_output result{};
		callFunction(QNFT_CONTRACT_INDEX, 4, QNFT::GetItem_input{i}, result);
		ASSERT_EQ(result.code, QNFT::Code::Ok);
		EXPECT_EQ(result.item.dividendAssetName, name);
		EXPECT_EQ(result.item.dividendBps, 1000);
	}
}

TEST_F(NftSaleDividendTest, RejectsUserIssuedTokenEvenWith676Units)
{
	collection();
	char nameText[8] = "USER";
	char units[8] = {};
	int issuance = 0, owner = 0, possession = 0;
	ASSERT_EQ(issueAsset(alice, nameText, 0, units, 676, QX_CONTRACT_INDEX, &issuance, &owner, &possession), 676);
	auto in = edition(1);
	setSaleDividend(in, *reinterpret_cast<uint64*>(nameText), 1000);
	EXPECT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
}

TEST_F(NftSaleDividendTest, AggregatesSplitManagementRecordsIntoOnePayment)
{
	INIT_CONTRACT(TESTEXA);
	const id recipient(TESTEXA_CONTRACT_INDEX, 0, 0, 0);
	collection();
	auto name = shares(QX_CONTRACT_INDEX, {{recipient, 676}});
	QPI::AssetPossessionIterator iter(QPI::Asset{NULL_ID, name}, QPI::AssetOwnershipSelect::any(),
	                                  QPI::AssetPossessionSelect::byPossessor(recipient));
	ASSERT_FALSE(iter.reachedEnd());
	ASSERT_TRUE(transferShareManagementRights(iter.ownershipIndex(), iter.possessionIndex(), QNFT_CONTRACT_INDEX, QNFT_CONTRACT_INDEX, 338, nullptr,
	                                          nullptr, true));
	auto in = edition(1);
	in.royaltyReceiver = recipient;
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	contractSystemProcedures[TESTEXA_CONTRACT_INDEX][POST_INCOMING_TRANSFER] = countRevenueCallback;
	const auto before = getBalance(recipient);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(recipient) - before, 149372);
	EXPECT_EQ(reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->heavyComputationResult, 1);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, UsesPossessorRatherThanOwnershipIdentity)
{
	collection();
	auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	QPI::AssetPossessionIterator iter(QPI::Asset{NULL_ID, name}, QPI::AssetOwnershipSelect::any(), QPI::AssetPossessionSelect::byPossessor(carol));
	ASSERT_FALSE(iter.reachedEnd());
	// Model a custodial ledger record: the ownership identity differs from its possession identity.
	assets[iter.ownershipIndex()].varStruct.ownership.publicKey = alice;
	auto in = edition(1);
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list();
	const auto c = getBalance(carol), a = getBalance(alice);
	ASSERT_EQ(buy(listing.listingId).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(carol) - c, 149372);
	EXPECT_EQ(getBalance(alice) - a, 847628);
	expectQuCollateral();
}

TEST_F(NftSaleDividendTest, MetadataChangesPreserveTermsAndSaleJournalReportsAllocation)
{
	collection();
	auto in = edition(1);
	in.policy = QNFT::Policy::Mutable;
	auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	ASSERT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://changed", 123), 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().dividendAssetName, name);
	EXPECT_EQ(item().dividendBps, 1000);
	auto listing = list();
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listing.listingId, 1, 1000000, 2}, bought, bob, 1000000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	QNFT::GetEvents_output events{};
	callFunction(QNFT_CONTRACT_INDEX, 35, QNFT::GetEvents_input{0, 16}, events);
	ASSERT_EQ(events.code, QNFT::Code::Ok);
	unsigned found = 0;
	for (uint32 i = 0; i < events.count; ++i)
	{
		const auto& event = events.events.get(i);
		if (event.kind == QNFT::EventKind::Market && event.action == static_cast<uint32>(QNFT::MarketAction::SaleDividend))
		{
			++found;
			EXPECT_EQ(event.reference, listing.listingId);
			EXPECT_EQ(event.quantity, name);
			EXPECT_EQ(event.version, 1000);
			EXPECT_EQ(event.amount, 99372);
		}
	}
	EXPECT_EQ(found, 1);
}

TEST_F(NftSaleDividendTest, FullAllocationAtMaximumPriceConservesPayment)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->shopFeeBPS = 0;
	auto in = edition(1);
	in.royaltyBps = 0;
	setSaleDividend(in, shares(QX_CONTRACT_INDEX, {{carol, 676}}), 10000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto listing = list(MAX_AMOUNT);
	increaseEnergy(bob, MAX_AMOUNT - getBalance(bob));
	const auto a = getBalance(alice), c = getBalance(carol);
	auto bought = buy(listing.listingId, MAX_AMOUNT);
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.shareholderAmount, 999999999999728LL);
	EXPECT_EQ(getBalance(alice) - a, 272);
	EXPECT_EQ(getBalance(carol) - c, 999999999999728LL);
	EXPECT_EQ(getBalance(bob), 0);
	expectQuCollateral();
}

namespace
{
	constexpr unsigned short DIVIDEND_INFO_FUNCTION = 12;

	struct DividendQuoteWire
	{
		uint64 dividendAssetName;
		sint64 shareholderAmount;
		sint64 amountPerShare;
		QNFT::Code code;
		uint16 dividendBps;
	};
} // namespace

class NftDividendInfoTest : public NftSaleDividendTest
{
public:
	DividendQuoteWire quote(uint32 itemId, sint64 price)
	{
		DividendQuoteWire out{};
		callFunction(QNFT_CONTRACT_INDEX, DIVIDEND_INFO_FUNCTION, QNFT::RoyaltyInfo_input{price, itemId}, out);
		return out;
	}
};

TEST_F(NftDividendInfoTest, QuoteMatchesPurchaseAndDoesNotTransferFunds)
{
	ASSERT_EQ(contractUserFunctionOutputSizes[QNFT_CONTRACT_INDEX][DIVIDEND_INFO_FUNCTION], sizeof(DividendQuoteWire));
	collection();
	auto in = edition(1);
	const auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	const auto before = getBalance(carol), contractBefore = getBalance(nftId);
	auto out = quote(0, 1000000);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.dividendAssetName, name);
	EXPECT_EQ(out.dividendBps, 1000);
	EXPECT_EQ(out.amountPerShare, 147);
	EXPECT_EQ(out.shareholderAmount, 99372);
	EXPECT_EQ(getBalance(carol), before);
	EXPECT_EQ(getBalance(nftId), contractBefore);
	auto listing = list();
	auto bought = buy(listing.listingId);
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	EXPECT_EQ(bought.shareholderAmount, out.shareholderAmount);
}

TEST_F(NftDividendInfoTest, QuotePreservesTermsWhenRoundingToZeroAndIgnoresShopFee)
{
	ASSERT_EQ(contractUserFunctionOutputSizes[QNFT_CONTRACT_INDEX][DIVIDEND_INFO_FUNCTION], sizeof(DividendQuoteWire));
	collection();
	auto in = edition(1);
	const auto name = shares(QX_CONTRACT_INDEX, {{carol, 676}});
	setSaleDividend(in, name, 1000);
	ASSERT_EQ(action(6, in, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->shopFeeBPS = 10000;
	for (sint64 price : {0LL, 1LL, 6759LL, 6760LL})
	{
		auto out = quote(0, price);
		ASSERT_EQ(out.code, QNFT::Code::Ok);
		EXPECT_EQ(out.dividendAssetName, name);
		EXPECT_EQ(out.dividendBps, 1000);
		EXPECT_EQ(out.amountPerShare, price == 6760 ? 1 : 0);
		EXPECT_EQ(out.shareholderAmount, price == 6760 ? 676 : 0);
	}
	auto out = quote(0, 9223372036854775807LL);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_LE(out.shareholderAmount, 922337203685477580LL);
	EXPECT_GT(out.shareholderAmount, 922337203685476904LL);
	EXPECT_EQ(out.shareholderAmount % 676, 0);
}

TEST_F(NftDividendInfoTest, QuoteHandlesDisabledMissingBurnedAndInvalidInputs)
{
	ASSERT_EQ(contractUserFunctionOutputSizes[QNFT_CONTRACT_INDEX][DIVIDEND_INFO_FUNCTION], sizeof(DividendQuoteWire));
	collection();
	ASSERT_EQ(action(6, edition(1), alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	auto out = quote(0, 1000000);
	ASSERT_EQ(out.code, QNFT::Code::Ok);
	EXPECT_EQ(out.dividendAssetName, 0);
	EXPECT_EQ(out.dividendBps, 0);
	EXPECT_EQ(out.shareholderAmount, 0);
	EXPECT_EQ(out.amountPerShare, 0);
	EXPECT_EQ(quote(0, -1).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(quote(999, 1).code, QNFT::Code::NotFound);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	auto record = state->items.get(0);
	record.dividendBps = 10001;
	record.dividendAssetName = 1;
	state->items.set(0, record);
	EXPECT_EQ(quote(0, 1000).code, QNFT::Code::Conflict);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(quote(0, 1000).code, QNFT::Code::NotFound);
}

namespace
{
	struct MintQuotaAccess : QNFT
	{
		using QNFT::CheckMintCollectionQuota;
		using QNFT::remainingCollectionItems;
		using QNFT::remainingCollectionUnits;
	};
} // namespace

TEST(NftMintQuotaTest, PrefixCheckRejectsOverflowAndCountsOnlyMatchingCollections)
{
	QNFT::MintMany_input batch{};
	QNFT::MintCollectionQuotaScratch scratch{};
	QNFT::Collection collection{};
	collection.maxItems = 2;
	QNFT::Mint_input entry{};
	entry.quantity = MAX_AMOUNT;
	batch.entries.set(0, entry);
	entry.collectionId = 1;
	batch.entries.set(1, entry);
	EXPECT_TRUE(MintQuotaAccess::CheckMintCollectionQuota(batch, 1, collection, scratch));
	entry.collectionId = 0;
	entry.quantity = 1;
	batch.entries.set(2, entry);
	EXPECT_FALSE(MintQuotaAccess::CheckMintCollectionQuota(batch, 2, collection, scratch));
	entry.quantity = 2;
	batch.entries.set(0, entry);
	EXPECT_TRUE(MintQuotaAccess::CheckMintCollectionQuota(batch, 2, collection, scratch));
	collection.maxUnits = 2;
	EXPECT_FALSE(MintQuotaAccess::CheckMintCollectionQuota(batch, 2, collection, scratch));
	collection.maxUnits = 0;
	collection.maxItems = 1;
	EXPECT_FALSE(MintQuotaAccess::CheckMintCollectionQuota(batch, 2, collection, scratch));
}

namespace
{
#if defined(_MSC_VER)
#define QNFT_BENCH_NOINLINE __declspec(noinline)
#else
#define QNFT_BENCH_NOINLINE __attribute__((noinline))
#endif
	QNFT_BENCH_NOINLINE uint32 originalMintQuotaScan(const QNFT::MintMany_input& batch, const QNFT::Collection& collection)
	{
		for (uint32 i = 0; i < batch.count; ++i)
		{
			sint64 units = batch.entries.get(i).quantity;
			uint32 items = 1;
			for (uint32 j = 0; j < i; ++j)
			{
				if (batch.entries.get(j).collectionId != batch.entries.get(i).collectionId)
				{
					continue;
				}
				++items;
				if (batch.entries.get(j).quantity > MAX_AMOUNT - units)
				{
					return i;
				}
				units += batch.entries.get(j).quantity;
			}
			if (items > MintQuotaAccess::remainingCollectionItems(collection) || units > MintQuotaAccess::remainingCollectionUnits(collection))
			{
				return i;
			}
		}
		return batch.count;
	}

	QNFT_BENCH_NOINLINE uint32 extractedMintQuotaScan(const QNFT::MintMany_input& batch, const QNFT::Collection& collection)
	{
		QNFT::MintCollectionQuotaScratch scratch{};
		for (uint32 i = 0; i < batch.count; ++i)
		{
			if (!MintQuotaAccess::CheckMintCollectionQuota(batch, i, collection, scratch))
			{
				return i;
			}
		}
		return batch.count;
	}

#undef QNFT_BENCH_NOINLINE
} // namespace

TEST(NftMintQuotaTest, ExtractedCheckMatchesOriginalForMixedPrefixes)
{
	QNFT::MintMany_input batch{};
	QNFT::Collection collection{};
	for (uint32 count = 1; count <= QNFT_TOKEN_BATCH_CAPACITY; ++count)
	{
		batch.count = count;
		for (uint32 i = 0; i < count; ++i)
		{
			QNFT::Mint_input entry{};
			entry.collectionId = (i % 3) * 32;
			entry.quantity = i + 1;
			batch.entries.set(i, entry);
		}
		for (uint32 cap = 0; cap <= 32; ++cap)
		{
			collection.maxItems = cap;
			collection.maxUnits = cap * 2;
			EXPECT_EQ(extractedMintQuotaScan(batch, collection), originalMintQuotaScan(batch, collection));
		}
	}
}

// Opt-in microbenchmark comparing the original inline loop with the extracted helper; excludes issuance.
TEST(NftMintQuotaTest, DISABLED_ExtractedCheckBenchmark)
{
	constexpr unsigned iterations = 100000;
	QNFT::MintMany_input batch{};
	QNFT::Collection collection{};
	volatile uint64 checksum = 0;
	for (uint32 count : {1U, 2U, 8U, 16U})
	{
		batch.count = count;
		for (uint32 distribution = 0; distribution < 4; ++distribution)
		{
			for (uint32 i = 0; i < count; ++i)
			{
				QNFT::Mint_input entry{};
				entry.collectionId = distribution == 0 ? 0 : distribution == 1 ? i % 4 : distribution == 2 ? i : i * 33;
				entry.quantity = i + 1;
				batch.entries.set(i, entry);
			}
			double best[2] = {1e30, 1e30};
			for (unsigned repeat = 0; repeat < 5; ++repeat)
			{
				for (unsigned order = 0; order < 2; ++order)
				{
					const unsigned method = (order + repeat) % 2;
					auto scan = method ? extractedMintQuotaScan : originalMintQuotaScan;
					const auto start = std::chrono::steady_clock::now();
					for (unsigned j = 0; j < iterations; ++j)
					{
						auto first = batch.entries.get(0);
						first.quantity = 1 + (j & 1);
						batch.entries.set(0, first);
						checksum = checksum + scan(batch, collection);
					}
					const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / iterations;
					if (ns < best[method])
					{
						best[method] = ns;
					}
				}
			}
			std::cout << "QUOTA_BENCH count=" << count << " distribution=" << distribution << " original_ns=" << best[0] << " helper_ns=" << best[1]
			          << " ratio=" << best[1] / best[0] << '\n';
		}
	}
	EXPECT_GT(checksum, 0U);
}

TEST_F(NftTest, MetadataEntryPointsMatchWireTypes)
{
	EXPECT_EQ(contractUserProcedureInputSizes[QNFT_CONTRACT_INDEX][9], sizeof(QNFT::UpdateMetadata_input));
	EXPECT_EQ(contractUserProcedureInputSizes[QNFT_CONTRACT_INDEX][10], sizeof(QNFT::FreezeMetadata_input));
	EXPECT_EQ(static_cast<uint8>(QNFT::Policy::Mutable), 1);
}

// These cases exercise public requests against real asset and payment state.
class NftCoverageTest : public Nft1155ExtrasTest
{
};

class NftRentalCoverageTest : public NftRentalTest
{
};

TEST_F(NftCoverageTest, ReplacedNomineeCannotAcceptAdministration)
{
	collection();
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
	const auto funds = getBalance(bob);
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob, 17).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(getBalance(bob), funds);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
	QNFT::GetCollection_output result{};
	callFunction(QNFT_CONTRACT_INDEX, 1, QNFT::GetCollection_input{0}, result);
	EXPECT_EQ(result.collection.creator, alice);
	EXPECT_EQ(result.collection.administrator, carol);
	EXPECT_EQ(result.collection.nominatedAdministrator, NULL_ID);
	EXPECT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol, 17).code, QNFT::Code::Unauthorized);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, NomineeHasNoManagementRightsUntilAcceptance)
{
	collection();
	ASSERT_EQ(mint(1, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(6, edition(1), bob, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://nominee"), 0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(action(4, QNFT::SetMintingStatus_input{0, QNFT::MintingStatus::Closed}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(owned(0, alice), 1);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, InvalidMintRecipientsDoNotIssueOrCharge)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto funds = getBalance(alice), fees = state->epochFees;
	const auto events = state->lastEventId;
	for (const auto receiver : {NULL_ID, nftId, id(TESTEXA_CONTRACT_INDEX, 0, 0, 0)})
	{
		EXPECT_EQ(action(6, edition(1, receiver), alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(state->collections.get(0).issuedItems, 0U);
		EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
		EXPECT_EQ(state->epochFees, fees);
		EXPECT_EQ(state->lastEventId, events);
		EXPECT_EQ(getBalance(alice), funds);
	}
	expectQuCollateral();
}

TEST_F(NftCoverageTest, MalformedMetadataCannotCreateAnEdition)
{
	collection();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto funds = getBalance(alice);
	for (unsigned variant = 0; variant < 6; ++variant)
	{
		SCOPED_TRACE(variant);
		auto request = edition(1);
		if (variant == 0)
		{
			request.metadata.contentHash = NULL_ID;
		}
		if (variant == 1)
		{
			request.metadata.uri = {};
		}
		if (variant == 2)
		{
			request.metadata.uri.set(1, 31);
		}
		if (variant == 3)
		{
			request.metadata.uri.set(1, 127);
		}
		if (variant == 4)
		{
			request.metadata.uri.set(255, 'X');
		}
		if (variant == 5)
		{
			for (unsigned i = 0; i < 256; ++i)
			{
				request.metadata.uri.set(i, 'X');
			}
		}
		EXPECT_EQ(action(6, request, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
		EXPECT_EQ(state->itemCount, 0U);
		EXPECT_EQ(getBalance(alice), funds);
	}
	expectQuCollateral();
}

TEST_F(NftCoverageTest, LongestTerminatedMetadataUriCanBeMintedAndRead)
{
	collection();
	auto request = edition(1);
	request.metadata.uri = {};
	for (unsigned i = 0; i < 255; ++i)
	{
		request.metadata.uri.set(i, 'x');
	}
	ASSERT_EQ(action(6, request, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
	const auto stored = item();
	for (unsigned i = 0; i < 255; ++i)
	{
		EXPECT_EQ(stored.metadata.uri.get(i), 'x');
	}
	EXPECT_EQ(stored.metadata.uri.get(255), 0);
	EXPECT_EQ(stored.metadata.contentHash, request.metadata.contentHash);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, InvalidMintPolicyAndQuantitiesLeaveQuotaUntouched)
{
	collection();
	const auto funds = getBalance(alice);
	for (sint64 quantity : {-1LL, 0LL, MAX_AMOUNT + 1})
	{
		EXPECT_EQ(action(6, edition(quantity), alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	}
	auto request = edition(1);
	request.policy = static_cast<QNFT::Policy>(2);
	EXPECT_EQ(action(6, request, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(mintStatus().remainingItems, 10U);
	EXPECT_EQ(mintStatus().remainingUnits, 1000);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, FrozenMetadataRejectsFurtherUpdatesWithoutCharging)
{
	collection();
	ASSERT_EQ(mint(1, QNFT::Policy::Mutable).code, QNFT::Code::Ok);
	ASSERT_EQ(action(10, QNFT::FreezeMetadata_input{0}).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->updateMetadataFee = 7;
	const auto previous = item();
	const auto funds = getBalance(alice), fees = state->epochFees;
	const auto events = state->lastEventId;
	EXPECT_EQ(action(9, QNFT::UpdateMetadata_input{metadata("ipfs://changed", 2), 0}, alice, 17).code, QNFT::Code::Frozen);
	EXPECT_EQ(item().metadata.contentHash, previous.metadata.contentHash);
	EXPECT_EQ(item().metadataVersion, previous.metadataVersion);
	EXPECT_EQ(state->lastEventId, events);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(alice), funds);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, TransferRejectsZeroSelfAndLowerContractRecipients)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	const auto funds = getBalance(alice);
	for (const auto receiver : {NULL_ID, nftId, id(QX_CONTRACT_INDEX, 0, 0, 0)})
	{
		EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, receiver, 1, 0}, alice, 17).code, QNFT::Code::InvalidInput);
		EXPECT_EQ(owned(0, alice), 10);
		EXPECT_EQ(getBalance(alice), funds);
	}
	expectQuCollateral();
}

TEST_F(NftCoverageTest, DuplicateBatchOverflowRejectsBeforeMovingAnyUnits)
{
	collection(0, 0);
	ASSERT_EQ(mint(MAX_AMOUNT).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input request{};
	request.owner = alice;
	request.receiver = bob;
	request.count = 2;
	request.entries.set(0, QNFT::TokenAmount{MAX_AMOUNT, 0});
	request.entries.set(1, QNFT::TokenAmount{1, 0});
	const auto funds = getBalance(alice);
	const auto result = transferBatch(request, alice, 17);
	EXPECT_EQ(result.code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(result.failureIndex, 1U);
	EXPECT_EQ(result.attempted, 0U);
	EXPECT_EQ(result.completed, 0U);
	EXPECT_EQ(result.fee, 0);
	EXPECT_EQ(owned(0, alice), MAX_AMOUNT);
	EXPECT_EQ(owned(0, bob), 0);
	EXPECT_EQ(getBalance(alice), funds);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, SelfBatchDoesNotConsumeRepeatedHoldings)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input request{};
	request.owner = alice;
	request.receiver = alice;
	request.count = 3;
	for (unsigned i = 0; i < 3; ++i)
	{
		request.entries.set(i, QNFT::TokenAmount{10, 0});
	}
	const auto result = transferBatch(request);
	EXPECT_EQ(result.code, QNFT::Code::Ok);
	EXPECT_EQ(result.completed, 3U);
	EXPECT_EQ(owned(0, alice), 10);
	EXPECT_EQ(item().burned, 0);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, InvalidLaterBurnEntryCannotDestroyEarlierUnits)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	QNFT::BurnBatch_input request{};
	request.owner = alice;
	request.count = 2;
	request.entries.set(0, QNFT::TokenAmount{3, 0});
	request.entries.set(1, QNFT::TokenAmount{1, 999});
	const auto result = burnBatch(request);
	EXPECT_EQ(result.code, QNFT::Code::NotFound);
	EXPECT_EQ(result.failureIndex, 1U);
	EXPECT_EQ(result.attempted, 0U);
	EXPECT_EQ(result.completed, 0U);
	EXPECT_EQ(owned(0, alice), 10);
	EXPECT_EQ(item().burned, 0);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, AuthorCannotBurnCopiesSoldToAnotherOwner)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
	ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{1000, 1, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	QNFT::Buy_output bought{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 1000, 1}, bought, bob, 1000));
	ASSERT_EQ(bought.code, QNFT::Code::Ok);
	for (const auto caller : {alice, carol})
	{
		EXPECT_EQ(action(14, QNFT::Burn_input{bob, 1, 0}, caller, 17).code, QNFT::Code::Unauthorized);
	}
	EXPECT_EQ(owned(0, bob), 1);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, bob), 1);
	ASSERT_EQ(action(14, QNFT::Burn_input{bob, 1, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(owned(0, bob), 0);
	EXPECT_EQ(item().burned, 2);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, LastCopyBurnPreservesMetadataAndExhaustedMintQuota)
{
	collection(1, 1);
	ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
	const auto previous = item();
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
	EXPECT_FALSE(exists(0));
	QNFT::TotalSupply_output supply{};
	callFunction(QNFT_CONTRACT_INDEX, 7, QNFT::TotalSupply_input{0}, supply);
	EXPECT_EQ(supply.code, QNFT::Code::Ok);
	EXPECT_EQ(supply.supply, 0);
	EXPECT_EQ(item().metadata.contentHash, previous.metadata.contentHash);
	EXPECT_EQ(item().creator, alice);
	EXPECT_EQ(mint(1).code, QNFT::Code::Capacity);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, SellerCannotBurnEscrowBeforeCancellingListing)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	QNFT::ListForSale_output listed{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 2, 0}, listed, alice, 0));
	ASSERT_EQ(listed.code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(item().burned, 0);
	QNFT::CancelSale_output cancelled{};
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listed.listingId}, cancelled, alice, 0));
	ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 2, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(item().burned, 2);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, ApprovalRejectsInvalidOperatorsWithoutCharging)
{
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->approveFee = 7;
	const auto funds = getBalance(alice), fees = state->epochFees;
	for (const auto who : {NULL_ID, alice})
	{
		EXPECT_EQ(action(11, QNFT::SetApprovalForAll_input{who, true}, alice, 17).code, QNFT::Code::InvalidInput);
		EXPECT_EQ(state->operatorApprovals.population(), 0U);
		EXPECT_EQ(state->epochFees, fees);
		EXPECT_EQ(getBalance(alice), funds);
	}
	expectQuCollateral();
}

TEST_F(NftCoverageTest, OperatorPermissionIncludesFutureItemsButNeverOtherOwners)
{
	ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{bob, true}).code, QNFT::Code::Ok);
	collection();
	ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 1, 0}, bob).code, QNFT::Code::Ok);
	EXPECT_EQ(action(12, QNFT::TransferFrom_input{carol, bob, 1, 0}, bob).code, QNFT::Code::Unauthorized);
	EXPECT_EQ(owned(0, alice), 2);
	EXPECT_EQ(owned(0, carol), 1);
	EXPECT_EQ(owned(0, bob), 0);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, OnlyRenterMayChangeTheirDeposit)
{
	const auto key = open();
	const auto deposit = rental(key).rental.deposit;
	for (const auto caller : {alice, carol})
	{
		const auto funds = getBalance(caller);
		EXPECT_EQ(change(23, QNFT::TopUpRental_input{key}, caller, 100).code, QNFT::Code::Unauthorized);
		EXPECT_EQ(change(24, QNFT::WithdrawRentalDeposit_input{key, 100}, caller, 17).code, QNFT::Code::Unauthorized);
		EXPECT_EQ(getBalance(caller), funds);
		EXPECT_EQ(rental(key).rental.deposit, deposit);
	}
	EXPECT_EQ(change(25, QNFT::CancelRentalRenewal_input{key}, carol, 17).code, QNFT::Code::Unauthorized);
	EXPECT_TRUE(rental(key).rental.autoRenew);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, InvalidDepositAmountsCannotChangeCollateral)
{
	const auto key = open();
	const auto funds = getBalance(bob);
	EXPECT_EQ(change(23, QNFT::TopUpRental_input{key}, bob, 0).code, QNFT::Code::InvalidInput);
	for (sint64 amount : {-1LL, 0LL})
	{
		EXPECT_EQ(change(24, QNFT::WithdrawRentalDeposit_input{key, amount}, bob, 17).code, QNFT::Code::InvalidInput);
	}
	EXPECT_EQ(change(24, QNFT::WithdrawRentalDeposit_input{key, 3001}, bob, 17).code, QNFT::Code::InsufficientBalance);
	EXPECT_EQ(rental(key).rental.deposit, 3000);
	EXPECT_EQ(getBalance(bob), funds);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, UnusedDepositCanBeWithdrawnAfterPaidAccessEnds)
{
	const auto key = open();
	ASSERT_EQ(change(25, QNFT::CancelRentalRenewal_input{key}).code, QNFT::Code::Ok);
	advance(15);
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Ended);
	const auto funds = getBalance(bob);
	ASSERT_EQ(change(24, QNFT::WithdrawRentalDeposit_input{key, 3000}, bob, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(getBalance(bob), funds + 3000);
	EXPECT_EQ(rental(key).code, QNFT::Code::NotFound);
	EXPECT_EQ(change(24, QNFT::WithdrawRentalDeposit_input{key, 3000}).code, QNFT::Code::NotFound);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, CancellingRenewalTwiceDoesNotEmitOrChargeTwice)
{
	const auto key = open();
	ASSERT_EQ(change(25, QNFT::CancelRentalRenewal_input{key}).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto events = state->lastEventId;
	const auto funds = getBalance(alice);
	EXPECT_EQ(change(25, QNFT::CancelRentalRenewal_input{key}, alice, 17).code, QNFT::Code::Ok);
	EXPECT_EQ(state->lastEventId, events);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(rental(key).rental.deposit, 3000);
	EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Active);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, RentalIdExhaustionPreservesOfferAndRefunds)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->lastRentalId = 0x7fffffffffffffffULL;
	const auto funds = getBalance(bob), fees = state->epochFees;
	EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, bob, 1500).code, QNFT::Code::Capacity);
	QNFT::GetRentalOffer_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 23, QNFT::GetRentalOffer_input{offer.offerId}, remaining);
	EXPECT_EQ(remaining.code, QNFT::Code::Ok);
	EXPECT_EQ(remaining.offer.quantity, 2);
	EXPECT_EQ(state->rentals.population(), 0U);
	EXPECT_EQ(state->rentalDeposits, 0);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(bob), funds);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, OnlyLenderMayCancelUnpaidOffer)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	EXPECT_EQ(change(21, QNFT::CancelRentalOffer_input{offer.offerId}, bob, 17).code, QNFT::Code::Unauthorized);
	QNFT::GetRentalOffer_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 23, QNFT::GetRentalOffer_input{offer.offerId}, remaining);
	EXPECT_EQ(remaining.code, QNFT::Code::Ok);
	EXPECT_EQ(remaining.offer.quantity, 2);
	ASSERT_EQ(change(21, QNFT::CancelRentalOffer_input{offer.offerId}, alice).code, QNFT::Code::Ok);
	EXPECT_EQ(action(14, QNFT::Burn_input{alice, 2, 0}).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, OfferPagesFilterOwnersWithoutSkippingOrRepeatingEntries)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 4, 0}).code, QNFT::Code::Ok);
	const auto first = change(20, QNFT::CreateRentalOffer_input{100, 1, 0, 15}, alice);
	const auto second = change(20, QNFT::CreateRentalOffer_input{200, 2, 0, 15}, alice);
	const auto other = change(20, QNFT::CreateRentalOffer_input{300, 3, 0, 15}, bob);
	ASSERT_EQ(first.code, QNFT::Code::Ok);
	ASSERT_EQ(second.code, QNFT::Code::Ok);
	ASSERT_EQ(other.code, QNFT::Code::Ok);
	for (const auto owner : {alice, bob, carol, NULL_ID})
	{
		std::set<uint64> seen;
		uint32 cursor = 0;
		for (;;)
		{
			QNFT::ListRentalOffers_output page{};
			callFunction(QNFT_CONTRACT_INDEX, 24, QNFT::ListRentalOffers_input{owner, cursor, 1, system.tick}, page);
			ASSERT_EQ(page.code, QNFT::Code::Ok);
			ASSERT_LE(page.count, 1U);
			EXPECT_EQ(page.tick, system.tick);
			for (uint32 i = 0; i < page.count; ++i)
			{
				const auto entry = page.offers.get(i);
				EXPECT_TRUE(seen.insert(entry.offerId).second);
				if (owner != NULL_ID)
				{
					EXPECT_EQ(entry.owner, owner);
				}
			}
			if (page.done)
			{
				break;
			}
			ASSERT_GT(page.nextCursor, cursor);
			cursor = page.nextCursor;
		}
		std::set<uint64> expected;
		if (owner == alice || owner == NULL_ID)
		{
			expected.insert(first.offerId);
			expected.insert(second.offerId);
		}
		if (owner == bob || owner == NULL_ID)
		{
			expected.insert(other.offerId);
		}
		EXPECT_EQ(seen, expected);
	}
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, RentalPagesRespectLenderRenterAndBothRoles)
{
	const auto borrowed = open();
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 2, 0}).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 1, 0, 15}, bob);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	const auto lent = change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, carol, 3000);
	ASSERT_EQ(lent.code, QNFT::Code::Ok);
	const auto* state = reinterpret_cast<const QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto deposits = state->rentalDeposits, fees = state->epochFees;
	for (const auto role : {QNFT::RentalRole::Both, QNFT::RentalRole::Lender, QNFT::RentalRole::Renter})
	{
		std::set<uint64> seen;
		uint32 cursor = 0;
		for (;;)
		{
			QNFT::ListUserRentals_output page{};
			callFunction(QNFT_CONTRACT_INDEX, 26, QNFT::ListUserRentals_input{bob, cursor, 1, system.tick, role}, page);
			ASSERT_EQ(page.code, QNFT::Code::Ok);
			ASSERT_LE(page.count, 1U);
			for (uint32 i = 0; i < page.count; ++i)
			{
				const auto entry = page.rentals.get(i);
				EXPECT_TRUE(seen.insert(entry.rentalId).second);
				if (role == QNFT::RentalRole::Lender)
				{
					EXPECT_EQ(entry.owner, bob);
				}
				if (role == QNFT::RentalRole::Renter)
				{
					EXPECT_EQ(entry.renter, bob);
				}
			}
			if (page.done)
			{
				break;
			}
			ASSERT_GT(page.nextCursor, cursor);
			cursor = page.nextCursor;
		}
		std::set<uint64> expected;
		if (role != QNFT::RentalRole::Lender)
		{
			expected.insert(borrowed);
		}
		if (role != QNFT::RentalRole::Renter)
		{
			expected.insert(lent.rentalId);
		}
		EXPECT_EQ(seen, expected);
	}
	EXPECT_EQ(state->rentalDeposits, deposits);
	EXPECT_EQ(state->epochFees, fees);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, RentalQueriesRejectInvalidAndStalePages)
{
	open();
	QNFT::ListRentalOffers_input offers{alice, 0, 1, system.tick + 1};
	QNFT::ListRentalOffers_output offerPage{};
	callFunction(QNFT_CONTRACT_INDEX, 24, offers, offerPage);
	EXPECT_EQ(offerPage.code, QNFT::Code::Conflict);
	EXPECT_EQ(offerPage.count, 0U);
	offers.tick = 0;
	for (uint32 limit : {0U, 17U})
	{
		offers.limit = limit;
		callFunction(QNFT_CONTRACT_INDEX, 24, offers, offerPage);
		EXPECT_EQ(offerPage.code, QNFT::Code::InvalidInput);
	}
	QNFT::ListUserRentals_input query{bob, 0, 1, system.tick + 1, QNFT::RentalRole::Both};
	QNFT::ListUserRentals_output page{};
	callFunction(QNFT_CONTRACT_INDEX, 26, query, page);
	EXPECT_EQ(page.code, QNFT::Code::Conflict);
	EXPECT_EQ(page.count, 0U);
	query.tick = 0;
	query.role = static_cast<QNFT::RentalRole>(3);
	callFunction(QNFT_CONTRACT_INDEX, 26, query, page);
	EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
	query.role = QNFT::RentalRole::Both;
	query.user = NULL_ID;
	callFunction(QNFT_CONTRACT_INDEX, 26, query, page);
	EXPECT_EQ(page.code, QNFT::Code::InvalidInput);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, ExpiredRentalReadsProjectRightsWithoutChargingDeposit)
{
	const auto key = open();
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	const auto fees = state->epochFees, deposits = state->rentalDeposits;
	const auto funds = getBalance(alice);
	advance(15);
	for (unsigned i = 0; i < 3; ++i)
	{
		const auto projected = rental(key);
		ASSERT_EQ(projected.code, QNFT::Code::Ok);
		EXPECT_EQ(projected.rental.status, QNFT::RentalStatus::Active);
		EXPECT_EQ(projected.rental.deposit, 0);
		EXPECT_EQ(state->rentalDeposits, deposits);
		EXPECT_EQ(state->epochFees, fees);
		EXPECT_EQ(getBalance(alice), funds);
	}
	callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
	EXPECT_EQ(state->rentalDeposits, 0);
	EXPECT_EQ(state->epochFees, fees + 210);
	EXPECT_EQ(getBalance(alice), funds + 2790);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, MixedPublicOperationsMatchIndependentBalanceModel)
{
	collection(0, 0);
	for (unsigned token = 0; token < 3; ++token)
	{
		ASSERT_EQ(mint(40).code, QNFT::Code::Ok);
	}
	const id users[] = {alice, bob, carol};
	sint64 holdings[3][3] = {{40, 0, 0}, {40, 0, 0}, {40, 0, 0}};
	sint64 burned[3] = {};
	bool approved[3][3] = {};
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->transferFee = 7;
	state->approveFee = 3;
	sint64 fees = state->epochFees;
	sint64 funds[] = {getBalance(alice), getBalance(bob), getBalance(carol)};
	uint32 seed = 20261029;
	auto next = [&]()
	{
		seed = seed * 1664525U + 1013904223U;
		return seed >> 8;
	};
	for (unsigned step = 0; step < 120; ++step)
	{
		SCOPED_TRACE(step);
		const auto token = next() % 3, owner = next() % 3, caller = next() % 3, receiver = next() % 3;
		const sint64 quantity = next() % 9;
		const auto operation = next() % 3;
		if (operation == 0 && owner != caller)
		{
			const bool grant = (next() & 1) != 0;
			ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{users[caller], grant}, users[owner], 17).code, QNFT::Code::Ok);
			approved[owner][caller] = grant;
			if (grant)
			{
				fees += 3;
				funds[owner] -= 3;
			}
		}
		else
		{
			const bool burn = operation == 1 && burned[token] + quantity < 40;
			QNFT::Code expected = QNFT::Code::Ok;
			if (burn && quantity == 0)
			{
				expected = QNFT::Code::InvalidInput;
			}
			else if (owner != caller && !approved[owner][caller])
			{
				expected = QNFT::Code::Unauthorized;
			}
			else if (quantity > holdings[token][owner])
			{
				expected = QNFT::Code::InsufficientBalance;
			}
			const auto result = burn ? action(14, QNFT::Burn_input{users[owner], quantity, token}, users[caller], 17)
			                         : action(12, QNFT::TransferFrom_input{users[owner], users[receiver], quantity, token}, users[caller], 17);
			ASSERT_EQ(result.code, expected);
			if (expected == QNFT::Code::Ok)
			{
				holdings[token][owner] -= quantity;
				if (burn)
				{
					burned[token] += quantity;
				}
				else
				{
					holdings[token][receiver] += quantity;
					fees += 7;
					funds[caller] -= 7;
				}
			}
		}
		for (unsigned t = 0; t < 3; ++t)
		{
			sint64 total = 0;
			for (unsigned user = 0; user < 3; ++user)
			{
				EXPECT_EQ(owned(t, users[user]), holdings[t][user]);
				total += holdings[t][user];
			}
			EXPECT_EQ(total, 40 - burned[t]);
			EXPECT_EQ(item(t).burned, burned[t]);
			QNFT::TotalSupply_output supply{};
			callFunction(QNFT_CONTRACT_INDEX, 7, QNFT::TotalSupply_input{t}, supply);
			EXPECT_EQ(supply.supply, total);
		}
		for (unsigned user = 0; user < 3; ++user)
		{
			EXPECT_EQ(getBalance(users[user]), funds[user]);
		}
		EXPECT_EQ(state->epochFees, fees);
		expectQuCollateral();
	}
}

TEST_F(NftCoverageTest, MaximumBatchTransfersAndBurnsEveryRequestedUnit)
{
	collection();
	ASSERT_EQ(mint(16).code, QNFT::Code::Ok);
	QNFT::BatchTransfer_input transfers{};
	transfers.owner = alice;
	transfers.receiver = bob;
	transfers.count = 16;
	QNFT::BurnBatch_input burns{};
	burns.owner = bob;
	burns.count = 16;
	for (unsigned i = 0; i < 16; ++i)
	{
		transfers.entries.set(i, QNFT::TokenAmount{1, 0});
		burns.entries.set(i, QNFT::TokenAmount{1, 0});
	}
	const auto moved = transferBatch(transfers);
	ASSERT_EQ(moved.code, QNFT::Code::Ok);
	EXPECT_EQ(moved.completed, 16U);
	EXPECT_EQ(owned(0, alice), 0);
	EXPECT_EQ(owned(0, bob), 16);
	const auto destroyed = burnBatch(burns, bob);
	ASSERT_EQ(destroyed.code, QNFT::Code::Ok);
	EXPECT_EQ(destroyed.completed, 16U);
	for (unsigned i = 0; i < 16; ++i)
	{
		EXPECT_EQ(moved.results.get(i).quantity, 1);
		EXPECT_EQ(destroyed.results.get(i).quantity, 1);
	}
	EXPECT_FALSE(exists(0));
	EXPECT_EQ(item().burned, 16);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, OversizedBurnBatchRefundsBeforeTouchingHoldings)
{
	collection();
	ASSERT_EQ(mint(16).code, QNFT::Code::Ok);
	QNFT::BurnBatch_input request{};
	request.owner = alice;
	request.count = 17;
	request.entries.set(0, QNFT::TokenAmount{1, 0});
	QNFT::BurnBatch_output result{};
	const auto funds = getBalance(alice);
	ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 15, request, result, alice, 17));
	EXPECT_EQ(result.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(result.attempted, 0U);
	EXPECT_EQ(result.completed, 0U);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(owned(0, alice), 16);
	EXPECT_EQ(item().burned, 0);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, TransferBatchRequiresEntireFeeBeforeFirstEntry)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->transferFee = 7;
	const auto fees = state->epochFees, funds = getBalance(alice);
	QNFT::BatchTransfer_input request{};
	request.owner = alice;
	request.receiver = bob;
	request.count = 2;
	request.entries.set(0, QNFT::TokenAmount{1, 0});
	request.entries.set(1, QNFT::TokenAmount{1, 0});
	const auto result = transferBatch(request, alice, 13);
	EXPECT_EQ(result.code, QNFT::Code::InsufficientFee);
	EXPECT_EQ(result.attempted, 0U);
	EXPECT_EQ(owned(0, alice), 10);
	EXPECT_EQ(owned(0, bob), 0);
	EXPECT_EQ(state->epochFees, fees);
	EXPECT_EQ(getBalance(alice), funds);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, SingleAndBatchMintRejectMalformedLaterMetadataBeforeIssuance)
{
	collection();
	QNFT::MintBatch_input request{};
	request.count = 2;
	request.entries.set(0, edition(1));
	auto invalid = edition(1);
	invalid.metadata.contentHash = NULL_ID;
	request.entries.set(1, invalid);
	const auto funds = getBalance(alice);
	const auto result = mintBatch(request);
	EXPECT_EQ(result.code, QNFT::Code::InvalidInput);
	EXPECT_EQ(result.failureIndex, 1U);
	EXPECT_EQ(result.attempted, 0U);
	EXPECT_EQ(result.completed, 0U);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(mintStatus().remainingItems, 10U);
	EXPECT_FALSE(exists(0));
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, InvalidOfferTermsDoNotReserveHoldings)
{
	collection();
	ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
	const QNFT::CreateRentalOffer_input requests[] = {{0, 1, 0, 15},          {-1, 1, 0, 15},         {1, 0, 0, 15},    {1, 1, 0, 14},
	                                                  {MAX_AMOUNT, 1, 0, 15}, {1, MAX_AMOUNT, 0, 15}, {100, 1, 999, 15}};
	const auto funds = getBalance(alice);
	for (const auto& request : requests)
	{
		EXPECT_EQ(change(20, request, alice, 17).code, QNFT::Code::InvalidInput);
		EXPECT_EQ(getBalance(alice), funds);
	}
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 10, 0}).code, QNFT::Code::Ok);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, OwnerCannotRentTheirOwnOffer)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice);
	ASSERT_EQ(offer.code, QNFT::Code::Ok);
	const auto funds = getBalance(alice);
	EXPECT_EQ(change(22, QNFT::Rent_input{offer.offerId, 1, 100, 15}, alice, 1500).code, QNFT::Code::InvalidInput);
	QNFT::GetRentalOffer_output remaining{};
	callFunction(QNFT_CONTRACT_INDEX, 23, QNFT::GetRentalOffer_input{offer.offerId}, remaining);
	EXPECT_EQ(remaining.offer.quantity, 2);
	EXPECT_EQ(getBalance(alice), funds);
	expectQuCollateral();
}

TEST_F(NftRentalCoverageTest, OfferIdExhaustionDoesNotLockAdditionalUnits)
{
	collection();
	ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
	auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
	state->lastRentalOfferId = 0x7fffffffffffffffULL;
	const auto funds = getBalance(alice);
	EXPECT_EQ(change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice, 17).code, QNFT::Code::Capacity);
	EXPECT_EQ(getBalance(alice), funds);
	EXPECT_EQ(state->rentalOffers.population(), 0U);
	ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 2, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(balance(0, bob), 2);
	expectQuCollateral();
}

TEST_F(NftCoverageTest, ConfigCountsIssuedTypesRatherThanCopiesOrLiveSupply)
{
	collection();
	collection();
	ASSERT_EQ(mint(7, QNFT::Policy::Immutable, 0).code, QNFT::Code::Ok);
	ASSERT_EQ(mint(3, QNFT::Policy::Immutable, 1).code, QNFT::Code::Ok);
	ASSERT_EQ(action(14, QNFT::Burn_input{alice, 7, 0}).code, QNFT::Code::Ok);
	EXPECT_EQ(action(6, edition(0), alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
	QNFT::GetConfig_output result{};
	callFunction(QNFT_CONTRACT_INDEX, 29, QNFT::GetConfig_input{}, result);
	EXPECT_EQ(result.mintIssuer, nftId);
	EXPECT_EQ(result.collectionCount, 2U);
	EXPECT_EQ(result.itemCount, 2U);
	EXPECT_FALSE(exists(0));
	EXPECT_EQ(owned(1, alice), 3);
	expectQuCollateral();
}

TEST_F(NftTest, RegistrationQpiScansAllIssuersAndPreservesExactNullFilter)
{
    collection();
    ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
    char name[8] = "PUNKS";
    char units[8] = {};
    int issuance, owner, possession;
    ASSERT_EQ(issueAsset(alice, name, 3, units, 123, QX_CONTRACT_INDEX, &issuance, &owner, &possession), 123);
    QPI::AssetIssuanceIterator issued(QPI::AssetIssuanceSelect::byIssuer(alice));
    ASSERT_FALSE(issued.reachedEnd());
    EXPECT_EQ(issued.numberOfDecimalPlaces(), 3);
    QPI::AssetOwnerIterator iterator;
    iterator.beginAnyIssuer(alice);
    EXPECT_FALSE(iterator.next(0));
    EXPECT_EQ(iterator.cursor(), 0U);
    sint64 total = 0;
    unsigned records = 0;
    while (!iterator.reachedEnd())
    {
        const auto before = iterator.cursor();
        if (iterator.next(1))
        {
            total += iterator.numberOfOwnedShares();
            ++records;
        }
        EXPECT_LE(iterator.cursor() - before, 1U);
    }
    EXPECT_EQ(total, 133);
    EXPECT_EQ(records, 2U);
    iterator.begin(alice, NULL_ID);
    while (!iterator.reachedEnd())
    {
        EXPECT_FALSE(iterator.next(1));
    }
    iterator.begin(alice, alice);
    ASSERT_TRUE(iterator.next(ASSETS_CAPACITY));
    EXPECT_EQ(iterator.numberOfOwnedShares(), 123);
    iterator.beginAnyIssuer(alice, ASSETS_CAPACITY + 1);
    EXPECT_FALSE(iterator.isValidCursor());
    EXPECT_FALSE(iterator.next(1));
}

class NftRegistrationTest : public Nft1155ExtrasTest
{
protected:
    template<class Input, class Output>
    void callProcedure(unsigned contract, unsigned procedure, const Input& input, const id& caller, sint64 reward, Output& output)
    {
        ASSERT_TRUE(invokeUserProcedure(contract, procedure, input, output, caller, reward));
    }

    QNFT::RegisterAsset_input request(const id& issuer = alice, sint64 quantity = 100, sint8 decimals = 0, uint64 name = 0x534b4e5550ULL)
    {
        char nameBytes[8]{};
        memcpy(nameBytes, &name, 8);
        char units[8]{};
        int issuance, ownership, possession;
        EXPECT_EQ(issueAsset(issuer, nameBytes, decimals, units, quantity, QX_CONTRACT_INDEX,
                             &issuance, &ownership, &possession), quantity);
        QNFT::RegisterAsset_input input{};
        input.asset = QPI::Asset{issuer, name};
        input.metadata = metadata();
        input.policy = QNFT::Policy::Mutable;
        return input;
    }

    QNFT::GetItemByAsset_output lookup(const QPI::Asset& asset)
    {
        QNFT::GetItemByAsset_output output{};
        callFunction(QNFT_CONTRACT_INDEX, 36, QNFT::GetItemByAsset_input{asset}, output);
        return output;
    }

    void manage(const QPI::Asset& asset, const id& owner, sint64 quantity)
    {
        QPI::AssetPossessionIterator it(asset, QPI::AssetOwnershipSelect::byOwner(owner), QPI::AssetPossessionSelect::any());
        ASSERT_FALSE(it.reachedEnd());
        ASSERT_TRUE(transferShareManagementRights(it.ownershipIndex(), it.possessionIndex(), QNFT_CONTRACT_INDEX,
                                                 QNFT_CONTRACT_INDEX, quantity, nullptr, nullptr, true));
    }
};

TEST_F(NftRegistrationTest, RegistrationPreservesLedgerAndConsumesOnlyItemQuota)
{
    collection(1, 2);
    const auto input = request();
    QPI::AssetPossessionIterator before(input.asset);
    ASSERT_FALSE(before.reachedEnd());
    const auto ownerIndex = before.ownershipIndex(), possessionIndex = before.possessionIndex();
    unsigned char ownerRecord[sizeof(AssetRecord)], possessionRecord[sizeof(AssetRecord)];
    memcpy(ownerRecord, &assets[ownerIndex], sizeof(ownerRecord));
    memcpy(possessionRecord, &assets[possessionIndex], sizeof(possessionRecord));
    const auto money = getBalance(alice);
    const auto result = action(29, input, alice, TEST_MINT_FEE + 17);
    ASSERT_EQ(result.code, QNFT::Code::Ok);
    EXPECT_EQ(result.quantity, 100);
    EXPECT_EQ(getBalance(alice), money - TEST_MINT_FEE);
    EXPECT_EQ(memcmp(&ownerRecord, &assets[ownerIndex], sizeof(ownerRecord)), 0);
    EXPECT_EQ(memcmp(&possessionRecord, &assets[possessionIndex], sizeof(possessionRecord)), 0);
    auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
    EXPECT_EQ(state->collections.get(0).issuedItems, 1U);
    EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
    EXPECT_EQ(state->pendingMintDeliveries.population(), 0U);
    const auto registered = item();
    EXPECT_EQ(registered.issuer, alice);
    EXPECT_EQ(registered.origin, QNFT::ItemOrigin::RegisteredAsset);
    EXPECT_EQ(registered.burned, 0);
    EXPECT_EQ(owned(0, alice), 100);
    EXPECT_EQ(lookup(input.asset).itemId, 0U);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
    EXPECT_EQ(mint(1).code, QNFT::Code::Ok);
    EXPECT_EQ(lookup(QPI::Asset{nftId, item(1).assetName}).itemId, 1U);
    expectQuCollateral();
}

TEST_F(NftRegistrationTest, ValidationRejectsWithoutChargingOrAllocating)
{
    collection();
    const auto input = request();
    const auto money = getBalance(alice);
    EXPECT_EQ(action(29, input, bob, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE - 1).code, QNFT::Code::InsufficientFee);
    auto invalid = input;
    invalid.asset.issuer = NULL_ID;
    EXPECT_EQ(action(29, invalid, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    invalid = input;
    invalid.asset.assetName |= 1ULL << 56;
    EXPECT_EQ(action(29, invalid, alice, TEST_MINT_FEE).code, QNFT::Code::NotFound);
    invalid = input;
    invalid.metadata.contentHash = NULL_ID;
    EXPECT_EQ(action(29, invalid, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    invalid = input;
    invalid.royaltyBps = 10001;
    EXPECT_EQ(action(29, invalid, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    invalid = input;
    invalid.collectionId = 9;
    EXPECT_EQ(action(29, invalid, alice, TEST_MINT_FEE).code, QNFT::Code::NotFound);
    EXPECT_EQ(getBalance(alice), money);
    EXPECT_EQ(reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->itemCount, 0U);
}

TEST_F(NftRegistrationTest, FractionalAssetAndHashCollisionAreRejected)
{
    collection();
    const auto fractional = request(alice, 100, 2);
    EXPECT_EQ(action(29, fractional, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    auto input = request(alice, 100, 0, 0x524548544fULL);
    ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
    QPI::Asset key{};
    key.issuer = input.asset.issuer;
    key.assetName = input.asset.assetName;
    id digest;
    KangarooTwelve(&key, sizeof(key), &digest, sizeof(digest));
    reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX])->registeredAssets.set(digest, 0);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
}

TEST_F(NftRegistrationTest, PartialManagementOperatorTransferAndBurnUseExternalIssuer)
{
    collection();
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    QNFT::TransferFrom_input transfer{};
    transfer.owner = alice; transfer.receiver = bob; transfer.quantity = 10;
    EXPECT_NE(action(12, transfer).code, QNFT::Code::Ok);
    manage(input.asset, alice, 30);
    QNFT::SetApprovalForAll_input approval{};
    approval.operatorId = carol; approval.approved = true;
    ASSERT_EQ(action(11, approval).code, QNFT::Code::Ok);
    ASSERT_EQ(action(12, transfer, carol).code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, bob), 10);
    QNFT::Burn_input burn{};
    burn.owner = bob; burn.quantity = 10;
    ASSERT_EQ(action(14, burn, bob).code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, bob), 0);
    EXPECT_EQ(item().burned, 0);
    EXPECT_EQ(item().origin, QNFT::ItemOrigin::RegisteredAsset);
}

TEST_F(NftRegistrationTest, NativeLookingNamesAreDistinctAcrossIssuers)
{
    collection();
    ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
    const auto input = request(alice, 20, 0, item().assetName);
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    EXPECT_EQ(lookup(input.asset).itemId, 1U);
    EXPECT_EQ(lookup(QPI::Asset{nftId, input.asset.assetName}).itemId, 0U);
    EXPECT_EQ(owned(1, alice), 20);
    EXPECT_EQ(owned(0, alice), 1);
}

TEST_F(NftRegistrationTest, ReissuanceKeepsIdentityAndSnapshotWithoutInventedBurns)
{
    collection();
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    QNFT::Burn_input burn{};
    burn.owner = alice; burn.quantity = 100;
    ASSERT_EQ(action(14, burn).code, QNFT::Code::Ok);
    EXPECT_FALSE(exists(0));
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::Ok);
    assetsEndEpoch();
    const auto reissued = request(alice, 200, 3);
    EXPECT_EQ(lookup(reissued.asset).itemId, 0U);
    EXPECT_TRUE(exists(0));
    EXPECT_EQ(owned(0, alice), 200);
    EXPECT_EQ(item().supply, 100);
    EXPECT_EQ(item().burned, 0);
    EXPECT_EQ(item().origin, QNFT::ItemOrigin::RegisteredAsset);
    EXPECT_EQ(action(29, reissued, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
}

TEST_F(NftRegistrationTest, ExternalAssetShopRentalAndExportLifecycle)
{
    collection();
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    QNFT::ListForSale_output listing{};
    callProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 10, 0}, alice, 0, listing);
    ASSERT_EQ(listing.code, QNFT::Code::Ok);
    QNFT::CancelSale_output cancelled{};
    callProcedure(QNFT_CONTRACT_INDEX, 19, QNFT::CancelSale_input{listing.listingId}, alice, 0, cancelled);
    ASSERT_EQ(cancelled.code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, alice), 100);
    callProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{100, 10, 0}, alice, 0, listing);
    ASSERT_EQ(listing.code, QNFT::Code::Ok);
    QNFT::Buy_output bought{};
    callProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listing.listingId, 10, 100, 1}, bob, 1000, bought);
    ASSERT_EQ(bought.code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, bob), 10);
    QNFT::RentalResult offer{}, rented{};
    callProcedure(QNFT_CONTRACT_INDEX, 20, QNFT::CreateRentalOffer_input{1, 2, 0, 15}, bob, 0, offer);
    ASSERT_EQ(offer.code, QNFT::Code::Ok);
    callProcedure(QNFT_CONTRACT_INDEX, 22, QNFT::Rent_input{offer.offerId, 2, 1, 15}, carol, 30, rented);
    ASSERT_EQ(rented.code, QNFT::Code::Ok);
    EXPECT_EQ(action(12, QNFT::TransferFrom_input{bob, alice, 10, 0}, bob).code, QNFT::Code::Conflict);
    INIT_CONTRACT(TESTEXA);
    reinterpret_cast<TESTEXA::StateData*>(contractStates[TESTEXA_CONTRACT_INDEX])->preAcquireSharesOutput.allowTransfer = true;
    QNFT::Export_input exportInput{};
    exportInput.owner = alice; exportInput.itemId = 0; exportInput.quantity = 5; exportInput.destination = TESTEXA_CONTRACT_INDEX;
    ASSERT_EQ(action(16, exportInput).code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, alice), 90);
    expectQuCollateral();
}

TEST_F(NftRegistrationTest, IssuerAdministratorCanRegisterWithoutHoldingUnits)
{
    collection();
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
    const auto input = request(bob);
    QPI::AssetPossessionIterator it(input.asset);
    ASSERT_FALSE(it.reachedEnd());
    int destinationOwner = 0, destinationPossession = 0;
    ASSERT_TRUE(transferShareOwnershipAndPossession(it.ownershipIndex(), it.possessionIndex(), carol, 100, &destinationOwner, &destinationPossession, true));
    const auto result = action(29, input, bob, TEST_MINT_FEE);
    ASSERT_EQ(result.code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, bob), 0);
    EXPECT_EQ(owned(0, carol), 100);
    EXPECT_EQ(item().creator, alice);
    EXPECT_EQ(item().issuer, bob);
    EXPECT_EQ(action(29, input, carol, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
}

TEST_F(NftRegistrationTest, RegistrationEmitsUriAndCommandWithoutMintTransfer)
{
    collection();
    const auto input = request();
    auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
    const auto events = state->lastEventId;
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    EXPECT_EQ(state->lastEventId, events + 2);
    const auto after = state->lastEventId;
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
    EXPECT_EQ(state->lastEventId, after);
}

TEST_F(NftRegistrationTest, ClosedCollectionsCapacityAndZeroFeeFollowMintRules)
{
    collection(1, 1);
    const auto input = request();
    auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
    for (const auto status : {QNFT::MintingStatus::Paused, QNFT::MintingStatus::Closed})
    {
        state->mintingStatus.set(0, status);
        EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
    }
    state->mintingStatus.set(0, QNFT::MintingStatus::Open);
    state->itemCount = QNFT_MAX_ITEMS;
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Capacity);
    state->itemCount = 0;
    state->mintFee = 0;
    ASSERT_EQ(action(29, input).code, QNFT::Code::Ok);
    const auto another = request(alice, 1, 0, 0x524548544fULL);
    EXPECT_EQ(action(29, another).code, QNFT::Code::Capacity);
    EXPECT_EQ(state->collections.get(0).issuedUnits, 0);
}

TEST_F(NftRegistrationTest, UserPagesIncludeExternalAndNativeAssets)
{
    collection();
    ASSERT_EQ(mint(2).code, QNFT::Code::Ok);
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    request(alice, 3, 0, 0x524548544fULL); // Unregistered units must not leak into NFT pages.
    QNFT::ListUserNfts_input query{};
    query.user = alice; query.limit = 1;
    std::set<uint32> ids;
    for (unsigned i = 0; i < 100; ++i)
    {
        QNFT::ListUserNfts_output output{};
        callFunction(QNFT_CONTRACT_INDEX, 18, query, output);
        ASSERT_EQ(output.code, QNFT::Code::Ok);
        for (uint32 j = 0; j < output.count; ++j)
        {
            EXPECT_TRUE(ids.insert(output.entries.get(j).itemId).second);
        }
        if (output.phase != QNFT::UserNftPhase::Ownership) break;
        query.cursor = output.nextCursor; query.tick = output.tick;
    }
    EXPECT_EQ(ids, (std::set<uint32>{0, 1}));
}

TEST(NftAbi, ExistingAssetRegistrationLayouts)
{
    EXPECT_EQ(sizeof(QNFT::RegisterAsset_input), 384U);
    EXPECT_EQ(sizeof(QNFT::GetItemByAsset_input), 40U);
    EXPECT_EQ(sizeof(QNFT::GetItemByAsset_output), 8U);
    EXPECT_EQ(static_cast<uint32>(QNFT::Operation::RegisterAsset), 11U);
    EXPECT_EQ(sizeof(QNFT::StateData), 123829272U);
}

TEST(NftAbi, InformationLogsHaveExplicitKindBeforeTypedAction)
{
    EXPECT_EQ(offsetof(QNFT::Event, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::MarketEvent, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::GovernanceEvent, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::TransferSingleEvent, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::UriEvent, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::SaleDividendEvent, kind), 4U);
    EXPECT_EQ(offsetof(QNFT::GovernanceEvent, action), 8U);
    EXPECT_EQ(offsetof(QNFT::TransferSingleEvent, type), 8U);
    EXPECT_EQ(offsetof(QNFT::UriEvent, type), 8U);
    EXPECT_EQ(offsetof(QNFT::SaleDividendEvent, type), 8U);
    EXPECT_EQ(offsetof(QNFT::TransferSingleEvent, _terminator), 124U);
    EXPECT_EQ(offsetof(QNFT::UriEvent, _terminator), 272U);
    EXPECT_EQ(offsetof(QNFT::SaleDividendEvent, _terminator), 48U);
    EXPECT_EQ(static_cast<uint32>(QNFT::TokenEventType::TransferSingle), 0U);
    EXPECT_EQ(static_cast<uint32>(QNFT::TokenEventType::URI), 1U);
    EXPECT_EQ(sizeof(QNFT::JournalEvent), 408U);
}

class NftRegistrationExpansionTest : public NftRegistrationTest {};
class NftRentalExpansionTest : public NftRentalTest {};
class NftReadExpansionTest : public Nft1155ExtrasTest {};

TEST_F(NftRegistrationExpansionTest, ExhaustedMintUnitsDoNotPreventRegisteringExistingSupply)
{
    collection(1, 2);
    ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    const auto status = mintStatus();
    EXPECT_EQ(status.remainingItems, 0U);
    EXPECT_EQ(status.remainingUnits, 0);
    EXPECT_EQ(item(1).supply, 100);
    auto* state = reinterpret_cast<QNFT::StateData*>(contractStates[QNFT_CONTRACT_INDEX]);
    EXPECT_EQ(state->collections.get(0).issuedItems, 2U);
    EXPECT_EQ(state->collections.get(0).issuedUnits, 1);
    expectQuCollateral();
}

TEST_F(NftRegistrationExpansionTest, FullyBurnedUnregisteredAssetDoesNotConsumeFeeOrIdentity)
{
    collection();
    const auto input = request();
    QX::TransferShareOwnershipAndPossession_output burned{};
    callProcedure(QX_CONTRACT_INDEX, 2,
                  QX::TransferShareOwnershipAndPossession_input{alice, NULL_ID, input.asset.assetName, 100},
                  alice, 1000000, burned);
    ASSERT_EQ(burned.transferredNumberOfShares, 100);
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
    EXPECT_EQ(mintStatus().remainingItems, 10U);
    expectQuCollateral();
}

TEST_F(NftRegistrationExpansionTest, MissingIssuanceIsRejectedBeforeAllocatingRegistryIdentity)
{
    collection();
    QNFT::RegisterAsset_input input{};
    input.asset = Asset{alice, 0x534b4e5550ULL};
    input.metadata = metadata();
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::NotFound);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
    const auto issued = request();
    const auto registered = action(29, issued, alice, TEST_MINT_FEE);
    ASSERT_EQ(registered.code, QNFT::Code::Ok);
    EXPECT_EQ(registered.itemId, 0U);
}

TEST_F(NftRegistrationExpansionTest, CollectionManagerCannotRegisterAnotherIssuerAsset)
{
    collection();
    const auto input = request(bob);
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
    EXPECT_EQ(action(29, input, bob, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
    EXPECT_EQ(action(29, input, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);
}

TEST_F(NftRegistrationExpansionTest, FormerAdministratorCannotRegisterTheirAsset)
{
    collection();
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
    const auto input = request(bob);
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
    const auto funds = getBalance(bob);
    EXPECT_EQ(action(29, input, bob, TEST_MINT_FEE).code, QNFT::Code::Unauthorized);
    EXPECT_EQ(getBalance(bob), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
}

TEST_F(NftRegistrationExpansionTest, RegisteredIdentityCannotBeMovedToAnotherCollection)
{
    collection();
    collection();
    auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    input.collectionId = 1;
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Conflict);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(item().collectionId, 0U);
    EXPECT_EQ(mintStatus(1).remainingItems, 10U);
}

TEST_F(NftRegistrationExpansionTest, PriorApprovalAppliesAfterRealQxManagementAcquisition)
{
    collection();
    ASSERT_EQ(action(11, QNFT::SetApprovalForAll_input{carol, true}).code, QNFT::Code::Ok);
    const auto input = request();
    QX::TransferShareManagementRights_input incoming{input.asset, 20, QNFT_CONTRACT_INDEX};
    QX::TransferShareManagementRights_output acquired{};
    callProcedure(QX_CONTRACT_INDEX, 9, incoming, alice, 1000000, acquired);
    EXPECT_EQ(acquired.transferredNumberOfShares, 0);
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    callProcedure(QX_CONTRACT_INDEX, 9, incoming, alice, 1000000, acquired);
    ASSERT_EQ(acquired.transferredNumberOfShares, 20);
    ASSERT_EQ(action(12, QNFT::TransferFrom_input{alice, bob, 7, 0}, carol).code, QNFT::Code::Ok);
    EXPECT_EQ(balance(0, alice, QX_CONTRACT_INDEX), 80);
    EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 13);
    EXPECT_EQ(owned(0, bob), 7);
    EXPECT_EQ(owned(0, alice), 93);
    expectQuCollateral();
}

TEST_F(NftRegistrationExpansionTest, MixedNativeAndRegisteredBatchPreservesIssuerAndBurnAccounting)
{
    collection();
    ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    QNFT::BatchTransfer_input transfer{};
    transfer.owner = alice;
    transfer.receiver = bob;
    transfer.count = 2;
    transfer.entries.set(0, QNFT::TokenAmount{3, 0});
    transfer.entries.set(1, QNFT::TokenAmount{20, 1});
    ASSERT_EQ(transferBatch(transfer).code, QNFT::Code::Ok);
    QNFT::BurnBatch_input burn{};
    burn.owner = bob;
    burn.count = 2;
    burn.entries.set(0, QNFT::TokenAmount{2, 0});
    burn.entries.set(1, QNFT::TokenAmount{5, 1});
    ASSERT_EQ(burnBatch(burn, bob).code, QNFT::Code::Ok);
    EXPECT_EQ(owned(0, alice), 7);
    EXPECT_EQ(owned(0, bob), 1);
    EXPECT_EQ(owned(1, alice), 80);
    EXPECT_EQ(owned(1, bob), 15);
    EXPECT_EQ(item(0).burned, 2);
    EXPECT_EQ(item(1).burned, 0);
    EXPECT_EQ(item(1).supply, 100);
    EXPECT_EQ(item(1).issuer, alice);
    expectQuCollateral();
}

TEST_F(NftRegistrationExpansionTest, UnmanagedExternalEntryRejectsWholeMixedTransfer)
{
    collection();
    ASSERT_EQ(mint(10).code, QNFT::Code::Ok);
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    QNFT::BatchTransfer_input transfer{};
    transfer.owner = alice;
    transfer.receiver = bob;
    transfer.count = 2;
    transfer.entries.set(0, QNFT::TokenAmount{3, 0});
    transfer.entries.set(1, QNFT::TokenAmount{1, 1});
    const auto funds = getBalance(alice);
    const auto result = transferBatch(transfer, alice, 123);
    EXPECT_EQ(result.code, QNFT::Code::InsufficientBalance);
    EXPECT_EQ(result.completed, 0U);
    EXPECT_EQ(owned(0, alice), 10);
    EXPECT_EQ(owned(0, bob), 0);
    EXPECT_EQ(owned(1, alice), 100);
    EXPECT_EQ(getBalance(alice), funds);
}

TEST_F(NftRegistrationExpansionTest, ExternalTransferPolicyCanBeReenabledWithoutChangingOwnership)
{
    collection();
    const auto input = request();
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    ASSERT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, true}).code, QNFT::Code::Ok);
    QNFT::Export_input exportInput{alice, 10, 1000000, 0, QX_CONTRACT_INDEX};
    EXPECT_EQ(action(16, exportInput, alice, 1000000).code, QNFT::Code::Conflict);
    EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 100);
    ASSERT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, false}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(16, exportInput, alice, 1000000).code, QNFT::Code::Ok);
    EXPECT_EQ(balance(0, alice, QX_CONTRACT_INDEX), 10);
    EXPECT_EQ(owned(0, alice), 100);
    EXPECT_EQ(lookup(input.asset).itemId, 0U);
}

TEST_F(NftRegistrationExpansionTest, RegisteredMetadataUsesCollectionAuthorityAfterIssuerLosesAdminRole)
{
    collection();
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{bob, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, bob).code, QNFT::Code::Ok);
    const auto input = request(bob);
    ASSERT_EQ(action(29, input, bob, TEST_MINT_FEE).code, QNFT::Code::Ok);
    ASSERT_EQ(action(2, QNFT::NominateAdmin_input{carol, 0}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(3, QNFT::AcceptAdmin_input{0}, carol).code, QNFT::Code::Ok);
    QNFT::UpdateMetadata_input update{metadata("ipfs://revised", 9), 0};
    EXPECT_EQ(action(9, update, bob).code, QNFT::Code::Unauthorized);
    ASSERT_EQ(action(9, update, carol).code, QNFT::Code::Ok);
    EXPECT_EQ(item().metadata.contentHash, id(9, 2, 3, 4));
    ASSERT_EQ(action(10, QNFT::FreezeMetadata_input{0}, alice).code, QNFT::Code::Ok);
    EXPECT_EQ(action(9, update, carol).code, QNFT::Code::Frozen);
    EXPECT_EQ(item().issuer, bob);
    EXPECT_EQ(item().creator, alice);
    EXPECT_EQ(item().metadataVersion, 2U);
}

TEST_F(NftRegistrationExpansionTest, RegisteredRoyaltyQuoteAndShopPayTheConfiguredBeneficiary)
{
    collection();
    auto input = request();
    input.royaltyReceiver = carol;
    input.royaltyBps = 500;
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    const auto quote = royalty(0, 2000);
    ASSERT_EQ(quote.code, QNFT::Code::Ok);
    EXPECT_EQ(quote.receiver, carol);
    EXPECT_EQ(quote.royaltyAmount, 100);
    QNFT::ListForSale_output listed{};
    callProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{1000, 2, 0}, alice, 0, listed);
    ASSERT_EQ(listed.code, QNFT::Code::Ok);
    const auto seller = getBalance(alice), artist = getBalance(carol), buyer = getBalance(bob);
    QNFT::Buy_output bought{};
    callProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 2, 1000, 1}, bob, 2023, bought);
    ASSERT_EQ(bought.code, QNFT::Code::Ok);
    EXPECT_EQ(bought.fee, 6);
    EXPECT_EQ(bought.royalty, 100);
    EXPECT_EQ(getBalance(alice) - seller, 1894);
    EXPECT_EQ(getBalance(carol) - artist, 100);
    EXPECT_EQ(buyer - getBalance(bob), 2000);
    EXPECT_EQ(owned(0, bob), 2);
    expectQuCollateral();
}

TEST_F(NftReadExpansionTest, BalanceBatchWithInvalidLaterOwnerDoesNotExposePrefix)
{
    collection();
    ASSERT_EQ(mint(7).code, QNFT::Code::Ok);
    QNFT::BalanceOfBatch_input input{};
    input.count = 3;
    input.entries.set(0, QNFT::BalanceOf_input{alice, 0});
    input.entries.set(1, QNFT::BalanceOf_input{bob, 999});
    input.entries.set(2, QNFT::BalanceOf_input{NULL_ID, 0});
    QNFT::BalanceOfBatch_output output{};
    callFunction(QNFT_CONTRACT_INDEX, 14, input, output);
    EXPECT_EQ(output.code, QNFT::Code::InvalidInput);
    EXPECT_EQ(output.count, 0U);
    EXPECT_EQ(output.balances.get(0), 0);
    input.entries.set(2, QNFT::BalanceOf_input{bob, 0});
    callFunction(QNFT_CONTRACT_INDEX, 14, input, output);
    ASSERT_EQ(output.code, QNFT::Code::Ok);
    EXPECT_EQ(output.count, 3U);
    EXPECT_EQ(output.balances.get(0), 7);
    EXPECT_EQ(output.balances.get(1), 0);
    EXPECT_EQ(output.balances.get(2), 0);
}

TEST_F(NftRegistrationExpansionTest, IpoSharesCannotBeRegisteredEvenByCollectionCreatorAndHolder)
{
    collection();
    std::vector<std::pair<m256i, unsigned int>> holders{{alice, 676}};
    issueContractShares(QX_CONTRACT_INDEX, holders);
    QNFT::RegisterAsset_input input{};
    input.asset = Asset{NULL_ID, *reinterpret_cast<const uint64*>(contractDescriptions[QX_CONTRACT_INDEX].assetName)};
    input.metadata = metadata();
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
    EXPECT_EQ(mintStatus().remainingItems, 10U);
}

TEST_F(NftRegistrationExpansionTest, SignedNonzeroDecimalPrecisionCannotBeRegistered)
{
    collection();
    const auto input = request(alice, 10, -1);
    const auto funds = getBalance(alice);
    EXPECT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::InvalidInput);
    EXPECT_EQ(getBalance(alice), funds);
    EXPECT_EQ(lookup(input.asset).code, QNFT::Code::NotFound);
    expectQuCollateral();
}

TEST_F(NftRegistrationExpansionTest, RegisteredSaleDividendsUseActualIpoHolders)
{
    collection();
    std::vector<std::pair<m256i, unsigned int>> holders{{carol, 676}};
    issueContractShares(QX_CONTRACT_INDEX, holders);
    auto input = request();
    input.dividendAssetName = *reinterpret_cast<const uint64*>(contractDescriptions[QX_CONTRACT_INDEX].assetName);
    input.dividendBps = 1000;
    ASSERT_EQ(action(29, input, alice, TEST_MINT_FEE).code, QNFT::Code::Ok);
    manage(input.asset, alice, 100);
    QNFT::DividendInfo_output quote{};
    callFunction(QNFT_CONTRACT_INDEX, 12, QNFT::DividendInfo_input{1000000, 0}, quote);
    ASSERT_EQ(quote.code, QNFT::Code::Ok);
    EXPECT_EQ(quote.amountPerShare, 147);
    EXPECT_EQ(quote.shareholderAmount, 99372);
    QNFT::ListForSale_output listed{};
    callProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{1000000, 1, 0}, alice, 0, listed);
    ASSERT_EQ(listed.code, QNFT::Code::Ok);
    const auto holder = getBalance(carol), seller = getBalance(alice);
    QNFT::Buy_output bought{};
    callProcedure(QNFT_CONTRACT_INDEX, 18, QNFT::Buy_input{bob, listed.listingId, 1, 1000000, 1}, bob, 1000000, bought);
    ASSERT_EQ(bought.code, QNFT::Code::Ok);
    EXPECT_EQ(getBalance(carol) - holder, 99372);
    EXPECT_EQ(getBalance(alice) - seller, 897628);
    EXPECT_EQ(bought.fee, 3000);
    EXPECT_EQ(owned(0, bob), 1);
    expectQuCollateral();
}

TEST_F(NftReadExpansionTest, UnsolicitedManagementAcquisitionCannotBypassExportAuthorization)
{
    collection();
    ASSERT_EQ(mint(5).code, QNFT::Code::Ok);
    INIT_CONTRACT(TESTEXA);
    const TESTEXA::AcquireShareManagementRights_input input{Asset{nftId, item().assetName}, alice, 5, QNFT_CONTRACT_INDEX};
    TESTEXA::AcquireShareManagementRights_output output{};
    ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 6, input, output, bob, 0));
    EXPECT_EQ(output.transferredNumberOfShares, 0);
    EXPECT_EQ(balance(0, alice, QNFT_CONTRACT_INDEX), 5);
    EXPECT_EQ(balance(0, alice, TESTEXA_CONTRACT_INDEX), 0);
    EXPECT_EQ(item().reserved, 0);
}

TEST_F(NftReadExpansionTest, FullyBurnedItemRetainsMetadataButHasNoUsageOrRoyaltyQuote)
{
    collection();
    ASSERT_EQ(mint(1).code, QNFT::Code::Ok);
    ASSERT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}).code, QNFT::Code::Ok);
    QNFT::URI_output uri{};
    callFunction(QNFT_CONTRACT_INDEX, 9, QNFT::URI_input{0}, uri);
    ASSERT_EQ(uri.code, QNFT::Code::Ok);
    EXPECT_EQ(uri.uri.get(0), 'i');
    EXPECT_FALSE(exists(0));
    EXPECT_EQ(royalty(0, 1000).code, QNFT::Code::NotFound);
    QNFT::GetUserNft_output user{};
    callFunction(QNFT_CONTRACT_INDEX, 17, QNFT::GetUserNft_input{alice, 0}, user);
    ASSERT_EQ(user.code, QNFT::Code::Ok);
    EXPECT_EQ(user.ownedQuantity, 0);
    EXPECT_EQ(user.availableQuantity, 0);
    EXPECT_EQ(user.rentedQuantity, 0);
    EXPECT_EQ(item().burned, 1);
}

TEST_F(NftRentalExpansionTest, PaidUsageDoesNotAuthorizeRenterToSellBurnOrExportLenderUnits)
{
    const auto key = open();
    EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 1, 0}, bob).code, QNFT::Code::Unauthorized);
    EXPECT_EQ(action(14, QNFT::Burn_input{alice, 1, 0}, bob).code, QNFT::Code::Unauthorized);
    QNFT::Export_input exportInput{alice, 1, 1000000, 0, QX_CONTRACT_INDEX};
    EXPECT_EQ(action(16, exportInput, bob, 1000000).code, QNFT::Code::Unauthorized);
    QNFT::ListForSale_output listing{};
    ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 1, 0}, listing, bob, 0));
    EXPECT_EQ(listing.code, QNFT::Code::InsufficientBalance);
    EXPECT_EQ(balance(0, alice), 10);
    EXPECT_EQ(balance(0, bob), 0);
    EXPECT_EQ(rental(key).rental.renter, bob);
    expectQuCollateral();
}

TEST_F(NftRentalExpansionTest, ReservedOfferBlocksListingUntilOwnerCancelsIt)
{
    collection();
    ASSERT_EQ(mint(3).code, QNFT::Code::Ok);
    const auto offer = change(20, QNFT::CreateRentalOffer_input{1, 3, 0, 15}, alice);
    ASSERT_EQ(offer.code, QNFT::Code::Ok);
    QNFT::ListForSale_output listed{};
    ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 1, 0}, listed, alice, 0));
    EXPECT_EQ(listed.code, QNFT::Code::Conflict);
    ASSERT_EQ(change(21, QNFT::CancelRentalOffer_input{offer.offerId}, alice).code, QNFT::Code::Ok);
    ASSERT_TRUE(invokeUserProcedure(QNFT_CONTRACT_INDEX, 17, QNFT::ListForSale_input{10, 3, 0}, listed, alice, 0));
    EXPECT_EQ(listed.code, QNFT::Code::Ok);
    EXPECT_EQ(balance(0, nftId), 3);
    EXPECT_EQ(balance(0, alice), 0);
}

TEST_F(NftRentalExpansionTest, ClosingMintAndDisablingExportPreservesPaidUsageAndRenewals)
{
    const auto key = open();
    ASSERT_EQ(action(4, QNFT::SetMintingStatus_input{0, QNFT::MintingStatus::Closed}).code, QNFT::Code::Ok);
    ASSERT_EQ(action(5, QNFT::SetExternalTransfersDisabled_input{0, true}).code, QNFT::Code::Ok);
    advance(15);
    const auto before = getBalance(alice);
    callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
    ASSERT_EQ(rental(key).code, QNFT::Code::Ok);
    EXPECT_EQ(rental(key).rental.status, QNFT::RentalStatus::Active);
    EXPECT_EQ(rental(key).rental.deposit, 0);
    EXPECT_EQ(getBalance(alice) - before, 2790);
    EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 9, 0}).code, QNFT::Code::Conflict);
    expectQuCollateral();
}

TEST_F(NftRentalExpansionTest, PartialRenewalDepositEndsRightsWithoutConsumingRemainder)
{
    const auto key = open(5999);
    advance(15);
    const auto before = getBalance(alice);
    callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
    const auto ended = rental(key);
    ASSERT_EQ(ended.code, QNFT::Code::Ok);
    EXPECT_EQ(ended.rental.status, QNFT::RentalStatus::Ended);
    EXPECT_EQ(ended.rental.deposit, 2999);
    EXPECT_EQ(getBalance(alice), before);
    const auto renterFunds = getBalance(bob);
    EXPECT_EQ(change(24, QNFT::RentalDeposit_input{key, 2999}).code, QNFT::Code::Ok);
    EXPECT_EQ(getBalance(bob) - renterFunds, 2999);
    EXPECT_EQ(action(12, QNFT::TransferFrom_input{alice, carol, 10, 0}).code, QNFT::Code::Ok);
    expectQuCollateral();
}

TEST_F(NftRentalExpansionTest, ZeroDepositEndedRentalIsReclaimedWithoutReusingItsId)
{
    const auto oldId = open(3000);
    advance(15);
    callSystemProcedure(QNFT_CONTRACT_INDEX, END_TICK);
    EXPECT_EQ(rental(oldId).code, QNFT::Code::NotFound);
    const auto offer = change(20, QNFT::CreateRentalOffer_input{100, 2, 0, 15}, alice);
    ASSERT_EQ(offer.code, QNFT::Code::Ok);
    const auto next = change(22, QNFT::Rent_input{offer.offerId, 2, 100, 15}, bob, 3000);
    ASSERT_EQ(next.code, QNFT::Code::Ok);
    EXPECT_GT(next.rentalId, oldId);
    EXPECT_EQ(rental(oldId).code, QNFT::Code::NotFound);
    EXPECT_EQ(change(23, QNFT::GetRental_input{oldId}, bob, 100).code, QNFT::Code::NotFound);
    expectQuCollateral();
}

TEST(QnftRentalProjectionExpansion, ClockHorizonCannotConsumeDepositForUnrepresentableRenewal)
{
    QNFT::Rental rental{};
    rental.paidUntil = QPI::DateAndTime(65535, 12, 31);
    rental.status = QNFT::RentalStatus::Active;
    rental.autoRenew = true;
    rental.periodDays = 15;
    rental.periodPrice = 15;
    rental.deposit = 30;
    QNFT::RentalPeriodProjection out{};
    QNFT::ProjectRentalPeriods(rental, rental.paidUntil, out);
    EXPECT_FALSE(out.active);
    EXPECT_EQ(out.periodsPaid, 0U);
    EXPECT_EQ(out.paidUntil, QPI::DateAndTime(65535, 12, 31));
    EXPECT_EQ(rental.deposit, 30);
}

TEST(QnftRentalProjectionExpansion, ExpiryBoundaryAndInsufficientDepositHaveLiteralExpectedCoverage)
{
    struct Case { uint64 day; sint64 deposit; uint64 paid; bool active; uint64 untilDay; };
    const Case cases[] = {{15, 0, 0, true, 16}, {16, 14, 0, false, 16}, {16, 15, 1, true, 31},
                          {31, 15, 1, false, 31}, {31, 30, 2, true, 15}};
    for (const auto& c : cases)
    {
        SCOPED_TRACE(c.day);
        QNFT::Rental rental{};
        rental.paidUntil = QPI::DateAndTime(2026, 1, 16);
        rental.status = QNFT::RentalStatus::Active;
        rental.autoRenew = true;
        rental.periodDays = 15;
        rental.periodPrice = 15;
        rental.deposit = c.deposit;
        QNFT::RentalPeriodProjection out{};
        QNFT::ProjectRentalPeriods(rental, QPI::DateAndTime(2026, 1, c.day), out);
        EXPECT_EQ(out.periodsPaid, c.paid);
        EXPECT_EQ(out.active, c.active);
        EXPECT_EQ(out.paidUntil, QPI::DateAndTime(2026, c.paid == 2 ? 2 : 1, c.untilDay));
        EXPECT_EQ(rental.deposit, c.deposit);
    }
}
