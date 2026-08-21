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
//   dominates below 13,334 QU.
//
//   PROMO RATES ADDITION: a specific seller can be given a discounted rate
//   below QPAYHUB_FEE_PERMILLE via SetPromoRate (search PROMO RATES ADDITION
//   below). This is the one on-chain concession to the admin-free design
//   described below - see that section for the operator/recovery model and
//   why the discount is bounded rather than open-ended. The 100 QU floor is
//   untouched by a promo rate; only the percentage is discounted, never the
//   dust floor.
//
//   AFFILIATE ADDITION: a seller can be attributed to a referring affiliate
//   via SetAffiliate (search AFFILIATE ADDITION below), who then earns
//   QPAYHUB_AFFILIATE_COMMISSION_PERMILLE of that seller's fee - never the
//   seller's net, never the buyer's payment - for QPAYHUB_AFFILIATE_TERM_EPOCHS
//   epochs from the referral. A growth incentive for whoever brings a
//   merchant onto QPAY, bounded and time-limited the same way promo rates
//   are bounded by a floor.
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
//   SetPromoRate/RemovePromoRate/ChangeOperator/GetPromoRate and Pay's
//   promo-rate lookup (search PROMO RATES ADDITION below): ported from
//   profitphil/qubic-x402 PR #3 (feature/qpay-promo-rates), reviewed for
//   access-control/bounds correctness, and re-tested against this file's
//   own GoogleTest suite (test/contract_qpayhub.cpp) - see that file's
//   Promo/Operator/Recovery test cases.
//   SetAffiliate/RemoveAffiliate/ChangeAffiliateRegistrar/GetAffiliate and
//   Pay's affiliate-cut split (search AFFILIATE ADDITION below): ported
//   from the same PR #3 branch after it grew affiliate referrals, reviewed
//   for the same access-control/bounds properties (self-referral blocked,
//   cut comes out of the fee not the seller's net, first-attribution-wins,
//   term-expiry enforced in both Pay() and END_EPOCH), and re-tested
//   against this file's own GoogleTest suite - see that file's Affiliate
//   test cases.
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
//   UPDATE - PROMO RATES ADDITION: this is no longer fully accurate. A
//   merchant promo-pricing feature (early-adopter discounted rates) needed
//   a real privileged actor - there is no way to single out one seller for
//   a better rate without someone authorized to say which seller and what
//   rate. Rather than pretend that requirement doesn't exist, it is scoped
//   as tightly as this file's other design choices are: two keys, not one -
//   operatorId (day-to-day SetPromoRate/RemovePromoRate) and recoveryId
//   (can only reassign operatorId via ChangeOperator, so a compromised
//   operator key can be rotated out without redeploying the contract).
//   Neither key can push a seller's rate below QPAYHUB_PROMO_FLOOR_PERMILLE
//   - that floor is a hardcoded constexpr, not admin-settable, specifically
//   so a compromised or malicious key's worst case is bounded and known in
//   advance rather than "the operator can waive fees entirely." Every
//   promo rate is publicly readable via GetPromoRate - discounted pricing
//   is visible on-chain, not a private side deal. See PROMO RATES ADDITION
//   below for the full mechanism.
//
//   UPDATE - AFFILIATE ADDITION: a second, separate admin role -
//   affiliateRegistrarId - was added the same way, deliberately not folded
//   into operatorId, so a compromised key on one surface (promo rates)
//   can't touch the other (affiliate attribution), and vice versa.
//   recoveryId can reassign either via ChangeOperator or
//   ChangeAffiliateRegistrar. Every affiliate link is publicly readable via
//   GetAffiliate, same transparency rationale as promo rates above.
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
//   - The id(...) character constructor (src/platform/m256.h) takes five
//     REQUIRED single-letter params and defaults the remaining 27 to 0 -
//     id(Q,U,B,I,C) is exactly QUBIC, no padding needed. A currency shorter
//     than five letters MUST still be padded explicitly, as id(U,S,D,T,null)
//     is: with only four arguments that constructor is not viable at all and
//     overload resolution silently falls through to
//     id(uint64,uint64,uint64,uint64), which stores the four ASCII codes as
//     separate 64-bit limbs instead of packing "USDT" into the first four
//     bytes - a different value entirely, and one the compiler accepts
//     without a warning.
//   - Oracle subscriptions last exactly one epoch. The node drops all of
//     them during the epoch transition (qubic.cpp beginEpoch() ->
//     oracleEngine.beginEpoch() -> reset()) and runs each contract's
//     BEGIN_EPOCH only afterwards, so BEGIN_EPOCH below clears
//     priceOracleSubscriptionId back to -1 and the feed is renewed by
//     calling SubscribeToPriceFeed once per epoch. Without that reset the
//     `>= 0` guard in SubscribeToPriceFeed would reject every renewal and
//     the price feed would go permanently stale after the first epoch.
//
// NOT YET VERIFIED (the currency pair and the subscription renewal/lifetime
// semantics are now covered by GoogleTest - see
// SubscribeToPriceFeedUsesQubicUsdtCurrencyPair and
// BeginEpochClearsStaleSubscriptionSoFeedCanBeRenewed in
// test/contract_qpayhub.cpp - but this remains open):
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

