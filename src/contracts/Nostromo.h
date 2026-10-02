using namespace QPI;

namespace QPI
{
	inline bool operator==(const Asset& lhs, const Asset& rhs)
	{
		return lhs.assetName == rhs.assetName && lhs.issuer == rhs.issuer;
	}
} // namespace QPI

// Maximum number of active auction records stored by the contract, in auctions.
constexpr uint64 NOST_AUCTION_NUM = 2048;
// Number of full closed-auction snapshots retained in the history ring buffer, in entries.
constexpr uint64 NOST_AUCTION_HISTORY_NUM = 1024;
// Fixed length of an auction metadata IPFS CID, in bytes.
constexpr uint64 NOST_AUCTION_METADATA_CID_LENGTH = 64;
// Maximum number of active auction-participant bid records, in entries.
constexpr uint64 NOST_AUCTION_PARTICIPANT_NUM = 4096;
// Maximum number of wallets with unpaid QU obligations retained by the contract.
constexpr uint64 NOST_PENDING_PAYOUT_NUM = 8192;
// Maximum pending-payout slots one auction settlement may require before END_EPOCH fee distribution.
constexpr uint64 NOST_AUCTION_REVENUE_MAX_PAYOUT_RECIPIENTS = 1;
// Additional pending-payout slot reserved for the Batch bid caller's possible overpayment refund.
constexpr uint64 NOST_BATCH_BID_CALLER_PAYOUT_RECIPIENTS = 1;
// Maximum pending-payout slots reserved by a Standard bid for refunds and an immediate Buy Now settlement.
constexpr uint64 NOST_STANDARD_BID_MAX_PAYOUT_RECIPIENTS = 6;
// Maximum pending-payout slots reserved by Standard settlement for revenue distribution and bidder handling.
constexpr uint64 NOST_STANDARD_FINALIZATION_MAX_PAYOUT_RECIPIENTS = 5;
// Number of QPI-sized QU transfer chunks attempted for an immediate refund or settlement payout.
constexpr uint64 NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL = 1;
// Maximum number of pending-payout wallets retried automatically during one END_EPOCH call.
constexpr uint64 NOST_END_EPOCH_PAYOUT_RECIPIENT_NUM = 64;
// Number of QPI-sized QU transfer chunks retried per pending-payout wallet at END_EPOCH.
constexpr uint64 NOST_END_EPOCH_PAYOUT_CHUNKS_PER_RECIPIENT = 1;
// Maximum number of QPI-sized QU transfers attempted for one wallet in one procedure call.
constexpr uint64 NOST_MAX_QU_TRANSFER_CHUNKS_PER_CALL = 16;
// Sentinel for "no participant slot".
constexpr uint64 NOST_INVALID_PARTICIPANT_SLOT = NOST_AUCTION_PARTICIPANT_NUM;
// Maximum number of entries returned by one paginated auction getter call.
constexpr uint64 NOST_AUCTION_GETTER_PAGE_SIZE = 64;
// Maximum number of asset entries in a Batch Auction lot.
constexpr uint64 NOST_BATCH_AUCTION_LOT_ITEM_NUM = 1;
// Integer offset that makes the Batch coverage threshold include the first quantity below the minimum allocation.
constexpr uint64 NOST_BATCH_COVERAGE_THRESHOLD_OFFSET = 1;
// Maximum number of asset entries in a Standard Auction lot.
constexpr uint64 NOST_AUCTION_LOT_ITEM_NUM = 4;
// Maximum number of bidder wallets allowed by a private auction wallet gate.
constexpr uint64 NOST_AUCTION_ALLOWED_WALLET_NUM = 16;
// Maximum number of alternative assets accepted by a private auction asset gate.
constexpr uint64 NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM = 4;
// Maximum configured duration of any auction, in days.
constexpr uint32 NOST_AUCTION_MAX_DURATION_DAYS = 30;
// Default fee charged to create a private auction, in qu.
constexpr sint64 NOST_DEFAULT_PRIVATE_AUCTION_FEE = 50000000LL;
// Default fee accumulated after successfully creating a public auction and distributed at END_EPOCH, in qu.
constexpr sint64 NOST_PUBLIC_AUCTION_CREATION_FEE = 100LL;
// Minimum total payment target for small accepted Batch Auction bids, in qu.
constexpr uint64 NOST_BATCH_BID_FEE_CUTOFF = 100ULL;
// Default fee deducted when an auction is cancelled, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_CANCELLATION_FEE_BP = 1000ULL;
// Default management fee applied to gross auction proceeds, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_MANAGEMENT_FEE_BP = 50ULL;
// Default development fee applied to gross auction proceeds, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_DEVELOPMENT_FEE_BP = 50ULL;
// Default takeover coordinator fee applied to gross auction proceeds, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_TAKEOVER_COORDINATOR_FEE_BP = 50ULL;
// Shareholder allocation of auction creation, small-bid, and cancellation service fees, in basis points.
constexpr uint64 NOST_AUCTION_SERVICE_FEE_SHAREHOLDER_BP = 7270ULL;
// Management allocation of auction creation, small-bid, and cancellation service fees, in basis points.
constexpr uint64 NOST_AUCTION_SERVICE_FEE_MANAGEMENT_BP = 910ULL;
// Development allocation of auction creation, small-bid, and cancellation service fees, in basis points.
constexpr uint64 NOST_AUCTION_SERVICE_FEE_DEVELOPMENT_BP = 910ULL;
// Takeover coordinator allocation of auction creation, small-bid, and cancellation service fees, in basis points.
constexpr uint64 NOST_AUCTION_SERVICE_FEE_TAKEOVER_COORDINATOR_BP = 910ULL;
// Default portion of the shareholder fee distributed as dividends, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_SHAREHOLDER_DIVIDEND_BP = 9000ULL;
// Default shareholder fee for gross proceeds in tier 1, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_1 = 500ULL;
// Default shareholder fee for gross proceeds in tier 2, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_2 = 450ULL;
// Default shareholder fee for gross proceeds in tier 3, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_3 = 400ULL;
// Default shareholder fee for gross proceeds in tier 4, in basis points.
constexpr uint64 NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_4 = 350ULL;
// Inclusive upper gross-proceeds threshold for shareholder fee tier 1, in qu.
constexpr uint64 NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_1 = 5000000000ULL;
// Inclusive upper gross-proceeds threshold for shareholder fee tier 2, in qu.
constexpr uint64 NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_2 = 50000000000ULL;
// Inclusive upper gross-proceeds threshold for shareholder fee tier 3, in qu.
constexpr uint64 NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_3 = 200000000000ULL;
// Time added when an accepted bid arrives near an auction deadline, in seconds.
constexpr uint64 NOST_AUCTION_EXTENSION_SECONDS = 300ULL;
// Number of seconds used to convert one auction duration day.
constexpr uint64 NOST_SECONDS_PER_DAY = 86400ULL;
// Time allowed for a Standard Auction seller to resolve a pending sale, in seconds.
constexpr uint64 NOST_AUCTION_SELLER_DECISION_WINDOW_SECONDS = 604800ULL;
// Duration of the scheduled auction pause before an epoch transition, in seconds.
constexpr uint64 NOST_AUCTION_PRE_EPOCH_PAUSE_SECONDS = 1800ULL;
// Duration of the auction launch pause after `BEGIN_EPOCH`, in ticks.
constexpr uint32 NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS = 500U;
// Denominator representing 100 percent in basis-point calculations.
constexpr uint64 NOST_BASIS_POINTS_SCALE = 10000ULL;
// Number of microseconds used to convert a timestamp duration to seconds.
constexpr uint64 NOST_MICROSECONDS_PER_SECOND = 1000000ULL;
// Epoch at which the contract reapplies its default configuration, in epochs.
constexpr uint16 NOST_REINITIALIZATION_EPOCH = 220U;
// Quantity used to sell a Standard Auction lot as one indivisible unit, not an asset count.
constexpr uint64 NOST_STANDARD_AUCTION_LOT_COUNT = 1ULL;
// Minimum allowed Standard Auction starting and sale price, in qu.
constexpr uint64 NOST_STANDARD_MIN_PRICE = 1000000ULL;
// Minimum allowed Standard Auction bid increment, in qu.
constexpr uint64 NOST_STANDARD_MIN_BID_INCREMENT = 1000ULL;
// Year component of the packed initial date stamp.
constexpr uint8 NOST_DEFAULT_INIT_YEAR = 22U;
// Month component of the packed initial date stamp.
constexpr uint8 NOST_DEFAULT_INIT_MONTH = 4U;
// Day component of the packed initial date stamp.
constexpr uint8 NOST_DEFAULT_INIT_DAY = 13U;
// Bit offset of the year component in a packed date stamp, in bits.
constexpr uint8 NOST_DATE_STAMP_YEAR_SHIFT = 9U;
// Bit offset of the month component in a packed date stamp, in bits.
constexpr uint8 NOST_DATE_STAMP_MONTH_SHIFT = 5U;
// Runtime day-of-week index on which the scheduled pre-epoch pause begins.
constexpr uint8 NOST_PRE_EPOCH_PAUSE_DAY_OF_WEEK = 0U;
// UTC hour at which the scheduled pre-epoch pause begins.
constexpr uint8 NOST_PRE_EPOCH_PAUSE_HOUR = 11U;
// Minute within the configured hour at which the scheduled pre-epoch pause begins.
constexpr uint8 NOST_PRE_EPOCH_PAUSE_MINUTE = 30U;
// Packed date stamp used to recognize the contract's initial runtime date.
constexpr uint32 NOST_DEFAULT_INIT_TIME =
    NOST_DEFAULT_INIT_YEAR << NOST_DATE_STAMP_YEAR_SHIFT | NOST_DEFAULT_INIT_MONTH << NOST_DATE_STAMP_MONTH_SHIFT | NOST_DEFAULT_INIT_DAY;
// Default enabled flag that routes all collected auction fees to development.
constexpr uint8 NOST_ROUTE_ALL_FEES_TO_DEVELOPMENT = 1;
// Default drop in the execution fee reserve that triggers an emergency pause, in basis points.
constexpr uint64 NOST_DEFAULT_FEE_RESERVE_GUARD_DROP_BP = 1000ULL;
// Default rolling window used to evaluate the execution fee reserve drop, in seconds.
constexpr uint64 NOST_DEFAULT_FEE_RESERVE_GUARD_WINDOW_SECONDS = 600ULL;

/** Old */
constexpr uint32 NOSTROMO_MAX_USER_OLD = 262144;
constexpr uint32 NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST_OLD = 128;
constexpr uint32 NOSTROMO_MAX_NUMBER_TOKEN_OLD = 262144;
constexpr uint32 NOSTROMO_MAX_NUMBER_PROJECT_OLD = 262144;

struct NOST2
{
};

struct NOST : public ContractBase
{
	enum class EProcedureId : uint8
	{
		CreateAuction = 1,
		PlaceBid = 2,
		CancelAuction = 3,
		TransferShareManagementRights = 4,
		ResolvePendingStandardAuction = 5,
		SetAuctionFees = 6,
		SetAuctionFeesByManagement = 7,
		SetManagement = 8,
		SetFeeReserveGuardConfig = 9,
		SetEmergencyPause = 10
	};

	/** @brief Stable public-function identifiers used by the contract ABI. */
	enum class EFunctionId : uint16
	{
		GetAuctionByIndex = 1,
		GetAuctionParticipant = 2,
		GetTicksBeforeAuctionLaunch = 3,
		GetAuctionFees = 4,
		GetFeeRecipients = 5,
		GetClosedAuctionHistory = 6,
		GetRouteAllFeesToDevelopment = 7,
		GetContractStats = 8,
		GetAuctionSummaries = 9,
		GetActiveAuctionIndices = 10,
		GetAuctionsBySeller = 11,
		GetAuctionByMetadataCid = 12,
		GetAuctionSummariesByIndexBatch = 13,
		GetAuctionParticipants = 14,
		GetUserParticipations = 15,
		GetLatestAuctionIndex = 16,
		GetAuctionCountBySeller = 17,
		GetAuctionAtCreationSnapshot = 18,
		GetBatchAuctionBidAvailability = 19,
		CalculateBatchAuctionBidFee = 20,
		GetPendingServiceFeePool = 21,
		GetFeeReserveGuardState = 22,
		GetPendingPayout = 23,
		GetNostromoFeePool = 24
	};

	enum class EAuctionType : uint8
	{
		None,
		Batch,
		Standard
	};

	enum class EAuctionVisibility : uint8
	{
		None,
		Public,
		Private
	};

	enum class EAuctionStatus : uint8
	{
		None,
		Active,
		Finalized,
		Cancelled,
		PendingSellerDecision
	};

	enum class EAuctionError : uint8
	{
		Success,
		InvalidInput,
		AuctionNotFound,
		AuctionClosed,
		Forbidden,
		InsufficientFunds,
		InsufficientAssetBalance,
		StorageFull,
		InvalidAuctionType,
		InvalidVisibility,
		BidTooLow,
		PrivateAuctionAccessDenied,
		AuctionPaused,
		AuctionIndexExhausted,
		QuantityUnavailable,
		AuctionHasAcceptedBid,
		PayoutQueueFull
	};

	/**
	 * @brief Stores one bid slot in one auction.
	 * @note The same struct is shared by batch and standard auctions.
	 */
	struct AuctionParticipantData
	{
		/** @brief Auction that owns this bid slot. */
		uint64 auctionIndex;

		/** @brief Monotonic bid sequence inside the auction, used for FIFO tie-breaks. */
		uint64 bidIndex;

		/** @brief Amount currently locked in escrow for the participant bid. */
		uint64 escrowedAmount;

		/** @brief Quantity requested by the participant; standard auctions always use the whole lot quantity. */
		uint64 requestedQuantity;

		/** @brief Quantity finally allocated to the participant after batch auction settlement. */
		uint64 allocatedQuantity;

		/** @brief Offered price per asset in a batch auction, or total offered price for the whole lot in a standard auction. */
		uint64 bidAmount;

		/** @brief Wallet that owns this participant record. */
		id participant;

		/** @brief Timestamp of the participant's latest accepted bid. */
		DateAndTime lastBidTime;

		/** @brief Marks whether this fixed array slot contains a reusable historical or active record. */
		uint8 isUsed;

		/** @brief Marks bids that are still eligible for allocation or standard highest-bid settlement. */
		uint8 isActive;

		/** @brief Marks bids that remain inside the winning allocation after settlement. */
		uint8 isWinningBid;
	};

	/**
	 * @brief Describes an asset and quantity used by an auction lot or private access rule.
	 */
	struct AuctionAssetEntry
	{
		/** @brief Asset included in a lot or used as an access requirement. */
		Asset asset;

		/** @brief Lot quantity or minimum ownership quantity required for access. */
		sint64 quantity;
	};

	/**
	 * @brief Shared auction fields used by persistent state and public getter views.
	 * @note Container fields differ between persistent state and ABI views, so access-control collections stay outside this struct.
	 * @note `metadataIpfsCid` points to off-chain auction metadata stored in IPFS.
	 * @note `sellerDecisionDeadline` stays zero until a standard auction enters the manual decision window.
	 */
	struct AuctionCore
	{
		/** @brief Assets and quantities offered by the auction. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Lowercase base32 CIDv1 stored in Pinata for the auction name and description metadata. */
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/** @brief Wallet that created the auction and offers the lot for sale. */
		id seller;

		/** @brief Wallet that currently holds the highest bid. */
		id highestBidder;

		/** @brief Timestamp when the seller created the auction. */
		DateAndTime createdAt;

		/** @brief Timestamp of the most recent accepted bid. */
		DateAndTime lastBidAt;

		/** @brief Deadline for the seller to accept or reject a standard auction bid that ended between Initial Price and Sale Price. */
		DateAndTime sellerDecisionDeadline;

		/** @brief Timestamp when the auction was finalized, cancelled, or otherwise settled. */
		DateAndTime settledAt;

		/** @brief Total sale units offered; batch auctions use asset quantity, standard auctions use one unit for the whole lot. */
		uint64 quantityForSale;

		/** @brief Quantity already assigned to winning bids after settlement. */
		uint64 allocatedQuantity;

		/** @brief Minimum quantity requested by each batch bid; always zero for standard auctions. */
		uint64 minimumPurchaseQuantity;

		/** @brief Initial price for a standard auction; bids cannot start below this total price for the whole lot. */
		uint64 initialPrice;

		/** @brief Minimum selling price per asset in a batch auction, or desired minimum total selling price for the whole lot in a standard
		 * auction. */
		uint64 salePrice;

		/** @brief Minimum increment by which a new standard auction bid must exceed the current highest bid. */
		uint64 minimumBidIncrement;

		/** @brief Buy Now price that closes a standard auction immediately when matched or exceeded. */
		uint64 buyNowPrice;

		/** @brief Highest offered price per asset in a batch auction, or highest total offered price in a standard auction. */
		uint64 highestBidPrice;

		/** @brief Quantity requested by the current highest bid. */
		uint64 highestBidQuantity;

		/** @brief Total amount escrowed by the current highest bid; equal to the committed highest bid amount. */
		uint64 highestBidAmount;

		/** @brief Auction duration in seconds, derived from the duration configured in days. */
		uint64 auctionDurationSeconds;

		/** @brief Monotonic identifier assigned when the auction is created. */
		uint64 auctionIndex;

		/** @brief Monotonic per-auction bid index used to store every batch bid as a separate position. */
		uint64 nextBidIndex;

		/** @brief Fixed-array slot of the current standard-auction highest bid, or `NOST_INVALID_PARTICIPANT_SLOT`. */
		uint64 highestBidSlotIndex;

		/** @brief Auction House mode: Batch Auction or Standard Auction. */
		EAuctionType type;

		/** @brief Auction visibility: public or restricted private access. */
		EAuctionVisibility visibility;

		/** @brief Current lifecycle status of the auction, including the seller decision phase for standard auctions. */
		EAuctionStatus status;
	};

	/**
	 * @brief Stores all persistent data for one auction.
	 * @note The same struct is shared by batch and standard auctions.
	 */
	struct AuctionData
	{
		/** @brief Fields shared with the public auction view. */
		AuctionCore core;

		/** @brief Wallet whitelist used when the private auction uses wallet-based access. */
		HashSet<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/** @brief Minimum quantity by asset required for participation; owning any one entry grants access. */
		HashMap<Asset, sint64, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;
	};

	/**
	 * @brief Serializable view of one auction for public getter outputs.
	 * @note Persistent state uses `HashSet` for access checks, but ABI payloads expose fixed arrays because `HashSet` is not valid in
	 * input/output structs.
	 */
	struct AuctionView
	{
		/** @brief Fields shared with the persistent auction record. */
		AuctionCore core;

		/** @brief Wallet list used when the private auction restricts participation to predefined wallets. */
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/** @brief Asset and minimum-quantity alternatives used by private asset-based access. */
		Array<AuctionAssetEntry, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;

		/** @brief Number of populated entries in `requiredAccessAssets`. */
		uint64 requiredAccessAssetCount;

		/** @brief Number of populated entries in `allowedBidderWallets`. */
		uint64 allowedBidderWalletCount;
	};

	struct OldStateData
	{
		struct investInfo
		{
			uint64 investedAmount;
			uint64 claimedAmount;
			uint32 indexOfFundraising;
		};

		struct projectInfo
		{
			id creator;
			uint64 tokenName;
			uint64 supplyOfToken;
			uint32 startDate;
			uint32 endDate;
			uint32 numberOfYes;
			uint32 numberOfNo;
			bit isCreatedFundarasing;
		};

		struct fundaraisingInfo
		{
			uint64 tokenPrice;
			uint64 soldAmount;
			uint64 requiredFunds;
			uint64 raisedFunds;
			uint32 indexOfProject;
			uint32 firstPhaseStartDate;
			uint32 firstPhaseEndDate;
			uint32 secondPhaseStartDate;
			uint32 secondPhaseEndDate;
			uint32 thirdPhaseStartDate;
			uint32 thirdPhaseEndDate;
			uint32 listingStartDate;
			uint32 cliffEndDate;
			uint32 vestingEndDate;
			uint8 threshold;
			uint8 TGE;
			uint8 stepOfVesting;
			bit isCreatedToken;
		};

		HashMap<id, uint8, NOSTROMO_MAX_USER_OLD> users;
		HashMap<id, Array<uint32, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST_OLD>, NOSTROMO_MAX_USER_OLD> voteStatus;
		HashMap<id, uint32, NOSTROMO_MAX_USER_OLD> numberOfVotedProject;
		HashSet<uint64, NOSTROMO_MAX_NUMBER_TOKEN_OLD> tokens;

		HashMap<id, Array<investInfo, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST_OLD>, NOSTROMO_MAX_USER_OLD> investors;
		HashMap<id, uint32, NOSTROMO_MAX_USER_OLD> numberOfInvestedProjects;
		Array<investInfo, NOSTROMO_MAX_NUMBER_OF_PROJECT_USER_INVEST_OLD> tmpInvestedList;

		Array<projectInfo, NOSTROMO_MAX_NUMBER_PROJECT_OLD> projects;

		Array<fundaraisingInfo, NOSTROMO_MAX_NUMBER_PROJECT_OLD> fundaraisings;

		id teamAddress;
		sint64 transferRightsFee;
		uint64 epochRevenue, totalPoolWeight;
		uint32 numberOfRegister, numberOfCreatedProject, numberOfFundraising;
	};

	/**
	 * @brief Epoch fee accrual shared by Nostromo modules.
	 * @note Auction shareholder amounts are separated by sale tier because their fee formulas differ. Other recipient amounts are compatible sums.
	 */
	struct NostromoFeePool
	{
		uint64 shareholderDividendTier1Amount;
		uint64 shareholderDividendTier2Amount;
		uint64 shareholderDividendTier3Amount;
		uint64 shareholderDividendTier4Amount;
		uint64 commonServiceFeeAmount;
		uint64 shareholderDividendAmount;
		uint64 managementAmount;
		uint64 developmentAmount;
		uint64 takeoverCoordinatorAmount;
	};

	struct StateData
	{
		/** @brief Configured fee charged when creating a private auction. */
		sint64 privateAuctionFee;

		/** @brief Configured non-negative fee accumulated when creating a public auction and distributed at `END_EPOCH`. */
		sint64 publicAuctionCreationFee;

		/** @brief Configured cancellation fee rate in basis points. */
		uint64 auctionCancellationFeeBasisPoints;

		/** @brief Undistributed shareholder revenue from the shared fee pool reserved for contract dividends. */
		uint64 auctionShareholderDividendPool;

		/** @brief Configured management fee rate in basis points, charged from auction proceeds. */
		uint64 managementFeeBasisPoints;

		/** @brief Configured development fee rate in basis points, charged from auction proceeds. */
		uint64 developmentFeeBasisPoints;

		/** @brief Configured takeover coordinator fee rate in basis points, charged from auction proceeds. */
		uint64 takeoverCoordinatorFeeBasisPoints;

		/** @brief Share of the shareholder fee redirected to dividends, expressed in basis points. */
		uint64 shareholderDividendBasisPoints;

		/** @brief Shareholder fee tier applied to auctions up to the first threshold. */
		uint64 shareholderFeeBasisPointsTier1;

		/** @brief Shareholder fee tier applied to auctions above the first threshold and up to the second threshold. */
		uint64 shareholderFeeBasisPointsTier2;

		/** @brief Shareholder fee tier applied to auctions above the second threshold and up to the third threshold. */
		uint64 shareholderFeeBasisPointsTier3;

		/** @brief Shareholder fee tier applied to auctions above the third threshold. */
		uint64 shareholderFeeBasisPointsTier4;

		/** @brief Start of the currently active global auction timer pause interval. */
		DateAndTime auctionTimerPauseStartedAt;

		/** @brief End of the currently active global auction timer pause interval. */
		DateAndTime auctionTimerPauseEndsAt;

		/** @brief Configured maximum auction duration in days. */
		uint32 maxAuctionDurationDays;

		/** @brief Cached QX transfer fee refreshed at the beginning of each epoch. */
		uint32 qxTransferFee;

		/** @brief Flag indicating whether the post-`BEGIN_EPOCH()` auction pause is active for the current epoch. */
		uint8 isPostBeginEpochPauseArmed;

		/** @brief Flag indicating whether auction deadlines are currently frozen by a global pause interval. */
		uint8 isAuctionTimerPaused;

		/** @brief Flag indicating whether every auction fee is routed to the development wallet. */
		uint8 routeAllFeesToDevelopment;

		id management;

		id development;

		id takeoverCoordinator;

		/** @brief Total number of auctions ever created; also the next auction index. */
		uint64 totalAuctionsCreated;

		/** @brief Circular buffer with full snapshots of finalized and cancelled auctions. */
		Array<AuctionData, NOST_AUCTION_HISTORY_NUM> closedAuctionHistory;

		/** @brief Monotonic insertion counter for `closedAuctionHistory`. */
		uint64 closedAuctionHistoryCounter;

		HashMap<uint64, AuctionData, NOST_AUCTION_NUM> auctionList;
		/** @brief Active bid records; slots are cleared as soon as the bid leaves the live order book. */
		Array<AuctionParticipantData, NOST_AUCTION_PARTICIPANT_NUM> participants;
		/** @brief Bounded history of completed, refunded, and displaced bid records. */
		Array<AuctionParticipantData, NOST_AUCTION_PARTICIPANT_NUM> participantHistory;
		/** @brief Monotonic insertion counter for `participantHistory`. */
		uint64 participantHistoryCounter;

		/** @brief Wallet-indexed QU liabilities registered before an auction is finalized. */
		HashMap<id, uint64, NOST_PENDING_PAYOUT_NUM> pendingQuPayouts;
		/** @brief Sum of all values in `pendingQuPayouts`, in qu. */
		uint64 totalPendingQuPayouts;
		/** @brief Physical hash-map slot from which the next bounded automatic payout scan starts. */
		uint64 pendingPayoutScanCursor;
		/** @brief Lifetime number of finalized auctions. */
		uint64 totalFinalizedAuctions;
		/** @brief Lifetime number of cancelled auctions. */
		uint64 totalCancelledAuctions;

		/** @brief Shared fee accrual for Auction House and future Nostromo modules, settled at `END_EPOCH`. */
		NostromoFeePool feePool;

		/** @brief Configured drop in the execution fee reserve that triggers an emergency pause, in basis points. */
		uint64 feeReserveGuardDropBasisPoints;

		/** @brief Configured rolling window used to evaluate the execution fee reserve drop, in seconds. */
		uint64 feeReserveGuardWindowSeconds;

		/** @brief Execution fee reserve value recorded at the start of the current guard window. */
		sint64 feeReserveBaseline;

		/** @brief Start of the current guard window; invalid when the window has not been initialized. */
		DateAndTime feeReserveBaselineAt;

		/** @brief Timestamp at which the emergency pause was triggered; invalid when not paused. */
		DateAndTime emergencyPausedAt;

		/** @brief Flag indicating whether an emergency pause is currently blocking auction interactions. */
		uint8 isEmergencyPaused;
	};

	/** @brief Input payload used to create a Batch Auction or Standard Auction in the Auction House. */
	struct CreateAuction_input
	{
		/** @brief Lowercase base32 CIDv1 stored in Pinata for the auction name and description metadata. */
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/** @brief Assets and quantities offered by the auction. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Asset and minimum-quantity alternatives used by private asset-based access. */
		Array<AuctionAssetEntry, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;

		/** @brief Wallet list used when the private auction restricts participation to predefined wallets. */
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/** @brief Required minimum requested quantity for batch bids; ignored for standard auctions. */
		uint64 minimumPurchaseQuantity;

		/** @brief Initial price for a standard auction; bids cannot be placed below this total price for the whole lot. */
		uint64 initialPrice;

		/** @brief Minimum selling price per asset in a batch auction, or desired minimum total selling price for the whole lot in a standard
		 * auction. */
		uint64 salePrice;

		/** @brief Minimum increment by which each new standard auction bid must exceed the current highest bid. */
		uint64 minimumBidIncrement;

		/** @brief Buy Now price that immediately closes a standard auction once matched or exceeded. */
		uint64 buyNowPrice;

		/** @brief Auction duration in days, capped by the contract configuration. */
		uint32 durationDays;

		/** @brief Auction House mode selected by the seller: Batch Auction or Standard Auction. */
		uint8 auctionType;

		/** @brief Visibility selected by the seller: public or private. */
		uint8 auctionVisibility;
	};

	/** @brief Result of auction creation. */
	struct CreateAuction_output
	{
		/** @brief Monotonic index assigned to the new auction when creation succeeds. */
		uint64 auctionIndex;

		/** @brief Result code describing whether the auction creation succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used to place a bid in a Batch Auction or Standard Auction. */
	struct PlaceBid_input
	{
		/** @brief Monotonic index of the target auction. */
		uint64 auctionIndex;

		/** @brief Requested quantity for a batch auction, which must meet its configured minimum; ignored for a standard auction. */
		uint64 quantity;

		/** @brief Offered price per asset in a batch auction, or total offered price for the whole lot in a standard auction. */
		uint64 bidAmount;
	};

	/** @brief Result of a bid placement request. */
	struct PlaceBid_output
	{
		/** @brief Amount that remains escrowed for the accepted bid. */
		uint64 escrowedAmount;

		/** @brief Amount refunded to the bidder, including replaced escrow or invocation change. */
		uint64 refundedAmount;

		/** @brief Result code describing whether the bid placement succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used to cancel an active auction. */
	struct CancelAuction_input
	{
		/** @brief Monotonic index of the auction that the seller wants to cancel. */
		uint64 auctionIndex;
	};

	/** @brief Result of an auction cancellation request. */
	struct CancelAuction_output
	{
		/** @brief Total amount refunded to bidders because of the cancellation. */
		uint64 refundedAmount;

		/** @brief Cancellation fee charged to the seller according to the auction rules. */
		uint64 cancellationFee;

		/** @brief Result code describing whether the cancellation succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by the seller to accept or reject a pending standard auction result. */
	struct ResolvePendingStandardAuction_input
	{
		/** @brief Monotonic index of the standard auction awaiting the seller decision. */
		uint64 auctionIndex;

		/** @brief Set to `1` to accept the sale or `0` to reject it. */
		uint8 acceptSale;
	};

	/** @brief Result of a seller decision on a pending standard auction. */
	struct ResolvePendingStandardAuction_output
	{
		/** @brief Amount refunded to the bidder when the seller rejects the sale. */
		uint64 refundedAmount;

		/** @brief Result code describing whether the seller decision was applied. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by the takeover coordinator to overwrite the full auction fee configuration. */
	struct SetAuctionFees_input
	{
		/** @brief Fee charged when a private auction is created. */
		sint64 privateAuctionFee;

