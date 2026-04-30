#define NO_UEFI

#include "contract_testing.h"

using namespace QPI;

namespace
{
	static constexpr uint64 QX_ISSUE_ASSET_FEE = 1000000000ULL;
	static const id NOST_CONTRACT_ID(NOST_CONTRACT_INDEX, 0, 0, 0);
} // namespace

class NOSTChecker : public NOST, public NOST::StateData
{

public:
	const QPI::ContractState<StateData, NOST_CONTRACT_INDEX>& asState() const
	{
		return *reinterpret_cast<const QPI::ContractState<StateData, NOST_CONTRACT_INDEX>*>(static_cast<const StateData*>(this));
	}

	void calculateAuctionRevenueBreakdown(uint64 grossAmount, AuctionRevenueBreakdown& output) const
	{
		NOST::calculateAuctionRevenueBreakdown(grossAmount, asState(), output);
	}

	void calculateAuctionServiceFeeBreakdown(uint64 feeAmount, AuctionServiceFeeBreakdown& output) const
	{
		NOST::calculateAuctionServiceFeeBreakdown(feeAmount, output);
	}

	uint64 getAuctionShareholderFeeBasisPoints(uint64 grossAmount) const { return NOST::getAuctionShareholderFeeBasisPoints(grossAmount, asState()); }
};

class ContractTestingNOST : protected ContractTesting
{
public:
	ContractTestingNOST()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(NOST);
		system.initialTick = system.tick;
		callSystemProcedure(NOST_CONTRACT_INDEX, INITIALIZE);
		INIT_CONTRACT(QX);
		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
		setNow(2026, 1, 1, 9, 0, 0);
		callSystemProcedure(NOST_CONTRACT_INDEX, END_TICK);
	}

	NOSTChecker* state() { return reinterpret_cast<NOSTChecker*>(contractStates[NOST_CONTRACT_INDEX]); }

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
		system.initialTick = system.tick;
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

		seedUser(issuer, QX_ISSUE_ASSET_FEE);
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
			seedUser(seller, reward);
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

		seedUser(bidder, reward );
		invokeUserProcedure(NOST_CONTRACT_INDEX, 2, input, output, bidder, reward);
		return output;
	}

	NOST::PlaceBid_output placeBidWithFundedReward(const id& bidder, const id& auctionId, uint64 quantity, uint64 bidAmount, sint64 reward)
	{
		NOST::PlaceBid_input input{};
		NOST::PlaceBid_output output{};

		input.auctionId = auctionId;
		input.quantity = quantity;
		input.bidAmount = bidAmount;

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
			seedUser(seller, reward );
		}
		else
		{
			ensureUser(seller);
		}
		invokeUserProcedure(NOST_CONTRACT_INDEX, 3, input, output, seller, reward);
		return output;
	}

	NOST::TransferShareManagementRights_output transferManagedSharesWithReward(const id& owner, const Asset& asset, sint64 numberOfShares,
	                                                                           uint32 contractIndex, sint64 reward)
	{
		if (reward > 0)
		{
			seedUser(owner, reward);
		}
		else
		{
			ensureUser(owner);
		}
		return transferManagedSharesWithFundedReward(owner, asset, numberOfShares, contractIndex, reward);
	}

	NOST::TransferShareManagementRights_output transferManagedSharesWithFundedReward(const id& owner, const Asset& asset, sint64 numberOfShares,
	                                                                                 uint32 contractIndex, sint64 reward)
	{
		NOST::TransferShareManagementRights_input input{};
		NOST::TransferShareManagementRights_output output{};

		input.asset = asset;
		input.numberOfShares = numberOfShares;
		input.newManagingContractIndex = contractIndex;

		invokeUserProcedure(NOST_CONTRACT_INDEX, 4, input, output, owner, reward);
		return output;
	}

	NOST::TransferShareManagementRights_output transferManagedShares(const id& owner, const Asset& asset, sint64 numberOfShares, uint32 contractIndex)
	{
		syncCachedQxTransferFee();
		return transferManagedSharesWithReward(owner, asset, numberOfShares, contractIndex, getCachedQxTransferFee());
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

	NOST::GetClosedAuctionHistory_output getClosedAuctionHistory() const
	{
		NOST::GetClosedAuctionHistory_input input{};
		NOST::GetClosedAuctionHistory_output output{};

		callFunction(NOST_CONTRACT_INDEX, 6, input, output);
		return output;
	}

	NOST::GetRouteAllFeesToDevelopment_output getRouteAllFeesToDevelopmentPublic() const
	{
		NOST::GetRouteAllFeesToDevelopment_input input{};
		NOST::GetRouteAllFeesToDevelopment_output output{};

		callFunction(NOST_CONTRACT_INDEX, 7, input, output);
		return output;
	}

	NOST::StateData& stateData() { return *reinterpret_cast<NOST::StateData*>(contractStates[NOST_CONTRACT_INDEX]); }
	const NOST::StateData& stateData() const { return *reinterpret_cast<const NOST::StateData*>(contractStates[NOST_CONTRACT_INDEX]); }
	QX::StateData& qxStateData() { return *reinterpret_cast<QX::StateData*>(contractStates[QX_CONTRACT_INDEX]); }

	void setRouteAllFeesToDevelopment(uint8 enabled) { stateData().routeAllFeesToDevelopment = enabled; }
	uint8 getRouteAllFeesToDevelopment() const { return stateData().routeAllFeesToDevelopment; }
	void syncCachedQxTransferFee() { stateData().qxTransferFee = qxStateData()._transferFee; }
	uint32 getCachedQxTransferFee() const { return stateData().qxTransferFee; }

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

	static Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> makeFullLot(const Asset& asset, sint64 quantity)
	{
		Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM> lot{};
		for (uint64 index = 0; index < lot.capacity(); ++index)
		{
			NOST::AuctionLotEntry entry{};
			entry.asset = asset;
			entry.quantity = quantity;
			lot.set(index, entry);
		}
		return lot;
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

	void calculateAuctionRevenueBreakdown(uint64 grossAmount, NOST::AuctionRevenueBreakdown& output)
	{
		state()->calculateAuctionRevenueBreakdown(grossAmount, output);
	}

	void calculateAuctionServiceFeeBreakdown(uint64 feeAmount, NOST::AuctionServiceFeeBreakdown& output)
	{
		state()->calculateAuctionServiceFeeBreakdown(feeAmount, output);
	}

	sint64 expectedDividendPoolIncrease(uint64 addedDividendAmount) const
	{
		const uint64 poolBefore = stateData().auctionShareholderDividendPool;
		const uint64 poolAfterFunding = poolBefore + addedDividendAmount;
		return static_cast<sint64>(poolAfterFunding % NUMBER_OF_COMPUTORS) - static_cast<sint64>(poolBefore);
	}

	uint64 getAuctionShareholderFeeBasisPoints(uint64 grossAmount) { return state()->getAuctionShareholderFeeBasisPoints(grossAmount); }

	static id managementWallet()
	{
		return ID(_I, _G, _P, _Z, _X, _Q, _O, _R, _J, _Y, _Q, _P, _A, _G, _V, _A, _B, _N, _T, _N, _I, _S, _O, _Y, _T, _M, _T, _A, _N, _M, _K, _Z, _A,
		          _S, _T, _P, _P, _G, _Z, _O, _N, _A, _Q, _J, _X, _Q, _O, _S, _W, _Q, _O, _V, _J, _C, _K, _D);
	}

	static id developmentWallet()
	{
		return ID(_D, _Q, _V, _H, _M, _Z, _F, _C, _W, _O, _K, _M, _H, _F, _B, _H, _L, _X, _U, _I, _U, _G, _P, _P, _X, _R, _Z, _C, _U, _V, _S, _N, _J,
		          _F, _Z, _J, _F, _M, _Q, _M, _Y, _D, _B, _X, _E, _S, _E, _A, _T, _M, _W, _L, _K, _N, _L, _D);
	}

	static id takeoverCoordinatorWallet()
	{
		return ID(_X, _J, _O, _S, _N, _L, _T, _Z, _V, _V, _H, _N, _Z, _C, _B, _Y, _X, _I, _E, _V, _N, _E, _P, _P, _B, _O, _Q, _A, _W, _D, _B, _V, _G,
		          _E, _N, _Z, _O, _X, _S, _V, _O, _B, _K, _G, _Z, _C, _C, _F, _D, _B, _D, _M, _T, _M, _L, _C);
	}
};

static bool containsWallet(const Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM>& wallets, uint64 count, const id& wallet)
{
	for (uint64 index = 0; index < count; ++index)
	{
		if (wallets.get(index) == wallet)
		{
			return true;
		}
	}
	return false;
}

static bool containsAccessAsset(const Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM>& assets, uint64 count, const Asset& asset)
{
	for (uint64 index = 0; index < count; ++index)
	{
		if (assets.get(index) == asset)
		{
			return true;
		}
	}
	return false;
}

static bool containsAuctionId(const Array<id, NOST_AUCTION_HISTORY_NUM>& auctionIds, uint64 count, const id& auctionId)
{
	const uint64 boundedCount = count < auctionIds.capacity() ? count : auctionIds.capacity();
	for (uint64 index = 0; index < boundedCount; ++index)
	{
		if (auctionIds.get(index) == auctionId)
		{
			return true;
		}
	}
	return false;
}

TEST(ContractNostromoAuction, InitialStateAndGettersAuction)
{
	ContractTestingNOST nostromo;

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
	EXPECT_EQ(recipients.management, ContractTestingNOST::managementWallet());
	EXPECT_EQ(recipients.development, ContractTestingNOST::developmentWallet());
	EXPECT_EQ(recipients.takeoverCoordinator, ContractTestingNOST::takeoverCoordinatorWallet());

	const id missingAuction(777, 0, 0, 0);
	const id missingParticipant(888, 0, 0, 0);
	const auto auctionOutput = nostromo.getAuction(missingAuction);
	const auto participantOutput = nostromo.getParticipant(missingAuction, missingParticipant);
	const auto launchPause = nostromo.getTicksBeforeAuctionLaunch();

	EXPECT_TRUE(isZero(auctionOutput.auction.core.auctionId));
	EXPECT_EQ(participantOutput.found, 0);
	EXPECT_EQ(launchPause.ticks, 0U);
	EXPECT_EQ(nostromo.getClosedAuctionHistory().totalEntries, 0ULL);
	EXPECT_EQ(nostromo.getRouteAllFeesToDevelopmentPublic().enabled, NOST_ROUTE_ALL_FEES_TO_DEVELOPMENT);

	nostromo.setRouteAllFeesToDevelopment(1);
	EXPECT_EQ(nostromo.getRouteAllFeesToDevelopmentPublic().enabled, 1);
	nostromo.setRouteAllFeesToDevelopment(0);
	EXPECT_EQ(nostromo.getRouteAllFeesToDevelopmentPublic().enabled, 0);

	nostromo.beginEpoch();
	EXPECT_EQ(nostromo.getCachedQxTransferFee(), nostromo.qxStateData()._transferFee);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);
	nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, 0U);
}

