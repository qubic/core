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
// Max royalty percent leaving room for market (20/1000) and shareholder (10/1000) fees on QU sales.
constexpr uint32 QBAY_MAX_ROYALTY_PERCENT = 100 - div(QBAY_FEE_NFT_SALE_MARKET + QBAY_FEE_NFT_SALE_SHAREHOLDERS, 10U);

// ---------------------------------------------------------------------------------------------------------------
// On-chain random drop (anti-reroll fix)
//
// Before this change, mintOfDrop() minted whatever URI the caller passed in. The drop item was therefore chosen
// off-chain (by the QubicBay server) and the caller could look up the rarity of the offered item, re-roll the
// server until a rare one was offered and then mint exactly that one (or simply pass any URI it liked).
//
// Now the drop item is chosen by the contract:
//   1. configureDrop(): the collection creator (or marketplace owner) commits a Merkle root over the ordered list of
//      the drop items (one leaf per item URI). The list may be shorter than the free slots of the collection; the
//      collection supply (currentSize) is then reduced to the list length. The list can be replaced any number of
//      times until the first mintOfDrop of the collection; from then on it is immutable (locked).
//   2. mintOfDrop(): after payment, the contract draws a uniformly random item index among the not-yet-minted items
//      (lazy Fisher-Yates shuffle, O(1) per mint). The entropy is derived from the digests of the previous tick,
//      which do not exist yet when the minter signs the transaction (tick data of tick T is fixed while tick T-2 is
//      processed), so the result cannot be predicted or chosen by the minter. The minted NFT is bound to that index.
//      The minter can pass the root of the list it was shown (expectedRoot) so that a list replaced just before the
//      mint is refunded instead of minted. mintOfDrop is paused while the marketplace status is off.
//   3. revealDropNFT(): anyone can set the NFT URI by presenting the URI committed at that index together with its
//      Merkle proof. The URI of a drop NFT can never be anything else than the committed one for its drawn index.
// ---------------------------------------------------------------------------------------------------------------
constexpr uint32 QBAY_MAX_DROP_COLLECTION_SIZE = 10000;                    // createCollection: volume <= 10 -> <= 10000 NFTs
constexpr uint32 QBAY_DROP_MAX_MERKLE_DEPTH = 16;                          // 2^16 leaves >= QBAY_MAX_DROP_COLLECTION_SIZE
constexpr uint8 QBAY_DROP_STATUS_NOT_CONFIGURED = 0;                       // mintOfDrop is disabled for the collection
constexpr uint8 QBAY_DROP_STATUS_CONFIGURED = 1;                           // list committed, mintOfDrop enabled, list can still be replaced
constexpr uint8 QBAY_DROP_STATUS_LOCKED = 2;                               // first item minted, list is immutable
// Packed per-NFT drop record (state.dropNFTInfo): bits 0..13 drop index, bits 14..28 collection id, bit 30 minted by
// random drop, bit 31 revealed. 0 means "not a random-drop NFT" (all NFTs minted before this upgrade).
constexpr uint32 QBAY_DROP_NFT_INDEX_BITS = 14;
constexpr uint32 QBAY_DROP_NFT_INDEX_MASK = (1U << QBAY_DROP_NFT_INDEX_BITS) - 1U;
constexpr uint32 QBAY_DROP_NFT_COLLECTION_MASK = (1U << 15) - 1U;
constexpr uint32 QBAY_DROP_NFT_FLAG_MINTED = 1U << 30;
constexpr uint32 QBAY_DROP_NFT_FLAG_REVEALED = 1U << 31;
STATIC_ASSERT(QBAY_MAX_DROP_COLLECTION_SIZE - 1U <= QBAY_DROP_NFT_INDEX_MASK, QbayDropIndexMustFitIntoPackedNFTInfo);
STATIC_ASSERT(QBAY_MAX_COLLECTION - 1U <= QBAY_DROP_NFT_COLLECTION_MASK, QbayCollectionIdMustFitIntoPackedNFTInfo);
STATIC_ASSERT(QBAY_MAX_DROP_COLLECTION_SIZE <= (1U << QBAY_DROP_MAX_MERKLE_DEPTH), QbayDropMerkleDepthTooSmall);
STATIC_ASSERT(QBAY_MAX_DROP_COLLECTION_SIZE < 65535U, QbayDropPoolEntriesMustFitIntoUint16);

