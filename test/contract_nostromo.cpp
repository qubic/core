#define NO_UEFI

#include "contract_testing.h"

using namespace QPI;

namespace
{
	static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ULL;
	static const id NOST_CONTRACT_ID(NOST_CONTRACT_INDEX, 0, 0, 0);

	class ContractTestingNostromoAuctionFromScratch : protected ContractTesting
	{
	public:
		ContractTestingNostromoAuctionFromScratch()
		{
			initEmptySpectrum();
			initEmptyUniverse();
			INIT_CONTRACT(NOST);
			callSystemProcedure(NOST_CONTRACT_INDEX, INITIALIZE);
			INIT_CONTRACT(QX);
			callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
			setNow(2026, 1, 1, 9, 0, 0);
			callSystemProcedure(NOST_CONTRACT_INDEX, END_TICK);
		}

		void ensureUser(const id& user, sint64 amount = 1000)
		{
			if (getBalance(user) == 0)
			{
				increaseEnergy(user, amount);
			}
		}

		void seedUser(const id& user, sint64 amount = 2000000000LL) { increaseEnergy(user, amount); }

		void setNow(uint16 year, uint8 month, uint8 day, uint8 hour, uint8 minute, uint8 second)
		{
			utcTime.Year = year;
			utcTime.Month = month;
			utcTime.Day = day;
			utcTime.Hour = hour;
			utcTime.Minute = minute;
			utcTime.Second = second;
			utcTime.Nanosecond = 0;
			updateQpiTime();
		}

		void advanceAndEndTick(uint64 milliseconds)
		{
			advanceTimeAndTick(milliseconds);
			callSystemProcedure(NOST_CONTRACT_INDEX, END_TICK);
		}

		void advanceTicks(uint32 count, uint64 millisecondsPerTick = 1000ULL)
		{
			for (uint32 i = 0; i < count; ++i)
			{
				advanceAndEndTick(millisecondsPerTick);
			}
		}

		void beginEpoch()
		{
			++system.epoch;
			callSystemProcedure(NOST_CONTRACT_INDEX, BEGIN_EPOCH);
		}

		sint64 issueAsset(const id& issuer, uint64 assetName, sint64 numberOfShares)
		{
			QX::IssueAsset_input input{};
			QX::IssueAsset_output output{};

			input.assetName = assetName;
			input.numberOfShares = numberOfShares;
			input.unitOfMeasurement = 0;
			input.numberOfDecimalPlaces = 0;

			seedUser(issuer, QX_ISSUE_ASSET_FEE + 1000);
			invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, issuer, QX_ISSUE_ASSET_FEE);
			return output.issuedNumberOfShares;
		}

