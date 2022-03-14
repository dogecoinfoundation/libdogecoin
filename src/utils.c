/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <tgmath.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h> 
#include <string.h>
#include <assert.h>

#include <errno.h>
#include <dogecoin/cstr.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>

#ifdef WIN32

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4804)
#pragma warning(disable : 4805)
#pragma warning(disable : 4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h> /* for _commit */
#include <shlobj.h>

#else /* WIN32 */

#include <unistd.h>

#endif

#define MAX_LEN 128

static uint8_t buffer_hex_to_uint8[TO_UINT8_HEX_BUF_LEN];
static char buffer_uint8_to_hex[TO_UINT8_HEX_BUF_LEN];


/**
 * @brief This function clears the buffers used for
 * functions inside utils.c.
 *
 * @return Nothing.
 */
void utils_clear_buffers(void)
{
    dogecoin_mem_zero(buffer_hex_to_uint8, TO_UINT8_HEX_BUF_LEN);
    dogecoin_mem_zero(buffer_uint8_to_hex, TO_UINT8_HEX_BUF_LEN);
}

/**
 * @brief This function takes a hex-encoded string and
 * loads a buffer with its binary representation.
 *
 * @param str The hex string to convert.
 * @param out The buffer for the raw data to be returned.
 * @param inLen The number of characters in the hex string.
 * @param outLen The number of raw bytes that were written to the out buffer.
 *
 * @return Nothing.
 */
void utils_hex_to_bin(const char* str, unsigned char* out, int inLen, int* outLen)
    {
    int bLen = inLen / 2;
    int i;
    dogecoin_mem_zero(out, bLen);
    for (i = 0; i < bLen; i++) {
        if (str[i * 2] >= '0' && str[i * 2] <= '9') {
            *out = (str[i * 2] - '0') << 4;
            }
        if (str[i * 2] >= 'a' && str[i * 2] <= 'f') {
            *out = (10 + str[i * 2] - 'a') << 4;
            }
        if (str[i * 2] >= 'A' && str[i * 2] <= 'F') {
            *out = (10 + str[i * 2] - 'A') << 4;
            }
        if (str[i * 2 + 1] >= '0' && str[i * 2 + 1] <= '9') {
            *out |= (str[i * 2 + 1] - '0');
            }
        if (str[i * 2 + 1] >= 'a' && str[i * 2 + 1] <= 'f') {
            *out |= (10 + str[i * 2 + 1] - 'a');
            }
        if (str[i * 2 + 1] >= 'A' && str[i * 2 + 1] <= 'F') {
            *out |= (10 + str[i * 2 + 1] - 'A');
            }
        out++;
        }
    *outLen = i;
    }


/**
 * @brief This function takes a hex-encoded string and
 * returns the binary representation as a uint8_t array.
 *
 * @param str The hex string to convert.
 *
 * @return The array of binary data.
 */
uint8_t* utils_hex_to_uint8(const char* str)
    {
    uint8_t c;
    size_t i;
    if (strlens(str) > TO_UINT8_HEX_BUF_LEN) {
        return NULL;
    }
    dogecoin_mem_zero(buffer_hex_to_uint8, TO_UINT8_HEX_BUF_LEN);
    for (i = 0; i < strlens(str) / 2; i++) {
        c = 0;
        if (str[i * 2] >= '0' && str[i * 2] <= '9') {
            c += (str[i * 2] - '0') << 4;
            }
        if (str[i * 2] >= 'a' && str[i * 2] <= 'f') {
            c += (10 + str[i * 2] - 'a') << 4;
            }
        if (str[i * 2] >= 'A' && str[i * 2] <= 'F') {
            c += (10 + str[i * 2] - 'A') << 4;
            }
        if (str[i * 2 + 1] >= '0' && str[i * 2 + 1] <= '9') {
            c += (str[i * 2 + 1] - '0');
            }
        if (str[i * 2 + 1] >= 'a' && str[i * 2 + 1] <= 'f') {
            c += (10 + str[i * 2 + 1] - 'a');
            }
        if (str[i * 2 + 1] >= 'A' && str[i * 2 + 1] <= 'F') {
            c += (10 + str[i * 2 + 1] - 'A');
            }
        buffer_hex_to_uint8[i] = c;
        }
    return buffer_hex_to_uint8;
    }


/**
 * @brief This function takes an array of raw data and
 * converts them to a hex-encoded string.
 *
 * @param bin_in The array of raw data to convert.
 * @param inlen The number of bytes in the array.
 * @param hex_out The resulting hex string.
 *
 * @return Nothing.
 */