		/** @brief Non-negative fee accumulated when a public auction is created and distributed at `END_EPOCH`. */
		sint64 publicAuctionCreationFee;

		/** @brief Cancellation fee rate in basis points. */
		uint64 auctionCancellationFeeBasisPoints;

		/** @brief Management fee rate in basis points. */
		uint64 managementFeeBasisPoints;

		/** @brief Development fee rate in basis points. */
		uint64 developmentFeeBasisPoints;

		/** @brief Takeover coordinator fee rate in basis points. */
		uint64 takeoverCoordinatorFeeBasisPoints;

		/** @brief Percentage of the shareholder fee distributed as dividends, in basis points. */
		uint64 shareholderDividendBasisPoints;

		/** @brief Shareholder fee tier for auctions up to the first threshold. */
		uint64 shareholderFeeBasisPointsTier1;

		/** @brief Shareholder fee tier for auctions above the first threshold and up to the second threshold. */
		uint64 shareholderFeeBasisPointsTier2;

		/** @brief Shareholder fee tier for auctions above the second threshold and up to the third threshold. */
		uint64 shareholderFeeBasisPointsTier3;

		/** @brief Shareholder fee tier for auctions above the third threshold. */
		uint64 shareholderFeeBasisPointsTier4;
	};

	struct SetAuctionFees_output
	{
		/** @brief Result code describing whether the fee update succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by management to update every fee except takeover coordinator-specific splits. */
	struct SetAuctionFeesByManagement_input
	{
		/** @brief Fee charged when a private auction is created. */
		sint64 privateAuctionFee;

		/** @brief Non-negative fee accumulated when a public auction is created and distributed at `END_EPOCH`. */
		sint64 publicAuctionCreationFee;

		/** @brief Cancellation fee rate in basis points. */
		uint64 auctionCancellationFeeBasisPoints;

		/** @brief Management fee rate in basis points. */
		uint64 managementFeeBasisPoints;

		/** @brief Development fee rate in basis points. */
		uint64 developmentFeeBasisPoints;

		/** @brief Shareholder fee tier for auctions up to the first threshold. */
		uint64 shareholderFeeBasisPointsTier1;

		/** @brief Shareholder fee tier for auctions above the first threshold and up to the second threshold. */
		uint64 shareholderFeeBasisPointsTier2;

		/** @brief Shareholder fee tier for auctions above the second threshold and up to the third threshold. */
		uint64 shareholderFeeBasisPointsTier3;

		/** @brief Shareholder fee tier for auctions above the third threshold. */
		uint64 shareholderFeeBasisPointsTier4;
	};

	struct SetAuctionFeesByManagement_output
	{
		/** @brief Result code describing whether the fee update succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by the takeover coordinator to appoint a new management wallet. */
	struct SetManagement_input
	{
		/** @brief New wallet that will receive management privileges. */
		id management;
	};

	struct SetManagement_output
	{
		/** @brief Result code describing whether the management update succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by the takeover coordinator or management to configure the execution fee reserve guard. */
	struct SetFeeReserveGuardConfig_input
	{
		/** @brief Drop in the execution fee reserve, relative to the window baseline, that triggers an emergency pause, in basis points. */
		uint64 dropBasisPoints;

		/** @brief Rolling window used to evaluate the execution fee reserve drop, in seconds. */
		uint64 windowSeconds;
	};

	struct SetFeeReserveGuardConfig_output
	{
		/** @brief Result code describing whether the guard configuration update succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used by the takeover coordinator or management to manually pause or resume auction interactions. */
	struct SetEmergencyPause_input
	{
		/** @brief Set to `1` to activate the emergency pause or `0` to resume normal operation. */
		uint8 paused;
	};

	struct SetEmergencyPause_output
	{
		/** @brief Result code describing whether the emergency pause update succeeded. */
		EAuctionError errorCode;
	};

	/** @brief Input payload used to fetch one auction from storage. */
	struct GetAuctionByIndex_input
	{
		/** @brief Monotonic index of the auction to read. */
		uint64 auctionIndex;
	};

	/** @brief Auction data returned by the read-only auction getter. */
	struct GetAuctionByIndex_output
	{
		/** @brief Serializable auction data stored for the requested auction. */
		AuctionView auction;

		/** @brief Flag indicating whether the auction record exists. */
		uint8 found;
	};

	/** @brief Input payload used to fetch one participant record from an auction. */
	struct GetAuctionParticipant_input
	{
		/** @brief Monotonic index of the auction that owns the participant record. */
		uint64 auctionIndex;

		/** @brief Wallet whose participant record should be returned. */
		id participant;
	};

	/** @brief Participant data returned by the read-only participant getter. */
	struct GetAuctionParticipant_output
	{
		/** @brief Participant record for the requested wallet in the requested auction. */
		AuctionParticipantData participantData;

		/** @brief Flag indicating whether the participant record exists. */
		uint8 found;
	};

	/** @brief Input payload used to query the remaining post-BEGIN_EPOCH auction launch pause. */
	using GetTicksBeforeAuctionLaunch_input = NoData;

	/** @brief Result returned by the auction launch pause getter. */
	struct GetTicksBeforeAuctionLaunch_output
	{
		/** @brief Number of ticks remaining before auction interactions resume after `BEGIN_EPOCH`. */
		uint32 ticks;
	};

	/** @brief Input payload used to read the current auction fee configuration. */
	/** @brief Input payload used to read the amount of accumulated service fees awaiting distribution at `END_EPOCH`. */
	using GetPendingServiceFeePool_input = NoData;

	struct GetPendingServiceFeePool_output
	{
		/** @brief Aggregate fee amount still awaiting `END_EPOCH` settlement. */
		uint64 pendingServiceFeePool;
	};

	/** @brief Input payload used to inspect the detailed shared Nostromo fee pool. */
	using GetNostromoFeePool_input = NoData;

	struct GetNostromoFeePool_output
	{
		/** @brief Detailed fee accumulators that have not yet been moved to dividends or recipient payout liabilities. */
		NostromoFeePool feePool;

		/** @brief Aggregate of every amount in `feePool`. */
		uint64 totalAmount;
	};

	/** @brief Input used to inspect a wallet's registered QU payout. */
	struct GetPendingPayout_input
	{
		/** @brief Wallet whose unpaid QU amount should be returned. */
		id account;
	};

	struct GetPendingPayout_output
	{
		/** @brief QU currently owed to the requested wallet. */
		uint64 amount;
	};

	/** @brief Input payload used to read the current state of the execution fee reserve guard. */
	using GetFeeReserveGuardState_input = NoData;

	struct GetFeeReserveGuardState_output
	{
		/** @brief Live execution fee reserve value read from the system contract. */
		sint64 currentFeeReserve;

		/** @brief Execution fee reserve value recorded at the start of the current guard window. */
		sint64 feeReserveBaseline;

		/** @brief Start of the current guard window; invalid when the window has not been initialized. */
		DateAndTime feeReserveBaselineAt;

		/** @brief Timestamp at which the emergency pause was triggered; invalid when not paused. */
		DateAndTime emergencyPausedAt;

		/** @brief Configured drop in the execution fee reserve that triggers an emergency pause, in basis points. */
		uint64 dropBasisPoints;

		/** @brief Configured rolling window used to evaluate the execution fee reserve drop, in seconds. */
		uint64 windowSeconds;

		/** @brief Flag indicating whether an emergency pause is currently blocking auction interactions. */
		uint8 isEmergencyPaused;
	};

	using GetAuctionFees_input = NoData;

	struct GetAuctionFees_output
	{
		/** @brief Fee charged when a private auction is created. */
		sint64 privateAuctionFee;

		/** @brief Non-negative fee accumulated when a public auction is created and distributed at `END_EPOCH`. */
		sint64 publicAuctionCreationFee;

		/** @brief Cancellation fee rate in basis points. */
		uint64 auctionCancellationFeeBasisPoints;

		/** @brief Management fee rate in basis points. */
		uint64 managementFeeBasisPoints;

		/** @brief Development fee rate in basis points. */
		uint64 developmentFeeBasisPoints;

		/** @brief Takeover coordinator fee rate in basis points. */
		uint64 takeoverCoordinatorFeeBasisPoints;

		/** @brief Percentage of the shareholder fee distributed as dividends, in basis points. */
		uint64 shareholderDividendBasisPoints;

		/** @brief Shareholder fee tier for auctions up to the first threshold. */
		uint64 shareholderFeeBasisPointsTier1;

		/** @brief Shareholder fee tier for auctions above the first threshold and up to the second threshold. */
		uint64 shareholderFeeBasisPointsTier2;

		/** @brief Shareholder fee tier for auctions above the second threshold and up to the third threshold. */
		uint64 shareholderFeeBasisPointsTier3;

		/** @brief Shareholder fee tier for auctions above the third threshold. */
		uint64 shareholderFeeBasisPointsTier4;
	};

	/** @brief Input for the arithmetic-only Batch Auction bid reward calculator. */
	struct CalculateBatchAuctionBidFee_input
	{
		/** @brief Number of assets requested by the prospective bid. */
		uint64 bidQuantity;

		/** @brief Prospective price per asset, in qu. */
		uint64 bidAmount;
	};

	/** @brief Escrow, accumulated fee, and total reward required by the Batch Auction bid arithmetic. */
	struct CalculateBatchAuctionBidFee_output
	{
		/** @brief Saturating product of `bidQuantity` and `bidAmount`. */
		uint64 escrowAmount;

		/** @brief Amount accumulated for distribution at `END_EPOCH` for an accepted bid: `max(100 - bidQuantity * bidAmount, 0)`. */
		uint64 fee;

		/** @brief Saturating sum of `escrowAmount` and `fee`. */
		uint64 requiredReward;
	};

	/**
	 * @brief Pure breakdown of one auction revenue split.
	 * @note Runtime settlement and tests share this struct to keep fee arithmetic aligned.
	 */
	struct AuctionRevenueBreakdown
	{
		/** @brief Net amount that remains for the seller after every configured fee is applied. */
		uint64 sellerPayout;

		/** @brief Shareholder fee tier selected for the provided gross amount. */
		uint64 shareholderFeeBasisPoints;

		/** @brief Gross shareholder fee amount before dividend retention is split out. */
		uint64 shareholderFeeAmount;

		/** @brief Portion of the shareholder fee retained by the contract for dividend distribution. */
		uint64 shareholderDividendAmount;

		/** @brief Management wallet fee amount. */
		uint64 managementFeeAmount;

		/** @brief Development wallet fee amount. */
		uint64 developmentFeeAmount;

		/** @brief Base takeover coordinator fee charged directly from the gross amount. */
		uint64 takeoverCoordinatorBaseAmount;

		/** @brief Total takeover coordinator gain including retained shareholder-fee remainder. */
		uint64 takeoverCoordinatorFeeAmount;
	};

	/**
	 * @brief Pure breakdown of one service fee charged for private-auction creation or auction cancellation.
	 * @note Runtime settlement and tests share this struct to keep fee arithmetic aligned.
	 */
	struct AuctionServiceFeeBreakdown
	{
		/** @brief Portion retained by the contract for shareholder dividends. */
		uint64 shareholderDividendAmount;

		/** @brief Management wallet fee amount. */
		uint64 managementFeeAmount;

		/** @brief Development wallet fee amount. */
		uint64 developmentFeeAmount;

		/** @brief Takeover coordinator wallet fee amount. */
		uint64 takeoverCoordinatorFeeAmount;
	};

	/** @brief Input payload used to read the wallets that receive auction fee transfers. */
	using GetFeeRecipients_input = NoData;

	struct GetFeeRecipients_output
	{
		/** @brief Wallet that receives the management fee. */
		id management;

		/** @brief Wallet that receives the development fee. */
		id development;

		/** @brief Wallet that receives the takeover coordinator fee. */
		id takeoverCoordinator;
	};

	/** @brief Input payload used to read the closed auctions history ring buffer. */
	using GetClosedAuctionHistory_input = NoData;

	struct GetClosedAuctionHistory_output
	{
		/** @brief Ring buffer of auction indices recorded after finalization or cancellation. */
		Array<uint64, NOST_AUCTION_HISTORY_NUM> auctionIndices;

		/** @brief Total number of history writes since initialization. */
		uint64 totalEntries;
	};

	/** @brief Input payload used to read the temporary fee routing override flag. */
	using GetRouteAllFeesToDevelopment_input = NoData;

	struct GetRouteAllFeesToDevelopment_output
	{
		/** @brief `1` routes every fee to development, `0` uses the standard fee distribution. */
		uint8 enabled;
	};

	struct AuctionSummary
	{
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;
		id seller;
		id highestBidder;
		DateAndTime createdAt;
		DateAndTime settledAt;
		uint64 auctionIndex;
		uint64 quantityForSale;
		uint64 allocatedQuantity;
		uint64 initialPrice;
		uint64 salePrice;
		uint64 buyNowPrice;
		uint64 highestBidPrice;
		uint64 highestBidQuantity;
		uint64 highestBidAmount;
		uint8 type;
		uint8 visibility;
		uint8 status;
	};

	struct ParticipantSummary
	{
		id participant;
		DateAndTime lastBidTime;
		uint64 bidAmount;
		uint64 escrowedAmount;
		uint64 requestedQuantity;
		uint64 allocatedQuantity;
		uint8 isWinningBid;
	};

	struct UserParticipationSummary
	{
		id participant;
		DateAndTime lastBidTime;
		uint64 auctionIndex;
		uint64 bidAmount;
		uint64 escrowedAmount;
		uint64 requestedQuantity;
		uint64 allocatedQuantity;
		uint8 isWinningBid;
	};

	struct ContractStats
	{
		uint64 totalAuctionsCreated;
		uint64 activeAuctionCount;
		uint64 pendingSellerDecisionAuctionCount;
		uint64 finalizedAuctionCount;
		uint64 cancelledAuctionCount;
		uint64 participantCount;
		uint64 closedAuctionHistoryCounter;
		uint64 auctionShareholderDividendPool;
		uint64 pendingServiceFeePool;
		uint64 totalPendingQuPayouts;
		uint64 retainedClosedAuctionCount;
		uint64 retainedParticipantHistoryCount;
		uint32 qxTransferFee;
		uint8 routeAllFeesToDevelopment;
		uint8 isAuctionTimerPaused;
		uint8 isPostBeginEpochPauseArmed;
		uint8 isEmergencyPaused;
	};

	using GetContractStats_input = NoData;
	struct GetContractStats_output
	{
		ContractStats stats;
	};

	struct GetAuctionSummaries_input
	{
		uint64 offset;
		uint64 limit;
	};
	struct GetAuctionSummaries_output
	{
		Array<AuctionSummary, NOST_AUCTION_GETTER_PAGE_SIZE> auctions;
		uint64 totalCount;
		uint64 returnedCount;
	};

	struct GetActiveAuctionIndices_input
	{
		uint64 offset;
		uint64 limit;
	};
	struct GetActiveAuctionIndices_output
	{
		Array<uint64, NOST_AUCTION_GETTER_PAGE_SIZE> auctionIndices;
		uint64 totalCount;
		uint64 returnedCount;
	};

	struct GetAuctionsBySeller_input
	{
		id seller;
		uint64 offset;
		uint64 limit;
	};
	struct GetAuctionsBySeller_output
	{
		Array<AuctionSummary, NOST_AUCTION_GETTER_PAGE_SIZE> auctions;
		uint64 totalCount;
		uint64 returnedCount;
	};

	struct GetAuctionByMetadataCid_input
	{
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;
	};
	struct GetAuctionByMetadataCid_output
	{
		AuctionSummary auction;
		uint64 auctionIndex;
		uint8 found;
	};

	struct GetAuctionSummariesByIndexBatch_input
	{
		Array<uint64, NOST_AUCTION_GETTER_PAGE_SIZE> auctionIndices;
		uint64 count;
	};
	struct GetAuctionSummariesByIndexBatch_output
	{
		Array<AuctionSummary, NOST_AUCTION_GETTER_PAGE_SIZE> auctions;
		Array<uint8, NOST_AUCTION_GETTER_PAGE_SIZE> found;
		uint64 returnedCount;
	};

	struct GetAuctionParticipants_input
	{
		uint64 auctionIndex;
		uint64 offset;
		uint64 limit;
	};
	struct GetAuctionParticipants_output
	{
		Array<ParticipantSummary, NOST_AUCTION_GETTER_PAGE_SIZE> participants;
		uint64 totalCount;
		uint64 returnedCount;
	};

	struct GetUserParticipations_input
	{
		id participant;
		uint64 offset;
		uint64 limit;
	};
	struct GetUserParticipations_output
	{
		Array<UserParticipationSummary, NOST_AUCTION_GETTER_PAGE_SIZE> participations;
		uint64 totalCount;
		uint64 returnedCount;
	};

	using GetLatestAuctionIndex_input = NoData;
	struct GetLatestAuctionIndex_output
	{
		uint64 auctionIndex;
		uint8 found;
	};

	struct GetAuctionCountBySeller_input
	{
		id seller;
	};
	struct GetAuctionCountBySeller_output
	{
		uint64 count;
	};

	struct GetAuctionAtCreationSnapshot_input
	{
		uint64 auctionIndex;
	};
	struct GetAuctionAtCreationSnapshot_output
	{
		id seller;
		DateAndTime createdAt;
		uint64 auctionIndex;
		uint64 quantityForSale;
		uint64 initialPrice;
		uint64 salePrice;
		uint64 minimumBidIncrement;
		uint64 buyNowPrice;
		uint64 auctionDurationSeconds;
		uint8 type;
		uint8 visibility;
		uint8 found;
	};

	/** @brief Input payload used to read current bid capacity guidance for one active Batch Auction. */
	struct GetBatchAuctionBidAvailability_input
	{
		/** @brief Monotonic index of the Batch Auction to inspect. */
		uint64 auctionIndex;
	};

	/** @brief Read-only guidance for the next acceptable Batch Auction bid. */
	struct GetBatchAuctionBidAvailability_output
	{
		/** @brief Lowest price per asset that can currently accept a new bid meeting the auction minimum quantity. */
		uint64 minimumBidPrice;

		/** @brief Quantity available at `minimumBidPrice`; zero when no valid new bid can be accepted. */
		uint64 availableQuantity;

		/** @brief Flag indicating whether the auction exists. */
		uint8 found;

		/** @brief Flag indicating whether the auction is an active Batch Auction that can accept another valid bid. */
		uint8 isAcceptingBids;
	};

	/** @brief Internal input used to validate an auction lot and resolve its total escrow quantity. */
	struct AnalyzeAuctionLot_input
	{
		/** @brief Auction lot contents to validate. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Requested auction duration in days. */
		uint32 durationDays;
	};

	/** @brief Internal output returned after validating an auction lot. */
	struct AnalyzeAuctionLot_output
	{
		/** @brief Total quantity that must be escrowed from the lot. */
		uint64 totalEscrowQuantity;

		/** @brief Number of non-empty lot entries found in the lot. */
		uint64 lotItemCount;

		/** @brief Flag indicating whether the lot and duration are valid. */
		uint8 isValid;
	};

	struct AnalyzeAuctionLot_locals
	{
		AuctionAssetEntry lotItem;
		uint64 lotItemIndex;
	};

	/** @brief Internal input used to count non-empty wallet entries in a private wallet whitelist. */
	struct CountAllowedBidderWallets_input
	{
		/** @brief Wallet list provided for private wallet-based access control. */
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;
	};

	/** @brief Internal output containing the number of non-empty wallet whitelist entries. */
	struct CountAllowedBidderWallets_output
	{
		/** @brief Number of non-zero wallet entries found in the whitelist. */
		uint64 allowedWalletCount;
	};

	struct CountAllowedBidderWallets_locals
	{
		uint64 allowedWalletIndex;
	};

	/** @brief Internal input used to count non-empty asset entries in a private asset access list. */
	struct CountRequiredAccessAssets_input
	{
		/** @brief Asset and minimum-quantity alternatives provided for private access control. */
		Array<AuctionAssetEntry, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;
	};

	/** @brief Internal output containing the number of non-empty private access assets. */
	struct CountRequiredAccessAssets_output
	{
		/** @brief Number of populated asset entries found in the private access list. */
		uint64 requiredAccessAssetCount;

		/** @brief Flag indicating whether every entry has a valid asset/quantity combination. */
		uint8 isValid;
	};

	struct CountRequiredAccessAssets_locals
	{
		AuctionAssetEntry requiredAccessAsset;
		uint64 requiredAccessAssetIndex;
	};

	struct NostromoProcedureLog
	{
		uint32 contractIndex;
		uint32 errorCode;
		id actor;
		sint64 amount;
		uint64 auctionIndex;
		uint8 procedure;
		sint8 _terminator;
	};

	/** @brief Internal input used to locate either a live or retained closed auction. */
	struct FindAuction_input
	{
		uint64 auctionIndex;
	};

	struct FindAuction_output
	{
		AuctionData auction;
		uint8 found;
	};

	struct FindAuction_locals
	{
		AuctionData archivedAuction;
		uint64 historyIndex;
	};

	/** @brief Internal input used to test whether an auction remains in retained closed history. */
	struct IsClosedAuctionRetained_input
	{
		uint64 auctionIndex;
	};

	struct IsClosedAuctionRetained_output
	{
		uint8 found;
	};

	struct IsClosedAuctionRetained_locals
	{
		uint64 retainedClosedAuctionCount;
		uint64 historyIndex;
	};

	/** @brief Internal cursor used to enumerate retained auctions in ascending creation order. */
	struct SelectNextRetainedAuction_input
	{
		id seller;
		uint64 afterAuctionIndex;
		uint8 hasAfterAuctionIndex;
		uint8 includeClosedAuctions;
		uint8 filterBySeller;
	};

	struct SelectNextRetainedAuction_output
	{
		AuctionData auction;
		uint8 found;
	};

	struct SelectNextRetainedAuction_locals
	{
		AuctionData candidateAuction;
		uint64 retainedClosedAuctionCount;
		uint64 historyIndex;
		sint64 auctionElementIndex;
	};

	struct CountRetainedAuctionsBySeller_input
	{
		id seller;
	};

	struct CountRetainedAuctionsBySeller_output
	{
		uint64 count;
	};

	struct CountRetainedAuctionsBySeller_locals
	{
		AuctionData candidateAuction;
		uint64 retainedClosedAuctionCount;
		uint64 historyIndex;
		sint64 auctionElementIndex;
	};

	struct FindFirstRetainedAuctionByMetadataCid_input
	{
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;
	};

	struct FindFirstRetainedAuctionByMetadataCid_output
	{
		AuctionData auction;
		uint8 found;
	};

	struct FindFirstRetainedAuctionByMetadataCid_locals
	{
		AuctionData candidateAuction;
		uint64 retainedClosedAuctionCount;
		uint64 metadataIndex;
		uint64 historyIndex;
		sint64 auctionElementIndex;
		uint8 metadataMatches;
	};

	struct GetAuctionByIndex_locals
	{
		AuctionData auction;
		FindAuction_input findAuctionInput;
		FindAuction_output findAuctionOutput;
		AuctionAssetEntry requiredAccessAsset;
		id allowedBidderWallet;
		sint64 requiredAccessAssetSetIndex;
		sint64 allowedBidderWalletSetIndex;
	};

	struct GetterScan_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionSummary auctionSummary;
		ParticipantSummary participantSummary;
		UserParticipationSummary userParticipationSummary;
		SelectNextRetainedAuction_input selectNextAuctionInput;
		SelectNextRetainedAuction_output selectNextAuctionOutput;
		FindAuction_input findAuctionInput;
		FindAuction_output findAuctionOutput;
		uint64 auctionIndex;
		uint64 boundedLimit;
		uint64 metadataIndex;
		uint64 requestedIndex;
		uint64 participantSlotIndex;
		uint64 historyIndex;
		uint64 scannedAuctionCount;
		sint64 auctionElementIndex;
		uint8 metadataMatches;
	};

	using GetContractStats_locals = GetterScan_locals;

	struct GetAuctionSummaries_locals
	{
		AuctionData auction;
		AuctionSummary auctionSummary;
		SelectNextRetainedAuction_input selectNextAuctionInput;
		SelectNextRetainedAuction_output selectNextAuctionOutput;
		uint64 boundedLimit;
		uint64 scannedAuctionCount;
	};

	struct GetActiveAuctionIndices_locals
	{
		SelectNextRetainedAuction_input selectNextAuctionInput;
		SelectNextRetainedAuction_output selectNextAuctionOutput;
		uint64 boundedLimit;
		uint64 scannedAuctionCount;
	};

	struct GetAuctionsBySeller_locals
	{
		AuctionData auction;
		AuctionSummary auctionSummary;
		SelectNextRetainedAuction_input selectNextAuctionInput;
		SelectNextRetainedAuction_output selectNextAuctionOutput;
		CountRetainedAuctionsBySeller_input countAuctionsInput;
		CountRetainedAuctionsBySeller_output countAuctionsOutput;
		uint64 boundedLimit;
		uint64 scannedAuctionCount;
	};

	struct GetAuctionByMetadataCid_locals
	{
		FindFirstRetainedAuctionByMetadataCid_input findAuctionInput;
		FindFirstRetainedAuctionByMetadataCid_output findAuctionOutput;
	};

	using GetAuctionSummariesByIndexBatch_locals = GetterScan_locals;
	using GetAuctionParticipants_locals = GetterScan_locals;
	using GetUserParticipations_locals = GetterScan_locals;

	struct GetAuctionCountBySeller_locals
	{
		CountRetainedAuctionsBySeller_input countAuctionsInput;
		CountRetainedAuctionsBySeller_output countAuctionsOutput;
	};

	using GetAuctionAtCreationSnapshot_locals = GetterScan_locals;

	struct GetAuctionParticipant_locals
	{
		AuctionParticipantData participantData;
		uint64 participantSlotIndex;
		uint64 bestParticipantSlotIndex;
		uint8 bestParticipantFound;
	};

	struct GetClosedAuctionHistory_locals
	{
		AuctionData auction;
		uint64 historyIndex;
	};

	/** @brief Internal input used to compute Batch Auction capacity at a candidate bid price. */
	struct ComputeBatchBidAvailability_input
	{
		/** @brief Monotonic index of the Batch Auction to inspect. */
		uint64 auctionIndex;

		/** @brief Candidate bid price; zero returns capacity at the computed minimum valid price. */
		uint64 bidAmount;
	};

	using ComputeBatchBidAvailability_output = GetBatchAuctionBidAvailability_output;

	struct ComputeBatchBidAvailability_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		uint64 lowestWinningPrice;
		uint64 outputPrice;
		uint64 priorityQuantity;
		uint64 salePriorityQuantity;
		uint64 effectiveCoverageQuantity;
		uint64 participantIndex;
		uint8 lowestWinningPriceFound;
	};

	struct GetBatchAuctionBidAvailability_locals
	{
		ComputeBatchBidAvailability_input computeBatchBidAvailabilityInput;
		IsClosedAuctionRetained_input isClosedAuctionRetainedInput;
		IsClosedAuctionRetained_output isClosedAuctionRetainedOutput;
	};

	/** @brief Internal input used to verify whether the invocator satisfies any private asset requirement. */
	struct HasRequiredAccessAsset_input
	{
		/** @brief Monotonic index of the auction whose private asset-based access rules should be evaluated. */
		uint64 auctionIndex;
	};

	/** @brief Internal output of the private asset access check. */
	struct HasRequiredAccessAsset_output
	{
		/** @brief Flag indicating whether the invocator owns the minimum quantity of any required asset. */
		uint8 hasRequiredAccessAsset;
	};

	struct HasRequiredAccessAsset_locals
	{
		AuctionData auction;
		AuctionAssetEntry requiredAccessAsset;
		sint64 requiredAccessAssetSetIndex;
		sint64 possessedAccessShares;
	};

	/** @brief Internal input used to settle a batch auction after its bidding window closes. */
	struct FinalizeBatchAuction_input
	{
		/** @brief Timestamp used as the auction settlement time. */
		DateAndTime currentDate;

		/** @brief Monotonic index of the batch auction to finalize. */
		uint64 auctionIndex;
	};

	/** @brief Internal output returned after batch auction finalization. */
	struct FinalizeBatchAuction_output
	{
		/**
		 * @brief Flag indicating whether batch settlement finished successfully.
		 * @note Final allocations are never smaller than the auction minimum; any insufficient remainder is returned to the seller.
		 */
		uint8 success;
	};

	/** @brief Internal input used to settle a standard auction when it is accepted or auto-finalized. */
	struct FinalizeStandardAuction_input
	{
		/** @brief Timestamp used as the auction settlement time. */
		DateAndTime currentDate;

		/** @brief Monotonic index of the standard auction to finalize. */
		uint64 auctionIndex;
	};

	/** @brief Internal output returned after standard auction finalization. */
	struct FinalizeStandardAuction_output
	{
		/** @brief Flag indicating whether standard auction settlement finished successfully. */
		uint8 success;
	};

	/** @brief Internal input used to reject a pending standard auction during the seller decision window. */
	struct RejectStandardAuction_input
	{
		/** @brief Timestamp used as the auction settlement time. */
		DateAndTime currentDate;

		/** @brief Monotonic index of the pending standard auction to reject. */
		uint64 auctionIndex;
	};

	/** @brief Internal output returned after rejecting a pending standard auction. */
	struct RejectStandardAuction_output
	{
		/** @brief Amount refunded to the highest bidder after the rejection. */
		uint64 refundedAmount;

		/** @brief Flag indicating whether the rejection flow finished successfully. */
		uint8 success;
	};

	/** @brief Internal input used to evaluate whether auction interactions are currently paused. */
	struct IsAuctionInteractionPaused_input
	{
	};

	/** @brief Internal output of the auction interaction pause check. */
	struct IsAuctionInteractionPaused_output
	{
		/** @brief Flag indicating whether auction interactions are blocked by bootstrap time or by epoch timing pauses. */
		uint8 isPaused;
	};

	/** @brief Internal input used to resolve the currently active global auction pause interval. */
	struct GetAuctionPauseState_input
	{
	};

	/** @brief Internal output describing the current global auction pause interval. */
	struct GetAuctionPauseState_output
	{
		/** @brief Pause start timestamp for the current active pause interval. */
		DateAndTime pauseStartedAt;

		/** @brief Pause end timestamp for the current active pause interval. */
		DateAndTime pauseEndsAt;

		/** @brief Flag indicating whether auction timers are currently paused. */
		uint8 isPaused;
	};

	/** @brief Internal locals used to resolve the currently active global auction pause interval. */
	struct GetAuctionPauseState_locals
	{
		/** @brief Compact current date marker used to detect the bootstrap default time sentinel. */
		DateAndTime currentDate;
		uint32 currentDateStamp;
	};

