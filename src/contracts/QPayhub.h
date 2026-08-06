using namespace QPI;

// ============================================================================
// QPAY — x402 settlement layer for Qubic: on-chain receipts for
// pay-per-request commerce, plus a live QUBIC/USDT price feed subscribed
// from Qubic own Oracle Machines via QPI.
//
// FLOW
//   A buyer invokes Pay with the purchase descriptor (seller, resourceId,
//   nonce) and the price attached as the invocation reward. The contract
//   retains a protocol fee, pushes the net amount to the seller in the same
//   transaction (non-custodial), and records a receipt keyed by
//   K12(payer, seller, resourceId, nonce). An off-chain facilitator reads
//   the receipt via GetReceipt to confirm payment before serving the
//   resource; the seller may additionally call Consume to mark a receipt
//   used on-chain, making single-use enforcement global rather than
//   per-facilitator.
//
// WHAT THIS CONTRACT DOES NOT DO
//   No pricing for Pay itself (the 402 challenge is off-chain; the
//   facilitator compares amountPaid against the advertised price), no
//   delivery escrow, no seller registry — any identity can receive
//   payments with zero registration.
//
// FEES
//   Protocol fee is QPAY_FEE_PERMILLE of each payment (0.75%) or
//   QPAY_FEE_FLOOR_QU (100 QU), whichever is greater - modeled like a card
//   processor's percentage-plus-minimum, so a sale small enough that 0.75%
//   would round to near-nothing still pays a floor rather than the
//   protocol effectively processing it for free. The floor can never push
//   fee above the paid amount itself (see Pay_locals.fee clamp) - the
//   worst case is fee == amount, net == 0, at a payment right at the
//   QPAY_MIN_PAYMENT boundary, never a negative transfer to the seller.
//   Accrued to feePool and distributed each epoch above an execution-fee
//   reserve: 10% to QPAY shareholders, 90% pro-rata to QPAY token holders.
//   Receipts expire after QPAY_RECEIPT_RETENTION_EPOCHS epochs.
//
//   REFUNDS: the fee is NOT returned on a refund. No path pays out of
//   feePool, and invoice.js validates refunds against the GROSS amount,
//   so a full refund costs the merchant the fee - as card processors do.
//
//   Pricing near or below the floor nets the seller close to nothing,
//   same as pricing near a card processor's minimum fee - not a bug, but
//   worth flagging: this project's OTHER default, QUBIC_PRICE_PER_CALL in
//   the off-chain facilitator's .env, defaults to exactly 1000 QU, which
//   nets a seller 900 QU under this fee once routed through QPAY (0.75% of
//   1000 is 7, so the 100 QU floor applies). Anyone pricing per-call
//   sales through QPAY should be aware the floor, not the percentage,
//   dominates below 10,000 QU.
//
// TEST STATUS (append-only, keep accurate)
//   Pay/Consume/GetReceipt/ComputeReceiptKey/GetInfo/END_EPOCH/
//   POST_INCOMING_TRANSFER: 11/11 GoogleTest unit + 19/19 live-chain
//   integration checks passed on a real aio-qubic-dev-kit testnet - see
//   contract/TEST-RESULTS.md. Nothing in this section of the contract
//   changed as part of folding in the oracle-price feed below; the merge
//   was verified as a strict, isolated addition (diffed line-for-line
//   against the last-tested version before merging).
//   GetQuUsdPrice/SubscribeToPriceFeed/NotifyQuUsdPriceReply (the oracle-
//   price feed, search ORACLE PRICE FEED ADDITIONS below): syntax-checked
//   only so far (0 errors, 0 prohibited tokens - contract/check.cpp). Not
//   yet run through GoogleTest or a live chain - that is the next gate,
//   same two-step bar every other piece of this contract already cleared.
//   Pay's fee-floor clamp (QPAY_FEE_FLOOR_QU, search FEE FLOOR ADDITION
//   below): syntax-checked against real qubic/core headers cross-compiled
//   to x86_64 from this arm64 machine (0 errors - contract/check.cpp; the
//   full GoogleTest harness itself still needs the real x86 toolchain per
//   the note above, same constraint that blocked the oracle additions'
//   test file from even a syntax check). New GoogleTest cases were added
//   to contract/test/contract_qpay.cpp for floor-dominates, percent-
//   dominates, and the exact-tie amount, modeled directly on the existing
//   PayHappyPathFeeSplitAcrossAmounts test - reviewed by hand, not run.
//
// WHY THE ORACLE ADDITIONS EXIST
//   src/priceOracle.js (the off-chain USD->QU pricing used by the POS/
//   checkout invoices) currently sources its rate from CoinGecko, because
//   an off-chain Node process cannot reach Qubic Oracle Machines
//   directly - interaction with them is managed via QPI, a smart-contract-
//   only interface (see the README POS section for the full reasoning on why
//   an undocumented raw-protocol integration was not attempted). But QPAY
//   already IS a deployed contract sitting exactly at that boundary. If
//   QPAY itself subscribes to the oracle and republishes the price through
//   a normal read function, the facilitator can read it the exact same way
//   it already reads GetReceipt - via querySmartContract, no new off-chain
//   networking risk at all. The oracle interaction happens where it is
//   actually supported: inside the contract, via QPI.
//
// DESIGN CHOICE WORTH FLAGGING: staying admin-free.
//   QPAY whole identity is no admin, no seizure surface (see the
//   the original file). Subscribing to an oracle could easily have
//   been bolted on as an admin-only procedure, but that would reintroduce
//   exactly the privileged-actor surface QPAY was built to avoid. Instead,
//   SubscribeToPriceFeed takes NO caller-supplied query parameters at all -
//   the oracle source and currency pair are fixed constants inside the
//   procedure body, so ANYONE can permissionlessly call it (e.g. to renew
//   an expired subscription) without being able to redirect the feed to a
//   bogus source. Permissionless renewal, not permissioned control.
//
// VERIFIED BEFORE WRITING THIS:
//   - The oracle interaction pattern (QUERY_ORACLE/SUBSCRIBE_ORACLE macros,
//     the OracleNotificationInput callback shape, fee functions) is copied
//     from src/contracts/QUtil.h and src/contracts/TestExampleC.h in the
//     qubic/core checkout on this machine - both are real, already-
//     compiling contracts using this exact interface, not a guess.
//   - OI::Price (src/oracle_interfaces/Price.h) is a real, live oracle
//     interface: OracleQuery{oracle, timestamp, currency1, currency2} ->
//     OracleReply{numerator, denominator}, where
//     currency1 = currency2 * numerator / denominator.
//   - OI:: is auto-available to every contract via
//     contract_core/contract_def.h including oracle_core/oracle_interfaces_def.h
//     before any contract file - no include directive needed here,
//     consistent with the no-include rule.
//   - The id(...) five-letter constructor (src/platform/m256.h) takes five
//     REQUIRED single-letter params - id(Q,U,B,I,C) is exactly QUBIC, no
//     padding needed; id(U,S,D,T) for USDT leaves the unused 5th param at
//     its default of 0.
//
// NOT YET VERIFIED (this file has only had the standalone syntax check
// run against it, same as Qpay.h originally did - GoogleTest coverage and
// the exact subscription renewal/lifetime semantics need the real test
// harness on x86, see oracle_testing.h in the core checkout):
//   - Whether a subscription persists indefinitely or needs periodic
//     re-subscription - designed conservatively so calling
//     SubscribeToPriceFeed again when priceOracleSubscriptionId is already
//     valid can serve as a manual renewal if needed.
//   - The real numeric magnitude/precision of a live QUBIC/USDT reply
//     (this was written for the numerator/denominator SHAPE Price.h
//     documents, not against an observed live value for this specific
//     pair - QUBIC actual listing symbol on Binance/MEXC may need
//     confirming, e.g. whether it is paired directly against USDT or only
//     via an intermediate).
// ============================================================================

