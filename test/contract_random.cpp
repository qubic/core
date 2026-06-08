#define NO_UEFI

#include "contract_testing.h"
#include "kangaroo_twelve.h"

// Helper: build a deterministic non-zero bit_4096 from a seed.
static QPI::bit_4096 makeReveal(uint64 seed)
{
    QPI::bit_4096 v;
    v.setAll(0);
    for (uint64 i = 0; i < 4096; ++i)
    {
        v.set(i, (bit)(((seed * 0x9E3779B97F4A7C15ULL + i * 0xBF58476D1CE4E5B9ULL) >> 32) & 1));
    }
    return v;
}

// Helper: K12(reveal) -> id, matching qpi.K12() used by RevealAndCommit to validate reveals.
static id commitOf(const QPI::bit_4096& reveal)
{
    id digest;
    KangarooTwelve(&reveal, sizeof(reveal), &digest, sizeof(digest));
    return digest;
}

static id getUser(uint64 i)
{
    return id(i + 1, i / 2 + 4, i + 10, i * 3 + 8);
}

// Reward amount required by RevealAndCommit to select a given collateral tier.
static sint64 collateralForTier(uint8 tier)
{
    sint64 v = 1;
    for (uint8 t = 0; t < tier; ++t)
        v *= 10;
    return v;
}

class ContractTestingRandom : public ContractTesting
{
public:
    ContractTestingRandom()
    {
        initEmptySpectrum();
        initEmptyUniverse();
        system.epoch = contractDescriptions[RANDOM_CONTRACT_INDEX].constructionEpoch;
        system.tick = 1000;
        INIT_CONTRACT(RANDOM);
        callSystemProcedure(RANDOM_CONTRACT_INDEX, INITIALIZE);
    }

    RANDOM::StateData* state()
    {
        return (RANDOM::StateData*)contractStates[RANDOM_CONTRACT_INDEX];
    }

    void setTick(uint32 t) { system.tick = t; }
    uint32 tick() const { return system.tick; }
    void endTick() { callSystemProcedure(RANDOM_CONTRACT_INDEX, END_TICK); }

    RANDOM::Fees_output fees()
    {
        RANDOM::Fees_input input{};
        RANDOM::Fees_output output{};
        callFunction(RANDOM_CONTRACT_INDEX, 1, input, output);
        return output;
    }

    void revealAndCommit(const id& user, const QPI::bit_4096& reveal,
                        const id& commit, sint64 collateral)
    {
        RANDOM::RevealAndCommit_input input{};
        input.reveal = reveal;
        input.commit = commit;
        RANDOM::RevealAndCommit_output output{};
        invokeUserProcedure(RANDOM_CONTRACT_INDEX, 1, input, output, user, collateral);
    }

    RANDOM::BuyEntropy_output buyEntropy(const id& user, uint8 collateralTier,
                                         uint16 numberOfBits, sint64 amount,
                                         const id& trustee = id::zero())
    {
        RANDOM::BuyEntropy_input input{};
        input.collateralTier = collateralTier;
        input.numberOfBits = numberOfBits;
        input.trustee = trustee;
        RANDOM::BuyEntropy_output output{};
        invokeUserProcedure(RANDOM_CONTRACT_INDEX, 2, input, output, user, amount);
        return output;
    }
};

// ---------------------------------------------------------------------------
// Basic sanity tests
// ---------------------------------------------------------------------------

TEST(ContractRandom, FeesReturns100PerBitForFirstTenTiers)
{
    ContractTestingRandom r;
    auto out = r.fees();
    for (uint64 i = 0; i < 10; ++i)
    {
        EXPECT_EQ(out.fees.get(i), 100u) << "fees[" << i << "] should be 100 qu/bit";
    }
}

TEST(ContractRandom, BuyEntropyRefundsWhenEntropyMissing)
{
    ContractTestingRandom r;

    const uint8 tier = 3;
    const uint16 numberOfBits = 64;
    const sint64 fee = (sint64)numberOfBits * 100;

    id user = getUser(1);
    increaseEnergy(user, fee);
    const long long balanceBefore = getBalance(user);

    auto out = r.buyEntropy(user, tier, numberOfBits, fee);

    EXPECT_EQ(getBalance(user), balanceBefore) << "BuyEntropy must refund when no entropy is available";

    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_TRUE(out.entropy == zero) << "Output must remain all-zero on refund";
    EXPECT_EQ(r.state()->earnedAmount, 0u) << "Refund must not credit earnedAmount";
}

// Spec: "BuyEntropy procedure is for buying entropy. The fee is returned if there
// is no entropy, in this case output will be all zeros." By symmetry, invalid
// inputs (bad tier, zero/oversize bits, underpaid fee) must also refund in full:
// no entropy delivered, earnedAmount unchanged.

TEST(ContractRandom, BuyEntropyRefundsOnInvalidTier)
{
    ContractTestingRandom r;
    const sint64 fee = 64 * 100;

    id user = getUser(200);
    increaseEnergy(user, fee);
    const long long before = getBalance(user);
    const uint64 earnedBefore = r.state()->earnedAmount;

    auto out = r.buyEntropy(user, /*tier*/10, /*bits*/64, fee);

    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_EQ(getBalance(user), before)
        << "invalid tier must refund the full fee";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore)
        << "invalid tier must NOT credit earnedAmount";
    EXPECT_TRUE(out.entropy == zero)
        << "rejected request must not deliver entropy";
}

