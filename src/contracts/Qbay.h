using namespace QPI;

constexpr uint64 QBAY_SALE_PRICE = 2000000000000000ULL;
constexpr uint32 QBAY_MAX_NUMBER_NFT = 2097152;
constexpr uint32 QBAY_MAX_COLLECTION = 32768;
constexpr uint32 QBAY_SINGLE_NFT_CREATE_FEE = 5000000;
constexpr uint32 QBAY_LENGTH_OF_URI = 59;
constexpr uint32 QBAY_CFB_NAME = 4343363;
constexpr uint32 QBAY_MIN_DELTA_SIZE = 1000000;
constexpr uint32 QBAY_FEE_NFT_SALE_MARKET = 20;
constexpr uint32 QBAY_FEE_NFT_SALE_SHAREHOLDERS = 10;

struct QBAY2
{
};

struct QBAY : public ContractBase
{
	enum LogInfo {
		success = 0,
		insufficientQubic = 1,
		invalidInput = 2,
		//      For createCollection
		invalidVolumeSize = 3,
		insufficientCFB = 4,
		limitCollectionVolume = 5,
		errorTransferAsset = 6,
		maxNumberOfCollection = 7,
		//      For mint
		overflowNFT = 8,
		limitHoldingNFTPerOneId = 9,
		notCollectionCreator = 10,
		collectionForDrop = 11,
		//		For listInMarket & sale
		notPossessor = 12,
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
		creatorOfAuction = 31,
		possessor = 32,
	};

	struct Logger
	{
		uint32 _contractIndex;
		uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
		sint8 _terminator; // Only data before "_terminator" are logged
	};

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

	// The size of one collection is 112 byte

	struct InfoOfNFT
	{
		id creator;                             	//      Identity of NFT creator
		id possessor;								//		Identity of NFT possessor
		id askUser;									//		Identity of Asked user
		id creatorOfAuction;						//		Creator of Auction
		uint64 salePrice;							//		This price should be set by possessor
		uint64 askMaxPrice;							//	 	This price is the max of asked prices
		uint64 currentPriceOfAuction;				//		This price is the start price of auctions
		uint32 startTimeOfAuction;					//		The start time of auction
		uint32 endTimeOfAuction;					//		The end time of auction
		uint32 royalty;								//		Percent from 0 ~ 100
		uint32 NFTidForExchange;					//      NFT Id that want to exchange
		Array<uint8, 64> URI;			            //      URI for this NFT
		uint8 statusOfAuction;						//		Status of Auction(0 means that there is no auction, 1 means that tranditional Auction is started, 2 means that one user buyed the NFT in traditinoal auction)
		bit statusOfSale;							//		Status of Sale, 0 means possessor don't want to sell
		bit statusOfAsk;							//		Status of Ask
		bit paymentMethodOfAsk;						//		0 means the asked user want to buy using $Qubic, 1 means that want to buy using $CFB
		bit statusOfExchange;						//		Status of Exchange
		bit paymentMethodOfAuction;					//		0 means the user can buy using only $Qubic, 1 means that can buy using only $CFB
	};

	// The size of one NFT is 238 byte;

	struct StateData
	{
		uint64 priceOfCFB;							// The amount of $CFB per 1 USD
		uint64 priceOfQubic;						// The amount of $Qubic per 1 USD
		uint64 numberOfNFTIncoming;					// The number of NFT after minting of all NFTs for collection
		uint64 earnedQubic;							// The amount of Qubic that the marketplace earned
		uint64 earnedCFB;							// The amount of CFB that the marketplace earned
		uint64 collectedShareHoldersFee;
		sint64 transferRightsFee;
		uint32 numberOfCollection;
		uint32 numberOfNFT;
		id cfbIssuer;
		id marketPlaceOwner;
		bit statusOfMarketPlace;					// The Marketplace owner can adjust the status of marketplace. if owner does not turn on the status with 1, it will not be available to invoke the "createCollection" and "mint" procedures

		Array<InfoOfCollection, QBAY_MAX_COLLECTION> Collections;
		Array<InfoOfNFT, QBAY_MAX_NUMBER_NFT> NFTs;
	};

	/****** PORTED TIMEUTILS FROM OLD QUOTTERY *****/
	/**
	 * Compare 2 date in uint32 format
	 * @return -1 lesser(ealier) A<B, 0 equal A=B, 1 greater(later) A>B
	 */
	inline static sint32 dateCompare(uint32& A, uint32& B, sint32& i)
	{
		if (A == B) return 0;
		if (A < B) return -1;
		return 1;
	}

	/**
	 * @return pack qbay datetime data from year, month, day, hour, minute, second to a uint32
	 * year is counted from 24 (2024)
	 */
	inline static void packQbayDate(uint32 _year, uint32 _month, uint32 _day, uint32 _hour, uint32 _minute, uint32 _second, uint32& res)
	{
		res = ((_year - 24) << 26) | (_month << 22) | (_day << 17) | (_hour << 12) | (_minute << 6) | (_second);
	}

	inline static uint32 qbayGetYear(uint32 data)
	{
		return ((data >> 26) + 24);
	}
	inline static uint32 qbayGetMonth(uint32 data)
	{
		return ((data >> 22) & 0b1111);
	}
	inline static uint32 qbayGetDay(uint32 data)
	{
		return ((data >> 17) & 0b11111);
	}
	inline static uint32 qbayGetHour(uint32 data)
	{
		return ((data >> 12) & 0b11111);
	}
	inline static uint32 qbayGetMinute(uint32 data)
	{
		return ((data >> 6) & 0b111111);
	}
	inline static uint32 qbayGetSecond(uint32 data)
	{
		return (data & 0b111111);
	}
	/*
	* @return unpack qbay datetime from uin32 to year, month, day, hour, minute, secon
	*/
	inline static void unpackQbayDate(uint8& _year, uint8& _month, uint8& _day, uint8& _hour, uint8& _minute, uint8& _second, uint32 data)
	{
		_year = qbayGetYear(data); // 6 bits
		_month = qbayGetMonth(data); //4bits
		_day = qbayGetDay(data); //5bits
		_hour = qbayGetHour(data); //5bits
		_minute = qbayGetMinute(data); //6bits
		_second = qbayGetSecond(data); //6bits
	}

