using namespace QPI;

constexpr uint64 QRAFFLE_REGISTER_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_QXMR_REGISTER_AMOUNT = 250000000ull;
constexpr uint64 QRAFFLE_MAX_QRE_AMOUNT = 1000000000ull;
constexpr uint64 QRAFFLE_ASSET_NAME = 19505638103142993;
constexpr uint64 QRAFFLE_QXMR_ASSET_NAME = 1380800593; // QXMR token asset name
constexpr uint32 QRAFFLE_LOGOUT_FEE = div<uint32>(QRAFFLE_REGISTER_AMOUNT, 20);
constexpr uint32 QRAFFLE_QXMR_LOGOUT_FEE = div<uint32>(QRAFFLE_QXMR_REGISTER_AMOUNT, 20); // QXMR logout fee
constexpr uint32 QRAFFLE_TRANSFER_SHARE_FEE = 100;
constexpr uint32 QRAFFLE_BURN_FEE = 5; // percent
constexpr uint32 QRAFFLE_REGISTER_FEE = 5; // percent
constexpr uint32 QRAFFLE_FEE = 1; // percent
constexpr uint32 QRAFFLE_CHARITY_FEE = 1; // percent
constexpr uint32 QRAFFLE_SHAREHOLDER_FEE = 8; // percent
constexpr uint32 QRAFFLE_MAX_EPOCH = 65536;
constexpr uint32 QRAFFLE_MAX_PROPOSAL_EPOCH = 128;
constexpr uint32 QRAFFLE_MAX_MEMBER = 65536;
constexpr uint32 QRAFFLE_DEFAULT_QRAFFLE_AMOUNT = 10000000ull;
constexpr uint32 QRAFFLE_MIN_QRAFFLE_AMOUNT = 1000000ull;
constexpr uint32 QRAFFLE_MAX_QRAFFLE_AMOUNT = 1000000000ull;
// Ended token-raffle ring: 16 384 slots × ~96 B ≈ 1.5 MB.
// At most QRAFFLE_MAX_PROPOSAL_EPOCH (128) raffles/epoch → covers ~128 epochs of history.
constexpr uint32 QRAFFLE_MAX_TOKEN_RAFFLES = 16384;
constexpr uint32 QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE = 512; // 2^9, max members per token raffle
constexpr uint8 QRAFFLE_MAX_PROPOSALS_PER_PROPOSER = 3; // max proposals per user per epoch

constexpr sint32 QRAFFLE_SUCCESS = 0;
constexpr sint32 QRAFFLE_INSUFFICIENT_FUND = 1;
constexpr sint32 QRAFFLE_ALREADY_REGISTERED = 2;
constexpr sint32 QRAFFLE_UNREGISTERED = 3;
constexpr sint32 QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED = 4;
constexpr sint32 QRAFFLE_INVALID_PROPOSAL = 5;
constexpr sint32 QRAFFLE_FAILED_TO_DEPOSIT = 6;
constexpr sint32 QRAFFLE_ALREADY_VOTED = 7;
constexpr sint32 QRAFFLE_INVALID_TOKEN_RAFFLE = 8;
constexpr sint32 QRAFFLE_INVALID_OFFSET_OR_LIMIT = 9;
constexpr sint32 QRAFFLE_INVALID_EPOCH = 10;
constexpr sint32 QRAFFLE_MAX_MEMBER_REACHED = 11;
constexpr sint32 QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT = 12;
constexpr sint32 QRAFFLE_INSUFFICIENT_QXMR = 13;
constexpr sint32 QRAFFLE_INVALID_TOKEN_TYPE = 14;
constexpr sint32 QRAFFLE_USER_NOT_FOUND = 15;
constexpr sint32 QRAFFLE_INVALID_ENTRY_AMOUNT = 16;
constexpr sint32 QRAFFLE_EMPTY_QU_RAFFLE = 17;
constexpr sint32 QRAFFLE_EMPTY_TOKEN_RAFFLE = 18;
constexpr sint32 QRAFFLE_MAX_PROPOSAL_PER_USER_REACHED = 19;

// Asset Raffle return codes
constexpr sint32 QRAFFLE_INVALID_BUNDLE             = 20;
constexpr sint32 QRAFFLE_INVALID_RESERVE_PRICE      = 21;
constexpr sint32 QRAFFLE_BUNDLE_ESCROW_FAILED       = 22;
constexpr sint32 QRAFFLE_INVALID_ASSET_RAFFLE       = 23;
constexpr sint32 QRAFFLE_ASSET_RAFFLE_FULL          = 24;
constexpr sint32 QRAFFLE_TICKET_LIMIT_REACHED       = 25;
constexpr sint32 QRAFFLE_MAX_ASSET_RAFFLES_REACHED  = 26;
constexpr sint32 QRAFFLE_CANCEL_NOT_ALLOWED         = 27;

// Asset Raffle configuration
constexpr uint64 QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE       = 500000ull;    // 500K Qu; non-refundable anti-spam
constexpr uint32 QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH     = 64;              // concurrent active raffles
constexpr uint32 QRAFFLE_MAX_ASSETS_PER_BUNDLE           = 4;               // items per bundle
constexpr uint32 QRAFFLE_MAX_ASSET_TICKET_BUYERS         = 1024;            // distinct buyers per raffle
constexpr uint32 QRAFFLE_MAX_TICKETS_PER_BUYER           = 100;             // per-buyer cap (anti-griefing)
constexpr uint32 QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR   = 2;               // per creator per epoch
constexpr uint32 QRAFFLE_MAX_ENDED_ASSET_RAFFLES         = 8192;            // history ring buffer
constexpr uint64 QRAFFLE_MIN_ASSET_TICKET_AMOUNT         = 1000000ull;      // 1M Qu
constexpr uint64 QRAFFLE_MAX_ASSET_TICKET_AMOUNT         = 1000000000000ull;// 1T Qu
// Flat array strides: raffle i occupies [i*stride .. i*stride+count)
constexpr uint32 QRAFFLE_ASSET_RAFFLE_BUNDLE_FLAT_SIZE   = QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH * QRAFFLE_MAX_ASSETS_PER_BUNDLE;   // 512
constexpr uint32 QRAFFLE_ASSET_RAFFLE_BUYERS_FLAT_SIZE   = QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH * QRAFFLE_MAX_ASSET_TICKET_BUYERS; // 65536

struct QRAFFLE2
{
};

struct QRAFFLE : public ContractBase
{
public:
	enum LogInfo {
		QRAFFLE_success = 0,
		QRAFFLE_insufficientQubic = 1,
		QRAFFLE_insufficientQXMR = 2,
		QRAFFLE_alreadyRegistered = 3,
		QRAFFLE_unregistered = 4,
		QRAFFLE_maxMemberReached = 5,
		QRAFFLE_maxProposalEpochReached = 6,
		QRAFFLE_invalidProposal = 7,
		QRAFFLE_failedToDeposit = 8,
		QRAFFLE_alreadyVoted = 9,
		QRAFFLE_invalidTokenRaffle = 10,
		QRAFFLE_invalidOffsetOrLimit = 11,
		QRAFFLE_invalidEpoch = 12,
		QRAFFLE_initialRegisterCannotLogout = 13,
		QRAFFLE_invalidTokenType = 14,
		QRAFFLE_invalidEntryAmount = 15,
		QRAFFLE_maxMemberReachedForQuRaffle = 16,
		QRAFFLE_proposalNotFound = 17,
		QRAFFLE_proposalAlreadyEnded = 18,
		QRAFFLE_notEnoughShares = 19,
		QRAFFLE_transferFailed = 20,
		QRAFFLE_epochEnded = 21,
		QRAFFLE_winnerSelected = 22,
		QRAFFLE_revenueDistributed = 23,
		QRAFFLE_tokenRaffleCreated = 24,
		QRAFFLE_tokenRaffleEnded = 25,
		QRAFFLE_proposalSubmitted = 26,
		QRAFFLE_proposalVoted = 27,
		QRAFFLE_quRaffleDeposited = 28,
		QRAFFLE_tokenRaffleDeposited = 29,
		QRAFFLE_shareManagementRightsTransferred = 30,
		QRAFFLE_emptyQuRaffle = 31,
		QRAFFLE_emptyTokenRaffle = 32,
		QRAFFLE_maxProposalPerUserReached = 33,
		// Asset Raffle log types
		QRAFFLE_assetRaffleCreated = 34,
		QRAFFLE_assetRaffleTicketBought = 35,
		QRAFFLE_assetRaffleSucceeded = 36,
		QRAFFLE_assetRaffleRefunded = 37,
		QRAFFLE_assetRaffleBundleEscrowFailed = 38,
		QRAFFLE_assetRaffleBundleDeliveryFailed = 39,
		QRAFFLE_assetRaffleCancelled = 40,
		QRAFFLE_invalidBundle = 41,
		QRAFFLE_invalidReservePrice = 42,
		QRAFFLE_assetRaffleFull = 43,
		QRAFFLE_ticketLimitReached = 44,
		QRAFFLE_maxAssetRafflesReached = 45,
		QRAFFLE_cancelNotAllowed = 46
	};

	struct Logger
	{
		uint32 _contractIndex;
		uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	// Enhanced logger for END_EPOCH with detailed information
	struct EndEpochLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _epoch; // Current epoch number
		uint32 _memberCount; // Number of QuRaffle members
		uint64 _totalAmount; // Total amount being processed
		uint64 _winnerAmount; // Amount won by winner
		uint32 _winnerIndex; // Index of the winner
		sint8 _terminator;
	};

