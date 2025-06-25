#pragma once

#include <lib/platform_common/qintrin.h>

#include "platform/memory.h"
#include "platform/assert.h"

// Compiler flag to determine AVX512 or generic implementation of K12

#define GENERIC_K12 1


////////// KangarooTwelve \\\\\\\\\\

#define KeccakF1600RoundConstant0   0x000000008000808bULL
#define KeccakF1600RoundConstant1   0x800000000000008bULL
#define KeccakF1600RoundConstant2   0x8000000000008089ULL
#define KeccakF1600RoundConstant3   0x8000000000008003ULL
#define KeccakF1600RoundConstant4   0x8000000000008002ULL
#define KeccakF1600RoundConstant5   0x8000000000000080ULL
#define KeccakF1600RoundConstant6   0x000000000000800aULL
#define KeccakF1600RoundConstant7   0x800000008000000aULL
#define KeccakF1600RoundConstant8   0x8000000080008081ULL
#define KeccakF1600RoundConstant9   0x8000000000008080ULL
#define KeccakF1600RoundConstant10  0x0000000080000001ULL

// NOTE: This is needed for the score function on avx512 BM.
// For some reasons, avx512 implementation for K12 is much slower when the neuron count higher than 16K

#define declareABCDEScalar                            \
    unsigned long long Aba, Abe, Abi, Abo, Abu; \
    unsigned long long Aga, Age, Agi, Ago, Agu; \
    unsigned long long Aka, Ake, Aki, Ako, Aku; \
    unsigned long long Ama, Ame, Ami, Amo, Amu; \
    unsigned long long Asa, Ase, Asi, Aso, Asu; \
    unsigned long long Bba, Bbe, Bbi, Bbo, Bbu; \
    unsigned long long Bga, Bge, Bgi, Bgo, Bgu; \
    unsigned long long Bka, Bke, Bki, Bko, Bku; \
    unsigned long long Bma, Bme, Bmi, Bmo, Bmu; \
    unsigned long long Bsa, Bse, Bsi, Bso, Bsu; \
    unsigned long long Ca, Ce, Ci, Co, Cu;      \
    unsigned long long Da, De, Di, Do, Du;      \
    unsigned long long Eba, Ebe, Ebi, Ebo, Ebu; \
    unsigned long long Ega, Ege, Egi, Ego, Egu; \
    unsigned long long Eka, Eke, Eki, Eko, Eku; \
    unsigned long long Ema, Eme, Emi, Emo, Emu; \
    unsigned long long Esa, Ese, Esi, Eso, Esu;

#define declareBCDEScalar                            \
    unsigned long long Bba, Bbe, Bbi, Bbo, Bbu; \
    unsigned long long Bga, Bge, Bgi, Bgo, Bgu; \
    unsigned long long Bka, Bke, Bki, Bko, Bku; \
    unsigned long long Bma, Bme, Bmi, Bmo, Bmu; \
    unsigned long long Bsa, Bse, Bsi, Bso, Bsu; \
    unsigned long long Ca, Ce, Ci, Co, Cu;      \
    unsigned long long Da, De, Di, Do, Du;      \
    unsigned long long Eba, Ebe, Ebi, Ebo, Ebu; \
    unsigned long long Ega, Ege, Egi, Ego, Egu; \
    unsigned long long Eka, Eke, Eki, Eko, Eku; \
    unsigned long long Ema, Eme, Emi, Emo, Emu; \
    unsigned long long Esa, Ese, Esi, Eso, Esu;

#define copyFromStateScalar(state) \
    Aba = state[0];          \
    Abe = state[1];          \
    Abi = state[2];          \
    Abo = state[3];          \
    Abu = state[4];          \
    Aga = state[5];          \
    Age = state[6];          \
    Agi = state[7];          \
    Ago = state[8];          \
    Agu = state[9];          \
    Aka = state[10];         \
    Ake = state[11];         \
    Aki = state[12];         \
    Ako = state[13];         \
    Aku = state[14];         \
    Ama = state[15];         \
    Ame = state[16];         \
    Ami = state[17];         \
    Amo = state[18];         \
    Amu = state[19];         \
    Asa = state[20];         \
    Ase = state[21];         \
    Asi = state[22];         \
    Aso = state[23];         \
    Asu = state[24];

#define copyToStateScalar(state) \
    state[0] = Aba;        \
    state[1] = Abe;        \
    state[2] = Abi;        \
    state[3] = Abo;        \
    state[4] = Abu;        \
    state[5] = Aga;        \
    state[6] = Age;        \
    state[7] = Agi;        \
    state[8] = Ago;        \
    state[9] = Agu;        \
    state[10] = Aka;       \
    state[11] = Ake;       \
    state[12] = Aki;       \
    state[13] = Ako;       \
    state[14] = Aku;       \
    state[15] = Ama;       \
    state[16] = Ame;       \
    state[17] = Ami;       \
    state[18] = Amo;       \
    state[19] = Amu;       \
    state[20] = Asa;       \
    state[21] = Ase;       \
    state[22] = Asi;       \
    state[23] = Aso;       \
    state[24] = Asu;

#define thetaRhoPiChiIotaPrepareThetaScalar(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstant##i; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
    Cu = E##bu; \
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    Ci ^= E##gi; \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
    Cu ^= E##gu; \
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^((~Bka)&  Bke ); \
    Cu ^= E##ku; \
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    Ci ^= E##mi; \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
    Cu ^= E##mu; \
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
    Cu ^= E##su;

#define rounds12Scalar                                                                    \
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;                                               \
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;                                               \
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;                                               \
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;                                               \
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;                                               \
    thetaRhoPiChiIotaPrepareThetaScalar(0, A, E)                                          \
        thetaRhoPiChiIotaPrepareThetaScalar(1, E, A)                                      \
            thetaRhoPiChiIotaPrepareThetaScalar(2, A, E)                                  \
                thetaRhoPiChiIotaPrepareThetaScalar(3, E, A)                              \
                    thetaRhoPiChiIotaPrepareThetaScalar(4, A, E)                          \
                        thetaRhoPiChiIotaPrepareThetaScalar(5, E, A)                      \
                            thetaRhoPiChiIotaPrepareThetaScalar(6, A, E)                  \
                                thetaRhoPiChiIotaPrepareThetaScalar(7, E, A)              \
                                    thetaRhoPiChiIotaPrepareThetaScalar(8, A, E)          \
                                        thetaRhoPiChiIotaPrepareThetaScalar(9, E, A)      \
                                            thetaRhoPiChiIotaPrepareThetaScalar(10, A, E) \
                                                Da = Cu ^ ROL64(Ce, 1);             \
    De = Ca ^ ROL64(Ci, 1);                                                         \
    Di = Ce ^ ROL64(Co, 1);                                                         \
    Do = Ci ^ ROL64(Cu, 1);                                                         \
    Du = Co ^ ROL64(Ca, 1);                                                         \
    Eba ^= Da;                                                                      \
    Bba = Eba;                                                                      \
    Ege ^= De;                                                                      \
    Bbe = ROL64(Ege, 44);                                                           \
    Eki ^= Di;                                                                      \
    Bbi = ROL64(Eki, 43);                                                           \
    Emo ^= Do;                                                                      \
    Bbo = ROL64(Emo, 21);                                                           \
    Esu ^= Du;                                                                      \
    Bbu = ROL64(Esu, 14);                                                           \
    Aba = Bba ^ ((~Bbe) & Bbi);                                                     \
    Aba ^= 0x8000000080008008ULL;                                                   \
    Abe = Bbe ^ ((~Bbi) & Bbo);                                                     \
    Abi = Bbi ^ ((~Bbo) & Bbu);                                                     \
    Abo = Bbo ^ ((~Bbu) & Bba);                                                     \
    Abu = Bbu ^ ((~Bba) & Bbe);                                                     \
    Ebo ^= Do;                                                                      \
    Bga = ROL64(Ebo, 28);                                                           \
    Egu ^= Du;                                                                      \
    Bge = ROL64(Egu, 20);                                                           \
    Eka ^= Da;                                                                      \
    Bgi = ROL64(Eka, 3);                                                            \
    Eme ^= De;                                                                      \
    Bgo = ROL64(Eme, 45);                                                           \
    Esi ^= Di;                                                                      \
    Bgu = ROL64(Esi, 61);                                                           \
    Aga = Bga ^ ((~Bge) & Bgi);                                                     \
    Age = Bge ^ ((~Bgi) & Bgo);                                                     \
    Agi = Bgi ^ ((~Bgo) & Bgu);                                                     \
    Ago = Bgo ^ ((~Bgu) & Bga);                                                     \
    Agu = Bgu ^ ((~Bga) & Bge);                                                     \
    Ebe ^= De;                                                                      \
    Bka = ROL64(Ebe, 1);                                                            \
    Egi ^= Di;                                                                      \
    Bke = ROL64(Egi, 6);                                                            \
    Eko ^= Do;                                                                      \
    Bki = ROL64(Eko, 25);                                                           \
    Emu ^= Du;                                                                      \
    Bko = ROL64(Emu, 8);                                                            \
    Esa ^= Da;                                                                      \
    Bku = ROL64(Esa, 18);                                                           \
    Aka = Bka ^ ((~Bke) & Bki);                                                     \
    Ake = Bke ^ ((~Bki) & Bko);                                                     \
    Aki = Bki ^ ((~Bko) & Bku);                                                     \
    Ako = Bko ^ ((~Bku) & Bka);                                                     \
    Aku = Bku ^ ((~Bka) & Bke);                                                     \
    Ebu ^= Du;                                                                      \
    Bma = ROL64(Ebu, 27);                                                           \
    Ega ^= Da;                                                                      \
    Bme = ROL64(Ega, 36);                                                           \
    Eke ^= De;                                                                      \
    Bmi = ROL64(Eke, 10);                                                           \
    Emi ^= Di;                                                                      \
    Bmo = ROL64(Emi, 15);                                                           \
    Eso ^= Do;                                                                      \
    Bmu = ROL64(Eso, 56);                                                           \
    Ama = Bma ^ ((~Bme) & Bmi);                                                     \
    Ame = Bme ^ ((~Bmi) & Bmo);                                                     \
    Ami = Bmi ^ ((~Bmo) & Bmu);                                                     \
    Amo = Bmo ^ ((~Bmu) & Bma);                                                     \
    Amu = Bmu ^ ((~Bma) & Bme);                                                     \
    Ebi ^= Di;                                                                      \
    Bsa = ROL64(Ebi, 62);                                                           \
    Ego ^= Do;                                                                      \
    Bse = ROL64(Ego, 55);                                                           \
    Eku ^= Du;                                                                      \
    Bsi = ROL64(Eku, 39);                                                           \
    Ema ^= Da;                                                                      \
    Bso = ROL64(Ema, 41);                                                           \
    Ese ^= De;                                                                      \
    Bsu = ROL64(Ese, 2);                                                            \
    Asa = Bsa ^ ((~Bse) & Bsi);                                                     \
    Ase = Bse ^ ((~Bsi) & Bso);                                                     \
    Asi = Bsi ^ ((~Bso) & Bsu);                                                     \
    Aso = Bso ^ ((~Bsu) & Bsa);                                                     \
    Asu = Bsu ^ ((~Bsa) & Bse);

