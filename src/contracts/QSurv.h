using namespace QPI;

// ============================================
// QSurv - Trustless Survey Platform
// Decentralized survey creation with escrow and AI-verified payouts
// ============================================

// Forward declaration for state expansion
struct QSURV2 {};

struct QSURV : public ContractBase {
  // ============================================
  // CONSTANTS
  // ============================================
  static constexpr uint64 PLATFORM_FEE_PERCENT = 5;
  static constexpr uint64 REFERRAL_REWARD_PERCENT = 25;
  static constexpr uint64 BASE_REWARD_PERCENT = 60;
  static constexpr uint32 MAX_SURVEYS = 1024;
  static constexpr uint32 IPFS_HASH_SIZE = 64;

  // ============================================
  // STRUCTS
  // ============================================
  struct Survey {
    uint64 surveyId;
    id creator;
    uint64 rewardAmount;
    uint64 rewardPerRespondent;
    uint32 maxRespondents;
    uint32 currentRespondents;
    uint64 balance;
    Array<uint8, 64> ipfsHash;
    bit isActive;
  };

  // ============================================
  // STATE (Persistent Storage)
  // ============================================
protected:
  Array<Survey, MAX_SURVEYS> _surveys;
  uint32 _surveyCount;
  id _oracleAddress;

  // ============================================
  // SYSTEM PROCEDURES
  // ============================================
public:
  INITIALIZE() {
    // State is automatically reset before INITIALIZE() is called
    // No initialization needed - state fields start at zero
  }

  BEGIN_EPOCH() {
    // Called at the beginning of each epoch
    // Can be used for cleanup, stats, or balance updates
  }

  END_EPOCH() {
    // Called at the end of each epoch
  }

  BEGIN_TICK() {
    // Called before processing transactions in a tick
  }

  END_TICK() {
    // Called after processing transactions in a tick
  }

  // ============================================
  // INPUT/OUTPUT STRUCTS
  // ============================================

  // --- CreateSurvey ---
  struct createSurvey_input {
    uint64 rewardPool;
    uint32 maxRespondents;
    Array<uint8, 64> ipfsHash;
  };

  struct createSurvey_output {
    uint64 surveyId;
    bit success;
  };

  struct createSurvey_locals {
    uint32 i;
    Survey tempSurvey;
  };

  // --- Payout ---
  struct payout_input {
    uint64 surveyId;
    id respondentAddress;
    id referrerAddress;
    uint8 respondentTier;
  };

  struct payout_output {
    uint64 amountPaid;
    uint64 bonusPaid;
    uint64 referralPaid;
    bit success;
  };

  struct payout_locals {
    uint32 index;
    bit found;
    uint64 totalReward;
    uint64 baseReward;
    uint64 referralReward;
    uint64 platformFee;
    uint64 bonus;
    uint64 totalSpent;
    uint32 i;
    Survey tempSurvey;
  };

  // --- GetSurvey (Read-only) ---
  struct getSurvey_input {
    uint64 surveyId;
  };

  struct getSurvey_output {
    uint64 surveyId;
    id creator;
    uint64 rewardAmount;
    uint64 rewardPerRespondent;
    uint32 maxRespondents;
    uint32 currentRespondents;
    uint64 balance;
    bit isActive;
    bit found;
  };

  struct getSurvey_locals {
    uint32 i;
  };

  // --- GetSurveyCount (Read-only) ---
  struct getSurveyCount_input {};

  struct getSurveyCount_output {
    uint32 count;
  };

  // --- AbortSurvey (Creator only) ---
  struct abortSurvey_input {
    uint64 surveyId;
  };

  struct abortSurvey_output {
    bit success;
  };

  struct abortSurvey_locals {
    uint32 i;
    uint32 index;
    bit found;
    Survey tempSurvey;
  };

  // --- SetOracle (Admin) ---
  struct setOracle_input {
    id newOracleAddress;
  };

  struct setOracle_output {
    bit success;
  };

  // ============================================
  // USER PROCEDURES (State-Modifying)
  // ============================================

