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
constexpr uint16 PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS = PULSE_MAX_NUMBER_OF_PLAYERS * 0.5;
constexpr uint8 PULSE_PLAYER_DIGITS = 6;
constexpr uint8 PULSE_PLAYER_DIGITS_ALIGNED = PULSE_PLAYER_DIGITS + 2;
constexpr uint8 PULSE_WINNING_DIGITS = PULSE_PLAYER_DIGITS;
constexpr uint8 PULSE_WINNING_DIGITS_ALIGNED = PULSE_PLAYER_DIGITS_ALIGNED;
constexpr uint8 PULSE_MAX_DIGIT = 9;
constexpr uint8 PULSE_MAX_DIGIT_ALIGNED = PULSE_MAX_DIGIT + 7;
constexpr uint64 PULSE_TICKET_PRICE_DEFAULT = 200000;
constexpr uint16 PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY = 1024;
constexpr uint64 PULSE_QHEART_ASSET_NAME = 92712259110993ULL; // "QHEART"
constexpr uint8 PULSE_DEFAULT_DEV_PERCENT = 10;
constexpr uint8 PULSE_DEFAULT_BURN_PERCENT = 5;
constexpr uint8 PULSE_DEFAULT_SHAREHOLDERS_PERCENT = 5;
constexpr uint8 PULSE_DEFAULT_QHEART_PERCENT = 5;
constexpr uint64 PULSE_DEFAULT_QHEART_HOLD_LIMIT = 2000000000ULL;
constexpr uint8 PULSE_TICK_UPDATE_PERIOD = 100;
constexpr uint8 PULSE_DEFAULT_DRAW_HOUR = 11; // 11:00 UTC
constexpr uint8 PULSE_DEFAULT_SCHEDULE = 1 << WEDNESDAY | 1 << FRIDAY | 1 << SUNDAY;
constexpr uint32 PULSE_DEFAULT_INIT_TIME = 22 << 9 | 4 << 5 | 13;
constexpr uint16 PULSE_DEFAULT_MAX_AUTO_TICKETS_PER_USER = PULSE_MAX_NUMBER_OF_PLAYERS;
constexpr uint64 PULSE_CLEANUP_THRESHOLD = 75;

