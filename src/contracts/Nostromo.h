using namespace QPI;

namespace QPI
{
	inline bool operator==(const Asset& lhs, const Asset& rhs)
	{
		return lhs.assetName == rhs.assetName && lhs.issuer == rhs.issuer;
	}
} // namespace QPI

constexpr uint64 NOST_AUCTION_NUM = 2048;
constexpr uint64 NOST_AUCTION_METADATA_CID_LENGTH = 64;
constexpr uint64 NOST_AUCTION_PARTICIPANT_NUM = 4096;
constexpr uint64 NOST_AUCTION_LOT_ITEM_NUM = 64;
constexpr uint64 NOST_AUCTION_ALLOWED_WALLET_NUM = 128;
constexpr uint64 NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM = 16;
constexpr uint32 NOST_AUCTION_MAX_DURATION_DAYS = 30;
constexpr sint64 NOST_PRIVATE_AUCTION_FEE = 50000000LL;
constexpr uint64 NOST_AUCTION_CANCELLATION_FEE_BP = 1000ULL;
constexpr uint64 NOST_AUCTION_EXTENSION_SECONDS = 300ULL;
constexpr uint64 NOST_SECONDS_PER_DAY = 86400ULL;
constexpr uint64 NOST_AUCTION_SELLER_DECISION_WINDOW_SECONDS = 604800ULL;

struct NOST2
{
};