	using SyncAuctionPauseState_input = NoData;
	using SyncAuctionPauseState_output = NoData;

	/** @brief Internal locals used to synchronize auction deadlines with the global pause interval. */
	struct SyncAuctionPauseState_locals
	{
		AuctionData auction;
		DateAndTime currentDate;
		GetAuctionPauseState_input getAuctionPauseStateInput;
		GetAuctionPauseState_output getAuctionPauseStateOutput;
		uint64 pausedSeconds;
		sint64 auctionIndex;
	};

	/** @brief Internal input used to split auction proceeds between seller and configured fee recipients. */
	struct DistributeAuctionRevenue_input
	{
		/** @brief Seller wallet that receives the net proceeds. */
		id seller;

		/** @brief Gross amount collected from the auction before fee distribution. */
		uint64 grossAmount;
	};

	/** @brief Internal output returned after auction revenue distribution is computed. */
	struct DistributeAuctionRevenue_output
	{
		/** @brief Net amount that should be transferred to the seller after auction fees. */
		uint64 sellerPayout;

		/** @brief Flag indicating whether the revenue distribution completed successfully. */
		uint8 success;
	};

	/** @brief Internal input used to accrue a service fee in the shared Nostromo fee pool. */
	struct AccumulateAuctionServiceFee_input
	{
		/** @brief Fee amount that should be accumulated. */
		uint64 feeAmount;
	};

	/** @brief Internal output returned after service-fee accrual is completed. */
	struct AccumulateAuctionServiceFee_output
	{
		/** @brief Flag indicating whether the service fee was recorded. */
		uint8 success;
	};

	using DistributeNostromoFeePool_input = NoData;

	struct DistributeNostromoFeePool_output
	{
		/** @brief Flag indicating whether every current pool accumulator was durably settled. */
		uint8 success;
	};

	/** @brief Internal input used to compute the remaining post-BEGIN_EPOCH launch pause. */
	struct GetTicksBeforeAuctionLaunchInternal_input
	{
	};

	/** @brief Internal output containing the remaining post-BEGIN_EPOCH launch pause. */
	struct GetTicksBeforeAuctionLaunchInternal_output
	{
		/** @brief Number of ticks remaining before auction interactions resume after `BEGIN_EPOCH`. */
		uint32 ticks;
	};

	struct GetTicksBeforeAuctionLaunchInternal_locals
	{
		DateAndTime currentDate;
		DateAndTime pauseEndsAt;
		uint64 remainingSeconds;
	};

	struct GetTicksBeforeAuctionLaunch_locals
	{
		DateAndTime currentDate;
		DateAndTime pauseEndsAt;
		uint64 remainingSeconds;
	};

	/** @brief Internal input used to register a QU liability before settlement side effects are committed. */
	struct QueueQuPayout_input
	{
		id recipient;
		uint64 amount;
	};

	struct QueueQuPayout_output
	{
		uint8 success;
	};

	struct QueueQuPayout_locals
	{
		uint64 previousAmount;
		uint64 updatedAmount;
		sint64 payoutIndex;
	};

	/** @brief Internal input used to discharge a bounded number of QPI-sized payout chunks. */
	struct FlushQuPayout_input
	{
		id recipient;
		uint64 maxChunks;
	};

	struct FlushQuPayout_output
	{
		uint64 transferredAmount;
		uint64 remainingAmount;
		uint8 success;
	};

	struct FlushQuPayout_locals
	{
		uint64 chunkAmount;
		uint64 chunkIndex;
		sint64 transferResult;
	};

	using ProcessPendingQuPayouts_input = NoData;
	using ProcessPendingQuPayouts_output = NoData;

	/** @brief Internal locals used by the bounded round-robin pending-payout processor. */
	struct ProcessPendingQuPayouts_locals
	{
		FlushQuPayout_input flushQuPayoutInput;
		FlushQuPayout_output flushQuPayoutOutput;
		id pendingPayoutRecipient;
		uint64 payoutScanIndex;
		uint64 payoutTargetRecipientCount;
		uint64 processedPayoutRecipientCount;
		sint64 payoutElementIndex;
	};

	struct QueueAndFlushQuPayout_input
	{
		id recipient;
		uint64 amount;
		uint64 maxChunks;
	};

	struct QueueAndFlushQuPayout_output
	{
		uint64 transferredAmount;
		uint64 remainingAmount;
		uint8 success;
	};

	struct QueueAndFlushQuPayout_locals
	{
		QueueQuPayout_input queueQuPayoutInput;
		QueueQuPayout_output queueQuPayoutOutput;
		FlushQuPayout_input flushQuPayoutInput;
		FlushQuPayout_output flushQuPayoutOutput;
	};

	/** @brief Internal input used to move a bid record from live storage into bounded history. */
	struct ArchiveParticipant_input
	{
		AuctionParticipantData participantData;
	};

	using ArchiveParticipant_output = NoData;

	struct ArchiveParticipant_locals
	{
		uint64 historyIndex;
	};

	/** @brief Internal input used to process a batch auction bid after the common PlaceBid checks succeed. */
	struct ProcessBatchBid_input
	{
		/** @brief Monotonic index of the target batch auction. */
		uint64 auctionIndex;

		/** @brief Quantity requested by the bidder in the batch auction. */
		uint64 effectiveQuantity;

		/** @brief Offered price per asset for the requested quantity in the batch auction. */
		uint64 bidAmount;

		/** @brief Timestamp of the accepted bid. */
		DateAndTime currentDate;

		/** @brief Seconds elapsed since auction creation at the moment of the bid. */
		uint64 elapsedSeconds;
	};

	/** @brief Internal input used to refresh the cached highest bid fields of one batch auction. */
	struct RecomputeBatchHighestBid_input
	{
		/** @brief Monotonic index of the batch auction whose cached top bid must be rebuilt. */
		uint64 auctionIndex;
	};

	using RecomputeBatchHighestBid_output = NoData;

	/** @brief Internal output returned after processing a batch auction bid. */
	struct ProcessBatchBid_output
	{
		/** @brief Amount that remains escrowed for the accepted batch bid. */
		uint64 escrowedAmount;

		/** @brief Amount refunded during batch bid processing. */
		uint64 refundedAmount;

		/** @brief Result code describing whether the batch bid processing succeeded. */
		EAuctionError errorCode;

		/** @brief Flag indicating whether batch bid processing completed successfully. */
		uint8 success;
	};

	struct ProcessBatchBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData worstParticipantData;
		ComputeBatchBidAvailability_input computeBatchBidAvailabilityInput;
		ComputeBatchBidAvailability_output computeBatchBidAvailabilityOutput;
		RecomputeBatchHighestBid_input recomputeBatchHighestBidInput;
		RecomputeBatchHighestBid_output recomputeBatchHighestBidOutput;
		ArchiveParticipant_input archiveParticipantInput;
		ArchiveParticipant_output archiveParticipantOutput;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		AccumulateAuctionServiceFee_input accumulateAuctionServiceFeeInput;
		AccumulateAuctionServiceFee_output accumulateAuctionServiceFeeOutput;
		uint64 activeQuantity;
		uint64 displacedQuantity;
		uint64 displacedRefund;
		uint64 excessQuantity;
		uint64 remainingWorstQuantity;
		CalculateBatchAuctionBidFee_output bidFeeCalculation;
		uint64 participantIndex;
		uint64 freeParticipantSlotIndex;
		uint64 worstParticipantSlotIndex;
		uint8 worstParticipantFound;
		uint8 freeParticipantSlotFound;
	};

	struct RecomputeBatchHighestBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData bestParticipantData;
		uint64 participantIndex;
		uint64 bestParticipantSlotIndex;
		uint8 bestParticipantFound;
	};

	/** @brief Internal input used to process a standard auction bid after the common PlaceBid checks succeed. */
	struct ProcessStandardBid_input
	{
		/** @brief Monotonic index of the target standard auction. */
		uint64 auctionIndex;

		/** @brief Total amount the bidder commits for the standard auction lot. */
		uint64 bidAmount;

		/** @brief Timestamp of the accepted bid. */
		DateAndTime currentDate;

		/** @brief Seconds elapsed since auction creation at the moment of the bid. */
		uint64 elapsedSeconds;
	};

	/** @brief Internal output returned after processing a standard auction bid. */
	struct ProcessStandardBid_output
	{
		/** @brief Amount that remains escrowed for the accepted standard bid. */
		uint64 escrowedAmount;

		/** @brief Amount refunded during standard bid processing. */
		uint64 refundedAmount;

		/** @brief Result code describing whether the standard bid processing succeeded. */
		EAuctionError errorCode;

		/** @brief Flag indicating whether standard bid processing completed successfully. */
		uint8 success;
	};

	struct ProcessStandardBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData previousHighestBidderData;
		FinalizeStandardAuction_input finalizeStandardAuctionInput;
		FinalizeStandardAuction_output finalizeStandardAuctionOutput;
		ArchiveParticipant_input archiveParticipantInput;
		ArchiveParticipant_output archiveParticipantOutput;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		uint64 previousEscrow;
		uint64 requiredEscrow;
		uint64 participantSlotIndex;
		uint64 highestBidderSlotIndex;
		uint64 freeParticipantSlotIndex;
		uint8 participantExists;
		uint8 highestBidderExists;
		uint8 freeParticipantSlotFound;
		uint8 finalizeImmediately;
	};

	/** @brief Internal input used to validate the IPFS metadata CID format required by the Auction House. */
	struct ValidateMetadataCid_input
	{
		/** @brief Candidate lowercase base32 CIDv1 for auction metadata stored in Pinata. */
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;
	};

	/** @brief Internal output of the metadata CID validation routine. */
	struct ValidateMetadataCid_output
	{
		/** @brief Flag indicating whether the metadata CID has the required lowercase base32 CIDv1 format. */
		uint8 isValid;
	};

	struct ValidateMetadataCid_locals
	{
		uint64 cidIndex;
		uint8 cidChar;
		uint8 hasPayloadCharacters;
		uint8 reachedTerminator;
	};

	/** @brief Internal input used to verify that the seller owns enough shares for every asset in the auction lot. */
	struct VerifyAuctionLotBalances_input
	{
		/** @brief Auction lot that should be checked against the seller balance. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	/** @brief Internal output of the seller balance verification routine. */
	struct VerifyAuctionLotBalances_output
	{
		/** @brief Flag indicating whether the seller owns enough shares for the entire lot. */
		uint8 hasEnoughBalance;
	};

	struct VerifyAuctionLotBalances_locals
	{
		AuctionAssetEntry lotItem;
		uint64 lotItemIndex;
		sint64 possessedShares;
	};

	/** @brief Internal input used to transfer the auction lot from the seller into contract escrow. */
	struct EscrowAuctionLotAssets_input
	{
		/** @brief Auction lot that must be moved into contract escrow. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	/** @brief Internal output of the lot escrow routine. */
	struct EscrowAuctionLotAssets_output
	{
		/** @brief Flag indicating whether every lot asset was successfully escrowed. */
		uint8 success;
	};

	struct EscrowAuctionLotAssets_locals
	{
		AuctionAssetEntry lotItem;
		uint64 lotItemIndex;
		uint64 rollbackLotItemIndex;
		sint64 remainingShares;
	};

	/** @brief Internal input used to return an auction lot from contract escrow to a target wallet. */
	struct RollbackAuctionLotAssets_input
	{
		/** @brief Auction lot that must be transferred out of contract escrow. */
		Array<AuctionAssetEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Destination wallet that should receive the lot from escrow. */
		id recipient;
	};

	using RollbackAuctionLotAssets_output = NoData;

	struct RollbackAuctionLotAssets_locals
	{
		AuctionAssetEntry lotItem;
		uint64 lotItemIndex;
	};

	/** @brief Internal input used to archive and remove a closed auction from active storage. */
	struct ArchiveClosedAuction_input
	{
		AuctionData auction;
	};

	using ArchiveClosedAuction_output = NoData;

	struct ArchiveClosedAuction_locals
	{
		uint64 historyIndex;
	};

	struct FinalizeBatchAuction_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData bestParticipantData;
		AuctionAssetEntry batchLotItem;
		DistributeAuctionRevenue_input distributeAuctionRevenueInput;
		DistributeAuctionRevenue_output distributeAuctionRevenueOutput;
		ArchiveParticipant_input archiveParticipantInput;
		ArchiveParticipant_output archiveParticipantOutput;
		ArchiveClosedAuction_input archiveClosedAuctionInput;
		ArchiveClosedAuction_output archiveClosedAuctionOutput;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		DateAndTime currentDate;
		uint64 remainingQuantity;
		uint64 allocatedQuantity;
		uint64 requiredPayment;
		uint64 refundAmount;
		uint64 soldQuantity;
		uint64 totalGrossAmount;
		uint64 lotItemIndex;
		uint64 participantIndex;
		uint64 bestParticipantSlotIndex;
		uint8 bestParticipantFound;
		uint8 lotItemFound;
	};

	struct FinalizeStandardAuction_locals
	{
		AuctionData auction;
		AuctionParticipantData highestBidderData;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		DistributeAuctionRevenue_input distributeAuctionRevenueInput;
		DistributeAuctionRevenue_output distributeAuctionRevenueOutput;
		ArchiveParticipant_input archiveParticipantInput;
		ArchiveParticipant_output archiveParticipantOutput;
		ArchiveClosedAuction_input archiveClosedAuctionInput;
		ArchiveClosedAuction_output archiveClosedAuctionOutput;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		uint64 highestBidderSlotIndex;
		uint8 highestBidderExists;
		uint8 lotSold;
	};

	struct DistributeAuctionRevenue_locals
	{
		AuctionRevenueBreakdown auctionRevenueBreakdown;
		NostromoFeePool feePool;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		uint64 shareholderFeeTierIndex;
	};

	struct AccumulateAuctionServiceFee_locals
	{
		NostromoFeePool feePool;
	};

	struct DistributeNostromoFeePool_locals
	{
		AuctionServiceFeeBreakdown auctionServiceFeeBreakdown;
		NostromoFeePool feePool;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		uint64 shareholderDividendAmount;
		uint64 distributedDividendAmount;
		uint64 dividendPerShare;
	};

	struct RejectStandardAuction_locals
	{
		AuctionData auction;
		AuctionParticipantData highestBidderData;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		ArchiveParticipant_input archiveParticipantInput;
		ArchiveParticipant_output archiveParticipantOutput;
		ArchiveClosedAuction_input archiveClosedAuctionInput;
		ArchiveClosedAuction_output archiveClosedAuctionOutput;
		QueueAndFlushQuPayout_input payoutInput;
		QueueAndFlushQuPayout_output payoutOutput;
		uint64 highestBidderSlotIndex;
		uint8 highestBidderExists;
	};

	struct CreateAuction_locals
	{
		AuctionData auction;
		NostromoProcedureLog log;
		IsAuctionInteractionPaused_input isAuctionInteractionPausedInput;
		IsAuctionInteractionPaused_output isAuctionInteractionPausedOutput;
		ValidateMetadataCid_input validateMetadataCidInput;
		ValidateMetadataCid_output validateMetadataCidOutput;
		AnalyzeAuctionLot_input analyzeAuctionLotInput;
		AnalyzeAuctionLot_output analyzeAuctionLotOutput;
		CountAllowedBidderWallets_input countAllowedBidderWalletsInput;
		CountAllowedBidderWallets_output countAllowedBidderWalletsOutput;
		CountRequiredAccessAssets_input countRequiredAccessAssetsInput;
		CountRequiredAccessAssets_output countRequiredAccessAssetsOutput;
		AuctionAssetEntry requiredAccessAsset;
		VerifyAuctionLotBalances_input verifyAuctionLotBalancesInput;
		EscrowAuctionLotAssets_input escrowAuctionLotAssetsInput;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		AccumulateAuctionServiceFee_input accumulateAuctionServiceFeeInput;
		sint64 requiredFee;
		sint64 existingRequiredAccessQuantity;
		uint64 resolvedQuantityForSale;
		uint64 resolvedMinimumPurchaseQuantity;
		uint64 allowedWalletIndex;
		uint64 requiredAccessAssetIndex;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		EscrowAuctionLotAssets_output escrowAuctionLotAssetsOutput;
		VerifyAuctionLotBalances_output verifyAuctionLotBalancesOutput;
		AccumulateAuctionServiceFee_output accumulateAuctionServiceFeeOutput;
	};

	struct PlaceBid_locals
	{
		AuctionData auction;
		FindAuction_input findAuctionInput;
		FindAuction_output findAuctionOutput;
		NostromoProcedureLog log;
		IsAuctionInteractionPaused_input isAuctionInteractionPausedInput;
		IsAuctionInteractionPaused_output isAuctionInteractionPausedOutput;
		HasRequiredAccessAsset_input hasRequiredAccessAssetInput;
		HasRequiredAccessAsset_output hasRequiredAccessAssetOutput;
		ProcessBatchBid_input processBatchBidInput;
		ProcessBatchBid_output processBatchBidOutput;
		ProcessStandardBid_input processStandardBidInput;
		ProcessStandardBid_output processStandardBidOutput;
		uint64 elapsedSeconds;
		DateAndTime currentDate;
		uint8 hasAccess;
	};

	struct CancelAuction_locals
	{
		AuctionData auction;
		FindAuction_input findAuctionInput;
		FindAuction_output findAuctionOutput;
		AuctionParticipantData participantData;
		NostromoProcedureLog log;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		AccumulateAuctionServiceFee_input accumulateAuctionServiceFeeInput;
		AccumulateAuctionServiceFee_output accumulateAuctionServiceFeeOutput;
		ArchiveClosedAuction_input archiveClosedAuctionInput;
		ArchiveClosedAuction_output archiveClosedAuctionOutput;
		DateAndTime currentDate;
		uint64 cancellationBaseAmount;
		uint64 participantIndex;
	};

	struct ResolvePendingStandardAuction_locals
	{
		AuctionData auction;
		FindAuction_input findAuctionInput;
		FindAuction_output findAuctionOutput;
		DateAndTime currentDate;
		NostromoProcedureLog log;

		IsAuctionInteractionPaused_input isAuctionInteractionPausedInput;
		IsAuctionInteractionPaused_output isAuctionInteractionPausedOutput;
		FinalizeStandardAuction_input finalizeStandardAuctionInput;
		FinalizeStandardAuction_output finalizeStandardAuctionOutput;
		RejectStandardAuction_input rejectStandardAuctionInput;
		RejectStandardAuction_output rejectStandardAuctionOutput;
	};

	struct END_TICK_locals
	{
		AuctionData auction;
		DateAndTime currentDate;
		SyncAuctionPauseState_input syncAuctionPauseStateInput;
		SyncAuctionPauseState_output syncAuctionPauseStateOutput;
		uint64 elapsedSeconds;
		uint32 currentDateStamp;
		sint64 auctionIndex;
		FinalizeBatchAuction_input finalizeBatchAuctionInput;
		FinalizeBatchAuction_output finalizeBatchAuctionOutput;
		FinalizeStandardAuction_input finalizeStandardAuctionInput;
		FinalizeStandardAuction_output finalizeStandardAuctionOutput;
		sint64 currentReserve;
		sint64 reserveDrop;
		uint64 guardElapsedSeconds;
		uint64 guardDropThreshold;
	};

	struct BEGIN_EPOCH_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	struct END_EPOCH_locals
	{
		DistributeNostromoFeePool_input distributeNostromoFeePoolInput;
		DistributeNostromoFeePool_output distributeNostromoFeePoolOutput;
		ProcessPendingQuPayouts_input processPendingQuPayoutsInput;
		ProcessPendingQuPayouts_output processPendingQuPayoutsOutput;
	};

	/** @brief Input payload used to move share management rights to another managing contract. */
	struct TransferShareManagementRights_input
	{
		/** @brief Asset whose management rights should be transferred. */
		Asset asset;

		/** @brief Number of shares whose management rights should be transferred. */
		sint64 numberOfShares;

		/** @brief Destination managing contract index. */
		uint32 newManagingContractIndex;
	};

	/** @brief Result of a share management rights transfer request. */
	struct TransferShareManagementRights_output
	{
		/** @brief Number of shares whose management rights were transferred. */
		sint64 transferredNumberOfShares;

		/** @brief Result code describing whether the transfer request succeeded. */
		EAuctionError errorCode;
	};

	struct TransferShareManagementRights_locals
	{
		NostromoProcedureLog log;

		sint64 result;
		sint64 reward;
		sint64 refundAmount;
		bit success;
	};

	struct SetAuctionFees_locals
	{
		NostromoProcedureLog log;
	};

	struct SetAuctionFeesByManagement_locals
	{
		NostromoProcedureLog log;
	};

	struct SetManagement_locals
	{
		NostromoProcedureLog log;
	};

	struct SetFeeReserveGuardConfig_locals
	{
		NostromoProcedureLog log;
	};

	struct SetEmergencyPause_locals
	{
		NostromoProcedureLog log;
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateAuction, static_cast<uint16>(EProcedureId::CreateAuction));
		REGISTER_USER_PROCEDURE(PlaceBid, static_cast<uint16>(EProcedureId::PlaceBid));
		REGISTER_USER_PROCEDURE(CancelAuction, static_cast<uint16>(EProcedureId::CancelAuction));
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, static_cast<uint16>(EProcedureId::TransferShareManagementRights));
		REGISTER_USER_PROCEDURE(ResolvePendingStandardAuction, static_cast<uint16>(EProcedureId::ResolvePendingStandardAuction));
		REGISTER_USER_PROCEDURE(SetAuctionFees, static_cast<uint16>(EProcedureId::SetAuctionFees));
		REGISTER_USER_PROCEDURE(SetAuctionFeesByManagement, static_cast<uint16>(EProcedureId::SetAuctionFeesByManagement));
		REGISTER_USER_PROCEDURE(SetManagement, static_cast<uint16>(EProcedureId::SetManagement));
		REGISTER_USER_PROCEDURE(SetFeeReserveGuardConfig, static_cast<uint16>(EProcedureId::SetFeeReserveGuardConfig));
		REGISTER_USER_PROCEDURE(SetEmergencyPause, static_cast<uint16>(EProcedureId::SetEmergencyPause));

		REGISTER_USER_FUNCTION(GetAuctionByIndex, static_cast<uint16>(EFunctionId::GetAuctionByIndex));
		REGISTER_USER_FUNCTION(GetAuctionParticipant, static_cast<uint16>(EFunctionId::GetAuctionParticipant));
		REGISTER_USER_FUNCTION(GetTicksBeforeAuctionLaunch, static_cast<uint16>(EFunctionId::GetTicksBeforeAuctionLaunch));
		REGISTER_USER_FUNCTION(GetAuctionFees, static_cast<uint16>(EFunctionId::GetAuctionFees));
		REGISTER_USER_FUNCTION(GetFeeRecipients, static_cast<uint16>(EFunctionId::GetFeeRecipients));
		REGISTER_USER_FUNCTION(GetClosedAuctionHistory, static_cast<uint16>(EFunctionId::GetClosedAuctionHistory));
		REGISTER_USER_FUNCTION(GetRouteAllFeesToDevelopment, static_cast<uint16>(EFunctionId::GetRouteAllFeesToDevelopment));
		REGISTER_USER_FUNCTION(GetContractStats, static_cast<uint16>(EFunctionId::GetContractStats));
		REGISTER_USER_FUNCTION(GetAuctionSummaries, static_cast<uint16>(EFunctionId::GetAuctionSummaries));
		REGISTER_USER_FUNCTION(GetActiveAuctionIndices, static_cast<uint16>(EFunctionId::GetActiveAuctionIndices));
		REGISTER_USER_FUNCTION(GetAuctionsBySeller, static_cast<uint16>(EFunctionId::GetAuctionsBySeller));
		REGISTER_USER_FUNCTION(GetAuctionByMetadataCid, static_cast<uint16>(EFunctionId::GetAuctionByMetadataCid));
		REGISTER_USER_FUNCTION(GetAuctionSummariesByIndexBatch, static_cast<uint16>(EFunctionId::GetAuctionSummariesByIndexBatch));
		REGISTER_USER_FUNCTION(GetAuctionParticipants, static_cast<uint16>(EFunctionId::GetAuctionParticipants));
		REGISTER_USER_FUNCTION(GetUserParticipations, static_cast<uint16>(EFunctionId::GetUserParticipations));
		REGISTER_USER_FUNCTION(GetLatestAuctionIndex, static_cast<uint16>(EFunctionId::GetLatestAuctionIndex));
		REGISTER_USER_FUNCTION(GetAuctionCountBySeller, static_cast<uint16>(EFunctionId::GetAuctionCountBySeller));
		REGISTER_USER_FUNCTION(GetAuctionAtCreationSnapshot, static_cast<uint16>(EFunctionId::GetAuctionAtCreationSnapshot));
		REGISTER_USER_FUNCTION(GetBatchAuctionBidAvailability, static_cast<uint16>(EFunctionId::GetBatchAuctionBidAvailability));
		REGISTER_USER_FUNCTION(CalculateBatchAuctionBidFee, static_cast<uint16>(EFunctionId::CalculateBatchAuctionBidFee));
		REGISTER_USER_FUNCTION(GetPendingServiceFeePool, static_cast<uint16>(EFunctionId::GetPendingServiceFeePool));
		REGISTER_USER_FUNCTION(GetFeeReserveGuardState, static_cast<uint16>(EFunctionId::GetFeeReserveGuardState));
		REGISTER_USER_FUNCTION(GetPendingPayout, static_cast<uint16>(EFunctionId::GetPendingPayout));
		REGISTER_USER_FUNCTION(GetNostromoFeePool, static_cast<uint16>(EFunctionId::GetNostromoFeePool));
	}

	/**
	 * @brief Initializes default governance, fee, pause, and guard settings.
	 */
	INITIALIZE()
	{
		// Install the default governance, fee, pause, and guard configuration into the zeroed contract state.
		state.mut().privateAuctionFee = NOST_DEFAULT_PRIVATE_AUCTION_FEE;
		state.mut().publicAuctionCreationFee = NOST_PUBLIC_AUCTION_CREATION_FEE;
		state.mut().auctionCancellationFeeBasisPoints = NOST_DEFAULT_AUCTION_CANCELLATION_FEE_BP;
		state.mut().managementFeeBasisPoints = NOST_DEFAULT_AUCTION_MANAGEMENT_FEE_BP;
		state.mut().developmentFeeBasisPoints = NOST_DEFAULT_AUCTION_DEVELOPMENT_FEE_BP;
		state.mut().takeoverCoordinatorFeeBasisPoints = NOST_DEFAULT_AUCTION_TAKEOVER_COORDINATOR_FEE_BP;
		state.mut().shareholderDividendBasisPoints = NOST_DEFAULT_AUCTION_SHAREHOLDER_DIVIDEND_BP;
		state.mut().shareholderFeeBasisPointsTier1 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_1;
		state.mut().shareholderFeeBasisPointsTier2 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_2;
		state.mut().shareholderFeeBasisPointsTier3 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_3;
		state.mut().shareholderFeeBasisPointsTier4 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_4;
		state.mut().maxAuctionDurationDays = NOST_AUCTION_MAX_DURATION_DAYS;
		state.mut().isAuctionTimerPaused = 1;
		state.mut().routeAllFeesToDevelopment = NOST_ROUTE_ALL_FEES_TO_DEVELOPMENT;
		state.mut().auctionTimerPauseStartedAt.setInvalid();
		state.mut().auctionTimerPauseEndsAt.setInvalid();
		state.mut().feeReserveGuardDropBasisPoints = NOST_DEFAULT_FEE_RESERVE_GUARD_DROP_BP;
		state.mut().feeReserveGuardWindowSeconds = NOST_DEFAULT_FEE_RESERVE_GUARD_WINDOW_SECONDS;
		state.mut().management = ID(_I, _G, _P, _Z, _X, _Q, _O, _R, _J, _Y, _Q, _P, _A, _G, _V, _A, _B, _N, _T, _N, _I, _S, _O, _Y, _T, _M, _T, _A,
		                            _N, _M, _K, _Z, _A, _S, _T, _P, _P, _G, _Z, _O, _N, _A, _Q, _J, _X, _Q, _O, _S, _W, _Q, _O, _V, _J, _C, _K, _D);
		state.mut().development = ID(_D, _Q, _V, _H, _M, _Z, _F, _C, _W, _O, _K, _M, _H, _F, _B, _H, _L, _X, _U, _I, _U, _G, _P, _P, _X, _R, _Z, _C,
		                             _U, _V, _S, _N, _J, _F, _Z, _J, _F, _M, _Q, _M, _Y, _D, _B, _X, _E, _S, _E, _A, _T, _M, _W, _L, _K, _N, _L, _D);
		state.mut().takeoverCoordinator =
		    ID(_X, _J, _O, _S, _N, _L, _T, _Z, _V, _V, _H, _N, _Z, _C, _B, _Y, _X, _I, _E, _V, _N, _E, _P, _P, _B, _O, _Q, _A, _W, _D, _B, _V, _G, _E,
		       _N, _Z, _O, _X, _S, _V, _O, _B, _K, _G, _Z, _C, _C, _F, _D, _B, _D, _M, _T, _M, _L, _C);
	}