TEST(ContractNostromoAuction, TransferShareManagementRightsAuction)
{
	ContractTestingNOST nostromo;
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

TEST(ContractNostromoAuction, TransferShareManagementRightsRequiresInvocationRewardAuction)
{
	{
		ContractTestingNOST nostromo;
		const id owner(5, 6, 7, 8);
		const uint64 assetName = assetNameFromString("TRFEXA");
		const Asset asset{owner, assetName};

		EXPECT_EQ(nostromo.issueAsset(owner, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(owner, asset, 4), 4);
		nostromo.syncCachedQxTransferFee();

		const auto output = nostromo.transferManagedSharesWithReward(owner, asset, 2, QX_CONTRACT_INDEX, nostromo.getCachedQxTransferFee());
		EXPECT_EQ(output.transferredNumberOfShares, 2);
		EXPECT_EQ(nostromo.managedShares(asset, owner), 2);
		EXPECT_EQ(nostromo.sharesManagedBy(asset, owner, QX_CONTRACT_INDEX), 2);
	}

	{
		ContractTestingNOST nostromo;
		const id owner(9, 10, 11, 12);
		const uint64 assetName = assetNameFromString("TRFINS");
		const Asset asset{owner, assetName};

		EXPECT_EQ(nostromo.issueAsset(owner, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(owner, asset, 4), 4);
		nostromo.syncCachedQxTransferFee();

		const auto output = nostromo.transferManagedSharesWithReward(owner, asset, 2, QX_CONTRACT_INDEX, nostromo.getCachedQxTransferFee() - 1);
		EXPECT_EQ(output.transferredNumberOfShares, 0);
		EXPECT_EQ(nostromo.managedShares(asset, owner), 4);
		EXPECT_EQ(nostromo.sharesManagedBy(asset, owner, QX_CONTRACT_INDEX), 0);
	}

	{
		ContractTestingNOST nostromo;
		const id owner(13, 14, 15, 16);
		const uint64 assetName = assetNameFromString("TRFEXC");
		const Asset asset{owner, assetName};

		EXPECT_EQ(nostromo.issueAsset(owner, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(owner, asset, 4), 4);
		nostromo.syncCachedQxTransferFee();
		const sint64 reward = static_cast<sint64>(nostromo.getCachedQxTransferFee()) + 50;
		nostromo.seedUser(owner, reward);
		const sint64 ownerBefore = getBalance(owner);

		const auto output = nostromo.transferManagedSharesWithFundedReward(owner, asset, 2, QX_CONTRACT_INDEX, reward);
		EXPECT_EQ(output.transferredNumberOfShares, 2);
		EXPECT_EQ(getBalance(owner) - ownerBefore, -static_cast<sint64>(nostromo.getCachedQxTransferFee()));
		EXPECT_EQ(nostromo.managedShares(asset, owner), 2);
		EXPECT_EQ(nostromo.sharesManagedBy(asset, owner, QX_CONTRACT_INDEX), 2);
	}

	{
		ContractTestingNOST nostromo;
		const id owner(17, 18, 19, 20);
		const uint64 assetName = assetNameFromString("TRFINV");
		const Asset asset{owner, assetName};

		EXPECT_EQ(nostromo.issueAsset(owner, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(owner, asset, 4), 4);
		nostromo.syncCachedQxTransferFee();

		const auto invalidDestination = nostromo.transferManagedSharesWithReward(owner, asset, 2, 0, nostromo.getCachedQxTransferFee());
		EXPECT_EQ(invalidDestination.transferredNumberOfShares, 0);
		EXPECT_EQ(nostromo.managedShares(asset, owner), 4);

		Asset zeroAsset{};
		const auto zeroAssetOutput = nostromo.transferManagedSharesWithReward(owner, zeroAsset, 2, QX_CONTRACT_INDEX, nostromo.getCachedQxTransferFee());
		EXPECT_EQ(zeroAssetOutput.transferredNumberOfShares, 0);
		EXPECT_EQ(nostromo.managedShares(asset, owner), 4);
	}
}

TEST(ContractNostromoAuction, CreateBatchPublicAuctionEscrowsLotAuction)
{
	ContractTestingNOST nostromo;
	const id seller(11, 12, 13, 14);
	const uint64 assetName = assetNameFromString("CRTBTN");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 9), 9);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 9), 9);

	auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 9, 25);
	const auto output = nostromo.createAuction(seller, input);
	ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_FALSE(isZero(output.auctionId));

	const auto auction = nostromo.getAuction(output.auctionId).auction;
	EXPECT_EQ(auction.core.auctionId, output.auctionId);
	EXPECT_EQ(auction.core.quantityForSale, 9ULL);
	EXPECT_EQ(auction.core.minimumPurchaseQuantity, 0ULL);
	EXPECT_EQ(auction.core.salePrice, 25ULL);
	EXPECT_EQ(auction.core.auctionDurationSeconds, NOST_SECONDS_PER_DAY);
	EXPECT_EQ(auction.core.seller, seller);
	EXPECT_EQ(auction.core.type, NOST::EAuctionType::Batch);
	EXPECT_EQ(auction.core.visibility, NOST::EAuctionVisibility::Public);
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Active);
	EXPECT_EQ(auction.core.auctionLotItems.get(0).asset, asset);
	EXPECT_EQ(auction.core.auctionLotItems.get(0).quantity, 9);
	EXPECT_EQ(auction.core.metadataIpfsCid.get(0), 'b');
	EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
	EXPECT_EQ(nostromo.sharesManagedBy(asset, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 9);
}

TEST(ContractNostromoAuction, CreateStandardBundleAuctionEscrowsMixedLotAuction)
{
	ContractTestingNOST nostromo;
	const id seller(21, 22, 23, 24);
	const uint64 assetNameA = assetNameFromString("CRTSTA");
	const uint64 assetNameB = assetNameFromString("CRTSTB");
	const Asset assetA{seller, assetNameA};
	const Asset assetB{seller, assetNameB};

	EXPECT_EQ(nostromo.issueAsset(seller, assetNameA, 2), 2);
	EXPECT_EQ(nostromo.issueAsset(seller, assetNameB, 3), 3);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetA, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, assetB, 3), 3);

	auto input = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeTwoAssetLot(assetA, 2, assetB, 3), 100, 150, 5);

	const auto output = nostromo.createAuction(seller, input);
	ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(output.auctionId).auction;
	EXPECT_EQ(auction.core.quantityForSale, 1ULL);
	EXPECT_EQ(auction.core.minimumPurchaseQuantity, 1ULL);
	EXPECT_EQ(auction.core.initialPrice, 100ULL);
	EXPECT_EQ(auction.core.salePrice, 150ULL);
	EXPECT_EQ(auction.core.minimumBidIncrement, 5ULL);
	EXPECT_EQ(auction.core.type, NOST::EAuctionType::Standard);
	EXPECT_EQ(auction.core.auctionLotItems.get(0).asset, assetA);
	EXPECT_EQ(auction.core.auctionLotItems.get(0).quantity, 2);
	EXPECT_EQ(auction.core.auctionLotItems.get(1).asset, assetB);
	EXPECT_EQ(auction.core.auctionLotItems.get(1).quantity, 3);
	EXPECT_EQ(nostromo.sharesManagedBy(assetA, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 2);
	EXPECT_EQ(nostromo.sharesManagedBy(assetB, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 3);
}

TEST(ContractNostromoAuction, CreateStandardAuctionAcceptsMaximumLotEntriesAuction)
{
	ContractTestingNOST nostromo;
	const id seller(25, 26, 27, 28);
	const id bidder(29, 30, 31, 32);
	const uint64 assetName = assetNameFromString("MAXLOT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, NOST_AUCTION_LOT_ITEM_NUM), static_cast<sint64>(NOST_AUCTION_LOT_ITEM_NUM));
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, NOST_AUCTION_LOT_ITEM_NUM),
	          static_cast<sint64>(NOST_AUCTION_LOT_ITEM_NUM));

	const auto createOutput = nostromo.createAuction(
	    seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeFullLot(asset, 1), 100, 100, 1));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
	EXPECT_EQ(nostromo.sharesManagedBy(asset, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), static_cast<sint64>(NOST_AUCTION_LOT_ITEM_NUM));

	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 100, 100).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.managedShares(asset, bidder), static_cast<sint64>(NOST_AUCTION_LOT_ITEM_NUM));
	EXPECT_EQ(nostromo.sharesManagedBy(asset, NOST_CONTRACT_ID, NOST_CONTRACT_INDEX), 0);
}

