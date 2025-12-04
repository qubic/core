using namespace QPI;

// --- Core game parameters ----------------------------------------------------
static constexpr uint64 QTF_MAX_NUMBER_OF_PLAYERS = 1024;
static constexpr uint64 QTF_RANDOM_VALUES_COUNT = 4;
static constexpr uint64 QTF_MAX_RANDOM_VALUE = 30;
static constexpr uint64 QTF_TICKET_PRICE = 1000000;

// Baseline split for k2/k3 when FR is OFF (spec leaves this open; choose 50/50 as default).
static constexpr uint64 QTF_BASE_K3_SHARE_BP = 5000; // basis points of winners block (50.00%).

// --- Fast-Recovery (FR) parameters (spec defaults) --------------------------
// Fast-Recovery base redirect percentages (always active when FR=ON)
static constexpr uint64 QTF_FR_DEV_REDIRECT_BP = 100;  // 1.00% of R (base redirect, always applied)
static constexpr uint64 QTF_FR_DIST_REDIRECT_BP = 100; // 1.00% of R (base redirect, always applied)

// Deficit-driven extra redirect parameters (dynamic, no hard N threshold)
// The extra redirect is calculated based on:
//   - Deficit: Δ = max(0, targetJackpot - currentJackpot)
//   - Expected rounds to k=4: E_k4(N) = 1 / (1 - (1-p4)^N)
//   - Horizon: H = min(E_k4(N), R_goal_rounds_cap)
//   - Needed gain: g_need = max(0, Δ/H - baseGain)
//   - Extra percentage: extra_pp = clamp(g_need / R, 0, extra_max)
//   - Split equally: dev_extra = dist_extra = extra_pp / 2
static constexpr uint64 QTF_FR_EXTRA_MAX_BP = 70;        // Maximum extra redirect: 0.70% of R total (0.35% each Dev/Dist)
static constexpr uint64 QTF_FR_GOAL_ROUNDS_CAP = 100;    // Cap on expected rounds horizon H for deficit calculation
static constexpr uint64 QTF_FIXED_POINT_SCALE = 1000000; // Scale for fixed-point arithmetic (6 decimals precision)

// Probability constants for k=4 win (exact combinatorics: 4-of-30)
// p4 = C(4,4) * C(26,0) / C(30,4) = 1 / 27405
static constexpr uint64 QTF_P4_DENOMINATOR = 27405;   // Denominator for k=4 probability (1/27405)
static constexpr uint64 QTF_FR_WINNERS_RAKE_BP = 500; // 5% of winners block from k3
static constexpr uint64 QTF_FR_K3_SHARE_BP = 3500;    // 35% of win_eff to k3
static constexpr uint64 QTF_FR_K2_SHARE_BP = 6500;    // 65% of win_eff to k2
static constexpr uint64 QTF_FR_ALPHA_BP = 500;        // alpha = 0.05 -> 95% overflow to jackpot
static constexpr uint16 QTF_FR_POST_K4_WINDOW_ROUNDS = 50;
static constexpr uint16 QTF_FR_HYSTERESIS_ROUNDS = 3;

// --- Floors and reserve safety ----------------------------------------------
static constexpr uint64 QTF_K2_FLOOR_MULT = 1; // numerator for 0.5 * P (we divide by 2)
static constexpr uint64 QTF_K2_FLOOR_DIV = 2;
static constexpr uint64 QTF_K3_FLOOR_MULT = 5;              // 5 * P
static constexpr uint64 QTF_TOPUP_PER_WINNER_CAP_MULT = 25; // 25 * P
static constexpr uint64 QTF_TOPUP_RESERVE_PCT_BP = 1000;    // 10% of reserve per round
static constexpr uint64 QTF_RESERVE_SOFT_FLOOR_MULT = 20;   // keep at least 20 * P in reserve

// Baseline overflow split (reserve share in basis points). If spec is updated, adjust here.
static constexpr uint64 QTF_BASELINE_OVERFLOW_ALPHA_BP = 5000; // 50% reserve / 50% jackpot

// Reserve split between JackpotRebuild and General (50/50 default)
static constexpr uint64 QTF_RESERVE_SPLIT_JACKPOT_BP = 5000; // 50% to JackpotRebuild, 50% to General

// Default fee percentages (fallback if RL::GetFees fails)
static constexpr uint64 QTF_DEFAULT_DEV_PERCENT = 10;
static constexpr uint64 QTF_DEFAULT_DIST_PERCENT = 20;
static constexpr uint64 QTF_DEFAULT_BURN_PERCENT = 2;
static constexpr uint64 QTF_DEFAULT_WINNERS_PERCENT = 68;

// Maximum attempts to generate unique random value before fallback
static constexpr uint8 QTF_MAX_RANDOM_GENERATION_ATTEMPTS = 100;

static constexpr uint8 QTF_DEFAULT_SCHEDULE = 1u << WEDNESDAY;
static constexpr uint8 QTF_DEFAULT_DRAW_HOUR = 11;                        // 11:00 UTC
static constexpr uint32 QTF_DEFAULT_INIT_TIME = 22u << 9 | 4u << 5 | 13u; // RL_DEFAULT_INIT_TIME

static const id QTF_ADDRESS_DEV_TEAM =
    ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R, _L, _W, _E, _T, _H, _N, _G,
       _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
static const id QTF_RANDOM_LOTTERY_CONTRACT_ID = id(RL_CONTRACT_INDEX, 0, 0, 0);
static const uint64 QTF_RANDOM_LOTTERY_ASSET_NAME = *reinterpret_cast<const uint64*>("RL");
static const id QTF_RESERVE_POOL_CONTRACT_ID = id(QRP_CONTRACT_INDEX, 0, 0, 0);

using QTFRandomValues = Array<uint8, QTF_RANDOM_VALUES_COUNT>;
using QFTWinnerPlayers = Array<id, QTF_MAX_NUMBER_OF_PLAYERS>;

struct QTF2
{
};

struct QTF : public ContractBase
{
public:
	enum class EReturnCode : uint8
	{
		SUCCESS,
		INVALID_TICKET_PRICE,
		MAX_PLAYERS_REACHED,
		ACCESS_DENIED,
		INVALID_NUMBERS,
		INVALID_VALUE,
		TICKET_SELLING_CLOSED,

		MAX_VALUE = UINT8_MAX
	};

	enum EState : uint8
	{
		STATE_NONE = 0,
		STATE_SELLING = 1 << 0
	};

	struct PlayerData
	{
		id player;
		QTFRandomValues randomValues;
	};

	struct WinnerData
	{
		Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> winners;
		QTFRandomValues winnerValues;
		uint64 winnerCounter;
		uint16 epoch;
	};

	struct NextEpochData
	{
	public:
		void clear() { setMemory(*this, 0); }

		void apply(QTF& state) const
		{
			if (newTicketPrice > 0)
			{
				state.ticketPrice = newTicketPrice;
			}
			if (newTargetJackpot > 0)
			{
				state.targetJackpot = newTargetJackpot;
			}
			if (newSchedule > 0)
			{
				state.schedule = newSchedule;
			}
			if (newDrawHour > 0)
			{
				state.drawHour = newDrawHour;
			}
		}

	public:
		uint64 newTicketPrice;
		uint64 newTargetJackpot;
		uint8 newSchedule;
		uint8 newDrawHour;
	};

	struct PoolsSnapshot
	{
		uint64 jackpot;
		uint64 reserveGeneral;
		uint64 reserveJackpot;
		uint64 targetJackpot;
		uint8 frActive;
		uint16 roundsSinceK4;
	};

	struct PoolsSnapshot_input
	{
	};

