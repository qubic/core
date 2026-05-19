#define NO_UEFI

#include "contract_testing.h"

namespace
{
constexpr uint16 RANDOM_FUNCTION_FEES = 1;
constexpr uint16 RANDOM_PROCEDURE_BUY_ENTROPY = 2;

constexpr uint64 BIT_FEE = 100;

static const id BUYER(0x1111, 0x2222, 0x3333, 0x4444);
static const id PROVIDER(0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD);

static uint32 buyEntropyStream(uint32 tick)
{
	return (tick + 2) % 3;
}

static uint32 endTickStream(uint32 tick)
{
	return tick % 3;
}

static uint32 entropySlot(uint32 tick, uint8 collateralTier)
{
	return buyEntropyStream(tick) * 10 + collateralTier;
}

static bit_4096 makeEntropyWithBitSet(uint64 bitIndex)
{
	bit_4096 entropy{};
	entropy.set(bitIndex, 1);
	return entropy;
}

static bool outputEntropySlotHasBit(const RANDOM::BuyEntropy_output& output, uint64 outputSlot, uint64 bitIndex)
{
	return output.entropy.get(outputSlot).get(bitIndex) != 0;
}

static bool bit4096IsZero(const bit_4096& value)
{
	bit_4096 zero{};
	return value == zero;
}

} // namespace

class RANDOMChecker : public RANDOM, public RANDOM::StateData
{
public:
	void setEntropySlot(uint32 index, const bit_4096& value)
	{
		entropy.set(index, value);
	}

	const bit_4096& getEntropySlot(uint32 index) const
	{
		return entropy.get(index);
	}

	uint64 getEarnedAmount() const
	{
		return earnedAmount;
	}

	void setPopulation(uint32 stream, uint32 population)
	{
		populations.set(stream, population);
	}

	void setProvider(uint32 stream, uint32 index, const id& provider)
	{
		providers.set(stream * 1365 + index, provider);
	}

	void setCollateralTier(uint32 stream, uint32 index, uint64 tier)
	{
		collateralTiers.set(stream * 1365 + index, tier);
	}

	void setCommit(uint32 stream, uint32 index, const id& commit)
	{
		commits.set(stream * 1365 + index, commit);
	}

	void setReveal(uint32 stream, uint32 index, const bit_4096& reveal)
	{
		reveals.set(stream * 1365 + index, reveal);
	}

	void setRevealOrCommitFlag(uint32 stream, uint32 index, bool value)
	{
		revealOrCommitFlags.set(stream * 1365 + index, value ? 1 : 0);
	}
};

class ContractTestingRANDOM : protected ContractTesting
{
public:
	ContractTestingRANDOM()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(RANDOM);
		callSystemProcedure(RANDOM_CONTRACT_INDEX, INITIALIZE);
		increaseEnergy(BUYER, 10'000'000);
		increaseEnergy(PROVIDER, 10'000'000);
	}

	RANDOMChecker* state()
	{
		return reinterpret_cast<RANDOMChecker*>(contractStates[RANDOM_CONTRACT_INDEX]);
	}

	void setTick(uint32 tick)
	{
		system.tick = tick;
	}

	uint32 getTick() const
	{
		return system.tick;
	}

	void endTick(bool expectSuccess = true)
	{
		callSystemProcedure(RANDOM_CONTRACT_INDEX, END_TICK, expectSuccess);
	}

	RANDOM::Fees_output fees() const
	{
		RANDOM::Fees_input input{};
		RANDOM::Fees_output output{};
		callFunction(RANDOM_CONTRACT_INDEX, RANDOM_FUNCTION_FEES, input, output);
		return output;
	}

	bool buyEntropy(
		const id& user,
		uint64 reward,
		uint8 collateralTier,
		uint16 numberOfBits,
		RANDOM::BuyEntropy_output& output,
		bool expectSuccess = true)
	{
		RANDOM::BuyEntropy_input input{};
		input.collateralTier = collateralTier;
		input.numberOfBits = numberOfBits;
		return invokeUserProcedure(
			RANDOM_CONTRACT_INDEX,
			RANDOM_PROCEDURE_BUY_ENTROPY,
			input,
			output,
			user,
			static_cast<sint64>(reward),
			true,
			expectSuccess);
	}

};

class RandomBuyEntropyTest : public ContractTestingRANDOM, public ::testing::Test
{
protected:
	void SetUp() override
	{
		setTick(0);
	}

	void seedEntropyForCurrentTick(uint8 collateralTier, uint64 bitIndex)
	{
		const uint32 slot = entropySlot(getTick(), collateralTier);
		state()->setEntropySlot(slot, makeEntropyWithBitSet(bitIndex));
	}
};