struct NOST : public ContractBase
{
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
		PrivateAuctionAccessDenied
	};

	struct AuctionParticipantKey
	{
		id auctionId;
		id participant;

		bool operator==(const AuctionParticipantKey& rhs) const { return auctionId == rhs.auctionId && participant == rhs.participant; }
	};

	/**
	 * @brief Stores the active bid state of one wallet in one auction.
	 * @note The same struct is shared by batch and standard auctions.
	 */
	struct AuctionParticipantData
	{
		/** @brief Amount currently locked in escrow for the participant bid. */
		uint64 escrowedAmount;

		/** @brief Quantity requested by the participant; standard auctions always use the whole lot quantity. */
		uint64 requestedQuantity;

		/** @brief Quantity finally allocated to the participant after batch auction settlement. */
		uint64 allocatedQuantity;

		/** @brief Offered price per asset in a batch auction, or total offered price for the whole lot in a standard auction. */
		uint64 bidAmount;

		/** @brief Timestamp of the participant's latest accepted bid. */
		DateAndTime lastBidTime;

		/** @brief Wallet that owns this participant record. */
		id participant;

		/** @brief Marks bids that remain inside the winning allocation after settlement. */
		uint8 isWinningBid;
	};

	/**
	 * @brief Describes one asset entry inside an auction lot.
	 * @note One non-zero entry means a single-asset auction lot.
	 * @note Multiple non-zero entries mean a bundle of different assets.
	 */
	struct AuctionLotEntry
	{
		/** @brief Asset included in the auction lot. */
		Asset asset;

		/** @brief Quantity of this asset included in the auction lot. */
		sint64 quantity;
	};

	/**
	 * @brief Stores all persistent data for one auction.
	 * @note The same struct is shared by batch and standard auctions.
	 * @note `metadataIpfsCid` points to off-chain auction metadata stored in IPFS.
	 * @note `sellerDecisionDeadline` stays zero until a standard auction enters the manual decision window.
	 */
	struct AuctionData
	{
		/** @brief Unique identifier of the auction created in the Auction House. */
		id auctionId;

		/** @brief Total sale units offered; batch auctions use asset quantity, standard auctions use one unit for the whole lot. */
		uint64 quantityForSale;

		/** @brief Quantity already assigned to winning bids after settlement. */
		uint64 allocatedQuantity;

		/** @brief Minimum quantity a bidder may request in a batch auction; standard auctions always sell the whole lot as one unit. */
		uint64 minimumPurchaseQuantity;

		/** @brief Initial price for a standard auction; bids cannot start below this total price for the whole lot. */
		uint64 initialPrice;

		/** @brief Minimum acceptable price per asset in a batch auction, or desired minimum total selling price for the whole lot in a standard
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

		/** @brief Timestamp when the seller created the auction. */
		DateAndTime createdAt;

		/** @brief Timestamp of the most recent accepted bid. */
		DateAndTime lastBidAt;

		/** @brief Deadline for the seller to accept or reject a standard auction bid that ended between Initial Price and Sale Price. */
		DateAndTime sellerDecisionDeadline;

		/** @brief Timestamp when the auction was finalized, cancelled, or otherwise settled. */
		DateAndTime settledAt;

		/** @brief Wallet that created the auction and offers the lot for sale. */
		id seller;

		/** @brief Wallet that currently holds the highest bid. */
		id highestBidder;

		/** @brief Asset set required for participation when the private auction uses asset-based access. */
		HashSet<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;

		/** @brief Auction lot contents; one non-empty entry means a single asset, multiple non-empty entries mean a bundle. */
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Wallet whitelist used when the private auction uses wallet-based access. */
		HashSet<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/** @brief Lowercase base32 CIDv1 stored in Pinata for the auction name and description metadata. */
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/** @brief Auction House mode: Batch Auction or Standard Auction. */
		EAuctionType type;

		/** @brief Auction visibility: public or restricted private access. */
		EAuctionVisibility visibility;

		/** @brief Current lifecycle status of the auction, including the seller decision phase for standard auctions. */
		EAuctionStatus status;
	};

	struct StateData
	{
		/** @brief Configured fee charged when creating a private auction. */
		sint64 privateAuctionFee;

		/** @brief Configured cancellation fee rate in basis points. */
		uint64 auctionCancellationFeeBasisPoints;

		/** @brief Configured maximum auction duration in days. */
		uint32 maxAuctionDurationDays;

		HashMap<id, AuctionData, NOST_AUCTION_NUM> auctionList;
		HashMap<AuctionParticipantKey, AuctionParticipantData, NOST_AUCTION_PARTICIPANT_NUM> participants;
	};

	/** @brief Input payload used to create a Batch Auction or Standard Auction in the Auction House. */
	struct CreateAuction_input
	{
		/** @brief Lowercase base32 CIDv1 stored in Pinata for the auction name and description metadata. */
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/** @brief Auction lot contents; one non-empty entry means a single asset, multiple non-empty entries mean a bundle. */
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/** @brief Asset list required to participate when the private auction uses asset-based access. */
		Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;

		/** @brief Wallet list used when the private auction restricts participation to predefined wallets. */
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/** @brief Minimum quantity a bidder may request in a batch auction; standard auctions always sell the whole lot as one unit. */
		uint64 minimumPurchaseQuantity;

		/** @brief Initial price for a standard auction; bids cannot be placed below this total price for the whole lot. */
		uint64 initialPrice;

		/** @brief Minimum acceptable price per asset in a batch auction, or desired minimum total selling price for the whole lot in a standard
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
		/** @brief Identifier assigned to the new auction when creation succeeds. */
		id auctionId;

		/** @brief Result code describing whether the auction creation succeeded. */
		uint8 errorCode;
	};

	/** @brief Input payload used to place a bid in a Batch Auction or Standard Auction. */
	struct PlaceBid_input
	{
		/** @brief Identifier of the target auction. */
		id auctionId;

		/** @brief Requested quantity for a batch auction; ignored for a standard auction because the whole lot is sold as one unit. */
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
		uint8 errorCode;
	};

	/** @brief Input payload used to cancel an active auction. */
	struct CancelAuction_input
	{
		/** @brief Identifier of the auction that the seller wants to cancel. */
		id auctionId;
	};

	/** @brief Result of an auction cancellation request. */
	struct CancelAuction_output
	{
		/** @brief Total amount refunded to bidders because of the cancellation. */
		uint64 refundedAmount;

		/** @brief Cancellation fee charged to the seller according to the auction rules. */
		uint64 cancellationFee;

		/** @brief Result code describing whether the cancellation succeeded. */
		uint8 errorCode;
	};

	/** @brief Input payload used to fetch one auction from storage. */
	struct GetAuction_input
	{
		/** @brief Identifier of the auction to read. */
		id auctionId;
	};

	/** @brief Auction data returned by the read-only auction getter. */
	struct GetAuction_output
	{
		/** @brief Persistent auction data stored for the requested auction. */
		AuctionData auction;
	};

	/** @brief Input payload used to fetch one participant record from an auction. */
	struct GetAuctionParticipant_input
	{
		/** @brief Identifier of the auction that owns the participant record. */
		id auctionId;

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

	/** @brief Internal input used to validate an auction lot and resolve its total escrow quantity. */
	struct AnalyzeAuctionLot_input
	{
		/** @brief Auction lot contents to validate. */
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

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
		AuctionLotEntry lotItem;
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
		/** @brief Asset list provided for private asset-based access control. */
		Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAccessAssets;
	};

	/** @brief Internal output containing the number of non-empty private access assets. */
	struct CountRequiredAccessAssets_output
	{
		/** @brief Number of non-zero asset entries found in the private access list. */
		uint64 requiredAccessAssetCount;
	};

	struct CountRequiredAccessAssets_locals
	{
		Asset requiredAccessAsset;
		uint64 requiredAccessAssetIndex;
	};

	/** @brief Internal input used to verify whether the invocator owns at least one required private access asset. */
	struct HasRequiredAccessAsset_input
	{
		/** @brief Auction whose private asset-based access rules should be evaluated. */
		AuctionData auction;
	};

	/** @brief Internal output of the private asset access check. */
	struct HasRequiredAccessAsset_output
	{
		/** @brief Flag indicating whether the invocator owns at least one required access asset. */
		uint8 hasRequiredAccessAsset;
	};

	struct HasRequiredAccessAsset_locals
	{
		Asset requiredAccessAsset;
		sint64 requiredAccessAssetSetIndex;
		sint64 possessedAccessShares;
	};

	/** @brief Internal input used to process a batch auction bid after the common PlaceBid checks succeed. */
	struct ProcessBatchBid_input
	{
		/** @brief Identifier of the target batch auction. */
		id auctionId;

		/** @brief Quantity requested by the bidder in the batch auction. */
		uint64 effectiveQuantity;

		/** @brief Offered price per asset for the requested quantity in the batch auction. */
		uint64 bidAmount;

		/** @brief Timestamp of the accepted bid. */
		DateAndTime currentDate;

		/** @brief Seconds elapsed since auction creation at the moment of the bid. */
		uint64 elapsedSeconds;
	};

	/** @brief Internal output returned after processing a batch auction bid. */
	struct ProcessBatchBid_output
	{
		/** @brief Amount that remains escrowed for the accepted batch bid. */
		uint64 escrowedAmount;

		/** @brief Amount refunded during batch bid processing. */
		uint64 refundedAmount;

		/** @brief Result code describing whether the batch bid processing succeeded. */
		uint8 errorCode;

		/** @brief Flag indicating whether batch bid processing completed successfully. */
		uint8 success;
	};

	struct ProcessBatchBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantKey participantKey;
		uint64 previousEscrow;
		uint64 requiredEscrow;
		uint8 participantExists;
	};

	/** @brief Internal input used to process a standard auction bid after the common PlaceBid checks succeed. */
	struct ProcessStandardBid_input
	{
		/** @brief Identifier of the target standard auction. */
		id auctionId;

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
		uint8 errorCode;

		/** @brief Flag indicating whether standard bid processing completed successfully. */
		uint8 success;
	};

	struct ProcessStandardBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData previousHighestBidderData;
		AuctionParticipantKey participantKey;
		AuctionParticipantKey highestBidderKey;
		uint64 previousEscrow;
		uint64 requiredEscrow;
		uint8 participantExists;
		uint8 highestBidderExists;
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
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	/** @brief Internal output of the seller balance verification routine. */
	struct VerifyAuctionLotBalances_output
	{
		/** @brief Flag indicating whether the seller owns enough shares for the entire lot. */
		uint8 hasEnoughBalance;
	};

	struct VerifyAuctionLotBalances_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
		sint64 possessedShares;
	};

	/** @brief Internal input used to transfer the auction lot from the seller into contract escrow. */
	struct EscrowAuctionLotAssets_input
	{
		/** @brief Auction lot that must be moved into contract escrow. */
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	/** @brief Internal output of the lot escrow routine. */
	struct EscrowAuctionLotAssets_output
	{
		/** @brief Flag indicating whether every lot asset was successfully escrowed. */
		uint8 success;
	};

	struct EscrowAuctionLotAssets_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
		uint64 rollbackLotItemIndex;
		sint64 transferredShares;
	};

	/** @brief Internal input used to roll back an auction lot escrow attempt or to return the lot after cancellation. */
	struct RollbackAuctionLotAssets_input
	{
		/** @brief Auction lot that must be returned from contract escrow to the seller. */
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	typedef NoData RollbackAuctionLotAssets_output;

	struct RollbackAuctionLotAssets_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
	};

	struct CreateAuction_locals
	{
		AuctionData auction;
		ValidateMetadataCid_input validateMetadataCidInput;
		ValidateMetadataCid_output validateMetadataCidOutput;
		AnalyzeAuctionLot_input analyzeAuctionLotInput;
		AnalyzeAuctionLot_output analyzeAuctionLotOutput;
		CountAllowedBidderWallets_input countAllowedBidderWalletsInput;
		CountAllowedBidderWallets_output countAllowedBidderWalletsOutput;
		CountRequiredAccessAssets_input countRequiredAccessAssetsInput;
		CountRequiredAccessAssets_output countRequiredAccessAssetsOutput;
		VerifyAuctionLotBalances_input verifyAuctionLotBalancesInput;
		EscrowAuctionLotAssets_input escrowAuctionLotAssetsInput;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		sint64 requiredFee;
		uint64 resolvedQuantityForSale;
		uint64 resolvedMinimumPurchaseQuantity;
		uint64 allowedWalletIndex;
		uint64 requiredAccessAssetIndex;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		EscrowAuctionLotAssets_output escrowAuctionLotAssetsOutput;
		VerifyAuctionLotBalances_output verifyAuctionLotBalancesOutput;
	};

	struct PlaceBid_locals
	{
		AuctionData auction;
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
		AuctionParticipantData participantData;
		AuctionParticipantKey participantKey;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		DateAndTime currentDate;
		uint64 cancellationBaseAmount;
		sint64 participantIndex;
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
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateAuction, 1);
		REGISTER_USER_PROCEDURE(PlaceBid, 2);
		REGISTER_USER_PROCEDURE(CancelAuction, 3);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 4);

		REGISTER_USER_FUNCTION(GetAuction, 1);
		REGISTER_USER_FUNCTION(GetAuctionParticipant, 2);
	}

	INITIALIZE()
	{
		state.mut().privateAuctionFee = NOST_PRIVATE_AUCTION_FEE;
		state.mut().auctionCancellationFeeBasisPoints = NOST_AUCTION_CANCELLATION_FEE_BP;
		state.mut().maxAuctionDurationDays = NOST_AUCTION_MAX_DURATION_DAYS;
	}

	PRE_ACQUIRE_SHARES()
	{
		output.requestedFee = 0;
		output.allowTransfer = true;
	}

	BEGIN_EPOCH()
	{
		// TODO: Change to valid epoch
		if (qpi.epoch() == 220)
		{
			// Initialize
			state.mut().privateAuctionFee = NOST_PRIVATE_AUCTION_FEE;
			state.mut().auctionCancellationFeeBasisPoints = NOST_AUCTION_CANCELLATION_FEE_BP;
			state.mut().maxAuctionDurationDays = NOST_AUCTION_MAX_DURATION_DAYS;
		}
	}

	END_EPOCH()
	{
		state.mut().auctionList.cleanupIfNeeded();
		state.mut().participants.cleanupIfNeeded();
	}

	PRIVATE_FUNCTION_WITH_LOCALS(AnalyzeAuctionLot)
	{
		output.totalEscrowQuantity = 0;
		output.lotItemCount = 0;
		output.isValid = 0;

		if (input.durationDays == 0 || input.durationDays > state.get().maxAuctionDurationDays)
		{
			return;
		}

		for (locals.lotItemIndex = 0; locals.lotItemIndex < NOST_AUCTION_LOT_ITEM_NUM; ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
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

	PRIVATE_FUNCTION_WITH_LOCALS(CountAllowedBidderWallets)
	{
		output.allowedWalletCount = 0;
		for (locals.allowedWalletIndex = 0; locals.allowedWalletIndex < NOST_AUCTION_ALLOWED_WALLET_NUM; ++locals.allowedWalletIndex)
		{
			if (!isZero(input.allowedBidderWallets.get(locals.allowedWalletIndex)))
			{
				output.allowedWalletCount = sadd(output.allowedWalletCount, 1ULL);
			}
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(CountRequiredAccessAssets)
	{
		output.requiredAccessAssetCount = 0;
		for (locals.requiredAccessAssetIndex = 0; locals.requiredAccessAssetIndex < input.requiredAccessAssets.capacity();
		     ++locals.requiredAccessAssetIndex)
		{
			locals.requiredAccessAsset = input.requiredAccessAssets.get(locals.requiredAccessAssetIndex);
			if (!isZeroAsset(locals.requiredAccessAsset))
			{
				output.requiredAccessAssetCount = sadd(output.requiredAccessAssetCount, 1ULL);
			}
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(HasRequiredAccessAsset)
	{
		output.hasRequiredAccessAsset = 0;
		for (locals.requiredAccessAssetSetIndex = input.auction.requiredAccessAssets.nextElementIndex(NULL_INDEX);
		     locals.requiredAccessAssetSetIndex != NULL_INDEX;
		     locals.requiredAccessAssetSetIndex = input.auction.requiredAccessAssets.nextElementIndex(locals.requiredAccessAssetSetIndex))
		{
			locals.requiredAccessAsset = input.auction.requiredAccessAssets.key(locals.requiredAccessAssetSetIndex);
			locals.possessedAccessShares = qpi.numberOfShares(locals.requiredAccessAsset, AssetOwnershipSelect::byOwner(qpi.invocator()),
			                                                  AssetPossessionSelect::byPossessor(qpi.invocator()));
			if (locals.possessedAccessShares > 0)
			{
				output.hasRequiredAccessAsset = 1;
				return;
			}
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessBatchBid)
	{
		output.escrowedAmount = 0;
		output.refundedAmount = 0;
		output.errorCode = static_cast<uint8>(EAuctionError::Success);
		output.success = 0;
		if (!state.get().auctionList.get(input.auctionId, locals.auction))
		{
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionNotFound);
			return;
		}

		if (input.effectiveQuantity == 0 || input.effectiveQuantity < locals.auction.minimumPurchaseQuantity || input.bidAmount == 0)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);
			return;
		}

		if (input.bidAmount < locals.auction.salePrice)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
			return;
		}

		locals.requiredEscrow = smul(input.effectiveQuantity, input.bidAmount);
		if (static_cast<uint64>(qpi.invocationReward()) < locals.requiredEscrow)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientFunds);
			return;
		}

		locals.participantKey = {input.auctionId, qpi.invocator()};
		locals.participantExists = state.get().participants.get(locals.participantKey, locals.participantData);
		locals.previousEscrow = locals.participantExists ? locals.participantData.escrowedAmount : 0;

		locals.participantData.escrowedAmount = locals.requiredEscrow;
		locals.participantData.requestedQuantity = input.effectiveQuantity;
		locals.participantData.allocatedQuantity = 0;
		locals.participantData.bidAmount = input.bidAmount;
		locals.participantData.lastBidTime = input.currentDate;
		locals.participantData.participant = qpi.invocator();
		locals.participantData.isWinningBid = 0;

		if (input.bidAmount > locals.auction.highestBidPrice)
		{
			locals.auction.highestBidder = qpi.invocator();
			locals.auction.highestBidPrice = input.bidAmount;
			locals.auction.highestBidQuantity = input.effectiveQuantity;
			locals.auction.highestBidAmount = locals.requiredEscrow;
		}

		locals.auction.lastBidAt = input.currentDate;
		if ((locals.auction.auctionDurationSeconds - input.elapsedSeconds) <= NOST_AUCTION_EXTENSION_SECONDS)
		{
			locals.auction.auctionDurationSeconds = sadd(locals.auction.auctionDurationSeconds, NOST_AUCTION_EXTENSION_SECONDS);
		}

		if (state.mut().participants.set(locals.participantKey, locals.participantData) == NULL_INDEX)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::StorageFull);
			return;
		}
		state.mut().auctionList.replace(input.auctionId, locals.auction);

		if (locals.previousEscrow > 0)
		{
			qpi.transfer(qpi.invocator(), locals.previousEscrow);
			output.refundedAmount = sadd(output.refundedAmount, locals.previousEscrow);
		}
		if (static_cast<uint64>(qpi.invocationReward()) > locals.requiredEscrow)
		{
			qpi.transfer(qpi.invocator(), static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow);
			output.refundedAmount = sadd(output.refundedAmount, static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow);
		}

		output.escrowedAmount = locals.requiredEscrow;
		output.success = 1;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessStandardBid)
	{
		output.escrowedAmount = 0;
		output.refundedAmount = 0;
		output.errorCode = static_cast<uint8>(EAuctionError::Success);
		output.success = 0;
		locals.highestBidderExists = 0;
		if (!state.get().auctionList.get(input.auctionId, locals.auction))
		{
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionNotFound);
			return;
		}

		if (locals.auction.quantityForSale == 0 || locals.auction.quantityForSale < locals.auction.minimumPurchaseQuantity || input.bidAmount == 0)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);
			return;
		}

		locals.requiredEscrow = input.bidAmount;
		if (static_cast<uint64>(qpi.invocationReward()) < locals.requiredEscrow)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientFunds);
			return;
		}

		if (locals.auction.highestBidPrice == 0)
		{
			if (input.bidAmount < locals.auction.initialPrice)
			{
				output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
				return;
			}
		}
		else if (input.bidAmount < sadd(locals.auction.highestBidPrice, locals.auction.minimumBidIncrement))
		{
			output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
			return;
		}

		locals.participantKey = {input.auctionId, qpi.invocator()};
		locals.participantExists = state.get().participants.get(locals.participantKey, locals.participantData);
		locals.previousEscrow = locals.participantExists ? locals.participantData.escrowedAmount : 0;

		locals.participantData.escrowedAmount = locals.requiredEscrow;
		locals.participantData.requestedQuantity = locals.auction.quantityForSale;
		locals.participantData.allocatedQuantity = 0;
		locals.participantData.bidAmount = input.bidAmount;
		locals.participantData.lastBidTime = input.currentDate;
		locals.participantData.participant = qpi.invocator();
		locals.participantData.isWinningBid = 0;

		if (!isZero(locals.auction.highestBidder))
		{
			locals.highestBidderKey = {locals.auction.auctionId, locals.auction.highestBidder};
			locals.highestBidderExists = state.get().participants.get(locals.highestBidderKey, locals.previousHighestBidderData);
		}
		if (locals.highestBidderExists && locals.previousHighestBidderData.participant != qpi.invocator())
		{
			qpi.transfer(locals.previousHighestBidderData.participant, locals.previousHighestBidderData.escrowedAmount);
			output.refundedAmount = sadd(output.refundedAmount, locals.previousHighestBidderData.escrowedAmount);
			locals.previousHighestBidderData.escrowedAmount = 0;
			locals.previousHighestBidderData.isWinningBid = 0;
			state.mut().participants.replace(locals.highestBidderKey, locals.previousHighestBidderData);
		}

		locals.participantData.isWinningBid = 1;
		locals.auction.highestBidder = qpi.invocator();
		locals.auction.highestBidPrice = input.bidAmount;
		locals.auction.highestBidQuantity = locals.auction.quantityForSale;
		locals.auction.highestBidAmount = locals.requiredEscrow;

		locals.auction.lastBidAt = input.currentDate;
		if ((locals.auction.auctionDurationSeconds - input.elapsedSeconds) <= NOST_AUCTION_EXTENSION_SECONDS)
		{
			locals.auction.auctionDurationSeconds = sadd(locals.auction.auctionDurationSeconds, NOST_AUCTION_EXTENSION_SECONDS);
		}
		if (locals.auction.buyNowPrice > 0 && input.bidAmount >= locals.auction.buyNowPrice)
		{
			locals.auction.status = EAuctionStatus::Finalized;
			locals.auction.settledAt = input.currentDate;
			locals.participantData.isWinningBid = 1;
		}

		if (state.mut().participants.set(locals.participantKey, locals.participantData) == NULL_INDEX)
		{
			output.errorCode = static_cast<uint8>(EAuctionError::StorageFull);
			return;
		}
		state.mut().auctionList.replace(input.auctionId, locals.auction);

		if (locals.previousEscrow > 0)
		{
			qpi.transfer(qpi.invocator(), locals.previousEscrow);
			output.refundedAmount = sadd(output.refundedAmount, locals.previousEscrow);
		}
		if (static_cast<uint64>(qpi.invocationReward()) > locals.requiredEscrow)
		{
			qpi.transfer(qpi.invocator(), static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow);
			output.refundedAmount = sadd(output.refundedAmount, static_cast<uint64>(qpi.invocationReward()) - locals.requiredEscrow);
		}

		output.escrowedAmount = locals.requiredEscrow;
		output.success = 1;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateMetadataCid)
	{
		output.isValid = 0;
		locals.hasPayloadCharacters = 0;
		locals.reachedTerminator = 0;

		if (input.metadataIpfsCid.get(0) != 'b')
		{
			return;
		}

		for (locals.cidIndex = 1; locals.cidIndex < NOST_AUCTION_METADATA_CID_LENGTH; ++locals.cidIndex)
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

			if ((locals.cidChar >= 'a' && locals.cidChar <= 'z') || (locals.cidChar >= '2' && locals.cidChar <= '7'))
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

	PRIVATE_FUNCTION_WITH_LOCALS(VerifyAuctionLotBalances)
	{
		output.hasEnoughBalance = 1;
		for (locals.lotItemIndex = 0; locals.lotItemIndex < NOST_AUCTION_LOT_ITEM_NUM; ++locals.lotItemIndex)
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

	PRIVATE_PROCEDURE_WITH_LOCALS(RollbackAuctionLotAssets)
	{
		for (locals.lotItemIndex = 0; locals.lotItemIndex < NOST_AUCTION_LOT_ITEM_NUM; ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
			{
				continue;
			}
			qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer, SELF, SELF, locals.lotItem.quantity,
			                                        qpi.invocator());
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(EscrowAuctionLotAssets)
	{
		output.success = 1;
		for (locals.lotItemIndex = 0; locals.lotItemIndex < NOST_AUCTION_LOT_ITEM_NUM; ++locals.lotItemIndex)
		{
			locals.lotItem = input.auctionLotItems.get(locals.lotItemIndex);
			if (isZeroAsset(locals.lotItem.asset) || locals.lotItem.quantity <= 0)
			{
				continue;
			}

			locals.transferredShares = qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer,
			                                                                   qpi.invocator(), qpi.invocator(), locals.lotItem.quantity, SELF);
			if (locals.transferredShares < locals.lotItem.quantity)
			{
				if (locals.transferredShares > 0)
				{
					qpi.transferShareOwnershipAndPossession(locals.lotItem.asset.assetName, locals.lotItem.asset.issuer, SELF, SELF,
					                                        locals.transferredShares, qpi.invocator());
				}
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

	PUBLIC_PROCEDURE_WITH_LOCALS(CreateAuction)
	{
		output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);

		if (!isSupportedAuctionType(static_cast<EAuctionType>(input.auctionType)))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InvalidAuctionType);
			return;
		}

		if (!isSupportedAuctionVisibility(static_cast<EAuctionVisibility>(input.auctionVisibility)))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InvalidVisibility);
			return;
		}

		if (state.get().auctionList.population() >= state.get().auctionList.capacity())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::StorageFull);
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
			return;
		}
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
					return;
				}
				break;
			case EAuctionType::Standard:
				if (!resolveStandardAuctionCreateParams(input.minimumPurchaseQuantity, input.minimumBidIncrement, locals.resolvedQuantityForSale,
				                                        locals.resolvedMinimumPurchaseQuantity, input.buyNowPrice, input.initialPrice,
				                                        input.salePrice))
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return;
				}
				break;
			default:
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = static_cast<uint8>(EAuctionError::InvalidAuctionType);
				return;
		}

		locals.countAllowedBidderWalletsInput.allowedBidderWallets = input.allowedBidderWallets;
		CALL(CountAllowedBidderWallets, locals.countAllowedBidderWalletsInput, locals.countAllowedBidderWalletsOutput);
		locals.countRequiredAccessAssetsInput.requiredAccessAssets = input.requiredAccessAssets;
		CALL(CountRequiredAccessAssets, locals.countRequiredAccessAssetsInput, locals.countRequiredAccessAssetsOutput);
		if (!validatePrivateAuctionAccess(static_cast<EAuctionVisibility>(input.auctionVisibility),
		                                  locals.countRequiredAccessAssetsOutput.requiredAccessAssetCount,
		                                  locals.countAllowedBidderWalletsOutput.allowedWalletCount))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		locals.requiredFee = getCreateAuctionFee(static_cast<EAuctionVisibility>(input.auctionVisibility), state);
		if (qpi.invocationReward() < locals.requiredFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientFunds);
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
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientAssetBalance);
			return;
		}

		locals.escrowAuctionLotAssetsInput.auctionLotItems = input.auctionLotItems;
		CALL(EscrowAuctionLotAssets, locals.escrowAuctionLotAssetsInput, locals.escrowAuctionLotAssetsOutput);
		if (!locals.escrowAuctionLotAssetsOutput.success)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientAssetBalance);
			return;
		}

		locals.auction.auctionId = id::randomValue();
		locals.auction.quantityForSale = locals.resolvedQuantityForSale;
		locals.auction.minimumPurchaseQuantity = locals.resolvedMinimumPurchaseQuantity;
		locals.auction.initialPrice = input.initialPrice;
		locals.auction.salePrice = input.salePrice;
		locals.auction.minimumBidIncrement = input.minimumBidIncrement;
		locals.auction.buyNowPrice = input.buyNowPrice;
		locals.auction.auctionDurationSeconds = smul(static_cast<uint64>(input.durationDays), NOST_SECONDS_PER_DAY);
		locals.auction.createdAt = qpi.now();
		locals.auction.lastBidAt = locals.auction.createdAt;
		locals.auction.seller = qpi.invocator();
		for (locals.requiredAccessAssetIndex = 0; locals.requiredAccessAssetIndex < input.requiredAccessAssets.capacity();
		     ++locals.requiredAccessAssetIndex)
		{
			if (!isZeroAsset(input.requiredAccessAssets.get(locals.requiredAccessAssetIndex)))
			{
				locals.auction.requiredAccessAssets.add(input.requiredAccessAssets.get(locals.requiredAccessAssetIndex));
			}
		}
		locals.auction.auctionLotItems = input.auctionLotItems;
		for (locals.allowedWalletIndex = 0; locals.allowedWalletIndex < NOST_AUCTION_ALLOWED_WALLET_NUM; ++locals.allowedWalletIndex)
		{
			if (!isZero(input.allowedBidderWallets.get(locals.allowedWalletIndex)))
			{
				locals.auction.allowedBidderWallets.add(input.allowedBidderWallets.get(locals.allowedWalletIndex));
			}
		}
		locals.auction.metadataIpfsCid = input.metadataIpfsCid;
		locals.auction.type = static_cast<EAuctionType>(input.auctionType);
		locals.auction.visibility = static_cast<EAuctionVisibility>(input.auctionVisibility);
		locals.auction.status = EAuctionStatus::Active;

		if (state.mut().auctionList.set(locals.auction.auctionId, locals.auction) == NULL_INDEX)
		{
			locals.rollbackAuctionLotAssetsInput.auctionLotItems = input.auctionLotItems;
			CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::StorageFull);
			return;
		}

		if (qpi.invocationReward() > locals.requiredFee)
		{
			qpi.transfer(qpi.invocator(), static_cast<uint64>(qpi.invocationReward()) - locals.requiredFee);
		}

		output.auctionId = locals.auction.auctionId;
		output.errorCode = static_cast<uint8>(EAuctionError::Success);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(PlaceBid)
	{
		output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);

		if (!state.get().auctionList.get(input.auctionId, locals.auction))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionNotFound);
			return;
		}

		if (locals.auction.status != EAuctionStatus::Active)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionClosed);
			return;
		}

		if (locals.auction.seller == qpi.invocator())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::Forbidden);
			return;
		}

		locals.currentDate = qpi.now();
		diffDateInSecond(locals.auction.createdAt, locals.currentDate, locals.elapsedSeconds);
		if (locals.elapsedSeconds >= locals.auction.auctionDurationSeconds)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionClosed);
			return;
		}

		if (locals.auction.visibility == EAuctionVisibility::Private)
		{
			if (locals.auction.requiredAccessAssets.population() > 0)
			{
				locals.hasRequiredAccessAssetInput.auction = locals.auction;
				CALL(HasRequiredAccessAsset, locals.hasRequiredAccessAssetInput, locals.hasRequiredAccessAssetOutput);
				locals.hasAccess = locals.hasRequiredAccessAssetOutput.hasRequiredAccessAsset;
			}
			else
			{
				locals.hasAccess = locals.auction.allowedBidderWallets.contains(qpi.invocator());
			}

			if (!locals.hasAccess)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = static_cast<uint8>(EAuctionError::PrivateAuctionAccessDenied);
				return;
			}
		}

		switch (locals.auction.type)
		{
			case EAuctionType::Batch:
				locals.processBatchBidInput.auctionId = input.auctionId;
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
					output.errorCode = locals.processBatchBidOutput.errorCode;
					return;
				}
				output.refundedAmount = sadd(output.refundedAmount, locals.processBatchBidOutput.refundedAmount);
				output.escrowedAmount = locals.processBatchBidOutput.escrowedAmount;
				break;
			case EAuctionType::Standard:
				locals.processStandardBidInput.auctionId = input.auctionId;
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
				output.errorCode = static_cast<uint8>(EAuctionError::InvalidAuctionType);
				return;
		}
		output.errorCode = static_cast<uint8>(EAuctionError::Success);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(CancelAuction)
	{
		output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);

		if (!state.get().auctionList.get(input.auctionId, locals.auction))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionNotFound);
			return;
		}

		if (locals.auction.status != EAuctionStatus::Active)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::AuctionClosed);
			return;
		}

		if (locals.auction.seller != qpi.invocator())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::Forbidden);
			return;
		}

		locals.cancellationBaseAmount = max(locals.auction.highestBidAmount, locals.auction.salePrice);
		if (locals.auction.type == EAuctionType::Batch)
		{
			locals.cancellationBaseAmount = max(locals.auction.highestBidAmount, smul(locals.auction.salePrice, locals.auction.quantityForSale));
		}
		output.cancellationFee = div<uint64>(smul(locals.cancellationBaseAmount, state.get().auctionCancellationFeeBasisPoints), 10000ULL);

		if (static_cast<uint64>(qpi.invocationReward()) < output.cancellationFee)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientFunds);
			return;
		}

		locals.participantIndex = state.get().participants.nextElementIndex(NULL_INDEX);
		while (locals.participantIndex != NULL_INDEX)
		{
			locals.participantKey = state.get().participants.key(locals.participantIndex);
			if (locals.participantKey.auctionId == input.auctionId)
			{
				locals.participantData = state.get().participants.value(locals.participantIndex);
				if (locals.participantData.escrowedAmount > 0)
				{
					qpi.transfer(locals.participantData.participant, locals.participantData.escrowedAmount);
					output.refundedAmount = sadd(output.refundedAmount, locals.participantData.escrowedAmount);
				}
				state.mut().participants.removeByKey(locals.participantKey);
			}

			locals.participantIndex = state.get().participants.nextElementIndex(locals.participantIndex);
		}
		state.mut().participants.cleanupIfNeeded();

		locals.rollbackAuctionLotAssetsInput.auctionLotItems = locals.auction.auctionLotItems;
		CALL(RollbackAuctionLotAssets, locals.rollbackAuctionLotAssetsInput, locals.rollbackAuctionLotAssetsOutput);

		locals.currentDate = qpi.now();
		locals.auction.status = EAuctionStatus::Cancelled;
		locals.auction.settledAt = locals.currentDate;
		state.mut().auctionList.replace(input.auctionId, locals.auction);

		if (static_cast<uint64>(qpi.invocationReward()) > output.cancellationFee)
		{
			qpi.transfer(qpi.invocator(), static_cast<uint64>(qpi.invocationReward()) - output.cancellationFee);
		}

		output.errorCode = static_cast<uint8>(EAuctionError::Success);
	}

	PUBLIC_FUNCTION(GetAuction) { state.get().auctionList.get(input.auctionId, output.auction); }

	PUBLIC_FUNCTION(GetAuctionParticipant)
	{
		output.found = state.get().participants.get({input.auctionId, input.participant}, output.participantData) ? 1 : 0;
	}

	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.numberOfShares <= 0 || input.asset.assetName == 0 || input.newManagingContractIndex == 0)
		{
			output.transferredNumberOfShares = 0;
			return;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) <
		    input.numberOfShares)
		{
			output.transferredNumberOfShares = 0;
			return;
		}

		if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
		                      input.newManagingContractIndex, 0) < 0)
		{
			// error
			output.transferredNumberOfShares = 0;
			return;
		}

		// success
		output.transferredNumberOfShares = input.numberOfShares;
	}

