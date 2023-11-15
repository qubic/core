#define NO_UEFI

#include <iostream>

#include "../../src/score_reference.h"

#include "../../test/uc32.h"


void print_random_test_case()
{
    // generate random input data
    UC32x nonce(true);
    UC32x publicKey(true);
    unsigned long long processor_number;
    _rdrand64_step(&processor_number);
    processor_number %= 1024;
    unsigned int output_score = score(processor_number, publicKey.uc32x, nonce.uc32x);
    std::cout << "EXPECT_EQ(" << output_score << ",  score("
        << processor_number << ", "
        << "UC32x(" << publicKey.ull4[0] << "ULL, " << publicKey.ull4[1] << "ULL, " << publicKey.ull4[2] << "ULL, " << publicKey.ull4[3] << "ULL).uc32x, "
        << "UC32x(" << nonce.ull4[0] << "ULL, " << nonce.ull4[1] << "ULL, " << nonce.ull4[2] << "ULL, " << nonce.ull4[3] << "ULL).uc32x));" << std::endl;
}

int main()
{
    for (int i = 0; i < 30; ++i)
        print_random_test_case();
}
