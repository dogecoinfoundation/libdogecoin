#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>

/**
 * NIST Test Vectors for SHA-1 and HMAC-SHA1
 * Reference: https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
 * RFC 2202: https://datatracker.ietf.org/doc/html/rfc2202
 */

struct sha1_test_v_short {
    int len;
    const char* msg;
    const char* digest_hex;
};

struct sha_hmac_test_v {
    const uint8_t* key;
    size_t key_len;
    const uint8_t* msg;
    size_t msg_len;
    const char* digest_hex;
};

static const struct sha1_test_v_short nist_sha1_test_vectors_short[] = {
    {0, "", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
    {3, "abc", "a9993e364706816aba3e25717850c26c9cd0d89d"},
    {56, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
    {112, "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
           "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", "a49b2446a02c645bf419f995b67091253a04a259"},
    {0, NULL, NULL}
};

static const struct sha_hmac_test_v sha_hmac_test_vectors[] = {
    {(const uint8_t*)"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 20,
     (const uint8_t*)"Hi There", 8,
     "b617318655057264e28bc0b6fb378c8ef146be00"},
    {(const uint8_t*)"Jefe", 4,
     (const uint8_t*)"what do ya want for nothing?", 28,
     "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"},
    {(const uint8_t*)"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 20,
     (const uint8_t*)"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd", 50,
     "125d7342b9ac11cd91a39af48aa17b4f63f175d3"},
    {(const uint8_t*)"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19", 25,
     (const uint8_t*)"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd", 50,
     "4c9007f4026250c6bc8414f9bf50c86c2d7235da"},
    {(const uint8_t*)"\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c", 20,
     (const uint8_t*)"Test With Truncation", 20,
     "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04"},
    {(const uint8_t*)"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 80,
     (const uint8_t*)"Test Using Larger Than Block-Size Key - Hash Key First", 54,
     "aa4ae5e15272d00e95705637ce8a3b55ed402112"},
    {(const uint8_t*)"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 80,
     (const uint8_t*)"Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data", 73,
     "e8e99d0f45237d786d6bbaa7965c7808bbff1a91"},
    {NULL, 0, NULL, 0, NULL}
};

void test_sha1() {
    sha1_context context;
    uint8_t digest[SHA1_DIGEST_LENGTH];
    unsigned int i;

    for (i = 0; nist_sha1_test_vectors_short[i].msg != NULL; ++i) {
        sha1_Init(&context);
        sha1_Update(&context, (const uint8_t*)nist_sha1_test_vectors_short[i].msg, nist_sha1_test_vectors_short[i].len);
        sha1_Final(&context, digest);

        uint8_t expected_digest[SHA1_DIGEST_LENGTH];
        size_t expected_digest_len = sizeof(expected_digest);
        utils_hex_to_bin(nist_sha1_test_vectors_short[i].digest_hex, expected_digest, SHA1_DIGEST_LENGTH * 2, &expected_digest_len);

        assert(memcmp(digest, expected_digest, SHA1_DIGEST_LENGTH) == 0);
        debug_print("SHA1 Test %d passed.\n", i + 1);
    }
}

void test_sha1_hmac() {
    uint8_t hmac[SHA1_DIGEST_LENGTH];
    unsigned int i;

    for (i = 0; sha_hmac_test_vectors[i].key != NULL; ++i) {
        hmac_sha1(sha_hmac_test_vectors[i].key, sha_hmac_test_vectors[i].key_len,
                  sha_hmac_test_vectors[i].msg, sha_hmac_test_vectors[i].msg_len, hmac);

        uint8_t expected_hmac[SHA1_DIGEST_LENGTH];
        size_t expected_hmac_len = sizeof(expected_hmac);
        utils_hex_to_bin(sha_hmac_test_vectors[i].digest_hex, expected_hmac, SHA1_DIGEST_LENGTH * 2, &expected_hmac_len);

        assert(memcmp(hmac, expected_hmac, SHA1_DIGEST_LENGTH) == 0);
        debug_print("HMAC-SHA1 Test %d passed.\n", i + 1);
    }
}
