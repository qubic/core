#pragma once

#include "contracts/qpi.h"

#include "ticking/ticking.h"

unsigned char QPI::QpiContextFunctionCall::day() const
{
    return etalonTick.day;
}

unsigned char QPI::QpiContextFunctionCall::year() const
{
    return etalonTick.year;
}

unsigned char QPI::QpiContextFunctionCall::hour() const
{
    return etalonTick.hour;
}

unsigned short QPI::QpiContextFunctionCall::millisecond() const
{
    return etalonTick.millisecond;
}

unsigned char QPI::QpiContextFunctionCall::minute() const
{
    return etalonTick.minute;
}

unsigned char QPI::QpiContextFunctionCall::month() const
{
    return etalonTick.month;
}

int QPI::QpiContextFunctionCall::numberOfTickTransactions() const
{
    return numberTickTransactions;
}

unsigned char QPI::QpiContextFunctionCall::second() const
{
    return etalonTick.second;
}

QPI::DateAndTime QPI::QpiContextFunctionCall::now() const
{
    QPI::DateAndTime result;
    result.year = etalonTick.year;
    result.month = etalonTick.month;
    result.day = etalonTick.day;
    result.hour = etalonTick.hour;
    result.minute = etalonTick.minute;
    result.second = etalonTick.second;
    result.millisecond = etalonTick.millisecond;
    return result;
}

m256i QPI::QpiContextFunctionCall::getPrevSpectrumDigest() const
{
    return etalonTick.prevSpectrumDigest;
}

m256i QPI::QpiContextFunctionCall::getPrevUniverseDigest() const
{
    return etalonTick.prevUniverseDigest;
}

m256i QPI::QpiContextFunctionCall::getPrevComputerDigest() const
{
    return etalonTick.prevComputerDigest;
}