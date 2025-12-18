#ifndef HELIX2_INCL_H
#define HELIX2_INCL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// DLL export/import macros
#ifdef _HELIX2_DLL
    #ifdef _HELIX2_EXPORT
        #define HELIX2_API __declspec(dllexport)
    #else
        #define HELIX2_API __declspec(dllimport)
    #endif
#else
    #define HELIX2_API
#endif

// HELIX2 definitions
#define HELIX2_KEYSTREAM_SIZE 64

typedef struct
{
    uint8_t nonce[20];
    uint8_t key[32];
    uint32_t stream[HELIX2_KEYSTREAM_SIZE / sizeof(uint32_t)];
    uint32_t state[HELIX2_KEYSTREAM_SIZE / sizeof(uint32_t)];
} helix2_context_t;

// Exported functions
HELIX2_API void helix2_initialize_context(helix2_context_t* context, const uint8_t* key, uint8_t* nonce);
HELIX2_API void helix2_buffer(helix2_context_t* context, uint8_t* buffer, size_t size, uint64_t start_offset);
#endif