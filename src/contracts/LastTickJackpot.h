#pragma once

using namespace QPI;

// Requested empty struct
struct LTJ2 { };

struct LTJ : public ContractBase
{
public:
    // I/O
    struct Init_input   { id owner; };
    struct Init_output  {};

    struct SetParams_input { uint64 ticketPrice; uint64 extendTicks; uint64 minExtendTail; uint64 maxExtendCap; uint64 startTimer; };
    struct SetParams_output {};

    struct SetPerc_input { uint64 feeNum; uint64 feeDen; uint64 carryNum; uint64 carryDen; };
    struct SetPerc_output {};

    struct SetFeeDev_input { uint64 num; uint64 den; };
    struct SetFeeDev_output {};

    struct StartRound_input {};
    struct StartRound_output {};

    struct Buy_input { uint64 qty; id ref; };
    struct Buy_output {};

    struct Finalize_input {};
    struct Finalize_output {};

    struct Quote_input { uint64 qty; };
    struct Quote_output { uint128 total; };

    struct GetRound_input {};
    struct GetRound_output {
        uint64  roundId;
        uint64  endTick;
        uint128 pot;
        id      lastBuyer;
        uint64  ticketPrice;
        uint64  extendTicks;
        uint64  timeLeft;
        bit     active;
    };

    struct History_input { uint64 back; };
    struct History_output { uint64 rid; id winner; uint128 prize; };

protected:
    // Constants (ticks)
    static constexpr uint64 DEFAULT_TICKET_PRICE = 100000;
    static constexpr uint64 DEFAULT_EXTEND_TICKS = 20;
    static constexpr uint64 DEFAULT_MIN_TAIL     = 10;
    static constexpr uint64 DEFAULT_MAX_CAP      = 300;
    static constexpr uint64 DEFAULT_START_TIMER  = 60;
    static constexpr uint64 MAX_QTY              = 10;
    static constexpr uint64 COOLDOWN_TICKS       = 5;
    static constexpr uint64 RING                 = 256;

    // Scheme #2
    static constexpr uint64 DEF_FEE_NUM = 8,  DEF_FEE_DEN = 100;  // 8%
    static constexpr uint64 DEF_CAR_NUM = 2,  DEF_CAR_DEN = 100;  // 2%
    static constexpr uint64 DEF_FD_NUM  = 3,  DEF_FD_DEN  = 8;    // 3/8 of fee

    // Structures for arrays
    struct AntiRec { id addr = id::zero(); uint64 tick = 0; }; 
    struct HistRec { uint64 rid = 0; id winner = id::zero(); uint128 prize = 0; };

        // Parameters
        id     owner;
        uint64 ticketPrice, extendTicks, minExtendTail, maxExtendCap, startTimer;
        // Shares
        uint64 feeNum, feeDen, carryNum, carryDen, feeDevNum, feeDevDen;
        // Round
        uint64  roundId, startTick, endTick;
        id      lastBuyer;
        uint128 pot;
        bit     active;
        // No-contest
        id      firstBuyer;
        bit     hasSecond;
        uint128 roundSeed;
        // Anti-spam and history (arrays of structures)
        Array<AntiRec, RING> cd;    uint64 cd_head;
        Array<HistRec, RING> hist;  uint64 hist_head;

    // Utilities
    static uint64  mod_u64(uint64 a, uint64 m)  { return mod(a, m); }
    static uint128 mul_u128_u64(uint64 a, uint64 b) { return (uint128)a * (uint128)b; }
    uint128 mul_div_u128_u64_u64(uint128 amount, uint64 num, uint64 den) {
        if (!den) return 0; return div(amount * (uint128)num, (uint128)den);
    }

    bit cooldown_ok_and_touch(const id& who, uint64 now) {
        uint64 i = 0;
        while (i < RING) {
            AntiRec rec = cd.get(i);
            if (rec.addr == who) {
                if (now < rec.tick + COOLDOWN_TICKS) return 0;
                rec.tick = now; cd.set(i, rec); return 1;
            }
            i = i + 1;
        }
        uint64 h = cd_head;
        AntiRec nr; nr.addr = who; nr.tick = now;
        cd.set(h, nr);
        cd_head = mod_u64(h + 1, RING);
        return 1;
    }

    void history_push(uint64 rid, const id& winner, uint128 prize) {
        uint64 i = hist_head;
        HistRec r; r.rid = rid; r.winner = winner; r.prize = prize;
        hist.set(i,r);
        hist_head = mod_u64(i + 1, RING);
    }