constexpr uint64 QPAYHUB_RECEIPT_CAPACITY = 262144; // 2^18 (was 65536) -- ~9.4k/day mainnet headroom
constexpr sint64 QPAYHUB_MIN_PAYMENT = 100;        // dust floor, QU - rejects the payment outright
constexpr uint64 QPAYHUB_FEE_PERMILLE = 75;        // 75 per 10000 = 0.75 percent
// Floor on the fee itself, not a rejection threshold like MIN_PAYMENT.
constexpr sint64 QPAYHUB_FEE_FLOOR_QU = 100;
constexpr uint32 QPAYHUB_RECEIPT_RETENTION_EPOCHS = 2;
constexpr sint64 QPAYHUB_EXEC_RESERVE = 1000000;   // never distributed, QU

// Epoch fee pool splits: shares get this, QPAY-token holders the rest.
constexpr uint64 QPAYHUB_DIVIDEND_SHAREHOLDER_PERMILLE = 100;   // 100 per 1000 = 10%
constexpr uint64 QPAYHUB_TOKEN_ASSETNAME = 1497452625ULL;       // "QPAY"

// ---- ORACLE PRICE FEED ADDITIONS: constants ----
// A 16-minute renewal period sits in the efficient tier of the fee table
// (1,784 QU) - frequent enough for an invoice payment window, rare
// enough not to be worth querying per-invoice instead. notifyPreviousValue
// is on, so a fresh subscriber sees the last known reply immediately
// rather than waiting a full period for the first notification.
constexpr uint32 QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS = 16u * 60u * 1000u;
// bit has no constexpr constructor (see money-safety notes below), so this
// is passed as a literal 1 at the SUBSCRIBE_ORACLE call site instead of a
// named constant.
// A price older than this many ticks is reported stale rather than trusted
// silently - callers (GetQuUsdPrice) decide what to do with that, the
// contract just never hides it.
constexpr uint32 QPAYHUB_PRICE_STALE_TICKS = 4000; // roughly 20-25 min at current tick rates

