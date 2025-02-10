using namespace QPI;

constexpr uint64 PFP_SALE_PRICE = 2000000000000000ULL;
constexpr uint32 PFP_MAX_NUMBER_NFT = 2097152;
constexpr uint32 PFP_MAX_COLLECTION = 32768;
constexpr uint32 PFP_SINGLE_NFT_CREATE_FEE = 5000000;
constexpr uint32 PFP_LENGTH_OF_URI = 46;
constexpr uint32 PFP_CFB_NAME = 4343363;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_2_200 = 100;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_201_1000 = 200;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_1001_2000 = 400;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_2001_3000 = 600;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_3001_4000 = 800;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_4001_5000 = 1000;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_5001_6000 = 1200;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_6001_7000 = 1400;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_7001_8000 = 1600;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_8001_9000 = 1800;
constexpr uint32 PFP_FEE_COLLECTION_CREATE_9001_10000 = 2000;
constexpr uint32 PFP_FEE_NFT_SALE_MARKET = 20;
constexpr uint32 PFP_FEE_NFT_SALE_SHAREHOLDERS = 5;

constexpr sint32 PFP_SUCCESS = 0;
constexpr sint32 PFP_INSUFFICIENT_FUND = 1;
constexpr sint32 PFP_INVALID_INPUT = 2;

//      For createCollection
constexpr sint32 PFP_INVALID_VOLUME_SIZE = 3;
constexpr sint32 PFP_INSUFFICIENT_CFB = 4;
constexpr sint32 PFP_LIMIT_COLLECTION_VOLUME = 5;
constexpr sint32 PFP_ERROR_TRANSFER_SHARE_MANAGEMENT = 6;
constexpr sint32 PFP_MAX_NUMBER_COLLECTION = 7;

//      For mint
constexpr sint32 PFP_OVERFLOW_NFT = 8;
constexpr sint32 PFP_LIMIT_HOLDING_NFT_PER_ONE_ADDRESS = 9;
constexpr sint32 PFP_NOT_COLLECTION_CREATOR = 10;
constexpr sint32 PFP_COLLECTION_FOR_DROP = 11;

//		For listInMarket & sale
constexpr sint32 PFP_NOT_POSSESOR = 12;
constexpr sint32 PFP_WRONG_NFTID = 13;
constexpr sint32 PFP_WRONG_URI = 14;
constexpr sint32 PFP_NOT_SALE_STATUS = 15;
constexpr sint32 PFP_LOW_PRICE = 16;
constexpr sint32 PFP_NOT_ASK_STATUS = 17;
constexpr sint32 PFP_NOT_OWNER = 18;
constexpr sint32 PFP_NOT_ASK_USER = 19;

//		For DropMint

constexpr sint32 PFP_NOT_COLLECTION_FOR_DROP = 20;
constexpr sint32 PFP_OVERFLOW_MAX_SIZE_PER_ONEID = 21;

//		For Auction

constexpr sint32 PFP_NOT_ENDED_AUCTION = 22;
constexpr sint32 PFP_NOT_TRADITIONAL_AUCTION = 23;
constexpr sint32 PFP_NOT_AUCTION_TIME = 24;
constexpr sint32 PFP_SMALL_PRICE = 25;
constexpr sint32 PFP_NOT_MATCH_PAYMENT_METHOD = 26;

enum PFPLogInfo {
    success = 0,
	insufficientQubic = 1,
	invalidInput = 2,
	//      For createCollection
	invalidVolumnSize = 3,
	insufficientCFB = 4,
	limitCollectionVolumn = 5,
	errorTransferShareManagement = 6,
	maxNumberOfCollection = 7,
	//      For mint
	overflowNFT = 8,
	limitHoldingNFTPerOneId = 9,
	notCollectionCreator = 10,
	collectionForDrop = 11,
	//		For listInMarket & sale
	notPossesor = 12,
	wrongNFTId = 13,
	wrongURI = 14,
	notSaleStatus = 15,
	lowPrice = 16,
	notAskStatus = 17,
	notOwner = 18,
	notAskUser = 19,
	//		For DropMint
	notCollectionForDrop = 20,
	overflowMaxSizePerOneId = 21,
	//		For Auction
	notEndedAuction = 22,
	notTraditionalAuction = 23,
	notAuctionTime = 24,
	smallPrice = 25,
	notMatchPaymentMethod = 26,
};

struct PFPLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

struct PFP2
{
};

struct PFP : public ContractBase
{

	struct settingCFBAndQubicPrice_input 
	{

		uint64 CFBPrice;                         //    The amount of $CFB per 1 USD
		uint64 QubicPrice;						 //    The amount of $Qubic per 1 USD

	};

	struct settingCFBAndQubicPrice_output 
	{

		uint32 returnCode;

	};

	struct createCollection_input 
	{

		uint64 priceForDropMint;
		uint32 volumn;
		uint32 royalty;
		uint32 maxSizePerOneId;
		Array<uint8, 64> URI;
		bit typeOfCollection;

	};

	struct createCollection_output 
	{

		uint32 returnCode;

	};

	struct mint_input
	{

		uint32 royalty;
		uint32 collectionId;
		Array<uint8, 64> URI;
		bit typeOfMint;						// 0 means that creator uses the his collection to mint the NFT. 1 means that creator mints the single NFT.

	};

	struct mint_output 
	{

		uint32 returnCode;

	};

	struct mintOfDrop_input 
	{

		uint32 collectionId;
		Array<uint8, 64> URI;

	};

	struct mintOfDrop_output 
	{

		uint32 returnCode;

	};

	struct transfer_input 
	{

		uint32 NFTid;
		id receiver;

	};

	struct transfer_output 
	{

		uint32 returnCode;

	};

	struct listInMarket_input 
	{

		uint64 price;
		uint32 NFTid;

	};

	struct listInMarket_output 
	{

		uint32 returnCode;

	};

	struct sell_input 
	{

		uint32 NFTid;
		bit methodOfPayment;

	};

	struct sell_output 
	{

		uint32 returnCode;

	};

	struct cancelSale_input 
	{

		uint32 NFTid;

	};

	struct cancelSale_output 
	{

		uint32 returnCode;

	};

	struct listInExchange_input 
	{

		uint32 possessedNFT;
		uint32 anotherNFT;

	};

	struct listInExchange_output 
	{

		uint32 returnCode;

	};

	struct cancelExchange_input 
	{

		uint32 possessedNFT;
		
	};

	struct cancelExchange_output 
	{

		uint32 returnCode;

	};

	struct makeOffer_input 
	{

		sint64 askPrice;
		uint32 NFTid;
		bit paymentMethod;
		
	};

	struct makeOffer_output 
	{

		uint32 returnCode;

	};

	struct acceptOffer_input 
	{

		uint32 NFTid;

	};

	struct acceptOffer_output 
	{

		uint32 returnCode;

	};

	struct cancelOffer_input 
	{

		uint32 NFTid;

	};

	struct cancelOffer_output 
	{

		uint32 returnCode;

	};

	struct createTraditionalAuction_input
	{

		uint64 minPrice;
		uint32 NFTId;
		uint32 startYear;
		uint32 startMonth;
		uint32 startDay;
		uint32 startHour;
		uint32 endYear;
		uint32 endMonth;
		uint32 endDay;
		uint32 endHour;
		bit paymentMethodOfAuction;
		
	};