    void start_new_round_with_carry(uint64 now) {
        roundId   = roundId + 1;
        startTick = now;
        endTick   = now + startTimer;
        lastBuyer = id::zero();
        firstBuyer= id::zero();
        hasSecond = 0;
        active    = 1;
        roundSeed = pot; // fix seed
    }

    void finalize_no_contest(const QPI::QpiContextProcedureCall& qpi, uint64 now) {
        id      sole       = firstBuyer;
        uint128 refundable = (pot > roundSeed) ? (pot - roundSeed) : (uint128)0;
        active = 0; lastBuyer = id::zero(); pot = roundSeed;
        if (sole != id::zero() && refundable > uint128(0)) qpi.transfer(sole, refundable);
        history_push(roundId, id::zero(), (uint128)0);
        start_new_round_with_carry(now);
    }

    void finalize_regular(const QPI::QpiContextProcedureCall& qpi, uint64 now) {
        uint128 bank = pot; id winner = lastBuyer; uint64 rid = roundId;
        active = 0; pot = 0; lastBuyer = id::zero();

        uint128 fee_total = mul_div_u128_u64_u64(bank, feeNum,   feeDen);   // 8%
        uint128 carry     = mul_div_u128_u64_u64(bank, carryNum, carryDen); // 2%
        uint128 fee_dev   = mul_div_u128_u64_u64(fee_total, feeDevNum, feeDevDen); // 3/8 fee
        uint128 fee_div   = fee_total - fee_dev; (void)fee_div; // remains in contract
        uint128 prize     = bank - fee_total - carry;

        if (winner != id::zero() && prize > uint128(0)) qpi.transfer(winner, prize);
        id dev = owner; if (fee_dev > uint128(0)) qpi.transfer(dev, fee_dev);
        pot = carry;
        history_push(rid, winner, prize);
        start_new_round_with_carry(now);
    }

    void finalize_internal(const QPI::QpiContextProcedureCall& qpi) {
        uint64 now = qpi.tick();
        if (!hasSecond) { finalize_no_contest(qpi, now); } else { finalize_regular(qpi, now); }
    }

public:
    // ===== Procedures =====
    PUBLIC_PROCEDURE(Init) {
        if (state.owner != id::zero()) return;
        state.owner         = input.owner;
        state.ticketPrice   = DEFAULT_TICKET_PRICE;
        state.extendTicks   = DEFAULT_EXTEND_TICKS;
        state.minExtendTail = DEFAULT_MIN_TAIL;
        state.maxExtendCap  = DEFAULT_MAX_CAP;
        state.startTimer    = DEFAULT_START_TIMER;
        state.feeNum = DEF_FEE_NUM; state.feeDen = DEF_FEE_DEN;
        state.carryNum = DEF_CAR_NUM; state.carryDen = DEF_CAR_DEN;
        state.feeDevNum = DEF_FD_NUM; state.feeDevDen = DEF_FD_DEN;
        state.roundId=0; state.startTick=0; state.endTick=0; state.lastBuyer=id::zero();
        state.pot=0; state.active=0; state.firstBuyer=id::zero(); state.hasSecond=0; state.roundSeed=0;
        state.cd_head=0; state.hist_head=0;
    }

    PUBLIC_PROCEDURE(SetParams) {
        if (qpi.invocator() != state.owner) return;
        if (state.active) return;
        state.ticketPrice   = input.ticketPrice;
        state.extendTicks   = input.extendTicks;
        state.minExtendTail = input.minExtendTail;
        state.maxExtendCap  = input.maxExtendCap;
        state.startTimer    = input.startTimer;
    }

    PUBLIC_PROCEDURE(SetPerc) {
        if (qpi.invocator() != state.owner) return;
        if (state.active) return;
        if (!input.feeDen || !input.carryDen) return;
        uint128 lhs = (uint128)input.feeNum * (uint128)input.carryDen +
                      (uint128)input.carryNum * (uint128)input.feeDen;
        uint128 rhs = (uint128)input.feeDen * (uint128)input.carryDen;
        if (lhs > rhs) return;
        state.feeNum = input.feeNum; state.feeDen = input.feeDen;
        state.carryNum = input.carryNum; state.carryDen = input.carryDen;
    }

    PUBLIC_PROCEDURE(SetFeeDev) {
        if (qpi.invocator() != state.owner) return;
        if (state.active) return;
        if (!input.den || input.num > input.den) return;
        state.feeDevNum = input.num; state.feeDevDen = input.den;
    }

