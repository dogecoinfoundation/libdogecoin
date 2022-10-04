/*

 The MIT License (MIT)

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

#include <dogecoin/mem.h>
#include <dogecoin/vector.h>


/**
 * @brief This function creates a new vector object
 * and initializes it to 0.
 * 
 * @param res The size of memory to allocate for the vector's contents.
 * @param free_f The function that will be called when a vector element is freed.
 * 
 * @return A pointer to the new vector object.
 */
vector* vector_new(size_t res, void (*free_f)(void*))
{
    vector* vec = dogecoin_calloc(1, sizeof(vector));
    if (!vec)
        return NULL;

    vec->alloc = 8;
    while (vec->alloc < res)
        vec->alloc *= 2;

    vec->elem_free_f = free_f;
    vec->data = dogecoin_calloc(1, vec->alloc * sizeof(void*));
    if (!vec->data) {
        dogecoin_free(vec);
        return NULL;
    }

    return vec;
}


/**
 * @brief This function frees all of a vector's elements,
 * calling the function associated with its free operation
 * set during vector creation. The vector object itself 
 * is not freed.
 * 
 * @param vec The pointer to the vector to be freed.
 * 
 * @return Nothing.
 */
static void vector_free_data(vector* vec)
{
    if (!vec->data)
        return;

    if (vec->elem_free_f) {
        unsigned int i;
        for (i = 0; i < vec->len; i++)
            if (vec->data[i]) {
                vec->elem_free_f(vec->data[i]);
                vec->data[i] = NULL;
            }
    }

    dogecoin_free(vec->data);
    vec->data = NULL;
    vec->alloc = 0;
    vec->len = 0;
}


/**
 * @brief This function frees an entire vector 
 * object and all of its elements if specified.
 * 
 * @param vec The pointer to the vector to be freed.
 * @param free_array The flag denoting whether to free the vector's elements.
 * 
 * @return Nothing.
 */
void vector_free(vector* vec, dogecoin_bool free_array)
{
    if (!vec) {
        return;
    }

    if (free_array) {
        vector_free_data(vec);
    }

    dogecoin_mem_zero(vec, sizeof(*vec));
    dogecoin_free(vec);
}


/**
 * @brief This function grows the vector by doubling
 * in size until it is larger than the size specified.
 * 
 * @param vec The pointer to the vector to be grown.
 * @param min_sz The minimum size the vector must be grown to.
 * 
 * @return 1 if the vector is grown successfully, 0 if it reaches the max size allowed.
 */
static dogecoin_bool vector_grow(vector* vec, size_t min_sz)
{
    size_t new_alloc = vec->alloc;
    while (new_alloc < min_sz) {
        new_alloc *= 2;
    }

    if (vec->alloc == new_alloc) {
        return true;
    }

    void* new_data = dogecoin_realloc(vec->data, new_alloc * sizeof(void*));
    if (!new_data) {
        return false;
    }

    vec->data = new_data;
    vec->alloc = new_alloc;
    return true;
}


/**
 * @brief This function finds and returns the first element
 * in the vector whose data matches the data specified.
 * 
 * @param vec The pointer to the vector to search.
 * @param data The data to match.
 * 
 * @return The index of the data if it exists in the vector, -1 otherwise.
 */
ssize_t vector_find(vector* vec, void* data)
{
    if (vec && vec->len) {
        size_t i;
        for (i = 0; i < vec->len; i++) {
            if (vec->data[i] == data) {
                return (ssize_t)i;
            }
        }
    }

    return -1;
}


/**
 * @brief This function adds an element to an existing
 * vector, growing it by one if necessary.
 * 
 * @param vec The pointer to the vector to add to.
 * @param data The data to be added into the vector.
 * 
 * @return 1 if the element was added successfully, 0 otherwise.
 */
dogecoin_bool vector_add(vector* vec, void* data)
{
    if (vec->len == vec->alloc) {
        if (!vector_grow(vec, vec->len + 1)) {
            return false;
        }
    }

    vec->data[vec->len] = data;
    vec->len++;
    return true;
}


/**
 * @brief This function deletes a range of consecutive
 * elements from the specified vector.
 * 
 * @param vec The pointer to the vector to edit.
 * @param pos The index of the first item to remove.
 * @param len The number of consecutive elements to remove.
 * 
 * @return Nothing.
 */
void vector_remove_range(vector* vec, size_t pos, size_t len)
{
    if (!vec || ((pos + len) > vec->len)) {
        return;
    }

    if (vec->elem_free_f) {
        size_t i, count;
        for (i = pos, count = 0; count < len; i++, count++) {
            vec->elem_free_f(vec->data[i]);
        }
    }

    memmove(&vec->data[pos], &vec->data[pos + len], (vec->len - pos - len) * sizeof(void*));
    vec->len -= len;
}


/**
 * @brief This function removes a single element from
 * the specified vector.
 * 
 * @param vec The pointer to the vector to edit.
 * @param pos The index of the element to remove.
 */
void vector_remove_idx(vector* vec, size_t pos)
{
    vector_remove_range(vec, pos, 1);
}


/**
 * @brief This function finds an element whose data
 * matches the data specified and removes it if it
 * exists.
 * 
 * @param vec The pointer to the vector to edit.
 * @param data The data to match.
 * 
 * @return 1 if the element was removed successfully, 0 if the data was not found.
 */
dogecoin_bool vector_remove(vector* vec, void* data)
{
    ssize_t idx = vector_find(vec, data);
    if (idx < 0) {
        return false;
    }

    vector_remove_idx(vec, idx);
    return true;
}


/**
 * @brief This function resizes the vector to be newsz
 * elements long. If the new size is bigger, the vector
 * is grown and the new elements are left empty. If the
 * new size is smaller, vector elements will be truncated.
 * If the new size is the same, do nothing.
 * 
 * @param vec The pointer to the vector to resize.
 * @param newsz The new desired size of the vector.
 * 
 * @return 1 if the vector was resized successfully, 0 otherwise.
 */
dogecoin_bool vector_resize(vector* vec, size_t newsz)
{
    size_t i;

    /* same size */
    if (newsz == vec->len) {
        return true;
    }

    /* truncate */
    else if (newsz < vec->len) {
        size_t del_count = vec->len - newsz;

        for (i = (vec->len - del_count); i < vec->len; i++) {
            if (vec->elem_free_f) {
                vec->elem_free_f(vec->data[i]);
            }
            vec->data[i] = NULL;
        }

        vec->len = newsz;
        return true;
    }

    /* last possibility: grow */
    if (!vector_grow(vec, newsz)) {
        return false;
    }

    /* set new elements to NULL */
    for (i = vec->len; i < newsz; i++) {
        vec->data[i] = NULL;
    }

    return true;
}
