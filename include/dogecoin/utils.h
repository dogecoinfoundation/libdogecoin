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

#ifndef __LIBDOGECOIN_UTILS_H__
#define __LIBDOGECOIN_UTILS_H__

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/mem.h>
#include <dogecoin/vector.h>

#define TO_UINT8_HEX_BUF_LEN 2048
#define VARINT_LEN 20
#define MAX_LEN 128

#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))
#define UBEGIN(a)           ((unsigned char*)&(a))
#define UEND(a)             ((unsigned char*)&((&(a))[1]))
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

#define strlens(s) (s == NULL ? 0 : strlen(s))

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API void utils_clear_buffers(void);
LIBDOGECOIN_API void utils_hex_to_bin(const char* str, unsigned char* out, size_t inLen, size_t* outLen);
LIBDOGECOIN_API void utils_bin_to_hex(unsigned char* bin_in, size_t inlen, char* hex_out);
LIBDOGECOIN_API uint8_t* utils_hex_to_uint8(const char* str);
LIBDOGECOIN_API char* utils_uint8_to_hex(const uint8_t* bin, size_t l);
LIBDOGECOIN_API void utils_reverse_hex(char* h, size_t len);
LIBDOGECOIN_API signed char utils_hex_digit(char c);
LIBDOGECOIN_API void utils_uint256_sethex(char* psz, uint8_t* out);
LIBDOGECOIN_API uint256* uint256S(const char *str);
LIBDOGECOIN_API unsigned char* parse_hex(const char* psz);
LIBDOGECOIN_API void swap_bytes(uint8_t *buf, int buf_size);
LIBDOGECOIN_API const char *find_needle(const char *haystack, size_t haystack_length, const char *needle, size_t needle_length);
LIBDOGECOIN_API uint8_t* bytes_find(uint8_t* haystack, size_t haystackLen, uint8_t* needle, size_t needleLen);
LIBDOGECOIN_API char* to_string(uint8_t* x);
LIBDOGECOIN_API char* hash_to_string(uint8_t* x);
LIBDOGECOIN_API uint8_t* hash_to_bytes(uint8_t* x);
LIBDOGECOIN_API void* safe_malloc(size_t size);
LIBDOGECOIN_API void dogecoin_cheap_random_bytes(uint8_t* buf, size_t len);
LIBDOGECOIN_API void dogecoin_get_default_datadir(cstring* path_out);
LIBDOGECOIN_API void dogecoin_file_commit(FILE* file);
LIBDOGECOIN_API void print_image(FILE *fptr);
LIBDOGECOIN_API void print_header(char *filepath);
LIBDOGECOIN_API uint8_t* bytearray_concatenate(uint8_t* input1, uint8_t* input2);
LIBDOGECOIN_API void print_bits(size_t const size, void const* ptr);
LIBDOGECOIN_API void prepend(char* s, const char* t);
LIBDOGECOIN_API void append(char* s, char* t);
LIBDOGECOIN_API char* concat(char* prefix, char* suffix);
LIBDOGECOIN_API void slice(const char *str, char *result, size_t start, size_t end);
LIBDOGECOIN_API void replace_last_after_delim(const char *str, char* delim, char* replacement);
LIBDOGECOIN_API void text_to_hex(char* in, char* out);
LIBDOGECOIN_API const char* get_build();
LIBDOGECOIN_API char* getpass(const char *prompt);
LIBDOGECOIN_API void dogecoin_str_reverse(char s[]);
LIBDOGECOIN_API void dogecoin_uitoa(int n, char s[]);
LIBDOGECOIN_API bool dogecoin_network_enabled();
LIBDOGECOIN_API int integer_length(int x);
LIBDOGECOIN_API int file_copy (char src [], char dest []);
unsigned int base64_int(unsigned int ch);
unsigned int base64_encoded_size(unsigned int in_size);
unsigned int base64_decoded_size(unsigned int in_size);
unsigned int base64_encode(const unsigned char* in, unsigned int in_len, unsigned char* out);
unsigned int base64_decode(const unsigned char* in, unsigned int in_len, unsigned char* out);

#define _SEARCH_PRIVATE
#ifdef _SEARCH_PRIVATE
/* support substitute for GNU only tdestroy */
/* let's hope the node struct is always compatible */
typedef struct dogecoin_btree_node {
    void *key;
    struct dogecoin_btree_node *left, *right;
} dogecoin_btree_node_t;
#endif
#define _DIAGASSERT assert