    PUBLIC_PROCEDURE(StartRound) {
        if (qpi.invocator() != state.owner) return;
        if (state.active) return;
        state.start_new_round_with_carry(qpi.tick());
    }

    PUBLIC_PROCEDURE(Buy) {
        if (!state.active) return;
        uint64 now = qpi.tick();
        if (now >= state.endTick) { state.finalize_internal(qpi); return; }
        if (input.qty == 0 || input.qty > MAX_QTY) {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        if (!state.cooldown_ok_and_touch(qpi.invocator(), now)) {
            if (qpi.invocationReward() > 0) qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }
        uint128 expected = state.mul_u128_u64(state.ticketPrice, input.qty);
        uint128 paid     = qpi.invocationReward();
        if (!(paid == expected)) { if (paid > (uint128)0) qpi.transfer(qpi.invocator(), paid); return; }

        if (state.firstBuyer == id::zero()) state.firstBuyer = qpi.invocator();
        else if (qpi.invocator() != state.firstBuyer) state.hasSecond = 1;

        state.pot = state.pot + paid;
        state.lastBuyer = qpi.invocator();

        uint64 add  = state.extendTicks * input.qty;
        uint64 left = (state.endTick > now) ? (state.endTick - now) : 0;
        if (left < 60 && add < state.minExtendTail) add = state.minExtendTail;

        uint64 cappedEnd = now + state.maxExtendCap;
        uint64 newEnd    = state.endTick + add;
        if (newEnd > cappedEnd) newEnd = cappedEnd;
        state.endTick = newEnd;
    }

    PUBLIC_PROCEDURE(Finalize) {
        if (!state.active) return;
        if (qpi.tick() < state.endTick) return;
        state.finalize_internal(qpi);
    }

    // ===== Functions =====
    PUBLIC_FUNCTION(GetRound) {
        output.roundId     = state.roundId;
        output.endTick     = state.endTick;
        output.pot         = state.pot;
        output.lastBuyer   = state.lastBuyer;
        output.ticketPrice = state.ticketPrice;
        output.extendTicks = state.extendTicks;
        uint64 now = qpi.tick();
        output.timeLeft = (!state.active || now >= state.endTick) ? 0 : (state.endTick - now);
        output.active   = state.active;
    }

    PUBLIC_FUNCTION(Quote) {
        output.total = mul_u128_u64(state.ticketPrice, input.qty);
    }

    PUBLIC_FUNCTION(History) {
        if (input.back >= RING) { output.rid=0; output.winner=id::zero(); output.prize=(uint128)0; return; }
        uint64 head = state.hist_head;
        uint64 idx  = mod_u64(head + RING - 1 - input.back, RING);
        HistRec r = state.hist.get(idx);
        output.rid = r.rid; output.winner = r.winner; output.prize = r.prize;
    }

    // ===== Registration =====
    REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
        REGISTER_USER_FUNCTION(GetRound,  1);
        REGISTER_USER_FUNCTION(Quote,     2);
        REGISTER_USER_FUNCTION(History,   3);

        REGISTER_USER_PROCEDURE(Init,        1);
        REGISTER_USER_PROCEDURE(SetParams,   2);
        REGISTER_USER_PROCEDURE(SetPerc,     3);
        REGISTER_USER_PROCEDURE(SetFeeDev,   4);
        REGISTER_USER_PROCEDURE(StartRound,  5);
        REGISTER_USER_PROCEDURE(Buy,         6);
        REGISTER_USER_PROCEDURE(Finalize,    7);
    }

    // ===== Initialization =====
    INITIALIZE() {
        state.owner = id::zero();

        state.ticketPrice   = DEFAULT_TICKET_PRICE;
        state.extendTicks   = DEFAULT_EXTEND_TICKS;
        state.minExtendTail = DEFAULT_MIN_TAIL;
        state.maxExtendCap  = DEFAULT_MAX_CAP;
        state.startTimer    = DEFAULT_START_TIMER;

        state.feeNum   = DEF_FEE_NUM;  state.feeDen   = DEF_FEE_DEN;
        state.carryNum = DEF_CAR_NUM;  state.carryDen = DEF_CAR_DEN;
        state.feeDevNum= DEF_FD_NUM;   state.feeDevDen= DEF_FD_DEN;

        state.roundId=0; state.startTick=0; state.endTick=0; state.lastBuyer=id::zero();
        state.pot=0; state.active=0; state.firstBuyer=id::zero(); state.hasSecond=0; state.roundSeed=0;
        state.cd_head=0; state.hist_head=0;
    }
};
