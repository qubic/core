/**
 * @file Pulse.h
 * @brief Pulse lottery contract: 6 digits per ticket, winning digits are 6 draws from 0..9.
 *
 * Mechanics:
 *  - Tickets are sold during SELLING state (1 ticket per call).
 *  - Draw is triggered on scheduled days after drawHour (UTC).
 *  - Ticket revenue (QHeart asset) is split by configured percents; remainder stays in contract.
 *  - Fixed rewards are paid from the contract QHeart balance ("Pulse wallet").
 *  - If contract balance exceeds cap, excess is sent to QHeart wallet.
 */

using namespace QPI;

constexpr uint16 PULSE_MAX_NUMBER_OF_PLAYERS = 1024;
constexpr uint16 PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS = div<uint16>(PULSE_MAX_NUMBER_OF_PLAYERS, 2);
constexpr uint8 PULSE_PLAYER_DIGITS = 6;
constexpr uint8 PULSE_PLAYER_DIGITS_ALIGNED = PULSE_PLAYER_DIGITS + 2;
constexpr uint8 PULSE_WINNING_DIGITS = PULSE_PLAYER_DIGITS;
constexpr uint8 PULSE_WINNING_DIGITS_ALIGNED = PULSE_PLAYER_DIGITS_ALIGNED;
constexpr uint8 PULSE_MAX_DIGIT = 9;
constexpr uint8 PULSE_MAX_DIGIT_ALIGNED = PULSE_MAX_DIGIT + 7;
constexpr uint64 PULSE_TICKET_PRICE_DEFAULT = 200000ULL;
constexpr uint16 PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;
constexpr uint64 PULSE_QHEART_ASSET_NAME = 92712259110993ULL; // "QHEART"
constexpr uint8 PULSE_DEFAULT_DEV_PERCENT = 10;
constexpr uint8 PULSE_DEFAULT_BURN_PERCENT = 10;
constexpr uint8 PULSE_DEFAULT_SHAREHOLDERS_PERCENT = 10;
constexpr uint8 PULSE_DEFAULT_RL_SHAREHOLDERS_PERCENT = 5;
constexpr uint64 PULSE_DEFAULT_QHEART_HOLD_LIMIT = 2000000000ULL;
constexpr uint8 PULSE_TICK_UPDATE_PERIOD = 100;
constexpr uint8 PULSE_DEFAULT_DRAW_HOUR = 11; // 11:00 UTC
constexpr uint8 PULSE_DEFAULT_SCHEDULE = 1 << WEDNESDAY | 1 << FRIDAY | 1 << SUNDAY;
constexpr uint32 PULSE_DEFAULT_INIT_TIME = 22 << 9 | 4 << 5 | 13;

constexpr uint64 PULSE_CONTRACT_ASSET_NAME = 297750254928ULL; // "PULSE"

struct PULSE2
{
};

struct PULSE : public ContractBase
{
public:
	// Bitmask for runtime state flags.
	enum class EState : uint8
	{
		SELLING = 1 << 0,
	};

