#define NO_UEFI

#include "../src/platform/memory.h"
#include "../src/four_q.h"
#include "utils.h"

#include <lib/platform_common/qintrin.h>
#include "gtest/gtest.h"

#include <chrono>
#include <iostream>

static constexpr int ID_SIZE = 61;
static inline void getIDChar(const unsigned char* key, char* identity, bool isLowerCase)
{
    CHAR16 computorID[61];
    getIdentity(key, computorID, true);
    for (int k = 0; k < 60; ++k)
    {
        identity[k] = computorID[k] - L'a' + 'a';
    }
    identity[60] = 0;
}

TEST(TestFourQ, TestMultiply)
{
    // 8 test cases for 256-bit multiplication
    unsigned long long a[8][4] = {
        {9951791076627133056ULL, 8515301911953011018ULL, 10503917255838740547ULL, 9403542041099946340ULL},
        {9634782769625085733ULL, 3923345248364070851ULL, 12874006609097115757ULL, 9445681298461330583ULL},
        {9314926113594160360ULL, 9012577733633554087ULL, 15853326627100346762ULL, 3353532907889994600ULL},
        {11822735244239455150ULL, 14860878323222532373ULL, 839169842161576273ULL, 8384082473945502970ULL},
        {6391904870724534887ULL, 7752608459014781040ULL, 8834893383869603648ULL, 14432583643443481392ULL},
        {9034457083341789982ULL, 15550692794033658766ULL, 18370398459251091929ULL, 161212377777301450ULL},
        {12066041174979511630ULL, 6197228902632247602ULL, 15544684064627230784ULL, 8662358800126738212ULL},
        {2997608593061094407ULL, 10746661492960439270ULL, 13066743968851273858ULL, 901611315508727516ULL}
    };

    unsigned long long b[8][4] = {
        {14556080569315562443ULL, 4784279743451576405ULL, 16952050128007612055ULL, 17448141405813274955ULL},
        {16953856751996506377ULL, 5957469746201176117ULL, 413985909494190460ULL, 5019301766552018644ULL},
        {8337584125020700765ULL, 9891896711220896307ULL, 3688562803407556063ULL, 15879907979249125147ULL},
        {5253913930687524613ULL, 14356908424098313115ULL, 7294083945257658276ULL, 11357758627518780620ULL},
        {6604082675214113798ULL, 8102242472442817269ULL, 4231600794557460268ULL, 9254306641367892880ULL},
        {15307070962626904180ULL, 14565308158529607085ULL, 7804612167412830134ULL, 11197002641182899202ULL},
        {5681082236069360781ULL, 11354469612480482261ULL, 10740484893427922886ULL, 4093428096946105430ULL},
        {16936346349005670285ULL, 16111331879026478134ULL, 281576863978497861ULL, 4843225515675739317ULL}
    };

    unsigned long long expectedMultiplicationResults[8][8] = {
        {13505937776277228416ULL, 10691581058996783029ULL, 15857294677093499275ULL, 10551077288120234079ULL,
         10488747005868148888ULL, 3163167577502768305ULL, 12011108917152358447ULL, 8894487319443104894ULL},

        {11258722506082215245ULL, 3752109657065586715ULL, 9754007644313481322ULL, 10650543212606486248ULL,
         14000725040689989368ULL, 6868107242688413590ULL, 12132679480588703742ULL, 2570140542862762927ULL},

        {7980800202961401928ULL, 18091087109835938886ULL, 11937230724836153237ULL, 18437285308724511498ULL,
         9256451621004954121ULL, 2817287347866660760ULL, 7356871972350029435ULL, 2886893956455686033ULL},

        {14821519003237893222ULL, 11951435221854993875ULL, 5649570579164725909ULL, 16529503750125471729ULL,
         5712698943065886767ULL, 10417044944053178538ULL, 10215165497768617151ULL, 5162124257364100363ULL},

        {10299854845140439658ULL, 5620198573463725080ULL, 18403939479767000599ULL, 3997239017815343129ULL,
         17558583433366224073ULL, 16662387952814143598ULL, 16240400534467578973ULL, 7240494806558978920ULL},

        {15852260790186789272ULL, 6843720495231156925ULL, 18209245341578934878ULL, 3229051715759855960ULL,
         6553675393672969791ULL, 11442787882881602486ULL, 5043402961965006398ULL, 97854418782578342ULL},

        {16919816915648955382ULL, 15728350531867604818ULL, 262149976282468082ULL, 16822220236393767682ULL,
         17482650320082366559ULL, 4634282717190265856ULL, 3892072508178358212ULL, 1922222304195309433ULL},

        {3674093358531938523ULL, 797775358977430453ULL, 6686355987721165902ULL, 16831290265741585642ULL,
         11378657779800286238ULL, 14872963278680745844ULL, 15850192255010623436ULL, 236719656924026199ULL}
    };

    long long averageProcessingTime = 0;
    for (int i = 0; i < 8; i++)
    {
        unsigned long long result[8] = { 0 };

        multiply(a[i], b[i], result);

        for (int k = 0; k < 8; k++)
        {
            EXPECT_EQ(result[k], expectedMultiplicationResults[i][k]) << " at [" << k << "]";
        }
    }
}

