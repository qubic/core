#pragma once

#define EQUAL(a, b) (_mm256_movemask_epi8(_mm256_cmpeq_epi64(a, b)) == 0xFFFFFFFF)
