using namespace QPI;

/** @brief Maximum entries in compact native multi-token requests and responses. */
constexpr uint32 QNFT_TOKEN_BATCH_CAPACITY = 16;
/** @brief Full metadata must fit QPI's procedure input limit. */
constexpr uint32 QNFT_MINT_BATCH_CAPACITY = 2;
/** @brief First ASCII character of every native QNFT asset name. */
constexpr uint64 QNFT_ASSET_NAME_PREFIX = Ch::N;
/** @brief Number of base-36 digits encoding the item ID after the prefix. */
constexpr uint32 QNFT_ASSET_NAME_DIGITS = 6;
/** @brief Number of decimal characters in the shared asset-name alphabet. */
constexpr uint32 QNFT_ASSET_NAME_DECIMAL_DIGITS = Ch::_9 - Ch::_0 + 1;
/** @brief Number of numeric and uppercase-letter symbols in the asset-name alphabet. */
constexpr uint32 QNFT_ASSET_NAME_RADIX = QNFT_ASSET_NAME_DECIMAL_DIGITS + Ch::Z - Ch::A + 1;
/** @brief Width of one packed asset-name character, in bits. */
constexpr uint32 QNFT_ASSET_NAME_BYTE_BITS = 8;
/** @brief Mask extracting one ASCII character from the packed asset name. */
constexpr uint64 QNFT_ASSET_NAME_BYTE_MASK = (1ULL << QNFT_ASSET_NAME_BYTE_BITS) - 1;
/** @brief Fixed metadata URI capacity including the zero terminator, in bytes. */
constexpr uint32 QNFT_METADATA_URI_CAPACITY = 256;
/** @brief First permitted non-space printable ASCII character in a metadata URI. */
constexpr uint8 QNFT_METADATA_URI_MIN_CHARACTER = Ch::exclamation_mark;
/** @brief Last printable ASCII character (tilde); this QPI revision has no Ch::tilde enumerator. */
constexpr uint8 QNFT_METADATA_URI_MAX_CHARACTER = 126;
/** @brief Fixed explanation URL capacity in the native proposal ABI, in bytes. */
constexpr uint32 QNFT_PROPOSAL_URL_CAPACITY = 256;
/** @brief Power-of-two wire capacity for proposed fee changes; unused entries must be zero. */
constexpr uint32 QNFT_FEE_BATCH_CAPACITY = 16;
/** @brief Number of governed fee parameters; checked against FeeVariable below. */
constexpr uint32 QNFT_FEE_VARIABLE_COUNT = 14;
/** @brief Variable IDs 3 and 4 jointly govern the complementary revenue shares. */
constexpr uint32 QNFT_REVENUE_SPLIT_VARIABLE_MASK = (1U << 3) | (1U << 4);
/** @brief Maximum number of listing IDs returned in one shop page. */
constexpr uint32 QNFT_PAGE_CAPACITY = 32;
/** @brief Maximum full registry records per response, keeping both page outputs below the QPI 65535-byte limit. */
constexpr uint32 QNFT_REGISTRY_PAGE_CAPACITY = 16;
/** @brief Number of recent public events retained independently of node logging. */
constexpr uint32 QNFT_EVENT_CAPACITY = 65536;
/** @brief Maximum public journal records returned by one query. */
constexpr uint32 QNFT_EVENT_PAGE_CAPACITY = 16;
/** @brief Power-of-two ring slot mask; event IDs start at one. */
constexpr uint64 QNFT_EVENT_SLOT_MASK = QNFT_EVENT_CAPACITY - 1;
/** @brief Last assignable event ID, leaving room for an exclusive end cursor. */
constexpr uint64 QNFT_LAST_EVENT_ID = 0xfffffffffffffffeULL;
/** @brief Maximum proposal indices returned by the QPI proposal-index getter, in entries. */
constexpr uint32 QNFT_PROPOSAL_INDEX_CAPACITY = 64;
/** @brief Deferred refund map capacity, with spare slots beyond all 1,024 core contract IDs. */
constexpr uint32 QNFT_CONTRACT_REFUND_SLOTS = 2048;
/** @brief Maximum deferred refund transfers attempted per tick. */
constexpr uint32 QNFT_REFUNDS_PER_TICK = 8;
/** @brief Maximum pending native mint deliveries attempted on one tick. */
constexpr uint32 QNFT_MINT_DELIVERIES_PER_TICK = 32;
/** @brief Removed-slot percentage above which persistent hash maps are compacted. */
constexpr uint32 QNFT_MAP_CLEANUP_PERCENT = 25;
/** @brief Last permitted metadata revision; updates must not wrap this counter. */
constexpr uint32 QNFT_MAX_METADATA_VERSION = 0xffffffffU;
/** @brief Maximum number of issued item IDs; IDs are never recycled. */
constexpr uint32 QNFT_MAX_ITEMS = 131072;
/** @brief Maximum number of collection records stored by the contract. */
constexpr uint32 QNFT_MAX_COLLECTIONS = 16384;
/** @brief Maximum number of live owner/operator approvals admitted to the map. */
constexpr uint32 QNFT_MAX_APPROVALS = 49152;
/** @brief Operator approval hash-map capacity, including spare slots for efficient lookup. */
constexpr uint32 QNFT_APPROVAL_SLOTS = 65536;
/** @brief Shop listing hash-map capacity, including unused and removed slots. */
constexpr uint32 QNFT_LISTING_SLOTS = 65536;
/** @brief Default fee for creating one collection, in QU. */
constexpr sint64 QNFT_COLLECTION_FEE = 1000000;
/** @brief Default fee for issuing one native asset, regardless of edition size, in QU. */
constexpr sint64 QNFT_MINT_FEE = 100000;
/** @brief Maximum number of active escrow listings admitted to the shop. */
constexpr uint32 QNFT_MAX_LISTINGS = 32768;
/** @brief Default shop commission on gross sale consideration, in basis points. */
constexpr uint32 QNFT_SHOP_FEE_BPS = 30;
/** @brief Denominator representing 100 percent in basis-point calculations. */
constexpr uint32 QNFT_BPS_DENOMINATOR = 10000;
/** @brief Fixed developer allocation of protocol revenue, in basis points; not governable. */
constexpr uint32 QNFT_DEVELOPER_FEE_BPS = 2000;
/** @brief Default shareholder allocation of protocol revenue, in basis points. */
constexpr uint32 QNFT_SHAREHOLDER_FEE_BPS = 6000;
/** @brief Default execution-reserve burn allocation of protocol revenue, in basis points. */
constexpr uint32 QNFT_BURN_FEE_BPS = 2000;
/** @brief Maximum number of native QPI shareholder proposal slots. */
constexpr uint16 QNFT_PROPOSAL_SLOTS = 8;
/** @brief Default fee for creating or replacing one scalar or batch proposal, in QU. */
constexpr sint64 QNFT_PROPOSAL_FEE = 1000000;
/** @brief Default tariff for routine paid actions, in QU. */
constexpr sint64 QNFT_ACTION_FEE = 10000;
/** @brief Default fee for updating mutable item metadata, in QU. */
constexpr sint64 QNFT_METADATA_FEE = 50000;
/** @brief Packed IPO share name used to identify QNFT shareholders for voting and dividends. */
constexpr uint64 QNFT_SHARE_NAME = Ch::Q | (Ch::N << 8) | (Ch::F << 16) | (Ch::T << 24);
/** @brief Rental storage and bounded processing limits. */
constexpr uint32 QNFT_MAX_RENTAL_OFFERS = 4096;
/** @brief Maximum retained rentals, including ended records with deposits, credits or queued work. */
constexpr uint32 QNFT_MAX_RENTALS = 8192;
/** @brief Maximum due rentals and, separately, queued rental payments processed per tick. */
constexpr uint32 QNFT_RENTALS_PER_TICK = 32;
/** @brief Maximum storage slots inspected per rental or user-holdings page. */
constexpr uint32 QNFT_RENTAL_PAGE_SCAN = 256;
/** @brief Minimum fixed paid rental period in whole days. */
constexpr uint32 QNFT_MIN_RENTAL_DAYS = 15;
/** @brief Microseconds in one rental billing day; used to project elapsed periods. */
constexpr sint64 QNFT_DAY_MICROSECONDS = 86400000000LL;
/** @brief Default rental protocol and creator allocations, governed after initialization. */
constexpr uint32 QNFT_RENTAL_PROTOCOL_BPS = 700;
/** @brief Default immutable collection creator allocation of rent, in basis points. */
constexpr uint32 QNFT_RENTAL_CREATOR_BPS = 300;

/**
 * @brief Reserved extension contract type.
 */
struct QNFT2
{
};

/**
 * @brief Native-asset NFT registry and fixed-price shop.
 */
struct QNFT : public ContractBase
{
	/** @brief Standard QPI yes/no proposal wire format. */
	using ProposalDataT = ProposalDataYesNo;

	/** @brief QNFT IPO share possession determines proposal rights and snapshotted vote weights. */
	using ProposersAndVotersT = ProposalAndVotingByShareholders<QNFT_PROPOSAL_SLOTS, QNFT_SHARE_NAME>;

	/** @brief Native QPI proposal and ballot storage; no custom vote-counting engine. */
	using ProposalVotingT = ProposalVoting<ProposersAndVotersT, ProposalDataT>;

	/**
	 * @brief Public proposal variable IDs; developerFeeBPS is deliberately not addressable.
	 */
	enum class FeeVariable : uint64
	{
		/** @brief Collection creation charge in QU. */
		CollectionFee = 0,
		/** @brief Native NFT issuance charge in QU. */
		MintFee,
		/** @brief Shop commission in basis points. */
		ShopFeeBPS,
		/** @brief Revenue share for dividends; burn is the complementary share. */
		ShareholderFeeBPS,
		/** @brief Revenue share for burn; dividends are the complementary share. */
		BurnFeeBPS,
		/** @brief Governed nominateAdminFee charge in QU. */
		NominateAdminFee,
		/** @brief Governed transferFee charge in QU. */
		TransferFee,
		/** @brief Governed approveFee charge in QU. */
		ApproveFee,
		/** @brief Governed updateMetadataFee charge in QU. */
		UpdateMetadataFee,
		/** @brief Governed exportFee charge in QU. */
		ExportFee,
		/** @brief Governed listingFee charge in QU. */
		ListingFee,
		/** @brief Governed proposalFee charge in QU. */
		ProposalFee,
		/** @brief Protocol allocation of each rental period, in basis points. */
		RentalProtocolBPS,
		/** @brief Collection creator allocation of each rental period, in basis points. */
		RentalCreatorBPS,
	};

	STATIC_ASSERT(QNFT_FEE_VARIABLE_COUNT == static_cast<uint32>(FeeVariable::RentalCreatorBPS) + 1, QNFT_FEE_VARIABLE_COUNT_MATCHES_IDS);
	STATIC_ASSERT(QNFT_REVENUE_SPLIT_VARIABLE_MASK ==
	                  ((1U << static_cast<uint32>(FeeVariable::ShareholderFeeBPS)) | (1U << static_cast<uint32>(FeeVariable::BurnFeeBPS))),
	              QNFT_REVENUE_SPLIT_MASK_MATCHES_IDS);

	/**
	 * @brief One governed parameter and its proposed QU amount or basis-point rate.
	 */
	struct FeeChange
	{
		/** @brief Existing governed variable ID; developer rate has no ID. */
		FeeVariable variable;

		/** @brief Proposed value, validated before storage and application. */
		sint64 value;
	};

	/**
	 * @brief Canonical, hash-bound set of changes; unused entries and padding are zero.
	 * @note Only the populated prefix is applied; zeroed tail and padding make the hashed payload canonical.
	 */
	struct FeeProposalBatch
	{
		/** @brief At most one entry for each governed variable. */
		Array<FeeChange, QNFT_FEE_BATCH_CAPACITY> changes;

		/** @brief Number of populated entries; at most 12 distinct governed IDs are supported. */
		uint32 count;
	};

	/**
	 * @brief Governance log actions, separate from NFT and market events.
	 */
	enum class GovernanceAction : uint32
	{
		/** @brief Create, replace or cancel a proposal. */
		SetProposal,

		/** @brief Cast or clear a shareholder's votes. */
		Vote,

		/** @brief Apply a quorum-approved state-variable value. */
		Apply
	};

	/**
	 * @brief NFT command identifiers.
	 */
	enum class Operation : uint32
	{
		/** @brief Create a collection and charge the configured collection fee. */
		CreateCollection = 1,
		/** @brief Nominate the next collection administrator as the creator or current administrator. */
		NominateAdmin,
		/** @brief Accept collection administration as the nominated account. */
		AcceptAdmin,
		/** @brief Issue a fixed edition and deliver it, preserving recoverable delivery failures. */
		Mint,
		/** @brief Transfer owned units or use an owner-wide operator approval. */
		Transfer,
		/** @brief Update mutable metadata as a collection manager. */
		UpdateMetadata,
		/** @brief Permanently freeze editable native-item metadata. */
		FreezeMetadata,
		/** @brief Release asset management to an accepting registered destination. */
		Export,
		/** @brief Burn native units without restoring collection issuance quota. */
		Burn,
		/** @brief Burn the entire invocation reward into the execution reserve. */
		FundReserve,
	};

	/**
	 * @brief Shared status codes for command and query results.
	 */
	enum class Code : uint32
	{
		/** @brief Operation completed successfully. */
		Ok,

		/** @brief Malformed or out-of-range arguments. */
		InvalidInput,

		/** @brief Requested object or asset was not found. */
		NotFound,

		/** @brief Caller lacks the required permission. */
		Unauthorized,

		/** @brief A bounded registry or credit ledger is full. */
		Capacity,

		/** @brief Attached QU does not cover the required charge. */
		InsufficientFee,

		/** @brief Available holdings or credit are too small. */
		InsufficientBalance,

		/** @brief A native or cross-contract operation failed. */
		ExternalFailure,

		/** @brief Metadata changes are permanently disabled. */
		Frozen,

		/** @brief Request conflicts with the current lifecycle or listing state. */
		Conflict,

		/** @brief A recoverable operation requires a later completion or retry. */
		Pending
	};

	/**
	 * @brief Collection issuance gate; Closed cannot be reopened.
	 */
	enum class MintingStatus : uint8
	{
		/** @brief New editions may be created within the remaining quota. */
		Open,
		/** @brief New issuance is suspended; either collection manager may resume it. */
		Paused,
		/** @brief New issuance is permanently disabled; accepted deliveries remain recoverable. */
		Closed
	};

	/** @brief Permitted metadata lifecycle. */
	enum class Policy : uint8
	{
		/** @brief Metadata cannot be edited after creation. */
		Immutable,

		/** @brief Collection managers may update metadata until frozen. */
		Mutable
	};

	/** @brief Token log identifiers, disjoint from command and market actions. */
	enum class TokenEventType : uint32
	{
		/** @brief One ownership balance change, including issuance and burning. */
		TransferSingle = 0x10000,

		/** @brief Current metadata URI for an item. */
		URI
	};

	/**
	 * @brief Content reference stored alongside the native asset.
	 */
	struct Metadata
	{
		/** @brief Zero-terminated printable URI; unused bytes must be zero. */
		Array<uint8, QNFT_METADATA_URI_CAPACITY> uri;

		/** @brief Digest identifying the referenced content. */
		id contentHash;
	};

	/**
	 * @brief Authorship, administration and cumulative collection issuance limits.
	 */
	struct Collection
	{
		/** @brief Original author recorded at creation; immutable. */
		id creator;

		/** @brief Optional administrator with the same collection-management authority as its creator; zero until acceptance. */
		id administrator;

		/** @brief Proposed administrator who must accept the role. */
		id nominatedAdministrator;

		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Lifetime edition-unit issuance cap; zero means no author-imposed limit. */
		sint64 maxUnits;

		/** @brief Lifetime issued units. */
		sint64 issuedUnits;

		/** @brief Lifetime distinct-item cap; zero means no author-imposed limit. */
		uint32 maxItems;

		/** @brief Lifetime issued items. */
		uint32 issuedItems;

		/** @brief Collection policy: one forbids export and delivery to external smart contracts; creator or administrator may change it. */
		bit externalTransfersDisabled;
	};

	/**
	 * @brief Canonical NFT metadata and fixed edition supply.
	 * @note The asset identity is (QNFT issuer, assetName); metadata is shared by every unit of the edition.
	 * @note Issued item IDs are never recycled.
	 */
	struct Item
	{
		/** @brief Immutable collection creator, including when an administrator issues this item. */
		id creator;

		/** @brief Account credited with creator royalties on shop sales. */
		id royaltyReceiver;

		/** @brief Beneficiary of undelivered mint units. */
		id reservedRecipient;

		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Original fixed edition size in native asset units. */
		sint64 supply;

		/** @brief Local burn counter; GetItem derives total burns from the ledger. */
		sint64 burned;

		/** @brief Units awaiting delivery to the recorded beneficiary. */
		sint64 reserved;

		/** @brief Packed seven-character native asset name under the QNFT issuer. */
		uint64 assetName;

		/** @brief IPO share asset under NULL_ID; zero disables sale dividends. Immutable after mint. */
		uint64 dividendAssetName;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Metadata revision used to reject stale purchases. */
		uint32 metadataVersion;

		/** @brief Creator royalty in basis points; 10000 means 100%. */
		uint16 royaltyBps;

		/** @brief Immutable share of gross shop consideration in basis points. */
		uint16 dividendBps;

		/** @brief Rules governing metadata changes. */
		Policy policy;

		/** @brief Nonzero once metadata changes are permanently disabled. */
		bit frozen;
	};

	/**
	 * @brief Owner and item identifying the holdings checked for authorization.
	 */
	struct OwnerItem
	{
		/** @brief Account whose holdings, authorization or credit are addressed. */
		id owner;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint64 itemId;
	};

	/**
	 * @brief Escrowed edition units offered at a fixed price and metadata revision.
	 */
	struct Listing
	{
		/** @brief Owner of the escrowed units and beneficiary of sale proceeds. */
		id seller;

		/** @brief Price per edition unit in QU. */
		sint64 unitPrice;

		/** @brief Unsold native units held in shop escrow. */
		sint64 quantity;

		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Metadata revision used to reject stale purchases. */
		uint32 metadataVersion;
	};

	/**
	 * @brief Stable identifiers for shop events and market failures.
	 */
	enum class MarketAction : uint32
	{
		/** @brief Create a fixed-price escrow listing. */
		List = 1,

		/** @brief Return remaining escrow to its seller. */
		Cancel,

		/** @brief Buy a protected-price quantity and settle seller proceeds, royalties and configured sale dividends. */
		Buy,

		/** @brief Allocate the end-of-epoch fee pool. */
		Distribution,
		/** @brief Sale allocation to the selected IPO share holders. */
		SaleDividend
	};

	/** @brief Domain identifying the fields of a public journal record. */
	enum class EventKind : uint32
	{
		/** @brief Successful or pending NFT command. */
		Command = 1,
		/** @brief Shop action or epoch distribution. */
		Market,
		/** @brief Successful governance submission, vote or application. */
		Governance,
		/** @brief Actual asset ownership movement. */
		TransferSingle,
		/** @brief Stored metadata URI announcement. */
		URI,
		/** @brief Owner-wide operator permission grant or revocation; quantity carries the boolean flag. */
		ApprovalForAll,
		/** @brief Collection issuance gate changed; reference is collection ID and action is the new status. */
		MintingStatusChanged,
		/** @brief Collection creator or administrator changed the export/contract-delivery policy. */
		CollectionTransferPolicyChanged,
		/** @brief Unpaid rental availability was created or cancelled. */
		RentalOfferChanged,
		/** @brief A renter paid for the first usage period; ownership stays with the lender. */
		RentalStarted,
		/** @brief Elapsed renewal periods were charged against the prepaid deposit. */
		RentalRenewed,
		/** @brief Paid coverage ended and lender/renter indexes were released. */
		RentalEnded,
		/** @brief Future renewal was disabled without revoking the current paid period. */
		RentalRenewalCancelled,
		/** @brief Unused QU collateral was added or withdrawn; event amount carries the sign. */
		RentalDepositChanged,
	};

	/** @brief Fixed public event record; unused fields are zero. */
	struct JournalEvent
	{
		/** @brief Invocator, or the actor recorded by governance or epoch settlement. */
		id actor;

		/** @brief Debited owner for TransferSingle; zero for issuance or other kinds. */
		id from;

		/** @brief Payment beneficiary or command/market recipient. */
		id to;

		/** @brief Exact URI for URI events; empty for other kinds. */
		Array<uint8, QNFT_METADATA_URI_CAPACITY> uri;

		/** @brief Monotonic identifier beginning at one. */
		uint64 eventId;

		/** @brief Listing ID for market events, fee variable ID for governance. */
		uint64 reference;

		/** @brief Native units, or QU per share for a distribution event. */
		sint64 quantity;

		/** @brief Market QU amount or proposed/applied governance value. */
		sint64 amount;

		/** @brief Network tick at which the record was appended. */
		uint32 tick;

		/** @brief Discriminator selecting record semantics. */
		EventKind kind;

		/** @brief Operation, MarketAction or GovernanceAction numeric value. */
		uint32 action;

		/** @brief Item ID where the event concerns an item. */
		uint32 itemId;

		/** @brief Command metadata version or governance proposal index. */
		uint32 version;

		/** @brief Governance result; zero for other event kinds. */
		Code code;
	};

	/** @brief Record to append; identifier and tick are assigned internally. */
	using AppendEvent_input = JournalEvent;

	/** @brief No result is returned after appending a journal event. */
	using AppendEvent_output = NoData;

	/** @brief Query the retained event interval. */
	using GetEventRange_input = NoData;

	/** @brief Inclusive first ID and exclusive end of the retained journal. */
	struct GetEventRange_output
	{
		/** @brief First retained ID, or nextEventId when empty. */
		uint64 firstEventId;

		/** @brief Exclusive end cursor; never wraps. */
		uint64 nextEventId;

		/** @brief Maximum number of retained events. */
		uint32 capacity;

		/** @brief Number of records currently retained. */
		uint32 count;
	};

	/** @brief Inclusive cursor and bounded page size for public event reads. */
	struct GetEvents_input
	{
		/** @brief First requested ID; zero starts at the oldest retained event. */
		uint64 eventId;

		/** @brief Requested number of records, from one to the event page capacity. */
		uint32 limit;
	};

	/** @brief Public journal page and the interval observed by this query. */
	struct GetEvents_output
	{
		/** @brief Chronological records; entries beyond count are zero. */
		Array<JournalEvent, QNFT_EVENT_PAGE_CAPACITY> events;

		/** @brief Oldest retained event ID at query time. */
		uint64 firstEventId;

		/** @brief Exclusive end of the journal at query time. */
		uint64 nextEventId;

		/** @brief Inclusive cursor for the following page. */
		uint64 cursor;

		/** @brief Number of returned records. */
		uint32 count;

		/** @brief NotFound means the requested history was overwritten. */
		Code code;
	};

	/** @brief Full collection record and its registry identity. */
	struct CollectionEntry
	{
		/** @brief Current collection metadata, administration and issuance limits. */
		Collection collection;

		/** @brief Registry ID of this collection. */
		uint32 collectionId;
	};

	/** @brief Page through the contiguous collection registry. */
	struct ListCollections_input
	{
		/** @brief Starting collection ID; collectionCount is an empty terminal page. */
		uint32 offset;

		/** @brief Maximum full records returned, from 1 to QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
	};

	/** @brief Bounded full collection records from the current state. */
	struct ListCollections_output
	{
		/** @brief Collections in ascending ID order; only the first count entries are populated. */
		Array<CollectionEntry, QNFT_REGISTRY_PAGE_CAPACITY> collections;

		/** @brief Number of returned collections. */
		uint32 count;

		/** @brief Next collection ID; equals totalCount at the end of this call's registry. */
		uint32 nextOffset;

		/** @brief Total allocated collection count at this call. */
		uint32 totalCount;

		/** @brief Ok or InvalidInput for an invalid limit or offset past the registry. */
		Code code;
	};

	/** @brief Record assembled for each collection in a page. */
	struct ListCollections_locals
	{
		/** @brief Collection snapshot and its explicit ID. */
		CollectionEntry entry;
	};

	/** @brief Owner/operator identity pair hashed for persistent owner-wide permission. */
	struct OperatorKey
	{
		/** @brief Account whose ownership or authority is queried or acted upon; NULL_ID is invalid for balance and movement requests. */
		id owner;
		/** @brief Identity receiving delegated authority; cannot be NULL_ID or the owner when granting permission. */
		id operatorId;
	};

	/** @brief Fixed rental terms; remaining units stay reserved until accepted or cancelled. */
	struct RentalOffer
	{
		/** @brief Native owner backing the units; rental usage does not transfer ownership. */
		id owner;
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
		/** @brief Fixed price in QU per unit per day; positive and bounded by MAX_AMOUNT. */
		sint64 dailyPrice;
		/** @brief Unpaid units still available in this offer; removed when all units are accepted or cancelled. */
		sint64 quantity;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Fixed renewal period in whole days, at least QNFT_MIN_RENTAL_DAYS; expiry must remain representable. */
		uint32 periodDays;
	};

	/** @brief A closed subscription cannot be revived by a later deposit. */
	enum class RentalStatus : uint32
	{
		/** @brief Usage rights are active or require projection to determine whether funded renewal extends them. */
		Active,
		/** @brief Paid usage has ended permanently; remaining deposits and credits may still be retained. */
		Ended
	};

	/** @brief Owner-backed usage rights and their QU collateral; amounts never represent NFT ownership. */
	struct Rental
	{
		/** @brief Native owner backing the units; rental usage does not transfer ownership. */
		id owner;
		/** @brief Account entitled to use the rented units; receives any unused deposit. */
		id renter;
		/** @brief Immutable collection creator receiving the creator allocation of rent. */
		id creator;
		/** @brief Exclusive end of the paid usage interval; renewal may extend it from prepaid collateral. */
		DateAndTime paidUntil;
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
		/** @brief Fixed price in QU per unit per day; positive and bounded by MAX_AMOUNT. */
		sint64 dailyPrice;
		/** @brief QU charged for all rented units for one complete period; positive and bounded by MAX_AMOUNT. */
		sint64 periodPrice;
		/** @brief Positive number of native units covered by this offer or rental. */
		sint64 quantity;
		/** @brief Unused renter-funded QU reserved for future renewal; withdrawable after elapsed rent is settled. */
		sint64 deposit;
		/** @brief Accrued unpaid QU owed to the lender; retained until a transfer succeeds. */
		sint64 ownerCredit;
		/** @brief Accrued unpaid QU owed to the immutable collection creator; retained until paid. */
		sint64 creatorCredit;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Fixed renewal period in whole days, at least QNFT_MIN_RENTAL_DAYS; expiry must remain representable. */
		uint32 periodDays;
		/** @brief Stored lifecycle state; active records require time projection to determine current coverage. */
		RentalStatus status;
		/** @brief Whether future elapsed periods may consume the deposit; cancellation preserves paid coverage. */
		bool autoRenew;
		/** @brief Whether the rental remains in both lender/type and renter/type indexes. */
		bool indexed;
		/** @brief Whether a payment-queue entry retains this record; prevents duplicate queue insertion. */
		bool payoutQueued;
	};

	/** @brief Public rental snapshot; paidUntil is Unix timestamp in microseconds since 1970-01-01 UTC, exclusive. */
	struct RentalView
	{
		/** @brief Native owner backing the units; rental usage does not transfer ownership. */
		id owner;
		/** @brief Account entitled to use the rented units; receives any unused deposit. */
		id renter;
		/** @brief Immutable collection creator receiving the creator allocation of rent. */
		id creator;
		/** @brief Exclusive usage expiry as Unix microseconds since 1970-01-01 UTC; zero for ownership-only rows. */
		uint64 paidUntil;
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
		/** @brief Fixed price in QU per unit per day; positive and bounded by MAX_AMOUNT. */
		sint64 dailyPrice;
		/** @brief QU charged for all rented units for one complete period; positive and bounded by MAX_AMOUNT. */
		sint64 periodPrice;
		/** @brief Positive number of native units covered by this offer or rental. */
		sint64 quantity;
		/** @brief Unused renter-funded QU reserved for future renewal; withdrawable after elapsed rent is settled. */
		sint64 deposit;
		/** @brief Accrued unpaid QU owed to the lender; retained until a transfer succeeds. */
		sint64 ownerCredit;
		/** @brief Accrued unpaid QU owed to the immutable collection creator; retained until paid. */
		sint64 creatorCredit;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Fixed renewal period in whole days, at least QNFT_MIN_RENTAL_DAYS; expiry must remain representable. */
		uint32 periodDays;
		/** @brief Stored lifecycle state; active records require time projection to determine current coverage. */
		RentalStatus status;
		/** @brief Whether future elapsed periods may consume the deposit; cancellation preserves paid coverage. */
		bit autoRenew;
	};

	/** @brief Public rental mutation result; paid is newly charged rent, not a future deposit. */
	struct RentalResult
	{
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
		/** @brief QU charged for the first period on Rent; zero for other rental mutations. */
		sint64 paid;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Owner/type key, shared with the existing approval key layout. */
	struct RentalBalance_input
	{
		/** @brief Account whose native holdings and outgoing/incoming rental quantities are queried. */
		id owner;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
	};

	/** @brief Locally movable units and effective reservations at the current network time. */
	struct RentalBalance_output
	{
		/** @brief Units both owned and possessed by this account under QNFT management. */
		sint64 managedQuantity;
		/** @brief Units reserved by unpaid offers for this owner/type pair. */
		sint64 offeredQuantity;
		/** @brief Units backing effective outgoing rentals, including affordable pending renewals. */
		sint64 lentQuantity;
		/** @brief Effective incoming usage units; never added to transferable native holdings. */
		sint64 rentedQuantity;
		/** @brief Locally managed units free of offer and active-rental reservations; never negative. */
		sint64 availableQuantity;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	using TransferableBalance_input = RentalBalance_input;

	/** @brief Movable local quantity after offer and effective rental reservations; errors invalidate the quantity. */
	struct TransferableBalance_output
	{
		/** @brief Locally managed units free of offer and active-rental reservations; never negative. */
		sint64 availableQuantity;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/**
	 * @brief Persistent NFT registry, marketplace, accounting, and governance state.
	 * @note Deferred refunds and undistributed revenue are separate obligations backed by retained QU.
	 */
	struct StateData
	{
		/** @brief Rental collateral remains in the native owner account; these registries only lock quantities. */
		HashMap<uint64, RentalOffer, QNFT_MAX_RENTAL_OFFERS * 2> rentalOffers;
		/** @brief Retained rental records keyed by stable rental ID; ended liabilities remain until cleared. */
		HashMap<uint64, Rental, QNFT_MAX_RENTALS * 2> rentals;
		/** @brief Sum of unpaid offer quantities keyed by K12(owner, itemId); updated with each offer mutation. */
		HashMap<id, sint64, QNFT_MAX_RENTAL_OFFERS * 2> offeredRentalQuantities;
		/** @brief Lender/type index of retained active rentals; bounded only by global rental capacity. Element indices are not persisted. */
		QPI::Collection<uint64, QNFT_MAX_RENTALS> lentRentals;
		/** @brief Renter/type index of usage grants; priorities are stable rental IDs for ordered traversal. */
		QPI::Collection<uint64, QNFT_MAX_RENTALS> takenRentals;
		/** @brief One wake-up per rental, ordered by negative Unix expiry timestamps in microseconds. */
		QPI::Collection<uint64, QNFT_MAX_RENTALS> rentalDeadlines;
		/** @brief FIFO of rental IDs with payment work; payoutQueued prevents duplicate entries. */
		LinkedList<uint64, QNFT_MAX_RENTALS> rentalPayments;
		/** @brief Last assigned offer ID; new offers reject exhaustion rather than wrap. */
		uint64 lastRentalOfferId;
		/** @brief Last assigned rental ID, bounded by signed 64-bit priorities; IDs never wrap. */
		uint64 lastRentalId;
		/** @brief Total unused QU held for all renter deposits; excluded from protocol revenue. */
		sint64 rentalDeposits;
		/** @brief Total unpaid lender and creator QU credits, backed separately from deposits and fees. */
		sint64 rentalCredits;

		/** @brief K12 owner/operator keys with active grants. */
		HashMap<id, bit, QNFT_APPROVAL_SLOTS> operatorApprovals;
		/** @brief Per-token guard set during releaseShares callbacks to reject reentrant movements of that token. */
		BitArray<QNFT_MAX_ITEMS> exporting;

		// Fixed-capacity registries bound persistent memory use.

		/** @brief Item registry indexed by item ID. */
		Array<Item, QNFT_MAX_ITEMS> items;

		/** @brief Collection registry indexed by collection ID. */
		Array<Collection, QNFT_MAX_COLLECTIONS> collections;

		/** @brief Number of allocated item registry entries. */
		uint32 itemCount;

		/** @brief Number of allocated collection registry entries. */
		uint32 collectionCount;

		// Failed transfers retain their collateral until automatic repayment succeeds.

		/** @brief Unpaid QU refunds, royalties and sale dividends keyed by beneficiary; retried on END_TICK. */
		HashMap<id, sint64, QNFT_CONTRACT_REFUND_SLOTS> contractRefunds;
		// Shop state retains only escrow listings and protocol revenue.

		/** @brief Active fixed-price listings keyed by unique listing ID. */
		HashMap<uint64, Listing, QNFT_LISTING_SLOTS> listings;

		/** @brief Current developer beneficiary; only this identity can transfer the role. */
		id developer;

		/** @brief QU commissions retained until end-of-epoch allocation. */
		sint64 epochFees;

		/** @brief Unpaid developer QU; retried after a failed transfer or missing recipient. */
		sint64 developerPending;

		/** @brief Dividend rounding dust or failed dividend pool retained for the next epoch. */
		sint64 shareholderCarry;

		/** @brief Last allocated listing ID, including removed listings. */
		uint64 lastListingId;

		/** @brief Collection creation charge in QU. */
		sint64 collectionFee;

		/** @brief Native NFT issuance charge in QU. */
		sint64 mintFee;

		/** @brief Shop commission in basis points. */
		uint32 shopFeeBPS;

		/** @brief Last committed allocation epoch; failed payments retry next epoch. */
		uint32 lastDistributionEpoch;

		/** @brief Developer share of epoch fees in basis points. */
		uint32 developerFeeBPS;

		/** @brief Shareholder share of epoch fees in basis points. */
		uint32 shareholderFeeBPS;

		/** @brief Reserve burn share of epoch fees in basis points; receives split dust. */
		uint32 burnFeeBPS;

		/** @brief Eight QPI proposal slots with per-proposal shareholder snapshots. */
		ProposalVotingT proposals;

		/** @brief Epoch whose accepted proposals were already applied. */
		uint32 lastGovernanceEpoch;

		/** @brief Governed NominateAdmin invocation fee in QU. */
		sint64 nominateAdminFee;

		/** @brief Governed Transfer invocation fee in QU. */
		sint64 transferFee;

		/** @brief Governed SetApprovalForAll grant fee in QU. */
		sint64 approveFee;

		/** @brief Governed UpdateMetadata invocation fee in QU. */
		sint64 updateMetadataFee;

		/** @brief Governed Export invocation fee in QU. */
		sint64 exportFee;

		/** @brief Governed Listing invocation fee in QU. */
		sint64 listingFee;

		/** @brief Governed Proposal invocation fee in QU. */
		sint64 proposalFee;

		/** @brief Sidecar payloads bound to native proposals by K12. */
		Array<FeeProposalBatch, QNFT_PROPOSAL_SLOTS> feeBatches;

		/** @brief Bounded public history, independent of optional node logs. */
		Array<JournalEvent, QNFT_EVENT_CAPACITY> journal;

		/** @brief Last assigned journal ID; zero before the first event. */
		uint64 lastEventId;

		/** @brief Next refund map slot to inspect; rotating cursor prevents starvation. */
		uint32 refundCursor;
		/** @brief Open, paused or irreversibly closed collection issuance; zero initializes Open. */
		Array<MintingStatus, QNFT_MAX_COLLECTIONS> mintingStatus;
		/** @brief Governed rental deductions; their sum must not exceed 10000 basis points. */
		uint32 rentalProtocolBPS;
		/** @brief Current creator allocation of rent in basis points; combined rental deductions cannot exceed 10000. */
		uint32 rentalCreatorBPS;
		/** @brief Callback guard: payment recipients cannot mutate rental accounting during a payout. */
		bool rentalPaymentRunning;
		/** @brief One queued entry per pending item; total entries cannot exceed the item capacity. */
		LinkedList<uint32, QNFT_MAX_ITEMS> pendingMintDeliveries;
	};

	/** @brief Read the current fixed tariffs and shop percentage. */
	using GetFeeSchedule_input = NoData;

	/**
	 * @brief Fees in QU; protective operations and votes are always free.
	 */
	struct GetFeeSchedule_output
	{
		/** @brief Current collectionFee in QU. */
		sint64 collectionFee;

		/** @brief Current mintFee in QU. */
		sint64 mintFee;

		/** @brief Current nominateAdminFee in QU. */
		sint64 nominateAdminFee;

		/** @brief Current transferFee in QU. */
		sint64 transferFee;

		/** @brief Current approveFee in QU. */
		sint64 approveFee;

		/** @brief Current updateMetadataFee in QU. */
		sint64 updateMetadataFee;

		/** @brief Current exportFee in QU. */
		sint64 exportFee;

		/** @brief Current listingFee in QU. */
		sint64 listingFee;

		/** @brief Current proposalFee in QU. */
		sint64 proposalFee;

		/** @brief Percentage of gross sale consideration, in basis points. */
		uint32 shopFeeBPS;
		/** @brief Protocol and creator deductions from each rental period, in basis points. */
		uint32 rentalProtocolBPS;
		/** @brief Current creator allocation of rent in basis points; combined rental deductions cannot exceed 10000. */
		uint32 rentalCreatorBPS;
	};

	/**
	 * @brief Shared compact result returned by NFT command procedures.
	 */
	struct OperationResult
	{
		/** @brief Packed seven-character native asset name under the QNFT issuer. */
		uint64 assetName;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to CreateCollection.
	 */
	struct CreateCollection_input
	{
		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Lifetime edition-unit issuance cap; zero means no author-imposed limit. */
		sint64 maxUnits;

		/** @brief Lifetime distinct-item cap; zero means no author-imposed limit. */
		uint32 maxItems;

		/** @brief Collection policy: one forbids export and delivery to external smart contracts; creator or administrator may change it. */
		bit externalTransfersDisabled;
	};

	/** @brief Reuse the shared OperationResult layout for CreateCollection_output. */
	using CreateCollection_output = OperationResult;

	/**
	 * @brief Arguments supplied to NominateAdmin.
	 */
	struct NominateAdmin_input
	{
		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;
	};

	/** @brief Reuse the shared OperationResult layout for NominateAdmin_output. */
	using NominateAdmin_output = OperationResult;

	/**
	 * @brief Arguments supplied to AcceptAdmin.
	 */
	struct AcceptAdmin_input
	{
		/** @brief Zero-based collection identifier. */
		uint32 collectionId;
	};

	/** @brief Reuse the shared OperationResult layout for AcceptAdmin_output. */
	using AcceptAdmin_output = OperationResult;

