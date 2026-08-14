#pragma once

#include "qpi_types.h"

// ASSERT can be used to support debugging and speed-up development
#include "../platform/assert.h"

namespace QPI
{
	/**
	 * Date and time (up to microsecond precision, year range from 0 to 65535, 8-byte representation)
	 *
	 * May contain invalid dates. Follows Gregorian calendar, implementing leap years but no leap seconds.
	 */
	struct DateAndTime
	{
		/// Return instance with current time (defined in qpi_ticking_impl.h)
		static inline DateAndTime now();

		/// Init with value 0 (no valid date).
		DateAndTime()
		{
			value = 0;
		}

		/// Init object with date/time. See `set()` for info about parameters.
		DateAndTime(uint64 year, uint64 month, uint64 day, uint64 hour = 0, uint64 minute = 0, uint64 second = 0, uint64 millisec = 0, uint64 microsecDuringMillisec = 0)
		{
			set(year, month, day, hour, minute, second, millisec, microsecDuringMillisec);
		}

		/// Copy object
		DateAndTime(const DateAndTime& other)
		{
			value = other.value;
		}

		/// Assign object
		DateAndTime& operator = (const DateAndTime& other)
		{
			value = other.value;
			return *this;
		}

		/**
		* @brief Set date and time value without checking if it is valid.
		* @param year	Year of the date (without offset). Should be in range 0 to 65335.
		* @param month	Month of the date. Should be in range 1 to 12 to be valid.
		* @param day	Day of the month. Should be in range 1 to 31/30/29/28 to be valid, depending on year and month.
		* @param hour	Hour during the day. Should be in range 0 to 23 to be valid.
		* @param minute	Minute during the hour. Should be in range 0 to 59 to be valid.
		* @param second	Second during the minute. Should be in range 0 to 59 to be valid.
		* @param millisec Millisecond during the second. Should be in range 0 to 999 to be valid.
		* @param microsecDuringMillisec Microsecond during the millisecond. Should be in range 0 to 999 to be valid.
		*/
		inline void set(uint64 year, uint64 month, uint64 day, uint64 hour, uint64 minute, uint64 second, uint64 millisec = 0, uint64 microsecDuringMillisec = 0)
		{
			value = (year << 46) | (month << 42) | (day << 37)
				| (hour << 32) | (minute << 26) | (second << 20)
				| (millisec << 10) | (microsecDuringMillisec);
		}

		/// Set date/time if valid (returns true). Otherwise returns false.
		bool setIfValid(uint64 year, uint64 month, uint64 day, uint64 hour, uint64 minute, uint64 second, uint64 millisec = 0, uint64 microsecDuringMillisec = 0)
		{
			if (!isValid(year, month, day, hour, minute, second, millisec, microsecDuringMillisec))
				return false;
			set(year, month, day, hour, minute, second, millisec, microsecDuringMillisec);
			return true;
		}

		/// Set date/time to invalid value. Depending on the desired behavior of comparison, you may chose a value less or greater than all valid values.
		inline void setInvalid(bool smallestValue = true)
		{
			value = (smallestValue) ? 0 : UINT64_MAX;
		}

		/**
		* @brief Set date value without checking if it is valid.
		* @param year	Year of the date (without offset). Should be in range 0 to 65335.
		* @param month	Month of the date. Should be in range 1 to 12 to be valid.
		* @param day	Day of the month. Should be in range 1 to 31/30/29/28 to be valid, depending on year and month.
		*/
		inline void setDate(uint64 year, uint64 month, uint64 day)
		{
			// clear bits of current date (only keep 37 bits of time) before setting new date
			value &= 0x1fffffffff;
			value |= (year << 46) | (month << 42) | (day << 37);
		}

		/**
		* @brief Set time without checking if it is valid.
		* @param hour	Hour during the day. Should be in range 0 to 23 to be valid.
		* @param minute	Minute during the hour. Should be in range 0 to 59 to be valid.
		* @param second	Second during the minute. Should be in range 0 to 59 to be valid.
		* @param millisec Millisecond during the second. Should be in range 0 to 999 to be valid.
		* @param microsecDuringMillisec Microsecond during the millisecond. Should be in range 0 to 999 to be valid.
		*/
		inline void setTime(uint64 hour, uint64 minute, uint64 second, uint64 millisec = 0, uint64 microsecDuringMillisec = 0)
		{
			// clear bits of current time (only keep 25 bits of date) before setting new time
			value &= (0x1ffffffllu << 37llu);
			value |= (hour << 32) | (minute << 26) | (second << 20)
				| (millisec << 10) | (microsecDuringMillisec);
		}

