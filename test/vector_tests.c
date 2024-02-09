/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022-2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dogecoin/utils.h>
#include <dogecoin/vector.h>

struct teststruct {
    void* dummy1;
    void* dummy2;
};

void free_dummy(void* data)
{
    dogecoin_free(((struct teststruct*)data)->dummy1);
    dogecoin_free(((struct teststruct*)data)->dummy2);
    dogecoin_free((struct teststruct*)data);
}

void test_vector()
{
    dogecoin_bool res;
    char str0[] = "string";
    char str1[] = "rumba";
    vector* vec = vector_new(10, NULL);
    assert(vec != NULL);
    assert(vec->len == 0);
    assert(vec->alloc > 0);
    res = vector_add(vec, str0);
    assert(res == true);
    assert(vec->len == 1);
    res = vector_add(vec, str1);
    assert(res == true);
    assert(vec->len == 2);
    assert(vector_find(vec, str0) == 0);
    assert(vector_find(vec, "test") == -1);
    assert(vector_find(vec, str1) == 1);
    assert(strcmp(vector_idx(vec, 0), "string") == 0);
    assert(strcmp(vector_idx(vec, 1), "rumba") == 0);
    vector_remove_idx(vec, 0);
    assert(res == true);
    assert(strcmp(vector_idx(vec, 0), "rumba") == 0);
    vector_free(vec, true);
    vec = vector_new(10, free);
    res = vector_add(vec, strdup("TEST0"));
    assert(res == true);
    res = vector_add(vec, strdup("TEST1"));
    assert(res == true);
    char* a_str = strdup("TEST2");
    res = vector_add(vec, a_str);
    assert(res == true);
    assert(vec->len == 3);
    res = vector_remove(vec, a_str);
    assert(res == true);
    assert(vec->len == 2);
    vector_free(vec, true);
    /* test resize */
    vec = vector_new(1, free);
    res = vector_resize(vec, 30);
    assert(res == true);
    res = vector_resize(vec, 30);
    assert(res == true);
    char str[80];
    size_t i;
    for (i = 0; i < 20; i++) {
        sprintf(str, "TEST%ld", i);
        res = vector_add(vec, strdup(str));
        assert(res == true);
        assert(vec->len == i + 1);
    }
    res = vector_resize(vec, 5);
    assert(res == true);
    assert(strcmp(vector_idx(vec, 0), "TEST0") == 0);
    assert(strcmp(vector_idx(vec, 4), "TEST4") == 0);
    assert(vector_idx(vec, 5) == NULL);
    vector_remove_range(vec, 0, 4);
    assert(strcmp(vector_idx(vec, 0), "TEST4") == 0);
    vector_free(vec, true);
    /* test custom free callback handler */
    struct teststruct* some_data = dogecoin_calloc(1, sizeof(struct teststruct));
    some_data->dummy1 = dogecoin_calloc(1, 10);
    some_data->dummy2 = dogecoin_calloc(1, 10);
    vec = vector_new(1, free_dummy);
    vector_add(vec, some_data);
    vector_free(vec, true);
    /* test serialization */
    char serialized[MAX_SERIALIZE_SIZE] = { 0 };
    size_t len = 15;
    size_t written = 0;
    size_t read = 0;
    vec = vector_new(1, free);
    res = vector_add(vec, strdup("TEST0"));
    assert(res == true);
    res = vector_add(vec, strdup("TEST1"));
    assert(res == true);
    res = vector_add(vec, strdup("TEST2"));
    assert(res == true);
    assert(serializeVector(vec, serialized, len, &written) == true);
    assert(strcmp(serialized, "TEST0TEST1TEST2") == 0);
    /* test deserialization */
    assert(deserializeVector(vec, serialized, len, &read) == true);
    assert(strcmp(vector_idx(vec, 0), "TEST0") == 0);
    assert(strcmp(vector_idx(vec, 1), "TEST1") == 0);
    assert(strcmp(vector_idx(vec, 2), "TEST2") == 0);
    vector_free(vec, true);
}