	struct createTraditionalAuction_output
	{

		uint32 returnCode;

	};

	struct bidOnTraditionalAuction_input
	{

		uint64 price;
		uint32 NFTId;
		bit paymentMethod;

	};

	struct bidOnTraditionalAuction_output
	{

		uint32 returnCode;

	};

	struct getNumberOfNFTForUser_input
	{
		id user;
	};

	struct getNumberOfNFTForUser_output
	{
		uint32 numberOfNFT;
	};

	struct getInfoOfNFTUserPossessed_input
	{
		id user;
		uint32 NFTNumber;
	};

	struct getInfoOfNFTUserPossessed_output
	{
		id creator;                             	//      Identity of NFT creator
		id possesor;								//		Identity of NFT possesor
		id askUser;									//		Identity of Asked user
		id creatorOfAuction;						//		Creator of Auction
		sint64 salePrice;							//		This price should be set by possesor
		sint64 askMaxPrice;							//	 	This price is the max of asked prices
		uint64 currentPriceOfAuction;				//		This price is the start price of auctions
		uint32 royalty;								//		Percent from 0 ~ 100
		uint32 NFTidForExchange;					//      NFT Id that want to exchange
		Array<uint8, 64> URI;			            //      URI for this NFT
		uint8 statusOfAuction;						//		Status of Auction(0 means that there is no auction, 1 means that tranditional Auction is started, 2 means that one user buyed the NFT in traditinoal auction, 3 means that dutch auction is started)
		uint8 yearAuctionStarted;
		uint8 monthAuctionStarted;
		uint8 dayAuctionStarted;
		uint8 hourAuctionStarted;
		uint8 minuteAuctionStarted;
		uint8 secondAuctionStarted;
		uint8 yearAuctionEnded;
		uint8 monthAuctionEnded;
		uint8 dayAuctionEnded;
		uint8 hourAuctionEnded;
		uint8 minuteAuctionEnded;
		uint8 secondAuctionEnded;
		bit statusOfSale;							//		Status of Sale, 0 means possesor don't want to sell
		bit statusOfAsk;							//		Status of Ask
		bit paymentMethodOfAsk;						//		0 means the asked user want to buy using $Qubic, 1 means that want to buy using $CFB
		bit statusOfExchange;						//		Status of Exchange
		bit paymentMethodOfAuction;					//		0 means the user can buy using only $Qubic, 1 means that can buy using only $CFB

	};

	struct getInfoOfMarketplace_input
	{

	};

	struct getInfoOfMarketplace_output
	{
		uint64 priceOfCFB;							// The amount of $CFB per 1 USD
		uint64 priceOfQubic;						// The amount of $Qubic per 1 USD
		uint64 numberOfNFTIncoming;					// The number of NFT after minting of all NFTs for collection
		uint32 numberOfCollection;
		uint32 numberOfNFT;
	};

	struct getInfoOfCollectionByCreator_input
	{
		id creator;
		uint32 orderOfCollection;
	};

	struct getInfoOfCollectionByCreator_output
	{
		uint64 priceForDropMint;
		uint32 idOfCollection;
		uint32 royalty;
		sint32 currentSize;	
		uint32 maxSizeHoldingPerOneId;   
		Array<uint8, 64> URI;
		bit typeOfCollection;
	};

	struct getInfoOfCollectionById_input
	{
		uint32 idOfColletion;
	};

	struct getInfoOfCollectionById_output
	{
		id creator;	
		uint64 priceForDropMint;
		uint32 royalty;
		sint32 currentSize;	
		uint32 maxSizeHoldingPerOneId;   
		Array<uint8, 64> URI;
		bit typeOfCollection;
	};

	struct getIncomingAuctions_input
	{
		uint32 offset;
		uint32 count;
	};

	struct getIncomingAuctions_output
	{
		Array<uint32, 1024> NFTId;
	};

protected:

    uint64 priceOfCFB;							// The amount of $CFB per 1 USD
	uint64 priceOfQubic;						// The amount of $Qubic per 1 USD
	uint64 numberOfNFTIncoming;					// The number of NFT after minting of all NFTs for collection
	uint32 numberOfCollection;
	uint32 numberOfNFT;
	id cfbIssuer;
	id marketplaceOwner;

	struct InfoOfCollection 
	{

		uint64 priceForDropMint;				// the price for initial sale for NFT if the collection is for Drop.
		id creator;								// address of collection creator
		uint32 royalty;							// percent from 0 ~ 100
		sint32 currentSize;						// collection volume
		uint32 maxSizeHoldingPerOneId;          // max size that one ID can hold the NFTs of one collection.
		Array<uint8, 64> URI;			        // URI for this Collection
		bit typeOfCollection;					// 0 means that the collection is for Drop, 1 means that the collection is for normal collection

	};

	// The capacity of one collection is 53 byte

	Array<InfoOfCollection, PFP_MAX_COLLECTION> Collections;

	struct InfoOfNFT 
	{

		id creator;                             	//      Identity of NFT creator
		id possesor;								//		Identity of NFT possesor
		id askUser;									//		Identity of Asked user
		id creatorOfAuction;						//		Creator of Auction
		sint64 salePrice;							//		This price should be set by possesor
		sint64 askMaxPrice;							//	 	This price is the max of asked prices
		uint64 currentPriceOfAuction;				//		This price is the start price of auctions
		uint32 startTimeOfAuction;					//		The start time of auction
		uint32 endTimeOfAuction;					//		The end time of auction
		uint32 royalty;								//		Percent from 0 ~ 100
		uint32 NFTidForExchange;					//      NFT Id that want to exchange
		Array<uint8, 64> URI;			            //      URI for this NFT
		uint8 statusOfAuction;						//		Status of Auction(0 means that there is no auction, 1 means that tranditional Auction is started, 2 means that one user buyed the NFT in traditinoal auction, 3 means that dutch auction is started)
		bit statusOfSale;							//		Status of Sale, 0 means possesor don't want to sell
		bit statusOfAsk;							//		Status of Ask
		bit paymentMethodOfAsk;						//		0 means the asked user want to buy using $Qubic, 1 means that want to buy using $CFB
		bit statusOfExchange;						//		Status of Exchange
		bit paymentMethodOfAuction;					//		0 means the user can buy using only $Qubic, 1 means that can buy using only $CFB

	};

	// The capacity of one NFT is 238 byte;
		
	Array<InfoOfNFT, PFP_MAX_NUMBER_NFT> NFTs;

	/**
	* @return Current date from core node system
	*/

