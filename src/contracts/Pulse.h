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

const id PULSE_QHEART_ISSUER = ID(_S, _S, _G, _X, _S, _L, _S, _X, _F, _E, _J, _O, _O, _B, _T, _Z, _W, _V, _D, _S, _R, _C, _E, _F, _G, _X, _N, _D, _Y,
                                  _U, _V, _D, _X, _M, _Q, _A, _L, _X, _L, _B, _X, _G, _D, _C, _R, _X, _T, _K, _F, _Z, _I, _O, _T, _G, _Z, _F);
constexpr uint64 PULSE_CONTRACT_ASSET_NAME = 297750254928ULL; // "PULSE"

struct PULSE2
{
};

struct PULSE : public ContractBase
{
public:
	enum class EState : uint8
	{
		SELLING = 1 << 0,
	};

	friend EState operator|(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
	friend EState operator&(const EState& a, const EState& b) { return static_cast<EState>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
	friend EState operator~(const EState& a) { return static_cast<EState>(~static_cast<uint8>(a)); }
	template<typename T> friend bool operator==(const EState& a, const T& b) { return static_cast<uint8>(a) == b; }
	template<typename T> friend bool operator!=(const EState& a, const T& b) { return !(a == b); }

	enum class EReturnCode : uint8
	{
		SUCCESS,
		TICKET_INVALID_PRICE,
		TICKET_ALL_SOLD_OUT,
		TICKET_SELLING_CLOSED,
		INVALID_NUMBERS,
		ACCESS_DENIED,
		INVALID_VALUE,
		UNKNOWN_ERROR = UINT8_MAX
	};

	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); };

	struct Ticket
	{
		id player;
		Array<uint8, PULSE_PLAYER_DIGITS_ALIGNED> digits;
	};

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
		uint64 reward;
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
		uint64 reward;
		uint64 slotsLeft;
		sint64 userBalance;
		sint64 transferResult;
		uint64 totalPrice;
		m256i mixedSpectrumValue;
		uint64 randomSeed;
		uint64 tempSeed;
		uint16 i;
		Ticket ticket;
		GetRandomDigits_input randomInput;
		GetRandomDigits_output randomOutput;
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
		uint64 prize;
		uint64 totalPrize;
		uint64 availableBalance;
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

		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		REGISTER_USER_PROCEDURE(SetSchedule, 3);
		REGISTER_USER_PROCEDURE(SetDrawHour, 4);
		REGISTER_USER_PROCEDURE(SetFees, 5);
		REGISTER_USER_PROCEDURE(SetQHeartHoldLimit, 6);
		REGISTER_USER_PROCEDURE(BuyRandomTickets, 7);
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