TEST(TestFourQ, TestMontgomeryMultiplyModOrder)
{

    // 8 test cases for 256-bit MontgomeryMultiplyMod
    unsigned long long a[8][4] = {
        {9951791076627133056ULL, 8515301911953011018ULL, 10503917255838740547ULL, 9403542041099946340ULL},
        {9634782769625085733ULL, 3923345248364070851ULL, 12874006609097115757ULL, 9445681298461330583ULL},
        {9314926113594160360ULL, 9012577733633554087ULL, 15853326627100346762ULL, 3353532907889994600ULL},
        {11822735244239455150ULL, 14860878323222532373ULL, 839169842161576273ULL, 8384082473945502970ULL},
        {6391904870724534887ULL, 7752608459014781040ULL, 8834893383869603648ULL, 14432583643443481392ULL},
        {9034457083341789982ULL, 15550692794033658766ULL, 18370398459251091929ULL, 161212377777301450ULL},
        {12066041174979511630ULL, 6197228902632247602ULL, 15544684064627230784ULL, 8662358800126738212ULL},
        {2997608593061094407ULL, 10746661492960439270ULL, 13066743968851273858ULL, 901611315508727516ULL}
    };

    unsigned long long b[8][4] = {
        {14556080569315562443ULL, 4784279743451576405ULL, 16952050128007612055ULL, 17448141405813274955ULL},
        {16953856751996506377ULL, 5957469746201176117ULL, 413985909494190460ULL, 5019301766552018644ULL},
        {8337584125020700765ULL, 9891896711220896307ULL, 3688562803407556063ULL, 15879907979249125147ULL},
        {5253913930687524613ULL, 14356908424098313115ULL, 7294083945257658276ULL, 11357758627518780620ULL},
        {6604082675214113798ULL, 8102242472442817269ULL, 4231600794557460268ULL, 9254306641367892880ULL},
        {15307070962626904180ULL, 14565308158529607085ULL, 7804612167412830134ULL, 11197002641182899202ULL},
        {5681082236069360781ULL, 11354469612480482261ULL, 10740484893427922886ULL, 4093428096946105430ULL},
        {16936346349005670285ULL, 16111331879026478134ULL, 281576863978497861ULL, 4843225515675739317ULL}
    };

    unsigned long long expectedMontgomeryMultiplyModOrderResults[8][4] = {
        {1178600784049730938ULL,13475129099769568773ULL,8171380610981515619ULL,8889798462048389782ULL,},
        {9346893806433251032ULL,3783576366952291632ULL,9006661425833189295ULL,2561156787602149305ULL,},
        {15012255874803770290ULL,11566062810664104635ULL,4497422501535458145ULL,2875434161571900946ULL,},
        {12004931297526373125ULL,10222857380028780508ULL,17154413062382081055ULL,5158706721726943589ULL,},
        {9724623868153589743ULL,17410218506619807138ULL,7496133043478274651ULL,7229774243864893754ULL,},
        {10156145653863087884ULL,16403847498912163678ULL,18326820829694769537ULL,90612319098335675ULL,},
        {2160566501146101694ULL,16888000406840707060ULL,8270191582668357443ULL,1911769260568884212ULL,},
        {6774298701428010308ULL,11825708701777781499ULL,11766967071579472107ULL,229574109642630470ULL,},
    };

    for (int i = 0; i < 8; i++)
    {
        unsigned long long result[4] = { 0 };
        // (a * b) mod n
        Montgomery_multiply_mod_order(a[i], b[i], result);

        for (int k = 0; k < 4; k++)
        {
            EXPECT_EQ(result[k], expectedMontgomeryMultiplyModOrderResults[i][k]) << " at [" << k << "]";
        }
    }
}

