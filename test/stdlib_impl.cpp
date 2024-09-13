// Implements NO_UEFI versions of several functions that require cstdlib functions.
// Having them in a separate cpp file avoids the name clash between cstdlib's system and Qubic's system.

#include <cstring>
#include <cstdlib>

void setMem(void* buffer, unsigned long long size, unsigned char value)
{
    memset(buffer, value, size);
}

void copyMem(void* destination, const void* source, unsigned long long length)
{
    memcpy(destination, source, length);
}

bool allocatePool(unsigned long long size, void** buffer)
{
    void* ptr = malloc(size);
    if (ptr)
    {
        *buffer = ptr;
        return true;
    }
    return false;
}

void freePool(void* buffer)
{
    free(buffer);
}
