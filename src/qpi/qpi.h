// Qubic Programming Interface 1.0.0

#pragma once

#include "qpi_types.h"

namespace QPI
{
	/*

	Prohibited character combinations in contracts:

	"
	#
	%
	'
	* (not prohibited as multiplication operator)
	...
	/ as division operator
	:: (not prohibited as scope operator for structs, enums, and namespaces defined in contracts and `qpi.h`)
	[
	]
	__
	double
	float
	typedef
	union

	const_cast
	QpiContext

	*/

	// Copy memory of src to dst. Both may have different types, but size of both must match exactly.
	template <typename T1, typename T2>
	inline void copyMemory(T1& dst, const T2& src);

	// Copy object src into buffer dst. The size of the dst buffer must be grater or equal to the size of src object.
	// If dst size is greater than src size and setTailToZero is true, set the part of dst to zero that follows
	// behind the copy of src.
	template <typename T1, typename T2>
	inline void copyToBuffer(T1& dst, const T2& src, bool setTailToZero = false);

	// Set object dst from buffer src. The size of the src buffer must be grater or equal to the size of dst object.
	template <typename T1, typename T2>
	inline void copyFromBuffer(T1& dst, const T2& src);

	// Set all memory of dst to byte value.
	template <typename T>
	inline void setMemory(T& dst, uint8 value);

	// Divide a by b, but return 0 if b is 0 (rounding to lower magnitude in case of integers)
	template <typename T>
	inline static constexpr T div(T a, T b)
	{
		return b ? (a / b) : T(0);
	}

	// Return remainder of dividing a by b, but return 0 if b is 0 (requires modulo % operator)
	template <typename T>
	inline static constexpr T mod(T a, T b)
	{
		return b ? (a % b) : 0;
	}
	
	//////////
	// safety multiplying a and b and then clamp

	inline static sint64 smul(sint64 a, sint64 b);
	inline static uint64 smul(uint64 a, uint64 b);
	inline static sint32 smul(sint32 a, sint32 b);
	inline static uint32 smul(uint32 a, uint32 b);

	//////////
	// safety adding a and b and then clamp

	inline static sint64 sadd(sint64 a, sint64 b);
	inline static uint64 sadd(uint64 a, uint64 b);
	inline static sint32 sadd(sint32 a, sint32 b);
	inline static uint32 sadd(uint32 a, uint32 b);
}

#include "qpi_assets.h"
#include "qpi_containers.h"
#include "qpi_context.h"
#include "qpi_date_time.h"
#include "qpi_macros.h"
#include "qpi_proposals.h"