	struct PoolsSnapshot_output
	{
		PoolsSnapshot pools;
	};

	// ValidateNumbers: Check if all numbers are valid and unique
	struct ValidateNumbers_input
	{
		QTFRandomValues numbers; // Numbers to validate
	};
	struct ValidateNumbers_output
	{
		bit isValid; // true if all numbers valid and unique
	};
	struct ValidateNumbers_locals
	{
		HashSet<uint8, QTF_MAX_RANDOM_VALUE + 2> seen;
		uint8 idx;
		uint8 value;
	};

	// Buy Ticket
	struct BuyTicket_input
	{
		QTFRandomValues randomValues;
	};
	struct BuyTicket_output
	{
		EReturnCode returnCode;
	};
	struct BuyTicket_locals
	{
		// CALL parameters for ValidateNumbers
		ValidateNumbers_input validateInput;
		ValidateNumbers_output validateOutput;
	};

	// Set Price
	struct SetPrice_input
	{
		uint64 newPrice;
	};
	struct SetPrice_output
	{
		EReturnCode returnCode;
	};

	// Set Schedule
	struct SetSchedule_input
	{
		uint8 newSchedule;
	};
	struct SetSchedule_output
	{
		EReturnCode returnCode;
	};

	// Set draw hour
	struct SetDrawHour_input
	{
		uint8 newDrawHour;
	};
	struct SetDrawHour_output
	{
		EReturnCode returnCode;
	};

	// Set Target Carry (Jackpot target)
	struct SetTargetJackpot_input
	{
		uint64 newTargetJackpot;
	};
	struct SetTargetJackpot_output
	{
		EReturnCode returnCode;
	};

	// Return All Tickets (refund all players)
	struct ReturnAllTickets_input
	{
	};
	struct ReturnAllTickets_output
	{
	};
	struct ReturnAllTickets_locals
	{
		uint64 i; // Loop counter for mass-refund
	};

	// Check Contract Balance
	struct CheckContractBalance_input
	{
		uint64 expectedRevenue; // Expected revenue to compare against balance
	};
	struct CheckContractBalance_output
	{
		bit hasEnough;        // true if balance >= expectedRevenue
		uint64 actualBalance; // Current contract balance
	};
	struct CheckContractBalance_locals
	{
		Entity entity;
	};

	// Calculate Base Gain (FR base carry growth estimation)
	struct CalculateBaseGain_input
	{
		uint64 revenue;      // Round revenue (N * ticketPrice)
		uint64 winnersBlock; // 68% of revenue allocated to winners
	};
	struct CalculateBaseGain_output
	{
		uint64 baseGain; // Estimated carry gain in qu
	};
	struct CalculateBaseGain_locals
	{
		uint64 devRedirect;
		uint64 distRedirect;
		uint64 winnersRake;
		uint64 estimatedOverflow;
		uint64 overflowToCarry;
	};

	// PowerFixedPoint: Computes (base^exp) in fixed-point arithmetic
	struct PowerFixedPoint_input
	{
		uint64 base; // Base value in fixed-point (scaled by QTF_FIXED_POINT_SCALE)
		uint64 exp;  // Exponent (integer)
	};
	struct PowerFixedPoint_output
	{
		uint64 result; // base^exp in fixed-point
	};
	struct PowerFixedPoint_locals
	{
		uint64 tmpBase;
		uint64 expCopy; // Copy of exp (modified during computation)
	};

	// CalculateExpectedRoundsToK4: E_k4(N) = 1 / (1 - (1-p4)^N)
	struct CalculateExpectedRoundsToK4_input
	{
		uint64 N; // Number of tickets
	};
	struct CalculateExpectedRoundsToK4_output
	{
		uint64 expectedRounds; // E_k4(N) in fixed-point
	};
	struct CalculateExpectedRoundsToK4_locals
	{
		uint64 oneMinusP4;
		uint64 pow1mP4N;
		uint64 denomFP;
		PowerFixedPoint_input pfInput;
		PowerFixedPoint_output pfOutput;
	};

	// Calculate Extra Redirect BP (deficit-driven)
	struct CalculateExtraRedirectBP_input
	{
		uint64 N;        // Number of tickets
		uint64 delta;    // Deficit to target jackpot
		uint64 revenue;  // Round revenue
		uint64 baseGain; // Base carry gain per round (without extra)
	};
	struct CalculateExtraRedirectBP_output
	{
		uint64 extraBP; // Extra redirect in basis points (total, to be split 50/50 Dev/Dist)
	};
	struct CalculateExtraRedirectBP_locals
	{
		uint64 horizonFP;
		uint64 horizon;
		uint64 requiredGainPerRound;
		uint64 gNeed;
		uint64 extraBPTemp;
		CalculateExpectedRoundsToK4_input calcE4Input;
		CalculateExpectedRoundsToK4_output calcE4Output;
	};

	// GetRandomValues: Generate 4 unique random values from [1..30]
	struct GetRandomValues_input
	{
		uint64 seed; // Random seed from K12
	};
	struct GetRandomValues_output
	{
		QTFRandomValues values; // 4 unique random values [1..30]
	};
	struct GetRandomValues_locals
	{
		uint64 tempValue;
		uint8 index;
		uint8 candidate;
		uint8 attempts;
		uint8 fallback;
		HashSet<uint8, QTF_MAX_RANDOM_VALUE + 2> used;
	};

	// CalcReserveTopUp: Calculate safe reserve top-up amount
	struct CalcReserveTopUp_input
	{
		uint64 availableReserve;
		uint64 needed;
		uint64 perWinnerCapTotal;
		uint64 ticketPrice;
	};
	struct CalcReserveTopUp_output
	{
		uint64 topUpAmount;
	};
	struct CalcReserveTopUp_locals
	{
		uint64 softFloor;
		uint64 usableReserve;
		uint64 maxPerRound;
	};

	// Ticket Price
	struct GetTicketPrice_input
	{
	};
	struct GetTicketPrice_output
	{
		uint64 ticketPrice;
	};

	// Next Epoch Data
	struct GetNextEpochData_input
	{
	};
	struct GetNextEpochData_output
	{
		NextEpochData nextEpochData;
	};

	// Schedule
	struct GetSchedule_input
	{
	};
	struct GetSchedule_output
	{
		uint8 schedule;
	};

	// Winner Data
	struct GetWinnerData_input
	{
	};

	struct GetWinnerData_output
	{
		WinnerData winnerData;
	};

	// Pools
	struct GetPools_input
	{
	};
	struct GetPools_output
	{
		PoolsSnapshot pools;
	};

	// Draw hour
	struct GetDrawHour_input
	{
	};
	struct GetDrawHour_output
	{
		uint8 drawHour;
	};

	struct GetState_input
	{
	};
	struct GetState_output
	{
		uint8 currentState;
	};

	struct SettleEpoch_input
	{
	};

	struct SettleEpoch_output
	{
	};

	struct CountMatches_input
	{
		QTFRandomValues playerValues;
		QTFRandomValues winningValues;
	};

	struct CountMatches_output
	{
		uint8 matches;
	};
	struct CountMatches_locals
	{
		uint64 i;
		uint8 maskA;
		uint8 maskB;
	};

