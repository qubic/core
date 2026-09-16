#pragma once

#include "qpi/qpi.h"
#include "system.h"

unsigned short QPI::QpiContextFunctionCall::epoch() const
{
    return system.epoch;
}

unsigned int QPI::QpiContextFunctionCall::tick() const
{
    return system.tick;
}

unsigned int QPI::QpiContextFunctionCall::initialTick() const
{
	return system.initialTick;
}