void utils_bin_to_hex(unsigned char* bin_in, size_t inlen, char* hex_out)
    {
    static char digits[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < inlen; i++) {
        hex_out[i * 2] = digits[(bin_in[i] >> 4) & 0xF];
        hex_out[i * 2 + 1] = digits[bin_in[i] & 0xF];
        }
    hex_out[inlen * 2] = '\0';
    }


/**
 * @brief This function takes an array of raw bytes and
 * converts them to a hex-encoded string.
 *
 * @param bin The array of raw bytes to convert.
 * @param l The number of bytes to convert.
 *
 * @return The hex-encoded string.
 */
char* utils_uint8_to_hex(const uint8_t* bin, size_t l)
    {
    static char digits[] = "0123456789abcdef";
    size_t i;
    if (l > (TO_UINT8_HEX_BUF_LEN / 2 - 1)) {
        return NULL;
    }
    dogecoin_mem_zero(buffer_uint8_to_hex, TO_UINT8_HEX_BUF_LEN);
    for (i = 0; i < l; i++) {
        buffer_uint8_to_hex[i * 2] = digits[(bin[i] >> 4) & 0xF];
        buffer_uint8_to_hex[i * 2 + 1] = digits[bin[i] & 0xF];
        }
    buffer_uint8_to_hex[l * 2] = '\0';
    return buffer_uint8_to_hex;
    }


/**
 * @brief This function takes a hex-encoded string and
 * reverses the order of its bytes.
 *
 * @param h The hex string to reverse.
 * @param len The length of the hex string.
 *
 * @return Nothing.
 */
void utils_reverse_hex(char* h, int len)
    {
    char* copy = dogecoin_calloc(1, len);
    int i;
    memcpy_safe(copy, h, len);
    for (i = 0; i < len - 1; i += 2) {
        h[i] = copy[len - i - 2];
        h[i + 1] = copy[len - i - 1];
        }
    dogecoin_free(copy);
    }

const signed char p_util_hexdigit[256] =
    {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
    -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };


/**
 * @brief This function takes a char from a hex string
 * and returns the actual hex digit as a signed char.
 *
 * @param c The character to convert to hex digit.
 *
 * @return The equivalent hex digit.
 */
signed char utils_hex_digit(char c)
    {
    return p_util_hexdigit[(unsigned char)c];
    }


/**
 * @brief This function takes a hex-encoded string
 * and sets a 256-bit array to the numerical value in
 * little endian format.
 *
 * @param psz The hex string to convert.
 * @param out The resulting byte array.
 *
 * @return Nothing.
 */
void utils_uint256_sethex(char* psz, uint8_t* out)
{
    dogecoin_mem_zero(out, sizeof(uint256));

    // skip leading spaces
    while (isspace(*psz)) {
        psz++;
        }

    // skip 0x
    if (psz[0] == '0' && tolower(psz[1]) == 'x') {
        psz += 2;
        }

    // hex string to uint
    const char* pbegin = psz;
    while (utils_hex_digit(*psz) != -1) {
        psz++;
        }
    psz--;
    unsigned char* p1 = (unsigned char*)out;
    unsigned char* pend = p1 + sizeof(uint256);
    while (psz >= pbegin && p1 < pend) {
        *p1 = utils_hex_digit(*psz--);
        if (psz >= pbegin) {
            *p1 |= ((unsigned char)utils_hex_digit(*psz--) << 4);
            p1++;
            }
        }
    }


/**
 * @brief This function executes malloc() but exits the
 * program if unsuccessful.
 *
 * @param size The size of the memory to allocate.
 *
 * @return A pointer to the memory that was allocated.
 */
void* safe_malloc(size_t size)
    {
    void* result;

    if ((result = malloc(size))) { /* assignment intentional */
        return (result);
        }
    else {
        printf("memory overflow: malloc failed in safe_malloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
        }
    }


/**
 * @brief This function generates a buffer of random bytes.
 *
 * @param buf The buffer to store the random data.
 * @param len The number of random bytes to generate.
 *
 * @return Nothing.
 */
void dogecoin_cheap_random_bytes(uint8_t* buf, uint32_t len)
    {
    srand(time(NULL)); // insecure
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = rand(); // weak non secure cryptographic rng
        }
    }


/**
 * @brief This function takes a path variable and appends
 * the default data directory according to the user's
 * operating system.
 *
 * @param path_out The pointer to the cstring containing the path.
 */
