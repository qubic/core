#define NO_UEFI
#include "contract_testing.h"

constexpr uint16 PROCEDURE_INDEX_CREATE_SURVEY = 1;
constexpr uint16 PROCEDURE_INDEX_PAYOUT = 2;
constexpr uint16 PROCEDURE_INDEX_SET_ORACLE = 3;
constexpr uint16 PROCEDURE_INDEX_ABORT_SURVEY = 4;

constexpr uint16 FUNCTION_INDEX_GET_SURVEY = 1;
constexpr uint16 FUNCTION_INDEX_GET_SURVEY_COUNT = 2;

class ContractTestingQSurv : public ContractTesting {
public:
  ContractTestingQSurv() {
    initEmptySpectrum();
    initEmptyUniverse();
    INIT_CONTRACT(QSURV);
    callSystemProcedure(QSURV_CONTRACT_INDEX, INITIALIZE);
  }

  QSURV::createSurvey_output createSurvey(const id &user, uint64 rewardPool,
                                          uint32 maxRespondents,
                                          const QPI::Array<uint8, 64> &ipfsHash,
                                          uint64 attachedAmount) {
    QSURV::createSurvey_input input;
    input.rewardPool = rewardPool;
    input.maxRespondents = maxRespondents;
    for (int i = 0; i < 64; i++) {
      input.ipfsHash.set(i, ipfsHash.get(i));
    }

    QSURV::createSurvey_output output;
    output.success = 0;
    invokeUserProcedure(QSURV_CONTRACT_INDEX, PROCEDURE_INDEX_CREATE_SURVEY,
                        input, output, user, attachedAmount);
    return output;
  }

  QSURV::payout_output payout(const id &user, uint64 surveyId,
                              const id &respondent, const id &referrer,
                              uint8 tier) {
    QSURV::payout_input input;
    input.surveyId = surveyId;
    input.respondentAddress = respondent;
    input.referrerAddress = referrer;
    input.respondentTier = tier;

    QSURV::payout_output output;
    output.success = 0;
    invokeUserProcedure(QSURV_CONTRACT_INDEX, PROCEDURE_INDEX_PAYOUT, input,
                        output, user, 0);
    return output;
  }

  QSURV::setOracle_output setOracle(const id &user, const id &newOracle) {
    QSURV::setOracle_input input;
    input.newOracleAddress = newOracle;

    QSURV::setOracle_output output;
    output.success = 0;
    invokeUserProcedure(QSURV_CONTRACT_INDEX, PROCEDURE_INDEX_SET_ORACLE, input,
                        output, user, 0);
    return output;
  }

  QSURV::abortSurvey_output abortSurvey(const id &user, uint64 surveyId) {
    QSURV::abortSurvey_input input;
    input.surveyId = surveyId;

    QSURV::abortSurvey_output output;
    output.success = 0;
    invokeUserProcedure(QSURV_CONTRACT_INDEX, PROCEDURE_INDEX_ABORT_SURVEY,
                        input, output, user, 0);
    return output;
  }

  QSURV::getSurvey_output getSurvey(uint64 surveyId) {
    QSURV::getSurvey_input input;
    input.surveyId = surveyId;
    QSURV::getSurvey_output output;
    output.found = 0;
    callFunction(QSURV_CONTRACT_INDEX, FUNCTION_INDEX_GET_SURVEY, input,
                 output);
    return output;
  }

  QSURV::getSurveyCount_output getSurveyCount() {
    QSURV::getSurveyCount_input input;
    QSURV::getSurveyCount_output output;
    output.count = 0;
    callFunction(QSURV_CONTRACT_INDEX, FUNCTION_INDEX_GET_SURVEY_COUNT, input,
                 output);
    return output;
  }
};

TEST(ContractQSurv, CreateSurvey_Success) {
  ContractTestingQSurv qsurv;
  const id creator(1, 0, 0, 0);
  const uint64 rewardPool = 1000;
  const uint32 maxRespondents = 10;

  increaseEnergy(creator, rewardPool + 1000);

  QPI::Array<uint8, 64> hash;
  for (int i = 0; i < 64; i++) {
    hash.set(i, 1);
  }

  auto output =
      qsurv.createSurvey(creator, rewardPool, maxRespondents, hash, rewardPool);

  EXPECT_EQ(output.success, 1);
  EXPECT_EQ(output.surveyId, 1);

  auto countOut = qsurv.getSurveyCount();
  EXPECT_EQ(countOut.count, 1);

  auto surveyOut = qsurv.getSurvey(1);
  EXPECT_EQ(surveyOut.found, 1);
  EXPECT_EQ(surveyOut.rewardAmount, rewardPool);
  EXPECT_EQ(surveyOut.creator, creator);
}