TEST(ContractRandom, BuyEntropyRefundsOnZeroBits)
{
    ContractTestingRandom r;
    const sint64 fee = 64 * 100;

    id user = getUser(201);
    increaseEnergy(user, fee);
    const long long before = getBalance(user);
    const uint64 earnedBefore = r.state()->earnedAmount;

    auto out = r.buyEntropy(user, /*tier*/0, /*bits*/0, fee);

    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_EQ(getBalance(user), before)
        << "numberOfBits=0 must refund the full fee";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore);
    EXPECT_TRUE(out.entropy == zero);
}

TEST(ContractRandom, BuyEntropyRefundsOnUnderpaidFee)
{
    ContractTestingRandom r;
    const sint64 fee = 64 * 100;

    id user = getUser(202);
    increaseEnergy(user, fee - 1);
    const long long before = getBalance(user);
    const uint64 earnedBefore = r.state()->earnedAmount;

    auto out = r.buyEntropy(user, /*tier*/0, /*bits*/64, fee - 1);

    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_EQ(getBalance(user), before)
        << "underpaid fee must be refunded (no entropy was delivered)";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore);
    EXPECT_TRUE(out.entropy == zero);
}

TEST(ContractRandom, BuyEntropyRefundsOnOversizeBits)
{
    ContractTestingRandom r;
    // 4097 bits is one more than the bit_4096 capacity -> must be rejected.
    const uint16 oversize = 4097;
    const sint64 fee = (sint64)oversize * 100;

    id user = getUser(203);
    increaseEnergy(user, fee);
    const long long before = getBalance(user);
    const uint64 earnedBefore = r.state()->earnedAmount;

    auto out = r.buyEntropy(user, /*tier*/0, oversize, fee);

    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_EQ(getBalance(user), before)
        << "numberOfBits > 4096 must be refunded";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore);
    EXPECT_TRUE(out.entropy == zero);
}

// ---------------------------------------------------------------------------
// End-to-end: drive RevealAndCommit + END_TICK to produce real entropy, then
// verify BuyEntropy reads the correct finalized stream.
//
// Lifecycle on stream s = startTick % 3:
//   Tick T   : first-commit only. END_TICK leaves entropy[s*10+tier] = 0.
//   Tick T+3 : reveal=reveal1, re-commit. END_TICK XORs reveal1 in -> entropy = reveal1.
//   Tick T+4 : BuyEntropy reads stream (T+4+2)%3 = T%3 = s -> must return reveal1.
//
// Repeated for all three residues of T mod 3 to catch the original bug where
// BuyEntropy always read stream 0 instead of the latest finalized stream.
// ---------------------------------------------------------------------------

namespace
{
    void runFullRandomCycle(uint32 startTick)
    {
        ContractTestingRandom r;
        r.setTick(startTick);

        const uint8 tier = 2;                              // 100 qu collateral
        const sint64 collateral = collateralForTier(tier); // 100
        const uint32 stream = startTick % 3;

        // NOTE: identifiers C1/C2 (and R1/R2 with the FourQ headers) are
        // macros in src/four_q.h, so we use longer names here to avoid the
        // macro expansion.
        const QPI::bit_4096 reveal1 = makeReveal(startTick * 1234567u + 1u);
        const QPI::bit_4096 reveal2 = makeReveal(startTick * 1234567u + 2u);
        const id commit1 = commitOf(reveal1);
        const id commit2 = commitOf(reveal2);

        const id provider = getUser(0xC0DE + startTick);

        // ----- Tick T : initial commit only -------------------------------
        increaseEnergy(provider, collateral);
        QPI::bit_4096 zero; zero.setAll(0);
        r.revealAndCommit(provider, /*reveal=*/zero, /*commit=*/commit1, collateral);

        // Provider was registered at slot 0 of this stream
        EXPECT_EQ(r.state()->populations.get(stream), 1u);

        r.endTick();
        // After END_TICK on stream s: flag cleared, collateral stays locked, entropy stays zero.
        EXPECT_TRUE(r.state()->entropy.get(stream * 10 + tier) == zero);
        EXPECT_EQ(r.state()->populations.get(stream), 1u);

        // Advance two ticks (streams (s+1)%3 and (s+2)%3). We don't need to
        // call END_TICK for them; they have no providers, so the BuyEntropy
        // verification only depends on stream s.
        // ----- Tick T+3 : reveal reveal1, commit commit2 ----------------------
        r.setTick(startTick + 3);
        increaseEnergy(provider, collateral);
        r.revealAndCommit(provider, /*reveal=*/reveal1, /*commit=*/commit2, collateral);

        // The reveal must have been accepted (commits[i] == K12(reveal1))
        EXPECT_FALSE(r.state()->reveals.get(stream * 1365 + 0) == zero)
            << "Reveal was not accepted by RevealAndCommit";

        r.endTick();

        // END_TICK XORs reveal1 into entropy[stream*10+tier], leaving it = reveal1.
        EXPECT_TRUE(r.state()->entropy.get(stream * 10 + tier) == reveal1)
            << "END_TICK should have finalized entropy = reveal1 for stream "
            << stream << " tier " << tier;

        // ----- Tick T+4 : BuyEntropy must return the latest entropy -------
        r.setTick(startTick + 4);

        // Sanity: the contract's "latest finalized stream" formula must
        // resolve to s, which is where we just wrote reveal1.
        const uint32 expectedLatest = (r.tick() + 2u) % 3u;
        ASSERT_EQ(expectedLatest, stream);

        // Request the full 4096 bits so the whole entropy value is delivered
        // and can be compared against reveal1 directly.
        const uint16 numberOfBits = 4096;
        const sint64 fee = (sint64)numberOfBits * 100;

        id buyer = getUser(0xBEEF + startTick);
        increaseEnergy(buyer, fee);
        const long long buyerBefore = getBalance(buyer);
        const uint64 earnedBefore = r.state()->earnedAmount;

        auto out = r.buyEntropy(buyer, tier, numberOfBits, fee);

        // Fee was consumed (no refund path)
        EXPECT_EQ(getBalance(buyer), buyerBefore - fee)
            << "BuyEntropy must consume fee when entropy is available";
        EXPECT_EQ(r.state()->earnedAmount, earnedBefore + (uint64)fee);

        // Output entropy must equal what END_TICK produced
        EXPECT_TRUE(out.entropy == reveal1)
            << "BuyEntropy at tick " << r.tick()
            << " (stream " << stream
            << ") returned wrong entropy. With the old bug "
               "(entropy[tier] = stream 0 only) this fails on startTick%3 != 0.";
    }
}

