#include "gtest/gtest.h"
#include "contracts/HashStore.h"

TEST(HashStore, SetAndGetHash)
{
    // Instancia o estado do contrato
    HASHSTORE state = {};

    // Cria um hash de teste
    SetHash_input setInput = {};
    for (uint32_t i = 0; i < 32; ++i)
        setInput.hash[i] = static_cast<uint8_t>(i);

    SetHash_output setOutput = {};

    // Chama o procedimento para setar o hash
    // Simula o contexto do QPI (pode ser vazio para teste simples)
    QpiContextProcedureCall qpi_proc;
    SetHash(state, setInput, setOutput, qpi_proc);

    // Consulta o hash salvo
    GetHash_input getInput = {};
    GetHash_output getOutput = {};
    QpiContextFunctionCall qpi_func;
    GetHash(state, getInput, getOutput, qpi_func);

    // Verifica se o hash retornado é igual ao que foi setado
    for (uint32_t i = 0; i < 32; ++i)
        EXPECT_EQ(getOutput.hash[i], setInput.hash[i]);
} 