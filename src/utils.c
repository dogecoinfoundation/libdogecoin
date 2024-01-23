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

#if defined HAVE_CONFIG_H
#include "libdogecoin-config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <dogecoin/cstr.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>
#include <dogecoin/vector.h>

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

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

#ifdef _MSC_VER
#include <win/winunistd.h>
#else
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
void utils_hex_to_bin(const char* str, unsigned char* out, size_t inLen, size_t* outLen)
    {
    size_t bLen = inLen / 2;
    size_t i;
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
void utils_reverse_hex(char* h, size_t len)
    {
    char* copy = dogecoin_calloc(1, len);
    size_t i;
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

uint256* uint256S(const char *str)
{
    return (uint256*)utils_hex_to_uint8(str);
}

unsigned char* parse_hex(const char* psz)
{
    int i = 0;
    unsigned char* input = dogecoin_uchar_vla(strlen(psz));
    while (true)
    {
        while (isspace(*psz))
            psz++;
        signed char c = utils_hex_digit(*psz++);
        if (c == (signed char)-1)
            break;
        unsigned char n = (c << 4);
        c = utils_hex_digit(*psz++);
        if (c == (signed char)-1)
            break;
        n |= c;
        input[i] = n;
        i++;
    }
    return input;
}

/**
 * Swaps bytes of a given buffer, effectively performing a big-endian to/from little-endian conversion
 */
void swap_bytes(uint8_t *buf, int buf_size) {
    int i = 0;
    for (; i < buf_size/2; i++)
    {
        uint8_t temp = buf[i];
        buf[i] = buf[buf_size-i-1];
        buf[buf_size-i-1] = temp;
    }
}

// Returns a pointer to the first byte of needle inside haystack,
uint8_t* bytes_find(uint8_t* haystack, size_t haystackLen, uint8_t* needle, size_t needleLen) {
    if (needleLen > haystackLen) {
        return false;
    }
    uint8_t* match = memchr(haystack, needle[0], haystackLen);
    if (match != NULL) {
        size_t remaining = haystackLen - ((uint8_t*)match - haystack);
        if (needleLen <= remaining) {
            if (memcmp(match, needle, needleLen) == 0) {
                return match;
            }
        }
    }
    return NULL;
}

const char *find_needle(const char *haystack, size_t haystack_length, const char *needle, size_t needle_length) {
    size_t haystack_index = 0;
    for (; haystack_index < haystack_length; haystack_index++) {

        bool needle_found = true;
        size_t needle_index = 0;
        for (; needle_index < needle_length; needle_index++) {
            const int haystack_character = haystack[haystack_index + needle_index];
            const int needle_character = needle[needle_index];
            if (haystack_character == needle_character) {
                continue;
            } else {
                needle_found = false;
                break;
            }
        }

        if (needle_found) {
            return &haystack[haystack_index];
        }
    }

    return NULL;
}

char* to_string(uint8_t* x) {
    return utils_uint8_to_hex(x, 32);
}

char* hash_to_string(uint8_t* x) {
    char* hexbuf = to_string(x);
    utils_reverse_hex(hexbuf, DOGECOIN_HASH_LENGTH*2);
    return hexbuf;
}

uint8_t* hash_to_bytes(uint8_t* x) {
    char* hexbuf = hash_to_string(x);
    return utils_hex_to_uint8(hexbuf);
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
    void dogecoin_cheap_random_bytes(uint8_t* buf, size_t len)
    {
    srand(time(NULL)); // insecure
    for (size_t i = 0; i < len; i++) {
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
#elif defined(__linux__) || defined(__NetBSD__)
    fdatasync(fileno(file));
#elif defined(__APPLE__) && defined(F_FULLFSYNC)
    fcntl(fileno(file), F_FULLFSYNC, 0);
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
    size_t i = 0, length = 0;
    /* get length of char* s */
    for (; memcmp(&s[i], "\0", 1) != 0; i++) length++;
    /*  append char* t to char* s */
    for (i = 0; memcmp(&t[i], "\0", 1) != 0; i++) {
        s[length + i] = t[i];
        }

    memcpy(&s[length + i], "\0", 1);
    }

char* concat(char* prefix, char* suffix) {
    size_t suffix_length = strlen(suffix), prefix_length = strlen(prefix);
    char* file = dogecoin_char_vla(prefix_length + suffix_length + 1);
    memcpy_safe(file, prefix, prefix_length + 1);
    memcpy_safe(file + prefix_length, suffix, suffix_length + 1);
    return file;
}

void slice(const char *str, char *result, size_t start, size_t end)
{
    strncpy(result, str + start, end - start);
}

void remove_substr(char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
        *match = '\0';
        strcat(string, match+len);
    }
}

void replace_last_after_delim(const char *str, char* delim, char* replacement) {
    char* tmp = strdup((char*)str);
    char* new = tmp;
    char *strptr = strtok(new, delim);
    char* last = NULL;
    while (strptr != NULL) {
        last = strptr;
        strptr = strtok(NULL, delim);
    }
    if (last) {
        remove_substr((char*)str, last);
        append((char*)str, replacement);
    }
    dogecoin_free(tmp);
}

/**
 * @brief function to convert ascii text to hexadecimal string
 *
 * @param in
 * @param output
 */
void text_to_hex(char* in, char* out) {
    size_t length = 0;
    size_t i = 0;

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

/**
 * @brief Gets a password from the user
 *
 * Gets a password from the user without echoing the input to the console.
 *
 * @param[in] prompt The prompt to display to the user
 * @return The password entered by the user
 */
char *getpass(const char *prompt) {
    char buffer[MAX_LEN] = {0};  // Initialize to zero

#ifndef USE_OPENENCLAVE
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;

    if (!GetConsoleMode(hStdin, &mode) || !SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT))
        return NULL;

    printf("%s", prompt);
    fflush(stdout);

    if (!ReadConsole(hStdin, buffer, sizeof(buffer) - 1, &count, NULL))
        return NULL;  // -1 to ensure null-termination

    if (count > 0 && buffer[count-1] == '\n')
        count--;  // Remove newline character

    if (count > 0 && buffer[count-1] == '\r')
        count--;  // Remove carriage return character

    if (!SetConsoleMode(hStdin, mode))
        return NULL;

    buffer[count] = '\0';  // Ensure null-termination

#else
    struct termios old, new;
    ssize_t nread;

    if (tcgetattr(STDIN_FILENO, &old) != 0)
        return NULL;

    new = old;
    new.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
        return NULL;

    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buffer, sizeof(buffer), stdin))
        return NULL;

    nread = strlen(buffer);
    if (nread > 0 && buffer[nread-1] == '\n')
        buffer[nread-1] = '\0';  // Remove newline character

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &old) != 0)
        return NULL;

