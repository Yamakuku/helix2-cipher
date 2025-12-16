/**
 * @file helix2-cl.c
 * @brief Helix-2 Stream Cipher implementation, command-line tool example
 * 
 * Helix-2 is an educational ARX (Add-Rotate-XOR) stream cipher inspired by
 * ChaCha20 but using nested operations for higher per-operation complexity.
 * 
 * WARNING: This cipher is experimental and has NOT undergone formal 
 * cryptanalysis. It should NOT be used for production security applications.
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

/* Enable large file support on Linux/POSIX systems */
#ifndef _WIN32
    #define _POSIX_C_SOURCE 200112L
    #define _FILE_OFFSET_BITS 64
#endif

#include "../src/helix2.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#define BUFFER_SIZE 1024
#define PROGRESS_WIDTH 10

void syntax(); 
void derive_key_from_password(const char *password, uint8_t *key);
void generate_nonce_from_seed(uint32_t seed, uint32_t* nonce_out);
void print_progress(uint64_t current, uint64_t total);

void print_progress(uint64_t current, uint64_t total) {
    static unsigned int last_percent = 101;  /* track last displayed filled count */
    
    if (total == 0) return;
    
    unsigned int percent = (unsigned int)(((double)current / (double)total) * 100.0);
    unsigned int filled = (unsigned int)(((double)current / (double)total) * (double)PROGRESS_WIDTH);

    /* Only print if the bar changed */
    if (percent != last_percent) {
        printf("\r[");
        for (int i = 0; i < PROGRESS_WIDTH; i++) {
            putchar(i < filled ? 'X' : '_');
        }
        printf("] %u%%", percent);
        fflush(stdout);
        last_percent = percent;
    }
}

void derive_key_from_password(const char *password, uint8_t *key) {
    uint32_t state = 0x9E3779B9u;
    size_t pwd_len = strlen(password);
    
    /* Initialize key array to zeros */
    memset(key, 0, 32);
    
    /* Temporary uint32_t array for mixing */
    uint32_t key_words[8] = {0};
    
    /* Mix password bytes into key words */
    for (size_t i = 0; i < pwd_len; i++) {
        uint8_t byte = (uint8_t)password[i];
        int key_idx = i % 8;
        
        state ^= byte;
        state = (state << 13) | (state >> 19);  /* rotate left 13 bits */
        state += 0x9E3779B9u;
        
        key_words[key_idx] ^= state;
    }
    
    /* Strengthen key if password is short */
    for (int round = 0; round < 4; round++) {
        for (int i = 0; i < 8; i++) {
            uint32_t z = key_words[i];
            z = (z ^ (z >> 16)) * 0x7FEB352Du;
            z = (z ^ (z >> 15)) * 0x846CA68Bu;
            z = z ^ (z >> 16);
            key_words[i] = z;
        }
    }
    
    /* Convert uint32_t words to uint8_t bytes (little-endian) */
    for (int i = 0; i < 8; i++) {
        key[i*4 + 0] = key_words[i] & 0xFF;
        key[i*4 + 1] = (key_words[i] >> 8) & 0xFF;
        key[i*4 + 2] = (key_words[i] >> 16) & 0xFF;
        key[i*4 + 3] = (key_words[i] >> 24) & 0xFF;
    }
}


/* Expand a 32-bit seed into a unique 4x32-bit nonce.
   Guarantees different seeds yield different nonce vectors by placing
   the seed in nonce_out[0]. The other words are mixed variants. */
void generate_nonce_from_seed(uint32_t seed, uint32_t* nonce_out) {
    nonce_out[0] = seed;                 /* ensures uniqueness per seed */
    uint32_t state = seed + 0x9E3779B9u; /* splitmix starting state */
    for (int i = 1; i < 4; ++i) {
        state += 0x9E3779B9u;
        uint32_t z = state;
        z = (z ^ (z >> 16)) * 0x7FEB352Du;
        z = (z ^ (z >> 15)) * 0x846CA68Bu;
        z = z ^ (z >> 16);
        /* mix in index and seed for extra decorrelation */
        nonce_out[i] = z ^ (seed + (uint32_t)(0xA5A5A5A5u * i));
    }
}