	MIGRATE()
	{
		state.mut().privateAuctionFee = NOST_DEFAULT_PRIVATE_AUCTION_FEE;
		state.mut().publicAuctionCreationFee = NOST_PUBLIC_AUCTION_CREATION_FEE;
		state.mut().auctionCancellationFeeBasisPoints = NOST_DEFAULT_AUCTION_CANCELLATION_FEE_BP;
		state.mut().managementFeeBasisPoints = NOST_DEFAULT_AUCTION_MANAGEMENT_FEE_BP;
		state.mut().developmentFeeBasisPoints = NOST_DEFAULT_AUCTION_DEVELOPMENT_FEE_BP;
		state.mut().takeoverCoordinatorFeeBasisPoints = NOST_DEFAULT_AUCTION_TAKEOVER_COORDINATOR_FEE_BP;
		state.mut().shareholderDividendBasisPoints = NOST_DEFAULT_AUCTION_SHAREHOLDER_DIVIDEND_BP;
		state.mut().shareholderFeeBasisPointsTier1 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_1;
		state.mut().shareholderFeeBasisPointsTier2 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_2;
		state.mut().shareholderFeeBasisPointsTier3 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_3;
		state.mut().shareholderFeeBasisPointsTier4 = NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_4;
		state.mut().maxAuctionDurationDays = NOST_AUCTION_MAX_DURATION_DAYS;
		state.mut().routeAllFeesToDevelopment = NOST_ROUTE_ALL_FEES_TO_DEVELOPMENT;
		state.mut().feeReserveGuardDropBasisPoints = NOST_DEFAULT_FEE_RESERVE_GUARD_DROP_BP;
		state.mut().feeReserveGuardWindowSeconds = NOST_DEFAULT_FEE_RESERVE_GUARD_WINDOW_SECONDS;
		state.mut().management = ID(_I, _G, _P, _Z, _X, _Q, _O, _R, _J, _Y, _Q, _P, _A, _G, _V, _A, _B, _N, _T, _N, _I, _S, _O, _Y, _T, _M, _T, _A,
		                            _N, _M, _K, _Z, _A, _S, _T, _P, _P, _G, _Z, _O, _N, _A, _Q, _J, _X, _Q, _O, _S, _W, _Q, _O, _V, _J, _C, _K, _D);
		state.mut().development = ID(_D, _Q, _V, _H, _M, _Z, _F, _C, _W, _O, _K, _M, _H, _F, _B, _H, _L, _X, _U, _I, _U, _G, _P, _P, _X, _R, _Z, _C,
		                             _U, _V, _S, _N, _J, _F, _Z, _J, _F, _M, _Q, _M, _Y, _D, _B, _X, _E, _S, _E, _A, _T, _M, _W, _L, _K, _N, _L, _D);
		state.mut().takeoverCoordinator =
		    ID(_X, _J, _O, _S, _N, _L, _T, _Z, _V, _V, _H, _N, _Z, _C, _B, _Y, _X, _I, _E, _V, _N, _E, _P, _P, _B, _O, _Q, _A, _W, _D, _B, _V, _G, _E,
		       _N, _Z, _O, _X, _S, _V, _O, _B, _K, _G, _Z, _C, _C, _F, _D, _B, _D, _M, _T, _M, _L, _C);
	}

	/**
	 * @brief Allows share acquisition without charging an additional contract fee.
	 */
	PRE_ACQUIRE_SHARES()
	{
		output.requestedFee = 0;
		output.allowTransfer = true;
	}

	/**
	 * @brief Refreshes epoch-scoped configuration and arms auction timer pauses.
	 */
	BEGIN_EPOCH_WITH_LOCALS()
	{
		// Refresh the QX fee cache once per epoch so share transfers can expose current cost guidance.
		CALL_OTHER_CONTRACT_FUNCTION(QX, Fees, locals.feesInput, locals.feesOutput);
		// Preserve the previous cache when QX is temporarily unavailable; a failed call must not install an undefined fee.
		if (interContractCallError == NoCallError)
		{
			state.mut().qxTransferFee = locals.feesOutput.transferFee;
		}

		// Freeze auction timers across the epoch boundary; END_TICK later accounts this pause back into deadlines.
		state.mut().isPostBeginEpochPauseArmed = 1;
		if (!state.get().isAuctionTimerPaused)
		{
			state.mut().isAuctionTimerPaused = 1;
			state.mut().auctionTimerPauseStartedAt = qpi.now();
			state.mut().auctionTimerPauseEndsAt = qpi.now();
			return;
		}

		if (!state.get().auctionTimerPauseStartedAt.isValid() || qpi.now() < state.get().auctionTimerPauseStartedAt)
		{
			state.mut().auctionTimerPauseStartedAt = qpi.now();
		}
		if (!state.get().auctionTimerPauseEndsAt.isValid() || qpi.now() > state.get().auctionTimerPauseEndsAt)
		{
			state.mut().auctionTimerPauseEndsAt = qpi.now();
		}
	}

	/**
	 * @brief Retries pending QU payouts, settles the shared Nostromo fee pool, and performs storage cleanup.
	 */
	END_EPOCH_WITH_LOCALS()
	{
		CALL(ProcessPendingQuPayouts, locals.processPendingQuPayoutsInput, locals.processPendingQuPayoutsOutput);

		CALL(DistributeNostromoFeePool, locals.distributeNostromoFeePoolInput, locals.distributeNostromoFeePoolOutput);

		state.mut().auctionList.cleanupIfNeeded();
		state.mut().pendingQuPayouts.cleanupIfNeeded();
	}

	/**
	 * @brief Advances auction lifecycle state and finalizes auctions whose deadlines elapsed.
	 */
	END_TICK_WITH_LOCALS()
	{
		makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);
		locals.currentDate = qpi.now();

		// The reserve guard converts a sudden execution-fee reserve drop into an emergency pause.
		if (!state.get().isEmergencyPaused)
		{
			locals.currentReserve = qpi.queryFeeReserve(SELF_INDEX);
			// The first observation establishes a baseline instead of interpreting startup state as a reserve drop.
			if (!state.get().feeReserveBaselineAt.isValid())
			{
				state.mut().feeReserveBaseline = locals.currentReserve;
				state.mut().feeReserveBaselineAt = locals.currentDate;
			}
			else
			{
				// Subsequent observations either trigger the guard or roll the baseline into a new window.
				diffDateInSecond(state.get().feeReserveBaselineAt, locals.currentDate, locals.guardElapsedSeconds);
				locals.reserveDrop = state.get().feeReserveBaseline - locals.currentReserve;
				if (state.get().feeReserveBaseline > 0 && locals.reserveDrop > 0)
				{
					locals.guardDropThreshold =
					    div<uint64>(smul(static_cast<uint64>(state.get().feeReserveBaseline), state.get().feeReserveGuardDropBasisPoints),
					                NOST_BASIS_POINTS_SCALE);
					if (static_cast<uint64>(locals.reserveDrop) >= locals.guardDropThreshold &&
					    locals.guardElapsedSeconds <= state.get().feeReserveGuardWindowSeconds)
					{
						state.mut().isEmergencyPaused = 1;
						state.mut().emergencyPausedAt = locals.currentDate;
						state.mut().feeReserveBaselineAt.setInvalid();
					}
					else if (locals.guardElapsedSeconds >= state.get().feeReserveGuardWindowSeconds)
					{
						state.mut().feeReserveBaseline = locals.currentReserve;
						state.mut().feeReserveBaselineAt = locals.currentDate;
					}
				}
				else if (locals.guardElapsedSeconds >= state.get().feeReserveGuardWindowSeconds)
				{
					state.mut().feeReserveBaseline = locals.currentReserve;
					state.mut().feeReserveBaselineAt = locals.currentDate;
				}
			}
		}

		CALL(SyncAuctionPauseState, locals.syncAuctionPauseStateInput, locals.syncAuctionPauseStateOutput);
		// Lifecycle transitions must not advance while SyncAuctionPauseState still owns the global timer freeze.
		if (state.get().isAuctionTimerPaused)
		{
			return;
		}

		// Only live auctions advance after pause synchronization has extended their timers.
		locals.auctionIndex = state.get().auctionList.nextElementIndex(NULL_INDEX);
		while (locals.auctionIndex != NULL_INDEX)
		{
			locals.auction = state.get().auctionList.value(locals.auctionIndex);
			switch (locals.auction.core.status)
			{
				case EAuctionStatus::Active:
					diffDateInSecond(locals.auction.core.createdAt, locals.currentDate, locals.elapsedSeconds);
					// Only an elapsed active auction is eligible for automatic settlement or seller-decision transition.
					if (locals.elapsedSeconds >= locals.auction.core.auctionDurationSeconds)
					{
						switch (locals.auction.core.type)
						{
							case EAuctionType::Batch:
								locals.finalizeBatchAuctionInput.auctionIndex = locals.auction.core.auctionIndex;
								locals.finalizeBatchAuctionInput.currentDate = locals.currentDate;
								CALL(FinalizeBatchAuction, locals.finalizeBatchAuctionInput, locals.finalizeBatchAuctionOutput);
								break;
							case EAuctionType::Standard:
								// No-bid and reserve-satisfying outcomes are deterministic and need no seller approval window.
								if (locals.auction.core.highestBidAmount == 0 || locals.auction.core.highestBidPrice >= locals.auction.core.salePrice)
								{
									locals.finalizeStandardAuctionInput.auctionIndex = locals.auction.core.auctionIndex;
									locals.finalizeStandardAuctionInput.currentDate = locals.currentDate;
									CALL(FinalizeStandardAuction, locals.finalizeStandardAuctionInput, locals.finalizeStandardAuctionOutput);
								}
								else
								{
									// A funded bid below the seller's sale price requires an explicit, time-bounded seller choice.
									// Below-sale standard bids enter a seller decision window instead of settling immediately.
									locals.auction.core.status = EAuctionStatus::PendingSellerDecision;
									locals.auction.core.sellerDecisionDeadline = locals.currentDate;
									locals.auction.core.sellerDecisionDeadline.add(0, 0, 0, 0, 0, NOST_AUCTION_SELLER_DECISION_WINDOW_SECONDS);
									state.mut().auctionList.replace(locals.auction.core.auctionIndex, locals.auction);
								}
								break;
							default: break;
						};
					}
					break;
				case EAuctionStatus::PendingSellerDecision:
					switch (locals.auction.core.type)
					{
						case EAuctionType::Standard:
							// Expiry resolves in favor of the recorded highest bidder so the seller cannot lock escrow indefinitely.
							if (locals.auction.core.sellerDecisionDeadline <= locals.currentDate)
							{
								locals.finalizeStandardAuctionInput.auctionIndex = locals.auction.core.auctionIndex;
								locals.finalizeStandardAuctionInput.currentDate = locals.currentDate;
								CALL(FinalizeStandardAuction, locals.finalizeStandardAuctionInput, locals.finalizeStandardAuctionOutput);
							}
							break;
						default: break;
					}
					break;
				default: break;
			}

			locals.auctionIndex = state.get().auctionList.nextElementIndex(locals.auctionIndex);
		}
	}

	/**
	 * @brief Validates auction lot entries and totals the escrowed quantity.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(AnalyzeAuctionLot)
	{
		output.totalEscrowQuantity = 0;
		output.lotItemCount = 0;
		output.isValid = 0;

		// Lot validation also enforces the configured maximum auction lifetime.
		if (input.durationDays == 0 || input.durationDays > state.get().maxAuctionDurationDays)
		{
			return;
		}

		// Scan the full fixed ABI array because valid entries may be followed only by zero-padded slots.
		for (locals.lotItemIndex = 0; locals.lotItemIndex < input.auctionLotItems.capacity(); ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			// A zero asset is padding only when its paired quantity is also zero.
			if (isZeroAsset(locals.lotItem.asset))
			{
				if (locals.lotItem.quantity != 0)
				{
					return;
				}
				continue;
			}

			if (locals.lotItem.quantity <= 0)
			{
				return;
			}

			output.lotItemCount = sadd(output.lotItemCount, 1ULL);
			output.totalEscrowQuantity = sadd(output.totalEscrowQuantity, static_cast<uint64>(locals.lotItem.quantity));
		}

		output.isValid = output.lotItemCount > 0 ? 1 : 0;
	}

	/**
	 * @brief Resolves whether the current tick belongs to a scheduled auction pause window.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(GetAuctionPauseState)
	{
		output.isPaused = 0;
		output.pauseStartedAt.setInvalid();
		output.pauseEndsAt.setInvalid();

		// The initial runtime date is treated as a full-day launch pause.
		locals.currentDate = qpi.now();
		makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);
		if (locals.currentDateStamp == NOST_DEFAULT_INIT_TIME)
		{
			output.isPaused = 1;
			output.pauseStartedAt = locals.currentDate;
			output.pauseStartedAt.setTime(0, 0, 0, 0, 0);
			output.pauseEndsAt = output.pauseStartedAt;
			output.pauseEndsAt.addDays(1);
			return;
		}

		// Scheduled pre-epoch pauses keep auctions from expiring during the transition window.
		if (qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day()) == NOST_PRE_EPOCH_PAUSE_DAY_OF_WEEK && qpi.hour() == NOST_PRE_EPOCH_PAUSE_HOUR &&
		    qpi.minute() >= NOST_PRE_EPOCH_PAUSE_MINUTE)
		{
			output.isPaused = 1;
			output.pauseStartedAt = locals.currentDate;
			output.pauseStartedAt.setTime(NOST_PRE_EPOCH_PAUSE_HOUR, NOST_PRE_EPOCH_PAUSE_MINUTE, 0, 0, 0);
			output.pauseEndsAt = output.pauseStartedAt;
			output.pauseEndsAt.add(0, 0, 0, 0, 0, NOST_AUCTION_PRE_EPOCH_PAUSE_SECONDS);
		}
	}

	/**
	 * @brief Reports whether user-facing auction interactions are currently paused.
	 */
	PRIVATE_FUNCTION(IsAuctionInteractionPaused)
	{
		// Emergency pause takes precedence over scheduled and post-epoch launch pauses.
		if (state.get().isEmergencyPaused)
		{
			output.isPaused = 1;
			return;
		}

		output.isPaused = state.get().isAuctionTimerPaused;
		if (output.isPaused)
		{
			return;
		}

		output.isPaused = state.get().isPostBeginEpochPauseArmed && (qpi.tick() - qpi.initialTick()) < NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS;
	}