	friend EState operator|(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
	friend EState operator&(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
	friend EState operator~(const EState& a) { return static_cast<EState>(~static_cast<uint8>(a)); }
	template<typename T> friend bool operator==(const EState& a, const T& b) { return static_cast<uint8>(a) == b; }
	template<typename T> friend bool operator!=(const EState& a, const T& b) { return !(a == b); }

	// Public return codes for user procedures/functions.
	enum class EReturnCode : uint8
	{
		SUCCESS,
		TICKET_INVALID_PRICE,
		TICKET_ALL_SOLD_OUT,
		TICKET_SELLING_CLOSED,
		INVALID_NUMBERS = 5,
		ACCESS_DENIED,
		INVALID_VALUE,
		TRANSFER_TO_PULSE_FAILED,
		TRANSFER_FROM_PULSE_FAILED,
		UNKNOWN_ERROR = UINT8_MAX
	};

	// Ticket payload stored per round; digits use QPI-aligned storage.
	struct Ticket
	{
		id player;
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits;

		bool isValid() const { return !isZero(player); }
	};

	struct AutoParticipant
	{
		id player;
		sint64 deposit;
		sint64 entropyDeposit;
		uint16 desiredTickets;
	};

	// Forward declaration for use by NextEpochData::apply.
	struct StateData;

	// Deferred settings applied at END_EPOCH to avoid mid-round changes.
	struct NextEpochData
	{
		void clear()
		{
			hasNewPrice = false;
			hasNewSchedule = false;
			hasNewDrawHour = false;
			hasNewFee = false;
			hasNewQHeartHoldLimit = false;
			newPrice = 0;
			newSchedule = 0;
			newDrawHour = 0;
			newDevPercent = 0;
			newBurnPercent = 0;
			newShareholdersPercent = 0;
			newQHeartHoldLimit = 0;
		}

		void apply(StateData& s) const
		{
			if (hasNewPrice)
			{
				s.ticketPrice = newPrice;
			}
			if (hasNewSchedule)
			{
				s.schedule = newSchedule;
			}
			if (hasNewDrawHour)
			{
				s.drawHour = newDrawHour;
			}
			if (hasNewFee)
			{
				s.devPercent = newDevPercent;
				s.burnPercent = newBurnPercent;
				s.shareholdersPercent = newShareholdersPercent;
				s.rlShareholdersPercent = newRLShareholdersPercent;
			}
			if (hasNewQHeartHoldLimit)
			{
				s.qheartHoldLimit = newQHeartHoldLimit;
			}
		}

		bit hasNewPrice;
		bit hasNewSchedule;
		bit hasNewDrawHour;
		bit hasNewFee;
		bit hasNewQHeartHoldLimit;
		uint64 newPrice;
		uint8 newSchedule;
		uint8 newDrawHour;
		uint8 newDevPercent;
		uint8 newBurnPercent;
		uint8 newShareholdersPercent;
		uint8 newRLShareholdersPercent;
		uint64 newQHeartHoldLimit;
	};

	// Winner history entry returned by GetWinners.
	struct WinnerInfo
	{
		id winnerAddress;
		uint64 revenue;
		uint16 epoch;
	};

	struct StateData
	{
		// Ring buffer of recent winners; index is winnersCounter % capacity.
		Array<WinnerInfo, PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
		// Tickets for the current round; valid range is [0, ticketCounter).
		Array<Ticket, PULSE_MAX_NUMBER_OF_PLAYERS> tickets;
		// Auto-buy participants keyed by user id.
		HashMap<id, AutoParticipant, PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS> autoParticipants;
		// Last settled winning digits; undefined before the first draw.
		Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> lastWinningDigits;
		NextEpochData nextEpochData;
		id teamAddress;
		id qheartIssuer;
		// Monotonic winner count used to rotate the winners ring buffer.
		uint64 winnersCounter;
		sint64 ticketCounter;
		sint64 ticketPrice;
		// Contract balance above this cap is swept to the QHeart wallet after settlement.
		uint64 qheartHoldLimit;
		// Date stamp of the most recent draw; PULSE_DEFAULT_INIT_TIME is a bootstrap sentinel.
		uint32 lastDrawDateStamp;
		// Per-user auto-purchase limits; 0 means unlimited.
		uint16 maxAutoTicketsPerUser;
		uint8 devPercent;
		uint8 burnPercent;
		uint8 shareholdersPercent;
		uint8 rlShareholdersPercent;
		uint8 schedule;
		uint8 drawHour;
		EState currentState;
	};

	struct ValidateDigits_input
	{
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits;
	};
	struct ValidateDigits_output
	{
		bit isValid;
	};
	struct ValidateDigits_locals
	{
		uint8 idx;
		uint8 value;
	};

	struct BuyTicket_input
	{
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits;
	};

	struct BuyTicket_output
	{
		EReturnCode returnCode;
	};

	struct BuyTicket_locals
	{
		uint64 slotsLeft;
		sint64 userBalance;
		sint64 transferResult;
		Ticket ticket;
		ValidateDigits_input validateInput;
		ValidateDigits_output validateOutput;
	};

	struct GetRandomDigits_input
	{
		uint64 seed;
	};
	struct GetRandomDigits_output
	{
		Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> digits;
	};
	struct GetRandomDigits_locals
	{
		uint64 tempValue;
		uint8 index;
		uint8 candidate;
	};

	struct PrepareRandomTickets_input
	{
		uint16 count;
	};

	struct PrepareRandomTickets_output
	{
		EReturnCode returnCode;
		uint16 count;
	};

	struct PrepareRandomTickets_locals
	{
		uint64 slotsLeft;
	};

	struct ChargeTicketsFromPlayer_input
	{
		id player;
		uint16 count;
	};

	struct ChargeTicketsFromPlayer_output
	{
		EReturnCode returnCode;
	};

	struct ChargeTicketsFromPlayer_locals
	{
		sint64 userBalance;
		sint64 transferResult;
		uint64 totalPrice;
	};

	struct AllocateRandomTickets_input
	{
		id player;
		uint16 count;
	};

	struct AllocateRandomTickets_output
	{
		EReturnCode returnCode;
	};

	struct AllocateRandomTickets_locals
	{
		uint64 slotsLeft;
		uint64 randomSeed;
		uint64 tempSeed;
		m256i randomDigest;
		RANDOM::BuyEntropy_input buyEntropyInput;
		RANDOM::BuyEntropy_output buyEntropyOutput;
		uint16 i;
		Ticket ticket;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;

		struct RandomData
		{
			bit_4096 entropy;
			AllocateRandomTickets_input allocateInput;
			sint64 ticketCounter;
			uint32 tick;
		} randomData;
	};

	struct BuyRandomTickets_input
	{
		uint16 count;
	};

	struct BuyRandomTickets_output
	{
		EReturnCode returnCode;
	};

	struct BuyRandomTickets_locals
	{
		PrepareRandomTickets_input prepareInput;
		PrepareRandomTickets_output prepareOutput;
		ChargeTicketsFromPlayer_input chargeInput;
		ChargeTicketsFromPlayer_output chargeOutput;
		AllocateRandomTickets_input allocateInput;
		AllocateRandomTickets_output allocateOutput;
		sint64 refundAmount;
	};

	struct DepositManagedQHeart_input
	{
		sint64 amount;
	};
	struct DepositManagedQHeart_output
	{
		EReturnCode returnCode;
	};
	struct DepositManagedQHeart_locals
	{
		sint64 transferResult;
		sint64 userBalance;
	};

	/**
	 * @brief Input for TransferShareManagementRights.
	 */
	struct TransferShareManagementRights_input
	{
		// Number of managed QHeart shares to release.
		sint64 numberOfShares;
		// Destination contract index that should acquire management rights.
		uint16 newManagingContractIndex;
	};
	/**
	 * @brief Output for TransferShareManagementRights.
	 */
	struct TransferShareManagementRights_output
	{
		// EReturnCode value describing validation or share-release result.
		EReturnCode returnCode;
	};
	struct TransferShareManagementRights_locals
	{
		Asset asset;
		sint64 releaseResult;
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

	struct GetTicketPrice_input
	{
	};
	struct GetTicketPrice_output
	{
		uint64 ticketPrice;
	};

	struct GetPlayerBalance_input
	{
		id player;
	};
	struct GetPlayerBalance_output
	{
		uint64 balance;
		EReturnCode returnCode;
	};

	struct GetFees_input
	{
	};
	struct GetFees_output
	{
		uint8 devPercent;
		uint8 burnPercent;
		uint8 shareholdersPercent;
		uint8 rlShareholdersPercent;
		EReturnCode returnCode;
	};

	struct GetQHeartHoldLimit_input
	{
	};
	struct GetQHeartHoldLimit_output
	{
		uint64 qheartHoldLimit;
	};

	struct GetQHeartWallet_input
	{
	};
	struct GetQHeartWallet_output
	{
		id wallet;
	};

	struct GetWinningDigits_input
	{
	};
	struct GetWinningDigits_output
	{
		Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> digits;
	};

	struct GetBalance_input
	{
	};
	struct GetBalance_output
	{
		uint64 balance;
	};

	struct GetPlayers_input
	{
	};
	struct GetPlayers_output
	{
		Array<Ticket, PULSE_MAX_NUMBER_OF_PLAYERS> players;
		EReturnCode returnCode;
	};

	struct GetPrizeTable_input
	{
	};
	struct GetPrizeTable_output
	{
		Array<uint64, PULSE_PLAYER_DIGITS_ALIGNED> leftAlignedRewards;
		Array<uint64, PULSE_PLAYER_DIGITS_ALIGNED> anyPositionRewards;
		uint64 ticketPrice;
		EReturnCode returnCode;
	};
	struct GetPrizeTable_locals
	{
		uint8 matches;
	};

	struct GetRoundState_input
	{
	};
	struct GetRoundState_output
	{
		uint32 epoch;
		uint32 lastDrawDateStamp;
		uint16 ticketCounter;
		uint16 maxPlayers;
		uint16 slotsLeft;
		uint8 currentState;
		uint8 drawHour;
		uint8 schedule;
		bit sellingOpen;
		EReturnCode returnCode;
	};
	struct GetRoundState_locals
	{
		sint64 slotsLeft;
	};

	struct FillWinnersInfo_input
	{
		id winnerAddress;
		uint64 revenue;
	};
	struct FillWinnersInfo_output
	{
	};
	struct FillWinnersInfo_locals
	{
		WinnerInfo winnerInfo;
		uint64 insertIdx;
	};

	struct GetWinners_input
	{
	};
	struct GetWinners_output
	{
		Array<WinnerInfo, PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
		uint64 winnersCounter;
		EReturnCode returnCode;
	};

	struct SetPrice_input
	{
		uint64 newPrice;
	};
	struct SetPrice_output
	{
		EReturnCode returnCode;
	};

	struct SetSchedule_input
	{
		uint8 newSchedule;
	};
	struct SetSchedule_output
	{
		EReturnCode returnCode;
	};

	struct SetDrawHour_input
	{
		uint8 newDrawHour;
	};
	struct SetDrawHour_output
	{
		EReturnCode returnCode;
	};

	struct SetFees_input
	{
		uint8 devPercent;
		uint8 burnPercent;
		uint8 shareholdersPercent;
		uint8 rlShareholdersPercent;
	};
	struct SetFees_output
	{
		EReturnCode returnCode;
	};

	struct SetQHeartHoldLimit_input
	{
		uint64 newQHeartHoldLimit;
	};
	struct SetQHeartHoldLimit_output
	{
		EReturnCode returnCode;
	};

	struct SetQHeartWallet_input
	{
		id newWallet;
	};
	struct SetQHeartWallet_output
	{
		EReturnCode returnCode;
	};

	struct ComputePrize_locals
	{
		uint8 leftAlignedMatches;
		uint8 anyPositionMatches;
		uint8 j;
		uint8 digitValue;
		uint64 leftAlignedReward;
		uint64 anyPositionReward;
		uint64 prize;
		Array<uint8, PULSE_MAX_DIGIT_ALIGNED> ticketCounts;
		Array<uint8, PULSE_MAX_DIGIT_ALIGNED> winningCounts;
	};

	struct TransferTokenToShareholder_input
	{
		sint64 shareholdersAmount;
		Asset shareholdersAsset;
		sint64 shareholdersTotalShares;
	};

	struct TransferTokenToShareholder_output
	{
	};

	struct TransferTokenToShareholder_locals
	{
		AssetPossessionIterator shareholdersIter;
		sint64 shareholdersDividendPerShare;
		sint64 shareholdersHolderShares;
	};

	using ReturnAllTickets_input = NoData;
	using ReturnAllTickets_output = NoData;

	struct ReturnAllTickets_locals
	{
		Ticket ticket;
		sint64 i;
	};

	struct SettleRound_input
	{
	};
	struct SettleRound_output
	{
	};
	struct SettleRound_locals
	{
		sint64 i;
		sint64 roundRevenue;
		sint64 devAmount;
		sint64 burnAmount;
		sint64 shareholdersAmount;
		sint64 rlShareholdersAmount;
		sint64 balanceSigned;
		uint64 balance;
		uint64 availableBalance;
		uint64 prize;
		uint64 totalPrize;
		uint64 reservedBalance;
		m256i randomDigest;
		RANDOM::BuyEntropy_input buyEntropyInput;
		RANDOM::BuyEntropy_output buyEntropyOutput;
		uint64 randomSeed;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;
		Ticket ticket;
		Entity entity;
		ComputePrize_locals computePrizeLocals;
		FillWinnersInfo_input fillWinnersInfoInput;
		FillWinnersInfo_output fillWinnersInfoOutput;
		TransferTokenToShareholder_input transferInput;
		TransferTokenToShareholder_output transferOutput;
		ReturnAllTickets_input returnAllTicketsInput;
		ReturnAllTickets_output returnAllTicketsOutput;

		struct SettleEntropyData
		{
			bit_4096 entropy;
			sint64 ticketCounter;
			sint64 ticketPrice;
			uint32 tick;
			uint16 epoch;
		} settleEntropyData;
	};

	struct BEGIN_TICK_locals
	{
		uint32 currentDateStamp;
		uint8 currentDayOfWeek;
		uint8 currentHour;
		uint8 isWednesday;
		uint8 isScheduledToday;
		SettleRound_input settleInput;
		SettleRound_output settleOutput;
	};

	struct BEGIN_EPOCH_locals
	{
		QX::Fees_input feesInput;
		QX::Fees_output feesOutput;
	};

public:
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetTicketPrice, 1);
		REGISTER_USER_FUNCTION(GetPlayerBalance, 2);
		REGISTER_USER_FUNCTION(GetRoundState, 3);
		REGISTER_USER_FUNCTION(GetFees, 4);
		REGISTER_USER_FUNCTION(GetQHeartHoldLimit, 5);
		REGISTER_USER_FUNCTION(GetQHeartWallet, 6);
		REGISTER_USER_FUNCTION(GetWinningDigits, 7);
		REGISTER_USER_FUNCTION(GetBalance, 8);
		REGISTER_USER_FUNCTION(GetWinners, 9);
		REGISTER_USER_FUNCTION(ValidateDigits, 12);
		REGISTER_USER_FUNCTION(GetPlayers, 13);
		REGISTER_USER_FUNCTION(GetPrizeTable, 14);

		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		REGISTER_USER_PROCEDURE(SetSchedule, 3);
		REGISTER_USER_PROCEDURE(SetDrawHour, 4);
		REGISTER_USER_PROCEDURE(SetFees, 5);
		REGISTER_USER_PROCEDURE(SetQHeartHoldLimit, 6);
		REGISTER_USER_PROCEDURE(BuyRandomTickets, 7);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 12);
		REGISTER_USER_PROCEDURE(DepositManagedQHeart, 13);
	}

