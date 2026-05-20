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

// Helper: compute K12(reveal) -> id, exactly the same operation the contract
// performs via qpi.K12(input.reveal) when validating reveals against commits.
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
                                         uint16 numberOfBits, sint64 amount)
    {
        RANDOM::BuyEntropy_input input{};
        input.collateralTier = collateralTier;
        input.numberOfBits = numberOfBits;
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

// Specification (per CFB): "BuyEntropy procedure is for buying entropy. The
// fee is returned if there is no entropy, in this case output will be all
// zeros." -- the user only loses funds when entropy is actually delivered.
// By symmetry, when the request is rejected because of invalid inputs (bad
// tier, zero/oversize bit count, underpaid fee) no entropy is delivered, so
// the spec-correct behaviour is also a full refund. earnedAmount must NOT
// be credited and output must stay all-zero.
//
// KNOWN BUG (Random.h, BuyEntropy): the procedure has no `else` branch on
// the top-level guard, so invalid inputs silently keep the fee. These tests
// encode the spec and will fail until the contract adds an explicit refund
// for invalid inputs:
//     else { qpi.transfer(qpi.invocator(), qpi.invocationReward()); }

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
// End-to-end test: drive RevealAndCommit + END_TICK so the contract itself
// produces real entropy, then verify BuyEntropy hands back the *latest*
// finalized entropy slot (not the bug's hardcoded stream-0 slot).
//
// Lifecycle of one provider on stream s (= tick % 3):
//   tick T       : RevealAndCommit(reveal=0, commit=K12(reveal1))   -> provider
//                  registered, commit stored. Reveals stay zero, so END_TICK
//                  finalizes entropy[s*10+tier] = 0 (XOR with zero).
//   tick T+3     : RevealAndCommit(reveal=reveal1, commit=K12(reveal2)) -> reveal
//                  reveal1 is accepted because K12(reveal1) matches the prior
//                  commit. END_TICK XORs reveal1 into entropy[s*10+tier], so
//                  entropy = reveal1.
//   tick T+4     : BuyEntropy(tier). The contract reads
//                  latestStream = (tick + 2) % 3 = (T+4+2) % 3 = T % 3 = s,
//                  so it must return entropy[s*10+tier] = reveal1.
//
// Done for every residue of T mod 3, so the test is sensitive to the original
// bug where BuyEntropy always read stream-0 instead of the latest stream.
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
        // After END_TICK on stream s, collateral was refunded, flag cleared,
        // entropy slot stays zero (reveal was zero).
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

// C1: a provider who does nothing in a round receives NOTHING. Their locked
// stake is burned (slashed) and they are removed from the pool. The old bug
// paid silent providers from the treasury.
TEST(ContractRandom, SilentProviderIsSlashedNotPaid)
{
    ContractTestingRandom r;
    const uint8 tier = 2;
    const sint64 collateral = collateralForTier(tier);
    const uint32 stream = r.tick() % 3;

    id provider = getUser(1);
    QPI::bit_4096 zero; zero.setAll(0);

    // First commit, then survive one END_TICK with the stake locked.
    increaseEnergy(provider, collateral);
    r.revealAndCommit(provider, zero, commitOf(makeReveal(1)), collateral);
    r.endTick();

    const uint64 burnedBefore = r.state()->burnedAmount;
    const long long balanceBefore = getBalance(provider);

    // Next cycle: the provider stays silent.
    r.setTick(r.tick() + 3);
    r.endTick();

    // C1: silent provider is not paid; their stake is burned and they are gone.
    EXPECT_EQ(getBalance(provider), balanceBefore)
        << "a silent provider must not receive any payout";
    EXPECT_EQ(r.state()->burnedAmount, burnedBefore + (uint64)collateral)
        << "the silent provider's locked collateral must be burned";
    EXPECT_EQ(r.state()->populations.get(stream), 0u)
        << "the silent provider must be removed from the pool";
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