	/**
	 * @brief Synchronizes timer pause state and extends affected auction deadlines.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(SyncAuctionPauseState)
	{
		locals.currentDate = qpi.now();

		// While emergency pause is active, keep extending the timer pause window.
		if (state.get().isEmergencyPaused)
		{
			if (!state.get().isAuctionTimerPaused)
			{
				state.mut().isAuctionTimerPaused = 1;
				state.mut().auctionTimerPauseStartedAt = locals.currentDate;
				state.mut().auctionTimerPauseEndsAt = locals.currentDate;
			}
			else
			{
				state.mut().auctionTimerPauseEndsAt = locals.currentDate;
			}
			return;
		}

		CALL(GetAuctionPauseState, locals.getAuctionPauseStateInput, locals.getAuctionPauseStateOutput);

		// The launch pause can overlap the scheduled pause; merge both windows before timers resume.
		if (state.get().isPostBeginEpochPauseArmed)
		{
			if ((qpi.tick() - qpi.initialTick()) < NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS)
			{
				if (!state.get().isAuctionTimerPaused)
				{
					state.mut().isAuctionTimerPaused = 1;
					state.mut().auctionTimerPauseStartedAt = locals.currentDate;
					state.mut().auctionTimerPauseEndsAt = locals.currentDate;
				}
				else
				{
					if (!state.get().auctionTimerPauseStartedAt.isValid())
					{
						state.mut().auctionTimerPauseStartedAt = locals.currentDate;
					}
					if (!state.get().auctionTimerPauseEndsAt.isValid() || locals.currentDate > state.get().auctionTimerPauseEndsAt)
					{
						state.mut().auctionTimerPauseEndsAt = locals.currentDate;
					}
				}

				if (locals.getAuctionPauseStateOutput.isPaused)
				{
					if (!state.get().auctionTimerPauseStartedAt.isValid() ||
					    locals.getAuctionPauseStateOutput.pauseStartedAt < state.get().auctionTimerPauseStartedAt)
					{
						state.mut().auctionTimerPauseStartedAt = locals.getAuctionPauseStateOutput.pauseStartedAt;
					}
					if (!state.get().auctionTimerPauseEndsAt.isValid() ||
					    locals.getAuctionPauseStateOutput.pauseEndsAt > state.get().auctionTimerPauseEndsAt)
					{
						state.mut().auctionTimerPauseEndsAt = locals.getAuctionPauseStateOutput.pauseEndsAt;
					}
				}
				return;
			}

			state.mut().isPostBeginEpochPauseArmed = 0;
		}

		// Scheduled pauses are recorded as a window that will later be added to all active deadlines.
		if (locals.getAuctionPauseStateOutput.isPaused)
		{
			if (!state.get().isAuctionTimerPaused)
			{
				state.mut().isAuctionTimerPaused = 1;
				state.mut().auctionTimerPauseStartedAt = locals.getAuctionPauseStateOutput.pauseStartedAt;
				state.mut().auctionTimerPauseEndsAt = locals.getAuctionPauseStateOutput.pauseEndsAt;
				return;
			}

			if (!state.get().auctionTimerPauseStartedAt.isValid() ||
			    locals.getAuctionPauseStateOutput.pauseStartedAt < state.get().auctionTimerPauseStartedAt)
			{
				state.mut().auctionTimerPauseStartedAt = locals.getAuctionPauseStateOutput.pauseStartedAt;
			}
			if (!state.get().auctionTimerPauseEndsAt.isValid() || locals.getAuctionPauseStateOutput.pauseEndsAt > state.get().auctionTimerPauseEndsAt)
			{
				state.mut().auctionTimerPauseEndsAt = locals.getAuctionPauseStateOutput.pauseEndsAt;
			}
			return;
		}

		if (!state.get().isAuctionTimerPaused)
		{
			return;
		}

		if (!state.get().auctionTimerPauseStartedAt.isValid() || !state.get().auctionTimerPauseEndsAt.isValid())
		{
			state.mut().isAuctionTimerPaused = 0;
			state.mut().auctionTimerPauseStartedAt.setInvalid();
			state.mut().auctionTimerPauseEndsAt.setInvalid();
			return;
		}

		// When the pause ends, preserve elapsed auction time by extending every affected deadline.
		diffDateInSecond(state.get().auctionTimerPauseStartedAt, state.get().auctionTimerPauseEndsAt, locals.pausedSeconds);
		if (locals.pausedSeconds > 0)
		{
			locals.auctionIndex = state.get().auctionList.nextElementIndex(NULL_INDEX);
			while (locals.auctionIndex != NULL_INDEX)
			{
				locals.auction = state.get().auctionList.value(locals.auctionIndex);
				if (locals.auction.core.status == EAuctionStatus::Active)
				{
					locals.auction.core.auctionDurationSeconds = sadd(locals.auction.core.auctionDurationSeconds, locals.pausedSeconds);
					state.mut().auctionList.replace(locals.auction.core.auctionIndex, locals.auction);
				}
				else if (locals.auction.core.status == EAuctionStatus::PendingSellerDecision && locals.auction.core.sellerDecisionDeadline.isValid())
				{
					locals.auction.core.sellerDecisionDeadline.add(0, 0, 0, 0, 0, static_cast<sint64>(locals.pausedSeconds));
					state.mut().auctionList.replace(locals.auction.core.auctionIndex, locals.auction);
				}
				locals.auctionIndex = state.get().auctionList.nextElementIndex(locals.auctionIndex);
			}
		}

		state.mut().isAuctionTimerPaused = 0;
		state.mut().auctionTimerPauseStartedAt.setInvalid();
		state.mut().auctionTimerPauseEndsAt.setInvalid();
	}

	/**
	 * @brief Returns remaining launch-delay ticks after the current epoch begins.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(GetTicksBeforeAuctionLaunchInternal)
	{
		output.ticks = 0;

		// An unarmed delay has no remaining ticks even if the current tick is near the epoch boundary.
		if (!state.get().isPostBeginEpochPauseArmed)
		{
			return;
		}

		output.ticks = static_cast<uint32>(max<sint64>(static_cast<sint64>(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS) -
		                                                   (static_cast<sint64>(qpi.tick()) - static_cast<sint64>(qpi.initialTick())),
		                                               0));
	}

	/**
	 * @brief Registers a QU obligation before the associated settlement becomes final.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(QueueQuPayout)
	{
		output.success = 0;
		// Zero is an idempotent no-op, while a non-zero obligation must always have a payable recipient.
		if (input.amount == 0 || isZero(input.recipient))
		{
			output.success = input.amount == 0;
			return;
		}

		locals.previousAmount = 0;
		// Reject aggregate overflow before touching either the per-wallet entry or its mirrored total.
		if (input.amount > UINT64_MAX - state.get().totalPendingQuPayouts)
		{
			return;
		}

		// Multiple settlements for one wallet share one liability entry to conserve bounded map capacity.
		if (state.get().pendingQuPayouts.get(input.recipient, locals.previousAmount))
		{
			// The wallet-level value must remain exactly reconcilable with totalPendingQuPayouts.
			if (input.amount > UINT64_MAX - locals.previousAmount)
			{
				return;
			}

			locals.updatedAmount = sadd(locals.previousAmount, input.amount);
			if (!state.mut().pendingQuPayouts.replace(input.recipient, locals.updatedAmount))
			{
				return;
			}
		}
		else
		{
			// First-time recipients consume a new map slot; failure leaves the global liability total unchanged.
			locals.payoutIndex = state.mut().pendingQuPayouts.set(input.recipient, input.amount);
			if (locals.payoutIndex == NULL_INDEX)
			{
				return;
			}
		}

		state.mut().totalPendingQuPayouts = sadd(state.get().totalPendingQuPayouts, input.amount);
		output.success = 1;
	}

	/**
	 * @brief Pays a bounded number of chunks and preserves every unpaid remainder in state.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FlushQuPayout)
	{
		output.success = 0;
		output.transferredAmount = 0;
		output.remainingAmount = 0;
		// Absence is distinct from a paid zero balance because zero-balance entries are removed immediately.
		if (!state.get().pendingQuPayouts.get(input.recipient, output.remainingAmount))
		{
			return;
		}

		locals.chunkIndex = 0;
		// Bound both transfer size and iteration count so one payout attempt cannot exhaust contract execution time.
		while (output.remainingAmount > 0 && locals.chunkIndex < input.maxChunks)
		{
			locals.chunkAmount = min(output.remainingAmount, static_cast<uint64>(MAX_AMOUNT));
			locals.transferResult = qpi.transfer(input.recipient, static_cast<sint64>(locals.chunkAmount));
			// A failed transfer stops delivery without decrementing the durable obligation.
			if (locals.transferResult < 0)
			{
				break;
			}
			output.remainingAmount -= locals.chunkAmount;
			output.transferredAmount = sadd(output.transferredAmount, locals.chunkAmount);
			state.mut().totalPendingQuPayouts -= locals.chunkAmount;
			++locals.chunkIndex;
		}

		// Fully discharged entries release map capacity; partial delivery persists the exact remainder for retry.
		if (output.remainingAmount == 0)
		{
			state.mut().pendingQuPayouts.removeByKey(input.recipient);
		}
		else
		{
			state.mut().pendingQuPayouts.replace(input.recipient, output.remainingAmount);
		}
		output.success = 1;
	}

	/**
	 * @brief Retries a bounded set of pending QU payouts and advances the persistent round-robin cursor.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessPendingQuPayouts)
	{
		locals.payoutScanIndex = mod(state.get().pendingPayoutScanCursor, state.get().pendingQuPayouts.capacity());
		locals.payoutTargetRecipientCount = min(state.get().pendingQuPayouts.population(), NOST_END_EPOCH_PAYOUT_RECIPIENT_NUM);
		locals.processedPayoutRecipientCount = 0;
		locals.payoutElementIndex = state.get().pendingQuPayouts.nextElementIndex(static_cast<sint64>(locals.payoutScanIndex) - 1);
		// Round-robin scanning bounds epoch work and prevents a permanently failing wallet from starving later map slots.
		while (locals.processedPayoutRecipientCount < locals.payoutTargetRecipientCount)
		{
			// Wrap once the physical end is reached; the initial population snapshot prevents duplicate processing.
			if (locals.payoutElementIndex == NULL_INDEX)
			{
				locals.payoutElementIndex = state.get().pendingQuPayouts.nextElementIndex(NULL_INDEX);
				if (locals.payoutElementIndex == NULL_INDEX)
				{
					break;
				}
			}
			locals.pendingPayoutRecipient = state.get().pendingQuPayouts.key(locals.payoutElementIndex);
			locals.payoutScanIndex = mod(sadd(static_cast<uint64>(locals.payoutElementIndex), 1ULL), state.get().pendingQuPayouts.capacity());
			locals.flushQuPayoutInput.recipient = locals.pendingPayoutRecipient;
			locals.flushQuPayoutInput.maxChunks = NOST_END_EPOCH_PAYOUT_CHUNKS_PER_RECIPIENT;
			CALL(FlushQuPayout, locals.flushQuPayoutInput, locals.flushQuPayoutOutput);
			locals.processedPayoutRecipientCount = sadd(locals.processedPayoutRecipientCount, 1ULL);
			locals.payoutElementIndex = state.get().pendingQuPayouts.nextElementIndex(locals.payoutElementIndex);
		}
		state.mut().pendingPayoutScanCursor = locals.payoutScanIndex;
	}

	/**
	 * @brief Registers a payout exactly once for this call and immediately attempts bounded delivery.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(QueueAndFlushQuPayout)
	{
		output.success = 0;
		output.transferredAmount = 0;
		output.remainingAmount = 0;
		locals.queueQuPayoutInput.recipient = input.recipient;
		locals.queueQuPayoutInput.amount = input.amount;
		CALL(QueueQuPayout, locals.queueQuPayoutInput, locals.queueQuPayoutOutput);
		// Never attempt delivery unless the complete liability was made durable first.
		if (!locals.queueQuPayoutOutput.success)
		{
			return;
		}
		// QueueQuPayout treats zero as success, but there is no map entry for FlushQuPayout to consume.
		if (input.amount == 0)
		{
			output.success = 1;
			return;
		}
		locals.flushQuPayoutInput.recipient = input.recipient;
		locals.flushQuPayoutInput.maxChunks = input.maxChunks;
		CALL(FlushQuPayout, locals.flushQuPayoutInput, locals.flushQuPayoutOutput);
		output.transferredAmount = locals.flushQuPayoutOutput.transferredAmount;
		output.remainingAmount = locals.flushQuPayoutOutput.remainingAmount;
		output.success = locals.flushQuPayoutOutput.success;
	}

	/**
	 * @brief Appends a participant snapshot to bounded history.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ArchiveParticipant)
	{
		locals.historyIndex = mod(state.get().participantHistoryCounter, state.get().participantHistory.capacity());
		state.mut().participantHistory.set(locals.historyIndex, input.participantData);
		state.mut().participantHistoryCounter = sadd(state.get().participantHistoryCounter, 1ULL);
	}

	/**
	 * @brief Archives a closed auction and releases its active hash-map slot.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ArchiveClosedAuction)
	{
		locals.historyIndex = mod(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		state.mut().closedAuctionHistory.set(locals.historyIndex, input.auction);
		state.mut().closedAuctionHistoryCounter = sadd(state.get().closedAuctionHistoryCounter, 1ULL);
		state.mut().auctionList.removeByKey(input.auction.core.auctionIndex);
	}

	/**
	 * @brief Finds a live auction or a retained closed-auction snapshot.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(FindAuction)
	{
		output.found = state.get().auctionList.get(input.auctionIndex, output.auction);
		// Active storage is authoritative and avoids the bounded linear archive scan for live auctions.
		if (output.found)
		{
			return;
		}
		// Closed auctions remain queryable only while their full snapshot is retained in the ring buffer.
		for (locals.historyIndex = 0; locals.historyIndex < state.get().closedAuctionHistory.capacity(); ++locals.historyIndex)
		{
			locals.archivedAuction = state.get().closedAuctionHistory.get(locals.historyIndex);
			if (locals.archivedAuction.core.status != EAuctionStatus::None && locals.archivedAuction.core.auctionIndex == input.auctionIndex)
			{
				output.auction = locals.archivedAuction;
				output.found = 1;
				return;
			}
		}
	}

	/**
	 * @brief Tests retained closed history without copying an auction into the caller's locals.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(IsClosedAuctionRetained)
	{
		output.found = 0;
		locals.retainedClosedAuctionCount = min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		// Only initialized ring-buffer entries can match; inspect the const snapshot in place to keep this lookup lightweight.
		for (locals.historyIndex = 0; locals.historyIndex < locals.retainedClosedAuctionCount; ++locals.historyIndex)
		{
			if (state.get().closedAuctionHistory.get(locals.historyIndex).core.status != EAuctionStatus::None &&
			    state.get().closedAuctionHistory.get(locals.historyIndex).core.auctionIndex == input.auctionIndex)
			{
				output.found = 1;
				return;
			}
		}
	}

	/**
	 * @brief Selects the smallest retained auction index after an optional cursor.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(SelectNextRetainedAuction)
	{
		output.found = 0;
		// Hash-map iteration is not creation ordered, so retain the smallest eligible live index beyond the cursor.
		for (locals.auctionElementIndex = state.get().auctionList.nextElementIndex(NULL_INDEX); locals.auctionElementIndex != NULL_INDEX;
		     locals.auctionElementIndex = state.get().auctionList.nextElementIndex(locals.auctionElementIndex))
		{
			locals.candidateAuction = state.get().auctionList.value(locals.auctionElementIndex);
			// Apply the cursor and optional seller filter before comparing creation indices.
			if ((input.hasAfterAuctionIndex && locals.candidateAuction.core.auctionIndex <= input.afterAuctionIndex) ||
			    (input.filterBySeller && locals.candidateAuction.core.seller != input.seller))
			{
				continue;
			}
			if (!output.found || locals.candidateAuction.core.auctionIndex < output.auction.core.auctionIndex)
			{
				output.auction = locals.candidateAuction;
				output.found = 1;
			}
		}
		// Live-only callers avoid the archive scan entirely.
		if (!input.includeClosedAuctions)
		{
			return;
		}

		locals.retainedClosedAuctionCount = min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		// Merge only retained closed snapshots without assuming physical ring order.
		for (locals.historyIndex = 0; locals.historyIndex < locals.retainedClosedAuctionCount; ++locals.historyIndex)
		{
			locals.candidateAuction = state.get().closedAuctionHistory.get(locals.historyIndex);
			// Apply the same cursor and seller filter to archived candidates.
			if (locals.candidateAuction.core.status == EAuctionStatus::None ||
			    (input.hasAfterAuctionIndex && locals.candidateAuction.core.auctionIndex <= input.afterAuctionIndex) ||
			    (input.filterBySeller && locals.candidateAuction.core.seller != input.seller))
			{
				continue;
			}
			if (!output.found || locals.candidateAuction.core.auctionIndex < output.auction.core.auctionIndex)
			{
				output.auction = locals.candidateAuction;
				output.found = 1;
			}
		}
	}

	/**
	 * @brief Counts retained live and closed auctions belonging to one seller without reconstructing creation order.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CountRetainedAuctionsBySeller)
	{
		output.count = 0;
		// A physical live-map pass is sufficient because counting does not depend on creation order.
		for (locals.auctionElementIndex = state.get().auctionList.nextElementIndex(NULL_INDEX); locals.auctionElementIndex != NULL_INDEX;
		     locals.auctionElementIndex = state.get().auctionList.nextElementIndex(locals.auctionElementIndex))
		{
			locals.candidateAuction = state.get().auctionList.value(locals.auctionElementIndex);
			if (locals.candidateAuction.core.seller == input.seller)
			{
				output.count = sadd(output.count, 1ULL);
			}
		}

		locals.retainedClosedAuctionCount = min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		// Only initialized ring slots can contribute to the retained seller count.
		for (locals.historyIndex = 0; locals.historyIndex < locals.retainedClosedAuctionCount; ++locals.historyIndex)
		{
			locals.candidateAuction = state.get().closedAuctionHistory.get(locals.historyIndex);
			if (locals.candidateAuction.core.status != EAuctionStatus::None && locals.candidateAuction.core.seller == input.seller)
			{
				output.count = sadd(output.count, 1ULL);
			}
		}
	}

	/**
	 * @brief Finds the smallest retained auction index whose complete fixed-size metadata CID matches the input.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(FindFirstRetainedAuctionByMetadataCid)
	{
		output.found = 0;
		// Select the minimum matching live index directly instead of repeatedly reconstructing global order.
		for (locals.auctionElementIndex = state.get().auctionList.nextElementIndex(NULL_INDEX); locals.auctionElementIndex != NULL_INDEX;
		     locals.auctionElementIndex = state.get().auctionList.nextElementIndex(locals.auctionElementIndex))
		{
			locals.candidateAuction = state.get().auctionList.value(locals.auctionElementIndex);
			locals.metadataMatches = 1;
			// Compare the complete fixed CID field, including zero padding.
			for (locals.metadataIndex = 0; locals.metadataIndex < NOST_AUCTION_METADATA_CID_LENGTH; ++locals.metadataIndex)
			{
				if (locals.candidateAuction.core.metadataIpfsCid.get(locals.metadataIndex) != input.metadataIpfsCid.get(locals.metadataIndex))
				{
					locals.metadataMatches = 0;
					break;
				}
			}
			if (locals.metadataMatches && (!output.found || locals.candidateAuction.core.auctionIndex < output.auction.core.auctionIndex))
			{
				output.auction = locals.candidateAuction;
				output.found = 1;
			}
		}

		locals.retainedClosedAuctionCount = min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		// Closed snapshots share the same index ordering but occupy unordered ring slots.
		for (locals.historyIndex = 0; locals.historyIndex < locals.retainedClosedAuctionCount; ++locals.historyIndex)
		{
			locals.candidateAuction = state.get().closedAuctionHistory.get(locals.historyIndex);
			if (locals.candidateAuction.core.status == EAuctionStatus::None)
			{
				continue;
			}
			locals.metadataMatches = 1;
			// Compare the complete fixed CID field, including zero padding.
			for (locals.metadataIndex = 0; locals.metadataIndex < NOST_AUCTION_METADATA_CID_LENGTH; ++locals.metadataIndex)
			{
				if (locals.candidateAuction.core.metadataIpfsCid.get(locals.metadataIndex) != input.metadataIpfsCid.get(locals.metadataIndex))
				{
					locals.metadataMatches = 0;
					break;
				}
			}
			if (locals.metadataMatches && (!output.found || locals.candidateAuction.core.auctionIndex < output.auction.core.auctionIndex))
			{
				output.auction = locals.candidateAuction;
				output.found = 1;
			}
		}
	}

	/**
	 * @brief Pays auction sale proceeds to the seller and records every fee for end-of-epoch settlement.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(DistributeAuctionRevenue)
	{
		output.sellerPayout = input.grossAmount;
		output.success = 0;

		// Zero-gross settlements still report success so callers can close no-sale auctions cleanly.
		if (input.grossAmount == 0)
		{
			output.success = 1;
			return;
		}
		// Only the seller is queued during settlement; fee recipients are handled by END_EPOCH.
		if (state.get().pendingQuPayouts.population() > state.get().pendingQuPayouts.capacity() - NOST_AUCTION_REVENUE_MAX_PAYOUT_RECIPIENTS)
		{
			return;
		}

		calculateAuctionRevenueBreakdown(input.grossAmount, state, locals.auctionRevenueBreakdown);
		output.sellerPayout = locals.auctionRevenueBreakdown.sellerPayout;

		// Register the seller liability before recording fees so a queue-capacity failure cannot duplicate fee accrual on retry.
		locals.payoutInput.recipient = input.seller;
		locals.payoutInput.amount = output.sellerPayout;
		locals.payoutInput.maxChunks = NOST_MAX_QU_TRANSFER_CHUNKS_PER_CALL;
		CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
		if (!locals.payoutOutput.success)
		{
			return;
		}

		locals.feePool = state.get().feePool;
		// The routing decision and fee configuration are captured when revenue is settled; recipient wallets are resolved at END_EPOCH.
		if (routeAllFeesToDevelopment(state))
		{
			locals.feePool.developmentAmount = sadd(locals.feePool.developmentAmount, input.grossAmount - output.sellerPayout);
		}
		else
		{
			locals.shareholderFeeTierIndex = getAuctionShareholderFeeTierIndex(input.grossAmount);
			switch (locals.shareholderFeeTierIndex)
			{
				case 0:
					locals.feePool.shareholderDividendTier1Amount =
					    sadd(locals.feePool.shareholderDividendTier1Amount, locals.auctionRevenueBreakdown.shareholderDividendAmount);
					break;
				case 1:
					locals.feePool.shareholderDividendTier2Amount =
					    sadd(locals.feePool.shareholderDividendTier2Amount, locals.auctionRevenueBreakdown.shareholderDividendAmount);
					break;
				case 2:
					locals.feePool.shareholderDividendTier3Amount =
					    sadd(locals.feePool.shareholderDividendTier3Amount, locals.auctionRevenueBreakdown.shareholderDividendAmount);
					break;
				default:
					locals.feePool.shareholderDividendTier4Amount =
					    sadd(locals.feePool.shareholderDividendTier4Amount, locals.auctionRevenueBreakdown.shareholderDividendAmount);
					break;
			}

			locals.feePool.managementAmount = sadd(locals.feePool.managementAmount, locals.auctionRevenueBreakdown.managementFeeAmount);
			locals.feePool.developmentAmount = sadd(locals.feePool.developmentAmount, locals.auctionRevenueBreakdown.developmentFeeAmount);
			locals.feePool.takeoverCoordinatorAmount =
			    sadd(locals.feePool.takeoverCoordinatorAmount, locals.auctionRevenueBreakdown.takeoverCoordinatorFeeAmount);
		}

		state.mut().feePool = locals.feePool;
		output.success = 1;
	}

	/**
	 * @brief Accumulates a service fee using the routing mode active when the fee is charged.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(AccumulateAuctionServiceFee)
	{
		output.success = 0;

		// Creation, bidding, and cancellation paths may call this with zero after fee configuration changes.
		if (input.feeAmount == 0)
		{
			output.success = 1;
			return;
		}

		locals.feePool = state.get().feePool;
		if (routeAllFeesToDevelopment(state))
		{
			locals.feePool.developmentAmount = sadd(locals.feePool.developmentAmount, input.feeAmount);
		}
		else
		{
			locals.feePool.commonServiceFeeAmount = sadd(locals.feePool.commonServiceFeeAmount, input.feeAmount);
		}
		state.mut().feePool = locals.feePool;
		output.success = 1;
	}

	/**
	 * @brief Materializes compatible service fees and settles every shared pool accumulator using the recipients active at `END_EPOCH`.
	 * @note Each accumulator is cleared only after its value has moved to dividend dust or a durable payout liability.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(DistributeNostromoFeePool)
	{
		output.success = 0;
		locals.feePool = state.get().feePool;

		if (locals.feePool.commonServiceFeeAmount > 0)
		{
			calculateAuctionServiceFeeBreakdown(locals.feePool.commonServiceFeeAmount, locals.auctionServiceFeeBreakdown);
			locals.feePool.shareholderDividendAmount =
			    sadd(locals.feePool.shareholderDividendAmount, locals.auctionServiceFeeBreakdown.shareholderDividendAmount);
			locals.feePool.managementAmount = sadd(locals.feePool.managementAmount, locals.auctionServiceFeeBreakdown.managementFeeAmount);
			locals.feePool.developmentAmount = sadd(locals.feePool.developmentAmount, locals.auctionServiceFeeBreakdown.developmentFeeAmount);
			locals.feePool.takeoverCoordinatorAmount =
			    sadd(locals.feePool.takeoverCoordinatorAmount, locals.auctionServiceFeeBreakdown.takeoverCoordinatorFeeAmount);
			locals.feePool.commonServiceFeeAmount = 0;
			state.mut().feePool = locals.feePool;
		}

		locals.shareholderDividendAmount =
		    sadd(sadd(sadd(locals.feePool.shareholderDividendTier1Amount, locals.feePool.shareholderDividendTier2Amount),
		              sadd(locals.feePool.shareholderDividendTier3Amount, locals.feePool.shareholderDividendTier4Amount)),
		         locals.feePool.shareholderDividendAmount);
		if (locals.shareholderDividendAmount > 0)
		{
			state.mut().auctionShareholderDividendPool = sadd(state.get().auctionShareholderDividendPool, locals.shareholderDividendAmount);
			locals.feePool.shareholderDividendTier1Amount = 0;
			locals.feePool.shareholderDividendTier2Amount = 0;
			locals.feePool.shareholderDividendTier3Amount = 0;
			locals.feePool.shareholderDividendTier4Amount = 0;
			locals.feePool.shareholderDividendAmount = 0;
			state.mut().feePool = locals.feePool;
		}

		locals.dividendPerShare = div<uint64>(state.get().auctionShareholderDividendPool, NUMBER_OF_COMPUTORS);
		if (locals.dividendPerShare > 0 && qpi.distributeDividends(locals.dividendPerShare))
		{
			locals.distributedDividendAmount = smul(locals.dividendPerShare, static_cast<uint64>(NUMBER_OF_COMPUTORS));
			state.mut().auctionShareholderDividendPool -= locals.distributedDividendAmount;
		}

		if (state.get().feePool.managementAmount > 0)
		{
			locals.payoutInput.recipient = state.get().management;
			locals.payoutInput.amount = state.get().feePool.managementAmount;
			locals.payoutInput.maxChunks = NOST_MAX_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			if (!locals.payoutOutput.success)
			{
				return;
			}
			state.mut().feePool.managementAmount = 0;
		}
		if (state.get().feePool.developmentAmount > 0)
		{
			locals.payoutInput.recipient = state.get().development;
			locals.payoutInput.amount = state.get().feePool.developmentAmount;
			locals.payoutInput.maxChunks = NOST_MAX_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			if (!locals.payoutOutput.success)
			{
				return;
			}
			state.mut().feePool.developmentAmount = 0;
		}
		if (state.get().feePool.takeoverCoordinatorAmount > 0)
		{
			locals.payoutInput.recipient = state.get().takeoverCoordinator;
			locals.payoutInput.amount = state.get().feePool.takeoverCoordinatorAmount;
			locals.payoutInput.maxChunks = NOST_MAX_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			if (!locals.payoutOutput.success)
			{
				return;
			}
			state.mut().feePool.takeoverCoordinatorAmount = 0;
		}

		output.success = 1;
	}

	/**
	 * @brief Counts non-empty wallet entries allowed to bid in a private auction.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CountAllowedBidderWallets)
	{
		output.allowedWalletCount = 0;
		for (locals.allowedWalletIndex = 0; locals.allowedWalletIndex < input.allowedBidderWallets.capacity(); ++locals.allowedWalletIndex)
		{
			if (!isZero(input.allowedBidderWallets.get(locals.allowedWalletIndex)))
			{
				output.allowedWalletCount = sadd(output.allowedWalletCount, 1ULL);
			}
		}
	}

	/**
	 * @brief Counts valid access-asset requirements for private auction gating.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CountRequiredAccessAssets)
	{
		output.requiredAccessAssetCount = 0;
		output.isValid = 1;
		// Empty asset slots are allowed only when their quantity is also empty.
		for (locals.requiredAccessAssetIndex = 0; locals.requiredAccessAssetIndex < input.requiredAccessAssets.capacity();
		     ++locals.requiredAccessAssetIndex)
		{
			locals.requiredAccessAsset = input.requiredAccessAssets.get(locals.requiredAccessAssetIndex);
			if (isZeroAsset(locals.requiredAccessAsset.asset))
			{
				if (locals.requiredAccessAsset.quantity != 0)
				{
					output.isValid = 0;
					return;
				}
				continue;
			}

			if (locals.requiredAccessAsset.quantity <= 0)
			{
				output.isValid = 0;
				return;
			}

			output.requiredAccessAssetCount = sadd(output.requiredAccessAssetCount, 1ULL);
		}
	}

	/**
	 * @brief Checks whether the invocator owns at least one configured access asset.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(HasRequiredAccessAsset)
	{
		output.hasRequiredAccessAsset = 0;
		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		// Owning any one configured access asset at the required quantity grants private auction access.
		for (locals.requiredAccessAssetSetIndex = locals.auction.requiredAccessAssets.nextElementIndex(NULL_INDEX);
		     locals.requiredAccessAssetSetIndex != NULL_INDEX;
		     locals.requiredAccessAssetSetIndex = locals.auction.requiredAccessAssets.nextElementIndex(locals.requiredAccessAssetSetIndex))
		{
			locals.requiredAccessAsset.asset = locals.auction.requiredAccessAssets.key(locals.requiredAccessAssetSetIndex);
			locals.requiredAccessAsset.quantity = locals.auction.requiredAccessAssets.value(locals.requiredAccessAssetSetIndex);
			locals.possessedAccessShares = qpi.numberOfShares(locals.requiredAccessAsset.asset, AssetOwnershipSelect::byOwner(qpi.invocator()),
			                                                  AssetPossessionSelect::byPossessor(qpi.invocator()));
			if (locals.possessedAccessShares >= locals.requiredAccessAsset.quantity)
			{
				output.hasRequiredAccessAsset = 1;
				return;
			}
		}
	}

	/**
	 * @brief Recomputes the displayed highest active Batch Auction bid.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(RecomputeBatchHighestBid)
	{
		locals.bestParticipantFound = 0;

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		if (locals.auction.core.type != EAuctionType::Batch)
		{
			return;
		}

		// Batch auctions expose the highest active price, with FIFO tie-breaking for equal bids.
		for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}

			if (!locals.participantData.isActive || locals.participantData.escrowedAmount == 0)
			{
				continue;
			}

			if (!locals.bestParticipantFound || locals.participantData.bidAmount > locals.bestParticipantData.bidAmount ||
			    (locals.participantData.bidAmount == locals.bestParticipantData.bidAmount &&
			     locals.participantData.bidIndex < locals.bestParticipantData.bidIndex))
			{
				locals.bestParticipantFound = 1;
				locals.bestParticipantData = locals.participantData;
				locals.bestParticipantSlotIndex = locals.participantIndex;
			}
		}

		if (locals.bestParticipantFound)
		{
			locals.auction.core.highestBidder = locals.bestParticipantData.participant;
			locals.auction.core.highestBidPrice = locals.bestParticipantData.bidAmount;
			locals.auction.core.highestBidQuantity = locals.bestParticipantData.requestedQuantity;
			locals.auction.core.highestBidAmount = locals.bestParticipantData.escrowedAmount;
			locals.auction.core.highestBidSlotIndex = locals.bestParticipantSlotIndex;
		}
		else
		{
			locals.auction.core.highestBidAmount = 0;
			locals.auction.core.highestBidPrice = 0;
			locals.auction.core.highestBidQuantity = 0;
			locals.auction.core.highestBidder = NULL_ID;
			locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		}

		state.mut().auctionList.replace(locals.auction.core.auctionIndex, locals.auction);
	}

	/**
	 * @brief Computes the price and quantity still available for a Batch Auction bid.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ComputeBatchBidAvailability)
	{
		output.found = 0;
		output.isAcceptingBids = 0;
		output.minimumBidPrice = 0;
		output.availableQuantity = 0;
		locals.lowestWinningPriceFound = 0;
		locals.lowestWinningPrice = 0;
		locals.salePriorityQuantity = 0;
		locals.priorityQuantity = 0;

		// Availability is defined only for a retained live auction; closed snapshots never accept bids.
		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		output.found = 1;
		if (locals.auction.core.type != EAuctionType::Batch || locals.auction.core.status != EAuctionStatus::Active ||
		    locals.auction.core.quantityForSale < locals.auction.core.minimumPurchaseQuantity)
		{
			return;
		}

		// Existing sale-price-or-better bids reserve priority quantity before a new bid can enter.
		for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}

			if (!locals.participantData.isActive || locals.participantData.escrowedAmount == 0 || locals.participantData.requestedQuantity == 0)
			{
				continue;
			}

			if (!locals.lowestWinningPriceFound || locals.participantData.bidAmount < locals.lowestWinningPrice)
			{
				locals.lowestWinningPriceFound = 1;
				locals.lowestWinningPrice = locals.participantData.bidAmount;
			}

			if (locals.participantData.bidAmount >= locals.auction.core.salePrice)
			{
				locals.salePriorityQuantity = sadd(locals.salePriorityQuantity, locals.participantData.requestedQuantity);
			}
		}

		locals.effectiveCoverageQuantity =
		    locals.auction.core.quantityForSale - locals.auction.core.minimumPurchaseQuantity + NOST_BATCH_COVERAGE_THRESHOLD_OFFSET;
		// Once less than one minimum allocation remains, report no sale-price capacity instead of an unusable fragment.
		if (locals.salePriorityQuantity >= locals.effectiveCoverageQuantity)
		{
			output.availableQuantity = 0;
		}
		else
		{
			// Otherwise expose the full unreserved quantity; the minimum check below decides whether bidding remains viable.
			output.availableQuantity = locals.auction.core.quantityForSale - locals.salePriorityQuantity;
		}

		// If sale-price capacity is exhausted, new bids must improve the current lowest winning price.
		if (output.availableQuantity >= locals.auction.core.minimumPurchaseQuantity)
		{
			output.minimumBidPrice = locals.auction.core.salePrice;
			output.isAcceptingBids = 1;
		}
		else
		{
			// A full book can still accept a strictly better bid that displaces the current lowest-priced allocation.
			output.availableQuantity = 0;
			if (!locals.lowestWinningPriceFound || locals.lowestWinningPrice == UINT64_MAX)
			{
				return;
			}

			output.minimumBidPrice = sadd(locals.lowestWinningPrice, 1ULL);
			output.isAcceptingBids = 1;
			if (input.bidAmount == 0)
			{
				return;
			}
		}

		locals.outputPrice = input.bidAmount > 0 ? input.bidAmount : output.minimumBidPrice;
		if (locals.outputPrice < output.minimumBidPrice)
		{
			output.availableQuantity = 0;
			return;
		}

		// Recompute capacity at the requested price so callers know the maximum acceptable quantity.
		locals.priorityQuantity = 0;
		for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}

			if (!locals.participantData.isActive || locals.participantData.escrowedAmount == 0 || locals.participantData.requestedQuantity == 0)
			{
				continue;
			}

			if (locals.participantData.bidAmount > locals.outputPrice || locals.participantData.bidAmount == locals.outputPrice)
			{
				locals.priorityQuantity = sadd(locals.priorityQuantity, locals.participantData.requestedQuantity);
			}
		}

		// Equal-priced existing bids have FIFO priority, so a candidate at that price receives only later capacity.
		if (locals.priorityQuantity >= locals.auction.core.quantityForSale)
		{
			output.availableQuantity = 0;
			return;
		}

		output.availableQuantity = locals.auction.core.quantityForSale - locals.priorityQuantity;
	}

	/**
	 * @brief Validates, escrows, and ranks a new Batch Auction bid.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessBatchBid)
	{
		output.escrowedAmount = 0;
		output.refundedAmount = 0;
		output.errorCode = EAuctionError::Success;
		output.success = 0;
		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::AuctionNotFound;
			return;
		}

		if (input.effectiveQuantity < locals.auction.core.minimumPurchaseQuantity || input.bidAmount == 0)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::InvalidInput;
			return;
		}
		// Retain enough payout slots to refund every active participant plus the caller's possible overpayment.
		if (state.get().pendingQuPayouts.population() >
		    state.get().pendingQuPayouts.capacity() - state.get().participants.capacity() - NOST_BATCH_BID_CALLER_PAYOUT_RECIPIENTS)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::PayoutQueueFull;
			return;
		}

		calculateBatchAuctionBidFee(input.effectiveQuantity, input.bidAmount, locals.bidFeeCalculation);
		if (locals.bidFeeCalculation.escrowAmount == 0)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::InvalidInput;
			return;
		}

		if (input.bidAmount < locals.auction.core.salePrice)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::BidTooLow;
			return;
		}

		locals.computeBatchBidAvailabilityInput.auctionIndex = input.auctionIndex;
		locals.computeBatchBidAvailabilityInput.bidAmount = input.bidAmount;
		CALL(ComputeBatchBidAvailability, locals.computeBatchBidAvailabilityInput, locals.computeBatchBidAvailabilityOutput);
		if (!locals.computeBatchBidAvailabilityOutput.isAcceptingBids || input.bidAmount < locals.computeBatchBidAvailabilityOutput.minimumBidPrice)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::BidTooLow;
			return;
		}
		if (input.effectiveQuantity > locals.computeBatchBidAvailabilityOutput.availableQuantity)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::QuantityUnavailable;
			return;
		}

		if (static_cast<uint64>(qpi.invocationReward()) < locals.bidFeeCalculation.requiredReward)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::InsufficientFunds;
			return;
		}

		// Batch bids consume live slots only; displaced and settled records move to the history ring.
		locals.freeParticipantSlotFound = 0;
		for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (!locals.participantData.isUsed)
			{
				locals.freeParticipantSlotFound = 1;
				locals.freeParticipantSlotIndex = locals.participantIndex;
				break;
			}
		}

		if (!locals.freeParticipantSlotFound || locals.auction.core.nextBidIndex == UINT64_MAX)
		{
			output.refundedAmount = static_cast<uint64>(qpi.invocationReward());
			output.errorCode = EAuctionError::StorageFull;
			return;
		}

		locals.participantData.escrowedAmount = locals.bidFeeCalculation.escrowAmount;
		locals.participantData.requestedQuantity = input.effectiveQuantity;
		locals.participantData.allocatedQuantity = 0;
		locals.participantData.bidAmount = input.bidAmount;
		locals.participantData.lastBidTime = input.currentDate;
		locals.participantData.participant = qpi.invocator();
		locals.participantData.auctionIndex = input.auctionIndex;
		locals.participantData.bidIndex = locals.auction.core.nextBidIndex;
		locals.participantData.isUsed = 1;
		locals.participantData.isActive = 1;
		locals.participantData.isWinningBid = 1;

		// Accepted bids near deadline extend the auction to reduce last-moment sniping.
		locals.auction.core.lastBidAt = input.currentDate;
		if ((locals.auction.core.auctionDurationSeconds - input.elapsedSeconds) <= NOST_AUCTION_EXTENSION_SECONDS)
		{
			locals.auction.core.auctionDurationSeconds = sadd(locals.auction.core.auctionDurationSeconds, NOST_AUCTION_EXTENSION_SECONDS);
		}

		locals.auction.core.nextBidIndex = sadd(locals.auction.core.nextBidIndex, 1ULL);
		state.mut().participants.set(locals.freeParticipantSlotIndex, locals.participantData);
		state.mut().auctionList.replace(input.auctionIndex, locals.auction);

		// Keep only the highest-priority quantity active; displaced escrow is refunded immediately.
		locals.activeQuantity = 0;
		for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}
			if (locals.participantData.isActive && locals.participantData.escrowedAmount > 0 && locals.participantData.requestedQuantity > 0)
			{
				locals.activeQuantity = sadd(locals.activeQuantity, locals.participantData.requestedQuantity);
			}
		}

		// Repeatedly evict the lowest-priority tail until active demand fits the finite lot supply.
		while (locals.activeQuantity > locals.auction.core.quantityForSale)
		{
			locals.worstParticipantFound = 0;
			// Lowest price loses first; for equal prices the newest bid loses to preserve FIFO priority.
			for (locals.participantIndex = 0; locals.participantIndex < state.get().participants.capacity(); ++locals.participantIndex)
			{
				locals.participantData = state.get().participants.get(locals.participantIndex);
				if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
				{
					continue;
				}

				if (!locals.participantData.isActive || locals.participantData.escrowedAmount == 0 || locals.participantData.requestedQuantity == 0)
				{
					continue;
				}

				if (!locals.worstParticipantFound || locals.participantData.bidAmount < locals.worstParticipantData.bidAmount ||
				    (locals.participantData.bidAmount == locals.worstParticipantData.bidAmount &&
				     locals.participantData.bidIndex > locals.worstParticipantData.bidIndex))
				{
					locals.worstParticipantFound = 1;
					locals.worstParticipantData = locals.participantData;
					locals.worstParticipantSlotIndex = locals.participantIndex;
				}
			}

			if (!locals.worstParticipantFound)
			{
				break;
			}

			locals.excessQuantity = locals.activeQuantity - locals.auction.core.quantityForSale;
			locals.displacedQuantity = min(locals.excessQuantity, locals.worstParticipantData.requestedQuantity);
			locals.displacedRefund = smul(locals.displacedQuantity, locals.worstParticipantData.bidAmount);
			locals.remainingWorstQuantity = locals.worstParticipantData.requestedQuantity - locals.displacedQuantity;
			// A partial order smaller than the minimum is removed in full; keeping it would create an invalid final allocation.
			if (locals.remainingWorstQuantity > 0 && locals.remainingWorstQuantity < locals.auction.core.minimumPurchaseQuantity)
			{
				locals.displacedQuantity = locals.worstParticipantData.requestedQuantity;
				locals.displacedRefund = locals.worstParticipantData.escrowedAmount;
			}
			// Full displacement retires the live slot; partial displacement keeps a valid minimum-sized order active.
			if (locals.displacedQuantity >= locals.worstParticipantData.requestedQuantity)
			{
				locals.worstParticipantData.escrowedAmount = 0;
				locals.worstParticipantData.requestedQuantity = 0;
				locals.worstParticipantData.allocatedQuantity = 0;
				locals.worstParticipantData.isActive = 0;
				locals.worstParticipantData.isWinningBid = 0;
			}
			else
			{
				locals.worstParticipantData.requestedQuantity -= locals.displacedQuantity;
				locals.worstParticipantData.escrowedAmount -= locals.displacedRefund;
				locals.worstParticipantData.isWinningBid = 1;
			}

			if (locals.displacedRefund > 0)
			{
				locals.payoutInput.recipient = locals.worstParticipantData.participant;
				locals.payoutInput.amount = locals.displacedRefund;
				locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
				CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
				output.refundedAmount = sadd(output.refundedAmount, locals.displacedRefund);
			}
			locals.activeQuantity -= locals.displacedQuantity;
			// Archive only retired orders; partially displaced orders remain in the live priority book.
			if (!locals.worstParticipantData.isActive)
			{
				locals.archiveParticipantInput.participantData = locals.worstParticipantData;
				CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
				locals.worstParticipantData = {};
			}
			state.mut().participants.set(locals.worstParticipantSlotIndex, locals.worstParticipantData);
		}

		locals.recomputeBatchHighestBidInput.auctionIndex = input.auctionIndex;
		CALL(RecomputeBatchHighestBid, locals.recomputeBatchHighestBidInput, locals.recomputeBatchHighestBidOutput);

		// Small-bid service fees are retained even if the bid is later displaced.
		if (locals.bidFeeCalculation.fee > 0)
		{
			locals.accumulateAuctionServiceFeeInput.feeAmount = locals.bidFeeCalculation.fee;
			CALL(AccumulateAuctionServiceFee, locals.accumulateAuctionServiceFeeInput, locals.accumulateAuctionServiceFeeOutput);
		}

		if (static_cast<uint64>(qpi.invocationReward()) > locals.bidFeeCalculation.requiredReward)
		{
			locals.payoutInput.recipient = qpi.invocator();
			locals.payoutInput.amount = static_cast<uint64>(qpi.invocationReward()) - locals.bidFeeCalculation.requiredReward;
			locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			output.refundedAmount =
			    sadd(output.refundedAmount, static_cast<uint64>(qpi.invocationReward()) - locals.bidFeeCalculation.requiredReward);
		}

		output.escrowedAmount = locals.bidFeeCalculation.escrowAmount;
		output.success = 1;
	}

	/**
	 * @brief Validates and records a Standard Auction bid, refunding replaced escrow.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessStandardBid)
	{
		output.escrowedAmount = 0;
		output.refundedAmount = 0;
		output.errorCode = EAuctionError::Success;
		output.success = 0;
		locals.highestBidderExists = 0;
		locals.finalizeImmediately = 0;
		locals.participantExists = 0;
		locals.freeParticipantSlotFound = 0;
		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			output.errorCode = EAuctionError::AuctionNotFound;
			return;
		}

		if (locals.auction.core.quantityForSale == 0 || locals.auction.core.quantityForSale < locals.auction.core.minimumPurchaseQuantity ||
		    input.bidAmount == 0)
		{
			output.errorCode = EAuctionError::InvalidInput;
			return;
		}
		// Reserve distinct entries for a replaced bidder, bidder change, three fee wallets, and the seller.
		// This also guarantees that an accepted Buy Now bid can complete settlement in the same call.
		if (state.get().pendingQuPayouts.population() > state.get().pendingQuPayouts.capacity() - NOST_STANDARD_BID_MAX_PAYOUT_RECIPIENTS)
		{
			output.errorCode = EAuctionError::PayoutQueueFull;
			return;
		}

		locals.requiredEscrow = input.bidAmount;
		if (static_cast<uint64>(qpi.invocationReward()) < locals.requiredEscrow)
		{
			output.errorCode = EAuctionError::InsufficientFunds;
			return;
		}

		if (locals.auction.core.highestBidPrice == 0)
		{
			if (input.bidAmount < locals.auction.core.initialPrice)
			{
				output.errorCode = EAuctionError::BidTooLow;
				return;
			}
		}
		else if (input.bidAmount < sadd(locals.auction.core.highestBidPrice, locals.auction.core.minimumBidIncrement))
		{
			output.errorCode = EAuctionError::BidTooLow;
			return;
		}

		// Standard bidders update their own active slot, while a new bidder needs one reusable slot.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participants.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantSlotIndex);
			if (locals.participantData.isUsed && locals.participantData.isActive && locals.participantData.auctionIndex == input.auctionIndex &&
			    locals.participantData.participant == qpi.invocator())
			{
				locals.participantExists = 1;
				break;
			}
			if (!locals.freeParticipantSlotFound && !locals.participantData.isUsed)
			{
				locals.freeParticipantSlotFound = 1;
				locals.freeParticipantSlotIndex = locals.participantSlotIndex;
			}
		}
		locals.previousEscrow = locals.participantExists ? locals.participantData.escrowedAmount : 0;
		if (!locals.participantExists && !locals.freeParticipantSlotFound)
		{
			output.errorCode = EAuctionError::StorageFull;
			return;
		}
		if (!locals.participantExists)
		{
			locals.participantSlotIndex = locals.freeParticipantSlotIndex;
			if (locals.auction.core.nextBidIndex == UINT64_MAX)
			{
				output.errorCode = EAuctionError::StorageFull;
				return;
			}
		}

		locals.participantData.escrowedAmount = locals.requiredEscrow;
		locals.participantData.requestedQuantity = locals.auction.core.quantityForSale;
		locals.participantData.allocatedQuantity = 0;
		locals.participantData.bidAmount = input.bidAmount;
		locals.participantData.lastBidTime = input.currentDate;
		locals.participantData.participant = qpi.invocator();
		locals.participantData.auctionIndex = input.auctionIndex;
		locals.participantData.bidIndex = locals.participantExists ? locals.participantData.bidIndex : locals.auction.core.nextBidIndex;
		locals.participantData.isUsed = 1;
		locals.participantData.isActive = 1;
		locals.participantData.isWinningBid = 0;
		if (!locals.participantExists)
		{
			locals.auction.core.nextBidIndex = sadd(locals.auction.core.nextBidIndex, 1ULL);
		}

		// A new highest bid releases the previous bidder's escrow before storing the replacement.
		locals.highestBidderSlotIndex = locals.auction.core.highestBidSlotIndex;
		if (locals.highestBidderSlotIndex < state.get().participants.capacity())
		{
			locals.previousHighestBidderData = state.get().participants.get(locals.highestBidderSlotIndex);
			locals.highestBidderExists = locals.previousHighestBidderData.isUsed && locals.previousHighestBidderData.isActive &&
			                             locals.previousHighestBidderData.auctionIndex == input.auctionIndex;
		}
		if (locals.highestBidderExists && locals.previousHighestBidderData.participant != qpi.invocator())
		{
			locals.payoutInput.recipient = locals.previousHighestBidderData.participant;
			locals.payoutInput.amount = locals.previousHighestBidderData.escrowedAmount;
			locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			output.refundedAmount = sadd(output.refundedAmount, locals.previousHighestBidderData.escrowedAmount);
			locals.previousHighestBidderData.escrowedAmount = 0;
			locals.previousHighestBidderData.requestedQuantity = 0;
			locals.previousHighestBidderData.isActive = 0;
			locals.previousHighestBidderData.isWinningBid = 0;
			locals.archiveParticipantInput.participantData = locals.previousHighestBidderData;
			CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
			locals.previousHighestBidderData = {};
			state.mut().participants.set(locals.highestBidderSlotIndex, locals.previousHighestBidderData);
		}

		locals.participantData.isWinningBid = 1;
		locals.auction.core.highestBidder = qpi.invocator();
		locals.auction.core.highestBidPrice = input.bidAmount;
		locals.auction.core.highestBidQuantity = locals.auction.core.quantityForSale;
		locals.auction.core.highestBidAmount = locals.requiredEscrow;
		locals.auction.core.highestBidSlotIndex = locals.participantSlotIndex;

		locals.auction.core.lastBidAt = input.currentDate;
		if ((locals.auction.core.auctionDurationSeconds - input.elapsedSeconds) <= NOST_AUCTION_EXTENSION_SECONDS)
		{
			locals.auction.core.auctionDurationSeconds = sadd(locals.auction.core.auctionDurationSeconds, NOST_AUCTION_EXTENSION_SECONDS);
		}
		if (locals.auction.core.buyNowPrice > 0 && input.bidAmount >= locals.auction.core.buyNowPrice)
		{
			locals.finalizeImmediately = 1;
		}

		state.mut().participants.set(locals.participantSlotIndex, locals.participantData);
		state.mut().auctionList.replace(input.auctionIndex, locals.auction);

		// Refund replaced self-escrow and excess reward after the new bid state is durable.
		if (locals.previousEscrow > 0)
		{
			locals.payoutInput.recipient = qpi.invocator();
			locals.payoutInput.amount = locals.previousEscrow;
			locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			output.refundedAmount = sadd(output.refundedAmount, locals.previousEscrow);
		}
		if (static_cast<uint64>(qpi.invocationReward()) > locals.requiredEscrow)
		{
			locals.payoutInput.recipient = qpi.invocator();
			locals.payoutInput.amount = static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow;
			locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			output.refundedAmount = sadd(output.refundedAmount, static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow);
		}

		output.escrowedAmount = locals.requiredEscrow;
		output.success = 1;

		// Buy Now closes the auction in the same procedure after the winning bid is recorded.
		if (locals.finalizeImmediately)
		{
			locals.finalizeStandardAuctionInput.auctionIndex = input.auctionIndex;
			locals.finalizeStandardAuctionInput.currentDate = input.currentDate;
			CALL(FinalizeStandardAuction, locals.finalizeStandardAuctionInput, locals.finalizeStandardAuctionOutput);
		}
	}

	/**
	 * @brief Validates the fixed-size IPFS metadata CID field.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateMetadataCid)
	{
		output.isValid = 0;
		locals.hasPayloadCharacters = 0;
		locals.reachedTerminator = 0;

		// Nostromo stores lowercase base32 CIDv1 values, which begin with the multibase prefix `b`.
		if (input.metadataIpfsCid.get(0) != QPI::Ch::b)
		{
			return;
		}

		// After the first zero byte, the fixed-size CID field must remain zero-padded.
		for (locals.cidIndex = 1; locals.cidIndex < input.metadataIpfsCid.capacity(); ++locals.cidIndex)
		{
			locals.cidChar = input.metadataIpfsCid.get(locals.cidIndex);
			if (locals.cidChar == 0)
			{
				locals.reachedTerminator = 1;
				continue;
			}

			if (locals.reachedTerminator)
			{
				return;
			}

			if ((locals.cidChar >= QPI::Ch::a && locals.cidChar <= QPI::Ch::z) || (locals.cidChar >= QPI::Ch::_2 && locals.cidChar <= QPI::Ch::_7))
			{
				locals.hasPayloadCharacters = 1;
				continue;
			}

			return;
		}

		if (!locals.hasPayloadCharacters)
		{
			return;
		}

		output.isValid = 1;
	}

	/**
	 * @brief Verifies that the invocator can escrow every non-empty lot asset.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(VerifyAuctionLotBalances)
	{
		output.hasEnoughBalance = 1;
		// Creation validates possession before attempting escrow so failures can refund without rollback.
		for (locals.lotItemIndex = 0; locals.lotItemIndex < input.auctionLotItems.capacity(); ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
			{
				continue;
			}

			locals.possessedShares = qpi.numberOfPossessedShares(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer, qpi.invocator(),
			                                                     qpi.invocator(), SELF_INDEX, SELF_INDEX);
			if (locals.possessedShares < locals.lotItem.quantity)
			{
				output.hasEnoughBalance = 0;
				return;
			}
		}
	}

	/**
	 * @brief Returns escrowed lot assets to the specified recipient.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(RollbackAuctionLotAssets)
	{
		// Rollback is shared by cancellation, failed creation, rejected standard sales, and no-sale finalization.
		for (locals.lotItemIndex = 0; locals.lotItemIndex < input.auctionLotItems.capacity(); ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
			{
				continue;
			}
			qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer, SELF, SELF, locals.lotItem.quantity,
			                                        input.recipient);
		}
	}

	/**
	 * @brief Settles a Batch Auction by allocating winning quantities and closing the auction.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeBatchAuction)
	{
		output.success = 0;
		locals.bestParticipantFound = 0;
		locals.lotItemFound = 0;
		locals.soldQuantity = 0;
		locals.totalGrossAmount = 0;

		// Abort if the auction no longer exists or is no longer an active batch auction.
		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		if (locals.auction.core.type != EAuctionType::Batch || locals.auction.core.status != EAuctionStatus::Active)
		{
			return;
		}
		if (state.get().pendingQuPayouts.population() >
		    state.get().pendingQuPayouts.capacity() - state.get().participants.capacity() - NOST_AUCTION_REVENUE_MAX_PAYOUT_RECIPIENTS)
		{
			return;
		}

		// Resolve the single sellable lot entry that represents the batch asset and quantity in escrow.
		for (locals.lotItemIndex = 0; locals.lotItemIndex < locals.auction.core.auctionLotItems.capacity(); ++locals.lotItemIndex)
		{
			locals.batchLotItem = locals.auction.core.auctionLotItems.get(locals.lotItemIndex);
			if (!isZeroAsset(locals.batchLotItem.asset) && locals.batchLotItem.quantity > 0)
			{
				locals.lotItemFound = 1;
				break;
			}
		}
		if (!locals.lotItemFound)
		{
			return;
		}

		// Stop before producing a fragment below the auction minimum; the remainder stays with the seller.
		locals.remainingQuantity = locals.auction.core.quantityForSale;
		while (locals.remainingQuantity >= locals.auction.core.minimumPurchaseQuantity)
		{
			locals.bestParticipantFound = 0;
			locals.participantIndex = 0;

			// Price priority is descending; the monotonic bid index is the only FIFO tie-breaker.
			while (locals.participantIndex < state.get().participants.capacity())
			{
				locals.participantData = state.get().participants.get(locals.participantIndex);
				if (locals.participantData.isUsed && locals.participantData.auctionIndex == input.auctionIndex)
				{
					if (locals.participantData.isActive && locals.participantData.escrowedAmount > 0)
					{
						if (!locals.bestParticipantFound || locals.participantData.bidAmount > locals.bestParticipantData.bidAmount ||
						    (locals.participantData.bidAmount == locals.bestParticipantData.bidAmount &&
						     locals.participantData.bidIndex < locals.bestParticipantData.bidIndex))
						{
							locals.bestParticipantFound = 1;
							locals.bestParticipantData = locals.participantData;
							locals.bestParticipantSlotIndex = locals.participantIndex;
						}
					}
				}
				++locals.participantIndex;
			}

			if (!locals.bestParticipantFound)
			{
				break;
			}

			// Price the winning allocation and compute any escrow surplus that must be returned immediately.
			locals.allocatedQuantity = min(locals.remainingQuantity, locals.bestParticipantData.requestedQuantity);
			locals.requiredPayment = smul(locals.allocatedQuantity, locals.bestParticipantData.bidAmount);
			locals.refundAmount = 0;
			if (locals.bestParticipantData.escrowedAmount > locals.requiredPayment)
			{
				locals.refundAmount = locals.bestParticipantData.escrowedAmount - locals.requiredPayment;
			}

			// Transfer the awarded shares, mark the participant as a winner, and advance settlement totals.
			if (locals.allocatedQuantity > 0)
			{
				qpi.transferShareOwnershipAndPossession(locals.batchLotItem.asset.assetName, locals.batchLotItem.asset.issuer, SELF, SELF,
				                                        locals.allocatedQuantity, locals.bestParticipantData.participant);
				locals.bestParticipantData.allocatedQuantity = locals.allocatedQuantity;
				locals.bestParticipantData.isWinningBid = 1;
				locals.soldQuantity = sadd(locals.soldQuantity, locals.allocatedQuantity);
				locals.totalGrossAmount = sadd(locals.totalGrossAmount, locals.requiredPayment);
				locals.remainingQuantity -= locals.allocatedQuantity;
			}

			// Return the unused part of the winner escrow when the participant requested more than the remaining supply.
			if (locals.refundAmount > 0)
			{
				locals.payoutInput.recipient = locals.bestParticipantData.participant;
				locals.payoutInput.amount = locals.refundAmount;
				locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
				CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
				if (!locals.payoutOutput.success)
				{
					return;
				}
			}

			// Archive the completed bid and release its active slot immediately.
			locals.bestParticipantData.escrowedAmount = 0;
			locals.bestParticipantData.isActive = 0;
			locals.archiveParticipantInput.participantData = locals.bestParticipantData;
			CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
			locals.bestParticipantData = {};
			state.mut().participants.set(locals.bestParticipantSlotIndex, locals.bestParticipantData);
		}

		// Refund every non-winning or non-allocated bid that still has escrow locked after winner selection.
		locals.participantIndex = 0;
		while (locals.participantIndex < state.get().participants.capacity())
		{
			locals.participantData = state.get().participants.get(locals.participantIndex);
			if (locals.participantData.isUsed && locals.participantData.auctionIndex == input.auctionIndex)
			{
				if (locals.participantData.escrowedAmount > 0)
				{
					locals.payoutInput.recipient = locals.participantData.participant;
					locals.payoutInput.amount = locals.participantData.escrowedAmount;
					locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
					CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
					if (!locals.payoutOutput.success)
					{
						return;
					}
					locals.participantData.escrowedAmount = 0;
					locals.participantData.allocatedQuantity = 0;
					locals.participantData.isWinningBid = 0;
				}
				locals.participantData.isActive = 0;
				locals.archiveParticipantInput.participantData = locals.participantData;
				CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
				locals.participantData = {};
				state.mut().participants.set(locals.participantIndex, locals.participantData);
			}
			++locals.participantIndex;
		}

		// Return any unsold batch quantity to the seller when demand did not consume the entire lot.
		if (locals.soldQuantity < locals.auction.core.quantityForSale)
		{
			qpi.transferShareOwnershipAndPossession(locals.batchLotItem.asset.assetName, locals.batchLotItem.asset.issuer, SELF, SELF,
			                                        locals.auction.core.quantityForSale - locals.soldQuantity, locals.auction.core.seller);
		}

		// Split the collected proceeds according to Nostromo auction fee rules and pay the seller net amount.
		locals.distributeAuctionRevenueInput.seller = locals.auction.core.seller;
		locals.distributeAuctionRevenueInput.grossAmount = locals.totalGrossAmount;
		CALL(DistributeAuctionRevenue, locals.distributeAuctionRevenueInput, locals.distributeAuctionRevenueOutput);
		if (!locals.distributeAuctionRevenueOutput.success)
		{
			return;
		}

		// Persist the final sold quantity and close the auction as settled.
		locals.auction.core.allocatedQuantity = locals.soldQuantity;
		locals.auction.core.status = EAuctionStatus::Finalized;
		locals.auction.core.settledAt = input.currentDate;
		locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		state.mut().totalFinalizedAuctions = sadd(state.get().totalFinalizedAuctions, 1ULL);
		locals.archiveClosedAuctionInput.auction = locals.auction;
		CALL(ArchiveClosedAuction, locals.archiveClosedAuctionInput, locals.archiveClosedAuctionOutput);
		output.success = 1;
	}

	/**
	 * @brief Settles a Standard Auction by transferring the lot or returning it to the seller.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeStandardAuction)
	{
		output.success = 0;
		locals.highestBidderExists = 0;
		locals.lotSold = 0;

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		if (locals.auction.core.type != EAuctionType::Standard)
		{
			return;
		}
		if (state.get().pendingQuPayouts.population() > state.get().pendingQuPayouts.capacity() - NOST_STANDARD_FINALIZATION_MAX_PAYOUT_RECIPIENTS)
		{
			return;
		}

		locals.highestBidderSlotIndex = locals.auction.core.highestBidSlotIndex;
		if (locals.highestBidderSlotIndex < state.get().participants.capacity())
		{
			locals.highestBidderData = state.get().participants.get(locals.highestBidderSlotIndex);
			locals.highestBidderExists =
			    locals.highestBidderData.isUsed && locals.highestBidderData.isActive && locals.highestBidderData.auctionIndex == input.auctionIndex;
		}

		// A valid highest bid transfers the whole standard lot and treats escrow as gross proceeds.
		if (locals.highestBidderExists && locals.highestBidderData.escrowedAmount > 0)
		{
			locals.rollbackAuctionLotAssetsInput.auctionLotItems = locals.auction.core.auctionLotItems;
			locals.rollbackAuctionLotAssetsInput.recipient = locals.highestBidderData.participant;
			CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);

			locals.distributeAuctionRevenueInput.seller = locals.auction.core.seller;
			locals.distributeAuctionRevenueInput.grossAmount = locals.highestBidderData.escrowedAmount;
			CALL(DistributeAuctionRevenue, locals.distributeAuctionRevenueInput, locals.distributeAuctionRevenueOutput);
			if (!locals.distributeAuctionRevenueOutput.success)
			{
				return;
			}

			locals.highestBidderData.allocatedQuantity = locals.auction.core.quantityForSale;
			locals.highestBidderData.isWinningBid = 1;
			locals.highestBidderData.escrowedAmount = 0;
			locals.highestBidderData.isActive = 0;
			locals.archiveParticipantInput.participantData = locals.highestBidderData;
			CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
			locals.highestBidderData = {};
			state.mut().participants.set(locals.highestBidderSlotIndex, locals.highestBidderData);
			locals.auction.core.allocatedQuantity = locals.auction.core.quantityForSale;
			locals.lotSold = 1;
		}
		else
		{
			// No active funded bid means the seller receives the lot back with no revenue distribution.
			locals.rollbackAuctionLotAssetsInput.auctionLotItems = locals.auction.core.auctionLotItems;
			locals.rollbackAuctionLotAssetsInput.recipient = locals.auction.core.seller;
			CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);
			locals.auction.core.allocatedQuantity = 0;
		}

		// Closed standard auctions retain winner fields only when the lot actually sold.
		locals.auction.core.status = EAuctionStatus::Finalized;
		locals.auction.core.settledAt = input.currentDate;
		if (!locals.lotSold)
		{
			locals.auction.core.highestBidAmount = 0;
			locals.auction.core.highestBidPrice = 0;
			locals.auction.core.highestBidQuantity = 0;
			locals.auction.core.highestBidder = NULL_ID;
		}
		locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		state.mut().totalFinalizedAuctions = sadd(state.get().totalFinalizedAuctions, 1ULL);
		locals.archiveClosedAuctionInput.auction = locals.auction;
		CALL(ArchiveClosedAuction, locals.archiveClosedAuctionInput, locals.archiveClosedAuctionOutput);
		output.success = 1;
	}

	/**
	 * @brief Rejects a pending Standard Auction bid and closes the auction without a sale.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(RejectStandardAuction)
	{
		output.refundedAmount = 0;
		output.success = 0;
		locals.highestBidderExists = 0;

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			return;
		}

		if (locals.auction.core.type != EAuctionType::Standard || locals.auction.core.status != EAuctionStatus::PendingSellerDecision)
		{
			return;
		}
		if (state.get().pendingQuPayouts.population() == state.get().pendingQuPayouts.capacity())
		{
			return;
		}

		locals.highestBidderSlotIndex = locals.auction.core.highestBidSlotIndex;
		if (locals.highestBidderSlotIndex < state.get().participants.capacity())
		{
			locals.highestBidderData = state.get().participants.get(locals.highestBidderSlotIndex);
			locals.highestBidderExists =
			    locals.highestBidderData.isUsed && locals.highestBidderData.isActive && locals.highestBidderData.auctionIndex == input.auctionIndex;
		}

		// Seller rejection unwinds the pending bid instead of distributing its escrow as proceeds.
		if (locals.highestBidderExists && locals.highestBidderData.escrowedAmount > 0)
		{
			locals.payoutInput.recipient = locals.highestBidderData.participant;
			locals.payoutInput.amount = locals.highestBidderData.escrowedAmount;
			locals.payoutInput.maxChunks = NOST_IMMEDIATE_QU_TRANSFER_CHUNKS_PER_CALL;
			CALL(QueueAndFlushQuPayout, locals.payoutInput, locals.payoutOutput);
			if (!locals.payoutOutput.success)
			{
				return;
			}
			output.refundedAmount = locals.highestBidderData.escrowedAmount;
			locals.highestBidderData.escrowedAmount = 0;
			locals.highestBidderData.allocatedQuantity = 0;
			locals.highestBidderData.isActive = 0;
			locals.highestBidderData.isWinningBid = 0;
			locals.archiveParticipantInput.participantData = locals.highestBidderData;
			CALL(ArchiveParticipant, locals.archiveParticipantInput, locals.archiveParticipantOutput);
			locals.highestBidderData = {};
			state.mut().participants.set(locals.highestBidderSlotIndex, locals.highestBidderData);
		}

		// The seller keeps the lot after rejection, and the auction is closed as finalized.
		locals.rollbackAuctionLotAssetsInput.auctionLotItems = locals.auction.core.auctionLotItems;
		locals.rollbackAuctionLotAssetsInput.recipient = locals.auction.core.seller;
		CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);

		locals.auction.core.allocatedQuantity = 0;
		locals.auction.core.highestBidAmount = 0;
		locals.auction.core.highestBidPrice = 0;
		locals.auction.core.highestBidQuantity = 0;
		locals.auction.core.highestBidder = NULL_ID;
		locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		locals.auction.core.status = EAuctionStatus::Finalized;
		locals.auction.core.settledAt = input.currentDate;
		state.mut().totalFinalizedAuctions = sadd(state.get().totalFinalizedAuctions, 1ULL);
		locals.archiveClosedAuctionInput.auction = locals.auction;
		CALL(ArchiveClosedAuction, locals.archiveClosedAuctionInput, locals.archiveClosedAuctionOutput);
		output.success = 1;
	}

	/**
	 * @brief Transfers auction lot assets into contract escrow during creation.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(EscrowAuctionLotAssets)
	{
		output.success = 1;
		// Escrow entries one by one; a later failure rolls back earlier successful transfers.
		for (locals.lotItemIndex = 0; locals.lotItemIndex < input.auctionLotItems.capacity(); ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
			{
				continue;
			}

			locals.remainingShares = qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer,
			                                                                 qpi.invocator(), qpi.invocator(), locals.lotItem.quantity, SELF);
			if (locals.remainingShares < 0)
			{
				// `transferShareOwnershipAndPossession` returns the remaining number of matching shares after a successful transfer.
				// Negative values mean the transfer failed without moving the requested lot entry.
				for (locals.rollbackLotItemIndex = 0; locals.rollbackLotItemIndex < locals.lotItemIndex; ++locals.rollbackLotItemIndex)
				{
					locals.lotItem = input.auctionLotItems.get(locals.rollbackLotItemIndex);
					if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
					{
						continue;
					}
					qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer, SELF, SELF,
					                                        locals.lotItem.quantity, qpi.invocator());
				}
				output.success = 0;
				return;
			}
		}
	}

	/**
	 * @brief Creates a new Batch Auction or Standard Auction in the Nostromo Auction House.
	 * @note `CreateAuction_input` defines the IPFS metadata CID stored through Pinata, the auction lot, pricing, duration, and visibility rules.
	 * @note Batch auctions require `minimumPurchaseQuantity` in the range `[1, quantityForSale]`; standard auctions ignore it and store zero.
	 * @note A successful public Batch or Standard Auction accumulates the configured public creation fee, distributed at `END_EPOCH`.
	 * Insufficient payment rejects creation, overpayment is refunded, and failed creation refunds the full reward.
	 * @note Private auctions require the configured private auction fee, which is accumulated and distributed at `END_EPOCH` between shareholders
	 * and the configured fee recipients, and must use at least one access mode. If both modes are configured, either one grants access.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateAuction)
	{
		output.errorCode = EAuctionError::InvalidInput;

		// Any rejection before escrow succeeds refunds the full invocation reward.
		CALL(IsAuctionInteractionPaused, locals.isAuctionInteractionPausedInput, locals.isAuctionInteractionPausedOutput);
		if (locals.isAuctionInteractionPausedOutput.isPaused)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionPaused;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		if (!isSupportedAuctionType(static_cast<EAuctionType>(input.auctionType)))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InvalidAuctionType;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		if (!isSupportedAuctionVisibility(static_cast<EAuctionVisibility>(input.auctionVisibility)))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InvalidVisibility;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		if (state.get().auctionList.population() >= state.get().auctionList.capacity())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::StorageFull;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		if (state.get().totalAuctionsCreated == UINT64_MAX)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionIndexExhausted;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		locals.validateMetadataCidInput.metadataIpfsCid = input.metadataIpfsCid;
		CALL(ValidateMetadataCid, locals.validateMetadataCidInput, locals.validateMetadataCidOutput);
		if (!locals.validateMetadataCidOutput.isValid)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		locals.analyzeAuctionLotInput.auctionLotItems = input.auctionLotItems;
		locals.analyzeAuctionLotInput.durationDays = input.durationDays;
		CALL(AnalyzeAuctionLot, locals.analyzeAuctionLotInput, locals.analyzeAuctionLotOutput);
		if (!locals.analyzeAuctionLotOutput.isValid)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}
		// Resolve auction-type-specific quantity and price invariants before touching assets.
		locals.resolvedQuantityForSale = 0;
		locals.resolvedMinimumPurchaseQuantity = 0;
		switch (static_cast<EAuctionType>(input.auctionType))
		{
			case EAuctionType::Batch:

				if (!resolveBatchAuctionCreateParams(locals.analyzeAuctionLotOutput.lotItemCount, locals.analyzeAuctionLotOutput.totalEscrowQuantity,
				                                     input.minimumPurchaseQuantity, locals.resolvedQuantityForSale,
				                                     locals.resolvedMinimumPurchaseQuantity, input.buyNowPrice))
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.errorCode = EAuctionError::InvalidInput;
					setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
					                     qpi.invocationReward());
					logProcedureResult(locals.log);
					return;
				}
				break;
			case EAuctionType::Standard:
				if (!resolveStandardAuctionCreateParams(input.minimumBidIncrement, locals.resolvedQuantityForSale,
				                                        locals.resolvedMinimumPurchaseQuantity, input.buyNowPrice, input.initialPrice,
				                                        input.salePrice))
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.errorCode = EAuctionError::InvalidInput;
					setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
					                     qpi.invocationReward());
					logProcedureResult(locals.log);
					return;
				}
				break;
			default:
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = EAuctionError::InvalidAuctionType;
				setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
				                     qpi.invocationReward());
				logProcedureResult(locals.log);
				return;
		}

		// Private auctions require at least one access gate and may combine wallet and asset access.
		locals.countAllowedBidderWalletsInput.allowedBidderWallets = input.allowedBidderWallets;
		CALL(CountAllowedBidderWallets, locals.countAllowedBidderWalletsInput, locals.countAllowedBidderWalletsOutput);
		locals.countRequiredAccessAssetsInput.requiredAccessAssets = input.requiredAccessAssets;
		CALL(CountRequiredAccessAssets, locals.countRequiredAccessAssetsInput, locals.countRequiredAccessAssetsOutput);
		if (!locals.countRequiredAccessAssetsOutput.isValid ||
		    !validatePrivateAuctionAccess(static_cast<EAuctionVisibility>(input.auctionVisibility),
		                                  locals.countRequiredAccessAssetsOutput.requiredAccessAssetCount,
		                                  locals.countAllowedBidderWalletsOutput.allowedWalletCount))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		locals.requiredFee = getCreateAuctionFee(static_cast<EAuctionVisibility>(input.auctionVisibility), state);
		if (qpi.invocationReward() < locals.requiredFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InsufficientFunds;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		locals.verifyAuctionLotBalancesInput.auctionLotItems = input.auctionLotItems;
		CALL(VerifyAuctionLotBalances, locals.verifyAuctionLotBalancesInput, locals.verifyAuctionLotBalancesOutput);
		if (!locals.verifyAuctionLotBalancesOutput.hasEnoughBalance)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InsufficientAssetBalance;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		// From this point onward, asset escrow may need explicit rollback on storage failure.
		locals.escrowAuctionLotAssetsInput.auctionLotItems = input.auctionLotItems;
		CALL(EscrowAuctionLotAssets, locals.escrowAuctionLotAssetsInput, locals.escrowAuctionLotAssetsOutput);
		if (!locals.escrowAuctionLotAssetsOutput.success)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InsufficientAssetBalance;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		locals.auction.core.auctionIndex = state.get().totalAuctionsCreated;
		locals.auction.core.quantityForSale = locals.resolvedQuantityForSale;
		locals.auction.core.minimumPurchaseQuantity = locals.resolvedMinimumPurchaseQuantity;
		locals.auction.core.initialPrice = input.initialPrice;
		locals.auction.core.salePrice = input.salePrice;
		locals.auction.core.minimumBidIncrement = input.minimumBidIncrement;
		locals.auction.core.buyNowPrice = input.buyNowPrice;
		locals.auction.core.auctionDurationSeconds = smul(static_cast<uint64>(input.durationDays), NOST_SECONDS_PER_DAY);
		locals.auction.core.createdAt = qpi.now();
		locals.auction.core.lastBidAt = locals.auction.core.createdAt;
		locals.auction.core.seller = qpi.invocator();
		locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		// Duplicate required access assets collapse to the highest configured quantity.
		for (locals.requiredAccessAssetIndex = 0; locals.requiredAccessAssetIndex < input.requiredAccessAssets.capacity();
		     ++locals.requiredAccessAssetIndex)
		{
			locals.requiredAccessAsset = input.requiredAccessAssets.get(locals.requiredAccessAssetIndex);
			if (!isZeroAsset(locals.requiredAccessAsset.asset) &&
			    (!locals.auction.requiredAccessAssets.get(locals.requiredAccessAsset.asset, locals.existingRequiredAccessQuantity) ||
			     locals.requiredAccessAsset.quantity > locals.existingRequiredAccessQuantity))
			{
				locals.auction.requiredAccessAssets.set(locals.requiredAccessAsset.asset, locals.requiredAccessAsset.quantity);
			}
		}
		locals.auction.core.auctionLotItems = input.auctionLotItems;
		for (locals.allowedWalletIndex = 0; locals.allowedWalletIndex < input.allowedBidderWallets.capacity(); ++locals.allowedWalletIndex)
		{
			if (!isZero(input.allowedBidderWallets.get(locals.allowedWalletIndex)))
			{
				locals.auction.allowedBidderWallets.add(input.allowedBidderWallets.get(locals.allowedWalletIndex));
			}
		}
		locals.auction.core.metadataIpfsCid = input.metadataIpfsCid;
		locals.auction.core.type = static_cast<EAuctionType>(input.auctionType);
		locals.auction.core.visibility = static_cast<EAuctionVisibility>(input.auctionVisibility);
		locals.auction.core.status = EAuctionStatus::Active;

		// If persistent auction storage fails after escrow, return the lot before refunding the fee reward.
		if (state.mut().auctionList.set(locals.auction.core.auctionIndex, locals.auction) == NULL_INDEX)
		{
			locals.rollbackAuctionLotAssetsInput.auctionLotItems = input.auctionLotItems;
			locals.rollbackAuctionLotAssetsInput.recipient = qpi.invocator();
			CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::StorageFull;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex,
			                     qpi.invocationReward());
			logProcedureResult(locals.log);
			return;
		}

		// Creation fees are held until END_EPOCH; overpayment is returned immediately.
		if (locals.requiredFee > 0)
		{
			locals.accumulateAuctionServiceFeeInput.feeAmount = static_cast<uint64>(locals.requiredFee);
			CALL(AccumulateAuctionServiceFee, locals.accumulateAuctionServiceFeeInput, locals.accumulateAuctionServiceFeeOutput);
		}

		if (qpi.invocationReward() > locals.requiredFee)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.requiredFee);
		}

		output.auctionIndex = locals.auction.core.auctionIndex;
		state.mut().totalAuctionsCreated = sadd(state.get().totalAuctionsCreated, 1ULL);
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CreateAuction, output.errorCode, output.auctionIndex, qpi.invocationReward());
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Places a bid in an active auction.
	 * @note Batch auctions interpret `bidAmount` as price per asset and reject requested `quantity` below `minimumPurchaseQuantity` with a full
	 * refund.
	 * @note An accepted Batch bid escrows `quantity * bidAmount` and accumulates `max(100 - quantity * bidAmount, 0)` qu for distribution at
	 * `END_EPOCH`. Excess reward is refunded; rejected bids refund the full reward. The accumulated fee is not refunded if the bid is later
	 * displaced.
	 * @note Batch final allocations are also at least `minimumPurchaseQuantity`; smaller unsold remainders return to the seller and affected bids are
	 * fully refunded.
	 * @note Standard auctions interpret `bidAmount` as the total price for the whole lot and ignore `quantity`.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(PlaceBid)
	{
		output.errorCode = EAuctionError::InvalidInput;

		// Common auction gates run before type-specific bid processing; failed gates refund the reward.
		CALL(IsAuctionInteractionPaused, locals.isAuctionInteractionPausedInput, locals.isAuctionInteractionPausedOutput);
		if (locals.isAuctionInteractionPausedOutput.isPaused)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionPaused;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			locals.findAuctionInput.auctionIndex = input.auctionIndex;
			CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = locals.findAuctionOutput.found ? EAuctionError::AuctionClosed : EAuctionError::AuctionNotFound;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.status != EAuctionStatus::Active)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionClosed;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.seller == qpi.invocator())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
			logProcedureResult(locals.log);
			return;
		}

		locals.currentDate = qpi.now();
		diffDateInSecond(locals.auction.core.createdAt, locals.currentDate, locals.elapsedSeconds);
		if (locals.elapsedSeconds >= locals.auction.core.auctionDurationSeconds)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionClosed;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
			logProcedureResult(locals.log);
			return;
		}

		// When both gates are configured, satisfying either one grants access.
		if (locals.auction.core.visibility == EAuctionVisibility::Private)
		{
			locals.hasAccess = locals.auction.allowedBidderWallets.population() > 0 && locals.auction.allowedBidderWallets.contains(qpi.invocator());
			if (!locals.hasAccess && locals.auction.requiredAccessAssets.population() > 0)
			{
				locals.hasRequiredAccessAssetInput.auctionIndex = input.auctionIndex;
				CALL(HasRequiredAccessAsset, locals.hasRequiredAccessAssetInput, locals.hasRequiredAccessAssetOutput);
				locals.hasAccess = locals.hasRequiredAccessAssetOutput.hasRequiredAccessAsset;
			}

			if (!locals.hasAccess)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = EAuctionError::PrivateAuctionAccessDenied;
				setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex,
				                     output.escrowedAmount);
				logProcedureResult(locals.log);
				return;
			}
		}

		// Type-specific processors own escrow/refund details once common validation succeeds.
		switch (locals.auction.core.type)
		{
			case EAuctionType::Batch:
				locals.processBatchBidInput.auctionIndex = input.auctionIndex;
				locals.processBatchBidInput.effectiveQuantity = input.quantity;
				locals.processBatchBidInput.bidAmount = input.bidAmount;
				locals.processBatchBidInput.currentDate = locals.currentDate;
				locals.processBatchBidInput.elapsedSeconds = locals.elapsedSeconds;
				CALL(ProcessBatchBid, locals.processBatchBidInput, locals.processBatchBidOutput);
				if (!locals.processBatchBidOutput.success)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.refundedAmount = locals.processBatchBidOutput.refundedAmount;
					output.errorCode = locals.processBatchBidOutput.errorCode;
					setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex,
					                     output.escrowedAmount);
					logProcedureResult(locals.log);
					return;
				}
				output.refundedAmount = sadd(output.refundedAmount, locals.processBatchBidOutput.refundedAmount);
				output.escrowedAmount = locals.processBatchBidOutput.escrowedAmount;
				break;
			case EAuctionType::Standard:
				locals.processStandardBidInput.auctionIndex = input.auctionIndex;
				locals.processStandardBidInput.bidAmount = input.bidAmount;
				locals.processStandardBidInput.currentDate = locals.currentDate;
				locals.processStandardBidInput.elapsedSeconds = locals.elapsedSeconds;
				CALL(ProcessStandardBid, locals.processStandardBidInput, locals.processStandardBidOutput);
				if (!locals.processStandardBidOutput.success)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.errorCode = locals.processStandardBidOutput.errorCode;
					setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex,
					                     output.escrowedAmount);
					logProcedureResult(locals.log);
					return;
				}
				output.refundedAmount = sadd(output.refundedAmount, locals.processStandardBidOutput.refundedAmount);
				output.escrowedAmount = locals.processStandardBidOutput.escrowedAmount;
				break;
			default:
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = EAuctionError::InvalidAuctionType;
				setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex,
				                     output.escrowedAmount);
				logProcedureResult(locals.log);
				return;
		}
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::PlaceBid, output.errorCode, input.auctionIndex, output.escrowedAmount);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Cancels an active auction before the first accepted bid is placed.
	 * @note Once any bid is accepted, the seller can no longer cancel the auction.
	 * @note The cancellation fee is based on the configured reserve price for the full batch quantity or standard lot and is distributed between
	 * shareholders and the configured fee recipients.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CancelAuction)
	{
		output.refundedAmount = 0;
		output.cancellationFee = 0;
		output.errorCode = EAuctionError::InvalidInput;

		// Cancellation is blocked during emergency pause but does not use the scheduled auction timer pause.
		if (state.get().isEmergencyPaused)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionPaused;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			locals.findAuctionInput.auctionIndex = input.auctionIndex;
			CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = locals.findAuctionOutput.found ? EAuctionError::AuctionClosed : EAuctionError::AuctionNotFound;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.status != EAuctionStatus::Active)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionClosed;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.seller != qpi.invocator())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.nextBidIndex != 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::AuctionHasAcceptedBid;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		// The fee base represents the full reserve value of the lot being withdrawn.
		locals.cancellationBaseAmount = locals.auction.core.salePrice;
		if (locals.auction.core.type == EAuctionType::Batch)
		{
			locals.cancellationBaseAmount = smul(locals.auction.core.salePrice, locals.auction.core.quantityForSale);
		}
		output.cancellationFee = calculateBasisPointAmount(locals.cancellationBaseAmount, state.get().auctionCancellationFeeBasisPoints);

		if (static_cast<uint64>(qpi.invocationReward()) < output.cancellationFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = EAuctionError::InsufficientFunds;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex,
			                     output.cancellationFee);
			logProcedureResult(locals.log);
			return;
		}

		locals.rollbackAuctionLotAssetsInput.auctionLotItems = locals.auction.core.auctionLotItems;
		locals.rollbackAuctionLotAssetsInput.recipient = locals.auction.core.seller;
		CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);

		// Cancellation closes the auction and records it in the same history ring as finalized auctions.
		locals.currentDate = qpi.now();
		locals.auction.core.status = EAuctionStatus::Cancelled;
		locals.auction.core.settledAt = locals.currentDate;
		locals.auction.core.allocatedQuantity = 0;
		locals.auction.core.highestBidAmount = 0;
		locals.auction.core.highestBidPrice = 0;
		locals.auction.core.highestBidQuantity = 0;
		locals.auction.core.highestBidder = NULL_ID;
		locals.auction.core.highestBidSlotIndex = NOST_INVALID_PARTICIPANT_SLOT;
		state.mut().totalCancelledAuctions = sadd(state.get().totalCancelledAuctions, 1ULL);
		locals.archiveClosedAuctionInput.auction = locals.auction;
		CALL(ArchiveClosedAuction, locals.archiveClosedAuctionInput, locals.archiveClosedAuctionOutput);

		// Cancellation fees use the same epoch pool as creation and small-bid service fees.
		locals.accumulateAuctionServiceFeeInput.feeAmount = output.cancellationFee;
		CALL(AccumulateAuctionServiceFee, locals.accumulateAuctionServiceFeeInput, locals.accumulateAuctionServiceFeeOutput);

		if (static_cast<uint64>(qpi.invocationReward()) > output.cancellationFee)
		{
			qpi.transfer(qpi.invocator(), static_cast<uint64>(qpi.invocationReward()) - output.cancellationFee);
		}

		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::CancelAuction, output.errorCode, input.auctionIndex, output.cancellationFee);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Lets the seller accept or reject a pending standard auction whose highest bid stayed below the sale price.
	 * @note The manual decision window lasts one week; after expiry the contract finalizes the sale automatically in favor of the buyer.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(ResolvePendingStandardAuction)
	{
		output.refundedAmount = 0;
		output.errorCode = EAuctionError::InvalidInput;

		// This procedure does not need a reward; return any supplied amount before validation.
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		CALL(IsAuctionInteractionPaused, locals.isAuctionInteractionPausedInput, locals.isAuctionInteractionPausedOutput);
		if (locals.isAuctionInteractionPausedOutput.isPaused)
		{
			output.errorCode = EAuctionError::AuctionPaused;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (input.acceptSale > 1)
		{
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (!state.get().auctionList.get(input.auctionIndex, locals.auction))
		{
			locals.findAuctionInput.auctionIndex = input.auctionIndex;
			CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
			output.errorCode = locals.findAuctionOutput.found ? EAuctionError::AuctionClosed : EAuctionError::AuctionNotFound;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.seller != qpi.invocator())
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		if (locals.auction.core.type != EAuctionType::Standard || locals.auction.core.status != EAuctionStatus::PendingSellerDecision)
		{
			output.errorCode = EAuctionError::AuctionClosed;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		// If the decision window has expired, the automatic sale wins over the seller action.
		locals.currentDate = qpi.now();
		if (!state.get().isAuctionTimerPaused && locals.auction.core.sellerDecisionDeadline <= locals.currentDate)
		{
			locals.finalizeStandardAuctionInput.auctionIndex = input.auctionIndex;
			locals.finalizeStandardAuctionInput.currentDate = locals.currentDate;
			CALL(FinalizeStandardAuction, locals.finalizeStandardAuctionInput, locals.finalizeStandardAuctionOutput);

			output.errorCode = EAuctionError::AuctionClosed;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
			                     output.refundedAmount);
			logProcedureResult(locals.log);
			return;
		}

		// Accepting finalizes the sale; rejecting refunds the bidder and returns the lot to the seller.
		if (input.acceptSale)
		{
			locals.finalizeStandardAuctionInput.auctionIndex = input.auctionIndex;
			locals.finalizeStandardAuctionInput.currentDate = locals.currentDate;
			CALL(FinalizeStandardAuction, locals.finalizeStandardAuctionInput, locals.finalizeStandardAuctionOutput);
			output.errorCode = locals.finalizeStandardAuctionOutput.success ? EAuctionError::Success : EAuctionError::AuctionClosed;
		}
		else
		{
			locals.rejectStandardAuctionInput.auctionIndex = input.auctionIndex;
			locals.rejectStandardAuctionInput.currentDate = locals.currentDate;
			CALL(RejectStandardAuction, locals.rejectStandardAuctionInput, locals.rejectStandardAuctionOutput);
			output.refundedAmount = locals.rejectStandardAuctionOutput.refundedAmount;
			output.errorCode = locals.rejectStandardAuctionOutput.success ? EAuctionError::Success : EAuctionError::AuctionClosed;
		}
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::ResolvePendingStandardAuction, output.errorCode, input.auctionIndex,
		                     output.refundedAmount);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Overwrites the full auction fee configuration.
	 * @note Only the configured takeover coordinator can call this procedure.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetAuctionFees)
	{
		output.errorCode = EAuctionError::InvalidInput;
		// Administrative procedures never consume invocation rewards.
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().takeoverCoordinator)
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFees, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		// Validate all fee tiers together so no gross-proceeds tier can exceed 100 percent.
		if (!isValidAuctionFeeConfiguration(input.privateAuctionFee, input.publicAuctionCreationFee, input.auctionCancellationFeeBasisPoints,
		                                    input.managementFeeBasisPoints, input.developmentFeeBasisPoints, input.takeoverCoordinatorFeeBasisPoints,
		                                    input.shareholderDividendBasisPoints, input.shareholderFeeBasisPointsTier1,
		                                    input.shareholderFeeBasisPointsTier2, input.shareholderFeeBasisPointsTier3,
		                                    input.shareholderFeeBasisPointsTier4))
		{
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFees, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		state.mut().privateAuctionFee = input.privateAuctionFee;
		state.mut().publicAuctionCreationFee = input.publicAuctionCreationFee;
		state.mut().auctionCancellationFeeBasisPoints = input.auctionCancellationFeeBasisPoints;
		state.mut().managementFeeBasisPoints = input.managementFeeBasisPoints;
		state.mut().developmentFeeBasisPoints = input.developmentFeeBasisPoints;
		state.mut().takeoverCoordinatorFeeBasisPoints = input.takeoverCoordinatorFeeBasisPoints;
		state.mut().shareholderDividendBasisPoints = input.shareholderDividendBasisPoints;
		state.mut().shareholderFeeBasisPointsTier1 = input.shareholderFeeBasisPointsTier1;
		state.mut().shareholderFeeBasisPointsTier2 = input.shareholderFeeBasisPointsTier2;
		state.mut().shareholderFeeBasisPointsTier3 = input.shareholderFeeBasisPointsTier3;
		state.mut().shareholderFeeBasisPointsTier4 = input.shareholderFeeBasisPointsTier4;
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFees, output.errorCode, 0, 0);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Updates every auction fee except the takeover coordinator-specific splits.
	 * @note Only the configured management wallet can call this procedure.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetAuctionFeesByManagement)
	{
		output.errorCode = EAuctionError::InvalidInput;

		// Management can update operational fees, but takeover-specific fee parameters stay unchanged.
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().management)
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFeesByManagement, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		if (!isValidAuctionFeeConfiguration(input.privateAuctionFee, input.publicAuctionCreationFee, input.auctionCancellationFeeBasisPoints,
		                                    input.managementFeeBasisPoints, input.developmentFeeBasisPoints,
		                                    state.get().takeoverCoordinatorFeeBasisPoints, state.get().shareholderDividendBasisPoints,
		                                    input.shareholderFeeBasisPointsTier1, input.shareholderFeeBasisPointsTier2,
		                                    input.shareholderFeeBasisPointsTier3, input.shareholderFeeBasisPointsTier4))
		{
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFeesByManagement, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		state.mut().privateAuctionFee = input.privateAuctionFee;
		state.mut().publicAuctionCreationFee = input.publicAuctionCreationFee;
		state.mut().auctionCancellationFeeBasisPoints = input.auctionCancellationFeeBasisPoints;
		state.mut().managementFeeBasisPoints = input.managementFeeBasisPoints;
		state.mut().developmentFeeBasisPoints = input.developmentFeeBasisPoints;
		state.mut().shareholderFeeBasisPointsTier1 = input.shareholderFeeBasisPointsTier1;
		state.mut().shareholderFeeBasisPointsTier2 = input.shareholderFeeBasisPointsTier2;
		state.mut().shareholderFeeBasisPointsTier3 = input.shareholderFeeBasisPointsTier3;
		state.mut().shareholderFeeBasisPointsTier4 = input.shareholderFeeBasisPointsTier4;
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetAuctionFeesByManagement, output.errorCode, 0, 0);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Reassigns the management role to another wallet.
	 * @note Only the configured takeover coordinator can call this procedure.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetManagement)
	{
		output.errorCode = EAuctionError::InvalidInput;

		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().takeoverCoordinator)
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetManagement, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		if (isZero(input.management))
		{
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetManagement, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		state.mut().management = input.management;
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetManagement, output.errorCode, 0, 0);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Configures the execution fee reserve guard that triggers an emergency pause on a sudden reserve drop.
	 * @note Only the configured takeover coordinator or management wallet can call this procedure.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetFeeReserveGuardConfig)
	{
		output.errorCode = EAuctionError::InvalidInput;
		// Resetting the baseline forces the guard to start a fresh observation window.
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().takeoverCoordinator && qpi.invocator() != state.get().management)
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetFeeReserveGuardConfig, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		if (input.dropBasisPoints == 0 || input.dropBasisPoints > NOST_BASIS_POINTS_SCALE || input.windowSeconds == 0)
		{
			output.errorCode = EAuctionError::InvalidInput;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetFeeReserveGuardConfig, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		state.mut().feeReserveGuardDropBasisPoints = input.dropBasisPoints;
		state.mut().feeReserveGuardWindowSeconds = input.windowSeconds;
		state.mut().feeReserveBaselineAt.setInvalid();
		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetFeeReserveGuardConfig, output.errorCode, 0, 0);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Manually pauses or resumes every auction interaction, overriding the automatic execution fee reserve guard.
	 * @note Only the configured takeover coordinator or management wallet can call this procedure. Resuming clears the guard window so a stale
	 * baseline cannot immediately retrigger the pause.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetEmergencyPause)
	{
		output.errorCode = EAuctionError::InvalidInput;
		// Manual pause shares the same state as the automatic reserve guard.
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().takeoverCoordinator && qpi.invocator() != state.get().management)
		{
			output.errorCode = EAuctionError::Forbidden;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetEmergencyPause, output.errorCode, 0, 0);
			logProcedureResult(locals.log);
			return;
		}

		if (input.paused)
		{
			state.mut().isEmergencyPaused = 1;
			state.mut().emergencyPausedAt = qpi.now();
		}
		else
		{
			state.mut().isEmergencyPaused = 0;
			state.mut().emergencyPausedAt.setInvalid();
			state.mut().feeReserveBaselineAt.setInvalid();
		}

		output.errorCode = EAuctionError::Success;
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::SetEmergencyPause, output.errorCode, 0, 0);
		logProcedureResult(locals.log);
	}

	/**
	 * @brief Returns the stored state of one auction.
	 * @note The response contains a serializable auction view; access-control containers are returned as fixed arrays with counts.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionByIndex)
	{
		output.found = 0;
		locals.findAuctionInput.auctionIndex = input.auctionIndex;
		CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
		if (!locals.findAuctionOutput.found)
		{
			return;
		}
		locals.auction = locals.findAuctionOutput.auction;

		output.found = 1;
		output.auction.core = locals.auction.core;

		// Hash containers are flattened into arrays because they are not part of the public ABI surface.
		output.auction.requiredAccessAssetCount = 0;
		for (locals.requiredAccessAssetSetIndex = locals.auction.requiredAccessAssets.nextElementIndex(NULL_INDEX);
		     locals.requiredAccessAssetSetIndex != NULL_INDEX;
		     locals.requiredAccessAssetSetIndex = locals.auction.requiredAccessAssets.nextElementIndex(locals.requiredAccessAssetSetIndex))
		{
			locals.requiredAccessAsset.asset = locals.auction.requiredAccessAssets.key(locals.requiredAccessAssetSetIndex);
			locals.requiredAccessAsset.quantity = locals.auction.requiredAccessAssets.value(locals.requiredAccessAssetSetIndex);
			output.auction.requiredAccessAssets.set(output.auction.requiredAccessAssetCount, locals.requiredAccessAsset);
			output.auction.requiredAccessAssetCount = sadd(output.auction.requiredAccessAssetCount, 1ULL);
		}

		output.auction.allowedBidderWalletCount = 0;
		for (locals.allowedBidderWalletSetIndex = locals.auction.allowedBidderWallets.nextElementIndex(NULL_INDEX);
		     locals.allowedBidderWalletSetIndex != NULL_INDEX;
		     locals.allowedBidderWalletSetIndex = locals.auction.allowedBidderWallets.nextElementIndex(locals.allowedBidderWalletSetIndex))
		{
			locals.allowedBidderWallet = locals.auction.allowedBidderWallets.key(locals.allowedBidderWalletSetIndex);
			output.auction.allowedBidderWallets.set(output.auction.allowedBidderWalletCount, locals.allowedBidderWallet);
			output.auction.allowedBidderWalletCount = sadd(output.auction.allowedBidderWalletCount, 1ULL);
		}
	}

	/**
	 * @brief Returns the stored bid state of one wallet in one auction.
	 * @note The response indicates whether a participant record exists for the requested auction and wallet.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionParticipant)
	{
		output.found = 0;
		locals.bestParticipantFound = 0;
		// A wallet can have multiple historical batch bid slots; return the newest matching record.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participants.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex ||
			    locals.participantData.participant != input.participant)
			{
				continue;
			}

			if (!locals.bestParticipantFound || locals.participantData.bidIndex > output.participantData.bidIndex)
			{
				locals.bestParticipantFound = 1;
				locals.bestParticipantSlotIndex = locals.participantSlotIndex;
				output.participantData = locals.participantData;
				output.found = 1;
			}
		}
		// Search archived slots as well because displaced and settled bids are removed from the live array.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participantHistory.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participantHistory.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex ||
			    locals.participantData.participant != input.participant)
			{
				continue;
			}
			if (!locals.bestParticipantFound || locals.participantData.bidIndex > output.participantData.bidIndex)
			{
				locals.bestParticipantFound = 1;
				output.participantData = locals.participantData;
				output.found = 1;
			}
		}
	}

	/**
	 * @brief Returns the remaining post-BEGIN_EPOCH pause before auction interactions resume.
	 * @note This getter exposes the 500-tick launch pause referenced by the auction timing rules.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetTicksBeforeAuctionLaunch)
	{
		output.ticks = 0;

		if (!state.get().isPostBeginEpochPauseArmed)
		{
			return;
		}

		output.ticks = static_cast<uint32>(max<sint64>(static_cast<sint64>(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS) -
		                                                   (static_cast<sint64>(qpi.tick()) - static_cast<sint64>(qpi.initialTick())),
		                                               0));
	}

	/**
	 * @brief Returns the current auction fee configuration stored in contract state.
	 * @note The response includes creation, cancellation, revenue split, and tier-based shareholder fee parameters.
	 */
	PUBLIC_FUNCTION(GetAuctionFees)
	{
		output.privateAuctionFee = state.get().privateAuctionFee;
		output.auctionCancellationFeeBasisPoints = state.get().auctionCancellationFeeBasisPoints;
		output.managementFeeBasisPoints = state.get().managementFeeBasisPoints;
		output.developmentFeeBasisPoints = state.get().developmentFeeBasisPoints;
		output.takeoverCoordinatorFeeBasisPoints = state.get().takeoverCoordinatorFeeBasisPoints;
		output.shareholderDividendBasisPoints = state.get().shareholderDividendBasisPoints;
		output.shareholderFeeBasisPointsTier1 = state.get().shareholderFeeBasisPointsTier1;
		output.shareholderFeeBasisPointsTier2 = state.get().shareholderFeeBasisPointsTier2;
		output.shareholderFeeBasisPointsTier3 = state.get().shareholderFeeBasisPointsTier3;
		output.shareholderFeeBasisPointsTier4 = state.get().shareholderFeeBasisPointsTier4;
		output.publicAuctionCreationFee = state.get().publicAuctionCreationFee;
	}