  PUBLIC_PROCEDURE_WITH_LOCALS(createSurvey) {
    // Validation checks
    if (state._surveyCount >= MAX_SURVEYS) {
      return;
    }
    if (input.maxRespondents == 0) {
      return;
    }
    if (input.rewardPool == 0) {
      return;
    }

    // Verify invocation reward matches rewardPool
    if ((uint64)qpi.invocationReward() < input.rewardPool) {
      return;
    }

    // Create new survey - access state directly via copy pattern
    locals.tempSurvey.surveyId = state._surveyCount + 1;
    locals.tempSurvey.creator = qpi.invocator();
    locals.tempSurvey.rewardAmount = input.rewardPool;
    locals.tempSurvey.maxRespondents = input.maxRespondents;
    locals.tempSurvey.rewardPerRespondent =
        QPI::div(input.rewardPool, (uint64)input.maxRespondents);
    locals.tempSurvey.balance = input.rewardPool;
    locals.tempSurvey.isActive = 1;

    // Copy IPFS hash using Array's set method
    for (locals.i = 0; locals.i < IPFS_HASH_SIZE; locals.i++) {
      locals.tempSurvey.ipfsHash.set(locals.i, input.ipfsHash.get(locals.i));
    }

    // Commit state changes
    state._surveys.set(state._surveyCount, locals.tempSurvey);

    output.surveyId = state._surveyCount + 1;
    output.success = 1;
    state._surveyCount++;
  }

  PUBLIC_PROCEDURE_WITH_LOCALS(payout) {
    // Security: Oracle-only execution
    if (qpi.invocator() != state._oracleAddress) {
      return;
    }

    // Find survey by ID using found flag pattern
    for (locals.i = 0; locals.i < state._surveyCount; locals.i++) {
      if (state._surveys.get(locals.i).surveyId == input.surveyId) {
        locals.index = locals.i;
        locals.found = 1;
        break;
      }
    }

    if (!locals.found) {
      return;
    }

    // Copy state to local variable for reading and modification
    locals.tempSurvey = state._surveys.get(locals.index);

    // Validation checks
    if (!locals.tempSurvey.isActive) {
      return;
    }
    if (locals.tempSurvey.currentRespondents >=
        locals.tempSurvey.maxRespondents) {
      return;
    }
    if (locals.tempSurvey.balance < locals.tempSurvey.rewardPerRespondent) {
      return;
    }

    // Calculate reward splits using QPI::div (no / operator allowed)
    locals.totalReward = locals.tempSurvey.rewardPerRespondent;

    // Minimum guaranteed 40%, fixed referral 20%
    locals.baseReward = QPI::div(locals.totalReward * 40, 100ULL);
    locals.referralReward = QPI::div(locals.totalReward * 20, 100ULL);

    // Staking bonus tier system and fair oracle incentivization
    if (input.respondentTier == 1) {
      locals.bonus = QPI::div(locals.totalReward * 10, 100ULL);      // 10%
      locals.platformFee = QPI::div(locals.totalReward * 3, 100ULL); // 3%
    } else if (input.respondentTier == 2) {
      locals.bonus = QPI::div(locals.totalReward * 20, 100ULL);      // 20%
      locals.platformFee = QPI::div(locals.totalReward * 5, 100ULL); // 5%
    } else if (input.respondentTier == 3) {
      locals.bonus = QPI::div(locals.totalReward * 30, 100ULL);       // 30%
      locals.platformFee = QPI::div(locals.totalReward * 10, 100ULL); // 10%
    } else {                                                          // Tier 0
      locals.bonus = 0;
      locals.platformFee = QPI::div(locals.totalReward * 1, 100ULL); // 1%
    }

    locals.totalSpent = locals.baseReward + locals.bonus +
                        locals.referralReward + locals.platformFee;

    // Execute fund transfers
    qpi.transfer(input.respondentAddress, locals.baseReward + locals.bonus);

    if (input.referrerAddress != NULL_ID) {
      qpi.transfer(input.referrerAddress, locals.referralReward);
    } else {
      qpi.transfer(state._oracleAddress, locals.referralReward);
    }

    qpi.transfer(state._oracleAddress, locals.platformFee);

    // Update state in local variable. Only deduct what was ACTUALLY spent!
    locals.tempSurvey.balance = locals.tempSurvey.balance - locals.totalSpent;
    locals.tempSurvey.currentRespondents++;

    if (locals.tempSurvey.currentRespondents >=
        locals.tempSurvey.maxRespondents) {
      locals.tempSurvey.isActive = 0;
    }

    // Commit modifications back to state
    state._surveys.set(locals.index, locals.tempSurvey);

    output.success = 1;
    output.amountPaid = locals.baseReward;
    output.bonusPaid = locals.bonus;
    output.referralPaid = locals.referralReward;
  }