		sint64 transferShareManagementRightsToNostromo(const id& owner, const Asset& asset, sint64 numberOfShares)
		{
			QX::TransferShareManagementRights_input input{};
			QX::TransferShareManagementRights_output output{};

			input.asset = asset;
			input.numberOfShares = numberOfShares;
			input.newManagingContractIndex = NOST_CONTRACT_INDEX;

			invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, owner, 0);
			return output.transferredNumberOfShares;
		}

		NOST::CreateAuction_output createAuction(const id& seller, const NOST::CreateAuction_input& input, sint64 reward = 0)
		{
			NOST::CreateAuction_output output{};
			if (reward > 0)
			{
				seedUser(seller, reward + 1000);
			}
			else
			{
				ensureUser(seller);
			}
			invokeUserProcedure(NOST_CONTRACT_INDEX, 1, input, output, seller, reward);
			return output;
		}

		NOST::PlaceBid_output placeBid(const id& bidder, const id& auctionId, uint64 quantity, uint64 bidAmount, sint64 reward)
		{
			NOST::PlaceBid_input input{};
			NOST::PlaceBid_output output{};

			input.auctionId = auctionId;
			input.quantity = quantity;
			input.bidAmount = bidAmount;

			seedUser(bidder, reward + 1000);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 2, input, output, bidder, reward);
			return output;
		}

		NOST::CancelAuction_output cancelAuction(const id& seller, const id& auctionId, sint64 reward)
		{
			NOST::CancelAuction_input input{};
			NOST::CancelAuction_output output{};

			input.auctionId = auctionId;
			if (reward > 0)
			{
				seedUser(seller, reward + 1000);
			}
			else
			{
				ensureUser(seller);
			}
			invokeUserProcedure(NOST_CONTRACT_INDEX, 3, input, output, seller, reward);
			return output;
		}

		NOST::TransferShareManagementRights_output transferManagedShares(const id& owner, const Asset& asset, sint64 numberOfShares,
		                                                                 uint32 contractIndex)
		{
			NOST::TransferShareManagementRights_input input{};
			NOST::TransferShareManagementRights_output output{};

			input.asset = asset;
			input.numberOfShares = numberOfShares;
			input.newManagingContractIndex = contractIndex;

			ensureUser(owner);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 4, input, output, owner, 0);
			return output;
		}

		NOST::ResolvePendingStandardAuction_output resolvePendingStandardAuction(const id& seller, const id& auctionId, bool acceptSale)
		{
			NOST::ResolvePendingStandardAuction_input input{};
			NOST::ResolvePendingStandardAuction_output output{};

			input.auctionId = auctionId;
			input.acceptSale = acceptSale ? 1 : 0;

			ensureUser(seller);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 5, input, output, seller, 0);
			return output;
		}

		NOST::SetAuctionFees_output setAuctionFees(const id& caller, const NOST::SetAuctionFees_input& input)
		{
			NOST::SetAuctionFees_output output{};
			ensureUser(caller);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 6, input, output, caller, 0);
			return output;
		}

		NOST::SetAuctionFeesByManagement_output setAuctionFeesByManagement(const id& caller, const NOST::SetAuctionFeesByManagement_input& input)
		{
			NOST::SetAuctionFeesByManagement_output output{};
			ensureUser(caller);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 7, input, output, caller, 0);
			return output;
		}

		NOST::SetManagement_output setManagement(const id& caller, const id& management)
		{
			NOST::SetManagement_input input{};
			NOST::SetManagement_output output{};

			input.management = management;
			ensureUser(caller);
			invokeUserProcedure(NOST_CONTRACT_INDEX, 8, input, output, caller, 0);
			return output;
		}

		NOST::GetAuction_output getAuction(const id& auctionId) const
		{
			NOST::GetAuction_input input{};
			NOST::GetAuction_output output{};

			input.auctionId = auctionId;
			callFunction(NOST_CONTRACT_INDEX, 1, input, output);
			return output;
		}

		NOST::GetAuctionParticipant_output getParticipant(const id& auctionId, const id& participant) const
		{
			NOST::GetAuctionParticipant_input input{};
			NOST::GetAuctionParticipant_output output{};

			input.auctionId = auctionId;
			input.participant = participant;
			callFunction(NOST_CONTRACT_INDEX, 2, input, output);
			return output;
		}

		NOST::GetTicksBeforeAuctionLaunch_output getTicksBeforeAuctionLaunch() const
		{
			NOST::GetTicksBeforeAuctionLaunch_input input{};
			NOST::GetTicksBeforeAuctionLaunch_output output{};

			callFunction(NOST_CONTRACT_INDEX, 3, input, output);
			return output;
		}

		NOST::GetAuctionFees_output getAuctionFees() const
		{
			NOST::GetAuctionFees_input input{};
			NOST::GetAuctionFees_output output{};

			callFunction(NOST_CONTRACT_INDEX, 4, input, output);
			return output;
		}

		NOST::GetFeeRecipients_output getFeeRecipients() const
		{
			NOST::GetFeeRecipients_input input{};
			NOST::GetFeeRecipients_output output{};

			callFunction(NOST_CONTRACT_INDEX, 5, input, output);
			return output;
		}

		sint64 managedShares(const Asset& asset, const id& owner) const
		{
			return numberOfPossessedShares(asset.assetName, asset.issuer, owner, owner, NOST_CONTRACT_INDEX, NOST_CONTRACT_INDEX);
		}

		sint64 sharesManagedBy(const Asset& asset, const id& owner, uint32 contractIndex) const
		{
			return numberOfPossessedShares(asset.assetName, asset.issuer, owner, owner, contractIndex, contractIndex);
		}

		sint64 plainShares(const Asset& asset, const id& owner) const
		{
			return numberOfShares(asset, AssetOwnershipSelect::byOwner(owner), AssetPossessionSelect::byPossessor(owner));
		}

		static Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> makeMetadataCid()
		{
			Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> cid{};
			const char* cidText = "bafybeigdyrzt2a3x4m5n6p7qrstuvwx234567abcdefghijklmnopqrst";
			for (uint64 i = 0; cidText[i] != 0 && i < NOST_AUCTION_METADATA_CID_LENGTH; ++i)
			{
				cid.set(i, static_cast<uint8>(cidText[i]));
			}
			return cid;
		}

		static Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> makeInvalidMetadataCidFirstChar()
		{
			auto cid = makeMetadataCid();
			cid.set(0, 'c');
			return cid;
		}

		static Array<uint8, NOST_AUCTION_METADATA_CID_LENGTH> makeInvalidMetadataCidUppercase()
		{
			auto cid = makeMetadataCid();
			cid.set(5, 'A');
			return cid;
		}

		static Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> makeSingleLot(const Asset& asset, sint64 quantity)
		{
			Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> lot{};
			NOST::AuctionLotEntry entry{};

			entry.asset = asset;
			entry.quantity = quantity;
			lot.set(0, entry);
			return lot;
		}

		static Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> makeTwoAssetLot(const Asset& assetA, sint64 quantityA, const Asset& assetB,
		                                                                               sint64 quantityB)
		{
			Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> lot{};
			NOST::AuctionLotEntry entryA{};
			NOST::AuctionLotEntry entryB{};

			entryA.asset = assetA;
			entryA.quantity = quantityA;
			entryB.asset = assetB;
			entryB.quantity = quantityB;
			lot.set(0, entryA);
			lot.set(1, entryB);
			return lot;
		}

		static Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> makeAllowedWallets(std::initializer_list<id> wallets)
		{
			Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowed{};
			uint64 index = 0;
			for (const auto& wallet : wallets)
			{
				allowed.set(index++, wallet);
			}
			return allowed;
		}

		static Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> makeRequiredAccessAssets(std::initializer_list<Asset> assets)
		{
			Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> required{};
			uint64 index = 0;
			for (const auto& asset : assets)
			{
				required.set(index++, asset);
			}
			return required;
		}

		static NOST::CreateAuction_input makeBatchAuctionInput(const Asset& asset, sint64 quantity, uint64 salePrice = 10)
		{
			NOST::CreateAuction_input input{};
			input.metadataIpfsCid = makeMetadataCid();
			input.auctionLotItems = makeSingleLot(asset, quantity);
			input.salePrice = salePrice;
			input.durationDays = 1;
			input.auctionType = static_cast<uint8>(NOST::EAuctionType::Batch);
			input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Public);
			return input;
		}

		static NOST::CreateAuction_input makeStandardAuctionInput(const Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM>& lot,
		                                                          uint64 initialPrice = 100, uint64 salePrice = 150, uint64 minimumBidIncrement = 10,
		                                                          uint64 buyNowPrice = 0)
		{
			NOST::CreateAuction_input input{};
			input.metadataIpfsCid = makeMetadataCid();
			input.auctionLotItems = lot;
			input.minimumPurchaseQuantity = 1;
			input.initialPrice = initialPrice;
			input.salePrice = salePrice;
			input.minimumBidIncrement = minimumBidIncrement;
			input.buyNowPrice = buyNowPrice;
			input.durationDays = 1;
			input.auctionType = static_cast<uint8>(NOST::EAuctionType::Standard);
			input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Public);
			return input;
		}

		static id managementWallet()
		{
			return ID(_I, _G, _P, _Z, _X, _Q, _O, _R, _J, _Y, _Q, _P, _A, _G, _V, _A, _B, _N, _T, _N, _I, _S, _O, _Y, _T, _M, _T, _A, _N, _M, _K, _Z,
			          _A, _S, _T, _P, _P, _G, _Z, _O, _N, _A, _Q, _J, _X, _Q, _O, _S, _W, _Q, _O, _V, _J, _C, _K, _D);
		}

		static id developmentWallet()
		{
			return ID(_D, _Q, _V, _H, _M, _Z, _F, _C, _W, _O, _K, _M, _H, _F, _B, _H, _L, _X, _U, _I, _U, _G, _P, _P, _X, _R, _Z, _C, _U, _V, _S, _N,
			          _J, _F, _Z, _J, _F, _M, _Q, _M, _Y, _D, _B, _X, _E, _S, _E, _A, _T, _M, _W, _L, _K, _N, _L, _D);
		}

		static id takeoverCoordinatorWallet()
		{
			return ID(_X, _J, _O, _S, _N, _L, _T, _Z, _V, _V, _H, _N, _Z, _C, _B, _Y, _X, _I, _E, _V, _N, _E, _P, _P, _B, _O, _Q, _A, _W, _D, _B, _V,
			          _G, _E, _N, _Z, _O, _X, _S, _V, _O, _B, _K, _G, _Z, _C, _C, _F, _D, _B, _D, _M, _T, _M, _L, _C);
		}
	};

	uint64 expectedShareholderFeeBasisPoints(const NOST::GetAuctionFees_output& fees, uint64 grossAmount)
	{
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_1)
		{
			return fees.shareholderFeeBasisPointsTier1;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_2)
		{
			return fees.shareholderFeeBasisPointsTier2;
		}
		if (grossAmount <= NOST_AUCTION_SHAREHOLDER_FEE_THRESHOLD_TIER_3)
		{
			return fees.shareholderFeeBasisPointsTier3;
		}
		return fees.shareholderFeeBasisPointsTier4;
	}

	uint64 expectedSellerPayout(const NOST::GetAuctionFees_output& fees, uint64 grossAmount)
	{
		const uint64 shareholderFeeAmount = grossAmount * expectedShareholderFeeBasisPoints(fees, grossAmount) / 10000ULL;
		const uint64 managementFeeAmount = grossAmount * fees.managementFeeBasisPoints / 10000ULL;
		const uint64 developmentFeeAmount = grossAmount * fees.developmentFeeBasisPoints / 10000ULL;
		const uint64 takeoverCoordinatorBaseAmount = grossAmount * fees.takeoverCoordinatorFeeBasisPoints / 10000ULL;
		return grossAmount - shareholderFeeAmount - managementFeeAmount - developmentFeeAmount - takeoverCoordinatorBaseAmount;
	}

	uint64 expectedTakeoverCoordinatorGain(const NOST::GetAuctionFees_output& fees, uint64 grossAmount)
	{
		const uint64 shareholderFeeAmount = grossAmount * expectedShareholderFeeBasisPoints(fees, grossAmount) / 10000ULL;
		const uint64 shareholderDividendAmount = shareholderFeeAmount * fees.shareholderDividendBasisPoints / 10000ULL;
		const uint64 takeoverCoordinatorBaseAmount = grossAmount * fees.takeoverCoordinatorFeeBasisPoints / 10000ULL;
		return takeoverCoordinatorBaseAmount + (shareholderFeeAmount - shareholderDividendAmount);
	}

	uint64 expectedDividendRetention(const NOST::GetAuctionFees_output& fees, uint64 grossAmount)
	{
		const uint64 shareholderFeeAmount = grossAmount * expectedShareholderFeeBasisPoints(fees, grossAmount) / 10000ULL;
		return shareholderFeeAmount * fees.shareholderDividendBasisPoints / 10000ULL;
	}
} // namespace

