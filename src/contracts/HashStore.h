// src/contracts/HashStore.h

struct HASHSTORE {
    Array<uint8, 32> hash;
};

// Input/output para o procedimento de setar o hash
struct SetHash_input {
    Array<uint8, 32> hash;
};
struct SetHash_output {};

// Input/output para a função de consultar o hash
struct GetHash_input {};
struct GetHash_output {
    Array<uint8, 32> hash;
};

// Procedimento público para setar o hash
PUBLIC_PROCEDURE(SetHash)
{
    for (uint32 i = 0; i < 32; i += 1) {
        state.hash[i] = input.hash[i];
    }
}

// Função pública para consultar o hash
PUBLIC_FUNCTION(GetHash)
{
    for (uint32 i = 0; i < 32; i += 1) {
        output.hash[i] = state.hash[i];
    }
}

// Registro das interfaces
void REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
    REGISTER_USER_PROCEDURE(SetHash, 1);
    REGISTER_USER_FUNCTION(GetHash, 2);
}