/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dogecoin/hash.h>
#include <dogecoin/crypto/sha2.h>
#include <dogecoin/utils.h>

void test_DOGECOIN_hash()
{
    const char data[] = "cea946542b91ca50e2afecba73cf546ce1383d82668ecb6265f79ffaa07daa49abb43e21a19c6b2b15c8882b4bc01085a8a5b00168139dcb8f4b2bbe22929ce196d43532898d98a3b0ea4d63112ba25e724bb50711e3cf55954cf30b4503b73d785253104c2df8c19b5b63e92bd6b1ff2573751ec9c508085f3f206c719aa4643776bf425344348cbf63f1450389";

    const char expected[] = "52aa8dd6c598d91d580cc446624909e52a076064ffab67a1751f5758c9f76d26";
    uint8_t* digest_expected;
    digest_expected = utils_hex_to_uint8(expected);

    uint8_t hashout[32];
    dogecoin_hash((const unsigned char *)data, strlen(data), hashout);
    assert(memcmp(hashout, digest_expected, 32) == 0);
}