TEST_F(RandomBuyEntropyTest, FeesReturnsOneHundredPerBitForTiersZeroThroughNine)
{
	const RANDOM::Fees_output output = fees();
	for (uint64 tier = 0; tier < 10; ++tier)
	{
		EXPECT_EQ(output.fees.get(tier), 100u);
	}
}

TEST_F(RandomBuyEntropyTest, RejectsInsufficientRewardWithoutReturningEntropy)
{
	seedEntropyForCurrentTick(0, 7);

	const sint64 balanceBefore = getBalance(BUYER);
	const uint64 earnedBefore = state()->getEarnedAmount();
	const uint64 reward = 64 * BIT_FEE - 1;

	RANDOM::BuyEntropy_output output{};
	EXPECT_TRUE(buyEntropy(BUYER, reward, 0, 64, output));

	EXPECT_EQ(getBalance(BUYER), balanceBefore - static_cast<sint64>(reward));
	EXPECT_EQ(state()->getEarnedAmount(), earnedBefore);
	EXPECT_TRUE(bit4096IsZero(output.entropy.get(0)));
}

TEST_F(RandomBuyEntropyTest, RejectsInvalidCollateralTierWithoutReturningEntropy)
{
	seedEntropyForCurrentTick(0, 3);

	const sint64 balanceBefore = getBalance(BUYER);
	const uint64 reward = 100 * BIT_FEE;
	RANDOM::BuyEntropy_output output{};
	EXPECT_TRUE(buyEntropy(BUYER, reward, 10, 100, output));

	EXPECT_EQ(getBalance(BUYER), balanceBefore - static_cast<sint64>(reward));
	EXPECT_TRUE(bit4096IsZero(output.entropy.get(0)));
}

TEST_F(RandomBuyEntropyTest, RejectsInvalidNumberOfBitsWithoutReturningEntropy)
{
	seedEntropyForCurrentTick(0, 3);

	const sint64 balanceBefore = getBalance(BUYER);
	RANDOM::BuyEntropy_output output{};

	EXPECT_TRUE(buyEntropy(BUYER, 0, 0, 0, output));
	EXPECT_EQ(getBalance(BUYER), balanceBefore);

	EXPECT_TRUE(buyEntropy(BUYER, 4097 * BIT_FEE, 0, 4097, output));
	EXPECT_EQ(getBalance(BUYER), balanceBefore - static_cast<sint64>(4097 * BIT_FEE));
}

TEST_F(RandomBuyEntropyTest, RefundsWhenEntropySlotIsEmpty)
{
	const uint32 slot = entropySlot(getTick(), 4);
	ASSERT_TRUE(bit4096IsZero(state()->getEntropySlot(slot)));

	const sint64 balanceBefore = getBalance(BUYER);
	const uint64 reward = 16 * BIT_FEE;

	RANDOM::BuyEntropy_output output{};
	EXPECT_TRUE(buyEntropy(BUYER, reward, 4, 16, output));

	EXPECT_EQ(getBalance(BUYER), balanceBefore);
	EXPECT_EQ(state()->getEarnedAmount(), 0u);
	EXPECT_TRUE(bit4096IsZero(output.entropy.get(0)));
}

TEST_F(RandomBuyEntropyTest, ReturnsEntropyFromStreamSelectedByTickPlusTwo)
{
	for (uint32 tick = 0; tick < 12; ++tick)
	{
		setTick(tick);
		for (uint8 tier = 0; tier < 10; ++tier)
		{
			const uint32 slot = entropySlot(tick, tier);
			state()->setEntropySlot(slot, makeEntropyWithBitSet(tick * 100 + tier));
		}
	}

	for (uint32 tick = 0; tick < 12; ++tick)
	{
		setTick(tick);
		const uint32 expectedSlot = entropySlot(tick, 3);
		const bit_4096& expectedEntropy = state()->getEntropySlot(expectedSlot);

		RANDOM::BuyEntropy_output output{};
		const uint64 reward = 8 * BIT_FEE;
		const sint64 balanceBefore = getBalance(BUYER);
		EXPECT_TRUE(buyEntropy(BUYER, reward, 3, 8, output));

		EXPECT_EQ(getBalance(BUYER), balanceBefore - static_cast<sint64>(reward));
		EXPECT_EQ(state()->getEarnedAmount(), reward);

		for (uint64 outputSlot = 0; outputSlot < 8; ++outputSlot)
		{
			EXPECT_EQ(output.entropy.get(outputSlot), expectedEntropy);
		}
		for (uint64 outputSlot = 8; outputSlot < output.entropy.capacity(); ++outputSlot)
		{
			EXPECT_TRUE(bit4096IsZero(output.entropy.get(outputSlot)));
		}

		state()->earnedAmount = 0;
		increaseEnergy(BUYER, reward);
	}
}

