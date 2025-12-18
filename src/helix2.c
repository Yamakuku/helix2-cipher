/**
 * @file helix2.c
 * @brief Helix2 Stream Cipher implementation
 * 
 * Helix2-cipher is an educational ARX (Add-Rotate-XOR) stream cipher inspired by
 * ChaCha20 but using nested operations for higher per-operation complexity.
 * 
 * WARNING: This cipher is experimental and has NOT undergone formal 
 * cryptanalysis. It should NOT be used for production security applications.
 * 
 * Initial testing: Passes Dieharder and PractRand statistical tests (32GB)
 * 
 * Design:
 * - 256-bit key (32 bytes)
 * - 160-bit nonce (20 bytes)
 * - 64-byte blocks
 * - 8 operations per shuffle (4 compound + 4 simple)
 * - 8 shuffles per round, 1st round ; Row-wise mixing (horizontal) and Diagonal mixing (cross-diffusion), 2nd round ; Column-wise mixing (vertical) and mirrored Diagonal mixing (cross-diffusion)
 * - each round with intermediate state addition
 * 
 * For more information: https://github.com/Yamakuku/helix2-cipher
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
void _helix2_initialize_keystream(helix2_context_t* context, uint64_t block_index);
static inline void _helix2_shuffle(uint32_t *state, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static inline uint32_t _rotl32(uint32_t x, int n);
static inline uint32_t _pack4(const uint8_t *a);

// Exposed functions
// Initialize the Helix2 context with key and nonce
HELIX2_API void helix2_initialize_context(helix2_context_t* context, const uint8_t* key, uint8_t* nonce) {
    // Prepare the context
    memset(context, 0, sizeof(helix2_context_t));
    memcpy(context->key, key, sizeof(context->key));            // copy 32 x 8-bit key
    memcpy(context->nonce, nonce, sizeof(context->nonce));      // copy 20 x 8-bit nonce

    const uint8_t *magic_constant = (uint8_t*)"so!M4g1c";       // every little code needs some magic
    context->state[0] = _pack4(&magic_constant[0]);   // use pack4 to convert 8 bytes to 2 uint32_t
    context->state[1] = _pack4(&magic_constant[4]);   

    // Pack key using _pack4 (since key comes as bytes)
    context->state[2] = _pack4(&context->key[0]);
    context->state[3] = _pack4(&context->key[4]);
    context->state[4] = _pack4(&context->key[8]);
    context->state[5] = _pack4(&context->key[12]);
    context->state[6] = _pack4(&context->key[16]);
    context->state[7] = _pack4(&context->key[20]);
    context->state[8] = _pack4(&context->key[24]);
    context->state[9] = _pack4(&context->key[28]);

	context->state[10] = 0;                           // This will hold the block index, assigned later
    
    // Pack nonce using _pack4 (since nonce comes as bytes)
	context->state[11] = _pack4(&context->nonce[0]);  // The high bits of the block index, will be XORed here later
	context->state[12] = _pack4(&context->nonce[4]);
	context->state[13] = _pack4(&context->nonce[8]);    
    context->state[14] = _pack4(&context->nonce[12]);    
    context->state[15] = _pack4(&context->nonce[16]);
}

// Encrypt/Decrypt a buffer starting from a given offset
//   The buffer offset always starts at offset 0 within the provided buffer.
//   The start_offset is the offset in the keystream where the buffer processing should begins.
HELIX2_API void helix2_buffer(helix2_context_t* context, uint8_t* buffer, size_t size, uint64_t start_offset) {
    // Calculate starting block and offset within that block
    Helix2_Block_Index current_block = start_offset / HELIX2_KEYSTREAM_SIZE;
    uint8_t block_offset = start_offset % HELIX2_KEYSTREAM_SIZE;

    _helix2_initialize_keystream(context, current_block);

    uint8_t *keystream_bytes = (uint8_t *)context->stream;
    for (size_t i = 0; i < size; i++) {
        // Do I need to calculate a new keystream block?
        if (block_offset >= HELIX2_KEYSTREAM_SIZE) {
            current_block++;
            block_offset = 0;
            _helix2_initialize_keystream(context, current_block);
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
void _helix2_initialize_keystream(helix2_context_t* context, uint64_t block_index) {

    // Update the block index in the state
    context->state[10] = (uint32_t)(block_index & 0xFFFFFFFF);
    context->state[11] = _pack4(&context->nonce[0]) ^ (uint32_t)((block_index >> 32) & 0xFFFFFFFF);
    // Initialize the stream with the current state
    memcpy(context->stream, context->state, HELIX2_KEYSTREAM_SIZE);


    _helix2_shuffle(context->stream, 0,  1,  2,  3);
    _helix2_shuffle(context->stream, 4,  5,  6,  7);
    _helix2_shuffle(context->stream, 8,  9,  10, 11);
    _helix2_shuffle(context->stream, 12, 13, 14, 15);      

    _helix2_shuffle(context->stream, 0, 5, 10, 15);
    _helix2_shuffle(context->stream, 1, 6, 11, 12);
    _helix2_shuffle(context->stream, 2, 7, 8,  13);
    _helix2_shuffle(context->stream, 3, 4, 9,  14);    

    // Add the original state to the stream
    context->stream[0]  += context->state[0];  context->stream[1]  += context->state[1];
    context->stream[2]  += context->state[2];  context->stream[3]  += context->state[3];
    context->stream[4]  += context->state[4];  context->stream[5]  += context->state[5];
    context->stream[6]  += context->state[6];  context->stream[7]  += context->state[7];
    context->stream[8]  += context->state[8];  context->stream[9]  += context->state[9];
    context->stream[10] += context->state[10]; context->stream[11] += context->state[11];
    context->stream[12] += context->state[12]; context->stream[13] += context->state[13];
    context->stream[14] += context->state[14]; context->stream[15] += context->state[15];

    // Round 2 (Shuffle columns and mirrored diagonal)
    _helix2_shuffle(context->stream, 0, 4, 8,  12);
    _helix2_shuffle(context->stream, 1, 5, 9,  13);
    _helix2_shuffle(context->stream, 2, 6, 10, 14);
    _helix2_shuffle(context->stream, 3, 7, 11, 15);        

    _helix2_shuffle(context->stream, 3, 6, 9,  12);
    _helix2_shuffle(context->stream, 2, 5, 8,  15);
    _helix2_shuffle(context->stream, 1, 4, 11, 14);
    _helix2_shuffle(context->stream, 0, 7, 10, 13);

    // Add the original state to the stream, round 2
    context->stream[0]  += context->state[0];  context->stream[1]  += context->state[1];
    context->stream[2]  += context->state[2];  context->stream[3]  += context->state[3];
    context->stream[4]  += context->state[4];  context->stream[5]  += context->state[5];
    context->stream[6]  += context->state[6];  context->stream[7]  += context->state[7];
    context->stream[8]  += context->state[8];  context->stream[9]  += context->state[9];
    context->stream[10] += context->state[10]; context->stream[11] += context->state[11];
    context->stream[12] += context->state[12]; context->stream[13] += context->state[13];
    context->stream[14] += context->state[14]; context->stream[15] += context->state[15];

}