TEST(ContractNostromoAuction, InitialStateAndGettersAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;

	const auto fees = nostromo.getAuctionFees();
	EXPECT_EQ(fees.privateAuctionFee, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
	EXPECT_EQ(fees.auctionCancellationFeeBasisPoints, NOST_DEFAULT_AUCTION_CANCELLATION_FEE_BP);
	EXPECT_EQ(fees.managementFeeBasisPoints, NOST_DEFAULT_AUCTION_MANAGEMENT_FEE_BP);
	EXPECT_EQ(fees.developmentFeeBasisPoints, NOST_DEFAULT_AUCTION_DEVELOPMENT_FEE_BP);
	EXPECT_EQ(fees.takeoverCoordinatorFeeBasisPoints, NOST_DEFAULT_AUCTION_TAKEOVER_COORDINATOR_FEE_BP);
	EXPECT_EQ(fees.shareholderDividendBasisPoints, NOST_DEFAULT_AUCTION_SHAREHOLDER_DIVIDEND_BP);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier1, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_1);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier2, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_2);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier3, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_3);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier4, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_4);

	const auto recipients = nostromo.getFeeRecipients();
	EXPECT_EQ(recipients.management, ContractTestingNostromoAuctionFromScratch::managementWallet());
	EXPECT_EQ(recipients.development, ContractTestingNostromoAuctionFromScratch::developmentWallet());
	EXPECT_EQ(recipients.takeoverCoordinator, ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet());

	const id missingAuction(777, 0, 0, 0);
	const id missingParticipant(888, 0, 0, 0);
	const auto auctionOutput = nostromo.getAuction(missingAuction);
	const auto participantOutput = nostromo.getParticipant(missingAuction, missingParticipant);
	const auto launchPause = nostromo.getTicksBeforeAuctionLaunch();

	EXPECT_TRUE(isZero(auctionOutput.auction.auctionId));
	EXPECT_EQ(participantOutput.found, 0);
	EXPECT_EQ(launchPause.ticks, 0U);

	nostromo.beginEpoch();
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);
	nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, 0U);
}

TEST(ContractNostromoAuction, TransferShareManagementRightsAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id owner(1, 2, 3, 4);
	const uint64 assetName = assetNameFromString("NOSTTR");
	const Asset asset{owner, assetName};

	EXPECT_EQ(nostromo.issueAsset(owner, assetName, 10), 10);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(owner, asset, 7), 7);
	EXPECT_EQ(nostromo.managedShares(asset, owner), 7);
	EXPECT_EQ(nostromo.sharesManagedBy(asset, owner, QX_CONTRACT_INDEX), 3);

	const auto invalidZeroShares = nostromo.transferManagedShares(owner, asset, 0, QX_CONTRACT_INDEX);
	EXPECT_EQ(invalidZeroShares.transferredNumberOfShares, 0);

	Asset zeroAsset{};
	const auto invalidZeroAsset = nostromo.transferManagedShares(owner, zeroAsset, 1, QX_CONTRACT_INDEX);
	EXPECT_EQ(invalidZeroAsset.transferredNumberOfShares, 0);

	const auto invalidZeroContract = nostromo.transferManagedShares(owner, asset, 1, 0);
	EXPECT_EQ(invalidZeroContract.transferredNumberOfShares, 0);

	const auto insufficient = nostromo.transferManagedShares(owner, asset, 8, QX_CONTRACT_INDEX);
	EXPECT_EQ(insufficient.transferredNumberOfShares, 0);

	const auto success = nostromo.transferManagedShares(owner, asset, 5, QX_CONTRACT_INDEX);
	EXPECT_EQ(success.transferredNumberOfShares, 5);
	EXPECT_EQ(nostromo.managedShares(asset, owner), 2);
	EXPECT_EQ(nostromo.sharesManagedBy(asset, owner, QX_CONTRACT_INDEX), 8);
}

TEST(ContractNostromoAuction, CreateBatchPublicAuctionEscrowsLotAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(11, 12, 13, 14);
	const uint64 assetName = assetNameFromString("CRTBTN");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 9), 9);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 9), 9);

	auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 9, 25);
	const auto output = nostromo.createAuction(seller, input);
	ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_FALSE(isZero(output.auctionId));

	const auto auction = nostromo.getAuction(output.auctionId).auction;
	EXPECT_EQ(auction.auctionId, output.auctionId);
	EXPECT_EQ(auction.quantityForSale, 9ULL);
	EXPECT_EQ(auction.minimumPurchaseQuantity, 0ULL);
	EXPECT_EQ(auction.salePrice, 25ULL);
	EXPECT_EQ(auction.auctionDurationSeconds, NOST_SECONDS_PER_DAY);
	EXPECT_EQ(auction.seller, seller);
	EXPECT_EQ(auction.type, NOST::EAuctionType::Batch);
	EXPECT_EQ(auction.visibility, NOST::EAuctionVisibility::Public);
	EXPECT_EQ(auction.status, NOST::EAuctionStatus::Active);
	EXPECT_EQ(auction.auctionLotItems.get(0).asset, asset);
	EXPECT_EQ(auction.auctionLotItems.get(0).quantity, 9);
	EXPECT_EQ(auction.metadataIpfsCid.get(0), 'b');
	EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
	EXPECT_EQ(nostromo.sharesManagedBy(asset, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 9);
}