	INITIALIZE()
	{
		state.mut().teamAddress = ID(_R, _O, _J, _V, _A, _E, _M, _F, _B, _X, _X, _Y, _N, _G, _A, _U, _A, _U, _I, _I, _X, _L, _B, _U, _P, _D, _H, _C,
		                             _D, _P, _E, _S, _Y, _Z, _O, _V, _W, _U, _Y, _E, _C, _B, _Q, _V, _Z, _R, _F, _T, _K, _A, _G, _S, _H, _T, _N, _A);
		state.mut().qheartIssuer = ID(_S, _S, _G, _X, _S, _L, _S, _X, _F, _E, _J, _O, _O, _B, _T, _Z, _W, _V, _D, _S, _R, _C, _E, _F, _G, _X, _N, _D,
		                              _Y, _U, _V, _D, _X, _M, _Q, _A, _L, _X, _L, _B, _X, _G, _D, _C, _R, _X, _T, _K, _F, _Z, _I, _O, _T, _G, _Z, _F);

		state.mut().ticketPrice = PULSE_TICKET_PRICE_DEFAULT;
		state.mut().devPercent = PULSE_DEFAULT_DEV_PERCENT;
		state.mut().burnPercent = PULSE_DEFAULT_BURN_PERCENT;
		state.mut().shareholdersPercent = PULSE_DEFAULT_SHAREHOLDERS_PERCENT;
		state.mut().rlShareholdersPercent = PULSE_DEFAULT_RL_SHAREHOLDERS_PERCENT;
		state.mut().qheartHoldLimit = PULSE_DEFAULT_QHEART_HOLD_LIMIT;

		state.mut().schedule = PULSE_DEFAULT_SCHEDULE;
		state.mut().drawHour = PULSE_DEFAULT_DRAW_HOUR;
		state.mut().lastDrawDateStamp = PULSE_DEFAULT_INIT_TIME;

		enableBuyTicket(state, false);
	}

