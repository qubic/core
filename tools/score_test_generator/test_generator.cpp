#define NO_UEFI

#include <iostream>

#include "../../src/platform/m256.h"

void print_random_test_case()
{
    // generate random input data
    m256i nonce;
    nonce.setRandomValue();
    m256i publicKey;
    publicKey.setRandomValue();
    unsigned long long processor_number;
    _rdrand64_step(&processor_number);
    processor_number %= 1024;

    std::cout << "EXPECT_TRUE(test_score("
        << processor_number << ", "
        << "m256i(" << publicKey.m256i_u64[0] << "ULL, " << publicKey.m256i_u64[1] << "ULL, " << publicKey.m256i_u64[2] << "ULL, " << publicKey.m256i_u64[3] << "ULL).m256i_u8, "
        << "m256i(" << nonce.m256i_u64[0] << "ULL, " << nonce.m256i_u64[1] << "ULL, " << nonce.m256i_u64[2] << "ULL, " << nonce.m256i_u64[3] << "ULL).m256i_u8));" << std::endl;
}

int main()
{
    for (int i = 0; i < 30; ++i)
        print_random_test_case();
}
