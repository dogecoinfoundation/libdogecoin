/*

 The MIT License (MIT)

 Copyright (c) 2016 libbtc developers
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dogecoin/mem.h>

void* dogecoin_malloc_internal(size_t size);
void* dogecoin_calloc_internal(size_t count, size_t size);
void* dogecoin_realloc_internal(void* ptr, size_t size);
void dogecoin_free_internal(void* ptr);

static const dogecoin_mem_mapper default_mem_mapper = {dogecoin_malloc_internal, dogecoin_calloc_internal, dogecoin_realloc_internal, dogecoin_free_internal};
static dogecoin_mem_mapper current_mem_mapper = {dogecoin_malloc_internal, dogecoin_calloc_internal, dogecoin_realloc_internal, dogecoin_free_internal};


/**
 * @brief This function sets the current memory mapper
 * to the default memory mapper.
 * 
 * @return Nothing.
 */
void dogecoin_mem_set_mapper_default()
{
    current_mem_mapper = default_mem_mapper;
}


/**
 * @brief This function sets the current memory mapper
 * to the specified mapper.
 * 
 * @param mapper The mapper to be used.
 * 
 * @return Nothing.
 */
void dogecoin_mem_set_mapper(const dogecoin_mem_mapper mapper)
{
    current_mem_mapper = mapper;
}


/**
 * @brief This function calls dogecoin_malloc_internal() using 
 * the current memory mapper.
 * 
 * @param size The size of the memory block to be allocated.
 * 
 * @return A pointer to the allocated memory. 
 */
void* dogecoin_malloc(size_t size)
{
    return current_mem_mapper.dogecoin_malloc(size);
}


/**
 * @brief This function calls dogecoin_calloc_internal() using
 * the current memory mapper.
 * 
 * @param count The number of elements to allocate.
 * @param size The size of each element.
 * 
 * @return A pointer to the allocated memory. 
 */
void* dogecoin_calloc(size_t count, size_t size)
{
    return current_mem_mapper.dogecoin_calloc(count, size);
}


/**
 * @brief This function calls dogecoin_realloc_internal() using
 * the current memory mapper.
 * 
 * @param ptr The pointer to the old memory block.
 * @param size The size of the new memory block to be reallocated.
 * 
 * @return A pointer to the reallocated memory.
 */
void* dogecoin_realloc(void* ptr, size_t size)
{
    return current_mem_mapper.dogecoin_realloc(ptr, size);
}


/**
 * @brief This function calls dogecoin_free_internal() using
 * the current memory mapper.
 * 
 * @param ptr The pointer to the memory block to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_free(void* ptr)
{
    current_mem_mapper.dogecoin_free(ptr);
}


/**
 * @brief This function allocates a memory block.
 * 
 * @param size The size of the memory block to be allocated.
 * 
 * @return A pointer to the allocated memory. 
 */
void* dogecoin_malloc_internal(size_t size)
{
    void* result;

    if ((result = malloc(size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: malloc failed in dogecoin_malloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}


/**
 * @brief This function allocates memory and initializes all
 * bytes to zero.
 * 
 * @param count The number of elements to allocate.
 * @param size The size of each element.
 * 
 * @return A pointer to the allocated memory. 
 */
void* dogecoin_calloc_internal(size_t count, size_t size)
{
    void* result;

    if ((result = calloc(count, size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: calloc failed in dogecoin_calloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}


/**
 * @brief This function resizes the memory block pointed to
 * by ptr which was previously allocated.
 * 
 * @param ptr The pointer to the old memory block.
 * @param size The size of the new memory block to be reallocated.
 * 
 * @return A pointer to the reallocated memory.
 */
void* dogecoin_realloc_internal(void* ptr, size_t size)
{
    void* result;

    if ((result = realloc(ptr, size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: realloc failed in dogecoin_realloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}


/**
 * @brief This function frees previously allocated memory.
 * 
 * @param ptr The pointer to the memory block to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_free_internal(void* ptr)
{
    free(ptr);
}

void* memcpy_safe(void* destination, const void* source, size_t count) {
    char *pszDest = (char *)destination;
    const char *pszSource =( const char*)source;
    if((pszDest!= NULL) && (pszSource!= NULL)) {
        while(count) {
            *(pszDest++)= *(pszSource++);
            --count;
        }
    }
    return destination;
}

errno_t memset_safe(volatile void *v, rsize_t smax, int c, rsize_t n) {
  if (v == NULL) return EINVAL;
  if (smax > RSIZE_MAX) return EINVAL;
  if (n > smax) return EINVAL;
 
  volatile unsigned char *p = v;
  while (smax-- && n--) {
    *p++ = c;
  }
 
  return 0;
}

/**
 * "memset_safe() is a secure version of memset()."
 * 
 * The memset_safe() function is a secure version of memset() that fills the memory pointed to by dst with
 * zeros
 * 
 * @param dst The destination buffer to be zeroed.
 * @param len The length of the memory block to zero.
 */
volatile void* dogecoin_mem_zero(volatile void* dst, size_t len)
{
    memset_safe(dst, len, 0, len);
    return 0;
}

uint32_t* dogecoin_uint32_vla(size_t size)
{
    uint32_t* outarray;
    outarray = (uint32_t*)malloc(size * sizeof(uint32_t));
    return outarray;
}

uint8_t* dogecoin_uint8_vla(size_t size)
{
    uint8_t* outarray;
    outarray = (uint8_t*)malloc(size * sizeof(uint8_t));
    return outarray;
}

char* dogecoin_char_vla(size_t size)
{
    char* outarray;
    outarray = (char*)malloc(size * sizeof(char));
    return outarray;
}

char* dogecoin_string_vla(size_t size)
{
    char* outarray;
    outarray = (char*)malloc(size + 1 * sizeof(char));
    outarray[size] = '\0';
    return outarray;
}

unsigned char* dogecoin_uchar_vla(size_t size)
{
    unsigned char* outarray;
    outarray = (unsigned char*)malloc(size * sizeof(unsigned char));
    return outarray;
}

unsigned char** dogecoin_ucharptr_vla(size_t size)
{
    unsigned char** outarray;
    outarray = (unsigned char**)malloc(size * sizeof(unsigned char*));
    return outarray;
}

uint8_t** dogecoin_uint8ptr_vla(size_t size)
{
    uint8_t** outarray;
    outarray = (uint8_t**)malloc(size * sizeof(uint8_t*));
    return outarray;
}