TEST_F(RandomBuyEntropyTest, EndTickClearsStreamAndBuyEntropyRefundsUntilReplenished)
{
	setTick(0);
	const uint32 stream = endTickStream(0);
	const bit_4096 reveal = makeEntropyWithBitSet(99);

	state()->setPopulation(stream, 1);
	state()->setProvider(stream, 0, PROVIDER);
	state()->setCollateralTier(stream, 0, 0);
	state()->setCommit(stream, 0, id(5, 6, 7, 8));
	state()->setReveal(stream, 0, reveal);
	state()->setRevealOrCommitFlag(stream, 0, true);
	endTick();

	const uint32 slot = stream * 10 + 0;
	ASSERT_FALSE(bit4096IsZero(state()->getEntropySlot(slot)));

	setTick(1);
	RANDOM::BuyEntropy_output firstBuy{};
	ASSERT_TRUE(buyEntropy(BUYER, BIT_FEE, 0, 1, firstBuy));
	EXPECT_TRUE(outputEntropySlotHasBit(firstBuy, 0, 99));
	increaseEnergy(BUYER, BIT_FEE);

	setTick(3);
	endTick();
	EXPECT_TRUE(bit4096IsZero(state()->getEntropySlot(slot)));

	setTick(4);
	const sint64 balanceBefore = getBalance(BUYER);
	RANDOM::BuyEntropy_output secondBuy{};
	EXPECT_TRUE(buyEntropy(BUYER, BIT_FEE, 0, 1, secondBuy, false));
	EXPECT_EQ(getBalance(BUYER), balanceBefore);
	EXPECT_TRUE(bit4096IsZero(secondBuy.entropy.get(0)));
}

TEST_F(RandomBuyEntropyTest, EndToEndFreshEntropyProducedByEndTick)
{
	// END_TICK at tick U writes stream U%3. BuyEntropy at tick T reads stream (T+2)%3.
	// For U=0 and T=1: (1+2)%3 == 0%3.
	setTick(0);
	const uint32 stream = endTickStream(0);
	const bit_4096 reveal = makeEntropyWithBitSet(42);

	state()->setPopulation(stream, 1);
	state()->setProvider(stream, 0, PROVIDER);
	state()->setCollateralTier(stream, 0, 0);
	state()->setCommit(stream, 0, id(1, 2, 3, 4));
	state()->setReveal(stream, 0, reveal);
	state()->setRevealOrCommitFlag(stream, 0, true);

	endTick();

	const uint32 writtenSlot = stream * 10 + 0;
	ASSERT_FALSE(bit4096IsZero(state()->getEntropySlot(writtenSlot)));

	setTick(1);
	ASSERT_EQ(buyEntropyStream(1), stream);

	RANDOM::BuyEntropy_output output{};
	const uint64 reward = 4 * BIT_FEE;
	const sint64 balanceBefore = getBalance(BUYER);
	EXPECT_TRUE(buyEntropy(BUYER, reward, 0, 4, output));

	EXPECT_EQ(getBalance(BUYER), balanceBefore - static_cast<sint64>(reward));
	EXPECT_EQ(state()->getEarnedAmount(), reward);
	EXPECT_TRUE(outputEntropySlotHasBit(output, 0, 42));
	for (uint64 outputSlot = 1; outputSlot < 4; ++outputSlot)
	{
		EXPECT_TRUE(outputEntropySlotHasBit(output, outputSlot, 42));
	}
}

TEST_F(RandomBuyEntropyTest, StaleStreamIsNotReadableAfterItsNextEndTickCycle)
{
	setTick(0);
	const uint32 stream = endTickStream(0);
	const bit_4096 reveal = makeEntropyWithBitSet(77);

	state()->setPopulation(stream, 1);
	state()->setProvider(stream, 0, PROVIDER);
	state()->setCollateralTier(stream, 0, 0);
	state()->setCommit(stream, 0, id(9, 8, 7, 6));
	state()->setReveal(stream, 0, reveal);
	state()->setRevealOrCommitFlag(stream, 0, true);
	endTick();

	setTick(1);
	RANDOM::BuyEntropy_output firstBuy{};
	ASSERT_TRUE(buyEntropy(BUYER, BIT_FEE, 0, 1, firstBuy));
	EXPECT_TRUE(outputEntropySlotHasBit(firstBuy, 0, 77));

	increaseEnergy(BUYER, BIT_FEE);

	// tick 3 END_TICK clears stream 0 again before repopulating.
	setTick(3);
	endTick();
	ASSERT_TRUE(bit4096IsZero(state()->getEntropySlot(endTickStream(0) * 10 + 0)));

	setTick(4);
	const sint64 balanceBefore = getBalance(BUYER);
	RANDOM::BuyEntropy_output secondBuy{};
	EXPECT_TRUE(buyEntropy(BUYER, BIT_FEE, 0, 1, secondBuy, false));
	EXPECT_EQ(getBalance(BUYER), balanceBefore);
	EXPECT_TRUE(bit4096IsZero(secondBuy.entropy.get(0)));
}
