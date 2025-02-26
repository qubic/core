using namespace QPI;

constexpr uint64 QBAY_SALE_PRICE = 2000000000000000ULL;
constexpr uint32 QBAY_MAX_NUMBER_NFT = 2097152;
constexpr uint32 QBAY_MAX_COLLECTION = 32768;
constexpr uint32 QBAY_SINGLE_NFT_CREATE_FEE = 5000000;
constexpr uint32 QBAY_LENGTH_OF_URI = 59;
constexpr uint32 QBAY_CFB_NAME = 4343363;
constexpr uint32 QBAY_MIN_DELTA_SIZE = 1000000;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_2_200 = 100;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_201_1000 = 200;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_1001_2000 = 400;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_2001_3000 = 600;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_3001_4000 = 800;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_4001_5000 = 1000;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_5001_6000 = 1200;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_6001_7000 = 1400;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_7001_8000 = 1600;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_8001_9000 = 1800;
constexpr uint32 QBAY_FEE_COLLECTION_CREATE_9001_10000 = 2000;
constexpr uint32 QBAY_FEE_NFT_SALE_MARKET = 20;
constexpr uint32 QBAY_FEE_NFT_SALE_SHAREHOLDERS = 10;

constexpr sint32 QBAY_SUCCESS = 0;
constexpr sint32 QBAY_INSUFFICIENT_FUND = 1;
constexpr sint32 QBAY_INVALID_INPUT = 2;

//      For createCollection
constexpr sint32 QBAY_INVALID_VOLUME_SIZE = 3;
constexpr sint32 QBAY_INSUFFICIENT_CFB = 4;
constexpr sint32 QBAY_LIMIT_COLLECTION_VOLUME = 5;
constexpr sint32 QBAY_ERROR_TRANSFER_ASSET = 6;
constexpr sint32 QBAY_MAX_NUMBER_COLLECTION = 7;

//      For mint
constexpr sint32 QBAY_OVERFLOW_NFT = 8;
constexpr sint32 QBAY_LIMIT_HOLDING_NFT_PER_ONE_ADDRESS = 9;
constexpr sint32 QBAY_NOT_COLLECTION_CREATOR = 10;
constexpr sint32 QBAY_COLLECTION_FOR_DROP = 11;

//		For listInMarket & sale
constexpr sint32 QBAY_NOT_POSSESOR = 12;
constexpr sint32 QBAY_WRONG_NFTID = 13;
constexpr sint32 QBAY_WRONG_URI = 14;
constexpr sint32 QBAY_NOT_SALE_STATUS = 15;
constexpr sint32 QBAY_LOW_PRICE = 16;
constexpr sint32 QBAY_NOT_ASK_STATUS = 17;
constexpr sint32 QBAY_NOT_OWNER = 18;
constexpr sint32 QBAY_NOT_ASK_USER = 19;
constexpr sint32 QBAY_RESERVED_NFT = 27;

//		For DropMint

constexpr sint32 QBAY_NOT_COLLECTION_FOR_DROP = 20;
constexpr sint32 QBAY_OVERFLOW_MAX_SIZE_PER_ONEID = 21;

//		For Auction

constexpr sint32 QBAY_NOT_ENDED_AUCTION = 22;
constexpr sint32 QBAY_NOT_TRADITIONAL_AUCTION = 23;
constexpr sint32 QBAY_NOT_AUCTION_TIME = 24;
constexpr sint32 QBAY_SMALL_PRICE = 25;
constexpr sint32 QBAY_NOT_MATCH_PAYMENT_METHOD = 26;

constexpr sint32 QBAY_NOT_AVAILABLE_CREATE_AND_MINT = 28;
constexpr sint32 QBAY_EXCHANGE_STATUS = 29;
constexpr sint32 QBAY_SALE_STATUS = 30;


enum QBAYLogInfo {
    success = 0,
	insufficientQubic = 1,
	invalidInput = 2,
	//      For createCollection
	invalidVolumnSize = 3,
	insufficientCFB = 4,
	limitCollectionVolumn = 5,
	errorTransferAsset = 6,
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
	reservedNFT = 27,
	//		For DropMint
	notCollectionForDrop = 20,
	overflowMaxSizePerOneId = 21,
	//		For Auction
	notEndedAuction = 22,
	notTraditionalAuction = 23,
	notAuctionTime = 24,
	smallPrice = 25,
	notMatchPaymentMethod = 26,
	notAvailableCreateAndMint = 28,
	exchangeStatus = 29,
	saleStatus = 30,
};

struct QBAYLogger
{
    uint32 _contractIndex;
    uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

struct QBAY2
{
};

struct QBAY : public ContractBase
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
		uint32 volumn;						//	it means that how much NFTs can the collection be holded. 0 means that the collection holds up to 200 NFTs, 1 -> 1000 NFTs, 2 -> 3000 NFTs
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
		id receiver;
		uint32 NFTid;
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

	struct buy_input 
	{
		uint32 NFTid;
		bit methodOfPayment;
	};

	struct buy_output 
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
		uint32 anotherNFT;
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

	struct changeStatusOfMarketPlace_input
	{
		bit status;
	};

	struct changeStatusOfMarketPlace_output
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
		uint64 earnedQubic;
		uint64 earnedCFB;
		uint32 numberOfCollection;
		uint32 numberOfNFT;
		bit statusOfMarketPlace;
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

