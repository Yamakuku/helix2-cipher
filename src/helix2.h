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
typedef uint64_t    Helix2_Block_Index;
typedef struct
{
    uint32_t stream[HELIX2_KEYSTREAM_SIZE / sizeof(uint32_t)];
    uint32_t state[HELIX2_KEYSTREAM_SIZE / sizeof(uint32_t)];
    Helix2_Block_Index current_block_index;
    Helix2_Block_Index last_block_index;
} helix2_internal_keystream_t;
typedef struct
{
    uint8_t nonce[20];
    uint8_t key[32];
    helix2_internal_keystream_t keystream;
} helix2_context_t;

// Exported functions
HELIX2_API void helix2_initialize_context(helix2_context_t* context, const uint8_t* key, uint8_t* nonce);
HELIX2_API uint8_t helix2_byte(helix2_context_t* context, uint64_t offset, uint8_t byte);
HELIX2_API void helix2_buffer(helix2_context_t* context, uint8_t* buffer, size_t size, uint64_t start_offset);
HELIX2_API void helix2_buffer_next_block(helix2_context_t* context);
HELIX2_API void helix2_buffer_set_next_block(helix2_context_t* context, Helix2_Block_Index block_index);
#endif