#if defined(_MSC_VER)
#define ROL64(a, offset) _rotl64(a, offset)
#else
#define ROL64(a, offset) ((((unsigned long long)a) << offset) ^ (((unsigned long long)a) >> (64 - offset)))
#endif

#if defined (__AVX512F__) && !GENERIC_K12
static __m512i zero, moveThetaPrev, moveThetaNext, rhoB, rhoG, rhoK, rhoM, rhoS, pi1B, pi1G, pi1K, pi1M, pi1S, pi2S1, pi2S2, pi2BG, pi2KM, pi2S3, padding;
static __m512i K12RoundConst0, K12RoundConst1, K12RoundConst2, K12RoundConst3, K12RoundConst4, K12RoundConst5, K12RoundConst6, K12RoundConst7, K12RoundConst8, K12RoundConst9, K12RoundConst10, K12RoundConst11;

static void initAVX512KangarooTwelveConstants()
{
    zero = _mm512_maskz_set1_epi64(0, 0);
    moveThetaPrev = _mm512_setr_epi64(4, 0, 1, 2, 3, 5, 6, 7);
    moveThetaNext = _mm512_setr_epi64(1, 2, 3, 4, 0, 5, 6, 7);
    rhoB = _mm512_setr_epi64(0, 1, 62, 28, 27, 0, 0, 0);
    rhoG = _mm512_setr_epi64(36, 44, 6, 55, 20, 0, 0, 0);
    rhoK = _mm512_setr_epi64(3, 10, 43, 25, 39, 0, 0, 0);
    rhoM = _mm512_setr_epi64(41, 45, 15, 21, 8, 0, 0, 0);
    rhoS = _mm512_setr_epi64(18, 2, 61, 56, 14, 0, 0, 0);
    pi1B = _mm512_setr_epi64(0, 3, 1, 4, 2, 5, 6, 7);
    pi1G = _mm512_setr_epi64(1, 4, 2, 0, 3, 5, 6, 7);
    pi1K = _mm512_setr_epi64(2, 0, 3, 1, 4, 5, 6, 7);
    pi1M = _mm512_setr_epi64(3, 1, 4, 2, 0, 5, 6, 7);
    pi1S = _mm512_setr_epi64(4, 2, 0, 3, 1, 5, 6, 7);
    pi2S1 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 8, 10);
    pi2S2 = _mm512_setr_epi64(0, 1, 2, 3, 4, 5, 9, 11);
    pi2BG = _mm512_setr_epi64(0, 1, 8, 9, 6, 5, 6, 7);
    pi2KM = _mm512_setr_epi64(2, 3, 10, 11, 7, 5, 6, 7);
    pi2S3 = _mm512_setr_epi64(4, 5, 12, 13, 4, 5, 6, 7);
    padding = _mm512_maskz_set1_epi64(1, 0x8000000000000000);

    K12RoundConst0 = _mm512_maskz_set1_epi64(1, 0x000000008000808bULL);
    K12RoundConst1 = _mm512_maskz_set1_epi64(1, 0x800000000000008bULL);
    K12RoundConst2 = _mm512_maskz_set1_epi64(1, 0x8000000000008089ULL);
    K12RoundConst3 = _mm512_maskz_set1_epi64(1, 0x8000000000008003ULL);
    K12RoundConst4 = _mm512_maskz_set1_epi64(1, 0x8000000000008002ULL);
    K12RoundConst5 = _mm512_maskz_set1_epi64(1, 0x8000000000000080ULL);
    K12RoundConst6 = _mm512_maskz_set1_epi64(1, 0x000000000000800aULL);
    K12RoundConst7 = _mm512_maskz_set1_epi64(1, 0x800000008000000aULL);
    K12RoundConst8 = _mm512_maskz_set1_epi64(1, 0x8000000080008081ULL);
    K12RoundConst9 = _mm512_maskz_set1_epi64(1, 0x8000000000008080ULL);
    K12RoundConst10 = _mm512_maskz_set1_epi64(1, 0x0000000080000001ULL);
    K12RoundConst11 = _mm512_maskz_set1_epi64(1, 0x8000000080008008ULL);
}

#else
#define declareABCDE \
    unsigned long long Aba, Abe, Abi, Abo, Abu; \
    unsigned long long Aga, Age, Agi, Ago, Agu; \
    unsigned long long Aka, Ake, Aki, Ako, Aku; \
    unsigned long long Ama, Ame, Ami, Amo, Amu; \
    unsigned long long Asa, Ase, Asi, Aso, Asu; \
    unsigned long long Bba, Bbe, Bbi, Bbo, Bbu; \
    unsigned long long Bga, Bge, Bgi, Bgo, Bgu; \
    unsigned long long Bka, Bke, Bki, Bko, Bku; \
    unsigned long long Bma, Bme, Bmi, Bmo, Bmu; \
    unsigned long long Bsa, Bse, Bsi, Bso, Bsu; \
    unsigned long long Ca, Ce, Ci, Co, Cu; \
    unsigned long long Da, De, Di, Do, Du; \
    unsigned long long Eba, Ebe, Ebi, Ebo, Ebu; \
    unsigned long long Ega, Ege, Egi, Ego, Egu; \
    unsigned long long Eka, Eke, Eki, Eko, Eku; \
    unsigned long long Ema, Eme, Emi, Emo, Emu; \
    unsigned long long Esa, Ese, Esi, Eso, Esu; \

#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstant##i; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
    Cu = E##bu; \
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    Ci ^= E##gi; \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
    Cu ^= E##gu; \
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^((~Bka)&  Bke ); \
    Cu ^= E##ku; \
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    Ci ^= E##mi; \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
    Cu ^= E##mu; \
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
    Cu ^= E##su;

#define copyFromState(state) \
    Aba = state[ 0]; \
    Abe = state[ 1]; \
    Abi = state[ 2]; \
    Abo = state[ 3]; \
    Abu = state[ 4]; \
    Aga = state[ 5]; \
    Age = state[ 6]; \
    Agi = state[ 7]; \
    Ago = state[ 8]; \
    Agu = state[ 9]; \
    Aka = state[10]; \
    Ake = state[11]; \
    Aki = state[12]; \
    Ako = state[13]; \
    Aku = state[14]; \
    Ama = state[15]; \
    Ame = state[16]; \
    Ami = state[17]; \
    Amo = state[18]; \
    Amu = state[19]; \
    Asa = state[20]; \
    Ase = state[21]; \
    Asi = state[22]; \
    Aso = state[23]; \
    Asu = state[24];

#define copyToState(state) \
    state[ 0] = Aba; \
    state[ 1] = Abe; \
    state[ 2] = Abi; \
    state[ 3] = Abo; \
    state[ 4] = Abu; \
    state[ 5] = Aga; \
    state[ 6] = Age; \
    state[ 7] = Agi; \
    state[ 8] = Ago; \
    state[ 9] = Agu; \
    state[10] = Aka; \
    state[11] = Ake; \
    state[12] = Aki; \
    state[13] = Ako; \
    state[14] = Aku; \
    state[15] = Ama; \
    state[16] = Ame; \
    state[17] = Ami; \
    state[18] = Amo; \
    state[19] = Amu; \
    state[20] = Asa; \
    state[21] = Ase; \
    state[22] = Asi; \
    state[23] = Aso; \
    state[24] = Asu;