static_assert((QPAYHUB_RECEIPT_CAPACITY & (QPAYHUB_RECEIPT_CAPACITY - 1)) == 0);
static_assert(QPAYHUB_FEE_PERMILLE < 10000);
static_assert(QPAYHUB_DIVIDEND_SHAREHOLDER_PERMILLE <= 1000);

// Return codes — append-only, never renumber.
constexpr uint64 QPAYHUB_OK = 0;
constexpr uint64 QPAYHUB_ERR_INVALID_SELLER = 1;
constexpr uint64 QPAYHUB_ERR_AMOUNT_TOO_LOW = 2;
constexpr uint64 QPAYHUB_ERR_DUPLICATE = 3;
constexpr uint64 QPAYHUB_ERR_CAPACITY = 4;
constexpr uint64 QPAYHUB_ERR_NOT_FOUND = 5;
constexpr uint64 QPAYHUB_ERR_ACCESS_DENIED = 6;
constexpr uint64 QPAYHUB_ERR_ALREADY_CONSUMED = 7;
constexpr uint64 QPAYHUB_ERR_ALREADY_SUBSCRIBED = 8;
constexpr uint64 QPAYHUB_ERR_SUBSCRIBE_FAILED = 9;

struct QPAYHUB2
{
};

struct QPAYHUB : public ContractBase
{
    struct Receipt
    {
        id payer;
        id seller;
        id resourceId;
        uint64 nonce;
        sint64 amountPaid;
        sint64 fee;
        uint32 epochPaid;
        uint32 tickPaid;
        bit consumed;
    };

    // Hashed with K12 to derive the receipt key; deterministic, so buyers
    // and facilitators can compute the key off-chain without a query.
    struct ReceiptKeyMaterial
    {
        id payer;
        id seller;
        id resourceId;
        uint64 nonce;
    };

    struct StateData
    {
        HashMap<id, Receipt, QPAYHUB_RECEIPT_CAPACITY> receipts;
        sint64 feePool;
        uint64 totalPayments;
        uint64 totalVolume;
        uint64 totalFeesCollected;
        uint64 totalFeesDistributed;
        uint64 totalConsumed;
        uint64 totalPurged;

        // ---- ORACLE PRICE FEED ADDITIONS: state ----
        // 1 QUBIC = quUsdNumerator / quUsdDenominator USDT, as of
        // quUsdUpdatedTick. denominator == 0 means no reply has ever
        // landed yet (never trust a fresh contract price before this).
        sint64 quUsdNumerator;
        sint64 quUsdDenominator;
        uint32 quUsdUpdatedTick;
        sint32 priceOracleSubscriptionId; // -1 = not currently subscribed

