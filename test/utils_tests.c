/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022-2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <test/utest.h>

#include <assert.h>
#include <dogecoin/utils.h>

  /* test a buffer overflow protection */
static const char hash_buffer_exc[] = "28969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";

static const char hex2[] = "AA969cdfFFffFF3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";

void test_utils()
    {
    size_t outlen = 0;
    unsigned char data[] = { 0x00, 0xFF, 0x00, 0xAA, 0x00, 0xFF, 0x00, 0xAA };
    char hash[] = "28969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";
    char hex[sizeof(data) * 2 + 1];
    unsigned char data2[sizeof(data)];
    uint8_t* hash_bin = utils_hex_to_uint8(hash);
    char* new = utils_uint8_to_hex(hash_bin, 32);
    unsigned char data3[64];
    assert(strncmp(new, hash, 64) == 0);

    utils_clear_buffers();

    utils_bin_to_hex(data, sizeof(data), hex);
    assert(strcmp(hex, "00ff00aa00ff00aa") == 0);

    utils_hex_to_bin(hex, data2, strlen(hex), &outlen);
    assert(outlen == 8);
    assert(memcmp(data, data2, outlen) == 0);
    utils_hex_to_uint8(hash_buffer_exc);

    /* test upper and lowercase A / F */
    utils_hex_to_bin(hex2, data3, strlen(hex2), &outlen);
    utils_hex_to_uint8(hex2);
    utils_clear_buffers();
    }

void test_net_flag_defined() {
    assert(dogecoin_network_enabled()==true);
}

void test_net_flag_not_defined() {
    assert(dogecoin_network_enabled()==false);
}

void test_base64() {
    static const char* vstrIn[]  = {"","f","fo","foo","foob","fooba","foobar"};
    static const char* vstrOut[] = {"","Zg==","Zm8=","Zm9v","Zm9vYg==","Zm9vYmE=","Zm9vYmFy"};
    for (unsigned int i=0; i<sizeof(vstrIn)/sizeof(vstrIn[0]); i++)
    {
    	int input_length = strlen(vstrIn[i]);
        unsigned char* enc_output = dogecoin_uchar_vla(1+(sizeof(char)*base64_encoded_size(input_length)));
        unsigned int enc_out_len = base64_encode((unsigned char*)vstrIn[i], input_length, enc_output);
        u_assert_str_eq((const char*)enc_output, vstrOut[i]);
        unsigned char* dec_output = dogecoin_uchar_vla(base64_decoded_size(strlen((const char*)enc_output)+1)+1);
        unsigned int dec_out_len = base64_decode(enc_output, enc_out_len, dec_output);
        u_assert_str_eq((const char*)dec_output, vstrIn[i]);
        u_assert_int_eq(input_length, dec_out_len);
        dogecoin_free(enc_output);
        dogecoin_free(dec_output);
    }
}