	struct SettlementLocals
	{
		QTFRandomValues winningValues;
		ReturnAllTickets_input returnAllTicketsInput;
		ReturnAllTickets_output returnAllTicketsOutput;
		ReturnAllTickets_locals returnAllTicketsLocals;
		CheckContractBalance_input checkBalanceInput;
		CheckContractBalance_output checkBalanceOutput;
		CheckContractBalance_locals checkBalanceLocals;
		CountMatches_input countMatchesInput;
		CountMatches_output countMatchesOutput;
		uint16 currentEpoch;
		uint64 revenue; // ticketPrice * players count
		uint64 winnersBlock;
		uint64 k2Pool;
		uint64 k3Pool;
		uint64 carryAdd;
		uint64 reserveAdd;
		uint64 winnersOverflow;
		uint64 devPayout;  // Dev after redirects
		uint64 distPayout; // Distribution after redirects
		uint64 burnAmount;
		uint64 devPercent;
		uint64 distPercent;
		uint64 burnPercent;
		uint64 winnersPercent;
		uint64 devRedirect;
		uint64 distRedirect;
		uint64 winnersRake;
		uint64 k2PayoutPool;
		uint64 k3PayoutPool;
		uint64 k2PerWinner;
		uint64 k3PerWinner;
		uint64 topUpK2;
		uint64 topUpK3;
		uint64 countK2;
		uint64 countK3;
		uint64 countK4;
		uint64 tmp64a;
		uint64 tmp64b;
		uint64 tmp64c;
		uint64 i;
		uint8 matches;
		bit shouldActivateFR;
		// Deficit-driven extra redirect calculation
		uint64 delta;       // Deficit: max(0, targetJackpot - jackpot)
		uint64 devExtraBP;  // Dev share of extra: extraRedirectBP / 2
		uint64 distExtraBP; // Dist share of extra: extraRedirectBP / 2
		// CALL parameters for CalculateBaseGain
		CalculateBaseGain_input calcBaseGainInput;
		CalculateBaseGain_output calcBaseGainOutput;
		// CALL parameters for CalculateExtraRedirectBP
		CalculateExtraRedirectBP_input calcExtraInput;
		CalculateExtraRedirectBP_output calcExtraOutput;
		// CALL parameters for GetRandomValues
		GetRandomValues_input getRandomInput;
		GetRandomValues_output getRandomOutput;
		// CALL parameters for CalcReserveTopUp
		CalcReserveTopUp_input calcTopUpInput;
		CalcReserveTopUp_output calcTopUpOutput;
		// CALL_OTHER_CONTRACT parameters for QRP::GetReserve (external reserve pool)
		QRP::GetReserve_input qrpGetReserveInput;
		QRP::GetReserve_output qrpGetReserveOutput;
		uint64 qrpRequested;    // Amount requested from QRP
		uint64 qrpReceived;     // Amount actually received from QRP
		RL::GetFees_input feesInput;
		RL::GetFees_output feesOutput;
		uint64 dividendPerShare;
		Asset rlAsset;
		AssetPossessionIterator rlIter;
		uint64 rlTotalShares;
		uint64 rlPayback;
		uint64 rlShares;
		// Cache for countMatches results to avoid redundant calculations
		Array<uint8, QTF_MAX_NUMBER_OF_PLAYERS> cachedMatches;
	};

	struct SettleEpoch_locals : public SettlementLocals
	{
	};

	struct END_EPOCH_locals
	{
		SettleEpoch_locals settlement;
		SettleEpoch_input settleInput;
		SettleEpoch_output settleOutput;
	};

	struct BEGIN_TICK_locals
	{
		SettleEpoch_input settleInput;
		SettleEpoch_output settleOutput;
		uint32 currentDateStamp;
		uint8 currentDayOfWeek;
		uint8 currentHour;
		bit isWednesday;
		bit isScheduledToday;
	};

public:
	// Contract lifecycle methods
	INITIALIZE()
	{
		state.teamAddress = QTF_ADDRESS_DEV_TEAM;
		state.ownerAddress = state.teamAddress;
		state.ticketPrice = QTF_TICKET_PRICE;
		state.targetJackpot = 1000000000ULL;
		state.overflowAlphaBP = QTF_BASELINE_OVERFLOW_ALPHA_BP;
		state.schedule = QTF_DEFAULT_SCHEDULE;
		state.drawHour = QTF_DEFAULT_DRAW_HOUR;
		state.lastDrawDateStamp = QTF_DEFAULT_INIT_TIME;
		state.frActive = false;
		state.frRoundsSinceK4 = 0;
		state.frRoundsAtOrAboveTarget = 0;
		state.numberOfPlayers = 0;
		state.jackpot = 0;
		state.reserveGeneral = 0;
		state.reserveJackpotRebuild = 0;
		state.currentState = STATE_NONE;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(BuyTicket, 1);
		REGISTER_USER_PROCEDURE(SetPrice, 2);
		REGISTER_USER_PROCEDURE(SetSchedule, 3);
		REGISTER_USER_PROCEDURE(SetTargetJackpot, 4);
		REGISTER_USER_PROCEDURE(SetDrawHour, 5);
		REGISTER_USER_FUNCTION(GetTicketPrice, 1);
		REGISTER_USER_FUNCTION(GetNextEpochData, 2);
		REGISTER_USER_FUNCTION(GetWinnerData, 3);
		REGISTER_USER_FUNCTION(GetPools, 4);
		REGISTER_USER_FUNCTION(GetSchedule, 5);
		REGISTER_USER_FUNCTION(GetDrawHour, 6);
		REGISTER_USER_FUNCTION(GetState, 7);
	}

	BEGIN_EPOCH()
	{
		applyNextEpochData(state);

		if (state.schedule == 0)
		{
			state.schedule = QTF_DEFAULT_SCHEDULE;
		}
		if (state.drawHour == 0)
		{
			state.drawHour = QTF_DEFAULT_DRAW_HOUR;
		}
		RL::makeDateStamp(qpi.year(), qpi.month(), qpi.day(), state.lastDrawDateStamp);
		clearEpochState(state);
		enableBuyTicket(state, state.lastDrawDateStamp != RL_DEFAULT_INIT_TIME);
	}

	// Settle and reset at epoch end (uses locals buffer)
	END_EPOCH_WITH_LOCALS()
	{
		enableBuyTicket(state, false);
		CALL(SettleEpoch, locals.settleInput, locals.settleOutput);
		clearEpochState(state);
	}

	// Scheduled draw processor
	BEGIN_TICK_WITH_LOCALS()
	{
		// Throttle: run logic only once per RL_TICK_UPDATE_PERIOD ticks
		if (mod(qpi.tick(), static_cast<uint32>(RL_TICK_UPDATE_PERIOD)) != 0)
		{
			return;
		}

		locals.currentHour = qpi.hour();
		if (locals.currentHour < state.drawHour)
		{
			return;
		}

		locals.currentDayOfWeek = qpi.dayOfWeek(qpi.year(), qpi.month(), qpi.day());
		RL::makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);

