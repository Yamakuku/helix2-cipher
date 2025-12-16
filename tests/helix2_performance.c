/**
 * @file helix2.c
 * @brief Helix2 Stream Cipher implementation, simple performance test example
 * 
 * Helix2-cipher is an educational ARX (Add-Rotate-XOR) stream cipher inspired by
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

#include "../src/helix2.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]);
double benchmark_throughput(size_t buffer_size, int iterations);
void test_performance();

double benchmark_throughput(size_t buffer_size, int iterations) {
    helix2_context_t ctx;
    uint8_t key[32] = {0};
    uint8_t nonce[20] = {0};
    
    // Initialize with test key
    for (int i = 0; i < 32; i++) key[i] = i;
    helix2_initialize_context(&ctx, key, nonce);
    
    uint8_t *buffer = malloc(buffer_size);
    if (!buffer) return 0.0;
    memset(buffer, 0, buffer_size);
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        helix2_buffer(&ctx, buffer, buffer_size, (uint64_t)i * buffer_size);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    double mb_processed = (iterations * buffer_size) / (1024.0 * 1024.0);
    
    free(buffer);
    return mb_processed / seconds; // MB/s
}

void test_performance() {
    printf("\nHelix2 Performance Benchmark\n");
    printf("==============================\n\n");
    
    // Test different buffer sizes
    size_t sizes[] = {64, 256, 1024, 4096, 16384, 65536, 1024*1024};
    const char* size_names[] = {"64 B", "256 B", "1 KB", "4 KB", "16 KB", "64 KB", "1 MB"};
    
    for (int i = 0; i < 7; i++) {
        int iterations = (1024 * 1024 * 100) / sizes[i]; // Process ~100 MB
        if (iterations < 10) iterations = 10;
        
        double throughput = benchmark_throughput(sizes[i], iterations);
        printf("Buffer: %-6s  Throughput: %7.2f MB/s", size_names[i], throughput);
        
        if (throughput > 1024) {
            printf("  (%5.2f GB/s)", throughput / 1024.0);
        }
        printf("\n");
    }
    
    printf("\n");
}


int main(int argc, char *argv[]) {
    test_performance();
    return 0;
}