TEST(TestFourQ, TestGenerateKeys)
{
    // Data generate from clang-compiled qubic-cli
    unsigned char computorSeeds[][56] = {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "nhtighfbfdvgxnxrwxnbfmisknawppoewsycodciozvqpeqegttwofg",
        "fcvgljppwwwjhrawxeywxqdgssttiihcmikxbnnunugldvcitkhcrfl",
        "eijytgswxqzfkotmqvwulivpbximuhmgydvnaozwszguqflpfvltqge",
    };
    unsigned char expectedPrivateIDs[][ID_SIZE] = {
        "cctwbaulwuyhybijykxrmxnyrvzbalwryiiahltfwanuafhyfhepcjjgvaec",
        "qfldkxspcgsnbgnccmsjftuhxvlfrmarkqrvqjvjaebwoasbytasvcffdfwd",
        "mqlgaugwpdaphhqsacmqdcioomybxkaisrwyefyisayrikqjlckwkpdhuqlc",
        "qwqmcjhdlzphgabvnjbedsgwrgpbvplcipmxkzuogglbhzfjiytunaeactsn",
    };
    unsigned char expectedPublicIDs[][ID_SIZE] = {
        "bzbqfllbncxemglobhuvftluplvcpquassilfaboffbcadqssupnwlzbqexk",
        "lsgscfhdoahhlbdmlyzrfkvsfrqbbuznganescizcetyxkcdhljhemofxcwb",
        "zsvpltnzfdyjzetanimltroldybdzoctvfguybpbvdxbndsrhyreppgccspo",
        "xcfqbuwxxtufpfwyteglgchgnqyanubfbkpwtivfobxybgaqcgiqmzlgscwe"
    };

    unsigned char computorSubseeds[32];
    unsigned char computorPrivateKeys[32];
    unsigned char computorPublicKeys[32];
    char privakeyKeyId[ID_SIZE];
    char publicKeyId[ID_SIZE];

    int numberOfTests = sizeof(computorSeeds) / sizeof(computorSeeds[0]);

    for (int i = 0; i < numberOfTests; ++i)
    {
        getSubseed(computorSeeds[i], computorSubseeds);
        getPrivateKey(computorSubseeds, computorPrivateKeys);
        getPublicKey(computorPrivateKeys, computorPublicKeys);

        getIDChar(computorPrivateKeys, privakeyKeyId, true);
        getIDChar(computorPublicKeys, publicKeyId, true);
        // Verification
        for (int k = 0; k < ID_SIZE; ++k)
        {
            EXPECT_EQ(expectedPrivateIDs[i][k], privakeyKeyId[k]) << " at [" << i << "][" << k << "]";
            EXPECT_EQ(expectedPublicIDs[i][k], publicKeyId[k]) << " at [" << i << "][" << k << "]";
        }
    }
}

// sign(const unsigned char* subseed, const unsigned char* publicKey, const unsigned char* messageDigest, unsigned char* signature)
TEST(TestFourQ, TestSign)
{
    // For sign and verification, some constants need to be set
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif

    const std::string subSeedsStr[] = {
        "4ac19e2bf0d3776519aabe31924f7dc2589b3d0e7411a65f84c9b16df72c038e",
        "e8217c5b40aa91df662803ce4dbf18722e35f1097ac68fb5da10643a825799e3",
        "6d02f48bcb53ac397fc71a9028e4165df9b87044c53e116a0192d7fa83254bb0",
        "3cfa1097be482f6e5ce132c2aa657d0fb9d84121048de6f05b90a2cc136bf73a",
        "5208dd447a21f3c9911cae547637c580e74fa40d2a9c5e1fb26dcbfa408539d1",
        "987b163dc6fd492573b4e18af7016c52a027ced5345fb8904e69037ed2ac1b81",
        "d462ab0c83f931c4a87f12953e20bd576be849da5a8f1402c3b176ef92486d05",
        "713fc86e289f124bdb2a65f6a37c01e0559a148ebf430f6729c184de76b23a90",
        "15cd5b0700000000b168de3a00000000743af15000000000efcdab0000000000"
    };

    const std::string messageDigestsStr[] = {
         "94e120a4d3f58c217a53eb9046d9f2c5b11288a9fe340d6ce5a771cf04b82e63",
         "77f493b58ea40162dc33f9a718e2543b05f629884d7ca0e31598c45f021ae7c0",
         "5cc82fa973101da5bfb3e2448196f0a7d7d3324c86fbbe42907613d5c8c2f1a4",
         "c01ae5f2879d11439b30ddae5f4c7b22689f023e17b4955c3b2f05e8d9089af6",
         "2f893e70ad52c9186eb4b60dfe137288c4a9e0fb6d34a51897e2365a01b0d443",
         "ec09a3f415c2dda5f8419026678ab03524f67ed9817ba230cd24513750e01bc6",
         "48b59f32a61dff0e13528e4c7937bdf080c3efa7d221364be87f01d5c60f882a",
         "b31e704c8a3f1d02692f05a7d8e5f6c911d370f4a68b2ec3fa4c51d7289003de",
         "89d5f92a895987457400219e121e8730f6b248a1fd28bfee017611ef079105b5"
    };

    const std::string expectedSignaturesStr[] =
    {
        "357d47b1366f33eed311a4458ec7326d35728e9292328a9b7ff8d4ec0f7b0df9323f5d1cd01bd5a380a1a8e4f29ad3ae9c5d94e84f4181a61ca73030d6d11600",
        "7d2479a15746839c4c5e1fdf0aadb167974c292ceee80593e18b5135763db63163d8eee5bd309c506f47b16cd1242ddfe985887b19d3943c14ec6ab79a9c1900",
        "1b8ed83af3dc12deb1554f48df46bf5bc5e4654f62f97ef20656fae4e965ac87762c9fe6189dfe89192a619bca4a6c390f4e97bb1f926041263f2ba4206b0c00",
        "470ad247ff6b2e55d44e9f2a79ce402bfc5e8c5322ed297f71939a9c5398b6fcb5058c05e614d10d90d6bdec8ee4ecc6462cdd54e0ea830fde6be465de3f2900",
        "0851db3d4021bdc8cf3816b4672aba2f5f7cd0bf19e779e28ee60241bc4246dabe7442a11953703a44ed1cadd0af9fce683c5a312326341ac7a3e55a18c40100",
        "54466ae5ecad45c83798e4e3e02ab40e834bf8d3f4f1628b300601ab87894599a43278efd48be7e9615cd569e656356a9e2307ae257b85a3f1f0f333f2302200",
        "6d67294ccd03dc51fdb3bca649b7e060d3cf06c417e7053472ca617b93e5926928a7a48b1791c2487c7e83eeb4919046493709508c0541d1c02e9545401b2100",
        "20764a88943fb4e796f81a560bde5e652c82ffb203c00b4846102a268ae68f64cdee6c7a3edbf4de48dd25fd423a4b40e79d97a2a47fd11030b6f30a09130b00",
        "9f71d3138ff8a72db3b39883e056ce7f5bfe40de6387e64eff0c17e72bd1862ccd848000be1841725f1da87654235329b685e1c81c939cb0154bbc8d30a20c00",
    };

    constexpr size_t numberOfTests = sizeof(subSeedsStr) / sizeof(subSeedsStr[0]);
    m256i subseeds[numberOfTests];
    m256i messageDigests[numberOfTests];
    unsigned char expectedSignatures[numberOfTests][64];

    for (unsigned long long i = 0; i < numberOfTests; ++i)
    {
        subseeds[i] = test_utils::hexTo32Bytes(subSeedsStr[i], 32);
        messageDigests[i] = test_utils::hexTo32Bytes(messageDigestsStr[i], 32);
        test_utils::hexToByte(expectedSignaturesStr[i], 64, expectedSignatures[i]);
    }

    for (unsigned long long i = 0; i < numberOfTests; ++i)
    {
        unsigned char publicKey[32];
        unsigned char privateKey[32];
        getPrivateKey(subseeds[i].m256i_u8, privateKey);
        getPublicKey(privateKey, publicKey);

        unsigned char signature[64];
        sign(subseeds[i].m256i_u8, publicKey, messageDigests[i].m256i_u8, signature);

        // Verify functions
        bool verifyStatus = verify(publicKey, messageDigests[i].m256i_u8, signature);

        EXPECT_TRUE(verifyStatus);

        for (int k = 0; k < 64; ++k)
        {
            EXPECT_EQ(expectedSignatures[i][k], signature[k]) << " at [" << i << "][" << k << "]";
        }
    }
}