// ---- PROMO RATES ADDITION: constants ----
// A promo rate can only ever discount, never below this floor and never
// above the standard rate - see the header's UPDATE - PROMO RATES ADDITION
// note above for why this is hardcoded rather than admin-settable.
constexpr uint64 QPAYHUB_PROMO_CAPACITY = 32; // concurrent promo sellers; grow via redeploy, not a live concern at launch scale
constexpr uint64 QPAYHUB_PROMO_FLOOR_PERMILLE = 25; // 25 per 10000 = 0.25 percent, the deepest discount any operator key can grant

// ---- AFFILIATE ADDITION: constants ----
// A referrer earns a slice of a referred seller's fee for a bounded term,
// not an open-ended cut - see the header's UPDATE - AFFILIATE ADDITION note.
constexpr uint64 QPAYHUB_AFFILIATE_CAPACITY = 128; // concurrent active affiliate links; grow via redeploy
constexpr uint64 QPAYHUB_AFFILIATE_COMMISSION_PERMILLE = 500; // 500 per 10000 = 5% of the fee, not the seller's net
constexpr uint32 QPAYHUB_AFFILIATE_TERM_EPOCHS = 52; // ~12 months at ~1 epoch/week

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
static_assert((QPAYHUB_PROMO_CAPACITY & (QPAYHUB_PROMO_CAPACITY - 1)) == 0);
static_assert(QPAYHUB_PROMO_FLOOR_PERMILLE <= QPAYHUB_FEE_PERMILLE);
static_assert((QPAYHUB_AFFILIATE_CAPACITY & (QPAYHUB_AFFILIATE_CAPACITY - 1)) == 0);
static_assert(QPAYHUB_AFFILIATE_COMMISSION_PERMILLE <= 10000);

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
constexpr uint64 QPAYHUB_ERR_INVALID_RATE = 10; // SetPromoRate's feePermille outside [QPAYHUB_PROMO_FLOOR_PERMILLE, QPAYHUB_FEE_PERMILLE]
constexpr uint64 QPAYHUB_ERR_ALREADY_HAS_AFFILIATE = 11; // SetAffiliate: seller already has an attributed affiliate - remove it first

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

    // ---- AFFILIATE ADDITION: referral record ----
    struct Affiliate
    {
        id affiliate;
        uint32 referredAtEpoch;
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

        // ---- PROMO RATES ADDITION: state ----
        // operatorId: day-to-day SetPromoRate/RemovePromoRate caller.
        // recoveryId: can ONLY call ChangeOperator - narrow on purpose, see
        // header note above. Both fixed in INITIALIZE to real identities.
        id operatorId;
        id recoveryId;
        HashMap<id, uint64, QPAYHUB_PROMO_CAPACITY> promoFeePermille;

        // ---- AFFILIATE ADDITION: state ----
        // affiliateRegistrarId: day-to-day SetAffiliate/RemoveAffiliate caller,
        // a separate role from operatorId so a compromised key only ever
        // touches one of the two admin surfaces. recoveryId (above) can
        // reassign this one too, via ChangeAffiliateRegistrar.
        id affiliateRegistrarId;
        HashMap<id, Affiliate, QPAYHUB_AFFILIATE_CAPACITY> affiliateOf;
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
        uint64 feePermille; // QPAYHUB_FEE_PERMILLE, or this seller's promo rate if one is set - PROMO RATES ADDITION
        Affiliate aff;      // AFFILIATE ADDITION
        sint64 affiliateCut;
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
        // Likewise appended - AFFILIATE ADDITION.
        uint64 affiliateCommissionPermille;
        uint32 affiliateTermEpochs;
        uint32 padding1;
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

    // ---- PROMO RATES ADDITION: I/O structs ----

    struct GetPromoRate_input
    {
        id seller;
    };
    struct GetPromoRate_output
    {
        uint64 feePermille; // this seller's effective rate right now
        bit isPromo;        // 1 = feePermille came from promoFeePermille, 0 = it's the standard QPAYHUB_FEE_PERMILLE
    };
    struct GetPromoRate_locals
    {
        uint64 rate;
    };

    struct SetPromoRate_input
    {
        id seller;
        uint64 feePermille;
    };
    struct SetPromoRate_output
    {
        uint64 returnCode;
    };

    struct RemovePromoRate_input
    {
        id seller;
    };
    struct RemovePromoRate_output
    {
        uint64 returnCode;
    };

    struct ChangeOperator_input
    {
        id newOperator;
    };
    struct ChangeOperator_output
    {
        uint64 returnCode;
    };

    // ---- AFFILIATE ADDITION: I/O structs ----

    struct GetAffiliate_input
    {
        id seller;
    };
    struct GetAffiliate_output
    {
        id affiliate;
        uint32 referredAtEpoch;
        bit active; // within QPAYHUB_AFFILIATE_TERM_EPOCHS of referredAtEpoch
    };
    struct GetAffiliate_locals
    {
        Affiliate aff;
    };

    struct SetAffiliate_input
    {
        id seller;
        id affiliate;
    };
    struct SetAffiliate_output
    {
        uint64 returnCode;
    };
    struct SetAffiliate_locals
    {
        Affiliate a;
    };

    struct RemoveAffiliate_input
    {
        id seller;
    };
    struct RemoveAffiliate_output
    {
        uint64 returnCode;
    };

    struct ChangeAffiliateRegistrar_input
    {
        id newRegistrar;
    };
    struct ChangeAffiliateRegistrar_output
    {
        uint64 returnCode;
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
        output.affiliateCommissionPermille = QPAYHUB_AFFILIATE_COMMISSION_PERMILLE;
        output.affiliateTermEpochs = QPAYHUB_AFFILIATE_TERM_EPOCHS;
        output.padding1 = 0;
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

    // ---- PROMO RATES ADDITION: read function ----
    // Permissionless, like every other read in this file - a promo rate is
    // a public fact about a seller, not a private arrangement between that
    // seller and the operator.
    PUBLIC_FUNCTION_WITH_LOCALS(GetPromoRate)
    {
        if (state.get().promoFeePermille.get(input.seller, locals.rate))
        {
            output.feePermille = locals.rate;
            output.isPromo = 1;
        }
        else
        {
            output.feePermille = QPAYHUB_FEE_PERMILLE;
            output.isPromo = 0;
        }
    }

    // ---- AFFILIATE ADDITION: read function ----
    // Permissionless, same rationale as GetPromoRate above.
    PUBLIC_FUNCTION_WITH_LOCALS(GetAffiliate)
    {
        if (state.get().affiliateOf.get(input.seller, locals.aff))
        {
            output.affiliate = locals.aff.affiliate;
            output.referredAtEpoch = locals.aff.referredAtEpoch;
            output.active = (uint32)qpi.epoch() < locals.aff.referredAtEpoch + QPAYHUB_AFFILIATE_TERM_EPOCHS ? 1 : 0;
        }
        else
        {
            output.affiliate = NULL_ID;
            output.referredAtEpoch = 0;
            output.active = 0;
        }
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

        // PROMO RATES ADDITION: a seller with an entry in promoFeePermille
        // pays that rate instead of the standard QPAYHUB_FEE_PERMILLE. The
        // 100 QU floor below applies either way - a promo rate only
        // discounts the percentage, never the dust floor.
        if (!state.get().promoFeePermille.get(input.seller, locals.feePermille))
        {
            locals.feePermille = QPAYHUB_FEE_PERMILLE;
        }

        // 0.75% (or the seller's promo rate) or QPAYHUB_FEE_FLOOR_QU,
        // whichever is greater. Floor and minimum are both 100, so a
        // payment at the minimum nets the seller zero.
        locals.fee = (sint64)div((uint64)locals.amount * locals.feePermille, (uint64)10000);
        if (locals.fee < QPAYHUB_FEE_FLOOR_QU)
        {
            locals.fee = QPAYHUB_FEE_FLOOR_QU;
        }
        if (locals.fee > locals.amount)
        {
            locals.fee = locals.amount;
        }
        locals.net = locals.amount - locals.fee;

        // AFFILIATE ADDITION: if this seller was referred and is still
        // within term, the affiliate's cut comes out of the fee, never out
        // of the seller's net - locals.net above is already final.
        locals.affiliateCut = 0;
        if (state.get().affiliateOf.get(input.seller, locals.aff)
            && (uint32)qpi.epoch() < locals.aff.referredAtEpoch + QPAYHUB_AFFILIATE_TERM_EPOCHS)
        {
            locals.affiliateCut = (sint64)div((uint64)locals.fee * QPAYHUB_AFFILIATE_COMMISSION_PERMILLE, (uint64)10000);
        }

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
        state.mut().feePool = sadd(state.get().feePool, locals.fee - locals.affiliateCut);
        state.mut().totalPayments = sadd(state.get().totalPayments, (uint64)1);
        state.mut().totalVolume = sadd(state.get().totalVolume, (uint64)locals.amount);
        state.mut().totalFeesCollected = sadd(state.get().totalFeesCollected, (uint64)locals.fee);

        qpi.transfer(input.seller, locals.net);
        if (locals.affiliateCut > 0)
        {
            qpi.transfer(locals.aff.affiliate, locals.affiliateCut);
        }

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
            //
            // Always pass at least 5 characters, padding with null: id()'s
            // character constructor declares c0..c4 without defaults, so a
            // 4-character id(U, S, D, T) is not viable for it and silently
            // binds to id(uint64, uint64, uint64, uint64) instead, producing
            // the four ASCII codes as separate 64-bit limbs rather than
            // "USDT" packed into the first four bytes.
            using namespace Ch;
            locals.query.currency1 = id(Q, U, B, I, C);
            locals.query.currency2 = id(U, S, D, T, null);
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

    // ---- PROMO RATES ADDITION: operator-gated procedures ----
    // operatorId-only. feePermille is clamped to
    // [QPAYHUB_PROMO_FLOOR_PERMILLE, QPAYHUB_FEE_PERMILLE] - never below the
    // hardcoded floor, and never above the standard rate (this is a discount
    // mechanism, not a way to charge one seller more than everyone else).
    PUBLIC_PROCEDURE(SetPromoRate)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().operatorId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (input.seller == NULL_ID || input.seller == SELF)
        {
            output.returnCode = QPAYHUB_ERR_INVALID_SELLER;
            return;
        }
        if (input.feePermille < QPAYHUB_PROMO_FLOOR_PERMILLE || input.feePermille > QPAYHUB_FEE_PERMILLE)
        {
            output.returnCode = QPAYHUB_ERR_INVALID_RATE;
            return;
        }
        // Only a NEW seller consumes a capacity slot; updating an existing
        // seller's rate is a plain overwrite and always allowed.
        if (!state.get().promoFeePermille.contains(input.seller)
            && state.get().promoFeePermille.population() >= QPAYHUB_PROMO_CAPACITY)
        {
            output.returnCode = QPAYHUB_ERR_CAPACITY;
            return;
        }
        state.mut().promoFeePermille.set(input.seller, input.feePermille);
        output.returnCode = QPAYHUB_OK;
    }

    // operatorId-only. Reverting a seller to the standard rate is always
    // free of the capacity check above - it can only shrink the table.
    PUBLIC_PROCEDURE(RemovePromoRate)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().operatorId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (!state.get().promoFeePermille.contains(input.seller))
        {
            output.returnCode = QPAYHUB_ERR_NOT_FOUND;
            return;
        }
        state.mut().promoFeePermille.removeByKey(input.seller);
        output.returnCode = QPAYHUB_OK;
    }

    // Callable by the CURRENT operator (planned rotation, e.g. moving to a
    // new wallet) or by recoveryId (emergency override when the operator
    // key is lost or compromised and cannot be trusted to rotate itself).
    // recoveryId itself is fixed in INITIALIZE and has no procedure that
    // can ever change it - rotating recoveryId requires a redeploy, same
    // tier of rare as changing QPAYHUB_FEE_PERMILLE itself. See the
    // header's UPDATE - PROMO RATES ADDITION note for why this is
    // deliberately not a single self-reassigning key.
    PUBLIC_PROCEDURE(ChangeOperator)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().operatorId
            && qpi.invocator() != state.get().recoveryId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (input.newOperator == NULL_ID)
        {
            output.returnCode = QPAYHUB_ERR_INVALID_SELLER;
            return;
        }
        state.mut().operatorId = input.newOperator;
        output.returnCode = QPAYHUB_OK;
    }

    // ---- AFFILIATE ADDITION: registrar-gated procedures ----
    // affiliateRegistrarId-only. First-attribution-wins: an existing
    // affiliate link can't be overwritten directly, only removed then
    // re-set, so the registrar can't silently reassign credit after the
    // fact.
    PUBLIC_PROCEDURE_WITH_LOCALS(SetAffiliate)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().affiliateRegistrarId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (input.seller == NULL_ID || input.affiliate == NULL_ID || input.affiliate == input.seller)
        {
            output.returnCode = QPAYHUB_ERR_INVALID_SELLER;
            return;
        }
        if (state.get().affiliateOf.contains(input.seller))
        {
            output.returnCode = QPAYHUB_ERR_ALREADY_HAS_AFFILIATE;
            return;
        }
        if (state.get().affiliateOf.population() >= QPAYHUB_AFFILIATE_CAPACITY)
        {
            output.returnCode = QPAYHUB_ERR_CAPACITY;
            return;
        }
        locals.a.affiliate = input.affiliate;
        locals.a.referredAtEpoch = (uint32)qpi.epoch();
        state.mut().affiliateOf.set(input.seller, locals.a);
        output.returnCode = QPAYHUB_OK;
    }

    // affiliateRegistrarId OR recoveryId - a confirmed bad attribution needs
    // a way out even if the registrar key is the one that's compromised.
    PUBLIC_PROCEDURE(RemoveAffiliate)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().affiliateRegistrarId
            && qpi.invocator() != state.get().recoveryId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (!state.get().affiliateOf.contains(input.seller))
        {
            output.returnCode = QPAYHUB_ERR_NOT_FOUND;
            return;
        }
        state.mut().affiliateOf.removeByKey(input.seller);
        output.returnCode = QPAYHUB_OK;
    }

    // Callable by the CURRENT registrar (planned rotation) or by recoveryId
    // (emergency override) - same self-rotation/recovery-override pattern
    // as ChangeOperator above.
    PUBLIC_PROCEDURE(ChangeAffiliateRegistrar)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        if (qpi.invocator() != state.get().affiliateRegistrarId
            && qpi.invocator() != state.get().recoveryId)
        {
            output.returnCode = QPAYHUB_ERR_ACCESS_DENIED;
            return;
        }
        if (input.newRegistrar == NULL_ID)
        {
            output.returnCode = QPAYHUB_ERR_INVALID_SELLER;
            return;
        }
        state.mut().affiliateRegistrarId = input.newRegistrar;
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
        REGISTER_USER_FUNCTION(GetPromoRate, 5);
        REGISTER_USER_FUNCTION(GetAffiliate, 6);

        REGISTER_USER_PROCEDURE(Pay, 1);
        REGISTER_USER_PROCEDURE(Consume, 2);
        REGISTER_USER_PROCEDURE(SubscribeToPriceFeed, 3);
        REGISTER_USER_PROCEDURE(SetPromoRate, 4);
        REGISTER_USER_PROCEDURE(RemovePromoRate, 5);
        REGISTER_USER_PROCEDURE(ChangeOperator, 6);
        REGISTER_USER_PROCEDURE(SetAffiliate, 7);
        REGISTER_USER_PROCEDURE(RemoveAffiliate, 8);
        REGISTER_USER_PROCEDURE(ChangeAffiliateRegistrar, 9);

        REGISTER_USER_PROCEDURE_NOTIFICATION(NotifyQuUsdPriceReply);
    }

    INITIALIZE()
    {
        state.mut().priceOracleSubscriptionId = -1;
        state.mut().quUsdDenominator = 0; // 0 = no price known yet, GetQuUsdPrice reports stale

        // QPAY token issuer: QPAYNOWSWZMGHFEAEVJXGZAVSHABAZDDBDIHTEBOPCOGHRGBCYCUZOHCVLXG
        state.mut().dividendToken.issuer = ID(
            _Q, _P, _A, _Y, _N, _O, _W, _S,
            _W, _Z, _M, _G, _H, _F, _E, _A,
            _E, _V, _J, _X, _G, _Z, _A, _V,
            _S, _H, _A, _B, _A, _Z, _D, _D,
            _B, _D, _I, _H, _T, _E, _B, _O,
            _P, _C, _O, _G, _H, _R, _G, _B,
            _C, _Y, _C, _U, _Z, _O, _H, _C
        );
        state.mut().dividendToken.assetName = QPAYHUB_TOKEN_ASSETNAME;

        // ---- PROMO RATES ADDITION: operator/recovery init ----
        // Real identities ported from profitphil/qubic-x402 PR #3 - NOT
        // independently verifiable against their intended 60-character
        // source identities the way the QPAY issuer above was (that one
        // was cross-checked against a string this session was given
        // directly). Re-verify both before relying on them for a live
        // deployment.
        state.mut().operatorId = ID(
            _I, _Q, _D, _Z, _L, _W, _S, _S,
            _Q, _O, _G, _R, _F, _D, _W, _D,
            _I, _N, _C, _E, _O, _Q, _W, _H,
            _M, _F, _A, _A, _Z, _C, _G, _T,
            _N, _E, _R, _H, _A, _Z, _K, _F,
            _M, _A, _G, _L, _E, _J, _I, _Z,
            _I, _K, _O, _E, _Z, _X, _Y, _G
        );
        state.mut().recoveryId = ID(
            _L, _A, _W, _L, _X, _E, _I, _B,
            _Q, _H, _G, _W, _O, _G, _K, _U,
            _F, _K, _X, _Q, _L, _B, _K, _Q,
            _D, _M, _Z, _A, _I, _J, _Z, _L,
            _V, _E, _U, _C, _T, _F, _B, _C,
            _D, _A, _Z, _M, _N, _C, _S, _W,
            _W, _J, _Q, _Y, _L, _J, _C, _B
        );

        // ---- AFFILIATE ADDITION: registrar init ----
        // Real identity: LWKZMFLWSBGAIBGRHZTRANQQTVFDSBSEBJSLISUUYAVZUMLBWZCSTQJALJZE
        state.mut().affiliateRegistrarId = ID(
            _L, _W, _K, _Z, _M, _F, _L, _W,
            _S, _B, _G, _A, _I, _B, _G, _R,
            _H, _Z, _T, _R, _A, _N, _Q, _Q,
            _T, _V, _F, _D, _S, _B, _S, _E,
            _B, _J, _S, _L, _I, _S, _U, _U,
            _Y, _A, _V, _Z, _U, _M, _L, _B,
            _W, _Z, _C, _S, _T, _Q, _J, _A
        );
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

    // ---- ORACLE PRICE FEED ADDITIONS: subscription lifetime ----
    // Oracle subscriptions do not survive the epoch boundary: the node drops
    // every one of them during the epoch transition (qubic.cpp beginEpoch()
    // calls oracleEngine.beginEpoch(), whose reset() is documented there as
    // "Drop all queries of the previous epoch"), and only afterwards runs
    // each contract's BEGIN_EPOCH. So by the time this executes the stored id
    // refers to a subscription that no longer exists. Clearing it back to the
    // not-subscribed sentinel is what lets SubscribeToPriceFeed be called
    // again - otherwise its `>= 0` guard would reject every renewal and the
    // price feed would stay dead for the remaining life of the contract.
    // Renewal stays permissionless, exactly like the initial subscription.
    BEGIN_EPOCH()
    {
        state.mut().priceOracleSubscriptionId = -1;
    }

    struct END_EPOCH_locals
    {
        sint64 idx;
        Receipt r;
        Affiliate aff; // AFFILIATE ADDITION
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

        // AFFILIATE ADDITION: purge referral links past their term, freeing
        // the slot - same purge pattern as receipt retention above. Pay()
        // already stops paying an expired affiliate on its own (it checks
        // the term itself), this just reclaims the capacity slot.
        for (locals.idx = state.get().affiliateOf.nextElementIndex(NULL_INDEX);
             locals.idx != NULL_INDEX;
             locals.idx = state.get().affiliateOf.nextElementIndex(locals.idx))
        {
            locals.aff = state.get().affiliateOf.value(locals.idx);
            if (locals.cur >= locals.aff.referredAtEpoch + QPAYHUB_AFFILIATE_TERM_EPOCHS)
            {
                state.mut().affiliateOf.removeByKey(state.get().affiliateOf.key(locals.idx));
            }
        }
        state.mut().affiliateOf.cleanupIfNeeded();

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
