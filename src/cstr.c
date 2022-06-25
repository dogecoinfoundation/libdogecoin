/* Copyright 2015 BitPay, Inc.
 * Copyright 2022 bluezr
 * Copyright 2022 The Dogecoin Foundation
 * Distributed under the MIT/X11 software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include <dogecoin/cstr.h>
#include <dogecoin/mem.h>


/**
 * @brief This function takes a cstring and allocates a new
 * buffer of the specified size. If the buffer is already 
 * allocated and is large enough, no change occurs.
 * 
 * @param s The cstring whose buffer is to be reallocated.
 * @param sz The new desired size of the buffer.
 * 
 * @return 1 if the buffer is allocated successfully, 0 otherwise.
 */
static int cstr_alloc_min_sz(cstring* s, size_t sz)
{
    unsigned int shift;
    unsigned int al_sz;
    char* new_s;

    sz++; /* NULL overhead */

    if (s->alloc && (s->alloc >= sz)) {
        return 1;
    }

    shift = 3;
    while ((al_sz = (1 << shift)) < sz) {
        shift++;
    }

    new_s = dogecoin_realloc(s->str, al_sz);
    if (!new_s) {
        return 0;
    }

    s->str = new_s;
    s->alloc = al_sz;
    s->str[s->len] = 0;

    return 1;
}


/**
 * @brief This function takes a cstring and allocates a new
 * buffer of the specified size. If the new size is greater than
 * the current, it allocates a new buffer of this size and copies
 * the buffer's contents to the new location. If the new size is
 * smaller, the string will be truncated.
 * 
 * @param s The pointer to the cstring whose buffer is to be resized.
 * @param sz The new desired size of the buffer.
 * 
 * @return 1 if the buffer was successfully resized, 0 otherwise.
 */
int cstr_alloc_minsize(cstring* s, size_t new_sz)
{
    /* no change */
    if (new_sz == s->len) {
        return 1;
    }

    /* truncate string */
    if (new_sz <= s->len) {
        return 0;
    }

    /* increase string size */
    if (!cstr_alloc_min_sz(s, new_sz)) {
        return 0;
    }

    /* contents of string tail undefined */
    // s->len = new_sz;
    s->str[s->len] = 0;

    return 1;
}


/**
 * @brief This function allocates a new cstring of the
 * specified size.
 * 
 * @param sz The size of the string to allocate.
 * 
 * @return a pointer to the new cstring object.
 */
cstring* cstr_new_sz(size_t sz)
{
    cstring* s = dogecoin_calloc(1, sizeof(cstring));
    if (!s) {
        return NULL;
    }

    if (!cstr_alloc_min_sz(s, sz)) {
        dogecoin_free(s);
        return NULL;
    }

    return s;
}


/**
 * @brief The function creates a new cstring and 
 * initializes it with the first sz bytes of the 
 * specified buffer.
 * 
 * @param buf The data to be copied into the cstring.
 * @param sz The size of the string to allocate and initialize.
 * 
 * @return A pointer to the new cstring object.
 */
cstring* cstr_new_buf(const void* buf, size_t sz)
{
    cstring* s = cstr_new_sz(sz);
    if (!s) {
        return NULL;
    }

    memcpy_safe(s->str, buf, sz);
    s->len = sz;
    s->str[s->len] = 0;

    return s;
}


/**
 * @brief This function creates a new cstring from
 * an existing cstring. 
 * 
 * @param copy_str The cstring object to be copied.
 * 
 * @return A pointer to the new cstring object.
 */
cstring* cstr_new_cstr(const cstring* copy_str)
{
    return cstr_new_buf(copy_str->str, copy_str->len);
}

cstring* cstr_new(const char* init_str)
{
    size_t slen;

    if (!init_str || !*init_str) {
        return cstr_new_sz(0);
    }

    slen = strlen(init_str);
    return cstr_new_buf(init_str, slen);
}


/**
 * @brief This function frees the memory allocated for
 * the cstring object.
 * 
 * @param s The pointer to the cstring to be freed.
 * @param free_buf Whether the buffer inside the cstring should be freed.
 * 
 * @return Nothing.
 */
void cstr_free(cstring* s, int free_buf)
{
    if (!s) {
        return;
    }

    if (free_buf) {
        dogecoin_free(s->str);
    }

    dogecoin_mem_zero(s, sizeof(*s));
    dogecoin_free(s);
}