	/**
	 * @brief Calculates the escrow, accumulated fee, and reward required by Batch Auction bid arithmetic.
	 * @param input Prospective bid quantity and price per asset; zero values are accepted for arithmetic inspection.
	 * @param output Saturating escrow product, small-bid fee, and saturating total reward.
	 * @note Non-zero escrow pays enough fee to reach `NOST_BATCH_BID_FEE_CUTOFF`; escrow at or above the cutoff pays no bid fee.
	 * @note This function does not validate whether `PlaceBid` would accept the bid or mutate contract state.
	 */
	PUBLIC_FUNCTION(CalculateBatchAuctionBidFee) { calculateBatchAuctionBidFee(input.bidQuantity, input.bidAmount, output); }

	/**
	 * @brief Returns the current wallets that receive auction fee transfers.
	 * @note The response exposes the configured management, development, and takeover coordinator addresses.
	 */
	PUBLIC_FUNCTION(GetFeeRecipients)
	{
		output.management = state.get().management;
		output.development = state.get().development;
		output.takeoverCoordinator = state.get().takeoverCoordinator;
	}

	/**
	 * @brief Returns the ring buffer with recently closed auctions.
	 * @note The buffer stores auction identifiers for both finalized and cancelled auctions.
	 * @note When `totalEntries` exceeds `NOST_AUCTION_HISTORY_NUM`, older entries are overwritten in ring-buffer order.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetClosedAuctionHistory)
	{
		// Preserve physical ring positions to keep the existing auctionIndices ABI stable for clients.
		for (locals.historyIndex = 0; locals.historyIndex < state.get().closedAuctionHistory.capacity(); ++locals.historyIndex)
		{
			locals.auction = state.get().closedAuctionHistory.get(locals.historyIndex);
			if (locals.auction.core.status != EAuctionStatus::None)
			{
				output.auctionIndices.set(locals.historyIndex, locals.auction.core.auctionIndex);
			}
		}
		output.totalEntries = state.get().closedAuctionHistoryCounter;
	}

	/**
	 * @brief Returns whether the temporary fee override routes every fee to development.
	 */
	PUBLIC_FUNCTION(GetRouteAllFeesToDevelopment) { output.enabled = state.get().routeAllFeesToDevelopment; }