TEST(ContractQSurv, CreateSurvey_Fail_ZeroRespondentsOrPool) {
  ContractTestingQSurv qsurv;
  const id creator(1, 0, 0, 0);
  increaseEnergy(creator, 5000);

  QPI::Array<uint8, 64> hash;
  for (int i = 0; i < 64; i++) {
    hash.set(i, 1);
  }

  // Pool is 0
  auto out1 = qsurv.createSurvey(creator, 0, 10, hash, 0);
  EXPECT_EQ(out1.success, 0);

  // Respondents is 0
  auto out2 = qsurv.createSurvey(creator, 1000, 0, hash, 1000);
  EXPECT_EQ(out2.success, 0);
}

TEST(ContractQSurv, SetOracle_Security) {
  ContractTestingQSurv qsurv;
  const id system_invocator(0, 0, 0, 0);
  const id oracle(999, 0, 0, 0);
  const id hacker(888, 0, 0, 0);

  // setOracle initially by the creator/system
  qsurv.setOracle(system_invocator, oracle);

  // Hacker tries to steal
  auto setOut2 = qsurv.setOracle(hacker, hacker);
  EXPECT_EQ(setOut2.success, 0);
}

TEST(ContractQSurv, Payout_VerifyBalancesAndCompletion) {
  ContractTestingQSurv qsurv;
  const id system_invocator(0, 0, 0, 0);
  const id creator(1, 0, 0, 0);
  const id oracle(999, 0, 0, 0);
  const id respondent(2, 0, 0, 0);
  const id referrer(3, 0, 0, 0);

  increaseEnergy(creator, 10000);

  // Set oracle
  qsurv.setOracle(system_invocator, oracle);

  QPI::Array<uint8, 64> hash;
  for (int i = 0; i < 64; i++) {
    hash.set(i, 1);
  }

  uint64 rewardPool = 1000;
  uint32 maxResp = 1; // Only 1 to test completion logic easily

  qsurv.createSurvey(creator, rewardPool, maxResp, hash, rewardPool);

  auto surveyBefore = qsurv.getSurvey(1);
  EXPECT_EQ(surveyBefore.balance, 1000);
  EXPECT_EQ(surveyBefore.isActive, 1);

  uint64 respondentBalBefore = getBalance(respondent);
  uint64 referrerBalBefore = getBalance(referrer);
  uint64 oracleBalBefore = getBalance(oracle);

  // Payout with Tier 1 (10% bonus)
  // Reward per resp = 1000
  // Base (40%) = 400, Referral (20%) = 200, Platform (3%) = 30, Bonus (10%) =
  // 100. Total Spent = 730. Remaining balance = 270.
  auto payoutOut = qsurv.payout(oracle, 1, respondent, referrer, 1);
  EXPECT_EQ(payoutOut.success, 1);

  uint64 respondentBalAfter = getBalance(respondent);
  uint64 referrerBalAfter = getBalance(referrer);
  uint64 oracleBalAfter = getBalance(oracle);

  EXPECT_EQ(respondentBalAfter - respondentBalBefore,
            500);                                       // 400 base + 100 bonus
  EXPECT_EQ(referrerBalAfter - referrerBalBefore, 200); // 200 referral
  EXPECT_EQ(oracleBalAfter - oracleBalBefore, 30); // 30 platform fee for Tier 1

  auto surveyAfter = qsurv.getSurvey(1);
  EXPECT_EQ(surveyAfter.balance, 270); // 1000 - 730 spent
  EXPECT_EQ(surveyAfter.isActive,
            0); // Marked inactive since max respondents reached

  // Test over-payout fail
  auto overPayoutOut = qsurv.payout(oracle, 1, respondent, referrer, 1);
  EXPECT_EQ(overPayoutOut.success,
            0); // Should fail since inactive and respondents full
}

TEST(ContractQSurv, AbortSurvey_RefundAndReset) {
  ContractTestingQSurv qsurv;
  const id creator(1, 0, 0, 0);
  const id hacker(888, 0, 0, 0);

  increaseEnergy(creator, 5000);

  QPI::Array<uint8, 64> hash;
  for (int i = 0; i < 64; i++) {
    hash.set(i, 1);
  }

  uint64 rewardPool = 2000;
  qsurv.createSurvey(creator, rewardPool, 2, hash, rewardPool);

  auto surveyBefore = qsurv.getSurvey(1);
  EXPECT_EQ(surveyBefore.balance, 2000);
  EXPECT_EQ(surveyBefore.isActive, 1);

  uint64 creatorBalBefore = getBalance(creator);

  // Hacker tries to abort
  auto abortHacker = qsurv.abortSurvey(hacker, 1);
  EXPECT_EQ(abortHacker.success, 0);

  // Creator successfully aborts
  auto abortCreator = qsurv.abortSurvey(creator, 1);
  EXPECT_EQ(abortCreator.success, 1);

  uint64 creatorBalAfter = getBalance(creator);
  EXPECT_EQ(creatorBalAfter - creatorBalBefore,
            2000); // Reclaimed the unspent balance!

  // Survey slot should be entirely reset to allow reclaiming
  auto surveyAfter = qsurv.getSurvey(1);
  EXPECT_EQ(surveyAfter.isActive, 0);
  EXPECT_EQ(surveyAfter.balance, 0);
  EXPECT_EQ(surveyAfter.currentRespondents, surveyAfter.maxRespondents);
}