#endif

#else // USE_OPENENCLAVE
    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buffer, sizeof(buffer), stdin))
        return NULL;

    ssize_t nread = strlen(buffer);
    if (nread > 0 && buffer[nread-1] == '\n')
        buffer[nread-1] = '\0';  // Remove newline character
#endif
    return strdup(buffer);
}

/* reverse:  reverse string s in place */
void dogecoin_str_reverse(char s[])
{
    size_t i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  convert n to characters in s */
void dogecoin_uitoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    dogecoin_str_reverse(s);
}

bool dogecoin_network_enabled() {
#ifndef WITH_NET
    return false;
#else
    return true;
#endif
}

int integer_length(int x) {
    int count = 0;
    while (x > 0) {
        x /= 10;
        count++;
    }
    return count > 0 ? count : 1;
}

int file_copy(char src [], char dest [])
{
    int   c;
    FILE *stream_read;
    FILE *stream_write; 

    stream_read = fopen (src, "r");
    if (stream_read == NULL)
        return -1;
    stream_write = fopen (dest, "w");   //create and write to file
    if (stream_write == NULL)
     {
        fclose (stream_read);
        return -2;
     }    
    while ((c = fgetc(stream_read)) != EOF)
        fputc (c, stream_write);
    fclose (stream_read);
    fclose (stream_write);

    return 0;
}

unsigned char base64_char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned int base64_int(unsigned int ch) {

	// ASCII to base64_int
	// 65-90  Upper Case  >>  0-25
	// 97-122 Lower Case  >>  26-51
	// 48-57  Numbers     >>  52-61
	// 43     Plus (+)    >>  62
	// 47     Slash (/)   >>  63
	// 61     Equal (=)   >>  64~
	if (ch==43)
	return 62;
	if (ch==47)
	return 63;
	if (ch==61)
	return 64;
	if ((ch>47) && (ch<58))
	return ch + 4;
	if ((ch>64) && (ch<91))
	return ch - 'A';
	if ((ch>96) && (ch<123))
	return (ch - 'a') + 26;
	return 0;
}

unsigned int base64_encoded_size(unsigned int in_size) {

	// size equals 4*floor((1/3)*(in_size+2));
	unsigned int i, j = 0;
	for (i=0;i<in_size;i++) {
		if (i % 3 == 0)
		j += 1;
	}
	return (4*j);
}

unsigned int base64_decoded_size(unsigned int in_size) {

	return ((3*in_size)/4);
}

unsigned int base64_encode(const unsigned char* in, unsigned int in_len, unsigned char* out) {

	unsigned int i=0, j=0, k=0, s[3];

	for (i=0;i<in_len;i++) {
		s[j++]=*(in+i);
		if (j==3) {
			out[k+0] = base64_char[ (s[0]&255)>>2 ];
			out[k+1] = base64_char[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
			out[k+2] = base64_char[ ((s[1]&0x0F)<<2)+((s[2]&0xC0)>>6) ];
			out[k+3] = base64_char[ s[2]&0x3F ];
			j=0; k+=4;
		}
	}

	if (j) {
		if (j==1)
			s[1] = 0;
		out[k+0] = base64_char[ (s[0]&255)>>2 ];
		out[k+1] = base64_char[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
		if (j==2)
			out[k+2] = base64_char[ ((s[1]&0x0F)<<2) ];
		else
			out[k+2] = '=';
		out[k+3] = '=';
		k+=4;
	}

	out[k] = '\0';

	return k;
}

unsigned int base64_decode(const unsigned char* in, unsigned int in_len, unsigned char* out) {

	unsigned int i=0, j=0, k=0, s[4];

	for (i=0;i<in_len;i++) {
		s[j++]=base64_int(*(in+i));
		if (j==4) {
			out[k+0] = ((s[0]&255)<<2)+((s[1]&0x30)>>4);
			if (s[2]!=64) {
				out[k+1] = ((s[1]&0x0F)<<4)+((s[2]&0x3C)>>2);
				if ((s[3]!=64)) {
					out[k+2] = ((s[2]&0x03)<<6)+(s[3]); k+=3;
				} else {
					k+=2;
				}
			} else {
				k+=1;
			}
			j=0;
		}
	}

    out[k] = '\0';

	return k;
}