protected:
	template<typename T>
	static constexpr T min(const T& a, const T& b)
	{
		return (a < b) ? a : b;
	}
	template<typename T>
	static constexpr T max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}
	static bool resolveBatchAuctionCreateParams(uint64 lotItemCount, uint64 totalEscrowQuantity, uint64 minimumPurchaseQuantity,
	                                            uint64& quantityForSale, uint64& resolvedMinimumPurchaseQuantity, uint64 buyNowPrice)
	{
		quantityForSale = 0;
		resolvedMinimumPurchaseQuantity = 0;
		if (lotItemCount != 1 || minimumPurchaseQuantity == 0 || minimumPurchaseQuantity > totalEscrowQuantity || buyNowPrice != 0)
		{
			return false;
		}
		quantityForSale = totalEscrowQuantity;
		resolvedMinimumPurchaseQuantity = minimumPurchaseQuantity;
		return true;
	}

	static bool resolveStandardAuctionCreateParams(uint64 minimumPurchaseQuantity, uint64 minimumBidIncrement, uint64& quantityForSale,
	                                               uint64& resolvedMinimumPurchaseQuantity, uint64 buyNowPrice, uint64 initialPrice, uint64 salePrice)
	{
		quantityForSale = 0;
		resolvedMinimumPurchaseQuantity = 0;
		if (minimumPurchaseQuantity > 1 || minimumBidIncrement == 0 || salePrice == 0)
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

		quantityForSale = 1;
		resolvedMinimumPurchaseQuantity = 1;
		return true;
	}

	static bool validatePrivateAuctionAccess(EAuctionVisibility visibility, uint64 requiredAccessAssetCount, uint64 allowedWalletCount)
	{
		return visibility != EAuctionVisibility::Private || ((requiredAccessAssetCount > 0) != (allowedWalletCount > 0));
	}

	static sint64 getCreateAuctionFee(EAuctionVisibility visibility, const ContractState<StateData, CONTRACT_INDEX>& state)
	{
		return visibility == EAuctionVisibility::Private ? state.get().privateAuctionFee : 0;
	}

	static bool isSupportedAuctionType(EAuctionType auctionType)
	{
		return auctionType == EAuctionType::Batch || auctionType == EAuctionType::Standard;
	}

	static bool isSupportedAuctionVisibility(EAuctionVisibility visibility)
	{
		return visibility == EAuctionVisibility::Public || visibility == EAuctionVisibility::Private;
	}

	static bool isZeroAsset(const Asset& asset) { return asset.assetName == 0 && isZero(asset.issuer); }

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
		res = div<uint64>(a.durationMicrosec(b), 1000000ULL);
	}
};