const id PULSE_QHEART_ISSUER = ID(_S, _S, _G, _X, _S, _L, _S, _X, _F, _E, _J, _O, _O, _B, _T, _Z, _W, _V, _D, _S, _R, _C, _E, _F, _G, _X, _N, _D, _Y,
                                  _U, _V, _D, _X, _M, _Q, _A, _L, _X, _L, _B, _X, _G, _D, _C, _R, _X, _T, _K, _F, _Z, _I, _O, _T, _G, _Z, _F);
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
		AUTO_PARTICIPANTS_FULL,
		INVALID_NUMBERS,
		ACCESS_DENIED,
		INVALID_VALUE,
		TRANSFER_TO_PULSE_FAILED,
		TRANSFER_FROM_PULSE_FAILED,
		UNKNOWN_ERROR = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); };

	// Ticket payload stored per round; digits use QPI-aligned storage.
	struct Ticket
	{
		id player;
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits;
	};

	struct AutoParticipant
	{
		id player;
		sint64 deposit;
		uint16 desiredTickets;
	};

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
			newQHeartPercent = 0;
			newQHeartHoldLimit = 0;
		}

		void apply(PULSE& state) const
		{
			if (hasNewPrice)
			{
				state.ticketPrice = newPrice;
			}
			if (hasNewSchedule)
			{
				state.schedule = newSchedule;
			}
			if (hasNewDrawHour)
			{
				state.drawHour = newDrawHour;
			}
			if (hasNewFee)
			{
				state.devPercent = newDevPercent;
				state.burnPercent = newBurnPercent;
				state.shareholdersPercent = newShareholdersPercent;
				state.qheartPercent = newQHeartPercent;
			}
			if (hasNewQHeartHoldLimit)
			{
				state.qheartHoldLimit = newQHeartHoldLimit;
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
		uint8 newQHeartPercent;
		uint64 newQHeartHoldLimit;
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
		uint8 returnCode;
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
		uint8 returnCode;
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
		uint8 returnCode;
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
		uint8 returnCode;
	};

	struct AllocateRandomTickets_locals
	{
		uint64 slotsLeft;
		m256i mixedSpectrumValue;
		uint64 randomSeed;
		uint64 tempSeed;
		uint16 i;
		Ticket ticket;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;
	};

	struct BuyRandomTickets_input
	{
		uint16 count;
	};

	struct BuyRandomTickets_output
	{
		uint8 returnCode;
	};

	struct BuyRandomTickets_locals
	{
		PrepareRandomTickets_input prepareInput;
		PrepareRandomTickets_output prepareOutput;
		ChargeTicketsFromPlayer_input chargeInput;
		ChargeTicketsFromPlayer_output chargeOutput;
		AllocateRandomTickets_input allocateInput;
		AllocateRandomTickets_output allocateOutput;
	};

	struct FindAutoParticipant_input
	{
		id player;
	};
	struct FindAutoParticipant_output
	{
		bit found;
		sint64 index;
	};
	struct FindAutoParticipant_locals
	{
		sint64 elementIndex;
	};

	struct GetAutoParticipation_input
	{
	};
	struct GetAutoParticipation_output
	{
		uint64 deposit;
		uint16 desiredTickets;
		uint8 returnCode;
	};
	struct GetAutoParticipation_locals
	{
		AutoParticipant entry;
	};

	struct GetAutoStats_input
	{
	};
	struct GetAutoStats_output
	{
		uint16 autoParticipantsCounter;
		uint64 totalAutoDeposits;
		sint64 autoStartIndex;
		uint16 maxAutoTicketsPerUser;
		uint8 returnCode;
	};

	struct DepositAutoParticipation_input
	{
		sint64 amount;
		sint16 desiredTickets;
		bit buyNow;
	};
	struct DepositAutoParticipation_output
	{
		uint8 returnCode;
	};
	struct DepositAutoParticipation_locals
	{
		sint64 userBalance;
		sint64 transferResult;
		AutoParticipant entry;
		sint64 insertIndex;
		sint64 totalPrice;
		uint64 slotsLeft;
		uint64 affordable;
		uint64 toBuy;
		uint64 spend;
		uint64 seedIndex;
		m256i mixedSpectrumValue;
		uint64 randomSeed;
		uint64 tempSeed;
		uint16 i;
		Ticket ticket;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;
		BuyRandomTickets_input buyRandomTicketsInput;
		BuyRandomTickets_output buyRandomTicketsOutput;
	};

	struct WithdrawAutoParticipation_input
	{
		sint64 amount;
	};
	struct WithdrawAutoParticipation_output
	{
		uint8 returnCode;
	};
	struct WithdrawAutoParticipation_locals
	{
		sint64 transferResult;
		AutoParticipant entry;
		sint64 removedIndex;
		sint64 withdrawAmount;
	};

	struct SetAutoConfig_input
	{
		sint16 desiredTickets;
	};
	struct SetAutoConfig_output
	{
		uint8 returnCode;
	};
	struct SetAutoConfig_locals
	{
		sint64 insertIndex;
		sint64 removedIndex;
		AutoParticipant entry;
		uint16 desiredValue;
		FindAutoParticipant_input findInput;
		FindAutoParticipant_output findOutput;
	};

	struct SetAutoLimits_input
	{
		uint16 maxTicketsPerUser;
	};
	struct SetAutoLimits_output
	{
		uint8 returnCode;
	};

	struct GetTicketPrice_input
	{
	};
	struct GetTicketPrice_output
	{
		uint64 ticketPrice;
	};

	struct GetSchedule_input
	{
	};
	struct GetSchedule_output
	{
		uint8 schedule;
	};

	struct GetDrawHour_input
	{
	};
	struct GetDrawHour_output
	{
		uint8 drawHour;
	};

	struct GetFees_input
	{
	};
	struct GetFees_output
	{
		uint8 devPercent;
		uint8 burnPercent;
		uint8 shareholdersPercent;
		uint8 qheartPercent;
		uint8 returnCode;
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

	// Winner history entry returned by GetWinners.
	struct WinnerInfo
	{
		id winnerAddress;
		uint64 revenue;
		uint16 epoch;
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
		uint8 returnCode;
	};

	struct SetPrice_input
	{
		uint64 newPrice;
	};
	struct SetPrice_output
	{
		uint8 returnCode;
	};

	struct SetSchedule_input
	{
		uint8 newSchedule;
	};
	struct SetSchedule_output
	{
		uint8 returnCode;
	};

	struct SetDrawHour_input
	{
		uint8 newDrawHour;
	};
	struct SetDrawHour_output
	{
		uint8 returnCode;
	};

	struct SetFees_input
	{
		uint8 devPercent;
		uint8 burnPercent;
		uint8 shareholdersPercent;
		uint8 qheartPercent;
	};
	struct SetFees_output
	{
		uint8 returnCode;
	};

	struct SetQHeartHoldLimit_input
	{
		uint64 newQHeartHoldLimit;
	};
	struct SetQHeartHoldLimit_output
	{
		uint8 returnCode;
	};

	struct SetQHeartWallet_input
	{
		id newWallet;
	};
	struct SetQHeartWallet_output
	{
		uint8 returnCode;
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

	struct SettleRound_input
	{
	};
	struct SettleRound_output
	{
	};
	struct SettleRound_locals
	{
		uint64 i;
		sint64 roundRevenue;
		sint64 devAmount;
		sint64 burnAmount;
		sint64 shareholdersAmount;
		sint64 qheartAmount;
		sint64 balanceSigned;
		uint64 balance;
		uint64 availableBalance;
		uint64 prize;
		uint64 totalPrize;
		uint64 reservedBalance;
		m256i mixedSpectrumValue;
		uint64 randomSeed;
		Asset shareholdersAsset;
		AssetPossessionIterator shareholdersIter;
		sint64 shareholdersTotalShares;
		sint64 shareholdersDividendPerShare;
		sint64 shareholdersHolderShares;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;
		Ticket ticket;
		ComputePrize_locals computePrizeLocals;
		FillWinnersInfo_input fillWinnersInfoInput;
		FillWinnersInfo_output fillWinnersInfoOutput;
	};

	struct ProcessAutoTickets_input
	{
	};
	struct ProcessAutoTickets_output
	{
	};
	struct ProcessAutoTickets_locals
	{
		sint64 currentIndex;
		sint64 slotsLeft;
		uint64 affordable;
		sint64 toBuy;
		AutoParticipant entry;
		AllocateRandomTickets_input allocateInput;
		AllocateRandomTickets_output allocateOutput;
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
		ProcessAutoTickets_input autoTicketsInput;
		ProcessAutoTickets_output autoTicketsOutput;
	};

	struct BEGIN_EPOCH_locals
	{
		ProcessAutoTickets_input autoTicketsInput;
		ProcessAutoTickets_output autoTicketsOutput;
	};

public:
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetTicketPrice, 1);
		REGISTER_USER_FUNCTION(GetSchedule, 2);
		REGISTER_USER_FUNCTION(GetDrawHour, 3);
		REGISTER_USER_FUNCTION(GetFees, 4);
		REGISTER_USER_FUNCTION(GetQHeartHoldLimit, 5);
		REGISTER_USER_FUNCTION(GetQHeartWallet, 6);
		REGISTER_USER_FUNCTION(GetWinningDigits, 7);
		REGISTER_USER_FUNCTION(GetBalance, 8);
		REGISTER_USER_FUNCTION(GetWinners, 9);
		REGISTER_USER_FUNCTION(GetAutoParticipation, 10);
		REGISTER_USER_FUNCTION(GetAutoStats, 11);

		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		REGISTER_USER_PROCEDURE(SetSchedule, 3);
		REGISTER_USER_PROCEDURE(SetDrawHour, 4);
		REGISTER_USER_PROCEDURE(SetFees, 5);
		REGISTER_USER_PROCEDURE(SetQHeartHoldLimit, 6);
		REGISTER_USER_PROCEDURE(BuyRandomTickets, 7);
		REGISTER_USER_PROCEDURE(DepositAutoParticipation, 8);
		REGISTER_USER_PROCEDURE(WithdrawAutoParticipation, 9);
		REGISTER_USER_PROCEDURE(SetAutoConfig, 10);
		REGISTER_USER_PROCEDURE(SetAutoLimits, 11);
	}

	INITIALIZE()
	{
		state.teamAddress = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L,
		                       _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);

		state.ticketPrice = PULSE_TICKET_PRICE_DEFAULT;
		state.devPercent = PULSE_DEFAULT_DEV_PERCENT;
		state.burnPercent = PULSE_DEFAULT_BURN_PERCENT;
		state.shareholdersPercent = PULSE_DEFAULT_SHAREHOLDERS_PERCENT;
		state.qheartPercent = PULSE_DEFAULT_QHEART_PERCENT;
		state.qheartHoldLimit = PULSE_DEFAULT_QHEART_HOLD_LIMIT;

		state.schedule = PULSE_DEFAULT_SCHEDULE;
		state.drawHour = PULSE_DEFAULT_DRAW_HOUR;
		state.lastDrawDateStamp = PULSE_DEFAULT_INIT_TIME;

		state.maxAutoTicketsPerUser = PULSE_DEFAULT_MAX_AUTO_TICKETS_PER_USER;

		enableBuyTicket(state, false);
	}

	BEGIN_EPOCH_WITH_LOCALS()
	{
		if (state.schedule == 0)
		{
			state.schedule = PULSE_DEFAULT_SCHEDULE;
		}
		if (state.drawHour == 0)
		{
			state.drawHour = PULSE_DEFAULT_DRAW_HOUR;
		}

		makeDateStamp(qpi.year(), qpi.month(), qpi.day(), state.lastDrawDateStamp);
		enableBuyTicket(state, state.lastDrawDateStamp != PULSE_DEFAULT_INIT_TIME);
		if (state.lastDrawDateStamp != PULSE_DEFAULT_INIT_TIME)
		{
			CALL(ProcessAutoTickets, locals.autoTicketsInput, locals.autoTicketsOutput);
		}
	}

	END_EPOCH()
	{
		enableBuyTicket(state, false);
		clearStateOnEndEpoch(state);
		state.nextEpochData.apply(state);
		state.nextEpochData.clear();
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

		if (locals.currentHour < state.drawHour)
		{
			return;
		}

		makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);

		if (locals.currentDateStamp == PULSE_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, false);
			state.lastDrawDateStamp = PULSE_DEFAULT_INIT_TIME;
			return;
		}

		if (state.lastDrawDateStamp == PULSE_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, true);
			CALL(ProcessAutoTickets, locals.autoTicketsInput, locals.autoTicketsOutput);
			if (locals.isWednesday)
			{
				state.lastDrawDateStamp = locals.currentDateStamp;
			}
			else
			{
				state.lastDrawDateStamp = 0;
			}
		}

		if (state.lastDrawDateStamp == locals.currentDateStamp)
		{
			return;
		}

		locals.isScheduledToday = ((state.schedule & (1u << locals.currentDayOfWeek)) != 0);
		if (!locals.isWednesday && !locals.isScheduledToday)
		{
			return;
		}

		state.lastDrawDateStamp = locals.currentDateStamp;
		enableBuyTicket(state, false);

		CALL(SettleRound, locals.settleInput, locals.settleOutput);

		clearStateOnEndDraw(state);
		enableBuyTicket(state, !locals.isWednesday);
		if (!locals.isWednesday)
		{
			CALL(ProcessAutoTickets, locals.autoTicketsInput, locals.autoTicketsOutput);
		}
	}

	// Returns current ticket price in QHeart units.
	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	// Returns current draw schedule bitmask.
	PUBLIC_FUNCTION(GetSchedule) { output.schedule = state.schedule; }
	// Returns draw hour in UTC.
	PUBLIC_FUNCTION(GetDrawHour) { output.drawHour = state.drawHour; }
	// Returns QHeart balance cap retained by the contract.
	PUBLIC_FUNCTION(GetQHeartHoldLimit) { output.qheartHoldLimit = state.qheartHoldLimit; }
	// Returns the designated QHeart issuer wallet.
	PUBLIC_FUNCTION(GetQHeartWallet) { output.wallet = PULSE_QHEART_ISSUER; }
	// Returns digits from the last settled draw.
	PUBLIC_FUNCTION(GetWinningDigits) { output.digits = state.lastWinningDigits; }

	// Returns current fee split configuration.
	PUBLIC_FUNCTION(GetFees)
	{
		output.devPercent = state.devPercent;
		output.burnPercent = state.burnPercent;
		output.shareholdersPercent = state.shareholdersPercent;
		output.qheartPercent = state.qheartPercent;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Returns contract QHeart balance held in the Pulse wallet.
	PUBLIC_FUNCTION(GetBalance)
	{
		output.balance = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, SELF_INDEX, SELF_INDEX);
	}

	// Returns the winners ring buffer and total winners counter.
	PUBLIC_FUNCTION(GetWinners)
	{
		output.winners = state.winners;
		getWinnerCounter(state, output.winnersCounter);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/// Returns auto-participation settings for the invocator.
	/// @return Current deposit, config fields, and status code.
	PUBLIC_FUNCTION_WITH_LOCALS(GetAutoParticipation)
	{
		if (!state.autoParticipants.get(qpi.invocator(), locals.entry))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		output.deposit = locals.entry.deposit;
		output.desiredTickets = locals.entry.desiredTickets;

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/// Returns global auto-participation limits and counters.
	/// @return Current counters, limits, and status code.
	PUBLIC_FUNCTION(GetAutoStats)
	{
		output.autoParticipantsCounter = static_cast<uint16>(state.autoParticipants.population());
		output.maxAutoTicketsPerUser = state.maxAutoTicketsPerUser;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Schedules a new ticket price for the next epoch (owner-only).
	PUBLIC_PROCEDURE(SetPrice)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newPrice == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		state.nextEpochData.hasNewPrice = true;
		state.nextEpochData.newPrice = input.newPrice;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Schedules a new draw schedule bitmask for the next epoch (owner-only).
	PUBLIC_PROCEDURE(SetSchedule)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newSchedule == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.hasNewSchedule = true;
		state.nextEpochData.newSchedule = input.newSchedule;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Schedules a new draw hour in UTC for the next epoch (owner-only).
	PUBLIC_PROCEDURE(SetDrawHour)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newDrawHour > 23)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.hasNewDrawHour = true;
		state.nextEpochData.newDrawHour = input.newDrawHour;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Schedules new fee splits for the next epoch (owner-only).
	PUBLIC_PROCEDURE(SetFees)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.devPercent + input.burnPercent + input.shareholdersPercent + input.qheartPercent > 100)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.hasNewFee = true;
		state.nextEpochData.newDevPercent = input.devPercent;
		state.nextEpochData.newBurnPercent = input.burnPercent;
		state.nextEpochData.newShareholdersPercent = input.shareholdersPercent;
		state.nextEpochData.newQHeartPercent = input.qheartPercent;

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Schedules a new QHeart hold limit for the next epoch (owner-only).
	PUBLIC_PROCEDURE(SetQHeartHoldLimit)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		state.nextEpochData.hasNewQHeartHoldLimit = true;
		state.nextEpochData.newQHeartHoldLimit = input.newQHeartHoldLimit;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/** Deposits QHeart into the contract for automatic ticket purchases.
	 * @param amount QHeart amount to reserve for auto participation.
	 * @param desiredTickets Number of tickets to buy per draw
	 * @param buyNow When true, tries to buy immediately if selling is open.
	 * @return Status code describing the result.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositAutoParticipation)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (state.autoParticipants.population() >= state.autoParticipants.capacity())
		{
			output.returnCode = toReturnCode(EReturnCode::AUTO_PARTICIPANTS_FULL);
			return;
		}

		if (input.amount <= 0 || input.desiredTickets <= 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (state.maxAutoTicketsPerUser != 0)
		{
			input.desiredTickets = min(input.desiredTickets, state.maxAutoTicketsPerUser);
		}

		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		input.amount = min(locals.userBalance, input.amount);

		locals.totalPrice = smul(state.ticketPrice, static_cast<sint64>(input.desiredTickets));

		if (input.amount < locals.totalPrice)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		if (input.buyNow && isSellingOpen(state))
		{
			locals.buyRandomTicketsInput.count = input.desiredTickets;
			CALL(BuyRandomTickets, locals.buyRandomTicketsInput, locals.buyRandomTicketsOutput);
			if (locals.buyRandomTicketsOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
			{
				output.returnCode = locals.buyRandomTicketsOutput.returnCode;
				return;
			}

			input.buyNow = false;
			input.amount = input.amount - locals.totalPrice;

			// The entire deposit was spent
			if (input.amount <= 0)
			{
				output.returnCode = toReturnCode(EReturnCode::SUCCESS);
				return;
			}
		}

		state.autoParticipants.get(qpi.invocator(), locals.entry);
		locals.entry.player = qpi.invocator();

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(),
		                                                                qpi.invocator(), input.amount, SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TRANSFER_TO_PULSE_FAILED);
			return;
		}

		locals.entry.deposit = sadd(locals.entry.deposit, input.amount);
		if (input.desiredTickets > 0)
		{
			locals.entry.desiredTickets = input.desiredTickets;
		}

		state.autoParticipants.set(qpi.invocator(), locals.entry);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/// Withdraws QHeart from the invocator's auto-participation deposit.
	/// @param amount QHeart amount to withdraw; 0 withdraws the full deposit.
	/// @return Status code describing the result.
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawAutoParticipation)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!state.autoParticipants.contains(qpi.invocator()))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (!state.autoParticipants.get(qpi.invocator(), locals.entry))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.withdrawAmount = (input.amount <= 0) ? locals.entry.deposit : min(input.amount, locals.entry.deposit);

		if (locals.withdrawAmount == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.transferResult =
		    qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, locals.withdrawAmount, qpi.invocator());
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TRANSFER_FROM_PULSE_FAILED);
			return;
		}

		locals.entry.deposit -= locals.withdrawAmount;

		if (locals.entry.deposit <= 0)
		{
			state.autoParticipants.removeByKey(qpi.invocator());
			state.autoParticipants.cleanupIfNeeded(PULSE_CLEANUP_THRESHOLD);
		}
		else
		{
			state.autoParticipants.set(qpi.invocator(), locals.entry);
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/// Sets auto-participation config for the invocator.
	/// @param desiredTickets Signed: -1 ignore, >0 set new value.
	/// @param minTicketsToBuy Signed: -1 ignore, 0 disable, >0 set new value.
	/// @return Status code describing the result.
	PUBLIC_PROCEDURE_WITH_LOCALS(SetAutoConfig)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (input.desiredTickets < -1)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (!state.autoParticipants.contains(qpi.invocator()))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (input.desiredTickets > 0 && state.maxAutoTicketsPerUser != 0)
		{
			input.desiredTickets = min(input.desiredTickets, static_cast<sint16>(state.maxAutoTicketsPerUser));
		}

		state.autoParticipants.get(qpi.invocator(), locals.entry);

		locals.desiredValue = locals.entry.desiredTickets;

		if (input.desiredTickets != -1)
		{
			if (input.desiredTickets <= 0)
			{
				output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
				return;
			}
			locals.desiredValue = static_cast<uint16>(input.desiredTickets);
		}

		locals.entry.desiredTickets = locals.desiredValue;

		if (locals.entry.deposit == 0)
		{
			locals.removedIndex = state.autoParticipants.removeByKey(qpi.invocator());
			state.autoParticipants.cleanupIfNeeded(PULSE_CLEANUP_THRESHOLD);
		}
		else
		{
			state.autoParticipants.set(qpi.invocator(), locals.entry);
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/// Sets auto-participation limits (owner-only).
	/// @param maxTicketsPerUser Max tickets per user; 0 disables the limit.
	/// @param maxDepositPerUser Max deposit per user; 0 disables the limit.
	/// @return Status code describing the result.
	PUBLIC_PROCEDURE(SetAutoLimits)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != PULSE_QHEART_ISSUER)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.maxTicketsPerUser != 0 && input.maxTicketsPerUser > PULSE_MAX_NUMBER_OF_PLAYERS)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.maxAutoTicketsPerUser = input.maxTicketsPerUser;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Buys a single ticket; transfers ticket price from invocator.
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		locals.validateInput.digits = input.digits;
		CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
		if (!locals.validateOutput.isValid)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_NUMBERS);
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		if (locals.slotsLeft == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < state.ticketPrice)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(),
		                                                                qpi.invocator(), state.ticketPrice, SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.ticket.player = qpi.invocator();
		locals.ticket.digits = input.digits;
		state.tickets.set(state.ticketCounter, locals.ticket);
		state.ticketCounter = min(static_cast<uint64>(state.ticketCounter) + 1ull, state.tickets.capacity());

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Buys multiple random tickets; transfers total price from invocator.
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyRandomTickets)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		locals.prepareInput.count = input.count;
		CALL(PrepareRandomTickets, locals.prepareInput, locals.prepareOutput);
		if (locals.prepareOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			output.returnCode = locals.prepareOutput.returnCode;
			return;
		}

		locals.chargeInput.player = qpi.invocator();
		locals.chargeInput.count = locals.prepareOutput.count;
		CALL(ChargeTicketsFromPlayer, locals.chargeInput, locals.chargeOutput);
		if (locals.chargeOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
		{
			output.returnCode = locals.chargeOutput.returnCode;
			return;
		}

		locals.allocateInput.player = qpi.invocator();
		locals.allocateInput.count = locals.prepareOutput.count;
		CALL(AllocateRandomTickets, locals.allocateInput, locals.allocateOutput);

		output.returnCode = locals.allocateOutput.returnCode;
	}

private:
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessAutoTickets)
	{
		if (!isSellingOpen(state) || state.autoParticipants.population() == 0)
		{
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		if (locals.slotsLeft == 0)
		{
			return;
		}

		locals.currentIndex = state.autoParticipants.nextElementIndex(NULL_INDEX);
		while (locals.currentIndex != NULL_INDEX)
		{
			locals.slotsLeft = getSlotsLeft(state);
			if (locals.slotsLeft == 0)
			{
				break;
			}

			locals.entry = state.autoParticipants.value(locals.currentIndex);

			locals.affordable = div<sint64>(locals.entry.deposit, state.ticketPrice);
			if (locals.affordable == 0)
			{
				state.autoParticipants.removeByIndex(locals.currentIndex);
				locals.currentIndex = state.autoParticipants.nextElementIndex(locals.currentIndex);
				continue;
			}

			locals.toBuy = static_cast<sint64>(min(locals.affordable, static_cast<uint64>(locals.entry.desiredTickets)));
			locals.toBuy = min(locals.toBuy, locals.slotsLeft);
			if (locals.toBuy <= 0)
			{
				locals.currentIndex = state.autoParticipants.nextElementIndex(locals.currentIndex);
				continue;
			}

			locals.allocateInput.player = locals.entry.player;
			locals.allocateInput.count = static_cast<uint16>(locals.toBuy);
			CALL(AllocateRandomTickets, locals.allocateInput, locals.allocateOutput);
			if (locals.allocateOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
			{
				locals.currentIndex = state.autoParticipants.nextElementIndex(locals.currentIndex);
				continue;
			}

			locals.entry.deposit -= smul(locals.toBuy, state.ticketPrice);
			if (locals.entry.deposit <= 0)
			{
				state.autoParticipants.removeByIndex(locals.currentIndex);
			}
			else
			{
				state.autoParticipants.set(locals.entry.player, locals.entry);
			}

			locals.currentIndex = state.autoParticipants.nextElementIndex(locals.currentIndex);
		}

		state.autoParticipants.cleanupIfNeeded(PULSE_CLEANUP_THRESHOLD);
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateDigits)
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

	PRIVATE_FUNCTION_WITH_LOCALS(GetRandomDigits)
	{
		// Derive each digit independently to avoid shared PRNG state.
		for (locals.index = 0; locals.index < PULSE_WINNING_DIGITS; ++locals.index)
		{
			deriveOne(input.seed, locals.index, locals.tempValue);
			locals.candidate = static_cast<uint8>(mod(locals.tempValue, static_cast<uint64>(PULSE_MAX_DIGIT + 1)));

			output.digits.set(locals.index, locals.candidate);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(SettleRound)
	{
		if (state.ticketCounter == 0)
		{
			return;
		}

		locals.roundRevenue = smul(state.ticketPrice, state.ticketCounter);
		locals.devAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.devPercent)), 100LL);
		locals.burnAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.burnPercent)), 100LL);
		locals.shareholdersAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.shareholdersPercent)), 100LL);
		locals.qheartAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.qheartPercent)), 100LL);

		if (locals.devAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, locals.devAmount, state.teamAddress);
		}
		if (locals.shareholdersAmount > 0)
		{
			locals.shareholdersAsset.issuer = id::zero();
			locals.shareholdersAsset.assetName = PULSE_CONTRACT_ASSET_NAME;
			locals.shareholdersTotalShares = NUMBER_OF_COMPUTORS;

			locals.shareholdersDividendPerShare = div<sint64>(locals.shareholdersAmount, locals.shareholdersTotalShares);
			if (locals.shareholdersDividendPerShare > 0)
			{
				locals.shareholdersIter.begin(locals.shareholdersAsset);
				while (!locals.shareholdersIter.reachedEnd())
				{
					locals.shareholdersHolderShares = locals.shareholdersIter.numberOfPossessedShares();
					if (locals.shareholdersHolderShares > 0)
					{
						qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF,
						                                        smul(locals.shareholdersHolderShares, locals.shareholdersDividendPerShare),
						                                        locals.shareholdersIter.possessor());
					}
					locals.shareholdersIter.next();
				}
			}
		}
		if (locals.burnAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, locals.burnAmount, NULL_ID);
		}
		if (locals.qheartAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, locals.qheartAmount,
			                                        PULSE_QHEART_ISSUER);
		}

		locals.mixedSpectrumValue = qpi.getPrevSpectrumDigest();
		locals.randomSeed = qpi.K12(locals.mixedSpectrumValue).u64._0;
		locals.randomInput.seed = locals.randomSeed;
		CALL(GetRandomDigits, locals.randomInput, locals.randomOutput);
		state.lastWinningDigits = locals.randomOutput.digits;

		locals.balanceSigned = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, SELF_INDEX, SELF_INDEX);
		locals.balance = max(locals.balanceSigned, 0LL);

		locals.totalPrize = 0;
		for (locals.i = 0; locals.i < state.ticketCounter; ++locals.i)
		{
			locals.ticket = state.tickets.get(locals.i);
			locals.prize = computePrize(state, locals.ticket, state.lastWinningDigits, locals.computePrizeLocals);
			locals.totalPrize += locals.prize;
		}

		locals.availableBalance = locals.balance;
		for (locals.i = 0; locals.i < state.ticketCounter; ++locals.i)
		{
			locals.ticket = state.tickets.get(locals.i);
			locals.prize = computePrize(state, locals.ticket, state.lastWinningDigits, locals.computePrizeLocals);

			if (locals.totalPrize > 0 && locals.availableBalance < locals.totalPrize)
			{
				// Pro-rate payouts when the contract balance cannot cover all prizes.
				locals.prize = div<uint64>(smul(static_cast<sint64>(locals.prize), static_cast<sint64>(locals.availableBalance)),
				                           static_cast<sint64>(locals.totalPrize));
			}

			if (locals.prize > 0 && locals.balance >= locals.prize)
			{
				qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, static_cast<sint64>(locals.prize),
				                                        locals.ticket.player);
				locals.balance -= locals.prize;

				locals.fillWinnersInfoInput.winnerAddress = locals.ticket.player;
				locals.fillWinnersInfoInput.revenue = locals.prize;
				CALL(FillWinnersInfo, locals.fillWinnersInfoInput, locals.fillWinnersInfoOutput);
			}
		}

		locals.balanceSigned = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, SELF_INDEX, SELF_INDEX);
		locals.balance = (locals.balanceSigned > 0) ? static_cast<uint64>(locals.balanceSigned) : 0;

		if (state.qheartHoldLimit > 0 && locals.balance > state.qheartHoldLimit)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF,
			                                        static_cast<sint64>(locals.balance - state.qheartHoldLimit), PULSE_QHEART_ISSUER);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(FillWinnersInfo)
	{
		if (input.winnerAddress == id::zero())
		{
			return;
		}

		getWinnerCounter(state, locals.insertIdx);
		++state.winnersCounter;

		locals.winnerInfo.winnerAddress = input.winnerAddress;
		locals.winnerInfo.revenue = input.revenue;
		locals.winnerInfo.epoch = qpi.epoch();

		state.winners.set(locals.insertIdx, locals.winnerInfo);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(PrepareRandomTickets)
	{
		if (input.count == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			output.count = 0;
			return;
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_SELLING_CLOSED);
			output.count = 0;
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		output.count = min(input.count, static_cast<uint16>(locals.slotsLeft));
		if (output.count == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ChargeTicketsFromPlayer)
	{
		if (input.count == 0 || input.player == id::zero())
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.totalPrice = smul(static_cast<sint64>(input.count), state.ticketPrice);
		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, input.player, input.player, SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < static_cast<sint64>(locals.totalPrice))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, input.player, input.player,
		                                                                static_cast<sint64>(locals.totalPrice), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TRANSFER_TO_PULSE_FAILED);
			return;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AllocateRandomTickets)
	{
		if (input.count == 0 || input.player == id::zero())
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		locals.slotsLeft = getSlotsLeft(state);
		if (locals.slotsLeft < input.count)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		locals.mixedSpectrumValue = qpi.getPrevSpectrumDigest();
		locals.randomSeed = qpi.K12(locals.mixedSpectrumValue).u64._0;
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			deriveOne(locals.randomSeed, locals.i, locals.tempSeed);
			locals.randomInput.seed = locals.tempSeed;
			CALL(GetRandomDigits, locals.randomInput, locals.randomOutput);

			locals.ticket.player = input.player;
			locals.ticket.digits = locals.randomOutput.digits;
			state.tickets.set(state.ticketCounter, locals.ticket);
			state.ticketCounter = min(static_cast<uint64>(state.ticketCounter) + 1ULL, state.tickets.capacity());
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

public:
	// Encodes YYYY/MM/DD into a compact sortable date stamp.
	static void makeDateStamp(uint8 year, uint8 month, uint8 day, uint32& res) { res = static_cast<uint32>(year << 9 | month << 5 | day); }

	template<typename T> static constexpr T min(const T& a, const T& b) { return (a < b) ? a : b; }
	template<typename T> static constexpr T max(const T& a, const T& b) { return a > b ? a : b; }

	// Per-index mix to deterministically expand a single seed.
	static void deriveOne(const uint64& r, const uint64& idx, uint64& outValue) { mix64(r + 0x9e3779b97f4a7c15ULL * (idx + 1), outValue); }

	static void mix64(const uint64& x, uint64& outValue)
	{
		outValue = x;
		outValue ^= outValue >> 30;
		outValue *= 0xbf58476d1ce4e5b9ULL;
		outValue ^= outValue >> 27;
		outValue *= 0x94d049bb133111ebULL;
		outValue ^= outValue >> 31;
	}

	static sint64 getSlotsLeft(const PULSE& state)
	{
		return state.ticketCounter < state.tickets.capacity() ? (state.tickets.capacity() - state.ticketCounter) : 0;
	}

protected:
	// Ring buffer of recent winners; index is winnersCounter % capacity.
	Array<WinnerInfo, PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
	// Tickets for the current round; valid range is [0, ticketCounter).
	Array<Ticket, PULSE_MAX_NUMBER_OF_PLAYERS> tickets;
	// Auto-buy participants keyed by user id.
	HashMap<id, AutoParticipant, PULSE_MAX_NUMBER_OF_AUTO_PARTICIPANTS> autoParticipants;
	// Last settled winning digits; undefined before the first draw.
	Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> lastWinningDigits;
	sint64 ticketCounter;
	sint64 ticketPrice;
	// Per-user auto-purchase limits; 0 means unlimited.
	sint16 maxAutoTicketsPerUser;
	// Contract balance above this cap is swept to the QHeart wallet after settlement.
	uint64 qheartHoldLimit;
	// Date stamp of the most recent draw; PULSE_DEFAULT_INIT_TIME is a bootstrap sentinel.
	uint32 lastDrawDateStamp;
	uint8 devPercent;
	uint8 burnPercent;
	uint8 shareholdersPercent;
	uint8 qheartPercent;
	uint8 schedule;
	uint8 drawHour;
	EState currentState;
	id teamAddress;
	NextEpochData nextEpochData;
	// Monotonic winner count used to rotate the winners ring buffer.
	uint64 winnersCounter;

protected:
	static void clearStateOnEndEpoch(PULSE& state)
	{
		clearStateOnEndDraw(state);
		state.lastDrawDateStamp = 0;
	}

	static void clearStateOnEndDraw(PULSE& state)
	{
		state.ticketCounter = 0;
		setMemory(state.tickets, 0);
	}

	static void enableBuyTicket(PULSE& state, bool bEnable)
	{
		state.currentState = bEnable ? state.currentState | EState::SELLING : state.currentState & ~EState::SELLING;
	}

	static bool isSellingOpen(const PULSE& state) { return (state.currentState & EState::SELLING) != 0; }

	static void getWinnerCounter(const PULSE& state, uint64& outCounter) { outCounter = mod(state.winnersCounter, state.winners.capacity()); }

	static uint64 getLeftAlignedReward(const PULSE& state, uint8 matches)
	{
		switch (matches)
		{
			case 6: return 2000 * state.ticketPrice;
			case 5: return 300 * state.ticketPrice;
			case 4: return 60 * state.ticketPrice;
			case 3: return 20 * state.ticketPrice;
			case 2: return 4 * state.ticketPrice;
			case 1: return 1 * state.ticketPrice;
			default: return 0;
		}
	}

	static uint64 getAnyPositionReward(const PULSE& state, uint8 matches)
	{
		switch (matches)
		{
			case 6: return 150 * state.ticketPrice;
			case 5: return 30 * state.ticketPrice;
			case 4: return 8 * state.ticketPrice;
			case 3: return 2 * state.ticketPrice;
			case 2:
			case 1:
			default: return 0;
		}
	}

	static uint64 computePrize(const PULSE& state, const Ticket& ticket, const Array<uint8, PULSE_WINNING_DIGITS_ALIGNED>& winningDigits,
	                           ComputePrize_locals& locals)
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
			locals.anyPositionMatches += min<uint8>(locals.ticketCounts.get(locals.digitValue), locals.winningCounts.get(locals.digitValue));
		}

		// Reward the best of left-aligned or any-position matches to avoid double counting.
		locals.leftAlignedReward = getLeftAlignedReward(state, locals.leftAlignedMatches);
		locals.anyPositionReward = getAnyPositionReward(state, locals.anyPositionMatches);
		locals.prize = max(locals.leftAlignedReward, locals.anyPositionReward);
		return locals.prize;
	}
};