// Destroy a tree and free all allocated resources.
// This is a GNU extension, not available from NetBSD.
static inline void dogecoin_btree_tdestroy(void *root, void (*freekey)(void *))
{
    dogecoin_btree_node_t *r = (dogecoin_btree_node_t*)root;

    if (r == 0)
        return;
    if (freekey) goto end;
    if (r->left && !freekey) dogecoin_btree_tdestroy(r->left, freekey);
    if (r->right && !freekey) dogecoin_btree_tdestroy(r->right, freekey);

end:
    if (freekey) freekey(r->key);
    dogecoin_free(r);
}

/* delete node with given key */
static inline void *
dogecoin_btree_tdelete(const void *vkey,	/* key to be deleted */
	void      **vrootp,	/* address of the root of tree */
	int       (*compar)(const void *, const void *))
{
	dogecoin_btree_node_t **rootp = (dogecoin_btree_node_t **)vrootp;
	dogecoin_btree_node_t *p, *q, *r;
	int  cmp;

	_DIAGASSERT((uintptr_t)compar != (uintptr_t)NULL);

	if (rootp == NULL || (p = *rootp) == NULL)
		return NULL;

	while ((cmp = (*compar)(vkey, (*rootp)->key)) != 0) {
		p = *rootp;
		rootp = (cmp < 0) ?
		    &(*rootp)->left :		/* follow left branch */
		    &(*rootp)->right;		/* follow right branch */
		if (*rootp == NULL)
			return NULL;		/* key not found */
	}
	r = (*rootp)->right;			/* D1: */
	if ((q = (*rootp)->left) == NULL)	/* Left NULL? */
		q = r;
	else if (r != NULL) {			/* Right link is NULL? */
		if (r->left == NULL) {		/* D2: Find successor */
			r->left = q;
			q = r;
		} else {			/* D3: Find NULL link */
			for (q = r->left; q->left != NULL; q = r->left)
				r = q;
			r->left = q->right;
			q->left = (*rootp)->left;
			q->right = (*rootp)->right;
		}
	}
	dogecoin_free(*rootp);				/* D4: Free node */
	*rootp = q;				/* link parent to new node */
	return p;
}

/* find a node, or return 0 */
static inline void *
dogecoin_btree_tfind (const void *vkey, void * const *vrootp,
       int (*compar) (const void *, const void *))
{
  dogecoin_btree_node_t * const *rootp = (dogecoin_btree_node_t * const*)vrootp;

  if (rootp == NULL)
    return NULL;

  while (*rootp != NULL)
    {
      /* T1: */
      int r;

      if ((r = (*compar)(vkey, (*rootp)->key)) == 0)	/* T2: */
	return *rootp;		/* key found */
      rootp = (r < 0) ?
	  &(*rootp)->left :		/* T3: follow left branch */
	  &(*rootp)->right;		/* T4: follow right branch */
    }
  return NULL;
}

/* find or insert datum into search tree */
static inline void *
dogecoin_btree_tsearch (const void * __restrict vkey,		/* key to be located */
	 void ** __restrict vrootp,		/* address of tree root */
	 int (*compar) (const void *, const void *))
{
  dogecoin_btree_node_t *q, **n;
  dogecoin_btree_node_t **rootp = (dogecoin_btree_node_t **)vrootp;

  if (rootp == NULL)
    return NULL;

  n = rootp;
  while (*n != NULL)
    {
      /* Knuth's T1: */
      int r;

      if ((r = (*compar)(vkey, ((*n)->key))) == 0)	/* T2: */
	return *n;		/* we found it! */

      n = (r < 0) ?
	  &(*rootp)->left :		/* T3: follow left branch */
	  &(*rootp)->right;		/* T4: follow right branch */
      if (*n == NULL)
        break;
      rootp = n;
    }

  q = dogecoin_malloc(sizeof(dogecoin_btree_node_t));		/* T5: key not found */
  if (!q)
    return q;
  *n = q;
  /* make new node */
  /* LINTED const castaway ok */
  q->key = (void *)vkey;		/* initialize new node */
  q->left = q->right = NULL;
  return q;
}

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_UTILS_H__
