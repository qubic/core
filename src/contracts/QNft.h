using namespace QPI;

// First ASCII character of every native QNFT asset name.
constexpr uint64 QNFT_ASSET_NAME_PREFIX = Ch::N;
// Number of base-36 digits encoding the item ID after the prefix.
constexpr uint32 QNFT_ASSET_NAME_DIGITS = 6;
// Number of decimal characters in the shared asset-name alphabet.
constexpr uint32 QNFT_ASSET_NAME_DECIMAL_DIGITS = Ch::_9 - Ch::_0 + 1;
// Number of numeric and uppercase-letter symbols in the asset-name alphabet.
constexpr uint32 QNFT_ASSET_NAME_RADIX = QNFT_ASSET_NAME_DECIMAL_DIGITS + Ch::Z - Ch::A + 1;
// Width of one packed asset-name character, in bits.
constexpr uint32 QNFT_ASSET_NAME_BYTE_BITS = 8;
// Mask extracting one ASCII character from the packed asset name.
constexpr uint64 QNFT_ASSET_NAME_BYTE_MASK = (1ULL << QNFT_ASSET_NAME_BYTE_BITS) - 1;
// Fixed metadata URI capacity including the zero terminator, in bytes.
constexpr uint32 QNFT_METADATA_URI_CAPACITY = 256;
// First permitted non-space printable ASCII character in a metadata URI.
constexpr uint8 QNFT_METADATA_URI_MIN_CHARACTER = Ch::exclamation_mark;
// Last printable ASCII character (tilde); this QPI revision has no Ch::tilde enumerator.
constexpr uint8 QNFT_METADATA_URI_MAX_CHARACTER = 126;
// Fixed explanation URL capacity in the native proposal ABI, in bytes.
constexpr uint32 QNFT_PROPOSAL_URL_CAPACITY = 256;
// Power-of-two wire capacity for proposed fee changes; unused entries must be zero.
constexpr uint32 QNFT_FEE_BATCH_CAPACITY = 16;
// Number of stable public governance variable IDs; checked against FeeVariable below.
constexpr uint32 QNFT_FEE_VARIABLE_COUNT = 13;
// Stable variable IDs 3 and 4 jointly govern the complementary revenue shares.
constexpr uint32 QNFT_REVENUE_SPLIT_VARIABLE_MASK = (1U << 3) | (1U << 4);
// Maximum number of listing IDs returned in one shop page.
constexpr uint32 QNFT_PAGE_CAPACITY = 32;
// Maximum full registry records per response, keeping both page outputs below the QPI 65535-byte limit.
constexpr uint32 QNFT_REGISTRY_PAGE_CAPACITY = 16;
// Number of recent public events retained independently of node logging.
constexpr uint32 QNFT_EVENT_CAPACITY = 65536;
// Maximum public journal records returned by one query.
constexpr uint32 QNFT_EVENT_PAGE_CAPACITY = 16;
// Power-of-two ring slot mask; event IDs start at one.
constexpr uint64 QNFT_EVENT_SLOT_MASK = QNFT_EVENT_CAPACITY - 1;
// Last assignable event ID, leaving room for an exclusive end cursor.
constexpr uint64 QNFT_LAST_EVENT_ID = 0xfffffffffffffffeULL;
// Maximum proposal indices returned by the QPI proposal-index getter, in entries.
constexpr uint32 QNFT_PROPOSAL_INDEX_CAPACITY = 64;
// Deferred refund map capacity, with spare slots beyond all 1,024 core contract IDs.
constexpr uint32 QNFT_CONTRACT_REFUND_SLOTS = 2048;
// Maximum deferred refund transfers attempted per tick.
constexpr uint32 QNFT_REFUNDS_PER_TICK = 8;
// Removed-slot percentage above which persistent hash maps are compacted.
constexpr uint32 QNFT_MAP_CLEANUP_PERCENT = 25;
// Last permitted metadata revision; updates must not wrap this counter.
constexpr uint32 QNFT_MAX_METADATA_VERSION = 0xffffffffU;
// Denominator representing 100 percent in QBay royalty values.
constexpr uint32 QNFT_PERCENT_DENOMINATOR = 100;

// Maximum number of issued item IDs; import requests reserve capacity separately.
constexpr uint32 QNFT_MAX_ITEMS = 131072;
// Maximum number of collection records stored by the contract.
constexpr uint32 QNFT_MAX_COLLECTIONS = 16384;
// Maximum number of live owner/item approvals admitted to the map.
constexpr uint32 QNFT_MAX_APPROVALS = 49152;
// Maximum issued import deliveries attempted per tick, independent of queue length.
constexpr uint32 QNFT_IMPORT_DELIVERIES_PER_TICK = 8;
// Maximum number of simultaneously pending QBay import reservations.
constexpr uint32 QNFT_MAX_IMPORTS = 256;
// Keep the request map below half occupancy and reserve the last request ID as exhaustion marker.
constexpr uint32 QNFT_IMPORT_REQUEST_SLOTS = QNFT_MAX_IMPORTS * 2;
constexpr uint32 QNFT_REQUEST_ID_LIMIT = 0xffffffffU;
// Approval hash-map capacity, including spare slots for efficient lookup.
constexpr uint32 QNFT_APPROVAL_SLOTS = 65536;
// QBay-to-item identity map capacity; entries are retained to prevent duplicate imports.
constexpr uint32 QNFT_IMPORT_SLOTS = QNFT_MAX_ITEMS * 2;
// Shop listing hash-map capacity, including unused and removed slots.
constexpr uint32 QNFT_LISTING_SLOTS = 65536;
// Default fee for creating one collection, in QU.
constexpr sint64 QNFT_COLLECTION_FEE = 1000000;
// Default fee for issuing one native asset, regardless of edition size, in QU.
constexpr sint64 QNFT_MINT_FEE = 100000;
// Maximum number of active escrow listings admitted to the shop.
constexpr uint32 QNFT_MAX_LISTINGS = 32768;
// Default shop commission on gross sale consideration, in basis points.
constexpr uint32 QNFT_SHOP_FEE_BPS = 200;
// Denominator representing 100 percent in basis-point calculations.
constexpr uint32 QNFT_BPS_DENOMINATOR = 10000;
// Basis points corresponding to one whole percentage point of a QBay royalty.
constexpr uint32 QNFT_BPS_PER_PERCENT = div(QNFT_BPS_DENOMINATOR, QNFT_PERCENT_DENOMINATOR);
// Fixed developer allocation of protocol revenue, in basis points; not governable.
constexpr uint32 QNFT_DEVELOPER_FEE_BPS = 2000;
// Default shareholder allocation of protocol revenue, in basis points.
constexpr uint32 QNFT_SHAREHOLDER_FEE_BPS = 6000;
// Default execution-reserve burn allocation of protocol revenue, in basis points.
constexpr uint32 QNFT_BURN_FEE_BPS = 2000;
// Maximum number of native QPI shareholder proposal slots.
constexpr uint16 QNFT_PROPOSAL_SLOTS = 8;
// Default fee for creating or replacing one scalar or batch proposal, in QU.
constexpr sint64 QNFT_PROPOSAL_FEE = 1000000;
// Default tariff for routine paid actions, in QU.
constexpr sint64 QNFT_ACTION_FEE = 10000;
// Default fee for updating mutable item metadata, in QU.
constexpr sint64 QNFT_METADATA_FEE = 50000;
// Default fee for the first reservation of a QBay original, in QU.
constexpr sint64 QNFT_IMPORT_FEE = 100000;
// Packed IPO share name used to identify QNFT shareholders for voting and dividends.
constexpr uint64 QNFT_SHARE_NAME = Ch::Q | (Ch::N << 8) | (Ch::F << 16) | (Ch::T << 24);

/**
 * @brief Reserved extension contract type.
 */
struct QNFT2
{
};