	// Enhanced logger for revenue distribution
	struct RevenueLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint64 _burnAmount; // Amount burned
		uint64 _charityAmount; // Amount sent to charity
		uint64 _shareholderAmount; // Amount distributed to shareholders
		uint64 _registerAmount; // Amount distributed to registers
		uint64 _feeAmount; // Amount sent to fee address
		uint64 _winnerAmount; // Amount sent to winner
		sint8 _terminator;
	};

	// Enhanced logger for token raffle processing
	struct TokenRaffleLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _raffleIndex; // Index of the token raffle
		uint64 _assetName; // Asset name of the token
		uint32 _memberCount; // Number of members in this raffle
		uint64 _entryAmount; // Entry amount for this raffle
		uint32 _winnerIndex; // Winner index for this raffle
		uint64 _winnerAmount; // Amount won in this raffle
		sint8 _terminator;
	};

	// Enhanced logger for proposal processing
	struct ProposalLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _proposalIndex; // Index of the proposal
		id _proposer; // Proposer of the proposal
		uint32 _yesVotes; // Number of yes votes
		uint32 _noVotes; // Number of no votes
		uint64 _assetName; // Asset name if approved
		uint64 _entryAmount; // Entry amount if approved
		sint8 _terminator;
	};

	struct EmptyTokenRaffleLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _tokenRaffleIndex; // Index of the token raffle per epoch
		sint8 _terminator;
	};

	struct AssetRaffleCreatedLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _raffleIndex;
		id _creator;
		uint64 _reservePriceQu;
		uint64 _entryTicketQu;
		uint32 _bundleSize;
		sint8 _terminator;
	};

	struct AssetRaffleTicketLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _raffleIndex;
		id _buyer;
		uint32 _tickets;
		uint64 _cost;
		sint8 _terminator;
	};

	struct AssetRaffleEndedLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 _raffleIndex;
		id _creator;
		id _winner;
		uint64 _grossPoolQu;
		uint64 _creatorPaidQu;
		uint8 _reserveMet;
		sint8 _terminator;
	};

	// One item in an asset raffle bundle (token or SC share + quantity).
	struct AssetRaffleItem
	{
		Asset  asset;
		sint64 numberOfShares;
	};

	// Active asset raffle state (live during the epoch it was created).
	struct AssetRaffleInfo
	{
		id     creator;
		uint64 reservePriceQu;     // net Qu creator wants AFTER 20% fee
		uint64 entryTicketQu;      // Qu per ticket
		uint64 totalTicketsPaidQu; // gross Qu pool so far
		uint32 numberOfBuyers;
		uint32 totalTickets;
		uint32 bundleSize;
		uint32 epoch;
	};

	// Historical record written at END_EPOCH for each settled asset raffle.
	// Field order keeps the largest types first so trailing padding is minimal and
	// deterministic across compilers (no explicit pad needed → no plain C arrays).
	struct EndedAssetRaffleInfo
	{
		id     creator;
		id     epochWinner;      // NULL_ID if reserve was missed
		uint64 reservePriceQu;
		uint64 entryTicketQu;
		uint64 grossPoolQu;
		uint64 creatorPaidQu;    // 0 if reserve missed
		uint32 totalTickets;
		uint32 numberOfBuyers;
		uint32 bundleSize;
		uint32 epoch;
		uint32 reserveMet;       // 1 = reserve met and winner paid; 0 = refunded (uint32 keeps natural alignment)
	};

	struct ProposalInfo {
		Asset token;
		id proposer;
		uint64 entryAmount;
		uint32 nYes;
		uint32 nNo;
	};

	struct QuRaffleInfo
	{
		id epochWinner;
		uint64 receivedAmount;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
	};

	struct TokenRaffleInfo
	{
		id epochWinner;
		Asset token;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint32 epoch;
	};

	struct ActiveTokenRaffleInfo {
		Asset token;
		uint64 entryAmount;
	};

	struct StateData
	{
		HashMap <id, uint8, QRAFFLE_MAX_MEMBER> registers;
		Array <ProposalInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> proposals;

		// Per-user vote tracking with dual BitArray (qRWA pattern).
		// O(1) lookup via id hash, 1 bit per proposal. ~4 MB each, ~8 MB total.
		HashMap <id, BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH>, QRAFFLE_MAX_MEMBER> voteParticipation; // bit=1 if user has voted
		HashMap <id, BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH>, QRAFFLE_MAX_MEMBER> voteValues; // bit=1 for yes, bit=0 for no
		Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfVotedInProposal;
		Array <id, QRAFFLE_MAX_MEMBER> quRaffleMembers;
		// O(1) duplicate guard for quRaffle entries; mirrors quRaffleMembers for membership tests.
		HashSet <id, QRAFFLE_MAX_MEMBER> quRaffleMemberSet;

		Array <ActiveTokenRaffleInfo, QRAFFLE_MAX_PROPOSAL_EPOCH> activeTokenRaffle;
		// Per-user O(1) duplicate check for token raffle deposits. ~4 MB.
		HashMap <id, BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH>, QRAFFLE_MAX_MEMBER> tokenRaffleParticipation;
		// Flat indexed member storage: raffle i occupies slots [i*SLOT_SIZE .. i*SLOT_SIZE+count). ~2 MB.
		Array <id, QRAFFLE_MAX_MEMBER> tokenRaffleMemberSlots;
		Array <uint32, QRAFFLE_MAX_PROPOSAL_EPOCH> numberOfTokenRaffleMembers;

		Array <QuRaffleInfo, QRAFFLE_MAX_EPOCH> QuRaffles;
		Array <TokenRaffleInfo, QRAFFLE_MAX_TOKEN_RAFFLES> tokenRaffle;
		HashMap <id, uint64, QRAFFLE_MAX_MEMBER> quRaffleEntryAmount;

		id initialRegister1, initialRegister2, initialRegister3, initialRegister4, initialRegister5;
		id charityAddress, feeAddress, QXMRIssuer;
		uint64 epochRevenue, epochQXMRRevenue, qREAmount, totalBurnAmount, totalCharityAmount, totalShareholderAmount, totalRegisterAmount, totalFeeAmount, totalWinnerAmount, largestWinnerAmount;
		uint32 numberOfRegisters, numberOfQuRaffleMembers, numberOfEntryAmountSubmitted, numberOfProposals, numberOfActiveTokenRaffle, numberOfEndedTokenRaffle;
		Array<uint32, QRAFFLE_MAX_EPOCH> daoMemberCount; // Number of DAO members (registers) at each epoch
		HashMap <id, uint8, QRAFFLE_MAX_MEMBER> proposalsPerProposer;

		// ── Asset Raffle state ────────────────────────────────────────────────────────
		Array<AssetRaffleInfo, QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH> activeAssetRaffles;
		uint32 numberOfActiveAssetRaffles;

		// Bundle items: raffle i occupies [i*8 .. i*8+bundleSize).
		Array<AssetRaffleItem, QRAFFLE_ASSET_RAFFLE_BUNDLE_FLAT_SIZE> activeAssetRaffleItems;

		// Buyer lists: raffle i occupies [i*1024 .. i*1024+numberOfBuyers).
		Array<id,     QRAFFLE_ASSET_RAFFLE_BUYERS_FLAT_SIZE> activeAssetRaffleBuyers;
		Array<uint32, QRAFFLE_ASSET_RAFFLE_BUYERS_FLAT_SIZE> activeAssetRaffleBuyerTickets;

		// O(1) has-bought check: bit[i]=1 means this user has tickets in raffle i.
		HashMap<id, BitArray<QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH>, QRAFFLE_MAX_MEMBER> assetRaffleParticipation;

		// O(1) slot lookup: entry[i] = buyer's 0-based position in raffle i's buyer region.
		// Sentinel 0xFFFF = not present. ~8 MB (64 raffles × 2 B × 65536 buyers).
		// Reset each epoch alongside the buyer arrays.
		HashMap<id, Array<uint16, QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH>, QRAFFLE_MAX_MEMBER> assetRaffleBuyerSlotIndex;

		// Per-creator raffle count; reset each epoch to enforce QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR.
		HashMap<id, uint8, QRAFFLE_MAX_MEMBER> assetRafflesPerCreator;

		// Settled raffle history ring buffer.
		Array<EndedAssetRaffleInfo, QRAFFLE_MAX_ENDED_ASSET_RAFFLES> endedAssetRaffles;
		uint32 numberOfEndedAssetRaffles;

		// Accumulated proposal fees destined for DAO registers (50% of each 500K fee);
		// distributed in one O(R) pass at END_EPOCH alongside the register share bucket.
		uint64 epochAssetRaffleDaoBucket;

		// Aggregate analytics (monotonically increasing).
		uint64 totalAssetRaffleProposalFees;
		uint64 totalAssetRaffleCreatorPaid;
		uint64 totalAssetRaffleRefunded;
		uint32 totalAssetRafflesCreated;
		uint32 totalAssetRafflesSucceeded;
		uint32 totalAssetRafflesFailed;
	};

	struct registerInSystem_input
	{
		bit useQXMR; // 0 = use qubic, 1 = use QXMR tokens
	};

	struct registerInSystem_output
	{
		sint32 returnCode;
	};

	struct logoutInSystem_input
	{
	};

	struct logoutInSystem_output
	{
		sint32 returnCode;
	};

	struct submitEntryAmount_input
	{
		uint64 amount;
	};

	struct submitEntryAmount_output
	{
		sint32 returnCode;
	};

	struct submitProposal_input
	{
		id tokenIssuer;
		uint64 tokenName;
		uint64 entryAmount;
	};

	struct submitProposal_output
	{
		sint32 returnCode;
	};

	struct voteInProposal_input
	{
		uint32 indexOfProposal;
		bit yes;
	};

	struct voteInProposal_output
	{
		sint32 returnCode;
	};

	struct depositInQuRaffle_input
	{

	};

	struct depositInQuRaffle_output
	{
		sint32 returnCode;
	};

	struct depositInTokenRaffle_input
	{
		uint32 indexOfTokenRaffle;
	};

	struct depositInTokenRaffle_output
	{
		sint32 returnCode;
	};

	struct TransferShareManagementRights_input
	{
		id tokenIssuer;
		uint64 tokenName;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};

	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

	struct getRegisters_input
	{
		uint32 offset;
		uint32 limit;
	};

	struct getRegisters_output
	{
		id register1, register2, register3, register4, register5, register6, register7, register8, register9, register10, register11, register12, register13, register14, register15, register16, register17, register18, register19, register20;
		sint32 returnCode;
	};

	struct getAnalytics_input
	{
	};

	struct getAnalytics_output
	{
		uint64 currentQuRaffleAmount;
		uint64 totalBurnAmount;
		uint64 totalCharityAmount;
		uint64 totalShareholderAmount;
		uint64 totalRegisterAmount;
		uint64 totalFeeAmount;
		uint64 totalWinnerAmount;
		uint64 largestWinnerAmount;
		uint32 numberOfRegisters;
		uint32 numberOfProposals;
		uint32 numberOfQuRaffleMembers;
		uint32 numberOfActiveTokenRaffle;
		uint32 numberOfEndedTokenRaffle;
		uint32 numberOfEntryAmountSubmitted;
		sint32 returnCode;
	};

	struct getActiveProposal_input
	{
		uint32 indexOfProposal;
	};

	struct getActiveProposal_output
	{
		id tokenIssuer;
		id proposer;
		uint64 tokenName;
		uint64 entryAmount;
		uint32 nYes;
		uint32 nNo;
		sint32 returnCode;
	};

	struct getEndedTokenRaffle_input
	{
		uint32 indexOfRaffle;
	};

	struct getEndedTokenRaffle_output
	{
		id epochWinner;
		id tokenIssuer;
		uint64 tokenName;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint32 epoch;
		sint32 returnCode;
	};

	struct getEpochRaffleIndexes_input
	{
		uint32 epoch;
	};

	struct getEpochRaffleIndexes_output
	{
		uint32 StartIndex;
		uint32 EndIndex;
		sint32 returnCode;
	};

	struct getEndedQuRaffle_input
	{
		uint32 epoch;
	};

	struct getEndedQuRaffle_output
	{
		id epochWinner;
		uint64 receivedAmount;
		uint64 entryAmount;
		uint32 numberOfMembers;
		uint32 winnerIndex;
		uint32 numberOfDaoMembers;
		sint32 returnCode;
	};

	struct getActiveTokenRaffle_input
	{
		uint32 indexOfTokenRaffle;
	};

	struct getActiveTokenRaffle_output
	{
		id tokenIssuer;
		uint64 tokenName;
		uint64 entryAmount;
		uint32 numberOfMembers;
		sint32 returnCode;
	};

	struct getQuRaffleEntryAmountPerUser_input
	{
		id user;
	};

	struct getQuRaffleEntryAmountPerUser_output
	{
		uint64 entryAmount;
		sint32 returnCode;
	};

	struct getQuRaffleEntryAverageAmount_input
	{
	};

	struct getQuRaffleEntryAverageAmount_output
	{
		uint64 entryAverageAmount;
		sint32 returnCode;
	};

	// ── Asset Raffle I/O structs ───────────────────────────────────────────────

	struct createAssetRaffle_input
	{
		// Bundle: fixed-capacity QPI Array; only [0..bundleSize) are used.
		Array<AssetRaffleItem, QRAFFLE_MAX_ASSETS_PER_BUNDLE> bundleItems;
		uint32 bundleSize;
		uint64 reservePriceQu;  // min Qu creator wants AFTER 20% service fee
		uint64 entryTicketQu;   // Qu per ticket
	};

	struct createAssetRaffle_output
	{
		uint32 raffleIndex;
		sint32 returnCode;
	};

	struct buyAssetRaffleTicket_input
	{
		uint32 indexOfAssetRaffle;
		uint32 numberOfTickets;
	};

	struct buyAssetRaffleTicket_output
	{
		uint32 ticketsBought;
		sint32 returnCode;
	};

	struct cancelAssetRaffle_input
	{
		uint32 indexOfAssetRaffle;
	};

	struct cancelAssetRaffle_output
	{
		sint32 returnCode;
	};

	struct getActiveAssetRaffle_input
	{
		uint32 indexOfAssetRaffle;
	};

	struct getActiveAssetRaffle_output
	{
		id     creator;
		uint64 reservePriceQu;
		uint64 entryTicketQu;
		uint64 totalTicketsPaidQu;
		uint32 numberOfBuyers;
		uint32 totalTickets;
		uint32 bundleSize;
		uint32 epoch;
		sint32 returnCode;
	};

	struct getActiveAssetRaffleBundleItem_input
	{
		uint32 indexOfAssetRaffle;
		uint32 itemIndex;
	};

	struct getActiveAssetRaffleBundleItem_output
	{
		id     assetIssuer;
		uint64 assetName;
		sint64 numberOfShares;
		sint32 returnCode;
	};

	struct getActiveAssetRaffleBuyer_input
	{
		uint32 indexOfAssetRaffle;
		uint32 buyerIndex;
	};

	struct getActiveAssetRaffleBuyer_output
	{
		id     buyer;
		uint32 ticketCount;
		sint32 returnCode;
	};

	struct getEndedAssetRaffle_input
	{
		uint32 indexOfRaffle;
	};

	struct getEndedAssetRaffle_output
	{
		id     creator;
		id     epochWinner;
		uint64 reservePriceQu;
		uint64 entryTicketQu;
		uint64 grossPoolQu;
		uint64 creatorPaidQu;
		uint32 totalTickets;
		uint32 numberOfBuyers;
		uint32 bundleSize;
		uint32 epoch;
		uint8  reserveMet;
		sint32 returnCode;
	};

	struct getAssetRaffleAnalytics_input
	{
	};

	struct getAssetRaffleAnalytics_output
	{
		uint64 totalAssetRaffleProposalFees;
		uint64 totalAssetRaffleCreatorPaid;
		uint64 totalAssetRaffleRefunded;
		uint32 numberOfActiveAssetRaffles;
		uint32 numberOfEndedAssetRaffles;
		uint32 totalAssetRafflesCreated;
		uint32 totalAssetRafflesSucceeded;
		uint32 totalAssetRafflesFailed;
		sint32 returnCode;
	};