#define rounds12 \
    Ca = Aba^Aga^Aka^Ama^Asa; \
    Ce = Abe^Age^Ake^Ame^Ase; \
    Ci = Abi^Agi^Aki^Ami^Asi; \
    Co = Abo^Ago^Ako^Amo^Aso; \
    Cu = Abu^Agu^Aku^Amu^Asu; \
    thetaRhoPiChiIotaPrepareTheta(0, A, E) \
    thetaRhoPiChiIotaPrepareTheta(1, E, A) \
    thetaRhoPiChiIotaPrepareTheta(2, A, E) \
    thetaRhoPiChiIotaPrepareTheta(3, E, A) \
    thetaRhoPiChiIotaPrepareTheta(4, A, E) \
    thetaRhoPiChiIotaPrepareTheta(5, E, A) \
    thetaRhoPiChiIotaPrepareTheta(6, A, E) \
    thetaRhoPiChiIotaPrepareTheta(7, E, A) \
    thetaRhoPiChiIotaPrepareTheta(8, A, E) \
    thetaRhoPiChiIotaPrepareTheta(9, E, A) \
    thetaRhoPiChiIotaPrepareTheta(10, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
    Eba ^= Da; \
    Bba = Eba; \
    Ege ^= De; \
    Bbe = ROL64(Ege, 44); \
    Eki ^= Di; \
    Bbi = ROL64(Eki, 43); \
    Emo ^= Do; \
    Bbo = ROL64(Emo, 21); \
    Esu ^= Du; \
    Bbu = ROL64(Esu, 14); \
    Aba =   Bba ^((~Bbe)&  Bbi ); \
    Aba ^= 0x8000000080008008ULL; \
    Abe =   Bbe ^((~Bbi)&  Bbo ); \
    Abi =   Bbi ^((~Bbo)&  Bbu ); \
    Abo =   Bbo ^((~Bbu)&  Bba ); \
    Abu =   Bbu ^((~Bba)&  Bbe ); \
    Ebo ^= Do; \
    Bga = ROL64(Ebo, 28); \
    Egu ^= Du; \
    Bge = ROL64(Egu, 20); \
    Eka ^= Da; \
    Bgi = ROL64(Eka, 3); \
    Eme ^= De; \
    Bgo = ROL64(Eme, 45); \
    Esi ^= Di; \
    Bgu = ROL64(Esi, 61); \
    Aga =   Bga ^((~Bge)&  Bgi ); \
    Age =   Bge ^((~Bgi)&  Bgo ); \
    Agi =   Bgi ^((~Bgo)&  Bgu ); \
    Ago =   Bgo ^((~Bgu)&  Bga ); \
    Agu =   Bgu ^((~Bga)&  Bge ); \
    Ebe ^= De; \
    Bka = ROL64(Ebe, 1); \
    Egi ^= Di; \
    Bke = ROL64(Egi, 6); \
    Eko ^= Do; \
    Bki = ROL64(Eko, 25); \
    Emu ^= Du; \
    Bko = ROL64(Emu, 8); \
    Esa ^= Da; \
    Bku = ROL64(Esa, 18); \
    Aka =   Bka ^((~Bke)&  Bki ); \
    Ake =   Bke ^((~Bki)&  Bko ); \
    Aki =   Bki ^((~Bko)&  Bku ); \
    Ako =   Bko ^((~Bku)&  Bka ); \
    Aku =   Bku ^((~Bka)&  Bke ); \
    Ebu ^= Du; \
    Bma = ROL64(Ebu, 27); \
    Ega ^= Da; \
    Bme = ROL64(Ega, 36); \
    Eke ^= De; \
    Bmi = ROL64(Eke, 10); \
    Emi ^= Di; \
    Bmo = ROL64(Emi, 15); \
    Eso ^= Do; \
    Bmu = ROL64(Eso, 56); \
    Ama =   Bma ^((~Bme)&  Bmi ); \
    Ame =   Bme ^((~Bmi)&  Bmo ); \
    Ami =   Bmi ^((~Bmo)&  Bmu ); \
    Amo =   Bmo ^((~Bmu)&  Bma ); \
    Amu =   Bmu ^((~Bma)&  Bme ); \
    Ebi ^= Di; \
    Bsa = ROL64(Ebi, 62); \
    Ego ^= Do; \
    Bse = ROL64(Ego, 55); \
    Eku ^= Du; \
    Bsi = ROL64(Eku, 39); \
    Ema ^= Da; \
    Bso = ROL64(Ema, 41); \
    Ese ^= De; \
    Bsu = ROL64(Ese, 2); \
    Asa =   Bsa ^((~Bse)&  Bsi ); \
    Ase =   Bse ^((~Bsi)&  Bso ); \
    Asi =   Bsi ^((~Bso)&  Bsu ); \
    Aso =   Bso ^((~Bsu)&  Bsa ); \
    Asu =   Bsu ^((~Bsa)&  Bse );
#endif

#define K12_security        128
#define K12_capacity        (2 * K12_security)
#define K12_capacityInBytes (K12_capacity / 8)
#define K12_rateInBytes     ((1600 - K12_capacity) / 8)
#define K12_chunkSize       8192
#define K12_suffixLeaf      0x0B

typedef struct
{
    unsigned char state[200];
    unsigned char byteIOIndex;
} KangarooTwelve_F;

static void KeccakP1600_Permute_12rounds(unsigned char* state)
{
#if defined (__AVX512F__) && !GENERIC_K12
    __m512i Baeiou = _mm512_maskz_loadu_epi64(0x1F, state);
    __m512i Gaeiou = _mm512_maskz_loadu_epi64(0x1F, state + 40);
    __m512i Kaeiou = _mm512_maskz_loadu_epi64(0x1F, state + 80);
    __m512i Maeiou = _mm512_maskz_loadu_epi64(0x1F, state + 120);
    __m512i Saeiou = _mm512_maskz_loadu_epi64(0x1F, state + 160);
    __m512i b0, b1, b2, b3, b4, b5;

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst0);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst1);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst2);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst3);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst4);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst5);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst6);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst7);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst8);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst9);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst10);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst11);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    _mm512_mask_storeu_epi64(state, 0x1F, Baeiou);
    _mm512_mask_storeu_epi64(state + 40, 0x1F, Gaeiou);
    _mm512_mask_storeu_epi64(state + 80, 0x1F, Kaeiou);
    _mm512_mask_storeu_epi64(state + 120, 0x1F, Maeiou);
    _mm512_mask_storeu_epi64(state + 160, 0x1F, Saeiou);
#else
    declareABCDE
        unsigned long long* stateAsLanes = (unsigned long long*)state;
    copyFromState(stateAsLanes)
        rounds12
        copyToState(stateAsLanes)
#endif
}

static void KangarooTwelve_F_Absorb(KangarooTwelve_F* instance, const unsigned char* data, unsigned long long dataByteLen)
{
    unsigned long long i = 0;
    while (i < dataByteLen)
    {
        if (!instance->byteIOIndex && dataByteLen >= i + K12_rateInBytes)
        {
#if defined (__AVX512F__) && !GENERIC_K12
            __m512i Baeiou = _mm512_maskz_loadu_epi64(0x1F, instance->state);
            __m512i Gaeiou = _mm512_maskz_loadu_epi64(0x1F, instance->state + 40);
            __m512i Kaeiou = _mm512_maskz_loadu_epi64(0x1F, instance->state + 80);
            __m512i Maeiou = _mm512_maskz_loadu_epi64(0x1F, instance->state + 120);
            __m512i Saeiou = _mm512_maskz_loadu_epi64(0x1F, instance->state + 160);
#else
            declareABCDE
                unsigned long long* stateAsLanes = (unsigned long long*)instance->state;
            copyFromState(stateAsLanes)
#endif
                unsigned long long modifiedDataByteLen = dataByteLen - i;
            while (modifiedDataByteLen >= K12_rateInBytes)
            {
#if defined (__AVX512F__) && !GENERIC_K12
                Baeiou = _mm512_xor_si512(Baeiou, _mm512_maskz_loadu_epi64(0x1F, data));
                Gaeiou = _mm512_xor_si512(Gaeiou, _mm512_maskz_loadu_epi64(0x1F, data + 40));
                Kaeiou = _mm512_xor_si512(Kaeiou, _mm512_maskz_loadu_epi64(0x1F, data + 80));
                Maeiou = _mm512_xor_si512(Maeiou, _mm512_maskz_loadu_epi64(0x1F, data + 120));
                Saeiou = _mm512_xor_si512(Saeiou, _mm512_maskz_loadu_epi64(0x01, data + 160));
                __m512i b0, b1, b2, b3, b4, b5;

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst0);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst1);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst2);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst3);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst4);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst5);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst6);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst7);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst8);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst9);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst10);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

                b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
                b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
                b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
                b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
                b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
                b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
                b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
                b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
                Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst11);
                Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
                Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
                Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
                Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
                b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
                b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
                b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
                b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
                Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
                Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
                Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
                Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
                Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);
#else
                Aba ^= ((unsigned long long*)data)[0];
                Abe ^= ((unsigned long long*)data)[1];
                Abi ^= ((unsigned long long*)data)[2];
                Abo ^= ((unsigned long long*)data)[3];
                Abu ^= ((unsigned long long*)data)[4];
                Aga ^= ((unsigned long long*)data)[5];
                Age ^= ((unsigned long long*)data)[6];
                Agi ^= ((unsigned long long*)data)[7];
                Ago ^= ((unsigned long long*)data)[8];
                Agu ^= ((unsigned long long*)data)[9];
                Aka ^= ((unsigned long long*)data)[10];
                Ake ^= ((unsigned long long*)data)[11];
                Aki ^= ((unsigned long long*)data)[12];
                Ako ^= ((unsigned long long*)data)[13];
                Aku ^= ((unsigned long long*)data)[14];
                Ama ^= ((unsigned long long*)data)[15];
                Ame ^= ((unsigned long long*)data)[16];
                Ami ^= ((unsigned long long*)data)[17];
                Amo ^= ((unsigned long long*)data)[18];
                Amu ^= ((unsigned long long*)data)[19];
                Asa ^= ((unsigned long long*)data)[20];
                rounds12
#endif
                    data += K12_rateInBytes;
                modifiedDataByteLen -= K12_rateInBytes;
            }
#if defined (__AVX512F__) && !GENERIC_K12
            _mm512_mask_storeu_epi64(instance->state, 0x1F, Baeiou);
            _mm512_mask_storeu_epi64(instance->state + 40, 0x1F, Gaeiou);
            _mm512_mask_storeu_epi64(instance->state + 80, 0x1F, Kaeiou);
            _mm512_mask_storeu_epi64(instance->state + 120, 0x1F, Maeiou);
            _mm512_mask_storeu_epi64(instance->state + 160, 0x1F, Saeiou);