  PUBLIC_PROCEDURE_WITH_LOCALS(abortSurvey) {
    for (locals.i = 0; locals.i < state._surveyCount; locals.i++) {
      if (state._surveys.get(locals.i).surveyId == input.surveyId) {
        locals.index = locals.i;
        locals.found = 1;
        break;
      }
    }

    if (!locals.found) {
      return;
    }

    locals.tempSurvey = state._surveys.get(locals.index);

    // Only creator can abort
    if (qpi.invocator() != locals.tempSurvey.creator) {
      return;
    }

    // Must be active
    if (!locals.tempSurvey.isActive) {
      return;
    }

    // Refund remaining balance to creator
    if (locals.tempSurvey.balance > 0) {
      qpi.transfer(locals.tempSurvey.creator, locals.tempSurvey.balance);
    }

    // Zero out completely to reclaim slot safely
    locals.tempSurvey.isActive = 0;
    locals.tempSurvey.balance = 0;
    locals.tempSurvey.currentRespondents = locals.tempSurvey.maxRespondents;

    state._surveys.set(locals.index, locals.tempSurvey);
    output.success = 1;
  }

  PUBLIC_PROCEDURE(setOracle) {
    // Only allow setting oracle if not already set, or by current oracle
    if (state._oracleAddress == NULL_ID ||
        qpi.invocator() == state._oracleAddress) {
      state._oracleAddress = input.newOracleAddress;
      output.success = 1;
    }
  }

  // ============================================
  // USER FUNCTIONS (Read-Only)
  // ============================================

  PUBLIC_FUNCTION_WITH_LOCALS(getSurvey) {
    for (locals.i = 0; locals.i < state._surveyCount; locals.i++) {
      // No need to copy for read-only access, direct get() is fine
      if (state._surveys.get(locals.i).surveyId == input.surveyId) {
        output.surveyId = state._surveys.get(locals.i).surveyId;
        output.creator = state._surveys.get(locals.i).creator;
        output.rewardAmount = state._surveys.get(locals.i).rewardAmount;
        output.rewardPerRespondent =
            state._surveys.get(locals.i).rewardPerRespondent;
        output.maxRespondents = state._surveys.get(locals.i).maxRespondents;
        output.currentRespondents =
            state._surveys.get(locals.i).currentRespondents;
        output.balance = state._surveys.get(locals.i).balance;
        output.isActive = state._surveys.get(locals.i).isActive;
        output.found = 1;
        return;
      }
    }
  }

  PUBLIC_FUNCTION(getSurveyCount) { output.count = state._surveyCount; }

  // ============================================
  // REGISTER USER FUNCTIONS AND PROCEDURES
  // ============================================
  REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
    // Functions (Read-only queries)
    REGISTER_USER_FUNCTION(getSurvey, 1);
    REGISTER_USER_FUNCTION(getSurveyCount, 2);

    // Procedures (State-modifying)
    REGISTER_USER_PROCEDURE(createSurvey, 1);
    REGISTER_USER_PROCEDURE(payout, 2);
    REGISTER_USER_PROCEDURE(setOracle, 3);
    REGISTER_USER_PROCEDURE(abortSurvey, 4);
  }
};
