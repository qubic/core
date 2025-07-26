#include "gtest/gtest.h"
#include "contracts/Flipper.h"

TEST(Flipper, FlipFunction)
{
    // Instancia o estado do contrato
    FLIPPER state = {};
    state.value = 0; // começa como falso

    // Chama flip pela primeira vez
    Flip_input input = {};
    Flip_output output = {};
    QpiContextFunctionCall qpi;
    flip(state, input, output, qpi);
    EXPECT_EQ(state.value, 1);
    EXPECT_EQ(output.value, 1);

    // Chama flip novamente
    flip(state, input, output, qpi);
    EXPECT_EQ(state.value, 0);
    EXPECT_EQ(output.value, 0);

    // Chama flip mais uma vez
    flip(state, input, output, qpi);
    EXPECT_EQ(state.value, 1);
    EXPECT_EQ(output.value, 1);
} 