protected:


	struct registerInSystem_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(registerInSystem)
	{
		if (state.get().registers.contains(qpi.invocator()))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyRegistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().numberOfRegisters >= QRAFFLE_MAX_MEMBER)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (input.useQXMR)
		{
			// refund the invocation reward if the user uses QXMR for registration
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			// Use QXMR tokens for registration
			if (qpi.numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < QRAFFLE_QXMR_REGISTER_AMOUNT)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			// Transfer QXMR tokens to the contract
			if (qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, qpi.invocator(), qpi.invocator(), QRAFFLE_QXMR_REGISTER_AMOUNT, SELF) < 0)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			state.mut().registers.set(qpi.invocator(), 2);
		}
		else
		{
			// Use qubic for registration
			if (qpi.invocationReward() < QRAFFLE_REGISTER_AMOUNT)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_REGISTER_AMOUNT);
			state.mut().registers.set(qpi.invocator(), 1);
		}

		state.mut().numberOfRegisters++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct logoutInSystem_locals
	{
		sint64 refundAmount;
		uint8 tokenType;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(logoutInSystem)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (qpi.invocator() == state.get().initialRegister1 || qpi.invocator() == state.get().initialRegister2 || qpi.invocator() == state.get().initialRegister3 || qpi.invocator() == state.get().initialRegister4 || qpi.invocator() == state.get().initialRegister5)
		{
			output.returnCode = QRAFFLE_INITIAL_REGISTER_CANNOT_LOGOUT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_initialRegisterCannotLogout, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.get().registers.get(qpi.invocator(), locals.tokenType);

		if (locals.tokenType == 1)
		{
			// Use qubic for logout
			locals.refundAmount = QRAFFLE_REGISTER_AMOUNT - QRAFFLE_LOGOUT_FEE;
			qpi.transfer(qpi.invocator(), locals.refundAmount);
			state.mut().epochRevenue += QRAFFLE_LOGOUT_FEE;
		}
		else if (locals.tokenType == 2)
		{
			// Use QXMR tokens for logout
			locals.refundAmount = QRAFFLE_QXMR_REGISTER_AMOUNT - QRAFFLE_QXMR_LOGOUT_FEE;

			// Check if contract has enough QXMR tokens
			if (qpi.numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX) < locals.refundAmount)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			// Transfer QXMR tokens back to user
			if (qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, locals.refundAmount, qpi.invocator()) < 0)
			{
				output.returnCode = QRAFFLE_INSUFFICIENT_QXMR;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQXMR, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			state.mut().epochQXMRRevenue += QRAFFLE_QXMR_LOGOUT_FEE;
		}

		state.mut().registers.removeByKey(qpi.invocator());
		state.mut().numberOfRegisters--;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitEntryAmount_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitEntryAmount)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (input.amount < QRAFFLE_MIN_QRAFFLE_AMOUNT || input.amount > QRAFFLE_MAX_QRAFFLE_AMOUNT)
		{
			output.returnCode = QRAFFLE_INVALID_ENTRY_AMOUNT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidEntryAmount, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().quRaffleEntryAmount.contains(qpi.invocator()) == 0)
		{
			state.mut().numberOfEntryAmountSubmitted++;
		}
		state.mut().quRaffleEntryAmount.set(qpi.invocator(), input.amount);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_success, 0 };
		LOG_INFO(locals.log);
	}

	struct submitProposal_locals
	{
		ProposalInfo proposal;
		uint8 countThisEpoch;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(submitProposal)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (state.get().numberOfProposals >= QRAFFLE_MAX_PROPOSAL_EPOCH)
		{
			output.returnCode = QRAFFLE_MAX_PROPOSAL_EPOCH_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxProposalEpochReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.countThisEpoch = 0;
		if (state.get().proposalsPerProposer.contains(qpi.invocator()))
		{
			state.get().proposalsPerProposer.get(qpi.invocator(), locals.countThisEpoch);
		}
		if (locals.countThisEpoch >= QRAFFLE_MAX_PROPOSALS_PER_PROPOSER)
		{
			output.returnCode = QRAFFLE_MAX_PROPOSAL_PER_USER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxProposalPerUserReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.entryAmount < QRAFFLE_MIN_QRAFFLE_AMOUNT || input.entryAmount > QRAFFLE_MAX_QRAFFLE_AMOUNT)
		{
			output.returnCode = QRAFFLE_INVALID_ENTRY_AMOUNT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidEntryAmount, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (!qpi.isAssetIssued(input.tokenIssuer, input.tokenName))
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_TYPE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenType, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		// Reject internal tokens: QRAFFLE shares and QXMR are reserved for dividends/registration.
		if ((input.tokenName == QRAFFLE_ASSET_NAME && input.tokenIssuer == NULL_ID)
			|| (input.tokenName == QRAFFLE_QXMR_ASSET_NAME && input.tokenIssuer == state.get().QXMRIssuer))
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_TYPE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenType, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.proposal.token.issuer = input.tokenIssuer;
		locals.proposal.token.assetName = input.tokenName;
		locals.proposal.entryAmount = input.entryAmount;
		locals.proposal.proposer = qpi.invocator();
		state.mut().proposals.set(state.get().numberOfProposals, locals.proposal);
		state.mut().numberOfProposals++;
		state.mut().proposalsPerProposer.set(qpi.invocator(), locals.countThisEpoch + 1);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalSubmitted, 0 };
		LOG_INFO(locals.log);
	}

	struct voteInProposal_locals
	{
		ProposalInfo proposal;
		BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH> participation;
		BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH> values;
		uint32 votedCount;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(voteInProposal)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfProposal >= state.get().numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalNotFound, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.proposal = state.get().proposals.get(input.indexOfProposal);

		// O(1) vote lookup via per-user bitfield (id hash, no K12 overhead).
		state.get().voteParticipation.get(qpi.invocator(), locals.participation);
		if (locals.participation.get(input.indexOfProposal))
		{
			// Already voted — check if same direction.
			state.get().voteValues.get(qpi.invocator(), locals.values);
			if (locals.values.get(input.indexOfProposal) == input.yes)
			{
				output.returnCode = QRAFFLE_ALREADY_VOTED;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyVoted, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			// Flip the vote: update counters with underflow guard.
			if (input.yes)
			{
				locals.proposal.nYes++;
				if (locals.proposal.nNo > 0) { locals.proposal.nNo--; }
			}
			else
			{
				locals.proposal.nNo++;
				if (locals.proposal.nYes > 0) { locals.proposal.nYes--; }
			}
			state.mut().proposals.set(input.indexOfProposal, locals.proposal);
			locals.values.set(input.indexOfProposal, input.yes);
			state.mut().voteValues.replace(qpi.invocator(), locals.values);
			output.returnCode = QRAFFLE_SUCCESS;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalVoted, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		// New vote: check capacity before inserting.
		locals.votedCount = state.get().numberOfVotedInProposal.get(input.indexOfProposal);
		if (locals.votedCount >= QRAFFLE_MAX_MEMBER)
		{
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.yes)
		{
			locals.proposal.nYes++;
		}
		else
		{
			locals.proposal.nNo++;
		}
		state.mut().proposals.set(input.indexOfProposal, locals.proposal);

		// Mark participation and record vote direction.
		locals.participation.set(input.indexOfProposal, 1);
		state.mut().voteParticipation.set(qpi.invocator(), locals.participation);
		state.get().voteValues.get(qpi.invocator(), locals.values);
		locals.values.set(input.indexOfProposal, input.yes);
		state.mut().voteValues.set(qpi.invocator(), locals.values);

		state.mut().numberOfVotedInProposal.set(input.indexOfProposal, locals.votedCount + 1);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_proposalVoted, 0 };
		LOG_INFO(locals.log);
	}

	struct depositInQuRaffle_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositInQuRaffle)
	{
		if (state.get().numberOfQuRaffleMembers >= QRAFFLE_MAX_MEMBER)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReachedForQuRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (qpi.invocationReward() < (sint64)state.get().qREAmount)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		// O(1) duplicate check via HashSet (replaces former O(N) linear scan over quRaffleMembers).
		if (state.get().quRaffleMemberSet.contains(qpi.invocator()))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyRegistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().qREAmount);
		state.mut().quRaffleMembers.set(state.get().numberOfQuRaffleMembers, qpi.invocator());
		state.mut().quRaffleMemberSet.add(qpi.invocator());
		state.mut().numberOfQuRaffleMembers++;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_quRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct depositInTokenRaffle_locals
	{
		ActiveTokenRaffleInfo raffleInfo;
		BitArray<QRAFFLE_MAX_PROPOSAL_EPOCH> participation;
		uint32 currentMembers;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(depositInTokenRaffle)
	{
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		// Only registered members may deposit.
		if (state.get().registers.contains(qpi.invocator()) == 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		if (input.indexOfTokenRaffle >= state.get().numberOfActiveTokenRaffle)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenRaffle, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.currentMembers = state.get().numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle);
		if (locals.currentMembers >= QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_MAX_MEMBER_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxMemberReached, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		// O(1) duplicate check via per-user bitfield.
		state.get().tokenRaffleParticipation.get(qpi.invocator(), locals.participation);
		if (locals.participation.get(input.indexOfTokenRaffle))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_ALREADY_REGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_alreadyRegistered, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		locals.raffleInfo = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle);
		if (qpi.transferShareOwnershipAndPossession(locals.raffleInfo.token.assetName, locals.raffleInfo.token.issuer, qpi.invocator(), qpi.invocator(), locals.raffleInfo.entryAmount, SELF) < 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = QRAFFLE_FAILED_TO_DEPOSIT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_transferFailed, 0 };
			LOG_INFO(locals.log);
			return ;
		}
		// Keep QRAFFLE_TRANSFER_SHARE_FEE as service revenue; refund any excess.
		if (qpi.invocationReward() > QRAFFLE_TRANSFER_SHARE_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - QRAFFLE_TRANSFER_SHARE_FEE);
		}
		state.mut().epochRevenue += QRAFFLE_TRANSFER_SHARE_FEE;

		// Store member in flat slot array and mark participation.
		state.mut().tokenRaffleMemberSlots.set(input.indexOfTokenRaffle * QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE + locals.currentMembers, qpi.invocator());
		state.mut().numberOfTokenRaffleMembers.set(input.indexOfTokenRaffle, locals.currentMembers + 1);
		locals.participation.set(input.indexOfTokenRaffle, 1);
		state.mut().tokenRaffleParticipation.set(qpi.invocator(), locals.participation);
		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_tokenRaffleDeposited, 0 };
		LOG_INFO(locals.log);
	}

	struct TransferShareManagementRights_locals
	{
		Asset asset;
		sint64 offeredFee;
		sint64 paidFee;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		// Requires QRAFFLE_TRANSFER_SHARE_FEE minimum. The rest is offered to the destination
		// contract as transfer fee; the unused portion is refunded after releaseShares.
		if (qpi.invocationReward() < QRAFFLE_TRANSFER_SHARE_FEE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if (qpi.numberOfPossessedShares(input.tokenName, input.tokenIssuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// Not enough shares — refund in full.
			output.transferredNumberOfShares = 0;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_notEnoughShares, 0 };
			LOG_INFO(locals.log);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
		}
		else
		{
			locals.asset.assetName = input.tokenName;
			locals.asset.issuer = input.tokenIssuer;
			locals.offeredFee = qpi.invocationReward() - QRAFFLE_TRANSFER_SHARE_FEE;
			locals.paidFee = qpi.releaseShares(locals.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, locals.offeredFee);
			if (locals.paidFee < 0)
			{
				// Transfer rejected by the destination — refund everything.
				output.transferredNumberOfShares = 0;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_transferFailed, 0 };
				LOG_INFO(locals.log);
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
			}
			else
			{
				// Success — keep service fee as revenue, refund unused transfer fee.
				output.transferredNumberOfShares = input.numberOfShares;
				state.mut().epochRevenue += QRAFFLE_TRANSFER_SHARE_FEE;
				if (locals.offeredFee > locals.paidFee)
				{
					qpi.transfer(qpi.invocator(), locals.offeredFee - locals.paidFee);
				}
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_shareManagementRightsTransferred, 0 };
			LOG_INFO(locals.log);
			}
		}
	}

	// ── createAssetRaffle ──────────────────────────────────────────────────────
	struct createAssetRaffle_locals
	{
		AssetRaffleItem item;
		AssetRaffleItem dupItem;
		AssetRaffleItem rollbackItem;
		AssetRaffleInfo info;
		AssetRaffleCreatedLogger clog;
		Logger log;
		uint64 proposalFeeHalf;
		uint8  creatorCount;
		uint32 slot;
		uint32 i;
		uint32 j;
		sint64 escrowResult;
		bit    dupFound;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createAssetRaffle)
	{
		if (qpi.invocationReward() < (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE)
		{
			if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (!state.get().registers.contains(qpi.invocator()))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_UNREGISTERED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_unregistered, 0 };
			LOG_INFO(locals.log);
			return;
		}
		locals.creatorCount = 0;
		if (state.get().assetRafflesPerCreator.contains(qpi.invocator()))
		{
			state.get().assetRafflesPerCreator.get(qpi.invocator(), locals.creatorCount);
		}
		if (locals.creatorCount >= QRAFFLE_MAX_ASSET_RAFFLES_PER_CREATOR)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_MAX_ASSET_RAFFLES_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxAssetRafflesReached, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (state.get().numberOfActiveAssetRaffles >= QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_MAX_ASSET_RAFFLES_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_maxAssetRafflesReached, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (input.bundleSize == 0 || input.bundleSize > QRAFFLE_MAX_ASSETS_PER_BUNDLE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_INVALID_BUNDLE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidBundle, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (input.entryTicketQu < QRAFFLE_MIN_ASSET_TICKET_AMOUNT || input.entryTicketQu > QRAFFLE_MAX_ASSET_TICKET_AMOUNT)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_INVALID_ENTRY_AMOUNT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidEntryAmount, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (input.reservePriceQu == 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_INVALID_RESERVE_PRICE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidReservePrice, 0 };
			LOG_INFO(locals.log);
			return;
		}
		// Guard against uint64 overflow in the END_EPOCH reserve check (reservePriceQu * 100).
		if (input.reservePriceQu > div<uint64>(0xFFFFFFFFFFFFFFFFull, 100ull))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = QRAFFLE_INVALID_RESERVE_PRICE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidReservePrice, 0 };
			LOG_INFO(locals.log);
			return;
		}
		for (locals.i = 0; locals.i < input.bundleSize; locals.i++)
		{
			locals.item = input.bundleItems.get(locals.i);
			if (!qpi.isAssetIssued(locals.item.asset.issuer, locals.item.asset.assetName))
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				output.returnCode = QRAFFLE_INVALID_BUNDLE;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidBundle, 0 };
				LOG_INFO(locals.log);
				return;
			}
			if (locals.item.numberOfShares <= 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				output.returnCode = QRAFFLE_INVALID_BUNDLE;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidBundle, 0 };
				LOG_INFO(locals.log);
				return;
			}
			// QRAFFLE and QXMR are reserved for dividends/registration; disallow in bundles.
			if ((locals.item.asset.assetName == QRAFFLE_ASSET_NAME && locals.item.asset.issuer == NULL_ID)
				|| (locals.item.asset.assetName == QRAFFLE_QXMR_ASSET_NAME && locals.item.asset.issuer == state.get().QXMRIssuer))
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				output.returnCode = QRAFFLE_INVALID_BUNDLE;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidBundle, 0 };
				LOG_INFO(locals.log);
				return;
			}
			// Duplicate-asset check within the bundle; O(N²) acceptable for N ≤ 8.
			locals.dupFound = 0;
			for (locals.j = 0; locals.j < locals.i; locals.j++)
			{
				locals.dupItem = input.bundleItems.get(locals.j);
				if (locals.dupItem.asset.assetName == locals.item.asset.assetName
					&& locals.dupItem.asset.issuer == locals.item.asset.issuer)
				{
					locals.dupFound = 1;
					break;
				}
			}
			if (locals.dupFound)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				output.returnCode = QRAFFLE_INVALID_BUNDLE;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidBundle, 0 };
				LOG_INFO(locals.log);
				return;
			}
		}

		// Atomic escrow: if any item fails, roll back all previously transferred items.
		locals.slot = state.get().numberOfActiveAssetRaffles;
		for (locals.i = 0; locals.i < input.bundleSize; locals.i++)
		{
			locals.item = input.bundleItems.get(locals.i);
			locals.escrowResult = qpi.transferShareOwnershipAndPossession(
				locals.item.asset.assetName, locals.item.asset.issuer,
				qpi.invocator(), qpi.invocator(),
				locals.item.numberOfShares, SELF);
			if (locals.escrowResult < 0)
			{
				// Rollback: return all previously escrowed items to the invocator.
				// These transfers move shares we just received from the same invocator back to
				// them, so they should not fail in normal circumstances. If a rollback transfer
				// does fail (e.g. spectrum entry evicted), residual shares stay under contract
				// management for governance recovery.
				for (locals.j = 0; locals.j < locals.i; locals.j++)
				{
					locals.rollbackItem = input.bundleItems.get(locals.j);
					qpi.transferShareOwnershipAndPossession(
						locals.rollbackItem.asset.assetName, locals.rollbackItem.asset.issuer,
						SELF, SELF,
						locals.rollbackItem.numberOfShares, qpi.invocator());
				}
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				output.returnCode = QRAFFLE_BUNDLE_ESCROW_FAILED;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleBundleEscrowFailed, 0 };
				LOG_INFO(locals.log);
				return;
			}
			state.mut().activeAssetRaffleItems.set(locals.slot * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.i, locals.item);
		}

		locals.info.creator = qpi.invocator();
		locals.info.reservePriceQu = input.reservePriceQu;
		locals.info.entryTicketQu = input.entryTicketQu;
		locals.info.totalTicketsPaidQu = 0;
		locals.info.numberOfBuyers = 0;
		locals.info.totalTickets = 0;
		locals.info.bundleSize = input.bundleSize;
		locals.info.epoch = qpi.epoch();
		state.mut().activeAssetRaffles.set(locals.slot, locals.info);
		state.mut().numberOfActiveAssetRaffles++;
		state.mut().assetRafflesPerCreator.set(qpi.invocator(), locals.creatorCount + 1);

		// 50% of proposal fee → shareholders via epochRevenue; 50% → DAO bucket distributed in END_EPOCH.
		locals.proposalFeeHalf = div<uint64>(QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE, 2);
		state.mut().epochRevenue += locals.proposalFeeHalf;
		state.mut().epochAssetRaffleDaoBucket += (QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE - locals.proposalFeeHalf);
		state.mut().totalAssetRaffleProposalFees += QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE;
		state.mut().totalAssetRafflesCreated++;

		if (qpi.invocationReward() > (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)QRAFFLE_ASSET_RAFFLE_PROPOSAL_FEE);
		}

		output.raffleIndex = locals.slot;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.clog = AssetRaffleCreatedLogger{
			QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleCreated,
			locals.slot, qpi.invocator(),
			input.reservePriceQu, input.entryTicketQu,
			input.bundleSize, 0
		};
		LOG_INFO(locals.clog);
	}

	// ── buyAssetRaffleTicket ───────────────────────────────────────────────────
	struct buyAssetRaffleTicket_locals
	{
		AssetRaffleInfo info;
		AssetRaffleTicketLogger tlog;
		Logger log;
		BitArray<QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH> participation;
		Array<uint16, QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH> slotIndexArr;
		uint64 cost;
		uint32 baseSlot;
		uint32 existingTickets;
		uint32 buyerSlot;
		bit isNewBuyer;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(buyAssetRaffleTicket)
	{
		if (input.indexOfAssetRaffle >= state.get().numberOfActiveAssetRaffles)
		{
			if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenRaffle, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (input.numberOfTickets == 0 || input.numberOfTickets > QRAFFLE_MAX_TICKETS_PER_BUYER)
		{
			// Reject zero or absurd ticket counts up-front so the cost multiplication below cannot overflow.
			if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
			output.returnCode = QRAFFLE_INVALID_ENTRY_AMOUNT;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidEntryAmount, 0 };
			LOG_INFO(locals.log);
			return;
		}
		locals.info = state.get().activeAssetRaffles.get(input.indexOfAssetRaffle);
		// Overflow-safe cost: entryTicketQu ≤ 1e12 (QRAFFLE_MAX_ASSET_TICKET_AMOUNT) and tickets ≤ 100,
		// so cost ≤ 1e14, safely under uint64 max (~1.8e19). totalTicketsPaidQu accumulates across
		// up to 1024 buyers, max ~1e17, also safe.
		locals.cost = locals.info.entryTicketQu * (uint64)input.numberOfTickets;
		if (qpi.invocationReward() < (sint64)locals.cost)
		{
			if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
			output.returnCode = QRAFFLE_INSUFFICIENT_FUND;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_insufficientQubic, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.baseSlot = input.indexOfAssetRaffle * QRAFFLE_MAX_ASSET_TICKET_BUYERS;
		locals.existingTickets = 0;
		locals.isNewBuyer = 1;
		locals.buyerSlot = locals.baseSlot + locals.info.numberOfBuyers;

		state.get().assetRaffleParticipation.get(qpi.invocator(), locals.participation);
		if (locals.participation.get(input.indexOfAssetRaffle))
		{
			// Returning buyer: resolve slot via index map (O(1)).
			locals.isNewBuyer = 0;
			state.get().assetRaffleBuyerSlotIndex.get(qpi.invocator(), locals.slotIndexArr);
			locals.buyerSlot = locals.baseSlot + (uint32)locals.slotIndexArr.get(input.indexOfAssetRaffle);
			locals.existingTickets = state.get().activeAssetRaffleBuyerTickets.get(locals.buyerSlot);
		}

		if (locals.existingTickets + input.numberOfTickets > QRAFFLE_MAX_TICKETS_PER_BUYER)
		{
			if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
			output.returnCode = QRAFFLE_TICKET_LIMIT_REACHED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_ticketLimitReached, 0 };
			LOG_INFO(locals.log);
			return;
		}

		if (locals.isNewBuyer)
		{
			if (locals.info.numberOfBuyers >= QRAFFLE_MAX_ASSET_TICKET_BUYERS)
			{
				if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }
				output.returnCode = QRAFFLE_ASSET_RAFFLE_FULL;
				locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleFull, 0 };
				LOG_INFO(locals.log);
				return;
			}
			state.mut().activeAssetRaffleBuyers.set(locals.buyerSlot, qpi.invocator());
			// Record slot position so future purchases skip the buyer-list scan.
			state.get().assetRaffleBuyerSlotIndex.get(qpi.invocator(), locals.slotIndexArr);
			locals.slotIndexArr.set(input.indexOfAssetRaffle, (uint16)locals.info.numberOfBuyers);
			state.mut().assetRaffleBuyerSlotIndex.set(qpi.invocator(), locals.slotIndexArr);
			locals.participation.set(input.indexOfAssetRaffle, 1);
			state.mut().assetRaffleParticipation.set(qpi.invocator(), locals.participation);
			locals.info.numberOfBuyers++;
		}

		state.mut().activeAssetRaffleBuyerTickets.set(locals.buyerSlot, locals.existingTickets + input.numberOfTickets);
		locals.info.totalTickets += input.numberOfTickets;
		locals.info.totalTicketsPaidQu += locals.cost;
		state.mut().activeAssetRaffles.set(input.indexOfAssetRaffle, locals.info);

		if (qpi.invocationReward() > (sint64)locals.cost)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)locals.cost);
		}

		output.ticketsBought = input.numberOfTickets;
		output.returnCode = QRAFFLE_SUCCESS;
		locals.tlog = AssetRaffleTicketLogger{
			QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleTicketBought,
			input.indexOfAssetRaffle, qpi.invocator(),
			input.numberOfTickets, locals.cost, 0
		};
		LOG_INFO(locals.tlog);
	}

	// ── cancelAssetRaffle ──────────────────────────────────────────────────────
	struct cancelAssetRaffle_locals
	{
		AssetRaffleInfo info;
		AssetRaffleItem item;
		AssetRaffleInfo lastInfo;
		AssetRaffleItem lastItem;
		Array<uint16, QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH> slotIndexArr;
		BitArray<QRAFFLE_MAX_ASSET_RAFFLES_PER_EPOCH> participation;
		Logger log;
		id     movedBuyer;
		uint8  creatorCount;
		uint32 lastSlot;
		uint32 i;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelAssetRaffle)
	{
		if (qpi.invocationReward() > 0) { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }

		if (input.indexOfAssetRaffle >= state.get().numberOfActiveAssetRaffles)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_invalidTokenRaffle, 0 };
			LOG_INFO(locals.log);
			return;
		}
		locals.info = state.get().activeAssetRaffles.get(input.indexOfAssetRaffle);
		if (locals.info.creator != qpi.invocator())
		{
			output.returnCode = QRAFFLE_CANCEL_NOT_ALLOWED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_cancelNotAllowed, 0 };
			LOG_INFO(locals.log);
			return;
		}
		if (locals.info.numberOfBuyers > 0)
		{
			output.returnCode = QRAFFLE_CANCEL_NOT_ALLOWED;
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_cancelNotAllowed, 0 };
			LOG_INFO(locals.log);
			return;
		}

		for (locals.i = 0; locals.i < locals.info.bundleSize; locals.i++)
		{
			locals.item = state.get().activeAssetRaffleItems.get(input.indexOfAssetRaffle * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.i);
			qpi.transferShareOwnershipAndPossession(
				locals.item.asset.assetName, locals.item.asset.issuer,
				SELF, SELF,
				locals.item.numberOfShares, qpi.invocator());
		}

		// Swap-and-pop: fill the vacated slot with the last active raffle.
		// Three parallel arrays must be kept in sync: raffle info, bundle items, and buyer lists.
		locals.lastSlot = state.get().numberOfActiveAssetRaffles - 1;
		if (input.indexOfAssetRaffle < locals.lastSlot)
		{
			locals.lastInfo = state.get().activeAssetRaffles.get(locals.lastSlot);
			state.mut().activeAssetRaffles.set(input.indexOfAssetRaffle, locals.lastInfo);

			for (locals.i = 0; locals.i < locals.lastInfo.bundleSize; locals.i++)
			{
				locals.lastItem = state.get().activeAssetRaffleItems.get(locals.lastSlot * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.i);
				state.mut().activeAssetRaffleItems.set(input.indexOfAssetRaffle * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.i, locals.lastItem);
			}

			// Cancelled raffle already has numberOfBuyers==0, so the destination region is safe to overwrite.
			for (locals.i = 0; locals.i < locals.lastInfo.numberOfBuyers; locals.i++)
			{
				locals.movedBuyer = state.get().activeAssetRaffleBuyers.get(locals.lastSlot * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.i);
				state.mut().activeAssetRaffleBuyers.set(
					input.indexOfAssetRaffle * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.i,
					locals.movedBuyer);
				state.mut().activeAssetRaffleBuyerTickets.set(
					input.indexOfAssetRaffle * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.i,
					state.get().activeAssetRaffleBuyerTickets.get(locals.lastSlot * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.i));
				// Update slot-index map: raffle index changed from lastSlot to indexOfAssetRaffle;
				// relative position within the buyer region is preserved.
				state.get().assetRaffleBuyerSlotIndex.get(locals.movedBuyer, locals.slotIndexArr);
				locals.slotIndexArr.set(input.indexOfAssetRaffle, locals.slotIndexArr.get(locals.lastSlot));
				locals.slotIndexArr.set(locals.lastSlot, 0xFFFF);
				state.mut().assetRaffleBuyerSlotIndex.replace(locals.movedBuyer, locals.slotIndexArr);

				// Keep participation bitfield aligned with moved raffle index.
				state.get().assetRaffleParticipation.get(locals.movedBuyer, locals.participation);
				locals.participation.set(input.indexOfAssetRaffle, 1);
				locals.participation.set(locals.lastSlot, 0);
				state.mut().assetRaffleParticipation.replace(locals.movedBuyer, locals.participation);
			}
		}
		state.mut().numberOfActiveAssetRaffles--;

		// Decrement per-creator counter so the creator can replace the cancelled raffle
		// within the same epoch. The 500K proposal fee is *not* refunded (anti-spam).
		locals.creatorCount = 0;
		if (state.get().assetRafflesPerCreator.contains(qpi.invocator()))
		{
			state.get().assetRafflesPerCreator.get(qpi.invocator(), locals.creatorCount);
			if (locals.creatorCount > 0)
			{
				state.mut().assetRafflesPerCreator.set(qpi.invocator(), (uint8)(locals.creatorCount - 1));
			}
		}

		output.returnCode = QRAFFLE_SUCCESS;
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleCancelled, 0 };
		LOG_INFO(locals.log);
	}

	struct getRegisters_locals
	{
		id user;
		sint64 idx;
		uint32 i;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getRegisters)
	{
		if (input.limit > 20)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return ;
		}
		if (input.offset >= state.get().numberOfRegisters)
		{
			output.returnCode = QRAFFLE_SUCCESS;
			return ;
		}
		locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.user = state.get().registers.key(locals.idx);
			if (locals.i >= input.offset && locals.i < input.offset + input.limit)
			{
				if (locals.i - input.offset == 0)
				{
					output.register1 = locals.user;
				}
				else if (locals.i - input.offset == 1)
				{
					output.register2 = locals.user;
				}
				else if (locals.i - input.offset == 2)
				{
					output.register3 = locals.user;
				}
				else if (locals.i - input.offset == 3)
				{
					output.register4 = locals.user;
				}
				else if (locals.i - input.offset == 4)
				{
					output.register5 = locals.user;
				}
				else if (locals.i - input.offset == 5)
				{
					output.register6 = locals.user;
				}
				else if (locals.i - input.offset == 6)
				{
					output.register7 = locals.user;
				}
				else if (locals.i - input.offset == 7)
				{
					output.register8 = locals.user;
				}
				else if (locals.i - input.offset == 8)
				{
					output.register9 = locals.user;
				}
				else if (locals.i - input.offset == 9)
				{
					output.register10 = locals.user;
				}
				else if (locals.i - input.offset == 10)
				{
					output.register11 = locals.user;
				}
				else if (locals.i - input.offset == 11)
				{
					output.register12 = locals.user;
				}
				else if (locals.i - input.offset == 12)
				{
					output.register13 = locals.user;
				}
				else if (locals.i - input.offset == 13)
				{
					output.register14 = locals.user;
				}
				else if (locals.i - input.offset == 14)
				{
					output.register15 = locals.user;
				}
				else if (locals.i - input.offset == 15)
				{
					output.register16 = locals.user;
				}
				else if (locals.i - input.offset == 16)
				{
					output.register17 = locals.user;
				}
				else if (locals.i - input.offset == 17)
				{
					output.register18 = locals.user;
				}
				else if (locals.i - input.offset == 18)
				{
					output.register19 = locals.user;
				}
				else if (locals.i - input.offset == 19)
				{
					output.register20 = locals.user;
				}
			}
			if (locals.i >= input.offset + input.limit)
			{
				break;
			}
			locals.i++;
			locals.idx = state.get().registers.nextElementIndex(locals.idx);
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getAnalytics)
	{
		output.currentQuRaffleAmount = state.get().qREAmount;
		output.totalBurnAmount = state.get().totalBurnAmount;
		output.totalCharityAmount = state.get().totalCharityAmount;
		output.totalShareholderAmount = state.get().totalShareholderAmount;
		output.totalRegisterAmount = state.get().totalRegisterAmount;
		output.totalFeeAmount = state.get().totalFeeAmount;
		output.totalWinnerAmount = state.get().totalWinnerAmount;
		output.largestWinnerAmount = state.get().largestWinnerAmount;
		output.numberOfRegisters = state.get().numberOfRegisters;
		output.numberOfProposals = state.get().numberOfProposals;
		output.numberOfQuRaffleMembers = state.get().numberOfQuRaffleMembers;
		output.numberOfActiveTokenRaffle = state.get().numberOfActiveTokenRaffle;
		output.numberOfEndedTokenRaffle = state.get().numberOfEndedTokenRaffle;
		output.numberOfEntryAmountSubmitted = state.get().numberOfEntryAmountSubmitted;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getActiveProposal)
	{
		if (input.indexOfProposal >= state.get().numberOfProposals)
		{
			output.returnCode = QRAFFLE_INVALID_PROPOSAL;
			return ;
		}
		output.tokenName = state.get().proposals.get(input.indexOfProposal).token.assetName;
		output.tokenIssuer = state.get().proposals.get(input.indexOfProposal).token.issuer;
		output.proposer = state.get().proposals.get(input.indexOfProposal).proposer;
		output.entryAmount = state.get().proposals.get(input.indexOfProposal).entryAmount;
		output.nYes = state.get().proposals.get(input.indexOfProposal).nYes;
		output.nNo = state.get().proposals.get(input.indexOfProposal).nNo;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getEndedTokenRaffle)
	{
		if (input.indexOfRaffle >= state.get().numberOfEndedTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		// Reject indices that have been overwritten by the ring buffer.
		if (state.get().numberOfEndedTokenRaffle > QRAFFLE_MAX_TOKEN_RAFFLES
			&& input.indexOfRaffle < state.get().numberOfEndedTokenRaffle - QRAFFLE_MAX_TOKEN_RAFFLES)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		output.epochWinner = state.get().tokenRaffle.get(input.indexOfRaffle).epochWinner;
		output.tokenName = state.get().tokenRaffle.get(input.indexOfRaffle).token.assetName;
		output.tokenIssuer = state.get().tokenRaffle.get(input.indexOfRaffle).token.issuer;
		output.entryAmount = state.get().tokenRaffle.get(input.indexOfRaffle).entryAmount;
		output.numberOfMembers = state.get().tokenRaffle.get(input.indexOfRaffle).numberOfMembers;
		output.winnerIndex = state.get().tokenRaffle.get(input.indexOfRaffle).winnerIndex;
		output.epoch = state.get().tokenRaffle.get(input.indexOfRaffle).epoch;
		output.returnCode = QRAFFLE_SUCCESS;
	}

	struct getEpochRaffleIndexes_locals
	{
		uint32 ringStart;
		uint32 ringEnd;
		uint32 i;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getEpochRaffleIndexes)
	{
		output.StartIndex = 0;
		output.EndIndex = 0;
		if (input.epoch > qpi.epoch())
		{
			output.returnCode = QRAFFLE_INVALID_EPOCH;
			return ;
		}
		if (input.epoch == qpi.epoch())
		{
			output.StartIndex = 0;
			output.EndIndex = state.get().numberOfActiveTokenRaffle;
			output.returnCode = QRAFFLE_SUCCESS;
			return ;
		}
		// Only scan the valid ring window to avoid re-reading overwritten slots.
		locals.ringEnd = state.get().numberOfEndedTokenRaffle;
		locals.ringStart = (locals.ringEnd > QRAFFLE_MAX_TOKEN_RAFFLES) ? (locals.ringEnd - QRAFFLE_MAX_TOKEN_RAFFLES) : 0;
		for (locals.i = locals.ringStart; locals.i < locals.ringEnd; locals.i++)
		{
			if (state.get().tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.StartIndex = locals.i;
				break;
			}
		}
		locals.i = locals.ringEnd;
		while (locals.i > locals.ringStart)
		{
			locals.i--;
			if (state.get().tokenRaffle.get(locals.i).epoch == input.epoch)
			{
				output.EndIndex = locals.i;
				break;
			}
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getEndedQuRaffle)
	{
		// Note: indices are masked by the underlying Array (capacity is power-of-two), so
		// any uint32 epoch value is safely bounded inside the QuRaffles ring. Slots that
		// were never written return a zero-initialised QuRaffleInfo, which the caller can
		// detect via numberOfMembers == 0 / epochWinner == NULL_ID.
		output.epochWinner = state.get().QuRaffles.get(input.epoch).epochWinner;
		output.receivedAmount = state.get().QuRaffles.get(input.epoch).receivedAmount;
		output.entryAmount = state.get().QuRaffles.get(input.epoch).entryAmount;
		output.numberOfMembers = state.get().QuRaffles.get(input.epoch).numberOfMembers;
		output.winnerIndex = state.get().QuRaffles.get(input.epoch).winnerIndex;
		output.numberOfDaoMembers = state.get().daoMemberCount.get(input.epoch);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getActiveTokenRaffle)
	{
		if (input.indexOfTokenRaffle >= state.get().numberOfActiveTokenRaffle)
		{
			output.returnCode = QRAFFLE_INVALID_TOKEN_RAFFLE;
			return ;
		}
		output.tokenName = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.assetName;
		output.tokenIssuer = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).token.issuer;
		output.entryAmount = state.get().activeTokenRaffle.get(input.indexOfTokenRaffle).entryAmount;
		output.numberOfMembers = state.get().numberOfTokenRaffleMembers.get(input.indexOfTokenRaffle);
		output.returnCode = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getQuRaffleEntryAmountPerUser)
	{
		if (state.get().quRaffleEntryAmount.contains(input.user) == 0)
		{
			output.entryAmount = 0;
			output.returnCode = QRAFFLE_USER_NOT_FOUND;
		}
		else
		{
			state.get().quRaffleEntryAmount.get(input.user, output.entryAmount);
			output.returnCode = QRAFFLE_SUCCESS;
		}
	}

	struct getQuRaffleEntryAverageAmount_locals
	{
		uint64 entryAmount;
		uint64 totalEntryAmount;
		sint64 idx;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getQuRaffleEntryAverageAmount)
	{
		locals.entryAmount = 0;
		locals.totalEntryAmount = 0;
		locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.entryAmount = state.get().quRaffleEntryAmount.value(locals.idx);
			locals.totalEntryAmount += locals.entryAmount;
			locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		if (state.get().numberOfEntryAmountSubmitted > 0)
		{
			output.entryAverageAmount = div<uint64>(locals.totalEntryAmount, state.get().numberOfEntryAmountSubmitted);
		}
		else
		{
			output.entryAverageAmount = 0;
		}
		output.returnCode = QRAFFLE_SUCCESS;
	}

	// ── Asset Raffle view functions ────────────────────────────────────────────

	struct getActiveAssetRaffle_locals
	{
		AssetRaffleInfo info;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getActiveAssetRaffle)
	{
		if (input.indexOfAssetRaffle >= state.get().numberOfActiveAssetRaffles)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			return;
		}
		locals.info = state.get().activeAssetRaffles.get(input.indexOfAssetRaffle);
		output.creator            = locals.info.creator;
		output.reservePriceQu     = locals.info.reservePriceQu;
		output.entryTicketQu      = locals.info.entryTicketQu;
		output.totalTicketsPaidQu = locals.info.totalTicketsPaidQu;
		output.numberOfBuyers     = locals.info.numberOfBuyers;
		output.totalTickets       = locals.info.totalTickets;
		output.bundleSize         = locals.info.bundleSize;
		output.epoch              = locals.info.epoch;
		output.returnCode         = QRAFFLE_SUCCESS;
	}

	struct getActiveAssetRaffleBundleItem_locals
	{
		AssetRaffleInfo info;
		AssetRaffleItem item;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getActiveAssetRaffleBundleItem)
	{
		if (input.indexOfAssetRaffle >= state.get().numberOfActiveAssetRaffles)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			return;
		}
		locals.info = state.get().activeAssetRaffles.get(input.indexOfAssetRaffle);
		if (input.itemIndex >= locals.info.bundleSize)
		{
			output.returnCode = QRAFFLE_INVALID_BUNDLE;
			return;
		}
		locals.item = state.get().activeAssetRaffleItems.get(
			input.indexOfAssetRaffle * QRAFFLE_MAX_ASSETS_PER_BUNDLE + input.itemIndex);
		output.assetIssuer    = locals.item.asset.issuer;
		output.assetName      = locals.item.asset.assetName;
		output.numberOfShares = locals.item.numberOfShares;
		output.returnCode     = QRAFFLE_SUCCESS;
	}

	struct getActiveAssetRaffleBuyer_locals
	{
		AssetRaffleInfo info;
		uint32 slot;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getActiveAssetRaffleBuyer)
	{
		if (input.indexOfAssetRaffle >= state.get().numberOfActiveAssetRaffles)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			return;
		}
		locals.info = state.get().activeAssetRaffles.get(input.indexOfAssetRaffle);
		if (input.buyerIndex >= locals.info.numberOfBuyers)
		{
			output.returnCode = QRAFFLE_INVALID_OFFSET_OR_LIMIT;
			return;
		}
		locals.slot = input.indexOfAssetRaffle * QRAFFLE_MAX_ASSET_TICKET_BUYERS + input.buyerIndex;
		output.buyer       = state.get().activeAssetRaffleBuyers.get(locals.slot);
		output.ticketCount = state.get().activeAssetRaffleBuyerTickets.get(locals.slot);
		output.returnCode  = QRAFFLE_SUCCESS;
	}

	struct getEndedAssetRaffle_locals
	{
		EndedAssetRaffleInfo r;
		uint32 slot;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getEndedAssetRaffle)
	{
		if (input.indexOfRaffle >= state.get().numberOfEndedAssetRaffles)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			return;
		}
		// Reject indices that have been overwritten by the ring buffer.
		if (state.get().numberOfEndedAssetRaffles > QRAFFLE_MAX_ENDED_ASSET_RAFFLES
			&& input.indexOfRaffle < state.get().numberOfEndedAssetRaffles - QRAFFLE_MAX_ENDED_ASSET_RAFFLES)
		{
			output.returnCode = QRAFFLE_INVALID_ASSET_RAFFLE;
			return;
		}
		locals.slot = mod<uint32>(input.indexOfRaffle, QRAFFLE_MAX_ENDED_ASSET_RAFFLES);
		locals.r = state.get().endedAssetRaffles.get(locals.slot);
		output.creator        = locals.r.creator;
		output.epochWinner    = locals.r.epochWinner;
		output.reservePriceQu = locals.r.reservePriceQu;
		output.entryTicketQu  = locals.r.entryTicketQu;
		output.grossPoolQu    = locals.r.grossPoolQu;
		output.creatorPaidQu  = locals.r.creatorPaidQu;
		output.totalTickets   = locals.r.totalTickets;
		output.numberOfBuyers = locals.r.numberOfBuyers;
		output.bundleSize     = locals.r.bundleSize;
		output.epoch          = locals.r.epoch;
		output.reserveMet     = locals.r.reserveMet;
		output.returnCode     = QRAFFLE_SUCCESS;
	}

	PUBLIC_FUNCTION(getAssetRaffleAnalytics)
	{
		output.totalAssetRaffleProposalFees = state.get().totalAssetRaffleProposalFees;
		output.totalAssetRaffleCreatorPaid  = state.get().totalAssetRaffleCreatorPaid;
		output.totalAssetRaffleRefunded     = state.get().totalAssetRaffleRefunded;
		output.numberOfActiveAssetRaffles   = state.get().numberOfActiveAssetRaffles;
		output.numberOfEndedAssetRaffles    = state.get().numberOfEndedAssetRaffles;
		output.totalAssetRafflesCreated     = state.get().totalAssetRafflesCreated;
		output.totalAssetRafflesSucceeded   = state.get().totalAssetRafflesSucceeded;
		output.totalAssetRafflesFailed      = state.get().totalAssetRafflesFailed;
		output.returnCode                   = QRAFFLE_SUCCESS;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(getRegisters, 1);
		REGISTER_USER_FUNCTION(getAnalytics, 2);
		REGISTER_USER_FUNCTION(getActiveProposal, 3);
		REGISTER_USER_FUNCTION(getEndedTokenRaffle, 4);
		REGISTER_USER_FUNCTION(getEndedQuRaffle, 5);
		REGISTER_USER_FUNCTION(getActiveTokenRaffle, 6);
		REGISTER_USER_FUNCTION(getEpochRaffleIndexes, 7);
		REGISTER_USER_FUNCTION(getQuRaffleEntryAmountPerUser, 8);
		REGISTER_USER_FUNCTION(getQuRaffleEntryAverageAmount, 9);
    // Asset Raffle view functions
		REGISTER_USER_FUNCTION(getActiveAssetRaffle, 10);
		REGISTER_USER_FUNCTION(getActiveAssetRaffleBundleItem, 11);
		REGISTER_USER_FUNCTION(getActiveAssetRaffleBuyer, 12);
		REGISTER_USER_FUNCTION(getEndedAssetRaffle, 13);
		REGISTER_USER_FUNCTION(getAssetRaffleAnalytics, 14);

		REGISTER_USER_PROCEDURE(registerInSystem, 1);
		REGISTER_USER_PROCEDURE(logoutInSystem, 2);
		REGISTER_USER_PROCEDURE(submitEntryAmount, 3);
		REGISTER_USER_PROCEDURE(submitProposal, 4);
		REGISTER_USER_PROCEDURE(voteInProposal, 5);
		REGISTER_USER_PROCEDURE(depositInQuRaffle, 6);
		REGISTER_USER_PROCEDURE(depositInTokenRaffle, 7);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 8);
		// Asset Raffle procedures
		REGISTER_USER_PROCEDURE(createAssetRaffle, 9);
		REGISTER_USER_PROCEDURE(buyAssetRaffleTicket, 10);
		REGISTER_USER_PROCEDURE(cancelAssetRaffle, 11);
	}

	INITIALIZE()
	{
		state.mut().qREAmount = QRAFFLE_DEFAULT_QRAFFLE_AMOUNT;
		state.mut().charityAddress = ID(_D, _P, _Q, _R, _L, _S, _Z, _S, _S, _C, _X, _I, _Y, _F, _I, _Q, _G, _B, _F, _B, _X, _X, _I, _S, _D, _D, _E, _B, _E, _G, _Q, _N, _W, _N, _T, _Q, _U, _E, _I, _F, _S, _C, _U, _W, _G, _H, _V, _X, _J, _P, _L, _F, _G, _M, _Y, _D);
		state.mut().initialRegister1 = ID(_I, _L, _N, _J, _X, _V, _H, _A, _U, _X, _D, _G, _G, _B, _T, _T, _U, _O, _I, _T, _O, _Q, _G, _P, _A, _Y, _U, _C, _F, _T, _N, _C, _P, _X, _D, _K, _O, _C, _P, _U, _O, _C, _D, _O, _T, _P, _U, _W, _X, _B, _I, _G, _R, _V, _Q, _D);
		state.mut().initialRegister2 = ID(_L, _S, _D, _A, _A, _C, _L, _X, _X, _G, _I, _P, _G, _G, _L, _S, _O, _C, _L, _M, _V, _A, _Y, _L, _N, _T, _G, _D, _V, _B, _N, _O, _S, _S, _Y, _E, _Q, _D, _R, _K, _X, _D, _Y, _W, _B, _C, _G, _J, _I, _K, _C, _M, _Z, _K, _M, _F);
		state.mut().initialRegister3 = ID(_G, _H, _G, _R, _L, _W, _S, _X, _Z, _X, _W, _D, _A, _A, _O, _M, _T, _X, _Q, _Y, _U, _P, _R, _L, _P, _N, _K, _C, _W, _G, _H, _A, _E, _F, _I, _R, _J, _I, _Z, _A, _K, _C, _A, _U, _D, _G, _N, _M, _C, _D, _E, _Q, _R, _O, _Q, _B);
		state.mut().initialRegister4 = ID(_E, _U, _O, _N, _A, _Z, _J, _U, _A, _G, _V, _D, _C, _E, _I, _B, _A, _H, _J, _E, _T, _G, _U, _U, _H, _M, _N, _D, _J, _C, _S, _E, _T, _T, _Q, _V, _G, _Y, _F, _H, _M, _D, _P, _X, _T, _A, _L, _D, _Y, _U, _V, _E, _P, _F, _C, _A);
		state.mut().initialRegister5 = ID(_S, _L, _C, _J, _C, _C, _U, _X, _G, _K, _N, _V, _A, _D, _F, _B, _E, _A, _Y, _V, _L, _S, _O, _B, _Z, _P, _A, _B, _H, _K, _S, _G, _M, _H, _W, _H, _S, _H, _G, _G, _B, _A, _P, _J, _W, _F, _V, _O, _K, _Z, _J, _P, _F, _L, _X, _D);
		state.mut().QXMRIssuer = ID(_Q, _X, _M, _R, _T, _K, _A, _I, _I, _G, _L, _U, _R, _E, _P, _I, _Q, _P, _C, _M, _H, _C, _K, _W, _S, _I, _P, _D, _T, _U, _Y, _F, _C, _F, _N, _Y, _X, _Q, _L, _T, _E, _C, _S, _U, _J, _V, _Y, _E, _M, _M, _D, _E, _L, _B, _M, _D);
		state.mut().feeAddress = ID(_H, _H, _R, _L, _C, _Z, _Q, _V, _G, _O, _M, _G, _X, _G, _F, _P, _H, _T, _R, _H, _H, _D, _W, _A, _E, _U, _X, _C, _N, _D, _L, _Z, _S, _Z, _J, _R, _M, _O, _R, _J, _K, _A, _I, _W, _S, _U, _Y, _R, _N, _X, _I, _H, _H, _O, _W, _D);

		state.mut().registers.set(state.get().initialRegister1, 0);
		state.mut().registers.set(state.get().initialRegister2, 0);
		state.mut().registers.set(state.get().initialRegister3, 0);
		state.mut().registers.set(state.get().initialRegister4, 0);
		state.mut().registers.set(state.get().initialRegister5, 0);
		state.mut().numberOfRegisters = 5;
	}

	struct END_EPOCH_locals
	{
		ProposalInfo proposal;
		QuRaffleInfo qraffle;
		TokenRaffleInfo tRaffle;
		ActiveTokenRaffleInfo acTokenRaffle;
		AssetPossessionIterator iter;
		Asset QraffleAsset;
		id digest, computerDigest, winner, shareholder, baseSeed, raffleSeed;
		sint64 idx;
		sint64 sharesHeld;
		sint64 perShare;
		sint64 transferResult;
		uint64 sumOfEntryAmountSubmitted, r, winnerRevenue, burnAmount, charityRevenue, shareholderRevenue, registerRevenue, fee, oneShareholderRev;
		uint64 tokenPool;
		uint64 shareholderPerShareUnit;
		uint64 registerPerShareUnit;
		uint64 actualShareholderTotal;
		uint64 actualRegisterTotal;
		uint64 qxmrPerShare;
		uint64 qxmrDistributedTotal;
		uint64 qxmrContractBalance;
		uint32 i, j, winnerIndex;
		Logger log;
		EmptyTokenRaffleLogger emptyTokenRafflelog;
		EndEpochLogger endEpochLog;
		RevenueLogger revenueLog;
		TokenRaffleLogger tokenRaffleLog;
		ProposalLogger proposalLog;
		// Asset raffle settlement locals
		AssetRaffleInfo arInfo;
		AssetRaffleItem arItem;
		EndedAssetRaffleInfo arEnded;
		AssetRaffleEndedLogger arLog;
		uint64 arGross;
		uint64 arCreatorPay;
		uint64 arBurn;
		uint64 arCharity;
		uint64 arShareholderRev;
		uint64 arRegisterRev;
		uint64 arFee;
		uint64 arShareholderPerShare;
		uint64 arRegisterPerShare;
		uint64 arRegisterPerShareActual;
		uint64 arRegisterBucket;        // accumulated register share across all successful asset raffles
		uint64 arRegisterBucketPerReg;  // per-register amount distributed after the settlement loop
		uint64 arDaoBucketPerRegister;
		uint64 arTicketAcc;
		uint64 arRefund;                // per-buyer refund amount (used in reserve-missed path)
		uint32 arI;
		uint32 arJ;
		uint32 arBuyerSlot;
		uint32 arWinnerIndex;
		uint32 arEndedIdx;        // ring-buffer slot (masked) the settled raffle is written to
		uint32 arEndedGlobalIdx;  // monotonic global index callers pass to getEndedAssetRaffle
		bit    arReserveMet;
	};

	END_EPOCH_WITH_LOCALS()
	{
		// Distribute logout-fee revenue to shareholders.
		locals.oneShareholderRev = div<uint64>(state.get().epochRevenue, NUMBER_OF_COMPUTORS);
		if (locals.oneShareholderRev > 0)
		{
			qpi.distributeDividends(locals.oneShareholderRev);
			state.mut().epochRevenue -= locals.oneShareholderRev * NUMBER_OF_COMPUTORS;
		}

		// RNG seed: XOR of prevSpectrumDigest and prevComputerDigest, hashed with K12.
		// Tick-level inputs are excluded so a computor cannot grind the seed.
		locals.digest = qpi.getPrevSpectrumDigest();
		locals.computerDigest = qpi.getPrevComputerDigest();
		locals.baseSeed = qpi.K12(m256i(
			locals.digest.u64._0 ^ locals.computerDigest.u64._0,
			locals.digest.u64._1 ^ locals.computerDigest.u64._1,
			locals.digest.u64._2 ^ locals.computerDigest.u64._2,
			locals.digest.u64._3 ^ locals.computerDigest.u64._3));

		// Asset descriptor reused by the QXMR distribution and per-token-raffle shareholder
		// payout loops below; both iterate QRAFFLE_ASSET possessors directly via locals.iter.
		locals.QraffleAsset.assetName = QRAFFLE_ASSET_NAME;
		locals.QraffleAsset.issuer = NULL_ID;

		if (state.get().numberOfQuRaffleMembers > 0)
		{
			// Pick winner.
			locals.raffleSeed = qpi.K12(m256i(locals.baseSeed.u64._0, locals.baseSeed.u64._1, locals.baseSeed.u64._2, locals.baseSeed.u64._3));
			locals.r = locals.raffleSeed.u64._0;
			locals.winnerIndex = (uint32)mod(locals.r, state.get().numberOfQuRaffleMembers * 1ull);
			locals.winner = state.get().quRaffleMembers.get(locals.winnerIndex);

			// Calculate fee distributions.
			locals.tokenPool = state.get().qREAmount * state.get().numberOfQuRaffleMembers;
			locals.burnAmount = div<uint64>(locals.tokenPool * QRAFFLE_BURN_FEE, 100);
			locals.charityRevenue = div<uint64>(locals.tokenPool * QRAFFLE_CHARITY_FEE, 100);
			locals.shareholderRevenue = div<uint64>(locals.tokenPool * QRAFFLE_SHAREHOLDER_FEE, 100);
			locals.registerRevenue = div<uint64>(locals.tokenPool * QRAFFLE_REGISTER_FEE, 100);
			locals.fee = div<uint64>(locals.tokenPool * QRAFFLE_FEE, 100);
			// Round down per-share amounts; winner gets the remainder.
			locals.shareholderPerShareUnit = div<uint64>(locals.shareholderRevenue, NUMBER_OF_COMPUTORS);
			locals.actualShareholderTotal = locals.shareholderPerShareUnit * NUMBER_OF_COMPUTORS;
			locals.registerPerShareUnit = div<uint64>(locals.registerRevenue, state.get().numberOfRegisters);
			locals.actualRegisterTotal = locals.registerPerShareUnit * state.get().numberOfRegisters;
			locals.winnerRevenue = locals.tokenPool - locals.burnAmount - locals.charityRevenue - locals.actualShareholderTotal - locals.actualRegisterTotal - locals.fee;

			locals.revenueLog = RevenueLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_revenueDistributed,
				locals.burnAmount,
				locals.charityRevenue,
				locals.actualShareholderTotal,
				locals.actualRegisterTotal,
				locals.fee,
				locals.winnerRevenue,
				0
			};
			LOG_INFO(locals.revenueLog);

			qpi.transfer(locals.winner, locals.winnerRevenue);
			qpi.burn(locals.burnAmount);
			qpi.transfer(state.get().charityAddress, locals.charityRevenue);
			if (locals.shareholderPerShareUnit > 0)
			{
				qpi.distributeDividends(locals.shareholderPerShareUnit);
			}
			qpi.transfer(state.get().feeAddress, locals.fee);

			state.mut().totalBurnAmount += locals.burnAmount;
			state.mut().totalCharityAmount += locals.charityRevenue;
			state.mut().totalShareholderAmount += locals.actualShareholderTotal;
			state.mut().totalRegisterAmount += locals.actualRegisterTotal;
			state.mut().totalFeeAmount += locals.fee;
			state.mut().totalWinnerAmount += locals.winnerRevenue;
			if (locals.winnerRevenue > state.get().largestWinnerAmount)
			{
				state.mut().largestWinnerAmount = locals.winnerRevenue;
			}

			if (locals.registerPerShareUnit > 0)
			{
				locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					qpi.transfer(state.get().registers.key(locals.idx), locals.registerPerShareUnit);
					locals.idx = state.get().registers.nextElementIndex(locals.idx);
				}
			}

			locals.qraffle.epochWinner = locals.winner;
			locals.qraffle.receivedAmount = locals.winnerRevenue;
			locals.qraffle.entryAmount = state.get().qREAmount;
			locals.qraffle.numberOfMembers = state.get().numberOfQuRaffleMembers;
			locals.qraffle.winnerIndex = locals.winnerIndex;
			state.mut().QuRaffles.set(qpi.epoch(), locals.qraffle);

			locals.endEpochLog = EndEpochLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_revenueDistributed,
				qpi.epoch(),
				state.get().numberOfQuRaffleMembers,
				locals.tokenPool,
				locals.winnerRevenue,
				locals.winnerIndex,
				0
			};
			LOG_INFO(locals.endEpochLog);
		}
		else
		{
			locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_emptyQuRaffle, 0 };
			LOG_INFO(locals.log);
		}

		locals.qxmrPerShare = div<uint64>(state.get().epochQXMRRevenue, NUMBER_OF_COMPUTORS);
		locals.qxmrDistributedTotal = 0;
		if (locals.qxmrPerShare > 0)
		{
			locals.qxmrContractBalance = (uint64)qpi.numberOfPossessedShares(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX);
			locals.iter.begin(locals.QraffleAsset);
			while (!locals.iter.reachedEnd())
			{
				locals.sharesHeld = locals.iter.numberOfPossessedShares();
				if (locals.sharesHeld > 0)
				{
					locals.shareholder = locals.iter.possessor();
					locals.perShare = (sint64)(locals.qxmrPerShare * (uint64)locals.sharesHeld);
					if ((uint64)locals.perShare <= locals.qxmrContractBalance - locals.qxmrDistributedTotal)
					{
						locals.transferResult = qpi.transferShareOwnershipAndPossession(QRAFFLE_QXMR_ASSET_NAME, state.get().QXMRIssuer, SELF, SELF, locals.perShare, locals.shareholder);
						if (locals.transferResult >= 0)
						{
							locals.qxmrDistributedTotal += (uint64)locals.perShare;
						}
					}
				}
				locals.iter.next();
			}
		}
		state.mut().epochQXMRRevenue -= locals.qxmrDistributedTotal;

		// Process each active token raffle.
		for (locals.i = 0 ; locals.i < state.get().numberOfActiveTokenRaffle; locals.i++)
		{
			if (state.get().numberOfTokenRaffleMembers.get(locals.i) > 0)
			{
				locals.raffleSeed = qpi.K12(m256i(locals.baseSeed.u64._0, locals.baseSeed.u64._1, locals.baseSeed.u64._2, locals.baseSeed.u64._3 ^ ((uint64)locals.i + 1ULL)));
				locals.r = locals.raffleSeed.u64._0;
				locals.winnerIndex = (uint32)mod(locals.r, state.get().numberOfTokenRaffleMembers.get(locals.i) * 1ull);
				locals.winner = state.get().tokenRaffleMemberSlots.get(locals.i * QRAFFLE_TOKEN_RAFFLE_SLOT_SIZE + locals.winnerIndex);

				locals.acTokenRaffle = state.get().activeTokenRaffle.get(locals.i);

				locals.tokenPool = locals.acTokenRaffle.entryAmount * state.get().numberOfTokenRaffleMembers.get(locals.i);
				locals.burnAmount = div<uint64>(locals.tokenPool * QRAFFLE_BURN_FEE, 100);
				locals.charityRevenue = div<uint64>(locals.tokenPool * QRAFFLE_CHARITY_FEE, 100);
				locals.shareholderRevenue = div<uint64>(locals.tokenPool * QRAFFLE_SHAREHOLDER_FEE, 100);
				locals.registerRevenue = div<uint64>(locals.tokenPool * QRAFFLE_REGISTER_FEE, 100);
				locals.fee = div<uint64>(locals.tokenPool * QRAFFLE_FEE, 100);
				// Round down per-share amounts; winner gets the remainder.
				locals.shareholderPerShareUnit = div<uint64>(locals.shareholderRevenue, NUMBER_OF_COMPUTORS);
				locals.actualShareholderTotal = locals.shareholderPerShareUnit * NUMBER_OF_COMPUTORS;
				locals.registerPerShareUnit = div<uint64>(locals.registerRevenue, state.get().numberOfRegisters);
				locals.actualRegisterTotal = locals.registerPerShareUnit * state.get().numberOfRegisters;
				locals.winnerRevenue = locals.tokenPool - locals.burnAmount - locals.charityRevenue - locals.actualShareholderTotal - locals.actualRegisterTotal - locals.fee;

				// Send winner share. Skip the whole raffle if this fails to prevent partial distribution.
				locals.transferResult = qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.winnerRevenue, locals.winner);
				if (locals.transferResult < 0)
				{
					// Winner transfer failed — skip raffle, leave funds for next epoch.
					locals.emptyTokenRafflelog = EmptyTokenRaffleLogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_transferFailed, locals.i, 0 };
					LOG_INFO(locals.emptyTokenRafflelog);
					state.mut().numberOfTokenRaffleMembers.set(locals.i, 0);
					continue;
				}

				// Burn shares (NULL_ID); fall back to charity if burn is rejected.
				if (locals.burnAmount > 0)
				{
					locals.transferResult = qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.burnAmount, NULL_ID);
					if (locals.transferResult < 0)
					{
						qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.burnAmount, state.get().charityAddress);
					}
				}
				if (locals.charityRevenue > 0)
				{
					qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.charityRevenue, state.get().charityAddress);
				}
				if (locals.fee > 0)
				{
					qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, locals.fee, state.get().feeAddress);
				}

				// Pay shareholders proportional to possessed shares.
				if (locals.shareholderPerShareUnit > 0)
				{
					locals.iter.begin(locals.QraffleAsset);
					while (!locals.iter.reachedEnd())
					{
						locals.sharesHeld = locals.iter.numberOfPossessedShares();
						if (locals.sharesHeld > 0)
						{
							qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, (sint64)(locals.shareholderPerShareUnit * (uint64)locals.sharesHeld), locals.iter.possessor());
						}
						locals.iter.next();
					}
				}

				if (locals.registerPerShareUnit > 0)
				{
					locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
					while (locals.idx != NULL_INDEX)
					{
						qpi.transferShareOwnershipAndPossession(locals.acTokenRaffle.token.assetName, locals.acTokenRaffle.token.issuer, SELF, SELF, (sint64)locals.registerPerShareUnit, state.get().registers.key(locals.idx));
						locals.idx = state.get().registers.nextElementIndex(locals.idx);
					}
				}

				locals.tRaffle.epochWinner = locals.winner;
				locals.tRaffle.token.assetName = locals.acTokenRaffle.token.assetName;
				locals.tRaffle.token.issuer = locals.acTokenRaffle.token.issuer;
				locals.tRaffle.entryAmount = locals.acTokenRaffle.entryAmount;
				locals.tRaffle.numberOfMembers = state.get().numberOfTokenRaffleMembers.get(locals.i);
				locals.tRaffle.winnerIndex = locals.winnerIndex;
				locals.tRaffle.epoch = qpi.epoch();
				state.mut().tokenRaffle.set(state.get().numberOfEndedTokenRaffle, locals.tRaffle);

				locals.tokenRaffleLog = TokenRaffleLogger{
					QRAFFLE_CONTRACT_INDEX,
					QRAFFLE_tokenRaffleEnded,
					state.mut().numberOfEndedTokenRaffle++,
					locals.acTokenRaffle.token.assetName,
					state.get().numberOfTokenRaffleMembers.get(locals.i),
					locals.acTokenRaffle.entryAmount,
					locals.winnerIndex,
					locals.winnerRevenue,
					0
				};
				LOG_INFO(locals.tokenRaffleLog);

				state.mut().numberOfTokenRaffleMembers.set(locals.i, 0);
			}
			else
			{
				locals.emptyTokenRafflelog = EmptyTokenRaffleLogger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_emptyTokenRaffle, locals.i, 0 };
				LOG_INFO(locals.emptyTokenRafflelog);
			}
		}

		// ── Asset Raffle settlement ────────────────────────────────────────────
		locals.arRegisterBucket = 0;
		for (locals.arI = 0; locals.arI < state.get().numberOfActiveAssetRaffles; locals.arI++)
		{
			locals.arInfo = state.get().activeAssetRaffles.get(locals.arI);
			locals.arGross = locals.arInfo.totalTicketsPaidQu;

			// Reserve test: gross * 80 >= reservePriceQu * 100
			locals.arReserveMet = (locals.arInfo.totalTickets > 0)
				&& (locals.arGross * 80ull >= locals.arInfo.reservePriceQu * 100ull);

			if (locals.arReserveMet)
			{
				// Weighted winner selection by ticket count.
				locals.raffleSeed = qpi.K12(m256i(
					locals.baseSeed.u64._0, locals.baseSeed.u64._1,
					locals.baseSeed.u64._2,
					locals.baseSeed.u64._3 ^ (0xA55E7000ULL + (uint64)locals.arI)));
				locals.r = locals.raffleSeed.u64._0;
				locals.r = mod(locals.r, (uint64)locals.arInfo.totalTickets);
				locals.arTicketAcc = 0;
				locals.arWinnerIndex = 0;
				locals.winner = NULL_ID;
				for (locals.arJ = 0; locals.arJ < locals.arInfo.numberOfBuyers; locals.arJ++)
				{
					locals.arBuyerSlot = locals.arI * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.arJ;
					locals.arTicketAcc += (uint64)state.get().activeAssetRaffleBuyerTickets.get(locals.arBuyerSlot);
					if (locals.r < locals.arTicketAcc)
					{
						locals.arWinnerIndex = locals.arJ;
						locals.winner = state.get().activeAssetRaffleBuyers.get(locals.arBuyerSlot);
						break;
					}
				}

				// Transfer each bundle item to the winner. If an item fails, log and continue;
				// we cannot pull assets back from a user's wallet, so the winner keeps whatever
				// was delivered and the remaining escrowed items stay in the contract.
				for (locals.arJ = 0; locals.arJ < locals.arInfo.bundleSize; locals.arJ++)
				{
					locals.arItem = state.get().activeAssetRaffleItems.get(
						locals.arI * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.arJ);
					locals.transferResult = qpi.transferShareOwnershipAndPossession(
						locals.arItem.asset.assetName, locals.arItem.asset.issuer,
						SELF, SELF,
						locals.arItem.numberOfShares, locals.winner);
					if (locals.transferResult < 0)
					{
						locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_assetRaffleBundleDeliveryFailed, 0 };
						LOG_INFO(locals.log);
					}
				}

				// Qu pool distribution: 80% to creator, 20% to fee buckets.
				// Always executed when reserve is met, regardless of per-item delivery outcome.
				// (Assets already delivered to winner; we cannot recall them from a user's wallet.)
				locals.arBurn            = div<uint64>(locals.arGross * (uint64)QRAFFLE_BURN_FEE,        100ull);
				locals.arCharity         = div<uint64>(locals.arGross * (uint64)QRAFFLE_CHARITY_FEE,     100ull);
				locals.arShareholderRev  = div<uint64>(locals.arGross * (uint64)QRAFFLE_SHAREHOLDER_FEE, 100ull);
				locals.arRegisterRev     = div<uint64>(locals.arGross * (uint64)QRAFFLE_REGISTER_FEE,    100ull);
				locals.arFee             = div<uint64>(locals.arGross * (uint64)QRAFFLE_FEE,             100ull);
				// Round down per-share amounts; all rounding dust goes to creator.
				locals.arShareholderPerShare    = div<uint64>(locals.arShareholderRev, (uint64)NUMBER_OF_COMPUTORS);
				locals.arRegisterPerShare       = (state.get().numberOfRegisters > 0)
					? div<uint64>(locals.arRegisterRev, (uint64)state.get().numberOfRegisters)
					: 0;
				locals.arRegisterPerShareActual = locals.arRegisterPerShare * (uint64)state.get().numberOfRegisters;
				// Creator gets gross minus all deductions; rounding dust stays with creator.
				locals.arCreatorPay = locals.arGross
					- locals.arBurn
					- locals.arCharity
					- (locals.arShareholderPerShare * (uint64)NUMBER_OF_COMPUTORS)
					- locals.arRegisterPerShareActual
					- locals.arFee;

				qpi.transfer(locals.arInfo.creator, locals.arCreatorPay);
				qpi.burn(locals.arBurn);
				qpi.transfer(state.get().charityAddress, locals.arCharity);
				if (locals.arShareholderPerShare > 0)
				{
					qpi.distributeDividends(locals.arShareholderPerShare);
				}
				qpi.transfer(state.get().feeAddress, locals.arFee);
				// Accumulate register share; distributed in a single pass after the loop.
				locals.arRegisterBucket += locals.arRegisterPerShareActual;

				state.mut().totalAssetRaffleCreatorPaid += locals.arCreatorPay;
				state.mut().totalAssetRafflesSucceeded++;
			}

			if (!locals.arReserveMet)
			{
				// Reserve not met: refund all Qu to buyers and return bundle to creator.
				for (locals.arJ = 0; locals.arJ < locals.arInfo.numberOfBuyers; locals.arJ++)
				{
					locals.arBuyerSlot = locals.arI * QRAFFLE_MAX_ASSET_TICKET_BUYERS + locals.arJ;
					locals.arRefund = (uint64)state.get().activeAssetRaffleBuyerTickets.get(locals.arBuyerSlot) * locals.arInfo.entryTicketQu;
					qpi.transfer(state.get().activeAssetRaffleBuyers.get(locals.arBuyerSlot), locals.arRefund);
					state.mut().totalAssetRaffleRefunded += locals.arRefund;
				}
				for (locals.arJ = 0; locals.arJ < locals.arInfo.bundleSize; locals.arJ++)
				{
					locals.arItem = state.get().activeAssetRaffleItems.get(
						locals.arI * QRAFFLE_MAX_ASSETS_PER_BUNDLE + locals.arJ);
					qpi.transferShareOwnershipAndPossession(
						locals.arItem.asset.assetName, locals.arItem.asset.issuer,
						SELF, SELF,
						locals.arItem.numberOfShares, locals.arInfo.creator);
				}
				locals.arCreatorPay = 0;
				locals.winner = NULL_ID;
				locals.arWinnerIndex = 0;
				state.mut().totalAssetRafflesFailed++;
			}

			// Write to ended ring buffer.
			// arEndedGlobalIdx is the monotonic logical index (pre-increment); this is the
			// value callers pass to getEndedAssetRaffle. arEndedIdx is its ring-masked slot.
			locals.arEndedGlobalIdx = state.get().numberOfEndedAssetRaffles;
			locals.arEndedIdx = mod<uint32>(locals.arEndedGlobalIdx, QRAFFLE_MAX_ENDED_ASSET_RAFFLES);
			locals.arEnded.creator        = locals.arInfo.creator;
			locals.arEnded.epochWinner    = locals.winner;
			locals.arEnded.reservePriceQu = locals.arInfo.reservePriceQu;
			locals.arEnded.entryTicketQu  = locals.arInfo.entryTicketQu;
			locals.arEnded.grossPoolQu    = locals.arGross;
			locals.arEnded.creatorPaidQu  = locals.arCreatorPay;
			locals.arEnded.totalTickets   = locals.arInfo.totalTickets;
			locals.arEnded.numberOfBuyers = locals.arInfo.numberOfBuyers;
			locals.arEnded.bundleSize     = locals.arInfo.bundleSize;
			locals.arEnded.epoch          = locals.arInfo.epoch;
			locals.arEnded.reserveMet     = locals.arReserveMet ? 1u : 0u;
			state.mut().endedAssetRaffles.set(locals.arEndedIdx, locals.arEnded);
			state.mut().numberOfEndedAssetRaffles++;

			locals.arLog = AssetRaffleEndedLogger{
				QRAFFLE_CONTRACT_INDEX,
				locals.arReserveMet ? (uint32)QRAFFLE_assetRaffleSucceeded : (uint32)QRAFFLE_assetRaffleRefunded,
				locals.arEndedGlobalIdx,
				locals.arInfo.creator,
				locals.winner,
				locals.arGross,
				locals.arCreatorPay,
				locals.arReserveMet ? (uint8)1 : (uint8)0,
				0
			};
			LOG_INFO(locals.arLog);
		}

		// Distribute accumulated register share from all successful asset raffles in one O(R) pass.
		// Any integer-division remainder (up to numberOfRegisters-1 Qu) is folded into the DAO
		// bucket below so the dust either pays out this same epoch via the DAO distribution that
		// follows, or carries forward — never silently leaks into untracked contract balance.
		if (locals.arRegisterBucket > 0 && state.get().numberOfRegisters > 0)
		{
			locals.arRegisterBucketPerReg = div<uint64>(locals.arRegisterBucket, (uint64)state.get().numberOfRegisters);
			if (locals.arRegisterBucketPerReg > 0)
			{
				locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					qpi.transfer(state.get().registers.key(locals.idx), locals.arRegisterBucketPerReg);
					locals.idx = state.get().registers.nextElementIndex(locals.idx);
				}
			}
			state.mut().epochAssetRaffleDaoBucket += locals.arRegisterBucket - locals.arRegisterBucketPerReg * (uint64)state.get().numberOfRegisters;
		}

		// Distribute DAO proposal-fee bucket evenly to registers.
		// Subtract only what was actually paid out so the integer-division remainder carries
		// forward to the next epoch instead of being silently dropped into contract balance.
		if (state.get().epochAssetRaffleDaoBucket > 0 && state.get().numberOfRegisters > 0)
		{
			locals.arDaoBucketPerRegister = div<uint64>(state.get().epochAssetRaffleDaoBucket, (uint64)state.get().numberOfRegisters);
			if (locals.arDaoBucketPerRegister > 0)
			{
				locals.idx = state.get().registers.nextElementIndex(NULL_INDEX);
				while (locals.idx != NULL_INDEX)
				{
					qpi.transfer(state.get().registers.key(locals.idx), locals.arDaoBucketPerRegister);
					locals.idx = state.get().registers.nextElementIndex(locals.idx);
				}
			}
			state.mut().epochAssetRaffleDaoBucket -= locals.arDaoBucketPerRegister * (uint64)state.get().numberOfRegisters;
		}

		// Reset asset raffle per-epoch state.
		state.mut().numberOfActiveAssetRaffles = 0;
		state.mut().assetRaffleParticipation.reset();
		state.mut().assetRaffleBuyerSlotIndex.reset();
		state.mut().assetRafflesPerCreator.reset();

		// Calculate new qREAmount and log
		locals.log = Logger{ QRAFFLE_CONTRACT_INDEX, QRAFFLE_revenueDistributed, 0 };
		LOG_INFO(locals.log);

		locals.sumOfEntryAmountSubmitted = 0;
		locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(NULL_INDEX);
		while (locals.idx != NULL_INDEX)
		{
			locals.sumOfEntryAmountSubmitted += state.get().quRaffleEntryAmount.value(locals.idx);
			locals.idx = state.get().quRaffleEntryAmount.nextElementIndex(locals.idx);
		}
		if (state.get().numberOfEntryAmountSubmitted > 0)
		{
			state.mut().qREAmount = div<uint64>(locals.sumOfEntryAmountSubmitted, state.get().numberOfEntryAmountSubmitted);
		}
		else
		{
			state.mut().qREAmount = QRAFFLE_DEFAULT_QRAFFLE_AMOUNT;
		}

		state.mut().numberOfActiveTokenRaffle = 0;

		// Process approved proposals and create new token raffles
		for (locals.i = 0 ; locals.i < state.get().numberOfProposals; locals.i++)
		{
			locals.proposal = state.get().proposals.get(locals.i);

			// Log proposal processing with detailed information
			locals.proposalLog = ProposalLogger{
				QRAFFLE_CONTRACT_INDEX,
				QRAFFLE_proposalSubmitted,
				locals.i,
				locals.proposal.proposer,
				locals.proposal.nYes,
				locals.proposal.nNo,
				locals.proposal.token.assetName,
				locals.proposal.entryAmount,
				0
			};
			LOG_INFO(locals.proposalLog);

			if (locals.proposal.nYes > locals.proposal.nNo)
			{
				locals.acTokenRaffle.token.assetName = locals.proposal.token.assetName;
				locals.acTokenRaffle.token.issuer = locals.proposal.token.issuer;
				locals.acTokenRaffle.entryAmount = locals.proposal.entryAmount;

				state.mut().activeTokenRaffle.set(state.mut().numberOfActiveTokenRaffle++, locals.acTokenRaffle);
			}
		}

		// Record DAO member count for this epoch before resetting per-epoch state.
		state.mut().daoMemberCount.set(qpi.epoch(), state.get().numberOfRegisters);

		state.mut().numberOfVotedInProposal.setAll(0);
		state.mut().tokenRaffleParticipation.reset();
		state.mut().proposalsPerProposer.reset();
		state.mut().quRaffleEntryAmount.reset();
		state.mut().voteParticipation.reset();
		state.mut().voteValues.reset();
		state.mut().numberOfEntryAmountSubmitted = 0;
		state.mut().numberOfProposals = 0;
		state.mut().numberOfQuRaffleMembers = 0;
		state.mut().quRaffleMemberSet.reset();
		if (state.get().registers.needsCleanup()) { state.mut().registers.cleanup(); }
	}

	PRE_ACQUIRE_SHARES()
    {
		// Accept all incoming share management transfers for free.
		// Service fees are collected in user procedures (depositInTokenRaffle, TransferShareManagementRights).
		output.requestedFee = 0;
		output.allowTransfer = true;
    }

	POST_ACQUIRE_SHARES()
	{
		// Credit any received fee to epochRevenue.
		if (input.receivedFee > 0)
		{
			state.mut().epochRevenue += (uint64)input.receivedFee;
		}
	}
};