        // Fixed in INITIALIZE; NULL_ID issuer leaves the 90% in feePool.
        Asset  dividendToken;
        uint64 totalShareholderDividends;
        uint64 totalTokenholderDividends;
    };

    // ------------------------------------------------------------------
    // Input, output and locals structs
    // ------------------------------------------------------------------

    struct Pay_input
    {
        id seller;
        id resourceId;
        uint64 nonce;
    };
    struct Pay_output
    {
        uint64 returnCode;
        id receiptKey;
        sint64 net;
        sint64 fee;
    };
    struct Pay_locals
    {
        ReceiptKeyMaterial km;
        Receipt r;
        id key;
        sint64 amount;
        sint64 fee;
        sint64 net;
    };

    struct Consume_input
    {
        id receiptKey;
    };
    struct Consume_output
    {
        uint64 returnCode;
    };
    struct Consume_locals
    {
        Receipt r;
    };

    struct GetReceipt_input
    {
        id receiptKey;
    };
    struct GetReceipt_output
    {
        uint64 returnCode;
        id payer;
        id seller;
        id resourceId;
        uint64 nonce;
        sint64 amountPaid;
        sint64 fee;
        uint32 epochPaid;
        uint32 tickPaid;
        bit consumed;
    };
    struct GetReceipt_locals
    {
        Receipt r;
    };

    struct ComputeReceiptKey_input
    {
        id payer;
        id seller;
        id resourceId;
        uint64 nonce;
    };
    struct ComputeReceiptKey_output
    {
        id receiptKey;
    };
    struct ComputeReceiptKey_locals
    {
        ReceiptKeyMaterial km;
    };

    struct GetInfo_input
    {
    };
    struct GetInfo_output
    {
        uint64 feePermille;
        sint64 minPayment;
        uint32 retentionEpochs;
        uint32 padding0;
        uint64 receiptCount;
        sint64 feePool;
        uint64 totalPayments;
        uint64 totalVolume;
        uint64 totalFeesCollected;
        uint64 totalFeesDistributed;
        uint64 totalConsumed;
        uint64 totalPurged;
        // ---- FEE FLOOR ADDITION ---- appended at the end rather than
        // inserted by field order, so every existing offset in this struct
        // (see qubicStructs.js's dumped-layout comment) stays unchanged.
        sint64 feeFloorQu;
        // Likewise appended to keep existing offsets stable.
        uint64 shareholderPermille;
        uint64 totalShareholderDividends;
        uint64 totalTokenholderDividends;
    };

    // ---- ORACLE PRICE FEED ADDITIONS: I/O structs ----

    struct GetQuUsdPrice_input
    {
    };
    struct GetQuUsdPrice_output
    {
        sint64 numerator;
        sint64 denominator; // 0 = no price known yet
        uint32 updatedTick;
        bit stale;
    };
    struct GetQuUsdPrice_locals
    {
        uint32 age;
    };

    struct SubscribeToPriceFeed_input
    {
    };
    struct SubscribeToPriceFeed_output
    {
        uint64 returnCode;
        sint32 subscriptionId;
    };
    struct SubscribeToPriceFeed_locals
    {
        OI::Price::OracleQuery query;
        sint64 fee;
    };

    // The reference contracts this pattern is copied from use an older
    // C-style alias keyword here; the modern alias-declaration syntax below
    // has identical effect and is used instead, per the pre-submission
    // checklist in the contract-builder guide.
    using NotifyQuUsdPriceReply_input = OracleNotificationInput<OI::Price>;
    using NotifyQuUsdPriceReply_output = NoData;
    struct NotifyQuUsdPriceReply_locals
    {
        OI::Price::OracleReply reply;
    };

    // ------------------------------------------------------------------
    // Functions (read-only)
    // ------------------------------------------------------------------

    PUBLIC_FUNCTION_WITH_LOCALS(GetReceipt)
    {
        if (!state.get().receipts.get(input.receiptKey, locals.r))
        {
            output.returnCode = QPAYHUB_ERR_NOT_FOUND;
            return;
        }
        output.returnCode = QPAYHUB_OK;
        output.payer = locals.r.payer;
        output.seller = locals.r.seller;
        output.resourceId = locals.r.resourceId;
        output.nonce = locals.r.nonce;
        output.amountPaid = locals.r.amountPaid;
        output.fee = locals.r.fee;
        output.epochPaid = locals.r.epochPaid;
        output.tickPaid = locals.r.tickPaid;
        output.consumed = locals.r.consumed;
    }

