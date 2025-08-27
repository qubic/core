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