// Helpers for the verify() negative/malleability tests below.
namespace
{
    // A few (subseed, messageDigest) vectors reused from TestSign to produce valid signatures.
    const char* const kVerifySubSeeds[] = {
        "4ac19e2bf0d3776519aabe31924f7dc2589b3d0e7411a65f84c9b16df72c038e",
        "e8217c5b40aa91df662803ce4dbf18722e35f1097ac68fb5da10643a825799e3",
        "6d02f48bcb53ac397fc71a9028e4165df9b87044c53e116a0192d7fa83254bb0",
        "3cfa1097be482f6e5ce132c2aa657d0fb9d84121048de6f05b90a2cc136bf73a",
    };
    const char* const kVerifyDigests[] = {
        "94e120a4d3f58c217a53eb9046d9f2c5b11288a9fe340d6ce5a771cf04b82e63",
        "77f493b58ea40162dc33f9a718e2543b05f629884d7ca0e31598c45f021ae7c0",
        "5cc82fa973101da5bfb3e2448196f0a7d7d3324c86fbbe42907613d5c8c2f1a4",
        "c01ae5f2879d11439b30ddae5f4c7b22689f023e17b4955c3b2f05e8d9089af6",
    };
    constexpr int kNumVerifyVectors = sizeof(kVerifySubSeeds) / sizeof(kVerifySubSeeds[0]);

    // Produce a valid signature (and its publicKey + digest) for vector i.
    void makeValidSignature(int i, unsigned char publicKey[32], unsigned char digest[32], unsigned char signature[64])
    {
        m256i subseed = test_utils::hexTo32Bytes(kVerifySubSeeds[i], 32);
        m256i md = test_utils::hexTo32Bytes(kVerifyDigests[i], 32);
        unsigned char privateKey[32];
        getPrivateKey(subseed.m256i_u8, privateKey);
        getPublicKey(privateKey, publicKey);
        copyMem(digest, md.m256i_u8, 32);
        sign(subseed.m256i_u8, publicKey, digest, signature);
    }

    // out = s + curve_order (256-bit), s is the lower 32 bytes interpreted little-endian.
    void addCurveOrder(const unsigned char s[32], unsigned char out[32])
    {
        unsigned long long si[4], ri[4] = { CURVE_ORDER_0, CURVE_ORDER_1, CURVE_ORDER_2, CURVE_ORDER_3 };
        copyMem(si, s, 32);
        unsigned long long oi[4];
        unsigned char carry = _addcarry_u64(0, si[0], ri[0], &oi[0]);
        carry = _addcarry_u64(carry, si[1], ri[1], &oi[1]);
        carry = _addcarry_u64(carry, si[2], ri[2], &oi[2]);
        _addcarry_u64(carry, si[3], ri[3], &oi[3]);
        copyMem(out, oi, 32);
    }
}