    PUBLIC_FUNCTION_WITH_LOCALS(ComputeReceiptKey)
    {
        locals.km.payer = input.payer;
        locals.km.seller = input.seller;
        locals.km.resourceId = input.resourceId;
        locals.km.nonce = input.nonce;
        output.receiptKey = qpi.K12(locals.km);
    }

    PUBLIC_FUNCTION(GetInfo)
    {
        output.feePermille = QPAYHUB_FEE_PERMILLE;
        output.minPayment = QPAYHUB_MIN_PAYMENT;
        output.retentionEpochs = QPAYHUB_RECEIPT_RETENTION_EPOCHS;
        output.padding0 = 0;
        output.receiptCount = state.get().receipts.population();
        output.feePool = state.get().feePool;
        output.totalPayments = state.get().totalPayments;
        output.totalVolume = state.get().totalVolume;
        output.totalFeesCollected = state.get().totalFeesCollected;
        output.totalFeesDistributed = state.get().totalFeesDistributed;
        output.totalConsumed = state.get().totalConsumed;
        output.totalPurged = state.get().totalPurged;
        output.feeFloorQu = QPAYHUB_FEE_FLOOR_QU;
        output.shareholderPermille = QPAYHUB_DIVIDEND_SHAREHOLDER_PERMILLE;
        output.totalShareholderDividends = state.get().totalShareholderDividends;
        output.totalTokenholderDividends = state.get().totalTokenholderDividends;
    }

    // ---- ORACLE PRICE FEED ADDITIONS: read function ----
    // A plain instant read - the oracle interaction already happened in
    // the background via the subscription; this never itself talks to an
    // oracle, so it costs nothing beyond a normal query.
    PUBLIC_FUNCTION_WITH_LOCALS(GetQuUsdPrice)
    {
        output.numerator = state.get().quUsdNumerator;
        output.denominator = state.get().quUsdDenominator;
        output.updatedTick = state.get().quUsdUpdatedTick;
        if (output.denominator == 0)
        {
            output.stale = 1; // never received a reply - trivially stale
            return;
        }
        locals.age = (uint32)qpi.tick() - output.updatedTick;
        output.stale = (locals.age > QPAYHUB_PRICE_STALE_TICKS) ? 1 : 0;
    }

    // ------------------------------------------------------------------
    // Procedures
    // ------------------------------------------------------------------

    PUBLIC_PROCEDURE_WITH_LOCALS(Pay)
    {
        locals.amount = qpi.invocationReward();
        if (input.seller == NULL_ID || input.seller == SELF)
        {
            if (locals.amount > 0)
            {
                qpi.transfer(qpi.invocator(), locals.amount);
            }
            output.returnCode = QPAYHUB_ERR_INVALID_SELLER;
            return;
        }
        if (locals.amount < QPAYHUB_MIN_PAYMENT)
        {
            if (locals.amount > 0)
            {
                qpi.transfer(qpi.invocator(), locals.amount);
            }
            output.returnCode = QPAYHUB_ERR_AMOUNT_TOO_LOW;
            return;
        }

        locals.km.payer = qpi.invocator();
        locals.km.seller = input.seller;
        locals.km.resourceId = input.resourceId;
        locals.km.nonce = input.nonce;
        locals.key = qpi.K12(locals.km);

        // A duplicate key is a replayed payment attempt, not a new purchase.
        if (state.get().receipts.contains(locals.key))
        {
            qpi.transfer(qpi.invocator(), locals.amount);
            output.returnCode = QPAYHUB_ERR_DUPLICATE;
            return;
        }
        // Capacity is checked before any money moves so the receipt insert
        // below can never fail after the seller has been paid.
        if (state.get().receipts.population() >= QPAYHUB_RECEIPT_CAPACITY)
        {
            qpi.transfer(qpi.invocator(), locals.amount);
            output.returnCode = QPAYHUB_ERR_CAPACITY;
            return;
        }

        // 0.75% or QPAYHUB_FEE_FLOOR_QU, whichever is greater. Floor and
        // minimum are both 100, so a payment at the minimum nets the seller zero.
        locals.fee = (sint64)div((uint64)locals.amount * QPAYHUB_FEE_PERMILLE, (uint64)10000);
        if (locals.fee < QPAYHUB_FEE_FLOOR_QU)
        {
            locals.fee = QPAYHUB_FEE_FLOOR_QU;
        }
        if (locals.fee > locals.amount)
        {
            locals.fee = locals.amount;
        }
        locals.net = locals.amount - locals.fee;

        // Effects before the outbound interaction: record everything first,
        // then pay the seller last.
        locals.r.payer = qpi.invocator();
        locals.r.seller = input.seller;
        locals.r.resourceId = input.resourceId;
        locals.r.nonce = input.nonce;
        locals.r.amountPaid = locals.amount;
        locals.r.fee = locals.fee;
        locals.r.epochPaid = (uint32)qpi.epoch();
        locals.r.tickPaid = (uint32)qpi.tick();
        locals.r.consumed = 0;
        state.mut().receipts.set(locals.key, locals.r);
        state.mut().feePool = sadd(state.get().feePool, locals.fee);
        state.mut().totalPayments = sadd(state.get().totalPayments, (uint64)1);
        state.mut().totalVolume = sadd(state.get().totalVolume, (uint64)locals.amount);
        state.mut().totalFeesCollected = sadd(state.get().totalFeesCollected, (uint64)locals.fee);

        qpi.transfer(input.seller, locals.net);

        output.returnCode = QPAYHUB_OK;
        output.receiptKey = locals.key;
        output.net = locals.net;
        output.fee = locals.fee;
    }