		/// Return year of date/time (range 0 to 65535).
		uint16 getYear() const
		{
			return static_cast<uint16>(value >> 46);
		}

		/// Return month of date/time (range 1 to 12 if valid).
		uint8 getMonth() const
		{
			return static_cast<uint8>(value >> 42) & 0b1111;
		}

		/// Return month of date/time (range 1 to 31/30/29/28 depending on month if valid).
		uint8 getDay() const
		{
			return static_cast<uint8>(value >> 37) & 0b11111;
		}

		/// Return hour of date/time (range 0 to 23 if valid).
		uint8 getHour() const
		{
			return static_cast<uint8>(value >> 32) & 0b11111;
		}

		/// Return minute of date/time (range 0 to 59 if valid).
		uint8 getMinute() const
		{
			return static_cast<uint8>(value >> 26) & 0b111111;
		}

		/// Return second of date/time (range 0 to 59 if valid).
		uint8 getSecond() const
		{
			return static_cast<uint8>(value >> 20) & 0b111111;
		}

		/// Return millisecond of date/time (range 0 to 999 if valid).
		uint16 getMillisec() const
		{
			return static_cast<uint16>(value >> 10) & 0b1111111111;
		}

		/// Return microsecond in current millisecond of date/time (range 0 to 999 if valid).
		uint16 getMicrosecDuringMillisec() const
		{
			return static_cast<uint16>(value) & 0b1111111111;
		}

		/// Check if this instance contains a valid date and time.
		bool isValid() const
		{
			if (!value)
				return false;
			return isValid(getYear(), getMonth(), getDay(), getHour(), getMinute(), getSecond(), getMillisec(), getMicrosecDuringMillisec());
		}

		/// Check if a year is a leap year.
		static bool isLeapYear(uint64 year)
		{
			if (year % 4 != 0)
				return false;
			if (year % 100 == 0)
			{
				if (year % 400 == 0)
					return true;
				else
					return false;
			}
			return true;
		}

		/// Return the number of days in a month of a specific year.
		static uint8 daysInMonth(uint64 year, uint64 month)
		{
			const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			if (month < 1 || month > 12)
				return 0;
			if (month == 2 && DateAndTime::isLeapYear(year))
				return 29;
			else
				return daysInMonth[month];
		};

		/// Check if the date and time given by parameters is valid.
		static inline bool isValid(uint64 year, uint64 month, uint64 day, uint64 hour, uint64 minute, uint64 second, uint64 millisec, uint64 microsecDuringMillisec)
		{
			if (year > 0xffffu)
				return false;
			if (month > 12 || month == 0)
				return false;
			if (day > 31 || day == 0)
				return false;
			if ((day == 31) && (month != 1) && (month != 3) && (month != 5) && (month != 7) && (month != 8) && (month != 10) && (month != 12))
				return false;
			if ((day == 30) && (month == 2))
				return false;
			if ((day == 29) && (month == 2) && !isLeapYear(year))
				return false;
			if (hour >= 24)
				return false;
			if (minute >= 60)
				return false;
			if (second >= 60)
				return false;
			if (millisec >= 1000)
				return false;
			if (microsecDuringMillisec >= 1000)
				return false;
			return true;
		}

		/// Checks if this date is earlier than the `other` date.
		bool operator<(const DateAndTime& other) const
		{
			return value < other.value;
		}

		/// Checks if this date is later than the `other` date.
		bool operator>(const DateAndTime& other) const
		{
			return value > other.value;
		}

		/// Checks if this date is earlier than the `other` date or the same.
		bool operator<=(const DateAndTime& other) const
		{
			return value <= other.value;
		}

		/// Checks if this date is later than the `other` date or the same.
		bool operator>=(const DateAndTime& other) const
		{
			return value >= other.value;
		}

		/// Checks if this date is identical to the `other` date.
		bool operator==(const DateAndTime& other) const
		{
			return value == other.value;
		}

		/// Checks if this date is different from the `other` date.
		bool operator!=(const DateAndTime& other) const
		{
			return value != other.value;
		}