// Negative test: verify() must reject signatures that have been tampered with.
TEST(TestFourQ, TestVerifyRejectsTamperedSignature)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    for (int i = 0; i < kNumVerifyVectors; ++i)
    {
        unsigned char publicKey[32], digest[32], signature[64];
        makeValidSignature(i, publicKey, digest, signature);

        // Sanity: the untouched signature must verify.
        ASSERT_TRUE(verify(publicKey, digest, signature)) << " valid signature rejected at [" << i << "]";

        // Flip a bit in the commitment R (first 32 bytes) -> must fail.
        {
            unsigned char bad[64];
            copyMem(bad, signature, 64);
            bad[0] ^= 0x01;
            EXPECT_FALSE(verify(publicKey, digest, bad)) << " tampered R accepted at [" << i << "]";
        }

        // Flip a low bit in the scalar S (bytes 32..63) -> must fail (still canonical, just wrong).
        {
            unsigned char bad[64];
            copyMem(bad, signature, 64);
            bad[32] ^= 0x01;
            EXPECT_FALSE(verify(publicKey, digest, bad)) << " tampered S accepted at [" << i << "]";
        }

        // Wrong message digest -> must fail.
        {
            unsigned char otherDigest[32];
            copyMem(otherDigest, digest, 32);
            otherDigest[0] ^= 0x01;
            EXPECT_FALSE(verify(publicKey, otherDigest, signature)) << " wrong digest accepted at [" << i << "]";
        }

        // Wrong public key -> must fail.
        {
            unsigned char otherPub[32];
            copyMem(otherPub, publicKey, 32);
            otherPub[0] ^= 0x01;
            EXPECT_FALSE(verify(otherPub, digest, signature)) << " wrong public key accepted at [" << i << "]";
        }
    }
}

// Malleability test: the twin S' = S + curve_order is a second valid SchnorrQ scalar for the
// same (R, publicKey, message). It MUST be rejected by the canonical S < curve_order check.
// Before that check existed, S' passed (only S < 2^246 was enforced) and produced a distinct
// transaction hash, enabling double-execution.
TEST(TestFourQ, TestVerifyRejectsMalleableSignature)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    int testedTwins = 0;
    for (int i = 0; i < kNumVerifyVectors; ++i)
    {
        unsigned char publicKey[32], digest[32], signature[64];
        makeValidSignature(i, publicKey, digest, signature);
        ASSERT_TRUE(verify(publicKey, digest, signature)) << " valid signature rejected at [" << i << "]";

        // Build the malleable twin: same R (bytes 0..31), S' = S + r (bytes 32..63).
        unsigned char twin[64];
        copyMem(twin, signature, 32);
        unsigned char twinScalar[32];
        addCurveOrder(signature + 32, twinScalar);
        copyMem(twin + 32, twinScalar, 32);

        // Only twins that still fit in 2^246 would have slipped past the OLD check; those are the
        // dangerous ones this fix must catch. (S' >= 2^246 was already rejected before.)
        const bool twinFitsOldCheck = !((twin[62] & 0xC0) || twin[63]);
        if (twinFitsOldCheck)
        {
            ++testedTwins;
            EXPECT_FALSE(verify(publicKey, digest, twin))
                << " malleable twin S+r accepted at [" << i << "] (signature malleability not blocked)";
        }
    }
    // At least one vector should exercise the dangerous (old-check-passing) twin range.
    EXPECT_GT(testedTwins, 0) << " no test vector produced a twin in the malleable range";
}

// Boundary test: a scalar S exactly equal to the curve order is non-canonical and must be rejected.
TEST(TestFourQ, TestVerifyRejectsScalarEqualToCurveOrder)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    unsigned char publicKey[32], digest[32], signature[64];
    makeValidSignature(0, publicKey, digest, signature);

    // S = r exactly (>= r, so rejected by the canonical check regardless of the rest).
    unsigned long long ri[4] = { CURVE_ORDER_0, CURVE_ORDER_1, CURVE_ORDER_2, CURVE_ORDER_3 };
    unsigned char atOrder[64];
    copyMem(atOrder, signature, 32);
    copyMem(atOrder + 32, ri, 32);
    EXPECT_FALSE(verify(publicKey, digest, atOrder)) << " scalar S == curve_order accepted";
}