void dogecoin_get_default_datadir(cstring* path_out)
    {
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\Bitcoin
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\Bitcoin
    // Mac: ~/Library/Application Support/Bitcoin
    // Unix: ~/.dogecoin
#ifdef WIN32
    // Windows
    char* homedrive = getenv("HOMEDRIVE");
    char* homepath = getenv("HOMEDRIVE");
    cstr_append_buf(path_out, homedrive, strlen(homedrive));
    cstr_append_buf(path_out, homepath, strlen(homepath));
#else
    char* home = getenv("HOME");
    if (home == NULL || strlen(home) == 0)
        cstr_append_c(path_out, '/');
    else
        cstr_append_buf(path_out, home, strlen(home));
#ifdef __APPLE__
    // Mac
    char* osx_home = "/Library/Application Support/Dogecoin";
    cstr_append_buf(path_out, osx_home, strlen(osx_home));
#else
    // Unix
    char* posix_home = "/.dogecoin";
    cstr_append_buf(path_out, posix_home, strlen(posix_home));
#endif
#endif
    }


/**
 * @brief This function flushes all data left in the output
 * stream into the specified file.
 *
 * @param file The pointer to the file descriptor that will store the data.
 *
 * @return Nothing.
 */
void dogecoin_file_commit(FILE* file)
    {
    fflush(file); // harmless if redundantly called
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    FlushFileBuffers(hFile);
#else
#if defined(__linux__) || defined(__NetBSD__)
    fdatasync(fileno(file));
#elif defined(__APPLE__) && defined(F_FULLFSYNC)
    fcntl(fileno(file), F_FULLFSYNC, 0);
#else
    fsync(fileno(file));
#endif
#endif
    }

void print_header(char* filepath) {
    if (!filepath) return;
    char* filename = filepath;
    FILE* fptr = NULL;

    if ((fptr = fopen(filename, "r")) == NULL)
        {
        fprintf(stderr, "error opening %s\n", filename);
        }

    print_image(fptr);

    fclose(fptr);
    }

void print_image(FILE* fptr)
    {
    char read_string[MAX_LEN];

    while (fgets(read_string, sizeof(read_string), fptr) != NULL)
        printf("%s", read_string);
    }

int cmp_ld_uint64(long double x, uint64_t y) {
    return (x == (long double)y) && (x != 0x1p64l) && ((uint64_t)x == y);
    }

inline long double inv_trunc(long double Number, uint64_t target) {
    return Number < target ? floor(Number) : ceil(Number);
}

long double rnd(long double v, long double digit) {
    long double _pow;
    _pow = pow(10.0, digit);
    long double t = v * _pow;
    long double r = ceil(t + 0.5);
    return r / _pow;
}

long double get_koinu_mantissa(long double in) {
    return fmodl(in, 1e8) / 1e8;
}

long double get_koinu_integer(uint64_t in) {
    return (long double)truncl(in / 1e8);
}

uint64_t get_diff(uint64_t x) {
    return x % 10;
}

long double get_diffl(long double x) {
    return fmodl(x, 10);
}

long double length(double n) {
    if (n < 0) n = (n == DBL_MIN) ? DBL_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    if (n < 10000000000) return 10;
    if (n < 100000000000) return 11;
    if (n < 1000000000000) return 12;
    if (n < 10000000000000) return 13;
    if (n < 100000000000000) return 14;
    if (n < 1000000000000000) return 15;
    if (n < 10000000000000000) return 16;
    if (n < 100000000000000000) return 17;
    if (n < 1000000000000000000) return 18;
    if (n < 10000000000000000000UL) return 19;
    // UINT64_MAX // 18446744073709551615
    return 20;
    }

long double get_mantissa_upto(long double in, long double x) {
    return fmodl(in, powl(10.0, x)) / powl(10, x);
}

long double append_koinu(long double x, long double y) {
    debug_print("x:                                          %.25Lf\n", x);
    y = y * 1e7;
    debug_print("y:                                          %.25Lf\n", y);
    return (long double)((x + y) * 1e1) / 100000000;
}

int compare_diffs(uint64_t a, uint64_t b) {
    return a - b;
}

void show_fe_currentrnding_direction(void)
{
    switch (fegetround()) {
           case FE_TONEAREST:  debug_print ("FE_TONEAREST: %d\n", FE_TONEAREST);  break;
           case FE_DOWNWARD:   debug_print ("FE_DOWNWARD: %d\n", FE_DOWNWARD);   break;
           case FE_UPWARD:     debug_print ("FE_UPWARD: %d\n", FE_UPWARD);     break;
           case FE_TOWARDZERO: debug_print ("FE_TOWARDZERO: %d\n", FE_TOWARDZERO); break;
           default:            debug_print ("%s\n", "unknown");
    };
}

long double koinu_to_coins(uint64_t koinu) {
    long double output;
#if defined(__ARM_ARCH_7A__)
    int rounding_mode = fegetround();
    int l = length(koinu);
    output = (long double)koinu / (long double)1e8;
    if (l >= 9) {
        fesetround(FE_UPWARD);
        output = (long double)koinu / (long double)1e8;
    } else if (l >= 17) {
        output = rnd((long double)koinu / (long double)1e8, 8.5) + .000000005;
    }
    fesetround(rounding_mode);
#elif defined(WIN32)
    output = (long double)koinu / (long double)1e8;
#else
    output = (long double)koinu / (long double)1e8;
#endif
    return output;
}