		/**
		* Change date and time by adding a combination of time units.
		* @param years Number of years to add. May be negative.
		* @param months Number of months to add. May be negative and abs(months) may be > 12.
		* @param days Number of days to add. May be negative and abs(days) may be > 365.
		* @param hours Number of hours to add. May be negative and abs(hours) may be > 24.
		* @param minutes Number of minutes to add. May be negative and abs(minutes) may be > 60.
		* @param seconds Number of seconds to add. May be negative and abs(seconds) may be > 60.
		* @param millisecs Number of millisecs to add. May be negative and abs(millisecs) may be > 1000.
		* @param microsecsDuringMillisec Number of millisecs to add. May be negative and abs(microsecsDuringMillisec) may be > 1000.
		* @return Returns if update of date and time was successful. Error cases: Overflow, starting with invalid date (see below).
		*
		* This function requires a valid date to start with if it needs to change the date. If less than 1 day is added/subtracted
		* and the date does not flip due to the added/subtracted time (hours/minutes/seconds etc.), `add()` succeeds even with an
		* invalid date. Thus, for example if you want to accumulate short periods of time, you may use `add()` with an invalid date
		* such as 0000-00-00. However, it will fail and return false if you the accumulated time exceeds 23:59:59.999'999.
		*/
		bool add(sint64 years, sint64 months, sint64 days, sint64 hours, sint64 minutes, sint64 seconds, sint64 millisecs = 0, sint64 microsecsDuringMillisec = 0)
		{
			sint64 newMicrosec = getMicrosecDuringMillisec();
			sint64 newMillisec = getMillisec();
			sint64 newSec = getSecond();
			sint64 newMinute = getMinute();
			sint64 newHour = getHour();
			sint64 millisecCarry = 0, secCarry = 0, minuteCarry = 0, hourCarry = 0, dayCarry = 0;

			// update microseconds (checking for overflow)
			if (microsecsDuringMillisec &&
				!addAndComputeCarry(newMicrosec, millisecCarry, 1000, microsecsDuringMillisec))
			{
				return false;
			}

			// update milliseconds (checking for overflow)
			if ((millisecs || millisecCarry) &&
				!addAndComputeCarry(newMillisec, secCarry, 1000, millisecs, millisecCarry))
			{
				return false;
			}

			// update seconds (checking for overflow)
			if ((seconds || secCarry) &&
				!addAndComputeCarry(newSec, minuteCarry, 60, seconds, secCarry))
			{
				return false;
			}

			// update minutes (checking for overflow)
			if ((minutes | minuteCarry) &&
				!addAndComputeCarry(newMinute, hourCarry, 60, minutes, minuteCarry))
			{
				return false;
			}

			// update hours (checking for overflow)
			if ((hours | hourCarry) &&
				!addAndComputeCarry(newHour, dayCarry, 24, hours, hourCarry))
			{
				return false;
			}

			// set time
			if (this->isValid())
			{
				ASSERT(isValid(getYear(), getMonth(), getDay(), newHour, newMinute, newSec, newMillisec, newMicrosec));
			}
			setTime(newHour, newMinute, newSec, newMillisec, newMicrosec);

			// update date if needed
			if (dayCarry || days || months || years)
			{
				if (dayCarry && !addWithoutOverflow(days, dayCarry))
					return false;
				return add(years, months, days);
			}

			return true;
		}