TEST(ContractNostromoAuction, CreatePrivateAuctionsByWalletAndAccessAssetAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(31, 32, 33, 34);
		const id allowedBidder(35, 36, 37, 38);
		const uint64 assetName = assetNameFromString("PRIWAL");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 4, 12);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({allowedBidder});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(output.auctionId).auction;
		EXPECT_EQ(auction.core.visibility, NOST::EAuctionVisibility::Private);
		EXPECT_EQ(auction.allowedBidderWalletCount, 1U);
		EXPECT_EQ(auction.allowedBidderWallets.get(0), allowedBidder);
		EXPECT_EQ(auction.requiredAccessAssetCount, 0U);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(41, 42, 43, 44);
		const id gatedBidder(45, 46, 47, 48);
		const uint64 saleAssetName = assetNameFromString("PRIACC");
		const uint64 gateAssetName = assetNameFromString("GATEAS");
		const Asset saleAsset{seller, saleAssetName};
		const Asset gateAsset{gatedBidder, gateAssetName};

		EXPECT_EQ(nostromo.issueAsset(seller, saleAssetName, 5), 5);
		EXPECT_EQ(nostromo.issueAsset(gatedBidder, gateAssetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, saleAsset, 5), 5);

		auto input = ContractTestingNOST::makeBatchAuctionInput(saleAsset, 5, 20);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNOST::makeRequiredAccessAssets({gateAsset});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(output.auctionId).auction;
		EXPECT_EQ(auction.core.visibility, NOST::EAuctionVisibility::Private);
		EXPECT_EQ(auction.allowedBidderWalletCount, 0U);
		EXPECT_EQ(auction.requiredAccessAssetCount, 1U);
		EXPECT_EQ(auction.requiredAccessAssets.get(0), gateAsset);
		EXPECT_GT(nostromo.plainShares(gateAsset, gatedBidder), 0);
	}
}

TEST(ContractNostromoAuction, GetAuctionViewExposesAccessListsAndFoundFlagAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(45, 46, 47, 48);
		const id walletA(49, 50, 51, 52);
		const id walletB(53, 54, 55, 56);
		const uint64 assetName = assetNameFromString("VIEWWL");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({walletA, walletB});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auctionOutput = nostromo.getAuction(createOutput.auctionId);
		EXPECT_EQ(auctionOutput.found, 1);
		EXPECT_EQ(auctionOutput.auction.core.auctionId, createOutput.auctionId);
		EXPECT_EQ(auctionOutput.auction.allowedBidderWalletCount, 2U);
		EXPECT_TRUE(containsWallet(auctionOutput.auction.allowedBidderWallets, auctionOutput.auction.allowedBidderWalletCount, walletA));
		EXPECT_TRUE(containsWallet(auctionOutput.auction.allowedBidderWallets, auctionOutput.auction.allowedBidderWalletCount, walletB));
		EXPECT_EQ(auctionOutput.auction.requiredAccessAssetCount, 0U);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(57, 58, 59, 60);
		const id gateIssuerA(61, 62, 63, 64);
		const id gateIssuerB(65, 66, 67, 68);
		const uint64 assetName = assetNameFromString("VIEWAC");
		const Asset asset{seller, assetName};
		const Asset accessAssetA{gateIssuerA, assetNameFromString("GATEA1")};
		const Asset accessAssetB{gateIssuerB, assetNameFromString("GATEB1")};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNOST::makeRequiredAccessAssets({accessAssetA, accessAssetB});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auctionOutput = nostromo.getAuction(createOutput.auctionId);
		EXPECT_EQ(auctionOutput.found, 1);
		EXPECT_EQ(auctionOutput.auction.requiredAccessAssetCount, 2U);
		EXPECT_TRUE(containsAccessAsset(auctionOutput.auction.requiredAccessAssets, auctionOutput.auction.requiredAccessAssetCount, accessAssetA));
		EXPECT_TRUE(containsAccessAsset(auctionOutput.auction.requiredAccessAssets, auctionOutput.auction.requiredAccessAssetCount, accessAssetB));
		EXPECT_EQ(auctionOutput.auction.allowedBidderWalletCount, 0U);
	}

	{
		ContractTestingNOST nostromo;
		const id missingAuction(999, 998, 997, 996);
		const auto auctionOutput = nostromo.getAuction(missingAuction);
		EXPECT_EQ(auctionOutput.found, 0);
		EXPECT_TRUE(isZero(auctionOutput.auction.core.auctionId));
	}
}

TEST(ContractNostromoAuction, GetAuctionViewDeduplicatesPrivateAccessInputsAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(69, 70, 71, 72);
		const id wallet(73, 74, 75, 76);
		const uint64 assetName = assetNameFromString("DUPWAL");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({wallet, wallet});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		EXPECT_EQ(auction.allowedBidderWalletCount, 1U);
		EXPECT_TRUE(containsWallet(auction.allowedBidderWallets, auction.allowedBidderWalletCount, wallet));
	}

	{
		ContractTestingNOST nostromo;
		const id seller(77, 78, 79, 80);
		const id gateIssuer(81, 82, 83, 84);
		const uint64 assetName = assetNameFromString("DUPACC");
		const Asset asset{seller, assetName};
		const Asset accessAsset{gateIssuer, assetNameFromString("GATEDP")};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNOST::makeRequiredAccessAssets({accessAsset, accessAsset});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		EXPECT_EQ(auction.requiredAccessAssetCount, 1U);
		EXPECT_TRUE(containsAccessAsset(auction.requiredAccessAssets, auction.requiredAccessAssetCount, accessAsset));
	}
}

TEST(ContractNostromoAuction, PrivateAuctionAccessListsSupportMaximumViewCapacityAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(47, 48, 49, 50);
		const id allowedBidder(30127, 31127, 32127, 33127);
		const uint64 assetName = assetNameFromString("MAXWAL");
		const Asset asset{seller, assetName};
		Array<id, NOST_AUCTION_ALLOWED_WALLET_NUM> allowedWallets{};

		for (uint64 index = 0; index < NOST_AUCTION_ALLOWED_WALLET_NUM; ++index)
		{
			allowedWallets.set(index, id(30000 + index, 31000 + index, 32000 + index, 33000 + index));
		}

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 1, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = allowedWallets;
		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auctionOutput = nostromo.getAuction(createOutput.auctionId);
		EXPECT_EQ(auctionOutput.auction.allowedBidderWalletCount, NOST_AUCTION_ALLOWED_WALLET_NUM);
		EXPECT_TRUE(containsWallet(auctionOutput.auction.allowedBidderWallets, auctionOutput.auction.allowedBidderWalletCount, allowedBidder));
		EXPECT_EQ(nostromo.placeBid(allowedBidder, createOutput.auctionId, 1, 10, 10).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::Success));
	}

	{
		ContractTestingNOST nostromo;
		const id seller(57, 58, 59, 60);
		const id accessBidder(61, 62, 63, 64);
		const id gateIssuer(65, 66, 67, 68);
		const uint64 saleAssetName = assetNameFromString("MAXACC");
		const uint64 bidderAccessAssetName = assetNameFromString("MAXACB");
		const Asset saleAsset{seller, saleAssetName};
		Array<Asset, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM> requiredAssets{};

		for (uint64 index = 0; index < NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM; ++index)
		{
			requiredAssets.set(index, Asset{gateIssuer, 34000 + index});
		}
		requiredAssets.set(NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM - 1, Asset{accessBidder, bidderAccessAssetName});

		EXPECT_EQ(nostromo.issueAsset(seller, saleAssetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, saleAsset, 1), 1);
		EXPECT_EQ(nostromo.issueAsset(accessBidder, bidderAccessAssetName, 1), 1);

		auto input = ContractTestingNOST::makeBatchAuctionInput(saleAsset, 1, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = requiredAssets;
		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		const auto auctionOutput = nostromo.getAuction(createOutput.auctionId);
		EXPECT_EQ(auctionOutput.auction.requiredAccessAssetCount, NOST_AUCTION_REQUIRED_ACCESS_ASSET_NUM);
		EXPECT_TRUE(containsAccessAsset(auctionOutput.auction.requiredAccessAssets, auctionOutput.auction.requiredAccessAssetCount,
		                                Asset{accessBidder, bidderAccessAssetName}));
		EXPECT_EQ(nostromo.placeBid(accessBidder, createOutput.auctionId, 1, 10, 10).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::Success));
	}
}