	/**
	 * @brief Arguments supplied to Mint.
	 */
	struct Mint_input
	{
		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Account credited with creator royalties on shop sales. */
		id royaltyReceiver;

		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief IPO share asset under NULL_ID; zero disables sale dividends. Immutable after mint. */
		uint64 dividendAssetName;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Creator royalty in basis points; 10000 means 100%. */
		uint16 royaltyBps;

		/** @brief Immutable share of gross shop consideration in basis points. */
		uint16 dividendBps;

		/** @brief Rules governing metadata changes. */
		Policy policy;
	};

	/** @brief Reuse the shared OperationResult layout for Mint_output. */
	using Mint_output = OperationResult;

	/**
	 * @brief Arguments supplied to TransferFrom.
	 */
	struct TransferFrom_input
	{
		/** @brief Account whose holdings, authorization or credit are addressed. */
		id owner;

		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for TransferFrom_output. */
	using TransferFrom_output = OperationResult;

	/**
	 * @brief Arguments supplied to UpdateMetadata.
	 */
	struct UpdateMetadata_input
	{
		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for UpdateMetadata_output. */
	using UpdateMetadata_output = OperationResult;

	/**
	 * @brief Arguments supplied to FreezeMetadata.
	 */
	struct FreezeMetadata_input
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for FreezeMetadata_output. */
	using FreezeMetadata_output = OperationResult;

	/**
	 * @brief Arguments supplied to Export by the owner or its approved operator.
	 */
	struct Export_input
	{
		/** @brief Account whose units are burned or exported; caller must own them or hold its approval. */
		id owner;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Maximum QU offered to the receiving asset manager. */
		sint64 offeredFee;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Contract index receiving asset management rights. */
		uint16 destination;
	};

	/** @brief Reuse the shared OperationResult layout for Export_output. */
	using Export_output = OperationResult;

	/**
	 * @brief Arguments supplied to Burn by the owner or its approved operator.
	 */
	struct Burn_input
	{
		/** @brief Account whose units are burned or exported; caller must own them or hold its approval. */
		id owner;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for Burn_output. */
	using Burn_output = OperationResult;

	/** @brief Reuse the shared NoData layout for FundReserve_input. */
	using FundReserve_input = NoData;

	/** @brief Reuse the shared OperationResult layout for FundReserve_output. */
	using FundReserve_output = OperationResult;

	/** @brief New beneficiary and authority for developer revenue. */
	struct ChangeDeveloper_input
	{
		/** @brief New developer beneficiary and authority; NULL_ID and QNFT itself are rejected. */
		id developer;
	};

	/** @brief Returns Ok, Unauthorized, or InvalidInput in code. */
	using ChangeDeveloper_output = OperationResult;

	/**
	 * @brief Arguments supplied to GetItem.
	 */
	struct GetItem_input
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/**
	 * @brief Result returned by GetItem.
	 */
	struct GetItem_output
	{
		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to GetCollection.
	 */
	struct GetCollection_input
	{
		/** @brief Zero-based collection identifier. */
		uint32 collectionId;
	};

	/**
	 * @brief Result returned by GetCollection.
	 */
	struct GetCollection_output
	{
		/** @brief Working copy or returned collection record. */
		Collection collection;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to GetBalance.
	 */
	struct GetBalance_input
	{
		/** @brief Account whose holdings, authorization or credit are addressed. */
		id owner;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Manager index whose asset balance is queried. */
		uint16 managingContract;
	};

	/**
	 * @brief Result returned by GetBalance.
	 */
	struct GetBalance_output
	{
		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/** @brief Original recipient and QU awaiting automatic repayment. */
	struct RefundEntry
	{
		/** @brief Address to which the refund will be retried. */
		id recipient;

		/** @brief Positive pending amount, in QU. */
		sint64 amount;
	};

	/** @brief Address whose pending refunds, royalties and sale dividends are requested. */
	struct GetRefund_input
	{
		/** @brief Beneficiary of the deferred QU payment. */
		id owner;
	};

	/** @brief Pending QU for one recipient; an absent entry returns zero. */
	struct GetRefund_output
	{
		/** @brief QU awaiting automatic repayment. */
		sint64 quantity;

		/** @brief Ok, or InvalidInput for a null address. */
		Code code;
	};

	/** @brief Page through pending refunds, royalties and sale dividends by physical map-slot cursor. */
	struct ListRefunds_input
	{
		/** @brief First map slot to inspect; capacity denotes the terminal page. */
		uint32 offset;

		/** @brief Maximum returned records, from 1 to QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
	};

	/** @brief Bounded list of positive pending refunds, royalties and sale dividends. */
	struct ListRefunds_output
	{
		/** @brief Pending recipients and amounts in ascending map-slot order. */
		Array<RefundEntry, QNFT_REGISTRY_PAGE_CAPACITY> refunds;

		/** @brief Number of populated entries. */
		uint32 count;

		/** @brief Next eligible map slot, or QNFT_CONTRACT_REFUND_SLOTS at the end. */
		uint32 nextOffset;

		/** @brief Number of positive pending refunds across the whole map. */
		uint32 totalCount;

		/** @brief Ok or InvalidInput for an invalid cursor or limit. */
		Code code;
	};

	/** @brief Working state for refund enumeration without invoking recipients. */
	struct ListRefunds_locals
	{
		/** @brief Current occupied map slot. */
		sint64 index;

		/** @brief Current recipient and pending amount. */
		RefundEntry entry;
	};

	/**
	 * @brief Arguments supplied to GetConfig.
	 */
	struct GetConfig_input
	{
	};

	/**
	 * @brief Result returned by GetConfig.
	 */
	struct GetConfig_output
	{
		/** @brief Common native asset issuer identity. */
		id issuer;

		/** @brief Persistent state size in bytes. */
		uint64 stateBytes;

		/** @brief Collection creation charge in QU. */
		sint64 collectionFee;

		/** @brief Native NFT issuance charge in QU. */
		sint64 mintFee;

		/** @brief Lifetime distinct-item cap; zero means no author-imposed limit. */
		uint32 maxItems;

		/** @brief Maximum number of collection registry entries. */
		uint32 maxCollections;

		/** @brief Number of allocated item registry entries. */
		uint32 itemCount;

		/** @brief Number of allocated collection registry entries. */
		uint32 collectionCount;

		/** @brief Version of the exposed contract interface. */
		uint32 abiVersion;
	};

	/** @brief Full item snapshot and its permanent registry ID. */
	struct ItemEntry
	{
		/** @brief Complete item, with burns reconciled against the native ledger. */
		Item item;

		/** @brief Registry ID of this item. */
		uint32 itemId;
	};

	/**
	 * @brief Arguments supplied to ListItems.
	 */
	struct ListItems_input
	{
		/** @brief Starting item ID; itemCount is an empty terminal page. */
		uint32 offset;

		/** @brief Maximum full records returned, from 1 to QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
	};

	/**
	 * @brief Result returned by ListItems.
	 */
	struct ListItems_output
	{
		/** @brief Full records in ascending ID order; only count entries are populated. */
		Array<ItemEntry, QNFT_REGISTRY_PAGE_CAPACITY> items;

		/** @brief Number of populated entries in the returned page. */
		uint32 count;

		/** @brief Cursor to pass as the next page offset. */
		uint32 nextOffset;

		/** @brief Total number of allocated items at this call. */
		uint32 totalCount;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to ListForSale.
	 */
	struct ListForSale_input
	{
		/** @brief Price per edition unit in QU. */
		sint64 unitPrice;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/**
	 * @brief Result returned by ListForSale.
	 */
	struct ListForSale_output
	{
		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to CancelSale.
	 */
	struct CancelSale_input
	{
		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;
	};

	/**
	 * @brief Result returned by CancelSale.
	 */
	struct CancelSale_output
	{
		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to Buy.
	 */
	struct Buy_input
	{
		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Price per edition unit in QU. */
		sint64 unitPrice;

		/** @brief Metadata revision used to reject stale purchases. */
		uint32 metadataVersion;
	};

	/**
	 * @brief Result returned by Buy.
	 */
	struct Buy_output
	{
		/** @brief Gross purchase amount charged in QU; zero on public failure. */
		sint64 paid;

		/** @brief QU charge retained or paid by this operation. */
		sint64 fee;

		/** @brief Creator royalty amount in QU, rounded down. */
		sint64 royalty;

		/** @brief QU owed to sale shareholders, including deferred transfers; excludes rounding dust. */
		sint64 shareholderAmount;

		/** @brief Result or rejection reason. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to GetListing.
	 */
	struct GetListing_input
	{
		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;
	};

	/**
	 * @brief Result returned by GetListing.
	 */
	struct GetListing_output
	{
		/** @brief Working copy or returned listing record. */
		Listing listing;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to ListListings.
	 */
	struct ListListings_input
	{
		/** @brief Starting registry index or map slot for pagination. */
		uint32 offset;

		/** @brief Maximum number of results to return, at most 32. */
		uint32 limit;
	};

	/**
	 * @brief Result returned by ListListings.
	 */
	struct ListListings_output
	{
		/** @brief Active listing IDs returned in this page. */
		Array<uint64, QNFT_PAGE_CAPACITY> listingIds;

		/** @brief Number of populated entries in the returned page. */
		uint32 count;

		/** @brief Cursor to pass as the next page offset. */
		uint32 nextOffset;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to GetMarket.
	 */
	struct GetMarket_input
	{
	};

	/**
	 * @brief Result returned by GetMarket.
	 */
	struct GetMarket_output
	{
		/** @brief Current developer beneficiary; only this identity can transfer the role. */
		id developer;

		/** @brief QU commissions retained until end-of-epoch allocation. */
		sint64 epochFees;

		/** @brief Unpaid developer QU; retried after a failed transfer or missing recipient. */
		sint64 developerPending;

		/** @brief Dividend rounding dust or failed dividend pool retained for the next epoch. */
		sint64 shareholderCarry;

		/** @brief Last allocated listing ID, including removed listings. */
		uint64 lastListingId;

		/** @brief Number of currently active shop listings. */
		uint32 activeListings;

		/** @brief Shop commission in basis points. */
		uint32 feeBps;

		/** @brief Last committed allocation epoch; failed payments retry next epoch. */
		uint32 lastDistributionEpoch;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;
	};

	/**
	 * @brief Successful shop action log with the QPI contract/type prefix.
	 */
	struct MarketEvent
	{
		// The core logger requires the contract index and 32-bit event type first.

		/** @brief Required first log field identifying QNFT. */
		uint32 contractIndex;

		/** @brief Shop action recorded by this event. */
		MarketAction type;

		/** @brief Invoking account associated with this event. */
		id actor;

		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;

		/** @brief Asset units for trades; QU per share for distributions; zero for withdrawals. */
		sint64 quantity;

		/** @brief QU unit price, gross sale, withdrawal or burned allocation, depending on action. */
		sint64 amount;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Logger payload boundary; not serialized. */
		sint8 _terminator;
	};

	/** @brief Payload for the MarketEvent information log. */
	using EmitMarket_input = MarketEvent;

	/** @brief No result is returned by EmitMarket. */
	using EmitMarket_output = NoData;

	/** @brief ERC-1155-style ownership movement in the native Qubic log format. */
	struct TransferSingleEvent
	{
		// The logger requires this contract/type prefix before the address fields.
		/** @brief Contract emitting this log. */
		uint32 contractIndex;

		/** @brief TransferSingle discriminator. */
		TokenEventType type;

		/** @brief Caller responsible for the movement, distinct from the owner. */
		id operatorId;

		/** @brief Debited owner; NULL_ID denotes issuance. */
		id from;

		/** @brief Payment beneficiary; NULL_ID denotes burning. */
		id to;

		/** @brief Actual number of units moved. */
		sint64 quantity;

		/** @brief Item whose ownership changed. */
		uint32 itemId;

		/** @brief Logger boundary; not serialized. */
		sint8 _terminator;
	};

	/** @brief ERC-1155-style metadata URI announcement. */
	struct UriEvent
	{
		/** @brief Contract emitting this log. */
		uint32 contractIndex;

		/** @brief URI discriminator. */
		TokenEventType type;

		/** @brief Exact current URI, identical to GetItem.item.metadata.uri for this item. */
		Array<uint8, QNFT_METADATA_URI_CAPACITY> uri;

		/** @brief Item whose URI is announced. */
		uint32 itemId;

		/** @brief Logger boundary; not serialized. */
		sint8 _terminator;
	};

	/** @brief Movement fields completed by the owning operation. */
	using EmitTransfer_input = TransferSingleEvent;

	/** @brief No result is returned when emitting a movement. */
	using EmitTransfer_output = NoData;

	/** @brief Validated movement of a registered QNFT item managed by this contract. */
	struct TransferAsset_input
	{
		/** @brief Current owner and possessor of the transferred units. */
		id from;

		/** @brief New owner and possessor; NULL_ID performs a native burn. */
		id to;

		/** @brief Positive unit count validated by the caller. */
		sint64 quantity;

		/** @brief Existing item ID used for both native asset lookup and the event. */
		uint32 itemId;
	};

	/** @brief Preserve the native transfer result, including negative failure codes. */
	using TransferAsset_output = sint64;

	/** @brief Event data emitted only after a successful ledger movement. */
	struct TransferAsset_locals
	{
		/** @brief Owner/type pair used to check rental reservations before moving native units. */
		TransferableBalance_input rentalBalanceInput;
		/** @brief Effective reservations and available holdings returned by the balance helper. */
		TransferableBalance_output rentalBalance;
		/** @brief Actual movement recorded in the public journal and optional node log. */
		EmitTransfer_input event;

		/** @brief Empty event result. */
		EmitTransfer_output eventOutput;
	};

	/** @brief URI fields completed after metadata is committed. */
	using EmitUri_input = UriEvent;

	/** @brief No result is returned when emitting a URI. */
	using EmitUri_output = NoData;

	/**
	 * @brief Successful or pending NFT command log with the QPI contract/type prefix.
	 */
	struct Event
	{
		// The core logger requires the contract index and 32-bit event type first.

		/** @brief Required first log field identifying QNFT. */
		uint32 contractIndex;

		/** @brief NFT operation recorded by this event. */
		Operation type;

		/** @brief Invoking account associated with this event. */
		id actor;

		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Metadata revision at the time of this event. */
		uint32 version;

		/** @brief Logger payload boundary; not serialized. */
		sint8 _terminator;
	};

	/** @brief Payload for the Event information log. */
	using EmitCommand_input = Event;

	/** @brief No result is returned by EmitCommand. */
	using EmitCommand_output = NoData;

	/**
	 * @brief Disambiguates NFT commands, shop actions and management callbacks in error logs.
	 */
	enum class FailureDomain : uint32
	{
		/** @brief NFT command finalized through FinishCommand. */
		Command = 1,

		/** @brief Shop procedure or epoch distribution. */
		Market,

		/** @brief Incoming asset management callback. */
		Management
	};

	/**
	 * @brief Additive error log with domain, attempted operation and rejection code.
	 */
	struct FailureEvent
	{
		/** @brief Required first log field identifying QNFT. */
		uint32 contractIndex;

		/** @brief Namespace identifying the failing command, shop action or callback. */
		FailureDomain domain;

		/** @brief Invoking account associated with this event. */
		id actor;

		/** @brief Monotonic listing identifier; never reused. */
		uint64 listingId;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Public NFT operation being reported. */
		Operation operation;

		/** @brief Shop action that failed; zero outside the market domain. */
		MarketAction marketAction;

		/** @brief Result or rejection reason from the Code enum. */
		Code code;

		/** @brief Logger payload boundary; not serialized. */
		sint8 _terminator;
	};

	/** @brief Reuse the shared FailureEvent layout for LogFailure_input. */
	using LogFailure_input = FailureEvent;

	/** @brief Reuse the shared NoData layout for LogFailure_output. */
	using LogFailure_output = NoData;

	/**
	 * @brief Zero-initialized per-call working data for LogFailure.
	 */
	struct LogFailure_locals
	{
		/** @brief Log payload assembled for this execution. */
		FailureEvent event;
	};

	/** @brief Owner and item whose permission is checked against the invocator. */
	using isOwnerOrApprovedOperator_input = OwnerItem;

	/** @brief Result of the owner or owner-wide operator check. */
	struct isOwnerOrApprovedOperator_output
	{
		/** @brief True when the invocator may act for this owner and item. */
		bool authorized;
	};

	/** @brief Temporary data for the read-only authorization lookup. */
	struct isOwnerOrApprovedOperator_locals
	{
		/** @brief Owner/operator tuple used for the owner-wide permission lookup. */
		OperatorKey operatorKey;
		/** @brief Stored owner-wide permission flag; zero when no grant exists. */
		bit operatorApproved;
	};

	/**
	 * @brief Zero-initialized per-call working data for CheckMetadata.
	 */
	struct MetadataValidationScratch
	{
		/** @brief Bounded loop or pagination cursor. */
		uint32 i;

		/** @brief Whether a zero terminator has already been encountered. */
		bool terminated;
	};

	/**
	 * @brief Zero-initialized per-call working data for EncodeAssetName.
	 */
	struct AssetNameScratch
	{
		/** @brief Packed canonical native asset name assembled from the item identifier. */
		uint64 name;

		/** @brief Remaining item ID digits during base-36 encoding. */
		uint32 value;

		/** @brief Current base-36 digit or character being decoded. */
		uint32 digit;

		/** @brief Bounded loop or pagination cursor. */
		uint32 i;
	};

	/**
	 * @brief Arguments supplied to Lookup.
	 */
	struct Lookup_input
	{
		/** @brief Native ledger asset identified by issuer and name. */
		Asset asset;
	};

	/**
	 * @brief Result returned by Lookup.
	 */
	struct Lookup_output
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief True only when all validation checks succeed. */
		bool valid;
	};

	/**
	 * @brief Zero-initialized per-call working data for Lookup.
	 */
	struct Lookup_locals
	{
		/** @brief Digit-encoding scratch reused to construct or verify the canonical native asset name. */
		AssetNameScratch nameScratch;

		/** @brief Canonical name generated by the same function used when minting. */
		uint64 assetName;

		/** @brief Bounded loop or pagination cursor. */
		uint32 i;

		/** @brief Current base-36 digit or character being decoded. */
		uint32 digit;
	};

	/**
	 * @brief Arguments supplied to Refund.
	 */
	struct Refund_input
	{
		/** @brief QU amount to transfer, credit or report. */
		sint64 amount;
	};

	/**
	 * @brief Result returned by Refund.
	 */
	struct Refund_output
	{
	};

	/** @brief Validated protocol revenue retained in QU. */
	using AccrueProtocolFee_input = sint64;

	/** @brief Revenue accrual returns no additional result. */
	using AccrueProtocolFee_output = NoData;

	/** @brief Select whether finalization emits a standard event or preserves caller-owned logs. */
	enum class FinishEvents : uint8
	{
		/** @brief Emit the command or market result after settlement. */
		Standard,
		/** @brief Settle payment only; the caller owns its result and event format. */
		CallerManaged
	};

	/**
	 * @brief Arguments supplied to FinishCommand.
	 */
	struct FinishCommand_input
	{
		/** @brief Shop event payload, also identifying the attempted action on failure. */
		MarketEvent marketEvent;

		/** @brief Destination reported in the success event. */
		id receiver;

		/** @brief Result of the public operation. */
		OperationResult result;

		/** @brief Total invocation payment consumed, including sale proceeds or reserve funding. */
		sint64 retainedAmount;

		/** @brief Protocol fee credited to epoch revenue, or burned for reserve funding. */
		sint64 fee;

		/** @brief Requested item ID reported on failure. */
		uint32 itemId;

		/** @brief Requested collection ID reported on failure. */
		uint32 collectionId;

		/** @brief Public operation used for accounting and logs. */
		Operation operation;

		/** @brief Select command or shop event formatting. */
		FailureDomain domain;

		/** @brief CallerManaged settles payment without duplicating batch or governance events. */
		FinishEvents events;
	};

	/** @brief Reuse the shared OperationResult layout for FinishCommand_output. */
	using FinishCommand_output = OperationResult;

	/**
	 * @brief Zero-initialized per-call working data for FinishCommand.
	 */
	struct FinishCommand_locals
	{
		/** @brief Earned QU passed to centralized protocol revenue accrual. */
		AccrueProtocolFee_input accrualInput;

		/** @brief Empty result from centralized protocol revenue accrual. */
		AccrueProtocolFee_output accrualOutput;

		/** @brief Empty shop-event emission result. */
		EmitMarket_output marketOutput;

		/** @brief Arguments for LogFailure. */
		LogFailure_input failureInput;

		/** @brief Result from LogFailure. */
		LogFailure_output failureOutput;

		/** @brief Log payload assembled for this execution. */
		EmitCommand_input event;

		/** @brief Empty information-log result. */
		EmitCommand_output eventOutput;

		/** @brief Arguments for Refund. */
		Refund_input refundInput;

		/** @brief Result from Refund. */
		Refund_output refundOutput;
	};

	/** @brief Validated new collection ready for registry allocation. */
	using CommitNewCollection_input = Collection;

	/** @brief Allocated collection identifier. */
	using CommitNewCollection_output = uint32;

	/** @brief Listing whose asset units have already entered shop escrow. */
	using CommitNewListing_input = Listing;

	/** @brief Newly assigned monotonic listing identifier. */
	using CommitNewListing_output = uint64;

	/** @brief Validated document update and the existing item it replaces. */
	struct CommitMetadataUpdate_input
	{
		/** @brief Updated item snapshot. */
		Item item;

		/** @brief Existing registry index. */
		uint32 itemId;
	};

	/** @brief Metadata persistence returns no additional result. */
	using CommitMetadataUpdate_output = NoData;

	/** @brief Successful native burn and the item snapshot to update. */
	struct CommitBurn_input
	{
		/** @brief Item snapshot read before the native burn. */
		Item item;

		/** @brief Number of units actually burned. */
		sint64 quantity;

		/** @brief Existing registry index. */
		uint32 itemId;
	};

	/** @brief Burn persistence returns no additional result. */
	using CommitBurn_output = NoData;

	/** @brief Successful sale and the validated listing snapshot. */
	struct CommitListingFill_input
	{
		/** @brief Listing before subtracting the delivered units. */
		Listing listing;

		/** @brief Units already delivered to the buyer. */
		sint64 quantity;

		/** @brief Authoritative map key of the listing being filled. */
		uint64 listingId;
	};

	/** @brief Listing persistence returns no additional result. */
	using CommitListingFill_output = NoData;

	/** @brief Complete item snapshot after successful core issuance and capacity validation. */
	using CommitIssuedItem_input = Item;

	/** @brief Identity assigned to the newly registered item. */
	struct CommitIssuedItem_output
	{
		/** @brief Newly allocated registry index. */
		uint32 itemId;
	};

	/** @brief Collection counters updated together with the new registry entry. */
	struct CommitIssuedItem_locals
	{
		/** @brief Latest destination collection record. */
		Collection collection;
	};

	/** @brief Use the public collection arguments for validation and allocation. */
	using ExecuteCreateCollection_input = CreateCollection_input;

	/** @brief Return the allocated collection ID or the validation failure. */
	using ExecuteCreateCollection_output = OperationResult;

	/**
	 * @brief Zero-initialized working data for collection validation and allocation.
	 */
	struct ExecuteCreateCollection_locals
	{
		/** @brief Data for the centralized registry commit. */
		CommitNewCollection_output committedCollection;

		/** @brief Collection record to persist after validation. */
		Collection collection;

		/** @brief URI validation scratch; checks termination, printable bytes and zero trailing storage. */
		MetadataValidationScratch metadataScratch;
	};

	/**
	 * @brief Shared settlement arguments for the public collection procedure.
	 */
	struct CreateCollection_locals
	{
		/** @brief Result, fee and operation used for refunds and logs. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for NominateAdmin. */
	using ExecuteNominateAdmin_input = NominateAdmin_input;

	/** @brief Return the dedicated public result for NominateAdmin. */
	using ExecuteNominateAdmin_output = NominateAdmin_output;

	/**
	 * @brief Zero-initialized working data for ExecuteNominateAdmin.
	 */
	struct ExecuteNominateAdmin_locals
	{
		/** @brief Working copy or returned collection record. */
		Collection collection;
	};

	/**
	 * @brief Zero-initialized per-call working data for NominateAdmin.
	 */
	struct NominateAdmin_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for AcceptAdmin. */
	using ExecuteAcceptAdmin_input = AcceptAdmin_input;

	/** @brief Return the dedicated public result for AcceptAdmin. */
	using ExecuteAcceptAdmin_output = AcceptAdmin_output;

	/**
	 * @brief Zero-initialized working data for ExecuteAcceptAdmin.
	 */
	struct ExecuteAcceptAdmin_locals
	{
		/** @brief Working copy or returned collection record. */
		Collection collection;
	};

	/**
	 * @brief Zero-initialized per-call working data for AcceptAdmin.
	 */
	struct AcceptAdmin_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for Mint. */
	using ExecuteMint_input = Mint_input;

	/**
	 * @brief Result and settlement details returned by ExecuteMint.
	 */
	struct ExecuteMint_output
	{
		/** @brief Public operation result. */
		Mint_output result;

		/** @brief QU retained or burned after successful allocation. */
		sint64 fee;
	};

	/**
	 * @brief Zero-initialized working data for ExecuteMint.
	 */
	struct ExecuteMint_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Registry identity returned after committing the issued item. */
		CommitIssuedItem_output committedItem;

		/** @brief Current URI to announce after committing metadata. */
		EmitUri_input uriEvent;

		/** @brief Empty URI logging result. */
		EmitUri_output uriLogOutput;

		/** @brief Actual ownership movement to emit after successful ledger mutation. */
		EmitTransfer_input transferEvent;

		/** @brief Empty movement logging result. */
		EmitTransfer_output transferLogOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Working copy or returned collection record. */
		Collection collection;

		/** @brief Digit-encoding scratch reused to construct or verify the canonical native asset name. */
		AssetNameScratch nameScratch;

		/** @brief Result from Name. */
		uint64 assetName;

		/** @brief URI validation scratch; checks termination, printable bytes and zero trailing storage. */
		MetadataValidationScratch metadataScratch;

		/** @brief Validation result for the immutable sale-dividend asset and allocation terms. */
		Code dividendCode;
	};

	/**
	 * @brief Zero-initialized per-call working data for Mint.
	 */
	struct Mint_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;

		/** @brief Operation result and settlement details. */
		ExecuteMint_output operationOutput;
	};

	/** @brief Use the dedicated public arguments for Transfer. */
	using ExecuteTransfer_input = TransferFrom_input;

	/** @brief Return the dedicated public result for Transfer. */
	using ExecuteTransfer_output = TransferFrom_output;

	/**
	 * @brief Zero-initialized working data for ExecuteTransfer.
	 */
	struct ExecuteTransfer_locals
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Native unit movement request; source and destination share QNFT management. */
		TransferAsset_input transferInput;
		/** @brief Native transfer result; negative means the requested movement did not succeed. */
		TransferAsset_output transferResult;
	};

	/**
	 * @brief Zero-initialized per-call working data for Transfer.
	 */
	struct TransferFrom_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	using ValidateMetadataAccess_input = GetItem_input;

	/** @brief Editable item snapshot and access/lifecycle rejection code. */
	struct ValidateMetadataAccess_output
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Use the dedicated public arguments for UpdateMetadata. */
	using ExecuteUpdateMetadata_input = UpdateMetadata_input;

	/** @brief Return the dedicated public result for UpdateMetadata. */
	using ExecuteUpdateMetadata_output = UpdateMetadata_output;

	/**
	 * @brief Zero-initialized working data for ExecuteUpdateMetadata.
	 */
	struct ExecuteUpdateMetadata_locals
	{
		/** @brief Data for the centralized registry commit. */
		CommitMetadataUpdate_input commitInput;

		/** @brief Empty metadata commit result. */
		CommitMetadataUpdate_output commitOutput;

		/** @brief Current URI to announce after committing metadata. */
		EmitUri_input uriEvent;

		/** @brief Empty URI logging result. */
		EmitUri_output uriLogOutput;

		/** @brief Metadata-edit authorization result and editable item snapshot. */
		ValidateMetadataAccess_output access;
		/** @brief Item identifier whose collection authority and frozen status must be checked. */
		ValidateMetadataAccess_input accessInput;

		/** @brief URI validation scratch; checks termination, printable bytes and zero trailing storage. */
		MetadataValidationScratch metadataScratch;
	};

	/**
	 * @brief Zero-initialized per-call working data for UpdateMetadata.
	 */
	struct UpdateMetadata_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for FreezeMetadata. */
	using ExecuteFreezeMetadata_input = FreezeMetadata_input;

	/** @brief Return the dedicated public result for FreezeMetadata. */
	using ExecuteFreezeMetadata_output = FreezeMetadata_output;

	/**
	 * @brief Zero-initialized working data for ExecuteFreezeMetadata.
	 */
	struct ExecuteFreezeMetadata_locals
	{
		/** @brief Metadata-edit authorization result and editable item snapshot. */
		ValidateMetadataAccess_output access;
		/** @brief Item identifier whose collection authority and frozen status must be checked. */
		ValidateMetadataAccess_input accessInput;
	};

	/**
	 * @brief Zero-initialized per-call working data for FreezeMetadata.
	 */
	struct FreezeMetadata_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for Export. */
	using ExecuteExport_input = Export_input;

	/**
	 * @brief Result and settlement details returned by ExecuteExport.
	 */
	struct ExecuteExport_output
	{
		/** @brief Public operation result. */
		Export_output result;
		/** @brief Net destination payment plus the protocol fee on success. */
		sint64 retainedAmount;
		/** @brief Protocol fee, charged only after successful export. */
		sint64 fee;
	};

	/**
	 * @brief Zero-initialized working data for ExecuteExport.
	 */
	struct ExecuteExport_locals
	{
		/** @brief Owner/type pair used to check rental reservations before moving native units. */
		RentalBalance_input rentalBalanceInput;
		/** @brief Effective reservations and available holdings returned by the balance helper. */
		RentalBalance_output rentalBalance;

		/** @brief Owner and item passed to the authorization helper. */
		isOwnerOrApprovedOperator_input authorizationInput;

		/** @brief Whether the invocator is authorized for this operation. */
		isOwnerOrApprovedOperator_output authorizationOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Native ledger asset identified by issuer and name. */
		Asset asset;

		/** @brief QNFT entity balance before recipient management callbacks. */
		Entity beforeExport;

		/** @brief QNFT entity balance after recipient management callbacks. */
		Entity afterExport;

		/** @brief Net QU spent during export plus the QNFT fee charged on success. */
		sint64 spentAmount;

		/** @brief Whether management rights were released successfully. */
		bool exportSucceeded;
	};

	/**
	 * @brief Zero-initialized per-call working data for Export.
	 */
	struct Export_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;

		/** @brief Operation result and settlement details. */
		ExecuteExport_output operationOutput;
	};

	/** @brief Use the dedicated public arguments for Burn. */
	using ExecuteBurn_input = Burn_input;

	/** @brief Return the dedicated public result for Burn. */
	using ExecuteBurn_output = Burn_output;

	/**
	 * @brief Zero-initialized working data for ExecuteBurn.
	 */
	struct ExecuteBurn_locals
	{
		/** @brief Validated record mutation applied only after the associated native operation succeeds. */
		CommitBurn_input commitInput;
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Native unit movement request; source and destination share QNFT management. */
		TransferAsset_input transferInput;
		/** @brief Native transfer result; negative means the requested movement did not succeed. */
		TransferAsset_output transferResult;
		/** @brief Empty acknowledgement of the corresponding registry commit. */
		CommitBurn_output commitOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for Burn.
	 */
	struct Burn_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	using ProcessMintDeliveries_input = NoData;
	using ProcessMintDeliveries_output = NoData;

	/** @brief Bounded retry cursor and native transfer buffers for undelivered minted units. */
	struct ProcessMintDeliveries_locals
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Native unit movement request; source and destination share QNFT management. */
		TransferAsset_input transferInput;
		/** @brief Native transfer result; negative means the requested movement did not succeed. */
		TransferAsset_output transferResult;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
		/** @brief Number of queued entries left in this bounded processing pass. */
		uint64 remaining;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
	};

	/** @brief Empty request for deferred refund repayment. */
	using RetryRefunds_input = NoData;

	/** @brief Repayment results are reflected in balances and the pending map. */
	using RetryRefunds_output = NoData;

	/** @brief Working data for bounded, callback-safe refund repayment. */
	struct RetryRefunds_locals
	{
		/** @brief Original recipient, never replaceable by another caller. */
		id recipient;

		/** @brief QU removed before invoking the recipient. */
		sint64 amount;

		/** @brief New refunds accumulated during the recipient callback. */
		sint64 pending;

		/** @brief Current occupied map slot. */
		sint64 index;

		/** @brief Number of attempts remaining from the initial bounded population. */
		uint64 remaining;
	};

	/** @brief Existing deferred balance when a direct refund fails. */
	struct Refund_locals
	{
		/** @brief Previously retained QU for this recipient. */
		sint64 pending;
	};

	/** @brief Return attached QU through the existing deferred-refund mechanism. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteChangeDeveloper_input = ChangeDeveloper_input;
	using ExecuteChangeDeveloper_output = ChangeDeveloper_output;

	/** @brief Registry and event buffers for an authorized developer-beneficiary change. */
	struct ExecuteChangeDeveloper_locals
	{
		/** @brief Unused refund result; failed direct payment is recorded as a deferred credit. */
		Refund_output refundOutput;
	};

	/** @brief Shared settlement buffers for developer-role changes and invocation refunds. */
	struct ChangeDeveloper_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Tick hook arguments for automatic refund repayment. */
	struct END_TICK_locals
	{
		/** @brief Empty request for automatic refund repayment. */
		RetryRefunds_input refundInput;

		/** @brief Empty result of refund repayment. */
		RetryRefunds_output refundOutput;
	};

	/** @brief Use the dedicated public arguments for FundReserve. */
	using ExecuteFundReserve_input = FundReserve_input;

	/**
	 * @brief Result and settlement details returned by ExecuteFundReserve.
	 */
	struct ExecuteFundReserve_output
	{
		/** @brief Public operation result. */
		FundReserve_output result;

		/** @brief QU retained or burned after successful allocation. */
		sint64 fee;
	};

	/**
	 * @brief Zero-initialized working data for ExecuteFundReserve.
	 */
	struct ExecuteFundReserve_locals
	{
	};

	/**
	 * @brief Zero-initialized per-call working data for FundReserve.
	 */
	struct FundReserve_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;

		/** @brief Operation result and settlement details. */
		ExecuteFundReserve_output operationOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for PRE_ACQUIRE_SHARES.
	 */
	struct PRE_ACQUIRE_SHARES_locals
	{
		/** @brief Arguments for LogFailure. */
		LogFailure_input failureInput;

		/** @brief Result from LogFailure. */
		LogFailure_output failureOutput;

		/** @brief Arguments for Lookup. */
		Lookup_input lookupInput;

		/** @brief Result from Lookup. */
		Lookup_output lookupOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for GetItem.
	 */
	struct GetItem_locals
	{
		/** @brief Native ledger asset identified by issuer and name. */
		Asset asset;
	};

	/**
	 * @brief Zero-initialized per-call working data for GetBalance.
	 */
	struct GetBalance_locals
	{
		/** @brief Native ledger asset identified by issuer and name. */
		Asset asset;

		/** @brief Selector restricting the ledger query to the requested owner. */
		AssetOwnershipSelect ownership;

		/** @brief Selector restricting the ledger query to the requested possessor. */
		AssetPossessionSelect possession;
	};

	/**
	 * @brief Zero-initialized per-call working data for ListItems.
	 */
	struct ListItems_locals
	{
		/** @brief Item ID passed to the shared single-item query. */
		GetItem_input itemInput;

		/** @brief Complete snapshot including ledger-derived burns. */
		GetItem_output itemOutput;

		/** @brief Snapshot and explicit ID assembled for the current page entry. */
		ItemEntry entry;
	};

	/** @brief Use the public arguments for ListForSale execution. */
	using ExecuteListForSale_input = ListForSale_input;

	/** @brief Return the public result and the event for settlement. */
	struct ExecuteListForSale_output
	{
		/** @brief Market event emitted after execution succeeds. */
		MarketEvent event;

		/** @brief Public operation result. */
		ListForSale_output result;
	};

	/** @brief Working data for ListForSale execution. */
	struct ExecuteListForSale_locals
	{
		/** @brief Owner/type pair used to check rental reservations before moving native units. */
		RentalBalance_input rentalBalanceInput;
		/** @brief Effective reservations and available holdings returned by the balance helper. */
		RentalBalance_output rentalBalance;
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Data for the centralized registry commit. */
		CommitNewListing_output committedListing;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Working copy or returned listing record. */
		Listing listing;
	};

	/** @brief Public settlement data for ListForSale. */
	struct ListForSale_locals
	{
		/** @brief Execution result and market event. */
		ExecuteListForSale_output operationOutput;

		/** @brief Common fee, refund and event settlement arguments. */
		FinishCommand_input finishInput;

		/** @brief Settled operation status. */
		FinishCommand_output settled;
	};

	/** @brief Use the public arguments for CancelSale execution. */
	using ExecuteCancelSale_input = CancelSale_input;

	/** @brief Return the public result and the event for settlement. */
	struct ExecuteCancelSale_output
	{
		/** @brief Market event emitted after execution succeeds. */
		MarketEvent event;

		/** @brief Public operation result. */
		CancelSale_output result;
	};

	/** @brief Working data for CancelSale execution. */
	struct ExecuteCancelSale_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Working copy or returned listing record. */
		Listing listing;
	};

	/** @brief Public settlement data for CancelSale. */
	struct CancelSale_locals
	{
		/** @brief Execution result and market event. */
		ExecuteCancelSale_output operationOutput;

		/** @brief Common fee, refund and event settlement arguments. */
		FinishCommand_input finishInput;

		/** @brief Settled operation status. */
		FinishCommand_output settled;
	};

	/** @brief Validate immutable sale allocation settings before issuing an asset. */
	using CheckSaleDividend_input = Mint_input;
	using CheckSaleDividend_output = Code;

	/** @brief A single beneficiary's allocation; duplicate ledger records are consolidated. */
	struct SalePayment
	{
		/** @brief Beneficiary of the prepared QU payment; coincident obligations are consolidated. */
		id recipient;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
	};

	/** @brief Gross sale terms used to prepare royalty and IPO-shareholder obligations before delivery. */
	struct PrepareSalePayments_input
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Listing owner entitled to net sale proceeds; royalty to the same identity is paid with those proceeds. */
		id seller;
		/** @brief Gross QU consideration for the entire purchased quantity, before all deductions. */
		sint64 paid;
		/** @brief Creator royalty in QU computed from the full purchase price. */
		sint64 royalty;
	};

