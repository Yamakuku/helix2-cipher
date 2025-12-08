/**
 * @file helix2.c
 * @brief Helix-2 Stream Cipher implementation
 * 
 * Helix-2 is an educational ARX (Add-Rotate-XOR) stream cipher inspired by
 * ChaCha20 but using nested operations for higher per-operation complexity.
 * 
 * WARNING: This cipher is experimental and has NOT undergone formal 
 * cryptanalysis. It should NOT be used for production security applications.
 * For real-world use, please use ChaCha20, AES-GCM, or other standardized ciphers.
 * 
 * Security: Passes Dieharder and PractRand statistical tests (32GB)
 * 
 * Design:
 * - 256-bit key (32 bytes)
 * - 160-bit nonce (20 bytes)
 * - 64-byte blocks
 * - 2 rounds with intermediate state addition
 * - 8 operations per shuffle (4 compound + 4 simple)
 * 
 * @author Jarl "Yamakuku" Lindeneg
 * @date December 8, 2025
 * @version 2.1
 * 
 * @copyright Copyright (c) 2025 Jarl "Yamakuku" Lindeneg
 * @license MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "helix2.h"

#define _HELIX2_EXPORT

// Internal helper function declarations
void _helix2_initialize_keystream(helix2_context_t* context);
static inline void _helix2_shuffle(uint32_t *state, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static inline uint32_t _rotl32(uint32_t x, int n);
static inline uint32_t _pack4(const uint8_t *a);

// Exposed functions
// Initialize the Helix-2 context with key and nonce
HELIX2_API void helix2_initialize_context(helix2_context_t* context, const uint8_t* key, uint8_t* nonce) {
    // Prepare the context
    memset(context, 0, sizeof(helix2_context_t));
    memcpy(context->key, key, sizeof(context->key));            // copy 32 x 8-bit key
    memcpy(context->nonce, nonce, sizeof(context->nonce));      // copy 20 x 8-bit nonce

    const uint8_t *magic_constant = (uint8_t*)"so!M4g1c";       // every little code needs some magic
    context->keystream.state[0] = _pack4(&magic_constant[0]);   // use pack4 to convert 8 bytes to 2 uint32_t
    context->keystream.state[1] = _pack4(&magic_constant[4]);   

    // Pack key using _pack4 (since key comes as bytes)
    for (int i = 0; i < 8; i++) {
        context->keystream.state[2 + i] = _pack4(&context->key[i * 4]);
    }

	context->keystream.state[10] = 0;                           // This will hold the block index, assigned later
    // Pack nonce using _pack4 (since nonce comes as bytes)
	context->keystream.state[11] = _pack4(&context->nonce[0]);  // The high bits of the block index, will be XORed here later
	context->keystream.state[12] = _pack4(&context->nonce[4]);
	context->keystream.state[13] = _pack4(&context->nonce[8]);    
    context->keystream.state[14] = _pack4(&context->nonce[12]);    
    context->keystream.state[15] = _pack4(&context->nonce[16]);
  
    context->keystream.last_block_index = 0;                    // We will start by preparing block 0
    helix2_buffer_set_next_block(context, 0);
}

// Encrypt/Decrypt a single byte at a given offset
HELIX2_API uint8_t helix2_byte(helix2_context_t* context, uint64_t offset, uint8_t byte) {
    // Calculate the block index and offset within the block
    context->keystream.current_block_index  = offset / HELIX2_KEYSTREAM_SIZE;
    uint8_t block_offset = offset % HELIX2_KEYSTREAM_SIZE;

    // Generate the keystream if needed
    if (context->keystream.last_block_index != context->keystream.current_block_index) {
        helix2_buffer_set_next_block(context, context->keystream.current_block_index);
    }

    // Encrypt/Decrypt the byte using the generated keystream
    uint8_t *keystream_bytes = (uint8_t *)context->keystream.stream;
    uint8_t result = byte ^ keystream_bytes[block_offset];

    return result;
}


// Encrypt/Decrypt a buffer starting from a given offset
//   The buffer offset always starts at offset 0 within the provided buffer.
//   The start_offset is the offset in the keystream where the buffer processing should begins.
HELIX2_API void helix2_buffer(helix2_context_t* context, uint8_t* buffer, size_t size, uint64_t start_offset) {
    // Calculate starting block and offset within that block
    Helix2_Block_Index current_block = start_offset / HELIX2_KEYSTREAM_SIZE;
    uint8_t block_offset = start_offset % HELIX2_KEYSTREAM_SIZE;

    // Generate the keystream if needed
    if (context->keystream.last_block_index != current_block) {
        helix2_buffer_set_next_block(context, current_block);
    }

    uint8_t *keystream_bytes = (uint8_t *)context->keystream.stream;
    for (size_t i = 0; i < size; i++) {
        // Do I need to calculate a new keystream block?
        if (block_offset >= HELIX2_KEYSTREAM_SIZE) {
            current_block++;
            block_offset = 0;
            helix2_buffer_set_next_block(context, current_block);
        }
        
        // Encrypt/Decrypt the byte
        buffer[i] ^= keystream_bytes[block_offset];
        
        // Move to next byte in keystream
        block_offset++;
    }
}


// Internal helper functions
// Standard rotate left for 32-bit integers
static inline uint32_t _rotl32(uint32_t x, int n) 
{
	return (x << n) | (x >> (32 - n));
}

// Standard pack 4 bytes into a uint32_t (little-endian)
static inline uint32_t _pack4(const uint8_t *a) {
	uint32_t res = 0;
	res |= (uint32_t)a[0] << 0 * 8;
	res |= (uint32_t)a[1] << 1 * 8;
	res |= (uint32_t)a[2] << 2 * 8;
	res |= (uint32_t)a[3] << 3 * 8;
	return res;
}

// Set the keystream block index
void helix2_buffer_set_next_block(helix2_context_t* context, Helix2_Block_Index block_index) {
    context->keystream.current_block_index = block_index;
    _helix2_initialize_keystream(context);       
}

// Set the keystream to the next sequential block
void helix2_buffer_next_block(helix2_context_t* context) {
    helix2_buffer_set_next_block(context, context->keystream.current_block_index + 1);
}

// The core Helix2 keystream operations
static inline void _helix2_shuffle(uint32_t *state, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    
    state[c] += _rotl32(((state[a] ^ state[b]) + state[d]), 9);
    state[d] ^= _rotl32(((state[b] + state[c]) ^ state[a]), 13);
    state[a] += _rotl32(((state[c] ^ state[d]) + state[b]), 18);
    state[b] ^= _rotl32(((state[d] + state[a]) ^ state[c]), 22);

    state[c] ^= _rotl32((state[a] + state[b]), 7);
    state[d] += _rotl32((state[b] ^ state[c]), 21);
    state[a] ^= _rotl32((state[c] + state[d]), 11);
    state[b] += _rotl32((state[d] ^ state[a]), 16);

}

// Build the keystream for the current block index
void _helix2_initialize_keystream(helix2_context_t* context) {

    // Update the block index in the state
    context->keystream.state[10] = (uint32_t)(context->keystream.current_block_index & 0xFFFFFFFF);
    context->keystream.state[11] = _pack4(&context->nonce[0]) ^ (uint32_t)((context->keystream.current_block_index >> 32) & 0xFFFFFFFF);

    // Initialize the stream with the current state
    memcpy(context->keystream.stream, context->keystream.state, HELIX2_KEYSTREAM_SIZE);

    // Round 1
    _helix2_shuffle(context->keystream.stream, 0, 1, 2, 3);
    _helix2_shuffle(context->keystream.stream, 4, 5, 6, 7);
    _helix2_shuffle(context->keystream.stream, 8, 9, 10, 11);
    _helix2_shuffle(context->keystream.stream, 12, 13, 14, 15);

    _helix2_shuffle(context->keystream.stream, 0, 4, 8, 12);
    _helix2_shuffle(context->keystream.stream, 1, 5, 9, 13);
    _helix2_shuffle(context->keystream.stream, 2, 6, 10, 14);
    _helix2_shuffle(context->keystream.stream, 3, 7, 11, 15);        

    _helix2_shuffle(context->keystream.stream, 0, 5, 10, 15);
    _helix2_shuffle(context->keystream.stream, 1, 6, 11, 12);
    _helix2_shuffle(context->keystream.stream, 2, 7, 8, 13);
    _helix2_shuffle(context->keystream.stream, 3, 4, 9, 14);

    // Add the original state to the stream
    context->keystream.stream[0] += context->keystream.state[0];   context->keystream.stream[1] += context->keystream.state[1];
    context->keystream.stream[2] += context->keystream.state[2];   context->keystream.stream[3] += context->keystream.state[3];
    context->keystream.stream[4] += context->keystream.state[4];   context->keystream.stream[5] += context->keystream.state[5];
    context->keystream.stream[6] += context->keystream.state[6];   context->keystream.stream[7] += context->keystream.state[7];
    context->keystream.stream[8] += context->keystream.state[8];   context->keystream.stream[9] += context->keystream.state[9];
    context->keystream.stream[10] += context->keystream.state[10]; context->keystream.stream[11] += context->keystream.state[11];
    context->keystream.stream[12] += context->keystream.state[12]; context->keystream.stream[13] += context->keystream.state[13];
    context->keystream.stream[14] += context->keystream.state[14]; context->keystream.stream[15] += context->keystream.state[15];

    // Round 2
    _helix2_shuffle(context->keystream.stream, 0, 1, 2, 3);
    _helix2_shuffle(context->keystream.stream, 4, 5, 6, 7);
    _helix2_shuffle(context->keystream.stream, 8, 9, 10, 11);
    _helix2_shuffle(context->keystream.stream, 12, 13, 14, 15);

    _helix2_shuffle(context->keystream.stream, 0, 4, 8, 12);
    _helix2_shuffle(context->keystream.stream, 1, 5, 9, 13);
    _helix2_shuffle(context->keystream.stream, 2, 6, 10, 14);
    _helix2_shuffle(context->keystream.stream, 3, 7, 11, 15);        

    _helix2_shuffle(context->keystream.stream, 0, 5, 10, 15);
    _helix2_shuffle(context->keystream.stream, 1, 6, 11, 12);
    _helix2_shuffle(context->keystream.stream, 2, 7, 8, 13);
    _helix2_shuffle(context->keystream.stream, 3, 4, 9, 14);

    // Add the original state to the stream, round 2
    context->keystream.stream[0] += context->keystream.state[0];   context->keystream.stream[1] += context->keystream.state[1];
    context->keystream.stream[2] += context->keystream.state[2];   context->keystream.stream[3] += context->keystream.state[3];
    context->keystream.stream[4] += context->keystream.state[4];   context->keystream.stream[5] += context->keystream.state[5];
    context->keystream.stream[6] += context->keystream.state[6];   context->keystream.stream[7] += context->keystream.state[7];
    context->keystream.stream[8] += context->keystream.state[8];   context->keystream.stream[9] += context->keystream.state[9];
    context->keystream.stream[10] += context->keystream.state[10]; context->keystream.stream[11] += context->keystream.state[11];
    context->keystream.stream[12] += context->keystream.state[12]; context->keystream.stream[13] += context->keystream.state[13];
    context->keystream.stream[14] += context->keystream.state[14]; context->keystream.stream[15] += context->keystream.state[15];

    // Set last block index
    context->keystream.last_block_index = context->keystream.current_block_index;    
}