TEST(ContractNostromoAuction, PrivateAuctionFeeIsDistributedAcrossRecipientsAuction)
{
	const uint8 routeModes[] = {0, 1};
	for (uint32 routeIndex = 0; routeIndex < sizeof(routeModes) / sizeof(routeModes[0]); ++routeIndex)
	{
		ContractTestingNOST nostromo;
		const uint8 routeMode = routeModes[routeIndex];
		const id seller(61 + routeIndex, 62 + routeIndex, 63 + routeIndex, 64 + routeIndex);
		const id allowedBidder(71 + routeIndex, 72 + routeIndex, 73 + routeIndex, 74 + routeIndex);
		const uint64 assetName = assetNameFromString(routeMode ? "STDENR1" : "STDENR0");

		const Asset asset{seller, assetName};

		nostromo.setRouteAllFeesToDevelopment(routeMode);
		EXPECT_EQ(nostromo.getRouteAllFeesToDevelopment(), routeMode);
		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		const sint64 sellerBefore = getBalance(seller);
		const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
		const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
		const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
		const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);
		NOST::AuctionServiceFeeBreakdown expectedBreakdown{};
		nostromo.calculateAuctionServiceFeeBreakdown(static_cast<uint64>(NOST_DEFAULT_PRIVATE_AUCTION_FEE), expectedBreakdown);
		const sint64 expectedDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedBreakdown.shareholderDividendAmount);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 4, 12);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({allowedBidder});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		EXPECT_EQ(getBalance(seller) - sellerBefore, 0);
		if (routeMode != 0)
		{
			EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
			EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
			EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
			EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
		}
		else
		{
			EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedBreakdown.managementFeeAmount);
			EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedBreakdown.developmentFeeAmount);
			EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore,
			          expectedBreakdown.takeoverCoordinatorFeeAmount);
			EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendPoolIncrease);
		}
	}
}

TEST(ContractNostromoAuction, CreateAuctionRejectsInvalidInputsAuction)
{
	ContractTestingNOST nostromo;
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

	auto invalidCid = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidCid.metadataIpfsCid = ContractTestingNOST::makeInvalidMetadataCidFirstChar();
	EXPECT_EQ(nostromo.createAuction(seller, invalidCid).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidCidUppercase = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidCidUppercase.metadataIpfsCid = ContractTestingNOST::makeInvalidMetadataCidUppercase();
	EXPECT_EQ(nostromo.createAuction(seller, invalidCidUppercase).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto emptyLot = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	emptyLot.auctionLotItems = Array<NOST::AuctionLotEntry, NOST_AUCTION_LOT_ITEM_NUM>{};
	EXPECT_EQ(nostromo.createAuction(seller, emptyLot).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto negativeQuantity = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	negativeQuantity.auctionLotItems = ContractTestingNOST::makeSingleLot(assetA, -1);
	EXPECT_EQ(nostromo.createAuction(seller, negativeQuantity).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto zeroDuration = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	zeroDuration.durationDays = 0;
	EXPECT_EQ(nostromo.createAuction(seller, zeroDuration).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto tooLongDuration = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	tooLongDuration.durationDays = NOST_AUCTION_MAX_DURATION_DAYS + 1;
	EXPECT_EQ(nostromo.createAuction(seller, tooLongDuration).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidType = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidType.auctionType = 99;
	EXPECT_EQ(nostromo.createAuction(seller, invalidType).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidAuctionType));

	auto invalidVisibility = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidVisibility.auctionVisibility = 99;
	EXPECT_EQ(nostromo.createAuction(seller, invalidVisibility).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidVisibility));

	auto invalidBatchBundle = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidBatchBundle.auctionLotItems = ContractTestingNOST::makeTwoAssetLot(assetA, 2, assetB, 3);
	EXPECT_EQ(nostromo.createAuction(seller, invalidBatchBundle).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidBatchBuyNow = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	invalidBatchBuyNow.buyNowPrice = 100;
	EXPECT_EQ(nostromo.createAuction(seller, invalidBatchBuyNow).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardMinimumPurchase = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(assetA, 1));
	invalidStandardMinimumPurchase.minimumPurchaseQuantity = 2;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardMinimumPurchase).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardIncrement = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(assetA, 1));
	invalidStandardIncrement.minimumBidIncrement = 0;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardIncrement).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardPrice = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(assetA, 1), 200, 150, 10);
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardPrice).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardSalePrice = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(assetA, 1));
	invalidStandardSalePrice.salePrice = 0;
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardSalePrice).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto invalidStandardBuyNow = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(assetA, 1), 100, 150, 10, 140);
	EXPECT_EQ(nostromo.createAuction(seller, invalidStandardBuyNow).errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto privateWithoutGate = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	privateWithoutGate.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
	EXPECT_EQ(nostromo.createAuction(seller, privateWithoutGate, NOST_DEFAULT_PRIVATE_AUCTION_FEE).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	auto privateWithBothGates = ContractTestingNOST::makeBatchAuctionInput(assetA, 5, 10);
	privateWithBothGates.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
	privateWithBothGates.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({id(99, 1, 1, 1)});
	privateWithBothGates.requiredAccessAssets = ContractTestingNOST::makeRequiredAccessAssets({Asset{altIssuer, assetNameFromString("GATINV")}});
	EXPECT_EQ(nostromo.createAuction(seller, privateWithBothGates, NOST_DEFAULT_PRIVATE_AUCTION_FEE).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::InvalidInput));
}

TEST(ContractNostromoAuction, CreateAuctionRejectsInsufficientFundsInsufficientAssetBalanceAndPauseAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(61, 62, 63, 64);
		const uint64 assetName = assetNameFromString("PRIFEE");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 4, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({id(1, 1, 1, 1)});

		const auto output = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE - 1);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientFunds));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 4);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(71, 72, 73, 74);
		const uint64 assetName = assetNameFromString("BALLOW");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 3, 10);
		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientAssetBalance));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 2);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(81, 82, 83, 84);
		const uint64 assetName = assetNameFromString("PAUSEA");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 3, 10);
		nostromo.setNow(2026, 1, 7, 11, 40, 0);
		nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
		EXPECT_TRUE(isZero(output.auctionId));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(85, 86, 87, 88);
		const uint64 assetName = assetNameFromString("BOOTPA");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 3, 10);
		nostromo.setNow(2022, 4, 13, 12, 0, 0);
		nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

		const auto output = nostromo.createAuction(seller, input);
		EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
		EXPECT_TRUE(isZero(output.auctionId));
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}
}

TEST(ContractNostromoAuction, CreateAuctionRejectsWhenAuctionStorageIsFullAuction)
{
	ContractTestingNOST nostromo;
	const id seller(87, 88, 89, 90);
	const uint64 assetName = assetNameFromString("STOFUL");
	const Asset asset{seller, assetName};

	for (uint64 index = 0; index < NOST_AUCTION_NUM; ++index)
	{
		NOST::AuctionData auction{};
		auction.core.auctionId = id(10000 + index, 11000 + index, 12000 + index, 13000 + index);
		auction.core.seller = seller;
		auction.core.status = NOST::EAuctionStatus::Active;
		ASSERT_NE(nostromo.stateData().auctionList.set(auction.core.auctionId, auction), NULL_INDEX);
	}
	ASSERT_EQ(nostromo.stateData().auctionList.population(), NOST_AUCTION_NUM);

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto output = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 1, 10));
	EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::StorageFull));
	EXPECT_TRUE(isZero(output.auctionId));
	EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
}

TEST(ContractNostromoAuction, PlaceBidRejectsWhenParticipantStorageIsFullAuction)
{
	ContractTestingNOST nostromo;
	const id seller(91, 92, 93, 94);
	const id bidder(95, 96, 97, 98);
	const uint64 assetName = assetNameFromString("PARFUL");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 1, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	for (uint64 index = 0; index < NOST_AUCTION_PARTICIPANT_NUM; ++index)
	{
		NOST::AuctionParticipantData participant{};
		participant.participant = id(14000 + index, 15000 + index, 16000 + index, 17000 + index);
		participant.bidAmount = 1;
		participant.requestedQuantity = 1;
		NOST::AuctionParticipantKey key{id(18000 + index, 19000 + index, 20000 + index, 21000 + index), participant.participant};
		ASSERT_NE(nostromo.stateData().participants.set(key, participant), NULL_INDEX);
	}
	ASSERT_EQ(nostromo.stateData().participants.population(), NOST_AUCTION_PARTICIPANT_NUM);

	const auto output = nostromo.placeBid(bidder, createOutput.auctionId, 1, 10, 10);
	EXPECT_EQ(output.errorCode, static_cast<uint8>(NOST::EAuctionError::StorageFull));
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.highestBidAmount, 0ULL);
	EXPECT_EQ(nostromo.getParticipant(createOutput.auctionId, bidder).found, 0);
}