	struct getInfoOfNFTById_input
	{
		uint32 NFTId;
	};

	struct getInfoOfNFTById_output
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

	struct getUserCreatedCollection_input
	{
		id user;
		uint32 offset;
		uint32 count;
	};

	struct getUserCreatedCollection_output
	{
		Array<uint32, 1024> collectionId;
	};

	struct getUserCreatedNFT_input
	{
		id user;
		uint32 offset;
		uint32 count;
	};

	struct getUserCreatedNFT_output
	{
		Array<uint32, 1024> NFTId;
	};

protected:

    uint64 priceOfCFB;							// The amount of $CFB per 1 USD
	uint64 priceOfQubic;						// The amount of $Qubic per 1 USD
	uint64 numberOfNFTIncoming;					// The number of NFT after minting of all NFTs for collection
	uint64 earnedQubic;							// The amount of Qubic that the marketplace earned
	uint64 earnedCFB;							// The amount of CFB that the marketplace earned
	uint64 collectedShareHoldersFee;
	uint32 numberOfCollection;
	uint32 numberOfNFT;
	id cfbIssuer;
	id marketPlaceOwner;
	bit statusOfMarketPlace;					// The Marketplace owner can adjust the status of marketplace. if owner does not turn on the status with 1, it will not be available to invoke the "createCollection" and "mint" procedures

	struct InfoOfCollection 
	{
		id creator;								// address of collection creator
		uint64 priceForDropMint;				// the price for initial sale for NFT if the collection is for Drop.
		uint32 maxSizeHoldingPerOneId;          // max size that one ID can hold the NFTs of one collection.
		sint16 currentSize;						// collection volume
		Array<uint8, 64> URI;			        // URI for this Collection
		uint8 royalty;							// percent from 0 ~ 100
		uint8 typeOfCollection;					// 0 means that the collection is for Drop, 1 means that the collection is for normal collection
	};

	// The capacity of one collection is 112 byte