	inline static void accumulatedDay(sint32 month, uint64& res)
	{
		switch (month)
		{
		case 1: res = 0; break;
		case 2: res = 31; break;
		case 3: res = 59; break;
		case 4: res = 90; break;
		case 5: res = 120; break;
		case 6: res = 151; break;
		case 7: res = 181; break;
		case 8: res = 212; break;
		case 9: res = 243; break;
		case 10:res = 273; break;
		case 11:res = 304; break;
		case 12:res = 334; break;
		}
	}
	/**
	 * @return difference in number of second, A must be smaller than or equal B to have valid value
	 */
	inline static void diffDateInSecond(uint32& A, uint32& B, sint32& i, uint64& dayA, uint64& dayB, uint64& res)
	{
		if (dateCompare(A, B, i) >= 0)
		{
			res = 0;
			return;
		}
		accumulatedDay(qbayGetMonth(A), dayA);
		dayA += qbayGetDay(A);
		accumulatedDay(qbayGetMonth(B), dayB);
		dayB += (qbayGetYear(B) - qbayGetYear(A)) * 365ULL + qbayGetDay(B);

		// handling leap-year: only store last 2 digits of year here, don't care about mod 100 & mod 400 case
		for (i = qbayGetYear(A); (uint32)(i) < qbayGetYear(B); i++)
		{
			if (mod(i, 4) == 0)
			{
				dayB++;
			}
		}
		if (mod(sint32(qbayGetYear(A)), 4) == 0 && (qbayGetMonth(A) > 2)) dayA++;
		if (mod(sint32(qbayGetYear(B)), 4) == 0 && (qbayGetMonth(B) > 2)) dayB++;
		res = (dayB - dayA) * 3600ULL * 24;
		res += (qbayGetHour(B) * 3600 + qbayGetMinute(B) * 60 + qbayGetSecond(B));
		res -= (qbayGetHour(A) * 3600 + qbayGetMinute(A) * 60 + qbayGetSecond(A));
	}

	inline static bool checkValidQbayDateTime(uint32& A)
	{
		if (qbayGetMonth(A) > 12) return false;
		if (qbayGetDay(A) > 31) return false;
		if ((qbayGetDay(A) == 31) &&
			(qbayGetMonth(A) != 1) && (qbayGetMonth(A) != 3) && (qbayGetMonth(A) != 5) &&
			(qbayGetMonth(A) != 7) && (qbayGetMonth(A) != 8) && (qbayGetMonth(A) != 10) && (qbayGetMonth(A) != 12)) return false;
		if ((qbayGetDay(A) == 30) && (qbayGetMonth(A) == 2)) return false;
		if ((qbayGetDay(A) == 29) && (qbayGetMonth(A) == 2) && (mod(qbayGetYear(A), 4u) != 0)) return false;
		if (qbayGetHour(A) >= 24) return false;
		if (qbayGetMinute(A) >= 60) return false;
		if (qbayGetSecond(A) >= 60) return false;
		return true;
	}
	/****** END PORTED TIMEUTILS FROM OLD QUOTTERY *****/

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
		uint32 volume;						//	it means that how much NFTs can the collection be holded. 0 means that the collection holds up to 200 NFTs, 1 -> 1000 NFTs, 2 -> 3000 NFTs
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
		uint64 askPrice;
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

	struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
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
		id possessor;								//		Identity of NFT possessor
		id askUser;									//		Identity of Asked user
		id creatorOfAuction;						//		Creator of Auction
		sint64 salePrice;							//		This price should be set by possessor
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
		bit statusOfSale;							//		Status of Sale, 0 means possessor don't want to sell
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
		uint32 idOfCollection;
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
		id possessor;								//		Identity of NFT possessor
		id askUser;									//		Identity of Asked user
		id creatorOfAuction;						//		Creator of Auction
		sint64 salePrice;							//		This price should be set by possessor
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
		bit statusOfSale;							//		Status of Sale, 0 means possessor don't want to sell
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