	/** @brief Snapshot fixed before delivery or recipient callbacks. At most 676 holders plus a royalty receiver. */
	struct PrepareSalePayments_output
	{
		// Exact capacity keeps the enclosing QPI locals below the 32 KiB limit.
		/** @brief Consolidated positive beneficiary payments, excluding dividends credited to QNFT itself. */
		SlowAnySizeArray<SalePayment, NUMBER_OF_COMPUTORS + 1> payments;
		/** @brief Total QU allocated to IPO shareholders after rounding down to whole QU per share. */
		sint64 shareholderAmount;
		/** @brief Sale dividends owed to QNFT itself, credited directly to protocol revenue. */
		sint64 selfAmount;
		/** @brief Number of populated entries; remaining array slots are not part of the result. */
		uint32 count;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Gross QU consideration and immutable IPO-asset allocation terms for a holder snapshot. */
	struct CalculateSaleDividends_input
	{
		/** @brief Gross QU consideration before fees and royalties; must be nonnegative. */
		sint64 paid;
		/** @brief Packed name of an issued IPO asset with exactly NUMBER_OF_COMPUTORS shares; zero disables dividends. */
		uint64 dividendAssetName;
		/** @brief Immutable share of gross sale consideration in basis points; zero disables sale dividends. */
		uint16 dividendBps;
	};

	// Reuse the final payment buffer so the holder snapshot is not copied or duplicated in locals.
	using CalculateSaleDividends_output = PrepareSalePayments_output;

	/** @brief Possession scan and recipient-consolidation buffers; no state changes during calculation. */
	struct CalculateSaleDividends_locals
	{
		/** @brief Iterator over native possession records for the selected IPO asset. */
		AssetPossessionIterator holders;
		/** @brief Issuer-zero IPO asset whose current possessors receive sale dividends. */
		Asset shares;
		/** @brief One beneficiary amount being aggregated, reserved or transferred. */
		SalePayment payment;
		/** @brief Beneficiary of the prepared QU payment; coincident obligations are consolidated. */
		id recipient;
		/** @brief Accumulated IPO possession units; must equal NUMBER_OF_COMPUTORS before allocation is accepted. */
		sint64 totalShares;
		/** @brief Units in the current possession record, bounded by the remaining IPO share total. */
		sint64 sharesHeld;
		/** @brief Whole QU assigned to each IPO share; division dust remains with the seller. */
		sint64 perShare;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
		/** @brief Zero-based position in the populated payment array or validated batch prefix. */
		uint32 index;
	};

	/** @brief Royalty aggregation and deferred-debt capacity checks before irreversible NFT delivery. */
	struct PrepareSalePayments_locals
	{
		/** @brief One beneficiary amount being aggregated, reserved or transferred. */
		SalePayment payment;
		/** @brief Gross sale price and selected IPO terms passed to the dividend calculator. */
		CalculateSaleDividends_input dividendInput;
		/** @brief Existing unpaid QU for this beneficiary; included when checking debt capacity. */
		sint64 pending;
		/** @brief Zero-based position in the populated payment array or validated batch prefix. */
		uint32 index;
		/** @brief Number of new beneficiary keys required before delivery; existing keys need no extra slot. */
		uint32 missingSlots;
	};

	/** @brief Sale-dividend log; amount includes deferred debts and excludes seller rounding dust. */
	struct SaleDividendEvent
	{
		/** @brief QNFT contract index used to identify this native log record. */
		uint32 contractIndex;
		/** @brief Market event discriminator; SaleDividend identifies the immutable sale allocation. */
		MarketAction type;
		/** @brief Monotonic shop listing identifier tying the event to a completed purchase. */
		uint64 listingId;
		/** @brief Packed name of an issued IPO asset with exactly NUMBER_OF_COMPUTORS shares; zero disables dividends. */
		uint64 dividendAssetName;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Immutable share of gross sale consideration in basis points; zero disables sale dividends. */
		uint32 dividendBps;
		/** @brief Zero terminator required by native custom logging. */
		sint8 _terminator;
	};

	using EmitSaleDividend_input = SaleDividendEvent;
	using EmitSaleDividend_output = NoData;

	/** @brief Public journal buffers for the purchase allocation event. */
	struct EmitSaleDividend_locals
	{
		/** @brief Event payload appended to the bounded public journal. */
		AppendEvent_input journalInput;
		/** @brief Empty acknowledgement of journal publication. */
		AppendEvent_output journalOutput;
	};

	/** @brief Reuse the public purchase arguments for side-effect-free validation. */
	using ValidateBuy_input = Buy_input;

	/** @brief Validated escrow records and gross purchase amounts; Code::Ok permits payment preparation. */
	struct ValidateBuy_output
	{
		/** @brief Item snapshot whose metadata revision and allocation rates were checked. */
		Item item;
		/** @brief Listing snapshot with sufficient unsold quantity at the expected price. */
		Listing listing;
		/** @brief Gross price and deductions in QU; rejected requests leave amounts zero. */
		Buy_output result;
	};

	/** @brief Native balance snapshot used to verify backing before delivery. */
	struct ValidateBuy_locals
	{
		/** @brief QNFT incoming and outgoing QU counters, including the invocation reward. */
		Entity contractBalance;
	};

	/** @brief Use the dedicated public arguments for Buy. */
	using ExecuteBuy_input = Buy_input;

	/** @brief Return purchase amounts and the event to emit during shared settlement. */
	struct ExecuteBuy_output
	{
		/** @brief Purchase event emitted only by the public settlement step. */
		MarketEvent event;

		/** @brief Purchase price, fee, royalty, shareholder allocation and status. */
		Buy_output result;
	};

	/**
	 * @brief Zero-initialized working data for ExecuteBuy.
	 */
	struct ExecuteBuy_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Data for the centralized registry commit. */
		CommitListingFill_input commitInput;

		/** @brief Empty listing-fill commit result. */
		CommitListingFill_output commitOutput;

		/** @brief Working copy or returned listing record. */
		Listing listing;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Checked listing, metadata, payment and error result before any side effects. */
		ValidateBuy_output validation;

		/** @brief Validated item, seller and gross amounts used to prepare beneficiary obligations. */
		PrepareSalePayments_input paymentInput;
		/** @brief Prepared dividend/royalty obligations and capacity-validation result. */
		PrepareSalePayments_output paymentOutput;
		/** @brief One beneficiary amount being aggregated, reserved or transferred. */
		SalePayment payment;
		/** @brief Immutable allocation terms and actual distributed QU published for this purchase. */
		SaleDividendEvent dividendEvent;
		/** @brief Empty acknowledgement of the sale-dividend event. */
		EmitSaleDividend_output dividendEventOutput;

		/** @brief Net seller proceeds in QU after shop fee, royalty and sale dividends. */
		sint64 sellerAmount;
		/** @brief Existing beneficiary credit, kept separate from protocol revenue. */
		sint64 pendingRoyalty;
		/** @brief Zero-based cursor over consolidated beneficiary payments. */
		uint32 paymentIndex;
	};

	/**
	 * @brief Zero-initialized per-call working data for Buy.
	 */
	struct Buy_locals
	{
		/** @brief Common fee, refund and event settlement arguments. */
		FinishCommand_input finishInput;

		/** @brief Settled operation status. */
		FinishCommand_output settled;

		/** @brief Purchase result and its committed market event. */
		ExecuteBuy_output operationOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for ListListings.
	 */
	struct ListListings_locals
	{
		/** @brief Bounded loop or pagination cursor. */
		uint32 i;
	};

	/** @brief Distribution uses only persistent fee pools and configuration. */
	using DistributeFees_input = NoData;

	/** @brief Distribution reports failures through logs and retains the pool for retry. */
	using DistributeFees_output = NoData;

	/**
	 * @brief Zero-initialized per-call working data for DistributeFees.
	 */
	struct DistributeFees_locals
	{
		/** @brief Arguments for LogFailure. */
		LogFailure_input failureInput;

		/** @brief Result from LogFailure. */
		LogFailure_output failureOutput;

		/** @brief QNFT IPO share asset used for the dividend snapshot. */
		Asset shares;

		/** @brief Iterator over current IPO share possessors. */
		AssetPossessionIterator shareholders;

		/** @brief Developer allocation in QU, rounded down at the configured rate. */
		sint64 developerAmount;

		/** @brief Shareholder allocation in QU, including prior rounding carry. */
		sint64 shareholderAmount;

		/** @brief Whole QU distributed per IPO share. */
		sint64 perShare;

		/** @brief QU allocated to reserve burn, including split rounding remainder. */
		sint64 burnAmount;

		/** @brief Total IPO shares observed during allocation preflight. */
		sint64 totalShares;

		/** @brief Log payload assembled for this execution. */
		EmitMarket_input event;

		/** @brief Empty information-log result. */
		EmitMarket_output eventOutput;

		/** @brief QU balance used to preflight all revenue liabilities. */
		Entity contractBalance;
	};

	/** @brief Empty request for end-of-epoch storage compaction. */
	using CleanupEpochData_input = NoData;

	/** @brief Cleanup completes without a return value. */
	using CleanupEpochData_output = NoData;

	/**
	 * @brief Epoch hook arguments for fee distribution and approval maintenance.
	 */
	struct END_EPOCH_locals
	{
		/** @brief Empty request; allocation reads state. */
		DistributeFees_input distributionInput;

		/** @brief Empty result; failures are logged. */
		DistributeFees_output distributionOutput;
	};

	/**
	 * @brief Submit multiple changes through the native MultiVariablesYesNo voting type.
	 */
	struct SetFeeProposalBatch_input
	{
		/** @brief Human-readable proposal explanation, zero-terminated. */
		Array<uint8, QNFT_PROPOSAL_URL_CAPACITY> url;

		/** @brief Populated prefix followed by zero entries. */
		Array<FeeChange, QNFT_FEE_BATCH_CAPACITY> changes;

		/** @brief Nonzero marker for the current epoch; cancellation uses the standard procedure. */
		uint16 epoch;

		/** @brief Number of distinct governed parameters. */
		uint16 count;
	};

	/** @brief Native proposal slot, or INVALID_PROPOSAL_INDEX if rejected. */
	using SetFeeProposalBatch_output = uint16;

	/**
	 * @brief Read the exact batch attached to a native proposal slot.
	 */
	struct GetFeeProposalBatch_input
	{
		/** @brief Slot of a current or retained historical proposal. */
		uint16 proposalIndex;
	};

	/**
	 * @brief Batch contents and native ballot version; NotFound if slot is not a valid batch.
	 */
	struct GetFeeProposalBatch_output
	{
		/** @brief Hash-verified changes, zero on failure. */
		FeeProposalBatch batch;

		/** @brief Version required by the standard ballot. */
		uint32 proposalTick;

		/** @brief Ok or NotFound. */
		Code code;
	};

	/** @brief Validate a complete batch before any parameter is changed. */
	using ValidateFeeBatch_input = FeeProposalBatch;

	/**
	 * @brief Entire-batch validation result.
	 */
	struct ValidateFeeBatch_output
	{
		/** @brief True only when every entry and the combined split are valid. */
		bool valid;
	};

	/**
	 * @brief Validation scratch values; no contract-local stack variables.
	 */
	struct ValidateFeeBatch_locals
	{
		/** @brief Proposed final protocol rental rate in basis points, initialized from current state. */
		sint64 rentalProtocol;
		/** @brief Proposed final creator rental rate in basis points, initialized from current state. */
		sint64 rentalCreator;
		/** @brief Proposed shareholder BPS when present. */
		sint64 shareholder;
		/** @brief Proposed burn BPS when present. */
		sint64 burn;
		/** @brief Current entry index. */
		uint32 i;

		/** @brief Bit mask used to reject duplicate variables. */
		uint32 seen;
	};

	/** @brief Apply one previously validated entry; a batch validates all entries first. */
	using ApplyFeeChange_input = FeeChange;

	/** @brief Applying a validated scalar cannot fail or invoke external code. */
	using ApplyFeeChange_output = NoData;

	/** @brief Standard QPI proposal input, including epoch=0 cancellation. */
	using SetShareholderProposal_input = ProposalDataT;

	/** @brief Proposal slot or INVALID_PROPOSAL_INDEX on rejection. */
	using SetShareholderProposal_output = uint16;

	/** @brief Standard QPI multi-vote payload with version tick and weighted options. */
	using SetShareholderVotes_input = ProposalMultiVoteDataV1;

	/** @brief Whether the QPI voting engine accepted the ballot. */
	using SetShareholderVotes_output = bit;

	/**
	 * @brief Proposal lookup by slot.
	 */
	struct GetShareholderProposal_input
	{
		/** @brief Slot in the QPI proposal registry. */
		uint16 proposalIndex;
	};

	/**
	 * @brief Standard QPI proposal response; spelling matches the shared interface.
	 */
	struct GetShareholderProposal_output
	{
		/** @brief Stored proposal or type=0 on failure. */
		ProposalDataT proposal;

		/** @brief Account that owns this proposal slot. */
		id proposerPubicKey;
	};

	/**
	 * @brief Standard QPI cursor for active or finished proposals.
	 */
	struct GetShareholderProposalIndices_input
	{
		/** @brief True selects the current epoch; false selects earlier epochs. */
		bit activeProposals;

		/** @brief Previous slot or -1 to start enumeration. */
		sint32 prevProposalIndex;
	};

	/**
	 * @brief Standard QPI proposal index page.
	 */
	struct GetShareholderProposalIndices_output
	{
		/** @brief Number of populated indices. */
		uint16 numOfIndices;

		/** @brief Proposal slots returned in ascending order. */
		Array<uint16, QNFT_PROPOSAL_INDEX_CAPACITY> indices;
	};

	/** @brief Standard empty query for proposal and vote invocation charges. */
	using GetShareholderProposalFees_input = NoData;

	/**
	 * @brief Standard QPI governance fees, both in QU.
	 */
	struct GetShareholderProposalFees_output
	{
		/** @brief Charge for creating, replacing or cancelling a proposal. */
		sint64 setProposalFee;

		/** @brief Charge for casting votes; currently zero. */
		sint64 setVoteFee;
	};

	/**
	 * @brief Standard ballot lookup keyed by voter and proposal slot.
	 */
	struct GetShareholderVotes_input
	{
		/** @brief Snapshot voter whose ballot is requested. */
		id voter;

		/** @brief Slot containing the proposal. */
		uint16 proposalIndex;
	};

	/** @brief Standard QPI ballot response. */
	using GetShareholderVotes_output = ProposalMultiVoteDataV1;

	/**
	 * @brief Standard voting summary query.
	 */
	struct GetShareholderVotingResults_input
	{
		/** @brief Slot whose weighted results are requested. */
		uint16 proposalIndex;
	};

	/** @brief Standard QPI vote totals and quorum interpretation. */
	using GetShareholderVotingResults_output = ProposalSummarizedVotingDataV1;

	/** @brief Empty query for current revenue shares and governance configuration. */
	using GetGovernanceConfig_input = NoData;

	/**
	 * @brief Read-only configuration; developer share has no voting variable ID.
	 */
	struct GetGovernanceConfig_output
	{
		/** @brief Fixed developer share of revenue in basis points. */
		uint32 developerFeeBPS;

		/** @brief Current dividend share of revenue in basis points. */
		uint32 shareholderFeeBPS;

		/** @brief Current burn share of revenue in basis points. */
		uint32 burnFeeBPS;

		/** @brief Last epoch whose proposals were applied. */
		uint32 lastGovernanceEpoch;

		/** @brief Number of simultaneously occupied QPI proposal slots. */
		uint32 proposalSlots;
	};

	/**
	 * @brief Governance diagnostic payload; required logger prefix precedes actor and values.
	 */
	struct GovernanceEvent
	{
		/** @brief Required first field identifying QNFT. */
		uint32 contractIndex;

		/** @brief Proposal, vote or application action. */
		GovernanceAction action;

		/** @brief Invocator, or proposer for an applied proposal. */
		id actor;

		/** @brief Proposed variable; meaningful for proposal/application events. */
		FeeVariable variable;

		/** @brief Proposed QU or basis-point value; zero for vote events. */
		sint64 value;

		/** @brief QPI slot; INVALID_PROPOSAL_INDEX for rejected submissions. */
		uint32 proposalIndex;

		/** @brief Ok on success, otherwise the wrapper's rejection category. */
		Code code;

		/** @brief Payload boundary, excluded from the serialized log. */
		sint8 _terminator;
	};

	/** @brief Payload for the GovernanceEvent information log. */
	using EmitGovernance_input = GovernanceEvent;

	/** @brief No result is returned by EmitGovernance. */
	using EmitGovernance_output = NoData;

	/** @brief Use the native public arguments for SetFeeProposalBatch execution. */
	using ExecuteSetFeeProposalBatch_input = SetFeeProposalBatch_input;

	/** @brief Native result and governance diagnostic for public settlement. */
	struct ExecuteSetFeeProposalBatch_output
	{
		/** @brief Acceptance or rejection event. */
		EmitGovernance_input event;

		/** @brief Native proposal index or ballot acceptance result. */
		SetFeeProposalBatch_output result;

		/** @brief Protocol fee for an accepted, non-cancellation proposal. */
		sint64 fee;
	};

	/** @brief Validation and mutation working data for SetFeeProposalBatch. */
	using ApplySetFeeProposalBatch_input = ExecuteSetFeeProposalBatch_input;
	using ApplySetFeeProposalBatch_output = ExecuteSetFeeProposalBatch_output;

	/** @brief Native proposal and validated canonical payload buffers before submission. */
	struct ApplySetFeeProposalBatch_locals
	{
		/** @brief Native proposal carrying the batch digest in its 32-byte payload. */
		ProposalDataT proposal;

		/** @brief Canonical hashed sidecar. */
		FeeProposalBatch batch;

		/** @brief Full validation before native proposal replacement. */
		ValidateFeeBatch_output validation;
	};

	/** @brief Public governance settlement data. */
	struct ExecuteSetFeeProposalBatch_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetFeeProposalBatch_output operationOutput;

		/** @brief Defined below; submission diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;

		/** @brief Entry index used for successful per-variable logs. */
		uint32 i;
	};

	/** @brief Batch proposal execution result and shared payment settlement buffers. */
	struct SetFeeProposalBatch_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Execution result before finalization. */
		ExecuteSetFeeProposalBatch_output operationOutput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/**
	 * @brief Lookup scratch data for digest verification.
	 */
	struct GetFeeProposalBatch_locals
	{
		/** @brief Native record being matched against its sidecar. */
		ProposalDataT proposal;

		/** @brief Candidate sidecar, exposed only after digest verification. */
		FeeProposalBatch batch;
	};

	/** @brief Use the native public arguments for SetShareholderProposal execution. */
	using ExecuteSetShareholderProposal_input = SetShareholderProposal_input;

	/** @brief Zeroed batch used to invalidate payloads when a proposal slot is reused. */
	using ApplySetShareholderProposal_input = ExecuteSetShareholderProposal_input;
	/** @brief Native proposal result and diagnostic forwarded to shared payment settlement. */
	struct ExecuteSetShareholderProposal_output;
	using ApplySetShareholderProposal_output = ExecuteSetShareholderProposal_output;

	/** @brief Scalar proposal validation and sidecar invalidation buffers. */
	struct ApplySetShareholderProposal_locals
	{
		/** @brief Single-variable batch checked by the same rules as multi-variable fee proposals. */
		FeeProposalBatch validationBatch;
		/** @brief Zeroed sidecar written when a scalar proposal replaces or cancels a batch proposal. */
		FeeProposalBatch emptyBatch;
		/** @brief Preflight result; mutation may proceed only after validation succeeds. */
		ValidateFeeBatch_output validation;
	};

	/** @brief Native result and governance diagnostic for public settlement. */
	struct ExecuteSetShareholderProposal_output
	{
		/** @brief Acceptance or rejection event. */
		EmitGovernance_input event;

		/** @brief Native proposal index or ballot acceptance result. */
		SetShareholderProposal_output result;

		/** @brief Protocol fee for an accepted, non-cancellation proposal. */
		sint64 fee;
	};

	/** @brief Public governance settlement data. */
	struct ExecuteSetShareholderProposal_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetShareholderProposal_output operationOutput;

		/** @brief Proposal submission or rejection diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;
	};

	/** @brief Scalar shareholder proposal execution and refund buffers. */
	struct SetShareholderProposal_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Execution result before finalization. */
		ExecuteSetShareholderProposal_output operationOutput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Use the native public arguments for SetShareholderVotes execution. */
	using ExecuteSetShareholderVotes_input = SetShareholderVotes_input;

	/** @brief Native result and governance diagnostic for public settlement. */
	struct ExecuteSetShareholderVotes_output
	{
		/** @brief Acceptance or rejection event. */
		EmitGovernance_input event;

		/** @brief Native proposal index or ballot acceptance result. */
		SetShareholderVotes_output result;

		/** @brief Protocol fee for an accepted, non-cancellation proposal. */
		sint64 fee;
	};

	/** @brief Validation and mutation working data for SetShareholderVotes. */
	using ApplySetShareholderVotes_input = ExecuteSetShareholderVotes_input;
	using ApplySetShareholderVotes_output = ExecuteSetShareholderVotes_output;

	/** @brief Ballot submission and authorization diagnostics for native shareholder voting. */
	struct ApplySetShareholderVotes_locals
	{
		/** @brief Index in the eight-entry ballot. */
		uint32 i;

		/** @brief Bounded total requested weight before handing input to QPI. */
		uint32 voteCount;
	};

	/** @brief Public governance settlement data. */
	struct ExecuteSetShareholderVotes_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetShareholderVotes_output operationOutput;

		/** @brief Vote acceptance or rejection diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;
	};

	/** @brief Ballot execution and shared refund settlement buffers. */
	struct SetShareholderVotes_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Execution result before finalization. */
		ExecuteSetShareholderVotes_output operationOutput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/**
	 * @brief Typed copy of the core's shareholder proposal notification buffer.
	 */
	struct SET_SHAREHOLDER_PROPOSAL_locals
	{
		/** @brief Standard proposal decoded from notification input. */
		SetShareholderProposal_input userProcInput;
	};

	/** @brief Finalization uses the current epoch and proposal state only. */
	using FinalizeShareholderStateVarProposals_input = NoData;

	/** @brief Results are recorded through updated fee fields and governance logs. */
	using FinalizeShareholderStateVarProposals_output = NoData;

	/**
	 * @brief Working data for native QPI quorum evaluation and applying accepted variables.
	 */
	struct FinalizeShareholderStateVarProposals_locals
	{
		/** @brief Proposal being evaluated. */
		ProposalDataT proposal;

		/** @brief Native weighted votes and quorum result. */
		ProposalSummarizedVotingDataV1 results;

		/** @brief Applied variable or failed validation log. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;

		/** @brief Ascending slot cursor; starts at -1. */
		sint32 proposalIndex;

		/** @brief Entire candidate set validated before application. */
		FeeProposalBatch batch;

		/** @brief Atomic preflight result. */
		ValidateFeeBatch_output validation;

		/** @brief Mutable copy passed through CALL. */
		ApplyFeeChange_input applyInput;

		/** @brief Empty scalar application result. */
		ApplyFeeChange_output applyOutput;

		/** @brief Current entry during application and logging. */
		uint32 i;
	};

	/** @brief Zeroed persistent-event payload for EmitCommand. */
	struct EmitCommand_locals
	{
		/** @brief Record populated before optional LOG_INFO mutates its input prefix. */
		AppendEvent_input journalInput;

		/** @brief Empty append result. */
		AppendEvent_output journalOutput;
	};

	/** @brief Zeroed persistent-event payload for EmitMarket. */
	struct EmitMarket_locals
	{
		/** @brief Record populated before optional LOG_INFO mutates its input prefix. */
		AppendEvent_input journalInput;

		/** @brief Empty append result. */
		AppendEvent_output journalOutput;
	};

	/** @brief Zeroed persistent-event payload for EmitGovernance. */
	struct EmitGovernance_locals
	{
		/** @brief Record populated before optional LOG_INFO mutates its input prefix. */
		AppendEvent_input journalInput;

		/** @brief Empty append result. */
		AppendEvent_output journalOutput;
	};

	/** @brief Zeroed persistent-event payload for EmitTransfer. */
	struct EmitTransfer_locals
	{
		/** @brief Record populated before optional LOG_INFO mutates its input prefix. */
		AppendEvent_input journalInput;

		/** @brief Empty append result. */
		AppendEvent_output journalOutput;
	};

	/** @brief Zeroed persistent-event payload for EmitUri. */
	struct EmitUri_locals
	{
		/** @brief Record populated before optional LOG_INFO mutates its input prefix. */
		AppendEvent_input journalInput;

		/** @brief Empty append result. */
		AppendEvent_output journalOutput;
	};

	/** @brief Owner/operator pair for a read-only permission query. */
	using IsApprovedForAll_input = OperatorKey;

	/** @brief Owner-wide permission query result; independent of any token balance. */
	struct IsApprovedForAll_output
	{
		/** @brief Permission flag: 1 grants authority and 0 revokes it; other input values are invalid. */
		bit approved;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Grant or revoke authority over all present and future holdings of the caller. */
	struct SetApprovalForAll_input
	{
		/** @brief Identity receiving delegated authority; cannot be NULL_ID or the owner when granting permission. */
		id operatorId;
		/** @brief Permission flag: 1 grants authority and 0 revokes it; other input values are invalid. */
		bit approved;
	};

	/** @brief Permission key, dedicated event and fee-only settlement scratch data. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteSetApprovalForAll_input = SetApprovalForAll_input;
	using ExecuteSetApprovalForAll_output = OperationResult;

	/** @brief Owner/operator key and approval journal buffers for granting or revoking authority. */
	struct ExecuteSetApprovalForAll_locals
	{
		/** @brief Owner/operator tuple whose hash identifies the permission entry. */
		OperatorKey key;
		/** @brief Journal record prepared only for a committed state change. */
		AppendEvent_input event;
		/** @brief K12 hash of the owner/operator permission key. */
		id digest;
		/** @brief Unused result of appending the committed journal record. */
		AppendEvent_output eventOutput;
	};

	/** @brief Grant-fee and refund settlement for owner-wide operator authorization. */
	struct SetApprovalForAll_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Native protocol version and capability limits; does not advertise Ethereum ABI compatibility. */
	struct GetProtocolInfo_output
	{
		/** @brief Bit mask of native integration features; unknown bits must be ignored. */
		uint64 capabilities;
		/** @brief Largest allocatable token ID; does not imply that this ID has been issued. */
		uint64 maxTokenId;
		/** @brief Protocol generation identifying incompatible native interfaces. */
		uint32 majorVersion;
		/** @brief Feature revision within the reported native protocol generation. */
		uint32 minorVersion;
		/** @brief Qubic contract index used for ABI calls and native asset management. */
		uint32 contractIndex;
		/** @brief Number of bytes in a Qubic identity, currently 32. */
		uint32 addressBytes;
		/** @brief Maximum URI payload length in bytes, excluding its terminating zero. */
		uint32 maxUriBytes;
	};

	/** @brief Full sale consideration and token type for a currency-independent royalty quote. */
	struct RoyaltyInfo_input
	{
		/** @brief Nonnegative full sale price in caller-selected settlement units, before any marketplace fee. */
		sint64 salePrice;
		/** @brief Permanent QNFT token-type ID; query-specific rules define the result for an unallocated ID. */
		uint32 itemId;
	};

	/** @brief Royalty beneficiary and rounded-down amount; reading this result transfers no funds. */
	struct RoyaltyInfo_output
	{
		/** @brief Royalty beneficiary, possibly NULL_ID when royalties are disabled. */
		id receiver;
		/** @brief Royalty in salePrice units, rounded down; this quote does not itself pay the beneficiary. */
		sint64 royaltyAmount;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Full sale price in QU and item ID; the same 16-byte input layout as RoyaltyInfo. */
	using DividendInfo_input = RoyaltyInfo_input;

	/** @brief Immutable shareholder allocation quote; reading it neither pays nor snapshots holders. */
	struct DividendInfo_output
	{
		/** @brief Selected IPO share name under NULL_ID; zero when allocation is disabled. */
		uint64 dividendAssetName;
		/** @brief Total QU allocated to 676 shares, excluding rounding dust retained by the seller. */
		sint64 shareholderAmount;
		/** @brief Whole QU owed per share at the supplied gross sale price. */
		sint64 amountPerShare;
		/** @brief Ok, InvalidInput for negative prices, NotFound for absent/burned items, or Conflict for invalid terms. */
		Code code;
		/** @brief Immutable allocation rate in basis points, including when the amount rounds to zero. */
		uint16 dividendBps;
	};

	using SetApprovalForAll_output = OperationResult;
	using GetProtocolInfo_input = NoData;

	/** @brief Scratch for validating a registered item's owner, operator and available holdings. */
	struct ValidateOwnedUnits_locals
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Effective reservations and available holdings returned by the balance helper. */
		RentalBalance_output rentalBalance;
		/** @brief Owner/type pair used to check rental reservations before moving native units. */
		RentalBalance_input rentalBalanceInput;
		/** @brief Owner and item pair checked against the invocator and persistent operator approval. */
		isOwnerOrApprovedOperator_input authorizationInput;
		/** @brief Locally owned and possessed native units available under QNFT management. */
		sint64 balance;
		/** @brief Whether the invocator may spend the requested owner holdings. */
		isOwnerOrApprovedOperator_output authorizationOutput;
	};

	using ValidateOwnedUnits_input = Burn_input;
	using ValidateOwnedUnits_output = OperationResult;

	using ValidateTransfer_input = TransferFrom_input;

	using ValidateTransfer_output = OperationResult;

	/** @brief Item policy snapshot and authorized available-unit validation request. */
	struct ValidateTransfer_locals
	{
		/** @brief Registry snapshot used for validation or committed after the corresponding native operation. */
		Item item;
		/** @brief Requested owner/type quantity passed to shared balance and authority validation. */
		ValidateOwnedUnits_input holdings;
	};

	using ValidateMint_input = Mint_input;

	using ValidateMint_output = ExecuteMint_output;

	/** @brief Collection authorization, metadata syntax and immutable dividend validation buffers. */
	struct ValidateMint_locals
	{
		/** @brief Collection snapshot used for creator/administrator authorization and cumulative issuance quotas. */
		Collection collection;
		/** @brief URI validation scratch; checks termination, printable bytes and zero trailing storage. */
		MetadataValidationScratch metadataScratch;
		/** @brief Validation result for the immutable sale-dividend asset and allocation terms. */
		Code dividendCode;
	};

	/** @brief Native multi-token balance across all managers, including externally held units. */
	struct BalanceOf_input
	{
		/** @brief Account whose ownership or authority is queried or acted upon; NULL_ID is invalid for balance and movement requests. */
		id owner;
		/** @brief Permanent QNFT token-type ID; query-specific rules define the result for an unallocated ID. */
		uint32 itemId;
	};

	/** @brief Ownership units across every possessor and manager; unknown token IDs have zero balance. */
	struct BalanceOf_output
	{
		/** @brief Owned units across all managers and possessors; zero for an unknown token ID. */
		sint64 balance;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Native asset identity used to query ownership without cached balances. */
	struct BalanceOf_locals
	{
		/** @brief QNFT-issued asset identity resolved from the permanent token-type registry. */
		Asset asset;
	};

	/** @brief Ordered ownership queries; only the first count entries are used. */
	struct BalanceOfBatch_input
	{
		/** @brief Ordered owner/token pairs; unknown token IDs yield zero ownership units. */
		Array<BalanceOf_input, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Number of owner/token pairs to query, from 0 through 16. */
		uint32 count;
	};

	/** @brief Ordered balances; invalid input clears the entire response rather than exposing a prefix. */
	struct BalanceOfBatch_output
	{
		/** @brief Owned-unit results in the same order as the input queries; only count entries are valid. */
		Array<sint64, QNFT_TOKEN_BATCH_CAPACITY> balances;
		/** @brief Number of valid balances returned; zero when any input pair is invalid. */
		uint32 count;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Single-entry query/result buffers reused while building the batch response. */
	struct BalanceOfBatch_locals
	{
		/** @brief One owner/token pair copied from the ordered batch. */
		BalanceOf_input query;
		/** @brief One ownership query response, reset before reading the next input pair. */
		BalanceOf_output result;
		/** @brief Zero-based index of the current input entry. */
		uint32 i;
	};

	using TotalSupply_input = GetItem_input;

	/** @brief Global live supply from the ledger; unknown or fully burned IDs have zero units. */
	struct TotalSupply_output
	{
		/** @brief Current global live units across all managers, excluding burned units. */
		sint64 supply;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Native asset identity used to query supply across all managers. */
	struct TotalSupply_locals
	{
		/** @brief QNFT-issued asset identity resolved from the permanent token-type registry. */
		Asset asset;
	};

	using URI_input = GetItem_input;

	/** @brief Exact registered metadata URI; remains available after a full burn. */
	struct URI_output
	{
		/** @brief Zero-terminated ASCII URI, at most 255 payload bytes; unused tail bytes are zero. */
		Array<uint8, QNFT_METADATA_URI_CAPACITY> uri;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Live-supply validation and effective royalty rate for a read-only quote. */
	struct RoyaltyInfo_locals
	{
		/** @brief Global live-supply response used to reject unknown or fully burned types. */
		TotalSupply_output supply;
		/** @brief Token-type ID whose live supply is checked before quoting royalties. */
		TotalSupply_input query;
		/** @brief Effective royalty rate in basis points, from 0 through 10000; absent beneficiary means zero. */
		uint16 rate;
	};

	/** @brief Live-supply validation for the shareholder allocation quote. */
	struct DividendInfo_locals
	{
		/** @brief Live global supply result used to reject unknown or fully burned token types. */
		TotalSupply_output supply;
		/** @brief Item identifier passed to the live ledger supply query. */
		TotalSupply_input query;
	};

	using GetAssetState_input = GetItem_input;

	/** @brief Live units globally and under QNFT ownership management. */
	struct GetAssetState_output
	{
		/** @brief Current global live units across all managers, excluding burned units. */
		sint64 supply;
		/** @brief Live units whose ownership is managed by QNFT, irrespective of possession manager. */
		sint64 managedSupply;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	/** @brief Native asset identity for global and locally managed supply queries. */
	struct GetAssetState_locals
	{
		/** @brief QNFT-issued asset identity resolved from the permanent token-type registry. */
		Asset asset;
	};

	/** @brief One positive possession record with its owner, possessor and managing contracts. */
	struct HolderEntry
	{
		/** @brief Account whose ownership or authority is queried or acted upon; NULL_ID is invalid for balance and movement requests. */
		id owner;
		/** @brief Account possessing this ledger record; may differ from its owner. */
		id possessor;
		/** @brief Positive number of units represented by this possession record. */
		sint64 quantity;
		/** @brief Contract index managing ownership of this ledger record. */
		uint16 ownershipManager;
		/** @brief Contract index managing possession of this ledger record. */
		uint16 possessionManager;
	};

	/** @brief Bounded native-ledger page request; cursor includes historical zero-balance records. */
	struct ListHolders_input
	{
		/** @brief Permanent QNFT token-type ID; query-specific rules define the result for an unallocated ID. */
		uint32 itemId;
		/** @brief Number of ledger records to skip, including historical zero records; zero starts at the beginning. */
		uint32 cursor;
		/** @brief Maximum ledger records scanned, from 1 through QNFT_TOKEN_BATCH_CAPACITY. */
		uint32 limit;
		/** @brief Required snapshot tick; zero disables the tick check and does not freeze intra-tick state. */
		uint32 expectedTick;
	};

	/** @brief Positive holdings encountered within a bounded ledger scan and its continuation cursor. */
	struct ListHolders_output
	{
		/** @brief Positive possession records encountered in this scan; only count entries are populated. */
		Array<HolderEntry, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Number of positive holdings returned, which may be less than the number of scanned records. */
		uint32 count;
		/** @brief Ledger-record offset for continuation; includes scanned zero-balance entries. */
		uint32 nextCursor;
		/** @brief Network tick observed by the query; not an immutable intra-tick snapshot identifier. */
		uint32 tick;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
		/** @brief One when the ledger iterator reached its end; zero means more records may remain. */
		bit done;
	};

	/** @brief Ledger iterator and scan counters; no holdings are cached in contract state. */
	struct ListHolders_locals
	{
		/** @brief Native possession iterator spanning all ownership and possession managers. */
		AssetPossessionIterator iterator;
		/** @brief One positive holding copied from the current ledger record into the response. */
		HolderEntry entry;
		/** @brief QNFT-issued asset identity resolved from the permanent token-type registry. */
		Asset asset;
		/** @brief Number of native ledger records skipped while resolving the requested cursor. */
		uint32 skipped;
		/** @brief Records visited on this page, including records omitted for having zero units. */
		uint32 scanned;
	};

	/** @brief One token type and the quantity requested by a batch operation. */
	struct TokenAmount
	{
		/** @brief Native asset units; the operation validates its allowed range. */
		sint64 quantity;
		/** @brief Token-type identifier; repeated IDs are checked cumulatively. */
		uint32 itemId;
	};

	/** @brief Ordered bounded native batch; all entries use the same owner and receiver. */
	struct BatchTransfer_input
	{
		/** @brief Account whose ownership or authority is queried or acted upon; NULL_ID is invalid for balance and movement requests. */
		id owner;
		/** @brief Account receiving the requested units; movement entry points validate destination restrictions. */
		id receiver;
		/** @brief Ordered token quantities; only the first count slots are used. */
		Array<TokenAmount, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Number of ordered transfer entries, from 0 through 16. */
		uint32 count;
	};

	/** @brief Entries before completed are committed; attempted includes a failing entry when execution started.
	 * @note Pending denotes partial execution or reserved mint delivery, never an EVM-style rollback.
	 * Unattempted results are unspecified. failureIndex equals count when no error occurred.
	 */
	struct Batch_output
	{
		/** @brief Per-attempt results in input order; unattempted entries are unspecified. */
		Array<OperationResult, QNFT_TOKEN_BATCH_CAPACITY> results;
		/** @brief Total QU charged for committed entries only; zero when preflight rejects the request. */
		sint64 fee;
		/** @brief Committed entries, including issued editions with pending delivery; a failure may leave a committed prefix. */
		uint32 completed;
		/** @brief Execution attempts, including a failing execution entry; zero when preflight rejects the request. */
		uint32 attempted;
		/** @brief Zero-based failing entry, or original input count for a request-wide error or no error. */
		uint32 failureIndex;
		/** @brief Failure reason; Ok means no entry or request-wide failure was reported. */
		Code failureCode;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
	};

	using BatchTransfer_output = Batch_output;

	/** @brief Per-entry validation and cumulative duplicate-ID checks before ordered transfers. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteBatchTransfer_input = BatchTransfer_input;

	using ExecuteBatchTransfer_output = BatchTransfer_output;

	/** @brief Validated batch prefix whose repeated item quantities must fit the owner's available balance. */
	struct ValidateBatchBalance_input
	{
		/** @brief Ordered token quantities; only the validated prefix through the current index is considered. */
		Array<TokenAmount, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Native owner backing the units; rental usage does not transfer ownership. */
		id owner;
		/** @brief Zero-based position in the populated payment array or validated batch prefix. */
		uint32 index;
	};

	using ValidateBatchBalance_output = Code;

	/** @brief Cumulative duplicate-ID quantities and rental reservations for batch preflight. */
	struct ValidateBatchBalance_locals
	{
		/** @brief Effective reservations and available holdings returned by the balance helper. */
		RentalBalance_output rentalBalance;
		/** @brief Owner/type pair used to check rental reservations before moving native units. */
		RentalBalance_input rentalBalanceInput;
		/** @brief Total requested units for this token ID in the validated batch prefix. */
		sint64 cumulative;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Earlier entry cursor used to accumulate duplicate IDs or collection issuance quotas. */
		uint32 j;
	};