// ---------------------------------------------------------------------------
// Low-order (weak) public-key forgery.
//
// FourQ's group order is 392*r (cofactor 392 = 2^3 * 7^2, r = prime subgroup order).
// Any public key whose order divides 392 (the 392-point cofactor subgroup) lets an
// attacker forge a valid signature WITHOUT a private key, because h*A then depends only
// on h mod (order of A) and is enumerable. The identity point is the worst case: a single
// deterministic forgery is valid for EVERY message:
//     pick any canonical S < r;  R = encode(S*G);  signature = (R, S)
// verifies because S*G + h*A == S*G == R (h*A == O). The identity encodes as 01 00..00,
// which is also the QX contract address. verify() must reject the whole cofactor subgroup.
// See fourqExp.md.
// ---------------------------------------------------------------------------
namespace
{
    // Well-known low-order public keys, as raw 32-byte little-endian encodings (m256i limbs).
    // Each is a genuine, canonical cofactor-subgroup point. (Non-canonical y==p aliases are
    // intentionally NOT listed: decode() does not handle a literal y==p uniformly, so such
    // encodings do not reliably decode back to their low-order base point -- the reason the
    // proper remedy is canonical-y enforcement in decode(), tracked separately.)
    struct WeakKey { const char* name; unsigned long long limb[4]; };
    const WeakKey kWeakKeys[] = {
        { "identity (0,1) order 1 (== QX address)", { 1, 0, 0, 0 } },                            // hits fast-path
        { "NULL_ID (i,0) order 4",                  { 0, 0, 0, 0 } },                            // hits general check
        { "(-i,0) order 4",                         { 0, 0, 0, 0x8000000000000000ULL } },        // hits general check
        { "(0,-1) order 2",                         { 0xFFFFFFFFFFFFFFFEULL, 0x7FFFFFFFFFFFFFFFULL, 0, 0 } }, // general
    };
    constexpr int kNumWeakKeys = sizeof(kWeakKeys) / sizeof(kWeakKeys[0]);

    void weakKeyBytes(const WeakKey& w, unsigned char out[32])
    {
        copyMem(out, w.limb, 32);
    }

    // Independent of verify(): does the key decode and is [392]*A the neutral point?
    // True == A is a cofactor-subgroup (forgeable) point. Used to PROVE the test vectors
    // are genuinely low-order, and to exercise cofactor_clearing directly.
    bool isCofactorPoint(const unsigned char pubkey[32])
    {
        point_t A;
        if (!decode(pubkey, A))
        {
            return false; // not a decodable curve point
        }
        point_extproj_t P;
        point_setup(A, P);
        cofactor_clearing(P);          // P = 392 * A
        mod1271(P->x[0]);
        mod1271(P->x[1]);
        return (P->x[0][0] | P->x[0][1] | P->x[1][0] | P->x[1][1]) == 0; // projective X == 0
    }

    // Deterministic identity forgery: signature = encode(S*G) || S, with canonical S.
    // Uses the library's own generator multiplication, exactly matching what verify()
    // computes for the G term, so this is a genuine forgery (valid absent the guard).
    void forgeIdentitySignature(unsigned long long sLow, unsigned char signature[64])
    {
        unsigned long long s[4] = { sLow, 0, 0, 0 };
        point_t P;
        ecc_mul_fixed(s, P);              // P = S * G
        encode(P, signature);            // R = encode(S*G)  -> signature[0:32]
        copyMem(signature + 32, s, 32);  // S                -> signature[32:64]
    }

    // A faithful copy of the SHIPPED verify() with the two low-order guards (identity
    // fast-path + [392]*A==O cofactor check) intentionally REMOVED -- i.e. exactly the
    // pre-fix verifier. Everything else (bit128 reject, canonical S<r reject, decode,
    // ecc_mul_double, encode-compare) is kept identical. Used to prove a forgery would have
    // been ACCEPTED before the fix and is REJECTED only because of it. If verify()'s
    // non-guard logic ever changes, mirror it here.
    bool verifyUnpatched(const unsigned char* publicKey, const unsigned char* messageDigest, const unsigned char* signature)
    {
        point_t A;
        unsigned char temp[32 + 64], h[64];

        if ((publicKey[15] & 0x80) || (signature[15] & 0x80))
        {
            return false;
        }
        // Canonical scalar S < curve_order (this predates the low-order fix).
        {
            const unsigned long long* s = (const unsigned long long*)(signature + 32);
            static const unsigned long long r[4] = { CURVE_ORDER_0, CURVE_ORDER_1, CURVE_ORDER_2, CURVE_ORDER_3 };
            bool canonical = false;
            for (int i = 3; i >= 0; --i)
            {
                if (s[i] < r[i]) { canonical = true; break; }
                if (s[i] > r[i]) { break; }
            }
            if (!canonical)
            {
                return false;
            }
        }
        // <<< pre-fix: NO identity fast-path and NO [392]*A==O cofactor check here >>>
        if (!decode(publicKey, A))
        {
            return false;
        }
        copyMem(temp, signature, 32);
        copyMem(temp + 32, publicKey, 32);
        copyMem(temp + 64, messageDigest, 32);
        KangarooTwelve(temp, 32 + 64, h, 64);
        if (!ecc_mul_double((unsigned long long*)(signature + 32), (unsigned long long*)h, A))
        {
            return false;
        }
        unsigned char encoded[32];
        encode(A, encoded);
        for (int j = 0; j < 32; ++j)
        {
            if (encoded[j] != signature[j])
            {
                return false;
            }
        }
        return true;
    }
}

