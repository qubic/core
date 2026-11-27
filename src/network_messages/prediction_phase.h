#pragma once

#include "common_def.h"

// Phase of the epoch's prediction ranking. No payload, not signed.
struct RequestPredictionPhase
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_PREDICTION_PHASE;
    }
};

struct RespondPredictionPhase
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_PREDICTION_PHASE;
    }

    // 0: training. Else the tick the epoch switched in
    unsigned int switchTick;
    // First hour that counts, 0 in training
    unsigned int switchFirstHour;
    unsigned int computorsQualified;
    // Only a solution with at most this many errors in its frame answers; the first one qualifies the identity
    unsigned int threshold;
    // A solution answers an hour if it is executed at least this long before the hour
    unsigned int deadlineSeconds;
    // Digest of the prediction state at its last fold into the mining digest, 0 in training
    unsigned int foldedDigest;
};

static_assert(sizeof(RespondPredictionPhase) == 24, "RespondPredictionPhase unexpected size");

static constexpr unsigned int PREDICTION_IDENTITY_ANSWERS = 24 * 7;

// Prediction state of one identity. Not signed.
struct RequestPredictionIdentity
{
    m256i publicKey;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_PREDICTION_IDENTITY;
    }
};

static_assert(sizeof(RequestPredictionIdentity) == 32, "RequestPredictionIdentity unexpected size");

struct RespondPredictionIdentity
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_PREDICTION_IDENTITY;
    }

    // 0: the identity has not qualified. Else the first hour counted for it
    unsigned int firstHour;
    // Counted hours and how many of them were correct
    unsigned int hours;
    unsigned int correct;
    // The solution whose network answers now
    unsigned int solutionTick;
    unsigned int solutionIndexInTick;
    // answers[i] is the answer for hour baseHour + 1 + i
    unsigned int baseHour;
    // 0 = down, 1 = up, 2 = none
    unsigned char answers[PREDICTION_IDENTITY_ANSWERS];
};

static_assert(sizeof(RespondPredictionIdentity) == 192, "RespondPredictionIdentity unexpected size");
