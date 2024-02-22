/**
 * Copyright (c) 2015-2016 The Bitcoin Core developers
 * Copyright (c) 2024 edtubbs
 * Copyright (c) 2024 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <dogecoin/sha2.h>
#include <dogecoin/scrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <stdint.h>
#include <float.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <time.h>
#else
#include <x86intrin.h>
#endif

#define BUFFER_SIZE 1000*1000
#define HASH_SIZE 32

typedef struct {
    double start, end, minTime, maxTime, totalTime;
    uint64_t startCycles, endCycles, minCycles, maxCycles, totalCycles, count;
    uint8_t *input;
    uint8_t *output;
} benchmark_context;

double gettimedouble(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec * 0.000001 + tv.tv_sec;
}

uint64_t perf_cpucycles(void) {
#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (uint64_t)li.QuadPart;
#elif defined(__arm__) || defined(__aarch64__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
#else
    return (uint64_t)__rdtsc();
#endif
}

void run_benchmark(void (*benchmark_function)(benchmark_context *), const char *name) {
    benchmark_context ctx;
    ctx.input = (uint8_t*)calloc(BUFFER_SIZE, sizeof(uint8_t));
    ctx.output = (uint8_t*)malloc(HASH_SIZE);
    ctx.minTime = DBL_MAX;
    ctx.maxTime = DBL_MIN;
    ctx.minCycles = UINT64_MAX;
    ctx.maxCycles = 0;
    ctx.totalTime = 0;
    ctx.totalCycles = 0;
    ctx.count = 0;

    ctx.start = gettimedouble();
    ctx.startCycles = perf_cpucycles();

    while (1) {
        benchmark_function(&ctx);

        double time = ctx.end - ctx.start;
        uint64_t cycles = ctx.endCycles - ctx.startCycles;
        ctx.count++;
        ctx.minTime = (time < ctx.minTime) ? time : ctx.minTime;
        ctx.maxTime = (time > ctx.maxTime) ? time : ctx.maxTime;
        ctx.minCycles = (cycles < ctx.minCycles) ? cycles : ctx.minCycles;
        ctx.maxCycles = (cycles > ctx.maxCycles) ? cycles : ctx.maxCycles;

        if (ctx.totalTime > 3.0) break; // Run for a few seconds

        ctx.start = ctx.end;
        ctx.startCycles = ctx.endCycles;
    }

    printf("%-10s %-8lu %-10f %-10f %-10f %-12lu %-12lu %-12lu\n",
           name, ctx.count, ctx.minTime, ctx.maxTime, ctx.totalTime / ctx.count, ctx.minCycles, ctx.maxCycles, ctx.totalCycles / ctx.count);

    free(ctx.input);
    free(ctx.output);
}

void sha256_benchmark_function(benchmark_context *ctx) {
    sha256_raw(ctx->input, BUFFER_SIZE, ctx->output);
    ctx->end = gettimedouble();
    ctx->endCycles = perf_cpucycles();
    ctx->totalTime += ctx->end - ctx->start;
    ctx->totalCycles += ctx->endCycles - ctx->startCycles;
}

void scrypt_benchmark_function(benchmark_context *ctx) {
    scrypt_1024_1_1_256((char *)ctx->input, (char *)ctx->output);
    ctx->end = gettimedouble();
    ctx->endCycles = perf_cpucycles();
    ctx->totalTime += ctx->end - ctx->start;
    ctx->totalCycles += ctx->endCycles - ctx->startCycles;
}

int main() {
    printf("%-10s %-8s %-10s %-10s %-10s %-12s %-12s %-12s\n",
           "#Benchmark", "Count", "Min Time", "Max Time", "Avg Time",
           "Min Cycles", "Max Cycles", "Avg Cycles");

    run_benchmark(sha256_benchmark_function, "SHA256");
    run_benchmark(scrypt_benchmark_function, "Scrypt");

    printf("\nOptions:\n");
    #if defined(__AVX2__) && USE_AVX2
    printf("AVX2 SHA256\n");
    #endif
    #if defined(__SSE2__) && USE_SSE
    printf("SSE2 SHA256\n");
    #endif
    #if defined(__SSE2__) && USE_SSE2
    printf("SSE2 Scrypt\n");
    #endif

    return 0;
}