uint64_t get_true_uint64_t(long double x) {
    return (((uint64_t)get_koinu_integer(x * 100000000)) * 100000000) + (uint64_t)(rnd(get_koinu_mantissa(x * 100000000), 8.0) * 100000000);
}

long double round_ld(long double x)
{
    fenv_t save_env;
    feholdexcept(&save_env);
    long double result = rintl(x);
    if (fetestexcept(FE_INEXACT)) {
        int const save_round = fegetround();
        fesetround(FE_UPWARD);
        result = rintl(copysignl(0.5 + fabsl(x), x));
        debug_print("result: %.8Lf\n", result);
        fesetround(save_round);
    }
    feupdateenv(&save_env);
    return result;
}

long double get_suffix_at_length(long double in, long double n) {
    DISABLE_WARNING(-Wunused-variable)
    DISABLE_WARNING_PUSH
    long double base = n >= 8 ? powl(10, n - 8) : powl(10, 8 - n);
    DISABLE_WARNING_POP
    long double out, powered;
    powered = (in * powl(10, n));
    debug_print("powered %.8Lf\n", powered);
    out = ((uint64_t)(in * powl(10, n)) % 10);
    return out;
}

uint64_t coins_to_koinu_str(char* coins) {
    long double output;
    long double integer_length, mantissa;
    // length minus 1 representative of decimal and 8 representative of koinu
    integer_length = strlen(coins) - 9;
    char* int_end, int_str[256], mant_end, mant_str[256];
    uint64_t x = 0, count = 0;
    for (; x < integer_length; x++) {
        int_str[x] = coins[x];
    }
    unsigned long long int u64 = strtoull(int_str, &int_end, 10);
    u64 *= 1e8;
    for (uint64_t y = x; y <= strlen(coins) - 9; y++) {
        mantissa = roundl(strtold(&coins[y], &int_end) / 10);
        u64 += (uint64_t)mantissa;
    }
    return u64;
}

long long unsigned coins_to_koinu(long double coins) {
    long double output, integer_length, mantissa_length;
#if defined(__ARM_ARCH_7A__)
    char* str[256];
    sprintf(&str, "%.8Lf", coins);
    // length minus 1 representative of decimal and 8 representative of koinu
    integer_length = strlen(str) - 9;
    mantissa_length = integer_length + (8 - integer_length);
    if (integer_length <= mantissa_length) {
        output = coins * powl(10, mantissa_length);
    } else {
        output = round_ld(coins * powl(10, mantissa_length));
    }
#else
     output = (uint64_t)round((long double)coins * (long double)1e8);
#endif
    return output;
}

void print_bits(size_t const size, void const* ptr)
    {
    unsigned char* b = (unsigned char*)ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
            }
        }
    puts("");
    }

/**
 * @brief Allows prepending characters (char* t) to the beginning of a string (char* s).
 *
 * @param s The string to prepend to.
 * @param t The characters that will be prepended.
 */
void prepend(char* s, const char* t)
    {
    /* get length of const char* t */
    size_t length = strlen(t);

    /* allocate enough length of both s and t
    for s and move each char back one */
    memmove(s + length, s, strlen(s) + 1);

    /* prepend t to new empty space in s */
    memcpy(s, t, length);
    }

/**
 * @brief Allows appending characters (char* t) to the end of a string (char* s).
 *
 * @param s The string to append to.
 * @param t The characters that will be appended.
 */
void append(char* s, char* t)
    {
    int i = 0, length = 0;
    /* get length of char* s */
    for (; memcmp(&s[i], "\0", 1) != 0; i++) length++;

    /*  append char* t to char* s */
    for (i = 0; memcmp(&t[i], "\0", 1) != 0; i++) {
        s[length + i] = t[i];
        }

    memcpy(&s[length + i], "\0", 1);
    }

/**
 * @brief function to convert ascii text to hexadecimal string
 *
 * @param in
 * @param output
 */
void text_to_hex(char* in, char* out) {
    int length = 0;
    int i = 0;

    while (in[length] != '\0') {
        sprintf((char*)(out + i), "%02X", in[length]);
        length += 1;
        i += 2;
        }
    out[i++] = '\0';
    }

const char* get_build() {
        #if defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
        #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
            return "x86_32";
        #elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
            return "ARM7";
        #elif defined(__aarch64__) || defined(_M_ARM64)
            return "ARM64";
        #else
            return "UNKNOWN";
        #endif
    }