TEST(ContractNostromoAuction, PlaceBatchBidValidatesAndRecomputesHighestBidAuction)
{
	ContractTestingNOST nostromo;
	const id seller(91, 92, 93, 94);
	const id bidderA(95, 96, 97, 98);
	const id bidderB(99, 100, 101, 102);
	const id bidderC(103, 104, 105, 106);
	const uint64 assetName = assetNameFromString("BIDBAT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 6), 6);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 6), 6);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 6, 10));
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
	EXPECT_EQ(auction.core.highestBidder, bidderA);
	EXPECT_EQ(auction.core.highestBidPrice, 20ULL);
	EXPECT_EQ(auction.core.highestBidAmount, 40ULL);

	const auto bidA2 = nostromo.placeBid(bidderA, createOutput.auctionId, 2, 14, 28);
	EXPECT_EQ(bidA2.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	EXPECT_EQ(bidA2.escrowedAmount, 28ULL);
	EXPECT_EQ(bidA2.refundedAmount, 40ULL);

	auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.highestBidder, bidderB);
	EXPECT_EQ(auction.core.highestBidPrice, 15ULL);
	EXPECT_EQ(auction.core.highestBidAmount, 45ULL);

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
	ContractTestingNOST nostromo;
	const id seller(111, 112, 113, 114);
	const id bidder(115, 116, 117, 118);
	const uint64 assetName = assetNameFromString("BIDEXT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2026, 1, 2, 8, 56, 30);
	const auto bidOutput = nostromo.placeBid(bidder, createOutput.auctionId, 1, 15, 15);
	ASSERT_EQ(bidOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.auctionDurationSeconds, NOST_SECONDS_PER_DAY + NOST_AUCTION_EXTENSION_SECONDS);
}

TEST(ContractNostromoAuction, PlaceStandardBidValidatesRefundsAndPauseAuction)
{
	ContractTestingNOST nostromo;
	const id seller(121, 122, 123, 124);
	const id bidderA(125, 126, 127, 128);
	const id bidderB(129, 130, 131, 132);
	const uint64 assetName = assetNameFromString("STDVAL");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
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
	nostromo.advanceTicks(NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

	const auto bootstrapPausedBid = nostromo.placeBid(id(141, 142, 143, 144), createOutput.auctionId, 1, 150, 150);
	EXPECT_EQ(bootstrapPausedBid.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionPaused));
}

TEST(ContractNostromoAuction, PrivateAuctionAccessRulesAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(141, 142, 143, 144);
		const id allowed(145, 146, 147, 148);
		const id denied(149, 150, 151, 152);
		const uint64 assetName = assetNameFromString("PRIBID");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 3), 3);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 3), 3);

		auto input = ContractTestingNOST::makeBatchAuctionInput(asset, 3, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.allowedBidderWallets = ContractTestingNOST::makeAllowedWallets({allowed});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		EXPECT_EQ(nostromo.placeBid(denied, createOutput.auctionId, 1, 12, 12).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::PrivateAuctionAccessDenied));
		EXPECT_EQ(nostromo.placeBid(allowed, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	}

	{
		ContractTestingNOST nostromo;
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

		auto input = ContractTestingNOST::makeBatchAuctionInput(saleAsset, 3, 10);
		input.auctionVisibility = static_cast<uint8>(NOST::EAuctionVisibility::Private);
		input.requiredAccessAssets = ContractTestingNOST::makeRequiredAccessAssets({accessAsset});

		const auto createOutput = nostromo.createAuction(seller, input, NOST_DEFAULT_PRIVATE_AUCTION_FEE);
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		EXPECT_EQ(nostromo.placeBid(denied, createOutput.auctionId, 1, 12, 12).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::PrivateAuctionAccessDenied));
		EXPECT_EQ(nostromo.placeBid(allowed, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	}
}

TEST(ContractNostromoAuction, PlaceStandardBidBuyNowFinalizesImmediatelyAuction)
{
	ContractTestingNOST nostromo;
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

	auto input = ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeTwoAssetLot(assetA, 2, assetB, 1), 100, 150, 10, 180);
	NOST::AuctionRevenueBreakdown expectedRevenue{};
	nostromo.calculateAuctionRevenueBreakdown(180ULL, expectedRevenue);
	const auto createOutput = nostromo.createAuction(seller, input);
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	const sint64 sellerBalanceBefore = getBalance(seller);

	const auto bidOutput = nostromo.placeBid(bidder, createOutput.auctionId, 1, 180, 180);
	ASSERT_EQ(bidOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(auction.core.allocatedQuantity, 1ULL);
	ASSERT_EQ(participant.found, 1);
	EXPECT_EQ(participant.participantData.allocatedQuantity, 1ULL);
	EXPECT_EQ(participant.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(participant.participantData.isWinningBid, 1u);
	EXPECT_EQ(nostromo.managedShares(assetA, bidder), 2);
	EXPECT_EQ(nostromo.managedShares(assetB, bidder), 1);
	EXPECT_EQ(getBalance(seller) - sellerBalanceBefore, expectedRevenue.sellerPayout);
}

TEST(ContractNostromoAuction, EndTickFinalizesBatchAuctionByPriceTimeAndPartialFillAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(181, 182, 183, 184);
		const id bidderA(185, 186, 187, 188);
		const id bidderB(189, 190, 191, 192);
		const id bidderC(193, 194, 195, 196);
		const uint64 assetName = assetNameFromString("BATFIN");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 4), 4);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 4), 4);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 4, 10));
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

		EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.core.allocatedQuantity, 4ULL);
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
		ContractTestingNOST nostromo;
		const id seller(197, 198, 199, 200);
		const id bidder(201, 202, 203, 204);
		const uint64 assetName = assetNameFromString("BATRET");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 5), 5);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 5), 5);

		const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 5, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 2, 12, 24).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.core.allocatedQuantity, 2ULL);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 2);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 3);
	}
}

TEST(ContractNostromoAuction, BatchFinalizationRefundsLosingBidsAndTieBreaksByBidTimeAuction)
{
	ContractTestingNOST nostromo;
	const id seller(205, 206, 207, 208);
	const id earlierBidder(209, 210, 211, 212);
	const id laterBidder(213, 214, 215, 216);
	const id lowerBidder(217, 218, 219, 220);
	const uint64 assetName = assetNameFromString("BATTIE");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 1, 5));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.seedUser(earlierBidder, 100);
	nostromo.seedUser(laterBidder, 100);
	nostromo.seedUser(lowerBidder, 100);
	const sint64 earlierBefore = getBalance(earlierBidder);
	const sint64 laterBefore = getBalance(laterBidder);
	const sint64 lowerBefore = getBalance(lowerBidder);

	ASSERT_EQ(nostromo.placeBidWithFundedReward(earlierBidder, createOutput.auctionId, 1, 10, 10).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::Success));
	nostromo.setNow(2026, 1, 1, 9, 0, 1);
	ASSERT_EQ(nostromo.placeBidWithFundedReward(laterBidder, createOutput.auctionId, 1, 10, 10).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::Success));
	nostromo.setNow(2026, 1, 1, 9, 0, 2);
	ASSERT_EQ(nostromo.placeBidWithFundedReward(lowerBidder, createOutput.auctionId, 1, 9, 9).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	const auto earlier = nostromo.getParticipant(createOutput.auctionId, earlierBidder);
	const auto later = nostromo.getParticipant(createOutput.auctionId, laterBidder);
	const auto lower = nostromo.getParticipant(createOutput.auctionId, lowerBidder);
	ASSERT_EQ(earlier.found, 1);
	ASSERT_EQ(later.found, 1);
	ASSERT_EQ(lower.found, 1);
	EXPECT_EQ(earlier.participantData.allocatedQuantity, 1ULL);
	EXPECT_EQ(later.participantData.allocatedQuantity, 0ULL);
	EXPECT_EQ(lower.participantData.allocatedQuantity, 0ULL);
	EXPECT_EQ(earlier.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(later.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(lower.participantData.escrowedAmount, 0ULL);
	EXPECT_EQ(nostromo.managedShares(asset, earlierBidder), 1);
	EXPECT_EQ(nostromo.managedShares(asset, laterBidder), 0);
	EXPECT_EQ(nostromo.managedShares(asset, lowerBidder), 0);
	EXPECT_EQ(getBalance(earlierBidder), earlierBefore - 10);
	EXPECT_EQ(getBalance(laterBidder), laterBefore);
	EXPECT_EQ(getBalance(lowerBidder), lowerBefore);
}

TEST(ContractNostromoAuction, EndTickFinalizesStandardAuctionWithoutBidAuction)
{
	ContractTestingNOST nostromo;
	const id seller(211, 212, 213, 214);
	const uint64 assetName = assetNameFromString("STDNOB");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(auction.core.allocatedQuantity, 0ULL);
	EXPECT_TRUE(isZero(auction.core.highestBidder));
	EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
}

TEST(ContractNostromoAuction, WeeklyPauseShiftsActiveAuctionDeadlineAuction)
{
	ContractTestingNOST nostromo;
	const id seller(215, 216, 217, 218);
	const uint64 assetName = assetNameFromString("PAUSHL");
	const Asset asset{seller, assetName};

	nostromo.setNow(2026, 1, 6, 11, 40, 0);
	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2026, 1, 7, 11, 40, 0);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Active);

	nostromo.setNow(2026, 1, 7, 12, 0, 0);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Active);

	nostromo.setNow(2026, 1, 7, 12, 10, 1);
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
}

