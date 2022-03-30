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

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    memset(buffer_hex_to_uint8, 0, TO_UINT8_HEX_BUF_LEN);
    memset(buffer_uint8_to_hex, 0, TO_UINT8_HEX_BUF_LEN);
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
    memset(out, 0, bLen);
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
    memset(buffer_hex_to_uint8, 0, TO_UINT8_HEX_BUF_LEN);
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
    memset(buffer_uint8_to_hex, 0, TO_UINT8_HEX_BUF_LEN);
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
    strncpy(copy, h, len);
    for (i = 0; i < len; i += 2) {
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
    memset(out, 0, sizeof(uint256));

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
    } else {
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
    srand(time(NULL));
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = rand();
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

uint8_t* bytearray_concatenate(uint8_t* input1, uint8_t* input2)
{
    size_t input1_size;
    size_t input2_size;
    input1_size= sizeof(input1)/sizeof(input1[0]);
    input2_size= sizeof(input2)/sizeof(input2[0]);
    uint8_t* returnedarray = realloc(input1, input1_size+input2_size);
    memcpy(returnedarray+input1_size, input2, sizeof(returnedarray));
    free(input2);
    return returnedarray;
}

float koinu_to_coins(uint64_t koinu) {
    return (float)koinu / (float)100000000;
}

uint64_t coins_to_koinu(float coins) {
    return ((float)coins * (float)1000000000) / 10;
}

void print_bits(size_t const size, void const* ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}