#else
            copyToState(stateAsLanes)
#endif
                i = dataByteLen - modifiedDataByteLen;
        }
        else
        {
            unsigned char partialBlock;
            if ((dataByteLen - i) + instance->byteIOIndex > K12_rateInBytes)
            {
                partialBlock = K12_rateInBytes - instance->byteIOIndex;
            }
            else
            {
                partialBlock = (unsigned char)(dataByteLen - i);
            }
            i += partialBlock;

            if (!instance->byteIOIndex)
            {
                unsigned int j = 0;
                for (; (j + 8) <= (unsigned int)(partialBlock >> 3); j += 8)
                {
                    ((unsigned long long*)instance->state)[j + 0] ^= ((unsigned long long*)data)[j + 0];
                    ((unsigned long long*)instance->state)[j + 1] ^= ((unsigned long long*)data)[j + 1];
                    ((unsigned long long*)instance->state)[j + 2] ^= ((unsigned long long*)data)[j + 2];
                    ((unsigned long long*)instance->state)[j + 3] ^= ((unsigned long long*)data)[j + 3];
                    ((unsigned long long*)instance->state)[j + 4] ^= ((unsigned long long*)data)[j + 4];
                    ((unsigned long long*)instance->state)[j + 5] ^= ((unsigned long long*)data)[j + 5];
                    ((unsigned long long*)instance->state)[j + 6] ^= ((unsigned long long*)data)[j + 6];
                    ((unsigned long long*)instance->state)[j + 7] ^= ((unsigned long long*)data)[j + 7];
                }
                for (; (j + 4) <= (unsigned int)(partialBlock >> 3); j += 4)
                {
                    ((unsigned long long*)instance->state)[j + 0] ^= ((unsigned long long*)data)[j + 0];
                    ((unsigned long long*)instance->state)[j + 1] ^= ((unsigned long long*)data)[j + 1];
                    ((unsigned long long*)instance->state)[j + 2] ^= ((unsigned long long*)data)[j + 2];
                    ((unsigned long long*)instance->state)[j + 3] ^= ((unsigned long long*)data)[j + 3];
                }
                for (; (j + 2) <= (unsigned int)(partialBlock >> 3); j += 2)
                {
                    ((unsigned long long*)instance->state)[j + 0] ^= ((unsigned long long*)data)[j + 0];
                    ((unsigned long long*)instance->state)[j + 1] ^= ((unsigned long long*)data)[j + 1];
                }
                if (j < (unsigned int)(partialBlock >> 3))
                {
                    ((unsigned long long*)instance->state)[j + 0] ^= ((unsigned long long*)data)[j + 0];
                }
                if (partialBlock & 7)
                {
                    unsigned long long lane = 0;
                    copyMem(&lane, data + (partialBlock & 0xFFFFFFF8), partialBlock & 7);
                    ((unsigned long long*)instance->state)[partialBlock >> 3] ^= lane;
                }
            }
            else
            {
                unsigned int _sizeLeft = partialBlock;
                unsigned int _lanePosition = instance->byteIOIndex >> 3;
                unsigned int _offsetInLane = instance->byteIOIndex & 7;
                const unsigned char* _curData = data;
                while (_sizeLeft > 0)
                {
                    unsigned int _bytesInLane = 8 - _offsetInLane;
                    if (_bytesInLane > _sizeLeft)
                    {
                        _bytesInLane = _sizeLeft;
                    }
                    if (_bytesInLane)
                    {
                        unsigned long long lane = 0;
                        copyMem(&lane, (void*)_curData, _bytesInLane);
                        ((unsigned long long*)instance->state)[_lanePosition] ^= (lane << (_offsetInLane << 3));
                    }
                    _sizeLeft -= _bytesInLane;
                    _lanePosition++;
                    _offsetInLane = 0;
                    _curData += _bytesInLane;
                }
            }

            data += partialBlock;
            instance->byteIOIndex += partialBlock;
            if (instance->byteIOIndex == K12_rateInBytes)
            {
                KeccakP1600_Permute_12rounds(instance->state);
                instance->byteIOIndex = 0;
            }
        }
    }
}

static void KangarooTwelve(const unsigned char* input, unsigned int inputByteLen, unsigned char* output, unsigned int outputByteLen)
{
    KangarooTwelve_F queueNode;
    KangarooTwelve_F finalNode;
    unsigned int blockNumber, queueAbsorbedLen;

    setMem(&finalNode, sizeof(KangarooTwelve_F), 0);
    const unsigned int len = inputByteLen ^ ((K12_chunkSize ^ inputByteLen) & -(K12_chunkSize < inputByteLen));
    KangarooTwelve_F_Absorb(&finalNode, input, len);
    input += len;
    inputByteLen -= len;
    if (len == K12_chunkSize && inputByteLen)
    {
        blockNumber = 1;
        queueAbsorbedLen = 0;
        finalNode.state[finalNode.byteIOIndex] ^= 0x03;
        if (++finalNode.byteIOIndex == K12_rateInBytes)
        {
            KeccakP1600_Permute_12rounds(finalNode.state);
            finalNode.byteIOIndex = 0;
        }
        else
        {
            finalNode.byteIOIndex = (finalNode.byteIOIndex + 7) & ~7;
        }

        while (inputByteLen > 0)
        {
            const unsigned int len = K12_chunkSize ^ ((inputByteLen ^ K12_chunkSize) & -(inputByteLen < K12_chunkSize));
            setMem(&queueNode, sizeof(KangarooTwelve_F), 0);
            KangarooTwelve_F_Absorb(&queueNode, input, len);
            input += len;
            inputByteLen -= len;
            if (len == K12_chunkSize)
            {
                ++blockNumber;
                queueNode.state[queueNode.byteIOIndex] ^= K12_suffixLeaf;
                queueNode.state[K12_rateInBytes - 1] ^= 0x80;
                KeccakP1600_Permute_12rounds(queueNode.state);
                queueNode.byteIOIndex = K12_capacityInBytes;
                KangarooTwelve_F_Absorb(&finalNode, queueNode.state, K12_capacityInBytes);
            }
            else
            {
                queueAbsorbedLen = len;
            }
        }

        if (queueAbsorbedLen)
        {
            if (++queueNode.byteIOIndex == K12_rateInBytes)
            {
                KeccakP1600_Permute_12rounds(queueNode.state);
                queueNode.byteIOIndex = 0;
            }
            if (++queueAbsorbedLen == K12_chunkSize)
            {
                ++blockNumber;
                queueAbsorbedLen = 0;
                queueNode.state[queueNode.byteIOIndex] ^= K12_suffixLeaf;
                queueNode.state[K12_rateInBytes - 1] ^= 0x80;
                KeccakP1600_Permute_12rounds(queueNode.state);
                queueNode.byteIOIndex = K12_capacityInBytes;
                KangarooTwelve_F_Absorb(&finalNode, queueNode.state, K12_capacityInBytes);
            }
        }
        else
        {
            setMem(queueNode.state, sizeof(queueNode.state), 0);
            queueNode.byteIOIndex = 1;
            queueAbsorbedLen = 1;
        }
    }
    else
    {
        if (len == K12_chunkSize)
        {
            blockNumber = 1;
            finalNode.state[finalNode.byteIOIndex] ^= 0x03;
            if (++finalNode.byteIOIndex == K12_rateInBytes)
            {
                KeccakP1600_Permute_12rounds(finalNode.state);
                finalNode.byteIOIndex = 0;
            }
            else
            {
                finalNode.byteIOIndex = (finalNode.byteIOIndex + 7) & ~7;
            }

            setMem(queueNode.state, sizeof(queueNode.state), 0);
            queueNode.byteIOIndex = 1;
            queueAbsorbedLen = 1;
        }
        else
        {
            blockNumber = 0;
            if (++finalNode.byteIOIndex == K12_rateInBytes)
            {
                KeccakP1600_Permute_12rounds(finalNode.state);
                finalNode.state[0] ^= 0x07;
            }
            else
            {
                finalNode.state[finalNode.byteIOIndex] ^= 0x07;
            }
        }
    }

    if (blockNumber)
    {
        if (queueAbsorbedLen)
        {
            blockNumber++;
            queueNode.state[queueNode.byteIOIndex] ^= K12_suffixLeaf;
            queueNode.state[K12_rateInBytes - 1] ^= 0x80;
            KeccakP1600_Permute_12rounds(queueNode.state);
            KangarooTwelve_F_Absorb(&finalNode, queueNode.state, K12_capacityInBytes);
        }
        unsigned int n = 0;
        for (unsigned long long v = --blockNumber; v && (n < sizeof(unsigned long long)); ++n, v >>= 8)
        {
        }
        unsigned char encbuf[sizeof(unsigned long long) + 1 + 2];
        for (unsigned int i = 1; i <= n; ++i)
        {
            encbuf[i - 1] = (unsigned char)(blockNumber >> (8 * (n - i)));
        }
        encbuf[n] = (unsigned char)n;
        encbuf[++n] = 0xFF;
        encbuf[++n] = 0xFF;
        KangarooTwelve_F_Absorb(&finalNode, encbuf, ++n);
        finalNode.state[finalNode.byteIOIndex] ^= 0x06;
    }
    finalNode.state[K12_rateInBytes - 1] ^= 0x80;
    KeccakP1600_Permute_12rounds(finalNode.state);
    copyMem(output, finalNode.state, outputByteLen);
}