TEST(ContractNostromoAuction, EndTickSkipsAuctionProcessingAtBootstrapTimeAuction)
{
	ContractTestingNOST nostromo;
	const id seller(215, 216, 217, 218);
	const uint64 assetName = assetNameFromString("BOOTTK");
	const Asset asset{seller, assetName};

	nostromo.setNow(2022, 4, 12, 12, 0, 0);
	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.setNow(2022, 4, 13, 12, 0, 0);
	nostromo.advanceAndEndTick(0);

	const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Active);
	EXPECT_EQ(auction.core.allocatedQuantity, 0ULL);
	EXPECT_EQ(nostromo.managedShares(asset, seller), 0);
}

TEST(ContractNostromoAuction, PauseShiftsSellerDecisionDeadlineAuction)
{
	ContractTestingNOST nostromo;
	const id seller(219, 220, 221, 222);
	const id bidder(223, 224, 225, 226);
	const uint64 assetName = assetNameFromString("PDSHFT");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

	const auto createOutput =
	    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY) * 1000ULL);
	auto auction = nostromo.getAuction(createOutput.auctionId).auction;
	const auto originalSellerDecisionDeadline = auction.core.sellerDecisionDeadline;
	ASSERT_EQ(auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);
	EXPECT_EQ(auction.core.sellerDecisionDeadline.getHour(), 9);
	EXPECT_EQ(auction.core.sellerDecisionDeadline.getMinute(), 0);
	EXPECT_EQ(auction.core.sellerDecisionDeadline.getSecond(), 0);

	nostromo.setNow(2026, 1, 9, 8, 59, 50);
	nostromo.beginEpoch();
	const uint32 launchPauseTicksAfterBeginEpoch = nostromo.getTicksBeforeAuctionLaunch().ticks;
	EXPECT_EQ(launchPauseTicksAfterBeginEpoch, NOST_AUCTION_POST_BEGIN_EPOCH_PAUSE_TICKS);

	nostromo.advanceAndEndTick(1000);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, launchPauseTicksAfterBeginEpoch - 1);

	nostromo.setNow(2026, 1, 9, 9, 8, 10);
	nostromo.advanceAndEndTick(0);
	auction = nostromo.getAuction(createOutput.auctionId).auction;
	ASSERT_EQ(auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);
	EXPECT_EQ(auction.core.sellerDecisionDeadline, originalSellerDecisionDeadline);

	nostromo.advanceTicks(launchPauseTicksAfterBeginEpoch - 2);
	auction = nostromo.getAuction(createOutput.auctionId).auction;
	ASSERT_EQ(auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);
	EXPECT_EQ(nostromo.getTicksBeforeAuctionLaunch().ticks, 0U);
	EXPECT_GT(auction.core.sellerDecisionDeadline, originalSellerDecisionDeadline);

	auto shiftedDeadline = auction.core.sellerDecisionDeadline;
	shiftedDeadline.add(0, 0, 0, 0, 0, -1);
	nostromo.setNow(shiftedDeadline.getYear(), shiftedDeadline.getMonth(), shiftedDeadline.getDay(), shiftedDeadline.getHour(),
	                shiftedDeadline.getMinute(), shiftedDeadline.getSecond());
	nostromo.advanceAndEndTick(0);
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);

	shiftedDeadline = auction.core.sellerDecisionDeadline;
	shiftedDeadline.add(0, 0, 0, 0, 0, 1);
	nostromo.setNow(shiftedDeadline.getYear(), shiftedDeadline.getMonth(), shiftedDeadline.getDay(), shiftedDeadline.getHour(),
	                shiftedDeadline.getMinute(), shiftedDeadline.getSecond());
	nostromo.advanceAndEndTick(0);
	auction = nostromo.getAuction(createOutput.auctionId).auction;
	EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
}

TEST(ContractNostromoAuction, EndTickFinalizesStandardAuctionAtSalePriceAuction)
{
	const uint8 routeModes[] = {0, 1};
	for (uint32 routeIndex = 0; routeIndex < sizeof(routeModes) / sizeof(routeModes[0]); ++routeIndex)
	{
		ContractTestingNOST nostromo;
		const uint8 routeMode = routeModes[routeIndex];
		const id seller(221 + routeIndex, 222 + routeIndex, 223 + routeIndex, 224 + routeIndex);
		const id bidder(225 + routeIndex, 226 + routeIndex, 227 + routeIndex, 228 + routeIndex);
		const uint64 assetName = assetNameFromString(routeMode ? "STDENR1" : "STDENR0");
		const Asset asset{seller, assetName};

		nostromo.setRouteAllFeesToDevelopment(routeMode);
		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		NOST::AuctionRevenueBreakdown expectedRevenue{};
		nostromo.calculateAuctionRevenueBreakdown(10000ULL, expectedRevenue);
		const sint64 expectedDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedRevenue.shareholderDividendAmount);

		const auto createOutput = nostromo.createAuction(
		    seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 10000, 10000, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		const sint64 sellerBalanceBefore = getBalance(seller);
		const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
		const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
		const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
		const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 10000, 10000).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.core.allocatedQuantity, 1ULL);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
		EXPECT_EQ(getBalance(seller) - sellerBalanceBefore, expectedRevenue.sellerPayout);

		if (routeMode != 0)
		{
			EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
			EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 10000ULL - expectedRevenue.sellerPayout);
			EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
			EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
		}
		else
		{
			EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedRevenue.managementFeeAmount);
			EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedRevenue.developmentFeeAmount);
			EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, expectedRevenue.takeoverCoordinatorFeeAmount);
			EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendPoolIncrease);
		}
	}
}

TEST(ContractNostromoAuction, PendingSellerDecisionAcceptRejectAndTimeoutAuction)
{
	{
		ContractTestingNOST nostromo;
		const id seller(231, 232, 233, 234);
		const id bidder(235, 236, 237, 238);
		const uint64 assetName = assetNameFromString("PENACC");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);

		const auto forbidden = nostromo.resolvePendingStandardAuction(id(999, 999, 999, 999), createOutput.auctionId, true);
		EXPECT_EQ(forbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

		const auto acceptOutput = nostromo.resolvePendingStandardAuction(seller, createOutput.auctionId, true);
		EXPECT_EQ(acceptOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(239, 240, 241, 242);
		const id bidder(243, 244, 245, 246);
		const uint64 assetName = assetNameFromString("PENREJ");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		nostromo.seedUser(bidder, 500);
		const sint64 bidderBeforeBid = getBalance(bidder);
		ASSERT_EQ(nostromo.placeBidWithFundedReward(bidder, createOutput.auctionId, 1, 120, 120).errorCode,
		          static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		const auto rejectOutput = nostromo.resolvePendingStandardAuction(seller, createOutput.auctionId, false);
		EXPECT_EQ(rejectOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		EXPECT_EQ(rejectOutput.refundedAmount, 120ULL);

		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
		EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.core.allocatedQuantity, 0ULL);
		EXPECT_TRUE(isZero(auction.core.highestBidder));
		ASSERT_EQ(participant.found, 1);
		EXPECT_EQ(participant.participantData.allocatedQuantity, 0ULL);
		EXPECT_EQ(participant.participantData.escrowedAmount, 0ULL);
		EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
		EXPECT_EQ(getBalance(bidder), bidderBeforeBid);
	}

	{
		ContractTestingNOST nostromo;
		const id seller(247, 248, 249, 250);
		const id bidder(251, 252, 253, 254);
		const uint64 assetName = assetNameFromString("PENTMO");
		const Asset asset{seller, assetName};

		EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
		EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

		const auto createOutput =
		    nostromo.createAuction(seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 100, 150, 10));
		ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
		ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 120, 120).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

		nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);
		EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::PendingSellerDecision);

		nostromo.advanceAndEndTick((NOST_AUCTION_SELLER_DECISION_WINDOW_SECONDS + 1ULL) * 1000ULL);
		const auto auction = nostromo.getAuction(createOutput.auctionId).auction;
		const auto participant = nostromo.getParticipant(createOutput.auctionId, bidder);
		EXPECT_EQ(auction.core.status, NOST::EAuctionStatus::Finalized);
		EXPECT_EQ(auction.core.allocatedQuantity, 1ULL);
		ASSERT_EQ(participant.found, 1);
		EXPECT_EQ(participant.participantData.allocatedQuantity, 1ULL);
		EXPECT_EQ(nostromo.managedShares(asset, bidder), 1);
	}
}