// Sanity layer: the weak vectors really are cofactor-subgroup points (the forgeable
// class), and a legitimate key is not. This validates the vectors used by the rejection
// tests below and exercises cofactor_clearing end-to-end.
TEST(TestFourQ, TestLowOrderVectorsAreCofactorPoints)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    for (int i = 0; i < kNumWeakKeys; ++i)
    {
        unsigned char pk[32];
        weakKeyBytes(kWeakKeys[i], pk);
        EXPECT_TRUE(isCofactorPoint(pk)) << " expected low-order: " << kWeakKeys[i].name;
    }

    unsigned char publicKey[32], digest[32], signature[64];
    makeValidSignature(0, publicKey, digest, signature);
    EXPECT_FALSE(isCofactorPoint(publicKey)) << " legitimate key misclassified as low-order";
}

// Headline forgery: the identity point {1,0,0,0} (== QX contract address) with a single
// forged signature that is valid for EVERY message. verify() must reject it for all of
// them. The positive control proves verify() is not trivially returning false.
TEST(TestFourQ, TestVerifyRejectsIdentityForgery)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    unsigned char identity[32];
    weakKeyBytes(kWeakKeys[0], identity); // {1,0,0,0}

    // Positive control: a legitimate key + genuine signature still verifies.
    {
        unsigned char pub[32], dig[32], sig[64];
        makeValidSignature(0, pub, dig, sig);
        ASSERT_TRUE(verify(pub, dig, sig)) << " sanity: valid signature must verify";
    }

    const unsigned long long scalars[] = { 1ULL, 2ULL, 123456789ULL };
    for (int si = 0; si < (int)(sizeof(scalars) / sizeof(scalars[0])); ++si)
    {
        unsigned char signature[64];
        forgeIdentitySignature(scalars[si], signature);

        // Message-independence is the hallmark of the identity attack: absent the guard
        // this same (R,S) verifies for every message. Prove that directly per digest:
        // pre-fix verifier ACCEPTS, shipped verifier REJECTS.
        for (int d = 0; d < kNumVerifyVectors; ++d)
        {
            m256i md = test_utils::hexTo32Bytes(kVerifyDigests[d], 32);
            EXPECT_TRUE(verifyUnpatched(identity, md.m256i_u8, signature))
                << " pre-fix verifier should accept identity forgery (S=" << scalars[si] << ", digest idx " << d << ")";
            EXPECT_FALSE(verify(identity, md.m256i_u8, signature))
                << " identity forgery accepted (S=" << scalars[si] << ", digest idx " << d << ")";
        }
        unsigned char digest[32];
        setMem(digest, 32, 0xAB);
        EXPECT_TRUE(verifyUnpatched(identity, digest, signature))
            << " pre-fix verifier should accept identity forgery for 0xAB digest (S=" << scalars[si] << ")";
        EXPECT_FALSE(verify(identity, digest, signature))
            << " identity forgery accepted for 0xAB digest (S=" << scalars[si] << ")";
    }
}

// Defense in depth: verify() must reject every cofactor-subgroup public key (including the
// non-canonical y==p aliases), whatever signature is presented. The guard fires right after
// decode(), before the signature math. Each vector is first asserted to be genuinely
// low-order, so the rejection is attributable to the weak-key guard and not to an incidental
// signature mismatch.
TEST(TestFourQ, TestVerifyRejectsAllLowOrderPublicKeys)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    unsigned char signature[64];
    forgeIdentitySignature(7ULL, signature); // well-formed: R = encode(7*G), S = 7

    unsigned char digest[32];
    setMem(digest, 32, 0x5A);

    for (int i = 0; i < kNumWeakKeys; ++i)
    {
        unsigned char pk[32];
        weakKeyBytes(kWeakKeys[i], pk);
        ASSERT_TRUE(isCofactorPoint(pk)) << " test vector not low-order: " << kWeakKeys[i].name;
        EXPECT_FALSE(verify(pk, digest, signature))
            << " low-order public key accepted: " << kWeakKeys[i].name;
    }
}

