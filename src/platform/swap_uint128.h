#pragma once

#include <unordered_set>

typedef unsigned long long SwapUint64;

// baseed on https://github.com/calccrypto/uint128_t/blob/master/uint128_t.cpp


class uint128_t{
public:
    SwapUint64 low;
    SwapUint64 high;

    uint128_t(SwapUint64 n){
        low = n;
        high = 0;
    };


    uint128_t(SwapUint64 i_high, SwapUint64 i_low){
        high = i_high;
        low = i_low;
    }

    // transfrom
    operator bool() const {
		return (bool) (high | low);
    }

    // compare
	bool operator==(const uint128_t & rhs) const{
		return ((high == rhs.high) && (low == rhs.low));
	}

	bool operator<(const uint128_t & rhs) const{
		if (high == rhs.high) {
			return low < rhs.low;
		} else {
			return high < rhs.high;
		}
	}

	bool operator>(const uint128_t & rhs) const{
		if (high == rhs.high) {
			return low > rhs.low;
		} else {
			return high > rhs.high;
		}
	}

	bool operator>=(const uint128_t & rhs) const{
		return ((*this > rhs) || (*this == rhs));
	}

	bool operator<=(const uint128_t & rhs) const{
		return ((*this < rhs) || (*this == rhs));
	}

	uint128_t operator<<(const uint128_t & rhs) const{
		const SwapUint64 shift = rhs.low;
		if (((bool) rhs.high) || (shift >= 128)){
			return uint128_t(0);
		}
		else if (shift == 64){
			return uint128_t(low, 0);
		}
		else if (shift == 0){
			return *this;
		}
		else if (shift < 64){
			return uint128_t((high << shift) + (low >> (64 - shift)), low << shift);
		}
		else if ((128 > shift) && (shift > 64)){
			return uint128_t(low << (shift - 64), 0);
		}
		else{
			return uint128_t(0);
		}
	}

	uint128_t & operator<<=(const uint128_t & rhs){
		*this = *this << rhs;
		return *this;
	}

	uint128_t operator>>(const uint128_t & rhs) const{
		const SwapUint64 shift = rhs.low;
		if (((bool) rhs.high) || (shift >= 128)){
			return uint128_t(0);
		}
		else if (shift == 64){
			return uint128_t(0, high);
		}
		else if (shift == 0){
			return *this;
		}
		else if (shift < 64){
			return uint128_t(high >> shift, (high << (64 - shift)) + (low >> shift));
		}
		else if ((128 > shift) && (shift > 64)){
			return uint128_t(0, (high >> (shift - 64)));
		}
		else{
			return uint128_t(0);
		}
	}

	uint128_t & operator>>=(const uint128_t & rhs){
		*this = *this >> rhs;
		return *this;
	}

	uint128_t operator&(const uint128_t & rhs) const{
		return uint128_t(high & rhs.high, low & rhs.low);
	}

	uint128_t operator&(const int & rhs) const{
		return (*this) & uint128_t(rhs);
	}

	uint128_t operator>>(const unsigned int & rhs) const{
		return *this >> uint128_t(rhs);
	}

    // bits
	uint8_t bits() const{
		uint8_t out = 0;
		if (high){
			out = 64;
			SwapUint64 up = high;
			while (up){
				up >>= 1;
				out++;
			}
		}
		else{
			SwapUint64 inner_low = low;
			while (inner_low){
				inner_low >>= 1;
				out++;
			}
		}
		return out;
	}

    // calculation
	uint128_t operator+(const uint128_t & rhs) const{
		return uint128_t(high + rhs.high+ ((low+ rhs.low) < low), low + rhs.low);
	}

	uint128_t operator-(const uint128_t & rhs) const{
		return uint128_t(high - rhs.high - ((low - rhs.low) > low), low - rhs.low);
	}

	uint128_t & operator-=(const uint128_t & rhs){
		*this = *this - rhs;
		return *this;
	}

	uint128_t & operator+=(const uint128_t & rhs){
		high += rhs.high + ((low + rhs.low) < low);
		low += rhs.low;
		return *this;
	}

	uint128_t & operator++(){
		return *this += uint128_t(1);
	}

    //std::pair <uint128_t, uint128_t> divmod(const uint128_t & lhs, const uint128_t & rhs) const;
	std::pair <uint128_t, uint128_t> divmod(const uint128_t & lhs, const uint128_t & rhs) const{
		// Save some calculations /////////////////////
		//if (rhs == uint128_0){
		//    throw std::domain_error("Error: division or modulus by 0");
		//}
		if (rhs == uint128_t(1)){
			return std::pair <uint128_t, uint128_t> (lhs, uint128_t(0));
		}
		else if (lhs == rhs){
			return std::pair <uint128_t, uint128_t> (uint128_t(1), uint128_t(0));
		}
		else if ((lhs == uint128_t(0)) || (lhs < rhs)){
			return std::pair <uint128_t, uint128_t> (uint128_t(0), lhs);
		}

		std::pair <uint128_t, uint128_t> qr (uint128_t(0), uint128_t(0));
		for(uint8_t x = lhs.bits(); x > 0; x--){
			qr.first  <<= uint128_t(1);
			qr.second <<= uint128_t(1);

			if ((lhs >> (x - 1U)) & 1){
				++qr.second;
			}

			if (qr.second >= rhs){
				qr.second -= rhs;
				++qr.first;
			}
		}
		return qr;
	}

	uint128_t operator/(const uint128_t & rhs) const{
		return divmod(*this, rhs).first;
	}

	uint128_t operator*(const uint128_t& rhs) const{
		// split values into 4 32-bit parts
		SwapUint64 top[4] = {high >> 32, high & 0xffffffff, low >> 32, low & 0xffffffff};

		SwapUint64 bottom[4] = {rhs.high >> 32, rhs.high & 0xffffffff, rhs.low >> 32, rhs.low & 0xffffffff};
		SwapUint64 products[4][4];

		// multiply each component of the values
		for(int y = 3; y > -1; y--){
			for(int x = 3; x > -1; x--){
				products[3 - x][y] = top[x] * bottom[y];
			}
		}

		// first row
		SwapUint64 fourth32 = (products[0][3] & 0xffffffff);
		SwapUint64 third32  = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
		SwapUint64 second32 = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
		SwapUint64 first32  = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);

		// second row
		third32  += (products[1][3] & 0xffffffff);
		second32 += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
		first32  += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);

		// third row
		second32 += (products[2][3] & 0xffffffff);
		first32  += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);

		// fourth row
		first32  += (products[3][3] & 0xffffffff);

		// move carry to next digit
		third32  += fourth32 >> 32;
		second32 += third32  >> 32;
		first32  += second32 >> 32;

		// remove carry from current digit
		fourth32 &= 0xffffffff;
		third32  &= 0xffffffff;
		second32 &= 0xffffffff;
		first32  &= 0xffffffff;

		// printf("%llx, %llx, %llx, %llx\n", fourth32, second32, third32, fourth32);

		// combine components
		return uint128_t((first32<<32|second32), (third32<<32|fourth32));
	}
};