	struct settingCFBAndQubicPrice_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(settingCFBAndQubicPrice)
	{
		if(qpi.invocator() != state.get().marketPlaceOwner)
		{
			output.returnCode = LogInfo::notOwner;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notOwner, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.mut().priceOfCFB = input.CFBPrice;
		state.mut().priceOfQubic = input.QubicPrice;

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct createCollection_locals 
	{
		InfoOfCollection newCollection;
		sint64 possessedAmount;
		uint64 tmp;
		uint32 _t;
		uint32 numberOfNFT;
		uint16 fee;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createCollection)
	{
		if(state.get().statusOfMarketPlace == 0 && qpi.invocator() != state.get().marketPlaceOwner)
		{
			output.returnCode = LogInfo::notAvailableCreateAndMint;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notAvailableCreateAndMint, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(input.volume > 10 || input.royalty > 100) 
		{
			output.returnCode = LogInfo::invalidInput;  			// volume size should be 0 ~ 10
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(input.volume == 0)
		{
			locals.numberOfNFT = 200;
		}
		else 
		{
			locals.numberOfNFT = input.volume * 1000;
		}

		if(state.get().numberOfCollection >= QBAY_MAX_COLLECTION || state.get().numberOfNFTIncoming + locals.numberOfNFT >= QBAY_MAX_NUMBER_NFT) 
		{
			output.returnCode = LogInfo::overflowNFT; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::overflowNFT, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		locals.possessedAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
		if(input.volume == 0)
		{
			locals.fee = 100;
		}
		else 
		{
			locals.fee = input.volume * 200;
		}

		if(input.maxSizePerOneId > locals.numberOfNFT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(qpi.invocator() != state.get().marketPlaceOwner)
		{
			if(div(locals.possessedAmount * 1ULL, state.get().priceOfCFB) < locals.fee) 
			{
				output.returnCode = LogInfo::insufficientCFB;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.fee * state.get().priceOfCFB, SELF);
			state.mut().earnedCFB += locals.fee * state.get().priceOfCFB;
		}
		locals.newCollection.currentSize = locals.numberOfNFT;
		state.mut().numberOfNFTIncoming += locals.numberOfNFT;

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

		state.mut().Collections.set(state.get().numberOfCollection, locals.newCollection);
		state.mut().numberOfCollection++;
		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct mint_locals 
	{
		uint32 _t;
		uint32 cntOfNFTPerOne;
		InfoOfNFT newNFT;
		InfoOfCollection updatedCollection;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mint)
	{
		if(state.get().statusOfMarketPlace == 0)
		{
			output.returnCode = LogInfo::notAvailableCreateAndMint;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notAvailableCreateAndMint, 0 };
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
				output.returnCode = LogInfo::invalidInput;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);
				
				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(state.get().numberOfNFTIncoming + 1 >= QBAY_MAX_NUMBER_NFT) 
			{
				output.returnCode = LogInfo::overflowNFT; 
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::overflowNFT, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(qpi.invocationReward() < QBAY_SINGLE_NFT_CREATE_FEE)  //     The fee for single NFT should be more than 5M QU
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
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

			state.mut().earnedQubic += QBAY_SINGLE_NFT_CREATE_FEE;
			locals.newNFT.royalty = input.royalty;
			state.mut().numberOfNFTIncoming++;
		}
		else 
		{
			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			if(state.get().Collections.get(input.collectionId).creator != qpi.invocator())
			{
				output.returnCode = LogInfo::notCollectionCreator;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notCollectionCreator, 0 };
				LOG_INFO(locals.log);

				return ;
			}
			if(state.get().Collections.get(input.collectionId).typeOfCollection == 0)
			{
				output.returnCode = LogInfo::collectionForDrop;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::collectionForDrop, 0 };
				LOG_INFO(locals.log);

				return ;
			}
			if(state.get().Collections.get(input.collectionId).currentSize <= 0) 
			{
				output.returnCode = LogInfo::limitCollectionVolume;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::limitCollectionVolume, 0 };
				LOG_INFO(locals.log);

				return ;
			}
			locals.newNFT.royalty = state.get().Collections.get(input.collectionId).royalty;   // The royalty should be set as the royalty to be set in collection

			locals.updatedCollection = state.get().Collections.get(input.collectionId);
			locals.updatedCollection.currentSize--;

			state.mut().Collections.set(input.collectionId, locals.updatedCollection);
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
		locals.newNFT.possessor = qpi.invocator();	
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
		
		state.mut().NFTs.set(state.get().numberOfNFT, locals.newNFT);
		state.mut().numberOfNFT++;
		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct mintOfDrop_locals 
	{
		InfoOfCollection updatedCollection;
		InfoOfNFT newNFT;
		id creator;
		uint64 cntOfNFTHoldingPerOneId;
		uint32 _t;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mintOfDrop)
	{
		if(state.get().Collections.get(input.collectionId).typeOfCollection == 1)
		{
			output.returnCode = LogInfo::notCollectionForDrop;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notCollectionForDrop, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.get().Collections.get(input.collectionId).currentSize <= 0) 
		{
			output.returnCode = LogInfo::limitCollectionVolume;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::limitCollectionVolume, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		locals.cntOfNFTHoldingPerOneId = 0;
		locals.creator = state.get().Collections.get(input.collectionId).creator;

		for(locals._t = 0; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).creator == locals.creator && state.get().NFTs.get(locals._t).possessor == qpi.invocator())
			{
				locals.cntOfNFTHoldingPerOneId++;
			}
		}

		if(locals.cntOfNFTHoldingPerOneId >= state.get().Collections.get(input.collectionId).maxSizeHoldingPerOneId)
		{
			output.returnCode = LogInfo::overflowMaxSizePerOneId;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::overflowMaxSizePerOneId, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if((uint64)qpi.invocationReward() < state.get().Collections.get(input.collectionId).priceForDropMint)
		{
			output.returnCode = LogInfo::insufficientQubic;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if((uint64)qpi.invocationReward() > state.get().Collections.get(input.collectionId).priceForDropMint)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().Collections.get(input.collectionId).priceForDropMint);
		}

        qpi.transfer(state.get().Collections.get(input.collectionId).creator, state.get().Collections.get(input.collectionId).priceForDropMint);

		locals.updatedCollection = state.get().Collections.get(input.collectionId);
		locals.updatedCollection.currentSize--;

		state.mut().Collections.set(input.collectionId, locals.updatedCollection);

		locals.newNFT.creator = state.get().Collections.get(input.collectionId).creator;
		locals.newNFT.possessor = qpi.invocator();
		locals.newNFT.royalty = state.get().Collections.get(input.collectionId).royalty;
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

		state.mut().NFTs.set(state.get().numberOfNFT, locals.newNFT);
		state.mut().numberOfNFT++;
		output.returnCode = LogInfo::success;

		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct transfer_locals 
	{
		InfoOfNFT transferNFT;
		uint32 curDate;
		uint32 _t;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(transfer)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(input.receiver == qpi.invocator())
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}


		if(state.get().NFTs.get(input.NFTid).statusOfSale == 1)
		{
			output.returnCode = LogInfo::saleStatus;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::saleStatus, 0 };
			LOG_INFO(locals.log);

			return;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor != qpi.invocator())
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		for(locals._t = 0 ; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).NFTidForExchange == input.NFTid)
			{
				output.returnCode = LogInfo::exchangeStatus;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		locals.transferNFT = state.get().NFTs.get(input.NFTid);
		locals.transferNFT.possessor = input.receiver;
		locals.transferNFT.salePrice = QBAY_SALE_PRICE;
		locals.transferNFT.statusOfSale = 0;

		state.mut().NFTs.set(input.NFTid, locals.transferNFT);
		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct listInMarket_locals 
	{
		InfoOfNFT saleNFT;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInMarket)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor != qpi.invocator())   //		Checking the possessor
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		for(locals._t = 0 ; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).NFTidForExchange == input.NFTid)
			{
				output.returnCode = LogInfo::exchangeStatus;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		locals.saleNFT = state.get().NFTs.get(input.NFTid);
		locals.saleNFT.salePrice = input.price;
		locals.saleNFT.statusOfSale = 1;

		state.mut().NFTs.set(input.NFTid, locals.saleNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

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
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(buy)
	{
		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									//			NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).statusOfSale == 0) 
		{
			output.returnCode = LogInfo::notSaleStatus;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notSaleStatus, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor == qpi.invocator())
		{
			output.returnCode = LogInfo::possessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::possessor, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(input.methodOfPayment == 0)
		{
			if((sint64)state.get().NFTs.get(input.NFTid).salePrice > qpi.invocationReward()) 
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return ;
			}

			if((sint64)state.get().NFTs.get(input.NFTid).salePrice < qpi.invocationReward())
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - state.get().NFTs.get(input.NFTid).salePrice);
			}

			locals.creatorFee = div(state.get().NFTs.get(input.NFTid).salePrice * state.get().NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
			locals.marketFee = div(state.get().NFTs.get(input.NFTid).salePrice * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
			locals.shareHolderFee = div(state.get().NFTs.get(input.NFTid).salePrice * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

			state.mut().collectedShareHoldersFee += locals.shareHolderFee;
			qpi.transfer(state.get().NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.get().NFTs.get(input.NFTid).possessor, state.get().NFTs.get(input.NFTid).salePrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			state.mut().earnedQubic += locals.marketFee;
		}

		else 
		{
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			locals.possessedCFBAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
			if(div(state.get().NFTs.get(input.NFTid).salePrice * 1ULL, state.get().priceOfQubic) > div(locals.possessedCFBAmount * 1ULL, state.get().priceOfCFB)) 
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				return ;
			}

			locals.transferredAmountOfCFB = div(state.get().NFTs.get(input.NFTid).salePrice * 1ULL, state.get().priceOfQubic) * state.get().priceOfCFB;
			locals.creatorFee = div(locals.transferredAmountOfCFB * state.get().NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
			locals.marketFee = div(locals.transferredAmountOfCFB * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);

            qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.get().NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.transferredAmountOfCFB - locals.creatorFee - locals.marketFee, state.get().NFTs.get(input.NFTid).possessor);
			state.mut().earnedCFB += locals.marketFee;
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTid);

		locals.updatedNFT.possessor = qpi.invocator();
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
		locals.updatedNFT.statusOfSale = 0;

		state.mut().NFTs.set(input.NFTid, locals.updatedNFT);
		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct cancelSale_locals
	{
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelSale)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)									// NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor != qpi.invocator())   			// Checking the possessor
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTid);
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
		locals.updatedNFT.statusOfSale = 0;

		state.mut().NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct listInExchange_locals 
	{
		InfoOfNFT updatedNFT;
		id tmpPossessor;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(listInExchange)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= QBAY_MAX_NUMBER_NFT || input.anotherNFT >= QBAY_MAX_NUMBER_NFT )		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(input.possessedNFT == input.anotherNFT || state.get().NFTs.get(input.possessedNFT).possessor == state.get().NFTs.get(input.anotherNFT).possessor)
		{
			output.returnCode = LogInfo::invalidInput; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.possessedNFT).statusOfSale == 1)
		{
			output.returnCode = LogInfo::saleStatus; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::saleStatus, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.possessedNFT).endTimeOfAuction || locals.curDate <= state.get().NFTs.get(input.anotherNFT).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.possessedNFT).possessor != qpi.invocator())
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.possessedNFT).NFTidForExchange == input.anotherNFT)
		{
			locals.tmpPossessor = state.get().NFTs.get(input.possessedNFT).possessor;

			locals.updatedNFT = state.get().NFTs.get(input.possessedNFT);
			locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
			locals.updatedNFT.statusOfExchange = 0;
			locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
			locals.updatedNFT.statusOfSale = 0;
			locals.updatedNFT.possessor = state.get().NFTs.get(input.anotherNFT).possessor;

			state.mut().NFTs.set(input.possessedNFT, locals.updatedNFT);

			locals.updatedNFT = state.get().NFTs.get(input.anotherNFT);
			locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
			locals.updatedNFT.statusOfExchange = 0;
			locals.updatedNFT.salePrice = QBAY_SALE_PRICE;
			locals.updatedNFT.statusOfSale = 0;
			locals.updatedNFT.possessor = locals.tmpPossessor;

			state.mut().NFTs.set(input.anotherNFT, locals.updatedNFT);
		}

		else 
		{
			if(state.get().NFTs.get(input.anotherNFT).NFTidForExchange != QBAY_MAX_NUMBER_NFT)
			{
				output.returnCode = LogInfo::reservedNFT;

				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::reservedNFT, 0 };
				LOG_INFO(locals.log);
				return ;
			}
			locals.updatedNFT = state.get().NFTs.get(input.anotherNFT);
			locals.updatedNFT.NFTidForExchange = input.possessedNFT;
			locals.updatedNFT.statusOfExchange = 1;

			state.mut().NFTs.set(input.anotherNFT, locals.updatedNFT);
		}
		
		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct cancelExchange_locals
	{
		InfoOfNFT updatedNFT;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelExchange)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.possessedNFT >= QBAY_MAX_NUMBER_NFT || input.anotherNFT >= QBAY_MAX_NUMBER_NFT)		//	NFTid should be less than MAX_NUMBER_NFT
		{
			output.returnCode = LogInfo::wrongNFTId; 
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.possessedNFT).endTimeOfAuction || locals.curDate <= state.get().NFTs.get(input.anotherNFT).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.possessedNFT).possessor == qpi.invocator())
		{
			locals.updatedNFT = state.get().NFTs.get(input.possessedNFT);
			locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
			locals.updatedNFT.statusOfExchange = 0;

			state.mut().NFTs.set(input.possessedNFT, locals.updatedNFT);
		}

		if(state.get().NFTs.get(input.possessedNFT).possessor != qpi.invocator() || state.get().NFTs.get(input.anotherNFT).NFTidForExchange != input.possessedNFT)
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		locals.updatedNFT = state.get().NFTs.get(input.anotherNFT);
		locals.updatedNFT.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
		locals.updatedNFT.statusOfExchange = 0;

		state.mut().NFTs.set(input.anotherNFT, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct makeOffer_locals 
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT AskedNFT;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(makeOffer)
	{
		if(input.NFTid >= QBAY_MAX_NUMBER_NFT || input.askPrice == 0)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor == qpi.invocator())
		{
			output.returnCode = LogInfo::possessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::possessor, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0) 
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).statusOfAsk == 1)
		{
			if((input.askPrice < state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE && input.paymentMethod == state.get().NFTs.get(input.NFTid).paymentMethodOfAsk)
			|| (input.paymentMethod == 1 && state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0 && div(input.askPrice * 1ULL, state.get().priceOfCFB) < div((state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.get().priceOfQubic))
			|| (input.paymentMethod == 0 && state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 1 && div(input.askPrice * 1ULL, state.get().priceOfQubic) < div((state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.get().priceOfCFB)))
			{
				output.returnCode = LogInfo::lowPrice;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::lowPrice, 0 };
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
			if(qpi.invocationReward() < (sint64)input.askPrice)
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0) 
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				return;
			}

			if(qpi.invocationReward() > (sint64)input.askPrice)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.askPrice);
			}
		}
		
		else if(input.paymentMethod == 1)
		{
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			if(qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX) < (sint64)input.askPrice)
			{
				output.returnCode = LogInfo::insufficientCFB;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				return;
			}
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), input.askPrice, SELF);
		}

		if(state.get().NFTs.get(input.NFTid).statusOfAsk == 1)
		{

			if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
			{
				qpi.transfer(state.get().NFTs.get(input.NFTid).askUser, state.get().NFTs.get(input.NFTid).askMaxPrice);
			}
			else
			{
				locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
				locals.transferShareManagementRights_input.asset.issuer = state.get().cfbIssuer;
				locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
				locals.transferShareManagementRights_input.numberOfShares = state.get().NFTs.get(input.NFTid).askMaxPrice;

				INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, state.get().NFTs.get(input.NFTid).askMaxPrice, state.get().NFTs.get(input.NFTid).askUser);
			}

		}

		locals.AskedNFT = state.get().NFTs.get(input.NFTid);
		locals.AskedNFT.statusOfAsk = 1;
		locals.AskedNFT.askUser = qpi.invocator();
		locals.AskedNFT.askMaxPrice = input.askPrice;
		locals.AskedNFT.paymentMethodOfAsk = input.paymentMethod;

		state.mut().NFTs.set(input.NFTid, locals.AskedNFT);

		output.returnCode = LogInfo::success;

		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

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
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(acceptOffer)
	{
		if(qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).possessor != qpi.invocator())
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if(state.get().NFTs.get(input.NFTid).statusOfAsk == 0)
		{
			output.returnCode = LogInfo::notAskStatus;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		locals.creatorFee = div(state.get().NFTs.get(input.NFTid).askMaxPrice * state.get().NFTs.get(input.NFTid).royalty * 1ULL, 100ULL);
		locals.marketFee = div(state.get().NFTs.get(input.NFTid).askMaxPrice * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
		locals.shareHolderFee = div(state.get().NFTs.get(input.NFTid).askMaxPrice * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

		if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			state.mut().collectedShareHoldersFee += locals.shareHolderFee;
			qpi.transfer(state.get().NFTs.get(input.NFTid).creator, locals.creatorFee);
			qpi.transfer(state.get().NFTs.get(input.NFTid).possessor, state.get().NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			
			state.mut().earnedQubic += locals.marketFee;
		}

		else if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			
			locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.get().cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.get().NFTs.get(input.NFTid).askMaxPrice - locals.marketFee;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, locals.creatorFee, state.get().NFTs.get(input.NFTid).creator);
			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, state.get().NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee, qpi.invocator());
			state.mut().earnedCFB += locals.marketFee;
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTid);
		locals.updatedNFT.possessor = state.get().NFTs.get(input.NFTid).askUser;
		locals.updatedNFT.statusOfAsk = 0;
		locals.updatedNFT.askUser = NULL_ID;
		locals.updatedNFT.askMaxPrice = 0;
		locals.updatedNFT.paymentMethodOfAsk = 0;
		locals.updatedNFT.statusOfSale = 0;
		locals.updatedNFT.salePrice = QBAY_SALE_PRICE;

		state.mut().NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);

	}

	struct cancelOffer_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
		InfoOfNFT updatedNFT;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(cancelOffer)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate <= state.get().NFTs.get(input.NFTid).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).statusOfAsk == 0)
		{
			output.returnCode = LogInfo::notAskStatus;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notAskStatus, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).askUser != qpi.invocator() && state.get().NFTs.get(input.NFTid).possessor != qpi.invocator())
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			qpi.transfer(state.get().NFTs.get(input.NFTid).askUser, state.get().NFTs.get(input.NFTid).askMaxPrice);
		}
		else if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 1)
		{
			locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
			locals.transferShareManagementRights_input.asset.issuer = state.get().cfbIssuer;
			locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
			locals.transferShareManagementRights_input.numberOfShares = state.get().NFTs.get(input.NFTid).askMaxPrice;

			INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, state.get().NFTs.get(input.NFTid).askMaxPrice, state.get().NFTs.get(input.NFTid).askUser);
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTid);
		locals.updatedNFT.statusOfAsk = 0;
		locals.updatedNFT.askUser = NULL_ID;
		locals.updatedNFT.askMaxPrice = 0;
		locals.updatedNFT.paymentMethodOfAsk = 0;

		state.mut().NFTs.set(input.NFTid, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct createTraditionalAuction_locals
	{
		InfoOfNFT updatedNFT;
		uint32 curDate;
		uint32 startDate;
		uint32 endDate;
		uint32 _t, _r;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(createTraditionalAuction)
	{
		if(qpi.invocationReward() > 0) 
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(state.get().NFTs.get(input.NFTId).statusOfSale == 1)
		{
			output.returnCode = LogInfo::saleStatus;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::saleStatus, 0};
			LOG_INFO(locals.log);

			return;
		}

		packQbayDate(input.startYear, input.startMonth, input.startDay, input.startHour, 0, 0, locals.startDate);
		packQbayDate(input.endYear, input.endMonth, input.endDay, input.endHour, 0, 0, locals.endDate);

		if(input.NFTId >= QBAY_MAX_NUMBER_NFT || checkValidQbayDateTime(locals.startDate) == 0 || checkValidQbayDateTime(locals.endDate) == 0)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.startDate <= locals.curDate || locals.endDate <= locals.startDate)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			return;
		}

		if(locals.curDate <= state.get().NFTs.get(input.NFTId).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notEndedAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notEndedAuction, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if(state.get().NFTs.get(input.NFTId).possessor != qpi.invocator())
		{
			output.returnCode = LogInfo::notPossessor;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notPossessor, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		for(locals._t = 0 ; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).NFTidForExchange == input.NFTId)
			{
				output.returnCode = LogInfo::exchangeStatus;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::exchangeStatus, 0 };
				LOG_INFO(locals.log);

				return;
			}
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTId);
		locals.updatedNFT.currentPriceOfAuction = input.minPrice;
		locals.updatedNFT.startTimeOfAuction = locals.startDate;
		locals.updatedNFT.endTimeOfAuction = locals.endDate;
		locals.updatedNFT.statusOfAuction = 1;
		locals.updatedNFT.paymentMethodOfAuction = input.paymentMethodOfAuction;
		locals.updatedNFT.creatorOfAuction = qpi.invocator();

		state.mut().NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}


	struct bidOnTraditionalAuction_locals
	{
		InfoOfNFT updatedNFT;
		uint64 marketFee;
		uint64 creatorFee;
		uint64 shareHolderFee;
		uint64 possessedAmount;
		uint64 updatedBidPrice;
        sint64 tmp;
		uint32 _t;
		uint32 curDate;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(bidOnTraditionalAuction)
	{
		if(input.NFTId >= QBAY_MAX_NUMBER_NFT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return;
		}

		if(state.get().NFTs.get(input.NFTId).statusOfAuction == 0)
		{
			output.returnCode = LogInfo::notTraditionalAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notTraditionalAuction, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		if(locals.curDate < state.get().NFTs.get(input.NFTId).startTimeOfAuction || locals.curDate > state.get().NFTs.get(input.NFTId).endTimeOfAuction)
		{
			output.returnCode = LogInfo::notAuctionTime;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notAuctionTime, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(state.get().NFTs.get(input.NFTId).creatorOfAuction == qpi.invocator())
		{
			output.returnCode = LogInfo::creatorOfAuction;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::creatorOfAuction, 0 };
			LOG_INFO(locals.log);
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		if(state.get().NFTs.get(input.NFTId).paymentMethodOfAuction == 0)
		{
			if(input.paymentMethod == 1)
			{
				output.returnCode = LogInfo::notMatchPaymentMethod;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(input.price < state.get().NFTs.get(input.NFTId).currentPriceOfAuction + QBAY_MIN_DELTA_SIZE)
			{
				output.returnCode = LogInfo::smallPrice;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if((uint64)qpi.invocationReward() < input.price)
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
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

			if(state.get().NFTs.get(input.NFTId).statusOfAuction == 1)
			{
				locals.marketFee = div(input.price * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.get().NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.shareHolderFee = div(input.price * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);

				qpi.transfer(state.get().NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.get().NFTs.get(input.NFTId).possessor, input.price - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
				state.mut().earnedQubic += locals.marketFee;
				state.mut().collectedShareHoldersFee += locals.shareHolderFee;
			}

			if(state.get().NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.creatorFee = div((input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction) * state.get().NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);
				locals.marketFee = div((input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.shareHolderFee = div((input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_SHAREHOLDERS * 1ULL, 1000ULL);
				
				qpi.transfer(state.get().NFTs.get(input.NFTId).possessor, state.get().NFTs.get(input.NFTId).currentPriceOfAuction);
				qpi.transfer(state.get().NFTs.get(input.NFTId).creator, locals.creatorFee);
				qpi.transfer(state.get().NFTs.get(input.NFTId).creatorOfAuction, input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
				state.mut().earnedQubic += locals.marketFee;
				state.mut().collectedShareHoldersFee += locals.shareHolderFee;
			}
			
			locals.updatedBidPrice = input.price;
		}

		if(state.get().NFTs.get(input.NFTId).paymentMethodOfAuction == 1)
		{
			if(input.paymentMethod == 0)
			{
				output.returnCode = LogInfo::notMatchPaymentMethod;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notMatchPaymentMethod, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}
			if(input.price < state.get().NFTs.get(input.NFTId).currentPriceOfAuction + QBAY_MIN_DELTA_SIZE)
			{
				output.returnCode = LogInfo::smallPrice;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::smallPrice, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			locals.possessedAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);

			if(locals.possessedAmount < input.price)
			{
				output.returnCode = LogInfo::insufficientQubic;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
				LOG_INFO(locals.log);

				if(qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				return ;
			}

			if(state.get().NFTs.get(input.NFTId).statusOfAuction == 1)
			{
				locals.marketFee = div(input.price * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div(input.price * state.get().NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.get().NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - locals.creatorFee - locals.marketFee, state.get().NFTs.get(input.NFTId).possessor);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);

				state.mut().earnedCFB += locals.marketFee;
			}
			if(state.get().NFTs.get(input.NFTId).statusOfAuction == 2)
			{
				locals.marketFee = div((input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction) * QBAY_FEE_NFT_SALE_MARKET * 1ULL, 1000ULL);
				locals.creatorFee = div((input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction) * state.get().NFTs.get(input.NFTId).royalty * 1ULL, 100ULL);

				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.marketFee, SELF);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.creatorFee, state.get().NFTs.get(input.NFTId).creator);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), state.get().NFTs.get(input.NFTId).currentPriceOfAuction, state.get().NFTs.get(input.NFTId).possessor);
				qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), input.price - state.get().NFTs.get(input.NFTId).currentPriceOfAuction - locals.marketFee - locals.creatorFee, state.get().NFTs.get(input.NFTId).creatorOfAuction);
			
				state.mut().earnedCFB += locals.marketFee;
			}
			
			locals.updatedBidPrice = input.price;
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTId);
		locals.updatedNFT.possessor = qpi.invocator();
		locals.updatedNFT.statusOfAuction = 2;
		locals.updatedNFT.currentPriceOfAuction = locals.updatedBidPrice;

		state.mut().NFTs.set(input.NFTId, locals.updatedNFT);

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct changeStatusOfMarketPlace_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(changeStatusOfMarketPlace)
	{
		if(qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if(qpi.invocator() != state.get().marketPlaceOwner)
		{
			output.returnCode = LogInfo::notOwner;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notOwner, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		state.mut().statusOfMarketPlace = input.status;

		output.returnCode = LogInfo::success;
		locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
		LOG_INFO(locals.log);
	}

	struct TransferShareManagementRights_locals
	{
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (qpi.invocationReward() < state.get().transferRightsFee)
		{
			output.returnCode = LogInfo::insufficientQubic;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientQubic, 0 };
			LOG_INFO(locals.log);

			return ;
		}

		if (qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer,qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
		{
			// not enough shares available
			output.transferredNumberOfShares = 0;
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = LogInfo::insufficientCFB;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientCFB, 0 };
			LOG_INFO(locals.log);
		}
		else
		{
			if (qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares,
				input.newManagingContractIndex, input.newManagingContractIndex, state.get().transferRightsFee) < 0)
			{
				// error
				output.transferredNumberOfShares = 0;
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}

				output.returnCode = LogInfo::errorTransferAsset;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::errorTransferAsset, 0 };
				LOG_INFO(locals.log);
			}
			else
			{
				// success
				output.transferredNumberOfShares = input.numberOfShares;
				if (qpi.invocationReward() > state.get().transferRightsFee)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward() -  state.get().transferRightsFee);
				}

				output.returnCode = LogInfo::success;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::success, 0 };
				LOG_INFO(locals.log);
			}
		}
	}

	struct getNumberOfNFTForUser_locals
	{
		uint32 curDate;
		uint32 _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getNumberOfNFTForUser)
	{
		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		output.numberOfNFT = 0;

		for(locals._t = 0 ; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).possessor == input.user && locals.curDate > state.get().NFTs.get(locals._t).endTimeOfAuction)
			{
				output.numberOfNFT++;
			}
		}
	}

	struct getInfoOfNFTUserPossessed_locals
	{
		uint32 curDate;
		uint32 _t, _r;
		uint32 cnt;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfNFTUserPossessed)
	{
		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);

		locals.cnt = 0;

		for(locals._t = 0 ; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).possessor == input.user && locals.curDate > state.get().NFTs.get(locals._t).endTimeOfAuction)
			{
				locals.cnt++;
				if(input.NFTNumber == locals.cnt)
				{
					output.salePrice = state.get().NFTs.get(locals._t).salePrice;
					output.askMaxPrice = state.get().NFTs.get(locals._t).askMaxPrice;
					output.currentPriceOfAuction = state.get().NFTs.get(locals._t).currentPriceOfAuction;
					output.royalty = state.get().NFTs.get(locals._t).royalty;
					output.NFTidForExchange = state.get().NFTs.get(locals._t).NFTidForExchange;
					output.creator = state.get().NFTs.get(locals._t).creator;
					output.possessor = state.get().NFTs.get(locals._t).possessor;
					output.askUser = state.get().NFTs.get(locals._t).askUser;
					output.creatorOfAuction = state.get().NFTs.get(locals._t).creatorOfAuction;
					output.statusOfAuction = state.get().NFTs.get(locals._t).statusOfAuction;
					output.statusOfSale = state.get().NFTs.get(locals._t).statusOfSale;
					output.statusOfAsk = state.get().NFTs.get(locals._t).statusOfAsk;
					output.paymentMethodOfAsk = state.get().NFTs.get(locals._t).paymentMethodOfAsk;
					output.statusOfExchange = state.get().NFTs.get(locals._t).statusOfExchange;
					output.paymentMethodOfAuction = state.get().NFTs.get(locals._t).paymentMethodOfAuction;
					unpackQbayDate(output.yearAuctionStarted, output.monthAuctionStarted, output.dayAuctionStarted, output.hourAuctionStarted, output.minuteAuctionStarted, output.secondAuctionStarted, state.get().NFTs.get(locals._t).startTimeOfAuction);
					unpackQbayDate(output.yearAuctionEnded, output.monthAuctionEnded, output.dayAuctionEnded, output.hourAuctionEnded, output.minuteAuctionEnded, output.secondAuctionEnded, state.get().NFTs.get(locals._t).endTimeOfAuction);

					for(locals._r = 0 ; locals._r < 64; locals._r++)
					{
						output.URI.set(locals._r, state.get().NFTs.get(locals._t).URI.get(locals._r));
					}

					return ;
				}
			}
		}
	}

	PUBLIC_FUNCTION(getInfoOfMarketplace)
	{
		output.numberOfCollection = state.get().numberOfCollection;
		output.numberOfNFT = state.get().numberOfNFT;
		output.numberOfNFTIncoming = state.get().numberOfNFTIncoming;
		output.priceOfCFB = state.get().priceOfCFB;
		output.priceOfQubic = state.get().priceOfQubic;
		output.earnedQubic = state.get().earnedQubic;
		output.earnedCFB = state.get().earnedCFB;
		output.statusOfMarketPlace = state.get().statusOfMarketPlace;

	}

	struct getInfoOfCollectionByCreator_locals
	{
		uint32 _t, _r, cnt;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfCollectionByCreator)
	{
		locals.cnt = 0;

		for(locals._t = 0 ; locals._t < state.get().numberOfCollection; locals._t++)
		{
			if(state.get().Collections.get(locals._t).creator == input.creator)
			{
				locals.cnt++;
			}
			if(locals.cnt == input.orderOfCollection)
			{
				output.currentSize = state.get().Collections.get(locals._t).currentSize;
				output.idOfCollection = locals._t;
				output.maxSizeHoldingPerOneId = state.get().Collections.get(locals._t).maxSizeHoldingPerOneId;
				output.priceForDropMint = state.get().Collections.get(locals._t).priceForDropMint;
				output.royalty = state.get().Collections.get(locals._t).royalty;
				output.typeOfCollection = state.get().Collections.get(locals._t).typeOfCollection;
				
				for(locals._r = 0 ; locals._r < 64; locals._r++)
				{
					output.URI.set(locals._r, state.get().Collections.get(locals._t).URI.get(locals._r));
				}

				return ;
			}
		}

	}

	struct getInfoOfCollectionById_locals
	{
		uint32 _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfCollectionById)
	{
		if(input.idOfCollection>= state.get().numberOfCollection)
		{
			return ;
		}

		output.creator = state.get().Collections.get(input.idOfCollection).creator;
		output.currentSize = state.get().Collections.get(input.idOfCollection).currentSize;
		output.maxSizeHoldingPerOneId = state.get().Collections.get(input.idOfCollection).maxSizeHoldingPerOneId;
		output.priceForDropMint = state.get().Collections.get(input.idOfCollection).priceForDropMint;
		output.royalty = state.get().Collections.get(input.idOfCollection).royalty;
		output.typeOfCollection = state.get().Collections.get(input.idOfCollection).typeOfCollection;

		for(locals._t = 0 ; locals._t < 64; locals._t++)
		{
			output.URI.set(locals._t, state.get().Collections.get(input.idOfCollection).URI.get(locals._t));
		}

	}

	struct getIncomingAuctions_locals
	{
		uint32 curDate;
		uint32 _t, cnt, _r;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getIncomingAuctions)
	{
		if(input.offset + input.count >= state.get().numberOfNFT || input.count > 1024)
		{
			return ;
		}

		packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
		locals.cnt = 0;
		locals._r = 0;

		for(locals._t = 0; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(locals.curDate < state.get().NFTs.get(locals._t).startTimeOfAuction)
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

	}

	struct getInfoOfNFTById_locals
	{
		uint32 _r;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getInfoOfNFTById)
	{
		output.salePrice = state.get().NFTs.get(input.NFTId).salePrice;
		output.askMaxPrice = state.get().NFTs.get(input.NFTId).askMaxPrice;
		output.currentPriceOfAuction = state.get().NFTs.get(input.NFTId).currentPriceOfAuction;
		output.royalty = state.get().NFTs.get(input.NFTId).royalty;
		output.NFTidForExchange = state.get().NFTs.get(input.NFTId).NFTidForExchange;
		output.creator = state.get().NFTs.get(input.NFTId).creator;
		output.possessor = state.get().NFTs.get(input.NFTId).possessor;
		output.askUser = state.get().NFTs.get(input.NFTId).askUser;
		output.creatorOfAuction = state.get().NFTs.get(input.NFTId).creatorOfAuction;
		output.statusOfAuction = state.get().NFTs.get(input.NFTId).statusOfAuction;
		output.statusOfSale = state.get().NFTs.get(input.NFTId).statusOfSale;
		output.statusOfAsk = state.get().NFTs.get(input.NFTId).statusOfAsk;
		output.paymentMethodOfAsk = state.get().NFTs.get(input.NFTId).paymentMethodOfAsk;
		output.statusOfExchange = state.get().NFTs.get(input.NFTId).statusOfExchange;
		output.paymentMethodOfAuction = state.get().NFTs.get(input.NFTId).paymentMethodOfAuction;
		unpackQbayDate(output.yearAuctionStarted, output.monthAuctionStarted, output.dayAuctionStarted, output.hourAuctionStarted, output.minuteAuctionStarted, output.secondAuctionStarted, state.get().NFTs.get(input.NFTId).startTimeOfAuction);
		unpackQbayDate(output.yearAuctionEnded, output.monthAuctionEnded, output.dayAuctionEnded, output.hourAuctionEnded, output.minuteAuctionEnded, output.secondAuctionEnded, state.get().NFTs.get(input.NFTId).endTimeOfAuction);
		
		for(locals._r = 0 ; locals._r < 64; locals._r++)
		{
			output.URI.set(locals._r, state.get().NFTs.get(input.NFTId).URI.get(locals._r));
		}

	}

	struct getUserCreatedCollection_locals
	{
		uint32 _r, cnt;
		sint32 _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getUserCreatedCollection)
	{
		if(input.offset + input.count > state.get().numberOfCollection || input.count > 1024)
		{
			return ;
		}

		locals.cnt = 0;
		locals._r = 0;

		for(locals._t = state.get().numberOfCollection - 1 ; locals._t >= 0; locals._t--)
		{
			if(state.get().Collections.get(locals._t).creator == input.user)
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
	}

	struct getUserCreatedNFT_locals
	{
		uint32 _r, cnt;
		sint32 _t;
	};

	PUBLIC_FUNCTION_WITH_LOCALS(getUserCreatedNFT)
	{

	if(input.offset + input.count > state.get().numberOfNFT || input.count > 1024)
	{
		return ;
	}

	locals.cnt = 0;
	locals._r = 0;

	for(locals._t = state.get().numberOfCollection - 1 ; locals._t >= 0; locals._t--)
	{
		if(state.get().NFTs.get(locals._t).creator == input.user)
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
	}

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
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
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 16);
		REGISTER_USER_PROCEDURE(changeStatusOfMarketPlace, 17);

    }

	INITIALIZE()
	{
		state.mut().cfbIssuer = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
		state.mut().marketPlaceOwner = ID(_R, _K, _D, _H, _C, _M, _R, _J, _Y, _C, _G, _K, _P, _D, _U, _Y, _R, _X, _G, _D, _Y, _Z, _C, _I, _Z, _I, _T, _A, _H, _Y, _O, _V, _G, _I, _U, _T, _K, _N, _D, _T, _E, _H, _P, _C, _C, _L, _W, _L, _Z, _X, _S, _H, _N, _F, _P, _D);
		state.mut().transferRightsFee = 1000000;

	}

	BEGIN_EPOCH()
	{
		state.mut().transferRightsFee = 100;
	}

	struct END_EPOCH_locals
	{
		QX::TransferShareManagementRights_input transferShareManagementRights_input;
		QX::TransferShareManagementRights_output transferShareManagementRights_output;
	};

	END_EPOCH_WITH_LOCALS()
	{
		qpi.distributeDividends(div(state.get().collectedShareHoldersFee * 1ULL, 676ULL));
		qpi.burn(state.get().collectedShareHoldersFee - div(state.get().collectedShareHoldersFee, 676ULL) * 676ULL);
		state.mut().collectedShareHoldersFee = 0;


		qpi.transfer(state.get().marketPlaceOwner, state.get().earnedQubic);
		locals.transferShareManagementRights_input.asset.assetName = QBAY_CFB_NAME;
		locals.transferShareManagementRights_input.asset.issuer = state.get().cfbIssuer;
		locals.transferShareManagementRights_input.newManagingContractIndex = QBAY_CONTRACT_INDEX;
		locals.transferShareManagementRights_input.numberOfShares = state.get().earnedCFB;

		INVOKE_OTHER_CONTRACT_PROCEDURE(QX, TransferShareManagementRights, locals.transferShareManagementRights_input, locals.transferShareManagementRights_output, 0);

		qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, state.get().earnedCFB, state.get().marketPlaceOwner);

		state.mut().earnedCFB = 0;
		state.mut().earnedQubic = 0;

	}

    PRE_ACQUIRE_SHARES()
	{
		output.allowTransfer = true;
	}
};