TEST(ContractNostromoAuction, CreateStandardBundleAuctionEscrowsMixedLotAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(21, 22, 23, 24);
	const uint64 assetNameA = assetNameFromString("CRTSTA");
	const uint64 assetNameB = assetNameFromString("CRTSTB");
	const Asset assetA{seller, assetNameA};
	const Asset assetB{seller, assetNameB};

	EXPECT_EQ(nostromo.issueAsset(seller, assetNameA, 2), 2);
	EXPECT_EQ(nostromo.issueAsset(seller, assetNameB, 3), 3);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetA, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetB, 3), 3);

	auto input = ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	    ContractTestingNostromoAuctionFromScratch::makeTwoAssetLot(assetA, 2, assetB, 3), 100, 150, 5);

	const auto output = nostromo.createAuction(seller, input);
	ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(output.auctionId).auction;
	EXPECT_EQ(auction.quantityForSale, 1ULL);
	EXPECT_EQ(auction.minimumPurchaseQuantity, 1ULL);
	EXPECT_EQ(auction.initialPrice, 100ULL);
	EXPECT_EQ(auction.salePrice, 150ULL);
	EXPECT_EQ(auction.minimumBidIncrement, 5ULL);
	EXPECT_EQ(auction.type, NOST::EAuctionType::Standard);
	EXPECT_EQ(auction.auctionLotItems.get(0).asset, assetA);
	EXPECT_EQ(auction.auctionLotItems.get(0).quantity, 2);
	EXPECT_EQ(auction.auctionLotItems.get(1).asset, assetB);
	EXPECT_EQ(auction.auctionLotItems.get(1).quantity, 3);
	EXPECT_EQ(nostromo.sharesManagedBy(assetA, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 2);
	EXPECT_EQ(nostromo.sharesManagedBy(assetB, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 3);
}

TEST(ContractNostromoAuction, CreatePrivateAuctionsByWalletAndAccessAssetAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(31, 32, 33, 34);
		const id allowedBidder(35, 36, 37, 38);
		const uint64 assetName = assetNameFromString("PRIWAL");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 4, 12);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNostromoAuctionFromScratch::makeAllowedWallets({allowedBidder});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(output.auctionId).auction;
		EXPECT_EQ(auction.visibility, NOST::EAuctionVisibility::Private);
		EXPECT_TRUE(auction.allowedBidderWallets.contains(allowedBidder));
		EXPECT_EQ(auction.requiredAccessAssets.population(), 0U);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(41, 42, 43, 44);
		const id gatedBidder(45, 46, 47, 48);
		const uint64 saleAssetName = assetNameFromString("PRIACC");
		const uint64 gateAssetName = assetNameFromString("GATEAS");
		const Asset saleAsset{seller, saleAssetName};
		const Asset gateAsset{gatedBidder, gateAssetName};

		EXPECT_EQ(nostromo.issueAsset(seller, saleAssetName, 5), 5);
		EXPECT_EQ(nostromo.issueAsset(gatedBidder, gateAssetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, saleAsset, 5), 5);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(saleAsset, 5, 20);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNostromoAuctionFromScratch::makeRequiredAccessAssets({gateAsset});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(output.auctionId).auction;
		EXPECT_EQ(auction.visibility, NOST::EAuctionVisibility::Private);
		EXPECT_EQ(auction.allowedBidderWallets.population(), 0U);
		EXPECT_TRUE(auction.requiredAccessAssets.contains(gateAsset));
		EXPECT_GT(nostromo.plainShares(gateAsset, gatedBidder), 0);
	}
}

TEST(ContractNostromoAuction, CreateAuctionRejectsInvalidInputsAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(51, 52, 53, 54);
	const id altIssuer(55, 56, 57, 58);
	const uint64 assetNameA = assetNameFromString("INVAAA");
	const uint64 assetNameB = assetNameFromString("INVBBB");
	const Asset assetA{seller, assetNameA};
	const Asset assetB{seller, assetNameB};

	EXPECT_EQ(nostromo.issueAsset(seller, assetNameA, 5), 5);
	EXPECT_EQ(nostromo.issueAsset(seller, assetNameB, 5), 5);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetA, 5), 5);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetB, 5), 5);
	EXPECT_EQ(nostromo.issueAsset(altIssuer, assetNameFromString("GATINV"), 1), 1);

	auto invalidCid = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidCid.metadataIpfsCid = ContractTestingNostromoAuctionFromScratch::makeInvalidMetadataCidFirstChar();
	EXPECT_EQ(nostromo.createAuction(seller, invalidCid).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidCidUppercase = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidCidUppercase.metadataIpfsCid = ContractTestingNostromoAuctionFromScratch::makeInvalidMetadataCidUppercase();
	EXPECT_EQ(nostromo.createAuction(seller, invalidCidUppercase).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto emptyLot = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	emptyLot.auctionLotItems = Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM>{};
	EXPECT_EQ(nostromo.createAuction(seller, emptyLot).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto negativeQuantity = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	negativeQuantity.auctionLotItems = ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, -1);
	EXPECT_EQ(nostromo.createAuction(seller, negativeQuantity).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto zeroDuration = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	zeroDuration.durationDays = 0;
	EXPECT_EQ(nostromo.createAuction(seller, zeroDuration).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto tooLongDuration = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	tooLongDuration.durationDays = NOST_AUCTION_MAX_DURATION_DAYS + 1;
	EXPECT_EQ(nostromo.createAuction(seller, tooLongDuration).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidType = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidType.auctionType = 99;
	EXPECT_EQ(nostromo.createAuction(seller, invalidType).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidAuctionType));

	auto invalidVisibility = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidVisibility.auctionVisibility = 99;
	EXPECT_EQ(nostromo.createAuction(seller, invalidVisibility).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidVisibility));

	auto invalidBatchBundle = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidBatchBundle.auctionLotItems = ContractTestingNostromoAuctionFromScratch::makeTwoAssetLot(assetA, 2, assetB, 3);
	EXPECT_EQ(nostromo.createAuction(seller, invalidBatchBundle).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidBatchBuyNow = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	invalidBatchBuyNow.buyNowPrice = 100;
	EXPECT_EQ(nostromo.createAuction(seller, invalidBatchBuyNow).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardMinimumPurchase =
	    ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, 1));
	invalidStandardMinimumPurchase.minimumPurchaseQuantity = 2;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardMinimumPurchase).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardIncrement =
	    ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, 1));
	invalidStandardIncrement.minimumBidIncrement = 0;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardIncrement).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardPrice = ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	    ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, 1), 200, 150, 10);
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardPrice).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardSalePrice =
	    ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, 1));
	invalidStandardSalePrice.salePrice = 0;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardSalePrice).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardBuyNow = ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	    ContractTestingNostromoAuctionFromScratch::makeSingleLot(assetA, 1), 100, 150, 10, 140);
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardBuyNow).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto privateWithoutGate = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	privateWithoutGate.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
	EXPECT_EQ(nostromo.createAuction(seller, privateWithoutGate, NOST_DEFAULT_PRIVATE_AUCTION_FEE).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto privateWithBothGates = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(assetA, 5, 10);
	privateWithBothGates.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
	privateWithBothGates.allowedBidderWallets = ContractTestingNostromoAuctionFromScratch::makeAllowedWallets({id(99, 1, 1, 1)});
	privateWithBothGates.requiredAccessAssets =
	    ContractTestingNostromoAuctionFromScratch::makeRequiredAccessAssets({Asset{altIssuer, assetNameFromString("GATINV")}});
	EXPECT_EQ(nostromo.createAuction(seller, privateWithBothGates, NOST_DEFAULT_PRIVATE_AUCTION_FEE).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::InvalidInput));
}