TEST(ContractNostromoAuction, CancelAuctionWithoutBidsDistributesFeeAuction)
{
	const uint8 routeModes[] = {0, 1};
	for (uint32 routeIndex = 0; routeIndex < sizeof(routeModes) / sizeof(routeModes[0]); ++routeIndex)
	{
		{
			ContractTestingNOST nostromo;
			const uint8 routeMode = routeModes[routeIndex];
			const id seller(261 + routeIndex, 262 + routeIndex, 263 + routeIndex, 264 + routeIndex);
			const uint64 assetName = assetNameFromString(routeMode ? "CANBT1" : "CANBT0");
			const Asset asset{seller, assetName};

			nostromo.setRouteAllFeesToDevelopment(routeMode);
			EXPECT_EQ(nostromo.issueAsset(seller, assetName, 10), 10);
			EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 10), 10);

			const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 10, 1000));
			ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			const sint64 sellerBefore = getBalance(seller);
			const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
			const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
			const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
			const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);
			NOST::AuctionServiceFeeBreakdown expectedBreakdown{};
			nostromo.calculateAuctionServiceFeeBreakdown(1000ULL, expectedBreakdown);
			const sint64 expectedDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedBreakdown.shareholderDividendAmount);

			const auto cancelOutput = nostromo.cancelAuction(seller, createOutput.auctionId, 1000);
			EXPECT_EQ(cancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			EXPECT_EQ(cancelOutput.refundedAmount, 0ULL);
			EXPECT_EQ(cancelOutput.cancellationFee, 1000ULL);
			EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Cancelled);
			EXPECT_EQ(nostromo.managedShares(asset, seller), 10);
			EXPECT_EQ(getBalance(seller) - sellerBefore, 0);
			if (routeMode != 0)
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 1000ULL);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
			}
			else
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedBreakdown.managementFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedBreakdown.developmentFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore,
				          expectedBreakdown.takeoverCoordinatorFeeAmount);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendPoolIncrease);
			}
		}

		{
			ContractTestingNOST nostromo;
			const uint8 routeMode = routeModes[routeIndex];
			const id seller(273 + routeIndex, 274 + routeIndex, 275 + routeIndex, 276 + routeIndex);
			const uint64 assetName = assetNameFromString(routeMode ? "CANST1" : "CANST0");
			const Asset asset{seller, assetName};

			nostromo.setRouteAllFeesToDevelopment(routeMode);
			EXPECT_EQ(nostromo.issueAsset(seller, assetName, 1), 1);
			EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

			const auto createOutput = nostromo.createAuction(
			    seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), 8000, 10000, 10));
			ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			const sint64 sellerBefore = getBalance(seller);
			const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
			const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
			const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
			const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);
			NOST::AuctionServiceFeeBreakdown expectedBreakdown{};
			nostromo.calculateAuctionServiceFeeBreakdown(1000ULL, expectedBreakdown);
			const sint64 expectedDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedBreakdown.shareholderDividendAmount);

			const auto cancelOutput = nostromo.cancelAuction(seller, createOutput.auctionId, 1000);
			EXPECT_EQ(cancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			EXPECT_EQ(cancelOutput.refundedAmount, 0ULL);
			EXPECT_EQ(cancelOutput.cancellationFee, 1000ULL);
			EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Cancelled);
			EXPECT_EQ(nostromo.managedShares(asset, seller), 1);
			EXPECT_EQ(getBalance(seller) - sellerBefore, 0);
			if (routeMode != 0)
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 1000ULL);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
			}
			else
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedBreakdown.managementFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedBreakdown.developmentFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore,
				          expectedBreakdown.takeoverCoordinatorFeeAmount);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendPoolIncrease);
			}
		}
	}
}

TEST(ContractNostromoAuction, CancelAuctionUsesTruncatedFeeAndAssignsServiceFeeRemainderAuction)
{
	const uint8 routeModes[] = {0, 1};
	for (uint32 routeIndex = 0; routeIndex < sizeof(routeModes) / sizeof(routeModes[0]); ++routeIndex)
	{
		{
			ContractTestingNOST nostromo;
			const uint8 routeMode = routeModes[routeIndex];
			const id batchSeller(277 + routeIndex, 278 + routeIndex, 279 + routeIndex, 280 + routeIndex);
			const uint64 batchAssetName = assetNameFromString(routeMode ? "CANRN1" : "CANRN0");
			const Asset batchAsset{batchSeller, batchAssetName};

			nostromo.setRouteAllFeesToDevelopment(routeMode);
			EXPECT_EQ(nostromo.issueAsset(batchSeller, batchAssetName, 7), 7);
			EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(batchSeller, batchAsset, 7), 7);

			const auto batchCreateOutput = nostromo.createAuction(batchSeller, ContractTestingNOST::makeBatchAuctionInput(batchAsset, 7, 333));
			ASSERT_EQ(batchCreateOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

			NOST::AuctionServiceFeeBreakdown expectedBatchBreakdown{};
			nostromo.calculateAuctionServiceFeeBreakdown(233ULL, expectedBatchBreakdown);
			const sint64 expectedBatchDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedBatchBreakdown.shareholderDividendAmount);
			const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
			const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
			const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
			const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

			const auto batchCancelOutput = nostromo.cancelAuction(batchSeller, batchCreateOutput.auctionId, 233);
			EXPECT_EQ(batchCancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			EXPECT_EQ(batchCancelOutput.cancellationFee, 233ULL);
			if (routeMode != 0)
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 233ULL);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
			}
			else
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedBatchBreakdown.managementFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedBatchBreakdown.developmentFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore,
				          expectedBatchBreakdown.takeoverCoordinatorFeeAmount);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedBatchDividendPoolIncrease);
			}
			EXPECT_EQ(expectedBatchBreakdown.shareholderDividendAmount + expectedBatchBreakdown.managementFeeAmount +
			              expectedBatchBreakdown.developmentFeeAmount + expectedBatchBreakdown.takeoverCoordinatorFeeAmount,
			          batchCancelOutput.cancellationFee);
		}

		{
			ContractTestingNOST smallFeeNostromo;
			const uint8 routeMode = routeModes[routeIndex];
			const id standardSeller(281 + routeIndex, 282 + routeIndex, 283 + routeIndex, 284 + routeIndex);
			const uint64 standardAssetName = assetNameFromString(routeMode ? "CANON1" : "CANON0");
			const Asset standardAsset{standardSeller, standardAssetName};

			smallFeeNostromo.setRouteAllFeesToDevelopment(routeMode);
			EXPECT_EQ(smallFeeNostromo.issueAsset(standardSeller, standardAssetName, 1), 1);
			EXPECT_EQ(smallFeeNostromo.transferShareManagementRightsToNostromo(standardSeller, standardAsset, 1), 1);

			const auto standardCreateOutput = smallFeeNostromo.createAuction(
			    standardSeller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(standardAsset, 1), 19, 19, 1));
			ASSERT_EQ(standardCreateOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

			NOST::AuctionServiceFeeBreakdown expectedSmallBreakdown{};
			smallFeeNostromo.calculateAuctionServiceFeeBreakdown(1ULL, expectedSmallBreakdown);
			const sint64 expectedSmallDividendPoolIncrease =
			    smallFeeNostromo.expectedDividendPoolIncrease(expectedSmallBreakdown.shareholderDividendAmount);
			const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
			const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);

			const auto standardCancelOutput = smallFeeNostromo.cancelAuction(standardSeller, standardCreateOutput.auctionId, 1);
			EXPECT_EQ(standardCancelOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			EXPECT_EQ(standardCancelOutput.cancellationFee, 1ULL);
			EXPECT_EQ(expectedSmallBreakdown.shareholderDividendAmount, 1ULL);
			EXPECT_EQ(expectedSmallBreakdown.managementFeeAmount, 0ULL);
			EXPECT_EQ(expectedSmallBreakdown.developmentFeeAmount, 0ULL);
			EXPECT_EQ(expectedSmallBreakdown.takeoverCoordinatorFeeAmount, 0ULL);
			if (routeMode != 0)
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 1ULL);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
			}
			else
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, 0);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedSmallDividendPoolIncrease);
			}
		}
	}
}

TEST(ContractNostromoAuction, CancelAuctionRejectsAfterBidAuction)
{
	ContractTestingNOST nostromo;
	const id seller(281, 282, 283, 284);
	const id bidder(285, 286, 287, 288);
	const uint64 assetName = assetNameFromString("CANINV");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, 12, 12).errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto notFound = nostromo.cancelAuction(seller, id(800, 0, 0, 0), 10);
	EXPECT_EQ(notFound.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionNotFound));

	const auto forbidden = nostromo.cancelAuction(bidder, createOutput.auctionId, 10);
	EXPECT_EQ(forbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto insufficient = nostromo.cancelAuction(seller, createOutput.auctionId, 1);
	EXPECT_EQ(insufficient.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto success = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(success.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto closed = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(closed.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));
	EXPECT_EQ(nostromo.getAuction(createOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Active);
}

TEST(ContractNostromoAuction, CancelAuctionRejectsInvalidCasesWithoutBidsAuction)
{
	ContractTestingNOST nostromo;
	const id seller(289, 290, 291, 292);
	const id outsider(293, 294, 295, 296);
	const uint64 assetName = assetNameFromString("CANIN2");
	const Asset asset{seller, assetName};

	EXPECT_EQ(nostromo.issueAsset(seller, assetName, 2), 2);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 2), 2);

	const auto createOutput = nostromo.createAuction(seller, ContractTestingNOST::makeBatchAuctionInput(asset, 2, 10));
	ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto notFound = nostromo.cancelAuction(seller, id(801, 0, 0, 0), 10);
	EXPECT_EQ(notFound.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionNotFound));

	const auto forbidden = nostromo.cancelAuction(outsider, createOutput.auctionId, 10);
	EXPECT_EQ(forbidden.errorCode, static_cast<uint8>(NOST::EAuctionError::Forbidden));

	const auto insufficient = nostromo.cancelAuction(seller, createOutput.auctionId, 1);
	EXPECT_EQ(insufficient.errorCode, static_cast<uint8>(NOST::EAuctionError::InsufficientFunds));

	const auto success = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(success.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));

	const auto closed = nostromo.cancelAuction(seller, createOutput.auctionId, 2);
	EXPECT_EQ(closed.errorCode, static_cast<uint8>(NOST::EAuctionError::AuctionClosed));
}