/**
 * @brief Native-asset NFT registry, QBay wrapper and fixed-price shop.
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
		CollectionFee,

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

		/** @brief Governed importFee charge in QU. */
		ImportFee,

		/** @brief Governed listingFee charge in QU. */
		ListingFee,

		/** @brief Governed proposalFee charge in QU. */
		ProposalFee
	};

	STATIC_ASSERT(QNFT_FEE_VARIABLE_COUNT == static_cast<uint32>(FeeVariable::ProposalFee) + 1, QNFT_FEE_VARIABLE_COUNT_MATCHES_IDS);
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
	 * @brief Canonical, hash-bound set of changes; unused entries and reserved bytes are zero.
	 * @note Only the populated prefix is applied; zeroed tail and reserved bytes make the hashed payload canonical.
	 */
	struct FeeProposalBatch
	{
		/** @brief At most one entry for each governed variable. */
		Array<FeeChange, QNFT_FEE_BATCH_CAPACITY> changes;

		/** @brief Number of populated entries, from 1 through 13. */
		uint32 count;

		/** @brief Must be zero, including in the hashed representation. */
		uint32 reserved;
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
	 * @brief Stable NFT command identifiers.
	 */
	enum class Operation : uint16
	{
		/** @brief Create a collection and charge the configured collection fee. */
		CreateCollection = 1,

		/** @brief Nominate the next collection administrator; current administrator only. */
		NominateAdmin,

		/** @brief Accept collection administration as the nominated account. */
		AcceptAdmin,

		/** @brief Issue a fixed edition and deliver it, preserving recoverable delivery failures. */
		Mint,

		/** @brief Transfer owned units or use a persistent item approval. */
		Transfer,

		/** @brief Authorize one operator for an item until explicit revocation or replacement. */
		Approve,

		/** @brief Revoke the invocator's approval without requiring initialized network time. */
		Revoke,

		/** @brief Update mutable metadata as its authorized editor. */
		UpdateMetadata,

		/** @brief Permanently freeze editable native-item metadata. */
		FreezeMetadata,

		/** @brief Reveal metadata matching the original commitment and freeze it. */
		RevealMetadata,

		/** @brief Release asset management to an accepting registered destination. */
		Export,

		/** @brief Burn native units without restoring collection issuance quota. */
		Burn,

		/** @brief Recover reserved mint delivery as its recorded recipient. */
		ClaimMint,

		/** @brief Reserve a QBay wrapper import for the selected beneficiary. */
		PrepareImport,

		/** @brief Delete an unissued import request and release its unused collection reservation. */
		CancelImport,

		/** @brief Finalize a deposited QBay wrapper import. */
		FinalizeImport,

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
	 * @brief Permitted metadata lifecycle.
	 */
	enum class Policy : uint8
	{
		/** @brief Metadata cannot be edited after creation. */
		Immutable,

		/** @brief Committed metadata may be revealed once. */
		Reveal,

		/** @brief Authorized editor may update metadata until frozen. */
		Mutable
	};

	/**
	 * @brief Source of the native asset representation.
	 */
	enum class Origin : uint8
	{
		/** @brief Originally issued by this contract. */
		Native,

		/** @brief Native wrapper backed by a QBay NFT. */
		Qbay
	};

	/**
	 * @brief QBay custody lifecycle of a wrapper item.
	 */
	enum class Bridge : uint8
	{
		/** @brief No QBay backing lifecycle applies. */
		None,

		/** @brief Issued wrapper awaiting delivery to the recorded recipient. */
		Requested,

		/** @brief Finalized one-way import; the original remains in QNFT custody. */
		Permanent
	};

	/** @brief Token log identifiers, disjoint from command and market actions. */
	enum class TokenEventType : uint32
	{
		/** @brief One ownership balance change, including issuance and burning. */
		TransferSingle = 0x10000,

		/** @brief Current metadata URI for an item. */
		URI = 0x10001
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

		/** @brief Account authorized to administer and mint this collection. */
		id administrator;

		/** @brief Proposed administrator who must accept the role. */
		id nominatedAdministrator;

		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Lifetime edition-unit issuance cap for the collection. */
		sint64 maxUnits;

		/** @brief Lifetime issued units; pending import reservations are counted separately. */
		sint64 issuedUnits;

		/** @brief Maximum number of distinct items allowed. */
		uint32 maxItems;

		/** @brief Lifetime issued items; pending import reservations are counted separately. */
		uint32 issuedItems;
	};

	/**
	 * @brief Canonical NFT metadata, fixed edition supply and optional QBay custody record.
	 * @note The asset identity is (QNFT issuer, assetName); metadata is shared by every unit of the edition.
	 * @note Issued item IDs are never recycled; import requests do not allocate item IDs.
	 */
	struct Item
	{
		/** @brief Original author recorded at creation; immutable. */
		id creator;

		/** @brief Account authorized to edit metadata while policy permits. */
		id editor;

		/** @brief Account credited with creator royalties on shop sales. */
		id royaltyReceiver;

		/** @brief Committed metadata digest checked during reveal. */
		id commitment;

		/** @brief Beneficiary of undelivered mint units or a pending bridge operation. */
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

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Metadata revision used to reject stale purchases. */
		uint32 metadataVersion;

		/** @brief Identifier of the underlying QBay NFT. */
		uint32 qbayId;

		/** @brief Creator royalty in basis points; 10000 means 100%. */
		uint16 royaltyBps;

		/** @brief Rules governing metadata changes and reveal. */
		Policy policy;

		/** @brief Native mint or QBay-backed provenance. */
		Origin origin;

		/** @brief Current state of the one-way QBay import. */
		Bridge bridge;

		/** @brief Nonzero once metadata changes are permanently disabled. */
		uint8 frozen;
	};

	/**
	 * @brief Canonical owner/item tuple hashed to address one allowance.
	 */
	struct ApprovalKey
	{
		/** @brief Account whose holdings, authorization or credit are addressed. */
		id owner;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint64 itemId;
	};

	/**
	 * @brief Persistent permission to transfer, burn or export locally managed units of one item.
	 */
	struct Approval
	{
		/** @brief Operator permitted to manage this owner/item pair until explicit revocation. */
		id operatorId;
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
		Cancel = 2,

		/** @brief Buy a protected-price quantity and pay seller proceeds and royalties immediately. */
		Buy = 3,

		/** @brief Allocate the end-of-epoch fee pool. */
		Distribution = 5
	};

	/** @brief Domain identifying the fields of a public journal record. */
	enum class EventKind : uint32
	{
		/** @brief Successful or pending NFT command. */
		Command = 1,
		/** @brief Shop action or epoch distribution. */
		Market = 2,
		/** @brief Successful governance submission, vote or application. */
		Governance = 3,
		/** @brief Actual asset ownership movement. */
		TransferSingle = 4,
		/** @brief Stored metadata URI announcement. */
		URI = 5
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

		/** @brief Must be zero. */
		uint32 reserved;
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

	/** @brief Unissued import reservation keyed by the original QBay NFT identifier. */
	struct ImportReservation
	{
		/** @brief Metadata captured from QBay when the owner prepares the import. */
		Metadata metadata;

		/** @brief Account that prepared the reservation. */
		id requester;

		/** @brief Fixed recipient of the native asset. */
		id recipient;

		/** @brief Original QBay author and royalty recipient. */
		id creator;

		/** @brief Existing destination collection. */
		uint32 collectionId;

		/** @brief Original QBay NFT identifier. */
		uint32 qbayId;

		/** @brief Monotonic reservation version returned by preparation. */
		uint32 requestId;

		/** @brief Original royalty converted to basis points. */
		uint16 royaltyBps;
	};

	/** @brief Public snapshot uses the same fields as the stored unissued reservation. */
	using ImportRequest = ImportReservation;

	/** @brief Collection whose pending import capacity is requested. */
	struct CountImportReservations_input
	{
		/** @brief Destination collection identifier. */
		uint32 collectionId;
	};

	/** @brief Each unissued request reserves one item and one unit. */
	struct CountImportReservations_output
	{
		/** @brief Number of pending requests for the collection. */
		uint32 count;
	};

	/** @brief Iterator over the bounded pending-request map. */
	struct CountImportReservations_locals
	{
		/** @brief Current occupied map slot. */
		sint64 index;
	};

	/** @brief Stored reservation used to assemble the public import snapshot. */
	struct GetImport_locals
	{
		/** @brief Unissued reservation data returned to the caller. */
		ImportReservation request;
	};

	/** @brief Look up a live import reservation independently of the item registry. */
	struct GetImport_input
	{
		/** @brief Original QBay NFT identifier. */
		uint32 qbayId;
	};

	/** @brief Reservation snapshot; NotFound for cancelled or issued requests. */
	struct GetImport_output
	{
		/** @brief Current unissued reservation and its version. */
		ImportRequest request;

		/** @brief Lookup result. */
		Code code;
	};

	/** @brief Page through unissued requests in ascending request-version order. */
	struct ListImportRequests_input
	{
		/** @brief Inclusive requestId lower bound; start at zero, then use nextOffset. */
		uint32 offset;

		/** @brief Maximum full records returned, from 1 to QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
	};

	/** @brief Bounded full request records from the current state. */
	struct ListImportRequests_output
	{
		/** @brief Requests sorted by requestId; only the first count entries are populated. */
		Array<ImportRequest, QNFT_REGISTRY_PAGE_CAPACITY> requests;

		/** @brief Number of returned requests. */
		uint32 count;

		/** @brief Next version lower bound, or QNFT_REQUEST_ID_LIMIT when no later request exists. */
		uint32 nextOffset;

		/** @brief Total live request population at this call, independently of the cursor. */
		uint32 totalCount;

		/** @brief Ok or InvalidInput for a limit outside the supported range. */
		Code code;
	};

	/** @brief Bounded map search used to return requests independently of hash slot positions. */
	struct ListImportRequests_locals
	{
		/** @brief Current occupied map slot. */
		sint64 index;

		/** @brief Slot containing the next request in version order. */
		sint64 selectedIndex;

		/** @brief Smallest eligible request version found in the current scan. */
		uint32 selectedVersion;
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

	/**
	 * @brief Persistent NFT registry, marketplace, accounting, and governance state.
	 * @note Field order is persistent-state ABI. Existing snapshots require explicit migration when offsets change.
	 * @note Deferred refunds and undistributed revenue are separate obligations backed by retained QU.
	 */
	struct StateData
	{
		// Fixed-capacity registries; resizing requires a fresh deployment or explicit state migration.

		/** @brief Item registry indexed by item ID. */
		Array<Item, QNFT_MAX_ITEMS> items;

		/** @brief Collection registry indexed by collection ID. */
		Array<Collection, QNFT_MAX_COLLECTIONS> collections;

		/** @brief Bounded allowances keyed by K12(owner, item ID). */
		HashMap<id, Approval, QNFT_APPROVAL_SLOTS> approvals;

		/** @brief QBay NFT ID to issued item ID; retained even while delivery is pending. */
		HashMap<uint32, uint32, QNFT_IMPORT_SLOTS> imports;

		/** @brief Number of allocated item registry entries. */
		uint32 itemCount;

		/** @brief Number of allocated collection registry entries. */
		uint32 collectionCount;

		// Failed transfers retain their collateral until automatic repayment succeeds.

		/** @brief Failed QU refunds keyed by the original recipient address. */
		HashMap<id, sint64, QNFT_CONTRACT_REFUND_SLOTS> contractRefunds;
		// Shop state retains only escrow listings and protocol revenue.

		/** @brief Active fixed-price listings keyed by unique listing ID. */
		HashMap<uint64, Listing, QNFT_LISTING_SLOTS> listings;

		/** @brief Deployment-configured developer beneficiary; zero defers payment. */
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

		/** @brief Governed Approve invocation fee in QU. */
		sint64 approveFee;

		/** @brief Governed UpdateMetadata invocation fee in QU. */
		sint64 updateMetadataFee;

		/** @brief Governed Export invocation fee in QU. */
		sint64 exportFee;

		/** @brief Governed Import invocation fee in QU. */
		sint64 importFee;

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
		/** @brief Unissued requests keyed by the original QBay NFT ID. */
		HashMap<uint32, ImportReservation, QNFT_IMPORT_REQUEST_SLOTS> importRequests;

		/** @brief Next request ID; cancellation never rewinds this counter. */
		uint32 nextImportRequestId;

		/** @brief Issued imports awaiting delivery; each item is enqueued at most once. */
		LinkedList<uint32, QNFT_MAX_ITEMS> importDeliveries;

		/** @brief Next refund map slot to inspect; rotating cursor prevents starvation. */
		uint32 refundCursor;
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

		/** @brief Current importFee in QU. */
		sint64 importFee;

		/** @brief Current listingFee in QU. */
		sint64 listingFee;

		/** @brief Current proposalFee in QU. */
		sint64 proposalFee;

		/** @brief Percentage of gross sale consideration, in basis points. */
		uint32 shopFeeBPS;
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

		/** @brief Lifetime edition-unit issuance cap for the collection. */
		sint64 maxUnits;

		/** @brief Maximum number of distinct items allowed. */
		uint32 maxItems;
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

		/** @brief Account authorized to edit metadata while policy permits. */
		id editor;

		/** @brief Account credited with creator royalties on shop sales. */
		id royaltyReceiver;

		/** @brief Committed metadata digest checked during reveal. */
		id commitment;

		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Number of native asset units requested or returned. */
		sint64 quantity;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Creator royalty in basis points; 10000 means 100%. */
		uint16 royaltyBps;

		/** @brief Rules governing metadata changes and reveal. */
		Policy policy;
	};

	/** @brief Reuse the shared OperationResult layout for Mint_output. */
	using Mint_output = OperationResult;

	/**
	 * @brief Arguments supplied to Transfer.
	 */
	struct Transfer_input
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

	/** @brief Reuse the shared OperationResult layout for Transfer_output. */
	using Transfer_output = OperationResult;

	/**
	 * @brief Arguments supplied to Approve; the owner is the invocator.
	 */
	struct Approve_input
	{
		/** @brief Operator authorized for this item until replaced or explicitly revoked. */
		id receiver;

		/** @brief Zero-based item identifier covered by this permission. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for Approve_output. */
	using Approve_output = OperationResult;

	/**
	 * @brief Arguments supplied to Revoke; the owner is the invocator.
	 */
	struct Revoke_input
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for Revoke_output. */
	using Revoke_output = OperationResult;

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
	 * @brief Arguments supplied to RevealMetadata.
	 */
	struct RevealMetadata_input
	{
		/** @brief URI and content digest for this object. */
		Metadata metadata;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for RevealMetadata_output. */
	using RevealMetadata_output = OperationResult;

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

	/**
	 * @brief Arguments supplied to ClaimMint.
	 */
	struct ClaimMint_input
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/** @brief Reuse the shared OperationResult layout for ClaimMint_output. */
	using ClaimMint_output = OperationResult;

	/**
	 * @brief Arguments supplied to PrepareImport.
	 */
	struct PrepareImport_input
	{
		/** @brief Destination account for this operation. */
		id receiver;

		/** @brief Zero-based collection identifier. */
		uint32 collectionId;

		/** @brief Identifier of the underlying QBay NFT. */
		uint32 qbayId;
	};

	/** @brief Newly created reservation, separate from the NFT item registry. */
	struct PrepareImport_output
	{
		/** @brief Unique request identifier, meaningful on success. */
		uint32 requestId;

		/** @brief Result or rejection reason. */
		Code code;
	};

	/**
	 * @brief Arguments supplied to CancelImport.
	 */
	struct CancelImport_input
	{
		/** @brief Original QBay NFT identifier used as the request key. */
		uint32 qbayId;

		/** @brief Request identifier returned by PrepareImport. */
		uint32 requestId;
	};

	/** @brief Reuse the shared OperationResult layout for CancelImport_output. */
	using CancelImport_output = OperationResult;

	/**
	 * @brief Arguments supplied to FinalizeImport.
	 */
	struct FinalizeImport_input
	{
		/** @brief Original QBay NFT identifier used as the request key. */
		uint32 qbayId;

		/** @brief Request identifier returned by PrepareImport. */
		uint32 requestId;
	};

	/** @brief Reuse the shared OperationResult layout for FinalizeImport_output. */
	using FinalizeImport_output = OperationResult;

	/** @brief Reuse the shared NoData layout for FundReserve_input. */
	using FundReserve_input = NoData;

	/** @brief Reuse the shared OperationResult layout for FundReserve_output. */
	using FundReserve_output = OperationResult;

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

		/** @brief Reserved ABI field; must be zero. */
		uint16 reserved;
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

	/**
	 * @brief Arguments supplied to GetApproval.
	 */
	struct GetApproval_input
	{
		/** @brief Account whose holdings, authorization or credit are addressed. */
		id owner;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/**
	 * @brief Result returned by GetApproval.
	 */
	struct GetApproval_output
	{
		/** @brief Working copy or returned approval record. */
		Approval approval;

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

	/** @brief Address whose pending automatic refund is requested. */
	struct GetRefund_input
	{
		/** @brief Original recipient of the deferred refund. */
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

	/** @brief Page through pending refunds by physical map-slot cursor. */
	struct ListRefunds_input
	{
		/** @brief First map slot to inspect; capacity denotes the terminal page. */
		uint32 offset;

		/** @brief Maximum returned records, from 1 to QNFT_REGISTRY_PAGE_CAPACITY. */
		uint32 limit;
	};

	/** @brief Bounded list of positive pending refunds. */
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

		/** @brief Maximum number of distinct items allowed. */
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

		/** @brief Reserved ABI field; must be zero. */
		uint32 reserved;
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

		/** @brief Reserved ABI field; must be zero. */
		uint32 reserved;
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

		/** @brief Result or rejection reason from the Code enum. */
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
		/** @brief Deployment-configured developer beneficiary; zero defers payment. */
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
	 * @brief Successful shop action log; preserve the existing wire field order.
	 */
	struct MarketEvent
	{
		// The logger requires the contract/type prefix before IDs; this is wire ABI.

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
	 * @brief Successful or pending NFT command log; preserve the existing wire field order.
	 */
	struct Event
	{
		// The core logger requires contractIndex first; preserve this wire layout.

		/** @brief Required first log field identifying QNFT. */
		uint32 contractIndex;

		/** @brief NFT operation recorded by this event. */
		Operation type;

		/** @brief Preserve the 32-bit event operation slot on the wire. */
		uint16 typeReserved;

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
		Market = 2,

		/** @brief Incoming asset management callback. */
		Management = 3
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

		/** @brief Reserved ABI field; must be zero. */
		uint16 reserved;

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
	using isOwnerOrApprovedOperator_input = ApprovalKey;

	/** @brief Result of the owner or persistent item-operator check. */
	struct isOwnerOrApprovedOperator_output
	{
		/** @brief True when the invocator may act for this owner and item. */
		bool authorized;
	};

	/** @brief Temporary data for the read-only authorization lookup. */
	struct isOwnerOrApprovedOperator_locals
	{
		/** @brief Registered operator for the owner and item. */
		Approval approval;

		/** @brief Hash of the owner and item pair. */
		id key;
	};

	/**
	 * @brief Arguments supplied to CheckMetadata.
	 */
	struct CheckMetadata_input
	{
		/** @brief URI and content digest for this object. */
		Metadata metadata;
	};

	/**
	 * @brief Result returned by CheckMetadata.
	 */
	struct CheckMetadata_output
	{
		/** @brief True only when all validation checks succeed. */
		bool valid;
	};

	/**
	 * @brief Zero-initialized per-call working data for CheckMetadata.
	 */
	struct CheckMetadata_locals
	{
		/** @brief Bounded loop or pagination cursor. */
		uint32 i;

		/** @brief Whether a zero terminator has already been encountered. */
		bool terminated;
	};

	/**
	 * @brief Arguments supplied to Name.
	 */
	struct Name_input
	{
		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;
	};

	/**
	 * @brief Result returned by Name.
	 */
	struct Name_output
	{
		/** @brief Packed deterministic asset name. */
		uint64 name;
	};

	/**
	 * @brief Zero-initialized per-call working data for Name.
	 */
	struct Name_locals
	{
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
		/** @brief Decoded ID to re-encode using the issuance name generator. */
		Name_input nameInput;

		/** @brief Canonical name generated by the same function used when minting. */
		Name_output nameOutput;

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

	/** @brief Validated reservation ready for insertion under its QBay ID. */
	using CommitNewImportRequest_input = ImportReservation;

	/** @brief Newly assigned monotonic request version. */
	using CommitNewImportRequest_output = uint32;

	/** @brief Listing whose asset units have already entered shop escrow. */
	using CommitNewListing_input = Listing;

	/** @brief Newly assigned monotonic listing identifier. */
	using CommitNewListing_output = uint64;

	/** @brief Validated document update and the existing item it replaces. */
	struct CommitMetadataUpdate_input
	{
		/** @brief Updated item snapshot, including any reveal freeze. */
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

		/** @brief Metadata supplied for validation. */
		CheckMetadata_input metadataInput;

		/** @brief Whether the collection metadata is valid. */
		CheckMetadata_output metadataOutput;
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

		/** @brief Collection selected for capacity reservation counting. */
		CountImportReservations_input reservationsInput;

		/** @brief Pending items and units reserved for this collection. */
		CountImportReservations_output reservationsOutput;

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

		/** @brief Arguments for Name. */
		Name_input nameInput;

		/** @brief Result from Name. */
		Name_output nameOutput;

		/** @brief Arguments for CheckMetadata. */
		CheckMetadata_input metadataInput;

		/** @brief Result from CheckMetadata. */
		CheckMetadata_output metadataOutput;
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
	using ExecuteTransfer_input = Transfer_input;

	/** @brief Return the dedicated public result for Transfer. */
	using ExecuteTransfer_output = Transfer_output;

	/**
	 * @brief Zero-initialized working data for ExecuteTransfer.
	 */
	struct ExecuteTransfer_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Owner and item passed to the authorization helper. */
		isOwnerOrApprovedOperator_input authorizationInput;

		/** @brief Whether the invocator is authorized for this operation. */
		isOwnerOrApprovedOperator_output authorizationOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Available units or QU credit before the operation. */
		sint64 balance;
	};

	/**
	 * @brief Zero-initialized per-call working data for Transfer.
	 */
	struct Transfer_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for Approve. */
	using ExecuteApprove_input = Approve_input;

	/** @brief Return the dedicated public result for Approve. */
	using ExecuteApprove_output = Approve_output;

	/**
	 * @brief Zero-initialized working data for ExecuteApprove.
	 */
	struct ExecuteApprove_locals
	{
		/** @brief Working copy or returned approval record. */
		Approval approval;

		/** @brief Owner/item pair to hash for approval lookup. */
		ApprovalKey keyInput;

		/** @brief Approval lookup key or its unhashed owner/item pair. */
		id key;
	};

	/**
	 * @brief Zero-initialized per-call working data for Approve.
	 */
	struct Approve_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for Revoke. */
	using ExecuteRevoke_input = Revoke_input;

	/** @brief Return the dedicated public result for Revoke. */
	using ExecuteRevoke_output = Revoke_output;

	/**
	 * @brief Zero-initialized working data for ExecuteRevoke.
	 */
	struct ExecuteRevoke_locals
	{
		/** @brief Owner/item pair to hash for approval lookup. */
		ApprovalKey keyInput;

		/** @brief Approval lookup key or its unhashed owner/item pair. */
		id key;
	};

	/**
	 * @brief Zero-initialized per-call working data for Revoke.
	 */
	struct Revoke_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
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

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Arguments for CheckMetadata. */
		CheckMetadata_input metadataInput;

		/** @brief Result from CheckMetadata. */
		CheckMetadata_output metadataOutput;
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
		/** @brief Working copy or returned item record. */
		Item item;
	};

	/**
	 * @brief Zero-initialized per-call working data for FreezeMetadata.
	 */
	struct FreezeMetadata_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for RevealMetadata. */
	using ExecuteRevealMetadata_input = RevealMetadata_input;

	/** @brief Return the dedicated public result for RevealMetadata. */
	using ExecuteRevealMetadata_output = RevealMetadata_output;

	/**
	 * @brief Zero-initialized working data for ExecuteRevealMetadata.
	 */
	struct ExecuteRevealMetadata_locals
	{
		/** @brief Data for the centralized registry commit. */
		CommitMetadataUpdate_input commitInput;

		/** @brief Empty metadata commit result. */
		CommitMetadataUpdate_output commitOutput;

		/** @brief Current URI to announce after committing metadata. */
		EmitUri_input uriEvent;

		/** @brief Empty URI logging result. */
		EmitUri_output uriLogOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Arguments for CheckMetadata. */
		CheckMetadata_input metadataInput;

		/** @brief Result from CheckMetadata. */
		CheckMetadata_output metadataOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for RevealMetadata.
	 */
	struct RevealMetadata_locals
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

		/** @brief Specialized post-callback refund and logging already ran. */
		bool settlementCompleted;
	};

	/**
	 * @brief Zero-initialized working data for ExecuteExport.
	 */
	struct ExecuteExport_locals
	{
		/** @brief Earned QU passed to centralized protocol revenue accrual. */
		AccrueProtocolFee_input accrualInput;

		/** @brief Empty result from centralized protocol revenue accrual. */
		AccrueProtocolFee_output accrualOutput;

		/** @brief Owner and item passed to the authorization helper. */
		isOwnerOrApprovedOperator_input authorizationInput;

		/** @brief Whether the invocator is authorized for this operation. */
		isOwnerOrApprovedOperator_output authorizationOutput;

		/** @brief Rejected export diagnostic. */
		LogFailure_input failureInput;

		/** @brief Empty logging result. */
		LogFailure_output failureOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Native ledger asset identified by issuer and name. */
		Asset asset;

		/** @brief Log payload assembled for this execution. */
		EmitCommand_input event;

		/** @brief Empty information-log result. */
		EmitCommand_output eventOutput;

		/** @brief QNFT entity balance before recipient management callbacks. */
		Entity beforeExport;

		/** @brief QNFT entity balance after recipient management callbacks. */
		Entity afterExport;

		/** @brief Net QU spent during export plus the QNFT fee charged on success. */
		sint64 spentAmount;

		/** @brief Whether management rights were released successfully. */
		bool exportSucceeded;

		/** @brief Arguments for Refund. */
		Refund_input refundInput;

		/** @brief Result from Refund. */
		Refund_output refundOutput;
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
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Data for the centralized registry commit. */
		CommitBurn_input commitInput;

		/** @brief Empty burn commit result. */
		CommitBurn_output commitOutput;

		/** @brief Owner and item passed to the authorization helper. */
		isOwnerOrApprovedOperator_input authorizationInput;

		/** @brief Whether the invocator is authorized for this operation. */
		isOwnerOrApprovedOperator_output authorizationOutput;

		/** @brief Working copy or returned item record. */
		Item item;

		/** @brief Available units or QU credit before the operation. */
		sint64 balance;
	};

	/**
	 * @brief Zero-initialized per-call working data for Burn.
	 */
	struct Burn_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for ClaimMint. */
	using ExecuteClaimMint_input = ClaimMint_input;

	/** @brief Return the dedicated public result for ClaimMint. */
	using ExecuteClaimMint_output = ClaimMint_output;

	/**
	 * @brief Zero-initialized working data for ExecuteClaimMint.
	 */
	struct ExecuteClaimMint_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Working copy or returned item record. */
		Item item;
	};

	/**
	 * @brief Zero-initialized per-call working data for ClaimMint.
	 */
	struct ClaimMint_locals
	{
		/** @brief Arguments for FinishCommand. */
		FinishCommand_input finishInput;
	};

	/** @brief Use the dedicated public arguments for PrepareImport. */
	using ExecutePrepareImport_input = PrepareImport_input;

	/**
	 * @brief Result and settlement details returned by ExecutePrepareImport.
	 */
	struct ExecutePrepareImport_output
	{
		/** @brief Internal settlement result; itemId carries the new request ID. */
		OperationResult result;

		/** @brief QU retained or burned after successful allocation. */
		sint64 fee;
	};

	/**
	 * @brief Zero-initialized working data for ExecutePrepareImport.
	 */
	struct ExecutePrepareImport_locals
	{
		/** @brief Data for the centralized registry commit. */
		CommitNewImportRequest_output committedRequest;

		/** @brief Collection selected for capacity reservation counting. */
		CountImportReservations_input reservationsInput;

		/** @brief Pending items and units reserved for this collection. */
		CountImportReservations_output reservationsOutput;

		/** @brief Reservation being prepared. */
		ImportReservation request;

		/** @brief Working copy or returned collection record. */
		Collection collection;

		/** @brief Arguments for QBAY::getInfoOfMarketplace. */
		QBAY::getInfoOfMarketplace_input marketInput;

		/** @brief Result from QBAY::getInfoOfMarketplace. */
		QBAY::getInfoOfMarketplace_output marketOutput;

		/** @brief Arguments for QBAY::getInfoOfNFTById. */
		QBAY::getInfoOfNFTById_input originalInput;

		/** @brief QBay response describing the underlying NFT. */
		QBAY::getInfoOfNFTById_output original;
	};

	/**
	 * @brief Zero-initialized per-call working data for PrepareImport.
	 */
	struct PrepareImport_locals
	{
		/** @brief Shared logging and refund arguments. */
		FinishCommand_input finishInput;

		/** @brief Operation result and settlement details. */
		ExecutePrepareImport_output operationOutput;

		/** @brief Internal command settlement result. */
		OperationResult settled;
	};

	/** @brief Use the dedicated public arguments for CancelImport. */
	using ExecuteCancelImport_input = CancelImport_input;

	/** @brief Return the dedicated public result for CancelImport. */
	using ExecuteCancelImport_output = CancelImport_output;

	/**
	 * @brief Zero-initialized working data for ExecuteCancelImport.
	 */
	struct ExecuteCancelImport_locals
	{
		/** @brief Reservation being cancelled. */
		ImportReservation request;

		/** @brief Arguments for QBAY::getInfoOfNFTById. */
		QBAY::getInfoOfNFTById_input originalInput;

		/** @brief QBay response describing the underlying NFT. */
		QBAY::getInfoOfNFTById_output original;
	};

	/**
	 * @brief Zero-initialized per-call working data for CancelImport.
	 */
	struct CancelImport_locals
	{
		/** @brief Shared logging and refund arguments. */
		FinishCommand_input finishInput;
	};

	/** @brief Issued item whose reserved wrapper must reach its recorded beneficiary. */
	using DeliverImport_input = uint32;

	/** @brief Delivery status and issued identity; no issuance occurs here. */
	using DeliverImport_output = OperationResult;

	/** @brief Working data shared by public finalization and automatic delivery. */
	struct DeliverImport_locals
	{
		/** @brief Validated movement applied and announced as one operation. */
		TransferAsset_input transferInput;

		/** @brief Native transfer result; negative means no movement and no event. */
		TransferAsset_output transferResult;

		/** @brief Existing issued item with its fixed recipient. */
		Item item;
	};

	/** @brief Empty request; pending deliveries are read from the persistent queue. */
	using RetryImportDeliveries_input = NoData;

	/** @brief Empty result; delivery outcomes are persisted in items and events. */
	using RetryImportDeliveries_output = NoData;

	/** @brief Bounded FIFO delivery processing without user rewards or refunds. */
	struct RetryImportDeliveries_locals
	{
		/** @brief Delivery result; Pending entries return to the queue tail. */
		DeliverImport_output delivery;

		/** @brief Current queue node, removed before attempting delivery. */
		sint64 queueIndex;

		/** @brief Initial queue population capped by the per-tick work limit. */
		uint64 remaining;

		/** @brief Existing item ID taken from the queue head. */
		DeliverImport_input itemId;
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

	/** @brief Tick hook arguments for automatic import delivery and refund repayment. */
	struct END_TICK_locals
	{
		/** @brief Empty request for processing the delivery queue. */
		RetryImportDeliveries_input retryInput;

		/** @brief Empty result of queue processing. */
		RetryImportDeliveries_output retryOutput;

		/** @brief Empty request for automatic refund repayment. */
		RetryRefunds_input refundInput;

		/** @brief Empty result of refund repayment. */
		RetryRefunds_output refundOutput;
	};

	/** @brief Use the dedicated public arguments for FinalizeImport. */
	using ExecuteFinalizeImport_input = FinalizeImport_input;

	/** @brief Return the dedicated public result for FinalizeImport. */
	using ExecuteFinalizeImport_output = FinalizeImport_output;

	/**
	 * @brief Zero-initialized working data for ExecuteFinalizeImport.
	 */
	struct ExecuteFinalizeImport_locals
	{
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

		/** @brief Arguments for QBAY::getInfoOfNFTById. */
		QBAY::getInfoOfNFTById_input originalInput;

		/** @brief QBay response describing the underlying NFT. */
		QBAY::getInfoOfNFTById_output original;

		/** @brief Zero-based item identifier; meaningful in item operations. */
		uint32 itemId;

		/** @brief Unissued reservation used to construct the item. */
		ImportReservation request;

		/** @brief Arguments for deterministic asset naming. */
		Name_input nameInput;

		/** @brief Packed asset name for the new item. */
		Name_output nameOutput;
	};

	/**
	 * @brief Zero-initialized per-call working data for FinalizeImport.
	 */
	struct FinalizeImport_locals
	{
		/** @brief Shared logging and refund arguments. */
		FinishCommand_input finishInput;
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
	 * @brief Zero-initialized per-call working data for GetApproval.
	 */
	struct GetApproval_locals
	{
		/** @brief Approval lookup key or its unhashed owner/item pair. */
		ApprovalKey key;
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

	/** @brief Use the dedicated public arguments for Buy. */
	using ExecuteBuy_input = Buy_input;

	/** @brief Return purchase amounts and the event to emit during shared settlement. */
	struct ExecuteBuy_output
	{
		/** @brief Purchase event emitted only by the public settlement step. */
		MarketEvent event;

		/** @brief Purchase price, fee, royalty and status. */
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

		/** @brief Native QU balance checked before delivering escrow. */
		Entity contractBalance;

		/** @brief Net seller proceeds in QU after fee and royalty. */
		sint64 sellerAmount;
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

		/** @brief Must be zero. */
		uint32 reserved;
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
		bool valid; /* True only when every entry and the combined split are valid. */
	};

	/**
	 * @brief Validation scratch values; no contract-local stack variables.
	 */
	struct ValidateFeeBatch_locals
	{
		/** @brief Current entry index. */
		uint32 i;

		/** @brief Bit mask used to reject duplicate variables. */
		uint32 seen;

		/** @brief Proposed shareholder BPS when present. */
		sint64 shareholder;

		/** @brief Proposed burn BPS when present. */
		sint64 burn;
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
	};

	/** @brief Validation and mutation working data for SetFeeProposalBatch. */
	struct ExecuteSetFeeProposalBatch_locals
	{
		/** @brief Native proposal carrying the batch digest in its 32-byte payload. */
		ProposalDataT proposal;

		/** @brief Canonical hashed sidecar. */
		FeeProposalBatch batch;

		/** @brief Full validation before native proposal replacement. */
		ValidateFeeBatch_output validation;
	};

	/** @brief Public governance settlement data. */
	struct SetFeeProposalBatch_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetFeeProposalBatch_output operationOutput;

		/** @brief Earned QU passed to centralized protocol revenue accrual. */
		AccrueProtocolFee_input accrualInput;

		/** @brief Empty result from centralized protocol revenue accrual. */
		AccrueProtocolFee_output accrualOutput;

		/** @brief Excess or rejected invocation reward. */
		Refund_input refundInput;

		/** @brief Empty refund result. */
		Refund_output refundOutput;

		/** @brief Defined below; submission diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;

		/** @brief Entry index used for successful per-variable logs. */
		uint32 i;
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
	struct ExecuteSetShareholderProposal_locals
	{
		FeeProposalBatch emptyBatch;
	};

	/** @brief Native result and governance diagnostic for public settlement. */
	struct ExecuteSetShareholderProposal_output
	{
		/** @brief Acceptance or rejection event. */
		EmitGovernance_input event;

		/** @brief Native proposal index or ballot acceptance result. */
		SetShareholderProposal_output result;
	};

	/** @brief Public governance settlement data. */
	struct SetShareholderProposal_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetShareholderProposal_output operationOutput;

		/** @brief Earned QU passed to centralized protocol revenue accrual. */
		AccrueProtocolFee_input accrualInput;

		/** @brief Empty result from centralized protocol revenue accrual. */
		AccrueProtocolFee_output accrualOutput;

		/** @brief Return any attached QU beyond the proposal fee. */
		Refund_input refundInput;

		/** @brief Empty refund result. */
		Refund_output refundOutput;

		/** @brief Proposal submission or rejection diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;
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
	};

	/** @brief Validation and mutation working data for SetShareholderVotes. */
	struct ExecuteSetShareholderVotes_locals
	{
		/** @brief Index in the eight-entry ballot. */
		uint32 i;

		/** @brief Bounded total requested weight before handing input to QPI. */
		uint32 voteCount;
	};

	/** @brief Public governance settlement data. */
	struct SetShareholderVotes_locals
	{
		/** @brief Native execution result and diagnostic. */
		ExecuteSetShareholderVotes_output operationOutput;

		/** @brief Votes are free; refund the full invocation reward. */
		Refund_input refundInput;

		/** @brief Empty refund result. */
		Refund_output refundOutput;

		/** @brief Vote acceptance or rejection diagnostic. */
		EmitGovernance_input event;

		/** @brief Empty information-log result. */
		EmitGovernance_output eventOutput;
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

	/** @brief Authorize the owner or its registered item operator without modifying permission. */
	PRIVATE_FUNCTION_WITH_LOCALS(isOwnerOrApprovedOperator)
	{
		output.authorized = qpi.invocator() == input.owner;
		if (output.authorized)
		{
			return;
		}

		// Owners need no approval; other callers must match the registered item operator.
		locals.key = qpi.K12(input);
		output.authorized = state.get().approvals.get(locals.key, locals.approval) && locals.approval.operatorId == qpi.invocator();
	}

	/** @brief Register a validated new collection and advance its allocation counter together. */
	PRIVATE_PROCEDURE(CommitNewCollection)
	{
		// Capacity was checked before this commit; updates to existing collections do not allocate IDs.
		output = state.get().collectionCount;
		state.mut().collections.set(output, input);
		++state.mut().collectionCount;
	}

	/** @brief Store a validated import request and consume exactly one request version. */
	PRIVATE_PROCEDURE(CommitNewImportRequest)
	{
		// The caller has checked uniqueness, map capacity and version exhaustion.
		input.requestId = state.get().nextImportRequestId;
		state.mut().importRequests.set(input.qbayId, input);
		output = input.requestId;
		++state.mut().nextImportRequestId;
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
		// Both mutable updates and reveals preflight revision exhaustion before reaching this commit.
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
		if (input.listing.quantity)
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
		// Callers pass only earned, nonnegative QU backed by the invocation payment.
		state.mut().epochFees += input;
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

	/** @brief Expose the retained interval without log-reader credentials. */
	PUBLIC_FUNCTION(GetEventRange)
	{
		// The retained window is shorter than capacity until the ring fills for the first time.
		output.nextEventId = state.get().lastEventId + 1;
		output.count = static_cast<uint32>(RL::min(state.get().lastEventId, static_cast<uint64>(QNFT_EVENT_CAPACITY)));
		output.firstEventId = output.nextEventId - output.count;
		output.capacity = QNFT_EVENT_CAPACITY;
	}

	/** @brief Read a bounded chronological page and explicitly reject lost history. */
	PUBLIC_FUNCTION(GetEvents)
	{
		// Resolve the requested cursor against the currently retained interval before reading ring slots.
		output.nextEventId = state.get().lastEventId + 1;
		output.firstEventId = output.nextEventId - RL::min(state.get().lastEventId, static_cast<uint64>(QNFT_EVENT_CAPACITY));
		output.cursor = input.eventId ? input.eventId : output.firstEventId;
		if (!isInRange<uint32>(input.limit, 1, QNFT_EVENT_PAGE_CAPACITY) || input.reserved || output.cursor > output.nextEventId)
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
		// Resolve the asset from the same item ID that will be recorded in the event.
		output = qpi.transferShareOwnershipAndPossession(state.get().items.get(input.itemId).assetName, SELF, input.from, input.from, input.quantity,
		                                                 input.to);
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
	 * @brief Validate printable, nonempty, zero-padded metadata URIs.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CheckMetadata)
	{
		// Reject empty text and malformed bytes as soon as they are encountered.
		output.valid = false;
		if (input.metadata.uri.get(0) == Ch::null)
		{
			return;
		}

		for (locals.i = 0; locals.i < QNFT_METADATA_URI_CAPACITY; ++locals.i)
		{
			if (input.metadata.uri.get(locals.i) == Ch::null)
			{
				locals.terminated = true;
				continue;
			}

			if (locals.terminated ||
			    !isInRange<uint8>(input.metadata.uri.get(locals.i), QNFT_METADATA_URI_MIN_CHARACTER, QNFT_METADATA_URI_MAX_CHARACTER))
			{
				return;
			}
		}

		output.valid = locals.terminated;
	}

	/**
	 * @brief Encode an item ID using the shared asset-name format.
	 * @note The same encoder is used by Lookup to validate the prefix, digit representation and unused name bytes.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(Name)
	{
		// Pack the fixed-width base-36 ID after the shared prefix, retaining leading zero digits.
		output.name = QNFT_ASSET_NAME_PREFIX;
		locals.value = input.itemId;
		for (locals.i = QNFT_ASSET_NAME_DIGITS; locals.i > 0; --locals.i)
		{
			locals.digit = mod(locals.value, QNFT_ASSET_NAME_RADIX);
			locals.value = QPI::div(locals.value, QNFT_ASSET_NAME_RADIX);
			output.name |= static_cast<uint64>(locals.digit < QNFT_ASSET_NAME_DECIMAL_DIGITS ? Ch::_0 + locals.digit
			                                                                                 : Ch::A + locals.digit - QNFT_ASSET_NAME_DECIMAL_DIGITS)
			               << (QNFT_ASSET_NAME_BYTE_BITS * locals.i);
		}
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
		locals.nameInput.itemId = output.itemId;
		CALL(Name, locals.nameInput, locals.nameOutput);
		if (input.asset.assetName != locals.nameOutput.name)
		{
			return;
		}

		// A well-formed name must also refer to an allocated item with exactly this stored asset name.
		// The caller may use itemId only when valid is true; ownership and balances are checked separately.
		output.valid = output.itemId < state.get().itemCount && state.get().items.get(output.itemId).assetName == input.asset.assetName;
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

		// Shop payments include seller proceeds and royalties; only fee is protocol revenue.
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
			// Collection and import-request identifiers must not be interpreted as NFT item IDs.
			if (input.operation > Operation::AcceptAdmin && input.operation != Operation::FundReserve &&
			    input.operation != Operation::PrepareImport && input.operation != Operation::CancelImport && output.itemId < state.get().itemCount)
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

	/** @brief Test an inclusive range; reversed bounds match no value. */
	template<typename T>
	static constexpr bool isInRange(T value, T minimum, T maximum)
	{
		// Use one comparison type for the value and both bounds; no subtraction can overflow.
		return value >= minimum && value <= maximum;
	}

	/**
	 * @brief Validate collection parameters and allocate a record; leave fee settlement to the caller.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCreateCollection)
	{
		// Validate limits, metadata, storage capacity and payment before consuming a collection ID.
		if (!isInRange<uint32>(input.maxItems, 1, QNFT_MAX_ITEMS) || !isInRange<sint64>(input.maxUnits, 1, MAX_AMOUNT))
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.metadataInput.metadata = input.metadata;
		CALL(CheckMetadata, locals.metadataInput, locals.metadataOutput);
		if (!locals.metadataOutput.valid)
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

		// All rejection paths are complete; allocate the next collection and assign its initial administrator.
		locals.collection.creator = qpi.invocator();
		locals.collection.administrator = qpi.invocator();
		locals.collection.metadata = input.metadata;
		locals.collection.maxItems = input.maxItems;
		locals.collection.maxUnits = input.maxUnits;
		CALL(CommitNewCollection, locals.collection, locals.committedCollection);
		output.itemId = locals.committedCollection;
		output.code = Code::Ok;
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
	 * @brief Nominate the next collection administrator; current administrator only.
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
		if (qpi.invocator() != locals.collection.administrator)
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
	 * @brief Nominate the next collection administrator; current administrator only.
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
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteAcceptAdmin)
	{
		// Only the recorded nominee can consume the nomination and become administrator.
		if (input.collectionId >= state.get().collectionCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (qpi.invocator() != locals.collection.nominatedAdministrator)
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

	/** @brief Count reserved collection capacity without maintaining duplicate counters. */
	PRIVATE_FUNCTION_WITH_LOCALS(CountImportReservations)
	{
		// Only unissued requests remain here, so each matching entry reserves one item and one unit.
		for (locals.index = state.get().importRequests.nextElementIndex(NULL_INDEX); locals.index != NULL_INDEX;
		     locals.index = state.get().importRequests.nextElementIndex(locals.index))
		{
			if (state.get().importRequests.value(locals.index).collectionId == input.collectionId)
			{
				++output.count;
			}
		}
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
		// Validate the complete edition and its collection before making an irreversible core issuance call.

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || input.receiver == NULL_ID || qpi.isContractId(input.receiver) ||
		    input.metadata.contentHash == NULL_ID || input.policy > Policy::Mutable || input.royaltyBps > QNFT_BPS_DENOMINATOR ||
		    (input.policy == Policy::Reveal && input.commitment == NULL_ID) || (input.policy == Policy::Mutable && input.editor == NULL_ID))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (qpi.invocationReward() < state.get().mintFee)
		{
			output.result.code = Code::InsufficientFee;
			return;
		}

		locals.metadataInput.metadata = input.metadata;
		CALL(CheckMetadata, locals.metadataInput, locals.metadataOutput);
		if (!locals.metadataOutput.valid)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (input.collectionId >= state.get().collectionCount)
		{
			output.result.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (qpi.invocator() != locals.collection.administrator)
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		locals.reservationsInput.collectionId = input.collectionId;
		CALL(CountImportReservations, locals.reservationsInput, locals.reservationsOutput);

		// Honor both global import reservations and the collection's cumulative issuance limits.
		if (state.get().itemCount + state.get().importRequests.population() >= QNFT_MAX_ITEMS ||
		    locals.collection.issuedItems + locals.reservationsOutput.count >= locals.collection.maxItems ||
		    input.quantity > locals.collection.maxUnits - locals.collection.issuedUnits - locals.reservationsOutput.count)
		{
			output.result.code = Code::Capacity;
			return;
		}

		output.result.itemId = state.get().itemCount;
		locals.nameInput.itemId = output.result.itemId;
		CALL(Name, locals.nameInput, locals.nameOutput);
		if (qpi.issueAsset(locals.nameOutput.name, SELF, 0, input.quantity, 0) != input.quantity)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		// Persist issuance before attempting delivery: core does not roll back prior calls.
		locals.item.assetName = locals.nameOutput.name;
		locals.item.creator = qpi.invocator();
		locals.item.editor = input.editor;
		locals.item.royaltyReceiver = input.royaltyReceiver;
		locals.item.royaltyBps = input.royaltyBps;
		locals.item.commitment = input.commitment;
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
			output.result.code = Code::Pending;
			return;
		}

		locals.item.reserved = 0;
		locals.item.reservedRecipient = NULL_ID;
		state.mut().items.set(output.result.itemId, locals.item);
		output.result.code = Code::Ok;
	}

	/**
	 * @brief Issue a fixed edition and deliver it, preserving recoverable delivery failures.
	 * @note Charges once per issued asset, not per unit; failed delivery remains recoverable through ClaimMint.
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
	 * @brief Transfer owned units or use a persistent item approval.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteTransfer)
	{
		// Validate payment, participants and lifecycle before checking the selected owner's permission and holdings.
		if (qpi.invocationReward() < state.get().transferFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		if (!isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || input.owner == NULL_ID || input.owner == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.receiver == NULL_ID || qpi.isContractId(input.receiver) || input.receiver == input.owner)
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

		if (locals.item.supply == 0)
		{
			output.code = Code::NotFound;
			return;
		}

		if (locals.item.origin == Origin::Qbay && locals.item.bridge != Bridge::Permanent)
		{
			output.code = Code::Conflict;
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

		// An approval grants authority, not units; only the owner's balance managed here can move.
		locals.balance = qpi.numberOfPossessedShares(locals.item.assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX);
		if (locals.balance < input.quantity)
		{
			output.code = Code::InsufficientBalance;
			return;
		}

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
	 * @brief Transfer owned units or use a persistent item approval.
	 * @note Only the owner or the approved operator may transfer; delivery preserves approval.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Transfer)
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
	 * @brief Authorize one operator for an item until explicit revocation or replacement.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteApprove)
	{
		// Approval belongs to the invocator and this item, even if the account currently holds no units.
		if (qpi.invocationReward() < state.get().approveFee)
		{
			output.code = Code::InsufficientFee;
			return;
		}

		if (input.receiver == NULL_ID || input.receiver == qpi.invocator())
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (qpi.invocator() == NULL_ID || qpi.invocator() == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.keyInput.owner = qpi.invocator();
		locals.keyInput.itemId = input.itemId;
		locals.key = qpi.K12(locals.keyInput);

		// Permit replacement at capacity because it does not allocate another permission entry.
		if (!state.get().approvals.contains(locals.key) && state.get().approvals.population() >= QNFT_MAX_APPROVALS)
		{
			output.code = Code::Capacity;
			return;
		}

		locals.approval.operatorId = input.receiver;
		state.mut().approvals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().approvals.set(locals.key, locals.approval);
		output.code = Code::Ok;
		output.itemId = input.itemId;
	}

	/**
	 * @brief Authorize one operator for an item until explicit revocation or replacement.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Approve)
	{
		// Only a stored permission incurs the approval fee; failed attempts refund the attached QU.
		CALL(ExecuteApprove, input, output);

		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Approve;
		locals.finishInput.receiver = input.receiver;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.fee = output.code == Code::Ok ? state.get().approveFee : 0;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Revoke the invocator's approval without requiring initialized network time.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteRevoke)
	{
		// Derive the key from the invocator so no caller can revoke another owner's permission.

		if (qpi.invocator() == NULL_ID || qpi.invocator() == SELF)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.keyInput.owner = qpi.invocator();
		locals.keyInput.itemId = input.itemId;
		locals.key = qpi.K12(locals.keyInput);

		if (!state.get().approvals.contains(locals.key))
		{
			output.code = Code::NotFound;
			return;
		}

		state.mut().approvals.removeByKey(locals.key);
		output.itemId = input.itemId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Revoke the invocator's approval without requiring initialized network time.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Revoke)
	{
		// Revocation is free, allowing an owner to remove an operator without paying a fee.
		CALL(ExecuteRevoke, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Revoke;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Update mutable metadata as its authorized editor.
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

		locals.metadataInput.metadata = input.metadata;
		CALL(CheckMetadata, locals.metadataInput, locals.metadataOutput);
		if (!locals.metadataOutput.valid)
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

		if (locals.item.frozen || locals.item.origin != Origin::Native)
		{
			output.code = Code::Frozen;
			return;
		}

		if (qpi.invocator() != locals.item.editor && !(locals.item.policy == Policy::Reveal && qpi.invocator() == locals.item.creator))
		{
			output.code = Code::Unauthorized;
			return;
		}

		if (locals.item.policy != Policy::Mutable)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (locals.item.metadataVersion == QNFT_MAX_METADATA_VERSION)
		{
			output.code = Code::Capacity;
			return;
		}

		// Commit the new version before announcing its URI to readers.
		locals.item.metadata = input.metadata;
		locals.commitInput.item = locals.item;
		locals.commitInput.itemId = input.itemId;
		CALL(CommitMetadataUpdate, locals.commitInput, locals.commitOutput);
		locals.uriEvent.uri = locals.item.metadata.uri;
		locals.uriEvent.itemId = input.itemId;
		CALL(EmitUri, locals.uriEvent, locals.uriLogOutput);

		output.itemId = input.itemId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Update mutable metadata as its authorized editor.
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
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteFreezeMetadata)
	{
		// Only native, editable metadata can be frozen; the reveal policy must finish through its commitment check.
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		if (locals.item.frozen || locals.item.origin != Origin::Native)
		{
			output.code = Code::Frozen;
			return;
		}

		if (qpi.invocator() != locals.item.editor && qpi.invocator() != locals.item.creator)
		{
			output.code = Code::Unauthorized;
			return;
		}

		if (locals.item.policy == Policy::Reveal)
		{
			output.code = Code::Conflict;
			return;
		}

		// Freezing is permanent and does not alter the current URI or its version.
		locals.item.frozen = 1;
		state.mut().items.set(input.itemId, locals.item);
		output.itemId = input.itemId;
		output.code = Code::Ok;
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
	 * @brief Reveal metadata matching the original commitment and freeze it.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteRevealMetadata)
	{
		// Validate the document and authority, then require the exact metadata commitment recorded at minting.

		if (input.metadata.contentHash == NULL_ID)
		{
			output.code = Code::InvalidInput;
			return;
		}

		locals.metadataInput.metadata = input.metadata;
		CALL(CheckMetadata, locals.metadataInput, locals.metadataOutput);
		if (!locals.metadataOutput.valid)
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

		if (locals.item.frozen || locals.item.origin != Origin::Native)
		{
			output.code = Code::Frozen;
			return;
		}

		if (qpi.invocator() != locals.item.editor && qpi.invocator() != locals.item.creator)
		{
			output.code = Code::Unauthorized;
			return;
		}

		if (locals.item.policy != Policy::Reveal || qpi.K12(input.metadata) != locals.item.commitment)
		{
			output.code = Code::InvalidInput;
			return;
		}

		if (locals.item.metadataVersion == QNFT_MAX_METADATA_VERSION)
		{
			output.code = Code::Capacity;
			return;
		}

		// Commit the revealed document and permanently disable further metadata edits together.
		locals.item.frozen = 1;
		locals.item.metadata = input.metadata;
		locals.commitInput.item = locals.item;
		locals.commitInput.itemId = input.itemId;
		CALL(CommitMetadataUpdate, locals.commitInput, locals.commitOutput);
		locals.uriEvent.uri = locals.item.metadata.uri;
		locals.uriEvent.itemId = input.itemId;
		CALL(EmitUri, locals.uriEvent, locals.uriLogOutput);

		output.itemId = input.itemId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Reveal metadata matching the original commitment and freeze it.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(RevealMetadata)
	{
		// Revealing a committed document is free; the helper performs the one-way freeze.
		CALL(ExecuteRevealMetadata, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::RevealMetadata;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Release asset management to an accepting registered destination.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteExport)
	{
		// Budget the protocol fee and the offered destination fee separately before releasing management.

		if (input.owner == SELF || input.owner == NULL_ID || !isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || !input.destination ||
		    input.destination == SELF_INDEX || input.offeredFee < 0)
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

		if (!locals.item.supply)
		{
			output.result.code = Code::NotFound;
			return;
		}

		// Generic native-asset export. Core validates the destination lifecycle and callbacks.
		// Registered managers are trusted through Mainnet code review: callbacks run with SELF as invocator.
		if (locals.item.origin == Origin::Qbay && locals.item.bridge != Bridge::Permanent)
		{
			output.result.code = Code::Conflict;
			return;
		}

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

		locals.asset = Asset{.issuer = SELF, .assetName = locals.item.assetName};

		qpi.getEntity(SELF, locals.beforeExport);
		locals.exportSucceeded =
		    qpi.releaseShares(locals.asset, input.owner, input.owner, input.quantity, input.destination, input.destination, input.offeredFee) >= 0;
		qpi.getEntity(SELF, locals.afterExport);
		// Measure the net QU spent during releaseShares to calculate the caller's refund.
		locals.spentAmount = (locals.beforeExport.incomingAmount - locals.beforeExport.outgoingAmount) -
		                     (locals.afterExport.incomingAmount - locals.afterExport.outgoingAmount);
		if (locals.spentAmount < 0)
		{
			locals.spentAmount = 0;
		}

		if (locals.exportSucceeded)
		{
			locals.accrualInput = state.get().exportFee;
			CALL(AccrueProtocolFee, locals.accrualInput, locals.accrualOutput);
			locals.spentAmount += state.get().exportFee;
		}

		if (qpi.invocationReward() > locals.spentAmount)
		{
			locals.refundInput.amount = qpi.invocationReward() - locals.spentAmount;
			CALL(Refund, locals.refundInput, locals.refundOutput);
		}

		output.result.code = locals.exportSucceeded ? Code::Ok : Code::ExternalFailure;
		output.result.itemId = input.itemId;
		output.result.quantity = input.quantity;
		output.result.assetName = locals.item.assetName;
		locals.event = Event{.contractIndex = SELF_INDEX,
		                     .type = Operation::Export,
		                     .typeReserved = 0,
		                     .actor = qpi.invocator(),
		                     .receiver = id(input.destination, 0, 0, 0),
		                     .quantity = input.quantity,
		                     .itemId = input.itemId,
		                     .version = 0,
		                     ._terminator = 0};
		if (output.result.code == Code::Ok)
		{
			CALL(EmitCommand, locals.event, locals.eventOutput);
		}
		else
		{
			locals.failureInput.domain = FailureDomain::Command;
			locals.failureInput.operation = Operation::Export;
			locals.failureInput.itemId = input.itemId;
			locals.failureInput.code = output.result.code;
			CALL(LogFailure, locals.failureInput, locals.failureOutput);
		}

		output.settlementCompleted = true;
	}

	/**
	 * @brief Release asset management to an accepting registered destination.
	 * @note Ownership is unchanged. Destination fees already consumed by QPI cannot be refunded.
	 * @note Destination callbacks follow the accepted Mainnet contract-review trust model.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Export)
	{
		// The export helper may already have settled a destination payment; do not refund or log it twice.
		CALL(ExecuteExport, input, locals.operationOutput);
		output = locals.operationOutput.result;
		if (locals.operationOutput.settlementCompleted)
		{
			return;
		}

		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Export;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Burn native units without restoring collection issuance quota.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBurn)
	{
		// Validate native-item holdings and operator permission; imported originals remain permanently backed.

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

		locals.item = state.get().items.get(input.itemId);

		if (locals.item.supply == 0)
		{
			output.code = Code::NotFound;
			return;
		}

		if (locals.item.origin != Origin::Native)
		{
			output.code = Code::Conflict;
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

		locals.balance = qpi.numberOfPossessedShares(locals.item.assetName, SELF, input.owner, input.owner, SELF_INDEX, SELF_INDEX);
		if (locals.balance < input.quantity)
		{
			output.code = Code::InsufficientBalance;
			return;
		}

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

	/**
	 * @brief Burn native units without restoring collection issuance quota.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Burn)
	{
		// Burning is free and does not revoke the owner's persistent item approval.
		CALL(ExecuteBurn, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::Burn;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Recover reserved mint delivery as its recorded recipient.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteClaimMint)
	{
		// Only the recorded recipient may recover units retained after an incomplete mint delivery.
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(input.itemId);

		if (locals.item.origin != Origin::Native || !locals.item.reserved || locals.item.reservedRecipient != qpi.invocator())
		{
			output.code = Code::Unauthorized;
			return;
		}

		locals.transferInput.from = SELF;
		locals.transferInput.to = qpi.invocator();
		locals.transferInput.quantity = locals.item.reserved;
		locals.transferInput.itemId = input.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		// Clear the delivery reservation only after the core transfer succeeds.
		output.quantity = locals.item.reserved;
		output.itemId = input.itemId;
		locals.item.reserved = 0;
		locals.item.reservedRecipient = NULL_ID;
		state.mut().items.set(input.itemId, locals.item);
		output.code = Code::Ok;
	}

	/**
	 * @brief Recover reserved mint delivery as its recorded recipient.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(ClaimMint)
	{
		// Recovery returns already issued units, so no second mint fee is charged.
		CALL(ExecuteClaimMint, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::ClaimMint;
		locals.finishInput.itemId = input.itemId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Reserve a QBay wrapper import for the selected beneficiary.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecutePrepareImport)
	{
		// Validate the original and collection before creating a request; item allocation belongs to finalization.

		if (input.receiver == NULL_ID || qpi.isContractId(input.receiver))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (state.get().importRequests.population() >= QNFT_MAX_IMPORTS)
		{
			output.result.code = Code::Capacity;
			return;
		}

		// Check the original ID against QBay's count before querying its fixed-size NFT array.
		CALL_OTHER_CONTRACT_FUNCTION_E(QBAY, getInfoOfMarketplace, locals.marketInput, locals.marketOutput, marketError);
		if (marketError != NoCallError)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		if (input.qbayId >= locals.marketOutput.numberOfNFT)
		{
			output.result.code = Code::NotFound;
			return;
		}

		locals.originalInput.NFTId = input.qbayId;
		CALL_OTHER_CONTRACT_FUNCTION_E(QBAY, getInfoOfNFTById, locals.originalInput, locals.original, originalError);
		if (originalError != NoCallError)
		{
			output.result.code = Code::ExternalFailure;
			return;
		}

		if (locals.original.possessor != qpi.invocator())
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		if (locals.original.statusOfSale || locals.original.statusOfAsk || locals.original.statusOfExchange)
		{
			output.result.code = Code::Conflict;
			return;
		}

		if (state.get().imports.contains(input.qbayId))
		{
			output.result.code = Code::Conflict;
			return;
		}

		// The original ID is the map key, making duplicate reservations a direct lookup.
		if (state.get().importRequests.contains(input.qbayId))
		{
			output.result.code = Code::Conflict;
			return;
		}

		if (state.get().itemCount + state.get().importRequests.population() >= QNFT_MAX_ITEMS)
		{
			output.result.code = Code::Capacity;
			return;
		}

		if (qpi.invocationReward() < state.get().importFee)
		{
			output.result.code = Code::InsufficientFee;
			return;
		}

		if (input.collectionId >= state.get().collectionCount)
		{
			output.result.code = Code::NotFound;
			return;
		}

		locals.collection = state.get().collections.get(input.collectionId);
		if (locals.collection.administrator != qpi.invocator())
		{
			output.result.code = Code::Unauthorized;
			return;
		}

		locals.reservationsInput.collectionId = input.collectionId;
		CALL(CountImportReservations, locals.reservationsInput, locals.reservationsOutput);
		if (locals.collection.issuedItems + locals.reservationsOutput.count >= locals.collection.maxItems ||
		    locals.collection.issuedUnits + locals.reservationsOutput.count >= locals.collection.maxUnits)
		{
			output.result.code = Code::Capacity;
			return;
		}

		if (locals.original.royalty > QNFT_PERCENT_DENOMINATOR)
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (state.get().nextImportRequestId == QNFT_REQUEST_ID_LIMIT)
		{
			output.result.code = Code::Capacity;
			return;
		}

		locals.request.requester = qpi.invocator();
		locals.request.recipient = input.receiver;
		locals.request.collectionId = input.collectionId;
		locals.request.qbayId = input.qbayId;
		locals.request.creator = locals.original.creator;
		locals.request.royaltyBps = static_cast<uint16>(locals.original.royalty * QNFT_BPS_PER_PERCENT);
		copyToBuffer(locals.request.metadata.uri, locals.original.URI, true);

		// Reserve capacity without allocating an item or native asset name.
		CALL(CommitNewImportRequest, locals.request, locals.committedRequest);
		output.result.itemId = locals.committedRequest;
		output.fee = state.get().importFee;
		output.result.code = Code::Ok;
	}

	/**
	 * @brief Reserve a QBay wrapper import for the selected beneficiary.
	 * @note Requires the current QBay owner and a clean original; collection administration is required for every new reservation.
	 * @note Preparation only reserves capacity; item allocation is deferred until successful asset issuance.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(PrepareImport)
	{
		// Expose the independent request ID after settlement; it is not an allocated NFT item ID.
		CALL(ExecutePrepareImport, input, locals.operationOutput);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::PrepareImport;
		locals.finishInput.receiver = input.receiver;
		locals.finishInput.collectionId = input.collectionId;
		locals.finishInput.result = locals.operationOutput.result;
		locals.finishInput.fee = locals.operationOutput.fee;
		locals.finishInput.retainedAmount = locals.finishInput.fee;
		CALL(FinishCommand, locals.finishInput, locals.settled);
		output.requestId = locals.settled.itemId;
		output.code = locals.settled.code;
	}

	/**
	 * @brief Delete an unissued import request and release its unused collection reservation.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteCancelImport)
	{
		// Look up the unique request, so an old cancellation cannot affect a replacement reservation.
		if (!state.get().importRequests.get(input.qbayId, locals.request) || locals.request.requestId != input.requestId)
		{
			output.code = Code::NotFound;
			return;
		}

		// An issued asset must finish delivery; cancellation must not discard that recovery record.
		if (state.get().imports.contains(locals.request.qbayId))
		{
			output.code = Code::Conflict;
			return;
		}

		locals.originalInput.NFTId = locals.request.qbayId;
		CALL_OTHER_CONTRACT_FUNCTION_E(QBAY, getInfoOfNFTById, locals.originalInput, locals.original, originalError);
		if (originalError != NoCallError)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		if (locals.original.possessor == SELF)
		{
			output.code = Code::Conflict;
			return;
		}

		if (locals.request.requester != qpi.invocator() && locals.original.possessor != qpi.invocator())
		{
			output.code = Code::Unauthorized;
			return;
		}

		// Remove only the request and its capacity reservation; no item was allocated.
		state.mut().importRequests.removeByKey(input.qbayId);
		output.itemId = input.requestId;
		output.code = Code::Ok;
	}

	/**
	 * @brief Delete an unissued import request and release its unused collection reservation.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CancelImport)
	{
		// Cancellation is free; the original preparation fee remains in the epoch revenue pool.
		CALL(ExecuteCancelImport, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::CancelImport;
		locals.finishInput.itemId = input.requestId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
	}

	/**
	 * @brief Deliver an issued QBay wrapper without issuing again or changing issuance counters.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(DeliverImport)
	{
		if (input >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(input);
		if (locals.item.origin != Origin::Qbay || locals.item.bridge != Bridge::Requested || locals.item.reserved != 1)
		{
			output.code = Code::NotFound;
			return;
		}

		output.itemId = input;
		output.assetName = locals.item.assetName;
		// A failed transfer leaves the recipient and escrow quantity intact for a later tick.
		locals.transferInput.from = SELF;
		locals.transferInput.to = locals.item.reservedRecipient;
		locals.transferInput.quantity = 1;
		locals.transferInput.itemId = input;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.code = Code::Pending;
			return;
		}

		// Import is one-way: the original remains in QNFT custody permanently.
		locals.item.bridge = Bridge::Permanent;
		locals.item.reservedRecipient = NULL_ID;
		locals.item.reserved = 0;
		state.mut().items.set(input, locals.item);

		output.quantity = 1;
		output.code = Code::Ok;
	}

	/**
	 * @brief Finalize a deposited QBay wrapper import.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteFinalizeImport)
	{
		// Only an unissued request can be finalized; issued imports are delivered by the tick queue.
		if (!state.get().importRequests.get(input.qbayId, locals.request) || locals.request.requestId != input.requestId)
		{
			output.code = Code::NotFound;
			return;
		}

		locals.originalInput.NFTId = input.qbayId;
		CALL_OTHER_CONTRACT_FUNCTION_E(QBAY, getInfoOfNFTById, locals.originalInput, locals.original, originalError);
		if (originalError != NoCallError)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		if (locals.original.possessor != SELF)
		{
			output.code = Code::Conflict;
			return;
		}

		// The request reserved this capacity, even if native minting happened meanwhile.
		locals.itemId = state.get().itemCount;
		locals.nameInput.itemId = locals.itemId;
		CALL(Name, locals.nameInput, locals.nameOutput);
		locals.item.assetName = locals.nameOutput.name;
		locals.item.collectionId = locals.request.collectionId;
		locals.item.qbayId = input.qbayId;
		locals.item.creator = locals.request.creator;
		locals.item.royaltyReceiver = locals.request.creator;
		locals.item.royaltyBps = locals.request.royaltyBps;
		locals.item.metadata = locals.request.metadata;
		locals.item.origin = Origin::Qbay;
		locals.item.bridge = Bridge::Requested;
		locals.item.frozen = 1;
		locals.item.metadataVersion = 1;
		locals.item.reservedRecipient = locals.request.recipient;

		output.itemId = locals.itemId;
		output.assetName = locals.item.assetName;
		if (qpi.issueAsset(locals.item.assetName, SELF, 0, 1, 0) != 1)
		{
			output.code = Code::ExternalFailure;
			return;
		}

		// Commit identity only after issuance succeeds; retries must never issue again.
		locals.item.supply = 1;
		locals.item.reserved = 1;
		CALL(CommitIssuedItem, locals.item, locals.committedItem);
		locals.itemId = locals.committedItem.itemId;
		output.itemId = locals.itemId;
		// Issuance permanently binds the original, even if the following delivery must be retried.
		state.mut().imports.set(locals.request.qbayId, locals.itemId);
		state.mut().importRequests.removeByKey(input.qbayId);
		locals.transferEvent.from = NULL_ID;
		locals.transferEvent.to = SELF;
		locals.transferEvent.quantity = 1;
		locals.transferEvent.itemId = locals.itemId;
		CALL(EmitTransfer, locals.transferEvent, locals.transferLogOutput);

		locals.uriEvent.uri = locals.item.metadata.uri;
		locals.uriEvent.itemId = locals.itemId;
		CALL(EmitUri, locals.uriEvent, locals.uriLogOutput);

		// Both explicit finalization and the tick hook use the same delivery transition.
		CALL(DeliverImport, locals.itemId, output);
		if (output.code == Code::Pending)
		{
			// At most one queue entry per issued item fits the item registry's capacity.
			state.mut().importDeliveries.addTail(locals.itemId);
		}
	}

	/**
	 * @brief Finalize a deposited QBay wrapper import.
	 * @note Anyone may finalize an unissued request; pending delivery is retried only by the tick queue.
	 * @note Successful delivery sets Permanent; the original can never be returned through QNFT.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(FinalizeImport)
	{
		// Finalization is permissionless but the stored request fixes the recipient; no additional import fee is retained.
		CALL(ExecuteFinalizeImport, input, output);
		locals.finishInput.domain = FailureDomain::Command;
		locals.finishInput.operation = Operation::FinalizeImport;
		locals.finishInput.itemId = input.requestId;
		locals.finishInput.result = output;
		CALL(FinishCommand, locals.finishInput, output);
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
	 * @brief Preserve item approvals after management returns to QNFT.
	 */
	POST_ACQUIRE_SHARES()
	{
		// Returning management does not change ownership or revoke existing item permissions.
	}

	/**
	 * @brief Reject unsolicited acquisition requests; owners export through the explicit procedure.
	 */
	PRE_RELEASE_SHARES()
	{
		// Leave allowTransfer false; management leaves QNFT only through the authorized Export procedure.
	}

	/** @brief Return full pending import requests with a cursor stable across map cleanup and deletion. */
	PUBLIC_FUNCTION_WITH_LOCALS(ListImportRequests)
	{
		if (!isInRange<uint32>(input.limit, 1, QNFT_REGISTRY_PAGE_CAPACITY))
		{
			output.code = Code::InvalidInput;
			return;
		}

		output.code = Code::Ok;
		output.totalCount = static_cast<uint32>(state.get().importRequests.population());
		output.nextOffset = input.offset;
		// Each scan selects the next version. The map contains at most QNFT_MAX_IMPORTS live entries.
		while (output.count < input.limit)
		{
			locals.selectedIndex = NULL_INDEX;
			locals.selectedVersion = QNFT_REQUEST_ID_LIMIT;
			for (locals.index = state.get().importRequests.nextElementIndex(NULL_INDEX); locals.index != NULL_INDEX;
			     locals.index = state.get().importRequests.nextElementIndex(locals.index))
			{
				if (state.get().importRequests.value(locals.index).requestId >= output.nextOffset &&
				    state.get().importRequests.value(locals.index).requestId < locals.selectedVersion)
				{
					locals.selectedIndex = locals.index;
					locals.selectedVersion = state.get().importRequests.value(locals.index).requestId;
				}
			}

			if (locals.selectedIndex == NULL_INDEX)
			{
				output.nextOffset = QNFT_REQUEST_ID_LIMIT;
				return;
			}

			output.requests.set(output.count, state.get().importRequests.value(locals.selectedIndex));
			++output.count;
			// The exhaustion sentinel is never assigned to a request, so adding one cannot wrap.
			output.nextOffset = locals.selectedVersion + 1;
		}

		// Report an exact end marker even when the last page contains the requested maximum number of records.
		for (locals.index = state.get().importRequests.nextElementIndex(NULL_INDEX); locals.index != NULL_INDEX;
		     locals.index = state.get().importRequests.nextElementIndex(locals.index))
		{
			if (state.get().importRequests.value(locals.index).requestId >= output.nextOffset)
			{
				return;
			}
		}

		output.nextOffset = QNFT_REQUEST_ID_LIMIT;
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

	/** @brief Read the beneficiary, original and version of an unissued import request. */
	PUBLIC_FUNCTION_WITH_LOCALS(GetImport)
	{
		// Only unissued reservations remain here; cancelled or issued originals return NotFound.
		if (!state.get().importRequests.get(input.qbayId, locals.request))
		{
			output.code = Code::NotFound;
			return;
		}

		// Expose the version and fixed beneficiary needed to verify and finalize the request.
		output.request.metadata = locals.request.metadata;
		output.request.requester = locals.request.requester;
		output.request.recipient = locals.request.recipient;
		output.request.creator = locals.request.creator;
		output.request.collectionId = locals.request.collectionId;
		output.request.qbayId = locals.request.qbayId;
		output.request.royaltyBps = locals.request.royaltyBps;
		output.request.requestId = locals.request.requestId;
		output.code = Code::Ok;
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
		if (input.owner == NULL_ID || input.reserved)
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

	/**
	 * @brief Read one owner's persistent item authorization for transfer, burn or export.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetApproval)
	{
		// A missing permission for an existing item is a successful lookup with a zero operator.
		if (input.itemId >= state.get().itemCount)
		{
			output.code = Code::NotFound;
			return;
		}

		output.code = Code::Ok;
		locals.key.owner = input.owner;
		locals.key.itemId = input.itemId;

		state.get().approvals.get(qpi.K12(locals.key), output.approval);
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

	/** @brief Enumerate pending refunds without transfers or recipient callbacks. */
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
		// Expose allocated counts separately from capacity limits; pending imports do not increase itemCount.
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

	/** @brief Return full allocated items in a bounded page; exclude unissued import requests. */
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
	 * @brief Escrow locally managed units at a fixed QU price while preserving item approval.
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

		if (input.reserved || !isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || !isInRange<sint64>(input.unitPrice, 1, MAX_AMOUNT) ||
		    qpi.invocator() == NULL_ID || qpi.isContractId(qpi.invocator()))
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
		if (locals.item.supply <= 0 || state.get().shopFeeBPS > QNFT_BPS_DENOMINATOR ||
		    effectiveRoyaltyBps(locals.item) > QNFT_BPS_DENOMINATOR - state.get().shopFeeBPS ||
		    (locals.item.origin != Origin::Native && locals.item.bridge != Bridge::Permanent))
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
	 * @brief Buy a protected-price quantity and pay seller proceeds and royalties immediately.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteBuy)
	{
		// Reject stale prices, changed metadata and excessive quantities before accepting payment or delivering escrow.
		if (input.reserved || !isInRange<sint64>(input.quantity, 1, MAX_AMOUNT) || input.receiver == NULL_ID || qpi.isContractId(input.receiver) ||
		    !isInRange<sint64>(input.unitPrice, 1, MAX_AMOUNT))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		if (!state.get().listings.get(input.listingId, locals.listing))
		{
			output.result.code = Code::NotFound;
			return;
		}

		locals.item = state.get().items.get(locals.listing.itemId);
		if (state.get().shopFeeBPS > QNFT_BPS_DENOMINATOR || effectiveRoyaltyBps(locals.item) > QNFT_BPS_DENOMINATOR - state.get().shopFeeBPS ||
		    input.unitPrice != locals.listing.unitPrice || input.metadataVersion != locals.listing.metadataVersion ||
		    locals.item.metadataVersion != locals.listing.metadataVersion || input.quantity > locals.listing.quantity ||
		    input.receiver == locals.listing.seller || qpi.invocator() == locals.listing.seller)
		{
			output.result.code = Code::Conflict;
			return;
		}

		if (input.quantity > QPI::div(MAX_AMOUNT, locals.listing.unitPrice))
		{
			output.result.code = Code::InvalidInput;
			return;
		}

		// The division above proves quantity * unitPrice <= MAX_AMOUNT.
		output.result.paid = input.quantity * locals.listing.unitPrice;
		if (qpi.invocationReward() < output.result.paid)
		{
			output.result.code = Code::InsufficientFee;
			output.result.paid = 0;
			return;
		}

		// Discard fractional QU without imposing a minimum commission.
		output.result.fee = calculateFeeShare(output.result.paid, state.get().shopFeeBPS);
		output.result.royalty = calculateFeeShare(output.result.paid, effectiveRoyaltyBps(locals.item));
		locals.sellerAmount = output.result.paid - output.result.fee - output.result.royalty;
		// A positive invocation reward covers both payouts and the caller's change.
		// Callback-invoked procedures receive zero reward and cannot pass the paid-price check above.
		qpi.getEntity(SELF, locals.contractBalance);
		if (locals.contractBalance.incomingAmount - locals.contractBalance.outgoingAmount < qpi.invocationReward())
		{
			output.result.code = Code::InsufficientBalance;
			output.result.paid = 0;
			return;
		}

		locals.transferInput.from = SELF;
		locals.transferInput.to = input.receiver;
		locals.transferInput.quantity = input.quantity;
		locals.transferInput.itemId = locals.listing.itemId;
		CALL(TransferAsset, locals.transferInput, locals.transferResult);
		if (locals.transferResult < 0)
		{
			output.result.code = Code::ExternalFailure;
			output.result.paid = 0;
			return;
		}

		// Commit the completed sale before paying a royalty contract that may receive a callback.
		locals.commitInput.listing = locals.listing;
		locals.commitInput.listingId = input.listingId;
		locals.commitInput.quantity = input.quantity;
		CALL(CommitListingFill, locals.commitInput, locals.commitOutput);

		// Combine both shares when the seller is also the royalty recipient.
		if (locals.item.royaltyReceiver == locals.listing.seller)
		{
			locals.sellerAmount += output.result.royalty;
		}

		// Listing sellers are ordinary accounts: this first payment cannot invoke a callback.
		// Both positive amounts are bounded by paid and backed by the preflighted reward.
		if (locals.sellerAmount > 0)
		{
			qpi.transfer(locals.listing.seller, locals.sellerAmount);
		}

		// Pay a distinct royalty beneficiary last, after all sale state has been committed.
		if (output.result.royalty > 0 && locals.item.royaltyReceiver != locals.listing.seller)
		{
			qpi.transfer(locals.item.royaltyReceiver, output.result.royalty);
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
	 * @brief Buy a protected-price quantity and pay seller proceeds and royalties immediately.
	 * @note Pays seller proceeds and royalties after asset delivery and listing commit; failed purchases refund the reward.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(Buy)
	{
		// The helper delivers assets and pays beneficiaries; settlement refunds change and accrues only the shop fee.
		CALL(ExecuteBuy, input, locals.operationOutput);
		output = locals.operationOutput.result;
		if (output.code != Code::Ok)
		{
			output.paid = 0;
			output.fee = 0;
			output.royalty = 0;
		}

		locals.finishInput.domain = FailureDomain::Market;
		locals.finishInput.marketEvent = locals.operationOutput.event;
		locals.finishInput.marketEvent.type = MarketAction::Buy;
		locals.finishInput.marketEvent.listingId = input.listingId;
		locals.finishInput.result.code = output.code;
		locals.finishInput.retainedAmount = output.paid;
		locals.finishInput.fee = output.fee;
		CALL(FinishCommand, locals.finishInput, locals.settled);
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
			case FeeVariable::ImportFee:
			case FeeVariable::ListingFee:
			case FeeVariable::ProposalFee: return isInRange<sint64>(value, 0, MAX_AMOUNT);
			case FeeVariable::ShopFeeBPS: return isInRange<sint64>(value, 0, QNFT_BPS_DENOMINATOR);
			case FeeVariable::ShareholderFeeBPS:
			case FeeVariable::BurnFeeBPS:
				return developerFeeBPS <= QNFT_BPS_DENOMINATOR && isInRange<sint64>(value, 0, QNFT_BPS_DENOMINATOR - developerFeeBPS);
			default: return false;
		}
	}

	/**
	 * @brief Reject duplicate/unknown variables, invalid ranges, noncanonical tails and inconsistent splits.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateFeeBatch)
	{
		// Validate the entire batch before applying any entry, including unused trailing slots.
		output.valid = false;
		if (!isInRange<uint32>(input.count, 1, QNFT_FEE_VARIABLE_COUNT) || input.reserved)
		{
			return;
		}

		for (locals.i = 0; locals.i < input.changes.capacity(); ++locals.i)
		{
			if (locals.i >= input.count)
			{
				if (static_cast<uint64>(input.changes.get(locals.i).variable) || input.changes.get(locals.i).value)
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
			if (input.changes.get(locals.i).variable == FeeVariable::ShareholderFeeBPS)
			{
				locals.shareholder = input.changes.get(locals.i).value;
			}

			if (input.changes.get(locals.i).variable == FeeVariable::BurnFeeBPS)
			{
				locals.burn = input.changes.get(locals.i).value;
			}
		}

		// When both adjustable shares are proposed, their combined split must still total 100 percent.
		if ((locals.seen & QNFT_REVENUE_SPLIT_VARIABLE_MASK) == QNFT_REVENUE_SPLIT_VARIABLE_MASK &&
		    locals.shareholder + locals.burn + state.get().developerFeeBPS != QNFT_BPS_DENOMINATOR)
		{
			return;
		}

		output.valid = true;
	}

	/**
	 * @brief Create or replace a hash-bound batch for one native yes/no vote and one proposal fee.
	 * @note All entries must validate before the native QPI proposal is stored; one proposal fee covers the whole batch.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetFeeProposalBatch)
	{
		// Validate the payload before asking QPI to authorize the proposer and allocate a voting slot.
		output.result = INVALID_PROPOSAL_INDEX;
		output.event.action = GovernanceAction::SetProposal;
		output.event.actor = qpi.invocator();
		locals.batch.changes = input.changes;
		locals.batch.count = input.count;
		locals.batch.reserved = input.reserved;
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

	/** @brief Execute SetFeeProposalBatch and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetFeeProposalBatch)
	{
		CALL(ExecuteSetFeeProposalBatch, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.event = locals.operationOutput.event;
		locals.event.proposalIndex = output;
		locals.refundInput.amount = qpi.invocationReward();
		if (output != INVALID_PROPOSAL_INDEX)
		{
			locals.accrualInput = state.get().proposalFee;
			CALL(AccrueProtocolFee, locals.accrualInput, locals.accrualOutput);
			locals.refundInput.amount -= state.get().proposalFee;
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

		CALL(Refund, locals.refundInput, locals.refundOutput);
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
	 * @brief Validate fee bounds, then let QPI authorize, snapshot and create/replace/cancel the proposal.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetShareholderProposal)
	{
		// Epoch zero requests cancellation; other proposals require payment and a supported fee variable.
		output.result = INVALID_PROPOSAL_INDEX;
		output.event.action = GovernanceAction::SetProposal;
		output.event.actor = qpi.invocator();
		output.event.variable = static_cast<FeeVariable>(input.data.variableOptions.variable);
		output.event.value = input.data.variableOptions.value;
		if (state.get().lastGovernanceEpoch == qpi.epoch() ||
		    (input.epoch && (qpi.invocationReward() < state.get().proposalFee || input.type != ProposalTypes::VariableYesNo ||
		                     !isGovernedValueValid(output.event.variable, input.data.variableOptions.value, state.get().developerFeeBPS))))
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

	/** @brief Execute SetShareholderProposal and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetShareholderProposal)
	{
		CALL(ExecuteSetShareholderProposal, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.event = locals.operationOutput.event;
		locals.event.proposalIndex = output;
		locals.refundInput.amount = qpi.invocationReward();
		if (output != INVALID_PROPOSAL_INDEX)
		{
			if (input.epoch && state.get().proposalFee > 0)
			{
				locals.accrualInput = state.get().proposalFee;
				CALL(AccrueProtocolFee, locals.accrualInput, locals.accrualOutput);
				locals.refundInput.amount -= state.get().proposalFee;
			}

			CALL(EmitGovernance, locals.event, locals.eventOutput);
		}
		else
		{
			LOG_ERROR(locals.event);
		}

		CALL(Refund, locals.refundInput, locals.refundOutput);
	}

	/**
	 * @brief Submit a version-checked ballot to QPI using proposal-snapshot share weights.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteSetShareholderVotes)
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

	/** @brief Execute SetShareholderVotes and settle native governance events and refunds. */
	PUBLIC_PROCEDURE_WITH_LOCALS(SetShareholderVotes)
	{
		CALL(ExecuteSetShareholderVotes, input, locals.operationOutput);
		output = locals.operationOutput.result;
		locals.event = locals.operationOutput.event;
		if (output)
		{
			CALL(EmitGovernance, locals.event, locals.eventOutput);
		}
		else
		{
			LOG_ERROR(locals.event);
		}

		locals.refundInput.amount = qpi.invocationReward();
		CALL(Refund, locals.refundInput, locals.refundOutput);
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
		output.importFee = state.get().importFee;
		output.listingFee = state.get().listingFee;
		output.proposalFee = state.get().proposalFee;
		output.shopFeeBPS = state.get().shopFeeBPS;
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
			case FeeVariable::ImportFee: state.mut().importFee = input.value; break;
			case FeeVariable::ListingFee: state.mut().listingFee = input.value; break;
			case FeeVariable::ProposalFee: state.mut().proposalFee = input.value; break;
			case FeeVariable::ShopFeeBPS: state.mut().shopFeeBPS = static_cast<uint32>(input.value); break;
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

	/**
	 * @brief Retry a bounded batch of issued imports, retaining failed deliveries for later ticks.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(RetryImportDeliveries)
	{
		// Snapshot the queue length so a failed entry is attempted at most once this tick.
		locals.remaining = state.get().importDeliveries.population();
		if (locals.remaining > QNFT_IMPORT_DELIVERIES_PER_TICK)
		{
			locals.remaining = QNFT_IMPORT_DELIVERIES_PER_TICK;
		}

		while (locals.remaining)
		{
			locals.queueIndex = state.get().importDeliveries.headIndex();
			locals.itemId = state.get().importDeliveries.element(locals.queueIndex);
			state.mut().importDeliveries.remove(locals.queueIndex);
			CALL(DeliverImport, locals.itemId, locals.delivery);
			if (locals.delivery.code == Code::Pending)
			{
				// Rotate failures so a blocked recipient cannot starve the rest of the queue.
				state.mut().importDeliveries.addTail(locals.itemId);
			}

			// Completed entries leave the queue; failures were retained for a later tick.
			--locals.remaining;
		}
	}

	/** @brief Retry a bounded set of failed refunds, preserving callback-created credits. */
	PRIVATE_PROCEDURE_WITH_LOCALS(RetryRefunds)
	{
		locals.remaining = RL::min(state.get().contractRefunds.population(), static_cast<uint64>(QNFT_REFUNDS_PER_TICK));
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

	/** @brief Process bounded import delivery and refund retries after transactions. */
	END_TICK_WITH_LOCALS()
	{
		CALL(RetryImportDeliveries, locals.retryInput, locals.retryOutput);
		CALL(RetryRefunds, locals.refundInput, locals.refundOutput);
	}

	/**
	 * @brief Settle fees and governance, preserve item approvals, and compact persistent maps when needed.
	 * @note Cleanup preserves keys and values but invalidates listing map-slot cursors.
	 */
	END_EPOCH_WITH_LOCALS()
	{
		// Pay the closing epoch with its existing rates before applying shareholder-approved changes.
		CALL(DistributeFees, locals.distributionInput, locals.distributionOutput);
		CALL(FinalizeShareholderStateVarProposals, input, output);
		state.mut().approvals.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		// Compact after all epoch work: cleanup invalidates map slots, but preserves keys and values.
		// Listing pagination must restart across an epoch boundary; no slot indices are persisted here.
		state.mut().listings.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().importRequests.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().contractRefunds.cleanupIfNeeded(QNFT_MAP_CLEANUP_PERCENT);
		state.mut().refundCursor = 0;
	}

	/**
	 * @brief Register the public ABI by purpose, with issuance procedures first.
	 */
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// Issuance.
		REGISTER_USER_PROCEDURE(CreateCollection, 1);
		REGISTER_USER_PROCEDURE(Mint, 2);
		REGISTER_USER_PROCEDURE(ClaimMint, 3);

		// Collection administration.
		REGISTER_USER_PROCEDURE(NominateAdmin, 4);
		REGISTER_USER_PROCEDURE(AcceptAdmin, 5);

		// Metadata.
		REGISTER_USER_PROCEDURE(UpdateMetadata, 6);
		REGISTER_USER_PROCEDURE(RevealMetadata, 7);
		REGISTER_USER_PROCEDURE(FreezeMetadata, 8);

		// Ownership and approvals.
		REGISTER_USER_PROCEDURE(Transfer, 9);
		REGISTER_USER_PROCEDURE(Approve, 10);
		REGISTER_USER_PROCEDURE(Revoke, 11);
		REGISTER_USER_PROCEDURE(Burn, 12);
		REGISTER_USER_PROCEDURE(Export, 13);

		// One-way QBay import.
		REGISTER_USER_PROCEDURE(PrepareImport, 14);
		REGISTER_USER_PROCEDURE(FinalizeImport, 15);
		REGISTER_USER_PROCEDURE(CancelImport, 16);

		// Shop.
		REGISTER_USER_PROCEDURE(ListForSale, 17);
		REGISTER_USER_PROCEDURE(Buy, 18);
		REGISTER_USER_PROCEDURE(CancelSale, 19);

		// Reserve funding.
		REGISTER_USER_PROCEDURE(FundReserve, 20);

		// Collections, items, ownership and metadata queries.
		REGISTER_USER_FUNCTION(GetCollection, 1);
		REGISTER_USER_FUNCTION(ListCollections, 2);
		REGISTER_USER_FUNCTION(GetItem, 3);
		REGISTER_USER_FUNCTION(GetEventRange, 4);
		REGISTER_USER_FUNCTION(GetEvents, 5);
		REGISTER_USER_FUNCTION(GetImport, 6);
		REGISTER_USER_FUNCTION(ListImportRequests, 7);
		REGISTER_USER_FUNCTION(ListItems, 8);
		REGISTER_USER_FUNCTION(GetBalance, 9);
		REGISTER_USER_FUNCTION(GetApproval, 10);

		// Shop queries.
		REGISTER_USER_FUNCTION(GetListing, 11);
		REGISTER_USER_FUNCTION(ListListings, 12);
		REGISTER_USER_FUNCTION(GetMarket, 13);

		// Configuration, refunds and governance.
		REGISTER_USER_FUNCTION(GetConfig, 14);
		REGISTER_USER_FUNCTION(GetRefund, 15);
		REGISTER_USER_FUNCTION(ListRefunds, 16);
		REGISTER_USER_FUNCTION(GetGovernanceConfig, 17);
		REGISTER_USER_FUNCTION(GetFeeSchedule, 18);

		REGISTER_USER_PROCEDURE(SetFeeProposalBatch, 21);
		REGISTER_USER_FUNCTION(GetFeeProposalBatch, 19);
		REGISTER_SHAREHOLDER_PROPOSAL_VOTING();
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
		state.mut().importFee = QNFT_IMPORT_FEE;
		state.mut().listingFee = QNFT_ACTION_FEE;
		state.mut().proposalFee = QNFT_PROPOSAL_FEE;
		state.mut().shopFeeBPS = QNFT_SHOP_FEE_BPS;
		state.mut().developerFeeBPS = QNFT_DEVELOPER_FEE_BPS;
		state.mut().shareholderFeeBPS = QNFT_SHAREHOLDER_FEE_BPS;
		state.mut().burnFeeBPS = QNFT_BURN_FEE_BPS;
	}
};
