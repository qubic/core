using namespace QPI;

// --- Core game parameters ----------------------------------------------------
constexpr uint64 QTF_MAX_NUMBER_OF_PLAYERS = 1024;
constexpr uint64 QTF_RANDOM_VALUES_COUNT = 4;
constexpr uint64 QTF_MAX_RANDOM_VALUE = 30;
constexpr uint64 QTF_TICKET_PRICE = 1000000;

// Baseline split for k2/k3 when FR is OFF (per spec: k3=40%, k2=28% of Winners block).
// Initial 32% of Winners block is unallocated; overflow will also include unawarded k2/k3 funds.
constexpr uint64 QTF_BASE_K3_SHARE_BP = 4000; // 40% of winners block to k3
constexpr uint64 QTF_BASE_K2_SHARE_BP = 2800; // 28% of winners block to k2

// --- Fast-Recovery (FR) parameters (spec defaults) --------------------------
// Fast-Recovery base redirect percentages (applied when FR=ON, capped at available amounts)
constexpr uint64 QTF_FR_DEV_REDIRECT_BP = 100;  // 1.00% of R (base redirect)
constexpr uint64 QTF_FR_DIST_REDIRECT_BP = 100; // 1.00% of R (base redirect)

// Deficit-driven extra redirect parameters (dynamic, no hard N threshold)
// The extra redirect is calculated based on:
//   - Deficit: Δ = max(0, targetJackpot - currentJackpot)
//   - Expected rounds to k=4: E_k4(N) = 1 / (1 - (1-p4)^N)
//   - Horizon: H = min(E_k4(N), R_goal_rounds_cap)
//   - Needed gain: g_need = max(0, Δ/H - baseGain)
//   - Extra percentage: extra_pp = clamp(g_need / R, 0, extra_max)
//   - Split equally: dev_extra = dist_extra = extra_pp / 2
constexpr uint64 QTF_FR_EXTRA_MAX_BP = 70;        // Maximum extra redirect: 0.70% of R total (0.35% each Dev/Dist)
constexpr uint64 QTF_FR_GOAL_ROUNDS_CAP = 50;     // Cap on expected rounds horizon H for deficit calculation
constexpr uint64 QTF_FIXED_POINT_SCALE = 1000000; // Scale for fixed-point arithmetic (6 decimals precision)

// Probability constants for k=4 win (exact combinatorics: 4-of-30)
// p4 = C(4,4) * C(26,0) / C(30,4) = 1 / 27405
constexpr uint64 QTF_P4_DENOMINATOR = 27405;   // Denominator for k=4 probability (1/27405)
constexpr uint64 QTF_FR_WINNERS_RAKE_BP = 500; // 5% of winners block from k3
constexpr uint64 QTF_FR_ALPHA_BP = 500;        // alpha = 0.05 -> 95% overflow to jackpot
constexpr uint8 QTF_FR_POST_K4_WINDOW_ROUNDS = 50;
constexpr uint8 QTF_FR_HYSTERESIS_ROUNDS = 3;

// --- Floors and reserve safety ----------------------------------------------
constexpr uint64 QTF_K2_FLOOR_MULT = 1; // numerator for 0.5 * P (we divide by 2)
constexpr uint64 QTF_K2_FLOOR_DIV = 2;
constexpr uint64 QTF_K3_FLOOR_MULT = 5;              // 5 * P
constexpr uint64 QTF_TOPUP_PER_WINNER_CAP_MULT = 25; // 25 * P
constexpr uint64 QTF_TOPUP_RESERVE_PCT_BP = 1000;    // 10% of reserve per round
constexpr uint64 QTF_RESERVE_SOFT_FLOOR_MULT = 20;   // keep at least 20 * P in reserve

// Baseline overflow split (reserve share in basis points). If spec is updated, adjust here.
constexpr uint64 QTF_BASELINE_OVERFLOW_ALPHA_BP = 5000; // 50% reserve / 50% jackpot

// Default fee percentages (fallback if RL::GetFees fails)
constexpr uint8 QTF_DEFAULT_DEV_PERCENT = 10;
constexpr uint8 QTF_DEFAULT_DIST_PERCENT = 20;
constexpr uint8 QTF_DEFAULT_BURN_PERCENT = 2;
constexpr uint8 QTF_DEFAULT_WINNERS_PERCENT = 68;

// Maximum attempts to generate unique random value before fallback
constexpr uint8 QTF_MAX_RANDOM_GENERATION_ATTEMPTS = 100;

constexpr uint64 QTF_DEFAULT_TARGET_JACKPOT = 1000000000ULL; // 1 billion QU (1B)
constexpr uint8 QTF_DEFAULT_SCHEDULE = 1 << SATURDAY | 1u << WEDNESDAY;
constexpr uint8 QTF_DEFAULT_DRAW_HOUR = 11;                        // 11:00 UTC
constexpr uint32 QTF_DEFAULT_INIT_TIME = 22u << 9 | 4u << 5 | 13u; // RL_DEFAULT_INIT_TIME

const id QTF_ADDRESS_DEV_TEAM = ID(_Z, _T, _Z, _E, _A, _Q, _G, _U, _P, _I, _K, _T, _X, _F, _Y, _X, _Y, _E, _I, _T, _L, _A, _K, _F, _T, _D, _X, _C, _R,
                                   _L, _W, _E, _T, _H, _N, _G, _H, _D, _Y, _U, _W, _E, _Y, _Q, _N, _Q, _S, _R, _H, _O, _W, _M, _U, _J, _L, _E);