TEST(ContractNostromoAuction, ClosedAuctionHistoryRecordsFinalizedAndCancelledAuctionsAuction)
{
	ContractTestingNOST nostromo;
	const id finalizedSeller(501, 502, 503, 504);
	const id bidder(505, 506, 507, 508);
	const uint64 finalizedAssetName = assetNameFromString("HISFIN");
	const Asset finalizedAsset{finalizedSeller, finalizedAssetName};
	const id cancelledSeller(509, 510, 511, 512);
	const uint64 cancelledAssetName = assetNameFromString("HISCAN");
	const Asset cancelledAsset{cancelledSeller, cancelledAssetName};

	EXPECT_EQ(nostromo.issueAsset(finalizedSeller, finalizedAssetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(finalizedSeller, finalizedAsset, 1), 1);
	const auto finalizedCreateOutput =
	    nostromo.createAuction(finalizedSeller, ContractTestingNOST::makeBatchAuctionInput(finalizedAsset, 1, 10));
	ASSERT_EQ(finalizedCreateOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.placeBid(bidder, finalizedCreateOutput.auctionId, 1, 10, 10).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::Success));
	nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

	EXPECT_EQ(nostromo.issueAsset(cancelledSeller, cancelledAssetName, 1), 1);
	EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(cancelledSeller, cancelledAsset, 1), 1);
	const auto cancelledCreateOutput =
	    nostromo.createAuction(cancelledSeller, ContractTestingNOST::makeBatchAuctionInput(cancelledAsset, 1, 10));
	ASSERT_EQ(cancelledCreateOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
	ASSERT_EQ(nostromo.cancelAuction(cancelledSeller, cancelledCreateOutput.auctionId, 1).errorCode,
	          static_cast<uint8>(NOST::EAuctionError::Success));

	const auto history = nostromo.getClosedAuctionHistory();
	EXPECT_EQ(history.totalEntries, 2ULL);
	EXPECT_TRUE(containsAuctionId(history.auctionIds, history.totalEntries, finalizedCreateOutput.auctionId));
	EXPECT_TRUE(containsAuctionId(history.auctionIds, history.totalEntries, cancelledCreateOutput.auctionId));
	EXPECT_EQ(nostromo.getAuction(finalizedCreateOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Finalized);
	EXPECT_EQ(nostromo.getAuction(cancelledCreateOutput.auctionId).auction.core.status, NOST::EAuctionStatus::Cancelled);
}

TEST(ContractNostromoAuction, ClosedAuctionHistoryGetterExposesRingBufferOverwriteAuction)
{
	ContractTestingNOST nostromo;
	const id overwrittenAuctionId(22000, 22001, 22002, 22003);
	const id latestAuctionId(23000, 23001, 23002, 23003);

	nostromo.stateData().closedAuctionHistory.set(0, overwrittenAuctionId);
	nostromo.stateData().closedAuctionHistoryCounter = 1;
	for (uint64 index = 1; index < NOST_AUCTION_HISTORY_NUM; ++index)
	{
		nostromo.stateData().closedAuctionHistory.set(index, id(24000 + index, 25000 + index, 26000 + index, 27000 + index));
		++nostromo.stateData().closedAuctionHistoryCounter;
	}
	nostromo.stateData().closedAuctionHistory.set(0, latestAuctionId);
	++nostromo.stateData().closedAuctionHistoryCounter;

	const auto history = nostromo.getClosedAuctionHistory();
	EXPECT_EQ(history.totalEntries, NOST_AUCTION_HISTORY_NUM + 1ULL);
	EXPECT_FALSE(containsAuctionId(history.auctionIds, history.totalEntries, overwrittenAuctionId));
	EXPECT_TRUE(containsAuctionId(history.auctionIds, history.totalEntries, latestAuctionId));
	EXPECT_EQ(history.auctionIds.get(0), latestAuctionId);
}

TEST(ContractNostromoAuction, GovernanceAndFeeSettersAuction)
{
	ContractTestingNOST nostromo;
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
	const auto coordinatorInvalid = nostromo.setAuctionFees(ContractTestingNOST::takeoverCoordinatorWallet(), invalidCoordinatorInput);
	EXPECT_EQ(coordinatorInvalid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto coordinatorSuccess = nostromo.setAuctionFees(ContractTestingNOST::takeoverCoordinatorWallet(), coordinatorInput);
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

	const auto setManagementInvalid = nostromo.setManagement(ContractTestingNOST::takeoverCoordinatorWallet(), NULL_ID);
	EXPECT_EQ(setManagementInvalid.errorCode, static_cast<uint8>(NOST::EAuctionError::InvalidInput));

	const auto setManagementSuccess = nostromo.setManagement(ContractTestingNOST::takeoverCoordinatorWallet(), newManagement);
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

	const auto oldManagementForbidden = nostromo.setAuctionFeesByManagement(ContractTestingNOST::managementWallet(), managementInput);
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

	const uint8 routeModes[] = {0, 1};
	for (uint64 caseIndex = 0; caseIndex < sizeof(cases) / sizeof(cases[0]); ++caseIndex)
	{
		for (uint32 routeIndex = 0; routeIndex < sizeof(routeModes) / sizeof(routeModes[0]); ++routeIndex)
		{
			ContractTestingNOST nostromo;
			const uint8 routeMode = routeModes[routeIndex];
			const id seller(301 + caseIndex * 2 + routeIndex, 302 + caseIndex * 2 + routeIndex, 303 + caseIndex * 2 + routeIndex,
			                304 + caseIndex * 2 + routeIndex);
			const id bidder(401 + caseIndex * 2 + routeIndex, 402 + caseIndex * 2 + routeIndex, 403 + caseIndex * 2 + routeIndex,
			                404 + caseIndex * 2 + routeIndex);
			const Asset asset{seller, cases[caseIndex].assetName};
			NOST::AuctionRevenueBreakdown expectedRevenue{};
			nostromo.calculateAuctionRevenueBreakdown(cases[caseIndex].grossAmount, expectedRevenue);
			const sint64 expectedDividendPoolIncrease = nostromo.expectedDividendPoolIncrease(expectedRevenue.shareholderDividendAmount);

			nostromo.setRouteAllFeesToDevelopment(routeMode);
			EXPECT_EQ(nostromo.issueAsset(seller, cases[caseIndex].assetName, 1), 1);
			EXPECT_EQ(nostromo.transferShareManagementRightsToNostromo(seller, asset, 1), 1);

			const auto createOutput = nostromo.createAuction(
			    seller, ContractTestingNOST::makeStandardAuctionInput(ContractTestingNOST::makeSingleLot(asset, 1), cases[caseIndex].grossAmount,
			                                                          cases[caseIndex].grossAmount, 1));
			ASSERT_EQ(createOutput.errorCode, static_cast<uint8>(NOST::EAuctionError::Success));
			const sint64 sellerBefore = getBalance(seller);
			const sint64 managementBefore = getBalance(ContractTestingNOST::managementWallet());
			const sint64 developmentBefore = getBalance(ContractTestingNOST::developmentWallet());
			const sint64 coordinatorBefore = getBalance(ContractTestingNOST::takeoverCoordinatorWallet());
			const sint64 contractBefore = getBalance(NOST_CONTRACT_ID);
			ASSERT_EQ(nostromo.placeBid(bidder, createOutput.auctionId, 1, cases[caseIndex].grossAmount, cases[caseIndex].grossAmount).errorCode,
			          static_cast<uint8>(NOST::EAuctionError::Success));

			nostromo.advanceAndEndTick((NOST_SECONDS_PER_DAY + 1ULL) * 1000ULL);

			EXPECT_EQ(nostromo.getAuctionShareholderFeeBasisPoints(cases[caseIndex].grossAmount), cases[caseIndex].expectedShareholderFeeBp);
			EXPECT_EQ(getBalance(seller) - sellerBefore, expectedRevenue.sellerPayout);

			if (routeMode != 0)
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, 0);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore,
				          cases[caseIndex].grossAmount - expectedRevenue.sellerPayout);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore, 0);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, 0);
			}
			else
			{
				EXPECT_EQ(getBalance(ContractTestingNOST::managementWallet()) - managementBefore, expectedRevenue.managementFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::developmentWallet()) - developmentBefore, expectedRevenue.developmentFeeAmount);
				EXPECT_EQ(getBalance(ContractTestingNOST::takeoverCoordinatorWallet()) - coordinatorBefore,
				          expectedRevenue.takeoverCoordinatorFeeAmount);
				EXPECT_EQ(getBalance(NOST_CONTRACT_ID) - contractBefore, expectedDividendPoolIncrease);
			}
		}
	}
}
