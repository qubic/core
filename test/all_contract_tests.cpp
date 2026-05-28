// This file bundles all contract tests into one compilation unit
// to ensure the huge include chain from "contract_testing.h" is only compiled once.
#define NO_UEFI

// Add all stdlib includes for contract tests here (keep sorted alphabetically).
#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <thread>
#include <vector>

// Include of "contract_testing.h" goes last because it redefines 'system'.
#include "contract_testing.h"

// Include all qubic-specific header files at the top of your contract test file.

// Include all contract test files below.
#include "contract_tests/contract_ccf.cpp"
#include "contract_tests/contract_core.cpp"
#include "contract_tests/contract_escrow.cpp"
#include "contract_tests/contract_gqmprop.cpp"
#include "contract_tests/contract_msvault.cpp"
#include "contract_tests/contract_nostromo.cpp"
#include "contract_tests/contract_pulse.cpp"
#include "contract_tests/contract_qbay.cpp"
#include "contract_tests/contract_qbond.cpp"
#include "contract_tests/contract_qduel.cpp"
#include "contract_tests/contract_qearn.cpp"
#include "contract_tests/contract_qip.cpp"
#include "contract_tests/contract_qraffle.cpp"
#include "contract_tests/contract_qrp.cpp"
#include "contract_tests/contract_qrwa.cpp"
#include "contract_tests/contract_qswap.cpp"
#include "contract_tests/contract_qtf.cpp"
#include "contract_tests/contract_quottery.cpp"
#include "contract_tests/contract_qusino.cpp"
#include "contract_tests/contract_qutil.cpp"
#include "contract_tests/contract_qvault.cpp"
#include "contract_tests/contract_qx.cpp"
#include "contract_tests/contract_rl.cpp"
#include "contract_tests/contract_testex.cpp"
#include "contract_tests/contract_vottunbridge.cpp"