	/** @brief Whole-batch preflight and ordered transfer buffers; committed prefixes survive core failure. */
	struct ExecuteBatchTransfer_locals
	{
		/** @brief Participant/type holdings or validated batch prefix supplied to the balance checker. */
		ValidateBatchBalance_input balanceInput;

		/** @brief Current entry passed to validation and then execution; no later entries are committed after failure. */
		TransferFrom_input request;
		/** @brief Current entry status and execution outputs; reset before each validation or execution call. */
		OperationResult result;
		/** @brief Total QU needed for all requested transfers, checked before any movement. */
		sint64 requiredFee;
		/** @brief Zero-based index of the current input entry. */
		uint32 i;
		/** @brief Earlier input index used to detect duplicates or aggregate quantities for the same type or collection. */
		uint32 j;
		/** @brief Combined-balance validation status; a failure prevents batch execution. */
		ValidateBatchBalance_output balanceCode;
	};

	/** @brief Aggregate transfer fee, refund and batch-result settlement buffers. */
	struct BatchTransfer_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Up to two independent edition definitions; each successful definition consumes one token ID. */
	struct MintBatch_input

	{

		/** @brief Independent edition definitions; unused slots are ignored. */
		Array<Mint_input, QNFT_MINT_BATCH_CAPACITY> entries;

		/** @brief Number of independent edition definitions, from 0 through 2. */
		uint32 count;
	};

	using MintBatch_output = Batch_output;

	/** @brief Internal bounded edition definitions shared by full and compact mint wrappers. */
	struct MintMany_input
	{
		/** @brief Expanded edition definitions; only the first count entries are validated and issued. */
		Array<Mint_input, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Number of expanded definitions, from 0 through 16. */
		uint32 count;
	};

	using MintMany_output = Batch_output;

	/** @brief Expanded issuance request and refund buffer for request-wide validation failures. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteMintBatch_input = MintBatch_input;

	using ExecuteMintBatch_output = MintBatch_output;

	/** @brief Expanded per-edition request passed to shared mint preflight and execution. */
	struct ExecuteMintBatch_locals
	{
		/** @brief Expanded definitions forwarded to the shared issuance engine after validating input shape. */
		MintMany_input batch;
		/** @brief Zero-based index of the current input entry. */
		uint32 i;
	};

	/** @brief Aggregate mint execution, issuance fees and refund settlement buffers. */
	struct MintBatch_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Edition validation, aggregate collection quota and committed-fee accounting buffers. */
	struct MintCollectionQuotaScratch
	{
		/** @brief Cumulative new units for this collection in the mint batch prefix. */
		sint64 units;
		/** @brief Cumulative new token types for this collection in the mint batch prefix. */
		uint32 items;
		/** @brief Earlier entry cursor used to accumulate duplicate IDs or collection issuance quotas. */
		uint32 j;
	};

	/** @brief Per-edition validation and cumulative collection-quota checks before ordered issuance. */
	struct MintMany_locals
	{
		/** @brief Current edition definition validated before irreversible native issuance. */
		Mint_input request;
		/** @brief Per-edition result and actual issuance fee, including pending delivery. */
		ExecuteMint_output result;
		/** @brief Collection snapshot used for creator/administrator authorization and cumulative issuance quotas. */
		Collection collection;
		/** @brief Cumulative type/unit totals used to preflight the collection quota. */
		MintCollectionQuotaScratch quotaScratch;
		/** @brief Zero-based cursor through the populated input entries. */
		uint32 i;
	};

	using ValidateBurn_input = Burn_input;

	using ValidateBurn_output = Burn_output;

	using ValidateBurn_locals = NoData;

	/** @brief Compact multi-mint: prototype.quantity is reserved (zero); each quantity creates a new ID. */
	struct MultiMint_input
	{
		/** @brief Shared edition settings; quantity is reserved and must be zero. */
		Mint_input prototype;
		/** @brief Positive fixed-edition sizes, each from 1 through MAX_AMOUNT; each used entry creates a new ID. */
		Array<sint64, QNFT_TOKEN_BATCH_CAPACITY> quantities;
		/** @brief Number of distinct types to issue, from 0 through 16. */
		uint32 count;
	};

	using MultiMint_output = Batch_output;

	/** @brief Expanded edition definitions built from shared settings and per-type quantities. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteMultiMint_input = MultiMint_input;

	using ExecuteMultiMint_output = MultiMint_output;

	/** @brief Expansion buffers combining shared edition settings with individual quantities. */
	struct ExecuteMultiMint_locals
	{
		/** @brief Expanded definitions forwarded to the shared issuance engine after validating input shape. */
		MintMany_input batch;
		/** @brief Single edition definition copied from the prototype with its requested quantity substituted. */
		Mint_input item;
		/** @brief Zero-based index of the current input entry. */
		uint32 i;
	};

	/** @brief Aggregate fee and refund settlement for editions expanded from one prototype. */
	struct MultiMint_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Positive quantities of native editions to burn from one owner; duplicates are checked cumulatively. */
	struct BurnBatch_input
	{
		/** @brief Account whose ownership or authority is queried or acted upon; NULL_ID is invalid for balance and movement requests. */
		id owner;
		/** @brief Ordered burn entries; only the first count slots are used. */
		Array<TokenAmount, QNFT_TOKEN_BATCH_CAPACITY> entries;
		/** @brief Number of ordered burn entries, from 0 through 16. */
		uint32 count;
	};

	using BurnBatch_output = Batch_output;

	/** @brief Per-entry authorization, cumulative duplicate holdings and free-settlement buffers. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteBurnBatch_input = BurnBatch_input;

	using ExecuteBurnBatch_output = BurnBatch_output;

	/** @brief Owner/operator and duplicate-quantity preflight followed by ordered native burns. */
	struct ExecuteBurnBatch_locals
	{
		/** @brief Participant/type holdings or validated batch prefix supplied to the balance checker. */
		ValidateBatchBalance_input balanceInput;

		/** @brief Current entry passed to validation and then execution; no later entries are committed after failure. */
		Burn_input request;
		/** @brief Current entry status and execution outputs; reset before each validation or execution call. */
		OperationResult result;
		/** @brief Zero-based index of the current input entry. */
		uint32 i;
		/** @brief Earlier input index used to detect duplicates or aggregate quantities for the same type or collection. */
		uint32 j;
		/** @brief Combined-balance validation status; a failure prevents batch execution. */
		ValidateBatchBalance_output balanceCode;
	};

	/** @brief Free batch-burn settlement, including refund of all attached QU. */
	struct BurnBatch_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	/** @brief Administrator request to pause, resume or permanently close collection issuance. */
	struct SetMintingStatus_input

	{

		/** @brief QNFT collection ID; allocation and authority are validated before changing its state. */
		uint32 collectionId;

		/** @brief Issuance gate: Open, Paused or permanently Closed; accepted deliveries remain recoverable. */
		MintingStatus status;
	};

	using SetMintingStatus_output = OperationResult;

	/** @brief Issuance-status event and refund buffers for a free administration operation. */
	/** @brief Execution uses the unchanged public request and response layouts. */
	using ExecuteSetMintingStatus_input = SetMintingStatus_input;

	using ExecuteSetMintingStatus_output = SetMintingStatus_output;

	/** @brief Collection authority, current issuance status and transition journal buffers. */
	struct ExecuteSetMintingStatus_locals
	{
		/** @brief Journal record prepared only for a committed state change. */
		AppendEvent_input event;
		/** @brief Unused result of appending a committed change to the event journal. */
		AppendEvent_output logged;

		/** @brief Stored issuance state before validating the requested transition; Closed is irreversible. */
		MintingStatus mintingStatus;
	};

	/** @brief Free issuance-state change settlement and attached-QU refund buffers. */
	struct SetMintingStatus_locals
	{
		/** @brief Shared finalization request. */
		FinishCommand_input finishInput;
		/** @brief Settlement result separate from the public response. */
		FinishCommand_output finishOutput;
	};

	using GetMintingStatus_input = GetCollection_input;

	/** @brief Issuance gate and remaining lifetime quota. */
	struct GetMintingStatus_output
	{
		/** @brief Remaining units up to the author cap or MAX_AMOUNT; unavailable while paused or closed. */
		sint64 remainingUnits;
		/** @brief Remaining types up to the author cap or QNFT_MAX_ITEMS; global capacity is checked separately. */
		uint32 remainingItems;
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
		/** @brief Issuance gate: Open, Paused or permanently Closed; accepted deliveries remain recoverable. */
		MintingStatus status;
	};

	/** @brief Collection snapshot and reservation count for remaining-quota calculation. */
	struct GetMintingStatus_locals
	{
		/** @brief Collection snapshot whose lifetime issuance quotas are checked before creating new types. */
		Collection collection;
	};

	using Exists_input = TotalSupply_input;

	/** @brief Whether the native ledger currently contains positive supply for the requested type. */
	struct Exists_output
	{
		/** @brief Operation status; other result fields are meaningful only as defined for that status. */
		Code code;
		/** @brief One only when current global live supply is positive; unknown and fully burned IDs return zero. */
		bit exists;
	};

	/** @brief Global supply response used to distinguish live types from unknown or fully burned types. */
	struct Exists_locals
	{
		/** @brief Global live-supply response; zero units means no live type. */
		TotalSupply_output supply;
	};

	using ResolvedURI_input = URI_input;

	using ResolvedURI_output = URI_output;

	/** @brief Stored URI and bounded expansion cursors for lowercase hexadecimal token substitution. */
	struct ResolvedURI_locals
	{
		/** @brief Unmodified URI response used as the source of token-ID template expansion. */
		URI_output stored;
		/** @brief Read offset in the original URI buffer, before expansion. */
		uint32 source;
		/** @brief Write offset in the expanded URI buffer; capacity reserves a terminating zero. */
		uint32 target;
		/** @brief Hexadecimal digit position from 0 through 63 within an expanded token ID. */
		uint32 i;
		/** @brief One hexadecimal token-ID nibble, from 0 through 15, before ASCII conversion. */
		uint32 digit;
	};

	/** @brief Shared read-only projection of all period boundaries at the current network time. */
	using ProjectRental_input = Rental;

	/** @brief Effective rental snapshot and newly due QU allocations; projection never mutates state. */
	struct ProjectRental_output
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief QU consumed by newly due affordable renewal periods; projection alone does not debit state. */
		sint64 charged;
		/** @brief Protocol share in QU of newly projected renewal charges. */
		sint64 protocolPaid;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Effective coverage and affordable elapsed periods at one validated clock snapshot. */
	struct RentalPeriodProjection
	{
		/** @brief Exclusive end of the paid usage interval; renewal may extend it from prepaid collateral. */
		DateAndTime paidUntil;
		/** @brief Number of elapsed renewal boundaries through the validated current time. */
		uint64 periodsDue;
		/** @brief Elapsed periods affordable from the current deposit; never exceeds periodsDue. */
		uint64 periodsPaid;
		/** @brief Whether funded usage extends beyond the validated current time. */
		bool active;
	};

	/** @brief Validated network-time snapshot and per-period QU deductions for read-only projection. */
	struct ProjectRental_locals
	{
		/** @brief Coverage-only projection shared with transfer-balance checks. */
		RentalPeriodProjection period;
		/** @brief One consensus-time snapshot, validated before deadline arithmetic or dependent work. */
		DateAndTime now;
		/** @brief Creator deduction in QU for one complete renewal period, rounded down. */
		sint64 creatorPerPeriod;
		/** @brief Protocol deduction in QU for one complete renewal period, rounded down. */
		sint64 protocolPerPeriod;
	};

	/** @brief Lender/type traversal used to subtract effective reservations from local holdings. */
	struct TransferableBalance_locals
	{
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Coverage-only projection shared with transfer-balance checks. */
		RentalPeriodProjection period;
		/** @brief One consensus-time snapshot, validated before deadline arithmetic or dependent work. */
		DateAndTime now;
		/** @brief Units reserved by unpaid offers for this owner/type pair. */
		sint64 offeredQuantity;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
	};

	/** @brief Owner/type and renter/type traversal for effective holdings and usage quantities. */
	struct RentalBalance_locals
	{
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Read-only effective rental state and newly due accounting allocations. */
		ProjectRental_output projected;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
	};

	/** @brief A missing or fully reclaimed rental returns NotFound. */
	struct GetRental_input
	{
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
	};

	/** @brief Effective usage and payment snapshot; consumers must check code before interpreting it. */
	struct GetRental_output
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		RentalView rental;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Stored rental and read-only projection buffers for the public snapshot. */
	struct GetRental_locals
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Read-only effective rental state and newly due accounting allocations. */
		ProjectRental_output projected;
	};

	using StoreRental_input = Rental;

	using StoreRental_output = NoData;

	using SettleRental_input = GetRental_input;

	/** @brief Materialized rental accounting and status after elapsed-period settlement. */
	struct SettleRental_output
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Pre-settlement snapshot, participant indexes and lifecycle journal buffers. */
	struct SettleRental_locals
	{
		/** @brief Stored rental before elapsed-period settlement. */
		Rental before;
		/** @brief Read-only effective rental state and newly due accounting allocations. */
		ProjectRental_output projected;
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Participant/type pair to settle, with a selector for incoming versus outgoing rentals. */
	struct SweepRentalPair_input
	{
		/** @brief Participant and token type whose indexed rentals must be settled. */
		RentalBalance_input pair;
		/** @brief True traverses incoming rentals; false traverses the lender/type index. */
		bool renter;
	};

	using SweepRentalPair_output = NoData;

	/** @brief Stable rental-ID traversal that survives collection element relocation on removal. */
	struct SweepRentalPair_locals
	{
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
		/** @brief Next lower rental-ID priority, allowing safe traversal after element relocation. */
		sint64 nextPriority;
		/** @brief Stable rental ID passed to settlement or payout processing. */
		GetRental_input settleInput;
		/** @brief Materialized rental snapshot and settlement status. */
		SettleRental_output settled;
	};

	using PayRental_input = GetRental_input;

	using PayRental_output = NoData;

	/** @brief Outstanding owner/creator QU credits and transfer retry buffers. */
	struct PayRental_locals
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Toggle the common export/SC-delivery policy; original creator and current administrator are independent authorities. */
	struct SetExternalTransfersDisabled_input
	{
		/** @brief Zero-based collection whose mutable transfer policy is being changed. */
		uint32 collectionId;
		/** @brief True blocks export and delivery to contract recipients for this collection. */
		bit disabled;
	};

	using SetExternalTransfersDisabled_output = OperationResult;

	using ExecuteSetExternalTransfersDisabled_input = SetExternalTransfersDisabled_input;

	using ExecuteSetExternalTransfersDisabled_output = SetExternalTransfersDisabled_output;

	/** @brief Collection authority snapshot and policy-change journal buffers. */
	struct ExecuteSetExternalTransfersDisabled_locals
	{
		/** @brief Collection snapshot used for creator/administrator authorization and cumulative issuance quotas. */
		Collection collection;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Free transfer-policy change settlement and attached-QU refund buffers. */
	struct SetExternalTransfersDisabled_locals
	{
		/** @brief Shared invocation-reward retention, refund and event-settlement arguments. */
		FinishCommand_input finish;
		/** @brief Result of shared settlement after the business operation. */
		FinishCommand_output finished;
	};

	/** @brief Reserve locally managed units at fixed QU-per-unit-per-day terms; attached QU is refunded. */
	struct CreateRentalOffer_input
	{
		/** @brief Fixed price in QU per unit per day; positive and bounded by MAX_AMOUNT. */
		sint64 dailyPrice;
		/** @brief Positive number of native units covered by this offer or rental. */
		sint64 quantity;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Fixed renewal period in whole days, at least QNFT_MIN_RENTAL_DAYS; expiry must remain representable. */
		uint32 periodDays;
	};

	using CreateRentalOffer_output = RentalResult;

	/** @brief Available-unit validation, representable expiry and reservation commit buffers. */
	struct ExecuteCreateRentalOffer_locals
	{
		/** @brief Unpaid rental offer snapshot; only remaining units may be accepted or released. */
		RentalOffer offer;
		/** @brief Participant/type holdings or validated batch prefix supplied to the balance checker. */
		RentalBalance_input balanceInput;
		/** @brief Effective rental reservations and available local units for this operation. */
		RentalBalance_output balance;
		/** @brief Temporary expiry used to reject unrepresentable rental periods before reserving units. */
		DateAndTime until;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Aggregate unpaid offered units for the owner/type pair after the current adjustment. */
		sint64 held;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	using ExecuteCreateRentalOffer_input = CreateRentalOffer_input;

	using ExecuteCreateRentalOffer_output = RentalResult;

	/** @brief Free offer-creation settlement and attached-QU refund buffers. */
	struct CreateRentalOffer_locals
	{
		/** @brief Shared invocation-reward retention, refund and event-settlement arguments. */
		FinishCommand_input finish;
		/** @brief Result of shared settlement after the business operation. */
		FinishCommand_output finished;
	};

	/** @brief Close only the unpaid offer remainder; existing subscriptions retain their terms. */
	struct CancelRentalOffer_input
	{
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
	};

	using CancelRentalOffer_output = RentalResult;

	using ExecuteCancelRentalOffer_input = CancelRentalOffer_input;

	using ExecuteCancelRentalOffer_output = RentalResult;

	/** @brief Lender authorization and release of the unpaid offer remainder. */
	struct ExecuteCancelRentalOffer_locals
	{
		/** @brief Unpaid rental offer snapshot; only remaining units may be accepted or released. */
		RentalOffer offer;
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12 digest of participant identity and token type for reservation/index lookup. */
		id key;
		/** @brief Aggregate unpaid offered units for the owner/type pair after the current adjustment. */
		sint64 held;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Free offer-cancellation settlement and attached-QU refund buffers. */
	struct CancelRentalOffer_locals
	{
		/** @brief Shared invocation-reward retention, refund and event-settlement arguments. */
		FinishCommand_input finish;
		/** @brief Result of shared settlement after the business operation. */
		FinishCommand_output finished;
	};

	/** @brief Accept exact fixed terms; reward pays the first period and deposits the remainder. */
	struct Rent_input
	{
		/** @brief Monotonic offer identifier, starting at one; remains a historical reference after offer removal. */
		uint64 offerId;
		/** @brief Positive number of native units covered by this offer or rental. */
		sint64 quantity;
		/** @brief Expected QU price per unit per day; mismatches reject stale offer acceptance. */
		sint64 expectedDailyPrice;
		/** @brief Expected fixed period in days; mismatches reject stale offer acceptance. */
		uint32 expectedPeriodDays;
	};

	using Rent_output = RentalResult;

	using ExecuteRent_input = Rent_input;

	using ExecuteRent_output = RentalResult;

	/** @brief Fixed-term acceptance, participant-index preparation and first-period payment buffers. */
	struct ExecuteRent_locals
	{
		/** @brief Unpaid rental offer snapshot; only remaining units may be accepted or released. */
		RentalOffer offer;
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Zero-initialized owner/type key material used consistently for rental indexes. */
		RentalBalance_input keyInput;
		/** @brief K12(owner, itemId) used for outgoing rental and offer reservations. */
		id ownerKey;
		/** @brief K12(renter, itemId) used to index incoming usage grants. */
		id renterKey;
		/** @brief Participant/type traversal request used to settle existing rentals before a new acceptance. */
		SweepRentalPair_input sweep;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
		/** @brief Aggregate unpaid offered units for the owner/type pair after the current adjustment. */
		sint64 held;
		/** @brief First-period protocol allocation in QU, excluded from lender and creator credits. */
		sint64 protocol;
		/** @brief First-period creator allocation in QU owed to the immutable collection creator. */
		sint64 creator;
		/** @brief Unix epoch used to convert rental deadlines into comparable microsecond priorities. */
		DateAndTime origin;
		/** @brief New rental ID whose first-period credits are paid after all obligations are recorded. */
		GetRental_input payment;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
	};

	/** @brief Retain accepted rent and deposit; refund all attached QU when acceptance fails. */
	struct Rent_locals
	{
		/** @brief Shared invocation-reward retention, refund and event-settlement arguments. */
		FinishCommand_input finish;
		/** @brief Result of shared settlement after the business operation. */
		FinishCommand_output finished;
	};

	/** @brief Deposit mutation commands settle elapsed periods before touching free collateral. */
	struct RentalDeposit_input
	{
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
	};

	using TopUpRental_input = GetRental_input;

	using WithdrawRentalDeposit_input = RentalDeposit_input;

	using CancelRentalRenewal_input = GetRental_input;

	using TopUpRental_output = RentalResult;

	using WithdrawRentalDeposit_output = RentalResult;

	using CancelRentalRenewal_output = RentalResult;

	/** @brief Internal rental mutation selected by the public command. */
	enum class RentalAction : uint32
	{
		/** @brief Add the positive invocation reward to an active rental deposit. */
		TopUp,
		/** @brief Return a positive amount of unused deposit to its renter. */
		Withdraw,
		/** @brief Disable future renewal while preserving already paid usage rights. */
		CancelRenewal
	};

	/** @brief Authorized deposit or renewal mutation; the first step settles elapsed coverage. */
	struct ChangeRental_input
	{
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief QU assigned to the current payout or collateral operation. */
		sint64 amount;
		/** @brief Requested deposit/renewal mutation; determines authorization and retained payment. */
		RentalAction action;
	};

	using ChangeRental_output = RentalResult;

	/** @brief Settled rental, collateral adjustment and mutation-event buffers. */
	struct ChangeRental_locals
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Stable rental ID passed to settlement or payout processing. */
		GetRental_input settleInput;
		/** @brief Materialized rental snapshot and settlement status. */
		SettleRental_output settled;
		/** @brief Scratch reserved for a pre-mutation deposit snapshot; currently not consumed. */
		sint64 beforeDeposit;
		/** @brief Lifecycle or collateral event emitted after committing the corresponding state change. */
		JournalEvent event;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Typed request routed through shared rental mutation and authorization. */
	struct ExecuteTopUpRental_locals
	{
		/** @brief Typed deposit/renewal operation forwarded to shared rental mutation logic. */
		ChangeRental_input change;
	};

	using ExecuteTopUpRental_input = TopUpRental_input;

	using ExecuteTopUpRental_output = TopUpRental_output;

	using ExecuteWithdrawRentalDeposit_input = WithdrawRentalDeposit_input;

	using ExecuteWithdrawRentalDeposit_output = WithdrawRentalDeposit_output;

	using ExecuteWithdrawRentalDeposit_locals = ExecuteTopUpRental_locals;

	using ExecuteCancelRentalRenewal_input = CancelRentalRenewal_input;

	using ExecuteCancelRentalRenewal_output = CancelRentalRenewal_output;

	using ExecuteCancelRentalRenewal_locals = ExecuteTopUpRental_locals;

	/** @brief Deposit retention and refund settlement for a rental top-up. */
	struct TopUpRental_locals
	{
		/** @brief Shared invocation-reward retention, refund and event-settlement arguments. */
		FinishCommand_input finish;
		/** @brief Result of shared settlement after the business operation. */
		FinishCommand_output finished;
	};

	using WithdrawRentalDeposit_locals = TopUpRental_locals;

	using CancelRentalRenewal_locals = TopUpRental_locals;

	using ProcessRentals_input = NoData;

	using ProcessRentals_output = NoData;

	/** @brief Bounded due-deadline and failed-payment queue processing state for one tick. */
	struct ProcessRentals_locals
	{
		/** @brief Unix epoch used to convert rental deadlines into comparable microsecond priorities. */
		DateAndTime origin;
		/** @brief One consensus-time snapshot, validated before deadline arithmetic or dependent work. */
		DateAndTime now;
		/** @brief Negative Unix timestamp in microseconds; the greatest priority is the earliest deadline. */
		sint64 priority;
		/** @brief Current collection/map position; NULL_INDEX denotes no element for signed cursor fields. */
		sint64 index;
		/** @brief Number of due deadlines processed this tick, bounded by QNFT_RENTALS_PER_TICK. */
		uint32 count;
		/** @brief Number of queued entries left in this bounded processing pass. */
		uint64 remaining;
		/** @brief Stable rental ID passed to settlement or payout processing. */
		GetRental_input settleInput;
		/** @brief Materialized rental snapshot and settlement status. */
		SettleRental_output settled;
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Empty helper result; the called operation communicates through state or its other outputs. */
		NoData unused;
	};

	/** @brief Current unpaid offer remainder; fully consumed or cancelled offers return NotFound. */
	using GetRentalOffer_input = CancelRentalOffer_input;

	/** @brief Remaining unpaid offer terms or NotFound after full acceptance/cancellation. */
	struct GetRentalOffer_output
	{
		/** @brief Unpaid rental offer snapshot; only remaining units may be accepted or released. */
		RentalOffer offer;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Tick-bound map scan; empty pages may still have a continuation cursor. NULL_ID selects all owners. */
	struct ListRentalOffers_input
	{
		/** @brief Optional lender filter; NULL_ID returns offers from all owners. */
		id owner;
		/** @brief Continuation slot for the selected registry/phase; zero starts the scan. */
		uint32 cursor;
		/** @brief Maximum returned entries, from 1 through QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
	};

	/** @brief Bounded offer page with continuation and observed tick for consistency checks. */
	struct ListRentalOffers_output
	{
		/** @brief Remaining unpaid offers matching the requested owner filter. */
		Array<RentalOffer, QNFT_REGISTRY_PAGE_CAPACITY> offers;
		/** @brief Number of populated entries; remaining array slots are not part of the result. */
		uint32 count;
		/** @brief First storage position to inspect on the next page. */
		uint32 nextCursor;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
		/** @brief True when the selected scan is exhausted; false requires following the continuation. */
		bit done;
	};

	/** @brief Owner filtering and bounded slot scan for rental offer pagination. */
	struct ListRentalOffers_locals
	{
		/** @brief Unpaid rental offer snapshot; only remaining units may be accepted or released. */
		RentalOffer offer;
		/** @brief Storage positions inspected, including filtered-out entries; bounds page work. */
		uint32 scanned;
	};

	/** @brief Participant roles selected when listing retained rentals. */
	enum class RentalRole : uint32
	{
		/** @brief Include rentals where the queried user is either lender or renter. */
		Both,
		/** @brief Include only rentals backed by the queried owner. */
		Lender,
		/** @brief Include only rentals granting usage to the queried user. */
		Renter
	};

	/** @brief Select rentals by participant role, including retained finished contracts. */
	struct ListUserRentals_input
	{
		/** @brief Nonzero account whose owned records or rental participation are queried. */
		id user;
		/** @brief Continuation slot for the selected registry/phase; zero starts the scan. */
		uint32 cursor;
		/** @brief Maximum returned entries, from 1 through QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
		/** @brief Select lender, renter or either role when filtering retained rentals. */
		RentalRole role;
	};

	/** @brief Participant-filtered effective rental snapshots and continuation metadata. */
	struct ListUserRentals_output
	{
		/** @brief Effective participant-filtered rental snapshots; only count entries are populated. */
		Array<RentalView, QNFT_REGISTRY_PAGE_CAPACITY> rentals;
		/** @brief Number of populated entries; remaining array slots are not part of the result. */
		uint32 count;
		/** @brief First storage position to inspect on the next page. */
		uint32 nextCursor;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
		/** @brief True when the selected scan is exhausted; false requires following the continuation. */
		bit done;
	};

	/** @brief Role filtering and read-only rental projection during bounded pagination. */
	struct ListUserRentals_locals
	{
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Read-only effective rental state and newly due accounting allocations. */
		ProjectRental_output projected;
		/** @brief Public projection with Unix expiry and explicit ownership/usage identities. */
		RentalView view;
		/** @brief Storage positions inspected, including filtered-out entries; bounds page work. */
		uint32 scanned;
		/** @brief Whether this stored rental satisfies the requested participant role filter. */
		bool matches;
	};

	/** @brief Ledger ownership is independent of rental rights; local availability excludes offers and live leases. */
	using GetUserNft_input = RentalBalance_input;

	/** @brief Native ownership, local reservations and incoming usage rights kept as distinct quantities. */
	struct GetUserNft_output
	{
		/** @brief Native ownership across managers and possessors; excludes incoming rental rights. */
		sint64 ownedQuantity;
		/** @brief Units both owned and possessed by this account under QNFT management. */
		sint64 managedQuantity;
		/** @brief Units reserved by unpaid offers for this owner/type pair. */
		sint64 offeredQuantity;
		/** @brief Units backing effective outgoing rentals, including affordable pending renewals. */
		sint64 lentQuantity;
		/** @brief Effective incoming usage units; never added to transferable native holdings. */
		sint64 rentedQuantity;
		/** @brief Locally managed units free of offer and active-rental reservations; never negative. */
		sint64 availableQuantity;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
	};

	/** @brief Ledger identity and effective rental quantities for a user/type query. */
	struct GetUserNft_locals
	{
		/** @brief QNFT-issued native asset identity used for ledger ownership queries. */
		Asset asset;
		/** @brief Effective rental reservations and available local units for this operation. */
		RentalBalance_output balance;
	};

	/** @brief One native ownership record or one live incoming rental; multiple rows may share itemId. */
	enum class UserNftRelation : uint32
	{
		/** @brief Native ownership record; the quantity is not a rental usage grant. */
		Owned,
		/** @brief Active incoming usage grant; does not authorize transfer, sale or burn. */
		Rented
	};

	/** @brief One native ownership row or incoming usage grant; relation distinguishes their authority. */
	struct UserNftEntry
	{
		/** @brief Native owner backing the units; rental usage does not transfer ownership. */
		id owner;
		/** @brief Account entitled to use the rented units; receives any unused deposit. */
		id renter;
		/** @brief Monotonic rental identifier, starting at one; never reused after deletion. */
		uint64 rentalId;
		/** @brief Exclusive usage expiry as Unix microseconds since 1970-01-01 UTC; zero for ownership-only rows. */
		uint64 paidUntil;
		/** @brief Positive number of native units covered by this offer or rental. */
		sint64 quantity;
		/** @brief Zero-based registered NFT type; quantities refer to interchangeable copies of this type. */
		uint32 itemId;
		/** @brief Distinguishes native ownership from rented usage; only owned units can be spent. */
		UserNftRelation relation;
		/** @brief Native ownership manager for owned rows; QNFT index for rental usage rows. */
		uint16 managingContract;
		/** @brief Whether future elapsed periods may consume the deposit; cancellation preserves paid coverage. */
		bit autoRenew;
	};

	/** @brief Traversal stage for the combined ownership and incoming-rental listing. */
	enum class UserNftPhase : uint32
	{
		/** @brief Scan native ownership records first, preserving manager distinctions. */
		Ownership,
		/** @brief Scan active rental grants after the ownership phase. */
		IncomingRentals
	};

	/** @brief Echo the returned cursor, traversal phase and tick to continue listing. */
	struct ListUserNfts_input
	{
		/** @brief Nonzero account whose owned records or rental participation are queried. */
		id user;
		/** @brief Continuation slot for the selected registry/phase; zero starts the scan. */
		uint32 cursor;
		/** @brief Maximum returned entries, from 1 through QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
		/** @brief Ownership scan followed by incoming rentals; pass the returned phase with nextCursor. */
		UserNftPhase phase;
	};

	/** @brief Two-phase page of owned records and incoming usage grants with a continuation cursor. */
	struct ListUserNfts_output
	{
		/** @brief Owned rows or active rental usage grants; relation identifies each row's authority. */
		Array<UserNftEntry, QNFT_REGISTRY_PAGE_CAPACITY> entries;
		/** @brief Number of populated entries; remaining array slots are not part of the result. */
		uint32 count;
		/** @brief First storage position to inspect on the next page. */
		uint32 nextCursor;
		/** @brief Observed tick for responses; requests use zero to disable stale-tick rejection. */
		uint32 tick;
		/** @brief Ownership scan followed by incoming rentals; pass the returned phase with nextCursor. */
		UserNftPhase phase;
		/** @brief Operation status; only Code::Ok permits normal interpretation of returned data. */
		Code code;
		/** @brief True when the selected scan is exhausted; false requires following the continuation. */
		bit done;
	};

	/** @brief Bounded ledger scan, registry resolution and incoming-rental projection buffers. */
	struct ListUserNfts_locals
	{
		/** @brief Bounded native ownership scan for the requested user and QNFT issuer. */
		AssetOwnerIterator iterator;
		/** @brief One ownership or usage row prepared for the response page. */
		UserNftEntry entry;
		/** @brief Native asset identity to resolve to a registered QNFT item. */
		Lookup_input lookup;
		/** @brief Registry lookup result; itemId is usable only when valid is true. */
		Lookup_output found;
		/** @brief Stored or projected rental snapshot used by the current operation. */
		Rental rental;
		/** @brief Read-only effective rental state and newly due accounting allocations. */
		ProjectRental_output projected;
		/** @brief Public projection with Unix expiry and explicit ownership/usage identities. */
		RentalView view;
		/** @brief Storage positions inspected, including filtered-out entries; bounds page work. */
		uint32 scanned;
		/** @brief Ledger cursor before the current iterator step, used to charge the scan budget. */
		uint32 before;
	};

protected:

	/**
	 * @brief Validate printable, nonempty, zero-padded metadata URIs.
	 */
	static bool CheckMetadata(const Metadata& metadata, MetadataValidationScratch& locals)
	{
		// Reset scratch data because a batch may reuse it for several documents.
		locals.terminated = false;
		if (metadata.uri.get(0) == Ch::null)
		{
			return false;
		}

		for (locals.i = 0; locals.i < QNFT_METADATA_URI_CAPACITY; ++locals.i)
		{
			if (metadata.uri.get(locals.i) == Ch::null)
			{
				locals.terminated = true;
				continue;
			}

			if (locals.terminated || !isInRange<uint8>(metadata.uri.get(locals.i), QNFT_METADATA_URI_MIN_CHARACTER, QNFT_METADATA_URI_MAX_CHARACTER))
			{
				return false;
			}
		}

		return locals.terminated;
	}

	/**
	 * @brief Encode an item ID using the shared asset-name format.
	 * @note The same encoder is used by Lookup to validate the prefix, digit representation and unused name bytes.
	 */
	static uint64 EncodeAssetName(uint32 itemId, AssetNameScratch& locals)
	{
		// Pack the fixed-width base-36 ID after the shared prefix, retaining leading zero digits.
		locals.name = QNFT_ASSET_NAME_PREFIX;
		locals.value = itemId;
		for (locals.i = QNFT_ASSET_NAME_DIGITS; locals.i > 0; --locals.i)
		{
			locals.digit = mod(locals.value, QNFT_ASSET_NAME_RADIX);
			locals.value = QPI::div(locals.value, QNFT_ASSET_NAME_RADIX);
			locals.name |= static_cast<uint64>(locals.digit < QNFT_ASSET_NAME_DECIMAL_DIGITS ? Ch::_0 + locals.digit
			                                                                                 : Ch::A + locals.digit - QNFT_ASSET_NAME_DECIMAL_DIGITS)
			               << (QNFT_ASSET_NAME_BYTE_BITS * locals.i);
		}

		return locals.name;
	}

	/** @brief Check the immutable creator or current administrator; never grants authority to a zero account. */
	static bool IsCollectionManager(const Collection& collection, const id& account)
	{
		return account != NULL_ID && (account == collection.creator || account == collection.administrator);
	}

	/** @brief Return the smaller operand without reading state or invoking another contract. */
	template<typename T>
	static constexpr T minimum(T left, T right)
	{
		return left < right ? left : right;
	}

	/** @brief Return remaining capacity without wrapping when issued counts exhaust the limit. */
	template<typename T>
	static constexpr T remainingQuota(T limit, T issued)
	{
		if (issued > limit)
		{
			return 0;
		}

		return limit - issued;
	}

	/** @brief Remaining lifetime types; zero author cap uses the technical per-contract ceiling. */
	static uint32 remainingCollectionItems(const Collection& collection)
	{
		return remainingQuota<uint32>(collection.maxItems ? collection.maxItems : QNFT_MAX_ITEMS, collection.issuedItems);
	}

	/** @brief Remaining lifetime units; the technical ceiling protects the cumulative signed counter. */
	static sint64 remainingCollectionUnits(const Collection& collection)
	{
		return remainingQuota<sint64>(collection.maxUnits ? collection.maxUnits : MAX_AMOUNT, collection.issuedUnits);
	}

	/** @brief Check the current collection's batch prefix before any issuance. Entries through index must be validated. */
	static bool CheckMintCollectionQuota(const MintMany_input& batch, uint32 index, const Collection& collection, MintCollectionQuotaScratch& scratch)
	{
		scratch.units = batch.entries.get(index).quantity;
		scratch.items = 1;
		for (scratch.j = 0; scratch.j < index; ++scratch.j)
		{
			if (batch.entries.get(scratch.j).collectionId != batch.entries.get(index).collectionId)
			{
				continue;
			}

			++scratch.items;
			if (batch.entries.get(scratch.j).quantity > MAX_AMOUNT - scratch.units)
			{
				return false;
			}

			scratch.units += batch.entries.get(scratch.j).quantity;
		}

		return scratch.items <= remainingCollectionItems(collection) && scratch.units <= remainingCollectionUnits(collection);
	}

	/** @brief Test an inclusive range; reversed bounds match no value. */
	template<typename T>
	static constexpr bool isInRange(T value, T minimum, T maximum)
	{
		// Use one comparison type for the value and both bounds; no subtraction can overflow.
		return value >= minimum && value <= maximum;
	}

public:

	/**
	 * @brief Validate one governed variable without permitting access to the developer's share.
	 */
	static bool isGovernedValueValid(FeeVariable variable, sint64 value, uint32 developerFeeBPS)
	{
		// Bound fees in QU and rates in basis points; only the nondeveloper part of the split is governable.
		switch (variable)
		{
			case FeeVariable::CollectionFee:
			case FeeVariable::MintFee:
			case FeeVariable::NominateAdminFee:
			case FeeVariable::TransferFee:
			case FeeVariable::ApproveFee:
			case FeeVariable::UpdateMetadataFee:
			case FeeVariable::ExportFee:
			case FeeVariable::ListingFee:
			case FeeVariable::ProposalFee: return isInRange<sint64>(value, 0, MAX_AMOUNT);
			case FeeVariable::RentalProtocolBPS:
			case FeeVariable::RentalCreatorBPS:
			case FeeVariable::ShopFeeBPS: return isInRange<sint64>(value, 0, QNFT_BPS_DENOMINATOR);
			case FeeVariable::ShareholderFeeBPS:
			case FeeVariable::BurnFeeBPS:
				return developerFeeBPS <= QNFT_BPS_DENOMINATOR && isInRange<sint64>(value, 0, QNFT_BPS_DENOMINATOR - developerFeeBPS);
			default: return false;
		}
	}

protected:

	/** @brief Bound the full gross-price deductions before accepting a listing or purchase. */
	static bool validSaleRates(const Item& item, uint32 shopFeeBps)
	{
		return shopFeeBps <= QNFT_BPS_DENOMINATOR &&
		       static_cast<uint32>(effectiveRoyaltyBps(item)) + item.dividendBps <= QNFT_BPS_DENOMINATOR - shopFeeBps;
	}