TEST(ContractRandom, EndToEnd_LatestEntropyRetrieved_TickMod3_Is0)
{
    runFullRandomCycle(/*startTick=*/1002); // 1002 % 3 == 0 -> stream 0
}

TEST(ContractRandom, EndToEnd_LatestEntropyRetrieved_TickMod3_Is1)
{
    runFullRandomCycle(/*startTick=*/1003); // 1003 % 3 == 1 -> stream 1
}

TEST(ContractRandom, EndToEnd_LatestEntropyRetrieved_TickMod3_Is2)
{
    runFullRandomCycle(/*startTick=*/1004); // 1004 % 3 == 2 -> stream 2
}

// ---------------------------------------------------------------------------
// Trustee condition. BuyEntropy carries a single optional trustee:
//   - trustee == 0                         : unconditional, plain buy.
//   - trustee is a provider in the
//     stream/tier being bought             : entropy delivered, fee charged.
//   - trustee absent from that stream/tier : nothing delivered, nothing charged.
// ---------------------------------------------------------------------------

namespace
{
    // Drive one provider through commit -> reveal so entropy[stream*10+tier] = reveal1
    // at tick T+4. Leaves the contract at tick T+4.
    struct TrusteeScenario
    {
        uint8 tier;
        uint32 stream;
        id provider;
        QPI::bit_4096 entropy;
    };

    TrusteeScenario buildEntropyWithProvider(ContractTestingRandom& r, uint32 startTick,
                                             uint8 tier, uint64 providerSeed)
    {
        r.setTick(startTick);
        const sint64 collateral = collateralForTier(tier);
        const uint32 stream = startTick % 3;

        const QPI::bit_4096 reveal1 = makeReveal(providerSeed * 7654321u + 1u);
        const id commit1 = commitOf(reveal1);
        const id commit2 = commitOf(makeReveal(providerSeed * 7654321u + 2u));
        const id provider = getUser(providerSeed);

        QPI::bit_4096 zero; zero.setAll(0);

        // Tick T: first commit (no reveal yet).
        increaseEnergy(provider, collateral);
        r.revealAndCommit(provider, zero, commit1, collateral);
        r.endTick();

        // Tick T+3 (same stream): reveal reveal1, recommit. END_TICK XORs
        // reveal1 into entropy[stream*10+tier].
        r.setTick(startTick + 3);
        increaseEnergy(provider, collateral);
        r.revealAndCommit(provider, reveal1, commit2, collateral);
        r.endTick();

        // Tick T+4: the latest finalized stream is exactly `stream`.
        r.setTick(startTick + 4);

        TrusteeScenario s;
        s.tier = tier;
        s.stream = stream;
        s.provider = provider;
        s.entropy = reveal1;
        return s;
    }
}

// A trustee that is a provider in the bought stream/tier: behaves like a plain
// buy (entropy delivered, fee charged).
TEST(ContractRandom, BuyEntropyWithMatchingTrusteeSucceeds)
{
    ContractTestingRandom r;
    auto s = buildEntropyWithProvider(r, /*startTick=*/1002, /*tier=*/2, /*seed=*/0xA11CE);
    ASSERT_TRUE(r.state()->entropy.get(s.stream * 10 + s.tier) == s.entropy);

    const uint16 numberOfBits = 4096;
    const sint64 fee = (sint64)numberOfBits * 100;
    id buyer = getUser(0xB0B);
    increaseEnergy(buyer, fee);
    const long long before = getBalance(buyer);
    const uint64 earnedBefore = r.state()->earnedAmount;

    auto out = r.buyEntropy(buyer, s.tier, numberOfBits, fee, /*trustee=*/s.provider);

    EXPECT_EQ(getBalance(buyer), before - fee)
        << "buy with a present trustee must charge the fee";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore + (uint64)fee);
    EXPECT_TRUE(out.entropy == s.entropy)
        << "buy with a present trustee must return the finalized entropy";
}