TEST(ContractNostromoAuction, CreateAuctionRejectsInsufficientFundsInsufficientAssetBalanceAndPauseAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(61, 62, 63, 64);
		const uint64 assetName = assetNameFromString("PRIFEE");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 4, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNostromoAuctionFromScratch::makeAllowedWallets({id(1, 1, 1, 1)});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE - 1);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientFunds));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 4);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(71, 72, 73, 74);
		const uint64 assetName = assetNameFromString("BALLOW");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 3, 10);
		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientAssetBalance));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 2);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(81, 82, 83, 84);
		const uint64 assetName = assetNameFromString("PAUSEA");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 3, 10);
		nostromo.setNow(2026, 1, 7, 11, 40, 0);
		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
		EXPECT_TRUE(isZero(output.auctionId));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(85, 86, 87, 88);
		const uint64 assetName = assetNameFromString("BOOTPA");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 3, 10);
		nostromo.setNow(2022, 4, 13, 12, 0, 0);
		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
		EXPECT_TRUE(isZero(output.auctionId));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}
}

TEST(ContractNostromoAuction, PlaceBatchBidValidatesAndRecomputesHighestBidAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(91, 92, 93, 94);
	const id bidderA(95, 96, 97, 98);
	const id bidderB(99, 100, 101, 102);
	const id bidderC(103, 104, 105, 106);
	const uint64 assetName = assetNameFromString("BIDBAT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 6), 6);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 6), 6);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 6, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto sellerBid = nostromo.placeBid(seller, createOutput.auctionId, 1, 12, 12);
	EXPECT_EQ(sellerBid.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto missingAuction = nostromo.placeBid(bidderA, id(700, 0, 0, 0), 1, 12, 12);
	EXPECT_EQ(missingAuction.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionNotFound));

	const auto zeroQuantity = nostromo.placeBid(bidderA, createOutput.auctionId, 0, 12, 12);
	EXPECT_EQ(zeroQuantity.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto zeroBid = nostromo.placeBid(bidderA, createOutput.auctionId, 1, 0, 1);
	EXPECT_EQ(zeroBid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto tooLow = nostromo.placeBid(bidderA, createOutput.auctionId, 1, 9, 9);
	EXPECT_EQ(tooLow.errorCode, static_cast<uint8>(NOST::EAuctionError::BidTooLow));

	const auto insufficientFunds = nostromo.placeBid(bidderA, createOutput.auctionId, 2, 12, 23);
	EXPECT_EQ(insufficientFunds.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientFunds));

	const auto bidA1 = nostromo.placeBid(bidderA, createOutput.auctionId, 2, 20, 40);
	const auto bidB = nostromo.placeBid(bidderB, createOutput.auctionId, 3, 15, 45);
	ASSERT_EQ(bidA1.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(bidB.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.highestBidder, bidderA);
	EXPECT_EQ(auction.highestBidPrice, 20ULL);
	EXPECT_EQ(auction.highestBidAmount, 40ULL);

	const auto bidA2 = nostromo.placeBid(bidderA, createOutput.auctionId, 2, 14, 28);
	EXPECT_EQ(bidA2.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(bidA2.escrowedAmount, 28ULL);
	EXPECT_EQ(bidA2.refundedAmount, 40ULL);

	auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.highestBidder, bidderB);
	EXPECT_EQ(auction.highestBidPrice, 15ULL);
	EXPECT_EQ(auction.highestBidAmount, 45ULL);

	const auto participantA = nostromo.getParticipant(createOutput.auctionId, bidderA);
	ASSERT_EQ(participantA.found, 1);
	EXPECT_EQ(participantA.participantData.escrowedAmount, 28ULL);
	EXPECT_EQ(participantA.participantData.bidAmount, 14ULL);

	nostromo.setNow(2026, 1, 2, 9, 0, 1);
	const auto closed = nostromo.placeBid(bidderC, createOutput.auctionId, 1, 30, 30);
	EXPECT_EQ(closed.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionClosed));
}

TEST(ContractNostromoAuction, PlaceBatchBidExtendsAuctionNearEndAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(111, 112, 113, 114);
	const id bidder(115, 116, 117, 118);
	const uint64 assetName = assetNameFromString("BIDEXT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 2, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2026, 1, 2, 8, 56, 30);
	const auto bidOutput = nostromo.placeBid(bidder, createOutput.auctionId, 1, 15, 15);
	ASSERT_EQ(bidOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.auctionDurationSeconds, NOST_SECONDS_PER_DAY + NOST_AUCTION_EXTENSION_SECONDS);
}

TEST(ContractNostromoAuction, PlaceStandardBidValidatesRefundsAndPauseAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(121, 122, 123, 124);
	const id bidderA(125, 126, 127, 128);
	const id bidderB(129, 130, 131, 132);
	const uint64 assetName = assetNameFromString("STDVAL");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                                             ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto sellerBid = nostromo.placeBid(seller, createOutput.auctionId, 1, 100, 100);
	EXPECT_EQ(sellerBid.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto lowStart = nostromo.placeBid(bidderA, createOutput.auctionId, 1, 99, 99);
	EXPECT_EQ(lowStart.errorCode, static_cast<uint8>(NOST::EAuctionError::BidTooLow));

	const auto openingBid = nostromo.placeBid(bidderA, createOutput.auctionId, 1, 100, 100);
	ASSERT_EQ(openingBid.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(openingBid.escrowedAmount, 100ULL);

	const auto lowIncrement = nostromo.placeBid(bidderB, createOutput.auctionId, 1, 109, 109);
	EXPECT_EQ(lowIncrement.errorCode, static_cast<uint8>(NOST::EAuctionError::BidTooLow));

	const auto outbid = nostromo.placeBid(bidderB, createOutput.auctionId, 1, 110, 110);
	ASSERT_EQ(outbid.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(outbid.refundedAmount, 100ULL);

	const auto bidderAState = nostromo.getParticipant(createOutput.auctionId, bidderA);
	const auto bidderBState = nostromo.getParticipant(createOutput.auctionId, bidderB);
	ASSERT_EQ(bidderAState.found, 1);
	ASSERT_EQ(bidderBState.found, 1);
	EXPECT_EQ(bidderAState.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(bidderAState.participantData.isWinningBid, 0u);
	EXPECT_EQ(bidderBState.participantData.escrowedAmount, 110ULL);
	EXPECT_EQ(bidderBState.participantData.isWinningBid, 1u);

	const auto bidderBImprove = nostromo.placeBid(bidderB, createOutput.auctionId, 1, 130, 130);
	EXPECT_EQ(bidderBImprove.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(bidderBImprove.refundedAmount, 110ULL);
	EXPECT_EQ(bidderBImprove.escrowedAmount, 130ULL);

	nostromo.beginEpoch();
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);
	const auto pausedBid = nostromo.placeBid(id(133, 134, 135, 136), createOutput.auctionId, 1, 140, 140);
	EXPECT_EQ(pausedBid.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
	nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

	const auto resumedBid = nostromo.placeBid(id(137, 138, 139, 140), createOutput.auctionId, 1, 140, 140);
	EXPECT_EQ(resumedBid.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2022, 4, 13, 12, 0, 0);
	const auto bootstrapPausedBid = nostromo.placeBid(id(141, 142, 143, 144), createOutput.auctionId, 1, 150, 150);
	EXPECT_EQ(bootstrapPausedBid.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
}

TEST(ContractNostromoAuction, PrivateAuctionAccessRulesAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(141, 142, 143, 144);
		const id allowed(145, 146, 147, 148);
		const id denied(149, 150, 151, 152);
		const uint64 assetName = assetNameFromString("PRIBID");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 3, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNostromoAuctionFromScratch::makeAllowedWallets({allowed});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		EXPECT_EQ(nostromo.placeBid(denied, createOutput.auctionId, 1, 12, 12).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::PrivateAuctionAccessDenied));
		EXPECT_EQ(nostromo.placeBid(allowed, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(153, 154, 155, 156);
		const id allowed(157, 158, 159, 160);
		const id denied(161, 162, 163, 164);
		const uint64 saleAssetName = assetNameFromString("PRIACS");
		const uint64 accessAssetName = assetNameFromString("PRIACG");
		const Asset saleAsset{seller, saleAssetName};
		const Asset accessAsset{allowed, accessAssetName};

		EXPECT_EQ(nostromo.issueAsset(seller, saleAssetName, 3), 3);
		EXPECT_EQ(nostromo.issueAsset(allowed, accessAssetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, saleAsset, 3), 3);

		auto input = ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(saleAsset, 3, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNostromoAuctionFromScratch::makeRequiredAccessAssets({accessAsset});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		EXPECT_EQ(nostromo.placeBid(denied, createOutput.auctionId, 1, 12, 12).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::PrivateAuctionAccessDenied));
		EXPECT_EQ(nostromo.placeBid(allowed, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	}
}

TEST(ContractNostromoAuction, PlaceStandardBidBuyNowFinalizesImmediatelyAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(171, 172, 173, 174);
	const id bidder(175, 176, 177, 178);
	const uint64 assetNameA = assetNameFromString("BUYNWA");
	const uint64 assetNameB = assetNameFromString("BUYNWB");
	const Asset assetA{seller, assetNameA};
	const Asset assetB{seller, assetNameB};

	EXPECT_EQ(nostromo.issueAsset(seller, assetNameA, 2), 2);
	EXPECT_EQ(nostromo.issueAsset(seller, assetNameB, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetA, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetB, 1), 1);

	auto input = ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	    ContractTestingNostromoAuctionFromScratch::makeTwoAssetLot(assetA, 2, assetB, 1), 100, 150, 10, 180);
	const sint64 sellerBalanceBefore = getBalance(seller);
	const auto fees = nostromo.getAuctionFees();
	const auto createOutput = nostromo.createAuction(seller, input);
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto bidOutput = nostromo.placeBid(bidder, createOutput.auctionId, 1, 180, 180);
	ASSERT_EQ(bidOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
	EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(auction.allocatedQuantity, 1ULL);
	ASSERT_EQ(participant.found, 1);
	EXPECT_EQ(participant.participantData.allocatedQuantity, 1ULL);
	EXPECT_EQ(participant.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(participant.participantData.isWinningBid, 1u);
	EXPECT_EQ(nostromo.managedShares(assetA, bidder), 2);
	EXPECT_EQ(nostromo.managedShares(assetB, bidder), 1);
	EXPECT_EQ(getBalance(seller) - sellerBalanceBefore, expectedSellerPayout(fees, 180ULL));
}

TEST(ContractNostromoAuction, EndTickFinalizesBatchAuctionByPriceTimeAndPartialFillAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(181, 182, 183, 184);
		const id bidderA(185, 186, 187, 188);
		const id bidderB(189, 190, 191, 192);
		const id bidderC(193, 194, 195, 196);
		const uint64 assetName = assetNameFromString("BATFIN");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 4, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		ASSERT_EQ(nostromo.placeBid(bidderA, createOutput.auctionId, 3, 15, 45).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		nostromo.setNow(2026, 1, 1, 9, 0, 1);
		ASSERT_EQ(nostromo.placeBid(bidderB, createOutput.auctionId, 3, 15, 45).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		nostromo.setNow(2026, 1, 1, 9, 0, 2);
		ASSERT_EQ(nostromo.placeBid(bidderC, createOutput.auctionId, 2, 20, 40).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		const auto participantA = nostromo.getParticipant(createOutput.auctionId, bidderA);
		const auto participantB = nostromo.getParticipant(createOutput.auctionId, bidderB);
		const auto participantC = nostromo.getParticipant(createOutput.auctionId, bidderC);

		EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.allocatedQuantity, 4ULL);
		ASSERT_EQ(participantA.found, 1);
		ASSERT_EQ(participantB.found, 1);
		ASSERT_EQ(participantC.found, 1);
		EXPECT_EQ(participantC.participantData.allocatedQuantity, 2ULL);
		EXPECT_EQ(participantA.participantData.allocatedQuantity, 2ULL);
		EXPECT_EQ(participantB.participantData.allocatedQuantity, 0ULL);
		EXPECT_EQ(participantA.participantData.escrowedAmount, 0ULL);
		EXPECT_EQ(participantB.participantData.escrowedAmount, 0ULL);
		EXPECT_EQ(participantC.participantData.escrowedAmount, 0ULL);
		EXPECT_EQ(participantA.participantData.isWinningBid, 1u);
		EXPECT_EQ(participantB.participantData.isWinningBid, 0u);
		EXPECT_EQ(participantC.participantData.isWinningBid, 1u);
		EXPECT_EQ(nostromo.managedShares(asset, bidderA), 2);
		EXPECT_EQ(nostromo.managedShares(asset, bidderB), 0);
		EXPECT_EQ(nostromo.managedShares(asset, bidderC), 2);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(197, 198, 199, 200);
		const id bidder(201, 202, 203, 204);
		const uint64 assetName = assetNameFromString("BATRET");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 5), 5);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 5), 5);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 5, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 2, 12, 24).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.allocatedQuantity, 2ULL);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 2);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}
}

TEST(ContractNostromoAuction, EndTickFinalizesStandardAuctionWithoutBidAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(211, 212, 213, 214);
	const uint64 assetName = assetNameFromString("STDNOB");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                                             ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(auction.allocatedQuantity, 0ULL);
	EXPECT_TRUE(isZero(auction.highestBidder));
	EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
}

TEST(ContractNostromoAuction, WeeklyPauseShiftsActiveAuctionDeadlineAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(215, 216, 217, 218);
	const uint64 assetName = assetNameFromString("PAUSHL");
	const Asset asset{seller, assetName};

	nostromo.setNow(2026, 1, 6, 11, 40, 0);
	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                                             ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2026, 1, 7, 11, 40, 0);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Active);

	nostromo.setNow(2026, 1, 7, 12, 0, 0);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Active);

	nostromo.setNow(2026, 1, 7, 12, 10, 1);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
}

TEST(ContractNostromoAuction, EndTickSkipsAuctionProcessingAtBootstrapTimeAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(215, 216, 217, 218);
	const uint64 assetName = assetNameFromString("BOOTTK");
	const Asset asset{seller, assetName};

	nostromo.setNow(2022, 4, 12, 12, 0, 0);
	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                                             ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2022, 4, 13, 12, 0, 0);
	nostromo.advanceAndEndTick(0);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.status, NOST::EAuctionStatus::Active);
	EXPECT_EQ(auction.allocatedQuantity, 0ULL);
	EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
}

TEST(ContractNostromoAuction, PauseShiftsSellerDecisionDeadlineAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(219, 220, 221, 222);
	const id bidder(223, 224, 225, 226);
	const uint64 assetName = assetNameFromString("PDSHFT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY) * 1000ULL);
	auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	ASSERT_EQ(auction.status, NOST::EAuctionStatus::PendingSellerDecision);
	EXPECT_EQ(auction.sellerDecisionDeadline.getHour(), 9);
	EXPECT_EQ(auction.sellerDecisionDeadline.getMinute(), 0);
	EXPECT_EQ(auction.sellerDecisionDeadline.getSecond(), 0);

	nostromo.setNow(2026, 1, 9, 8, 59, 50);
	nostromo.beginEpoch();
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

	nostromo.advanceAndEndTick(1000);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS - 1);

	nostromo.setNow(2026, 1, 9, 9, 8, 10);
	nostromo.advanceAndEndTick(0);
	auction = nostromo.getAuction(createOutput.auctionId).auction;
	ASSERT_EQ(auction.status, NOST::EAuctionStatus::PendingSellerDecision);
	EXPECT_EQ(auction.sellerDecisionDeadline.getHour(), 9);
	EXPECT_EQ(auction.sellerDecisionDeadline.getMinute(), 8);
	EXPECT_EQ(auction.sellerDecisionDeadline.getSecond(), 21);

	nostromo.setNow(2026, 1, 9, 9, 8, 20);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::PendingSellerDecision);

	nostromo.setNow(2026, 1, 9, 9, 8, 22);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
}

