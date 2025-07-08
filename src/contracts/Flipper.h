// src/contracts/Flipper.h

struct FLIPPER {
    bit value;
};

// Input/output para a função flip
struct Flip_input {};
struct Flip_output {
    bit value;
};

// Função pública para inverter o valor salvo
PUBLIC_FUNCTION(flip)
{
    state.value = !state.value;
    output.value = state.value;
}

// Registro das interfaces
void REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {
    REGISTER_USER_FUNCTION(flip, 1);
} 