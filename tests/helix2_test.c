/**
 * @file helix2_test.c
 * @brief Helix-2 Stream Cipher implementation tests
 * 
 * Helix-2 is an educational ARX (Add-Rotate-XOR) stream cipher inspired by
 * ChaCha20 but using nested operations for higher per-operation complexity.
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

#include "../src/helix2.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

uint8_t key[32] = {
    0x78, 0x56, 0x34, 0x12,  // 0x12345678 in little-endian bytes
    0x01, 0xEF, 0xCD, 0xAB,  // 0xABCDEF01 in little-endian bytes
    0x11, 0x11, 0x11, 0x11,  // 0x11111111 in little-endian bytes
    0x22, 0x22, 0x22, 0x22,  // 0x22222222 in little-endian bytes
    0x33, 0x33, 0x33, 0x33,  // 0x33333333 in little-endian bytes
    0x44, 0x44, 0x44, 0x44,  // 0x44444444 in little-endian bytes
    0x55, 0x55, 0x55, 0x55,  // 0x55555555 in little-endian bytes
    0x66, 0x66, 0x66, 0x66   // 0x66666666 in little-endian bytes
};

void debug_print_state(helix2_context_t* context);
void debug_print_keystream(helix2_context_t* context);
static inline uint32_t _pack4(const uint8_t *a);
void test_symmetry(void);
void test_determinism(void);
void test_offset_seek(void);    
void test_entropy(void);
void test_vectors(void);
void run_all_tests(void);


void debug_print_state(helix2_context_t* context) {
    printf("State  8-bit serialized: ");
    uint8_t* bytes = (uint8_t*)context->keystream.state;
    for (int i = 0; i < 64; i++) {
        printf("%02x", bytes[i]);
        if ((i + 1) % 16 == 0) printf("\n                          ");
        else if ((i + 1) % 4 == 0) printf(" ");
    }
    printf("\nState 32-bits dump :      ");

    uint32_t* bytes32 = (uint32_t*)context->keystream.state;
    for (int i = 0; i < 16; i++) {
        printf("%08x", bytes32[i]);
        if ((i + 1) % 4 == 0) printf("\n                          ");
        else printf(" ");
    }
    printf("\n");    
}


void debug_print_keystream(helix2_context_t* context) {
    printf("Keystream  8-bit serialized: ");
    uint8_t* bytes = (uint8_t*)context->keystream.stream;
    for (int i = 0; i < 64; i++) {
        printf("%02x", bytes[i]);
        if ((i + 1) % 16 == 0) printf("\n                              ");
        else if ((i + 1) % 4 == 0) printf(" ");
    }
    printf("\nKeystream 32-bits dump :      ");

    uint32_t* bytes32 = (uint32_t*)context->keystream.stream;
    for (int i = 0; i < 16; i++) {
        printf("%08x", bytes32[i]);
        if ((i + 1) % 4 == 0) printf("\n                             ");
        printf(" ");
    }
    printf("\n");    
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

void test_symmetry(void) {
    helix2_context_t enc, dec;
    uint8_t nonce[20] = { 0x01, 0xEF, 0xCD, 0xAB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    helix2_initialize_context(&enc, key, nonce);
    helix2_initialize_context(&dec, key, nonce);

    uint8_t plain[256];
    uint8_t cipher[256];
    uint8_t result[256];

    for (int i = 0; i < 256; i++) plain[i] = (uint8_t)i;

    for (int i = 0; i < 256; i++)
        cipher[i] = helix2_byte(&enc, i, plain[i]);
    
    for (int i = 0; i < 256; i++)
        result[i] = helix2_byte(&dec, i, cipher[i]);

    for (int i = 0; i < 256; i++)
        assert(result[i] == plain[i]);
}

void test_determinism(void) {
    helix2_context_t ctx1, ctx2;
    uint8_t nonce[20] = { 0xBB, 0xBB, 0x22, 0x22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    helix2_initialize_context(&ctx1, key, nonce);
    helix2_initialize_context(&ctx2, key, nonce);

    for (int i = 0; i < 512; i++) {
        uint8_t b1 = helix2_byte(&ctx1, i, 0);
        uint8_t b2 = helix2_byte(&ctx2, i, 0);
        assert(b1 == b2);
    }
}

void test_offset_seek(void) {
    helix2_context_t ctxA, ctxB;
    uint8_t nonce[20] = { 0xCE, 0xFA, 0xED, 0xFE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    helix2_initialize_context(&ctxA, key, nonce);
    helix2_initialize_context(&ctxB, key, nonce);

    // Generate full stream with ctxA
    uint8_t stream[512];
    for (uint32_t i = 0; i < 512; i++)
        stream[i] = helix2_byte(&ctxA, i, 0);
        
    // Start ctxB at offset 300
    for (uint32_t i = 300; i < 512; i++) {
        uint8_t b = helix2_byte(&ctxB, i, 0);
        assert(b == stream[i]);
    }
}

void test_entropy(void) {
    helix2_context_t ctx;
    uint8_t nonce[20] = { 0xAA, 0xAA, 0xAA, 0xAA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    helix2_initialize_context(&ctx, key, nonce);

    uint64_t histogram[256] = {0};
    for (uint32_t i = 0; i < 65536; i++) {
        uint8_t b = helix2_byte(&ctx, i, 0);
        histogram[b]++;
    }

    // Expect roughly uniform distribution
    double avg = 65536.0 / 256.0;
    for (int i = 0; i < 256; i++) {
        assert(histogram[i] > avg * 0.5 && histogram[i] < avg * 1.5);
    }
}

void test_vectors(void) {
    // Placeholder for known-answer tests
    // Implement specific test vectors as needed
    // https://datatracker.ietf.org/doc/html/draft-agl-tls-chacha20poly1305-04#section-7
    // 2917185654

    helix2_context_t ctx;
    uint8_t vector_nonce[20] = {0};  // All zeros
    uint8_t vector_key[32] = {0};    // All zeros

    helix2_initialize_context(&ctx, vector_key, vector_nonce);
    debug_print_state(&ctx);
    debug_print_keystream(&ctx);

    uint8_t* bytes = (uint8_t*)ctx.keystream.stream;
    assert(bytes[0] == 0x14);
    assert(bytes[1] == 0x84);   
    assert(bytes[2] == 0xEC);
    assert(bytes[3] == 0xFD);

    uint8_t vector_key2[32] = {0};
    vector_key2[31] = 0x01;  // Last BYTE = 1 (not last word!)

    helix2_initialize_context(&ctx, vector_key2, vector_nonce);
    debug_print_state(&ctx);
    debug_print_keystream(&ctx);

    assert(bytes[0] == 0xEF);
    assert(bytes[1] == 0x71);   
    assert(bytes[2] == 0xCB);
    assert(bytes[3] == 0xC9);

    uint8_t vector_key3[32] = {0};
    uint8_t vector_nonce2[16] = {0};
    vector_nonce2[11] = 0x01;  // Last BYTE = 1

    helix2_initialize_context(&ctx, vector_key3, vector_nonce2);
    debug_print_state(&ctx);
    debug_print_keystream(&ctx);

    assert(bytes[0] == 0xF6);
    assert(bytes[1] == 0x8C);   
    assert(bytes[2] == 0x5A);
    assert(bytes[3] == 0xB0);    

    uint8_t vector_keyRFC8439[32] = {0x00, 0x01, 0x02, 0x03,
                                     0x04, 0x05, 0x06, 0x07,
                                     0x08, 0x09, 0x0A, 0x0B,
                                     0x0C, 0x0D, 0x0E, 0x0F,
                                     0x10, 0x11, 0x12, 0x13,
                                     0x14, 0x15, 0x16, 0x17,
                                     0x18, 0x19, 0x1A, 0x1B,
                                     0x1C, 0x1D, 0x1E, 0x1F};
    uint8_t vector_nonceRFC8439[16] = {0x00, 0x00, 0x00, 0x09,
                                       0x00, 0x00, 0x00, 0x4A,
                                       0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00};
    vector_nonce2[11] = 0x01;  // Last BYTE = 1

    helix2_initialize_context(&ctx, vector_keyRFC8439, vector_nonceRFC8439);
    helix2_buffer_next_block(&ctx);
    debug_print_state(&ctx);
    debug_print_keystream(&ctx);

    assert(bytes[0] == 0x89);
    assert(bytes[1] == 0xA9);   
    assert(bytes[2] == 0xBC);
    assert(bytes[3] == 0x3F);        
}

void test_64bit_counter(void) {
    printf("\n=== Testing 64-bit Block Counter ===\n");
    
    helix2_context_t ctx;
    uint8_t key[32] = {0};
    uint8_t nonce[20] = {0};
    
    helix2_initialize_context(&ctx, key, nonce);
    
    // Test 1: Block 0 (counter_low = 0, counter_high = 0)
    printf("Block 0 (0x0000000000000000):\n");
    helix2_buffer_set_next_block(&ctx, 0);
    printf("  state[10] = 0x%08X (should be 0x00000000)\n", ctx.keystream.state[10]);
    printf("  state[11] = 0x%08X (should be nonce[0] XOR 0 = nonce[0])\n", ctx.keystream.state[11]);
    assert(ctx.keystream.state[10] == 0x00000000);
    
    // Test 2: Block at 274 GB boundary (2^32 - 1)
    uint64_t block_274gb = 0xFFFFFFFFULL;  // 4,294,967,295 blocks
    printf("\nBlock %llu (0x00000000FFFFFFFF) - 274 GB boundary:\n", block_274gb);
    helix2_buffer_set_next_block(&ctx, block_274gb);
    printf("  state[10] = 0x%08X (should be 0xFFFFFFFF)\n", ctx.keystream.state[10]);
    printf("  state[11] = 0x%08X (should be nonce[0] XOR 0 = nonce[0])\n", ctx.keystream.state[11]);
    assert(ctx.keystream.state[10] == 0xFFFFFFFF);
    assert(ctx.keystream.state[11] == (_pack4(&nonce[0]) ^ 0x00000000));
    
    // Test 3: Block just after 274 GB (2^32)
    uint64_t block_after_274gb = 0x100000000ULL;  // 4,294,967,296 blocks
    printf("\nBlock %llu (0x0000000100000000) - Just after 274 GB:\n", block_after_274gb);
    helix2_buffer_set_next_block(&ctx, block_after_274gb);
    printf("  state[10] = 0x%08X (should be 0x00000000)\n", ctx.keystream.state[10]);
    printf("  state[11] = 0x%08X (should be nonce[0] XOR 1)\n", ctx.keystream.state[11]);
    assert(ctx.keystream.state[10] == 0x00000000);
    assert(ctx.keystream.state[11] == (_pack4(&nonce[0]) ^ 0x00000001));
    
    // Test 4: Very large block (1 TB = ~17.6 billion blocks)
    uint64_t block_1tb = 17592186044416ULL / 64;  // 1 TB in blocks
    printf("\nBlock %llu (0x0000040000000000) - 1 TB:\n", block_1tb);
    helix2_buffer_set_next_block(&ctx, block_1tb);
    uint32_t expected_low = (uint32_t)(block_1tb & 0xFFFFFFFF);
    uint32_t expected_high = (uint32_t)((block_1tb >> 32) & 0xFFFFFFFF);
    printf("  state[10] = 0x%08X (expected 0x%08X)\n", ctx.keystream.state[10], expected_low);
    printf("  state[11] = 0x%08X (expected nonce[0] XOR 0x%08X)\n", 
           ctx.keystream.state[11], expected_high);
    assert(ctx.keystream.state[10] == expected_low);
    assert(ctx.keystream.state[11] == (_pack4(&nonce[0]) ^ expected_high));
    
    // Test 5: Maximum block index (2^64 - 1)
    uint64_t max_block = 0xFFFFFFFFFFFFFFFFULL;
    printf("\nBlock %llu (0xFFFFFFFFFFFFFFFF) - Maximum:\n", max_block);
    helix2_buffer_set_next_block(&ctx, max_block);
    printf("  state[10] = 0x%08X (should be 0xFFFFFFFF)\n", ctx.keystream.state[10]);
    printf("  state[11] = 0x%08X (should be nonce[0] XOR 0xFFFFFFFF)\n", ctx.keystream.state[11]);
    assert(ctx.keystream.state[10] == 0xFFFFFFFF);
    assert(ctx.keystream.state[11] == (_pack4(&nonce[0]) ^ 0xFFFFFFFF));
    
    // Test 6: Verify different nonces produce different keystreams for same block
    uint8_t nonce1[20] = {0x12, 0x34, 0x56, 0x78};
    uint8_t nonce2[20] = {0x87, 0x65, 0x43, 0x21};
    
    helix2_context_t ctx1, ctx2;
    helix2_initialize_context(&ctx1, key, nonce1);
    helix2_initialize_context(&ctx2, key, nonce2);
    
    uint64_t test_block = 0x100000000ULL;  // Block where high bits = 1
    helix2_buffer_set_next_block(&ctx1, test_block);
    helix2_buffer_set_next_block(&ctx2, test_block);
    
    printf("\nNonce differentiation test at block %llu:\n", test_block);
    printf("  nonce1: state[11] = 0x%08X\n", ctx1.keystream.state[11]);
    printf("  nonce2: state[11] = 0x%08X\n", ctx2.keystream.state[11]);
    assert(ctx1.keystream.state[11] != ctx2.keystream.state[11]);
    
    printf("\n- ALL 64-BIT COUNTER TESTS PASSED!\n");
}

void run_all_tests(void) {
    test_symmetry();
    test_determinism();
    test_offset_seek();
    test_entropy();
    test_64bit_counter();
    test_vectors();
    printf("All tests passed successfully.\n");
}

int main(int argc, char *argv[]) {
    run_all_tests();
    return 0;
}