static inline void KangarooTwelve(const void* input, unsigned int inputByteLen, void* output, unsigned int outputByteLen)
{
    KangarooTwelve((const unsigned char*)input, inputByteLen, (unsigned char*)output, outputByteLen);
}

static void KangarooTwelve64To32(const unsigned char* input, unsigned char* output)
{
#if defined (__AVX512F__) && !GENERIC_K12
    __m512i Baeiou = _mm512_maskz_loadu_epi64(0x1F, input);
    __m512i Gaeiou = _mm512_set_epi64(0, 0, 0, 0, 0x0700, ((unsigned long long*)input)[7], ((unsigned long long*)input)[6], ((unsigned long long*)input)[5]);

    __m512i b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, zero, 0x96), zero, padding, 0x96);
    __m512i b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    __m512i b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(zero, b0, b1, 0x96), rhoK));
    __m512i b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(zero, b0, b1, 0x96), rhoM));
    __m512i b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(padding, b0, b1, 0x96), rhoS));
    __m512i b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst0);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    __m512i Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    __m512i Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    __m512i Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst1);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst2);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst3);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst4);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst5);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst6);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst7);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst8);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst9);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));
    Baeiou = _mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst10);
    Gaeiou = _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2);
    Kaeiou = _mm512_ternarylogic_epi64(b2, b3, b4, 0xD2);
    Maeiou = _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2);
    Saeiou = _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2);
    b0 = _mm512_permutex2var_epi64(_mm512_unpacklo_epi64(Baeiou, Gaeiou), pi2S1, Saeiou);
    b2 = _mm512_permutex2var_epi64(_mm512_unpackhi_epi64(Baeiou, Gaeiou), pi2S2, Saeiou);
    b1 = _mm512_unpacklo_epi64(Kaeiou, Maeiou);
    b3 = _mm512_unpackhi_epi64(Kaeiou, Maeiou);
    Baeiou = _mm512_permutex2var_epi64(b0, pi2BG, b1);
    Gaeiou = _mm512_permutex2var_epi64(b2, pi2BG, b3);
    Kaeiou = _mm512_permutex2var_epi64(b0, pi2KM, b1);
    Maeiou = _mm512_permutex2var_epi64(b2, pi2KM, b3);
    Saeiou = _mm512_mask_blend_epi64(0x10, _mm512_permutex2var_epi64(b0, pi2S3, b1), Saeiou);

    b0 = _mm512_ternarylogic_epi64(_mm512_ternarylogic_epi64(Baeiou, Gaeiou, Kaeiou, 0x96), Maeiou, Saeiou, 0x96);
    b1 = _mm512_permutexvar_epi64(moveThetaPrev, b0);
    b0 = _mm512_rol_epi64(_mm512_permutexvar_epi64(moveThetaNext, b0), 1);
    b2 = _mm512_permutexvar_epi64(pi1K, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Kaeiou, b0, b1, 0x96), rhoK));
    b3 = _mm512_permutexvar_epi64(pi1M, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Maeiou, b0, b1, 0x96), rhoM));
    b4 = _mm512_permutexvar_epi64(pi1S, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Saeiou, b0, b1, 0x96), rhoS));
    b5 = _mm512_permutexvar_epi64(pi1G, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Gaeiou, b0, b1, 0x96), rhoG));
    b0 = _mm512_permutexvar_epi64(pi1B, _mm512_rolv_epi64(_mm512_ternarylogic_epi64(Baeiou, b0, b1, 0x96), rhoB));

    _mm512_mask_storeu_epi64(output, 0xF, _mm512_permutex2var_epi64(_mm512_permutex2var_epi64(_mm512_unpacklo_epi64(_mm512_xor_si512(_mm512_ternarylogic_epi64(b0, b5, b2, 0xD2), K12RoundConst11), _mm512_ternarylogic_epi64(b5, b2, b3, 0xD2)), pi2S1, _mm512_ternarylogic_epi64(b4, b0, b5, 0xD2)), pi2BG, _mm512_unpacklo_epi64(_mm512_ternarylogic_epi64(b2, b3, b4, 0xD2), _mm512_ternarylogic_epi64(b3, b4, b0, 0xD2))));