// ---------------------------------------------------------------------------------------------------------------
// One-time cleanup of duplicate NFTs (runs once, in the first BEGIN_EPOCH executed by this code)
//
// Before the on-chain random drop, mintOfDrop minted any caller-supplied URI, so some drop items exist more than
// once on mainnet. QubicBay compiled the list in BEGIN_EPOCH (93 NFTs, checked against mainnet state in epoch 241):
//   - burn (41 NFTs, all held by their creator): possessor := NULL_ID. Skipped if the NFT is no longer held by its
//     creator, is listed for sale, has a running auction or an open offer. Open exchange proposals that target or
//     offer a burned NFT are cancelled, so a burned NFT can never be swapped out of NULL_ID again.
//   - set URI (52 NFTs): URI := new URI. Skipped if the current URI is not the expected one. A fixed-price listing
//     (no running auction) is cancelled so that nobody buys an item different from the one that was listed.
//     Nothing else is changed.
// Every entry is logged with its result (applied / skipped and why).
// ---------------------------------------------------------------------------------------------------------------
constexpr uint32 QBAY_CLEANUP_APPLIED = 0;
constexpr uint32 QBAY_CLEANUP_APPLIED_SALE_CANCELLED = 1;                 // set URI applied and fixed-price listing cancelled
constexpr uint32 QBAY_CLEANUP_SKIPPED_NO_NFT = 2;
constexpr uint32 QBAY_CLEANUP_SKIPPED_POSSESSOR_CHANGED = 3;
constexpr uint32 QBAY_CLEANUP_SKIPPED_FOR_SALE = 4;
constexpr uint32 QBAY_CLEANUP_SKIPPED_AUCTION_RUNNING = 5;
constexpr uint32 QBAY_CLEANUP_SKIPPED_OPEN_OFFER = 6;
constexpr uint32 QBAY_CLEANUP_SKIPPED_URI_CHANGED = 7;

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
		//		For on-chain random drop
		dropNotConfigured = 33,
		dropAlreadyConfigured = 34,				// the list is locked (an item has been minted already)
		invalidMerkleProof = 35,
		notDropNFT = 36,
		alreadyRevealed = 37,
		dropRootMismatch = 38,					// mintOfDrop: expectedRoot differs from the committed list
		//		Event log types (not return codes)
		dropConfiguredEvent = 40,
		dropMintedEvent = 41,
		dropRevealedEvent = 42,
		dropLockedEvent = 43,
		cleanupBurnEvent = 44,					// one-time duplicate cleanup (CleanupLogger.result = QBAY_CLEANUP_*)
		cleanupSetURIEvent = 45,
		cleanupProposalCancelledEvent = 46,
	};

	struct Logger
	{
		uint32 _contractIndex;
		uint32 _type; // Assign a random unique (per contract) number to distinguish messages of different types
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	struct DropLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 collectionId;
		uint32 NFTid;
		uint32 dropIndex;
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	struct CleanupLogger
	{
		uint32 _contractIndex;
		uint32 _type;
		uint32 NFTid;
		uint32 result;							// QBAY_CLEANUP_*
		sint8 _terminator; // Only data before "_terminator" are logged
	};

	// Per-collection configuration of the on-chain random drop.
	struct DropConfig
	{
		id merkleRoot;							// Merkle root over the committed item list (see DropMerkleLeaf / DropMerkleNode)
		uint32 poolOffset;						// first slot of this collection inside state.dropPool (assigned when locked)
		uint16 numberOfItems;					// number of committed items (== currentSize while not locked)
		uint8 merkleDepth;						// ceil(log2(numberOfItems)), leaves beyond numberOfItems are NULL_ID
		uint8 status;							// QBAY_DROP_STATUS_*
		uint16 reservedItems;					// free slots of the collection at the first configureDrop (upper bound of numberOfItems)
		uint16 _padding0;
		uint32 _padding1;
	};

	// Leaf of the drop Merkle tree: K12 of this 72-byte struct (no padding). URI bytes >= QBAY_LENGTH_OF_URI are 0.
	// Leaves (72 bytes) and inner nodes (64 bytes) have different input lengths, so they cannot be confused.
	struct DropMerkleLeaf
	{
		uint32 collectionId;
		uint32 dropIndex;
		Array<uint8, 64> URI;
	};

	// Inner node of the drop Merkle tree: K12 of this 64-byte struct.
	struct DropMerkleNode
	{
		id left;
		id right;
	};

	// Input of the per-mint entropy hash (176 bytes, no padding).
	struct DropEntropyInput
	{
		id prevSpectrumDigest;
		id prevUniverseDigest;
		id prevComputerDigest;
		id previousDropEntropy;
		id minter;
		uint32 tick;
		uint32 collectionId;
		uint32 remaining;
		uint32 NFTid;
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

		// ----- Added for the on-chain random drop. Appended at the end so the upgrade is a PADDING state change:
		// ----- all fields above keep their layout and the new fields start zeroed (= no drop configured).
		id dropEntropy;													// hash chain of all drop draws
		uint32 dropPoolUsed;											// number of dropPool slots handed out to collections
		uint32 duplicateCleanupDone;									// 1 once the one-time duplicate NFT cleanup has run
		Array<DropConfig, QBAY_MAX_COLLECTION> dropConfigs;				// indexed by collection id
		Array<uint16, QBAY_MAX_NUMBER_NFT> dropPool;					// lazy Fisher-Yates pools (0 = identity, else index + 1)
		Array<uint32, QBAY_MAX_NUMBER_NFT> dropNFTInfo;					// indexed by NFT id, packed (see QBAY_DROP_NFT_*)
	};

	inline static uint32 packDropNFTInfo(uint32 collectionId, uint32 dropIndex)
	{
		return QBAY_DROP_NFT_FLAG_MINTED | ((collectionId & QBAY_DROP_NFT_COLLECTION_MASK) << QBAY_DROP_NFT_INDEX_BITS) | (dropIndex & QBAY_DROP_NFT_INDEX_MASK);
	}
	inline static bit isDropNFTInfoMinted(uint32 info)
	{
		return (info & QBAY_DROP_NFT_FLAG_MINTED) != 0;
	}
	inline static bit isDropNFTInfoRevealed(uint32 info)
	{
		return (info & QBAY_DROP_NFT_FLAG_REVEALED) != 0;
	}
	inline static uint32 dropNFTInfoCollectionId(uint32 info)
	{
		return (info >> QBAY_DROP_NFT_INDEX_BITS) & QBAY_DROP_NFT_COLLECTION_MASK;
	}
	inline static uint32 dropNFTInfoIndex(uint32 info)
	{
		return info & QBAY_DROP_NFT_INDEX_MASK;
	}

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

	inline static bool isValidOraclePrice(uint64 price)
	{
		return price > 0 && price < (uint64)MAX_AMOUNT;
	}

	inline static uint64 ceilDiv(uint64 numerator, uint64 denominator)
	{
		return div(numerator + denominator - 1ULL, denominator);
	}

	// Converts a QU-denominated sale price to required CFB (ceil). Returns false on invalid prices or overflow.
	inline static bool quSalePriceToRequiredCFB(uint64 salePriceQu, uint64 priceOfCFB, uint64 priceOfQubic, uint64& requiredCFB)
	{
		requiredCFB = 0;
		if (!isValidOraclePrice(priceOfCFB) || !isValidOraclePrice(priceOfQubic) || salePriceQu == 0)
		{
			return false;
		}
		if (salePriceQu > div((uint64)MAX_AMOUNT, priceOfCFB))
		{
			return false;
		}
		requiredCFB = ceilDiv(salePriceQu * priceOfCFB, priceOfQubic);
		return requiredCFB > 0;
	}

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
		uint32 _padding0;
		// Optional: Merkle root of the item list shown to the minter. NULL_ID (or a 4-byte input, which the core pads
		// with zeros) skips the check; otherwise the mint is refunded if the committed list is a different one.
		// The former "URI" field was removed on purpose: the drop item is drawn by the contract, never chosen by the
		// caller. A client still sending the old 68-byte layout (collectionId + URI) puts URI bytes into expectedRoot
		// and is therefore refunded with dropRootMismatch instead of being charged for an item it did not choose.
		id expectedRoot;
	};

	struct mintOfDrop_output
	{
		uint32 returnCode;
		uint32 NFTid;							// id of the minted NFT (only valid if returnCode == success)
		uint32 dropIndex;						// drawn index in the committed item list (only valid if returnCode == success)
	};

	struct configureDrop_input
	{
		id merkleRoot;							// root over the ordered list of the remaining drop items
		uint32 collectionId;
		uint32 numberOfItems;					// 1 .. free slots of the collection; currentSize is set to this value
	};

	struct configureDrop_output
	{
		uint32 returnCode;
	};

	struct revealDropNFT_input
	{
		Array<id, QBAY_DROP_MAX_MERKLE_DEPTH> proof;	// sibling hashes from leaf level upwards (only merkleDepth are used)
		Array<uint8, 64> URI;					// URI committed at the NFT's drop index
		uint32 NFTid;
	};

	struct revealDropNFT_output
	{
		uint32 returnCode;
	};

	struct getDropConfig_input
	{
		uint32 collectionId;
	};

	struct getDropConfig_output
	{
		id merkleRoot;
		uint32 numberOfItems;
		uint32 remainingItems;
		uint32 merkleDepth;
		uint32 status;							// QBAY_DROP_STATUS_*
		uint32 reservedItems;					// max numberOfItems a replacement list may have (only while status == CONFIGURED)
	};

	struct getDropNFTInfo_input
	{
		uint32 NFTid;
	};

	struct getDropNFTInfo_output
	{
		uint32 collectionId;
		uint32 dropIndex;
		bit isRandomDrop;						// 1 if the NFT was minted through the on-chain random drop
		bit isRevealed;							// 1 if its URI has been set by revealDropNFT
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

		if (!isValidOraclePrice(input.CFBPrice) || !isValidOraclePrice(input.QubicPrice))
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);
			return;
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

		if(input.volume > 10 || input.royalty > QBAY_MAX_ROYALTY_PERCENT || input.priceForDropMint >= (uint64)MAX_AMOUNT)
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

			qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), locals.fee * state.get().priceOfCFB, NULL_ID);
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
			if(input.royalty > QBAY_MAX_ROYALTY_PERCENT)
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

	struct configureDrop_locals
	{
		InfoOfCollection collection;
		DropConfig newConfig;
		DropLogger dropLog;
		uint32 capacity;
		Logger log;
	};

	// Commits the ordered item list of a drop collection (Merkle root) and enables mintOfDrop for it.
	// Callable by the collection creator or by the marketplace owner (needed for drops that were already running
	// before this upgrade). The list covers the items that are still to be minted: for a new collection at most its
	// full volume, for a partially minted legacy collection at most its free slots. A shorter list reduces the supply
	// of the collection (currentSize = numberOfItems). The list can be replaced (also with a different length, up to
	// the free slots at the first configuration) as long as no item has been minted; the first mint locks it.
	PUBLIC_PROCEDURE_WITH_LOCALS(configureDrop)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.collectionId >= state.get().numberOfCollection || input.merkleRoot == NULL_ID)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.collection = state.get().Collections.get(input.collectionId);

		if (qpi.invocator() != locals.collection.creator && qpi.invocator() != state.get().marketPlaceOwner)
		{
			output.returnCode = LogInfo::notCollectionCreator;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notCollectionCreator, 0 };
			LOG_INFO(locals.log);
			return;
		}

		if (locals.collection.typeOfCollection != 0)
		{
			output.returnCode = LogInfo::notCollectionForDrop;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notCollectionForDrop, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.newConfig = state.get().dropConfigs.get(input.collectionId);

		// Once an item has been minted, the list is final (minted NFTs are bound to indices of this list).
		if (locals.newConfig.status == QBAY_DROP_STATUS_LOCKED)
		{
			output.returnCode = LogInfo::dropAlreadyConfigured;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::dropAlreadyConfigured, 0 };
			LOG_INFO(locals.log);
			return;
		}

		if (locals.newConfig.status == QBAY_DROP_STATUS_NOT_CONFIGURED)
		{
			if (locals.collection.currentSize <= 0)
			{
				output.returnCode = LogInfo::limitCollectionVolume;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::limitCollectionVolume, 0 };
				LOG_INFO(locals.log);
				return;
			}
			// First configuration: remember the free slots, they bound every (re)committed list of this collection.
			locals.newConfig.reservedItems = (uint16)locals.collection.currentSize;
		}

		// The list may be shorter than the free slots (fewer real items), never longer.
		if (input.numberOfItems == 0 || input.numberOfItems > locals.newConfig.reservedItems || input.numberOfItems > QBAY_MAX_DROP_COLLECTION_SIZE)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);
			return;
		}

		// The dropPool slots are assigned when the list gets locked by the first mint (see mintOfDrop), so replacing
		// the list does not consume pool space. Unused reserved slots are released to numberOfNFTIncoming at that point.
		locals.newConfig.merkleRoot = input.merkleRoot;
		locals.newConfig.poolOffset = 0;
		locals.newConfig.numberOfItems = (uint16)input.numberOfItems;
		locals.newConfig.merkleDepth = 0;
		locals.capacity = 1;
		while (locals.capacity < input.numberOfItems)
		{
			locals.capacity <<= 1;
			locals.newConfig.merkleDepth++;
		}
		locals.newConfig.status = QBAY_DROP_STATUS_CONFIGURED;
		state.mut().dropConfigs.set(input.collectionId, locals.newConfig);

		// Supply of the drop == length of the committed list.
		locals.collection.currentSize = (sint16)input.numberOfItems;
		state.mut().Collections.set(input.collectionId, locals.collection);

		output.returnCode = LogInfo::success;
		locals.dropLog = DropLogger{ QBAY_CONTRACT_INDEX, LogInfo::dropConfiguredEvent, input.collectionId, 0, input.numberOfItems, 0 };
		LOG_INFO(locals.dropLog);
	}

	struct mintOfDrop_locals 
	{
		InfoOfCollection collection;
		DropConfig dropConfig;
		InfoOfNFT newNFT;
		DropEntropyInput entropyInput;
		id randomDigest;
		DropLogger dropLog;
		uint64 cntOfNFTHoldingPerOneId;
		uint32 _t;
		uint32 NFTid;
		uint32 remaining;
		uint32 slot;
		uint32 lastSlot;
		uint32 dropIndex;
		uint32 lastItem;
		uint32 poolValue;
		Logger log;
	};

	PUBLIC_PROCEDURE_WITH_LOCALS(mintOfDrop)
	{
		// Pause switch: like mint(), mintOfDrop is unavailable (for everybody) while the marketplace status is off.
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

		if(input.collectionId >= state.get().numberOfCollection)
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

		locals.collection = state.get().Collections.get(input.collectionId);

		if(locals.collection.typeOfCollection == 1)
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

		if(locals.collection.currentSize <= 0) 
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

		// Drop minting is only possible after the item list has been committed on-chain (configureDrop).
		locals.dropConfig = state.get().dropConfigs.get(input.collectionId);
		if((locals.dropConfig.status != QBAY_DROP_STATUS_CONFIGURED && locals.dropConfig.status != QBAY_DROP_STATUS_LOCKED)
			|| (uint32)locals.collection.currentSize > locals.dropConfig.numberOfItems)
		{
			output.returnCode = LogInfo::dropNotConfigured;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::dropNotConfigured, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		// The list can be replaced until the first mint: let the minter pin the list it was shown.
		if(input.expectedRoot != NULL_ID && input.expectedRoot != locals.dropConfig.merkleRoot)
		{
			output.returnCode = LogInfo::dropRootMismatch;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::dropRootMismatch, 0 };
			LOG_INFO(locals.log);

			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			return ;
		}

		// First mint locks the list and assigns its dropPool slots. Cannot fail while the sum of the locked lists
		// <= numberOfNFTIncoming < QBAY_MAX_NUMBER_NFT, kept as a hard guard.
		if(locals.dropConfig.status == QBAY_DROP_STATUS_CONFIGURED && (uint64)state.get().dropPoolUsed + locals.dropConfig.numberOfItems > QBAY_MAX_NUMBER_NFT)
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

		locals.cntOfNFTHoldingPerOneId = 0;

		for(locals._t = 0; locals._t < state.get().numberOfNFT; locals._t++)
		{
			if(state.get().NFTs.get(locals._t).creator == locals.collection.creator && state.get().NFTs.get(locals._t).possessor == qpi.invocator())
			{
				locals.cntOfNFTHoldingPerOneId++;
			}
		}

		if(locals.cntOfNFTHoldingPerOneId >= locals.collection.maxSizeHoldingPerOneId)
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

		if((uint64)qpi.invocationReward() < locals.collection.priceForDropMint)
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

		if((uint64)qpi.invocationReward() > locals.collection.priceForDropMint)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)locals.collection.priceForDropMint);
		}

		qpi.transfer(locals.collection.creator, locals.collection.priceForDropMint);

		if(locals.dropConfig.status == QBAY_DROP_STATUS_CONFIGURED)
		{
			// Lock the list. Its pool region is fresh (never used), i.e. all zeros = identity permutation.
			locals.dropConfig.status = QBAY_DROP_STATUS_LOCKED;
			locals.dropConfig.poolOffset = state.get().dropPoolUsed;
			state.mut().dropPoolUsed += locals.dropConfig.numberOfItems;
			state.mut().dropConfigs.set(input.collectionId, locals.dropConfig);
			// Release the reserved slots that the (shorter) list does not use. Before locking they stay reserved, so
			// a replacement list can grow back up to reservedItems without competing with new collections.
			state.mut().numberOfNFTIncoming -= (uint64)(locals.dropConfig.reservedItems - locals.dropConfig.numberOfItems);

			locals.dropLog = DropLogger{ QBAY_CONTRACT_INDEX, LogInfo::dropLockedEvent, input.collectionId, 0, locals.dropConfig.numberOfItems, 0 };
			LOG_INFO(locals.dropLog);
		}

		// ---- Draw the item on-chain. Payment is final at this point, so every draw costs the full mint price. ----
		// Entropy: digests of the previous tick (unknown when the transaction was signed and when the tick leader fixed
		// the transaction set of this tick), chained with all previous draws and made unique per mint.
		locals.NFTid = state.get().numberOfNFT;
		locals.remaining = (uint32)locals.collection.currentSize;

		locals.entropyInput.prevSpectrumDigest = qpi.getPrevSpectrumDigest();
		locals.entropyInput.prevUniverseDigest = qpi.getPrevUniverseDigest();
		locals.entropyInput.prevComputerDigest = qpi.getPrevComputerDigest();
		locals.entropyInput.previousDropEntropy = state.get().dropEntropy;
		locals.entropyInput.minter = qpi.invocator();
		locals.entropyInput.tick = qpi.tick();
		locals.entropyInput.collectionId = input.collectionId;
		locals.entropyInput.remaining = locals.remaining;
		locals.entropyInput.NFTid = locals.NFTid;
		locals.randomDigest = qpi.K12(locals.entropyInput);
		state.mut().dropEntropy = locals.randomDigest;

		// Lazy Fisher-Yates: slots [0, remaining) of the collection's pool hold the not-yet-minted item indices.
		// A pool value of 0 means "slot holds its own index", otherwise it holds (item index + 1).
		// Modulo bias is < remaining / 2^64 (remaining <= 10000), i.e. negligible.
		locals.slot = (uint32)mod<uint64>(locals.randomDigest.u64._0, (uint64)locals.remaining);
		locals.lastSlot = locals.remaining - 1;

		locals.poolValue = state.get().dropPool.get(locals.dropConfig.poolOffset + locals.slot);
		if (locals.poolValue == 0)
		{
			locals.dropIndex = locals.slot;
		}
		else
		{
			locals.dropIndex = locals.poolValue - 1;
		}

		locals.poolValue = state.get().dropPool.get(locals.dropConfig.poolOffset + locals.lastSlot);
		if (locals.poolValue == 0)
		{
			locals.lastItem = locals.lastSlot;
		}
		else
		{
			locals.lastItem = locals.poolValue - 1;
		}
		// Move the last remaining item into the drawn slot; the last slot falls out of range with currentSize--.
		state.mut().dropPool.set(locals.dropConfig.poolOffset + locals.slot, (uint16)(locals.lastItem + 1));

		locals.collection.currentSize--;
		state.mut().Collections.set(input.collectionId, locals.collection);

		locals.newNFT.creator = locals.collection.creator;
		locals.newNFT.possessor = qpi.invocator();
		locals.newNFT.royalty = locals.collection.royalty;
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
		// URI stays empty until revealDropNFT() proves the committed URI of the drawn index.
		locals.newNFT.URI.setAll(0);

		state.mut().NFTs.set(locals.NFTid, locals.newNFT);
		state.mut().dropNFTInfo.set(locals.NFTid, packDropNFTInfo(input.collectionId, locals.dropIndex));
		state.mut().numberOfNFT++;

		output.returnCode = LogInfo::success;
		output.NFTid = locals.NFTid;
		output.dropIndex = locals.dropIndex;

		locals.dropLog = DropLogger{ QBAY_CONTRACT_INDEX, LogInfo::dropMintedEvent, input.collectionId, locals.NFTid, locals.dropIndex, 0 };
		LOG_INFO(locals.dropLog);
	}

	struct revealDropNFT_locals
	{
		DropConfig dropConfig;
		DropMerkleLeaf leaf;
		DropMerkleNode node;
		InfoOfNFT updatedNFT;
		DropLogger dropLog;
		id hash;
		uint32 info;
		uint32 collectionId;
		uint32 dropIndex;
		uint32 level;
		uint32 _t;
		Logger log;
	};

	// Sets the URI of a random-drop NFT. Permissionless: anybody (minter, QubicBay server, creator) can submit the
	// committed URI of the NFT's drawn index together with its Merkle proof; no other URI can pass verification.
	PUBLIC_PROCEDURE_WITH_LOCALS(revealDropNFT)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.NFTid >= state.get().numberOfNFT)
		{
			output.returnCode = LogInfo::wrongNFTId;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::wrongNFTId, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.info = state.get().dropNFTInfo.get(input.NFTid);
		if (!isDropNFTInfoMinted(locals.info))
		{
			output.returnCode = LogInfo::notDropNFT;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::notDropNFT, 0 };
			LOG_INFO(locals.log);
			return;
		}

		if (isDropNFTInfoRevealed(locals.info))
		{
			output.returnCode = LogInfo::alreadyRevealed;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::alreadyRevealed, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.collectionId = dropNFTInfoCollectionId(locals.info);
		locals.dropIndex = dropNFTInfoIndex(locals.info);
		locals.dropConfig = state.get().dropConfigs.get(locals.collectionId);

		// Leaf = K12(collectionId, dropIndex, URI normalized to QBAY_LENGTH_OF_URI bytes).
		locals.leaf.collectionId = locals.collectionId;
		locals.leaf.dropIndex = locals.dropIndex;
		for (locals._t = 0; locals._t < 64; locals._t++)
		{
			if (locals._t >= QBAY_LENGTH_OF_URI)
			{
				locals.leaf.URI.set(locals._t, 0);
			}
			else
			{
				locals.leaf.URI.set(locals._t, input.URI.get(locals._t));
			}
		}
		locals.hash = qpi.K12(locals.leaf);

		for (locals.level = 0; locals.level < locals.dropConfig.merkleDepth; locals.level++)
		{
			if (((locals.dropIndex >> locals.level) & 1U) == 1U)
			{
				locals.node.left = input.proof.get(locals.level);
				locals.node.right = locals.hash;
			}
			else
			{
				locals.node.left = locals.hash;
				locals.node.right = input.proof.get(locals.level);
			}
			locals.hash = qpi.K12(locals.node);
		}

		if (locals.hash != locals.dropConfig.merkleRoot)
		{
			output.returnCode = LogInfo::invalidMerkleProof;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidMerkleProof, 0 };
			LOG_INFO(locals.log);
			return;
		}

		locals.updatedNFT = state.get().NFTs.get(input.NFTid);
		locals.updatedNFT.URI = locals.leaf.URI;
		state.mut().NFTs.set(input.NFTid, locals.updatedNFT);
		state.mut().dropNFTInfo.set(input.NFTid, locals.info | QBAY_DROP_NFT_FLAG_REVEALED);

		output.returnCode = LogInfo::success;
		locals.dropLog = DropLogger{ QBAY_CONTRACT_INDEX, LogInfo::dropRevealedEvent, locals.collectionId, input.NFTid, locals.dropIndex, 0 };
		LOG_INFO(locals.dropLog);
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

		if(input.NFTid >= QBAY_MAX_NUMBER_NFT || input.price == 0 || input.price >= (uint64)MAX_AMOUNT)									//			NFTid should be less than MAX_NUMBER_NFT
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
		uint64 requiredCFB;
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

		if(state.get().NFTs.get(input.NFTid).salePrice == 0 || state.get().NFTs.get(input.NFTid).salePrice >= (uint64)MAX_AMOUNT)
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

		if(input.methodOfPayment == 0)
		{
			if((uint64)qpi.invocationReward() < state.get().NFTs.get(input.NFTid).salePrice) 
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

			if((uint64)qpi.invocationReward() > state.get().NFTs.get(input.NFTid).salePrice)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)state.get().NFTs.get(input.NFTid).salePrice);
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
			if(!quSalePriceToRequiredCFB(
				state.get().NFTs.get(input.NFTid).salePrice,
				state.get().priceOfCFB,
				state.get().priceOfQubic,
				locals.requiredCFB))
			{
				output.returnCode = LogInfo::invalidInput;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);
				return;
			}
			locals.possessedCFBAmount = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
			if(locals.possessedCFBAmount < 0 || (uint64)locals.possessedCFBAmount < locals.requiredCFB)
			{
				output.returnCode = LogInfo::insufficientCFB;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::insufficientCFB, 0 };
				LOG_INFO(locals.log);

				return ;
			}

			locals.transferredAmountOfCFB = (sint64)locals.requiredCFB;
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
		if(input.NFTid >= QBAY_MAX_NUMBER_NFT || input.askPrice == 0 || input.askPrice >= (uint64)MAX_AMOUNT)
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
			if(!isValidOraclePrice(state.get().priceOfCFB) || !isValidOraclePrice(state.get().priceOfQubic))
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

			if((input.askPrice < state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE && input.paymentMethod == state.get().NFTs.get(input.NFTid).paymentMethodOfAsk)
			|| (input.paymentMethod == 1 && state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0 && ceilDiv(input.askPrice, state.get().priceOfCFB) < div((state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.get().priceOfQubic))
			|| (input.paymentMethod == 0 && state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 1 && ceilDiv(input.askPrice, state.get().priceOfQubic) < div((state.get().NFTs.get(input.NFTid).askMaxPrice + QBAY_MIN_DELTA_SIZE) * 1ULL, state.get().priceOfCFB)))
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
			if((uint64)qpi.invocationReward() < input.askPrice)
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

			if((uint64)qpi.invocationReward() > input.askPrice)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)input.askPrice);
			}
		}
		
		else if(input.paymentMethod == 1)
		{
			if(qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			locals.tmp = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);
			if(locals.tmp < 0 || (uint64)locals.tmp < input.askPrice)
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
			if(state.get().NFTs.get(input.NFTid).askMaxPrice >= (uint64)MAX_AMOUNT)
			{
				output.returnCode = LogInfo::invalidInput;
				locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
				LOG_INFO(locals.log);

				// Roll back this call's payment only. Do not use qpi.invocationReward() here:
				// it is still the original reward amount, and QU/CFB paths above may have
				// already transferred part of it (excess QU refund or full QU refund before CFB).
				if(input.paymentMethod == 0)
				{
					qpi.transfer(qpi.invocator(), (sint64)input.askPrice);
				}
				else if(input.paymentMethod == 1)
				{
					qpi.transferShareOwnershipAndPossession(QBAY_CFB_NAME, state.get().cfbIssuer, SELF, SELF, input.askPrice, qpi.invocator());
				}
				return;
			}

			if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
			{
				qpi.transfer(state.get().NFTs.get(input.NFTid).askUser, (sint64)state.get().NFTs.get(input.NFTid).askMaxPrice);
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

		if(state.get().NFTs.get(input.NFTid).askMaxPrice == 0 || state.get().NFTs.get(input.NFTid).askMaxPrice >= (uint64)MAX_AMOUNT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
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
			qpi.transfer(state.get().NFTs.get(input.NFTid).possessor, (sint64)state.get().NFTs.get(input.NFTid).askMaxPrice - locals.creatorFee - locals.marketFee - locals.shareHolderFee);
			
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

		if(state.get().NFTs.get(input.NFTid).askMaxPrice == 0 || state.get().NFTs.get(input.NFTid).askMaxPrice >= (uint64)MAX_AMOUNT)
		{
			output.returnCode = LogInfo::invalidInput;
			locals.log = Logger{ QBAY_CONTRACT_INDEX, LogInfo::invalidInput, 0 };
			LOG_INFO(locals.log);
			return ;
		}

		if(state.get().NFTs.get(input.NFTid).paymentMethodOfAsk == 0)
		{
			qpi.transfer(state.get().NFTs.get(input.NFTid).askUser, (sint64)state.get().NFTs.get(input.NFTid).askMaxPrice);
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

		if(input.NFTId >= QBAY_MAX_NUMBER_NFT || input.minPrice == 0 || input.minPrice >= (uint64)MAX_AMOUNT || checkValidQbayDateTime(locals.startDate) == 0 || checkValidQbayDateTime(locals.endDate) == 0)
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
		if(input.NFTId >= QBAY_MAX_NUMBER_NFT || input.price == 0 || input.price >= (uint64)MAX_AMOUNT)
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
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - (sint64)input.price);
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

			locals.tmp = qpi.numberOfPossessedShares(QBAY_CFB_NAME, state.get().cfbIssuer, qpi.invocator(), qpi.invocator(), QBAY_CONTRACT_INDEX, QBAY_CONTRACT_INDEX);

			if(locals.tmp < 0 || (uint64)locals.tmp < input.price)
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

	for(locals._t = (sint32)state.get().numberOfNFT - 1 ; locals._t >= 0; locals._t--)
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

	PUBLIC_FUNCTION(getDropConfig)
	{
		if (input.collectionId >= state.get().numberOfCollection)
		{
			return;
		}
		output.merkleRoot = state.get().dropConfigs.get(input.collectionId).merkleRoot;
		output.numberOfItems = state.get().dropConfigs.get(input.collectionId).numberOfItems;
		output.merkleDepth = state.get().dropConfigs.get(input.collectionId).merkleDepth;
		output.status = state.get().dropConfigs.get(input.collectionId).status;
		output.reservedItems = state.get().dropConfigs.get(input.collectionId).reservedItems;
		if (output.status != QBAY_DROP_STATUS_NOT_CONFIGURED && state.get().Collections.get(input.collectionId).currentSize > 0)
		{
			output.remainingItems = (uint32)state.get().Collections.get(input.collectionId).currentSize;
		}
	}

	PUBLIC_FUNCTION(getDropNFTInfo)
	{
		if (input.NFTid >= state.get().numberOfNFT || !isDropNFTInfoMinted(state.get().dropNFTInfo.get(input.NFTid)))
		{
			return;
		}
		output.isRandomDrop = 1;
		output.isRevealed = isDropNFTInfoRevealed(state.get().dropNFTInfo.get(input.NFTid));
		output.collectionId = dropNFTInfoCollectionId(state.get().dropNFTInfo.get(input.NFTid));
		output.dropIndex = dropNFTInfoIndex(state.get().dropNFTInfo.get(input.NFTid));
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
		REGISTER_USER_FUNCTION(getDropConfig, 10);
		REGISTER_USER_FUNCTION(getDropNFTInfo, 11);

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
		REGISTER_USER_PROCEDURE(configureDrop, 18);
		REGISTER_USER_PROCEDURE(revealDropNFT, 19);

    }

	// ----- One-time duplicate NFT cleanup (see the comment at QBAY_CLEANUP_APPLIED) -----

	struct _cleanupBurn_input
	{
		uint32 NFTid;
	};

	struct _cleanupBurn_output
	{
		uint32 result;							// QBAY_CLEANUP_*
	};

	struct _cleanupBurn_locals
	{
		InfoOfNFT nft;
		CleanupLogger log;
		uint32 curDate;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(_cleanupBurn)
	{
		output.result = QBAY_CLEANUP_APPLIED;
		if (input.NFTid >= state.get().numberOfNFT)
		{
			output.result = QBAY_CLEANUP_SKIPPED_NO_NFT;
		}
		else
		{
			locals.nft = state.get().NFTs.get(input.NFTid);
			packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
			if (locals.nft.possessor != locals.nft.creator)
			{
				output.result = QBAY_CLEANUP_SKIPPED_POSSESSOR_CHANGED;
			}
			else if (locals.nft.statusOfSale == 1)
			{
				output.result = QBAY_CLEANUP_SKIPPED_FOR_SALE;
			}
			else if (locals.curDate <= locals.nft.endTimeOfAuction)
			{
				output.result = QBAY_CLEANUP_SKIPPED_AUCTION_RUNNING;
			}
			else if (locals.nft.statusOfAsk == 1)
			{
				output.result = QBAY_CLEANUP_SKIPPED_OPEN_OFFER;
			}
		}

		if (output.result == QBAY_CLEANUP_APPLIED)
		{
			locals.nft.possessor = NULL_ID;
			// Cancel an open proposal made to this NFT. Proposals that offer this NFT are stored on their target and
			// are cancelled by _cleanupCancelProposals.
			locals.nft.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
			locals.nft.statusOfExchange = 0;
			state.mut().NFTs.set(input.NFTid, locals.nft);
		}

		locals.log = CleanupLogger{ QBAY_CONTRACT_INDEX, LogInfo::cleanupBurnEvent, input.NFTid, output.result, 0 };
		LOG_INFO(locals.log);
	}

	struct _cleanupCancelProposals_input
	{
	};

	struct _cleanupCancelProposals_output
	{
	};

	struct _cleanupCancelProposals_locals
	{
		InfoOfNFT nft;
		CleanupLogger log;
		uint32 _t;
		uint32 offeredNFT;
	};

	// Cancels every open exchange proposal that offers a burned NFT (possessor NULL_ID). Without this, accepting such
	// a stale proposal (listInExchange) would swap the burned NFT back to a real possessor.
	PRIVATE_PROCEDURE_WITH_LOCALS(_cleanupCancelProposals)
	{
		for (locals._t = 0; locals._t < state.get().numberOfNFT; locals._t++)
		{
			locals.offeredNFT = state.get().NFTs.get(locals._t).NFTidForExchange;
			if (state.get().NFTs.get(locals._t).statusOfExchange == 1 && locals.offeredNFT < state.get().numberOfNFT
				&& state.get().NFTs.get(locals.offeredNFT).possessor == NULL_ID)
			{
				locals.nft = state.get().NFTs.get(locals._t);
				locals.nft.NFTidForExchange = QBAY_MAX_NUMBER_NFT;
				locals.nft.statusOfExchange = 0;
				state.mut().NFTs.set(locals._t, locals.nft);

				locals.log = CleanupLogger{ QBAY_CONTRACT_INDEX, LogInfo::cleanupProposalCancelledEvent, locals._t, locals.offeredNFT, 0 };
				LOG_INFO(locals.log);
			}
		}
	}

	struct _cleanupSetURI_input
	{
		uint32 NFTid;
		uint32 _padding0;
		uint64 expected0;						// expected current URI, 64 bytes as 8 little-endian words (zero padded)
		uint64 expected1;
		uint64 expected2;
		uint64 expected3;
		uint64 expected4;
		uint64 expected5;
		uint64 expected6;
		uint64 expected7;
		uint64 new0;							// new URI, same encoding
		uint64 new1;
		uint64 new2;
		uint64 new3;
		uint64 new4;
		uint64 new5;
		uint64 new6;
		uint64 new7;
	};

	struct _cleanupSetURI_output
	{
		uint32 result;							// QBAY_CLEANUP_*
	};

	struct _cleanupSetURI_locals
	{
		InfoOfNFT nft;
		Array<uint64, 8> expectedWords;
		Array<uint64, 8> newWords;
		CleanupLogger log;
		uint32 curDate;
		uint32 _t;
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(_cleanupSetURI)
	{
		output.result = QBAY_CLEANUP_APPLIED;
		if (input.NFTid >= state.get().numberOfNFT)
		{
			output.result = QBAY_CLEANUP_SKIPPED_NO_NFT;
		}
		else
		{
			locals.nft = state.get().NFTs.get(input.NFTid);
			locals.expectedWords.set(0, input.expected0);
			locals.expectedWords.set(1, input.expected1);
			locals.expectedWords.set(2, input.expected2);
			locals.expectedWords.set(3, input.expected3);
			locals.expectedWords.set(4, input.expected4);
			locals.expectedWords.set(5, input.expected5);
			locals.expectedWords.set(6, input.expected6);
			locals.expectedWords.set(7, input.expected7);
			for (locals._t = 0; locals._t < 64; locals._t++)
			{
				if (locals.nft.URI.get(locals._t) != (uint8)((locals.expectedWords.get(locals._t >> 3) >> ((locals._t & 7) << 3)) & 0xFF))
				{
					output.result = QBAY_CLEANUP_SKIPPED_URI_CHANGED;
				}
			}
		}

		if (output.result == QBAY_CLEANUP_APPLIED)
		{
			locals.newWords.set(0, input.new0);
			locals.newWords.set(1, input.new1);
			locals.newWords.set(2, input.new2);
			locals.newWords.set(3, input.new3);
			locals.newWords.set(4, input.new4);
			locals.newWords.set(5, input.new5);
			locals.newWords.set(6, input.new6);
			locals.newWords.set(7, input.new7);
			for (locals._t = 0; locals._t < 64; locals._t++)
			{
				locals.nft.URI.set(locals._t, (uint8)((locals.newWords.get(locals._t >> 3) >> ((locals._t & 7) << 3)) & 0xFF));
			}

			// A fixed-price listing was made for the old item: cancel it (like cancelSale). Running auctions and
			// exchange proposals are left untouched.
			packQbayDate(qpi.year(), qpi.month(), qpi.day(), qpi.hour(), qpi.minute(), qpi.second(), locals.curDate);
			if (locals.nft.statusOfSale == 1 && locals.curDate > locals.nft.endTimeOfAuction)
			{
				locals.nft.salePrice = QBAY_SALE_PRICE;
				locals.nft.statusOfSale = 0;
				output.result = QBAY_CLEANUP_APPLIED_SALE_CANCELLED;
			}
			state.mut().NFTs.set(input.NFTid, locals.nft);
		}

		locals.log = CleanupLogger{ QBAY_CONTRACT_INDEX, LogInfo::cleanupSetURIEvent, input.NFTid, output.result, 0 };
		LOG_INFO(locals.log);
	}

	INITIALIZE()
	{
		state.mut().cfbIssuer = ID(_C, _F, _B, _M, _E, _M, _Z, _O, _I, _D, _E, _X, _Q, _A, _U, _X, _Y, _Y, _S, _Z, _I, _U, _R, _A, _D, _Q, _L, _A, _P, _W, _P, _M, _N, _J, _X, _Q, _S, _N, _V, _Q, _Z, _A, _H, _Y, _V, _O, _P, _Y, _U, _K, _K, _J, _B, _J, _U, _C);
		state.mut().marketPlaceOwner = ID(_R, _K, _D, _H, _C, _M, _R, _J, _Y, _C, _G, _K, _P, _D, _U, _Y, _R, _X, _G, _D, _Y, _Z, _C, _I, _Z, _I, _T, _A, _H, _Y, _O, _V, _G, _I, _U, _T, _K, _N, _D, _T, _E, _H, _P, _C, _C, _L, _W, _L, _Z, _X, _S, _H, _N, _F, _P, _D);
		state.mut().transferRightsFee = 1000000;

	}

	struct BEGIN_EPOCH_locals
	{
		_cleanupBurn_input burn;
		_cleanupBurn_output burnOutput;
		_cleanupCancelProposals_input cancelProposals;
		_cleanupCancelProposals_output cancelProposalsOutput;
		_cleanupSetURI_input setURI;
		_cleanupSetURI_output setURIOutput;
	};

	BEGIN_EPOCH_WITH_LOCALS()
	{
		state.mut().transferRightsFee = 100;

		// One-time duplicate NFT cleanup (QubicBay list, see QBAY_CLEANUP_APPLIED). duplicateCleanupDone is a new
		// (zero-padded) state field, so this runs exactly once: in the first epoch executed by this code.
		if (state.get().duplicateCleanupDone == 0)
		{
			state.mut().duplicateCleanupDone = 1;

			// burn (41): possessor := NULL_ID
			locals.burn.NFTid = 11; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 0 Qbros, QmUwdk6TQRTAR91KJjQPUHhP8KRcMyFbvTQGQTJAsffYR7
			locals.burn.NFTid = 12; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 0 Qbros, QmPL2DyvsKFYK1pM6wftGw5nwHZpTDbiTsXUJFte82FEZB
			locals.burn.NFTid = 32; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 0 Qbros, QmZHi3GzSNUXhpcjxGMtr58YNdPzsd2fm7KTVMRgnsoyuC
			locals.burn.NFTid = 2318; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreieghnd5ix2f2lh4c66sdeqtku3aodzxvdjbctqrmcywbiocr4jym4
			locals.burn.NFTid = 2459; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreigt5rdo7n5xw5k6tmdlihyiqmqpbp657pkqii2cyo2ispkoj4z5ra
			locals.burn.NFTid = 2500; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreibv3lqzih2szlzqmxter2lrkduc53c3x5mdj73gcqdmwc7zpjstei
			locals.burn.NFTid = 2505; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreicd2tgc6273ojuz2h67okksxnqk3atxw5esxsdt4uhklgn7uzkigi
			locals.burn.NFTid = 2507; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreicd2tgc6273ojuz2h67okksxnqk3atxw5esxsdt4uhklgn7uzkigi
			locals.burn.NFTid = 2508; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreicd2tgc6273ojuz2h67okksxnqk3atxw5esxsdt4uhklgn7uzkigi
			locals.burn.NFTid = 2510; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreicd2tgc6273ojuz2h67okksxnqk3atxw5esxsdt4uhklgn7uzkigi
			locals.burn.NFTid = 2538; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihwt6yyj2etcca4x553l57tdgk3mlzwqgatbk3wmdnuig4csfa2yq
			locals.burn.NFTid = 2549; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiarue7iiipmgpc7pebujredpt2bk7zrh4hqpq4fbvzaeczkhey57u
			locals.burn.NFTid = 2592; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreie5ejjcnplkqpm4ucz6mfvpa36stumlweg5o5ildj76jwuxnuwcmq
			locals.burn.NFTid = 2598; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreigvfriropaikxg3lar3xzhy2c5qzoeok3rup64air3jndk5axyvvm
			locals.burn.NFTid = 2600; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihcbfhbuk5lzdvawq2y3njg7gz4jrvtpo47e6eenqqzwx4euwegh4
			locals.burn.NFTid = 2613; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiab6mmrv3uw2hmplwzxcomoq3hlppdnzx3ffagbd2y3bin346enfu
			locals.burn.NFTid = 2624; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihtypgeqqbmev3tze77ws557i2qu4x5ovqq4zqw3zyyaewr6bzyba
			locals.burn.NFTid = 2625; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihtypgeqqbmev3tze77ws557i2qu4x5ovqq4zqw3zyyaewr6bzyba
			locals.burn.NFTid = 2628; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreigvmro6iiob2eyoml5twazslbs2b6hlb66pudkd25lapd7d7bd6he
			locals.burn.NFTid = 2629; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreigvmro6iiob2eyoml5twazslbs2b6hlb66pudkd25lapd7d7bd6he
			locals.burn.NFTid = 2705; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreibalzshepmg72bs3hzvqecwillowowapjfcb5dvcvbvuvjb55d5yq
			locals.burn.NFTid = 2706; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreibalzshepmg72bs3hzvqecwillowowapjfcb5dvcvbvuvjb55d5yq
			locals.burn.NFTid = 2715; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiaalw3yeqrcehbh26hd3ce5xqymwbukh2htph3c7tk7x6gey3kvza
			locals.burn.NFTid = 2716; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiaalw3yeqrcehbh26hd3ce5xqymwbukh2htph3c7tk7x6gey3kvza
			locals.burn.NFTid = 2718; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiclgb75afeoe3vtsoauptewgkpn3nfb2rsa7dcyi2hojdvofebiua
			locals.burn.NFTid = 2729; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreifgpjtnoan4o7ya2tjcuuxkfbyvyyxjaybs5lm2fhisn3kdpslmca
			locals.burn.NFTid = 2783; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreib72ksvgd4jfzilmbkyip5dzwqa47apuzuoezb5m2skqm4nhr6oma
			locals.burn.NFTid = 2793; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiarelizl6c4soxd7hzsxth3maknbke5qvnytceddcpdi3wbqwwroa
			locals.burn.NFTid = 2801; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihf77gwkiuj2yf7tis5lbxlsbg5tycitebtzm3mldm64p7d7ncco4
			locals.burn.NFTid = 2831; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreievvwkhxcgxhnhybuelztnxtztec5ja5q7mttoz7plgbnxlrureru
			locals.burn.NFTid = 2834; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreigv535tgz3uxwer34a226kg4npyrlb4enhlxopsskcsxypj3mvmi4
			locals.burn.NFTid = 2863; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreibddkpu6i7atuw37owpjzrjlq4kwlarejhpyuiepc32ydw6qzxxnq
			locals.burn.NFTid = 2878; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiakmbdmk4cbaog4mcdfcla7qdl7kgx7q4z235kl7s732bwhc3zoii
			locals.burn.NFTid = 2884; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreiebakukrkafzlnzv2sxhrap4h34llealfei3qquhwn6ueadurhdli
			locals.burn.NFTid = 2892; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreib2lmhvgeersmsttmpzfm6q6cahr4wtmctsmn7i6mv2pje3qatunu
			locals.burn.NFTid = 2951; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreic6d5eoxgkx65sxjegraqhocw5mchwstcj4ilaoz2sow26arbvjo4
			locals.burn.NFTid = 2955; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihf7ktg2cp375bkirx7xxvf5v37efi3rxcdta3w7ynzvj26hmzwqm
			locals.burn.NFTid = 2957; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihf7ktg2cp375bkirx7xxvf5v37efi3rxcdta3w7ynzvj26hmzwqm
			locals.burn.NFTid = 2987; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreicoye6lazruo53ux6agrsdzzv5gqdznbk2gcycly4gm4xawdsj4lq
			locals.burn.NFTid = 2992; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihykkfmx2kuqooxjkstlfmfso45yff5uk6lsby335uqeto4tz5v5a
			locals.burn.NFTid = 3033; CALL(_cleanupBurn, locals.burn, locals.burnOutput);	// 5 COINS, bafkreihgp6l6yj4ggnzsud3tvb5ezhlut5uqfu3jwp2ggh7itfzdidfn6q
			CALL(_cleanupCancelProposals, locals.cancelProposals, locals.cancelProposalsOutput);

			// set URI (52): NFTid, 0, expected current URI (8 little-endian words), new URI (8 words)
			// 58, 0 Qbros: QmZDWEAfu3idRbHMxKZ2tWkfjyq1wVh4ZeESDJAnjd2RQN -> QmVxrTgtgFJDbv2rhbY5gfexA8FhVmGzHQ7VSEPzTtoy7c
			locals.setURI = _cleanupSetURI_input{ 58, 0, 0x66414557445a6d51ULL, 0x4d48625264693375ULL, 0x666b5774325a4b78ULL, 0x346856773171796aULL, 0x6e414a445345655aULL, 0x00004e515232646aULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x7467547278566d51ULL, 0x72327662444a4667ULL, 0x7865666735596268ULL, 0x7a476d5668463841ULL, 0x7a50455356375148ULL, 0x00006337796f7454ULL, 0x0000000000000000ULL, 0x0000000000000000ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1093, 1 Qub3s: bafkreidl2466vup54msnmoqs2pj5cwxyl7kznwfeidnbu7izngffntfxte -> bafkreiai7kh5736g5ww5fgm5ozpcie744tc3z2snhzhkd56fxivi6ouf64
			locals.setURI = _cleanupSetURI_input{ 1093, 0, 0x646965726b666162ULL, 0x707576363634326cULL, 0x716f6d6e736d3435ULL, 0x787763356a703273ULL, 0x66776e7a6b376c79ULL, 0x693775626e646965ULL, 0x66746e6666676e7aULL, 0x0000000000657478ULL, 0x616965726b666162ULL, 0x36333735686b3769ULL, 0x6d67663577773567ULL, 0x37656963707a6f35ULL, 0x73327a3363743434ULL, 0x3635646b687a686eULL, 0x756f366976697866ULL, 0x0000000000343666ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1351, 1 Qub3s: bafkreihprtknbui4eqmj3vmigg727guds3qvommtlmckb5wl5e4acfsyjq -> bafkreiftenmweonnuyuujw6evic3ftqxlwyihci6azawmg53oost66c6zm
			locals.setURI = _cleanupSetURI_input{ 1351, 0, 0x686965726b666162ULL, 0x6975626e6b747270ULL, 0x6d76336a6d716534ULL, 0x7567373237676769ULL, 0x6d6d6f7671337364ULL, 0x7735626b636d6c74ULL, 0x736663613465356cULL, 0x0000000000716a79ULL, 0x666965726b666162ULL, 0x6e6f65776d6e6574ULL, 0x36776a757579756eULL, 0x7174663363697665ULL, 0x6963686979776c78ULL, 0x35676d77617a6136ULL, 0x63363674736f6f33ULL, 0x00000000006d7a36ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1442, 2 Quties: bafkreignpafvhrv2ijkxs3jaecwugd4hjd34fycyrzpixsmyqxcqgcxqsm -> bafkreihf4hgad76u4opv2vb3xlw32ec3yv3smiukbuwzuyytn63mbw2yuu
			locals.setURI = _cleanupSetURI_input{ 1442, 0, 0x676965726b666162ULL, 0x767268766661706eULL, 0x6a3373786b6a6932ULL, 0x3464677577636561ULL, 0x6379663433646a68ULL, 0x6d737869707a7279ULL, 0x7863677163787179ULL, 0x00000000006d7371ULL, 0x686965726b666162ULL, 0x3637646167683466ULL, 0x62763276706f3475ULL, 0x63653233776c7833ULL, 0x75696d7333767933ULL, 0x7979757a7775626bULL, 0x3277626d33366e74ULL, 0x0000000000757579ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1613, 3 piQcells: bafkreias46nxhambq5neinwo34ercrnrff5pp32xm7en33lemhqm5z3c4u -> bafkreicyxoh4xzgar6axbg2vffrkuychxtwtt4wnxu6pymrnk3feuzm2um
			locals.setURI = _cleanupSetURI_input{ 1613, 0, 0x616965726b666162ULL, 0x6d6168786e363473ULL, 0x776e69656e357162ULL, 0x6e7263726534336fULL, 0x3233707035666672ULL, 0x6c33336e65376d78ULL, 0x337a356d71686d65ULL, 0x0000000000753463ULL, 0x636965726b666162ULL, 0x677a7834686f7879ULL, 0x3267627861367261ULL, 0x6379756b72666676ULL, 0x7734747477747868ULL, 0x726d79703675786eULL, 0x6d7a756566336b6eULL, 0x00000000006d7532ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1662, 0 Qbros: QmRM9bwhcQJXjCRqt4fR6zhVFN9k39vBb751A7p84Qt8Z2 -> QmRS8RrhSH7LqME12UvfVkDg5ctqEoLuGcPJfFPwz7k5BD
			locals.setURI = _cleanupSetURI_input{ 1662, 0, 0x687762394d526d51ULL, 0x7152436a584a5163ULL, 0x56687a3652663474ULL, 0x427639336b394e46ULL, 0x3870374131353762ULL, 0x0000325a38745134ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x6872523853526d51ULL, 0x31454d714c374853ULL, 0x67446b5666765532ULL, 0x754c6f4571746335ULL, 0x775046664a506347ULL, 0x00004442356b377aULL, 0x0000000000000000ULL, 0x0000000000000000ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1664, 2 Quties: bafkreibzql7qzmv7wdidp75ivmuehefey3vzya2isnwx26crlxdu5pably -> bafkreickvks6wqe63j57ddhugg22nzihjgriorbrecchtfntxfjq7d6iq4
			locals.setURI = _cleanupSetURI_input{ 1664, 0, 0x626965726b666162ULL, 0x766d7a71376c717aULL, 0x3537706469647737ULL, 0x66656865756d7669ULL, 0x3261797a76337965ULL, 0x63363278776e7369ULL, 0x6170357564786c72ULL, 0x0000000000796c62ULL, 0x636965726b666162ULL, 0x65717736736b766bULL, 0x68646437356a3336ULL, 0x697a6e3232676775ULL, 0x62726f6972676a68ULL, 0x6e66746863636572ULL, 0x366437716a667874ULL, 0x0000000000347169ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1678, 4 Garth: The Source Code: bafkreicw433z4tzf6jdy5e5jrxydsiz5ak6rg4jf5y4eg4ss5ghtguwdqa -> bafkreibsglcwfko6ydmw6lqgiucyjvkfk7kwlhekkyal73qllldndpzgu4
			locals.setURI = _cleanupSetURI_input{ 1678, 0, 0x636965726b666162ULL, 0x7a74347a33333477ULL, 0x35653579646a3666ULL, 0x7a6973647978726aULL, 0x6a346772366b6135ULL, 0x7334676534793566ULL, 0x7775677468673573ULL, 0x0000000000617164ULL, 0x626965726b666162ULL, 0x6f6b6677636c6773ULL, 0x716c36776d647936ULL, 0x6b766a7963756967ULL, 0x65686c776b376b66ULL, 0x7133376c61796b6bULL, 0x7a70646e646c6c6cULL, 0x0000000000347567ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1698, 4 Garth: The Source Code: bafkreidkbrb5zbvhc2xogyzonf6g6dkx4x7lgeh2qqlqw665ytfhfusuee -> bafkreiexlq5tsrldo266oryoskw5xgstm62ietx4h7loiha3b7su2a7knu
			locals.setURI = _cleanupSetURI_input{ 1698, 0, 0x646965726b666162ULL, 0x76627a356272626bULL, 0x7a79676f78326368ULL, 0x6b64366736666e6fULL, 0x6865676c37783478ULL, 0x363677716c717132ULL, 0x7375666866747935ULL, 0x0000000000656575ULL, 0x656965726b666162ULL, 0x6c72737435716c78ULL, 0x79726f3636326f64ULL, 0x73677835776b736fULL, 0x7874656932366d74ULL, 0x6168696f6c376834ULL, 0x3761327573376233ULL, 0x0000000000756e6bULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1722, 4 Garth: The Source Code: bafkreibjex3fuotpewn2pwmieqea3vpb3nfkongmrro4t5jr6gfilvtpni -> bafkreicgzjkvqkhffasxmozj2p63gkmds5biwlaqkoruiwzvnjdh3uz3oq
			locals.setURI = _cleanupSetURI_input{ 1722, 0, 0x626965726b666162ULL, 0x746f75663378656aULL, 0x6d7770326e776570ULL, 0x7076336165716569ULL, 0x676e6f6b666e3362ULL, 0x6a3574346f72726dULL, 0x74766c6966673672ULL, 0x0000000000696e70ULL, 0x636965726b666162ULL, 0x686b71766b6a7a67ULL, 0x7a6f6d7873616666ULL, 0x6d6b67333670326aULL, 0x616c776962357364ULL, 0x7a776975726f6b71ULL, 0x7a753368646a6e76ULL, 0x0000000000716f33ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1725, 4 Garth: The Source Code: bafkreibjex3fuotpewn2pwmieqea3vpb3nfkongmrro4t5jr6gfilvtpni -> bafkreigh7mslnx4foyt5geufw2jitechbo66xasmlmysl4gns6auoiv7we
			locals.setURI = _cleanupSetURI_input{ 1725, 0, 0x626965726b666162ULL, 0x746f75663378656aULL, 0x6d7770326e776570ULL, 0x7076336165716569ULL, 0x676e6f6b666e3362ULL, 0x6a3574346f72726dULL, 0x74766c6966673672ULL, 0x0000000000696e70ULL, 0x676965726b666162ULL, 0x34786e6c736d3768ULL, 0x7565673574796f66ULL, 0x636574696a327766ULL, 0x73617836366f6268ULL, 0x67346c73796d6c6dULL, 0x76696f756136736eULL, 0x0000000000657737ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1750, 4 Garth: The Source Code: bafkreihe22y47bciyaaxbsvk4gkhqotljbfq53bxm5erllf345j7nahty4 -> bafkreiech5g3karq2qrhkbg33yzqguk4wq3wyx65ugfu66kyjrau43qnye
			locals.setURI = _cleanupSetURI_input{ 1750, 0, 0x686965726b666162ULL, 0x6362373479323265ULL, 0x7673627861617969ULL, 0x746f71686b67346bULL, 0x6233357166626a6cULL, 0x666c6c7265356d78ULL, 0x68616e376a353433ULL, 0x0000000000347974ULL, 0x656965726b666162ULL, 0x72616b3367356863ULL, 0x67626b6872713271ULL, 0x6b7567717a793333ULL, 0x3678797733717734ULL, 0x6b36367566677535ULL, 0x7133347561726a79ULL, 0x000000000065796eULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1760, 4 Garth: The Source Code: bafkreicxvfijqdch7qsvypgio2m33zjdlyrvtyciywd2cv3ck6mrrjnvte -> bafkreigsmr6po7guejskymu7v5ny4d2qsqqxam3tm3elhchpzjy5ns3ic4
			locals.setURI = _cleanupSetURI_input{ 1760, 0, 0x636965726b666162ULL, 0x6364716a69667678ULL, 0x6770797673713768ULL, 0x6a7a33336d326f69ULL, 0x6379747672796c64ULL, 0x3376633264777969ULL, 0x6e6a72726d366b63ULL, 0x0000000000657476ULL, 0x676965726b666162ULL, 0x67376f7036726d73ULL, 0x756d796b736a6575ULL, 0x326434796e357637ULL, 0x336d617871717371ULL, 0x6863686c65336d74ULL, 0x33736e35796a7a70ULL, 0x0000000000346369ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1761, 4 Garth: The Source Code: bafkreieiksukzptzv6j6fm6uwjrjs7bmb3g3wdveld2r2s7gyanx26pdre -> bafkreib3exkaycw2wtjufwddbgmnqwxlkisthta7yxlzj2zsaq5eapt7gu
			locals.setURI = _cleanupSetURI_input{ 1761, 0, 0x656965726b666162ULL, 0x74707a6b75736b69ULL, 0x366d66366a36767aULL, 0x6237736a726a7775ULL, 0x766477336733626dULL, 0x3773327232646c65ULL, 0x703632786e617967ULL, 0x0000000000657264ULL, 0x626965726b666162ULL, 0x776379616b786533ULL, 0x647766756a747732ULL, 0x7877716e6d676264ULL, 0x6174687473696b6cULL, 0x7a326a7a6c787937ULL, 0x7470616535716173ULL, 0x0000000000756737ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1764, 4 Garth: The Source Code: bafkreickli7gyv5saoamtqjroyfizcyzanfpfc6iaqr6seopa5ffp3glaq -> bafkreieopzuziloacqjr6op2mg6o3csjtynlizvjlzeq7o4pozhfpxuoku
			locals.setURI = _cleanupSetURI_input{ 1764, 0, 0x636965726b666162ULL, 0x3576796737696c6bULL, 0x6a71746d616f6173ULL, 0x79637a6966796f72ULL, 0x36636670666e617aULL, 0x6f65733672716169ULL, 0x6733706666356170ULL, 0x000000000071616cULL, 0x656965726b666162ULL, 0x6f6c697a757a706fULL, 0x706f36726a716361ULL, 0x7363336f36676d32ULL, 0x767a696c6e79746aULL, 0x346f3771657a6c6aULL, 0x75787066687a6f70ULL, 0x0000000000756b6fULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1769, 4 Garth: The Source Code: bafkreihbd4ri4xklu2no3x5jvqjugraqs5yb2hnkkiohdy53jkpe3hrspq -> bafkreia6llycqbc34rykmwfqj2ttugmnukyi6s4wwxw34wrzouoj5ba4n4
			locals.setURI = _cleanupSetURI_input{ 1769, 0, 0x686965726b666162ULL, 0x6b78346972346462ULL, 0x3578336f6e32756cULL, 0x617267756a71766aULL, 0x6e68326279357371ULL, 0x357964686f696b6bULL, 0x72683365706b6a33ULL, 0x0000000000717073ULL, 0x616965726b666162ULL, 0x63627163796c6c36ULL, 0x66776d6b79723433ULL, 0x6d67757474326a71ULL, 0x34733669796b756eULL, 0x7277343377787777ULL, 0x6162356a6f756f7aULL, 0x0000000000346e34ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1770, 4 Garth: The Source Code: bafkreih3ig6r4gr4mzcn7g27e4xe7kqid34mwcq546ik74anzsktiy57y4 -> bafkreiff5g2pb2c6vpj7a2nimfsulrpb2iqcdtmmlvne7433mnuibgu5vu
			locals.setURI = _cleanupSetURI_input{ 1770, 0, 0x686965726b666162ULL, 0x7267347236676933ULL, 0x3267376e637a6d34ULL, 0x716b376578346537ULL, 0x7163776d34336469ULL, 0x6134376b69363435ULL, 0x357969746b737a6eULL, 0x0000000000347937ULL, 0x666965726b666162ULL, 0x6332627032673566ULL, 0x6e3261376a707636ULL, 0x70726c7573666d69ULL, 0x6d74646371693262ULL, 0x333437656e766c6dULL, 0x75676269756e6d33ULL, 0x0000000000757635ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1772, 4 Garth: The Source Code: bafkreiaritvmtwwzfcadk72x7dh32stsfgbmnmq73aj5d7hyiwpijzekhm -> bafkreietxcafi6v6c4tnf27nvrwkbbceb3yfg6axb6sdsbt2ovrwif23k4
			locals.setURI = _cleanupSetURI_input{ 1772, 0, 0x616965726b666162ULL, 0x7777746d76746972ULL, 0x32376b646163667aULL, 0x7473323368643778ULL, 0x716d6e6d62676673ULL, 0x683764356a613337ULL, 0x657a6a6970776979ULL, 0x00000000006d686bULL, 0x656965726b666162ULL, 0x7636696661637874ULL, 0x3732666e74346336ULL, 0x6362626b7772766eULL, 0x6136676679336265ULL, 0x7462736473366278ULL, 0x3266697772766f32ULL, 0x0000000000346b33ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1775, 4 Garth: The Source Code: bafkreia5pr6sf5mxgl2emmcsdvdvfcsmbfxbj6wjbphenvyswy6f7sktwm -> bafkreieqs5vlqc4m6hemrws6qanqg24tnivb5tp7tf3rv2uh4k5lnugt4e
			locals.setURI = _cleanupSetURI_input{ 1775, 0, 0x616965726b666162ULL, 0x6d35667336727035ULL, 0x636d6d65326c6778ULL, 0x7363667664766473ULL, 0x77366a627866626dULL, 0x79766e656870626aULL, 0x6b73376636797773ULL, 0x00000000006d7774ULL, 0x656965726b666162ULL, 0x3463716c76357371ULL, 0x7377726d6568366dULL, 0x343267716e617136ULL, 0x7074356276696e74ULL, 0x7532767233667437ULL, 0x67756e6c356b3468ULL, 0x0000000000653474ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1780, 4 Garth: The Source Code: bafkreiewsds4v6gself2ofelecnvxczgn4l7atrkclinrwjaxkbxwo4kma -> bafkreiah7eadg4cuoz4b4jixrc2ljhxkif35k5to2isd37twvrxe2yqi7q
			locals.setURI = _cleanupSetURI_input{ 1780, 0, 0x656965726b666162ULL, 0x6736763473647377ULL, 0x65666f32666c6573ULL, 0x7a6378766e63656cULL, 0x727461376c346e67ULL, 0x6a77726e696c636bULL, 0x346f7778626b7861ULL, 0x0000000000616d6bULL, 0x616965726b666162ULL, 0x6334676461653768ULL, 0x696a3462347a6f75ULL, 0x78686a6c32637278ULL, 0x74356b353366696bULL, 0x743733647369326fULL, 0x7179326578727677ULL, 0x0000000000713769ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1781, 4 Garth: The Source Code: bafkreidv5wtd64o5b6ns3zofwfekqdw5xv6nku6a4p4zrfkvhsqfzwxlcy -> bafkreie63525hr5rht23gukr4p76bat3nr5ebt7pbofkkufhaitxuh3zka
			locals.setURI = _cleanupSetURI_input{ 1781, 0, 0x646965726b666162ULL, 0x6f34366474773576ULL, 0x6f7a33736e366235ULL, 0x7764716b65667766ULL, 0x36756b6e36767835ULL, 0x6b66727a34703461ULL, 0x78777a6671736876ULL, 0x000000000079636cULL, 0x656965726b666162ULL, 0x3572683532353336ULL, 0x6b75673332746872ULL, 0x7461623637703472ULL, 0x3774626535726e33ULL, 0x66756b6b666f6270ULL, 0x3368757874696168ULL, 0x0000000000616b7aULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1785, 4 Garth: The Source Code: bafkreicngz6u6jchxzvzocmcr52el2yrkbdypw5vvky6qi5uyozcoinsi4 -> bafkreidvo2cpz5m5i6mhabkkhnprwldvhujdo4k3zy5htqtsgxljh5xfdy
			locals.setURI = _cleanupSetURI_input{ 1785, 0, 0x636965726b666162ULL, 0x636a3675367a676eULL, 0x6d636f7a767a7868ULL, 0x79326c6532357263ULL, 0x3577707964626b72ULL, 0x35697136796b7676ULL, 0x6e696f637a6f7975ULL, 0x0000000000346973ULL, 0x646965726b666162ULL, 0x6d357a7063326f76ULL, 0x6b6261686d366935ULL, 0x646c7772706e686bULL, 0x6b346f646a756876ULL, 0x7471746835797a33ULL, 0x7835686a6c786773ULL, 0x0000000000796466ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1786, 4 Garth: The Source Code: bafkreibbe37bwicydqdcml4mlcfd2l67eeigg7ycikwnm6v72fjn54vkaa -> bafkreigpe36xwtljsmsnnwjy4p7y5bbiyfnhpspyj4dhg3gql35ywgqr74
			locals.setURI = _cleanupSetURI_input{ 1786, 0, 0x626965726b666162ULL, 0x6369776237336562ULL, 0x346c6d6364716479ULL, 0x366c326466636c6dULL, 0x7937676769656537ULL, 0x76366d6e776b6963ULL, 0x7634356e6a663237ULL, 0x000000000061616bULL, 0x676965726b666162ULL, 0x6c74777836336570ULL, 0x6a776e6e736d736aULL, 0x6262357937703479ULL, 0x707370686e667969ULL, 0x6733676864346a79ULL, 0x7167777935336c71ULL, 0x0000000000343772ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1788, 4 Garth: The Source Code: bafkreiedeczj3vxa2y6xuvw3dwn6v24tbmr62lan237db5lzjeuf3ec4le -> bafkreifcd3blrsgyjd2f6np6kuonfy3qhsjnhvrcof75rfkayx22dt763y
			locals.setURI = _cleanupSetURI_input{ 1788, 0, 0x656965726b666162ULL, 0x7876336a7a636564ULL, 0x7776757836793261ULL, 0x343276366e776433ULL, 0x616c3236726d6274ULL, 0x6c3562643733326eULL, 0x6365336675656a7aULL, 0x0000000000656c34ULL, 0x666965726b666162ULL, 0x6773726c62336463ULL, 0x706e366632646a79ULL, 0x3379666e6f756b36ULL, 0x7276686e6a736871ULL, 0x6b66723537666f63ULL, 0x3774643232787961ULL, 0x0000000000793336ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1793, 4 Garth: The Source Code: bafkreih3ig6r4gr4mzcn7g27e4xe7kqid34mwcq546ik74anzsktiy57y4 -> bafkreibfg4lpailwg4xm7xaeqgrypvqqnssm7thz3b5noqhimwkdc22zfe
			locals.setURI = _cleanupSetURI_input{ 1793, 0, 0x686965726b666162ULL, 0x7267347236676933ULL, 0x3267376e637a6d34ULL, 0x716b376578346537ULL, 0x7163776d34336469ULL, 0x6134376b69363435ULL, 0x357969746b737a6eULL, 0x0000000000347937ULL, 0x626965726b666162ULL, 0x6c6961706c346766ULL, 0x6178376d78346777ULL, 0x7176707972677165ULL, 0x6874376d73736e71ULL, 0x68716f6e3562337aULL, 0x323263646b776d69ULL, 0x000000000065667aULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1794, 4 Garth: The Source Code: bafkreib5syahs2bfj3h2zcnoo3nosd4cqddl3fkojtxvwmcetiwx4bwhc4 -> bafkreidkugh447f52nlwvz257kvt5csu5lsyv472odibt4mkh4r34d2cde
			locals.setURI = _cleanupSetURI_input{ 1794, 0, 0x626965726b666162ULL, 0x6232736861797335ULL, 0x6e637a3268336a66ULL, 0x3464736f6e336f6fULL, 0x6b66336c64647163ULL, 0x636d777678746a6fULL, 0x7762347877697465ULL, 0x0000000000346368ULL, 0x646965726b666162ULL, 0x663734346867756bULL, 0x327a76776c6e3235ULL, 0x73633574766b3735ULL, 0x37347679736c3575ULL, 0x6d34746269646f32ULL, 0x326434337234686bULL, 0x0000000000656463ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1800, 4 Garth: The Source Code: bafkreidugz3jlvzhvnpbkbmleeb2ye7bvl4msxr7fwvtptx3ga5qze4rxm -> bafkreiawm2bd6forf4btbcd5zjwmicjmi6fzmxqwxh7u5gbc7h6dgw3ytq
			locals.setURI = _cleanupSetURI_input{ 1800, 0, 0x646965726b666162ULL, 0x7a766c6a337a6775ULL, 0x6d626b62706e7668ULL, 0x376579326265656cULL, 0x7278736d346c7662ULL, 0x7874707476776637ULL, 0x34657a7135616733ULL, 0x00000000006d7872ULL, 0x616965726b666162ULL, 0x6f66366462326d77ULL, 0x6463627462346672ULL, 0x6a63696d776a7a35ULL, 0x71786d7a6636696dULL, 0x6267357537687877ULL, 0x3377676436683763ULL, 0x0000000000717479ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1802, 4 Garth: The Source Code: bafkreianabzvvfgf5kwxvbtl2vogmz72kmfe3x6yeqioffkrtjago2vqk4 -> bafkreic7nhfblmsrz6jvij55iqanvfi2nb52j5u2qr7ytgbl6nqg3tsney
			locals.setURI = _cleanupSetURI_input{ 1802, 0, 0x616965726b666162ULL, 0x676676767a62616eULL, 0x74627678776b3566ULL, 0x377a6d676f76326cULL, 0x36783365666d6b32ULL, 0x6b66666f69716579ULL, 0x76326f67616a7472ULL, 0x0000000000346b71ULL, 0x636965726b666162ULL, 0x736d6c6266686e37ULL, 0x356a69766a367a72ULL, 0x6966766e61716935ULL, 0x75356a3235626e32ULL, 0x6267747937727132ULL, 0x73743367716e366cULL, 0x000000000079656eULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1810, 4 Garth: The Source Code: bafkreifgvmhmtxh5yzndu3qhooffzgakxtzs55e5rndvyw66xai5at2x6m -> bafkreifr7xrip7uhqabwc3tfvk7bloxzxtylrcgalamjokiuebyrjgd5me
			locals.setURI = _cleanupSetURI_input{ 1810, 0, 0x666965726b666162ULL, 0x6878746d686d7667ULL, 0x713375646e7a7935ULL, 0x61677a66666f6f68ULL, 0x653535737a74786bULL, 0x36777976646e7235ULL, 0x3274613569617836ULL, 0x00000000006d3678ULL, 0x666965726b666162ULL, 0x7537706972783772ULL, 0x7433637762617168ULL, 0x786f6c62376b7666ULL, 0x6763726c7974787aULL, 0x696b6f6a6d616c61ULL, 0x64676a7279626575ULL, 0x0000000000656d35ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1811, 4 Garth: The Source Code: bafkreih2mvnqkwnitrk5ueaxp5h7sm6f7svla7pn5gozavnk2oollspmz4 -> bafkreibn7tu24z6jvdhgfq5td37rtz4v5cv7jj2s5vrg46l6rdc4zuxzuu
			locals.setURI = _cleanupSetURI_input{ 1811, 0, 0x686965726b666162ULL, 0x6e776b716e766d32ULL, 0x616575356b727469ULL, 0x366d733768357078ULL, 0x7037616c76733766ULL, 0x6e76617a6f67356eULL, 0x70736c6c6f6f326bULL, 0x0000000000347a6dULL, 0x626965726b666162ULL, 0x367a34327574376eULL, 0x357166676864766aULL, 0x347a747237336474ULL, 0x326a6a3776633576ULL, 0x6c36346772763573ULL, 0x78757a3463647236ULL, 0x000000000075757aULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1812, 4 Garth: The Source Code: bafkreidugz3jlvzhvnpbkbmleeb2ye7bvl4msxr7fwvtptx3ga5qze4rxm -> bafkreiby35jizdcytbjg4rdw3u2xccbwryuoavkshtd3ijvieuiwqn4ome
			locals.setURI = _cleanupSetURI_input{ 1812, 0, 0x646965726b666162ULL, 0x7a766c6a337a6775ULL, 0x6d626b62706e7668ULL, 0x376579326265656cULL, 0x7278736d346c7662ULL, 0x7874707476776637ULL, 0x34657a7135616733ULL, 0x00000000006d7872ULL, 0x626965726b666162ULL, 0x63647a696a353379ULL, 0x647234676a627479ULL, 0x6263637832753377ULL, 0x6b76616f75797277ULL, 0x766a693364746873ULL, 0x346e717769756569ULL, 0x0000000000656d6fULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1814, 4 Garth: The Source Code: bafkreifn5uzs6rbkigrip36ba5d3ykgadlgorupcstvmqpaujm2y2dpgfq -> bafkreighxgvoys2sqxhjzmqu27ley57a2frufp7xyswhg25l3dmkqp554q
			locals.setURI = _cleanupSetURI_input{ 1814, 0, 0x666965726b666162ULL, 0x627236737a75356eULL, 0x363370697267696bULL, 0x676b793364356162ULL, 0x7075726f676c6461ULL, 0x6170716d76747363ULL, 0x70643279326d6a75ULL, 0x0000000000716667ULL, 0x676965726b666162ULL, 0x3273796f76677868ULL, 0x716d7a6a68787173ULL, 0x373579656c373275ULL, 0x3770667572663261ULL, 0x3532676877737978ULL, 0x3570716b6d64336cULL, 0x0000000000713435ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1815, 4 Garth: The Source Code: bafkreigpjvmfsclylqx7lkhxqgrtufhkoyv53kanekb5cc6ztb7qusqjuu -> bafkreieh6ezx6nuqwsp7k5hhu4uhzfbekgwombmyb476bwta77dnzcjkne
			locals.setURI = _cleanupSetURI_input{ 1815, 0, 0x676965726b666162ULL, 0x6c6373666d766a70ULL, 0x686b6c3778716c79ULL, 0x6866757472677178ULL, 0x616b333576796f6bULL, 0x36636335626b656eULL, 0x717375713762747aULL, 0x000000000075756aULL, 0x656965726b666162ULL, 0x756e36787a653668ULL, 0x68356b3770737771ULL, 0x62667a6875347568ULL, 0x6d626d6f77676b65ULL, 0x7477623637346279ULL, 0x6a637a6e64373761ULL, 0x0000000000656e6bULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1821, 4 Garth: The Source Code: bafkreigbrnjtdceedgb6px2uhh7uqis7cejyynxxrtrpvjlxncfr7q4ude -> bafkreigqamnz5a4tlmqwskxbvgk3hgomb4zk6bucorgjp7tkxwpeffoko4
			locals.setURI = _cleanupSetURI_input{ 1821, 0, 0x676965726b666162ULL, 0x656364746a6e7262ULL, 0x3278703662676465ULL, 0x7369717537686875ULL, 0x786e79796a656337ULL, 0x6c6a767072747278ULL, 0x3471377266636e78ULL, 0x0000000000656475ULL, 0x676965726b666162ULL, 0x3461357a6e6d6171ULL, 0x786b7377716d6c74ULL, 0x6f6768336b677662ULL, 0x7562366b7a34626dULL, 0x7437706a67726f63ULL, 0x6f6666657077786bULL, 0x0000000000346f6bULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1822, 4 Garth: The Source Code: bafkreicxvfijqdch7qsvypgio2m33zjdlyrvtyciywd2cv3ck6mrrjnvte -> bafkreic4an4vdjydjdwnquoag4ayk6ddjcswrewquofjhjkahbevslspjy
			locals.setURI = _cleanupSetURI_input{ 1822, 0, 0x636965726b666162ULL, 0x6364716a69667678ULL, 0x6770797673713768ULL, 0x6a7a33336d326f69ULL, 0x6379747672796c64ULL, 0x3376633264777969ULL, 0x6e6a72726d366b63ULL, 0x0000000000657476ULL, 0x636965726b666162ULL, 0x796a6476346e6134ULL, 0x6f75716e77646a64ULL, 0x64366b7961346761ULL, 0x7765727773636a64ULL, 0x6b6a686a666f7571ULL, 0x736c737665626861ULL, 0x0000000000796a70ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1823, 4 Garth: The Source Code: bafkreihe22y47bciyaaxbsvk4gkhqotljbfq53bxm5erllf345j7nahty4 -> bafkreih5zgh7vxsfgjyxyfs3pcndcyjali22ekkwrhhfypkjw2fdsocplm
			locals.setURI = _cleanupSetURI_input{ 1823, 0, 0x686965726b666162ULL, 0x6362373479323265ULL, 0x7673627861617969ULL, 0x746f71686b67346bULL, 0x6233357166626a6cULL, 0x666c6c7265356d78ULL, 0x68616e376a353433ULL, 0x0000000000347974ULL, 0x686965726b666162ULL, 0x7378763768677a35ULL, 0x73667978796a6766ULL, 0x6a7963646e637033ULL, 0x6b6b653232696c61ULL, 0x6b70796668687277ULL, 0x636f73646632776aULL, 0x00000000006d6c70ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1825, 4 Garth: The Source Code: bafkreiauonh4sdgolgw4mizrh4fnmqvaa7ztgxzmgdkwuwlyz6m3hhvo7a -> bafkreie6gwo5dbv35aj7stmvtcjtx5cl7px4jyc3iw3onpenuanzff5f5e
			locals.setURI = _cleanupSetURI_input{ 1825, 0, 0x616965726b666162ULL, 0x67647334686e6f75ULL, 0x7a696d3477676c6fULL, 0x76716d6e66346872ULL, 0x7a7867747a376161ULL, 0x6c7775776b64676dULL, 0x766868336d367a79ULL, 0x000000000061376fULL, 0x656965726b666162ULL, 0x766264356f776736ULL, 0x6d7473376a613533ULL, 0x633578746a637476ULL, 0x63796a347870376cULL, 0x65706e6f33776933ULL, 0x3566667a6e61756eULL, 0x0000000000653566ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1832, 4 Garth: The Source Code: bafkreibg4wwza3f4kfqpvr4maojx66memfy4bp5pjfm3aqlyhk35g6q3hq -> bafkreicawsdkjnpd4pdvzfcgpytonbqybf7meikmxlwybkr7npowuey2ui
			locals.setURI = _cleanupSetURI_input{ 1832, 0, 0x626965726b666162ULL, 0x6633617a77773467ULL, 0x3472767071666b34ULL, 0x6d3636786a6f616dULL, 0x3570623479666d65ULL, 0x6c7161336d666a70ULL, 0x71366735336b6879ULL, 0x0000000000716833ULL, 0x636965726b666162ULL, 0x706e6a6b64737761ULL, 0x63667a7664703464ULL, 0x71626e6f74797067ULL, 0x6b69656d37666279ULL, 0x726b6279776c786dULL, 0x796575776f706e37ULL, 0x0000000000697532ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1840, 4 Garth: The Source Code: bafkreienrqaobbzr4uo3wezfeyi6ul36qmlh6fb5ibinc56d5223eoxq6y -> bafkreihb3dyp2gauim5nlolhcgavsqceugzcqpsmzos23udfljsvytjm4i
			locals.setURI = _cleanupSetURI_input{ 1840, 0, 0x656965726b666162ULL, 0x7a62626f6171726eULL, 0x7a6577336f753472ULL, 0x336c753669796566ULL, 0x626636686c6d7136ULL, 0x3635636e69626935ULL, 0x786f653332323564ULL, 0x0000000000793671ULL, 0x686965726b666162ULL, 0x6167327079643362ULL, 0x6c6f6c6e356d6975ULL, 0x6371737661676368ULL, 0x737071637a677565ULL, 0x64753332736f7a6dULL, 0x6a747976736a6c66ULL, 0x000000000069346dULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1843, 4 Garth: The Source Code: bafkreif7dfzwa7ludjctgujt5dmlvhgjronbgdgfqiuo37owtswtaoqybu -> bafkreiccwnwr2robkkzqr6vqikuc7psg5ky4qg4wjr4uknbqerr6smvbne
			locals.setURI = _cleanupSetURI_input{ 1843, 0, 0x666965726b666162ULL, 0x6c3761777a666437ULL, 0x6a756774636a6475ULL, 0x6768766c6d643574ULL, 0x676467626e6f726aULL, 0x6f37336f75697166ULL, 0x716f617477737477ULL, 0x0000000000756279ULL, 0x636965726b666162ULL, 0x6f723272776e7763ULL, 0x763672717a6b6b62ULL, 0x73703763756b6971ULL, 0x34677134796b3567ULL, 0x626e6b7534726a77ULL, 0x766d733672726571ULL, 0x0000000000656e62ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1864, 4 Garth: The Source Code: bafkreigto7kk4qftzb2i3magubj42bpnzrvygo3sdptojafdvd4fmnqwbu -> bafkreifbqgouhpwor2kkb6ovg5qcbjmf3bj2j4i7qxyqrzub3xusobs2ee
			locals.setURI = _cleanupSetURI_input{ 1864, 0, 0x676965726b666162ULL, 0x6671346b6b376f74ULL, 0x616d336932627a74ULL, 0x706232346a627567ULL, 0x336f677976727a6eULL, 0x66616a6f74706473ULL, 0x716e6d6634647664ULL, 0x0000000000756277ULL, 0x666965726b666162ULL, 0x777068756f677162ULL, 0x6f36626b6b32726fULL, 0x6d6a626371356776ULL, 0x69346a326a623366ULL, 0x757a727179787137ULL, 0x73626f7375783362ULL, 0x0000000000656532ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1872, 4 Garth: The Source Code: bafkreiewsds4v6gself2ofelecnvxczgn4l7atrkclinrwjaxkbxwo4kma -> bafkreifz6bo44fattmt5psewcn5lf7j32fwwm4c2pgebkxc33zrc6wltxu
			locals.setURI = _cleanupSetURI_input{ 1872, 0, 0x656965726b666162ULL, 0x6736763473647377ULL, 0x65666f32666c6573ULL, 0x7a6378766e63656cULL, 0x727461376c346e67ULL, 0x6a77726e696c636bULL, 0x346f7778626b7861ULL, 0x0000000000616d6bULL, 0x666965726b666162ULL, 0x616634346f62367aULL, 0x65737035746d7474ULL, 0x6a37666c356e6377ULL, 0x63346d7777663233ULL, 0x63786b6265677032ULL, 0x6c773663727a3333ULL, 0x0000000000757874ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1878, 4 Garth: The Source Code: bafkreidbuoddcb35uifmvgkdkxz3jclyx47y3p2wy4jhp3knbubln6gf2a -> bafkreifpss746fvjj5wp422hbu35yuyaehtveze27dwwtgtfn67577grwq
			locals.setURI = _cleanupSetURI_input{ 1878, 0, 0x646965726b666162ULL, 0x33626364646f7562ULL, 0x6b67766d66697535ULL, 0x6c636a337a786b64ULL, 0x3270337937347879ULL, 0x6b3370686a347977ULL, 0x67366e6c6275626eULL, 0x0000000000613266ULL, 0x666965726b666162ULL, 0x7666363437737370ULL, 0x3232347077356a6aULL, 0x7975793533756268ULL, 0x657a657674686561ULL, 0x7467747777643732ULL, 0x6737373537366e66ULL, 0x0000000000717772ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1880, 4 Garth: The Source Code: bafkreicaactn33hnyuhyb6ur6qt3zyep7tgsvc2zn6ppgvlwdhzjro3ax4 -> bafkreihesr3qxne24rdexgpinwvanbwxxqch5uk7om6nmyv6grditphkn4
			locals.setURI = _cleanupSetURI_input{ 1880, 0, 0x636965726b666162ULL, 0x6833336e74636161ULL, 0x753662796875796eULL, 0x65797a3374713672ULL, 0x3263767367743770ULL, 0x6c76677070366e7aULL, 0x336f726a7a686477ULL, 0x0000000000347861ULL, 0x686965726b666162ULL, 0x656e787133727365ULL, 0x7067786564723432ULL, 0x77626e6176776e69ULL, 0x6b75356863717878ULL, 0x76796d6e366d6f37ULL, 0x6870746964726736ULL, 0x0000000000346e6bULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1884, 4 Garth: The Source Code: bafkreif3uoumb5wykywvjwywcd4tpw3sysmupsldobyu4sfn2qdwbcoug4 -> bafkreic2mlej432ewmvexu3yg3ogbpuyfsit6fagu5zwqjkdvzkamshcvu
			locals.setURI = _cleanupSetURI_input{ 1884, 0, 0x666965726b666162ULL, 0x7735626d756f7533ULL, 0x79776a7677796b79ULL, 0x3377707434646377ULL, 0x6c7370756d737973ULL, 0x6673347579626f64ULL, 0x6f6362776471326eULL, 0x0000000000346775ULL, 0x636965726b666162ULL, 0x3233346a656c6d32ULL, 0x33757865766d7765ULL, 0x757062676f336779ULL, 0x6166367469736679ULL, 0x6b6a71777a357567ULL, 0x68736d616b7a7664ULL, 0x0000000000757663ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1887, 4 Garth: The Source Code: bafkreiewsds4v6gself2ofelecnvxczgn4l7atrkclinrwjaxkbxwo4kma -> bafkreifwud4va6qquqyjwklqfxh4ulydsjix5yepargxcpulj6pahxrt4a
			locals.setURI = _cleanupSetURI_input{ 1887, 0, 0x656965726b666162ULL, 0x6736763473647377ULL, 0x65666f32666c6573ULL, 0x7a6378766e63656cULL, 0x727461376c346e67ULL, 0x6a77726e696c636bULL, 0x346f7778626b7861ULL, 0x0000000000616d6bULL, 0x666965726b666162ULL, 0x7136617634647577ULL, 0x6c6b776a79717571ULL, 0x796c753468786671ULL, 0x65793578696a7364ULL, 0x7570637867726170ULL, 0x7278686170366a6cULL, 0x0000000000613474ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1904, 1 Qub3s: bafkreibvpuaysuy4ppfikzudb6pmj7gz2h4qo5d73dg35vfeclh2hejyh4 -> bafkreigbzkmkajw43bl4yo2w5mxz5sm3tng33p3hwpuzw7lxe4pvumf63m
			locals.setURI = _cleanupSetURI_input{ 1904, 0, 0x626965726b666162ULL, 0x7975737961757076ULL, 0x757a6b6966707034ULL, 0x67376a6d70366264ULL, 0x64356f713468327aULL, 0x6676353367643337ULL, 0x6a656832686c6365ULL, 0x0000000000346879ULL, 0x676965726b666162ULL, 0x776a616b6d6b7a62ULL, 0x326f79346c623334ULL, 0x6d73357a786d3577ULL, 0x33703333676e7433ULL, 0x6c37777a75707768ULL, 0x666d757670346578ULL, 0x00000000006d3336ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 1915, 3 piQcells: bafkreifxnzluqkbmuc43gf2x2gjvb4kyrc3fj4d5yvuu6u2e4mq4huagky -> bafkreicua4h2nyzsl7gayioskna76hhv7lrl4iavge5cymu5fvpf3xmoqy
			locals.setURI = _cleanupSetURI_input{ 1915, 0, 0x666965726b666162ULL, 0x626b71756c7a6e78ULL, 0x326667333463756dULL, 0x6b3462766a673278ULL, 0x64346a6633637279ULL, 0x3275367575767935ULL, 0x61756834716d3465ULL, 0x0000000000796b67ULL, 0x636965726b666162ULL, 0x7a796e3268346175ULL, 0x6f69796167376c73ULL, 0x68683637616e6b73ULL, 0x6169346c726c3776ULL, 0x756d796335656776ULL, 0x6d78336670766635ULL, 0x000000000079716fULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 4991, 15 QDoge NFTs 1.0: bafkreias4fpguix2mgqbvw43zcijtbtertxa2kk5s5ntlpptozucafct2m -> bafkreihnyikvrkatpvioa34bdakgpoqd2k5ostmdpr6icj23l5mmxeazna
			locals.setURI = _cleanupSetURI_input{ 4991, 0, 0x616965726b666162ULL, 0x7869756770663473ULL, 0x3477766271676d32ULL, 0x7462746a69637a33ULL, 0x6b6b326178747265ULL, 0x70706c746e357335ULL, 0x63666163757a6f74ULL, 0x00000000006d3274ULL, 0x686965726b666162ULL, 0x616b72766b69796eULL, 0x3433616f69767074ULL, 0x716f70676b616462ULL, 0x6d74736f356b3264ULL, 0x326a636936727064ULL, 0x6165786d6d356c33ULL, 0x0000000000616e7aULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 5077, 15 QDoge NFTs 1.0: bafkreibvlxngjmzcx52v3fcdj2ytnjhdhafywwajlbh72likcl2t3r7qti -> bafkreiaryhdum64ch6fdsa6vnadqwb7w3p7e7yenlomn4i7pvu4lmxhqji
			locals.setURI = _cleanupSetURI_input{ 5077, 0, 0x626965726b666162ULL, 0x7a6d6a676e786c76ULL, 0x6366337632357863ULL, 0x686a6e7479326a64ULL, 0x6177777966616864ULL, 0x696c323768626c6aULL, 0x37723374326c636bULL, 0x0000000000697471ULL, 0x616965726b666162ULL, 0x34366d7564687972ULL, 0x3661736466366863ULL, 0x3762777164616e76ULL, 0x6579376537703377ULL, 0x3769346e6d6f6c6eULL, 0x68786d6c34757670ULL, 0x0000000000696a71ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 5085, 15 QDoge NFTs 1.0: bafkreichjhxh2hkemla3a5ceqbexuwurmdeebzvclfpedz57fs7a5pfjby -> bafkreihxqgj4xe6kpxj37stedxbrnwv7md75lvqfzdgr5piz4qrofck32m
			locals.setURI = _cleanupSetURI_input{ 5085, 0, 0x636965726b666162ULL, 0x6b68326878686a68ULL, 0x63356133616c6d65ULL, 0x7577757865627165ULL, 0x767a626565646d72ULL, 0x357a646570666c63ULL, 0x6670356137736637ULL, 0x000000000079626aULL, 0x686965726b666162ULL, 0x366578346a677178ULL, 0x747337336a78706bULL, 0x76776e7262786465ULL, 0x71766c3537646d37ULL, 0x6970357267647a66ULL, 0x6b63666f7271347aULL, 0x00000000006d3233ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
			// 5752, 1 Qub3s: bafkreig76xptwjb7vgugsnh3us4qdxdez5niqypt5kyutkpblsgxcgz6lm -> bafkreihduhk2qcxawjnzsu3mdvqdopveelvuqajy5clayi5tck5zja6tha
			locals.setURI = _cleanupSetURI_input{ 5752, 0, 0x676965726b666162ULL, 0x626a777470783637ULL, 0x686e736775677637ULL, 0x6478647134737533ULL, 0x707971696e357a65ULL, 0x706b7475796b3574ULL, 0x7a67637867736c62ULL, 0x00000000006d6c36ULL, 0x686965726b666162ULL, 0x786371326b687564ULL, 0x3375737a6e6a7761ULL, 0x76706f647176646dULL, 0x6a617175766c6565ULL, 0x356979616c633579ULL, 0x36616a7a356b6374ULL, 0x0000000000616874ULL };
			CALL(_cleanupSetURI, locals.setURI, locals.setURIOutput);
		}
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