	BEGIN_EPOCH_WITH_LOCALS()
	{
		if (state.get().schedule == 0)
		{
			state.mut().schedule = PULSE_DEFAULT_SCHEDULE;
		}
		if (state.get().drawHour == 0)
		{
			state.mut().drawHour = PULSE_DEFAULT_DRAW_HOUR;
		}

		RL::makeDateStamp(qpi.year(), qpi.month(), qpi.day(), state.mut().lastDrawDateStamp);
		enableBuyTicket(state, state.get().lastDrawDateStamp != PULSE_DEFAULT_INIT_TIME);
	}

	END_EPOCH()
	{
		enableBuyTicket(state, false);
		clearStateOnEndEpoch(state);
		state.mut().nextEpochData.apply(state.mut());
		state.mut().nextEpochData.clear();
	}

	BEGIN_TICK_WITH_LOCALS()
	{
		// Throttle draw checks to reduce per-tick cost.
		if (mod(qpi.tick(), static_cast<uint32>(PULSE_TICK_UPDATE_PERIOD)) != 0)
		{
			return;
		}

		locals.currentHour = qpi.hour();
		locals.currentDayOfWeek = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
		locals.isWednesday = locals.currentDayOfWeek == WEDNESDAY;

		if (locals.currentHour < state.get().drawHour)
		{
			return;
		}

		RL::makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);