    // Seller-only: mark a receipt consumed so it can never unlock the
    // resource again, globally. Optional — a facilitator may instead keep
    // single-use accounting off-chain.
    PUBLIC_PROCEDURE_WITH_LOCALS(Consume)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (!state.get().receipts.get(input.receiptKey, locals.r))
        {
            output.returnCode = QPAYHUB_ERR_NOT_FOUND;
            return;
        }
        if (qpi.invocator() != locals.r.seller)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (locals.r.consumed)
        {
            output.returnCode = QPAYHUB_ERR_ALREADY_CONSUMED;
            return;
        }
        locals.r.consumed = 1;
        state.mut().receipts.set(input.receiptKey, locals.r);
        state.mut().totalConsumed = sadd(state.get().totalConsumed, (uint64)1);
        output.returnCode = QPAYHUB_OK;
    }

    // ---- ORACLE PRICE FEED ADDITIONS: subscribe procedure ----
    // Deliberately permissionless (see file header): the query is fixed -
    // QUBIC/USDT via the combined Binance+MEXC oracle - so anyone can call
    // this to establish or renew the subscription without being able to
    // redirect it to an untrusted source. The subscription fee is paid
    // from the caller own invocationReward, refunded in full on any
    // rejection path.
    PUBLIC_PROCEDURE_WITH_LOCALS(SubscribeToPriceFeed)
    {
        if (state.get().priceOracleSubscriptionId >= 0)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPAYHUB_ERR_ALREADY_SUBSCRIBED;
            output.subscriptionId = state.get().priceOracleSubscriptionId;
            return;
        }

        locals.query.oracle = OI::Price::getBinanceMexcOracleId();
        {
            // Scoped so these single-letter names do not leak into the rest
            // of the file - the same pattern TestExampleC.h uses.
            using namespace Ch;
            locals.query.currency1 = id(Q, U, B, I, C);
            locals.query.currency2 = id(U, S, D, T);
        }
        locals.query.timestamp = qpi.now();

        locals.fee = OI::Price::getSubscriptionFee(locals.query, QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS);
        if (qpi.invocationReward() < locals.fee)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = QPAYHUB_ERR_SUBSCRIBE_FAILED;
            return;
        }
        // Refund the excess above the exact fee before the subscribe call,
        // consistent with the rest of this contract fee handling.
        if (qpi.invocationReward() > locals.fee)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.fee);
        }

        output.subscriptionId = SUBSCRIBE_ORACLE(
            OI::Price, locals.query, NotifyQuUsdPriceReply,
            QPAYHUB_PRICE_SUBSCRIBE_PERIOD_MS, 1);
        if (output.subscriptionId < 0)
        {
            // Subscription failed to register - the fee already left the
            // caller balance; the framework does not refund on this path
            // (mirrors QUtil.h own handling of the same failure), so
            // this is intentionally not retried automatically here.
            output.returnCode = QPAYHUB_ERR_SUBSCRIBE_FAILED;
            return;
        }
        state.mut().priceOracleSubscriptionId = output.subscriptionId;
        output.returnCode = QPAYHUB_OK;
    }

    // ---- ORACLE PRICE FEED ADDITIONS: notification callback ----
    // Fires whenever the subscription produces a new reply. Only a
    // confirmed SUCCESS with a sane reply updates state - everything else
    // (pending, unresolvable, timeout) leaves the last known good price in
    // place rather than overwriting it with nothing.
    PRIVATE_PROCEDURE_WITH_LOCALS(NotifyQuUsdPriceReply)
    {
        if (input.status != ORACLE_QUERY_STATUS_SUCCESS)
        {
            return;
        }
        if (!qpi.getOracleReply<OI::Price>(input.queryId, locals.reply))
        {
            return;
        }
        if (!OI::Price::replyIsValid(locals.reply))
        {
            return;
        }
        state.mut().quUsdNumerator = locals.reply.numerator;
        state.mut().quUsdDenominator = locals.reply.denominator;
        state.mut().quUsdUpdatedTick = (uint32)qpi.tick();
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetReceipt, 1);
        REGISTER_USER_FUNCTION(ComputeReceiptKey, 2);
        REGISTER_USER_FUNCTION(GetInfo, 3);
        REGISTER_USER_FUNCTION(GetQuUsdPrice, 4);

        REGISTER_USER_PROCEDURE(Pay, 1);
        REGISTER_USER_PROCEDURE(Consume, 2);
        REGISTER_USER_PROCEDURE(SubscribeToPriceFeed, 3);

        REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyQuUsdPriceReply);
    }

    INITIALIZE()
    {
        state.mut().priceOracleSubscriptionId = -1;
        state.mut().quUsdDenominator = 0; // 0 = no price known yet, GetQuUsdPrice reports stale

        // DEVNET issuer - must be re-pointed before mainnet deployment.
        state.mut().dividendToken.issuer = ID(
            _Q, _D, _O, _G, _E, _E, _E, _S,
            _K, _Y, _P, _A, _I, _C, _E, _C,
            _H, _E, _A, _H, _O, _X, _P, _U,
            _L, _E, _O, _A, _D, _T, _K, _G,
            _E, _J, _H, _A, _V, _Y, _P, _F,
            _K, _H, _L, _E, _W, _G, _X, _X,
            _Z, _Q, _U, _G, _I, _G, _M, _B
        );
        state.mut().dividendToken.assetName = QPAYHUB_TOKEN_ASSETNAME;
    }

    POST_INCOMING_TRANSFER()
    {
        // Plain QU sent to the contract address (donations, misdirected
        // transfers) accrues to the fee pool. Procedure-attached amounts are
        // accounted inside Pay and must not be counted twice here.
        if (input.type == TransferType::standardTransaction
            || input.type == TransferType::qpiTransfer
            || input.type == TransferType::qpiDistributeDividends
            || input.type == TransferType::revenueDonation)
        {
            state.mut().feePool = sadd(state.get().feePool, input.amount);
        }
    }

    struct END_EPOCH_locals
    {
        sint64 idx;
        Receipt r;
        uint32 cur;
        uint32 cutoff;
        Entity ent;
        sint64 balance;
        sint64 distributable;
        sint64 perShare;
        sint64 distributed;
        sint64 shareholderPart;
        sint64 tokenPart;
        sint64 paidShare;
        sint64 totalHeld;
        sint64 holderBal;
        sint64 reward;
        id     holder;
        AssetPossessionIterator it;
    };
    END_EPOCH_WITH_LOCALS()
    {
        state.mut().receipts.cleanupIfNeeded();

        // Purge receipts older than the retention window. A receipt paid in
        // epoch E survives until END_EPOCH of E + retention, far beyond any
        // payment freshness window a facilitator would accept.
        locals.cur = (uint32)qpi.epoch();
        if (locals.cur > QPAYHUB_RECEIPT_RETENTION_EPOCHS)
        {
            locals.cutoff = locals.cur - QPAYHUB_RECEIPT_RETENTION_EPOCHS;
        }
        else
        {
            locals.cutoff = 0;
        }
        for (locals.idx = state.get().receipts.nextElementIndex(NULL_INDEX);
             locals.idx != NULL_INDEX;
             locals.idx = state.get().receipts.nextElementIndex(locals.idx))
        {
            locals.r = state.get().receipts.value(locals.idx);
            if (locals.r.epochPaid < locals.cutoff)
            {
                state.mut().receipts.removeByKey(state.get().receipts.key(locals.idx));
                state.mut().totalPurged = sadd(state.get().totalPurged, (uint64)1);
            }
        }
        state.mut().receipts.cleanupIfNeeded();

        // Distribute above the exec reserve: 10% shares, 90% token holders.
        // Balance is re-read before spending; every payment is clamped to it.
        qpi.getEntity(SELF, locals.ent);
        locals.balance = locals.ent.incomingAmount - locals.ent.outgoingAmount;
        locals.distributable = state.get().feePool - QPAYHUB_EXEC_RESERVE;
        if (locals.distributable > locals.balance)
        {
            locals.distributable = locals.balance;
        }
        if (locals.distributable <= 0) return;

        locals.distributed = 0;

        locals.shareholderPart = (sint64)div(
            (uint64)locals.distributable * QPAYHUB_DIVIDEND_SHAREHOLDER_PERMILLE, (uint64)1000);
        // Token side takes the rounding remainder; nothing is stranded.
        locals.tokenPart = locals.distributable - locals.shareholderPart;
        if (locals.shareholderPart > 0)
        {
            locals.perShare = (sint64)div((uint64)locals.shareholderPart, (uint64)NUMBER_OF_COMPUTORS);
            if (locals.perShare > 0 && qpi.distributeDividends(locals.perShare))
            {
                locals.paidShare = locals.perShare * (sint64)NUMBER_OF_COMPUTORS;
                locals.distributed = locals.distributed + locals.paidShare;
                locals.balance = locals.balance - locals.paidShare;
                state.mut().totalShareholderDividends =
                    sadd(state.get().totalShareholderDividends, (uint64)locals.paidShare);
            }
        }

        // No token or no holders: the 90% stays in feePool for a later epoch.
        if (locals.tokenPart > 0 && state.get().dividendToken.issuer != NULL_ID)
        {
            locals.totalHeld = 0;
            for (locals.it.begin(state.get().dividendToken); !locals.it.reachedEnd(); locals.it.next())
            {
                // Issuer and contract are not recipients.
                if (locals.it.possessor() == SELF) continue;
                if (locals.it.possessor() == state.get().dividendToken.issuer) continue;
                locals.totalHeld = sadd(locals.totalHeld, locals.it.numberOfPossessedShares());
            }

            if (locals.totalHeld > 0)
            {
                for (locals.it.begin(state.get().dividendToken); !locals.it.reachedEnd(); locals.it.next())
                {
                    if (locals.it.possessor() == SELF) continue;
                    if (locals.it.possessor() == state.get().dividendToken.issuer) continue;
                    locals.holderBal = locals.it.numberOfPossessedShares();
                    if (locals.holderBal <= 0) continue;
                    // 128-bit intermediate: the product overflows 64 bits.
                    locals.reward = div(
                        (uint128)(uint64)locals.tokenPart * (uint128)(uint64)locals.holderBal,
                        (uint128)(uint64)locals.totalHeld).low;
                    if (locals.reward <= 0) continue;
                    if (locals.reward > locals.balance) locals.reward = locals.balance;
                    locals.holder = locals.it.possessor();
                    qpi.transfer(locals.holder, locals.reward);
                    locals.balance = locals.balance - locals.reward;
                    locals.distributed = locals.distributed + locals.reward;
                    state.mut().totalTokenholderDividends =
                        sadd(state.get().totalTokenholderDividends, (uint64)locals.reward);
                    if (locals.balance <= 0) break;
                }
            }
        }

        if (locals.distributed > 0)
        {
            state.mut().feePool = state.get().feePool - locals.distributed;
            state.mut().totalFeesDistributed =
                sadd(state.get().totalFeesDistributed, (uint64)locals.distributed);
        }
    }
};