	Array<InfoOfCollection, QBAY_MAX_COLLECTION> Collections;

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
		uint8 statusOfAuction;						//		Status of Auction(0 means that there is no auction, 1 means that tranditional Auction is started, 2 means that one user buyed the NFT in traditinoal auction)
		bit statusOfSale;							//		Status of Sale, 0 means possesor don't want to sell
		bit statusOfAsk;							//		Status of Ask
		bit paymentMethodOfAsk;						//		0 means the asked user want to buy using $Qubic, 1 means that want to buy using $CFB
		bit statusOfExchange;						//		Status of Exchange
		bit paymentMethodOfAuction;					//		0 means the user can buy using only $Qubic, 1 means that can buy using only $CFB
	};

	// The capacity of one NFT is 238 byte;
		
	Array<InfoOfNFT, QBAY_MAX_NUMBER_NFT> NFTs;

	/**
	* @return Current date from core node system
	*/

	inline static void getCurrentDate(const QPI::QpiContextFunctionCall& qpi, uint32& res) 
	{
        QUOTTERY::packQuotteryDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), res);
    }

	struct settingCFBAndQubicPrice_locals
	{
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(settingCFBAndQubicPrice)

		if(qpi.invocator() != state.marketPlaceOwner)
		{
			output.returnCode = QBAY_NOT_OWNER;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notOwner, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.priceOfCFB = input.CFBPrice;
		state.priceOfQubic = input.QubicPrice;

		output.returnCode = QBAY_SUCCESS;
	_

	struct createCollection_locals 
	{
		InfoOfCollection newCollection;
		sint64 possessedAmount;
		uint64 tmp;
		uint32 _t;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createCollection)

		if(state.statusOfMarketPlace == 0 && qpi.invocator() != state.marketPlaceOwner)
		{
			output.returnCode = QBAY_NOT_AVAILABLE_CREATE_AND_MINT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notAvailableCreateAndMint, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(state.numberOfCollection >= QBAY_MAX_COLLECTION || state.numberOfNFTIncoming + (input.volumn == 0 ? 200: input.volumn * 1000) >= QBAY_MAX_NUMBER_NFT) 
		{
			output.returnCode = QBAY_OVERFLOW_NFT; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::overflowNFT, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(input.volumn > 10 || input.royalty > 100) 
		{
			output.returnCode = QBAY_INVALID_VOLUME_SIZE;  			// volume size should be 0 ~ 10
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidVolumnSize, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		locals.possessedAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);


		if(input.volumn == 0) 
		{
			if(input.maxSizePerOneId > 200)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_2_200) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_2_200 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_2_200 * state.priceOfCFB;

			}
			locals.newCollection.currentSize = 200;
			state.numberOfNFTIncoming += 200;
		}

		if(input.volumn == 1) 
		{
			if(input.maxSizePerOneId > 1000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_201_1000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_201_1000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_201_1000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 1000;
			state.numberOfNFTIncoming += 1000;
		}

		if(input.volumn == 2) 
		{
			if(input.maxSizePerOneId > 2000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_1001_2000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_1001_2000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_1001_2000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 2000;
			state.numberOfNFTIncoming += 2000;
		}

		if(input.volumn == 3) 
		{
			if(input.maxSizePerOneId > 3000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_2001_3000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_2001_3000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_2001_3000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 3000;
			state.numberOfNFTIncoming += 3000;
		}

		if(input.volumn == 4) 
		{
			if(input.maxSizePerOneId > 4000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_3001_4000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_3001_4000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_3001_4000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 4000;
			state.numberOfNFTIncoming += 4000;
		}

		if(input.volumn == 5) 
		{
			if(input.maxSizePerOneId > 5000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_4001_5000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_4001_5000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_4001_5000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 5000;
			state.numberOfNFTIncoming += 5000;
		}

		if(input.volumn == 6) 
		{
			if(input.maxSizePerOneId > 6000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_5001_6000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_5001_6000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_5001_6000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 6000;
			state.numberOfNFTIncoming += 6000;
		}

		if(input.volumn == 7) 
		{
			if(input.maxSizePerOneId > 7000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_6001_7000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_6001_7000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_6001_7000 * state.priceOfCFB;
			}

			locals.newCollection.currentSize = 7000;
			state.numberOfNFTIncoming += 7000;
		}

		if(input.volumn == 8) 
		{
			if(input.maxSizePerOneId > 8000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_7001_8000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_7001_8000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_7001_8000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 8000;
			state.numberOfNFTIncoming += 8000;
		}

		if(input.volumn == 9) 
		{
			if(input.maxSizePerOneId > 9000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_8001_9000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}
				
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_8001_9000 * state.priceOfCFB, SELF);
				
				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_8001_9000 * state.priceOfCFB;
			}
			
			locals.newCollection.currentSize = 9000;
			state.numberOfNFTIncoming += 9000;
		}

		if(input.volumn == 10) 
		{
			if(input.maxSizePerOneId > 10000)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocator() != state.marketPlaceOwner)
			{
				if(div(locals.possessedAmount * 1ULL, state.priceOfCFB) < QBAY_FEE_COLLECTION_CREATE_9001_10000) 
				{
					output.returnCode = QBAY_INSUFFICIENT_CFB;
					locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientCFB, 0 };
					LOG_INFO(locals.log);

					if(qpi.invocationReward() > 0) 
					{
						qpi.transfer(qpi.invocator(), qpi.invocationReward());
					}
					return ;
				}

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_FEE_COLLECTION_CREATE_9001_10000 * state.priceOfCFB, SELF);

				state.earnedCFB += QBAY_FEE_COLLECTION_CREATE_9001_10000 * state.priceOfCFB;
			}
			
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
			if(locals._t >= QBAY_LENGTH_OF_URI) 
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
		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct mint_locals 
	{
		uint32 _t;
		uint32 cntOfNFTPerOne;
		InfoOfNFT newNFT;
		InfoOfCollection updatedCollection;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mint)

		if(state.statusOfMarketPlace == 0)
		{
			output.returnCode = QBAY_NOT_AVAILABLE_CREATE_AND_MINT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notAvailableCreateAndMint, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(input.typeOfMint == 1)     //     It means NFT creator mints the single NFT.
		{
			if(input.royalty >= 100)
			{
				output.returnCode = QBAY_INVALID_INPUT;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);
				
				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(state.numberOfNFTIncoming + 1 >= QBAY_MAX_NUMBER_NFT) 
			{
				output.returnCode = QBAY_OVERFLOW_NFT; 
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::overflowNFT, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(qpi.invocationReward() < QBAY_SINGLE_NFT_CREATE_FEE)  //     The fee for single NFT should be more than 5M QU
			{
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);
				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(qpi.invocationReward() > QBAY_SINGLE_NFT_CREATE_FEE)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - QBAY_SINGLE_NFT_CREATE_FEE);
			}

			state.earnedQubic += QBAY_SINGLE_NFT_CREATE_FEE;
			locals.newNFT.royalty = input.royalty;
			state.numberOfNFTIncoming++;
		}

		else 
		{
			if(state.Collections.get(input.collectionId).creator != qpi.invocator())
			{
				output.returnCode = QBAY_NOT_COLLECTION_CREATOR;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notCollectionCreator, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.Collections.get(input.collectionId).typeOfCollection == 0)
			{
				output.returnCode = QBAY_COLLECTION_FOR_DROP;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::collectionForDrop, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.Collections.get(input.collectionId).currentSize <= 0) 
			{
				output.returnCode = QBAY_LIMIT_COLLECTION_VOLUME;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::limitCollectionVolumn, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.newNFT.royalty = state.Collections.get(input.collectionId).royalty;   // The royalty should be set as the royalty to be set in collection

			locals.updatedCollection = state.Collections.get(input.collectionId);
			locals.updatedCollection.currentSize--;

			state.Collections.set(input.collectionId, locals.updatedCollection);
		}

		for(locals._t = 0 ; locals._t < 64; locals._t++) 
		{
			if(locals._t >= QBAY_LENGTH_OF_URI) 
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
		locals.newNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
		locals.newNFT.salePrice = QBAY_SALE_PRICE;
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
		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct mintOfDrop_locals 
	{
		InfoOfCollection updatedCollection;
		InfoOfNFT newNFT;
		uint64 cntOfNFTHoldingPerOneId;
		uint32 _t;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mintOfDrop)

		if(state.Collections.get(input.collectionId).typeOfCollection == 1)
		{
			output.returnCode = QBAY_NOT_COLLECTION_FOR_DROP;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notCollectionForDrop, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.Collections.get(input.collectionId).currentSize <= 0) 
		{
			output.returnCode = QBAY_LIMIT_COLLECTION_VOLUME;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::limitCollectionVolumn, 0 };
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

		if(locals.cntOfNFTHoldingPerOneId >= state.Collections.get(input.collectionId).maxSizeHoldingPerOneId)
		{
			output.returnCode = QBAY_OVERFLOW_MAX_SIZE_PER_ONEID;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::overflowMaxSizePerOneId, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if((uint64)qpi.invocationReward() < state.Collections.get(input.collectionId).priceForDropMint)
		{
			output.returnCode = QBAY_INSUFFICIENT_FUND;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
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

        qpi.transfer(state.Collections.get(input.collectionId).creator, state.Collections.get(input.collectionId).priceForDropMint);

		locals.updatedCollection = state.Collections.get(input.collectionId);
		locals.updatedCollection.currentSize--;

		state.Collections.set(input.collectionId, locals.updatedCollection);

		locals.newNFT.creator = state.Collections.get(input.collectionId).creator;
		locals.newNFT.possesor = qpi.invocator();
		locals.newNFT.royalty = state.Collections.get(input.collectionId).royalty;
		locals.newNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
		locals.newNFT.salePrice = QBAY_SALE_PRICE;
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
			if(locals._t >= QBAY_LENGTH_OF_URI) 
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
		output.returnCode = QBAY_SUCCESS;

		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct transfer_locals 
	{
		InfoOfNFT transferNFT;
		uint32 curDate;
		uint32 _t;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(transfer)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		for(locals._t = 0 ; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).NFTidForExchange == input.NFTid)
			{
				output.returnCode = QBAY_EXCHANGE_STATUS;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		if(state.NFTs.get(input.NFTid).statusOfSale == 1)
		{
			output.returnCode = QBAY_SALE_STATUS;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::saleStatus, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.transferNFT.possesor = input.receiver;
		locals.transferNFT.salePrice = QBAY_SALE_PRICE;
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
		

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.transferNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.transferNFT);
		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct listInMarket_locals 
	{
		InfoOfNFT saleNFT;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInMarket)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		for(locals._t = 0 ; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).NFTidForExchange == input.NFTid)
			{
				output.returnCode = QBAY_EXCHANGE_STATUS;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())   //		Checking the possesor
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
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
		

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.saleNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.saleNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct buy_locals 
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
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(buy)

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
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
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfSale == 0) 
		{
			output.returnCode = QBAY_NOT_SALE_STATUS;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notSaleStatus };
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
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
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
			locals.marketFee = div(state.NFTs.get(input.NFTid).salePrice * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
			locals.shareHolderFee = div(state.NFTs.get(input.NFTid).salePrice * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

			state.collectedShareHoldersFee += locals.shareHolderFee;
			qpi.transfer(state.NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.NFTs.get(input.NFTid).possesor, state.NFTs.get(input.NFTid).salePrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			state.earnedQubic += locals.marketFee;
		}

		else 
		{
			locals.possessedCFBAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
			if(div(state.NFTs.get(input.NFTid).salePrice * 1ULL, state.priceOfQubic) > div(locals.possessedCFBAmount * 1ULL, state.priceOfCFB)) 
			{
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
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
			locals.creatorFee = div(locals.transferredAmountOfCFB * state.NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
			locals.marketFee = div(locals.transferredAmountOfCFB * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);

            qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.transferredAmountOfCFB - locals.creatorFee - locals.marketFee, state.NFTs.get(input.NFTid).possesor);
			state.earnedCFB += locals.marketFee;
		}

		locals.updatedNFT.possesor = qpi.invocator();
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
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
		
		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);
		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct cancelSale_locals
	{
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelSale)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									// NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())   			// Checking the possesor
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
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

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct listInExchange_locals 
	{
		InfoOfNFT updatedNFT;
		id tmpPossesor;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInExchange)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= QBAY_MAX_NUMBER_NFT || input.anotherNFT >= QBAY_MAX_NUMBER_NFT )		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(input.possessedNFT == input.anotherNFT)
		{
			output.returnCode = QBAY_INVALID_VOLUME_SIZE; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).statusOfSale == 1)
		{
			output.returnCode = QBAY_SALE_STATUS; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::saleStatus, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.possessedNFT).endTimeOfAuction || locals.curDate <= state.NFTs.get(input.anotherNFT).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).possesor != qpi.invocator())
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).NFTidForExchange == input.anotherNFT)
		{
			locals.tmpPossesor = state.NFTs.get(input.possessedNFT).possesor;

			locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
			locals.updatedNFT.statusOfExchange = 0;
			locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
			locals.updatedNFT.statusOfSale = 0;
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
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.possessedNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.possessedNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.possessedNFT).statusOfAsk;
			

			for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.possessedNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.possessedNFT, locals.updatedNFT);

			locals.updatedNFT.creator = state.NFTs.get(input.anotherNFT).creator;
			locals.updatedNFT.possesor = locals.tmpPossesor;
			locals.updatedNFT.askUser = state.NFTs.get(input.anotherNFT).askUser;
			locals.updatedNFT.salePrice = state.NFTs.get(input.anotherNFT).salePrice;
			locals.updatedNFT.statusOfSale = state.NFTs.get(input.anotherNFT).statusOfSale;
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.anotherNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.anotherNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.anotherNFT).statusOfAsk;
			locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.anotherNFT).paymentMethodOfAsk;
			

			for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.anotherNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.anotherNFT, locals.updatedNFT);
		}

		else 
		{
			if(state.NFTs.get(input.anotherNFT).NFTidForExchange != QBAY_MAX_NUMBER_NFT)
			{
				output.returnCode = QBAY_RESERVED_NFT;

				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::reservedNFT, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			locals.updatedNFT.NFTidForExchange = input.possessedNFT;
			locals.updatedNFT.statusOfExchange = 1;

			locals.updatedNFT.creator = state.NFTs.get(input.anotherNFT).creator;
			locals.updatedNFT.possesor = state.NFTs.get(input.anotherNFT).possesor;
			locals.updatedNFT.askUser = state.NFTs.get(input.anotherNFT).askUser;
			locals.updatedNFT.salePrice = state.NFTs.get(input.anotherNFT).salePrice;
			locals.updatedNFT.statusOfSale = state.NFTs.get(input.anotherNFT).statusOfSale;
			locals.updatedNFT.askMaxPrice = state.NFTs.get(input.anotherNFT).askMaxPrice;
			locals.updatedNFT.royalty = state.NFTs.get(input.anotherNFT).royalty;
			locals.updatedNFT.statusOfAsk = state.NFTs.get(input.anotherNFT).statusOfAsk;
			locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.anotherNFT).paymentMethodOfAsk;
			locals.updatedNFT.currentPriceOfAuction = 0;
			locals.updatedNFT.startTimeOfAuction = 0;
			locals.updatedNFT.endTimeOfAuction = 0;
			locals.updatedNFT.statusOfAuction = 0;
			locals.updatedNFT.creatorOfAuction = NULL_ID;
			

			for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
			{
				locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.anotherNFT).URI.get(locals._t));
			}

			state.NFTs.set(input.anotherNFT, locals.updatedNFT);
		}
		
		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct cancelExchange_locals
	{
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelExchange)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= QBAY_MAX_NUMBER_NFT || input.anotherNFT >= QBAY_MAX_NUMBER_NFT)		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = QBAY_WRONG_NFTID; 
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.possessedNFT).endTimeOfAuction || locals.curDate <= state.NFTs.get(input.anotherNFT).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.possessedNFT).possesor != qpi.invocator() || state.NFTs.get(input.anotherNFT).NFTidForExchange != input.possessedNFT)
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
		locals.updatedNFT.statusOfExchange = 0;

		locals.updatedNFT.creator = state.NFTs.get(input.anotherNFT).creator;
		locals.updatedNFT.possesor = state.NFTs.get(input.anotherNFT).possesor;
		locals.updatedNFT.askUser = state.NFTs.get(input.anotherNFT).askUser;
		locals.updatedNFT.salePrice = state.NFTs.get(input.anotherNFT).salePrice;
		locals.updatedNFT.statusOfSale = state.NFTs.get(input.anotherNFT).statusOfSale;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.anotherNFT).askMaxPrice;
		locals.updatedNFT.royalty = state.NFTs.get(input.anotherNFT).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.anotherNFT).statusOfAsk;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.anotherNFT).paymentMethodOfAsk;
		locals.updatedNFT.currentPriceOfAuction = 0;
		locals.updatedNFT.startTimeOfAuction = 0;
		locals.updatedNFT.endTimeOfAuction = 0;
		locals.updatedNFT.statusOfAuction = 0;
		locals.updatedNFT.paymentMethodOfAuction = 0;
		locals.updatedNFT.creatorOfAuction = NULL_ID;
		

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.anotherNFT).URI.get(locals._t));
		}

		state.NFTs.set(input.anotherNFT, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct makeOffer_locals 
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT AskedNFT;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(makeOffer)

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT || input.askPrice == 0)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
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
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 1)
		{
			if((input.askPrice <= state.NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE && input.paymentMethod == state.NFTs.get(input.NFTid).paymentMethodOfAsk)
			|| (input.paymentMethod == 1 && state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0 && div(input.askPrice * 1ULL, state.priceOfCFB) <= div((state.NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.priceOfQubic))
			|| (input.paymentMethod == 0 && state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1 && div(input.askPrice * 1ULL, state.priceOfQubic) <= div((state.NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.priceOfCFB)))
			{
				output.returnCode = QBAY_LOW_PRICE;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::lowPrice, 0 };
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
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
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
			if(qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) < input.askPrice)
			{
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				return;
			}
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.askPrice, SELF);
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 1)
		{

			if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
			{
				qpi.transfer(state.NFTs.get(input.NFTid).askUser, state.NFTs.get(input.NFTid).askMaxPrice);
			}
			else
			{
				locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
				locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
				locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
				locals.transferShareManagementRights_input.numberOfShares = state.NFTs.get(input.NFTid).askMaxPrice;

				INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, SELF, SELF, state.NFTs.get(input.NFTid).askMaxPrice, state.NFTs.get(input.NFTid).askUser);
			}

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
		
		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.AskedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.AskedNFT);

		output.returnCode = QBAY_SUCCESS;

		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct acceptOffer_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
        sint64 tmp;
		sint64 creatorFee;
		sint64 marketFee;
		sint64 shareHolderFee;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(acceptOffer)

		if(qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).possesor != qpi.invocator())
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 0)
		{
			output.returnCode = QBAY_NOT_ASK_STATUS;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		locals.creatorFee = div(state.NFTs.get(input.NFTid).askMaxPrice * state.NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
		locals.marketFee = div(state.NFTs.get(input.NFTid).askMaxPrice * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
		locals.shareHolderFee = div(state.NFTs.get(input.NFTid).askMaxPrice * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

		if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			state.collectedShareHoldersFee += locals.shareHolderFee;
			qpi.transfer(state.NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.NFTs.get(input.NFTid).possesor, state.NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			
			state.earnedQubic += locals.marketFee;
		}

		else if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			
			locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.NFTs.get(input.NFTid).askMaxPrice - locals.marketFee;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, SELF, SELF, locals.creatorFee, state.NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, SELF, SELF, state.NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee, qpi.invocator());
			state.earnedCFB += locals.marketFee;
		}

		locals.updatedNFT.possesor = state.NFTs.get(input.NFTid).askUser;
		locals.updatedNFT.statusOfAsk = 0;
		locals.updatedNFT.askUser = NULL_ID;
		locals.updatedNFT.askMaxPrice = 0;
		locals.updatedNFT.paymentMethodOfAsk = 0;
		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
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

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);

	_

	struct cancelOffer_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelOffer)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.curDate <= state.NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).statusOfAsk == 0)
		{
			output.returnCode = QBAY_NOT_ASK_STATUS;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).askUser != qpi.invocator() && state.NFTs.get(input.NFTid).possesor != qpi.invocator())
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			qpi.transfer(state.NFTs.get(input.NFTid).askUser, state.NFTs.get(input.NFTid).askMaxPrice);
		}
		else if(state.NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.NFTs.get(input.NFTid).askMaxPrice;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, SELF, SELF, state.NFTs.get(input.NFTid).askMaxPrice, state.NFTs.get(input.NFTid).askUser);
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

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTid).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct createTraditionalAuction_locals
	{
		InfoOfNFT updatedNFT;
		uint32 curDate;
		uint32 startDate;
		uint32 endDate;
		uint32 _t, _r;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createTraditionalAuction)

		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		for(locals._t = 0 ; locals._t < state.numberOfNFT; locals._t++)
		{
			if(state.NFTs.get(locals._t).NFTidForExchange == input.NFTId)
			{
				output.returnCode = QBAY_EXCHANGE_STATUS;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		if(state.NFTs.get(input.NFTId).statusOfSale == 1)
		{
			output.returnCode = QBAY_SALE_STATUS;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::saleStatus, 0};
			LOG_INFO(locals.log);

			return;
		}

		QUOTTERY::packQuotteryDate(input.startYear, input.startMonth, input.startDay, input.startHour, 0, 0, locals.startDate);
		QUOTTERY::packQuotteryDate(input.endYear, input.endMonth, input.endDay, input.endHour, 0, 0, locals.endDate);

		if(input.NFTId >= QBAY_MAX_NUMBER_NFT || QUOTTERY::checkValidQtryDateTime(locals.startDate) == 0 || QUOTTERY::checkValidQtryDateTime(locals.endDate) == 0)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		getCurrentDate(qpi, locals.curDate);

		if(locals.startDate <= locals.curDate || locals.endDate <= locals.startDate)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		if(locals.curDate <= state.NFTs.get(input.NFTId).endTimeOfAuction)
		{
			output.returnCode = QBAY_NOT_ENDED_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.NFTs.get(input.NFTId).possesor != qpi.invocator())
		{
			output.returnCode = QBAY_NOT_POSSESOR;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notPossesor, 0 };
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
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTId).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTId).statusOfExchange;

		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;

		locals.updatedNFT.currentPriceOfAuction = input.minPrice;
		locals.updatedNFT.startTimeOfAuction = locals.startDate;
		locals.updatedNFT.endTimeOfAuction = locals.endDate;
		locals.updatedNFT.statusOfAuction = 1;
		locals.updatedNFT.paymentMethodOfAuction = input.paymentMethodOfAuction;
		locals.updatedNFT.creatorOfAuction = qpi.invocator();

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTId).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_


	struct bidOnTraditionalAuction_locals
	{
		InfoOfNFT updatedNFT;
		uint64 marketFee;
		uint64 creatorFee;
		uint64 shareHolderFee;
		uint64 possessedAmount;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(bidOnTraditionalAuction)

		if(input.NFTId >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = QBAY_INVALID_INPUT;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		if(state.NFTs.get(input.NFTId).statusOfAuction == 0)
		{
			output.returnCode = QBAY_NOT_TRADITIONAL_AUCTION;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notTraditionalAuction, 0 };
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
			output.returnCode = QBAY_NOT_AUCTION_TIME;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notAuctionTime, 0 };
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
				output.returnCode = QBAY_NOT_MATCH_PAYMENT_METHOD;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(input.price <= state.NFTs.get(input.NFTId).currentPriceOfAuction + QBAY_MIN_DELTA_SIZE)
			{
				output.returnCode = QBAY_SMALL_PRICE;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if((uint64)qpi.invocationReward() < input.price)
			{
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
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
				locals.marketFee = div(input.price * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.shareHolderFee = div(input.price * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

				qpi.transfer(state.NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.NFTs.get(input.NFTId).possesor, input.price - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
				state.earnedQubic += locals.marketFee;
				state.collectedShareHoldersFee += locals.shareHolderFee;
			}

			if(state.NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.creatorFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.marketFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.shareHolderFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);
				
				qpi.transfer(state.NFTs.get(input.NFTId).possesor, state.NFTs.get(input.NFTId).currentPriceOfAuction);
				qpi.transfer(state.NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.NFTs.get(input.NFTId).creatorOfAuction, input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
				state.earnedQubic += locals.marketFee;
				state.collectedShareHoldersFee += locals.shareHolderFee;
			}
			
			locals.updatedNFT.currentPriceOfAuction = input.price;
		}

		if(state.NFTs.get(input.NFTId).paymentMethodOfAuction == 1)
		{
			if(input.paymentMethod == 0)
			{
				output.returnCode = QBAY_NOT_MATCH_PAYMENT_METHOD;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(input.price <= state.NFTs.get(input.NFTId).currentPriceOfAuction + QBAY_MIN_DELTA_SIZE)
			{
				output.returnCode = QBAY_SMALL_PRICE;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.possessedAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);

			if(locals.possessedAmount < input.price)
			{
				output.returnCode = QBAY_INSUFFICIENT_FUND;
				locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.NFTs.get(input.NFTId).statusOfAuction == 1)
			{
				locals.marketFee = div(input.price * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - locals.creatorFee - locals.marketFee, state.NFTs.get(input.NFTId).possesor);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);

				state.earnedCFB += locals.marketFee;
			}
			if(state.NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.marketFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div((input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction) * state.NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), state.NFTs.get(input.NFTId).currentPriceOfAuction, state.NFTs.get(input.NFTId).possesor);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - state.NFTs.get(input.NFTId).currentPriceOfAuction - locals.marketFee - locals.creatorFee, state.NFTs.get(input.NFTId).creatorOfAuction);
			
				state.earnedCFB += locals.marketFee;
			}
			
			locals.updatedNFT.currentPriceOfAuction = input.price;
		}

		locals.updatedNFT.possesor = qpi.invocator();

		locals.updatedNFT.creator = state.NFTs.get(input.NFTId).creator;
		locals.updatedNFT.royalty = state.NFTs.get(input.NFTId).royalty;
		locals.updatedNFT.statusOfAsk = state.NFTs.get(input.NFTId).statusOfAsk;
		locals.updatedNFT.askUser = state.NFTs.get(input.NFTId).askUser;
		locals.updatedNFT.askMaxPrice = state.NFTs.get(input.NFTId).askMaxPrice;
		locals.updatedNFT.paymentMethodOfAsk = state.NFTs.get(input.NFTId).paymentMethodOfAsk;
		locals.updatedNFT.NFTidForExchange = state.NFTs.get(input.NFTId).NFTidForExchange;
		locals.updatedNFT.statusOfExchange = state.NFTs.get(input.NFTId).statusOfExchange;

		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;

		locals.updatedNFT.startTimeOfAuction = state.NFTs.get(input.NFTId).startTimeOfAuction;
		locals.updatedNFT.endTimeOfAuction = state.NFTs.get(input.NFTId).endTimeOfAuction;
		locals.updatedNFT.statusOfAuction = 2;
		locals.updatedNFT.paymentMethodOfAuction = state.NFTs.get(input.NFTId).paymentMethodOfAuction;
		locals.updatedNFT.creatorOfAuction = state.NFTs.get(input.NFTId).creatorOfAuction;

		for(locals._t = 0; locals._t < QBAY_LENGTH_OF_URI; locals._t++) 
		{
			locals.updatedNFT.URI.set(locals._t, state.NFTs.get(input.NFTId).URI.get(locals._t));
		}

		state.NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
		LOG_INFO(locals.log);
	_

	struct changeStatusOfMarketPlace_locals
	{
		QBAYLogger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(changeStatusOfMarketPlace)

		if(qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(qpi.invocator() != state.marketPlaceOwner)
		{
			output.returnCode = QBAY_NOT_OWNER;
			locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::notOwner, 0 };
			LOG_INFO(locals.log);
		}

		state.statusOfMarketPlace = input.status;

		output.returnCode = QBAY_SUCCESS;
		locals.log = QBAYLogger{ QBAY_CONTRACT_INDEX, QBAYLogInfo::success, 0 };
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
		output.earnedQubic = state.earnedQubic;
		output.earnedCFB = state.earnedCFB;
		output.statusOfMarketPlace = state.statusOfMarketPlace;

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
					output.URI.set(locals._r, state.Collections.get(locals._t).URI.get(locals._r));
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

		if(input.idOfColletion >= state.numberOfCollection)
		{
			return ;
		}

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

		if(input.offset + input.count >= state.numberOfNFT || input.count > 1024)
		{
			return ;
		}

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

	struct getInfoOfNFTById_locals
	{
		uint32 _r;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfNFTById)

		output.salePrice = state.NFTs.get(input.NFTId).salePrice;
		output.askMaxPrice = state.NFTs.get(input.NFTId).askMaxPrice;
		output.currentPriceOfAuction = state.NFTs.get(input.NFTId).currentPriceOfAuction;
		output.royalty = state.NFTs.get(input.NFTId).royalty;
		output.NFTidForExchange = state.NFTs.get(input.NFTId).NFTidForExchange;
		output.creator = state.NFTs.get(input.NFTId).creator;
		output.possesor = state.NFTs.get(input.NFTId).possesor;
		output.askUser = state.NFTs.get(input.NFTId).askUser;
		output.creatorOfAuction = state.NFTs.get(input.NFTId).creatorOfAuction;
		output.statusOfAuction = state.NFTs.get(input.NFTId).statusOfAuction;
		output.statusOfSale = state.NFTs.get(input.NFTId).statusOfSale;
		output.statusOfAsk = state.NFTs.get(input.NFTId).statusOfAsk;
		output.paymentMethodOfAsk = state.NFTs.get(input.NFTId).paymentMethodOfAsk;
		output.statusOfExchange = state.NFTs.get(input.NFTId).statusOfExchange;
		output.paymentMethodOfAuction = state.NFTs.get(input.NFTId).paymentMethodOfAuction;
		QUOTTERY::unpackQuotteryDate(output.yearAuctionStarted, output.monthAuctionStarted, output.dayAuctionStarted, output.hourAuctionStarted, output.minuteAuctionStarted, output.secondAuctionStarted, state.NFTs.get(input.NFTId).startTimeOfAuction);
		QUOTTERY::unpackQuotteryDate(output.yearAuctionEnded, output.monthAuctionEnded, output.dayAuctionEnded, output.hourAuctionEnded, output.minuteAuctionEnded, output.secondAuctionEnded, state.NFTs.get(input.NFTId).endTimeOfAuction);
		
		for(locals._r = 0 ; locals._r < 64; locals._r++)
		{
			output.URI.set(locals._r, state.NFTs.get(input.NFTId).URI.get(locals._r));
		}

	_

	struct getUserCreatedCollection_locals
	{
		uint32 _r, cnt, _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getUserCreatedCollection)

		if(input.offset + input.count > state.numberOfCollection || input.count > 1024)
		{
			return ;
		}

		locals.cnt = 0;
		locals._r = 0;

		for(locals._t = state.numberOfCollection - 1 ; locals._t >= 0; locals._t--)
		{
			if(state.Collections.get(locals._t).creator == input.user)
			{
				locals.cnt++;
				if(locals.cnt >= input.offset && locals._r < input.count)
				{
					output.collectionId.set(locals._r++, locals._t);
				}
				if(locals._r == input.count)
				{
					return ;
				}
			}
		}
	_

	struct getUserCreatedNFT_locals
	{
		uint32 _r, cnt, _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getUserCreatedNFT)

	if(input.offset + input.count > state.numberOfNFT || input.count > 1024)
	{
		return ;
	}

	locals.cnt = 0;
	locals._r = 0;

	for(locals._t = state.numberOfCollection - 1 ; locals._t >= 0; locals._t--)
	{
		if(state.NFTs.get(locals._t).creator == input.user)
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
		REGISTER_USER_FUNCTION(getInfoOfNFTById, 7);
		REGISTER_USER_FUNCTION(getUserCreatedCollection, 8);
		REGISTER_USER_FUNCTION(getUserCreatedNFT , 9);

		REGISTER_USER_PROCEDURE(settingCFBAndQubicPrice, 1);
		REGISTER_USER_PROCEDURE(createCollection, 2);
		REGISTER_USER_PROCEDURE(mint, 3);
		REGISTER_USER_PROCEDURE(mintOfDrop, 4);
		REGISTER_USER_PROCEDURE(transfer, 5);
		REGISTER_USER_PROCEDURE(listInMarket, 6);
		REGISTER_USER_PROCEDURE(buy, 7);
		REGISTER_USER_PROCEDURE(cancelSale, 8);
		REGISTER_USER_PROCEDURE(listInExchange, 9);
		REGISTER_USER_PROCEDURE(cancelExchange, 10);
		REGISTER_USER_PROCEDURE(makeOffer, 11);
		REGISTER_USER_PROCEDURE(acceptOffer, 12);
		REGISTER_USER_PROCEDURE(cancelOffer, 13);
		REGISTER_USER_PROCEDURE(createTraditionalAuction, 14);
		REGISTER_USER_PROCEDURE(bidOnTraditionalAuction, 15);
		REGISTER_USER_PROCEDURE(changeStatusOfMarketPlace, 17);

    _

	INITIALIZE
	
		state.cfbIssuer = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
		state.marketPlaceOwner = ID(_R, _K, _D, _H, _C, _M, _R, _J, _Y, _C, _G, _K, _P, _D, _U, _Y, _R, _X, _G, _D, _Y, _Z, _C, _I, _Z, _I, _T, _A, _H, _Y, _O, _V, _G, _I, _U, _T, _K, _N, _D, _T, _E, _H, _P, _C, _C, _L, _W, _L, _Z, _X, _S, _H, _N, _F, _P, _D);
		
    _

	struct END_EPOCH_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
	};

	END_EPOCH_WITH_LOCALS

		qpi.distributeDividends(div(state.collectedShareHoldersFee * 1ULL, 676ULL));
		qpi.burn(state.collectedShareHoldersFee - div(state.collectedShareHoldersFee, 676ULL) * 676ULL);
		state.collectedShareHoldersFee = 0;


		qpi.transfer(state.marketPlaceOwner, state.earnedQubic);
		locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
		locals.transferShareManagementRights_input.asset.issuer = state.cfbIssuer;
		locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
		locals.transferShareManagementRights_input.numberOfShares = state.earnedCFB;

		INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

		qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.cfbIssuer, SELF, SELF, state.earnedCFB, state.marketPlaceOwner);

		state.earnedCFB = 0;
		state.earnedQubic = 0;

	_

    PRE_ACQUIRE_SHARES
		output.allowTransfer = true;
	_
};