// A trustee absent from the stream: charge nothing, return all-zeros.
TEST(ContractRandom, BuyEntropyWithAbsentTrusteeRefundsAndReturnsZero)
{
    ContractTestingRandom r;
    auto s = buildEntropyWithProvider(r, /*startTick=*/1002, /*tier=*/2, /*seed=*/0xA11CE);

    const uint16 numberOfBits = 4096;
    const sint64 fee = (sint64)numberOfBits * 100;
    id buyer = getUser(0xB0B);
    increaseEnergy(buyer, fee);
    const long long before = getBalance(buyer);
    const uint64 earnedBefore = r.state()->earnedAmount;

    const id absentTrustee = getUser(0xDEAD); // never registered as a provider
    auto out = r.buyEntropy(buyer, s.tier, numberOfBits, fee, /*trustee=*/absentTrustee);

    EXPECT_EQ(getBalance(buyer), before)
        << "buy with an absent trustee must charge nothing (full refund)";
    EXPECT_EQ(r.state()->earnedAmount, earnedBefore)
        << "buy with an absent trustee must not earn any fee";
    QPI::bit_4096 zero; zero.setAll(0);
    EXPECT_TRUE(out.entropy == zero)
        << "buy with an absent trustee must return all-zero entropy";
}

// A zero trustee imposes no condition: identical to the original BuyEntropy.
TEST(ContractRandom, BuyEntropyZeroTrusteeBehavesLikePlainBuy)
{
    ContractTestingRandom r;
    auto s = buildEntropyWithProvider(r, /*startTick=*/1002, /*tier=*/2, /*seed=*/0xA11CE);

    const uint16 numberOfBits = 4096;
    const sint64 fee = (sint64)numberOfBits * 100;
    id buyer = getUser(0xB0B);
    increaseEnergy(buyer, fee);
    const long long before = getBalance(buyer);

    auto out = r.buyEntropy(buyer, s.tier, numberOfBits, fee, /*trustee=*/id::zero());

    EXPECT_EQ(getBalance(buyer), before - fee);
    EXPECT_TRUE(out.entropy == s.entropy);
}

// A trustee present only in a different tier does not satisfy the condition
// (tier must match). The bought tier has its own real entropy.
TEST(ContractRandom, BuyEntropyTrusteeInOtherTierRefunds)
{
    ContractTestingRandom r;
    const uint32 startTick = 1002;
    const uint32 stream = startTick % 3;
    const uint8 boughtTier = 2;
    const uint8 otherTier = 3;
    const sint64 collateralBought = collateralForTier(boughtTier);
    const sint64 collateralOther = collateralForTier(otherTier);

    const id providerBought = getUser(0xA11CE);
    const id providerOther = getUser(0xB0B0);

    const QPI::bit_4096 revealBought = makeReveal(11);
    const QPI::bit_4096 revealOther = makeReveal(22);
    QPI::bit_4096 zero; zero.setAll(0);

    // Tick T: both providers first-commit on the same stream, different tiers.
    r.setTick(startTick);
    increaseEnergy(providerBought, collateralBought);
    r.revealAndCommit(providerBought, zero, commitOf(revealBought), collateralBought);
    increaseEnergy(providerOther, collateralOther);
    r.revealAndCommit(providerOther, zero, commitOf(revealOther), collateralOther);
    r.endTick();

    // Tick T+3: both reveal, so tier 2 and tier 3 entropy both become non-zero.
    r.setTick(startTick + 3);
    increaseEnergy(providerBought, collateralBought);
    r.revealAndCommit(providerBought, revealBought, commitOf(makeReveal(111)), collateralBought);
    increaseEnergy(providerOther, collateralOther);
    r.revealAndCommit(providerOther, revealOther, commitOf(makeReveal(222)), collateralOther);
    r.endTick();

    r.setTick(startTick + 4);
    ASSERT_FALSE(r.state()->entropy.get(stream * 10 + boughtTier) == zero)
        << "tier being bought must have its own real entropy for this test to isolate the tier check";

    const uint16 numberOfBits = 4096;
    const sint64 fee = (sint64)numberOfBits * 100;
    id buyer = getUser(0xBEEF);
    increaseEnergy(buyer, fee);
    const long long before = getBalance(buyer);

    // Buy tier 2 but name the tier-3 provider as trustee.
    auto out = r.buyEntropy(buyer, boughtTier, numberOfBits, fee, /*trustee=*/providerOther);

    EXPECT_EQ(getBalance(buyer), before)
        << "a trustee present only in another tier must not satisfy the condition";
    EXPECT_TRUE(out.entropy == zero);

    // Sanity: naming the correct-tier provider succeeds against the same state.
    increaseEnergy(buyer, fee);
    const long long before2 = getBalance(buyer);
    auto out2 = r.buyEntropy(buyer, boughtTier, numberOfBits, fee, /*trustee=*/providerBought);
    EXPECT_EQ(getBalance(buyer), before2 - fee);
    EXPECT_TRUE(out2.entropy == r.state()->entropy.get(stream * 10 + boughtTier));
}