TEST(ContractNostromoAuction, EndTickFinalizesStandardAuctionAtSalePriceAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(221, 222, 223, 224);
	const id bidder(225, 226, 227, 228);
	const uint64 assetName = assetNameFromString("STDEND");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto fees = nostromo.getAuctionFees();
	const sint64 sellerBalanceBefore = getBalance(seller);
	const sint64 managementBefore = getBalance(ContractTestingNostromoAuctionFromScratch::managementWallet());
	const sint64 developmentBefore = getBalance(ContractTestingNostromoAuctionFromScratch::developmentWallet());
	const sint64 coordinatorBefore = getBalance(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet());
	const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
	                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 10000, 10000, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 10000, 10000).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(auction.allocatedQuantity, 1ULL);
	EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
	EXPECT_EQ(getBalance(seller) - sellerBalanceBefore, expectedSellerPayout(fees, 10000ULL));
	EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::managementWallet()) - managementBefore, 50);
	EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::developmentWallet()) - developmentBefore, 50);
	EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet()) - coordinatorBefore,
	          expectedTakeoverCoordinatorGain(fees, 10000ULL));
	EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendRetention(fees, 10000ULL));
}

TEST(ContractNostromoAuction, PendingSellerDecisionAcceptRejectAndTimeoutAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(231, 232, 233, 234);
		const id bidder(235, 236, 237, 238);
		const uint64 assetName = assetNameFromString("PENACC");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
		                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::PendingSellerDecision);

		const auto forbidden = nostromo.resolvePendingStandardAuction(id(999, 999, 999, 999), createOutput.auctionId, true);
		EXPECT_EQ(forbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

		const auto acceptOutput = nostromo.resolvePendingStandardAuction(seller, createOutput.auctionId, true);
		EXPECT_EQ(acceptOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(239, 240, 241, 242);
		const id bidder(243, 244, 245, 246);
		const uint64 assetName = assetNameFromString("PENREJ");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
		                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		const auto rejectOutput = nostromo.resolvePendingStandardAuction(seller, createOutput.auctionId, false);
		EXPECT_EQ(rejectOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(rejectOutput.refundedAmount, 120ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
		EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.allocatedQuantity, 0ULL);
		EXPECT_TRUE(isZero(auction.highestBidder));
		ASSERT_EQ(participant.found, 1);
		EXPECT_EQ(participant.participantData.allocatedQuantity, 0ULL);
		EXPECT_EQ(participant.participantData.escrowedAmount, 0ULL);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(247, 248, 249, 250);
		const id bidder(251, 252, 253, 254);
		const uint64 assetName = assetNameFromString("PENTMO");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
		                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::PendingSellerDecision);

		nostromo.advanceAndEndTick((NOST_AUCTION_SELLER_DECISION_WINDOW_SECONDS + 1ULL) * 1000ULL);
		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
		EXPECT_EQ(auction.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.allocatedQuantity, 1ULL);
		ASSERT_EQ(participant.found, 1);
		EXPECT_EQ(participant.participantData.allocatedQuantity, 1ULL);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
	}
}

