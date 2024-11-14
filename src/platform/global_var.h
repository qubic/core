#pragma once

// If multiple compile units are used, having all globals declared as static causes
// trouble if code is shared between compile units. Thus, globals need to be declared
// extern and defined in only one compile unit.

#if defined(SINGLE_COMPILE_UNIT)

// single compile unit -> make static
#define GLOBAL_VAR_DECL static
#define GLOBAL_VAR_INIT(val) =val

#else

// multiple compile units -> only define in one file (declare as extern in the others)
#if defined(DEFINE_VARIABLES_SHARED_BETWEEN_COMPILE_UNITS)
#define GLOBAL_VAR_DECL
#define GLOBAL_VAR_INIT(val) =val
#else
#define GLOBAL_VAR_DECL extern
#define GLOBAL_VAR_INIT(val)
#endif

#endif