	/**
	 * @brief Returns the aggregate shared fee amount awaiting `END_EPOCH` settlement.
	 * @note The legacy field name is retained for ABI compatibility.
	 */
	PUBLIC_FUNCTION(GetPendingServiceFeePool) { output.pendingServiceFeePool = getNostromoFeePoolTotal(state.get().feePool); }

	/** @brief Returns every accumulator in the shared Nostromo fee pool. */
	PUBLIC_FUNCTION(GetNostromoFeePool)
	{
		output.feePool = state.get().feePool;
		output.totalAmount = getNostromoFeePoolTotal(state.get().feePool);
	}

	/** @brief Returns the QU obligation currently registered for one wallet. */
	PUBLIC_FUNCTION(GetPendingPayout)
	{
		output.amount = 0;
		state.get().pendingQuPayouts.get(input.account, output.amount);
	}

	/**
	 * @brief Returns the current state of the execution fee reserve guard, including a live reserve reading.
	 */
	PUBLIC_FUNCTION(GetFeeReserveGuardState)
	{
		output.currentFeeReserve = qpi.queryFeeReserve(SELF_INDEX);
		output.feeReserveBaseline = state.get().feeReserveBaseline;
		output.feeReserveBaselineAt = state.get().feeReserveBaselineAt;
		output.emergencyPausedAt = state.get().emergencyPausedAt;
		output.dropBasisPoints = state.get().feeReserveGuardDropBasisPoints;
		output.windowSeconds = state.get().feeReserveGuardWindowSeconds;
		output.isEmergencyPaused = state.get().isEmergencyPaused;
	}