// Enrollment alone doesn't satisfy the trustee condition — the provider must have
// actually XOR'd a reveal into entropy this round (contributedToEntropyFlags=1).
//
// Scenario (stream 0, T=1002): Bob commits; T+3 Bob reveals + Alice first-commits;
// T+4 trustee=Alice refunds (flag=0), trustee=Bob succeeds (flag=1).
TEST(ContractRandom, BuyEntropyFreshTrusteeRefunds)
{
    ContractTestingRandom r;
    const uint32 startTick = 1002;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = startTick % 3;

    const id bob   = getUser(0xB0B);
    const id alice = getUser(0xA11CE);

    const QPI::bit_4096 revealBob = makeReveal(0xB0B * 7 + 1);
    const id commitBob1 = commitOf(revealBob);
    const id commitBob2 = commitOf(makeReveal(0xB0B * 7 + 2));
    const id commitAlice = commitOf(makeReveal(0xA11CE * 7 + 1));

    QPI::bit_4096 zero; zero.setAll(0);

    // Tick T: Bob first-commits only.
    r.setTick(startTick);
    increaseEnergy(bob, collateral);
    r.revealAndCommit(bob, zero, commitBob1, collateral);
    r.endTick();
    // Empty reveal channel -> entropy stays zero, Bob stays enrolled.
    ASSERT_TRUE(r.state()->entropy.get(stream * 10 + tier) == zero);
    ASSERT_EQ(r.state()->populations.get(stream), 1u);

    // Tick T+3: Bob reveals (entropy becomes revealBob), Alice first-commits.
    r.setTick(startTick + 3);
    increaseEnergy(bob, collateral);
    r.revealAndCommit(bob, revealBob, commitBob2, collateral);
    increaseEnergy(alice, collateral);
    r.revealAndCommit(alice, zero, commitAlice, collateral);
    r.endTick();

    // Sanity: entropy is now non-zero (Bob's reveal was XOR'd in).
    ASSERT_FALSE(r.state()->entropy.get(stream * 10 + tier) == zero)
        << "Bob's reveal must have produced non-zero entropy";
    ASSERT_EQ(r.state()->populations.get(stream), 2u)
        << "Both Bob and Alice must be enrolled";

    // Find Alice's index and verify her contributedToEntropyFlags is 0.
    bool aliceFound = false;
    for (uint32 i = 0; i < r.state()->populations.get(stream); i++)
    {
        uint32 idx = stream * RANDOM_STREAM_CAPACITY + i;
        if (r.state()->providers.get(idx) == alice)
        {
            aliceFound = true;
            EXPECT_EQ(r.state()->contributedToEntropyFlags.get(idx), 0)
                << "Alice only committed this round; her contribution flag must be 0";
            break;
        }
    }
    ASSERT_TRUE(aliceFound) << "Alice must be in the pool after first-commit";

    // Tick T+4: BuyEntropy.
    r.setTick(startTick + 4);
    const uint16 numberOfBits = 4096;
    const sint64 fee = (sint64)numberOfBits * 100;

    // --- Alice as trustee: must refund (she did not contribute to the entropy).
    id buyer = getUser(0xBEEF);
    increaseEnergy(buyer, fee);
    const long long beforeAlice = getBalance(buyer);
    const uint64 earnedBeforeAlice = r.state()->earnedAmount;

    auto outAlice = r.buyEntropy(buyer, tier, numberOfBits, fee, /*trustee=*/alice);

    EXPECT_EQ(getBalance(buyer), beforeAlice)
        << "BuyEntropy with fresh-committer trustee must refund (trustee did not reveal)";
    EXPECT_EQ(r.state()->earnedAmount, earnedBeforeAlice)
        << "earnedAmount must not increase on trustee refund";
    EXPECT_TRUE(outAlice.entropy == zero)
        << "output must be all-zero when trustee check fails";

    // --- Bob as trustee: must succeed (he did reveal and his flag is set).
    increaseEnergy(buyer, fee);
    const long long beforeBob = getBalance(buyer);
    const uint64 earnedBeforeBob = r.state()->earnedAmount;

    auto outBob = r.buyEntropy(buyer, tier, numberOfBits, fee, /*trustee=*/bob);

    EXPECT_EQ(getBalance(buyer), beforeBob - fee)
        << "BuyEntropy with an established trustee must charge the fee";
    EXPECT_EQ(r.state()->earnedAmount, earnedBeforeBob + (uint64)fee);
    EXPECT_FALSE(outBob.entropy == zero)
        << "BuyEntropy with an established trustee must return real entropy";
}

// ---------------------------------------------------------------------------
// Collateral-lifecycle tests (C1 / C2 / C3 / H4).
//
// The contract holds a provider's stake from the moment they commit until
// they reveal it in a later round. The core invariant is: the contract never
// pays or burns a tier amount unless that exact amount is recorded in
// state.lockedCollateralAmounts[index].
// ---------------------------------------------------------------------------

// C3: a first commit must NOT be refunded at END_TICK. The stake stays locked
// in the contract until the provider reveals it (or is slashed for not doing
// so). Refunding at the commit tick means no real stake is ever held.
TEST(ContractRandom, FirstCommitHoldsCollateralThroughEndTick)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier); // 100
    const uint32 stream = r.tick() % 3;
    const uint32 index = stream * 1365 + 0;

    id provider = getUser(1);
    increaseEnergy(provider, collateral);

    QPI::bit_4096 zero; zero.setAll(0);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(1)), collateral);

    // Stake was paid in and recorded as locked.
    EXPECT_EQ(getBalance(provider), 0);
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), (uint64)collateral);

    r.endTick();

    // C3: the collateral must still be locked after END_TICK, not refunded.
    EXPECT_EQ(getBalance(provider), 0)
        << "first-commit collateral must stay locked through END_TICK";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), (uint64)collateral)
        << "lockedCollateralAmounts must be unchanged by END_TICK";
    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "a provider who committed must remain in the pool";
}