#else
    unsigned long long Aba, Abe, Abi, Abo, Abu;
    unsigned long long Aga, Age, Agi, Ago, Agu;
    unsigned long long Aka, Ake, Aki, Ako, Aku;
    unsigned long long Ama, Ame, Ami, Amo, Amu;
    unsigned long long Asa, Ase, Asi, Aso, Asu;
    unsigned long long Bba, Bbe, Bbi, Bbo, Bbu;
    unsigned long long Bga, Bge, Bgi, Bgo, Bgu;
    unsigned long long Bka, Bke, Bki, Bko, Bku;
    unsigned long long Bma, Bme, Bmi, Bmo, Bmu;
    unsigned long long Bsa, Bse, Bsi, Bso, Bsu;
    unsigned long long Ca, Ce, Ci, Co, Cu;
    unsigned long long Da, De, Di, Do, Du;
    unsigned long long Eba, Ebe, Ebi, Ebo, Ebu;
    unsigned long long Ega, Ege, Egi, Ego, Egu;
    unsigned long long Eka, Eke, Eki, Eko, Eku;
    unsigned long long Ema, Eme, Emi, Emo, Emu;
    unsigned long long Esa, Ese, Esi, Eso, Esu;

    Ca = ((unsigned long long*)input)[0] ^ ((unsigned long long*)input)[5] ^ 0x8000000000000000;
    Ce = ((unsigned long long*)input)[1] ^ ((unsigned long long*)input)[6];
    Ci = ((unsigned long long*)input)[2] ^ ((unsigned long long*)input)[7];
    Co = ((unsigned long long*)input)[3] ^ 0x0700;

    Da = ((unsigned long long*)input)[4] ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(((unsigned long long*)input)[4], 1);
    Du = Co ^ ROL64(Ca, 1);
    Aba = ((unsigned long long*)input)[0] ^ Da;
    Bbe = ROL64(((unsigned long long*)input)[6] ^ De, 44);
    Bbi = ROL64(Di, 43);
    Bbo = ROL64(Do, 21);
    Bbu = ROL64(Du, 14);
    Eba = Aba ^ _andn_u64(Bbe, Bbi) ^ 0x000000008000808bULL;
    Ebe = Bbe ^ _andn_u64(Bbi, Bbo);
    Ebi = Bbi ^ _andn_u64(Bbo, Bbu);
    Ebo = Bbo ^ _andn_u64(Bbu, Aba);
    Ebu = Bbu ^ _andn_u64(Aba, Bbe);
    Bga = ROL64(((unsigned long long*)input)[3] ^ Do, 28);
    Bge = ROL64(Du, 20);
    Bgi = ROL64(Da, 3);
    Bgo = ROL64(De, 45);
    Bgu = ROL64(Di, 61);
    Ega = Bga ^ _andn_u64(Bge, Bgi);
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Egi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ego = Bgo ^ _andn_u64(Bgu, Bga);
    Egu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(((unsigned long long*)input)[1] ^ De, 1);
    Bke = ROL64(((unsigned long long*)input)[7] ^ Di, 6);
    Bki = ROL64(Do, 25);
    Bko = ROL64(Du, 8);
    Bku = ROL64(Da ^ 0x8000000000000000, 18);
    Eka = Bka ^ _andn_u64(Bke, Bki);
    Eke = Bke ^ _andn_u64(Bki, Bko);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Eko = Bko ^ _andn_u64(Bku, Bka);
    Eku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(((unsigned long long*)input)[4] ^ Du, 27);
    Bme = ROL64(((unsigned long long*)input)[5] ^ Da, 36);
    Bmi = ROL64(De, 10);
    Bmo = ROL64(Di, 15);
    Bmu = ROL64(Do, 56);
    Ema = Bma ^ _andn_u64(Bme, Bmi);
    Eme = Bme ^ _andn_u64(Bmi, Bmo);
    Emi = Bmi ^ _andn_u64(Bmo, Bmu);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Emu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(((unsigned long long*)input)[2] ^ Di, 62);
    Bse = ROL64(Do ^ 0x0700, 55);
    Bsi = ROL64(Du, 39);
    Bso = ROL64(Da, 41);
    Bsu = ROL64(De, 2);
    Esa = Bsa ^ _andn_u64(Bse, Bsi);
    Ese = Bse ^ _andn_u64(Bsi, Bso);
    Esi = Bsi ^ _andn_u64(Bso, Bsu);
    Eso = Bso ^ _andn_u64(Bsu, Bsa);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    Ce = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    Ci = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    Co = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    Cu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Eba ^= Da;
    Bbe = ROL64(Ege ^ De, 44);
    Bbi = ROL64(Eki ^ Di, 43);
    Bbo = ROL64(Emo ^ Do, 21);
    Bbu = ROL64(Esu ^ Du, 14);
    Aba = Eba ^ _andn_u64(Bbe, Bbi) ^ 0x800000000000008bULL;
    Abe = Bbe ^ _andn_u64(Bbi, Bbo);
    Abi = Bbi ^ _andn_u64(Bbo, Bbu);
    Abo = Bbo ^ _andn_u64(Bbu, Eba);
    Abu = Bbu ^ _andn_u64(Eba, Bbe);
    Bga = ROL64(Ebo ^ Do, 28);
    Bge = ROL64(Egu ^ Du, 20);
    Bgi = ROL64(Eka ^ Da, 3);
    Bgo = ROL64(Eme ^ De, 45);
    Bgu = ROL64(Esi ^ Di, 61);
    Aga = Bga ^ _andn_u64(Bge, Bgi);
    Age = Bge ^ _andn_u64(Bgi, Bgo);
    Agi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ago = Bgo ^ _andn_u64(Bgu, Bga);
    Agu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Ebe ^ De, 1);
    Bke = ROL64(Egi ^ Di, 6);
    Bki = ROL64(Eko ^ Do, 25);
    Bko = ROL64(Emu ^ Du, 8);
    Bku = ROL64(Esa ^ Da, 18);
    Aka = Bka ^ _andn_u64(Bke, Bki);
    Ake = Bke ^ _andn_u64(Bki, Bko);
    Aki = Bki ^ _andn_u64(Bko, Bku);
    Ako = Bko ^ _andn_u64(Bku, Bka);
    Aku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Ebu ^ Du, 27);
    Bme = ROL64(Ega ^ Da, 36);
    Bmi = ROL64(Eke ^ De, 10);
    Bmo = ROL64(Emi ^ Di, 15);
    Bmu = ROL64(Eso ^ Do, 56);
    Ama = Bma ^ _andn_u64(Bme, Bmi);
    Ame = Bme ^ _andn_u64(Bmi, Bmo);
    Ami = Bmi ^ _andn_u64(Bmo, Bmu);
    Amo = Bmo ^ _andn_u64(Bmu, Bma);
    Amu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Ebi ^ Di, 62);
    Bse = ROL64(Ego ^ Do, 55);
    Bsi = ROL64(Eku ^ Du, 39);
    Bso = ROL64(Ema ^ Da, 41);
    Bsu = ROL64(Ese ^ De, 2);
    Asa = Bsa ^ _andn_u64(Bse, Bsi);
    Ase = Bse ^ _andn_u64(Bsi, Bso);
    Asi = Bsi ^ _andn_u64(Bso, Bsu);
    Aso = Bso ^ _andn_u64(Bsu, Bsa);
    Asu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Aba ^= Da;
    Bbe = ROL64(Age ^ De, 44);
    Bbi = ROL64(Aki ^ Di, 43);
    Bbo = ROL64(Amo ^ Do, 21);
    Bbu = ROL64(Asu ^ Du, 14);
    Eba = Aba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000000008089ULL;
    Ebe = Bbe ^ _andn_u64(Bbi, Bbo);
    Ebi = Bbi ^ _andn_u64(Bbo, Bbu);
    Ebo = Bbo ^ _andn_u64(Bbu, Aba);
    Ebu = Bbu ^ _andn_u64(Aba, Bbe);
    Bga = ROL64(Abo ^ Do, 28);
    Bge = ROL64(Agu ^ Du, 20);
    Bgi = ROL64(Aka ^ Da, 3);
    Bgo = ROL64(Ame ^ De, 45);
    Bgu = ROL64(Asi ^ Di, 61);
    Ega = Bga ^ _andn_u64(Bge, Bgi);
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Egi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ego = Bgo ^ _andn_u64(Bgu, Bga);
    Egu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Abe ^ De, 1);
    Bke = ROL64(Agi ^ Di, 6);
    Bki = ROL64(Ako ^ Do, 25);
    Bko = ROL64(Amu ^ Du, 8);
    Bku = ROL64(Asa ^ Da, 18);
    Eka = Bka ^ _andn_u64(Bke, Bki);
    Eke = Bke ^ _andn_u64(Bki, Bko);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Eko = Bko ^ _andn_u64(Bku, Bka);
    Eku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Abu ^ Du, 27);
    Bme = ROL64(Aga ^ Da, 36);
    Bmi = ROL64(Ake ^ De, 10);
    Bmo = ROL64(Ami ^ Di, 15);
    Bmu = ROL64(Aso ^ Do, 56);
    Ema = Bma ^ _andn_u64(Bme, Bmi);
    Eme = Bme ^ _andn_u64(Bmi, Bmo);
    Emi = Bmi ^ _andn_u64(Bmo, Bmu);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Emu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Abi ^ Di, 62);
    Bse = ROL64(Ago ^ Do, 55);
    Bsi = ROL64(Aku ^ Du, 39);
    Bso = ROL64(Ama ^ Da, 41);
    Bsu = ROL64(Ase ^ De, 2);
    Esa = Bsa ^ _andn_u64(Bse, Bsi);
    Ese = Bse ^ _andn_u64(Bsi, Bso);
    Esi = Bsi ^ _andn_u64(Bso, Bsu);
    Eso = Bso ^ _andn_u64(Bsu, Bsa);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    Ce = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    Ci = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    Co = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    Cu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Eba ^= Da;
    Bbe = ROL64(Ege ^ De, 44);
    Bbi = ROL64(Eki ^ Di, 43);
    Bbo = ROL64(Emo ^ Do, 21);
    Bbu = ROL64(Esu ^ Du, 14);
    Aba = Eba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000000008003ULL;
    Abe = Bbe ^ _andn_u64(Bbi, Bbo);
    Abi = Bbi ^ _andn_u64(Bbo, Bbu);
    Abo = Bbo ^ _andn_u64(Bbu, Eba);
    Abu = Bbu ^ _andn_u64(Eba, Bbe);
    Bga = ROL64(Ebo ^ Do, 28);
    Bge = ROL64(Egu ^ Du, 20);
    Bgi = ROL64(Eka ^ Da, 3);
    Bgo = ROL64(Eme ^ De, 45);
    Bgu = ROL64(Esi ^ Di, 61);
    Aga = Bga ^ _andn_u64(Bge, Bgi);
    Age = Bge ^ _andn_u64(Bgi, Bgo);
    Agi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ago = Bgo ^ _andn_u64(Bgu, Bga);
    Agu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Ebe ^ De, 1);
    Bke = ROL64(Egi ^ Di, 6);
    Bki = ROL64(Eko ^ Do, 25);
    Bko = ROL64(Emu ^ Du, 8);
    Bku = ROL64(Esa ^ Da, 18);
    Aka = Bka ^ _andn_u64(Bke, Bki);
    Ake = Bke ^ _andn_u64(Bki, Bko);
    Aki = Bki ^ _andn_u64(Bko, Bku);
    Ako = Bko ^ _andn_u64(Bku, Bka);
    Aku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Ebu ^ Du, 27);
    Bme = ROL64(Ega ^ Da, 36);
    Bmi = ROL64(Eke ^ De, 10);
    Bmo = ROL64(Emi ^ Di, 15);
    Bmu = ROL64(Eso ^ Do, 56);
    Ama = Bma ^ _andn_u64(Bme, Bmi);
    Ame = Bme ^ _andn_u64(Bmi, Bmo);
    Ami = Bmi ^ _andn_u64(Bmo, Bmu);
    Amo = Bmo ^ _andn_u64(Bmu, Bma);
    Amu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Ebi ^ Di, 62);
    Bse = ROL64(Ego ^ Do, 55);
    Bsi = ROL64(Eku ^ Du, 39);
    Bso = ROL64(Ema ^ Da, 41);
    Bsu = ROL64(Ese ^ De, 2);
    Asa = Bsa ^ _andn_u64(Bse, Bsi);
    Ase = Bse ^ _andn_u64(Bsi, Bso);
    Asi = Bsi ^ _andn_u64(Bso, Bsu);
    Aso = Bso ^ _andn_u64(Bsu, Bsa);
    Asu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Aba ^= Da;
    Bbe = ROL64(Age ^ De, 44);
    Bbi = ROL64(Aki ^ Di, 43);
    Bbo = ROL64(Amo ^ Do, 21);
    Bbu = ROL64(Asu ^ Du, 14);
    Eba = Aba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000000008002ULL;
    Ebe = Bbe ^ _andn_u64(Bbi, Bbo);
    Ebi = Bbi ^ _andn_u64(Bbo, Bbu);
    Ebo = Bbo ^ _andn_u64(Bbu, Aba);
    Ebu = Bbu ^ _andn_u64(Aba, Bbe);
    Bga = ROL64(Abo ^ Do, 28);
    Bge = ROL64(Agu ^ Du, 20);
    Bgi = ROL64(Aka ^ Da, 3);
    Bgo = ROL64(Ame ^ De, 45);
    Bgu = ROL64(Asi ^ Di, 61);
    Ega = Bga ^ _andn_u64(Bge, Bgi);
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Egi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ego = Bgo ^ _andn_u64(Bgu, Bga);
    Egu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Abe ^ De, 1);
    Bke = ROL64(Agi ^ Di, 6);
    Bki = ROL64(Ako ^ Do, 25);
    Bko = ROL64(Amu ^ Du, 8);
    Bku = ROL64(Asa ^ Da, 18);
    Eka = Bka ^ _andn_u64(Bke, Bki);
    Eke = Bke ^ _andn_u64(Bki, Bko);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Eko = Bko ^ _andn_u64(Bku, Bka);
    Eku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Abu ^ Du, 27);
    Bme = ROL64(Aga ^ Da, 36);
    Bmi = ROL64(Ake ^ De, 10);
    Bmo = ROL64(Ami ^ Di, 15);
    Bmu = ROL64(Aso ^ Do, 56);
    Ema = Bma ^ _andn_u64(Bme, Bmi);
    Eme = Bme ^ _andn_u64(Bmi, Bmo);
    Emi = Bmi ^ _andn_u64(Bmo, Bmu);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Emu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Abi ^ Di, 62);
    Bse = ROL64(Ago ^ Do, 55);
    Bsi = ROL64(Aku ^ Du, 39);
    Bso = ROL64(Ama ^ Da, 41);
    Bsu = ROL64(Ase ^ De, 2);
    Esa = Bsa ^ _andn_u64(Bse, Bsi);
    Ese = Bse ^ _andn_u64(Bsi, Bso);
    Esi = Bsi ^ _andn_u64(Bso, Bsu);
    Eso = Bso ^ _andn_u64(Bsu, Bsa);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    Ce = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    Ci = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    Co = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    Cu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Eba ^= Da;
    Bbe = ROL64(Ege ^ De, 44);
    Bbi = ROL64(Eki ^ Di, 43);
    Bbo = ROL64(Emo ^ Do, 21);
    Bbu = ROL64(Esu ^ Du, 14);
    Aba = Eba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000000000080ULL;
    Abe = Bbe ^ _andn_u64(Bbi, Bbo);
    Abi = Bbi ^ _andn_u64(Bbo, Bbu);
    Abo = Bbo ^ _andn_u64(Bbu, Eba);
    Abu = Bbu ^ _andn_u64(Eba, Bbe);
    Bga = ROL64(Ebo ^ Do, 28);
    Bge = ROL64(Egu ^ Du, 20);
    Bgi = ROL64(Eka ^ Da, 3);
    Bgo = ROL64(Eme ^ De, 45);
    Bgu = ROL64(Esi ^ Di, 61);
    Aga = Bga ^ _andn_u64(Bge, Bgi);
    Age = Bge ^ _andn_u64(Bgi, Bgo);
    Agi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ago = Bgo ^ _andn_u64(Bgu, Bga);
    Agu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Ebe ^ De, 1);
    Bke = ROL64(Egi ^ Di, 6);
    Bki = ROL64(Eko ^ Do, 25);
    Bko = ROL64(Emu ^ Du, 8);
    Bku = ROL64(Esa ^ Da, 18);
    Aka = Bka ^ _andn_u64(Bke, Bki);
    Ake = Bke ^ _andn_u64(Bki, Bko);
    Aki = Bki ^ _andn_u64(Bko, Bku);
    Ako = Bko ^ _andn_u64(Bku, Bka);
    Aku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Ebu ^ Du, 27);
    Bme = ROL64(Ega ^ Da, 36);
    Bmi = ROL64(Eke ^ De, 10);
    Bmo = ROL64(Emi ^ Di, 15);
    Bmu = ROL64(Eso ^ Do, 56);
    Ama = Bma ^ _andn_u64(Bme, Bmi);
    Ame = Bme ^ _andn_u64(Bmi, Bmo);
    Ami = Bmi ^ _andn_u64(Bmo, Bmu);
    Amo = Bmo ^ _andn_u64(Bmu, Bma);
    Amu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Ebi ^ Di, 62);
    Bse = ROL64(Ego ^ Do, 55);
    Bsi = ROL64(Eku ^ Du, 39);
    Bso = ROL64(Ema ^ Da, 41);
    Bsu = ROL64(Ese ^ De, 2);
    Asa = Bsa ^ _andn_u64(Bse, Bsi);
    Ase = Bse ^ _andn_u64(Bsi, Bso);
    Asi = Bsi ^ _andn_u64(Bso, Bsu);
    Aso = Bso ^ _andn_u64(Bsu, Bsa);
    Asu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Aba ^= Da;
    Bbe = ROL64(Age ^ De, 44);
    Bbi = ROL64(Aki ^ Di, 43);
    Bbo = ROL64(Amo ^ Do, 21);
    Bbu = ROL64(Asu ^ Du, 14);
    Eba = Aba ^ _andn_u64(Bbe, Bbi) ^ 0x000000000000800aULL;
    Ebe = Bbe ^ _andn_u64(Bbi, Bbo);
    Ebi = Bbi ^ _andn_u64(Bbo, Bbu);
    Ebo = Bbo ^ _andn_u64(Bbu, Aba);
    Ebu = Bbu ^ _andn_u64(Aba, Bbe);
    Bga = ROL64(Abo ^ Do, 28);
    Bge = ROL64(Agu ^ Du, 20);
    Bgi = ROL64(Aka ^ Da, 3);
    Bgo = ROL64(Ame ^ De, 45);
    Bgu = ROL64(Asi ^ Di, 61);
    Ega = Bga ^ _andn_u64(Bge, Bgi);
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Egi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ego = Bgo ^ _andn_u64(Bgu, Bga);
    Egu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Abe ^ De, 1);
    Bke = ROL64(Agi ^ Di, 6);
    Bki = ROL64(Ako ^ Do, 25);
    Bko = ROL64(Amu ^ Du, 8);
    Bku = ROL64(Asa ^ Da, 18);
    Eka = Bka ^ _andn_u64(Bke, Bki);
    Eke = Bke ^ _andn_u64(Bki, Bko);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Eko = Bko ^ _andn_u64(Bku, Bka);
    Eku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Abu ^ Du, 27);
    Bme = ROL64(Aga ^ Da, 36);
    Bmi = ROL64(Ake ^ De, 10);
    Bmo = ROL64(Ami ^ Di, 15);
    Bmu = ROL64(Aso ^ Do, 56);
    Ema = Bma ^ _andn_u64(Bme, Bmi);
    Eme = Bme ^ _andn_u64(Bmi, Bmo);
    Emi = Bmi ^ _andn_u64(Bmo, Bmu);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Emu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Abi ^ Di, 62);
    Bse = ROL64(Ago ^ Do, 55);
    Bsi = ROL64(Aku ^ Du, 39);
    Bso = ROL64(Ama ^ Da, 41);
    Bsu = ROL64(Ase ^ De, 2);
    Esa = Bsa ^ _andn_u64(Bse, Bsi);
    Ese = Bse ^ _andn_u64(Bsi, Bso);
    Esi = Bsi ^ _andn_u64(Bso, Bsu);
    Eso = Bso ^ _andn_u64(Bsu, Bsa);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    Ce = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    Ci = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    Co = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    Cu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Eba ^= Da;
    Bbe = ROL64(Ege ^ De, 44);
    Bbi = ROL64(Eki ^ Di, 43);
    Bbo = ROL64(Emo ^ Do, 21);
    Bbu = ROL64(Esu ^ Du, 14);
    Aba = Eba ^ _andn_u64(Bbe, Bbi) ^ 0x800000008000000aULL;
    Abe = Bbe ^ _andn_u64(Bbi, Bbo);
    Abi = Bbi ^ _andn_u64(Bbo, Bbu);
    Abo = Bbo ^ _andn_u64(Bbu, Eba);
    Abu = Bbu ^ _andn_u64(Eba, Bbe);
    Bga = ROL64(Ebo ^ Do, 28);
    Bge = ROL64(Egu ^ Du, 20);
    Bgi = ROL64(Eka ^ Da, 3);
    Bgo = ROL64(Eme ^ De, 45);
    Bgu = ROL64(Esi ^ Di, 61);
    Aga = Bga ^ _andn_u64(Bge, Bgi);
    Age = Bge ^ _andn_u64(Bgi, Bgo);
    Agi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ago = Bgo ^ _andn_u64(Bgu, Bga);
    Agu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Ebe ^ De, 1);
    Bke = ROL64(Egi ^ Di, 6);
    Bki = ROL64(Eko ^ Do, 25);
    Bko = ROL64(Emu ^ Du, 8);
    Bku = ROL64(Esa ^ Da, 18);
    Aka = Bka ^ _andn_u64(Bke, Bki);
    Ake = Bke ^ _andn_u64(Bki, Bko);
    Aki = Bki ^ _andn_u64(Bko, Bku);
    Ako = Bko ^ _andn_u64(Bku, Bka);
    Aku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Ebu ^ Du, 27);
    Bme = ROL64(Ega ^ Da, 36);
    Bmi = ROL64(Eke ^ De, 10);
    Bmo = ROL64(Emi ^ Di, 15);
    Bmu = ROL64(Eso ^ Do, 56);
    Ama = Bma ^ _andn_u64(Bme, Bmi);
    Ame = Bme ^ _andn_u64(Bmi, Bmo);
    Ami = Bmi ^ _andn_u64(Bmo, Bmu);
    Amo = Bmo ^ _andn_u64(Bmu, Bma);
    Amu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Ebi ^ Di, 62);
    Bse = ROL64(Ego ^ Do, 55);
    Bsi = ROL64(Eku ^ Du, 39);
    Bso = ROL64(Ema ^ Da, 41);
    Bsu = ROL64(Ese ^ De, 2);
    Asa = Bsa ^ _andn_u64(Bse, Bsi);
    Ase = Bse ^ _andn_u64(Bsi, Bso);
    Asi = Bsi ^ _andn_u64(Bso, Bsu);
    Aso = Bso ^ _andn_u64(Bsu, Bsa);
    Asu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Aba ^= Da;
    Bbe = ROL64(Age ^ De, 44);
    Bbi = ROL64(Aki ^ Di, 43);
    Bbo = ROL64(Amo ^ Do, 21);
    Bbu = ROL64(Asu ^ Du, 14);
    Eba = Aba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000080008081ULL;
    Ebe = Bbe ^ _andn_u64(Bbi, Bbo);
    Ebi = Bbi ^ _andn_u64(Bbo, Bbu);
    Ebo = Bbo ^ _andn_u64(Bbu, Aba);
    Ebu = Bbu ^ _andn_u64(Aba, Bbe);
    Bga = ROL64(Abo ^ Do, 28);
    Bge = ROL64(Agu ^ Du, 20);
    Bgi = ROL64(Aka ^ Da, 3);
    Bgo = ROL64(Ame ^ De, 45);
    Bgu = ROL64(Asi ^ Di, 61);
    Ega = Bga ^ _andn_u64(Bge, Bgi);
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Egi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ego = Bgo ^ _andn_u64(Bgu, Bga);
    Egu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Abe ^ De, 1);
    Bke = ROL64(Agi ^ Di, 6);
    Bki = ROL64(Ako ^ Do, 25);
    Bko = ROL64(Amu ^ Du, 8);
    Bku = ROL64(Asa ^ Da, 18);
    Eka = Bka ^ _andn_u64(Bke, Bki);
    Eke = Bke ^ _andn_u64(Bki, Bko);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Eko = Bko ^ _andn_u64(Bku, Bka);
    Eku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Abu ^ Du, 27);
    Bme = ROL64(Aga ^ Da, 36);
    Bmi = ROL64(Ake ^ De, 10);
    Bmo = ROL64(Ami ^ Di, 15);
    Bmu = ROL64(Aso ^ Do, 56);
    Ema = Bma ^ _andn_u64(Bme, Bmi);
    Eme = Bme ^ _andn_u64(Bmi, Bmo);
    Emi = Bmi ^ _andn_u64(Bmo, Bmu);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Emu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Abi ^ Di, 62);
    Bse = ROL64(Ago ^ Do, 55);
    Bsi = ROL64(Aku ^ Du, 39);
    Bso = ROL64(Ama ^ Da, 41);
    Bsu = ROL64(Ase ^ De, 2);
    Esa = Bsa ^ _andn_u64(Bse, Bsi);
    Ese = Bse ^ _andn_u64(Bsi, Bso);
    Esi = Bsi ^ _andn_u64(Bso, Bsu);
    Eso = Bso ^ _andn_u64(Bsu, Bsa);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
    Ce = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
    Ci = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
    Co = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
    Cu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Eba ^= Da;
    Bbe = ROL64(Ege ^ De, 44);
    Bbi = ROL64(Eki ^ Di, 43);
    Bbo = ROL64(Emo ^ Do, 21);
    Bbu = ROL64(Esu ^ Du, 14);
    Aba = Eba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000000008080ULL;
    Abe = Bbe ^ _andn_u64(Bbi, Bbo);
    Abi = Bbi ^ _andn_u64(Bbo, Bbu);
    Abo = Bbo ^ _andn_u64(Bbu, Eba);
    Abu = Bbu ^ _andn_u64(Eba, Bbe);
    Bga = ROL64(Ebo ^ Do, 28);
    Bge = ROL64(Egu ^ Du, 20);
    Bgi = ROL64(Eka ^ Da, 3);
    Bgo = ROL64(Eme ^ De, 45);
    Bgu = ROL64(Esi ^ Di, 61);
    Aga = Bga ^ _andn_u64(Bge, Bgi);
    Age = Bge ^ _andn_u64(Bgi, Bgo);
    Agi = Bgi ^ _andn_u64(Bgo, Bgu);
    Ago = Bgo ^ _andn_u64(Bgu, Bga);
    Agu = Bgu ^ _andn_u64(Bga, Bge);
    Bka = ROL64(Ebe ^ De, 1);
    Bke = ROL64(Egi ^ Di, 6);
    Bki = ROL64(Eko ^ Do, 25);
    Bko = ROL64(Emu ^ Du, 8);
    Bku = ROL64(Esa ^ Da, 18);
    Aka = Bka ^ _andn_u64(Bke, Bki);
    Ake = Bke ^ _andn_u64(Bki, Bko);
    Aki = Bki ^ _andn_u64(Bko, Bku);
    Ako = Bko ^ _andn_u64(Bku, Bka);
    Aku = Bku ^ _andn_u64(Bka, Bke);
    Bma = ROL64(Ebu ^ Du, 27);
    Bme = ROL64(Ega ^ Da, 36);
    Bmi = ROL64(Eke ^ De, 10);
    Bmo = ROL64(Emi ^ Di, 15);
    Bmu = ROL64(Eso ^ Do, 56);
    Ama = Bma ^ _andn_u64(Bme, Bmi);
    Ame = Bme ^ _andn_u64(Bmi, Bmo);
    Ami = Bmi ^ _andn_u64(Bmo, Bmu);
    Amo = Bmo ^ _andn_u64(Bmu, Bma);
    Amu = Bmu ^ _andn_u64(Bma, Bme);
    Bsa = ROL64(Ebi ^ Di, 62);
    Bse = ROL64(Ego ^ Do, 55);
    Bsi = ROL64(Eku ^ Du, 39);
    Bso = ROL64(Ema ^ Da, 41);
    Bsu = ROL64(Ese ^ De, 2);
    Asa = Bsa ^ _andn_u64(Bse, Bsi);
    Ase = Bse ^ _andn_u64(Bsi, Bso);
    Asi = Bsi ^ _andn_u64(Bso, Bsu);
    Aso = Bso ^ _andn_u64(Bsu, Bsa);
    Asu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
    Ce = Abe ^ Age ^ Ake ^ Ame ^ Ase;
    Ci = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
    Co = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
    Cu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

    Da = Cu ^ ROL64(Ce, 1);
    De = Ca ^ ROL64(Ci, 1);
    Di = Ce ^ ROL64(Co, 1);
    Do = Ci ^ ROL64(Cu, 1);
    Du = Co ^ ROL64(Ca, 1);
    Bba = Aba ^ Da;
    Bbe = ROL64(Age ^ De, 44);
    Bbi = ROL64(Aki ^ Di, 43);
    Bbo = ROL64(Amo ^ Do, 21);
    Bbu = ROL64(Asu ^ Du, 14);
    Bga = ROL64(Abo ^ Do, 28);
    Bge = ROL64(Agu ^ Du, 20);
    Bgi = ROL64(Aka ^ Da, 3);
    Bgo = ROL64(Ame ^ De, 45);
    Bgu = ROL64(Asi ^ Di, 61);
    Bka = ROL64(Abe ^ De, 1);
    Bke = ROL64(Agi ^ Di, 6);
    Bki = ROL64(Ako ^ Do, 25);
    Bko = ROL64(Amu ^ Du, 8);
    Bku = ROL64(Asa ^ Da, 18);
    Bma = ROL64(Abu ^ Du, 27);
    Bme = ROL64(Aga ^ Da, 36);
    Bmi = ROL64(Ake ^ De, 10);
    Bmo = ROL64(Ami ^ Di, 15);
    Bmu = ROL64(Aso ^ Do, 56);
    Bsa = ROL64(Abi ^ Di, 62);
    Bse = ROL64(Ago ^ Do, 55);
    Bsi = ROL64(Aku ^ Du, 39);
    Bso = ROL64(Ama ^ Da, 41);
    Bsu = ROL64(Ase ^ De, 2);
    Eba = Bba ^ _andn_u64(Bbe, Bbi) ^ 0x0000000080000001ULL;
    Ege = Bge ^ _andn_u64(Bgi, Bgo);
    Eki = Bki ^ _andn_u64(Bko, Bku);
    Emo = Bmo ^ _andn_u64(Bmu, Bma);
    Esu = Bsu ^ _andn_u64(Bsa, Bse);
    Ca = Eba ^ Bga ^ Bka ^ Bma ^ Bsa ^ _andn_u64(Bge, Bgi) ^ _andn_u64(Bke, Bki) ^ _andn_u64(Bme, Bmi) ^ _andn_u64(Bse, Bsi);
    Ce = Bbe ^ Ege ^ Bke ^ Bme ^ Bse ^ _andn_u64(Bbi, Bbo) ^ _andn_u64(Bki, Bko) ^ _andn_u64(Bmi, Bmo) ^ _andn_u64(Bsi, Bso);
    Ci = Bbi ^ Bgi ^ Eki ^ Bmi ^ Bsi ^ _andn_u64(Bbo, Bbu) ^ _andn_u64(Bgo, Bgu) ^ _andn_u64(Bmo, Bmu) ^ _andn_u64(Bso, Bsu);
    Co = Bbo ^ Bgo ^ Bko ^ Emo ^ Bso ^ _andn_u64(Bbu, Bba) ^ _andn_u64(Bgu, Bga) ^ _andn_u64(Bku, Bka) ^ _andn_u64(Bsu, Bsa);
    Cu = Bbu ^ Bgu ^ Bku ^ Bmu ^ Esu ^ _andn_u64(Bba, Bbe) ^ _andn_u64(Bga, Bge) ^ _andn_u64(Bka, Bke) ^ _andn_u64(Bma, Bme);

    Bba = Eba ^ Cu ^ ROL64(Ce, 1);
    Bbe = ROL64(Ege ^ Ca ^ ROL64(Ci, 1), 44);
    Bbi = ROL64(Eki ^ Ce ^ ROL64(Co, 1), 43);
    Bbo = ROL64(Emo ^ Ci ^ ROL64(Cu, 1), 21);
    Bbu = ROL64(Esu ^ Co ^ ROL64(Ca, 1), 14);
    ((unsigned long long*)output)[0] = Bba ^ _andn_u64(Bbe, Bbi) ^ 0x8000000080008008ULL;
    ((unsigned long long*)output)[1] = Bbe ^ _andn_u64(Bbi, Bbo);
    ((unsigned long long*)output)[2] = Bbi ^ _andn_u64(Bbo, Bbu);
    ((unsigned long long*)output)[3] = Bbo ^ _andn_u64(Bbu, Bba);
#endif
}

static void KangarooTwelve64To32(const void* input, void* output)
{
    KangarooTwelve64To32((const unsigned char*)input, (unsigned char*)output);
}

static void random(const unsigned char* publicKey, const unsigned char* nonce, unsigned char* output, unsigned long long outputSize)
{
    unsigned char state[200];
    *((__m256i*) & state[0]) = *((__m256i*)publicKey);
    *((__m256i*) & state[32]) = *((__m256i*)nonce);
    setMem(&state[64], sizeof(state) - 64, 0);

    for (unsigned long long i = 0; i < outputSize / sizeof(state); i++)
    {
        KeccakP1600_Permute_12rounds(state);
        copyMem(output, state, sizeof(state));
        output += sizeof(state);
    }
    if (outputSize % sizeof(state))
    {
        KeccakP1600_Permute_12rounds(state);
        copyMem(output, state, outputSize % sizeof(state));
    }
}


