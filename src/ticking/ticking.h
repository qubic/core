#pragma once

#include "platform/global_var.h"
#include "platform/m256.h"

#include "network_messages/tick.h"

#include "ticking/tick_storage.h"
#include "ticking/pending_txs_pool.h"

#include "private_settings.h"

GLOBAL_VAR_DECL Tick etalonTick;
GLOBAL_VAR_DECL int numberTickTransactions GLOBAL_VAR_INIT(-1);