		// Wait for valid time initialization
		if (locals.currentDateStamp == QTF_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, false);
			state.lastDrawDateStamp = QTF_DEFAULT_INIT_TIME;
			return;
		}

		// First valid date after init: just record and exit
		if (state.lastDrawDateStamp == QTF_DEFAULT_INIT_TIME && locals.currentDateStamp != QTF_DEFAULT_INIT_TIME)
		{
			enableBuyTicket(state, true);
			if (locals.currentDayOfWeek == WEDNESDAY)
			{
				state.lastDrawDateStamp = locals.currentDateStamp;
			}
			else
			{
				state.lastDrawDateStamp = 0;
			}

			return;
		}

		if (locals.currentDateStamp == state.lastDrawDateStamp)
		{
			return;
		}

		locals.isWednesday = (locals.currentDayOfWeek == WEDNESDAY);
		locals.isScheduledToday = ((state.schedule & (1u << locals.currentDayOfWeek)) != 0);

		// Always draw on Wednesday; otherwise require schedule bit.
		if (!locals.isWednesday && !locals.isScheduledToday)
		{
			return;
		}

		state.lastDrawDateStamp = locals.currentDateStamp;

		// Pause selling during draw/settlement.
		enableBuyTicket(state, false);

		CALL(SettleEpoch, locals.settleInput, locals.settleOutput);
		clearEpochState(state);
		applyNextEpochData(state);
		enableBuyTicket(state, !locals.isWednesday);
	}

	POST_INCOMING_TRANSFER()
	{
		switch (input.type)
		{
			case TransferType::standardTransaction:
				// Return any funds sent via standard transaction
				if (input.amount > 0)
				{
					qpi.transfer(input.sourceId, input.amount);
				}
				break;
			default: break;
		}
	}

	// Procedures
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		if ((state.currentState & STATE_SELLING) == 0)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = EReturnCode::TICKET_SELLING_CLOSED;
			return;
		}

		if (state.numberOfPlayers >= QTF_MAX_NUMBER_OF_PLAYERS)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = EReturnCode::MAX_PLAYERS_REACHED;
			return;
		}

		if (qpi.invocationReward() != state.ticketPrice)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = EReturnCode::INVALID_TICKET_PRICE;
			return;
		}

		locals.validateInput.numbers = input.randomValues;
		CALL(ValidateNumbers, locals.validateInput, locals.validateOutput);
		if (!locals.validateOutput.isValid)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::INVALID_NUMBERS;
			return;
		}

		addPlayerInfo(state, qpi.invocator(), input.randomValues);
		output.returnCode = EReturnCode::SUCCESS;
	}

	PUBLIC_PROCEDURE(SetPrice)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newPrice == 0)
		{
			output.returnCode = EReturnCode::INVALID_TICKET_PRICE;
			return;
		}

		state.nextEpochData.newTicketPrice = input.newPrice;
		output.returnCode = EReturnCode::SUCCESS;
	}

	PUBLIC_PROCEDURE(SetSchedule)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newSchedule == 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.nextEpochData.newSchedule = input.newSchedule;
		output.returnCode = EReturnCode::SUCCESS;
	}

	PUBLIC_PROCEDURE(SetTargetJackpot)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newTargetJackpot == 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.nextEpochData.newTargetJackpot = input.newTargetJackpot;
		output.returnCode = EReturnCode::SUCCESS;
	}

	PUBLIC_PROCEDURE(SetDrawHour)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}

		if (input.newDrawHour == 0 || input.newDrawHour > 23)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}

		state.nextEpochData.newDrawHour = input.newDrawHour;
		output.returnCode = EReturnCode::SUCCESS;
	}

	// Functions
	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetNextEpochData) { output.nextEpochData = state.nextEpochData; }
	PUBLIC_FUNCTION(GetWinnerData) { output.winnerData = state.lastWinnerData; }
	PUBLIC_FUNCTION(GetPools)
	{
		output.pools.jackpot = state.jackpot;
		output.pools.reserveGeneral = state.reserveGeneral;
		output.pools.reserveJackpot = state.reserveJackpotRebuild;
		output.pools.targetJackpot = state.targetJackpot;
		output.pools.frActive = state.frActive;
		output.pools.roundsSinceK4 = state.frRoundsSinceK4;
	}
	PUBLIC_FUNCTION(GetSchedule) { output.schedule = state.schedule; }
	PUBLIC_FUNCTION(GetDrawHour) { output.drawHour = state.drawHour; }
	PUBLIC_FUNCTION(GetState) { output.currentState = static_cast<uint8>(state.currentState); }

protected:
	static void clearEpochState(QTF& state)
	{
		clearPlayerData(state);
		clearWinnerData(state);
	}

	static void applyNextEpochData(QTF& state)
	{
		state.nextEpochData.apply(state);
		state.nextEpochData.clear();
	}

	static void enableBuyTicket(QTF& state, bool bEnable)
	{
		if (bEnable)
		{
			state.currentState = static_cast<uint8>(state.currentState | STATE_SELLING);
		}
		else
		{
			state.currentState = static_cast<uint8>(state.currentState & static_cast<uint8>(~STATE_SELLING));
		}
	}

	// ========== Helper static functions ==========

	static void mix64(const uint64& x, uint64& outValue)
	{
		outValue = x;
		outValue ^= outValue >> 30;
		outValue *= 0xbf58476d1ce4e5b9ULL;
		outValue ^= outValue >> 27;
		outValue *= 0x94d049bb133111ebULL;
		outValue ^= outValue >> 31;
	}

	static void deriveOne(const uint64& r, const uint64& idx, uint64& outValue) { mix64(r + 0x9e3779b97f4a7c15ULL * (idx + 1), outValue); }

	static void addPlayerInfo(QTF& state, const id& playerId, const QTFRandomValues& randomValues)
	{
		state.players.set(state.numberOfPlayers++, {playerId, randomValues});
	}

	static uint8 bitcount32(uint32 v)
	{
		v = v - ((v >> 1) & 0x55555555u);
		v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
		v = (v + (v >> 4)) & 0x0F0F0F0Fu;
		v = v + (v >> 8);
		v = v + (v >> 16);
		return static_cast<uint8>(v & 0x3Fu);
	}

	static void clearWinnerData(QTF& state) { setMemory(state.lastWinnerData, 0); }

	static void clearPlayerData(QTF& state)
	{
		if (state.numberOfPlayers > 0)
		{
			setMemory(state.players, 0);
			state.numberOfPlayers = 0;
		}
	}

	static void fillWinnerData(QTF& state, const PlayerData& playerData, const QTFRandomValues& winnerValues, const uint16& epoch)
	{
		if (state.lastWinnerData.winnerCounter < state.lastWinnerData.winners.capacity())
		{
			state.lastWinnerData.winners.set(state.lastWinnerData.winnerCounter++, playerData);
		}
		state.lastWinnerData.winnerValues = winnerValues;
		state.lastWinnerData.epoch = epoch;
	}

protected:
	WinnerData lastWinnerData; // last winners snapshot

	NextEpochData nextEpochData; // queued config (ticket price)

	Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> players; // current epoch tickets

	id teamAddress; // Dev/team payout address

	id ownerAddress; // config authority

	uint64 numberOfPlayers; // tickets count in epoch

	uint64 ticketPrice; // active ticket price

	uint64 jackpot; // jackpot balance

	uint64 reserveGeneral; // reserve for floors

	uint64 reserveJackpotRebuild; // reserve earmarked to reseed jackpot

	uint64 targetJackpot; // FR target jackpot

	uint64 overflowAlphaBP; // baseline reserve share of overflow (bp)

	uint8 schedule; // bitmask of draw days

	uint8 drawHour; // draw hour UTC

	uint32 lastDrawDateStamp; // guard to avoid multiple draws per day

	bit frActive; // FR flag

	uint16 frRoundsSinceK4; // rounds since last jackpot hit

	uint16 frRoundsAtOrAboveTarget; // hysteresis counter for FR off

	uint8 currentState; // bitmask of STATE_* flags (e.g., STATE_SELLING)