// C3: a valid reveal refunds exactly the previously locked stake (once) and
// locks the freshly supplied stake for the next round.
TEST(ContractRandom, RevealRefundsOldCollateralExactlyOnce)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;
    const uint32 index = stream * 1365 + 0;

    auto reveal1 = makeReveal(1);
    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    // First commit.
    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(reveal1), collateral);
    r.endTick();

    // Next cycle: reveal the preimage and commit again.
    r.setTick(r.tick() + 3);
    increaseEnergy(provider, collateral);
    const long long before = getBalance(provider); // just-funded new stake

    r.revealAndCommit(provider, reveal1, commitOf(makeReveal(2)), collateral);

    // The provider paid one new stake and got the old one back -> net zero.
    EXPECT_EQ(getBalance(provider), before)
        << "reveal must refund exactly the previously locked collateral";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), (uint64)collateral)
        << "the new commit's collateral must now be locked";
}

// C2: a reveal carrying an empty commit must be rejected BEFORE any state
// change and refunded exactly once. The old bug set the participation flag and
// refunded, then END_TICK refunded the collateral a second time.
TEST(ContractRandom, RevealWithZeroCommitIsRejectedNoDoublePay)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;
    const uint32 index = stream * 1365 + 0;

    auto reveal1 = makeReveal(1);
    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    // First commit.
    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(reveal1), collateral);
    r.endTick();

    // Next cycle: attempt to reveal with commit == 0.
    r.setTick(r.tick() + 3);
    increaseEnergy(provider, collateral);
    const long long before = getBalance(provider);

    r.revealAndCommit(provider, reveal1, /*commit=*/id::zero(), collateral);

    // The call is rejected and refunded once; no state was touched.
    EXPECT_EQ(getBalance(provider), before)
        << "commit==0 must be refunded once, without double-paying";
    EXPECT_TRUE(r.state()->reveals.get(index) == zero)
        << "a rejected commit==0 call must not store the reveal";
    EXPECT_EQ(r.state()->revealedThisTickFlags.get(index), 0)
        << "a rejected commit==0 call must not set the participation flag";
}

// S1: commit and reveal in the same tick must be rejected. The whole point of the
// commit-reveal scheme is that the preimage is locked before the reveal window opens.
// If both are allowed in one tick, a provider can observe other reveals, pick an
// optimal preimage, and submit commit+reveal together — breaking entropy security.
//
// The guard reuses revealOrCommitFlags: the first-commit path sets the flag; END_TICK
// clears it. So a reveal attempted while the flag is already set means the provider
// committed this tick and must be rejected.
TEST(ContractRandom, SameTickCommitAndRevealIsRejected)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;
    const uint32 index = stream * RANDOM_STREAM_CAPACITY + 0;

    const QPI::bit_4096 reveal1 = makeReveal(0xDEAD);
    const id commit1 = commitOf(reveal1);
    const id commit2 = commitOf(makeReveal(0xBEEF));
    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    // Tx 1: first-commit. Registers the provider and sets revealOrCommitFlags=1.
    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commit1, collateral);
    ASSERT_EQ(r.state()->populations.get(stream), 1u);
    ASSERT_TRUE(r.state()->commits.get(index) == commit1);

    // Tx 2 (same tick): attempt to reveal the preimage of commit1 immediately.
    // Must be rejected — provider must not be able to pick the preimage after
    // observing other reveals in the same tick.
    increaseEnergy(provider, collateral);
    const long long before = getBalance(provider);

    r.revealAndCommit(provider, reveal1, commit2, collateral);

    EXPECT_EQ(getBalance(provider), before)
        << "same-tick reveal must be refunded in full";
    EXPECT_TRUE(r.state()->reveals.get(index) == zero)
        << "same-tick reveal must not store the reveal value";
    EXPECT_EQ(r.state()->revealedThisTickFlags.get(index), 0)
        << "same-tick reveal must not set the revealed flag";
    EXPECT_TRUE(r.state()->commits.get(index) == commit1)
        << "same-tick reveal must not overwrite the existing commit";

    // After END_TICK the provider is still enrolled (they committed legitimately).
    r.endTick();
    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "provider must remain enrolled after END_TICK";

    // Tick T+3: the normal reveal must now succeed.
    r.setTick(r.tick() + 3);
    increaseEnergy(provider, collateral);
    const long long before2 = getBalance(provider);

    r.revealAndCommit(provider, reveal1, commit2, collateral);

    EXPECT_EQ(getBalance(provider), before2)
        << "legitimate reveal at T+3 must refund prior collateral (net zero)";
    EXPECT_EQ(r.state()->revealedThisTickFlags.get(index), 1)
        << "legitimate reveal at T+3 must set the revealed flag";
}