TEST(ContractNostromoAuction, CancelAuctionRefundsAndChargesCorrectFeeAuction)
{
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(261, 262, 263, 264);
		const id bidderA(265, 266, 267, 268);
		const id bidderB(269, 270, 271, 272);
		const uint64 assetName = assetNameFromString("CANBAT");
		const Asset asset{seller, assetName};
		const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 10), 10);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 10), 10);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 10, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidderA, createOutput.auctionId, 2, 20, 40).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidderB, createOutput.auctionId, 3, 15, 45).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto cancelOutput = nostromo.cancelAuction(seller, createOutput.auctionId, 10);
		EXPECT_EQ(cancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(cancelOutput.refundedAmount, 85ULL);
		EXPECT_EQ(cancelOutput.cancellationFee, 10ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Cancelled);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 10);
		EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 10);
	}

	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(273, 274, 275, 276);
		const id bidder(277, 278, 279, 280);
		const uint64 assetName = assetNameFromString("CANSTD");
		const Asset asset{seller, assetName};
		const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
		                                       ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto cancelOutput = nostromo.cancelAuction(seller, createOutput.auctionId, 15);
		EXPECT_EQ(cancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(cancelOutput.refundedAmount, 120ULL);
		EXPECT_EQ(cancelOutput.cancellationFee, 15ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.status, NOST::EAuctionStatus::Cancelled);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
		EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 15);
	}
}

TEST(ContractNostromoAuction, CancelAuctionRejectsInvalidCasesAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id seller(281, 282, 283, 284);
	const id bidder(285, 286, 287, 288);
	const uint64 assetName = assetNameFromString("CANINV");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeBatchAuctionInput(asset, 2, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto notFound = nostromo.cancelAuction(seller, id(800, 0, 0, 0), 10);
	EXPECT_EQ(notFound.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionNotFound));

	const auto forbidden = nostromo.cancelAuction(bidder, createOutput.auctionId, 10);
	EXPECT_EQ(forbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto insufficient = nostromo.cancelAuction(seller, createOutput.auctionId, 1);
	EXPECT_EQ(insufficient.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientFunds));

	const auto success = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(success.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto closed = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(closed.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionClosed));
}