private:
	// Core settlement pipeline for one epoch: fees, FR redirects, payouts, jackpot/reserve updates.
	PRIVATE_PROCEDURE_WITH_LOCALS(SettleEpoch)
	{
		if (state.numberOfPlayers == 0)
		{
			return;
		}

		locals.currentEpoch = qpi.epoch();
		locals.revenue = smul(state.ticketPrice, state.numberOfPlayers);
		if (locals.revenue == 0)
		{
			CALL(ReturnAllTickets, locals.returnAllTicketsInput, locals.returnAllTicketsOutput);
			clearPlayerData(state);

			return;
		}

		// Check if contract has sufficient balance for settlement
		locals.checkBalanceInput.expectedRevenue = locals.revenue;
		CALL(CheckContractBalance, locals.checkBalanceInput, locals.checkBalanceOutput);
		if (!locals.checkBalanceOutput.hasEnough)
		{
			// Insufficient balance: refund all players and abort settlement
			CALL(ReturnAllTickets, locals.returnAllTicketsInput, locals.returnAllTicketsOutput);
			clearPlayerData(state);

			return;
		}

		// Pull fee percents from RL so Distribution matches RL shareholders.
		// Fallback to default percentages if RL returns zeros.
		CALL_OTHER_CONTRACT_FUNCTION(RL, GetFees, locals.feesInput, locals.feesOutput);
		locals.devPercent = locals.feesOutput.teamFeePercent;
		locals.distPercent = locals.feesOutput.distributionFeePercent;
		locals.burnPercent = locals.feesOutput.burnPercent;
		locals.winnersPercent = locals.feesOutput.winnerFeePercent;

		// Sanity check: if RL returns invalid fees, use defaults
		if (locals.devPercent == 0 || locals.distPercent == 0 || locals.winnersPercent == 0)
		{
			locals.devPercent = QTF_DEFAULT_DEV_PERCENT;
			locals.distPercent = QTF_DEFAULT_DIST_PERCENT;
			locals.burnPercent = QTF_DEFAULT_BURN_PERCENT;
			locals.winnersPercent = QTF_DEFAULT_WINNERS_PERCENT;
		}

		locals.winnersBlock = div<uint64>(smul(locals.revenue, locals.winnersPercent), 100);
		locals.devPayout = div<uint64>(smul(locals.revenue, locals.devPercent), 100);
		locals.distPayout = div<uint64>(smul(locals.revenue, locals.distPercent), 100);
		locals.burnAmount = div<uint64>(smul(locals.revenue, locals.burnPercent), 100);

		// FR detection and hysteresis logic.
		// Update hysteresis counter BEFORE activation check to ensure correct deactivation timing.
		if (state.jackpot >= state.targetJackpot)
		{
			state.frRoundsAtOrAboveTarget = sadd(state.frRoundsAtOrAboveTarget, 1);
		}
		else
		{
			state.frRoundsAtOrAboveTarget = 0;
		}

		// FR Activation/Deactivation logic (deficit-driven, no hard N threshold)
		// Activation: when carry < target AND within post-k4 window (adaptive)
		// Deactivation (hysteresis): after carry >= target for 3+ rounds
		locals.shouldActivateFR = (state.jackpot < state.targetJackpot) && (state.frRoundsSinceK4 < QTF_FR_POST_K4_WINDOW_ROUNDS);
		if (locals.shouldActivateFR)
		{
			state.frActive = true;
		}
		else if (state.frRoundsAtOrAboveTarget >= QTF_FR_HYSTERESIS_ROUNDS)
		{
			// Deactivate FR after target held for hysteresis rounds
			state.frActive = false;
		}

		// Fast-Recovery (FR) mode: redirect portions of Dev/Distribution to jackpot with deficit-driven extra.
		// Base redirect is always 1% Dev + 1% Dist when FR=ON.
		// Extra redirect is calculated dynamically based on deficit, expected k4 timing, and ticket volume.
		if (state.frActive)
		{
			// Calculate deficit to target jackpot
			locals.delta = (state.jackpot < state.targetJackpot) ? (state.targetJackpot - state.jackpot) : 0;

			// Estimate base gain from existing FR mechanisms (without extra)
			locals.calcBaseGainInput.revenue = locals.revenue;
			locals.calcBaseGainInput.winnersBlock = locals.winnersBlock;
			CALL(CalculateBaseGain, locals.calcBaseGainInput, locals.calcBaseGainOutput);

			// Calculate deficit-driven extra redirect in basis points
			locals.calcExtraInput.N = state.numberOfPlayers;
			locals.calcExtraInput.delta = locals.delta;
			locals.calcExtraInput.revenue = locals.revenue;
			locals.calcExtraInput.baseGain = locals.calcBaseGainOutput.baseGain;
			CALL(CalculateExtraRedirectBP, locals.calcExtraInput, locals.calcExtraOutput);

			// Split extra equally between Dev and Dist
			locals.devExtraBP = div<uint64>(locals.calcExtraOutput.extraBP, 2);
			locals.distExtraBP = locals.calcExtraOutput.extraBP - locals.devExtraBP; // Handle odd remainder

			// Total redirect BP = base + extra
			locals.tmp64a = sadd(QTF_FR_DEV_REDIRECT_BP, locals.devExtraBP);
			locals.tmp64b = sadd(QTF_FR_DIST_REDIRECT_BP, locals.distExtraBP);

			// Calculate redirect amounts
			locals.devRedirect = div<uint64>(smul(locals.revenue, locals.tmp64a), 10000);
			locals.distRedirect = div<uint64>(smul(locals.revenue, locals.tmp64b), 10000);

			// Deduct redirects from payouts (capped at available amounts)
			if (locals.devPayout > locals.devRedirect)
			{
				locals.devPayout -= locals.devRedirect;
			}
			else
			{
				locals.devRedirect = locals.devPayout;
				locals.devPayout = 0;
			}

			if (locals.distPayout > locals.distRedirect)
			{
				locals.distPayout -= locals.distRedirect;
			}
			else
			{
				locals.distRedirect = locals.distPayout;
				locals.distPayout = 0;
			}

			// FR rake: take 5% of winners block from k3 tier to accelerate jackpot rebuild
			locals.winnersRake = div<uint64>(smul(locals.winnersBlock, QTF_FR_WINNERS_RAKE_BP), 10000);
			locals.winnersBlock -= locals.winnersRake;

			// FR tier split: 35% k3, 65% k2 (tilted toward k2 for stability during FR)
			locals.k3Pool = div<uint64>(smul(locals.winnersBlock, QTF_FR_K3_SHARE_BP), 10000);
			locals.k2Pool = locals.winnersBlock - locals.k3Pool;
		}
		else
		{
			// Baseline mode: 50/50 split between k2 and k3 (no rake, no redirects)
			locals.k3Pool = div<uint64>(smul(locals.winnersBlock, QTF_BASE_K3_SHARE_BP), 10000);
			locals.k2Pool = locals.winnersBlock - locals.k3Pool;
		}

		locals.k2PayoutPool = locals.k2Pool; // mutable pools after top-ups
		locals.k3PayoutPool = locals.k3Pool;

		// Generate winning random values using CALL
		locals.getRandomInput.seed = qpi.K12(qpi.getPrevSpectrumDigest()).u64._0;
		CALL(GetRandomValues, locals.getRandomInput, locals.getRandomOutput);
		locals.winningValues = locals.getRandomOutput.values;

		// First pass: count matches and cache results for second pass
		locals.i = 0;
		while (locals.i < state.numberOfPlayers)
		{
			locals.countMatchesInput.playerValues = state.players.get(locals.i).randomValues;
			locals.countMatchesInput.winningValues = locals.winningValues;
			CALL(CountMatches, locals.countMatchesInput, locals.countMatchesOutput);

			locals.cachedMatches.set(locals.i, locals.countMatchesOutput.matches); // Cache result

			if (locals.countMatchesOutput.matches == 2)
			{
				++locals.countK2;
			}
			else if (locals.countMatchesOutput.matches == 3)
			{
				++locals.countK3;
			}
			else if (locals.countMatchesOutput.matches == 4)
			{
				++locals.countK4;
			}
			++locals.i;
		}

		// Minimum payout floors: ensure k2 winners get >= 0.5*P, k3 winners get >= 5*P.
		// Top up from Reserve.General if pool insufficient (subject to safety limits).
		if (locals.countK2 > 0)
		{
			// k2 floor: 0.5 * ticketPrice per winner
			locals.tmp64a = div<uint64>(smul(state.ticketPrice, QTF_K2_FLOOR_MULT), QTF_K2_FLOOR_DIV);
			locals.tmp64b = smul(locals.tmp64a, locals.countK2);                    // total floor needed
			locals.tmp64c = smul(state.ticketPrice, QTF_TOPUP_PER_WINNER_CAP_MULT); // per-winner cap (25*P)

			if (locals.k2PayoutPool < locals.tmp64b)
			{
				// Pool insufficient: top up from reserve (respecting safety limits)
				locals.calcTopUpInput.availableReserve = state.reserveGeneral;
				locals.calcTopUpInput.needed = locals.tmp64b - locals.k2PayoutPool;
				locals.calcTopUpInput.perWinnerCapTotal = smul(locals.tmp64c, locals.countK2);
				locals.calcTopUpInput.ticketPrice = state.ticketPrice;
				CALL(CalcReserveTopUp, locals.calcTopUpInput, locals.calcTopUpOutput);
				locals.topUpK2 = RL::min(locals.calcTopUpOutput.topUpAmount, locals.tmp64b - locals.k2PayoutPool);
				state.reserveGeneral -= locals.topUpK2;
				locals.k2PayoutPool = sadd(locals.k2PayoutPool, locals.topUpK2);
			}

			// Calculate actual per-winner payout (capped at 25*P)
			locals.k2PerWinner = RL::min(locals.tmp64c, locals.k2PayoutPool / locals.countK2);
			locals.winnersOverflow = sadd(locals.winnersOverflow, (locals.k2PayoutPool - smul(locals.k2PerWinner, locals.countK2)));
		}
		else
		{
			// No k2 winners: entire k2 pool becomes overflow
			locals.winnersOverflow = sadd(locals.winnersOverflow, locals.k2PayoutPool);
		}

		if (locals.countK3 > 0)
		{
			// k3 floor: 5 * ticketPrice per winner
			locals.tmp64a = smul(state.ticketPrice, QTF_K3_FLOOR_MULT);
			locals.tmp64b = smul(locals.tmp64a, locals.countK3);                    // total floor needed
			locals.tmp64c = smul(state.ticketPrice, QTF_TOPUP_PER_WINNER_CAP_MULT); // per-winner cap (25*P)

			if (locals.k3PayoutPool < locals.tmp64b)
			{
				// Pool insufficient: top up from reserve (respecting safety limits)
				locals.calcTopUpInput.availableReserve = state.reserveGeneral;
				locals.calcTopUpInput.needed = locals.tmp64b - locals.k3PayoutPool;
				locals.calcTopUpInput.perWinnerCapTotal = smul(locals.tmp64c, locals.countK3);
				locals.calcTopUpInput.ticketPrice = state.ticketPrice;
				CALL(CalcReserveTopUp, locals.calcTopUpInput, locals.calcTopUpOutput);
				locals.topUpK3 = RL::min(locals.calcTopUpOutput.topUpAmount, locals.tmp64b - locals.k3PayoutPool);
				state.reserveGeneral -= locals.topUpK3;
				locals.k3PayoutPool = sadd(locals.k3PayoutPool, locals.topUpK3);
			}

			// Calculate actual per-winner payout (capped at 25*P)
			locals.k3PerWinner = RL::min(locals.tmp64c, locals.k3PayoutPool / locals.countK3);
			locals.winnersOverflow = sadd(locals.winnersOverflow, (locals.k3PayoutPool - smul(locals.k3PerWinner, locals.countK3)));
		}
		else
		{
			// No k3 winners: entire k3 pool becomes overflow
			locals.winnersOverflow = sadd(locals.winnersOverflow, locals.k3PayoutPool);
		}

		locals.carryAdd = sadd(locals.carryAdd, locals.winnersRake);

		// Second pass: payout loop using cached match results (avoids redundant countMatches calls)
		// (Optimization: reduces player iteration from 4 passes to 2 passes + eliminates duplicate countMatches)
		locals.i = 0;
		while (locals.i < state.numberOfPlayers)
		{
			locals.matches = locals.cachedMatches.get(locals.i); // Use cached result

			// k2 payout
			if (locals.matches == 2 && locals.countK2 > 0 && locals.k2PerWinner > 0)
			{
				qpi.transfer(state.players.get(locals.i).player, locals.k2PerWinner);
				fillWinnerData(state, state.players.get(locals.i), locals.winningValues, locals.currentEpoch);
			}
			// k3 payout
			else if (locals.matches == 3 && locals.countK3 > 0 && locals.k3PerWinner > 0)
			{
				qpi.transfer(state.players.get(locals.i).player, locals.k3PerWinner);
				fillWinnerData(state, state.players.get(locals.i), locals.winningValues, locals.currentEpoch);
			}
			// k4 payout (jackpot)
			else if (locals.matches == 4 && locals.countK4 > 0 && state.jackpot > 0)
			{
				locals.tmp64a = state.jackpot / locals.countK4;
				if (locals.tmp64a > 0)
				{
					qpi.transfer(state.players.get(locals.i).player, locals.tmp64a);
					fillWinnerData(state, state.players.get(locals.i), locals.winningValues, locals.currentEpoch);
				}
			}

			++locals.i;
		}

		// Post-jackpot (k4) logic: reset counters and reseed if jackpot was hit
		if (locals.countK4 > 0 && state.jackpot > 0)
		{
			// Jackpot was paid out in combined loop above, now deplete it
			state.jackpot = 0;

			// Reset FR counters after jackpot hit
			state.frRoundsSinceK4 = 0;
			state.frRoundsAtOrAboveTarget = 0;

			// Reseed jackpot from Reserve.JackpotRebuild (up to targetJackpot or available balance)
			locals.tmp64b = RL::min(state.reserveJackpotRebuild, state.targetJackpot);
			state.jackpot = sadd(state.jackpot, locals.tmp64b);
			state.reserveJackpotRebuild -= locals.tmp64b;
		}
		else
		{
			// No jackpot hit: increment rounds counter for FR post-k4 window tracking
			++state.frRoundsSinceK4;
		}

		// Overflow split: unawarded tier funds split between reserve and jackpot.
		// FR mode: 95% to jackpot, 5% to reserve (alpha=0.05)
		// Baseline mode: 50/50 split (alpha=0.50)
		if (locals.winnersOverflow > 0)
		{
			if (state.frActive)
			{
				locals.reserveAdd = div<uint64>(smul(locals.winnersOverflow, QTF_FR_ALPHA_BP), 10000);
			}
			else
			{
				locals.reserveAdd = div<uint64>(smul(locals.winnersOverflow, state.overflowAlphaBP), 10000);
			}
			locals.carryAdd = sadd(locals.carryAdd, (locals.winnersOverflow - locals.reserveAdd));
		}

		// Add all jackpot contributions: overflow carryAdd + FR redirects (if active)
		locals.tmp64a = sadd(locals.carryAdd, sadd(locals.devRedirect, locals.distRedirect));
		state.jackpot = sadd(state.jackpot, locals.tmp64a);

		// Split reserve overflow between JackpotRebuild and General using configured split ratio
		locals.tmp64a = div<uint64>(smul(locals.reserveAdd, QTF_RESERVE_SPLIT_JACKPOT_BP), 10000);
		state.reserveJackpotRebuild = sadd(state.reserveJackpotRebuild, locals.tmp64a);
		state.reserveGeneral = sadd(state.reserveGeneral, (locals.reserveAdd - locals.tmp64a));

		if (locals.devPayout > 0)
		{
			qpi.transfer(state.teamAddress, locals.devPayout);
		}
		// Manual dividend payout to RL shareholders (no extra fee).
		if (locals.distPayout > 0)
		{
			locals.rlAsset.issuer = QTF_RANDOM_LOTTERY_CONTRACT_ID;
			locals.rlAsset.assetName = QTF_RANDOM_LOTTERY_ASSET_NAME;
			locals.rlTotalShares = NUMBER_OF_COMPUTORS;

			if (locals.rlTotalShares > 0)
			{
				locals.dividendPerShare = div<uint64>(locals.distPayout, locals.rlTotalShares);
				if (locals.dividendPerShare > 0)
				{
					locals.rlIter.begin(locals.rlAsset);
					while (!locals.rlIter.reachedEnd())
					{
						locals.rlShares = static_cast<uint64>(locals.rlIter.numberOfPossessedShares());
						if (locals.rlShares > 0)
						{
							qpi.transfer(locals.rlIter.possessor(), smul(locals.rlShares, locals.dividendPerShare));
						}
						locals.rlIter.next();
					}

					locals.rlPayback = locals.distPayout - smul(locals.dividendPerShare, locals.rlTotalShares);
					if (locals.rlPayback > 0)
					{
						qpi.transfer(QTF_RANDOM_LOTTERY_CONTRACT_ID, locals.rlPayback);
					}
				}
			}
		}

		if (locals.burnAmount > 0)
		{
			qpi.burn(locals.burnAmount);
		}
	}

	/**
	 * @brief Refunds ticket price to all players who bought tickets in the current epoch.
	 *
	 * This procedure is used to return funds to all participants, typically in cases where:
	 * - The round is invalid or cancelled
	 * - Technical issues prevent proper settlement
	 * - Only one player participated (cannot draw fairly)
	 *
	 * Performs one transfer per player entry. After refund, caller should reset numberOfPlayers.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ReturnAllTickets)
	{
		// Refund ticket price to each player
		for (locals.i = 0; locals.i < state.numberOfPlayers; ++locals.i)
		{
			qpi.transfer(state.players.get(locals.i).player, state.ticketPrice);
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(CountMatches)
	{
		locals.maskA = 0;
		locals.maskB = 0;
		for (locals.i = 0; locals.i < input.playerValues.capacity(); ++locals.i)
		{
			// Ensure value is in valid range [1..30] to prevent underflow/overflow in bit shift
			ASSERT(input.playerValues.get(locals.i) > 0 && input.playerValues.get(locals.i) <= QTF_MAX_RANDOM_VALUE);
			locals.maskA |= (1u << (input.playerValues.get(locals.i) - 1));
		}
		for (locals.i = 0; locals.i < input.winningValues.capacity(); ++locals.i)
		{
			ASSERT(input.winningValues.get(locals.i) > 0 && input.winningValues.get(locals.i) <= QTF_MAX_RANDOM_VALUE);
			locals.maskB |= (1u << (input.winningValues.get(locals.i) - 1));
		}
		output.matches = bitcount32(locals.maskA & locals.maskB);
	}

	/**
	 * @brief Checks if the contract has sufficient balance to cover expected revenue.
	 *
	 * This function verifies that the on-chain balance (incoming - outgoing) of the contract
	 * is at least equal to the expected revenue. Used as a safety check before settlement
	 * to prevent underflow or incomplete payouts.
	 *
	 * @param input.expectedRevenue - The amount of revenue expected to be available
	 * @param output.hasEnough - true if actualBalance >= expectedRevenue
	 * @param output.actualBalance - Current net balance of the contract
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CheckContractBalance)
	{
		qpi.getEntity(SELF, locals.entity);
		output.actualBalance = RL::max(locals.entity.incomingAmount - locals.entity.outgoingAmount, 0i64);
		output.hasEnough = (output.actualBalance >= input.expectedRevenue);
	}

	/**
	 * @brief Computes (base^exp) in fixed-point arithmetic using fast exponentiation.
	 *
	 * @param input.base - Base value in fixed-point (scaled by QTF_FIXED_POINT_SCALE)
	 * @param input.exp - Exponent (integer)
	 * @param output.result - (base^exp) in fixed-point
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(PowerFixedPoint)
	{
		if (input.exp == 0)
		{
			output.result = QTF_FIXED_POINT_SCALE; // base^0 = 1.0
			return;
		}

		locals.tmpBase = input.base;
		locals.expCopy = input.exp;
		output.result = QTF_FIXED_POINT_SCALE;

		while (locals.expCopy > 0)
		{
			if (locals.expCopy & 1)
			{
				// result *= tmpBase (both in fixed-point, so divide by SCALE)
				output.result = div<uint64>(smul(output.result, locals.tmpBase), QTF_FIXED_POINT_SCALE);
			}
			// tmpBase *= tmpBase
			locals.tmpBase = div<uint64>(smul(locals.tmpBase, locals.tmpBase), QTF_FIXED_POINT_SCALE);
			locals.expCopy >>= 1;
		}
	}

	/**
	 * @brief Calculates expected rounds until at least one k=4 win: E_k4(N) = 1 / (1 - (1-p4)^N)
	 *
	 * Uses exact p4 = 1/27405 (combinatorics for 4-of-30).
	 * Returns result in fixed-point scaled by QTF_FIXED_POINT_SCALE.
	 *
	 * @param input.N - Number of tickets sold in round
	 * @param output.expectedRounds - E_k4(N) in fixed-point
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalculateExpectedRoundsToK4)
	{
		if (input.N == 0)
		{
			// No tickets, infinite expected rounds
			output.expectedRounds = UINT64_MAX;
			return;
		}

		// p4 = 1/27405, so (1 - p4) = 27404/27405
		// In fixed-point: (1 - p4) = (27404 * SCALE) / 27405
		locals.oneMinusP4 = div<uint64>(smul(27404ULL, QTF_FIXED_POINT_SCALE), QTF_P4_DENOMINATOR);

		// Compute (1-p4)^N in fixed-point using CALL
		locals.pfInput.base = locals.oneMinusP4;
		locals.pfInput.exp = input.N;
		CALL(PowerFixedPoint, locals.pfInput, locals.pfOutput);
		locals.pow1mP4N = locals.pfOutput.result;

		// Compute 1 - (1-p4)^N
		if (locals.pow1mP4N < QTF_FIXED_POINT_SCALE)
		{
			locals.denomFP = QTF_FIXED_POINT_SCALE - locals.pow1mP4N;
		}
		else
		{
			// Fallback: should not happen, but protect against underflow
			locals.denomFP = 1;
		}

		// E_k4 = 1 / (1 - (1-p4)^N) = SCALE / denomFP
		if (locals.denomFP > 0)
		{
			output.expectedRounds = div<uint64>(QTF_FIXED_POINT_SCALE, locals.denomFP);
		}
		else
		{
			// Extremely unlikely, fallback to large value
			output.expectedRounds = UINT64_MAX;
		}

		// Additional safety: if result unreasonably large, cap it
		if (output.expectedRounds > smul(QTF_FR_GOAL_ROUNDS_CAP, QTF_FIXED_POINT_SCALE))
		{
			output.expectedRounds = smul(QTF_FR_GOAL_ROUNDS_CAP, QTF_FIXED_POINT_SCALE);
		}
	}

	/**
	 * @brief Generate 4 unique random values from [1..30] using seed.
	 *
	 * Protection against infinite loop: if unable to find unique value after max attempts,
	 * fallback to sequential selection of first available unused value.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(GetRandomValues)
	{
		for (locals.index = 0; locals.index < output.values.capacity(); ++locals.index)
		{
			deriveOne(input.seed, locals.index, locals.tempValue);
			locals.candidate = static_cast<uint8>(mod(locals.tempValue, QTF_MAX_RANDOM_VALUE) + 1);

			locals.attempts = 0;
			while (locals.used.contains(locals.candidate) && locals.attempts < QTF_MAX_RANDOM_GENERATION_ATTEMPTS)
			{
				++locals.attempts;
				// Regenerate a fresh candidate from the evolving tempValue until it is unique
				locals.tempValue ^= locals.tempValue >> 12;
				locals.tempValue ^= locals.tempValue << 25;
				locals.tempValue ^= locals.tempValue >> 27;
				locals.tempValue *= 2685821657736338717ULL;
				locals.candidate = static_cast<uint8>(mod(locals.tempValue, QTF_MAX_RANDOM_VALUE) + 1);
			}

			// Fallback: if still duplicate after max attempts, find first unused value
			if (locals.used.contains(locals.candidate))
			{
				for (locals.fallback = 1; locals.fallback <= QTF_MAX_RANDOM_VALUE; ++locals.fallback)
				{
					if (!locals.used.contains(locals.fallback))
					{
						locals.candidate = locals.fallback;
						break;
					}
				}
			}

			locals.used.add(locals.candidate);
			output.values.set(locals.index, locals.candidate);
		}
	}

	/**
	 * @brief Validates that all numbers in the array are unique and in range [1..30].
	 *
	 * @param input.numbers - Array of numbers to validate
	 * @param output.isValid - true if all numbers are valid and unique
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateNumbers)
	{
		output.isValid = true;
		for (locals.idx = 0; locals.idx < input.numbers.capacity(); ++locals.idx)
		{
			locals.value = input.numbers.get(locals.idx);
			if (locals.value == 0 || locals.value > QTF_MAX_RANDOM_VALUE)
			{
				output.isValid = false;
				return;
			}
			if (locals.seen.contains(locals.value))
			{
				output.isValid = false;
				return;
			}
			locals.seen.add(locals.value);
		}
	}

	/**
	 * @brief Calculate safe reserve top-up amount respecting safety limits.
	 *
	 * @param input.availableReserve - Current reserve balance
	 * @param input.needed - Amount needed for top-up
	 * @param input.perWinnerCapTotal - Per-winner cap multiplied by winner count
	 * @param input.ticketPrice - Current ticket price
	 * @param output.topUpAmount - Safe amount to top up from reserve
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalcReserveTopUp)
	{
		if (input.availableReserve == 0)
		{
			output.topUpAmount = 0;
			return;
		}

		// Calculate soft floor: keep at least 20 * P in reserve
		locals.softFloor = smul(input.ticketPrice, QTF_RESERVE_SOFT_FLOOR_MULT);

		// Calculate usable reserve (above soft floor)
		if (input.availableReserve > locals.softFloor)
		{
			locals.usableReserve = input.availableReserve - locals.softFloor;
		}
		else
		{
			locals.usableReserve = 0;
		}

		// Calculate max per round (10% of available reserve)
		locals.maxPerRound = div<uint64>(smul(input.availableReserve, QTF_TOPUP_RESERVE_PCT_BP), 10000);

		// Cap by usable reserve
		if (locals.maxPerRound > locals.usableReserve)
		{
			locals.maxPerRound = locals.usableReserve;
		}

		// Cap by per-winner cap
		if (locals.maxPerRound > input.perWinnerCapTotal)
		{
			locals.maxPerRound = input.perWinnerCapTotal;
		}

		// Return min of needed and max allowed
		if (input.needed < locals.maxPerRound)
		{
			output.topUpAmount = input.needed;
		}
		else
		{
			output.topUpAmount = locals.maxPerRound;
		}
	}

	/**
	 * @brief Estimates base carry gain per round from FR mechanisms (without extra redirect).
	 *
	 * Includes:
	 * - Base Dev redirect: QTF_FR_DEV_REDIRECT_BP of R
	 * - Base Dist redirect: QTF_FR_DIST_REDIRECT_BP of R
	 * - Winners rake: QTF_FR_WINNERS_RAKE_BP of winners block
	 * - Overflow bias: (1 - alpha_fr) of overflow to carry
	 *
	 * This is a simplified estimate; actual gain depends on winners count and overflow.
	 * We use conservative approximation: assume moderate overflow and typical winner distribution.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalculateBaseGain)
	{
		// Base redirects from Dev and Dist
		locals.devRedirect = div<uint64>(smul(input.revenue, QTF_FR_DEV_REDIRECT_BP), 10000);
		locals.distRedirect = div<uint64>(smul(input.revenue, QTF_FR_DIST_REDIRECT_BP), 10000);

		// Winners rake: 5% of winners block
		locals.winnersRake = div<uint64>(smul(input.winnersBlock, QTF_FR_WINNERS_RAKE_BP), 10000);

		// Overflow estimate: assume ~10% of winnersBlock not paid out (conservative)
		// With alpha_fr = 0.05, 95% of overflow goes to carry
		locals.estimatedOverflow = div<uint64>(input.winnersBlock, 10);
		locals.overflowToCarry = div<uint64>(smul(locals.estimatedOverflow, 10000 - QTF_FR_ALPHA_BP), 10000);

		// Total base gain
		output.baseGain = sadd(sadd(locals.devRedirect, locals.distRedirect), sadd(locals.winnersRake, locals.overflowToCarry));
	}

	/**
	 * @brief Calculates deficit-driven extra redirect percentage in basis points.
	 *
	 * Formula:
	 * - delta = max(0, targetJackpot - currentJackpot)
	 * - E_k4(N) = expected rounds to k=4
	 * - H = min(E_k4(N), cap)
	 * - g_need = max(0, delta/H - baseGain)
	 * - extra_pp = clamp(g_need / revenue, 0, extra_max)
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalculateExtraRedirectBP)
	{
		if (input.delta == 0 || input.revenue == 0 || input.N == 0)
		{
			output.extraBP = 0;
			return;
		}

		// Calculate E_k4(N) in fixed-point using CALL
		locals.calcE4Input.N = input.N;
		CALL(CalculateExpectedRoundsToK4, locals.calcE4Input, locals.calcE4Output);

		// Apply cap: H = min(E_k4, cap)
		locals.horizonFP = RL::min(locals.calcE4Output.expectedRounds, smul(QTF_FR_GOAL_ROUNDS_CAP, QTF_FIXED_POINT_SCALE));

		// Convert horizon back to integer rounds (divide by SCALE)
		locals.horizon = RL::max(div<uint64>(locals.horizonFP, QTF_FIXED_POINT_SCALE), 1ULL);

		// Calculate required gain per round: delta / H
		locals.requiredGainPerRound = div<uint64>(input.delta, locals.horizon);

		// Calculate needed additional gain: g_need = max(0, requiredGainPerRound - baseGain)
		if (locals.requiredGainPerRound > input.baseGain)
		{
			locals.gNeed = locals.requiredGainPerRound - input.baseGain;
		}
		else
		{
			output.extraBP = 0;
			return;
		}

		// Convert g_need to percentage of revenue in basis points: (g_need / revenue) * 10000
		locals.extraBPTemp = div<uint64>(smul(locals.gNeed, 10000ULL), input.revenue);

		// Clamp to maximum
		output.extraBP = RL::min(locals.extraBPTemp, QTF_FR_EXTRA_MAX_BP);
	}
};