		enableBuyTicket(state, false);
	}

	BEGIN_EPOCH()
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
	}

	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetSchedule) { output.schedule = state.schedule; }
	PUBLIC_FUNCTION(GetDrawHour) { output.drawHour = state.drawHour; }
	PUBLIC_FUNCTION(GetQHeartHoldLimit) { output.qheartHoldLimit = state.qheartHoldLimit; }
	PUBLIC_FUNCTION(GetQHeartWallet) { output.wallet = PULSE_QHEART_ISSUER; }
	PUBLIC_FUNCTION(GetWinningDigits) { output.digits = state.lastWinningDigits; }

	PUBLIC_FUNCTION(GetFees)
	{
		output.devPercent = state.devPercent;
		output.burnPercent = state.burnPercent;
		output.shareholdersPercent = state.shareholdersPercent;
		output.qheartPercent = state.qheartPercent;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION(GetBalance)
	{
		output.balance = qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, SELF_INDEX, SELF_INDEX);
	}

	PUBLIC_FUNCTION(GetWinners)
	{
		output.winners = state.winners;
		getWinnerCounter(state, output.winnersCounter);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

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

	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		locals.reward = qpi.invocationReward();
		if (locals.reward > 0)
		{
			qpi.transfer(qpi.invocator(), locals.reward);
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

		locals.slotsLeft = (state.ticketCounter < state.tickets.capacity()) ? (state.tickets.capacity() - state.ticketCounter) : 0;
		if (locals.slotsLeft == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < static_cast<sint64>(state.ticketPrice))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(),
		                                                                qpi.invocator(), static_cast<sint64>(state.ticketPrice), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.ticket.player = qpi.invocator();
		locals.ticket.digits = input.digits;
		state.tickets.set(state.ticketCounter, locals.ticket);
		state.ticketCounter = min(state.ticketCounter + 1, state.tickets.capacity());

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(BuyRandomTickets)
	{
		locals.reward = qpi.invocationReward();
		if (locals.reward > 0)
		{
			qpi.transfer(qpi.invocator(), locals.reward);
		}

		if (!isSellingOpen(state))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		if (input.count == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.slotsLeft = (state.ticketCounter < state.tickets.capacity()) ? (state.tickets.capacity() - state.ticketCounter) : 0;
		if (locals.slotsLeft < input.count)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_ALL_SOLD_OUT);
			return;
		}

		locals.totalPrice = smul(static_cast<uint64>(input.count), state.ticketPrice);
		locals.userBalance =
		    qpi.numberOfPossessedShares(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.userBalance < static_cast<sint64>(locals.totalPrice))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.transferResult = qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, qpi.invocator(),
		                                                                qpi.invocator(), static_cast<sint64>(locals.totalPrice), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}

		locals.mixedSpectrumValue = qpi.getPrevSpectrumDigest();
		locals.randomSeed = qpi.K12(locals.mixedSpectrumValue).u64._0;
		for (locals.i = 0; locals.i < input.count; ++locals.i)
		{
			deriveOne(locals.randomSeed, locals.i, locals.tempSeed);
			locals.randomInput.seed = locals.tempSeed;
			CALL(GetRandomDigits, locals.randomInput, locals.randomOutput);

			locals.ticket.player = qpi.invocator();
			locals.ticket.digits = locals.randomOutput.digits;
			state.tickets.set(state.ticketCounter, locals.ticket);
			state.ticketCounter = min(state.ticketCounter + 1, state.tickets.capacity());
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

private:
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

		locals.roundRevenue = static_cast<sint64>(smul(state.ticketPrice, state.ticketCounter));
		locals.devAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.devPercent)), 100LL);
		locals.burnAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.burnPercent)), 100LL);
		locals.shareholdersAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.shareholdersPercent)), 100LL);
		locals.qheartAmount = div<sint64>(smul(locals.roundRevenue, static_cast<sint64>(state.qheartPercent)), 100LL);

		if (locals.devAmount > 0)
		{
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, static_cast<sint64>(locals.devAmount),
			                                        state.teamAddress);
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
			qpi.transferShareOwnershipAndPossession(PULSE_QHEART_ASSET_NAME, PULSE_QHEART_ISSUER, SELF, SELF, static_cast<sint64>(locals.burnAmount),
			                                        NULL_ID);
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
		locals.balance = (locals.balanceSigned > 0) ? static_cast<uint64>(locals.balanceSigned) : 0;

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

public:
	static void makeDateStamp(uint8 year, uint8 month, uint8 day, uint32& res) { res = static_cast<uint32>(year << 9 | month << 5 | day); }

	template<typename T> static constexpr T min(const T& a, const T& b) { return (a < b) ? a : b; }
	template<typename T> static constexpr T max(const T& a, const T& b) { return a > b ? a : b; }

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

protected:
	Array<WinnerInfo, PULSE_MAX_NUMBER_OF_WINNERS_IN_HISTORY> winners;
	Array<Ticket, PULSE_MAX_NUMBER_OF_PLAYERS> tickets;
	Array<uint8, PULSE_WINNING_DIGITS_ALIGNED> lastWinningDigits;
	uint64 ticketCounter;
	uint64 ticketPrice;
	uint64 qheartHoldLimit;
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

		locals.leftAlignedReward = getLeftAlignedReward(state, locals.leftAlignedMatches);
		locals.anyPositionReward = getAnyPositionReward(state, locals.anyPositionMatches);
		locals.prize = max(locals.leftAlignedReward, locals.anyPositionReward);
		return locals.prize;
	}
};