/**
 * @brief This function resizes the buffer inside cstring object. 
 * 
 * @param s The pointer to the cstring whose buffer is to be resized.
 * @param new_sz The new desired size of the buffer.
 * 
 * @return 1 if buffer was resized successfully, 0 otherwise.
 */
int cstr_resize(cstring* s, size_t new_sz)
{
    /* no change */
    if (new_sz == s->len) {
        return 1;
    }

    /* truncate string */
    if (new_sz <= s->len) {
        s->len = new_sz;
        s->str[s->len] = 0;
        return 1;
    }

    /* increase string size */
    if (!cstr_alloc_min_sz(s, new_sz)) {
        return 0;
    }

    /* contents of string tail undefined */

    s->len = new_sz;
    s->str[s->len] = 0;

    return 1;
}


/**
 * @brief This function appends the contents of a buffer to the
 * buffer of the specified cstring, resizing the destination
 * buffer as necessary.
 * 
 * @param s The pointer to the cstring to append to.
 * @param buf The buffer to be appended.
 * @param sz The size of the buffer to be appended.
 * 
 * @return 1 if the data was appended successfully, 0 otherwise. 
 */
int cstr_append_buf(cstring* s, const void* buf, size_t sz)
{
    if (!cstr_alloc_min_sz(s, s->len + sz)) {
        return 0;
    }

    memcpy_safe(s->str + s->len, buf, sz);
    s->len += sz;
    s->str[s->len] = 0;

    return 1;
}


/**
 * @brief This function appends the contents of one cstring
 * to another, resizing the destination buffer as necessary.
 * 
 * @param s The pointer to the cstring to append to.
 * @param append The pointer to the cstring whose buffer will be appended.
 * 
 * @return 1 if the cstring was appended successfully, 0 otherwise.
 */
int cstr_append_cstr(cstring* s, cstring* append)
{
    return cstr_append_buf(s, append->str, append->len);
}


/**
 * @brief This function appends a single character to the
 * buffer of a cstring.
 * 
 * @param s The pointer to the cstring to append to. 
 * @param ch The character to be appended.
 * 
 * @return 1 if the character was appended successfully, 0 otherwise.
 */
int cstr_append_c(cstring* s, char ch)
{
    return cstr_append_buf(s, &ch, 1);
}


/**
 * @brief This function compares two cstrings and checks
 * if their contents are equal.
 * 
 * @param a The pointer to the reference cstring.
 * @param b The pointer to the cstring to compare.
 * 
 * @return 1 if the contents are equal, 0 otherwise.
 */
int cstr_equal(const cstring* a, const cstring* b)
{
    if (a == b) {
        return 1;
    }
    if (!a || !b) {
        return 0;
    }
    if (a->len != b->len) {
        return 0;
    }
    return (memcmp(a->str, b->str, a->len) == 0);
}


/**
 * @brief This function compares the buffers of two cstrings
 * and returns which one is greater.
 * 
 * @param a The pointer to the cstring to compare.
 * @param b The pointer to the reference cstring.
 * 
 * @return 1 if the string value of a is greater, -1 if the
 * string value of b is greater, 0 if the strings are equal.
 */
int cstr_compare(const cstring* a, const cstring* b)
{
    unsigned int i;
    if (a->len > b->len) {
        return (1);
    }
    if (a->len < b->len) {
        return (-1);
    }

    /* length equal, byte per byte compare */
    for (i = 0; i < a->len; i++) {
        char a1 = a->str[i];
        char b1 = b->str[i];

        if (a1 > b1) {
            return (1);
        }
        if (a1 < b1) {
            return (-1);
        }
    }
    return (0);
}


/**
 * @brief This function erases the characters in the string starting
 * at position pos and ending at pos + len.
 * 
 * @param s The pointer to the cstring to modify.
 * @param pos The index of the buffer to start erasing from.
 * @param len The number of characters to remove.
 * 
 * @return The number of characters successfully erased.
 */
int cstr_erase(cstring* s, size_t pos, ssize_t len)
{
    ssize_t old_tail;

    if (pos == s->len && len == 0) {
        return 1;
    }
    if (pos >= s->len) {
        return 0;
    }

    old_tail = s->len - pos;
    if ((len >= 0) && (len > old_tail)) {
        return 0;
    }

    memmove(&s->str[pos], &s->str[pos + len], old_tail - len);
    s->len -= len;
    s->str[s->len] = 0;

    return 1;
}
