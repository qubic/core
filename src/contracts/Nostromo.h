using namespace QPI;

constexpr uint64 NOST_AUCTION_NUM = 2048;
constexpr uint64 NOST_AUCTION_METADATA_CID_LENGTH = 64;
constexpr uint64 NOST_AUCTION_PARTICIPANT_NUM = 4096;
constexpr uint64 NOST_AUCTION_LOT_ITEM_NUM = 64;
constexpr uint64 NOST_AUCTION_ALLOWED_WALLET_NUM = 128;
constexpr uint64 NOST_PRIVATE_AUCTION_FEE = 50000000ULL;
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

		bool operator<(const AuctionParticipantKey& rhs) const
		{
			if (auctionId < rhs.auctionId)
			{
				return true;
			}
			if (rhs.auctionId < auctionId)
			{
				return false;
			}
			return participant < rhs.participant;
		}
	};

	/**
	 * @brief Stores the active bid state of one wallet in one auction.
	 * @note The same struct is shared by batch and standard auctions.
	 */
	struct AuctionParticipantData
	{
		uint64 escrowedAmount;
		uint64 requestedQuantity;
		uint64 allocatedQuantity;
		uint64 pricePerUnit;
		DateAndTime lastBidTime;
		id participant;
		uint8 isHighestBidder;
		uint8 isWinningBid;
	};

	/**
	 * @brief Describes one asset entry inside an auction lot.
	 * @note One non-zero entry means a single-asset auction lot.
	 * @note Multiple non-zero entries mean a bundle of different assets.
	 */
	struct AuctionLotEntry
	{
		/// Asset included in the lot.
		Asset asset;

		/// Quantity of this asset included in the lot.
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
		/// Unique identifier of the auction created in the Auction House.
		id auctionId;

		/// Total sale units offered in this auction; batch auctions use asset quantity, standard auctions use one unit for the whole lot.
		uint64 quantityForSale;

		/// Quantity already assigned to winning bids after settlement.
		uint64 allocatedQuantity;

		/// Minimum quantity a bidder may request in a batch auction; standard auctions sell the entire lot as one unit.
		uint64 minimumPurchaseQuantity;

		/// Initial Price for a standard auction; bids cannot start below this value.
		uint64 initialPricePerUnit;

		/// Sale Price defined by the seller as the desired / minimum acceptable selling price.
		uint64 salePricePerUnit;

		/// Minimum step by which a new bid must exceed the current highest bid.
		uint64 minimumBidIncrement;

		/// Buy Now price that closes a standard auction immediately when matched or exceeded.
		uint64 buyNowPricePerUnit;

		/// Highest price per unit currently offered by any active bid.
		uint64 highestBidPerUnit;

		/// Quantity requested by the current highest bid.
		uint64 highestBidQuantity;

		/// Total amount escrowed by the current highest bid.
		uint64 highestBidAmount;

		/// Auction duration in seconds, derived from the duration configured in days.
		uint64 auctionDurationSeconds;

		/// Timestamp when the seller created the auction.
		DateAndTime createdAt;

		/// Timestamp of the most recent accepted bid.
		DateAndTime lastBidAt;

		/// Deadline for the seller to manually accept or reject a bid after auction end when the highest bid is between Initial Price and Sale Price.
		DateAndTime sellerDecisionDeadline;

		/// Timestamp when the auction was finalized, cancelled, or otherwise settled.
		DateAndTime settledAt;

		/// Number of distinct bidders who have placed bids in this auction.
		uint32 bidderCount;

		/// Wallet that created the auction and offers the asset for sale.
		id seller;

		/// Wallet that currently holds the highest bid.
		id highestBidder;

		/// Primary asset reference of the lot, equal to the first non-empty lot entry.
		Asset assetForSale;

		/// Asset required for participation when the auction visibility is private.
		Asset requiredAccessAsset;

		/// Auction lot contents; one non-empty entry means a single asset, multiple non-empty entries mean a bundle.
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/// Wallet whitelist for private batch auctions; only these wallets may participate when wallet-based restriction is used.
		HashSet<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/// IPFS CID stored in Pinata that points to the auction name and description metadata.
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/// Auction House mode: Batch Auction or Standard Auction.
		EAuctionType type;

		/// Auction visibility: public or restricted private access.
		EAuctionVisibility visibility;

		/// Current lifecycle status of the auction, including the manual seller-decision phase.
		EAuctionStatus status;
	};

	struct StateData
	{
		HashMap<id, AuctionData, NOST_AUCTION_NUM> auctionList;
		HashMap<AuctionParticipantKey, AuctionParticipantData, NOST_AUCTION_PARTICIPANT_NUM> participants;
	};

	struct CreateAuction_input
	{
		/// IPFS CID stored in Pinata that points to the auction name and description metadata.
		Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> metadataIpfsCid;

		/// Auction lot contents; one non-empty entry means a single asset, multiple non-empty entries mean a bundle.
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;

		/// Asset required to participate when the auction is configured as private and asset-based access is used.
		Asset requiredAccessAsset;

		/// Wallet list for private batch auctions; copied into the auction whitelist on creation.
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;

		/// Minimum quantity a bidder may request in a batch auction; standard auctions sell the whole lot as one unit.
		uint64 minimumPurchaseQuantity;

		/// Initial Price for a standard auction; bids cannot be placed below this value.
		uint64 initialPricePerUnit;

		/// Sale Price defined by the seller as the desired / minimum acceptable selling price.
		uint64 salePricePerUnit;

		/// Minimum step by which each new bid must exceed the current highest bid.
		uint64 minimumBidIncrementPerUnit;

		/// Buy Now price that immediately closes a standard auction once matched or exceeded.
		uint64 buyNowPricePerUnit;

		/// Auction duration configured by the seller in days.
		uint32 durationDays;

		/// Auction House mode selected by the seller: Batch Auction or Standard Auction.
		uint8 auctionType;

		/// Visibility selected by the seller: public or private.
		uint8 auctionVisibility;
	};

	struct CreateAuction_output
	{
		id auctionId;
		uint8 errorCode;
	};

	struct PlaceBid_input
	{
		id auctionId;
		uint64 quantity;
		uint64 pricePerUnit;
	};

	struct PlaceBid_output
	{
		uint64 escrowedAmount;
		uint64 refundedAmount;
		uint8 errorCode;
	};

	struct GetAuction_input
	{
		id auctionId;
	};

	struct GetAuction_output
	{
		AuctionData auction;
	};

	struct GetAuctionParticipant_input
	{
		id auctionId;
		id participant;
	};

	struct GetAuctionParticipant_output
	{
		AuctionParticipantData participantData;
		uint8 found;
	};

	struct ResolveBatchAuctionCreateParams_input
	{
		uint64 lotItemCount;
		uint64 totalEscrowQuantity;
		uint64 minimumPurchaseQuantity;
	};

	struct ResolveBatchAuctionCreateParams_output
	{
		uint64 quantityForSale;
		uint64 minimumPurchaseQuantity;
		uint8 isValid;
	};

	struct ResolveBatchAuctionCreateParams_locals
	{
	};

	struct ResolveStandardAuctionCreateParams_input
	{
		uint64 minimumPurchaseQuantity;
		uint64 minimumBidIncrementPerUnit;
	};

	struct ResolveStandardAuctionCreateParams_output
	{
		uint64 quantityForSale;
		uint64 minimumPurchaseQuantity;
		uint8 isValid;
	};

	struct ResolveStandardAuctionCreateParams_locals
	{
	};

	struct AnalyzeAuctionLot_input
	{
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
		uint32 durationDays;
	};

	struct AnalyzeAuctionLot_output
	{
		uint64 totalEscrowQuantity;
		uint64 lotItemCount;
		AuctionLotEntry firstLotItem;
		uint8 isValid;
	};

	struct AnalyzeAuctionLot_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
	};

	struct CountAllowedBidderWallets_input
	{
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedBidderWallets;
	};

	struct CountAllowedBidderWallets_output
	{
		uint64 allowedWalletCount;
	};

	struct CountAllowedBidderWallets_locals
	{
		uint64 allowedWalletIndex;
	};

	struct ValidatePrivateAuctionAccess_input
	{
		EAuctionVisibility visibility;
		Asset requiredAccessAsset;
		uint64 allowedWalletCount;
	};

	struct ValidatePrivateAuctionAccess_output
	{
		uint8 isValid;
	};

	struct ValidatePrivateAuctionAccess_locals
	{
	};

	struct GetCreateAuctionFee_input
	{
		EAuctionVisibility visibility;
	};

	struct GetCreateAuctionFee_output
	{
		sint64 requiredFee;
	};

	struct GetCreateAuctionFee_locals
	{
	};

	struct VerifyAuctionLotBalances_input
	{
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	struct VerifyAuctionLotBalances_output
	{
		uint8 hasEnoughBalance;
	};

	struct VerifyAuctionLotBalances_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
		sint64 possessedShares;
	};

	struct EscrowAuctionLotAssets_input
	{
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	struct EscrowAuctionLotAssets_output
	{
		uint8 success;
	};

	struct EscrowAuctionLotAssets_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
		uint64 rollbackLotItemIndex;
		sint64 transferredShares;
	};

	struct RollbackAuctionLotAssets_input
	{
		Array<AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> auctionLotItems;
	};

	struct RollbackAuctionLotAssets_output
	{
	};

	struct RollbackAuctionLotAssets_locals
	{
		AuctionLotEntry lotItem;
		uint64 lotItemIndex;
	};

	struct BuildAuctionData_input
	{
		CreateAuction_input createInput;
		AuctionLotEntry firstLotItem;
		EAuctionType auctionType;
		EAuctionVisibility visibility;
		uint64 quantityForSale;
		uint64 minimumPurchaseQuantity;
	};

	struct BuildAuctionData_output
	{
		AuctionData auction;
	};

	struct BuildAuctionData_locals
	{
		uint64 allowedWalletIndex;
	};

	struct CreateAuction_locals
	{
		AuctionData auction;
		AnalyzeAuctionLot_input analyzeAuctionLotInput;
		AnalyzeAuctionLot_output analyzeAuctionLotOutput;
		CountAllowedBidderWallets_input countAllowedBidderWalletsInput;
		CountAllowedBidderWallets_output countAllowedBidderWalletsOutput;
		ValidatePrivateAuctionAccess_input validatePrivateAuctionAccessInput;
		ValidatePrivateAuctionAccess_output validatePrivateAuctionAccessOutput;
		GetCreateAuctionFee_input getCreateAuctionFeeInput;
		GetCreateAuctionFee_output getCreateAuctionFeeOutput;
		VerifyAuctionLotBalances_input verifyAuctionLotBalancesInput;
		VerifyAuctionLotBalances_output verifyAuctionLotBalancesOutput;
		EscrowAuctionLotAssets_input escrowAuctionLotAssetsInput;
		EscrowAuctionLotAssets_output escrowAuctionLotAssetsOutput;
		RollbackAuctionLotAssets_input rollbackAuctionLotAssetsInput;
		RollbackAuctionLotAssets_output rollbackAuctionLotAssetsOutput;
		BuildAuctionData_input buildAuctionDataInput;
		BuildAuctionData_output buildAuctionDataOutput;
		ResolveBatchAuctionCreateParams_input resolveBatchParamsInput;
		ResolveBatchAuctionCreateParams_output resolveBatchParamsOutput;
		ResolveStandardAuctionCreateParams_input resolveStandardParamsInput;
		ResolveStandardAuctionCreateParams_output resolveStandardParamsOutput;
		EAuctionType auctionType;
		EAuctionVisibility visibility;
		sint64 requiredFee;
		uint64 resolvedQuantityForSale;
		uint64 resolvedMinimumPurchaseQuantity;
	};

	struct PlaceBid_locals
	{
		AuctionData auction;
		AuctionParticipantData participantData;
		AuctionParticipantData previousHighestBidderData;
		AuctionParticipantKey participantKey;
		AuctionParticipantKey highestBidderKey;
		uint64 elapsedSeconds;
		uint64 requiredEscrow;
		uint64 previousEscrow;
		uint64 effectiveQuantity;
		DateAndTime currentDate;
		bool participantExists;
		bool highestBidderExists;
	};

	struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateAuction, 1);
		REGISTER_USER_PROCEDURE(PlaceBid, 2);
		REGISTER_USER_FUNCTION(GetAuction, 1);
		REGISTER_USER_FUNCTION(GetAuctionParticipant, 2);

		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 1);
	}

	INITIALIZE() {}

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
		}
	}

	END_EPOCH()
	{
		state.mut().auctionList.cleanupIfNeeded();
		state.mut().participants.cleanupIfNeeded();
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ResolveBatchAuctionCreateParams)
	{
		output.quantityForSale = 0;
		output.minimumPurchaseQuantity = 0;
		output.isValid = 0;

		if (input.lotItemCount != 1 || input.minimumPurchaseQuantity == 0 || input.minimumPurchaseQuantity > input.totalEscrowQuantity)
		{
			return;
		}

		output.quantityForSale = input.totalEscrowQuantity;
		output.minimumPurchaseQuantity = input.minimumPurchaseQuantity;
		output.isValid = 1;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ResolveStandardAuctionCreateParams)
	{
		output.quantityForSale = 0;
		output.minimumPurchaseQuantity = 0;
		output.isValid = 0;

		if (input.minimumPurchaseQuantity > 1 || input.minimumBidIncrementPerUnit == 0)
		{
			return;
		}

		output.quantityForSale = 1;
		output.minimumPurchaseQuantity = 1;
		output.isValid = 1;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(AnalyzeAuctionLot)
	{
		output.totalEscrowQuantity = 0;
		output.lotItemCount = 0;
		output.isValid = 0;

		if (input.durationDays == 0)
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

			if (output.lotItemCount == 0)
			{
				output.firstLotItem = locals.lotItem;
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

	PRIVATE_FUNCTION_WITH_LOCALS(ValidatePrivateAuctionAccess)
	{
		output.isValid = 1;
		if (input.visibility == EAuctionVisibility::Private && isZeroAsset(input.requiredAccessAsset) && input.allowedWalletCount == 0)
		{
			output.isValid = 0;
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(GetCreateAuctionFee)
	{
		output.requiredFee = 0;
		if (input.visibility == EAuctionVisibility::Private)
		{
			output.requiredFee = NOST_PRIVATE_AUCTION_FEE;
		}
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

	PRIVATE_FUNCTION_WITH_LOCALS(EscrowAuctionLotAssets)
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

	PRIVATE_FUNCTION_WITH_LOCALS(BuildAuctionData)
	{
		output.auction.allowedBidderWallets.reset();
		output.auction.auctionId = id::randomValue();
		output.auction.quantityForSale = input.quantityForSale;
		output.auction.allocatedQuantity = 0;
		output.auction.minimumPurchaseQuantity = input.minimumPurchaseQuantity;
		output.auction.initialPricePerUnit = input.createInput.initialPricePerUnit;
		output.auction.salePricePerUnit = input.createInput.salePricePerUnit;
		output.auction.minimumBidIncrement = input.createInput.minimumBidIncrementPerUnit;
		output.auction.buyNowPricePerUnit = input.createInput.buyNowPricePerUnit;
		output.auction.highestBidPerUnit = 0;
		output.auction.highestBidQuantity = 0;
		output.auction.highestBidAmount = 0;
		output.auction.auctionDurationSeconds = smul(static_cast<uint64>(input.createInput.durationDays), NOST_SECONDS_PER_DAY);
		output.auction.createdAt = qpi.now();
		output.auction.lastBidAt = output.auction.createdAt;
		output.auction.sellerDecisionDeadline = DateAndTime();
		output.auction.settledAt = DateAndTime();
		output.auction.bidderCount = 0;
		output.auction.seller = qpi.invocator();
		output.auction.highestBidder = NULL_ID;
		output.auction.assetForSale = input.firstLotItem.asset;
		output.auction.requiredAccessAsset = input.createInput.requiredAccessAsset;
		output.auction.auctionLotItems = input.createInput.auctionLotItems;
		for (locals.allowedWalletIndex = 0; locals.allowedWalletIndex < NOST_AUCTION_ALLOWED_WALLET_NUM; ++locals.allowedWalletIndex)
		{
			if (!isZero(input.createInput.allowedBidderWallets.get(locals.allowedWalletIndex)))
			{
				output.auction.allowedBidderWallets.add(input.createInput.allowedBidderWallets.get(locals.allowedWalletIndex));
			}
		}
		output.auction.metadataIpfsCid = input.createInput.metadataIpfsCid;
		output.auction.type = input.auctionType;
		output.auction.visibility = input.visibility;
		output.auction.status = EAuctionStatus::Active;
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(CreateAuction)
	{
		output.errorCode = static_cast<uint8>(EAuctionError::InvalidInput);

		locals.auctionType = static_cast<EAuctionType>(input.auctionType);
		locals.visibility = static_cast<EAuctionVisibility>(input.auctionVisibility);

		if (!isSupportedAuctionType(locals.auctionType))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InvalidAuctionType);
			return;
		}

		if (!isSupportedAuctionVisibility(locals.visibility))
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
		switch (locals.auctionType)
		{
		case EAuctionType::Batch:
			locals.resolveBatchParamsInput.lotItemCount = locals.analyzeAuctionLotOutput.lotItemCount;
			locals.resolveBatchParamsInput.totalEscrowQuantity = locals.analyzeAuctionLotOutput.totalEscrowQuantity;
			locals.resolveBatchParamsInput.minimumPurchaseQuantity = input.minimumPurchaseQuantity;
			CALL(ResolveBatchAuctionCreateParams, locals.resolveBatchParamsInput, locals.resolveBatchParamsOutput);
			if (!locals.resolveBatchParamsOutput.isValid)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return;
			}
			locals.resolvedQuantityForSale = locals.resolveBatchParamsOutput.quantityForSale;
			locals.resolvedMinimumPurchaseQuantity = locals.resolveBatchParamsOutput.minimumPurchaseQuantity;
			break;
		case EAuctionType::Standard:
			locals.resolveStandardParamsInput.minimumPurchaseQuantity = input.minimumPurchaseQuantity;
			locals.resolveStandardParamsInput.minimumBidIncrementPerUnit = input.minimumBidIncrementPerUnit;
			CALL(ResolveStandardAuctionCreateParams, locals.resolveStandardParamsInput, locals.resolveStandardParamsOutput);
			if (!locals.resolveStandardParamsOutput.isValid)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return;
			}
			locals.resolvedQuantityForSale = locals.resolveStandardParamsOutput.quantityForSale;
			locals.resolvedMinimumPurchaseQuantity = locals.resolveStandardParamsOutput.minimumPurchaseQuantity;
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
		locals.validatePrivateAuctionAccessInput.visibility = locals.visibility;
		locals.validatePrivateAuctionAccessInput.requiredAccessAsset = input.requiredAccessAsset;
		locals.validatePrivateAuctionAccessInput.allowedWalletCount = locals.countAllowedBidderWalletsOutput.allowedWalletCount;
		CALL(ValidatePrivateAuctionAccess, locals.validatePrivateAuctionAccessInput, locals.validatePrivateAuctionAccessOutput);
		if (!locals.validatePrivateAuctionAccessOutput.isValid)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		locals.getCreateAuctionFeeInput.visibility = locals.visibility;
		CALL(GetCreateAuctionFee, locals.getCreateAuctionFeeInput, locals.getCreateAuctionFeeOutput);
		locals.requiredFee = locals.getCreateAuctionFeeOutput.requiredFee;
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

		locals.buildAuctionDataInput.createInput = input;
		locals.buildAuctionDataInput.firstLotItem = locals.analyzeAuctionLotOutput.firstLotItem;
		locals.buildAuctionDataInput.auctionType = locals.auctionType;
		locals.buildAuctionDataInput.visibility = locals.visibility;
		locals.buildAuctionDataInput.quantityForSale = locals.resolvedQuantityForSale;
		locals.buildAuctionDataInput.minimumPurchaseQuantity = locals.resolvedMinimumPurchaseQuantity;
		CALL(BuildAuctionData, locals.buildAuctionDataInput, locals.buildAuctionDataOutput);
		locals.auction = locals.buildAuctionDataOutput.auction;

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

		if (static_cast<uint64>(qpi.invocationReward()) > locals.requiredFee)
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

		if (locals.auction.visibility == EAuctionVisibility::Private &&
		    qpi.numberOfShares(locals.auction.requiredAccessAsset, AssetOwnershipSelect::byOwner(qpi.invocator()),
		                       AssetPossessionSelect::byPossessor(qpi.invocator())) <= 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::PrivateAuctionAccessDenied);
			return;
		}

		locals.effectiveQuantity = input.quantity;
		if (locals.auction.type == EAuctionType::Standard)
		{
			locals.effectiveQuantity = locals.auction.quantityForSale;
		}
		if (locals.effectiveQuantity == 0 || locals.effectiveQuantity < locals.auction.minimumPurchaseQuantity || input.pricePerUnit == 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		if (locals.auction.type == EAuctionType::Batch)
		{
			if (input.pricePerUnit < locals.auction.salePricePerUnit)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
				return;
			}
		}
		else
		{
			if (locals.auction.highestBidPerUnit == 0)
			{
				if (input.pricePerUnit < locals.auction.initialPricePerUnit)
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
					return;
				}
			}
			else
			{
				if (input.pricePerUnit < sadd(locals.auction.highestBidPerUnit, locals.auction.minimumBidIncrement))
				{
					if (qpi.invocationReward() > 0)
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					output.errorCode = static_cast<uint8>(EAuctionError::BidTooLow);
					return;
				}
			}
		}

		locals.requiredEscrow = smul(locals.effectiveQuantity, input.pricePerUnit);
		if (static_cast<uint64>(qpi.invocationReward()) < locals.requiredEscrow)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.errorCode = static_cast<uint8>(EAuctionError::InsufficientFunds);
			return;
		}

		locals.participantKey = {input.auctionId, qpi.invocator()};
		locals.participantExists = state.get().participants.get(locals.participantKey, locals.participantData);
		locals.previousEscrow = 0;
		if (locals.participantExists)
		{
			locals.previousEscrow = locals.participantData.escrowedAmount;
		}

		locals.participantData.escrowedAmount = locals.requiredEscrow;
		locals.participantData.requestedQuantity = locals.effectiveQuantity;
		locals.participantData.allocatedQuantity = 0;
		locals.participantData.pricePerUnit = input.pricePerUnit;
		locals.participantData.lastBidTime = locals.currentDate;
		locals.participantData.participant = qpi.invocator();
		locals.participantData.isHighestBidder = 0;
		locals.participantData.isWinningBid = 0;

		if (!locals.participantExists)
		{
			locals.auction.bidderCount = sadd(locals.auction.bidderCount, 1U);
		}

		if (locals.auction.type == EAuctionType::Standard)
		{
			locals.highestBidderExists = false;
			if (!isZero(locals.auction.highestBidder))
			{
				locals.highestBidderKey = {input.auctionId, locals.auction.highestBidder};
				locals.highestBidderExists = state.get().participants.get(locals.highestBidderKey, locals.previousHighestBidderData);
			}
			if (locals.highestBidderExists && locals.previousHighestBidderData.participant != qpi.invocator())
			{
				qpi.transfer(locals.previousHighestBidderData.participant, locals.previousHighestBidderData.escrowedAmount);
				output.refundedAmount = sadd(output.refundedAmount, locals.previousHighestBidderData.escrowedAmount);
				locals.previousHighestBidderData.escrowedAmount = 0;
				locals.previousHighestBidderData.isHighestBidder = 0;
				locals.previousHighestBidderData.isWinningBid = 0;
				state.mut().participants.replace(locals.highestBidderKey, locals.previousHighestBidderData);
			}

			locals.participantData.isHighestBidder = 1;
			locals.participantData.isWinningBid = 1;
			locals.auction.highestBidder = qpi.invocator();
			locals.auction.highestBidPerUnit = input.pricePerUnit;
			locals.auction.highestBidQuantity = locals.effectiveQuantity;
			locals.auction.highestBidAmount = locals.requiredEscrow;
		}
		else
		{
			if (input.pricePerUnit > locals.auction.highestBidPerUnit)
			{
				locals.auction.highestBidder = qpi.invocator();
				locals.auction.highestBidPerUnit = input.pricePerUnit;
				locals.auction.highestBidQuantity = locals.effectiveQuantity;
				locals.auction.highestBidAmount = locals.requiredEscrow;
				locals.participantData.isHighestBidder = 1;
			}
		}

		locals.auction.lastBidAt = locals.currentDate;
		if ((locals.auction.auctionDurationSeconds - locals.elapsedSeconds) <= NOST_AUCTION_EXTENSION_SECONDS)
		{
			locals.auction.auctionDurationSeconds = sadd(locals.auction.auctionDurationSeconds, NOST_AUCTION_EXTENSION_SECONDS);
		}
		if (locals.auction.buyNowPricePerUnit > 0 && input.pricePerUnit >= locals.auction.buyNowPricePerUnit)
		{
			locals.auction.status = EAuctionStatus::Finalized;
			locals.auction.settledAt = locals.currentDate;
			locals.participantData.isWinningBid = 1;
			locals.participantData.isHighestBidder = 1;
		}

		if (state.mut().participants.set(locals.participantKey, locals.participantData) == NULL_INDEX)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
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