int main(int argc, char *argv[]) {
    const char *password = NULL;
    const char *input_file = NULL;
    const char *output_file = NULL;
    uint8_t nonce[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int mode = 0;  /* 0=none, 1=encrypt, 2=decrypt */

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            mode = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            mode = 2;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                password = argv[++i];
            } else {
                printf("Error: -p requires a password argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0) {
            // -n accepts hex string (40 hex chars = 20 bytes, e.g. -n 0123456789abcdef0123456789abcdef01234567)
            if (i + 1 < argc) {
                const char *s = argv[++i];
        
                // Remove 0x prefix if present
                if (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0) {
                    s += 2;
                }
        
                size_t len = strlen(s);
                if (len > 40) len = 40; // Truncate if too long
        
                // Convert hex string to bytes
                for (size_t i = 0; i < len/2 && i < 20; i++) {
                    char hex_byte[3] = {s[i*2], s[i*2+1], '\0'};
                    nonce[i] = (uint8_t)strtoul(hex_byte, NULL, 16);
                }
            } else {
                printf("Error: -n requires hex string argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                printf("Error: -o requires an output filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0) {
            syntax();
            return 0;
        } else if (argv[i][0] != '-') {
            /* Assume it's the input filename if it doesn't start with - */
            input_file = argv[i];
        } else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    /* Validate inputs */
    if (!input_file || !password || mode == 0) {
        printf("Error: Missing required arguments\n");
        syntax();
        return 1;
    }

    printf("Mode: %s\n", mode == 1 ? "Encrypt" : "Decrypt");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file ? output_file : input_file);
    printf("Password: ********\n");
    printf("Nonce: 0x");
    for (int i = 0; i < 20; i++) {
        printf("%02x", nonce[i]);
    }
    printf("\n");

/* Derive key from password */
    uint8_t key[32];
    derive_key_from_password(password, key);

    /* Initialize IV buffer */
    helix2_context_t ctx;
    helix2_initialize_context(&ctx, key, nonce);

    /* Open input and output files */
    FILE *fin = fopen(input_file, "rb");
    if (!fin) {
        printf("Error: Cannot open input file '%s'\n", input_file);
        return 1;
    }

    FILE *fout = fopen(output_file ? output_file : input_file, "wb");
    if (!fout) {
        printf("Error: Cannot open output file '%s'\n", output_file ? output_file : input_file);
        fclose(fin);
        return 1;
    }

    /* Get file size for progress tracking */
    #ifdef _WIN32
        _fseeki64(fin, 0, SEEK_END);
        uint64_t file_size = (uint64_t)_ftelli64(fin);
        _fseeki64(fin, 0, SEEK_SET);
    #else
        fseeko(fin, 0, SEEK_END);
        uint64_t file_size = (uint64_t)ftello(fin);
        fseeko(fin, 0, SEEK_SET);
    #endif

    /* Process file with buffer */
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_read;
    uint64_t file_offset = 0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fin)) > 0) {
        helix2_buffer(&ctx, buffer, bytes_read, file_offset);
        fwrite(buffer, 1, bytes_read, fout);
        file_offset += bytes_read;

        print_progress(file_offset, file_size);
    }

    printf("\n\nDone, processed %" PRIu64 " bytes\n", file_offset);
    
    fclose(fin);
    fclose(fout);

    return 0;
}

void syntax() {
    printf("Crypt Command Line Utility\n");
    printf("Usage: crypt_cl options filename\n");
    printf("Options:\n");
    printf("  -e            Encrypt the file\n");   
    printf("  -d            Decrypt the file\n");
    printf("  -p <password> Specify the encryption password\n");
    printf("  -n <nonce>    Specify the seed value (40 hex chars = 20 bytes), ex. 0123456789abcdef0123456789abcdef01234567\n");
    printf("  -o <output>   Specify the output filename, if ommited then the input files will be processed\n");
    printf("  -h            Show this help message\n");
}