// C1: when at least one reveal lands in a round, a no-show provider's stake is
// burned and they are evicted. A witness provider proves the reveal channel was
// live — without one END_TICK takes the empty-tick path and refunds instead of slashing.
TEST(ContractRandom, SilentProviderIsSlashedNotPaid)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id silent  = getUser(1);
    id witness = getUser(2);
    auto witR1 = makeReveal(101);
    auto witR2 = makeReveal(102);
    QPI::bit_4096 zero; zero.setAll(0);

    // T: both providers commit. T+3 will be a slash-eligible round once the
    // witness reveals.
    increaseEnergy(silent,  collateral);
    increaseEnergy(witness, collateral);
    r.revealAndCommit(silent,  zero, commitOf(makeReveal(1)), collateral);
    r.revealAndCommit(witness, zero, commitOf(witR1),         collateral);
    r.endTick();

    const uint64    burnedBefore  = r.state()->burnedAmount;
    const long long silentBefore  = getBalance(silent);

    // T+3: witness reveals, silent does nothing. The reveal channel is proven
    // usable, so slashing applies to silent.
    r.setTick(r.tick() + 3);
    increaseEnergy(witness, collateral);
    r.revealAndCommit(witness, witR1, commitOf(witR2), collateral);
    r.endTick();

    EXPECT_EQ(getBalance(silent), silentBefore)
        << "a silent provider must not receive any payout when a peer revealed";
    EXPECT_EQ(r.state()->burnedAmount, burnedBefore + (uint64)collateral)
        << "the silent provider's locked collateral must be burned";
    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "only the witness must remain; the silent provider is removed";
}

TEST(ContractRandom, EmptyRevealChannelRefundsAndRemovesPendingCommit)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();
    const uint32 index = stream * RANDOM_STREAM_CAPACITY;
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), (uint64)collateral);

    const uint64    burnedBefore  = r.state()->burnedAmount;
    const long long balanceBefore = getBalance(provider);

    // T+3: nobody reveals. END_TICK refunds the stake and evicts the provider
    // so the slot is reusable — a stale commit would brick re-registration.
    r.setTick(r.tick() + 3);
    r.endTick();

    EXPECT_EQ(getBalance(provider), balanceBefore + collateral)
        << "pending-commit stake must be refunded on empty reveal channel";
    EXPECT_EQ(r.state()->burnedAmount, burnedBefore)
        << "nothing must be burned when the reveal channel was empty";
    EXPECT_EQ(r.state()->populations.get(stream), 0u)
        << "stale-commit holder must be swap-deleted so slot is reusable";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), 0u);
    EXPECT_TRUE(r.state()->providers.get(index) == id::zero())
        << "provider slot must be cleared so re-registration is possible";
    EXPECT_TRUE(r.state()->commits.get(index) == id::zero());
}

// R1 regression test: a force-majeured provider must be able to register
// again on the next stream-tick. Without swap-delete in the empty branch
// their slot would stay bricked.
TEST(ContractRandom, ProviderCanReRegisterAfterEmptyRevealChannel)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();

    r.setTick(r.tick() + 3);
    r.endTick();
    ASSERT_EQ(r.state()->populations.get(stream), 0u);

    // Walk forward until tick%3 == stream again so we are in this stream's
    // commit window.
    while ((r.tick() % 3) != stream) { r.setTick(r.tick() + 1); r.endTick(); }

    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(2)), collateral);

    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "provider must be able to register again after a force-majeure refund";
    const uint32 index = stream * RANDOM_STREAM_CAPACITY;
    EXPECT_TRUE(r.state()->providers.get(index) == provider)
        << "fresh registration must populate the freed slot";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(index), (uint64)collateral)
        << "fresh stake must be locked";
}

TEST(ContractRandom, FirstCommitDoesNotCountAsRevealForEmptyChannel)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id stale   = getUser(1);  // committed at T, expected to reveal at T+3
    id joiner  = getUser(2);  // first-commits at T+3
    QPI::bit_4096 zero; zero.setAll(0);

    // T: stale commits.
    increaseEnergy(stale, collateral);
    r.revealAndCommit(stale, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();

    const uint64    burnedBefore = r.state()->burnedAmount;
    const long long staleBefore  = getBalance(stale);

    // T+3: stale does nothing, joiner does a first-commit only. No actual
    // reveal landed → empty-reveal-channel branch must fire.
    r.setTick(r.tick() + 3);
    increaseEnergy(joiner, collateral);
    r.revealAndCommit(joiner, zero, commitOf(makeReveal(2)), collateral);
    r.endTick();

    EXPECT_EQ(getBalance(stale), staleBefore + collateral)
        << "stale provider must be refunded - a first-commit is not a reveal";
    EXPECT_EQ(r.state()->burnedAmount, burnedBefore)
        << "no burn when the reveal channel was empty";
    // Stale is swap-deleted; fresh joiner stays. After swap-delete the
    // joiner may end up at any index (depends on which slot was vacated).
    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "stale removed, joiner kept";

    bool joinerFound = false;
    for (uint32 i = 0; i < r.state()->populations.get(stream); i++)
    {
        const uint32 idx = stream * RANDOM_STREAM_CAPACITY + i;
        if (r.state()->providers.get(idx) == joiner)
        {
            joinerFound = true;
            EXPECT_EQ(r.state()->lockedCollateralAmounts.get(idx), (uint64)collateral)
                << "joiner's fresh stake must remain locked for next round";
            EXPECT_FALSE(r.state()->commits.get(idx) == id::zero())
                << "joiner's fresh commit must be preserved";
        }
    }
    EXPECT_TRUE(joinerFound) << "joiner must remain in the pool";
}