		if (locals.currentDateStamp == PULSE_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, false);
			state.mut().lastDrawDateStamp = PULSE_DEFAULT_INIT_TIME;
			return;
		}

		if (state.get().lastDrawDateStamp == PULSE_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, true);
			if (locals.isWednesday)
			{
				state.mut().lastDrawDateStamp = locals.currentDateStamp;
			}
			else
			{
				state.mut().lastDrawDateStamp = 0;
			}
		}

		if (state.get().lastDrawDateStamp == locals.currentDateStamp)
		{
			return;
		}

		locals.isScheduledToday = ((state.get().schedule & (1u << locals.currentDayOfWeek)) != 0);
		if (!locals.isWednesday && !locals.isScheduledToday)
		{
			return;
		}

		state.mut().lastDrawDateStamp = locals.currentDateStamp;
		enableBuyTicket(state, false);

		CALL(SettleRound, locals.settleInput, locals.settleOutput);

		clearStateOnEndDraw(state);
		enableBuyTicket(state, !locals.isWednesday);
	}

	PRE_ACQUIRE_SHARES()
	{
		output.requestedFee = 0;
		output.allowTransfer = true;
	}

	/**
	 * Validates a ticket payload without changing contract state.
	 * @param digits Candidate ticket digits.
	 * @return `isValid = true` when every digit is within the supported range [0..9].
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ValidateDigits)
	{
		output.isValid = true;
		for (locals.idx = 0; locals.idx < PULSE_PLAYER_DIGITS; ++locals.idx)
		{
			locals.value = input.digits.get(locals.idx);
			if (locals.value > PULSE_MAX_DIGIT)
			{
				output.isValid = false;
				return;
			}
		}
	}

	/** Returns the current ticket price in QHeart units. */
	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.get().ticketPrice; }

	PUBLIC_FUNCTION(GetPlayerBalance)
	{
		output.balance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, input.player, input.player, SELF_INDEX, SELF_INDEX);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Returns the QHeart balance cap retained by the contract. */
	PUBLIC_FUNCTION(GetQHeartHoldLimit) { output.qheartHoldLimit = state.get().qheartHoldLimit; }
	/** Returns the designated QHeart issuer wallet. */
	PUBLIC_FUNCTION(GetQHeartWallet) { output.wallet = state.get().qheartIssuer; }
	/** Returns the digits from the last settled draw. */
	PUBLIC_FUNCTION(GetWinningDigits) { output.digits = state.get().lastWinningDigits; }

	/** Returns the current fee split configuration. */
	PUBLIC_FUNCTION(GetFees)
	{
		output.devPercent = state.get().devPercent;
		output.burnPercent = state.get().burnPercent;
		output.shareholdersPercent = state.get().shareholdersPercent;
		output.rlShareholdersPercent = state.get().rlShareholdersPercent;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Returns the contract QHeart balance held in the Pulse wallet. */
	PUBLIC_FUNCTION(GetBalance)
	{
		output.balance = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX);
	}

	/**
	 * Returns the current round ticket snapshot.
	 * @return Ticket entries already allocated for the ongoing round; unused trailing slots remain zeroed.
	 */
	PUBLIC_FUNCTION(GetPlayers)
	{
		output.players = state.get().tickets;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * Returns the payout table derived from the current ticket price.
	 * @return Reward arrays indexed by match count for left-aligned and any-position payouts, plus the active ticket price.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetPrizeTable)
	{
		output.ticketPrice = state.get().ticketPrice;
		for (locals.matches = 0; locals.matches <= PULSE_PLAYER_DIGITS; ++locals.matches)
		{
			output.leftAlignedRewards.set(locals.matches, getLeftAlignedReward(state, locals.matches));
			output.anyPositionRewards.set(locals.matches, getAnyPositionReward(state, locals.matches));
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * Returns the current round lifecycle state and sale progress.
	 * @return Current epoch, last processed draw date stamp, ticket counters, runtime state flags, active schedule, draw hour, and selling status.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetRoundState)
	{
		locals.slotsLeft = getSlotsLeft(state);

		output.epoch = qpi.epoch();
		output.lastDrawDateStamp = state.get().lastDrawDateStamp;
		output.ticketCounter =
		    static_cast<uint16>(RL::min(static_cast<uint64>(RL::max(state.get().ticketCounter, 0LL)), state.get().tickets.capacity()));
		output.maxPlayers = static_cast<uint16>(state.get().tickets.capacity());
		output.slotsLeft = static_cast<uint16>(RL::min(static_cast<uint64>(RL::max(locals.slotsLeft, 0LL)), state.get().tickets.capacity()));
		output.currentState = static_cast<uint8>(state.get().currentState);
		output.drawHour = state.get().drawHour;
		output.schedule = state.get().schedule;
		output.sellingOpen = isSellingOpen(state);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Returns the winners ring buffer snapshot and the current insertion counter. */
	PUBLIC_FUNCTION(GetWinners)
	{
		output.winners = state.get().winners;
		getWinnerCounter(state, output.winnersCounter);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Schedules a new ticket price for the next epoch (owner-only). */
	PUBLIC_PROCEDURE(SetPrice)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().qheartIssuer)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newPrice == 0)
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}

		state.mut().nextEpochData.hasNewPrice = true;
		state.mut().nextEpochData.newPrice = input.newPrice;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Schedules a new draw schedule bitmask for the next epoch (owner-only). */
	PUBLIC_PROCEDURE(SetSchedule)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().qheartIssuer)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newSchedule == 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.mut().nextEpochData.hasNewSchedule = true;
		state.mut().nextEpochData.newSchedule = input.newSchedule;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Schedules a new draw hour in UTC for the next epoch (owner-only). */
	PUBLIC_PROCEDURE(SetDrawHour)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().qheartIssuer)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newDrawHour > 23)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.mut().nextEpochData.hasNewDrawHour = true;
		state.mut().nextEpochData.newDrawHour = input.newDrawHour;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Schedules new fee splits for the next epoch (owner-only). */
	PUBLIC_PROCEDURE(SetFees)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().qheartIssuer)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.devPercent + input.burnPercent + input.shareholdersPercent + input.rlShareholdersPercent > 100)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.mut().nextEpochData.hasNewFee = true;
		state.mut().nextEpochData.newDevPercent = input.devPercent;
		state.mut().nextEpochData.newBurnPercent = input.burnPercent;
		state.mut().nextEpochData.newShareholdersPercent = input.shareholdersPercent;
		state.mut().nextEpochData.newRLShareholdersPercent = input.rlShareholdersPercent;

		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Schedules a new QHeart hold limit for the next epoch (owner-only). */
	PUBLIC_PROCEDURE(SetQHeartHoldLimit)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().qheartIssuer)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		state.mut().nextEpochData.hasNewQHeartHoldLimit = true;
		state.mut().nextEpochData.newQHeartHoldLimit = input.newQHeartHoldLimit;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Buys a single ticket and transfers the ticket price from the invocator. */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = EReturnCode::TICKET_SELLING_CLOSED;
			return;
		}

		locals.validateInput.digits = input.digits;
		CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
		if (!locals.validateOutput.isValid)
		{
			output.returnCode = EReturnCode::INVALID_NUMBERS;
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		if (locals.slotsLeft == 0)
		{
			output.returnCode = EReturnCode::TICKET_ALL_SOLD_OUT;
			return;
		}

		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < state.get().ticketPrice)
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, qpi.invocator(),
		                                                                qpi.invocator(), state.get().ticketPrice, SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}

		locals.ticket.player = qpi.invocator();
		locals.ticket.digits = input.digits;
		state.mut().tickets.set(state.get().ticketCounter, locals.ticket);
		state.mut().ticketCounter = RL::min(static_cast<uint64>(state.get().ticketCounter) + 1ull, state.get().tickets.capacity());

		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Buys multiple random tickets and transfers the total price from the invocator. */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyRandomTickets)
	{
		locals.prepareInput.count = input.count;
		CALL(PrepareRandomTickets, locals.prepareInput, locals.prepareOutput);
		if (locals.prepareOutput.returnCode != EReturnCode::SUCCESS)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.prepareOutput.returnCode;
			return;
		}

		if (qpi.invocationReward() < static_cast<sint64>(RL_RANDOM_ENTROPY_FEE))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		if (qpi.invocationReward() > static_cast<sint64>(RL_RANDOM_ENTROPY_FEE))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - static_cast<sint64>(RL_RANDOM_ENTROPY_FEE));
		}

		locals.chargeInput.player = qpi.invocator();
		locals.chargeInput.count = locals.prepareOutput.count;
		CALL(ChargeTicketsFromPlayer, locals.chargeInput, locals.chargeOutput);
		if (locals.chargeOutput.returnCode != EReturnCode::SUCCESS)
		{
			qpi.transfer(qpi.invocator(), RL_RANDOM_ENTROPY_FEE);
			output.returnCode = locals.chargeOutput.returnCode;
			return;
		}

		locals.allocateInput.player = qpi.invocator();
		locals.allocateInput.count = locals.prepareOutput.count;
		CALL(AllocateRandomTickets, locals.allocateInput, locals.allocateOutput);

		if (locals.allocateOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundAmount =
			    static_cast<sint64>(smul(static_cast<uint64>(locals.prepareOutput.count), static_cast<uint64>(state.get().ticketPrice)));
			if (locals.refundAmount > 0)
			{
				qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, locals.refundAmount,
				                                        qpi.invocator());
			}
			qpi.transfer(qpi.invocator(), RL_RANDOM_ENTROPY_FEE);
		}

		output.returnCode = locals.allocateOutput.returnCode;
	}

	/**
	 * Deposits QHeart already managed by Pulse into the Pulse wallet.
	 * @param amount QHeart amount to transfer from the invocator to the contract.
	 * @return Status code describing whether the transfer succeeded.
	 * @note This only moves managed QHeart into `SELF`; it does not update auto-participation or other accounting.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositManagedQHeart)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.amount <= 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < input.amount)
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, qpi.invocator(),
		                                                                qpi.invocator(), input.amount, SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_TO_PULSE_FAILED;
			return;
		}

		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Releases managed QHeart token rights to another contract for the invocator.
	 * @param input Number of QHeart tokens and the contract index that should acquire management rights.
	 * @param output Status code describing validation or rights-release result.
	 * @note The destination contract fee is paid from the invocation reward; any unused reward is refunded.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (input.numberOfShares <= 0 || input.newManagingContractIndex == 0 || input.newManagingContractIndex >= MAX_NUMBER_OF_CONTRACTS ||
		    input.newManagingContractIndex == SELF_INDEX)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		if (qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) <
		    input.numberOfShares)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::TRANSFER_FROM_PULSE_FAILED;
			return;
		}

		locals.asset.issuer = state.get().qheartIssuer;
		locals.asset.assetName = PULSE_QHEART_ASSET_NAME;

		locals.releaseResult = qpi.releaseShares(locals.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
		                                         input.newManagingContractIndex, qpi.invocationReward());
		if (locals.releaseResult < 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::TRANSFER_FROM_PULSE_FAILED;
			return;
		}

		if (qpi.invocationReward() > locals.releaseResult)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.releaseResult);
		}

		output.returnCode = EReturnCode::SUCCESS;
	}

private:
	PRIVATE_FUNCTION_WITH_LOCALS(GetRandomDigits)
	{
		// Derive each digit independently to avoid shared PRNG state.
		for (locals.index = 0; locals.index < PULSE_WINNING_DIGITS; ++locals.index)
		{
			RL::deriveOne(input.seed, locals.index, locals.tempValue);
			locals.candidate = static_cast<uint8>(mod(locals.tempValue, PULSE_MAX_DIGIT + 1ULL));

			output.digits.set(locals.index, locals.candidate);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(SettleRound)
	{
		if (state.get().ticketCounter == 0)
		{
			return;
		}

		qpi.getEntity(SELF, locals.entity);
		if ((locals.entity.incomingAmount - locals.entity.outgoingAmount) < RL_RANDOM_ENTROPY_FEE)
		{
			CALL(ReturnAllTickets, locals.returnAllTicketsInput, locals.returnAllTicketsOutput);
			return;
		}

		locals.buyEntropyInput.collateralTier = RL_RANDOM_COLLATERAL_TIER;
		locals.buyEntropyInput.numberOfBits = RL_RANDOM_ENTROPY_BITS;
		locals.buyEntropyInput.trustee = id::zero();
		INVOKE_OTHER_CONTRACT_PROCEDURE(RANDOM, BuyEntropy, locals.buyEntropyInput, locals.buyEntropyOutput, RL_RANDOM_ENTROPY_FEE);
		if (interContractCallError != NoCallError || RL::isZeroEntropy(locals.buyEntropyOutput.entropy))
		{
			CALL(ReturnAllTickets, locals.returnAllTicketsInput, locals.returnAllTicketsOutput);
			return;
		}

		locals.settleEntropyData.entropy = locals.buyEntropyOutput.entropy;
		locals.settleEntropyData.ticketCounter = state.get().ticketCounter;
		locals.settleEntropyData.ticketPrice = state.get().ticketPrice;
		locals.settleEntropyData.tick = qpi.tick();
		locals.settleEntropyData.epoch = qpi.epoch();
		locals.randomDigest = qpi.K12(locals.settleEntropyData);

		locals.roundRevenue = smul(state.get().ticketPrice, state.get().ticketCounter);
		locals.devAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.get().devPercent)), 100LL);
		locals.burnAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.get().burnPercent)), 100LL);
		locals.shareholdersAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.get().shareholdersPercent)), 100LL);
		locals.rlShareholdersAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.get().rlShareholdersPercent)), 100LL);

		if (locals.devAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, locals.devAmount,
			                                        state.get().teamAddress);
		}
		if (locals.shareholdersAmount > 0)
		{
			locals.transferInput.shareholdersAmount = locals.shareholdersAmount;
			locals.transferInput.shareholdersAsset.issuer = id::zero();
			locals.transferInput.shareholdersAsset.assetName = PULSE_CONTRACT_ASSET_NAME;
			locals.transferInput.shareholdersTotalShares = NUMBER_OF_COMPUTORS;
			CALL(TransferTokenToShareholder, locals.transferInput, locals.transferOutput);
		}
		if (locals.rlShareholdersAmount > 0)
		{
			locals.transferInput.shareholdersAmount = locals.rlShareholdersAmount;
			locals.transferInput.shareholdersAsset.issuer = id::zero();
			locals.transferInput.shareholdersAsset.assetName = QTF_RANDOM_LOTTERY_ASSET_NAME;
			locals.transferInput.shareholdersTotalShares = NUMBER_OF_COMPUTORS;
			CALL(TransferTokenToShareholder, locals.transferInput, locals.transferOutput);
		}
		if (locals.burnAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, locals.burnAmount, NULL_ID);
		}

		locals.randomSeed = locals.randomDigest.u64._0;
		locals.randomInput.seed = locals.randomSeed;
		CALL(GetRandomDigits, locals.randomInput, locals.randomOutput);
		state.mut().lastWinningDigits = locals.randomOutput.digits;

		locals.balanceSigned = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX);
		locals.balance = RL::max(locals.balanceSigned, 0LL);

		locals.totalPrize = 0;
		for (locals.i = 0; locals.i < state.get().ticketCounter; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			locals.prize = computePrize(state, locals.ticket, state.get().lastWinningDigits, locals.computePrizeLocals);
			locals.totalPrize += locals.prize;
		}

		locals.availableBalance = locals.balance;
		for (locals.i = 0; locals.i < state.get().ticketCounter; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			locals.prize = computePrize(state, locals.ticket, state.get().lastWinningDigits, locals.computePrizeLocals);

			if (locals.totalPrize > 0 && locals.availableBalance < locals.totalPrize)
			{
				// Pro-rate payouts when the contract balance cannot cover all prizes.
				locals.prize = div<uint64>(smul(static_cast<sint64>(locals.prize), static_cast<sint64>(locals.availableBalance)),
				                           static_cast<sint64>(locals.totalPrize));
			}

			if (locals.prize > 0 && locals.balance >= locals.prize)
			{
				qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF,
				                                        static_cast<sint64>(locals.prize), locals.ticket.player);
				locals.balance -= locals.prize;

				locals.fillWinnersInfoInput.winnerAddress = locals.ticket.player;
				locals.fillWinnersInfoInput.revenue = locals.prize;
				CALL(FillWinnersInfo, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput);
			}
		}

		locals.balanceSigned = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, SELF_INDEX, SELF_INDEX);
		locals.balance = (locals.balanceSigned > 0) ? static_cast<uint64>(locals.balanceSigned) : 0;

		if (state.get().qheartHoldLimit > 0 && locals.balance > state.get().qheartHoldLimit)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF,
			                                        static_cast<sint64>(locals.balance - state.get().qheartHoldLimit), state.get().qheartIssuer);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo)
	{
		if (input.winnerAddress == id::zero())
		{
			return;
		}

		getWinnerCounter(state, locals.insertIdx);
		++state.mut().winnersCounter;

		locals.winnerInfo.winnerAddress = input.winnerAddress;
		locals.winnerInfo.revenue = input.revenue;
		locals.winnerInfo.epoch = qpi.epoch();

		state.mut().winners.set(locals.insertIdx, locals.winnerInfo);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(PrepareRandomTickets)
	{
		if (input.count == 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			output.count = 0;
			return;
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = EReturnCode::TICKET_SELLING_CLOSED;
			output.count = 0;
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		output.count = RL::min(input.count, static_cast<uint16>(locals.slotsLeft));
		if (output.count == 0)
		{
			output.returnCode = EReturnCode::TICKET_ALL_SOLD_OUT;
			return;
		}

		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ChargeTicketsFromPlayer)
	{
		if (input.count == 0 || input.player == id::zero())
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		locals.totalPrice = smul(static_cast<sint64>(input.count), state.get().ticketPrice);
		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, input.player, input.player, SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < static_cast<sint64>(locals.totalPrice))
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, input.player, input.player,
		                                                                static_cast<sint64>(locals.totalPrice), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_TO_PULSE_FAILED;
			return;
		}

		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AllocateRandomTickets)
	{
		if (input.count == 0 || input.player == id::zero())
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = EReturnCode::TICKET_SELLING_CLOSED;
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		if (locals.slotsLeft < input.count)
		{
			output.returnCode = EReturnCode::TICKET_ALL_SOLD_OUT;
			return;
		}

		locals.buyEntropyInput.collateralTier = RL_RANDOM_COLLATERAL_TIER;
		locals.buyEntropyInput.numberOfBits = RL_RANDOM_ENTROPY_BITS;
		locals.buyEntropyInput.trustee = id::zero();
		INVOKE_OTHER_CONTRACT_PROCEDURE(RANDOM, BuyEntropy, locals.buyEntropyInput, locals.buyEntropyOutput, RL_RANDOM_ENTROPY_FEE);
		if (interContractCallError != NoCallError || RL::isZeroEntropy(locals.buyEntropyOutput.entropy))
		{
			output.returnCode = EReturnCode::UNKNOWN_ERROR;
			return;
		}

		locals.randomData.entropy = locals.buyEntropyOutput.entropy;
		locals.randomData.allocateInput = input;
		locals.randomData.ticketCounter = state.get().ticketCounter;
		locals.randomData.tick = qpi.tick();

		locals.randomDigest = qpi.K12(locals.randomData);
		locals.randomSeed = locals.randomDigest.u64._0;
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			RL::deriveOne(locals.randomSeed, locals.i, locals.tempSeed);
			locals.randomInput.seed = locals.tempSeed;
			CALL(GetRandomDigits, locals.randomInput, locals.randomOutput);

			locals.ticket.player = input.player;
			locals.ticket.digits = locals.randomOutput.digits;
			state.mut().tickets.set(state.get().ticketCounter, locals.ticket);
			state.mut().ticketCounter = RL::min(state.get().ticketCounter + 1LL, static_cast<sint64>(state.get().tickets.capacity()));
		}

		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(TransferTokenToShareholder)
	{
		if (input.shareholdersAmount <= 0 || input.shareholdersTotalShares <= 0)
		{
			return;
		}

		if (input.shareholdersAsset.assetName == 0)
		{
			return;
		}

		locals.shareholdersDividendPerShare = div<sint64>(input.shareholdersAmount, input.shareholdersTotalShares);
		if (locals.shareholdersDividendPerShare <= 0)
		{
			return;
		}

		locals.shareholdersIter.begin(input.shareholdersAsset);
		while (!locals.shareholdersIter.reachedEnd())
		{
			locals.shareholdersHolderShares = locals.shareholdersIter.numberOfPossessedShares();
			if (locals.shareholdersHolderShares > 0)
			{
				qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF,
				                                        smul(locals.shareholdersHolderShares, locals.shareholdersDividendPerShare),
				                                        locals.shareholdersIter.possessor());
			}
			locals.shareholdersIter.next();
		}
	};

	PRIVATE_PROCEDURE_WITH_LOCALS(ReturnAllTickets)
	{
		if (state.get().ticketCounter == 0 || isZero(state.get().qheartIssuer) || state.get().ticketPrice <= 0)
		{
			return;
		}

		for (locals.i = 0; locals.i < state.get().ticketCounter; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			if (locals.ticket.isValid())
			{
				qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, state.get().qheartIssuer, SELF, SELF, state.get().ticketPrice,
				                                        locals.ticket.player);
			}
		}
	}

public:
	static sint64 getSlotsLeft(const QPI::ContractState<StateData, CONTRACT_INDEX>& state)
	{
		return state.get().ticketCounter < static_cast<sint64>(state.get().tickets.capacity())
		           ? static_cast<sint64>(state.get().tickets.capacity()) - state.get().ticketCounter
		           : 0LL;
	}

protected:
	static void clearStateOnEndEpoch(QPI::ContractState<StateData, CONTRACT_INDEX>& state)
	{
		clearStateOnEndDraw(state);
		state.mut().lastDrawDateStamp = 0;
	}

	static void clearStateOnEndDraw(QPI::ContractState<StateData, CONTRACT_INDEX>& state)
	{
		state.mut().ticketCounter = 0;
		setMemory(state.mut().tickets, 0);
	}

	static void enableBuyTicket(QPI::ContractState<StateData, CONTRACT_INDEX>& state, bool bEnable)
	{
		state.mut().currentState = bEnable ? state.get().currentState | EState::SELLING : state.get().currentState & ~EState::SELLING;
	}

	static bool isSellingOpen(const QPI::ContractState<StateData, CONTRACT_INDEX>& state)
	{
		return (state.get().currentState & EState::SELLING) != 0;
	}

	static void getWinnerCounter(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, uint64& outCounter)
	{
		outCounter = mod(state.get().winnersCounter, state.get().winners.capacity());
	}

	static uint64 getLeftAlignedReward(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, uint8 matches)
	{
		switch (matches)
		{
			case 6: return 2000 * state.get().ticketPrice;
			case 5: return 300 * state.get().ticketPrice;
			case 4: return 60 * state.get().ticketPrice;
			case 3: return 20 * state.get().ticketPrice;
			case 2: return 4 * state.get().ticketPrice;
			case 1: return 1 * state.get().ticketPrice;
			default: return 0;
		}
	}

	static uint64 getAnyPositionReward(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, uint8 matches)
	{
		switch (matches)
		{
			case 6: return 100 * state.get().ticketPrice;
			case 5: return 10 * state.get().ticketPrice;
			case 4: return 3 * state.get().ticketPrice;
			case 3: return 1 * state.get().ticketPrice;
			case 2:
			case 1:
			default: return 0;
		}
	}

	static uint64 computePrize(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, const Ticket& ticket,
	                           const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winningDigits, ComputePrize_locals& locals)
	{
		setMemory(locals, 0);

		for (locals.j = 0; locals.j < PULSE_PLAYER_DIGITS; ++locals.j)
		{
			if (ticket.digits.get(locals.j) != winningDigits.get(locals.j))
			{
				break;
			}
			++locals.leftAlignedMatches;
		}

		STATIC_ASSERT(PULSE_PLAYER_DIGITS == PULSE_WINNING_DIGITS, "PULSE_PLAYER_DIGITS == PULSE_WINNING_DIGITS");
		for (locals.j = 0; locals.j < PULSE_PLAYER_DIGITS; ++locals.j)
		{
			locals.digitValue = ticket.digits.get(locals.j);
			locals.ticketCounts.set(locals.digitValue, locals.ticketCounts.get(locals.digitValue) + 1);

			locals.digitValue = winningDigits.get(locals.j);
			locals.winningCounts.set(locals.digitValue, locals.winningCounts.get(locals.digitValue) + 1);
		}

		for (locals.digitValue = 0; locals.digitValue <= PULSE_MAX_DIGIT; ++locals.digitValue)
		{
			locals.anyPositionMatches += RL::min<uint8>(locals.ticketCounts.get(locals.digitValue), locals.winningCounts.get(locals.digitValue));
		}

		// Reward the best of left-aligned or any-position matches to avoid double counting.
		locals.leftAlignedReward = getLeftAlignedReward(state, locals.leftAlignedMatches);
		locals.anyPositionReward = getAnyPositionReward(state, locals.anyPositionMatches);
		locals.prize = RL::max(locals.leftAlignedReward, locals.anyPositionReward);
		return locals.prize;
	}

	static uint16 clampPublicTicketCount(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, sint64 value)
	{
		return static_cast<uint16>(RL::min(static_cast<uint64>(RL::max(value, 0LL)), state.get().tickets.capacity()));
	}
};