	/** @brief A missing beneficiary disables royalty payments while preserving the stored rate. */
	static uint16 effectiveRoyaltyBps(const Item& item)
	{
		// Use the same effective rate for sale admission, purchase deductions and royalty queries.
		return item.royaltyReceiver == NULL_ID ? 0 : item.royaltyBps;
	}

	/**
	 * @brief Return floor(amountQU * rateBPS / 10000) without overflowing the intermediate product.
	 * @note Preconditions: amountQU >= 0 and rateBPS <= QNFT_BPS_DENOMINATOR; checked by the caller.
	 */
	static sint64 calculateFeeShare(sint64 amountQU, uint32 rateBPS)
	{
		// Split the amount into quotient and remainder so the full amount is never multiplied by the rate.
		return QPI::div(amountQU, static_cast<sint64>(QNFT_BPS_DENOMINATOR)) * rateBPS +
		       QPI::div(mod(amountQU, static_cast<sint64>(QNFT_BPS_DENOMINATOR)) * rateBPS, static_cast<sint64>(QNFT_BPS_DENOMINATOR));
	}

public:

	/** @brief Reject invalid dates and the entire 2022-04-13 bootstrap day; does not prove clock synchronization. */
	static bool IsTimeValid(const DateAndTime& time)
	{
		return time.isValid() && !(time.getYear() == 2022 && time.getMonth() == 4 && time.getDay() == 13);
	}

	/** @brief Project funded coverage without fees or state changes. The caller must validate now. */
	static void ProjectRentalPeriods(const Rental& rental, const DateAndTime& now, RentalPeriodProjection& output)
	{
		output.paidUntil = rental.paidUntil;
		output.periodsDue = 0;
		output.periodsPaid = 0;
		output.active = rental.status == RentalStatus::Active;
		if (!output.active || now < rental.paidUntil)
		{
			return;
		}

		if (rental.autoRenew)
		{
			output.periodsDue =
			    div<uint64>(rental.paidUntil.durationMicrosec(now), smul(QNFT_DAY_MICROSECONDS, static_cast<sint64>(rental.periodDays))) + 1;
			output.periodsPaid = minimum(output.periodsDue, static_cast<uint64>(div<sint64>(rental.deposit, rental.periodPrice)));
			if (output.periodsPaid &&
			    !output.paidUntil.addDays(smul(static_cast<sint64>(output.periodsPaid), static_cast<sint64>(rental.periodDays))))
			{
				// An unrepresentable expiry ends coverage without charging for the failed extension.
				output.paidUntil = rental.paidUntil;
				output.periodsPaid = 0;
				output.active = false;
				return;
			}
		}

		output.active = now < output.paidUntil;
	}

protected:

	/** @brief Expose Unix microseconds so consumers can interpret expiry without DateAndTime internals. */
	static void DescribeRental(const Rental& input, RentalView& output)
	{
		output.paidUntil = DateAndTime(1970, 1, 1).durationMicrosec(input.paidUntil);
		output.owner = input.owner;
		output.renter = input.renter;
		output.creator = input.creator;
		output.rentalId = input.rentalId;
		output.offerId = input.offerId;
		output.dailyPrice = input.dailyPrice;
		output.periodPrice = input.periodPrice;
		output.quantity = input.quantity;
		output.deposit = input.deposit;
		output.ownerCredit = input.ownerCredit;
		output.creatorCredit = input.creatorCredit;
		output.itemId = input.itemId;
		output.periodDays = input.periodDays;
		output.status = input.status;
		output.autoRenew = input.autoRenew;
	}

	/** @brief Check cumulative duplicate-ID consumption after per-entry validation; does not mutate balances.
	 * @note Self transfers skip this check because earlier entries do not consume holdings.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateBatchBalance)
	{
		locals.itemId = input.entries.get(input.index).itemId;
		locals.cumulative = input.entries.get(input.index).quantity;
		for (locals.j = 0; locals.j < input.index; ++locals.j)
		{
			if (input.entries.get(locals.j).itemId != locals.itemId)
			{
				continue;
			}

			if (input.entries.get(locals.j).quantity > MAX_AMOUNT - locals.cumulative)
			{
				output = Code::InsufficientBalance;
				return;
			}

			locals.cumulative += input.entries.get(locals.j).quantity;
		}

		locals.rentalBalanceInput.owner = input.owner;
		locals.rentalBalanceInput.itemId = locals.itemId;
		CALL(RentalBalance, locals.rentalBalanceInput, locals.rentalBalance);
		if (locals.cumulative > locals.rentalBalance.availableQuantity && (locals.rentalBalance.offeredQuantity || locals.rentalBalance.lentQuantity))
		{
			output = Code::Conflict;
			return;
		}

		if (qpi.numberOfPossessedShares(state.get().items.get(locals.itemId).assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX) <
		    locals.cumulative)
		{
			output = Code::InsufficientBalance;
			return;
		}

		output = Code::Ok;
	}

	/** @brief Check supply, authority and available units after the caller validates participants and item ID. */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateOwnedUnits)
	{
		locals.item = state.get().items.get(input.itemId);

		if (locals.item.supply == 0)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.authorizationInput.owner = input.owner;
		locals.authorizationInput.itemId = input.itemId;
		CALL(isOwnerOrApprovedOperator, locals.authorizationInput, locals.authorizationOutput);
		if (!locals.authorizationOutput.authorized)
		{
			output.code = Code::Unauthorized;
			return;
		}

		locals.rentalBalanceInput.owner = input.owner;
		locals.rentalBalanceInput.itemId = input.itemId;
		CALL(RentalBalance, locals.rentalBalanceInput, locals.rentalBalance);
		if (locals.rentalBalance.code != Code::Ok)
		{
			output.code = locals.rentalBalance.code;
			return;
		}

		if (input.quantity > locals.rentalBalance.availableQuantity && (locals.rentalBalance.offeredQuantity || locals.rentalBalance.lentQuantity))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.balance = qpi.numberOfPossessedShares(locals.item.assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX);
		if (locals.balance < input.quantity)
		{
			output.code = Code::InsufficientBalance;
			return;
		}

		output.code = Code::Ok;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateBurn)
	{
		// Validate holdings and operator permission before burning native units.

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || input.owner == NULL_ID || input.owner == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		CALL(ValidateOwnedUnits, input, output);
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateTransfer)
	{
		// Validate participants, lifecycle and holdings; mutation entry points also check fees.

		if (!isInRange<sint64>(input.quantity, 0, MAX_AMOUNT) || input.owner == NULL_ID || input.owner == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.receiver == NULL_ID || (qpi.isContractId(input.receiver) && input.receiver.u64._0 <= SELF_INDEX))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		if (state.get().collections.get(locals.item.collectionId).externalTransfersDisabled && qpi.isContractId(input.receiver))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.holdings.owner = input.owner;
		locals.holdings.quantity = input.quantity;
		locals.holdings.itemId = input.itemId;
		CALL(ValidateOwnedUnits, locals.holdings, output);
	}

	/** @brief Accept only an issued native IPO asset and a feasible gross-price allocation. */
	PRIVATE_FUNCTION(CheckSaleDividend)
	{
		output = Code::InvalidInput;
		if (!input.dividendBps && !input.dividendAssetName)
		{
			output = Code::Ok;
			return;
		}

		if (!input.dividendBps || !input.dividendAssetName || state.get().shopFeeBPS > QNFT_BPS_DENOMINATOR)
		{
			return;
		}

		if (static_cast<uint32>(input.dividendBps) + (input.royaltyReceiver == NULL_ID ? 0U : input.royaltyBps) >
		    QNFT_BPS_DENOMINATOR - state.get().shopFeeBPS)
		{
			return;
		}

		if (qpi.numberOfShares(Asset{.issuer = NULL_ID, .assetName = input.dividendAssetName}) != NUMBER_OF_COMPUTORS)
		{
			return;
		}

		output = Code::Ok;
	}

	/** @brief Calculate dividends from current possessors, consolidating recipients and separating QNFT's share.
	 * @note The caller supplies a zero-initialized output buffer; no funds or debt entries are changed.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalculateSaleDividends)
	{
		output.code = Code::Conflict;
		if (input.dividendBps)
		{
			locals.perShare = QPI::div(calculateFeeShare(input.paid, input.dividendBps), static_cast<sint64>(NUMBER_OF_COMPUTORS));

			locals.shares = Asset{.issuer = NULL_ID, .assetName = input.dividendAssetName};
			locals.holders.begin(locals.shares);
			while (!locals.holders.reachedEnd())
			{
				locals.sharesHeld = locals.holders.numberOfPossessedShares();
				if (locals.sharesHeld < 0 || locals.sharesHeld > NUMBER_OF_COMPUTORS - locals.totalShares)
				{
					return;
				}

				locals.totalShares = sadd(locals.totalShares, locals.sharesHeld);
				locals.amount = smul(locals.perShare, locals.sharesHeld);
				locals.recipient = locals.holders.possessor();
				if (locals.amount > 0)
				{
					if (locals.recipient == SELF)
					{
						output.selfAmount = sadd(output.selfAmount, locals.amount);
					}
					else
					{
						locals.index = 0;
						while (locals.index < output.count && output.payments.get(locals.index).recipient != locals.recipient)
						{
							++locals.index;
						}

						locals.payment = SalePayment{.recipient = locals.recipient, .amount = locals.amount};
						if (locals.index == output.count)
						{
							++output.count;
						}
						else
						{
							locals.payment.amount = sadd(locals.payment.amount, output.payments.get(locals.index).amount);
						}

						output.payments.set(locals.index, locals.payment);
					}
				}

				locals.holders.next();
			}

			if (locals.totalShares != NUMBER_OF_COMPUTORS)
			{
				return;
			}

			output.shareholderAmount = smul(locals.perShare, static_cast<sint64>(NUMBER_OF_COMPUTORS));
		}

		output.code = Code::Ok;
	}

	/** @brief Combine calculated dividends with royalty and check debt capacity before irreversible NFT delivery. */
	PRIVATE_FUNCTION_WITH_LOCALS(PrepareSalePayments)
	{
		locals.dividendInput.paid = input.paid;
		locals.dividendInput.dividendAssetName = input.item.dividendAssetName;
		locals.dividendInput.dividendBps = input.item.dividendBps;
		CALL(CalculateSaleDividends, locals.dividendInput, output);
		if (output.code != Code::Ok)
		{
			return;
		}

		// A shared royalty/dividend recipient needs one reserved slot and one payment.
		if (input.royalty > 0 && input.item.royaltyReceiver != input.seller)
		{
			locals.index = 0;
			while (locals.index < output.count && output.payments.get(locals.index).recipient != input.item.royaltyReceiver)
			{
				++locals.index;
			}

			locals.payment = SalePayment{.recipient = input.item.royaltyReceiver, .amount = input.royalty};
			if (locals.index == output.count)
			{
				++output.count;
			}
			else
			{
				locals.payment.amount = sadd(locals.payment.amount, output.payments.get(locals.index).amount);
			}

			output.payments.set(locals.index, locals.payment);
		}

		output.code = Code::Capacity;
		for (locals.index = 0; locals.index < output.count; ++locals.index)
		{
			locals.payment = output.payments.get(locals.index);
			locals.pending = 0;
			if (!state.get().contractRefunds.get(locals.payment.recipient, locals.pending))
			{
				++locals.missingSlots;
			}

			if (locals.pending > MAX_AMOUNT - locals.payment.amount)
			{
				return;
			}
		}

		if (locals.missingSlots > QNFT_CONTRACT_REFUND_SLOTS - state.get().contractRefunds.population() ||
		    state.get().epochFees > MAX_AMOUNT - output.selfAmount)
		{
			return;
		}

		output.code = Code::Ok;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateMint)
	{
		CALL(CheckSaleDividend, input, locals.dividendCode);
		if (locals.dividendCode != Code::Ok)
		{
			output.result.code = locals.dividendCode;
			return;
		}

		// Validate the complete edition and its collection before making an irreversible core issuance call.

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.receiver == NULL_ID || qpi.isContractId(input.receiver))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.metadata.contentHash == NULL_ID)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.policy > Policy::Mutable)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.royaltyBps > QNFT_BPS_DENOMINATOR)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (!CheckMetadata(input.metadata, locals.metadataScratch))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.collectionId >= state.get().collectionCount)
		{
			output.result.code = Code::NotFound;
			return;
		}

		if (state.get().mintingStatus.get(input.collectionId) != MintingStatus::Open)
		{
			output.result.code = Code::Conflict;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (!IsCollectionManager(locals.collection, qpi.invocator()))
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		// Honor both global item capacity and the collection's cumulative issuance limits.
		if (state.get().itemCount >= QNFT_MAX_ITEMS || remainingCollectionItems(locals.collection) == 0 ||
		    input.quantity > remainingCollectionUnits(locals.collection))
		{
			output.result.code = Code::Capacity;
			return;
		}

		output.result.code = Code::Ok;
	}

	/** @brief Check owner-wide permission; export callbacks cannot move this item. */
	PRIVATE_FUNCTION_WITH_LOCALS(isOwnerOrApprovedOperator)
	{
		if (input.owner == NULL_ID || input.itemId >= state.get().itemCount || state.get().exporting.get(input.itemId))
		{
			return;
		}

		output.authorized = qpi.invocator() == input.owner;
		if (output.authorized)
		{
			return;
		}

		locals.operatorKey.owner = input.owner;
		locals.operatorKey.operatorId = qpi.invocator();
		state.get().operatorApprovals.get(qpi.K12(locals.operatorKey), locals.operatorApproved);
		output.authorized = locals.operatorApproved;
	}

	/**
	 * @brief Resolve a canonical QNFT asset name to an allocated item.
	 * @note Returns a registry identity only; callers must independently check ownership, balances and lifecycle.
	 * @note itemId is meaningful only when valid is true.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(Lookup)
	{
		// Reject a different issuer even when its asset name matches a registered NFT.
		if (input.asset.issuer != SELF)
		{
			return;
		}

		// Decode the fixed-width numeric part, most significant digit first.
		for (locals.i = 1; locals.i <= QNFT_ASSET_NAME_DIGITS; ++locals.i)
		{
			// Read one character from the packed asset name; digits map to 0-9 and letters to 10-35.
			locals.digit = static_cast<uint32>((input.asset.assetName >> (locals.i * QNFT_ASSET_NAME_BYTE_BITS)) & QNFT_ASSET_NAME_BYTE_MASK);
			if (isInRange<uint32>(locals.digit, Ch::_0, Ch::_9))
			{
				locals.digit -= Ch::_0;
			}
			else if (isInRange<uint32>(locals.digit, Ch::A, Ch::Z))
			{
				locals.digit = locals.digit - Ch::A + QNFT_ASSET_NAME_DECIMAL_DIGITS;
			}
			else
			{
				// Reject lowercase letters and all other characters outside the canonical alphabet.
				return;
			}

			// Append this digit to the numeric ID accumulated from the preceding characters.
			output.itemId = output.itemId * QNFT_ASSET_NAME_RADIX + locals.digit;
		}

		// Reuse the minting generator to validate the entire name, including prefix and unused bytes.
		// Changes to the generated prefix are automatically reflected here.
		locals.assetName = EncodeAssetName(output.itemId, locals.nameScratch);
		if (input.asset.assetName != locals.assetName)
		{
			return;
		}

		// A well-formed name must also refer to an allocated item with exactly this stored asset name.
		// The caller may use itemId only when valid is true; ownership and balances are checked separately.
		output.valid = output.itemId < state.get().itemCount && state.get().items.get(output.itemId).assetName == input.asset.assetName;
	}

	/** @brief Return editable metadata state only to the collection creator or current administrator. */
	PRIVATE_FUNCTION(ValidateMetadataAccess)
	{
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		output.item = state.get().items.get(input.itemId);
		if (output.item.frozen)
		{
			output.code = Code::Frozen;
			return;
		}

		if (!IsCollectionManager(state.get().collections.get(output.item.collectionId), qpi.invocator()))
		{
			output.code = Code::Unauthorized;
			return;
		}

		output.code = Code::Ok;
	}