TEST(ContractRandom, EmptyRevealChannelIsolatedToStream)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);

    // Align so each stream has a committed provider going into its next round.
    r.setTick(r.tick());
    const uint32 stream0 = r.tick() % 3;
    const uint32 stream1 = (r.tick() + 1) % 3;
    const uint32 stream2 = (r.tick() + 2) % 3;

    id p0 = getUser(10);
    id p1 = getUser(11);
    id p2 = getUser(12);
    QPI::bit_4096 zero; zero.setAll(0);

    // Each provider commits in their own stream's tick.
    increaseEnergy(p0, collateral);
    r.revealAndCommit(p0, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();

    r.setTick(r.tick() + 1);
    increaseEnergy(p1, collateral);
    r.revealAndCommit(p1, zero, commitOf(makeReveal(2)), collateral);
    r.endTick();

    r.setTick(r.tick() + 1);
    increaseEnergy(p2, collateral);
    r.revealAndCommit(p2, zero, commitOf(makeReveal(3)), collateral);
    r.endTick();

    // Now jump forward exactly 3 ticks (so we land on stream0's reveal round)
    // and run END_TICK with no reveal activity. Only stream0 must be touched.
    r.setTick(r.tick() + 1); // now == original_tick + 3, mod 3 == stream0
    const uint64 lockedS1Before = r.state()->lockedCollateralAmounts.get(stream1 * RANDOM_STREAM_CAPACITY);
    const uint64 lockedS2Before = r.state()->lockedCollateralAmounts.get(stream2 * RANDOM_STREAM_CAPACITY);
    r.endTick();

    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(stream0 * RANDOM_STREAM_CAPACITY), 0u)
        << "stream0's locked stake must be refunded (empty reveal tick)";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(stream1 * RANDOM_STREAM_CAPACITY), lockedS1Before)
        << "stream1 must be untouched by stream0's END_TICK";
    EXPECT_EQ(r.state()->lockedCollateralAmounts.get(stream2 * RANDOM_STREAM_CAPACITY), lockedS2Before)
        << "stream2 must be untouched by stream0's END_TICK";
    EXPECT_EQ(r.state()->populations.get(stream0), 0u)
        << "stream0's stale-commit holder is swap-deleted";
    EXPECT_EQ(r.state()->populations.get(stream1), 1u)
        << "stream1 untouched";
    EXPECT_EQ(r.state()->populations.get(stream2), 1u)
        << "stream2 untouched";
}

TEST(ContractRandom, ConsecutiveEmptyRevealChannelsDoNotDoubleRefund)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();

    const long long balanceBefore = getBalance(provider);

    // First empty reveal tick: refund.
    r.setTick(r.tick() + 3);
    r.endTick();
    EXPECT_EQ(getBalance(provider), balanceBefore + collateral)
        << "first empty reveal tick must refund once";

    // After the first empty tick the provider was swap-deleted, so the
    // second empty tick has no providers to process at all.
    r.setTick(r.tick() + 3);
    r.endTick();
    EXPECT_EQ(getBalance(provider), balanceBefore + collateral)
        << "second empty reveal tick must NOT refund again";
    EXPECT_EQ(r.state()->populations.get(stream), 0u)
        << "stream must remain empty across consecutive empty ticks";
}

// H4: when one provider in a tier is a no-show, the other providers in that
// same tier must not be locked out. END_TICK clears their reveal slot even
// though the poisoned tier's entropy XOR is skipped.
TEST(ContractRandom, NoShowInTierDoesNotLockOutOtherProviders)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    auto good1 = makeReveal(10);
    auto good2 = makeReveal(11);
    auto good3 = makeReveal(12);
    auto bad1  = makeReveal(20);

    id good = getUser(1);
    id bad  = getUser(2);
    QPI::bit_4096 zero; zero.setAll(0);

    // Tick T: both providers register in the same tier.
    increaseEnergy(good, collateral);
    increaseEnergy(bad, collateral);
    r.revealAndCommit(good, zero, commitOf(good1), collateral);
    r.revealAndCommit(bad,  zero, commitOf(bad1),  collateral);
    r.endTick();
    EXPECT_EQ(r.state()->populations.get(stream), 2u);

    // Tick T+3: good reveals, bad is a no-show.
    r.setTick(r.tick() + 3);
    increaseEnergy(good, collateral);
    r.revealAndCommit(good, good1, commitOf(good2), collateral);
    r.endTick();

    EXPECT_EQ(r.state()->populations.get(stream), 1u)
        << "the no-show provider must be removed from the pool";

    // Tick T+6: good must still be able to reveal+commit. This only works if
    // END_TICK cleared good's reveal slot at T+3 despite the poisoned tier.
    r.setTick(r.tick() + 3);
    increaseEnergy(good, collateral);
    r.revealAndCommit(good, good2, commitOf(good3), collateral);

    bool found = false;
    for (uint32 i = 0; i < r.state()->populations.get(stream); i++)
    {
        if (r.state()->providers.get(stream * 1365 + i) == good)
        {
            found = true;
            EXPECT_EQ(r.state()->revealedThisTickFlags.get(stream * 1365 + i), 1)
                << "good provider's reveal at T+6 must be accepted, not rejected";
        }
    }
    EXPECT_TRUE(found) << "good provider must still be in the pool";
}