// Real forged transactions the attacker executed on network. Each is a full Qubic
// transaction (source | dest | amount | tick | inputType | inputSize | signature) that
// spends QU *from the identity point* {1,0,0,0} == 01 00..00 == the QX contract address,
// to two attacker-controlled wallets. They passed the old verify() and must now be rejected.
// The digest is reproduced exactly as the node computes it in processBroadcastTransaction():
//     KangarooTwelve(tx, totalSize - SIGNATURE_SIZE, digest, 32).
TEST(TestFourQ, TestVerifyRejectsNetworkForgeries)
{
#ifdef __AVX512F__
    initAVX512FourQConstants();
#endif
    const char* const kNetworkForgeries[] = {
        "010000000000000000000000000000000000000000000000000000000000000091cfd01bb1d6b1d48e9f806a8ef65734ae6162fbb3b2643a0088c1046e55f776009435770000000072e878030000000052750c95db608b2ad33f260c13523f3392afdbeefcd071a824e3973403443d890f997c86e5c769fd2344c515a7ce19b4dffc30a06e6da2dab5a1bc09f2212300",
        "010000000000000000000000000000000000000000000000000000000000000091cfd01bb1d6b1d48e9f806a8ef65734ae6162fbb3b2643a0088c1046e55f776009435770000000060e8780300000000304d8a0eef6d5e578f7fe3623f21d97debd5c1a77a2bad8f347ae4cf41256a61f31cd9c1d5280e2895ba8d3864e7e33cb9197d7e0fca47e38211277fb2960b00",
        "01000000000000000000000000000000000000000000000000000000000000008220a28587c81abd47934c0f6e8af42d1b10182c494a3d46aa9908eaf83e606a00e40b5402000000e74a7803000000004c1357678e9aa76ef892379f94545e571a1435730c15b5ae366a1f34f9a8fa54834ba3e96e9c5b6407759e470b5ecceb5f233f067bbeecd72ebe0332b11f0200",
        "01000000000000000000000000000000000000000000000000000000000000008220a28587c81abd47934c0f6e8af42d1b10182c494a3d46aa9908eaf83e606a00f2052a01000000bf487803000000009f660e0561ec753b2d2395ef84a8b305489c091fbba0cfa95f910cc13d56e67f542412901ff6107611d2d4facf038fe73193a3f8b1dff646be0a9100ce752600",
        "0100000000000000000000000000000000000000000000000000000000000000d4902431eb401facb0e5f4c649b53801c3ad1228ba0294953922d2e662a66da301000000000000008d0d77030000000091f3a7dd1cf60d64a79efac0fe6f034a6848924a08f65d46059387a801761ebafc062f4764555f485fedbd34a7e536889d261582a46325cb5dd1e5b083412300",
        "01000000000000000000000000000000000000000000000000000000000000004de5b0cd0b1f9638e12b9906cc36a39334835644d958318957bb37b648b5822f01000000000000003917770300000000ef46106b7a44f73e67f857b8b9f66e4c2a0ed960453fe55a4058d0e4c4a052d679d773ae43ec0bc24e200ae4ce28abb5e0a195d45c313358719b0c77ae131c00",
        "01000000000000000000000000000000000000000000000000000000000000008220a28587c81abd47934c0f6e8af42d1b10182c494a3d46aa9908eaf83e606a01000000000000002b197703000000005cafd8c1dcf05b0eebf805cafb40d361848c13f94ce78640cfdb76fba0cddcde50ce8556637fdac894848fa18b18b5fdcbc49b0719ef505fcafe3100497f2200",
        "01000000000000000000000000000000000000000000000000000000000000008220a28587c81abd47934c0f6e8af42d1b10182c494a3d46aa9908eaf83e606a00e40b5402000000a0437703000000009573ad5ce6f0e8d8d02a28297334c3009a319580e63c27c5a1b257fe2e056fcc0b8307956b52c223eba07eb5a8a0384ba044626aed71637ef3238ad3ee160c00"
    };
    constexpr int n = sizeof(kNetworkForgeries) / sizeof(kNetworkForgeries[0]);

    auto bytesEqual = [](const unsigned char* a, const unsigned char* b, int len) {
        for (int j = 0; j < len; ++j) { if (a[j] != b[j]) return false; }
        return true;
    };
    unsigned char identity[32];
    setMem(identity, 32, 0);
    identity[0] = 1;

    for (int i = 0; i < n; ++i)
    {
        const std::string hex = kNetworkForgeries[i];
        ASSERT_EQ(hex.size() % 2, (size_t)0) << " vector " << i << " has odd hex length";
        const int totalBytes = (int)(hex.size() / 2);
        ASSERT_GE(totalBytes, 80 + 64) << " vector " << i << " too short to be a transaction";

        unsigned char tx[256];
        ASSERT_LE(totalBytes, (int)sizeof(tx));
        test_utils::hexToByte(hex, totalBytes, tx);

        // The spent-from account is the identity / QX address, and it is a low-order point.
        EXPECT_TRUE(bytesEqual(tx, identity, 32)) << " vector " << i << " source is not the identity point";
        EXPECT_TRUE(isCofactorPoint(tx)) << " vector " << i << " source is not a cofactor point";

        // Digest exactly as the node computes it: K12 over everything but the 64-byte signature.
        const int digestLen = totalBytes - 64;
        unsigned char digest[32];
        KangarooTwelve(tx, digestLen, digest, 32);
        const unsigned char* signature = tx + digestLen;

        // Independent structural check that this really is a valid identity forgery: for
        // A = identity, verify accepts iff encode(S*G) == R (since h*A == O).
        {
            unsigned long long S[4];
            copyMem(S, signature + 32, 32);
            point_t P;
            ecc_mul_fixed(S, P);
            unsigned char R[32];
            encode(P, R);
            EXPECT_TRUE(bytesEqual(R, signature, 32))
                << " vector " << i << " is not a valid identity forgery (encode(S*G) != R)";
        }

        // The before/after proof, on the SAME input and digest:
        //   pre-fix verifier  -> ACCEPTS (this is why the attack worked on network)
        //   shipped verifier   -> REJECTS (the low-order guard is what stops it)
        EXPECT_TRUE(verifyUnpatched(tx, digest, signature))
            << " vector " << i << ": forgery did NOT pass the pre-fix verifier (setup wrong)";
        EXPECT_FALSE(verify(tx, digest, signature))
            << " FORGERY ACCEPTED for vector " << i << " -- the fix is NOT working";
    }
}