const id QTF_RANDOM_LOTTERY_CONTRACT_ID = id(RL_CONTRACT_INDEX, 0, 0, 0);
constexpr uint64 QTF_RANDOM_LOTTERY_ASSET_NAME = 19538; // RL
const id QTF_RESERVE_POOL_CONTRACT_ID = id(QRP_CONTRACT_INDEX, 0, 0, 0);

struct QTF2
{
};

struct QTF : ContractBase
{
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

	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); };

	enum EState : uint8
	{
		STATE_NONE = 0,
		STATE_SELLING = 1 << 0
	};

	struct PlayerData
	{
		id player;
		Array<uint8, QTF_RANDOM_VALUES_COUNT> randomValues;
	};

	struct WinnerData
	{
		Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> winners;
		Array<uint8, QTF_RANDOM_VALUES_COUNT> winnerValues;
		uint64 winnerCounter;
		uint16 epoch;
	};

	struct NextEpochData
	{
		void clear()
		{
			newTicketPrice = 0;
			newTargetJackpot = 0;
			newSchedule = 0;
			newDrawHour = 0;
		}

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

		uint64 newTicketPrice;
		uint64 newTargetJackpot;
		uint8 newSchedule;
		uint8 newDrawHour;
	};

	struct PoolsSnapshot
	{
		uint64 jackpot;
		uint64 reserve; // Available reserve from QRP (not including locked amounts)
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

	// ValidateNumbers: Check if all numbers are valid [1..30] and unique
	struct ValidateNumbers_input
	{
		Array<uint8, QTF_RANDOM_VALUES_COUNT> numbers; // Numbers to validate
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
		Array<uint8, QTF_RANDOM_VALUES_COUNT> randomValues;
	};
	struct BuyTicket_output
	{
		uint8 returnCode;
	};
	struct BuyTicket_locals
	{
		// CALL parameters for ValidateNumbers
		ValidateNumbers_input validateInput;
		ValidateNumbers_output validateOutput;
		uint64 excess;
	};

	// Set Price
	struct SetPrice_input
	{
		uint64 newPrice;
	};
	struct SetPrice_output
	{
		uint8 returnCode;
	};

	// Set Schedule
	struct SetSchedule_input
	{
		uint8 newSchedule;
	};
	struct SetSchedule_output
	{
		uint8 returnCode;
	};

	// Set draw hour
	struct SetDrawHour_input
	{
		uint8 newDrawHour;
	};
	struct SetDrawHour_output
	{
		uint8 returnCode;
	};

	// Set Target Jackpot
	struct SetTargetJackpot_input
	{
		uint64 newTargetJackpot;
	};
	struct SetTargetJackpot_output
	{
		uint8 returnCode;
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

	// Calculate Base Gain (FR base carry growth estimation, excluding extra deficit-driven redirect)
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
		Array<uint8, QTF_RANDOM_VALUES_COUNT> values; // 4 unique random values [1..30]
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
		uint64 totalQRPBalance; // Actual QRP balance (for 10% limit and soft floor)
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
		uint64 availableAboveFloor;
		uint64 maxPerRound;
	};

	// ProcessTierPayout: Unified tier payout processing (k2/k3)
	struct ProcessTierPayout_input
	{
		uint64 floorPerWinner;  // Floor payout per winner (0.5*P for k2, 5*P for k3)
		uint64 winnerCount;     // Number of winners in this tier
		uint64 payoutPool;      // Initial payout pool for this tier
		uint64 perWinnerCap;    // Per-winner cap (25*P)
		uint64 totalQRPBalance; // QRP balance for safety limits
		uint64 ticketPrice;     // Current ticket price
	};
	struct ProcessTierPayout_output
	{
		uint64 perWinnerPayout; // Calculated per-winner payout
		uint64 overflow;        // Overflow amount (unused funds)
		uint64 topUpReceived;   // Amount received from QRP top-up
	};
	struct ProcessTierPayout_locals
	{
		uint64 floorTotalNeeded;
		uint64 finalPool;
		uint64 qrpRequested;
		CalcReserveTopUp_input calcTopUpInput;
		CalcReserveTopUp_output calcTopUpOutput;
		QRP::WithdrawReserve_input qrpGetReserveInput;
		QRP::WithdrawReserve_output qrpGetReserveOutput;
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
	struct GetPools_locals
	{
		QRP::GetAvailableReserve_input qrpInput;
		QRP::GetAvailableReserve_output qrpOutput;
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
		Array<uint8, QTF_RANDOM_VALUES_COUNT> playerValues;
		Array<uint8, QTF_RANDOM_VALUES_COUNT> winningValues;
	};

	struct CountMatches_output
	{
		uint8 matches;
	};
	struct CountMatches_locals
	{
		uint64 i;
		uint32 maskA;
		uint32 maskB;
		uint8 randomValue;
	};

	struct GetFees_input
	{
	};

	struct GetFees_output
	{
		uint8 teamFeePercent;         // Team share in percent
		uint8 distributionFeePercent; // Distribution/shareholders share in percent
		uint8 winnerFeePercent;       // Winner share in percent
		uint8 burnPercent;            // Burn share in percent
		uint8 returnCode;
	};

	struct GetFees_locals
	{
		RL::GetFees_input feesInput;
		RL::GetFees_output feesOutput;
	};

	// EstimateFRJackpotGrowth: Calculate minimum expected jackpot growth in FR mode
	// Used for testing to verify 95% overflow bias is working correctly
	struct EstimateFRJackpotGrowth_input
	{
		uint64 revenue;        // Total revenue (ticketPrice * numPlayers)
		uint64 winnersPercent; // Winners block percentage (typically 68)
	};
	struct EstimateFRJackpotGrowth_output
	{
		uint64 minJackpotGrowth;  // Minimum expected jackpot growth
		uint64 winnersRake;       // 5% of winners block
		uint64 overflowToJackpot; // 95% of overflow
		uint64 devRedirect;       // 1% of revenue
		uint64 distRedirect;      // 1% of revenue
	};
	struct EstimateFRJackpotGrowth_locals
	{
		uint64 winnersBlock;
		uint64 winnersBlockAfterRake;
		uint64 k3Pool;
		uint64 k2Pool;
		uint64 winnersOverflow;
		uint64 reserveAdd;
	};

	// CalculatePrizePools: Calculate k2/k3 prize pools from revenue
	// Reusable function for both settlement and estimation
	struct CalculatePrizePools_input
	{
		uint64 revenue;  // Total revenue (ticketPrice * numberOfPlayers)
		bit applyFRRake; // Whether to apply 5% FR rake
	};
	struct CalculatePrizePools_output
	{
		uint64 winnersBlock; // Winners block after fees
		uint64 winnersRake;  // 5% rake (if FR active)
		uint64 k2Pool;       // 28% of winners block (after rake)
		uint64 k3Pool;       // 40% of winners block (after rake)
	};
	struct CalculatePrizePools_locals
	{
		GetFees_input feesInput;
		GetFees_output feesOutput;
		uint64 winnersBlockBeforeRake;
	};

	// EstimatePrizePayouts: Calculate estimated prize payouts for k=2 and k=3 tiers
	// Based on current ticket sales and number of winners per tier
	struct EstimatePrizePayouts_input
	{
		uint64 k2WinnerCount; // Number of k=2 winners (estimated or actual)
		uint64 k3WinnerCount; // Number of k=3 winners (estimated or actual)
	};
	struct EstimatePrizePayouts_output
	{
		uint64 k2PayoutPerWinner; // Estimated payout per k=2 winner
		uint64 k3PayoutPerWinner; // Estimated payout per k=3 winner
		uint64 k2MinFloor;        // Minimum guaranteed payout for k=2 (0.5*P)
		uint64 k3MinFloor;        // Minimum guaranteed payout for k=3 (5*P)
		uint64 perWinnerCap;      // Maximum payout per winner (25*P)
		uint64 totalRevenue;      // Total revenue from ticket sales
		uint64 k2Pool;            // Total pool for k=2 tier
		uint64 k3Pool;            // Total pool for k=3 tier
	};
	struct EstimatePrizePayouts_locals
	{
		uint64 revenue;
		uint64 k2FloorTotal;
		uint64 k3FloorTotal;
		uint64 k2PayoutPoolEffective;
		uint64 k3PayoutPoolEffective;
		CalculatePrizePools_input calcPoolsInput;
		CalculatePrizePools_output calcPoolsOutput;
	};

	struct SettleEpoch_locals
	{
		Array<uint8, QTF_RANDOM_VALUES_COUNT> winningValues;
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
		uint64 devRedirect;
		uint64 distRedirect;
		uint64 winnersRake;
		uint64 k2PayoutPool;
		uint64 k3PayoutPool;
		uint64 k2PerWinner;
		uint64 k3PerWinner;
		uint64 countK2;
		uint64 countK3;
		uint64 countK4;
		uint64 totalDevRedirectBP;       // Total dev redirect in basis points (base + extra)
		uint64 totalDistRedirectBP;      // Total dist redirect in basis points (base + extra)
		uint64 perWinnerCap;             // Per-winner payout cap (25*P)
		uint64 jackpotPerK4Winner;       // Jackpot share per k4 winner
		uint64 totalJackpotContribution; // Total amount to add to jackpot
		uint64 i;
		uint8 matches;
		bit shouldActivateFR;
		// Deficit-driven extra redirect calculation
		uint64 delta;       // Deficit: max(0, targetJackpot - jackpot)
		uint64 devExtraBP;  // Dev share of extra: extraRedirectBP / 2
		uint64 distExtraBP; // Dist share of extra: extraRedirectBP / 2
		// CALL parameters for CalculatePrizePools (shared function)
		CalculatePrizePools_input calcPoolsInput;
		CalculatePrizePools_output calcPoolsOutput;
		// CALL parameters for CalculateBaseGain
		CalculateBaseGain_input calcBaseGainInput;
		CalculateBaseGain_output calcBaseGainOutput;
		// CALL parameters for CalculateExtraRedirectBP
		CalculateExtraRedirectBP_input calcExtraInput;
		CalculateExtraRedirectBP_output calcExtraOutput;
		// CALL parameters for GetRandomValues
		GetRandomValues_input getRandomInput;
		GetRandomValues_output getRandomOutput;
		// CALL parameters for ProcessTierPayout (unified k2/k3 processing)
		ProcessTierPayout_input tierPayoutInput;
		ProcessTierPayout_output tierPayoutOutput;
		// CALL_OTHER_CONTRACT parameters for QRP (external reserve pool)
		QRP::WithdrawReserve_input qrpGetReserveInput;
		QRP::WithdrawReserve_output qrpGetReserveOutput;
		QRP::GetAvailableReserve_input qrpGetAvailableInput;
		QRP::GetAvailableReserve_output qrpGetAvailableOutput;
		uint64 qrpRequested;    // Amount requested from QRP
		uint64 qrpReceived;     // Amount actually received from QRP
		uint64 totalQRPBalance; // Total balance in QRP (for safety limits)
		GetFees_input feesInput;
		GetFees_output feesOutput;
		uint64 dividendPerShare;
		Asset rlAsset;
		AssetPossessionIterator rlIter;
		uint64 rlTotalShares;
		uint64 rlPayback;
		uint64 rlShares;
		// Cache for countMatches results to avoid redundant calculations
		Array<uint8, QTF_MAX_NUMBER_OF_PLAYERS> cachedMatches;
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

	// Contract lifecycle methods
	INITIALIZE()
	{
		state.teamAddress = QTF_ADDRESS_DEV_TEAM;
		state.ownerAddress = state.teamAddress;
		state.ticketPrice = QTF_TICKET_PRICE;
		state.targetJackpot = QTF_DEFAULT_TARGET_JACKPOT;
		state.overflowAlphaBP = QTF_BASELINE_OVERFLOW_ALPHA_BP;
		state.schedule = QTF_DEFAULT_SCHEDULE;
		state.drawHour = QTF_DEFAULT_DRAW_HOUR;
		state.lastDrawDateStamp = QTF_DEFAULT_INIT_TIME;
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
		REGISTER_USER_FUNCTION(GetFees, 8);
		REGISTER_USER_FUNCTION(EstimatePrizePayouts, 9);
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

			output.returnCode = toReturnCode(EReturnCode::TICKET_SELLING_CLOSED);
			return;
		}

		if (state.numberOfPlayers >= QTF_MAX_NUMBER_OF_PLAYERS)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = toReturnCode(EReturnCode::MAX_PLAYERS_REACHED);
			return;
		}

		if (qpi.invocationReward() < static_cast<sint64>(state.ticketPrice))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}

			output.returnCode = toReturnCode(EReturnCode::INVALID_TICKET_PRICE);
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
			output.returnCode = toReturnCode(EReturnCode::INVALID_NUMBERS);
			return;
		}

		addPlayerInfo(state, qpi.invocator(), input.randomValues);

		// If overpaid, accept ticket and return excess to invocator.
		// Important: refund excess ONLY after validation, otherwise invalid tickets could be over-refunded.
		if (qpi.invocationReward() > static_cast<sint64>(state.ticketPrice))
		{
			locals.excess = qpi.invocationReward() - state.ticketPrice;
			if (locals.excess > 0)
			{
				qpi.transfer(qpi.invocator(), locals.excess);
			}
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(SetPrice)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newPrice == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TICKET_PRICE);
			return;
		}

		state.nextEpochData.newTicketPrice = input.newPrice;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(SetSchedule)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newSchedule == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.newSchedule = input.newSchedule;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(SetTargetJackpot)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newTargetJackpot == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.newTargetJackpot = input.newTargetJackpot;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_PROCEDURE(SetDrawHour)
	{
		if (qpi.invocator() != state.ownerAddress)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}

		if (input.newDrawHour == 0 || input.newDrawHour > 23)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.nextEpochData.newDrawHour = input.newDrawHour;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	// Functions
	PUBLIC_FUNCTION(GetTicketPrice) { output.ticketPrice = state.ticketPrice; }
	PUBLIC_FUNCTION(GetNextEpochData) { output.nextEpochData = state.nextEpochData; }
	PUBLIC_FUNCTION(GetWinnerData) { output.winnerData = state.lastWinnerData; }
	PUBLIC_FUNCTION_WITH_LOCALS(GetPools)
	{
		output.pools.jackpot = state.jackpot;
		CALL_OTHER_CONTRACT_FUNCTION(QRP, GetAvailableReserve, locals.qrpInput, locals.qrpOutput);
		output.pools.reserve = locals.qrpOutput.availableReserve;
		output.pools.targetJackpot = state.targetJackpot;
		output.pools.frActive = state.frActive;
		output.pools.roundsSinceK4 = state.frRoundsSinceK4;
	}
	PUBLIC_FUNCTION(GetSchedule) { output.schedule = state.schedule; }
	PUBLIC_FUNCTION(GetDrawHour) { output.drawHour = state.drawHour; }
	PUBLIC_FUNCTION(GetState) { output.currentState = static_cast<uint8>(state.currentState); }
	PUBLIC_FUNCTION_WITH_LOCALS(GetFees)
	{
		CALL_OTHER_CONTRACT_FUNCTION(RL, GetFees, locals.feesInput, locals.feesOutput);
		output.teamFeePercent = locals.feesOutput.teamFeePercent;
		output.distributionFeePercent = locals.feesOutput.distributionFeePercent;
		output.burnPercent = locals.feesOutput.burnPercent;
		output.winnerFeePercent = locals.feesOutput.winnerFeePercent;

		// Sanity check: if RL returns invalid fees, use defaults
		if (output.teamFeePercent == 0 || output.distributionFeePercent == 0 || output.winnerFeePercent == 0)
		{
			output.teamFeePercent = QTF_DEFAULT_DEV_PERCENT;
			output.distributionFeePercent = QTF_DEFAULT_DIST_PERCENT;
			output.burnPercent = QTF_DEFAULT_BURN_PERCENT;
			output.winnerFeePercent = QTF_DEFAULT_WINNERS_PERCENT;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(EstimatePrizePayouts)
	{
		// Calculate total revenue from current ticket sales
		locals.revenue = smul(state.ticketPrice, state.numberOfPlayers);
		output.totalRevenue = locals.revenue;

		// Set minimum floors and cap
		output.k2MinFloor = div<uint64>(smul(state.ticketPrice, QTF_K2_FLOOR_MULT), QTF_K2_FLOOR_DIV); // 0.5*P
		output.k3MinFloor = smul(state.ticketPrice, QTF_K3_FLOOR_MULT);                                // 5*P
		output.perWinnerCap = smul(state.ticketPrice, QTF_TOPUP_PER_WINNER_CAP_MULT);                  // 25*P

		if (locals.revenue == 0 || state.numberOfPlayers == 0)
		{
			// No tickets sold, no payouts
			output.k2PayoutPerWinner = 0;
			output.k3PayoutPerWinner = 0;
			output.k2Pool = 0;
			output.k3Pool = 0;
			return;
		}

		// Use shared CalculatePrizePools function to compute pools
		locals.calcPoolsInput.revenue = locals.revenue;
		locals.calcPoolsInput.applyFRRake = state.frActive;
		CALL(CalculatePrizePools, locals.calcPoolsInput, locals.calcPoolsOutput);

		output.k2Pool = locals.calcPoolsOutput.k2Pool;
		output.k3Pool = locals.calcPoolsOutput.k3Pool;

		// Calculate k2 payout per winner
		if (input.k2WinnerCount > 0)
		{
			locals.k2FloorTotal = smul(output.k2MinFloor, input.k2WinnerCount);
			locals.k2PayoutPoolEffective = output.k2Pool;

			// Note: This is an estimate - actual implementation may top up from reserve
			// If pool insufficient, we show floor; otherwise calculate actual per-winner amount
			if (locals.k2PayoutPoolEffective >= locals.k2FloorTotal)
			{
				output.k2PayoutPerWinner = RL::min(output.perWinnerCap, div<uint64>(locals.k2PayoutPoolEffective, input.k2WinnerCount));
			}
			else
			{
				// Pool insufficient, would need reserve top-up - show floor as estimate
				output.k2PayoutPerWinner = output.k2MinFloor;
			}
		}
		else
		{
			// No winners - show what a single winner would get
			output.k2PayoutPerWinner = RL::min(output.perWinnerCap, output.k2Pool);
		}

		// Calculate k3 payout per winner
		if (input.k3WinnerCount > 0)
		{
			locals.k3FloorTotal = smul(output.k3MinFloor, input.k3WinnerCount);
			locals.k3PayoutPoolEffective = output.k3Pool;

			// Note: This is an estimate - actual implementation may top up from reserve
			if (locals.k3PayoutPoolEffective >= locals.k3FloorTotal)
			{
				output.k3PayoutPerWinner = RL::min(output.perWinnerCap, div<uint64>(locals.k3PayoutPoolEffective, input.k3WinnerCount));
			}
			else
			{
				// Pool insufficient, would need reserve top-up - show floor as estimate
				output.k3PayoutPerWinner = output.k3MinFloor;
			}
		}
		else
		{
			// No winners - show what a single winner would get
			output.k3PayoutPerWinner = RL::min(output.perWinnerCap, output.k3Pool);
		}
	}

protected:
	static void clearEpochState(QTF& state) { clearPlayerData(state); }

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

	static void addPlayerInfo(QTF& state, const id& playerId, const Array<uint8, QTF_RANDOM_VALUES_COUNT>& randomValues)
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

	static void clearPlayerData(QTF& state)
	{
		if (state.numberOfPlayers > 0)
		{
			setMemory(state.players, 0);
			state.numberOfPlayers = 0;
		}
	}

	static void clearWinerData(QTF& state) { setMemory(state.lastWinnerData, 0); }

	static void fillWinnerData(QTF& state, const PlayerData& playerData, const Array<uint8, QTF_RANDOM_VALUES_COUNT>& winnerValues,
	                           const uint16& epoch)
	{
		if (!isZero(playerData.player))
		{
			if (state.lastWinnerData.winnerCounter < state.lastWinnerData.winners.capacity())
			{
				state.lastWinnerData.winners.set(state.lastWinnerData.winnerCounter++, playerData);
			}
		}

		state.lastWinnerData.winnerValues = winnerValues;
		state.lastWinnerData.epoch = epoch;
	}

	WinnerData lastWinnerData; // last winners snapshot

	NextEpochData nextEpochData; // queued config (ticket price)

	Array<PlayerData, QTF_MAX_NUMBER_OF_PLAYERS> players; // current epoch tickets

	id teamAddress; // Dev/team payout address

	id ownerAddress; // config authority

	uint64 numberOfPlayers; // tickets count in epoch

	uint64 ticketPrice; // active ticket price

	uint64 jackpot; // jackpot balance

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

		CALL(GetFees, locals.feesInput, locals.feesOutput);

		locals.devPayout = div<uint64>(smul(locals.revenue, static_cast<uint64>(locals.feesOutput.teamFeePercent)), 100);
		locals.distPayout = div<uint64>(smul(locals.revenue, static_cast<uint64>(locals.feesOutput.distributionFeePercent)), 100);
		locals.burnAmount = div<uint64>(smul(locals.revenue, static_cast<uint64>(locals.feesOutput.burnPercent)), 100);

		// FR detection and hysteresis logic.
		// Update hysteresis counter BEFORE activation check so deactivation can occur
		// immediately when reaching the threshold (3 consecutive rounds at/above target).
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
		else if (state.frRoundsSinceK4 >= QTF_FR_POST_K4_WINDOW_ROUNDS)
		{
			// Outside post-k4 window: FR must be OFF.
			state.frActive = false;
		}
		else if (state.frRoundsAtOrAboveTarget >= QTF_FR_HYSTERESIS_ROUNDS)
		{
			// Deactivate FR after target held for hysteresis rounds
			state.frActive = false;
		}

		// Calculate prize pools using shared function (handles FR rake if active)
		locals.calcPoolsInput.revenue = locals.revenue;
		locals.calcPoolsInput.applyFRRake = state.frActive;
		CALL(CalculatePrizePools, locals.calcPoolsInput, locals.calcPoolsOutput);

		locals.winnersBlock = locals.calcPoolsOutput.winnersBlock;
		locals.winnersRake = locals.calcPoolsOutput.winnersRake;
		locals.k2Pool = locals.calcPoolsOutput.k2Pool;
		locals.k3Pool = locals.calcPoolsOutput.k3Pool;

		// Calculate initial overflow: unallocated funds after k2/k3 allocation (32% baseline)
		// Additional unawarded k2/k3 funds will be added to this after tier processing
		locals.winnersOverflow = locals.winnersBlock - locals.k3Pool - locals.k2Pool;

		// Fast-Recovery (FR) mode: redirect portions of Dev/Distribution to jackpot with deficit-driven extra.
		// Base redirect is always 1% Dev + 1% Dist when FR=ON.
		// Extra redirect is calculated dynamically based on deficit, expected k4 timing, and ticket volume.
		if (state.frActive)
		{
			// Calculate deficit to target jackpot
			locals.delta = (state.jackpot < state.targetJackpot) ? (state.targetJackpot - state.jackpot) : 0;

			// Estimate base gain from existing FR mechanisms (without extra)
			locals.calcBaseGainInput.revenue = locals.revenue;
			locals.calcBaseGainInput.winnersBlock = locals.calcPoolsOutput.winnersBlock;
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
			locals.totalDevRedirectBP = sadd(QTF_FR_DEV_REDIRECT_BP, locals.devExtraBP);
			locals.totalDistRedirectBP = sadd(QTF_FR_DIST_REDIRECT_BP, locals.distExtraBP);

			// Calculate redirect amounts
			locals.devRedirect = div<uint64>(smul(locals.revenue, locals.totalDevRedirectBP), 10000);
			locals.distRedirect = div<uint64>(smul(locals.revenue, locals.totalDistRedirectBP), 10000);

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
		}

		locals.k2PayoutPool = locals.k2Pool; // mutable pools after top-ups
		locals.k3PayoutPool = locals.k3Pool;

		// Reset last-winner snapshot for this settlement (per-round view).
		clearWinerData(state);

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
		// First, get total QRP balance for safety limit calculations (10% of total reserve per round).
		CALL_OTHER_CONTRACT_FUNCTION(QRP, GetAvailableReserve, locals.qrpGetAvailableInput, locals.qrpGetAvailableOutput);
		locals.totalQRPBalance = locals.qrpGetAvailableOutput.availableReserve;
		// If a k=4 win happened this round, the jackpot reseed must not be blocked by floor top-ups.
		// We emulate reserve partitioning by limiting k2/k3 top-ups to the portion of QRP above targetJackpot.
		// This guarantees that if QRP had >= targetJackpot before settlement, reseed can still reach target after payouts.
		if (locals.countK4 > 0)
		{
			if (locals.totalQRPBalance > state.targetJackpot)
			{
				locals.totalQRPBalance -= state.targetJackpot;
			}
			else
			{
				locals.totalQRPBalance = 0;
			}
		}
		locals.perWinnerCap = smul(state.ticketPrice, QTF_TOPUP_PER_WINNER_CAP_MULT); // 25*P

		// Process k2 tier payout
		locals.tierPayoutInput.floorPerWinner = div<uint64>(smul(state.ticketPrice, QTF_K2_FLOOR_MULT), QTF_K2_FLOOR_DIV);
		locals.tierPayoutInput.winnerCount = locals.countK2;
		locals.tierPayoutInput.payoutPool = locals.k2PayoutPool;
		locals.tierPayoutInput.perWinnerCap = locals.perWinnerCap;
		locals.tierPayoutInput.totalQRPBalance = locals.totalQRPBalance;
		locals.tierPayoutInput.ticketPrice = state.ticketPrice;
		CALL(ProcessTierPayout, locals.tierPayoutInput, locals.tierPayoutOutput);
		locals.k2PerWinner = locals.tierPayoutOutput.perWinnerPayout;
		locals.winnersOverflow = sadd(locals.winnersOverflow, locals.tierPayoutOutput.overflow);

		// Process k3 tier payout
		locals.tierPayoutInput.floorPerWinner = smul(state.ticketPrice, QTF_K3_FLOOR_MULT);
		locals.tierPayoutInput.winnerCount = locals.countK3;
		locals.tierPayoutInput.payoutPool = locals.k3PayoutPool;
		locals.tierPayoutInput.perWinnerCap = locals.perWinnerCap;
		locals.tierPayoutInput.totalQRPBalance = locals.totalQRPBalance;
		locals.tierPayoutInput.ticketPrice = state.ticketPrice;
		CALL(ProcessTierPayout, locals.tierPayoutInput, locals.tierPayoutOutput);
		locals.k3PerWinner = locals.tierPayoutOutput.perWinnerPayout;
		locals.winnersOverflow = sadd(locals.winnersOverflow, locals.tierPayoutOutput.overflow);

		locals.carryAdd = sadd(locals.carryAdd, locals.winnersRake);

		// Compute k=4 jackpot payout per winner once.
		locals.jackpotPerK4Winner = 0;
		if (locals.countK4 > 0)
		{
			locals.jackpotPerK4Winner = div(state.jackpot, locals.countK4);
		}

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
			if (locals.matches == 3 && locals.countK3 > 0 && locals.k3PerWinner > 0)
			{
				qpi.transfer(state.players.get(locals.i).player, locals.k3PerWinner);
				fillWinnerData(state, state.players.get(locals.i), locals.winningValues, locals.currentEpoch);
			}
			// k4 payout (jackpot)
			if (locals.matches == 4 && locals.countK4 > 0)
			{
				if (locals.jackpotPerK4Winner > 0)
				{
					qpi.transfer(state.players.get(locals.i).player, locals.jackpotPerK4Winner);
				}
				fillWinnerData(state, state.players.get(locals.i), locals.winningValues, locals.currentEpoch);
			}

			++locals.i;
		}

		// Always save winning values and epoch, even if no winners
		state.lastWinnerData.winnerValues = locals.winningValues;
		state.lastWinnerData.epoch = locals.currentEpoch;

		// Post-jackpot (k4) logic: reset counters and reseed if jackpot was hit
		if (locals.countK4 > 0)
		{
			// Jackpot was paid out in combined loop above, now deplete it
			state.jackpot = 0;

			// Reset FR counters after jackpot hit
			state.frRoundsSinceK4 = 0;
			state.frRoundsAtOrAboveTarget = 0;

			// Reseed jackpot from QReservePool (up to targetJackpot or available reserve)
			// Re-query available reserve because k2/k3 top-ups may have reduced it.
			CALL_OTHER_CONTRACT_FUNCTION(QRP, GetAvailableReserve, locals.qrpGetAvailableInput, locals.qrpGetAvailableOutput);
			locals.qrpRequested = RL::min(locals.qrpGetAvailableOutput.availableReserve, state.targetJackpot);
			if (locals.qrpRequested > 0)
			{
				locals.qrpGetReserveInput.revenue = locals.qrpRequested;
				INVOKE_OTHER_CONTRACT_PROCEDURE(QRP, WithdrawReserve, locals.qrpGetReserveInput, locals.qrpGetReserveOutput, 0ll);

				if (locals.qrpGetReserveOutput.returnCode == QRP::toReturnCode(QRP::EReturnCode::SUCCESS))
				{
					locals.qrpReceived = locals.qrpGetReserveOutput.allocatedRevenue;
					state.jackpot = sadd(state.jackpot, locals.qrpReceived);
				}
			}
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
		locals.totalJackpotContribution = sadd(locals.carryAdd, sadd(locals.devRedirect, locals.distRedirect));
		state.jackpot = sadd(state.jackpot, locals.totalJackpotContribution);

		// Transfer reserve overflow to QReservePool
		if (locals.reserveAdd > 0)
		{
			qpi.transfer(QTF_RESERVE_POOL_CONTRACT_ID, locals.reserveAdd);
		}

		if (locals.devPayout > 0)
		{
			qpi.transfer(state.teamAddress, locals.devPayout);
		}
		// Manual dividend payout to RL shareholders (no extra fee).
		if (locals.distPayout > 0)
		{
			locals.rlAsset.issuer = id::zero();
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
	 * - Revenue calculation resulted in 0 (overflow or invalid state)
	 * - Contract balance is insufficient for settlement
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
			locals.randomValue = input.playerValues.get(locals.i);
			ASSERT(locals.randomValue > 0 && locals.randomValue <= QTF_MAX_RANDOM_VALUE);

			locals.maskA |= 1u << locals.randomValue;
		}

		for (locals.i = 0; locals.i < input.winningValues.capacity(); ++locals.i)
		{
			locals.randomValue = input.winningValues.get(locals.i);
			ASSERT(locals.randomValue > 0 && locals.randomValue <= QTF_MAX_RANDOM_VALUE);

			locals.maskB |= 1u << locals.randomValue;
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
			locals.candidate = static_cast<uint8>(sadd(mod(locals.tempValue, QTF_MAX_RANDOM_VALUE), 1ull));

			locals.attempts = 0;
			while (locals.used.contains(locals.candidate) && locals.attempts < QTF_MAX_RANDOM_GENERATION_ATTEMPTS)
			{
				++locals.attempts;
				// Regenerate a fresh candidate from the evolving tempValue until it is unique
				locals.tempValue ^= locals.tempValue >> 12;
				locals.tempValue ^= locals.tempValue << 25;
				locals.tempValue ^= locals.tempValue >> 27;
				locals.tempValue *= 2685821657736338717ULL;
				locals.candidate = static_cast<uint8>(sadd(mod(locals.tempValue, QTF_MAX_RANDOM_VALUE), 1ull));
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
	 * Safety limits per spec:
	 * - Max 10% of total QRP reserve per round
	 * - Soft floor: keep at least 20*P in QRP reserve
	 * - Per-winner cap: max 25*P per winner
	 *
	 * @param input.totalQRPBalance - Actual QRP contract balance (for 10% limit and soft floor)
	 * @param input.needed - Amount needed for top-up
	 * @param input.perWinnerCapTotal - Per-winner cap multiplied by winner count
	 * @param input.ticketPrice - Current ticket price
	 * @param output.topUpAmount - Safe amount to top up from reserve
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalcReserveTopUp)
	{
		if (input.totalQRPBalance == 0)
		{
			output.topUpAmount = 0;
			return;
		}

		// Calculate soft floor: keep at least 20 * P in total QRP reserve
		locals.softFloor = smul(input.ticketPrice, QTF_RESERVE_SOFT_FLOOR_MULT);

		// Calculate available reserve from QRP (above soft floor)
		if (input.totalQRPBalance > locals.softFloor)
		{
			locals.availableAboveFloor = input.totalQRPBalance - locals.softFloor;
		}
		else
		{
			locals.availableAboveFloor = 0;
		}

		// Calculate max per round (10% of total QRP reserve per spec)
		locals.maxPerRound = div<uint64>(smul(input.totalQRPBalance, QTF_TOPUP_RESERVE_PCT_BP), 10000);
		// Cap by available above floor
		locals.maxPerRound = RL::min(locals.maxPerRound, locals.availableAboveFloor);
		// Cap by per-winner cap
		locals.maxPerRound = RL::min(locals.maxPerRound, input.perWinnerCapTotal);

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
	 * @brief Unified tier payout processing for k2/k3 winners.
	 *
	 * Handles floor guarantee with QRP top-up if pool is insufficient.
	 * Calculates per-winner payout (capped at perWinnerCap) and overflow.
	 *
	 * @param input.floorPerWinner - Floor payout per winner (0.5*P for k2, 5*P for k3)
	 * @param input.winnerCount - Number of winners in this tier
	 * @param input.payoutPool - Initial payout pool for this tier
	 * @param input.perWinnerCap - Per-winner cap (25*P)
	 * @param input.totalQRPBalance - QRP balance for safety limits
	 * @param input.ticketPrice - Current ticket price
	 * @param output.perWinnerPayout - Calculated per-winner payout
	 * @param output.overflow - Overflow amount (unused funds)
	 * @param output.topUpReceived - Amount received from QRP top-up
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessTierPayout)
	{
		output.topUpReceived = 0;
		output.overflow = 0;
		output.perWinnerPayout = 0;

		if (input.winnerCount == 0)
		{
			// No winners: entire pool becomes overflow
			output.overflow = input.payoutPool;
			return;
		}

		locals.floorTotalNeeded = smul(input.floorPerWinner, input.winnerCount);
		locals.finalPool = input.payoutPool;

		// Top-up from QRP if pool insufficient for floor guarantee
		if (input.payoutPool < locals.floorTotalNeeded)
		{
			locals.calcTopUpInput.totalQRPBalance = input.totalQRPBalance;
			locals.calcTopUpInput.needed = locals.floorTotalNeeded - input.payoutPool;
			locals.calcTopUpInput.perWinnerCapTotal = smul(input.perWinnerCap, input.winnerCount);
			locals.calcTopUpInput.ticketPrice = input.ticketPrice;
			CALL(CalcReserveTopUp, locals.calcTopUpInput, locals.calcTopUpOutput);

			locals.qrpRequested = RL::min(locals.calcTopUpOutput.topUpAmount, locals.floorTotalNeeded - input.payoutPool);
			if (locals.qrpRequested > 0)
			{
				locals.qrpGetReserveInput.revenue = locals.qrpRequested;
				INVOKE_OTHER_CONTRACT_PROCEDURE(QRP, WithdrawReserve, locals.qrpGetReserveInput, locals.qrpGetReserveOutput, 0ll);

				if (locals.qrpGetReserveOutput.returnCode == QRP::toReturnCode(QRP::EReturnCode::SUCCESS))
				{
					output.topUpReceived = locals.qrpGetReserveOutput.allocatedRevenue;
					locals.finalPool = sadd(input.payoutPool, output.topUpReceived);
				}
			}
		}

		// Calculate per-winner payout (capped at perWinnerCap)
		output.perWinnerPayout = RL::min(input.perWinnerCap, div<uint64>(locals.finalPool, input.winnerCount));
		output.overflow = locals.finalPool - smul(output.perWinnerPayout, input.winnerCount);
	}

	/**
	 * @brief Calculate k2/k3 prize pools from revenue (reusable for settlement and estimation).
	 *
	 * This function encapsulates the common logic for calculating prize pools:
	 * 1. Get fee percentages from RL contract
	 * 2. Calculate winners block (typically 68% of revenue)
	 * 3. Apply FR rake if active (5% of winners block)
	 * 4. Split remaining into k2 (28%) and k3 (40%) pools
	 *
	 * @param input.revenue - Total revenue from ticket sales
	 * @param input.applyFRRake - Whether to apply 5% FR rake
	 * @param output.winnersBlock - Winners block after rake
	 * @param output.winnersRake - Amount taken as FR rake (0 if not applied)
	 * @param output.k2Pool - Prize pool for k=2 tier
	 * @param output.k3Pool - Prize pool for k=3 tier
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CalculatePrizePools)
	{
		if (input.revenue == 0)
		{
			output.winnersBlock = 0;
			output.winnersRake = 0;
			output.k2Pool = 0;
			output.k3Pool = 0;
			return;
		}

		// Get fee percentages from RL contract
		CALL(GetFees, locals.feesInput, locals.feesOutput);

		// Calculate winners block (typically 68% of revenue)
		locals.winnersBlockBeforeRake = div<uint64>(smul(input.revenue, static_cast<uint64>(locals.feesOutput.winnerFeePercent)), 100);

		// Apply FR rake if requested
		if (input.applyFRRake)
		{
			output.winnersRake = div<uint64>(smul(locals.winnersBlockBeforeRake, QTF_FR_WINNERS_RAKE_BP), 10000);
			output.winnersBlock = locals.winnersBlockBeforeRake - output.winnersRake;
		}
		else
		{
			output.winnersRake = 0;
			output.winnersBlock = locals.winnersBlockBeforeRake;
		}

		// Calculate k2 and k3 pools using shared static function
		calcK2K3Pool(output.winnersBlock, output.k2Pool, output.k3Pool);
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

		// Overflow estimate: assume ~10% of winnersBlock not paid out (conservative heuristic)
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

	static void calcK2K3Pool(uint64 winnersBlock, uint64& outK2Pool, uint64& outK3Pool)
	{
		outK3Pool = div<uint64>(smul(winnersBlock, QTF_BASE_K3_SHARE_BP), 10000);
		outK2Pool = div<uint64>(smul(winnersBlock, QTF_BASE_K2_SHARE_BP), 10000);
	}
};