		/**
		* Change date by adding number of days, months, and years.
		* @param years Number of years to add. May be negative.
		* @param months Number of months to add. May be negative and abs(months) may be > 12.
		* @param days Number of days to add. May be negative and abs(days) may be > 365.
		* @return Returns if update of date was successful. Error cases: starting with invalid date, overflow.
		*/
		bool add(sint64 years, sint64 months, sint64 days)
		{
			sint64 newDay = getDay();
			sint64 newMonth = getMonth();
			sint64 newYear = getYear();

			if (!isValid())
				return false;

			// speed-up processing large number of days (400 years and more)
			// (400 years always have the same number of leap years and days)
			constexpr sint64 daysPer400years = 97ll * 366ll + 303ll * 365ll;
			if (days >= daysPer400years || days <= -daysPer400years)
			{
				sint64 factor400years = days / daysPer400years;
				sint64 daysProcessed = factor400years * daysPer400years;
				newYear += factor400years * 400;
				days -= daysProcessed;
			}

			// speed-up processing large number of days (more than 1 year)
			if (days >= 365)
			{
				sint64 yShift = (newMonth >= 3) ? 1 : 0;
				while (days >= 365)
				{
					sint64 daysInYear = DateAndTime::isLeapYear(newYear + yShift) ? 366 : 365;
					if (days < daysInYear)
						break;
					++newYear;
					days -= daysInYear;
				}
			}
			else if (days <= -365)
			{
				sint64 yShift = (newMonth >= 3) ? 0 : -1;
				while (days <= -365)
				{
					sint64 daysInYear = DateAndTime::isLeapYear(newYear + yShift) ? -366 : -365;
					if (days > daysInYear)
						break;
					--newYear;
					days -= daysInYear;
				}
			}

			// general processing of any number of days
			while (days > 0)
			{
				// update day and month
				const sint64 monthDays = daysInMonth(newYear, newMonth);
				if (days >= monthDays)
				{
					// 1 month or more -> skip current month
					++newMonth;
					days -= monthDays;
				}
				else
				{
					// less than one month -> update day (and month if needed)
					newDay += days;
					days = 0;
					if (newDay > monthDays)
					{
						++newMonth;
						newDay -= monthDays;
					}
				}
				// update year if needed
				if (newMonth > 12)
				{
					newMonth = 1;
					++newYear;
				}
				// check if day exists in month
				if (newDay > 28)
				{
					const sint64 monthDays = daysInMonth(newYear, newMonth);
					if (newDay > monthDays)
					{
						ASSERT(newDay <= 31);
						ASSERT(newMonth < 12);
						newDay -= monthDays;
						newMonth += 1;
					}
				}
			}
			while (days < 0)
			{
				// update day and month
				if (-days < newDay)
				{
					// new date is in current month
					newDay += days;
					days = 0;
				}
				else
				{
					// new date is before current month
					--newMonth;
					if (newMonth < 1)
					{
						--newYear;
						newMonth = 12;
					}
					const sint64 monthDays = daysInMonth(newYear, newMonth);
					if (-days >= monthDays)
					{
						// at least one month back -> keep day (month was already updated before)
						days += monthDays;
					}
					else
					{
						// less than one month -> update day
						newDay += monthDays + days;
						days = 0;
					}
				}
			}

			// add month that were passed to this function
			if (months)
			{
				// Add months to newMonth, getting years carry. Months are computed in 0-11 instead of 1-12.
				sint64 yearsCarry = 0;
				--newMonth;
				if (!addAndComputeCarry(newMonth, yearsCarry, 12, months))
					return false;
				++newMonth;
				if (yearsCarry && !addWithoutOverflow(newYear, yearsCarry))
					return false;
			}

			// add years passed to function
			if (years && !addWithoutOverflow(newYear, years))
				return false;

			// check that day exists in final month
			if (newDay > 28)
			{
				const sint64 monthDays = daysInMonth(newYear, newMonth);
				if (newDay > monthDays)
				{
					ASSERT(newDay <= 31);
					ASSERT(newMonth < 12);
					newDay -= monthDays;
					newMonth += 1;
				}
			}

			// check if year is outside supported range
			if (newYear < 0 || newYear > 0xffff)
				return false;

			ASSERT(isValid(newYear, newMonth, newDay, getHour(), getMinute(), getSecond(), getMillisec(), getMicrosecDuringMillisec()));
			setDate(newYear, newMonth, newDay);

			return true;
		}

		/**
		* Convenience function for adding a number of days.
		* @param days Number of days to add. May be negative and abs(days) may be > 365.
		* @return Returns if update of date was successful. Error cases: starting with invalid date, overflow.
		*/
		bool addDays(sint64 days)
		{
			return add(0, 0, days);
		}

		/**
		* Convenience function for adding a number of milliseconds.
		* @param millisec Number of milliseconds to add. May be negative and abs(millisec) may be > 1000.
		* @return Returns if update of date was successful. Error cases: starting with invalid date, overflow.
		*/
		bool addMillisec(sint64 millisec)
		{
			return add(0, 0, 0, 0, 0, 0, millisec);
		}

		/**
		* Convenience function for adding a number of microseconds.
		* @param microsec Number of microsecs to add. May be negative and abs(microsecs) may be > 1000000.
		* @return Returns if update of date was successful. Error cases: starting with invalid date, overflow.
		*/
		bool addMicrosec(sint64 microsec)
		{
			return add(0, 0, 0, 0, 0, 0, 0, microsec);
		}