	/**
	 * @brief Returns aggregate auction, participant, fee, and pause counters.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetContractStats)
	{
		output.stats.totalAuctionsCreated = state.get().totalAuctionsCreated;
		output.stats.closedAuctionHistoryCounter = state.get().closedAuctionHistoryCounter;
		output.stats.auctionShareholderDividendPool = state.get().auctionShareholderDividendPool;
		output.stats.pendingServiceFeePool = getNostromoFeePoolTotal(state.get().feePool);
		output.stats.totalPendingQuPayouts = state.get().totalPendingQuPayouts;
		output.stats.retainedClosedAuctionCount = min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity());
		output.stats.retainedParticipantHistoryCount = min(state.get().participantHistoryCounter, state.get().participantHistory.capacity());
		output.stats.finalizedAuctionCount = state.get().totalFinalizedAuctions;
		output.stats.cancelledAuctionCount = state.get().totalCancelledAuctions;
		output.stats.qxTransferFee = state.get().qxTransferFee;
		output.stats.routeAllFeesToDevelopment = state.get().routeAllFeesToDevelopment;
		output.stats.isAuctionTimerPaused = state.get().isAuctionTimerPaused;
		output.stats.isPostBeginEpochPauseArmed = state.get().isPostBeginEpochPauseArmed;
		output.stats.isEmergencyPaused = state.get().isEmergencyPaused;

		// Stats scan fixed storage because participant slots and auction records are not separately indexed by status.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participants.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantSlotIndex);
			if (locals.participantData.isUsed)
			{
				output.stats.participantCount = sadd(output.stats.participantCount, 1ULL);
			}
		}

		for (locals.auctionElementIndex = state.get().auctionList.nextElementIndex(NULL_INDEX); locals.auctionElementIndex != NULL_INDEX;
		     locals.auctionElementIndex = state.get().auctionList.nextElementIndex(locals.auctionElementIndex))
		{
			locals.auction = state.get().auctionList.value(locals.auctionElementIndex);
			switch (locals.auction.core.status)
			{
				case EAuctionStatus::Active: output.stats.activeAuctionCount = sadd(output.stats.activeAuctionCount, 1ULL); break;
				case EAuctionStatus::PendingSellerDecision:
					output.stats.pendingSellerDecisionAuctionCount = sadd(output.stats.pendingSellerDecisionAuctionCount, 1ULL);
					break;
				default: break;
			}
		}
	}

	/**
	 * @brief Returns a page of auction summaries ordered by creation index.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionSummaries)
	{
		// Live and archived records are disjoint, so the retained total does not require an ordered scan.
		output.totalCount =
		    sadd(state.get().auctionList.population(), min(state.get().closedAuctionHistoryCounter, state.get().closedAuctionHistory.capacity()));
		output.returnedCount = 0;
		locals.boundedLimit = min(input.limit, NOST_AUCTION_GETTER_PAGE_SIZE);
		if (locals.boundedLimit == 0 || input.offset >= output.totalCount)
		{
			return;
		}
		locals.scannedAuctionCount = 0;
		locals.selectNextAuctionInput.hasAfterAuctionIndex = 0;
		locals.selectNextAuctionInput.includeClosedAuctions = 1;
		locals.selectNextAuctionInput.filterBySeller = 0;
		// Cursor selection reconstructs creation order across unordered live storage and the closed-history ring.
		while (output.returnedCount < locals.boundedLimit && locals.scannedAuctionCount < output.totalCount)
		{
			CALL(SelectNextRetainedAuction, locals.selectNextAuctionInput, locals.selectNextAuctionOutput);
			if (!locals.selectNextAuctionOutput.found)
			{
				break;
			}
			locals.auction = locals.selectNextAuctionOutput.auction;
			// Skip only the requested prefix; the exact total is already available without scanning the remainder.
			if (locals.scannedAuctionCount >= input.offset)
			{
				fillAuctionSummary(locals.auction, locals.auctionSummary);
				output.auctions.set(output.returnedCount, locals.auctionSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			locals.scannedAuctionCount = sadd(locals.scannedAuctionCount, 1ULL);
			locals.selectNextAuctionInput.afterAuctionIndex = locals.auction.core.auctionIndex;
			locals.selectNextAuctionInput.hasAfterAuctionIndex = 1;
		}
	}

	/**
	 * @brief Returns a page of active or pending-seller-decision auction indices.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetActiveAuctionIndices)
	{
		// Invariant: terminal auctions are archived and removed, so every live-map entry is active or awaiting a seller decision.
		output.totalCount = state.get().auctionList.population();
		output.returnedCount = 0;
		locals.boundedLimit = min(input.limit, NOST_AUCTION_GETTER_PAGE_SIZE);
		if (locals.boundedLimit == 0 || input.offset >= output.totalCount)
		{
			return;
		}

		locals.scannedAuctionCount = 0;
		locals.selectNextAuctionInput.hasAfterAuctionIndex = 0;
		locals.selectNextAuctionInput.includeClosedAuctions = 0;
		locals.selectNextAuctionInput.filterBySeller = 0;
		// Select only the requested live-map prefix and page; closed history cannot contain active auctions.
		while (output.returnedCount < locals.boundedLimit && locals.scannedAuctionCount < output.totalCount)
		{
			CALL(SelectNextRetainedAuction, locals.selectNextAuctionInput, locals.selectNextAuctionOutput);
			if (!locals.selectNextAuctionOutput.found)
			{
				break;
			}
			locals.selectNextAuctionInput.afterAuctionIndex = locals.selectNextAuctionOutput.auction.core.auctionIndex;
			locals.selectNextAuctionInput.hasAfterAuctionIndex = 1;
			if (locals.scannedAuctionCount >= input.offset)
			{
				output.auctionIndices.set(output.returnedCount, locals.selectNextAuctionOutput.auction.core.auctionIndex);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			locals.scannedAuctionCount = sadd(locals.scannedAuctionCount, 1ULL);
		}
	}

	/**
	 * @brief Returns a page of auction summaries created by a seller.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionsBySeller)
	{
		output.returnedCount = 0;
		locals.countAuctionsInput.seller = input.seller;
		CALL(CountRetainedAuctionsBySeller, locals.countAuctionsInput, locals.countAuctionsOutput);
		output.totalCount = locals.countAuctionsOutput.count;
		locals.boundedLimit = min(input.limit, NOST_AUCTION_GETTER_PAGE_SIZE);
		if (locals.boundedLimit == 0 || input.offset >= output.totalCount)
		{
			return;
		}

		locals.scannedAuctionCount = 0;
		locals.selectNextAuctionInput.seller = input.seller;
		locals.selectNextAuctionInput.hasAfterAuctionIndex = 0;
		locals.selectNextAuctionInput.includeClosedAuctions = 1;
		locals.selectNextAuctionInput.filterBySeller = 1;
		// The selector skips other sellers, so only the requested seller's prefix and page are ordered.
		while (output.returnedCount < locals.boundedLimit && locals.scannedAuctionCount < output.totalCount)
		{
			CALL(SelectNextRetainedAuction, locals.selectNextAuctionInput, locals.selectNextAuctionOutput);
			if (!locals.selectNextAuctionOutput.found)
			{
				break;
			}
			locals.auction = locals.selectNextAuctionOutput.auction;
			locals.selectNextAuctionInput.afterAuctionIndex = locals.auction.core.auctionIndex;
			locals.selectNextAuctionInput.hasAfterAuctionIndex = 1;
			if (locals.scannedAuctionCount >= input.offset)
			{
				fillAuctionSummary(locals.auction, locals.auctionSummary);
				output.auctions.set(output.returnedCount, locals.auctionSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			locals.scannedAuctionCount = sadd(locals.scannedAuctionCount, 1ULL);
		}
	}

	/**
	 * @brief Looks up the first auction matching a metadata CID.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionByMetadataCid)
	{
		output.found = 0;
		output.auctionIndex = 0;
		locals.findAuctionInput.metadataIpfsCid = input.metadataIpfsCid;
		CALL(FindFirstRetainedAuctionByMetadataCid, locals.findAuctionInput, locals.findAuctionOutput);
		// The helper compares all retained candidates and returns the smallest matching creation index.
		if (!locals.findAuctionOutput.found)
		{
			return;
		}
		output.found = 1;
		output.auctionIndex = locals.findAuctionOutput.auction.core.auctionIndex;
		fillAuctionSummary(locals.findAuctionOutput.auction, output.auction);
	}

	/**
	 * @brief Returns auction summaries for a batch of requested indices.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionSummariesByIndexBatch)
	{
		output.returnedCount = 0;
		locals.boundedLimit = min(input.count, NOST_AUCTION_GETTER_PAGE_SIZE);
		// Preserve input positions so callers can correlate each requested index with its found flag.
		for (locals.requestedIndex = 0; locals.requestedIndex < locals.boundedLimit; ++locals.requestedIndex)
		{
			locals.auctionIndex = input.auctionIndices.get(locals.requestedIndex);
			locals.findAuctionInput.auctionIndex = locals.auctionIndex;
			CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
			if (locals.findAuctionOutput.found)
			{
				locals.auction = locals.findAuctionOutput.auction;
				fillAuctionSummary(locals.auction, locals.auctionSummary);
				output.auctions.set(locals.requestedIndex, locals.auctionSummary);
				output.found.set(locals.requestedIndex, 1);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
		}
	}

	/**
	 * @brief Returns a page of participants for one auction.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionParticipants)
	{
		output.totalCount = 0;
		output.returnedCount = 0;
		locals.boundedLimit = min(input.limit, NOST_AUCTION_GETTER_PAGE_SIZE);
		// Participant storage is global, so auction participant pages are built by scanning all slots.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participants.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}
			if (output.totalCount >= input.offset && output.returnedCount < locals.boundedLimit)
			{
				fillParticipantSummary(locals.participantData, locals.participantSummary);
				output.participants.set(output.returnedCount, locals.participantSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			output.totalCount = sadd(output.totalCount, 1ULL);
		}
		// Append archived records after live records so an offset spans both storage tiers deterministically.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participantHistory.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participantHistory.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.auctionIndex != input.auctionIndex)
			{
				continue;
			}
			if (output.totalCount >= input.offset && output.returnedCount < locals.boundedLimit)
			{
				fillParticipantSummary(locals.participantData, locals.participantSummary);
				output.participants.set(output.returnedCount, locals.participantSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			output.totalCount = sadd(output.totalCount, 1ULL);
		}
	}

	/**
	 * @brief Returns a page of historical auction participations for one wallet.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetUserParticipations)
	{
		output.totalCount = 0;
		output.returnedCount = 0;
		locals.boundedLimit = min(input.limit, NOST_AUCTION_GETTER_PAGE_SIZE);
		// User participation history includes inactive records so settled and displaced bids remain visible.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participants.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participants.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.participant != input.participant)
			{
				continue;
			}
			if (output.totalCount >= input.offset && output.returnedCount < locals.boundedLimit)
			{
				fillUserParticipationSummary(locals.participantData.auctionIndex, locals.participantData, locals.userParticipationSummary);
				output.participations.set(output.returnedCount, locals.userParticipationSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			output.totalCount = sadd(output.totalCount, 1ULL);
		}
		// Continue the same page over archived bids after accounting for matching live entries.
		for (locals.participantSlotIndex = 0; locals.participantSlotIndex < state.get().participantHistory.capacity(); ++locals.participantSlotIndex)
		{
			locals.participantData = state.get().participantHistory.get(locals.participantSlotIndex);
			if (!locals.participantData.isUsed || locals.participantData.participant != input.participant)
			{
				continue;
			}
			if (output.totalCount >= input.offset && output.returnedCount < locals.boundedLimit)
			{
				fillUserParticipationSummary(locals.participantData.auctionIndex, locals.participantData, locals.userParticipationSummary);
				output.participations.set(output.returnedCount, locals.userParticipationSummary);
				output.returnedCount = sadd(output.returnedCount, 1ULL);
			}
			output.totalCount = sadd(output.totalCount, 1ULL);
		}
	}

	/**
	 * @brief Returns the most recently created auction index when one exists.
	 */
	PUBLIC_FUNCTION(GetLatestAuctionIndex)
	{
		output.found = state.get().totalAuctionsCreated > 0;
		output.auctionIndex = output.found ? state.get().totalAuctionsCreated - 1 : 0;
	}

	/**
	 * @brief Counts auctions created by a seller.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionCountBySeller)
	{
		locals.countAuctionsInput.seller = input.seller;
		CALL(CountRetainedAuctionsBySeller, locals.countAuctionsInput, locals.countAuctionsOutput);
		output.count = locals.countAuctionsOutput.count;
	}

	/**
	 * @brief Returns immutable creation-time fields for an auction.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAuctionAtCreationSnapshot)
	{
		output.found = 0;
		locals.findAuctionInput.auctionIndex = input.auctionIndex;
		CALL(FindAuction, locals.findAuctionInput, locals.findAuctionOutput);
		if (!locals.findAuctionOutput.found)
		{
			return;
		}
		locals.auction = locals.findAuctionOutput.auction;
		output.found = 1;
		output.seller = locals.auction.core.seller;
		output.createdAt = locals.auction.core.createdAt;
		output.auctionIndex = locals.auction.core.auctionIndex;
		output.quantityForSale = locals.auction.core.quantityForSale;
		output.initialPrice = locals.auction.core.initialPrice;
		output.salePrice = locals.auction.core.salePrice;
		output.minimumBidIncrement = locals.auction.core.minimumBidIncrement;
		output.buyNowPrice = locals.auction.core.buyNowPrice;
		output.auctionDurationSeconds = locals.auction.core.auctionDurationSeconds;
		output.type = static_cast<uint8>(locals.auction.core.type);
		output.visibility = static_cast<uint8>(locals.auction.core.visibility);
	}

	/**
	 * @brief Returns current read-only guidance for the next valid Batch Auction bid.
	 * @note `found` also covers closed auctions while their snapshots remain in retained history.
	 * @note `PlaceBid` re-runs the same availability validation before accepting a bid.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetBatchAuctionBidAvailability)
	{
		locals.computeBatchBidAvailabilityInput.auctionIndex = input.auctionIndex;
		locals.computeBatchBidAvailabilityInput.bidAmount = 0;
		CALL(ComputeBatchBidAvailability, locals.computeBatchBidAvailabilityInput, output);
		// Live auctions are fully classified by the availability helper, including non-Batch auctions.
		if (output.found)
		{
			return;
		}

		// A retained closed auction still exists for lookup purposes, but can never accept another bid.
		locals.isClosedAuctionRetainedInput.auctionIndex = input.auctionIndex;
		CALL(IsClosedAuctionRetained, locals.isClosedAuctionRetainedInput, locals.isClosedAuctionRetainedOutput);
		output.found = locals.isClosedAuctionRetainedOutput.found;
	}

	/**
	 * @brief Transfers share management rights for an asset position to another managing contract.
	 * @note The caller must currently possess at least the requested number of shares.
	 * @note The caller must send the destination contract's required transfer fee as invocation reward. This contract cannot query that
	 * fee before calling `releaseShares`, so callers must resolve it from `newManagingContractIndex`.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		locals.reward = qpi.invocationReward();
		locals.refundAmount = locals.reward;
		locals.success = false;
		output.transferredNumberOfShares = 0;
		output.errorCode = EAuctionError::InvalidInput;

		// Emergency pause blocks cross-contract share release and returns the caller's fee budget.
		if (state.get().isEmergencyPaused)
		{
			if (locals.refundAmount > 0)
			{
				qpi.transfer(qpi.invocator(), locals.refundAmount);
			}
			output.errorCode = EAuctionError::AuctionPaused;
			setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::TransferShareManagementRights, output.errorCode, 0,
			                     output.transferredNumberOfShares);
			logProcedureResult(locals.log);
			return;
		}

		// `releaseShares` consumes only the destination transfer fee; any unused reward is refunded below.
		if (input.numberOfShares > 0 && qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(),
		                                                            SELF_INDEX, SELF_INDEX) >= input.numberOfShares)
		{
			locals.result = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
			                                  input.newManagingContractIndex, locals.reward);
			if (locals.result != INVALID_AMOUNT && locals.result >= 0)
			{
				locals.success = true;
				locals.refundAmount = locals.reward - locals.result;
			}
		}

		if (locals.success)
		{
			output.transferredNumberOfShares = input.numberOfShares;
			output.errorCode = EAuctionError::Success;
		}

		if (locals.refundAmount > 0)
		{
			qpi.transfer(qpi.invocator(), locals.refundAmount);
		}
		setProcedureLogInput(locals.log, qpi.invocator(), EProcedureId::TransferShareManagementRights, output.errorCode, 0,
		                     output.transferredNumberOfShares);

		logProcedureResult(locals.log);
	}

protected:
	/**
	 * @brief Emits a procedure log as success or error based on its error code.
	 */
	static void logProcedureResult(const NostromoProcedureLog& log)
	{
		if (log.errorCode == static_cast<uint32>(EAuctionError::Success))
		{
			LOG_INFO(log);
		}
		else
		{
			LOG_ERROR(log);
		}
	}

	/**
	 * @brief Fills the common procedure log payload.
	 */
	static void setProcedureLogInput(NostromoProcedureLog& log, const id& actor, EProcedureId procedure, EAuctionError errorCode, uint64 auctionIndex,
	                                 sint64 amount)
	{
		log.contractIndex = SELF_INDEX;
		log.procedure = static_cast<uint8>(procedure);
		log.errorCode = static_cast<uint32>(errorCode);
		log.auctionIndex = auctionIndex;
		log.actor = actor;
		log.amount = amount;
		log._terminator = 0;
	}

	/**
	 * @brief Copies persisted auction data into a compact summary.
	 */
	static void fillAuctionSummary(const AuctionData& auction, AuctionSummary& summary)
	{
		summary.metadataIpfsCid = auction.core.metadataIpfsCid;
		summary.seller = auction.core.seller;
		summary.highestBidder = auction.core.highestBidder;
		summary.createdAt = auction.core.createdAt;
		summary.settledAt = auction.core.settledAt;
		summary.auctionIndex = auction.core.auctionIndex;
		summary.quantityForSale = auction.core.quantityForSale;
		summary.allocatedQuantity = auction.core.allocatedQuantity;
		summary.initialPrice = auction.core.initialPrice;
		summary.salePrice = auction.core.salePrice;
		summary.buyNowPrice = auction.core.buyNowPrice;
		summary.highestBidPrice = auction.core.highestBidPrice;
		summary.highestBidQuantity = auction.core.highestBidQuantity;
		summary.highestBidAmount = auction.core.highestBidAmount;
		summary.type = static_cast<uint8>(auction.core.type);
		summary.visibility = static_cast<uint8>(auction.core.visibility);
		summary.status = static_cast<uint8>(auction.core.status);
	}

	/**
	 * @brief Copies participant storage data into an auction participant summary.
	 */
	static void fillParticipantSummary(const AuctionParticipantData& participantData, ParticipantSummary& summary)
	{
		summary.participant = participantData.participant;
		summary.lastBidTime = participantData.lastBidTime;
		summary.bidAmount = participantData.bidAmount;
		summary.escrowedAmount = participantData.escrowedAmount;
		summary.requestedQuantity = participantData.requestedQuantity;
		summary.allocatedQuantity = participantData.allocatedQuantity;
		summary.isWinningBid = participantData.isWinningBid;
	}

	/**
	 * @brief Copies participant storage data into a user participation summary.
	 */
	static void fillUserParticipationSummary(uint64 auctionIndex, const AuctionParticipantData& participantData, UserParticipationSummary& summary)
	{
		summary.participant = participantData.participant;
		summary.lastBidTime = participantData.lastBidTime;
		summary.auctionIndex = auctionIndex;
		summary.bidAmount = participantData.bidAmount;
		summary.escrowedAmount = participantData.escrowedAmount;
		summary.requestedQuantity = participantData.requestedQuantity;
		summary.allocatedQuantity = participantData.allocatedQuantity;
		summary.isWinningBid = participantData.isWinningBid;
	}

	/**
	 * @brief Returns the smaller of two values.
	 */
	template<typename T>
	static constexpr T min(const T& a, const T& b)
	{
		return (a < b) ? a : b;
	}
	/**
	 * @brief Returns the larger of two values.
	 */
	template<typename T>
	static constexpr T max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	/**
	 * @brief Resolves Batch Auction quantity invariants from creation input.
	 */
	static bool resolveBatchAuctionCreateParams(uint64 lotItemCount, uint64 totalEscrowQuantity, uint64 minimumPurchaseQuantity,
	                                            uint64& quantityForSale, uint64& resolvedMinimumPurchaseQuantity, uint64 buyNowPrice)
	{
		quantityForSale = 0;
		resolvedMinimumPurchaseQuantity = 0;
		if (lotItemCount != NOST_BATCH_AUCTION_LOT_ITEM_NUM || totalEscrowQuantity == 0 || minimumPurchaseQuantity == 0 ||
		    minimumPurchaseQuantity > totalEscrowQuantity || buyNowPrice != 0)
		{
			return false;
		}
		quantityForSale = totalEscrowQuantity;
		resolvedMinimumPurchaseQuantity = minimumPurchaseQuantity;
		return true;
	}

	/**
	 * @brief Resolves Standard Auction quantity and price invariants from creation input.
	 */
	static bool resolveStandardAuctionCreateParams(uint64 minimumBidIncrement, uint64& quantityForSale, uint64& resolvedMinimumPurchaseQuantity,
	                                               uint64 buyNowPrice, uint64 initialPrice, uint64 salePrice)
	{
		quantityForSale = 0;
		resolvedMinimumPurchaseQuantity = 0;
		if (initialPrice < NOST_STANDARD_MIN_PRICE || salePrice < NOST_STANDARD_MIN_PRICE || minimumBidIncrement < NOST_STANDARD_MIN_BID_INCREMENT)
		{
			return false;
		}

		if (initialPrice > salePrice)
		{
			return false;
		}

		if (buyNowPrice > 0 && (buyNowPrice < initialPrice || buyNowPrice < salePrice))
		{

			return false;
		}

		quantityForSale = NOST_STANDARD_AUCTION_LOT_COUNT;
		resolvedMinimumPurchaseQuantity = 0;
		return true;
	}

	/**
	 * @brief Validates that private auctions use at least one supported access mode.
	 */
	constexpr static bool validatePrivateAuctionAccess(EAuctionVisibility visibility, uint64 requiredAccessAssetCount, uint64 allowedWalletCount)
	{
		return visibility != EAuctionVisibility::Private || requiredAccessAssetCount > 0 || allowedWalletCount > 0;
	}

	/**
	 * @brief Validates governance fee percentages and fixed service fees.
	 */
	constexpr static bool isValidAuctionFeeConfiguration(sint64 privateAuctionFee, sint64 publicAuctionCreationFee,
	                                                     uint64 auctionCancellationFeeBasisPoints, uint64 managementFeeBasisPoints,
	                                                     uint64 developmentFeeBasisPoints, uint64 takeoverCoordinatorFeeBasisPoints,
	                                                     uint64 shareholderDividendBasisPoints, uint64 shareholderFeeBasisPointsTier1,
	                                                     uint64 shareholderFeeBasisPointsTier2, uint64 shareholderFeeBasisPointsTier3,
	                                                     uint64 shareholderFeeBasisPointsTier4)
	{
		return privateAuctionFee >= 0 && publicAuctionCreationFee >= 0 && auctionCancellationFeeBasisPoints <= NOST_BASIS_POINTS_SCALE &&
		       managementFeeBasisPoints <= NOST_BASIS_POINTS_SCALE && developmentFeeBasisPoints <= NOST_BASIS_POINTS_SCALE &&
		       takeoverCoordinatorFeeBasisPoints <= NOST_BASIS_POINTS_SCALE && shareholderDividendBasisPoints <= NOST_BASIS_POINTS_SCALE &&
		       shareholderFeeBasisPointsTier1 <= NOST_BASIS_POINTS_SCALE && shareholderFeeBasisPointsTier2 <= NOST_BASIS_POINTS_SCALE &&
		       shareholderFeeBasisPointsTier3 <= NOST_BASIS_POINTS_SCALE && shareholderFeeBasisPointsTier4 <= NOST_BASIS_POINTS_SCALE &&
		       (shareholderFeeBasisPointsTier1 + managementFeeBasisPoints + developmentFeeBasisPoints + takeoverCoordinatorFeeBasisPoints) <=
		           NOST_BASIS_POINTS_SCALE &&
		       (shareholderFeeBasisPointsTier2 + managementFeeBasisPoints + developmentFeeBasisPoints + takeoverCoordinatorFeeBasisPoints) <=
		           NOST_BASIS_POINTS_SCALE &&
		       (shareholderFeeBasisPointsTier3 + managementFeeBasisPoints + developmentFeeBasisPoints + takeoverCoordinatorFeeBasisPoints) <=
		           NOST_BASIS_POINTS_SCALE &&
		       (shareholderFeeBasisPointsTier4 + managementFeeBasisPoints + developmentFeeBasisPoints + takeoverCoordinatorFeeBasisPoints) <=
		           NOST_BASIS_POINTS_SCALE;
	}

	/**
	 * @brief Selects the shareholder fee tier for a gross auction amount.
	 */
	static uint64 getAuctionShareholderFeeBasisPoints(uint64 grossAmount, const StateData& state)
	{
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_1)
		{
			return state.shareholderFeeBasisPointsTier1;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_2)
		{
			return state.shareholderFeeBasisPointsTier2;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_3)
		{
			return state.shareholderFeeBasisPointsTier3;
		}
		return state.shareholderFeeBasisPointsTier4;
	}

	/**
	 * @brief Selects the shareholder fee tier from contract state.
	 */
	static uint64 getAuctionShareholderFeeBasisPoints(uint64 grossAmount, const ContractState<StateData, CONTRACT_INDEX>& state)
	{
		return getAuctionShareholderFeeBasisPoints(grossAmount, state.get());
	}

	/** @brief Returns the zero-based shareholder fee tier selected by an auction gross amount. */
	constexpr static uint64 getAuctionShareholderFeeTierIndex(uint64 grossAmount)
	{
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_1)
		{
			return 0;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_2)
		{
			return 1;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_3)
		{
			return 2;
		}
		return 3;
	}

	/** @brief Returns the saturating aggregate of every unsettled fee-pool accumulator. */
	static uint64 getNostromoFeePoolTotal(const NostromoFeePool& feePool)
	{
		return sadd(sadd(sadd(feePool.shareholderDividendTier1Amount, feePool.shareholderDividendTier2Amount),
		                 sadd(feePool.shareholderDividendTier3Amount, feePool.shareholderDividendTier4Amount)),
		            sadd(sadd(feePool.commonServiceFeeAmount, feePool.shareholderDividendAmount),
		                 sadd(sadd(feePool.managementAmount, feePool.developmentAmount), feePool.takeoverCoordinatorAmount)));
	}

	/**
	 * @brief Computes `floor(amount * basisPoints / 10000)` without overflowing the intermediate product.
	 */
	static uint64 calculateBasisPointAmount(uint64 amount, uint64 basisPoints)
	{
		return sadd(smul(div<uint64>(amount, NOST_BASIS_POINTS_SCALE), basisPoints),
		            div<uint64>(smul(mod(amount, NOST_BASIS_POINTS_SCALE), basisPoints), NOST_BASIS_POINTS_SCALE));
	}

	/**
	 * @brief Computes the exact auction fee split without performing transfers.
	 * @note Keep this helper pure so tests can reuse the same arithmetic as `DistributeAuctionRevenue`.
	 */
	static void calculateAuctionRevenueBreakdown(uint64 grossAmount, const ContractState<StateData, CONTRACT_INDEX>& state,
	                                             AuctionRevenueBreakdown& output)
	{
		output.sellerPayout = grossAmount;
		output.shareholderFeeBasisPoints = getAuctionShareholderFeeBasisPoints(grossAmount, state);
		output.shareholderFeeAmount = calculateBasisPointAmount(grossAmount, output.shareholderFeeBasisPoints);
		output.shareholderDividendAmount = calculateBasisPointAmount(output.shareholderFeeAmount, state.get().shareholderDividendBasisPoints);
		output.managementFeeAmount = calculateBasisPointAmount(grossAmount, state.get().managementFeeBasisPoints);
		output.developmentFeeAmount = calculateBasisPointAmount(grossAmount, state.get().developmentFeeBasisPoints);
		output.takeoverCoordinatorBaseAmount = calculateBasisPointAmount(grossAmount, state.get().takeoverCoordinatorFeeBasisPoints);
		output.takeoverCoordinatorFeeAmount = output.takeoverCoordinatorBaseAmount + (output.shareholderFeeAmount - output.shareholderDividendAmount);
		output.sellerPayout = grossAmount - output.shareholderFeeAmount - output.managementFeeAmount - output.developmentFeeAmount -
		                      output.takeoverCoordinatorBaseAmount;
	}

	/**
	 * @brief Computes the exact service-fee split without performing transfers.
	 * @note Keep this helper pure so tests can reuse the same arithmetic as `DistributeNostromoFeePool`.
	 */
	static void calculateAuctionServiceFeeBreakdown(uint64 feeAmount, AuctionServiceFeeBreakdown& output)
	{
		output.shareholderDividendAmount = calculateBasisPointAmount(feeAmount, NOST_AUCTION_SERVICE_FEE_SHAREHOLDER_BP);
		output.managementFeeAmount = calculateBasisPointAmount(feeAmount, NOST_AUCTION_SERVICE_FEE_MANAGEMENT_BP);
		output.developmentFeeAmount = calculateBasisPointAmount(feeAmount, NOST_AUCTION_SERVICE_FEE_DEVELOPMENT_BP);
		output.takeoverCoordinatorFeeAmount = calculateBasisPointAmount(feeAmount, NOST_AUCTION_SERVICE_FEE_TAKEOVER_COORDINATOR_BP);
		// Shareholders receive the rounding remainder so the entire collected fee is distributed on-chain.
		output.shareholderDividendAmount =
		    sadd(output.shareholderDividendAmount, feeAmount - output.shareholderDividendAmount - output.managementFeeAmount -
		                                               output.developmentFeeAmount - output.takeoverCoordinatorFeeAmount);
	}

	/**
	 * @brief Computes escrow, bid fee, and required reward for a Batch Auction bid.
	 */
	static void calculateBatchAuctionBidFee(uint64 bidQuantity, uint64 bidAmount, CalculateBatchAuctionBidFee_output& output)
	{
		output.escrowAmount = smul(bidQuantity, bidAmount);
		if (output.escrowAmount == 0)
		{
			output.fee = 0;
			output.requiredReward = 0;
			return;
		}

		output.fee = output.escrowAmount <= NOST_BATCH_BID_FEE_CUTOFF ? NOST_BATCH_BID_FEE_CUTOFF - output.escrowAmount : 0;
		output.requiredReward = sadd(output.escrowAmount, output.fee);
	}

	/**
	 * @brief Returns the service fee required to create an auction.
	 */
	static sint64 getCreateAuctionFee(EAuctionVisibility visibility, const ContractState<StateData, CONTRACT_INDEX>& state)
	{
		switch (visibility)
		{
			case EAuctionVisibility::Public: return state.get().publicAuctionCreationFee; break;
			case EAuctionVisibility::Private: return state.get().privateAuctionFee; break;
			default: break;
		}
		return 0;
	}

	/**
	 * @brief Returns whether an auction type is accepted by the contract.
	 */
	static bool isSupportedAuctionType(EAuctionType auctionType)
	{
		return auctionType == EAuctionType::Batch || auctionType == EAuctionType::Standard;
	}

	/**
	 * @brief Returns whether an auction visibility is accepted by the contract.
	 */
	static bool isSupportedAuctionVisibility(EAuctionVisibility visibility)
	{
		return visibility == EAuctionVisibility::Public || visibility == EAuctionVisibility::Private;
	}

	/**
	 * @brief Returns whether an asset entry is empty.
	 */
	static bool isZeroAsset(const Asset& asset) { return asset.assetName == 0 && isZero(asset.issuer); }

	/** @brief Returns whether the runtime fee override routes every auction fee to the development wallet. */
	static bool routeAllFeesToDevelopment(const QPI::ContractState<StateData, CONTRACT_INDEX>& state)
	{
		return state.get().routeAllFeesToDevelopment;
	}

	/**
	 * @brief Packs year, month, and day into the contract date-stamp format.
	 */
	static void makeDateStamp(uint8 year, uint8 month, uint8 day, uint32& res)
	{
		res = static_cast<uint32>(year << NOST_DATE_STAMP_YEAR_SHIFT | month << NOST_DATE_STAMP_MONTH_SHIFT | day);
	}

	/**
	 * @brief Expands an accumulated pause window to include a candidate window.
	 */
	static void accumulatePauseWindow(uint8& hasPauseWindow, DateAndTime& pauseStartedAt, DateAndTime& pauseEndsAt,
	                                  const DateAndTime& candidatePauseStartedAt, const DateAndTime& candidatePauseEndsAt)
	{
		if (!hasPauseWindow)
		{
			hasPauseWindow = 1;
			pauseStartedAt = candidatePauseStartedAt;
			pauseEndsAt = candidatePauseEndsAt;
			return;
		}

		if (candidatePauseStartedAt < pauseStartedAt)
		{
			pauseStartedAt = candidatePauseStartedAt;
		}
		if (candidatePauseEndsAt > pauseEndsAt)
		{
			pauseEndsAt = candidatePauseEndsAt;
		}
	}

	/**
	 * @brief Compares two Nostromo timestamps.
	 * @param a Left-hand date-time.
	 * @param b Right-hand date-time.
	 * @return `-1` if `a < b`, `0` if `a == b`, `1` if `a > b`.
	 */
	static sint32 dateCompare(const DateAndTime& a, const DateAndTime& b)
	{
		if (a < b)
		{
			return -1;
		}
		if (a > b)
		{
			return 1;
		}
		return 0;
	}

	/**
	 * @brief Computes the difference in seconds between two `DateAndTime` values.
	 * @param a Start date-time.
	 * @param b End date-time.
	 * @param res Output difference in seconds, or `0` when `A >= B`.
	 */
	static void diffDateInSecond(const DateAndTime& a, const DateAndTime& b, uint64& res)
	{
		if (a >= b)
		{
			res = 0;
			return;
		}
		res = div<uint64>(a.durationMicrosec(b), NOST_MICROSECONDS_PER_SECOND);
	}
};