	/** @brief Validate price, metadata, quantity and payment backing without moving assets or QU. */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateBuy)
	{
		// Reject stale prices, changed metadata and excessive quantities before accepting payment or delivering escrow.
		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || input.receiver == NULL_ID || qpi.isContractId(input.receiver) ||
		    !isInRange<sint64>(input.unitPrice, 1, MAX_AMOUNT))
		{
			output.result.code = Code::InvalidInput;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (!state.get().listings.get(input.listingId, output.listing))
		{
			output.result.code = Code::NotFound;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		output.item = state.get().items.get(output.listing.itemId);
		if (!validSaleRates(output.item, state.get().shopFeeBPS))
		{
			output.result.code = Code::Conflict;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (input.unitPrice != output.listing.unitPrice)
		{
			output.result.code = Code::Conflict;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (input.metadataVersion != output.listing.metadataVersion || output.item.metadataVersion != output.listing.metadataVersion)
		{
			output.result.code = Code::Conflict;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (input.quantity > output.listing.quantity)
		{
			output.result.code = Code::Conflict;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (input.receiver == output.listing.seller || qpi.invocator() == output.listing.seller)
		{
			output.result.code = Code::Conflict;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		if (input.quantity > QPI::div(MAX_AMOUNT, output.listing.unitPrice))
		{
			output.result.code = Code::InvalidInput;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		// The division above proves quantity * unitPrice <= MAX_AMOUNT.
		output.result.paid = input.quantity * output.listing.unitPrice;
		if (qpi.invocationReward() < output.result.paid)
		{
			output.result.code = Code::InsufficientFee;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		// A positive invocation reward covers both payouts and the caller's change.
		// Callback-invoked procedures receive zero reward and cannot pass the paid-price check above.
		qpi.getEntity(SELF, locals.contractBalance);
		if (locals.contractBalance.incomingAmount - locals.contractBalance.outgoingAmount < qpi.invocationReward())
		{
			output.result.code = Code::InsufficientBalance;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		// Discard fractional QU without imposing a minimum commission.
		output.result.fee = calculateFeeShare(output.result.paid, state.get().shopFeeBPS);
		output.result.royalty = calculateFeeShare(output.result.paid, effectiveRoyaltyBps(output.item));
	}

	/**
	 * @brief Reject duplicate/unknown variables, invalid ranges, noncanonical tails and inconsistent splits.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateFeeBatch)
	{
		// Validate the entire batch before applying any entry, including unused trailing slots.
		output.valid = false;
		locals.rentalProtocol = state.get().rentalProtocolBPS;
		locals.rentalCreator = state.get().rentalCreatorBPS;
		if (!isInRange<uint32>(input.count, 1, QNFT_FEE_VARIABLE_COUNT))
		{
			return;
		}

		for (locals.i = 0; locals.i < input.changes.capacity(); ++locals.i)
		{
			if (locals.i >= input.count)
			{
				if (input.changes.get(locals.i).variable != FeeVariable::CollectionFee || input.changes.get(locals.i).value)
				{
					return;
				}

				continue;
			}

			if (!isGovernedValueValid(input.changes.get(locals.i).variable, input.changes.get(locals.i).value, state.get().developerFeeBPS))
			{
				return;
			}

			if (locals.seen & (1U << static_cast<uint32>(input.changes.get(locals.i).variable)))
			{
				return;
			}

			locals.seen |= 1U << static_cast<uint32>(input.changes.get(locals.i).variable);
			if (input.changes.get(locals.i).variable == FeeVariable::RentalProtocolBPS)
			{
				locals.rentalProtocol = input.changes.get(locals.i).value;
			}
			if (input.changes.get(locals.i).variable == FeeVariable::RentalCreatorBPS)
			{
				locals.rentalCreator = input.changes.get(locals.i).value;
			}
			if (input.changes.get(locals.i).variable == FeeVariable::ShareholderFeeBPS)
			{
				locals.shareholder = input.changes.get(locals.i).value;
			}

			if (input.changes.get(locals.i).variable == FeeVariable::BurnFeeBPS)
			{
				locals.burn = input.changes.get(locals.i).value;
			}
		}

		// Validate the final pair so a batch can rebalance rates without an invalid intermediate split.
		if (locals.rentalProtocol + locals.rentalCreator > QNFT_BPS_DENOMINATOR)
		{
			return;
		}

		// When both adjustable shares are proposed, their combined split must still total 100 percent.
		if ((locals.seen & QNFT_REVENUE_SPLIT_VARIABLE_MASK) == QNFT_REVENUE_SPLIT_VARIABLE_MASK &&
		    locals.shareholder + locals.burn + state.get().developerFeeBPS != QNFT_BPS_DENOMINATOR)
		{
			return;
		}

		output.valid = true;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ProjectRental)
	{
		output.rental = input;
		output.code = Code::Ok;
		locals.now = qpi.now();
		if (!IsTimeValid(locals.now))
		{
			output.code = Code::Conflict;
			return;
		}

		if (input.status == RentalStatus::Ended || locals.now < input.paidUntil)
		{
			return;
		}

		ProjectRentalPeriods(input, locals.now, locals.period);
		output.rental.paidUntil = locals.period.paidUntil;
		if (locals.period.periodsPaid)
		{
			output.charged = smul(static_cast<sint64>(locals.period.periodsPaid), input.periodPrice);
			locals.protocolPerPeriod = calculateFeeShare(input.periodPrice, state.get().rentalProtocolBPS);
			locals.creatorPerPeriod = calculateFeeShare(input.periodPrice, state.get().rentalCreatorBPS);
			output.protocolPaid = smul(static_cast<sint64>(locals.period.periodsPaid), locals.protocolPerPeriod);
			output.rental.deposit -= output.charged;
			output.rental.ownerCredit +=
			    smul(static_cast<sint64>(locals.period.periodsPaid), input.periodPrice - locals.protocolPerPeriod - locals.creatorPerPeriod);
			output.rental.creatorCredit += smul(static_cast<sint64>(locals.period.periodsPaid), locals.creatorPerPeriod);
		}

		if (!locals.period.active)
		{
			output.rental.status = RentalStatus::Ended;
			output.rental.autoRenew = false;
		}
	}

	/** @brief Read movable owned units without calculating income or inspecting incoming rentals. */
	PRIVATE_FUNCTION_WITH_LOCALS(TransferableBalance)
	{
		setMemory(output, 0);
		if (input.itemId >= state.get().itemCount || input.owner == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.keyInput = input;
		locals.key = qpi.K12(locals.keyInput);
		state.get().offeredRentalQuantities.get(locals.key, locals.offeredQuantity);
		output.availableQuantity =
		    qpi.numberOfPossessedShares(state.get().items.get(input.itemId).assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX) -
		    locals.offeredQuantity;

		locals.now = qpi.now();
		if (!IsTimeValid(locals.now))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.index = state.get().lentRentals.headIndex(locals.key);
		while (locals.index != NULL_INDEX)
		{
			if (state.get().rentals.get(state.get().lentRentals.element(locals.index), locals.rental))
			{
				// Funded renewals keep units locked even before the tick queue settles them.
				ProjectRentalPeriods(locals.rental, locals.now, locals.period);
				if (locals.period.active)
				{
					output.availableQuantity -= locals.rental.quantity;
				}
			}

			locals.index = state.get().lentRentals.nextElementIndex(locals.index);
		}

		if (output.availableQuantity < 0)
		{
			output.availableQuantity = 0;
		}

		output.code = Code::Ok;
	}

	/** @brief Effective quantities; expired but unprocessed records never keep units locked. */
	PRIVATE_FUNCTION_WITH_LOCALS(RentalBalance)
	{
		setMemory(output, 0);
		if (input.itemId >= state.get().itemCount || input.owner == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.keyInput.owner = input.owner;
		locals.keyInput.itemId = input.itemId;
		locals.key = qpi.K12(locals.keyInput);
		state.get().offeredRentalQuantities.get(locals.key, output.offeredQuantity);
		output.managedQuantity =
		    qpi.numberOfPossessedShares(state.get().items.get(input.itemId).assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX);

		locals.index = state.get().lentRentals.headIndex(locals.key);
		while (locals.index != NULL_INDEX)
		{
			if (state.get().rentals.get(state.get().lentRentals.element(locals.index), locals.rental))
			{
				CALL(ProjectRental, locals.rental, locals.projected);
				if (locals.projected.code != Code::Ok)
				{
					output.code = locals.projected.code;
					return;
				}

				if (locals.projected.rental.status == RentalStatus::Active)
				{
					output.lentQuantity += locals.rental.quantity;
				}
			}

			locals.index = state.get().lentRentals.nextElementIndex(locals.index);
		}

		locals.index = state.get().takenRentals.headIndex(locals.key);
		while (locals.index != NULL_INDEX)
		{
			if (state.get().rentals.get(state.get().takenRentals.element(locals.index), locals.rental))
			{
				CALL(ProjectRental, locals.rental, locals.projected);
				if (locals.projected.code != Code::Ok)
				{
					output.code = locals.projected.code;
					return;
				}

				if (locals.projected.rental.status == RentalStatus::Active)
				{
					output.rentedQuantity += locals.rental.quantity;
				}
			}

			locals.index = state.get().takenRentals.nextElementIndex(locals.index);
		}

		output.availableQuantity = output.managedQuantity - output.offeredQuantity - output.lentQuantity;
		if (output.availableQuantity < 0)
		{
			output.availableQuantity = 0;
		}

		output.code = Code::Ok;
	}

	/**
	 * @brief Read persistent owner-wide authority without changing state. Returns InvalidInput for zero identities.
	 * Otherwise returns Ok and the stored flag.
	 */
	PUBLIC_FUNCTION(IsApprovedForAll)
	{
		if (input.owner == NULL_ID || input.operatorId == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		state.get().operatorApprovals.get(qpi.K12(input), output.approved);
		output.code = Code::Ok;
	}

	/** @brief Read ownership quantity; a valid account with an unknown ID has zero balance. */
	PUBLIC_FUNCTION_WITH_LOCALS(BalanceOf)
	{
		if (input.owner == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.code = Code::Ok;
		if (input.itemId >= state.get().itemCount)
		{
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		output.balance = qpi.numberOfShares(locals.asset, AssetOwnershipSelect::byOwner(input.owner), AssetPossessionSelect::any());
	}

	/** @brief Read up to 16 ordered owner/ID pairs; invalid requests expose no partial result. */
	PUBLIC_FUNCTION_WITH_LOCALS(BalanceOfBatch)
	{
		if (input.count > QNFT_TOKEN_BATCH_CAPACITY)
		{
			output.code = Code::InvalidInput;
			return;
		}

		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.query = input.entries.get(locals.i);
			setMemory(locals.result, 0);
			CALL(BalanceOf, locals.query, locals.result);
			if (locals.result.code != Code::Ok)
			{
				setMemory(output, 0);
				output.code = locals.result.code;
				return;
			}

			output.balances.set(locals.i, locals.result.balance);
		}

		output.count = input.count;
		output.code = Code::Ok;
	}

	/** @brief Current supply for one ID across all managers; burned and unknown IDs return zero. */
	PUBLIC_FUNCTION_WITH_LOCALS(TotalSupply)
	{
		output.code = Code::Ok;
		if (input.itemId >= state.get().itemCount)
		{
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		output.supply = qpi.numberOfShares(locals.asset);
	}

	/** @brief Return the registered per-ID metadata URI, including after export or burn. */
	PUBLIC_FUNCTION(URI)
	{
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		output.uri = state.get().items.get(input.itemId).metadata.uri;
		output.code = Code::Ok;
	}

	/** @brief Quote floor(full sale price * bps / 10000) in the caller's settlement units.
	 * @return Receiver and amount; InvalidInput for negative prices, NotFound for absent/burned IDs.
	 * @note Read-only EIP-2981 adaptation. The marketplace pays; no shop fee is deducted.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(RoyaltyInfo)
	{
		if (input.salePrice < 0)
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.query.itemId = input.itemId;
		CALL(TotalSupply, locals.query, locals.supply);
		if (locals.supply.supply <= 0)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.rate = effectiveRoyaltyBps(state.get().items.get(input.itemId));
		if (locals.rate > QNFT_BPS_DENOMINATOR)
		{
			output.code = Code::Conflict;
			return;
		}

		output.receiver = state.get().items.get(input.itemId).royaltyReceiver;
		output.royaltyAmount = calculateFeeShare(input.salePrice, locals.rate);
		output.code = Code::Ok;
	}

	/** @brief Quote the sale allocation after rounding to a whole QU amount per IPO share.
	 * @param input Item ID and nonnegative gross sale price in QU, before fees and royalties.
	 * @return Selected shares, rate, per-share amount and total; errors leave all quote fields zero.
	 * @note Read-only. Ignores shop fees and current holders; Ok does not guarantee a purchase can execute.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(DividendInfo)
	{
		setMemory(output, 0);
		if (input.salePrice < 0)
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.query.itemId = input.itemId;
		CALL(TotalSupply, locals.query, locals.supply);
		if (locals.supply.supply <= 0)
		{
			output.code = Code::NotFound;
			return;
		}

		if (state.get().items.get(input.itemId).dividendBps > QNFT_BPS_DENOMINATOR ||
		    (state.get().items.get(input.itemId).dividendBps == 0) != (state.get().items.get(input.itemId).dividendAssetName == 0))
		{
			output.code = Code::Conflict;
			return;
		}

		output.dividendAssetName = state.get().items.get(input.itemId).dividendAssetName;
		output.dividendBps = state.get().items.get(input.itemId).dividendBps;
		output.amountPerShare = QPI::div(calculateFeeShare(input.salePrice, output.dividendBps), static_cast<sint64>(NUMBER_OF_COMPUTORS));
		output.shareholderAmount = smul(output.amountPerShare, static_cast<sint64>(NUMBER_OF_COMPUTORS));
		output.code = Code::Ok;
	}

	/** @brief Describe the native protocol; capability bits never claim Ethereum ABI or receiver hooks. */
	PUBLIC_FUNCTION(GetProtocolInfo)
	{
		// Native features include rentals (2048) and mutable collection transfer policy (4096).
		output.capabilities = 1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024 | 2048 | 4096;
		output.maxTokenId = QNFT_MAX_ITEMS - 1;
		output.majorVersion = 1;
		output.minorVersion = 0;
		output.contractIndex = SELF_INDEX;
		output.addressBytes = 32;
		output.maxUriBytes = QNFT_METADATA_URI_CAPACITY - 1;
	}

	/** @brief Read global live supply and units whose ownership is managed by QNFT; no cached balances. */
	PUBLIC_FUNCTION_WITH_LOCALS(GetAssetState)
	{
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		output.supply = qpi.numberOfShares(locals.asset);
		output.managedSupply = qpi.numberOfShares(locals.asset, AssetOwnershipSelect::byManagingContract(SELF_INDEX));
		output.code = Code::Ok;
	}

	/** @brief Page through native possession records, including external managers.
	 * @note Cursor counts ledger records, including historical zero entries. Use a fixed node snapshot.
	 * expectedTick detects advancing ticks but does not freeze intra-tick state. Skipping costs O(cursor).
	 * @return Positive holdings and continuation; zero-balance records consume scan budget but are omitted.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ListHolders)
	{
		output.tick = qpi.tick();
		if (!isInRange<uint32>(input.limit, 1, QNFT_TOKEN_BATCH_CAPACITY))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.expectedTick && input.expectedTick != output.tick)
		{
			output.code = Code::Conflict;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		locals.iterator.begin(locals.asset);
		while (!locals.iterator.reachedEnd() && locals.skipped < input.cursor)
		{
			locals.iterator.next();
			++locals.skipped;
		}

		if (locals.skipped != input.cursor)
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.nextCursor = input.cursor;
		while (!locals.iterator.reachedEnd() && locals.scanned < input.limit)
		{
			if (locals.iterator.numberOfPossessedShares() > 0)
			{
				locals.entry.owner = locals.iterator.owner();
				locals.entry.possessor = locals.iterator.possessor();
				locals.entry.quantity = locals.iterator.numberOfPossessedShares();
				locals.entry.ownershipManager = locals.iterator.ownershipManagingContract();
				locals.entry.possessionManager = locals.iterator.possessionManagingContract();
				output.entries.set(output.count, locals.entry);
				++output.count;
			}

			++locals.scanned;
			++output.nextCursor;
			locals.iterator.next();
		}

		output.done = locals.iterator.reachedEnd();
		output.code = Code::Ok;
	}

	/** @brief Read issuance status and remaining lifetime quota; remaining counts are not spendable while closed/paused. */
	PUBLIC_FUNCTION_WITH_LOCALS(GetMintingStatus)
	{
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		output.remainingItems = remainingCollectionItems(locals.collection);
		output.remainingUnits = remainingCollectionUnits(locals.collection);
		output.status = state.get().mintingStatus.get(input.collectionId);
		output.code = Code::Ok;
	}

	/** @brief Report live supply, not URI registration; false after complete local or external burn. */
	PUBLIC_FUNCTION_WITH_LOCALS(Exists)
	{
		CALL(TotalSupply, input, locals.supply);
		output.code = locals.supply.code;
		output.exists = locals.supply.supply > 0;
	}

	/** @brief Resolve every literal {id} using 64 lowercase hexadecimal digits without changing the stored URI.
	 * @return Capacity and an empty URI if expansion exceeds 255 bytes. URI itself still returns the exact event value.
	 * @note Localization and substitutions inside off-chain JSON remain client responsibilities.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ResolvedURI)
	{
		CALL(URI, input, locals.stored);
		output.code = locals.stored.code;
		if (output.code != Code::Ok)
		{
			return;
		}

		while (locals.source < QNFT_METADATA_URI_CAPACITY && locals.stored.uri.get(locals.source))
		{
			if (locals.source + 3 < QNFT_METADATA_URI_CAPACITY && locals.stored.uri.get(locals.source) == 123 &&
			    locals.stored.uri.get(locals.source + 1) == 105 && locals.stored.uri.get(locals.source + 2) == 100 &&
			    locals.stored.uri.get(locals.source + 3) == 125)
			{
				if (locals.target + 64 >= QNFT_METADATA_URI_CAPACITY)
				{
					setMemory(output, 0);
					output.code = Code::Capacity;
					return;
				}

				for (locals.i = 0; locals.i < 64; ++locals.i)
				{
					locals.digit = locals.i < 56 ? 0 : (input.itemId >> ((63 - locals.i) * 4)) & 15;
					output.uri.set(locals.target, static_cast<uint8>(locals.digit < 10 ? 48 + locals.digit : 87 + locals.digit));
					++locals.target;
				}

				locals.source += 4;
			}
			else
			{
				if (locals.target + 1 >= QNFT_METADATA_URI_CAPACITY)
				{
					setMemory(output, 0);
					output.code = Code::Capacity;
					return;
				}

				output.uri.set(locals.target, locals.stored.uri.get(locals.source));
				++locals.target;
				++locals.source;
			}
		}
	}

	/** @brief Expose the retained interval without log-reader credentials. */
	PUBLIC_FUNCTION(GetEventRange)
	{
		// The retained window is shorter than capacity until the ring fills for the first time.
		output.nextEventId = state.get().lastEventId + 1;
		output.count = static_cast<uint32>(minimum(state.get().lastEventId, static_cast<uint64>(QNFT_EVENT_CAPACITY)));
		output.firstEventId = output.nextEventId - output.count;
		output.capacity = QNFT_EVENT_CAPACITY;
	}

	/** @brief Read a bounded chronological page and explicitly reject lost history. */
	PUBLIC_FUNCTION(GetEvents)
	{
		// Resolve the requested cursor against the currently retained interval before reading ring slots.
		output.nextEventId = state.get().lastEventId + 1;
		output.firstEventId = output.nextEventId - minimum(state.get().lastEventId, static_cast<uint64>(QNFT_EVENT_CAPACITY));
		output.cursor = input.eventId ? input.eventId : output.firstEventId;
		if (!isInRange<uint32>(input.limit, 1, QNFT_EVENT_PAGE_CAPACITY) || output.cursor > output.nextEventId)
		{
			output.code = Code::InvalidInput;
			return;
		}

		// Report overwritten history explicitly instead of silently skipping missing events.
		if (output.cursor < output.firstEventId)
		{
			output.code = Code::NotFound;
			return;
		}

		while (output.count < input.limit && output.cursor < output.nextEventId)
		{
			output.events.set(output.count, state.get().journal.get((output.cursor - 1) & QNFT_EVENT_SLOT_MASK));
			++output.count;
			++output.cursor;
		}

		output.code = Code::Ok;
	}

	/** @brief Return full collection records in a bounded page with explicit collection IDs. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListCollections)
	{
		if (!isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY) || input.offset > state.get().collectionCount)
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.code = Code::Ok;
		output.totalCount = state.get().collectionCount;
		output.nextOffset = input.offset;
		// Collections have permanent contiguous IDs, so the next registry index is a stable cursor.
		while (output.count < input.limit && output.nextOffset < output.totalCount)
		{
			locals.entry.collectionId = output.nextOffset;
			locals.entry.collection = state.get().collections.get(output.nextOffset);
			output.collections.set(output.count, locals.entry);
			++output.count;
			++output.nextOffset;
		}
	}

	/**
	 * @brief Read issuance and metadata; derive network-wide burns from the current ledger.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetItem)
	{
		// Read the registry identity first, then reconcile burned units against the current core ledger.
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		output.item = state.get().items.get(input.itemId);
		if (output.item.supply > 0)
		{
			locals.asset = Asset{.issuer = SELF, .assetName = output.item.assetName};
			output.item.burned = output.item.supply - qpi.numberOfShares(locals.asset);
		}

		output.code = Code::Ok;
	}

	/**
	 * @brief Read collection limits and administration.
	 */
	PUBLIC_FUNCTION(GetCollection)
	{
		// Check allocation before reading the fixed-size array, whose unused entries are zero-initialized.
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		output.collection = state.get().collections.get(input.collectionId);
		output.code = Code::Ok;
	}

	/**
	 * @brief Read ledger quantity owned and possessed by an account, optionally by manager.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetBalance)
	{
		// Select both ownership and possession for the account; manager zero keeps all managing contracts in scope.
		if (input.owner == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		locals.ownership = AssetOwnershipSelect::byOwner(input.owner);
		locals.possession = AssetPossessionSelect::byPossessor(input.owner);
		if (input.managingContract)
		{
			locals.ownership.anyManagingContract = false;
			locals.ownership.managingContract = input.managingContract;
			locals.possession.anyManagingContract = false;
			locals.possession.managingContract = input.managingContract;
		}

		output.quantity = qpi.numberOfShares(locals.asset, locals.ownership, locals.possession);
		output.code = Code::Ok;
	}

	/** @brief Read one pending refund without invoking its recipient. */
	PUBLIC_FUNCTION(GetRefund)
	{
		if (input.owner == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		// Missing entries are successful zero balances; payouts remain automatic.
		state.get().contractRefunds.get(input.owner, output.quantity);
		output.code = Code::Ok;
	}

	/** @brief Enumerate pending refunds, royalties and sale dividends without transfers or recipient callbacks. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListRefunds)
	{
		if (!isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY) || input.offset > QNFT_CONTRACT_REFUND_SLOTS)
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.nextOffset = QNFT_CONTRACT_REFUND_SLOTS;
		// Count the current positive obligations; a credit cleared during repayment is not pending.
		for (locals.index = state.get().contractRefunds.nextElementIndex(NULL_INDEX); locals.index != NULL_INDEX;
		     locals.index = state.get().contractRefunds.nextElementIndex(locals.index))
		{
			locals.entry.amount = state.get().contractRefunds.value(locals.index);
			if (locals.entry.amount <= 0)
			{
				continue;
			}

			++output.totalCount;
			if (locals.index < input.offset)
			{
				continue;
			}

			if (output.count == input.limit)
			{
				// Resume at the first positive record not included in this page.
				if (output.nextOffset == QNFT_CONTRACT_REFUND_SLOTS)
				{
					output.nextOffset = static_cast<uint32>(locals.index);
				}

				continue;
			}

			locals.entry.recipient = state.get().contractRefunds.key(locals.index);
			output.refunds.set(output.count, locals.entry);
			++output.count;
		}

		output.code = Code::Ok;
	}

	/**
	 * @brief Describe ABI version, development fees and fixed storage ceilings.
	 */
	PUBLIC_FUNCTION(GetConfig)
	{
		// Expose allocated counts separately from capacity limits.
		output.issuer = SELF;
		output.stateBytes = sizeof(StateData);
		output.collectionFee = state.get().collectionFee;
		output.mintFee = state.get().mintFee;
		output.maxItems = QNFT_MAX_ITEMS;
		output.maxCollections = QNFT_MAX_COLLECTIONS;
		output.itemCount = state.get().itemCount;
		output.collectionCount = state.get().collectionCount;
		output.abiVersion = 1;
	}

	/** @brief Return full allocated items in a bounded page. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListItems)
	{
		if (!isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY) || input.offset > state.get().itemCount)
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.totalCount = state.get().itemCount;
		output.nextOffset = input.offset;
		// Permanent contiguous IDs provide a stable cursor; reuse GetItem for identical ledger reconciliation.
		while (output.count < input.limit && output.nextOffset < output.totalCount)
		{
			locals.itemInput.itemId = output.nextOffset;
			CALL(GetItem, locals.itemInput, locals.itemOutput);
			locals.entry.item = locals.itemOutput.item;
			locals.entry.itemId = output.nextOffset;
			output.items.set(output.count, locals.entry);
			++output.count;
			++output.nextOffset;
		}

		output.code = Code::Ok;
	}

	/**
	 * @brief Read an active listing by its globally unique ID.
	 */
	PUBLIC_FUNCTION(GetListing)
	{
		// Completed and cancelled listings have been removed and return NotFound.
		output.code = state.get().listings.get(input.listingId, output.listing) ? Code::Ok : Code::NotFound;
	}

	/**
	 * @brief Enumerate active listing IDs by map slot, at most 32 per page.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ListListings)
	{
		// The cursor is a map slot, not a listing ID; callers restart pagination after epoch compaction.
		if (!isInRange<uint32>(input.limit, 1, QNFT_PAGE_CAPACITY) || input.offset > QNFT_LISTING_SLOTS)
		{
			output.code = Code::InvalidInput;
			return;
		}

		for (locals.i = input.offset; locals.i < QNFT_LISTING_SLOTS && output.count < input.limit; ++locals.i)
		{
			if (!state.get().listings.isEmptySlot(locals.i))
			{
				output.listingIds.set(output.count, state.get().listings.key(locals.i));
				++output.count;
			}
		}

		output.nextOffset = locals.i;
		output.code = Code::Ok;
	}

	/**
	 * @brief Read shop configuration and undistributed epoch commission pools.
	 */
	PUBLIC_FUNCTION(GetMarket)
	{
		// Report accumulated revenue separately from unpaid developer and shareholder amounts.
		output.developer = state.get().developer;
		output.epochFees = state.get().epochFees;
		output.developerPending = state.get().developerPending;
		output.shareholderCarry = state.get().shareholderCarry;
		output.lastListingId = state.get().lastListingId;
		output.activeListings = static_cast<uint32>(state.get().listings.population());
		output.feeBps = state.get().shopFeeBPS;
		output.lastDistributionEpoch = state.get().lastDistributionEpoch;
		output.code = Code::Ok;
	}

	/**
	 * @brief Read a batch only when its digest matches the stored native proposal.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetFeeProposalBatch)
	{
		// A reused proposal slot must not expose a batch left over from an earlier proposal.
		if (input.proposalIndex >= QNFT_PROPOSAL_SLOTS || !qpi(state.get().proposals).getProposal(input.proposalIndex, locals.proposal) ||
		    locals.proposal.type != ProposalTypes::MultiVariablesYesNo)
		{
			output.code = Code::NotFound;
			return;
		}

		// Verify the payload digest against the current proposal before exposing the batch.
		locals.batch = state.get().feeBatches.get(input.proposalIndex);
		if (qpi.K12(locals.batch) != locals.proposal.data.transfer.destination)
		{
			output.code = Code::NotFound;
			return;
		}

		output.batch = locals.batch;
		output.proposalTick = locals.proposal.tick;
		output.code = Code::Ok;
	}

	/**
	 * @brief Return a standard proposal and its proposer identity.
	 */
	PUBLIC_FUNCTION(GetShareholderProposal)
	{
		// Read both fields from the same proposal slot; QPI supplies the standard missing-proposal behavior.
		output.proposerPubicKey = qpi(state.get().proposals).proposerId(input.proposalIndex);
		qpi(state.get().proposals).getProposal(input.proposalIndex, output.proposal);
	}

	/**
	 * @brief Enumerate active or historical proposal slots with the standard QPI cursor.
	 */
	PUBLIC_FUNCTION(GetShareholderProposalIndices)
	{
		// Use the appropriate QPI iterator for the current epoch or historical proposals and stop at page capacity.
		if (input.activeProposals)
		{
			while ((input.prevProposalIndex = qpi(state.get().proposals).nextProposalIndex(input.prevProposalIndex, qpi.epoch())) >= 0)
			{
				output.indices.set(output.numOfIndices++, input.prevProposalIndex);
				if (output.numOfIndices == output.indices.capacity())
				{
					break;
				}
			}
		}
		else
		{
			while ((input.prevProposalIndex = qpi(state.get().proposals).nextFinishedProposalIndex(input.prevProposalIndex)) >= 0)
			{
				output.indices.set(output.numOfIndices++, input.prevProposalIndex);
				if (output.numOfIndices == output.indices.capacity())
				{
					break;
				}
			}
		}
	}

	/**
	 * @brief Read all governed tariffs; recovery operations and voting remain free.
	 */
	PUBLIC_FUNCTION(GetFeeSchedule)
	{
		// Return current state values so clients quote fees after any accepted governance change.
		output.collectionFee = state.get().collectionFee;
		output.mintFee = state.get().mintFee;
		output.nominateAdminFee = state.get().nominateAdminFee;
		output.transferFee = state.get().transferFee;
		output.approveFee = state.get().approveFee;
		output.updateMetadataFee = state.get().updateMetadataFee;
		output.exportFee = state.get().exportFee;
		output.listingFee = state.get().listingFee;
		output.proposalFee = state.get().proposalFee;
		output.shopFeeBPS = state.get().shopFeeBPS;
		output.rentalProtocolBPS = state.get().rentalProtocolBPS;
		output.rentalCreatorBPS = state.get().rentalCreatorBPS;
	}

	/**
	 * @brief Report the standard proposal and vote invocation fees in QU.
	 */
	PUBLIC_FUNCTION(GetShareholderProposalFees)
	{
		// Proposal submission uses the governed tariff; voting itself never retains a fee.
		output.setProposalFee = state.get().proposalFee;
		output.setVoteFee = 0;
	}

	/**
	 * @brief Return the selected voter's current versioned ballot.
	 */
	PUBLIC_FUNCTION(GetShareholderVotes)
	{
		// Let QPI read the voter's versioned ballot rather than reconstructing votes from logs.
		qpi(state.get().proposals).getVotes(input.proposalIndex, input.voter, output);
	}

	/**
	 * @brief Return QPI's weighted yes/no totals and quorum data.
	 */
	PUBLIC_FUNCTION(GetShareholderVotingResults)
	{
		// Return QPI's snapshot-weighted result, including the quorum data used for acceptance.
		qpi(state.get().proposals).getVotingSummary(input.proposalIndex, output);
	}

	/**
	 * @brief Expose revenue rates and the proposal capacity without changing existing query layouts.
	 */
	PUBLIC_FUNCTION(GetGovernanceConfig)
	{
		// Expose the current split, including the fixed developer share, and the last finalized governance epoch.
		output.developerFeeBPS = state.get().developerFeeBPS;
		output.shareholderFeeBPS = state.get().shareholderFeeBPS;
		output.burnFeeBPS = state.get().burnFeeBPS;
		output.lastGovernanceEpoch = state.get().lastGovernanceEpoch;
		output.proposalSlots = QNFT_PROPOSAL_SLOTS;
	}

	/** @brief Return effective usage and collateral for one rental without mutation; NotFound for missing IDs and Conflict for invalid current time.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetRental)
	{
		if (!state.get().rentals.get(input.rentalId, locals.rental))
		{
			output.code = Code::NotFound;
			return;
		}

		CALL(ProjectRental, locals.rental, locals.projected);
		DescribeRental(locals.projected.rental, output.rental);
		output.code = locals.projected.code;
	}

	/** @brief Return the remaining unpaid offer by ID; NotFound after cancellation or full acceptance. Does not change reservations. */
	PUBLIC_FUNCTION(GetRentalOffer)
	{
		if (!state.get().rentalOffers.get(input.offerId, output.offer))
		{
			output.code = Code::NotFound;
		}
	}

	/** @brief Read a bounded offer page, optionally filtered by owner; rejects invalid limits/cursors and stale nonzero ticks. Continue with
	 * nextCursor until done. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListRentalOffers)
	{
		output.tick = qpi.tick();
		output.nextCursor = input.cursor;
		if (!isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY) || input.cursor > state.get().rentalOffers.capacity())
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.tick && input.tick != qpi.tick())
		{
			output.code = Code::Conflict;
			return;
		}

		while (output.nextCursor < state.get().rentalOffers.capacity() && output.count < input.limit && locals.scanned < QNFT_RENTAL_PAGE_SCAN)
		{
			if (!state.get().rentalOffers.isEmptySlot(output.nextCursor))
			{
				locals.offer = state.get().rentalOffers.value(output.nextCursor);
				if (input.owner == NULL_ID || input.owner == locals.offer.owner)
				{
					output.offers.set(output.count++, locals.offer);
				}
			}

			++output.nextCursor;
			++locals.scanned;
		}

		output.done = output.nextCursor == state.get().rentalOffers.capacity();
	}

	/** @brief Read effective rentals for one participant and role; rejects invalid paging, stale ticks or invalid projection time. Ended records may
	 * retain collateral. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListUserRentals)
	{
		output.tick = qpi.tick();
		output.nextCursor = input.cursor;
		if (input.user == NULL_ID || input.role > RentalRole::Renter || !isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY) ||
		    input.cursor > state.get().rentals.capacity())
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.tick && input.tick != qpi.tick())
		{
			output.code = Code::Conflict;
			return;
		}

		while (output.nextCursor < state.get().rentals.capacity() && output.count < input.limit && locals.scanned < QNFT_RENTAL_PAGE_SCAN)
		{
			if (!state.get().rentals.isEmptySlot(output.nextCursor))
			{
				locals.rental = state.get().rentals.value(output.nextCursor);
				locals.matches = (input.role != RentalRole::Renter && input.user == locals.rental.owner) ||
				                 (input.role != RentalRole::Lender && input.user == locals.rental.renter);
				if (locals.matches)
				{
					CALL(ProjectRental, locals.rental, locals.projected);
					if (locals.projected.code != Code::Ok)
					{
						output.code = locals.projected.code;
						output.count = 0;
						return;
					}

					DescribeRental(locals.projected.rental, locals.view);
					output.rentals.set(output.count++, locals.view);
				}
			}

			++output.nextCursor;
			++locals.scanned;
		}

		output.done = output.nextCursor == state.get().rentals.capacity();
	}

	/** @brief Read global ownership and separate local reservations/incoming usage for one account/type; invalid identities or failed rental
	 * projection return an error. */
	PUBLIC_FUNCTION_WITH_LOCALS(GetUserNft)
	{
		CALL(RentalBalance, input, locals.balance);
		output.code = locals.balance.code;
		if (output.code != Code::Ok)
		{
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = state.get().items.get(input.itemId).assetName};
		output.ownedQuantity = qpi.numberOfShares(locals.asset, AssetOwnershipSelect::byOwner(input.owner));
		output.managedQuantity = locals.balance.managedQuantity;
		output.offeredQuantity = locals.balance.offeredQuantity;
		output.lentQuantity = locals.balance.lentQuantity;
		output.rentedQuantity = locals.balance.rentedQuantity;
		output.availableQuantity = locals.balance.availableQuantity;
	}

	/** @brief Read owned ledger rows then active incoming usage grants for one user; continue with returned phase/cursor and check code before
	 * consuming a page. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListUserNfts)
	{
		output.tick = qpi.tick();
		output.nextCursor = input.cursor;
		output.phase = input.phase;
		if (input.user == NULL_ID || input.phase > UserNftPhase::IncomingRentals || !isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.tick && input.tick != qpi.tick())
		{
			output.code = Code::Conflict;
			return;
		}

		if (input.phase == UserNftPhase::Ownership)
		{
			locals.iterator.begin(input.user, SELF, input.cursor);
			if (!locals.iterator.isValidCursor())
			{
				output.code = Code::InvalidInput;
				return;
			}

			while (output.count < input.limit && locals.scanned < QNFT_RENTAL_PAGE_SCAN && !locals.iterator.reachedEnd())
			{
				locals.before = locals.iterator.cursor();
				if (!locals.iterator.next(QNFT_RENTAL_PAGE_SCAN - locals.scanned))
				{
					break;
				}

				locals.scanned += locals.iterator.cursor() - locals.before;
				locals.lookup.asset = locals.iterator.asset();
				setMemory(locals.found, 0);
				CALL(Lookup, locals.lookup, locals.found);
				if (locals.found.valid)
				{
					setMemory(locals.entry, 0);
					locals.entry.owner = input.user;
					locals.entry.itemId = locals.found.itemId;
					locals.entry.quantity = locals.iterator.numberOfOwnedShares();
					locals.entry.managingContract = locals.iterator.ownershipManagingContract();
					output.entries.set(output.count++, locals.entry);
				}
			}

			output.nextCursor = locals.iterator.cursor();
			if (locals.iterator.reachedEnd())
			{
				output.phase = UserNftPhase::IncomingRentals;
				output.nextCursor = 0;
			}

			return;
		}

		if (input.cursor > state.get().rentals.capacity())
		{
			output.code = Code::InvalidInput;
			return;
		}

		while (output.nextCursor < state.get().rentals.capacity() && output.count < input.limit && locals.scanned < QNFT_RENTAL_PAGE_SCAN)
		{
			if (!state.get().rentals.isEmptySlot(output.nextCursor))
			{
				locals.rental = state.get().rentals.value(output.nextCursor);
				if (locals.rental.renter == input.user)
				{
					CALL(ProjectRental, locals.rental, locals.projected);
					if (locals.projected.code != Code::Ok)
					{
						output.code = locals.projected.code;
						output.count = 0;
						return;
					}

					if (locals.projected.rental.status == RentalStatus::Active)
					{
						DescribeRental(locals.projected.rental, locals.view);
						setMemory(locals.entry, 0);
						locals.entry.owner = locals.rental.owner;
						locals.entry.renter = input.user;
						locals.entry.rentalId = locals.rental.rentalId;
						locals.entry.itemId = locals.rental.itemId;
						locals.entry.paidUntil = locals.view.paidUntil;
						locals.entry.quantity = locals.rental.quantity;
						locals.entry.relation = UserNftRelation::Rented;
						locals.entry.managingContract = SELF_INDEX;
						locals.entry.autoRenew = locals.rental.autoRenew;
						output.entries.set(output.count++, locals.entry);
					}
				}
			}

			++output.nextCursor;
			++locals.scanned;
		}

		output.done = output.nextCursor == state.get().rentals.capacity();
	}

	/** @brief Execute SetApprovalForAll without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetApprovalForAll)
	{
		locals.key.owner = qpi.invocator();
		locals.key.operatorId = input.operatorId;
		locals.digest = qpi.K12(locals.key);
		output.code = Code::Ok;
		if (input.operatorId == NULL_ID || input.operatorId == qpi.invocator() || qpi.invocator() == NULL_ID || qpi.invocator() == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.approved && qpi.invocationReward() < state.get().approveFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		if (input.approved && !state.get().operatorApprovals.contains(locals.digest) &&
		    state.get().operatorApprovals.population() >= QNFT_MAX_APPROVALS)
		{
			output.code = Code::Capacity;
			return;
		}

		if (input.approved)
		{
			state.mut().operatorApprovals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
			state.mut().operatorApprovals.set(locals.digest, input.approved);
		}
		else
		{
			state.mut().operatorApprovals.removeByKey(locals.digest);
		}

		locals.event.kind = EventKind::ApprovalForAll;
		locals.event.actor = qpi.invocator();
		locals.event.from = qpi.invocator();
		locals.event.to = input.operatorId;
		locals.event.quantity = input.approved;
		CALL(AppendEvent, locals.event, locals.eventOutput);
	}

	/** @brief Execute BatchTransfer without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBatchTransfer)
	{
		output.code = Code::Ok;
		output.failureCode = Code::Ok;
		output.failureIndex = input.count;

		if (input.count > QNFT_TOKEN_BATCH_CAPACITY)
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		if (input.owner == NULL_ID || input.owner == SELF)
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		if (input.receiver == NULL_ID || (qpi.isContractId(input.receiver) && input.receiver.u64._0 <= SELF_INDEX))
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		if (input.count > 0 && state.get().transferFee > QPI::div(MAX_AMOUNT, static_cast<sint64>(input.count)))
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		locals.requiredFee = smul(state.get().transferFee, static_cast<sint64>(input.count));
		if (qpi.invocationReward() < locals.requiredFee)
		{
			output.code = Code::InsufficientFee;
			output.failureCode = output.code;
			return;
		}

		locals.balanceInput.owner = input.owner;
		locals.balanceInput.entries = input.entries;
		locals.request.owner = input.owner;
		locals.request.receiver = input.receiver;
		// Validate the complete request before committing any entry; errors here retain no fee.
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request.itemId = input.entries.get(locals.i).itemId;
			locals.request.quantity = input.entries.get(locals.i).quantity;
			setMemory(locals.result, 0);
			CALL(ValidateTransfer, locals.request, locals.result);
			if (locals.result.code != Code::Ok)
			{
				output.code = locals.result.code;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}

			if (input.owner == input.receiver)
			{
				continue;
			}

			locals.balanceInput.index = locals.i;
			CALL(ValidateBatchBalance, locals.balanceInput, locals.balanceCode);
			if (locals.balanceCode != Code::Ok)
			{
				output.code = locals.balanceCode;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}
		}

		// Core operations do not roll back: preserve the committed prefix if a later execution fails.
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request.itemId = input.entries.get(locals.i).itemId;
			locals.request.quantity = input.entries.get(locals.i).quantity;
			setMemory(locals.result, 0);
			CALL(ExecuteTransfer, locals.request, locals.result);
			output.results.set(locals.i, locals.result);
			++output.attempted;
			if (locals.result.code != Code::Ok)
			{
				output.failureIndex = locals.i;
				output.failureCode = locals.result.code;
				output.code = output.completed ? Code::Pending : locals.result.code;
				return;
			}

			++output.completed;
			output.fee = sadd(output.fee, state.get().transferFee);
		}
	}

	/** @brief Execute MultiMint without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteMultiMint)
	{
		if (input.prototype.quantity != 0 || input.count > QNFT_TOKEN_BATCH_CAPACITY)
		{
			output.code = output.failureCode = Code::InvalidInput;
			output.failureIndex = input.count;
			return;
		}

		locals.batch.count = input.count;
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.item = input.prototype;
			locals.item.quantity = input.quantities.get(locals.i);
			locals.batch.entries.set(locals.i, locals.item);
		}

		CALL(MintMany, locals.batch, output);
	}

	/** @brief Execute BurnBatch without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBurnBatch)
	{
		output.code = Code::Ok;
		output.failureCode = Code::Ok;
		output.failureIndex = input.count;
		if (input.count > QNFT_TOKEN_BATCH_CAPACITY || input.owner == NULL_ID || input.owner == SELF)
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		locals.balanceInput.owner = input.owner;
		locals.balanceInput.entries = input.entries;
		locals.request.owner = input.owner;
		// Validate the complete request before committing any entry; errors here retain no fee.
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request.itemId = input.entries.get(locals.i).itemId;
			locals.request.quantity = input.entries.get(locals.i).quantity;
			setMemory(locals.result, 0);
			CALL(ValidateBurn, locals.request, locals.result);
			if (locals.result.code != Code::Ok)
			{
				output.code = locals.result.code;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}

			locals.balanceInput.index = locals.i;
			CALL(ValidateBatchBalance, locals.balanceInput, locals.balanceCode);
			if (locals.balanceCode != Code::Ok)
			{
				output.code = locals.balanceCode;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}
		}

		// Core operations do not roll back: preserve the committed prefix if a later execution fails.
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request.itemId = input.entries.get(locals.i).itemId;
			locals.request.quantity = input.entries.get(locals.i).quantity;
			setMemory(locals.result, 0);
			CALL(ExecuteBurn, locals.request, locals.result);
			output.results.set(locals.i, locals.result);
			++output.attempted;
			if (locals.result.code != Code::Ok)
			{
				output.failureIndex = locals.i;
				output.failureCode = locals.result.code;
				output.code = output.completed ? Code::Pending : locals.result.code;
				return;
			}

			++output.completed;
		}
	}

	/** @brief Execute SetMintingStatus without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetMintingStatus)
	{
		output.itemId = input.collectionId;
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		if (input.status > MintingStatus::Closed)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (!IsCollectionManager(state.get().collections.get(input.collectionId), qpi.invocator()))
		{
			output.code = Code::Unauthorized;
			return;
		}

		locals.mintingStatus = state.get().mintingStatus.get(input.collectionId);

		if (locals.mintingStatus == input.status)
		{
			output.code = Code::Ok;
			return;
		}

		if (locals.mintingStatus == MintingStatus::Closed)
		{
			output.code = Code::Conflict;
			return;
		}

		state.mut().mintingStatus.set(input.collectionId, input.status);

		output.code = Code::Ok;

		locals.event.kind = EventKind::MintingStatusChanged;
		locals.event.actor = qpi.invocator();
		locals.event.reference = input.collectionId;
		locals.event.action = static_cast<uint32>(input.status);
		CALL(AppendEvent, locals.event, locals.logged);
	}

	/** @brief Execute MintBatch without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteMintBatch)
	{
		if (input.count > QNFT_MINT_BATCH_CAPACITY)
		{
			output.code = output.failureCode = Code::InvalidInput;
			output.failureIndex = input.count;
			return;
		}

		locals.batch.count = input.count;
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.batch.entries.set(locals.i, input.entries.get(locals.i));
		}

		CALL(MintMany, locals.batch, output);
	}

	/** @brief Shared issuance engine for full definitions and compact multi-mint requests. */
	PRIVATE_PROCEDURE_WITH_LOCALS(MintMany)
	{
		output.code = Code::Ok;
		output.failureCode = Code::Ok;
		output.failureIndex = input.count;
		if (input.count > QNFT_TOKEN_BATCH_CAPACITY)
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		if (input.count > 0 && state.get().mintFee > QPI::div(MAX_AMOUNT, static_cast<sint64>(input.count)))
		{
			output.code = Code::InvalidInput;
			output.failureCode = output.code;
			return;
		}

		if (qpi.invocationReward() < smul(state.get().mintFee, static_cast<sint64>(input.count)))
		{
			output.code = Code::InsufficientFee;
			output.failureCode = output.code;
			return;
		}

		if (input.count > QNFT_MAX_ITEMS - state.get().itemCount)
		{
			output.code = Code::Capacity;
			output.failureCode = output.code;
			return;
		}

		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request = input.entries.get(locals.i);
			setMemory(locals.result, 0);
			CALL(ValidateMint, locals.request, locals.result);
			if (locals.result.result.code != Code::Ok)
			{
				output.code = locals.result.result.code;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}

			locals.collection = state.get().collections.get(locals.request.collectionId);
			if (!CheckMintCollectionQuota(input, locals.i, locals.collection, locals.quotaScratch))
			{
				output.code = Code::Capacity;
				output.failureIndex = locals.i;
				output.failureCode = output.code;
				return;
			}
		}

		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			locals.request = input.entries.get(locals.i);
			setMemory(locals.result, 0);
			CALL(ExecuteMint, locals.request, locals.result);
			output.results.set(locals.i, locals.result.result);
			++output.attempted;
			if (locals.result.result.code != Code::Ok && locals.result.result.code != Code::Pending)
			{
				output.failureIndex = locals.i;
				output.failureCode = locals.result.result.code;
				output.code = output.completed ? Code::Pending : locals.result.result.code;
				return;
			}

			++output.completed;
			output.fee = sadd(output.fee, locals.result.fee);
			if (locals.result.result.code == Code::Pending)
			{
				output.code = Code::Pending;
			}
		}
	}

	/** @brief Publish immutable sale allocation terms and the amount owed at this purchase. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitSaleDividend)
	{
		locals.journalInput.kind = EventKind::Market;
		locals.journalInput.action = static_cast<uint32>(MarketAction::SaleDividend);
		locals.journalInput.reference = input.listingId;
		locals.journalInput.itemId = input.itemId;
		locals.journalInput.quantity = static_cast<sint64>(input.dividendAssetName);
		locals.journalInput.version = input.dividendBps;
		locals.journalInput.amount = input.amount;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/** @brief Register a validated new collection and advance its allocation counter together. */
	PRIVATE_PROCEDURE(CommitNewCollection)
	{
		// Capacity was checked before this commit; updates to existing collections do not allocate IDs.
		output = state.get().collectionCount;
		state.mut().collections.set(output, input);
		++state.mut().collectionCount;
	}

	/** @brief Publish an escrowed listing and allocate its identifier in one place. */
	PRIVATE_PROCEDURE(CommitNewListing)
	{
		// Map capacity and ID exhaustion were checked before the asset entered escrow.
		input.listingId = state.get().lastListingId + 1;
		state.mut().listings.set(input.listingId, input);
		state.mut().lastListingId = input.listingId;
		output = input.listingId;
	}

	/** @brief Persist a validated metadata update together with its next revision. */
	PRIVATE_PROCEDURE(CommitMetadataUpdate)
	{
		// Metadata updates preflight revision exhaustion before reaching this commit.
		++input.item.metadataVersion;
		state.mut().items.set(input.itemId, input.item);
	}

	/** @brief Record successfully burned units without restoring lifetime issuance capacity. */
	PRIVATE_PROCEDURE(CommitBurn)
	{
		// The caller has already completed the native burn; failures never reach this persistence step.
		input.item.burned += input.quantity;
		state.mut().items.set(input.itemId, input.item);
	}

	/** @brief Decrease a sold listing's remaining units and persist or remove it together. */
	PRIVATE_PROCEDURE(CommitListingFill)
	{
		// Delivery succeeded and quantity was bounded by the listing balance before the transfer.
		input.listing.quantity -= input.quantity;
		if (input.listing.quantity > 0)
		{
			state.mut().listings.set(input.listingId, input.listing);
		}
		else
		{
			state.mut().listings.removeByKey(input.listingId);
		}
	}

	/** @brief Accumulate validated protocol revenue for distribution at the end of the epoch. */
	PRIVATE_PROCEDURE(AccrueProtocolFee)
	{
		// Zero-fee settlement must not dirty the state and trigger a full digest recomputation.
		if (input > 0)
		{
			state.mut().epochFees += input;
		}
	}

	/** @brief Append once to the ring without relying on the node logger. */
	PRIVATE_PROCEDURE(AppendEvent)
	{
		// Keep an exclusive end cursor representable even at counter exhaustion.
		if (state.get().lastEventId == QNFT_LAST_EVENT_ID)
		{
			return;
		}

		// Map the monotonic event ID to a reusable ring slot; old entries are overwritten only at capacity.
		input.eventId = state.get().lastEventId + 1;
		input.tick = qpi.tick();
		state.mut().journal.set((input.eventId - 1) & QNFT_EVENT_SLOT_MASK, input);
		state.mut().lastEventId = input.eventId;
	}

	/** @brief Write a Event payload using the QNFT information-log prefix. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitCommand)
	{
		// Copy command fields into the public journal before emitting the optional node log.
		input.contractIndex = SELF_INDEX;
		locals.journalInput.kind = EventKind::Command;
		locals.journalInput.actor = input.actor;
		locals.journalInput.to = input.receiver;
		locals.journalInput.quantity = input.quantity;
		locals.journalInput.action = static_cast<uint32>(input.type);
		locals.journalInput.itemId = input.itemId;
		locals.journalInput.version = input.version;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/** @brief Write a MarketEvent payload using the QNFT information-log prefix. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitMarket)
	{
		// Keep the listing reference and QU amount together so a sale can be reconstructed from the journal.
		input.contractIndex = SELF_INDEX;
		locals.journalInput.kind = EventKind::Market;
		locals.journalInput.actor = input.actor;
		locals.journalInput.to = input.receiver;
		locals.journalInput.reference = input.listingId;
		locals.journalInput.quantity = input.quantity;
		locals.journalInput.amount = input.amount;
		locals.journalInput.action = static_cast<uint32>(input.type);
		locals.journalInput.itemId = input.itemId;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/** @brief Write a GovernanceEvent payload using the QNFT information-log prefix. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitGovernance)
	{
		// Use the reference field for the fee variable and version for the native proposal slot.
		input.contractIndex = SELF_INDEX;
		locals.journalInput.kind = EventKind::Governance;
		locals.journalInput.actor = input.actor;
		locals.journalInput.reference = static_cast<uint64>(input.variable);
		locals.journalInput.amount = input.value;
		locals.journalInput.action = static_cast<uint32>(input.action);
		locals.journalInput.version = input.proposalIndex;
		locals.journalInput.code = input.code;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/** @brief Emit a committed movement before any later external call or refund. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitTransfer)
	{
		// Record the actual invocator as operator, including transfers performed under an approval.
		input.contractIndex = SELF_INDEX;
		input.type = TokenEventType::TransferSingle;
		input.operatorId = qpi.invocator();
		locals.journalInput.kind = EventKind::TransferSingle;
		locals.journalInput.actor = input.operatorId;
		locals.journalInput.from = input.from;
		locals.journalInput.to = input.to;
		locals.journalInput.quantity = input.quantity;
		locals.journalInput.itemId = input.itemId;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/**
	 * @brief Transfer validated QNFT units and emit their movement only when the native transfer succeeds.
	 * @note Callers validate the item, authority and quantity before calling this procedure.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(TransferAsset)
	{
		if (input.itemId >= state.get().itemCount || input.quantity < 0 || input.from == NULL_ID || state.get().exporting.get(input.itemId))
		{
			output = -1;
			return;
		}

		if (input.from != SELF)
		{
			locals.rentalBalanceInput.owner = input.from;
			locals.rentalBalanceInput.itemId = input.itemId;
			CALL(TransferableBalance, locals.rentalBalanceInput, locals.rentalBalance);
			if (locals.rentalBalance.code != Code::Ok || input.quantity > locals.rentalBalance.availableQuantity)
			{
				output = -1;
				return;
			}
		}

		// Resolve the asset from the same item ID that will be recorded in the event.
		if (input.quantity > 0)
		{
			output = qpi.transferShareOwnershipAndPossession(state.get().items.get(input.itemId).assetName, SELF, input.from, input.from,
			                                                 input.quantity, input.to);
		}

		if (output < 0)
		{
			return;
		}

		locals.event.from = input.from;
		locals.event.to = input.to;
		locals.event.quantity = input.quantity;
		locals.event.itemId = input.itemId;
		CALL(EmitTransfer, locals.event, locals.eventOutput);
	}

	/** @brief Announce the stored URI without interpreting its off-chain document. */
	PRIVATE_PROCEDURE_WITH_LOCALS(EmitUri)
	{
		// Store the exact URI in the journal so readers do not depend on optional node logs.
		input.contractIndex = SELF_INDEX;
		input.type = TokenEventType::URI;
		locals.journalInput.kind = EventKind::URI;
		locals.journalInput.uri = input.uri;
		locals.journalInput.itemId = input.itemId;
		CALL(AppendEvent, locals.journalInput, locals.journalOutput);
		LOG_INFO(input);
	}

	/**
	 * @brief Emit one diagnostic at the owning entry point; nested helpers only return status.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(LogFailure)
	{
		// Attach the calling account and this contract to the diagnostic returned by the operation.
		locals.event = input;
		locals.event.contractIndex = SELF_INDEX;
		locals.event.actor = qpi.invocator();
		LOG_ERROR(locals.event);
	}

	/**
	 * @brief Attempt every refund immediately and retain only failed transfers.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(Refund)
	{
		if (input.amount <= 0)
		{
			return;
		}

		// Contract recipients receive the same direct attempt as ordinary accounts.
		// QPI rejects nested contract transfers before debiting QU; retry those outside the callback.
		if (qpi.transfer(qpi.invocator(), input.amount) < 0)
		{
			locals.pending = 0;
			state.get().contractRefunds.get(qpi.invocator(), locals.pending);
			state.mut().contractRefunds.set(qpi.invocator(), locals.pending + input.amount);
		}
	}

	/**
	 * @brief Settle command fees and refunds, then log success, pending status or rejection.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FinishCommand)
	{
		// Retain only the amount reported by the operation; rejected commands normally report no fee.
		output = input.result;
		if (input.fee > 0)
		{
			if (input.domain == FailureDomain::Command && input.operation == Operation::FundReserve)
			{
				qpi.burn(input.fee);
			}
			else
			{
				locals.accrualInput = input.fee;
				CALL(AccrueProtocolFee, locals.accrualInput, locals.accrualOutput);
			}
		}

		// Return the unspent reward, or retain it as a credit when the caller is a contract.
		if (qpi.invocationReward() > input.retainedAmount)
		{
			locals.refundInput.amount = qpi.invocationReward() - input.retainedAmount;
			CALL(Refund, locals.refundInput, locals.refundOutput);
		}

		if (input.events == FinishEvents::CallerManaged)
		{
			return;
		}

		// This settlement credits the shop fee; the sale executor accounts for any self-held dividend separately.
		if (input.domain == FailureDomain::Market)
		{
			if (output.code == Code::Ok)
			{
				CALL(EmitMarket, input.marketEvent, locals.marketOutput);
			}
			else
			{
				locals.failureInput.domain = FailureDomain::Market;
				locals.failureInput.marketAction = input.marketEvent.type;
				locals.failureInput.listingId = input.marketEvent.listingId;
				locals.failureInput.itemId = input.marketEvent.itemId;
				locals.failureInput.code = output.code;
				CALL(LogFailure, locals.failureInput, locals.failureOutput);
			}

			return;
		}

		if (output.code == Code::Ok || output.code == Code::Pending)
		{
			locals.event.contractIndex = SELF_INDEX;
			locals.event.type = input.operation;
			locals.event.actor = qpi.invocator();
			locals.event.receiver = input.receiver;
			locals.event.quantity = output.quantity;
			locals.event.itemId = output.itemId;
			// Collection identifiers must not be interpreted as NFT item IDs.
			if (input.operation > Operation::AcceptAdmin && input.operation != Operation::FundReserve && input.operation != Operation::Export &&
			    output.itemId < state.get().itemCount)
			{
				locals.event.version = state.get().items.get(output.itemId).metadataVersion;
				output.assetName = state.get().items.get(output.itemId).assetName;
			}

			CALL(EmitCommand, locals.event, locals.eventOutput);
		}
		else
		{
			locals.failureInput.domain = FailureDomain::Command;
			locals.failureInput.operation = input.operation;
			locals.failureInput.itemId = input.itemId;
			locals.failureInput.collectionId = input.collectionId;
			locals.failureInput.code = output.code;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
		}
	}

	/**
	 * @brief Validate collection parameters and allocate a record; leave fee settlement to the caller.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCreateCollection)
	{
		// Validate limits, metadata, storage capacity and payment before consuming a collection ID.
		if (!isInRange<uint32>(input.maxItems, 0, QNFT_MAX_ITEMS) || !isInRange<sint64>(input.maxUnits, 0, MAX_AMOUNT))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (!CheckMetadata(input.metadata, locals.metadataScratch))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (state.get().collectionCount >= QNFT_MAX_COLLECTIONS)
		{
			output.code = Code::Capacity;
			return;
		}

		if (qpi.invocationReward() < state.get().collectionFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		// The immutable creator can manage the collection without appointing an administrator.
		locals.collection.creator = qpi.invocator();
		locals.collection.metadata = input.metadata;
		locals.collection.maxItems = input.maxItems;
		locals.collection.maxUnits = input.maxUnits;
		locals.collection.externalTransfersDisabled = input.externalTransfersDisabled;
		CALL(CommitNewCollection, locals.collection, locals.committedCollection);
		output.itemId = locals.committedCollection;
		output.code = Code::Ok;
	}

	/**
	 * @brief Nominate the next collection administrator as the creator or current administrator.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteNominateAdmin)
	{
		// Check payment and current authority before replacing the pending nominee.
		if (qpi.invocationReward() < state.get().nominateAdminFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		if (input.receiver == NULL_ID || qpi.isContractId(input.receiver))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (!IsCollectionManager(locals.collection, qpi.invocator()))
		{
			output.code = Code::Unauthorized;
			return;
		}

		locals.collection.nominatedAdministrator = input.receiver;
		state.mut().collections.set(input.collectionId, locals.collection);
		output.itemId = input.collectionId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Accept collection administration as the nominated account.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteAcceptAdmin)
	{
		// Only the recorded nominee can consume the nomination and become administrator.
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (locals.collection.nominatedAdministrator == NULL_ID || qpi.invocator() != locals.collection.nominatedAdministrator)
		{
			output.code = Code::Unauthorized;
			return;
		}

		// Consume the nomination so the former nominee cannot accept it again later.
		locals.collection.administrator = qpi.invocator();
		locals.collection.nominatedAdministrator = NULL_ID;
		state.mut().collections.set(input.collectionId, locals.collection);
		output.itemId = input.collectionId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Append an issued item and advance global and collection issuance counters together.
	 * @note Call once after successful core issuance, with capacity and collection validity already checked.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(CommitIssuedItem)
	{
		// The caller has completed all fallible validation; committing an issued asset must not be skipped.
		output.itemId = state.get().itemCount;
		state.mut().items.set(output.itemId, input);
		++state.mut().itemCount;

		// Count distinct assets and their original edition sizes, including units awaiting delivery.
		locals.collection = state.get().collections.get(input.collectionId);
		++locals.collection.issuedItems;
		locals.collection.issuedUnits += input.supply;
		state.mut().collections.set(input.collectionId, locals.collection);
	}

	/**
	 * @brief Issue a fixed edition and deliver it, preserving recoverable delivery failures.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteMint)
	{
		CALL(ValidateMint, input, output);
		if (output.result.code != Code::Ok)
		{
			return;
		}

		if (qpi.invocationReward() < state.get().mintFee)
		{
			output.result.code = Code::InsufficientFee;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		output.result.itemId = state.get().itemCount;
		locals.assetName = EncodeAssetName(output.result.itemId, locals.nameScratch);
		if (qpi.issueAsset(locals.assetName, SELF, 0, input.quantity, 0) != input.quantity)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		// Persist issuance before attempting delivery: core does not roll back prior calls.
		locals.item.assetName = locals.assetName;
		locals.item.creator = locals.collection.creator;
		locals.item.royaltyReceiver = input.royaltyReceiver;
		locals.item.royaltyBps = input.royaltyBps;
		locals.item.dividendAssetName = input.dividendAssetName;
		locals.item.dividendBps = input.dividendBps;
		locals.item.metadata = input.metadata;
		locals.item.metadataVersion = 1;
		locals.item.policy = input.policy;
		locals.item.frozen = input.policy == Policy::Immutable;
		locals.item.supply = input.quantity;
		locals.item.collectionId = input.collectionId;
		locals.item.reservedRecipient = input.receiver;
		locals.item.reserved = input.quantity;
		CALL(CommitIssuedItem, locals.item, locals.committedItem);

		output.result.itemId = locals.committedItem.itemId;
		locals.transferEvent.from = NULL_ID;
		locals.transferEvent.to = SELF;
		locals.transferEvent.quantity = input.quantity;
		locals.transferEvent.itemId = output.result.itemId;
		CALL(EmitTransfer, locals.transferEvent, locals.transferLogOutput);

		locals.uriEvent.uri = locals.item.metadata.uri;
		locals.uriEvent.itemId = output.result.itemId;
		CALL(EmitUri, locals.uriEvent, locals.uriLogOutput);

		output.fee = state.get().mintFee;
		locals.transferInput.from = SELF;
		locals.transferInput.to = input.receiver;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = output.result.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		output.result.assetName = locals.item.assetName;
		output.result.quantity = input.quantity;
		if (locals.transferResult < 0)
		{
			// One pending delivery per issued item fits the queue bounded by QNFT_MAX_ITEMS.
			state.mut().pendingMintDeliveries.addTail(output.result.itemId);
			output.result.code = Code::Pending;
			return;
		}

		locals.item.reserved = 0;
		locals.item.reservedRecipient = NULL_ID;
		state.mut().items.set(output.result.itemId, locals.item);
		output.result.code = Code::Ok;
	}

	/**
	 * @brief Transfer owned units or use an owner-wide operator approval.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteTransfer)
	{
		if (qpi.invocationReward() < state.get().transferFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		CALL(ValidateTransfer, input, output);
		if (output.code != Code::Ok)
		{
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		locals.transferInput.from = input.owner;
		locals.transferInput.to = input.receiver;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = input.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		output.code = Code::Ok;
		output.itemId = input.itemId;
		output.quantity = input.quantity;
		output.assetName = locals.item.assetName;
	}

	/**
	 * @brief Update mutable metadata as a collection manager.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteUpdateMetadata)
	{
		// Validate the replacement document and editing authority before changing the stored metadata version.
		if (qpi.invocationReward() < state.get().updateMetadataFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		if (input.metadata.contentHash == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (!CheckMetadata(input.metadata, locals.metadataScratch))
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.accessInput.itemId = input.itemId;
		CALL(ValidateMetadataAccess, locals.accessInput, locals.access);
		if (locals.access.code != Code::Ok)
		{
			output.code = locals.access.code;
			return;
		}

		if (locals.access.item.policy != Policy::Mutable)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (locals.access.item.metadataVersion == QNFT_MAX_METADATA_VERSION)
		{
			output.code = Code::Capacity;
			return;
		}

		// Commit the new version before announcing its URI to readers.
		locals.access.item.metadata = input.metadata;
		locals.commitInput.item = locals.access.item;
		locals.commitInput.itemId = input.itemId;
		CALL(CommitMetadataUpdate, locals.commitInput, locals.commitOutput);
		locals.uriEvent.uri = locals.access.item.metadata.uri;
		locals.uriEvent.itemId = input.itemId;
		CALL(EmitUri, locals.uriEvent, locals.uriLogOutput);

		output.itemId = input.itemId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Permanently freeze editable native-item metadata.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteFreezeMetadata)
	{
		locals.accessInput.itemId = input.itemId;
		CALL(ValidateMetadataAccess, locals.accessInput, locals.access);
		if (locals.access.code != Code::Ok)
		{
			output.code = locals.access.code;
			return;
		}

		// Freezing is permanent and does not alter the current URI or its version.
		locals.access.item.frozen = true;
		state.mut().items.set(input.itemId, locals.access.item);
		output.itemId = input.itemId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Release asset management to an accepting registered destination.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteExport)
	{
		// Budget the protocol fee and the offered destination fee separately before releasing management.

		if (input.owner == SELF || input.owner == NULL_ID)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (!input.destination || input.destination == SELF_INDEX)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.offeredFee < 0)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (qpi.invocationReward() < state.get().exportFee || input.offeredFee > qpi.invocationReward() - state.get().exportFee)
		{
			output.result.code = Code::InsufficientFee;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.result.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		if (state.get().collections.get(locals.item.collectionId).externalTransfersDisabled)
		{
			output.result.code = Code::Conflict;
			return;
		}

		if (!locals.item.supply)
		{
			output.result.code = Code::NotFound;
			return;
		}

		// Generic native-asset export. Core validates the destination lifecycle and callbacks.
		// Registered managers are trusted through Mainnet code review: callbacks run with SELF as invocator.

		locals.authorizationInput.owner = input.owner;
		locals.authorizationInput.itemId = input.itemId;
		CALL(isOwnerOrApprovedOperator, locals.authorizationInput, locals.authorizationOutput);
		if (!locals.authorizationOutput.authorized)
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		if (qpi.numberOfPossessedShares(locals.item.assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX) < input.quantity)
		{
			output.result.code = Code::InsufficientBalance;
			return;
		}

		locals.rentalBalanceInput.owner = input.owner;
		locals.rentalBalanceInput.itemId = input.itemId;
		CALL(RentalBalance, locals.rentalBalanceInput, locals.rentalBalance);
		if (locals.rentalBalance.code != Code::Ok || input.quantity > locals.rentalBalance.availableQuantity)
		{
			output.result.code = Code::Conflict;
			return;
		}

		locals.asset = Asset{.issuer = SELF, .assetName = locals.item.assetName};

		state.mut().exporting.set(input.itemId, true);
		qpi.getEntity(SELF, locals.beforeExport);
		locals.exportSucceeded =
		    qpi.releaseShares(locals.asset, input.owner, input.owner, input.quantity, input.destination, input.destination, input.offeredFee) >= 0;
		qpi.getEntity(SELF, locals.afterExport);
		state.mut().exporting.set(input.itemId, false);
		// Measure the net QU spent during releaseShares to calculate the caller's refund.
		locals.spentAmount = (locals.beforeExport.incomingAmount - locals.beforeExport.outgoingAmount) -
		                     (locals.afterExport.incomingAmount - locals.afterExport.outgoingAmount);
		if (locals.spentAmount < 0)
		{
			locals.spentAmount = 0;
		}

		if (locals.exportSucceeded)
		{
			output.fee = state.get().exportFee;
			locals.spentAmount += state.get().exportFee;
		}

		output.retainedAmount = locals.spentAmount;

		output.result.code = locals.exportSucceeded ? Code::Ok : Code::ExternalFailure;
		output.result.itemId = input.itemId;
		output.result.quantity = input.quantity;
		output.result.assetName = locals.item.assetName;
	}

	/**
	 * @brief Burn native units without restoring collection issuance quota.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBurn)
	{
		CALL(ValidateBurn, input, output);
		if (output.code != Code::Ok)
		{
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		// Sending units to NULL_ID performs the core burn; permissions and collection quotas remain unchanged.
		locals.transferInput.from = input.owner;
		locals.transferInput.to = NULL_ID;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = input.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		locals.commitInput.item = locals.item;
		locals.commitInput.quantity = input.quantity;
		locals.commitInput.itemId = input.itemId;
		CALL(CommitBurn, locals.commitInput, locals.commitOutput);
		output.code = Code::Ok;
		output.itemId = input.itemId;
		output.quantity = input.quantity;
		output.assetName = locals.item.assetName;
	}

	/** @brief Retry each selected pending delivery once; failures rotate behind other recipients without additional fees. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessMintDeliveries)
	{
		locals.remaining = minimum<uint64>(state.get().pendingMintDeliveries.population(), QNFT_MINT_DELIVERIES_PER_TICK);
		while (locals.remaining)
		{
			locals.index = state.get().pendingMintDeliveries.headIndex();
			locals.itemId = state.get().pendingMintDeliveries.element(locals.index);
			state.mut().pendingMintDeliveries.remove(locals.index);
			--locals.remaining;
			locals.item = state.get().items.get(locals.itemId);
			if (!locals.item.reserved)
			{
				continue;
			}

			locals.transferInput.from = SELF;
			locals.transferInput.to = locals.item.reservedRecipient;
			locals.transferInput.quantity = locals.item.reserved;
			locals.transferInput.itemId = locals.itemId;
			CALL(TransferAsset, locals.transferInput, locals.transferResult);
			if (locals.transferResult < 0)
			{
				// Removing the current entry guarantees room for its retry.
				state.mut().pendingMintDeliveries.addTail(locals.itemId);
				continue;
			}

			// Clear the reservation only after the native ledger confirms delivery.
			locals.item.reserved = 0;
			locals.item.reservedRecipient = NULL_ID;
			state.mut().items.set(locals.itemId, locals.item);
		}
	}

	/**
	 * @brief Validate a reserve contribution and return the amount to burn during command settlement.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteFundReserve)
	{
		// Treat the whole positive reward as a reserve contribution rather than distributable revenue.
		if (qpi.invocationReward() <= 0)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		output.fee = qpi.invocationReward();
		output.result.quantity = output.fee;
		output.result.code = Code::Ok;
	}

	/** @brief Execute ChangeDeveloper without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteChangeDeveloper)
	{
		if (state.get().developer == NULL_ID || qpi.invocator() != state.get().developer)
		{
			output.code = Code::Unauthorized;
			return;
		}

		if (input.developer == NULL_ID || input.developer == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		state.mut().developer = input.developer;
		output.code = Code::Ok;
	}

	/**
	 * @brief Escrow locally managed units at a fixed QU price while preserving operator approval.
	 * @note Only the seller can cancel; price and metadata version are rechecked when buying.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteListForSale)
	{
		// Reject unpaid calls before validating input or reading registry entries.
		if (qpi.invocationReward() < state.get().listingFee)
		{
			output.result.code = Code::InsufficientFee;
			return;
		}

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || !isInRange<sint64>(input.unitPrice, 1, MAX_AMOUNT) || qpi.invocator() == NULL_ID ||
		    qpi.isContractId(qpi.invocator()))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.result.code = Code::NotFound;
			return;
		}

		// Only issued, deliverable items with a valid combined fee may enter shop escrow.
		locals.item = state.get().items.get(input.itemId);
		if (locals.item.supply <= 0 || !validSaleRates(locals.item, state.get().shopFeeBPS))
		{
			output.result.code = Code::Conflict;
			return;
		}

		if (state.get().listings.population() >= QNFT_MAX_LISTINGS || state.get().lastListingId == 0xffffffffffffffffULL)
		{
			output.result.code = Code::Capacity;
			return;
		}

		if (qpi.numberOfPossessedShares(locals.item.assetName, SELF, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.quantity)
		{
			output.result.code = Code::InsufficientBalance;
			return;
		}

		locals.rentalBalanceInput.owner = qpi.invocator();
		locals.rentalBalanceInput.itemId = input.itemId;
		CALL(RentalBalance, locals.rentalBalanceInput, locals.rentalBalance);
		if (locals.rentalBalance.code != Code::Ok || input.quantity > locals.rentalBalance.availableQuantity)
		{
			output.result.code = Code::Conflict;
			return;
		}

		// Persist a listing only after the native ledger confirms the escrow transfer.
		locals.transferInput.from = qpi.invocator();
		locals.transferInput.to = SELF;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = input.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		// Publish the listing only after escrow succeeds, and bind it to the current metadata version.
		locals.listing.seller = qpi.invocator();
		locals.listing.unitPrice = input.unitPrice;
		locals.listing.quantity = input.quantity;
		locals.listing.itemId = input.itemId;
		locals.listing.metadataVersion = locals.item.metadataVersion;
		CALL(CommitNewListing, locals.listing, locals.committedListing);
		output.result.listingId = locals.committedListing;
		output.result.code = Code::Ok;
		output.event = MarketEvent{.contractIndex = SELF_INDEX,
		                           .type = MarketAction::List,
		                           .actor = qpi.invocator(),
		                           .receiver = SELF,
		                           .listingId = output.result.listingId,
		                           .quantity = input.quantity,
		                           .amount = input.unitPrice,
		                           .itemId = input.itemId,
		                           ._terminator = 0};
	}

	/**
	 * @brief Return the unsold escrow quantity to the listing's seller.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCancelSale)
	{
		// Reject missing listings and unauthorized callers before touching escrow.
		if (!state.get().listings.get(input.listingId, locals.listing))
		{
			output.result.code = Code::NotFound;
			return;
		}

		if (locals.listing.seller != qpi.invocator())
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		// Keep the listing until all unsold units have returned to its seller.
		locals.transferInput.from = SELF;
		locals.transferInput.to = locals.listing.seller;
		locals.transferInput.quantity = locals.listing.quantity;
		locals.transferInput.itemId = locals.listing.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		state.mut().listings.removeByKey(input.listingId);
		output.result.code = Code::Ok;
		output.event = MarketEvent{.contractIndex = SELF_INDEX,
		                           .type = MarketAction::Cancel,
		                           .actor = qpi.invocator(),
		                           .receiver = locals.listing.seller,
		                           .listingId = input.listingId,
		                           .quantity = locals.listing.quantity,
		                           .amount = 0,
		                           .itemId = locals.listing.itemId,
		                           ._terminator = 0};
	}

	/**
	 * @brief Buy a protected-price quantity and settle seller proceeds, royalties and configured sale dividends.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBuy)
	{
		CALL(ValidateBuy, input, locals.validation);
		output.result = locals.validation.result;
		if (output.result.code != Code::Ok)
		{
			return;
		}

		locals.item = locals.validation.item;
		locals.listing = locals.validation.listing;

		locals.paymentInput.item = locals.item;
		locals.paymentInput.seller = locals.listing.seller;
		locals.paymentInput.paid = output.result.paid;
		locals.paymentInput.royalty = output.result.royalty;
		CALL(PrepareSalePayments, locals.paymentInput, locals.paymentOutput);
		if (locals.paymentOutput.code != Code::Ok)
		{
			setMemory(output.result, 0);
			output.result.code = locals.paymentOutput.code;
			return;
		}

		locals.sellerAmount = output.result.paid - output.result.fee - output.result.royalty - locals.paymentOutput.shareholderAmount;

		locals.transferInput.from = SELF;
		locals.transferInput.to = input.receiver;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = locals.listing.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.result.code = Code::ExternalFailure;
			output.result.paid = 0;
			output.result.fee = 0;
			output.result.royalty = 0;
			return;
		}

		// Commit the completed sale before paying a royalty contract that may receive a callback.
		locals.commitInput.listing = locals.listing;
		locals.commitInput.listingId = input.listingId;
		locals.commitInput.quantity = input.quantity;
		CALL(CommitListingFill, locals.commitInput, locals.commitOutput);

		output.result.shareholderAmount = locals.paymentOutput.shareholderAmount;
		state.mut().epochFees = sadd(state.get().epochFees, locals.paymentOutput.selfAmount);
		// Reserve every obligation before the first callback, including zero-balance slots during payment.
		for (locals.paymentIndex = 0; locals.paymentIndex < locals.paymentOutput.count; ++locals.paymentIndex)
		{
			locals.payment = locals.paymentOutput.payments.get(locals.paymentIndex);
			locals.pendingRoyalty = 0;
			state.get().contractRefunds.get(locals.payment.recipient, locals.pendingRoyalty);
			state.mut().contractRefunds.set(locals.payment.recipient, sadd(locals.pendingRoyalty, locals.payment.amount));
		}

		if (locals.item.royaltyReceiver == locals.listing.seller)
		{
			locals.sellerAmount = sadd(locals.sellerAmount, output.result.royalty);
		}

		if (locals.sellerAmount > 0)
		{
			qpi.transfer(locals.listing.seller, locals.sellerAmount);
		}

		for (locals.paymentIndex = 0; locals.paymentIndex < locals.paymentOutput.count; ++locals.paymentIndex)
		{
			locals.payment = locals.paymentOutput.payments.get(locals.paymentIndex);
			locals.pendingRoyalty = 0;
			state.get().contractRefunds.get(locals.payment.recipient, locals.pendingRoyalty);
			state.mut().contractRefunds.set(locals.payment.recipient, locals.pendingRoyalty - locals.payment.amount);
			if (qpi.transfer(locals.payment.recipient, locals.payment.amount) < 0)
			{
				locals.pendingRoyalty = 0;
				state.get().contractRefunds.get(locals.payment.recipient, locals.pendingRoyalty);
				state.mut().contractRefunds.set(locals.payment.recipient, sadd(locals.pendingRoyalty, locals.payment.amount));
			}

			locals.pendingRoyalty = 0;
			state.get().contractRefunds.get(locals.payment.recipient, locals.pendingRoyalty);
			if (locals.pendingRoyalty == 0)
			{
				state.mut().contractRefunds.removeByKey(locals.payment.recipient);
			}
		}

		if (locals.item.dividendBps)
		{
			locals.dividendEvent = SaleDividendEvent{.contractIndex = SELF_INDEX,
			                                         .type = MarketAction::SaleDividend,
			                                         .listingId = input.listingId,
			                                         .dividendAssetName = locals.item.dividendAssetName,
			                                         .amount = output.result.shareholderAmount,
			                                         .itemId = locals.listing.itemId,
			                                         .dividendBps = locals.item.dividendBps,
			                                         ._terminator = 0};
			CALL(EmitSaleDividend, locals.dividendEvent, locals.dividendEventOutput);
		}

		output.result.code = Code::Ok;
		output.event = MarketEvent{.contractIndex = SELF_INDEX,
		                           .type = MarketAction::Buy,
		                           .actor = qpi.invocator(),
		                           .receiver = input.receiver,
		                           .listingId = input.listingId,
		                           .quantity = input.quantity,
		                           .amount = output.result.paid,
		                           .itemId = locals.listing.itemId,
		                           ._terminator = 0};
	}

	/**
	 * @brief Create or replace a hash-bound batch for one native yes/no vote and one proposal fee.
	 * @note All entries must validate before the native QPI proposal is stored; one proposal fee covers the whole batch.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ApplySetFeeProposalBatch)
	{
		// Validate the payload before asking QPI to authorize the proposer and allocate a voting slot.
		output.result = INVALID_PROPOSAL_INDEX;
		output.event.action = GovernanceAction::SetProposal;
		output.event.actor = qpi.invocator();
		// Hashing includes tail padding, which must not depend on caller bytes.
		setMemory(locals.batch, 0);
		locals.batch.changes = input.changes;
		locals.batch.count = input.count;
		CALL(ValidateFeeBatch, locals.batch, locals.validation);
		if (!input.epoch || !locals.validation.valid || state.get().lastGovernanceEpoch == qpi.epoch() ||
		    qpi.invocationReward() < state.get().proposalFee)
		{
			output.event.code = Code::InvalidInput;
			return;
		}

		locals.proposal.url = input.url;
		locals.proposal.epoch = input.epoch;
		locals.proposal.type = ProposalTypes::MultiVariablesYesNo;
		// Bind the validated batch to the native proposal before storing its sidecar.
		locals.proposal.data.transfer.destination = qpi.K12(locals.batch);
		output.result = qpi(state.mut().proposals).setProposal(qpi.invocator(), locals.proposal);
		if (output.result == INVALID_PROPOSAL_INDEX)
		{
			output.event.code = Code::Conflict;
			return;
		}

		state.mut().feeBatches.set(output.result, locals.batch);
		output.event.code = Code::Ok;
	}

	/** @brief Execute SetFeeProposalBatch without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetFeeProposalBatch)
	{
		CALL(ApplySetFeeProposalBatch, input, locals.operationOutput);
		output = locals.operationOutput;
		locals.event = locals.operationOutput.event;
		locals.event.proposalIndex = output.result;
		if (output.result != INVALID_PROPOSAL_INDEX)
		{
			output.fee = state.get().proposalFee;
			for (locals.i = 0; locals.i < input.count; ++locals.i)
			{
				locals.event.variable = input.changes.get(locals.i).variable;
				locals.event.value = input.changes.get(locals.i).value;
				CALL(EmitGovernance, locals.event, locals.eventOutput);
			}
		}
		else
		{
			LOG_ERROR(locals.event);
		}
	}

	/**
	 * @brief Validate fee bounds, then let QPI authorize, snapshot and create/replace/cancel the proposal.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ApplySetShareholderProposal)
	{
		// Epoch zero requests cancellation; other proposals require payment and a supported fee variable.
		output.result = INVALID_PROPOSAL_INDEX;
		output.event.action = GovernanceAction::SetProposal;
		output.event.actor = qpi.invocator();
		output.event.variable = static_cast<FeeVariable>(input.data.variableOptions.variable);
		output.event.value = input.data.variableOptions.value;
		if (input.epoch)
		{
			locals.validationBatch.count = 1;
			locals.validationBatch.changes.set(0, FeeChange{.variable = output.event.variable, .value = output.event.value});
			CALL(ValidateFeeBatch, locals.validationBatch, locals.validation);
		}
		if (state.get().lastGovernanceEpoch == qpi.epoch() ||
		    (input.epoch &&
		     (qpi.invocationReward() < state.get().proposalFee || input.type != ProposalTypes::VariableYesNo || !locals.validation.valid)))
		{
			output.event.code = Code::InvalidInput;
			return;
		}

		output.result = qpi(state.mut().proposals).setProposal(qpi.invocator(), input);
		if (output.result == INVALID_PROPOSAL_INDEX)
		{
			output.event.code = Code::Conflict;
			return;
		}

		// Replacing or cancelling invalidates the previous batch in this slot.
		state.mut().feeBatches.set(output.result, locals.emptyBatch);
		output.event.code = Code::Ok;
	}

	/** @brief Execute SetShareholderProposal without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetShareholderProposal)
	{
		CALL(ApplySetShareholderProposal, input, locals.operationOutput);
		output = locals.operationOutput;
		locals.event = locals.operationOutput.event;
		locals.event.proposalIndex = output.result;
		if (output.result != INVALID_PROPOSAL_INDEX)
		{
			if (input.epoch && state.get().proposalFee > 0)
			{
				output.fee = state.get().proposalFee;
			}

			CALL(EmitGovernance, locals.event, locals.eventOutput);
		}
		else
		{
			LOG_ERROR(locals.event);
		}
	}

	/**
	 * @brief Submit a version-checked ballot to QPI using proposal-snapshot share weights.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ApplySetShareholderVotes)
	{
		output.result = false;
		output.event.action = GovernanceAction::Vote;
		output.event.actor = qpi.invocator();
		output.event.proposalIndex = input.proposalIndex;
		if (state.get().lastGovernanceEpoch == qpi.epoch())
		{
			output.event.code = Code::Conflict;
			return;
		}

		// Reject oversized ballots before QPI checks the proposal version and shareholder weight.
		for (locals.i = 0; locals.i < input.voteCounts.capacity(); ++locals.i)
		{
			if (input.voteCounts.get(locals.i) > NUMBER_OF_COMPUTORS)
			{
				output.event.code = Code::Conflict;
				return;
			}

			locals.voteCount += input.voteCounts.get(locals.i);
		}

		if (locals.voteCount > NUMBER_OF_COMPUTORS)
		{
			output.event.code = Code::Conflict;
			return;
		}

		output.result = qpi(state.mut().proposals).vote(qpi.invocator(), input);
		output.event.code = output.result ? Code::Ok : Code::Conflict;
	}

	/** @brief Execute SetShareholderVotes without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetShareholderVotes)
	{
		CALL(ApplySetShareholderVotes, input, locals.operationOutput);
		output = locals.operationOutput;
		locals.event = locals.operationOutput.event;
		if (output.result)
		{
			CALL(EmitGovernance, locals.event, locals.eventOutput);
		}
		else
		{
			LOG_ERROR(locals.event);
		}
	}

	/**
	 * @brief Apply a validated fee entry without callbacks or fallible operations.
	 */
	PRIVATE_PROCEDURE(ApplyFeeChange)
	{
		// Changing either adjustable revenue share recomputes its complement while preserving the developer share.
		switch (input.variable)
		{
			case FeeVariable::CollectionFee: state.mut().collectionFee = input.value; break;
			case FeeVariable::MintFee: state.mut().mintFee = input.value; break;
			case FeeVariable::NominateAdminFee: state.mut().nominateAdminFee = input.value; break;
			case FeeVariable::TransferFee: state.mut().transferFee = input.value; break;
			case FeeVariable::ApproveFee: state.mut().approveFee = input.value; break;
			case FeeVariable::UpdateMetadataFee: state.mut().updateMetadataFee = input.value; break;
			case FeeVariable::ExportFee: state.mut().exportFee = input.value; break;
			case FeeVariable::ListingFee: state.mut().listingFee = input.value; break;
			case FeeVariable::ProposalFee: state.mut().proposalFee = input.value; break;
			case FeeVariable::ShopFeeBPS: state.mut().shopFeeBPS = static_cast<uint32>(input.value); break;
			case FeeVariable::RentalProtocolBPS: state.mut().rentalProtocolBPS = static_cast<uint32>(input.value); break;
			case FeeVariable::RentalCreatorBPS: state.mut().rentalCreatorBPS = static_cast<uint32>(input.value); break;
			case FeeVariable::ShareholderFeeBPS:
				state.mut().shareholderFeeBPS = static_cast<uint32>(input.value);
				state.mut().burnFeeBPS = QNFT_BPS_DENOMINATOR - state.get().developerFeeBPS - state.get().shareholderFeeBPS;
				break;
			case FeeVariable::BurnFeeBPS:
				state.mut().burnFeeBPS = static_cast<uint32>(input.value);
				state.mut().shareholderFeeBPS = QNFT_BPS_DENOMINATOR - state.get().developerFeeBPS - state.get().burnFeeBPS;
				break;
		}
	}

	/**
	 * @brief Apply native QPI quorum-approved values once after this epoch's revenue payment.
	 * @note Runs once per epoch after fee distribution; each accepted batch is validated before any entry is applied.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeShareholderStateVarProposals)
	{
		// The epoch marker prevents applying accepted proposals twice in the same epoch.
		if (state.get().lastGovernanceEpoch == qpi.epoch())
		{
			return;
		}

		state.mut().lastGovernanceEpoch = qpi.epoch();
		locals.proposalIndex = -1;
		while ((locals.proposalIndex = qpi(state.get().proposals).nextProposalIndex(locals.proposalIndex, qpi.epoch())) >= 0)
		{
			if (!qpi(state.get().proposals).getProposal(locals.proposalIndex, locals.proposal) ||
			    !qpi(state.get().proposals).getVotingSummary(locals.proposalIndex, locals.results) || locals.results.getAcceptedOption() != 1)
			{
				continue;
			}

			locals.event.action = GovernanceAction::Apply;
			locals.event.actor = qpi(state.get().proposals).proposerId(locals.proposalIndex);
			locals.event.proposalIndex = locals.proposalIndex;
			setMemory(locals.batch, 0);
			switch (locals.proposal.type)
			{
				case ProposalTypes::VariableYesNo:
					locals.batch.count = 1;
					locals.batch.changes.set(0, FeeChange{.variable = static_cast<FeeVariable>(locals.proposal.data.variableOptions.variable),
					                                      .value = locals.proposal.data.variableOptions.value});
					break;
				case ProposalTypes::MultiVariablesYesNo:
					locals.batch = state.get().feeBatches.get(locals.proposalIndex);
					if (qpi.K12(locals.batch) != locals.proposal.data.transfer.destination)
					{
						locals.event.code = Code::Conflict;
						LOG_ERROR(locals.event);
						continue;
					}

					break;

				default: continue;
			}

			// Recheck accepted values against the current state before changing any tariff.
			CALL(ValidateFeeBatch, locals.batch, locals.validation);
			if (!locals.validation.valid)
			{
				locals.event.code = Code::InvalidInput;
				LOG_ERROR(locals.event);
				continue;
			}

			// Preflight is complete. Apply the whole set without callbacks or further failure points.
			for (locals.i = 0; locals.i < locals.batch.count; ++locals.i)
			{
				locals.applyInput = locals.batch.changes.get(locals.i);
				CALL(ApplyFeeChange, locals.applyInput, locals.applyOutput);
				locals.event.variable = locals.batch.changes.get(locals.i).variable;
				locals.event.value = locals.batch.changes.get(locals.i).value;
				locals.event.code = Code::Ok;
				CALL(EmitGovernance, locals.event, locals.eventOutput);
			}
		}
	}

	/**
	 * @brief Pay epoch revenue through native transfers and dividends; retain only unpaid amounts and rounding dust.
	 * @note Checks revenue rates, IPO share supply and backing before burning or paying.
	 * @note Commits accounting before callbacks; failed payouts retain their unpaid pool for a later epoch.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(DistributeFees)
	{
		if (state.get().lastDistributionEpoch == qpi.epoch() ||
		    (state.get().epochFees == 0 && state.get().developerPending == 0 && state.get().shareholderCarry < NUMBER_OF_COMPUTORS))
		{
			return;
		}

		// Bound each operand before summation, so even corrupted uint32 rates cannot wrap.
		if (state.get().developerFeeBPS > QNFT_BPS_DENOMINATOR || state.get().shareholderFeeBPS > QNFT_BPS_DENOMINATOR ||
		    state.get().burnFeeBPS > QNFT_BPS_DENOMINATOR ||
		    state.get().developerFeeBPS + state.get().shareholderFeeBPS + state.get().burnFeeBPS != QNFT_BPS_DENOMINATOR)
		{
			locals.failureInput.domain = FailureDomain::Market;
			locals.failureInput.marketAction = MarketAction::Distribution;
			locals.failureInput.code = Code::InvalidInput;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
			return;
		}

		locals.shares = Asset{.issuer = NULL_ID, .assetName = QNFT_SHARE_NAME};
		locals.shareholders.begin(locals.shares);
		while (!locals.shareholders.reachedEnd())
		{
			locals.totalShares += locals.shareholders.numberOfPossessedShares();
			locals.shareholders.next();
		}

		// Missing/incomplete IPO shares cannot consume or misallocate accumulated fees.
		if (locals.totalShares != NUMBER_OF_COMPUTORS)
		{
			locals.failureInput.domain = FailureDomain::Market;
			locals.failureInput.marketAction = MarketAction::Distribution;
			locals.failureInput.code = Code::Conflict;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
			return;
		}

		// Do not spend seller proceeds or refund collateral when revenue backing is incomplete.
		qpi.getEntity(SELF, locals.contractBalance);
		if (locals.contractBalance.incomingAmount - locals.contractBalance.outgoingAmount <
		    state.get().epochFees + state.get().developerPending + state.get().shareholderCarry)
		{
			locals.failureInput.domain = FailureDomain::Market;
			locals.failureInput.marketAction = MarketAction::Distribution;
			locals.failureInput.code = Code::InsufficientBalance;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
			return;
		}

		locals.developerAmount = calculateFeeShare(state.get().epochFees, state.get().developerFeeBPS);
		locals.shareholderAmount = calculateFeeShare(state.get().epochFees, state.get().shareholderFeeBPS);
		// Validated rates sum to 100%; burn receives its configured share plus split rounding dust.
		locals.burnAmount = state.get().epochFees - locals.developerAmount - locals.shareholderAmount;
		locals.shareholderAmount += state.get().shareholderCarry;
		locals.perShare = QPI::div(locals.shareholderAmount, static_cast<sint64>(NUMBER_OF_COMPUTORS));
		if (locals.burnAmount > 0 && qpi.burn(locals.burnAmount) < 0)
		{
			locals.failureInput.domain = FailureDomain::Market;
			locals.failureInput.marketAction = MarketAction::Distribution;
			locals.failureInput.code = Code::ExternalFailure;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
			return;
		}

		state.mut().developerPending += locals.developerAmount;
		state.mut().shareholderCarry = locals.shareholderAmount - locals.perShare * NUMBER_OF_COMPUTORS;
		state.mut().epochFees = 0;
		state.mut().lastDistributionEpoch = qpi.epoch();
		// Commit epoch accounting before callbacks. Moderated Mainnet contracts may receive QU.
		if (locals.perShare > 0 && !qpi.distributeDividends(locals.perShare))
		{
			// A false QPI result means no dividend was transferred; retry the retained pool next epoch.
			state.mut().shareholderCarry = locals.shareholderAmount;
			locals.perShare = 0;
			locals.failureInput.domain = FailureDomain::Market;
			locals.failureInput.marketAction = MarketAction::Distribution;
			locals.failureInput.code = Code::ExternalFailure;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
		}

		if (state.get().developer != NULL_ID && state.get().developerPending > 0)
		{
			locals.developerAmount = state.get().developerPending;
			state.mut().developerPending = 0;
			if (qpi.transfer(state.get().developer, locals.developerAmount) < 0)
			{
				state.mut().developerPending = locals.developerAmount;
				locals.failureInput.domain = FailureDomain::Market;
				locals.failureInput.marketAction = MarketAction::Distribution;
				locals.failureInput.code = Code::ExternalFailure;
				CALL(LogFailure, locals.failureInput, locals.failureOutput);
			}
		}

		locals.event = MarketEvent{.contractIndex = SELF_INDEX,
		                           .type = MarketAction::Distribution,
		                           .actor = SELF,
		                           .receiver = state.get().developer,
		                           .listingId = 0,
		                           .quantity = locals.perShare,
		                           .amount = locals.burnAmount,
		                           .itemId = 0,
		                           ._terminator = 0};
		CALL(EmitMarket, locals.event, locals.eventOutput);
	}

	/** @brief Retry a bounded set of failed refunds, preserving callback-created credits. */
	PRIVATE_PROCEDURE_WITH_LOCALS(RetryRefunds)
	{
		locals.remaining = minimum(state.get().contractRefunds.population(), static_cast<uint64>(QNFT_REFUNDS_PER_TICK));
		while (locals.remaining > 0)
		{
			--locals.remaining;
			locals.index = state.get().contractRefunds.nextElementIndex(static_cast<sint64>(state.get().refundCursor) - 1);
			if (locals.index == NULL_INDEX)
			{
				locals.index = state.get().contractRefunds.nextElementIndex(NULL_INDEX);
			}

			if (locals.index == NULL_INDEX)
			{
				return;
			}

			// Advance before callbacks so a failing recipient cannot block later addresses.
			state.mut().refundCursor = static_cast<uint32>((locals.index + 1) & (QNFT_CONTRACT_REFUND_SLOTS - 1));
			locals.recipient = state.get().contractRefunds.key(locals.index);
			locals.amount = state.get().contractRefunds.value(locals.index);
			// Keep the slot reserved, but clear the paid amount before executing recipient code.
			state.mut().contractRefunds.set(locals.recipient, 0);
			if (qpi.transfer(locals.recipient, locals.amount) < 0)
			{
				locals.pending = 0;
				state.get().contractRefunds.get(locals.recipient, locals.pending);
				state.mut().contractRefunds.set(locals.recipient, locals.pending + locals.amount);
			}
			else
			{
				locals.pending = 0;
				state.get().contractRefunds.get(locals.recipient, locals.pending);
				if (locals.pending == 0)
				{
					state.mut().contractRefunds.removeByKey(locals.recipient);
				}
			}
		}
	}

	/** @brief Keep queued records alive until their queue entry is consumed; IDs are never recycled. */
	PRIVATE_PROCEDURE(StoreRental)
	{
		if (input.status == RentalStatus::Ended && !input.indexed && !input.deposit && !input.ownerCredit && !input.creatorCredit &&
		    !input.payoutQueued)
		{
			state.mut().rentals.removeByKey(input.rentalId);
		}
		else
		{
			state.mut().rentals.set(input.rentalId, input);
		}
	}

	/** @brief Materialize only elapsed periods; callers must settle before changing deposit or renewal consent. */
	PRIVATE_PROCEDURE_WITH_LOCALS(SettleRental)
	{
		if (!state.get().rentals.get(input.rentalId, locals.before))
		{
			output.code = Code::NotFound;
			return;
		}

		CALL(ProjectRental, locals.before, locals.projected);
		output.rental = locals.projected.rental;
		output.code = locals.projected.code;
		if (output.code != Code::Ok)
		{
			return;
		}

		if (locals.projected.charged > 0)
		{
			state.mut().rentalDeposits -= locals.projected.charged;
			state.mut().rentalCredits += locals.projected.charged - locals.projected.protocolPaid;
			state.mut().epochFees += locals.projected.protocolPaid;
			locals.event.kind = EventKind::RentalRenewed;
			locals.event.actor = output.rental.renter;
			locals.event.from = output.rental.owner;
			locals.event.reference = input.rentalId;
			locals.event.itemId = output.rental.itemId;
			locals.event.quantity = output.rental.quantity;
			locals.event.amount = locals.projected.charged;
			CALL(AppendEvent, locals.event, locals.unused);
		}

		if (output.rental.status == RentalStatus::Ended && output.rental.indexed)
		{
			locals.keyInput.owner = output.rental.owner;
			locals.keyInput.itemId = output.rental.itemId;
			locals.key = qpi.K12(locals.keyInput);
			locals.index = state.get().lentRentals.headIndex(locals.key);
			while (locals.index != NULL_INDEX)
			{
				if (state.get().lentRentals.element(locals.index) == input.rentalId)
				{
					state.mut().lentRentals.remove(locals.index);
					break;
				}

				locals.index = state.get().lentRentals.nextElementIndex(locals.index);
			}

			locals.keyInput.owner = output.rental.renter;
			locals.key = qpi.K12(locals.keyInput);
			locals.index = state.get().takenRentals.headIndex(locals.key);
			while (locals.index != NULL_INDEX)
			{
				if (state.get().takenRentals.element(locals.index) == input.rentalId)
				{
					state.mut().takenRentals.remove(locals.index);
					break;
				}

				locals.index = state.get().takenRentals.nextElementIndex(locals.index);
			}

			output.rental.indexed = false;
			setMemory(locals.event, 0);
			locals.event.kind = EventKind::RentalEnded;
			locals.event.actor = output.rental.renter;
			locals.event.from = output.rental.owner;
			locals.event.reference = input.rentalId;
			locals.event.itemId = output.rental.itemId;
			locals.event.quantity = output.rental.quantity;
			CALL(AppendEvent, locals.event, locals.unused);
		}

		if ((output.rental.ownerCredit || output.rental.creatorCredit) && !output.rental.payoutQueued)
		{
			state.mut().rentalPayments.addTail(input.rentalId);
			output.rental.payoutQueued = true;
		}

		CALL(StoreRental, output.rental, locals.unused);
	}

	/** @brief Settle the full pair list, resuming by stable rental ID after removals relocate indices. */
	PRIVATE_PROCEDURE_WITH_LOCALS(SweepRentalPair)
	{
		locals.keyInput.owner = input.pair.owner;
		locals.keyInput.itemId = input.pair.itemId;
		locals.key = qpi.K12(locals.keyInput);
		locals.index = input.renter ? state.get().takenRentals.headIndex(locals.key) : state.get().lentRentals.headIndex(locals.key);
		while (locals.index != NULL_INDEX)
		{
			locals.settleInput.rentalId =
			    input.renter ? state.get().takenRentals.element(locals.index) : state.get().lentRentals.element(locals.index);
			// Both indexes use unique, positive rental IDs as priorities; skip the processed ID even if it remains active.
			locals.nextPriority = static_cast<sint64>(locals.settleInput.rentalId) - 1;
			CALL(SettleRental, locals.settleInput, locals.settled);
			locals.index = input.renter ? state.get().takenRentals.headIndex(locals.key, locals.nextPriority)
			                            : state.get().lentRentals.headIndex(locals.key, locals.nextPriority);
		}
	}

	/** @brief Credit-before-callback accounting; a rejected QU transfer remains a liability of this rental. */
	PRIVATE_PROCEDURE_WITH_LOCALS(PayRental)
	{
		if (state.get().rentalPaymentRunning || !state.get().rentals.get(input.rentalId, locals.rental))
		{
			return;
		}

		state.mut().rentalPaymentRunning = true;
		locals.amount = locals.rental.ownerCredit;
		if (locals.amount > 0)
		{
			locals.rental.ownerCredit = 0;
			state.mut().rentalCredits -= locals.amount;
			state.mut().rentals.set(input.rentalId, locals.rental);
			if (qpi.transfer(locals.rental.owner, locals.amount) < 0)
			{
				locals.rental.ownerCredit = locals.amount;
				state.mut().rentalCredits += locals.amount;
			}
		}

		locals.amount = locals.rental.creatorCredit;
		if (locals.amount > 0)
		{
			locals.rental.creatorCredit = 0;
			state.mut().rentalCredits -= locals.amount;
			state.mut().rentals.set(input.rentalId, locals.rental);
			if (qpi.transfer(locals.rental.creator, locals.amount) < 0)
			{
				locals.rental.creatorCredit = locals.amount;
				state.mut().rentalCredits += locals.amount;
			}
		}

		state.mut().rentalPaymentRunning = false;
		if ((locals.rental.ownerCredit || locals.rental.creatorCredit) && !locals.rental.payoutQueued)
		{
			state.mut().rentalPayments.addTail(input.rentalId);
			locals.rental.payoutQueued = true;
		}

		CALL(StoreRental, locals.rental, locals.unused);
	}

	/** @brief Apply the collection transfer policy without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetExternalTransfersDisabled)
	{
		output.itemId = input.collectionId;
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (!IsCollectionManager(locals.collection, qpi.invocator()))
		{
			output.code = Code::Unauthorized;
			return;
		}

		if (locals.collection.externalTransfersDisabled == input.disabled)
		{
			return;
		}

		locals.collection.externalTransfersDisabled = input.disabled;
		state.mut().collections.set(input.collectionId, locals.collection);

		locals.event.kind = EventKind::CollectionTransferPolicyChanged;
		locals.event.actor = qpi.invocator();
		locals.event.reference = input.collectionId;
		locals.event.amount = locals.collection.externalTransfersDisabled;
		locals.event.action = input.disabled;
		CALL(AppendEvent, locals.event, locals.unused);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCreateRentalOffer)
	{
		if (state.get().rentalPaymentRunning)
		{
			output.code = Code::Conflict;
			return;
		}

		if (qpi.invocator() == NULL_ID || qpi.invocator() == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (!isInRange<sint64>(input.dailyPrice, 1, MAX_AMOUNT) || !isInRange<sint64>(input.quantity, 1, MAX_AMOUNT))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.periodDays < QNFT_MIN_RENTAL_DAYS)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.dailyPrice > div<sint64>(MAX_AMOUNT, input.periodDays) ||
		    input.quantity > div<sint64>(div<sint64>(MAX_AMOUNT, input.periodDays), input.dailyPrice))
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.until = qpi.now();
		if (!IsTimeValid(locals.until) || !locals.until.addDays(input.periodDays) || state.get().exporting.get(input.itemId))
		{
			output.code = Code::Conflict;
			return;
		}

		if (state.get().rentalOffers.population() >= QNFT_MAX_RENTAL_OFFERS || state.get().lastRentalOfferId == 0x7fffffffffffffffULL)
		{
			output.code = Code::Capacity;
			return;
		}

		locals.balanceInput.owner = qpi.invocator();
		locals.balanceInput.itemId = input.itemId;
		CALL(RentalBalance, locals.balanceInput, locals.balance);
		if (locals.balance.code != Code::Ok)
		{
			output.code = locals.balance.code;
			return;
		}

		if (locals.balance.availableQuantity < input.quantity)
		{
			output.code = Code::InsufficientBalance;
			return;
		}

		locals.key = qpi.K12(locals.balanceInput);
		locals.held = locals.balance.offeredQuantity + input.quantity;
		locals.offer.owner = qpi.invocator();
		locals.offer.offerId = ++state.mut().lastRentalOfferId;
		locals.offer.quantity = input.quantity;
		locals.offer.dailyPrice = input.dailyPrice;
		locals.offer.itemId = input.itemId;
		locals.offer.periodDays = input.periodDays;
		state.mut().rentalOffers.set(locals.offer.offerId, locals.offer);
		state.mut().offeredRentalQuantities.set(locals.key, locals.held);
		output.offerId = locals.offer.offerId;

		locals.event.kind = EventKind::RentalOfferChanged;
		locals.event.actor = qpi.invocator();
		locals.event.reference = output.offerId;
		locals.event.itemId = input.itemId;
		locals.event.quantity = input.quantity;
		CALL(AppendEvent, locals.event, locals.unused);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCancelRentalOffer)
	{
		output.offerId = input.offerId;
		if (state.get().rentalPaymentRunning)
		{
			output.code = Code::Conflict;
			return;
		}

		if (!state.get().rentalOffers.get(input.offerId, locals.offer))
		{
			output.code = Code::NotFound;
			return;
		}

		if (locals.offer.owner != qpi.invocator())
		{
			output.code = Code::Unauthorized;
			return;
		}

		locals.keyInput.owner = locals.offer.owner;
		locals.keyInput.itemId = locals.offer.itemId;
		locals.key = qpi.K12(locals.keyInput);
		state.get().offeredRentalQuantities.get(locals.key, locals.held);
		locals.held -= locals.offer.quantity;
		if (locals.held > 0)
		{
			state.mut().offeredRentalQuantities.set(locals.key, locals.held);
		}
		else
		{
			state.mut().offeredRentalQuantities.removeByKey(locals.key);
		}

		state.mut().rentalOffers.removeByKey(input.offerId);
		locals.event.kind = EventKind::RentalOfferChanged;
		locals.event.actor = qpi.invocator();
		locals.event.reference = input.offerId;
		locals.event.itemId = locals.offer.itemId;
		CALL(AppendEvent, locals.event, locals.unused);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteRent)
	{
		if (state.get().rentalPaymentRunning)
		{
			output.code = Code::Conflict;
			return;
		}

		if (!state.get().rentalOffers.get(input.offerId, locals.offer))
		{
			output.code = Code::NotFound;
			return;
		}

		if (qpi.invocator() == NULL_ID || qpi.invocator() == SELF || qpi.invocator() == locals.offer.owner)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (!isInRange<sint64>(input.quantity, 1, locals.offer.quantity))
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (qpi.invocationReward() > MAX_AMOUNT)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.expectedDailyPrice != locals.offer.dailyPrice || input.expectedPeriodDays != locals.offer.periodDays ||
		    state.get().exporting.get(locals.offer.itemId))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.rental.paidUntil = qpi.now();
		if (!IsTimeValid(locals.rental.paidUntil) || !locals.rental.paidUntil.addDays(locals.offer.periodDays))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.rental.periodPrice = smul(smul(locals.offer.dailyPrice, static_cast<sint64>(locals.offer.periodDays)), input.quantity);
		if (qpi.invocationReward() < locals.rental.periodPrice)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		locals.sweep.pair.owner = locals.offer.owner;
		locals.sweep.pair.itemId = locals.offer.itemId;
		CALL(SweepRentalPair, locals.sweep, locals.unused);
		locals.sweep.pair.owner = qpi.invocator();
		locals.sweep.renter = true;
		CALL(SweepRentalPair, locals.sweep, locals.unused);
		locals.keyInput.owner = locals.offer.owner;
		locals.keyInput.itemId = locals.offer.itemId;
		locals.ownerKey = qpi.K12(locals.keyInput);
		locals.keyInput.owner = qpi.invocator();
		locals.renterKey = qpi.K12(locals.keyInput);
		if (state.get().rentals.population() >= QNFT_MAX_RENTALS || state.get().rentalDeadlines.population() >= QNFT_MAX_RENTALS ||
		    state.get().lastRentalId == 0x7fffffffffffffffULL)
		{
			output.code = Code::Capacity;
			return;
		}

		locals.rental.rentalId = ++state.mut().lastRentalId;
		locals.rental.offerId = input.offerId;
		locals.rental.owner = locals.offer.owner;
		locals.rental.renter = qpi.invocator();
		locals.rental.creator = state.get().collections.get(state.get().items.get(locals.offer.itemId).collectionId).creator;
		locals.rental.quantity = input.quantity;
		locals.rental.itemId = locals.offer.itemId;
		locals.rental.periodDays = locals.offer.periodDays;
		locals.rental.dailyPrice = locals.offer.dailyPrice;
		locals.rental.autoRenew = true;
		locals.rental.indexed = true;
		locals.rental.payoutQueued = true;
		locals.rental.deposit = qpi.invocationReward() - locals.rental.periodPrice;
		locals.protocol = calculateFeeShare(locals.rental.periodPrice, state.get().rentalProtocolBPS);
		locals.creator = calculateFeeShare(locals.rental.periodPrice, state.get().rentalCreatorBPS);
		locals.rental.ownerCredit = locals.rental.periodPrice - locals.protocol - locals.creator;
		locals.rental.creatorCredit = locals.creator;
		// All reservations and liabilities are committed before any beneficiary callback.
		locals.offer.quantity -= input.quantity;
		if (locals.offer.quantity > 0)
		{
			state.mut().rentalOffers.set(input.offerId, locals.offer);
		}
		else
		{
			state.mut().rentalOffers.removeByKey(input.offerId);
		}

		state.get().offeredRentalQuantities.get(locals.ownerKey, locals.held);
		locals.held -= input.quantity;
		if (locals.held > 0)
		{
			state.mut().offeredRentalQuantities.set(locals.ownerKey, locals.held);
		}
		else
		{
			state.mut().offeredRentalQuantities.removeByKey(locals.ownerKey);
		}

		state.mut().rentals.set(locals.rental.rentalId, locals.rental);
		state.mut().lentRentals.add(locals.ownerKey, locals.rental.rentalId, static_cast<sint64>(locals.rental.rentalId));
		state.mut().takenRentals.add(locals.renterKey, locals.rental.rentalId, static_cast<sint64>(locals.rental.rentalId));
		// A fixed epoch makes deadlines comparable across rentals created on different ticks.
		// Collection puts the highest priority first; negation puts the earliest expiry first.
		locals.origin = DateAndTime(1970, 1, 1);
		state.mut().rentalDeadlines.add(SELF, locals.rental.rentalId, -static_cast<sint64>(locals.origin.durationMicrosec(locals.rental.paidUntil)));
		state.mut().rentalPayments.addTail(locals.rental.rentalId);
		state.mut().rentalDeposits += locals.rental.deposit;
		state.mut().rentalCredits += locals.rental.periodPrice - locals.protocol;
		state.mut().epochFees += locals.protocol;
		output.rentalId = locals.rental.rentalId;
		output.offerId = input.offerId;
		output.paid = locals.rental.periodPrice;
		locals.event.kind = EventKind::RentalStarted;
		locals.event.actor = qpi.invocator();
		locals.event.from = locals.offer.owner;
		locals.event.reference = output.rentalId;
		locals.event.itemId = locals.rental.itemId;
		locals.event.quantity = input.quantity;
		locals.event.amount = output.paid;
		CALL(AppendEvent, locals.event, locals.unused);
		locals.payment.rentalId = output.rentalId;
		CALL(PayRental, locals.payment, locals.unused);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ChangeRental)
	{
		output.rentalId = input.rentalId;
		if (state.get().rentalPaymentRunning)
		{
			output.code = Code::Conflict;
			return;
		}

		if (!state.get().rentals.get(input.rentalId, locals.rental))
		{
			output.code = Code::NotFound;
			return;
		}

		if (qpi.invocator() != locals.rental.renter && (input.action != RentalAction::CancelRenewal || qpi.invocator() != locals.rental.owner))
		{
			output.code = Code::Unauthorized;
			return;
		}

		if ((input.action == RentalAction::TopUp && !isInRange<sint64>(qpi.invocationReward(), 1, MAX_AMOUNT)) ||
		    (input.action == RentalAction::Withdraw && !isInRange<sint64>(input.amount, 1, MAX_AMOUNT)))
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.settleInput.rentalId = input.rentalId;
		CALL(SettleRental, locals.settleInput, locals.settled);
		if (locals.settled.code != Code::Ok)
		{
			output.code = locals.settled.code;
			return;
		}

		locals.rental = locals.settled.rental;
		if (input.action == RentalAction::TopUp)
		{
			if (locals.rental.status != RentalStatus::Active)
			{
				output.code = Code::Conflict;
				return;
			}

			if (qpi.invocationReward() > MAX_AMOUNT - locals.rental.deposit - locals.rental.ownerCredit - locals.rental.creatorCredit)
			{
				output.code = Code::Capacity;
				return;
			}

			locals.rental.deposit += qpi.invocationReward();
			state.mut().rentalDeposits += qpi.invocationReward();
			locals.event.amount = qpi.invocationReward();
		}
		else if (input.action == RentalAction::Withdraw)
		{
			if (input.amount > locals.rental.deposit)
			{
				output.code = Code::InsufficientBalance;
				return;
			}

			locals.rental.deposit -= input.amount;
			state.mut().rentalDeposits -= input.amount;
			state.mut().rentals.set(input.rentalId, locals.rental);
			state.mut().rentalPaymentRunning = true;
			if (qpi.transfer(locals.rental.renter, input.amount) < 0)
			{
				locals.rental.deposit += input.amount;
				state.mut().rentalDeposits += input.amount;
				state.mut().rentals.set(input.rentalId, locals.rental);
				output.code = Code::ExternalFailure;
			}

			state.mut().rentalPaymentRunning = false;
			if (output.code != Code::Ok)
			{
				return;
			}

			locals.event.amount = -input.amount;
		}
		else
		{
			if (locals.rental.status != RentalStatus::Active)
			{
				output.code = Code::Conflict;
				return;
			}

			if (!locals.rental.autoRenew)
			{
				return;
			}

			locals.rental.autoRenew = false;
		}

		CALL(StoreRental, locals.rental, locals.unused);
		locals.event.kind = input.action == RentalAction::CancelRenewal ? EventKind::RentalRenewalCancelled : EventKind::RentalDepositChanged;
		locals.event.actor = qpi.invocator();
		locals.event.reference = input.rentalId;
		locals.event.itemId = locals.rental.itemId;
		CALL(AppendEvent, locals.event, locals.unused);
		CALL(PayRental, locals.settleInput, locals.unused);
	}

	/** @brief Credit a rental deposit without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteTopUpRental)
	{
		locals.change.rentalId = input.rentalId;
		locals.change.action = RentalAction::TopUp;
		CALL(ChangeRental, locals.change, output);
	}

	/** @brief Withdraw free rental collateral without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteWithdrawRentalDeposit)
	{
		locals.change.rentalId = input.rentalId;
		locals.change.action = RentalAction::Withdraw;
		locals.change.amount = input.amount;
		CALL(ChangeRental, locals.change, output);
	}

	/** @brief Stop renewal without settling the invocation payment. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCancelRentalRenewal)
	{
		locals.change.rentalId = input.rentalId;
		locals.change.action = RentalAction::CancelRenewal;
		CALL(ChangeRental, locals.change, output);
	}

	/** @brief Bounded deadline and failed-payment queues; no full registry scan occurs on a tick. */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessRentals)
	{
		locals.now = qpi.now();
		if (!IsTimeValid(locals.now) || state.get().rentalPaymentRunning)
		{
			return;
		}

		// Use the same epoch and units as stored deadlines to process only rentals that are due.
		locals.origin = DateAndTime(1970, 1, 1);
		locals.priority = -static_cast<sint64>(locals.origin.durationMicrosec(locals.now));
		while (locals.count < QNFT_RENTALS_PER_TICK)
		{
			locals.index = state.get().rentalDeadlines.headIndex(SELF);
			if (locals.index == NULL_INDEX || state.get().rentalDeadlines.priority(locals.index) < locals.priority)
			{
				break;
			}

			locals.settleInput.rentalId = state.get().rentalDeadlines.element(locals.index);
			state.mut().rentalDeadlines.remove(locals.index);
			CALL(SettleRental, locals.settleInput, locals.settled);
			if (locals.settled.code == Code::Ok && locals.settled.rental.status == RentalStatus::Active)
			{
				state.mut().rentalDeadlines.add(SELF, locals.settleInput.rentalId,
				                                -static_cast<sint64>(locals.origin.durationMicrosec(locals.settled.rental.paidUntil)));
			}

			++locals.count;
		}

		locals.remaining = minimum<uint64>(state.get().rentalPayments.population(), QNFT_RENTALS_PER_TICK);
		while (locals.remaining)
		{
			locals.index = state.get().rentalPayments.headIndex();
			locals.settleInput.rentalId = state.get().rentalPayments.element(locals.index);
			state.mut().rentalPayments.remove(locals.index);
			if (state.get().rentals.get(locals.settleInput.rentalId, locals.rental))
			{
				locals.rental.payoutQueued = false;
				state.mut().rentals.set(locals.settleInput.rentalId, locals.rental);
				CALL(PayRental, locals.settleInput, locals.unused);
			}

			--locals.remaining;
		}
	}

	/**
	 * @brief Compact persistent maps after epoch settlement and restart refund traversal.
	 * @note Preserves keys and values; listing map-slot cursors must restart after cleanup.
	 */
	PRIVATE_PROCEDURE(CleanupEpochData)
	{
		state.mut().operatorApprovals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		// Compact after all epoch work: cleanup invalidates map slots, but preserves keys and values.
		// Listing pagination must restart across an epoch boundary; no slot indices are persisted here.
		state.mut().listings.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().contractRefunds.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().rentalOffers.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().rentals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().offeredRentalQuantities.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().lentRentals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().takenRentals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().refundCursor = 0;
	}

	/**
	 * @brief Grant or revoke caller-wide operator authority and emit ApprovalForAll on success. Rejects zero/self
	 * operators. Successful grants charge approveFee; revocations and errors refund attached QU.
	 * Failed refunds remain credits.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetApprovalForAll)
	{
		CALL(ExecuteSetApprovalForAll, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = output.code == Code::Ok && input.approved ? state.get().approveFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Preflight all entries, then transfer in order; core failures can commit a prefix.
	 * @note Charges per completed entry, including zero/self transfers. Emits TransferSingle per entry.
	 * Duplicate IDs are checked cumulatively before any transfer. No generic receiver callback is promised.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BatchTransfer)
	{
		CALL(ExecuteBatchTransfer, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = output.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Issue up to 16 types sharing receiver, collection, metadata and royalty settings.
	 * @note Use a literal {id} URI template for distinct off-chain documents. No additional issuance of an existing ID.
	 * @return The same preflight, per-entry results and partial-execution semantics as MintBatch.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(MultiMint)
	{
		CALL(ExecuteMultiMint, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = output.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Burn positive quantities of up to 16 native types, checking duplicate IDs cumulatively.
	 * @return Preflight errors burn nothing; an unexpected core failure reports the committed prefix.
	 * @note Free; attached QU is refunded. Lifetime mint quotas are not restored.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BurnBatch)
	{
		CALL(ExecuteBurnBatch, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = output.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Collection creator or current administrator may pause/resume issuance or close it permanently.
	 * @note No fee. Previously issued mint deliveries continue automatically on ticks.
	 * @return Conflict for attempts to reopen a closed collection, Unauthorized without collection-management authority.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetMintingStatus)
	{
		CALL(ExecuteSetMintingStatus, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Issue multiple fixed editions; preflight fees and aggregate quotas before irreversible issuance.
	 * @note A core failure can commit a prefix. Pending delivery is issued exactly once and retried automatically on ticks.
	 * @return Per-entry results and committed count; fees are charged only for successfully issued types.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(MintBatch)
	{
		CALL(ExecuteMintBatch, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = output.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/**
	 * @brief Create a collection and settle its fee or refund after every result.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateCollection)
	{
		// Charge for the collection only when allocation succeeds; rejected attempts retain no fee.
		CALL(ExecuteCreateCollection, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::CreateCollection;
		locals.finishInput.result = output;
		locals.finishInput.fee = output.code == Code::Ok ? state.get().collectionFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Nominate the next collection administrator as the creator or current administrator.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(NominateAdmin)
	{
		// A failed nomination must not retain the administration fee.
		CALL(ExecuteNominateAdmin, input, output);

		locals.finishInput.fee = output.code == Code::Ok ? state.get().nominateAdminFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::NominateAdmin;
		locals.finishInput.receiver = input.receiver;
		locals.finishInput.collectionId = input.collectionId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Accept collection administration as the nominated account.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(AcceptAdmin)
	{
		// Accepting a nomination is free; any attached QU is returned during settlement.
		CALL(ExecuteAcceptAdmin, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::AcceptAdmin;
		locals.finishInput.collectionId = input.collectionId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Issue a fixed edition and deliver it, preserving recoverable delivery failures.
	 * @note Charges once per issued asset, not per unit; failed delivery is retried automatically on ticks.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Mint)
	{
		// The helper reports an issuance fee even when delivery is pending, preventing a second issuance charge on recovery.
		CALL(ExecuteMint, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Mint;
		locals.finishInput.receiver = input.receiver;
		locals.finishInput.collectionId = input.collectionId;
		locals.finishInput.result = output;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Transfer owned units or use an owner-wide operator approval.
	 * @note Only the owner or the approved operator may transfer; delivery preserves approval.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferFrom)
	{
		// Charge the transfer fee only after the requested units have reached the recipient.
		CALL(ExecuteTransfer, input, output);

		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Transfer;
		locals.finishInput.receiver = input.receiver;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.fee = output.code == Code::Ok ? state.get().transferFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Update mutable metadata as a collection manager.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(UpdateMetadata)
	{
		// Charge the metadata fee only for a committed update.
		CALL(ExecuteUpdateMetadata, input, output);

		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::UpdateMetadata;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.fee = output.code == Code::Ok ? state.get().updateMetadataFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Permanently freeze editable native-item metadata.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(FreezeMetadata)
	{
		// Freezing metadata is free and settles any attached QU without charging an update fee.
		CALL(ExecuteFreezeMetadata, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::FreezeMetadata;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Release asset management to an accepting registered destination.
	 * @note Ownership is unchanged. Destination fees already consumed by QPI cannot be refunded.
	 * @note Destination callbacks follow the accepted Mainnet contract-review trust model.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Export)
	{
		CALL(ExecuteExport, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Export;
		locals.finishInput.receiver = id(input.destination, 0, 0, 0);
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.operationOutput.retainedAmount;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Burn native units without restoring collection issuance quota.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Burn)
	{
		// Burning is free and does not revoke the owner's owner-wide operator approval.
		CALL(ExecuteBurn, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Burn;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Burn the entire invocation reward into the execution reserve.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(FundReserve)
	{
		// FinishCommand burns reserve contributions instead of adding them to the epoch fee pool.
		CALL(ExecuteFundReserve, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::FundReserve;
		locals.finishInput.result = output;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Transfer developer authority and future revenue payouts to a new beneficiary.
	 * @param input New nonzero developer ID, excluding this contract.
	 * @return Unauthorized unless invoked by the current developer; InvalidInput for an invalid ID; otherwise Ok.
	 * @note Fee-free; attached QU is refunded. Unpaid developer revenue follows the new beneficiary.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(ChangeDeveloper)
	{
		CALL(ExecuteChangeDeveloper, input, output);
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Execute ListForSale and settle its fee, refund and market event. */
	PUBLIC_PROCEDURE_WITH_LOCALS(ListForSale)
	{
		CALL(ExecuteListForSale, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.marketEvent = locals.operationOutput.event;
		// Settle payment and emit exactly one market success or failure through the shared finalizer.
		locals.finishInput.domain = FailureDomain::Market;
		locals.finishInput.marketEvent.type = MarketAction::List;
		locals.finishInput.marketEvent.itemId = input.itemId;
		locals.finishInput.fee = output.code == Code::Ok ? state.get().listingFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result.code = output.code;
		CALL(FinishCommand, locals.finishInput, locals.settled);
	}

	/** @brief Execute CancelSale and settle its fee, refund and market event. */
	PUBLIC_PROCEDURE_WITH_LOCALS(CancelSale)
	{
		CALL(ExecuteCancelSale, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.marketEvent = locals.operationOutput.event;
		// Settle payment and emit exactly one market success or failure through the shared finalizer.
		locals.finishInput.domain = FailureDomain::Market;
		locals.finishInput.marketEvent.type = MarketAction::Cancel;
		locals.finishInput.marketEvent.listingId = input.listingId;
		locals.finishInput.result.code = output.code;
		CALL(FinishCommand, locals.finishInput, locals.settled);
	}

	/**
	 * @brief Buy a protected-price quantity and settle seller proceeds, royalties and configured sale dividends.
	 * @note Snapshots shareholders before delivery and reserves debts before callbacks. Failed beneficiary payments retry on END_TICK; failed
	 * purchases refund the reward.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Buy)
	{
		// The helper delivers assets and pays beneficiaries; settlement refunds change and accrues only the shop fee.
		CALL(ExecuteBuy, input, locals.operationOutput);
		output = locals.operationOutput.result;

		locals.finishInput.domain = FailureDomain::Market;
		locals.finishInput.marketEvent = locals.operationOutput.event;
		locals.finishInput.marketEvent.type = MarketAction::Buy;
		locals.finishInput.marketEvent.listingId = input.listingId;
		locals.finishInput.result.code = output.code;
		locals.finishInput.retainedAmount = output.paid;
		locals.finishInput.fee = output.fee;
		CALL(FinishCommand, locals.finishInput, locals.settled);
	}

	/** @brief Execute SetFeeProposalBatch and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetFeeProposalBatch)
	{
		CALL(ExecuteSetFeeProposalBatch, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Execute SetShareholderProposal and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetShareholderProposal)
	{
		CALL(ExecuteSetShareholderProposal, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Execute SetShareholderVotes and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetShareholderVotes)
	{
		CALL(ExecuteSetShareholderVotes, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.finishInput.events = FinishEvents::CallerManaged;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.finishOutput);
	}

	/** @brief Update the collection transfer policy and refund attached QU. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetExternalTransfersDisabled)
	{
		CALL(ExecuteSetExternalTransfersDisabled, input, output);
		locals.finish.result = output;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Reserve the caller's available units at fixed QU/day terms of at least 15 days; rejects invalid time, quantity or capacity and refunds
	 * attached QU. */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateRentalOffer)
	{
		CALL(ExecuteCreateRentalOffer, input, output);
		locals.finish.result.code = output.code;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Only the lender may cancel an unpaid offer remainder; releases its reserved units, preserves accepted rentals and refunds attached QU.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CancelRentalOffer)
	{
		CALL(ExecuteCancelRentalOffer, input, output);
		locals.finish.result.code = output.code;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Accept an unchanged offer for a positive quantity; pays the first period, retains excess QU as deposit and preserves lender ownership.
	 * Errors refund attached QU. */
	PUBLIC_PROCEDURE_WITH_LOCALS(Rent)
	{
		CALL(ExecuteRent, input, output);
		locals.finish.result.code = output.code;
		locals.finish.retainedAmount = output.code == Code::Ok ? qpi.invocationReward() : 0;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Credit attached QU to the rental deposit; rejected payments are refunded. */
	PUBLIC_PROCEDURE_WITH_LOCALS(TopUpRental)
	{
		CALL(ExecuteTopUpRental, input, output);
		locals.finish.result.code = output.code;
		locals.finish.retainedAmount = output.code == Code::Ok ? qpi.invocationReward() : 0;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Withdraw unused rental collateral and refund attached QU. */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawRentalDeposit)
	{
		CALL(ExecuteWithdrawRentalDeposit, input, output);
		locals.finish.result.code = output.code;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/** @brief Disable renewal while preserving paid access and refund attached QU. */
	PUBLIC_PROCEDURE_WITH_LOCALS(CancelRentalRenewal)
	{
		CALL(ExecuteCancelRentalRenewal, input, output);
		locals.finish.result.code = output.code;
		locals.finish.events = FinishEvents::CallerManaged;
		CALL(FinishCommand, locals.finish, locals.finished);
	}

	/**
	 * @brief Accept positive quantities of registered QNFT assets; log rejected acquisitions.
	 */
	PRE_ACQUIRE_SHARES_WITH_LOCALS()
	{
		// Accept management only for positive quantities whose issuer and encoded name resolve to a registered item.
		if (input.numberOfShares > 0)
		{
			locals.lookupInput.asset = input.asset;
			CALL(Lookup, locals.lookupInput, locals.lookupOutput);
			output.allowTransfer = locals.lookupOutput.valid;
		}

		if (!output.allowTransfer)
		{
			locals.failureInput.domain = FailureDomain::Management;
			locals.failureInput.code = input.numberOfShares <= 0 ? Code::InvalidInput : Code::NotFound;
			locals.failureInput.itemId = locals.lookupOutput.itemId;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
		}
	}

	/**
	 * @brief Preserve operator approvals after management returns to QNFT.
	 */
	POST_ACQUIRE_SHARES()
	{
		// Returning management does not change ownership or revoke existing operator permissions.
	}

	/**
	 * @brief Reject unsolicited acquisition requests; owners export through the explicit procedure.
	 */
	PRE_RELEASE_SHARES()
	{
		// Leave allowTransfer false; management leaves QNFT only through the authorized Export procedure.
	}

	// Support registered contract shareholders through the standard QPI notification route.
	SET_SHAREHOLDER_PROPOSAL_WITH_LOCALS()
	{
		// Decode the notification buffer and reuse the public proposal validation and settlement path.
		copyFromBuffer(locals.userProcInput, input);
		CALL(SetShareholderProposal, locals.userProcInput, output);
	}

	// Forward contract-shareholder ballots to the same authorization and refund path.
	SET_SHAREHOLDER_VOTES()
	{
		// Reuse QPI authorization and refund handling for contract-shareholder ballot notifications.
		CALL(SetShareholderVotes, input, output);
	}

	/** @brief Process bounded mint deliveries, rental work and refund retries after transactions. */
	END_TICK_WITH_LOCALS()
	{
		CALL(ProcessMintDeliveries, locals.refundInput, locals.refundOutput);
		CALL(ProcessRentals, locals.refundInput, locals.refundOutput);
		CALL(RetryRefunds, locals.refundInput, locals.refundOutput);
	}

	/**
	 * @brief Settle fees and governance, preserve operator approvals, and compact persistent maps when needed.
	 * @note Cleanup preserves keys and values but invalidates listing map-slot cursors.
	 */
	END_EPOCH_WITH_LOCALS()
	{
		// Pay the closing epoch with its existing rates before applying shareholder-approved changes.
		CALL(DistributeFees, locals.distributionInput, locals.distributionOutput);
		CALL(FinalizeShareholderStateVarProposals, input, output);
		CALL(CleanupEpochData, input, output);
	}

	/**
	 * @brief Initialize fee tariffs and the default 20% developer / 60% shareholders / 20% burn split.
	 * @note Sets the developer beneficiary for revenue payouts.
	 */
	INITIALIZE()
	{
		state.mut().developer = ID(_O, _C, _Z, _W, _N, _J, _S, _N, _R, _U, _Q, _J, _U, _A, _H, _Z, _C, _T, _R, _P, _N, _Y, _W, _G, _G, _E, _F, _C, _X,
		                           _B, _A, _V, _F, _O, _P, _R, _S, _N, _U, _L, _U, _E, _B, _S, _P, _U, _T, _R, _Z, _N, _T, _G, _F, _B, _I, _E);

		// QPI zero-initializes state; set only the nonzero initial tariffs and revenue rates here.
		state.mut().collectionFee = QNFT_COLLECTION_FEE;
		state.mut().mintFee = QNFT_MINT_FEE;
		state.mut().nominateAdminFee = QNFT_ACTION_FEE;
		state.mut().transferFee = QNFT_ACTION_FEE;
		state.mut().approveFee = QNFT_ACTION_FEE;
		state.mut().updateMetadataFee = QNFT_METADATA_FEE;
		state.mut().exportFee = QNFT_ACTION_FEE;
		state.mut().listingFee = QNFT_ACTION_FEE;
		state.mut().proposalFee = QNFT_PROPOSAL_FEE;
		state.mut().shopFeeBPS = QNFT_SHOP_FEE_BPS;
		state.mut().rentalProtocolBPS = QNFT_RENTAL_PROTOCOL_BPS;
		state.mut().rentalCreatorBPS = QNFT_RENTAL_CREATOR_BPS;
		state.mut().developerFeeBPS = QNFT_DEVELOPER_FEE_BPS;
		state.mut().shareholderFeeBPS = QNFT_SHAREHOLDER_FEE_BPS;
		state.mut().burnFeeBPS = QNFT_BURN_FEE_BPS;
	}

	/**
	 * @brief Register procedures and read-only functions in separate, purpose-ordered groups.
	 */
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// Procedures: collections and administration.
		REGISTER_USER_PROCEDURE(CreateCollection, 1);
		REGISTER_USER_PROCEDURE(NominateAdmin, 2);
		REGISTER_USER_PROCEDURE(AcceptAdmin, 3);
		REGISTER_USER_PROCEDURE(SetMintingStatus, 4);
		REGISTER_USER_PROCEDURE(SetExternalTransfersDisabled, 5);

		// Procedures: issuance.
		REGISTER_USER_PROCEDURE(Mint, 6);
		REGISTER_USER_PROCEDURE(MintBatch, 7);
		REGISTER_USER_PROCEDURE(MultiMint, 8);

		// Procedures: metadata.
		REGISTER_USER_PROCEDURE(UpdateMetadata, 9);
		REGISTER_USER_PROCEDURE(FreezeMetadata, 10);

		// Procedures: approvals.
		REGISTER_USER_PROCEDURE(SetApprovalForAll, 11);

		// Procedures: transfers, burns and export.
		REGISTER_USER_PROCEDURE(TransferFrom, 12);
		REGISTER_USER_PROCEDURE(BatchTransfer, 13);
		REGISTER_USER_PROCEDURE(Burn, 14);
		REGISTER_USER_PROCEDURE(BurnBatch, 15);
		REGISTER_USER_PROCEDURE(Export, 16);

		// Procedures: shop.
		REGISTER_USER_PROCEDURE(ListForSale, 17);
		REGISTER_USER_PROCEDURE(Buy, 18);
		REGISTER_USER_PROCEDURE(CancelSale, 19);

		// Procedures: rentals.
		REGISTER_USER_PROCEDURE(CreateRentalOffer, 20);
		REGISTER_USER_PROCEDURE(CancelRentalOffer, 21);
		REGISTER_USER_PROCEDURE(Rent, 22);
		REGISTER_USER_PROCEDURE(TopUpRental, 23);
		REGISTER_USER_PROCEDURE(WithdrawRentalDeposit, 24);
		REGISTER_USER_PROCEDURE(CancelRentalRenewal, 25);

		// Procedures: protocol funding, fees and management.
		REGISTER_USER_PROCEDURE(FundReserve, 26);
		REGISTER_USER_PROCEDURE(SetFeeProposalBatch, 27);
		REGISTER_USER_PROCEDURE(ChangeDeveloper, 28);

		// Functions: collections.
		REGISTER_USER_FUNCTION(GetCollection, 1);
		REGISTER_USER_FUNCTION(ListCollections, 2);
		REGISTER_USER_FUNCTION(GetMintingStatus, 3);

		// Functions: items and supply.
		REGISTER_USER_FUNCTION(GetItem, 4);
		REGISTER_USER_FUNCTION(ListItems, 5);
		REGISTER_USER_FUNCTION(Exists, 6);
		REGISTER_USER_FUNCTION(TotalSupply, 7);
		REGISTER_USER_FUNCTION(GetAssetState, 8);

		// Functions: metadata and royalties.
		REGISTER_USER_FUNCTION(URI, 9);
		REGISTER_USER_FUNCTION(ResolvedURI, 10);
		REGISTER_USER_FUNCTION(RoyaltyInfo, 11);
		REGISTER_USER_FUNCTION(DividendInfo, 12);

		// Functions: balances and holders.
		REGISTER_USER_FUNCTION(BalanceOf, 13);
		REGISTER_USER_FUNCTION(BalanceOfBatch, 14);
		REGISTER_USER_FUNCTION(GetBalance, 15);
		REGISTER_USER_FUNCTION(ListHolders, 16);
		REGISTER_USER_FUNCTION(GetUserNft, 17);
		REGISTER_USER_FUNCTION(ListUserNfts, 18);

		// Functions: approvals.
		REGISTER_USER_FUNCTION(IsApprovedForAll, 19);

		// Functions: shop.
		REGISTER_USER_FUNCTION(GetListing, 20);
		REGISTER_USER_FUNCTION(ListListings, 21);
		REGISTER_USER_FUNCTION(GetMarket, 22);

		// Functions: rentals.
		REGISTER_USER_FUNCTION(GetRentalOffer, 23);
		REGISTER_USER_FUNCTION(ListRentalOffers, 24);
		REGISTER_USER_FUNCTION(GetRental, 25);
		REGISTER_USER_FUNCTION(ListUserRentals, 26);

		// Functions: refunds and deferred payments.
		REGISTER_USER_FUNCTION(GetRefund, 27);
		REGISTER_USER_FUNCTION(ListRefunds, 28);

		// Functions: protocol configuration and fees.
		REGISTER_USER_FUNCTION(GetConfig, 29);
		REGISTER_USER_FUNCTION(GetProtocolInfo, 30);
		REGISTER_USER_FUNCTION(GetFeeSchedule, 31);

		// Functions: governance.
		REGISTER_USER_FUNCTION(GetGovernanceConfig, 32);
		REGISTER_USER_FUNCTION(GetFeeProposalBatch, 33);

		// Functions: event journal.
		REGISTER_USER_FUNCTION(GetEventRange, 34);
		REGISTER_USER_FUNCTION(GetEvents, 35);

		// Standard QPI shareholder voting uses reserved entry-point IDs.
		REGISTER_SHAREHOLDER_PROPOSAL_VOTING();
	}
};