	inline static void getCurrentDate(const QPI::QpiContextFunctionCall& qpi, uint32& res) 
	{
        QUOTTERY::packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), res);
    }

	inline static bool isLeapYear(uint32 year)
	{
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
			return true; // 366-day leap year
		}
		return false; 
	}

	inline static bool isBigMonth(uint32 month)
	{
		if(month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
		{
			return true;
		}
		return false;
	}

	inline static bool checkValidTime(uint32 year, uint32 month, uint32 day, uint32 hour)
	{
		if(month == 0 || month > 12)
		{
			return false;
		}
		if(day == 0 || (isBigMonth(month) == 1 && day > 31) || (isBigMonth(month) == 0 && day > 30))
		{
			return false;
		}
		if((isLeapYear(year) == 1 && month == 2 && day > 29) || (isLeapYear(year) == 0 && month == 2 && day > 28))
		{
			return false;
		}
		if(hour >= 24)
		{
			return false;
		}
		return true;
	}

	struct settingCFBAndQubicPrice_locals
	{
		PFPLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(settingCFBAndQubicPrice)

		if(qpi.invocator() != state.marketplaceOwner)
		{
			output.returnCode = PFP_NOT_OWNER;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notOwner, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.priceOfCFB = input.CFBPrice;
		state.priceOfQubic = input.QubicPrice;

		output.returnCode = PFP_SUCCESS;
	_

	struct createCollection_locals 
	{

		InfoOfCollection newCollection;
		sint64 possessedAmount;
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		uint32 _t;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createCollection)

		if(state.numberOfCollection >= PFP_MAX_COLLECTION || state.numberOfNFTIncoming + (input.volumn == 0 ? 200: input.volumn * 1000) >= PFP_MAX_NUMBER_NFT) 
		{
			output.returnCode = PFP_OVERFLOW_NFT; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::overflowNFT, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(input.volumn > 10 || input.royalty > 100) 
		{
			output.returnCode = PFP_INVALID_VOLUME_SIZE;  			// volume size should be 0 ~ 10
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidVolumnSize, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		locals.possessedAmount = qpi.numberOfPossessedShares(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);

		if(input.volumn == 0) 
		{
			if(input.maxSizePerOneId > 200)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_2_200) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_2_200 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_2_200 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_2_200 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 200;
			state.numberOfNFTIncoming += 200;
		}

		if(input.volumn == 1) 
		{
			if(input.maxSizePerOneId > 1000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_201_1000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_201_1000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_201_1000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_201_1000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 1000;
			state.numberOfNFTIncoming += 1000;
		}

		if(input.volumn == 2) 
		{
			if(input.maxSizePerOneId > 2000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_1001_2000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_1001_2000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_1001_2000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_1001_2000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 2000;
			state.numberOfNFTIncoming += 2000;
		}

		if(input.volumn == 3) 
		{
			if(input.maxSizePerOneId > 3000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_2001_3000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_2001_3000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_2001_3000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_2001_3000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 3000;
			state.numberOfNFTIncoming += 3000;
		}

		if(input.volumn == 4) 
		{
			if(input.maxSizePerOneId > 4000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_3001_4000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_3001_4000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_3001_4000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_3001_4000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 4000;
			state.numberOfNFTIncoming += 4000;
		}

		if(input.volumn == 5) 
		{
			if(input.maxSizePerOneId > 5000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_4001_5000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_4001_5000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_4001_5000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_4001_5000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 5000;
			state.numberOfNFTIncoming += 5000;
		}

		if(input.volumn == 6) 
		{
			if(input.maxSizePerOneId > 6000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_5001_6000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_5001_6000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_5001_6000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_5001_6000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 6000;
			state.numberOfNFTIncoming += 6000;
		}

		if(input.volumn == 7) 
		{
			if(input.maxSizePerOneId > 7000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_6001_7000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_6001_7000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_6001_7000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_6001_7000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 7000;
			state.numberOfNFTIncoming += 7000;
		}

		if(input.volumn == 8) 
		{
			if(input.maxSizePerOneId > 8000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_7001_8000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_7001_8000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_7001_8000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_7001_8000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 8000;
			state.numberOfNFTIncoming += 8000;
		}

		if(input.volumn == 9) 
		{
			if(input.maxSizePerOneId > 9000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_8001_9000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = PFP_FEE_COLLECTION_CREATE_8001_9000 * state.priceOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_8001_9000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), PFP_FEE_COLLECTION_CREATE_8001_9000 * state.priceOfCFB, SELF);

			locals.newCollection.currentSize = 9000;
			state.numberOfNFTIncoming += 9000;
		}

		if(input.volumn == 10) 
		{
			if(input.maxSizePerOneId > 10000)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < PFP_FEE_COLLECTION_CREATE_9001_10000) 
			{
				output.returnCode = PFP_INSUFFICIENT_CFB;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = 10;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != PFP_FEE_COLLECTION_CREATE_9001_10000 * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), 10, SELF);

			locals.newCollection.currentSize = 10000;
			state.numberOfNFTIncoming += 10000;
		}

		locals.newCollection.creator = qpi.invocator();
		locals.newCollection.royalty = input.royalty;
		locals.newCollection.maxSizeHoldingPerOneId = input.maxSizePerOneId;
		locals.newCollection.typeOfCollection = input.typeOfCollection;
		locals.newCollection.priceForDropMint = input.priceForDropMint;

		for(locals._t = 0 ; locals._t < 64; locals._t++) 
		{
			if(locals._t >= PFP_LENGTH_OF_URI) 
			{
				locals.newCollection.URI.set(locals._t, 0);
			}
			else 
			{
				locals.newCollection.URI.set(locals._t, input.URI.get(locals._t));
			}
		}

		state.Collections.set(state.numberOfCollection, locals.newCollection);
		state.numberOfCollection++;
		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct mint_locals 
	{

		uint32 _t;
		uint32 cntOfNFTPerOne;
		InfoOfNFT newNFT;
		InfoOfCollection updatedCollection;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mint)

		if(input.typeOfMint == 1)     //     It means NFT creator mints the single NFT.
		{
			if(input.royalty >= 100)
			{
				output.returnCode = PFP_INVALID_INPUT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);
				
				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(state.numberOfNFTIncoming + 1 >= PFP_MAX_NUMBER_NFT) 
			{
				output.returnCode = PFP_OVERFLOW_NFT; 
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::overflowNFT, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocationReward() < PFP_SINGLE_NFT_CREATE_FEE)  //     The fee for single NFT should be more than 5M QU
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);
				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(qpi.invocationReward() > PFP_SINGLE_NFT_CREATE_FEE)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - PFP_SINGLE_NFT_CREATE_FEE);
			}

			locals.newNFT.royalty = input.royalty;
			state.numberOfNFTIncoming++;
		}

		else 
		{
			if(state.Collections.get(input.collectionId).creator != qpi.invocator())
			{
				output.returnCode = PFP_NOT_COLLECTION_CREATOR;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notCollectionCreator, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.Collections.get(input.collectionId).typeOfCollection == 0)
			{
				output.returnCode = PFP_COLLECTION_FOR_DROP;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::collectionForDrop, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.Collections.get(input.collectionId).currentSize <= 0) 
			{
				output.returnCode = PFP_LIMIT_COLLECTION_VOLUME;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::limitCollectionVolumn, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.newNFT.royalty = state.Collections.get(input.collectionId).royalty;   // The royalty should be set as the royalty to be set in collection

			locals.updatedCollection.creator = state.Collections.get(input.collectionId).creator;
			locals.updatedCollection.currentSize = state.Collections.get(input.collectionId).currentSize - 1;
			locals.updatedCollection.maxSizeHoldingPerOneId = state.Collections.get(input.collectionId).maxSizeHoldingPerOneId;
			locals.updatedCollection.royalty = state.Collections.get(input.collectionId).royalty;
			locals.updatedCollection.priceForDropMint = state.Collections.get(input.collectionId).priceForDropMint;
			locals.updatedCollection.typeOfCollection = state.Collections.get(input.collectionId).typeOfCollection;

			state.Collections.set(input.collectionId, locals.updatedCollection);
		}

		for(locals._t = 0 ; locals._t < 64; locals._t++) 
		{
			if(locals._t >= PFP_LENGTH_OF_URI) 
			{
				locals.newNFT.URI.set(locals._t, 0);
			}
			else 
			{
				locals.newNFT.URI.set(locals._t, input.URI.get(locals._t));
			}
		}

		locals.newNFT.creator = qpi.invocator();
		locals.newNFT.possesor = qpi.invocator();	
		locals.newNFT.NFTidForExchange = PFP_MAX_NUMBER_NFT;
		locals.newNFT.salePrice = PFP_SALE_PRICE;
		locals.newNFT.statusOfAsk = 0;
		locals.newNFT.statusOfSale= 0;
		locals.newNFT.paymentMethodOfAsk = 0;
		locals.newNFT.askMaxPrice = 0;
		locals.newNFT.statusOfExchange = 0;
		locals.newNFT.askUser = NULL_ID;
		locals.newNFT.creatorOfAuction = NULL_ID;
		locals.newNFT.currentPriceOfAuction = 0;
		locals.newNFT.startTimeOfAuction = 0;
		locals.newNFT.endTimeOfAuction = 0;
		locals.newNFT.statusOfAuction = 0;
		locals.newNFT.paymentMethodOfAuction = 0;
		
		state.NFTs.set(state.numberOfNFT, locals.newNFT);
		state.numberOfNFT++;
		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct mintOfDrop_locals 
	{
		
		InfoOfCollection updatedCollection;
		InfoOfNFT newNFT;
		InfoOfCollection newCollection;
		uint64 cntOfNFTHoldingPerOneId;
		uint32 _t;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mintOfDrop)

		if(state.Collections.get(input.collectionId).typeOfCollection == 1)
		{
			output.returnCode = PFP_NOT_COLLECTION_FOR_DROP;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notCollectionForDrop, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.Collections.get(input.collectionId).currentSize <= 0) 
		{
			output.returnCode = PFP_LIMIT_COLLECTION_VOLUME;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::limitCollectionVolumn, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		locals.cntOfNFTHoldingPerOneId = 0;

		for(locals._t = 0; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).creator == state.Collections.get(input.collectionId).creator && state.NFTs.get(locals._t).possesor == qpi.invocator())
			{
				locals.cntOfNFTHoldingPerOneId++;
			}
		}

		if(locals.cntOfNFTHoldingPerOneId + 1 >= state.Collections.get(input.collectionId).maxSizeHoldingPerOneId)
		{
			output.returnCode = PFP_OVERFLOW_MAX_SIZE_PER_ONEID;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::overflowMaxSizePerOneId, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if((uint64)qpi.invocationReward() < state.Collections.get(input.collectionId).priceForDropMint)
		{
			output.returnCode = PFP_INSUFFICIENT_FUND;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if((uint64)qpi.invocationReward() > state.Collections.get(input.collectionId).priceForDropMint)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.Collections.get(input.collectionId).priceForDropMint);
		}

		locals.updatedCollection.creator = state.Collections.get(input.collectionId).creator;
		locals.updatedCollection.currentSize = state.Collections.get(input.collectionId).currentSize - 1;
		locals.updatedCollection.maxSizeHoldingPerOneId = state.Collections.get(input.collectionId).maxSizeHoldingPerOneId;
		locals.updatedCollection.royalty = state.Collections.get(input.collectionId).royalty;
		locals.updatedCollection.priceForDropMint = state.Collections.get(input.collectionId).priceForDropMint;
		locals.updatedCollection.typeOfCollection = state.Collections.get(input.collectionId).typeOfCollection;

		state.Collections.set(input.collectionId, locals.updatedCollection);

		locals.newNFT.creator = state.Collections.get(input.collectionId).creator;
		locals.newNFT.possesor = qpi.invocator();
		locals.newNFT.royalty = state.Collections.get(input.collectionId).royalty;
		locals.newNFT.NFTidForExchange = PFP_MAX_NUMBER_NFT;
		locals.newNFT.salePrice = PFP_SALE_PRICE;
		locals.newNFT.statusOfAsk = 0;
		locals.newNFT.statusOfSale= 0;
		locals.newNFT.paymentMethodOfAsk = 0;
		locals.newNFT.askMaxPrice = 0;
		locals.newNFT.statusOfExchange = 0;
		locals.newNFT.askUser = NULL_ID;
		locals.newNFT.creatorOfAuction = NULL_ID;
		locals.newNFT.currentPriceOfAuction = 0;
		locals.newNFT.startTimeOfAuction = 0;
		locals.newNFT.endTimeOfAuction = 0;
		locals.newNFT.statusOfAuction = 0;
		locals.newNFT.paymentMethodOfAuction = 0;

		for(locals._t = 0 ; locals._t < 64; locals._t++) 
		{
			if(locals._t >= PFP_LENGTH_OF_URI) 
			{
				locals.newNFT.URI.set(locals._t, 0);
			}
			else 
			{
				locals.newNFT.URI.set(locals._t, input.URI.get(locals._t));
			}
		}

		state.NFTs.set(state.numberOfNFT, locals.newNFT);
		state.numberOfNFT++;
		output.returnCode = PFP_SUCCESS;

		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct transfer_locals 
	{

		InfoOfNFT transferNFT;
		uint32 curDate;
		uint32 _t;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(transfer)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.transferNFT.possesor = input.receiver;
		locals.transferNFT.salePrice = PFP_SALE_PRICE;
		locals.transferNFT.statusOfSale = 0;

		locals.transferNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.transferNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.transferNFT.statusOfAsk = state.NFTs.get(input.NFTid).statusOfAsk;
		locals.transferNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTid).paymentMethodOfAsk;
		locals.transferNFT.askUser = state.NFTs.get(input.NFTid).askUser;
		locals.transferNFT.askMaxPrice = state.NFTs.get(input.NFTid).askMaxPrice;
		locals.transferNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.transferNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;
		locals.transferNFT.currentPriceOfAuction = 0;
		locals.transferNFT.startTimeOfAuction = 0;
		locals.transferNFT.endTimeOfAuction = 0;
		locals.transferNFT.statusOfAuction = 0;
		locals.transferNFT.paymentMethodOfAuction = 0;
		locals.transferNFT.creatorOfAuction = NULL_ID;
		

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.transferNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.transferNFT);
		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct listInMarket_locals 
	{

		InfoOfNFT saleNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInMarket)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())   //		Checking the possesor
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.saleNFT.salePrice = input.price;
		locals.saleNFT.statusOfSale = 1;
		locals.saleNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.saleNFT.possesor = state.NFTs.get(input.NFTid).possesor;
		locals.saleNFT.askUser = state.NFTs.get(input.NFTid).askUser;
		locals.saleNFT.askMaxPrice = state.NFTs.get(input.NFTid).askMaxPrice;
		locals.saleNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.saleNFT.statusOfAsk = state.NFTs.get(input.NFTid).statusOfAsk;
		locals.saleNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTid).paymentMethodOfAsk;
		locals.saleNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.saleNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;
		locals.saleNFT.currentPriceOfAuction = 0;
		locals.saleNFT.startTimeOfAuction = 0;
		locals.saleNFT.endTimeOfAuction = 0;
		locals.saleNFT.statusOfAuction = 0;
		locals.saleNFT.paymentMethodOfAuction = 0;
		locals.saleNFT.creatorOfAuction = NULL_ID;
		

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.saleNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.saleNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct sell_locals 
	{

		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
		sint64 transferredAmountOfCFB;
		sint64 marketFee;
		sint64 creatorFee;
		sint64 shareHolderFee;
		sint64 possessedCFBAmount;
		sint64 transferredCFBAmount;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(sell)

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfSale == 0) 
		{
			output.returnCode = PFP_NOT_SALE_STATUS;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notSaleStatus };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(input.methodOfPayment == 0)
		{
			if(state.NFTs.get(input.NFTid).salePrice > qpi.invocationReward()) 
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return ;
			}

			if(state.NFTs.get(input.NFTid).salePrice < qpi.invocationReward())
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.NFTs.get(input.NFTid).salePrice);
			}

			locals.creatorFee = div(state.NFTs.get(input.NFTid).salePrice * state.NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
			locals.marketFee = div(state.NFTs.get(input.NFTid).salePrice * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
			locals.shareHolderFee = div(state.NFTs.get(input.NFTid).salePrice * PFP_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

			qpi.distributeDividends(div(locals.shareHolderFee * 1ULL, 676ULL));
			qpi.transfer(state.NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.NFTs.get(input.NFTid).possesor, state.NFTs.get(input.NFTid).salePrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
		}

		else 
		{
			locals.possessedCFBAmount = qpi.numberOfPossessedShares(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);
			if((uint64)state.NFTs.get(input.NFTid).salePrice >= div(locals.possessedCFBAmount * 1ULL, state.priceOfCFB) * state.priceOfQubic) 
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return ;
			}

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			locals.transferredAmountOfCFB = div(state.NFTs.get(input.NFTid).salePrice * 1ULL, state.priceOfQubic) * state.priceOfCFB;

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = locals.transferredAmountOfCFB;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != div(state.NFTs.get(input.NFTid).salePrice * 1ULL, state.priceOfQubic) * state.priceOfCFB)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			locals.creatorFee = div(locals.transferredAmountOfCFB * state.NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
			locals.marketFee = div(locals.transferredAmountOfCFB * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.transferredAmountOfCFB - locals.creatorFee - locals.marketFee, state.NFTs.get(input.NFTid).possesor);
		}

		locals.updatedNFT.possesor = qpi.invocator();
		locals.updatedNFT.salePrice = PFP_SALE_PRICE;
		locals.updatedNFT.statusOfSale = 0;

		locals.updatedNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.NFTid).statusOfAsk;
		locals.updatedNFT.askUser = state.NFTs.get(input.NFTid).askUser;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTid).paymentMethodOfAsk;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.NFTid).askMaxPrice;
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;
		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;
		
		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);
		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct cancelSale_locals
	{
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelSale)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)									// NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())   			// Checking the possesor
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT.salePrice = PFP_SALE_PRICE;
		locals.updatedNFT.statusOfSale = 0;

		locals.updatedNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.updatedNFT.possesor = state.NFTs.get(input.NFTid).possesor;
		locals.updatedNFT.askUser = state.NFTs.get(input.NFTid).askUser;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTid).paymentMethodOfAsk;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.NFTid).askMaxPrice;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.NFTid).statusOfAsk;
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;
		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct listInExchange_locals 
	{
		
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInExchange)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= PFP_MAX_NUMBER_NFT || input.anotherNFT >= PFP_MAX_NUMBER_NFT )		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.possessedNFT).endTimeOfAuction || locals.curDate <= state.NFTs.get(input.anotherNFT).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).possesor != qpi.invocator())
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.anotherNFT).NFTidForExchange == input.possessedNFT)
		{
			locals.updatedNFT.NFTidForExchange = PFP_MAX_NUMBER_NFT;
			locals.updatedNFT.statusOfExchange = 0;
			locals.updatedNFT.currentPriceOfAuction = 0;
			locals.updatedNFT.startTimeOfAuction = 0;
			locals.updatedNFT.endTimeOfAuction = 0;
			locals.updatedNFT.statusOfAuction = 0;
			locals.updatedNFT.paymentMethodOfAuction = 0;
			locals.updatedNFT.creatorOfAuction = NULL_ID;

			locals.updatedNFT.creator = state.NFTs.get(input.possessedNFT).creator;
			locals.updatedNFT.possesor = state.NFTs.get(input.anotherNFT).possesor;
			locals.updatedNFT.askUser = state.NFTs.get(input.possessedNFT).askUser;
			locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.possessedNFT).paymentMethodOfAsk;
			locals.updatedNFT.salePrice = state.NFTs.get(input.possessedNFT).salePrice;
			locals.updatedNFT.statusOfSale = state.NFTs.get(input.possessedNFT).statusOfSale;
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.possessedNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.possessedNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.possessedNFT).statusOfAsk;
			

			for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.possessedNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.possessedNFT, locals.updatedNFT);

			locals.updatedNFT.creator = state.NFTs.get(input.anotherNFT).creator;
			locals.updatedNFT.possesor = state.NFTs.get(input.possessedNFT).possesor;
			locals.updatedNFT.askUser = state.NFTs.get(input.anotherNFT).askUser;
			locals.updatedNFT.salePrice = state.NFTs.get(input.anotherNFT).salePrice;
			locals.updatedNFT.statusOfSale = state.NFTs.get(input.anotherNFT).statusOfSale;
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.anotherNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.anotherNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.anotherNFT).statusOfAsk;
			locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.anotherNFT).paymentMethodOfAsk;
			

			for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.anotherNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.anotherNFT, locals.updatedNFT);
		}

		else 
		{
			locals.updatedNFT.NFTidForExchange = input.anotherNFT;
			locals.updatedNFT.statusOfExchange = 1;

			locals.updatedNFT.creator = state.NFTs.get(input.possessedNFT).creator;
			locals.updatedNFT.possesor = state.NFTs.get(input.possessedNFT).possesor;
			locals.updatedNFT.askUser = state.NFTs.get(input.possessedNFT).askUser;
			locals.updatedNFT.salePrice = state.NFTs.get(input.possessedNFT).salePrice;
			locals.updatedNFT.statusOfSale = state.NFTs.get(input.possessedNFT).statusOfSale;
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.possessedNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.possessedNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.possessedNFT).statusOfAsk;
			locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.possessedNFT).paymentMethodOfAsk;
			locals.updatedNFT.currentPriceOfAuction = 0;
			locals.updatedNFT.startTimeOfAuction = 0;
			locals.updatedNFT.endTimeOfAuction = 0;
			locals.updatedNFT.statusOfAuction = 0;
			locals.updatedNFT.creatorOfAuction = NULL_ID;
			

			for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.possessedNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.possessedNFT, locals.updatedNFT);
		}
		
		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct cancelExchange_locals
	{

		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelExchange)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= PFP_MAX_NUMBER_NFT)		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = PFP_WRONG_NFTID; 
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.possessedNFT).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).possesor != qpi.invocator())
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT.NFTidForExchange = PFP_MAX_NUMBER_NFT;
		locals.updatedNFT.statusOfExchange = 0;

		locals.updatedNFT.creator = state.NFTs.get(input.possessedNFT).creator;
		locals.updatedNFT.possesor = state.NFTs.get(input.possessedNFT).possesor;
		locals.updatedNFT.askUser = state.NFTs.get(input.possessedNFT).askUser;
		locals.updatedNFT.salePrice = state.NFTs.get(input.possessedNFT).salePrice;
		locals.updatedNFT.statusOfSale = state.NFTs.get(input.possessedNFT).statusOfSale;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.possessedNFT).askMaxPrice;
		locals.updatedNFT.royalty = state.NFTs.get(input.possessedNFT).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.possessedNFT).statusOfAsk;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.possessedNFT).paymentMethodOfAsk;
		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;
		

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.possessedNFT).URI.get(locals._t));
		}

		state.NFTs.set(input.possessedNFT, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct makeOffer_locals 
	{

		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT AskedNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(makeOffer)

		if(input.NFTid >= PFP_MAX_NUMBER_NFT || input.askPrice == 0)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 1)
		{
			if(input.askPrice <= state.NFTs.get(input.NFTid).askMaxPrice && input.paymentMethod == state.NFTs.get(input.NFTid).paymentMethodOfAsk
			|| (input.paymentMethod == 1 && state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0 && div(input.askPrice * 1ULL, state.priceOfCFB) * state.priceOfQubic <= (uint64)state.NFTs.get(input.NFTid).askMaxPrice)
			|| (input.paymentMethod == 0 && state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1 && div(input.askPrice * 1ULL, state.priceOfQubic) * state.priceOfCFB <= (uint64)state.NFTs.get(input.NFTid).askMaxPrice))
			{
				output.returnCode = PFP_LOW_PRICE;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::lowPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
		}

		if(input.paymentMethod == 0)
		{
			if(qpi.invocationReward() < input.askPrice)
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return;
			}

			if(qpi.invocationReward() > input.askPrice)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.askPrice);
			}
		}
		
		else if(input.paymentMethod == 1)
		{
			if(qpi.numberOfPossessedShares(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX) < input.askPrice)
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = input.askPrice;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != input.askPrice)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);

				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.askPrice, SELF);
		}

		locals.AskedNFT.statusOfAsk = 1;
		locals.AskedNFT.askUser = qpi.invocator();
		locals.AskedNFT.askMaxPrice = input.askPrice;
		locals.AskedNFT.paymentMethodOfAsk = input.paymentMethod;

		locals.AskedNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.AskedNFT.possesor = state.NFTs.get(input.NFTid).possesor;
		locals.AskedNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.AskedNFT.salePrice = state.NFTs.get(input.NFTid).salePrice;
		locals.AskedNFT.statusOfSale = state.NFTs.get(input.NFTid).statusOfSale;
		locals.AskedNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.AskedNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;
		locals.AskedNFT.currentPriceOfAuction = 0;
		locals.AskedNFT.startTimeOfAuction = 0;
		locals.AskedNFT.endTimeOfAuction = 0;
		locals.AskedNFT.statusOfAuction = 0;
		locals.AskedNFT.paymentMethodOfAuction = 0;
		locals.AskedNFT.creatorOfAuction = NULL_ID;
		
		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.AskedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.AskedNFT);

		output.returnCode = PFP_SUCCESS;

		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct acceptOffer_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
		sint64 creatorFee;
		sint64 marketFee;
		sint64 shareHolderFee;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(acceptOffer)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 0)
		{
			output.returnCode = PFP_NOT_ASK_STATUS;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		locals.creatorFee = div(state.NFTs.get(input.NFTid).askMaxPrice * state.NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
		locals.marketFee = div(state.NFTs.get(input.NFTid).askMaxPrice * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
		locals.shareHolderFee = div(state.NFTs.get(input.NFTid).askMaxPrice * PFP_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

		if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			qpi.distributeDividends(div(locals.shareHolderFee * 1ULL, 676ULL));
			qpi.transfer(state.NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.NFTs.get(input.NFTid).possesor, state.NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
		}

		else if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.NFTs.get(input.NFTid).askMaxPrice - locals.marketFee;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != state.NFTs.get(input.NFTid).askMaxPrice - locals.marketFee)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);
				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, SELF, SELF, locals.creatorFee, state.NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, SELF, SELF, state.NFTs.get(input.NFTid).askMaxPrice - locals.marketFee - locals.creatorFee, state.NFTs.get(input.NFTid).possesor);
			
		}

		locals.updatedNFT.possesor = state.NFTs.get(input.NFTid).askUser;
		locals.updatedNFT.statusOfAsk = 0;
		locals.updatedNFT.askUser = NULL_ID;
		locals.updatedNFT.askMaxPrice = 0;
		locals.updatedNFT.paymentMethodOfAsk = 0;
		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = PFP_SALE_PRICE;
		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;

		locals.updatedNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct cancelOffer_locals
	{

		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelOffer)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= PFP_MAX_NUMBER_NFT)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 0 || state.NFTs.get(input.NFTid).askUser != qpi.invocator())
		{
			output.returnCode = PFP_NOT_ASK_STATUS;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			qpi.transfer(qpi.invocator(), state.NFTs.get(input.NFTid).askMaxPrice);
		}

		else if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.NFTs.get(input.NFTid).askMaxPrice;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != state.NFTs.get(input.NFTid).askMaxPrice)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);

				return ;
			}

			qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, SELF, SELF, state.NFTs.get(input.NFTid).askMaxPrice, qpi.invocator());
			
		}

		locals.updatedNFT.creator = state.NFTs.get(input.NFTid).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTid).royalty;
		locals.updatedNFT.possesor = state.NFTs.get(input.NFTid).possesor;
		locals.updatedNFT.statusOfSale = state.NFTs.get(input.NFTid).statusOfSale;
		locals.updatedNFT.salePrice = state.NFTs.get(input.NFTid).salePrice;
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTid).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTid).statusOfExchange;

		locals.updatedNFT.statusOfAsk = 0;
		locals.updatedNFT.askUser = NULL_ID;
		locals.updatedNFT.askMaxPrice = 0;
		locals.updatedNFT.paymentMethodOfAsk = 0;

		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct createTraditionalAuction_locals
	{
		InfoOfNFT updatedNFT;
		uint32 curDate;
		uint32 startDate;
		uint32 endDate;
		uint32 _t;
		PFPLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createTraditionalAuction)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTId >= PFP_MAX_NUMBER_NFT || checkValidTime(input.startYear, input.startMonth, input.startDay, input.startHour) == 0 || checkValidTime(input.endYear, input.endMonth, input.endDay, input.endHour) == 0)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		QUOTTERY::packQuotteryDate(input.startYear, input.startMonth, input.startDay, input.startHour, 0, 0, locals.startDate);
		QUOTTERY::packQuotteryDate(input.endYear, input.endMonth, input.endDay, input.endHour, 0, 0, locals.endDate);
		getCurrentDate(qpi, locals.curDate);

		if(locals.startDate <= locals.curDate || locals.endDate <= locals.startDate)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		if(locals.curDate <= state.NFTs.get(input.NFTId).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_ENDED_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTId).possesor != qpi.invocator())
		{
			output.returnCode = PFP_NOT_POSSESOR;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT.creator = state.NFTs.get(input.NFTId).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTId).royalty;
		locals.updatedNFT.possesor = state.NFTs.get(input.NFTId).possesor;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.NFTId).statusOfAsk;
		locals.updatedNFT.askUser = state.NFTs.get(input.NFTId).askUser;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.NFTId).askMaxPrice;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTId).paymentMethodOfAsk;

		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = PFP_SALE_PRICE;
		locals.updatedNFT.NFTidForExchange = 0;
		locals.updatedNFT.statusOfExchange = 0;

		locals.updatedNFT.currentPriceOfAuction = input.minPrice;
		locals.updatedNFT.startTimeOfAuction = locals.startDate;
		locals.updatedNFT.endTimeOfAuction = locals.endDate;
		locals.updatedNFT.statusOfAuction = 1;
		locals.updatedNFT.paymentMethodOfAuction = input.paymentMethodOfAuction;
		locals.updatedNFT.creatorOfAuction = qpi.invocator();

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTId).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_


	struct bidOnTraditionalAuction_locals
	{

		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
		uint64 marketFee;
		uint64 creatorFee;
		uint64 shareHolderFee;
		uint64 possessedAmount;
		uint32 _t;
		uint32 curDate;
		PFPLogger log;

	};

	PUBLIC_PROCEDURE_WITH_LOCALS(bidOnTraditionalAuction)

		if(input.NFTId >= PFP_MAX_NUMBER_NFT)
		{
			output.returnCode = PFP_INVALID_INPUT;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		if(state.NFTs.get(input.NFTId).statusOfAuction != 1 && state.NFTs.get(input.NFTId).statusOfAuction != 2)
		{
			output.returnCode = PFP_NOT_TRADITIONAL_AUCTION;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notTraditionalAuction, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate < state.NFTs.get(input.NFTId).startTimeOfAuction || locals.curDate > state.NFTs.get(input.NFTId).endTimeOfAuction)
		{
			output.returnCode = PFP_NOT_AUCTION_TIME;
			locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notAuctionTime, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(state.NFTs.get(input.NFTId).paymentMethodOfAuction == 0)
		{
			if(input.paymentMethod == 1)
			{
				output.returnCode = PFP_NOT_MATCH_PAYMENT_METHOD;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(input.price <= state.NFTs.get(input.NFTId).currentPriceOfAuction)
			{
				output.returnCode = PFP_SMALL_PRICE;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if((uint64)qpi.invocationReward() < input.price)
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if((uint64)qpi.invocationReward() > input.price)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.price);
			}

			if(state.NFTs.get(input.NFTId).statusOfAuction == 1)
			{
				locals.marketFee = div(input.price * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.shareHolderFee = div(input.price * PFP_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 100ULL);

				qpi.distributeDividends(div(locals.shareHolderFee * 1ULL, 676ULL));
				qpi.transfer(state.NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.NFTs.get(input.NFTId).possesor, input.price - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			}

			if(state.NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.creatorFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.marketFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.shareHolderFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * PFP_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 100ULL);
				
				qpi.distributeDividends(div(locals.shareHolderFee * 1ULL, 676ULL));
				qpi.transfer(state.NFTs.get(input.NFTId).possesor, state.NFTs.get(input.NFTId).currentPriceOfAuction);
				qpi.transfer(state.NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.NFTs.get(input.NFTId).creatorOfAuction, input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction - locals.creatorFee - locals.marketFee);
			}
			
			locals.updatedNFT.currentPriceOfAuction = input.price;
		}

		if(state.NFTs.get(input.NFTId).paymentMethodOfAuction == 1)
		{
			if(input.paymentMethod == 0)
			{
				output.returnCode = PFP_NOT_MATCH_PAYMENT_METHOD;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(input.price <= state.NFTs.get(input.NFTId).currentPriceOfAuction)
			{
				output.returnCode = PFP_SMALL_PRICE;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.possessedAmount = qpi.numberOfPossessedShares(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX);

			if(locals.possessedAmount < input.price)
			{
				output.returnCode = PFP_INSUFFICIENT_FUND;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.transferShareManagementRights_input.asset.assetName = PFP_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = PFP_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = input.price;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			if(locals.transferShareManagementRights_output.transferredNumberOfShares != input.price)
			{
				output.returnCode = PFP_ERROR_TRANSFER_SHARE_MANAGEMENT;
				locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::errorTransferShareManagement, 0 };
				LOG_INFO(locals.log);

				return ;
			}

			if(state.NFTs.get(input.NFTId).statusOfAuction == 1)
			{
				locals.marketFee = div(input.price * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - locals.creatorFee - locals.marketFee, state.NFTs.get(input.NFTId).possesor);
				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
			}
			if(state.NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.marketFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * PFP_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), state.NFTs.get(input.NFTId).currentPriceOfAuction, state.NFTs.get(input.NFTId).possesor);
				qpi.transferShareOwnershipAndPossession(PFP_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction - locals.marketFee - locals.creatorFee, state.NFTs.get(input.NFTId).creatorOfAuction);
			}
			
			locals.updatedNFT.currentPriceOfAuction = div(input.price, state.priceOfCFB) * state.priceOfQubic;
		}

		locals.updatedNFT.possesor = qpi.invocator();

		locals.updatedNFT.creator = state.NFTs.get(input.NFTId).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTId).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.NFTId).statusOfAsk;
		locals.updatedNFT.askUser = state.NFTs.get(input.NFTId).askUser;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.NFTId).askMaxPrice;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTId).paymentMethodOfAsk;

		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = PFP_SALE_PRICE;
		locals.updatedNFT.NFTidForExchange = 0;
		locals.updatedNFT.statusOfExchange = 0;

		locals.updatedNFT.startTimeOfAuction = state.NFTs.get(input.NFTId).startTimeOfAuction;
		locals.updatedNFT.endTimeOfAuction = state.NFTs.get(input.NFTId).endTimeOfAuction;
		locals.updatedNFT.statusOfAuction = 2;
		locals.updatedNFT.paymentMethodOfAuction = state.NFTs.get(input.NFTId).paymentMethodOfAuction;
		locals.updatedNFT.creatorOfAuction = state.NFTs.get(input.NFTId).creatorOfAuction;

		for(locals._t = 0; locals._t < PFP_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTId).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = PFP_SUCCESS;
		locals.log = PFPLogger{ PFP_CONTRACT_INDEX, PFPLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct getNumberOfNFTForUser_locals
	{
		uint32 curDate;
		uint32 _t;

	};

	PUBLIC_FUNCTION_WITH_LOCALS(getNumberOfNFTForUser)

		getCurrentDate(qpi, locals.curDate);

		output.numberOfNFT = 0;

		for(locals._t = 0 ; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).possesor == input.user && locals.curDate > state.NFTs.get(locals._t).endTimeOfAuction)
			{
				output.numberOfNFT++;
			}
		}
	_

	struct getInfoOfNFTUserPossessed_locals
	{
		uint32 curDate;
		uint32 _t, _r;
		uint32 cnt;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfNFTUserPossessed)

		getCurrentDate(qpi, locals.curDate);

		locals.cnt = 0;

		for(locals._t = 0 ; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).possesor == input.user && locals.curDate > state.NFTs.get(locals._t).endTimeOfAuction)
			{
				locals.cnt++;
				if(input.NFTNumber == locals.cnt)
				{
					output.salePrice = state.NFTs.get(locals._t).salePrice;
					output.askMaxPrice = state.NFTs.get(locals._t).askMaxPrice;
					output.currentPriceOfAuction = state.NFTs.get(locals._t).currentPriceOfAuction;
					output.royalty = state.NFTs.get(locals._t).royalty;
					output.NFTidForExchange = state.NFTs.get(locals._t).NFTidForExchange;
					output.creator = state.NFTs.get(locals._t).creator;
					output.possesor = state.NFTs.get(locals._t).possesor;
					output.askUser = state.NFTs.get(locals._t).askUser;
					output.creatorOfAuction = state.NFTs.get(locals._t).creatorOfAuction;
					output.statusOfAuction = state.NFTs.get(locals._t).statusOfAuction;
					output.statusOfSale = state.NFTs.get(locals._t).statusOfSale;
					output.statusOfAsk = state.NFTs.get(locals._t).statusOfAsk;
					output.paymentMethodOfAsk = state.NFTs.get(locals._t).paymentMethodOfAsk;
					output.statusOfExchange = state.NFTs.get(locals._t).statusOfExchange;
					output.paymentMethodOfAuction = state.NFTs.get(locals._t).paymentMethodOfAuction;
					QUOTTERY::unpackQuotteryDate(output.yearAuctionStarted, output.monthAuctionStarted, output.dayAuctionStarted, output.hourAuctionStarted, output.minuteAuctionStarted, output.secondAuctionStarted, state.NFTs.get(locals._t).startTimeOfAuction);
					QUOTTERY::unpackQuotteryDate(output.yearAuctionEnded, output.monthAuctionEnded, output.dayAuctionEnded, output.hourAuctionEnded, output.minuteAuctionEnded, output.secondAuctionEnded, state.NFTs.get(locals._t).endTimeOfAuction);
					
					for(locals._r = 0 ; locals._r < 64; locals._r++)
					{
						output.URI.set(locals._r, state.NFTs.get(locals._t).URI.get(locals._r));
					}

					return ;
				}
			}
		}
	_

	PUBLIC_FUNCTION(getInfoOfMarketplace)

		output.numberOfCollection = state.numberOfCollection;
		output.numberOfNFT = state.numberOfNFT;
		output.numberOfNFTIncoming = state.numberOfNFTIncoming;
		output.priceOfCFB = state.priceOfCFB;
		output.priceOfQubic = state.priceOfQubic;

	_

	struct getInfoOfCollectionByCreator_locals
	{
		uint32 _t, _r, cnt;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfCollectionByCreator)

		locals.cnt = 0;

		for(locals._t = 0 ; locals._t < state.numberOfCollection; locals._t++)
		{
			if(state.Collections.get(locals._t).creator == input.creator)
			{
				locals.cnt++;
			}
			if(locals.cnt == input.orderOfCollection)
			{
				output.currentSize = state.Collections.get(locals._t).currentSize;
				output.idOfCollection = locals._t;
				output.maxSizeHoldingPerOneId = state.Collections.get(locals._t).maxSizeHoldingPerOneId;
				output.priceForDropMint = state.Collections.get(locals._t).priceForDropMint;
				output.royalty = state.Collections.get(locals._t).royalty;
				output.typeOfCollection = state.Collections.get(locals._t).typeOfCollection;
				
				for(locals._r = 0 ; locals._r < 64; locals._r++)
				{
					output.URI.set(locals._t, state.Collections.get(locals._t).URI.get(locals._r));
				}

				return ;
			}
		}

	_

	struct getInfoOfCollectionById_locals
	{
		uint32 _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfCollectionById)

		output.creator = state.Collections.get(input.idOfColletion).creator;
		output.currentSize = state.Collections.get(input.idOfColletion).currentSize;
		output.maxSizeHoldingPerOneId = state.Collections.get(input.idOfColletion).maxSizeHoldingPerOneId;
		output.priceForDropMint = state.Collections.get(input.idOfColletion).priceForDropMint;
		output.royalty = state.Collections.get(input.idOfColletion).royalty;
		output.typeOfCollection = state.Collections.get(input.idOfColletion).typeOfCollection;

		for(locals._t = 0 ; locals._t < 64; locals._t++)
		{
			output.URI.set(locals._t, state.Collections.get(input.idOfColletion).URI.get(locals._t));
		}

	_

	struct getIncomingAuctions_locals
	{
		uint32 curDate;
		uint32 _t, cnt, _r;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getIncomingAuctions)

		getCurrentDate(qpi, locals.curDate);
		locals.cnt = 0;
		locals._r = 0;

		for(locals._t = 0; locals._t < state.numberOfNFT; locals._t++)
		{
			if(locals.curDate < state.NFTs.get(locals._t).startTimeOfAuction)
			{
				locals.cnt++;

				if(locals.cnt >= input.offset && locals._r < input.count)
				{
					output.NFTId.set(locals._r++, locals._t);
				}

				if(locals._r == input.count)
				{
					return ;
				}
			}
		}

	_

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES

		REGISTER_USER_FUNCTION(getNumberOfNFTForUser, 1);
		REGISTER_USER_FUNCTION(getInfoOfNFTUserPossessed, 2);
		REGISTER_USER_FUNCTION(getInfoOfMarketplace, 3);
		REGISTER_USER_FUNCTION(getInfoOfCollectionByCreator, 4);
		REGISTER_USER_FUNCTION(getInfoOfCollectionById, 5);
		REGISTER_USER_FUNCTION(getIncomingAuctions, 6);

		REGISTER_USER_PROCEDURE(settingCFBAndQubicPrice, 1);
		REGISTER_USER_PROCEDURE(createCollection, 2);
		REGISTER_USER_PROCEDURE(mint, 3);
		REGISTER_USER_PROCEDURE(mintOfDrop, 4);
		REGISTER_USER_PROCEDURE(transfer, 5);
		REGISTER_USER_PROCEDURE(listInMarket, 6);
		REGISTER_USER_PROCEDURE(sell, 7);
		REGISTER_USER_PROCEDURE(cancelSale, 8);
		REGISTER_USER_PROCEDURE(listInExchange, 9);
		REGISTER_USER_PROCEDURE(cancelExchange, 10);
		REGISTER_USER_PROCEDURE(makeOffer, 11);
		REGISTER_USER_PROCEDURE(acceptOffer, 12);
		REGISTER_USER_PROCEDURE(cancelOffer, 13);
		REGISTER_USER_PROCEDURE(createTraditionalAuction, 14);
		REGISTER_USER_PROCEDURE(bidOnTraditionalAuction, 15);

    _


	INITIALIZE
	
		state.cfbIssuer = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
		state.marketplaceOwner = ID(_Y, _R, _P, _H, _H, _S, _U, _E, _E, _B, _S, _A, _X, _B, _Y, _F, _B, _A, _X, _P, _U, _R, _E, _X, _F, _E, _S, _A, _Q, _F, _N, _C, _J, _O, _M, _R, _I, _G, _B, _C, _W, _D, _I, _M, _K, _R, _R, _I, _Z, _T, _K, _P, _O, _J, _F, _H);

    _

	PRE_RELEASE_SHARES

        output.allowTransfer = true;

    _
};