TEST(ContractNostromoAuction, GovernanceAndFeeSettersAuction)
{
	ContractTestingNostromoAuctionFromScratch nostromo;
	const id outsider(291, 292, 293, 294);
	const id newManagement(295, 296, 297, 298);

	NOST::SetAuctionFees_input coordinatorInput{};
	coordinatorInput.privateAuctionFee = 60000000;
	coordinatorInput.auctionCancellationFeeBasisPoints = 900;
	coordinatorInput.managementFeeBasisPoints = 60;
	coordinatorInput.developmentFeeBasisPoints = 70;
	coordinatorInput.takeoverCoordinatorFeeBasisPoints = 80;
	coordinatorInput.shareholderDividendBasisPoints = 8500;
	coordinatorInput.shareholderFeeBasisPointsTier1 = 400;
	coordinatorInput.shareholderFeeBasisPointsTier2 = 350;
	coordinatorInput.shareholderFeeBasisPointsTier3 = 300;
	coordinatorInput.shareholderFeeBasisPointsTier4 = 250;

	const auto coordinatorForbidden = nostromo.setAuctionFees(outsider, coordinatorInput);
	EXPECT_EQ(coordinatorForbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	NOST::SetAuctionFees_input invalidCoordinatorInput = coordinatorInput;
	invalidCoordinatorInput.privateAuctionFee = -1;
	const auto coordinatorInvalid =
	    nostromo.setAuctionFees(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet(), invalidCoordinatorInput);
	EXPECT_EQ(coordinatorInvalid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto coordinatorSuccess = nostromo.setAuctionFees(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet(), coordinatorInput);
	EXPECT_EQ(coordinatorSuccess.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	auto fees = nostromo.getAuctionFees();
	EXPECT_EQ(fees.privateAuctionFee, 60000000);
	EXPECT_EQ(fees.auctionCancellationFeeBasisPoints, 900ULL);
	EXPECT_EQ(fees.managementFeeBasisPoints, 60ULL);
	EXPECT_EQ(fees.developmentFeeBasisPoints, 70ULL);
	EXPECT_EQ(fees.takeoverCoordinatorFeeBasisPoints, 80ULL);
	EXPECT_EQ(fees.shareholderDividendBasisPoints, 8500ULL);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier1, 400ULL);

	const auto setManagementForbidden = nostromo.setManagement(outsider, newManagement);
	EXPECT_EQ(setManagementForbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto setManagementInvalid = nostromo.setManagement(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet(), NULL_ID);
	EXPECT_EQ(setManagementInvalid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto setManagementSuccess = nostromo.setManagement(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet(), newManagement);
	EXPECT_EQ(setManagementSuccess.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(nostromo.getFeeRecipients().management, newManagement);

	NOST::SetAuctionFeesByManagement_input managementInput{};
	managementInput.privateAuctionFee = 70000000;
	managementInput.auctionCancellationFeeBasisPoints = 800;
	managementInput.managementFeeBasisPoints = 90;
	managementInput.developmentFeeBasisPoints = 110;
	managementInput.shareholderFeeBasisPointsTier1 = 300;
	managementInput.shareholderFeeBasisPointsTier2 = 250;
	managementInput.shareholderFeeBasisPointsTier3 = 200;
	managementInput.shareholderFeeBasisPointsTier4 = 150;

	const auto oldManagementForbidden =
	    nostromo.setAuctionFeesByManagement(ContractTestingNostromoAuctionFromScratch::managementWallet(), managementInput);
	EXPECT_EQ(oldManagementForbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	NOST::SetAuctionFeesByManagement_input invalidManagementInput = managementInput;
	invalidManagementInput.managementFeeBasisPoints = 9900;
	invalidManagementInput.developmentFeeBasisPoints = 200;
	const auto managementInvalid = nostromo.setAuctionFeesByManagement(newManagement, invalidManagementInput);
	EXPECT_EQ(managementInvalid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto managementSuccess = nostromo.setAuctionFeesByManagement(newManagement, managementInput);
	EXPECT_EQ(managementSuccess.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	fees = nostromo.getAuctionFees();
	EXPECT_EQ(fees.privateAuctionFee, 70000000);
	EXPECT_EQ(fees.auctionCancellationFeeBasisPoints, 800ULL);
	EXPECT_EQ(fees.managementFeeBasisPoints, 90ULL);
	EXPECT_EQ(fees.developmentFeeBasisPoints, 110ULL);
	EXPECT_EQ(fees.takeoverCoordinatorFeeBasisPoints, 80ULL);
	EXPECT_EQ(fees.shareholderDividendBasisPoints, 8500ULL);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier1, 300ULL);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier2, 250ULL);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier3, 200ULL);
	EXPECT_EQ(fees.shareholderFeeBasisPointsTier4, 150ULL);
}

TEST(ContractNostromoAuction, ShareholderFeeTiersAreAppliedAuction)
{
	struct TierCase
	{
		uint64 grossAmount;
		uint64 expectedShareholderFeeBp;
		uint64 assetName;
	};

	const TierCase cases[] = {
	    {5000000000ULL, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_1, assetNameFromString("TIERA1")},
	    {5000000001ULL, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_2, assetNameFromString("TIERA2")},
	    {50000000001ULL, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_3, assetNameFromString("TIERA3")},
	    {200000000001ULL, NOST_DEFAULT_AUCTION_SHAREHOLDER_FEE_BP_TIER_4, assetNameFromString("TIERA4")},
	};

	for (uint64 caseIndex = 0; caseIndex < sizeof(cases) / sizeof(cases[0]); ++caseIndex)
	{
		ContractTestingNostromoAuctionFromScratch nostromo;
		const id seller(301 + caseIndex, 302 + caseIndex, 303 + caseIndex, 304 + caseIndex);
		const id bidder(401 + caseIndex, 402 + caseIndex, 403 + caseIndex, 404 + caseIndex);
		const Asset asset{seller, cases[caseIndex].assetName};
		const auto fees = nostromo.getAuctionFees();
		const sint64 sellerBefore = getBalance(seller);
		const sint64 managementBefore = getBalance(ContractTestingNostromoAuctionFromScratch::managementWallet());
		const sint64 developmentBefore = getBalance(ContractTestingNostromoAuctionFromScratch::developmentWallet());
		const sint64 coordinatorBefore = getBalance(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet());
		const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

		EXPECT_EQ(nostromo.issueAsset(seller, cases[caseIndex].assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNostromoAuctionFromScratch::makeStandardAuctionInput(
		                                                             ContractTestingNostromoAuctionFromScratch::makeSingleLot(asset, 1),
		                                                             cases[caseIndex].grossAmount, cases[caseIndex].grossAmount, 1));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, cases[caseIndex].grossAmount, cases[caseIndex].grossAmount).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

		EXPECT_EQ(expectedShareholderFeeBasisPoints(fees, cases[caseIndex].grossAmount), cases[caseIndex].expectedShareholderFeeBp);
		EXPECT_EQ(getBalance(seller) - sellerBefore, expectedSellerPayout(fees, cases[caseIndex].grossAmount));
		EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::managementWallet()) - managementBefore,
		          cases[caseIndex].grossAmount * fees.managementFeeBasisPoints / 10000ULL);
		EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::developmentWallet()) - developmentBefore,
		          cases[caseIndex].grossAmount * fees.developmentFeeBasisPoints / 10000ULL);
		EXPECT_EQ(getBalance(ContractTestingNostromoAuctionFromScratch::takeoverCoordinatorWallet()) - coordinatorBefore,
		          expectedTakeoverCoordinatorGain(fees, cases[caseIndex].grossAmount));
		EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendRetention(fees, cases[caseIndex].grossAmount));
	}
}