		/**
		* Compute duration between this and dt in microseconds. Returns UINT64_MAX if dt or this is invalid.
		*/
		uint64 durationMicrosec(const DateAndTime& dt) const
		{
			if (!isValid() || !dt.isValid())
				return UINT64_MAX;

			if (value == dt.value)
				return 0;

			DateAndTime begin = *this;
			DateAndTime end = dt;
			if (begin > end)
			{
				begin = dt;
				end = *this;
			}

			sint64 microDiff = end.getMicrosecDuringMillisec() - begin.getMicrosecDuringMillisec();
			sint64 milliDiff = end.getMillisec() - begin.getMillisec();
			sint64 secondDiff = end.getSecond() - begin.getSecond();
			sint64 minuteDiff = end.getMinute() - begin.getMinute();
			sint64 hourDiff = end.getHour() - begin.getHour();

			// compute the microsec offset needed to sync the time of t0 and t1 (may be negative)
			sint64 totalMicrosec = ((((((hourDiff * 60) + minuteDiff) * 60) + secondDiff) * 1000) + milliDiff) * 1000 + microDiff;
			bool okay = begin.add(0, 0, 0, hourDiff, minuteDiff, secondDiff, milliDiff, microDiff);
			ASSERT(okay);
			ASSERT((begin.value & 0x1fffffffff) == (end.value & 0x1fffffffff));
			ASSERT(begin.value <= end.value);

			// heuristic iterative algorithm for computing the days
			if (begin.value != end.value)
			{
				sint64 totalDays = 0;
				for (int i = 0; i < 10 && begin.value != end.value; ++i)
				{
					sint64 dayDiff = end.getDay() - begin.getDay();
					sint64 monthDiff = end.getMonth() - begin.getMonth();
					sint64 yearDiff = end.getYear() - begin.getYear();
					sint64 days = yearDiff * 365 + monthDiff * 28; // days may be negative
					if (!days)
						days = dayDiff;
					totalDays += days;
					okay = begin.add(0, 0, days);
					ASSERT(okay);
				}
				if (begin.value != end.value)
					return UINT64_MAX;
				ASSERT(totalDays >= 0);
				totalMicrosec += totalDays * (24llu * 60llu * 60 * 1000 * 1000);
			}

			ASSERT(totalMicrosec >= 0);
			return totalMicrosec;
		}

		/**
		* Compute duration between this and dt in full days. Returns UINT64_MAX if dt or this is invalid.
		*/
		uint64 durationDays(const DateAndTime& dt) const
		{
			uint64 ret = durationMicrosec(dt);
			if (ret != UINT64_MAX)
				ret /= (24llu * 60llu * 60llu * 1000llu * 1000llu);
			return ret;
		}

	protected:
		// condensed binary 8-byte representation supporting fast comparison:
		// - padding/reserved: 2 bits (most significant bits in 8-byte number, bits 62-63)
		// - year: 16 bits (bits 46-61)
		// - month: 4 bits (bits 42-45)
		// - day: 5 bits (bits 37-41)
		// - hour: 5 bits (bits 32-36)
		// - minute: 6 bits (bits 26-31)
		// - second: 6 bits (bits 20-25)
		// - millisecond: 10 bits (bits 10-19)
		// - microsecondDuringMillisecond: 10 bits (lowest significance in 8-byte number, bits 0-9)
		uint64 value;

		/// Adds valToAdd to valInAndOut and returns true if there is no overflow. Otherwise, returns false.
		static bool addWithoutOverflow(sint64& valInAndOut, sint64 valToAdd)
		{
			sint64 sum = valInAndOut + valToAdd;
			if (valInAndOut < 0 && valToAdd < 0 && sum > 0) // negative overflow
				return false;
			if (valInAndOut > 0 && valToAdd > 0 && sum < 0) // positive overflow
				return false;
			valInAndOut = sum;
			return true;
		}

		// Add of up to 2 values to low significance value (such as milliseconds) and split it
		// into high significance carry (for example in seconds) and low significance value (milliseconds).
		// All low and high values may be positive or negative.
		static bool addAndComputeCarry(sint64& low, sint64& highCarryOut, sint64 lowToHighFactor, sint64 lowAdd1, sint64 lowAdd2 = 0)
		{
			if (!addWithoutOverflow(low, lowAdd1))
				return false;

			if (lowAdd2 != 0 && !addWithoutOverflow(low, lowAdd2))
				return false;

			if (low == 0)
			{
				highCarryOut = 0;
			}
			else if (low > 0)
			{
				highCarryOut = low / lowToHighFactor;
				low %= lowToHighFactor;
			}
			else // (low < 0)
			{
				highCarryOut = (low - lowToHighFactor + 1) / lowToHighFactor;
				low = low - highCarryOut * lowToHighFactor;
				ASSERT(low < lowToHighFactor);
			}
			return true;
		}
	};
}
