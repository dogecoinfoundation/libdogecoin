/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
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

#include <dogecoin/chainparams.h>
#include <dogecoin/utils.h>

const dogecoin_chainparams dogecoin_chainparams_main = {
    "main",
    0x1e, // 30, starts with D
    0x16, // 22, starts with 9 or A
    "doge",                   // bech32_hrp planned for 0.21
    0x9e,                     // 158, starts with 6 (uncompressed) or Q (compressed)
    0x02fac398,               // starts with dgpv
    0x02facafd,               // starts with dgub
    {0xc0, 0xc0, 0xc0, 0xc0}, // pch msg prefixes (magic bytes)
    {0x91, 0x56, 0x35, 0x2c, 0x18, 0x18, 0xb3, 0x2e, 0x90, 0xc9, 0xe7, 0x92, 0xef, 0xd6, 0xa1, 0x1a, 0x82, 0xfe, 0x79, 0x56, 0xa6, 0x30, 0xf0, 0x3b, 0xbe, 0xe2, 0x36, 0xce, 0xda, 0xe3, 0x91, 0x1a},
    22556,
    {{"seed.multidoge.org"}, {{1}}},
    true,
    0x0062,
    {0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} // pow limit
};

const dogecoin_chainparams dogecoin_chainparams_test = {
    "testnet3",
    0x71, // 113 starts with n
    0xc4, // 196 starts with 2
    "tdge",                   // bech32_hrp 0.21
    0xf1,                     // 241 starts with 9 (uncompressed) or c (compressed)
    0x04358394,               // starts with tprv
    0x043587cf,               // starts with tpub
    {0xfc, 0xc1, 0xb7, 0xdc}, // pch msg prefixes (magic bytes)
    {0x9e, 0x55, 0x50, 0x73, 0xd0, 0xc4, 0xf3, 0x64, 0x56, 0xdb, 0x89, 0x51, 0xf4, 0x49, 0x70, 0x4d, 0x54, 0x4d, 0x28, 0x26, 0xd9, 0xaa, 0x60, 0x63, 0x6b, 0x40, 0x37, 0x46, 0x26, 0x78, 0x0a, 0xbb},
    44556,
    {{"testseed.jrn.me.uk"}, {{0}}},
    false,
    0x0062,
    {0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} // pow limit
};

const dogecoin_chainparams dogecoin_chainparams_regtest = {
    "regtest",
    0x6f,
    0xc4,
    "dcrt", // bech32_hrp 0.21
    0xEF,
    0x04358394,               // starts with tprv
    0x043587cf,               // starts with tpub
    {0xfa, 0xbf, 0xb5, 0xda}, // pch msg prefixes (magic bytes)
    {0xa5, 0x73, 0xe9, 0x1c, 0x17, 0x72, 0x07, 0x6c, 0x0d, 0x40, 0xf7, 0x0e, 0x44, 0x08, 0xc8, 0x3a, 0x31, 0x70, 0x5f, 0x29, 0x6a, 0xe6, 0xe7, 0x62, 0x9d, 0x4a, 0xdc, 0xb5, 0xa3, 0x60, 0x21, 0x3d},
    18332,
    {{"testseed.jrn.me.uk"}, {{0}}},
    true,
    0x0062,
    {0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff} // pow limit
};

const dogecoin_checkpoint dogecoin_mainnet_checkpoint_array[] = {
    {0, "1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691", 1386325540, 0x1e0ffff0},
    {104679, "35eb87ae90d44b98898fec8c39577b76cb1eb08e1261cfc10706c8ce9a1d01cf", 1392637497, 0x1b41676b},
    {145000, "cc47cae70d7c5c92828d3214a266331dde59087d4a39071fa76ddfff9b7bde72", 1395094679, 0x1b499dfd},
    {371337, "60323982f9c5ff1b5a954eac9dc1269352835f47c2c5222691d80f0d50dcf053", 1410464577, 0x1b364184},
    {450000, "d279277f8f846a224d776450aa04da3cf978991a182c6f3075db4c48b173bbd7", 1415413000, 0x1b03efda},
    {771275, "1b7d789ed82cbdc640952e7e7a54966c6488a32eaad54fc39dff83f310dbaaed", 1435666139, 0x1b0444d2},
    {1000000, "6aae55bea74235f0c80bd066349d4440c31f2d0f27d54265ecd484d8c1d11b47", 1450031952, 0x1b02dcf8},
    {1250000, "00c7a442055c1a990e11eea5371ca5c1c02a0677b33cc88ec728c45edc4ec060", 1465690401, 0x1b045d00},
    {1500000, "f1d32d6920de7b617d51e74bdf4e58adccaa582ffdc8657464454f16a952fca6", 1481313507, 0x1b0306ef},
    {1750000, "5c8e7327984f0d6f59447d89d143e5f6eafc524c82ad95d176c5cec082ae2001", 1496985750, 0x1b00d4d4},
    {2000000, "9914f0e82e39bbf21950792e8816620d71b9965bdbbc14e72a95e3ab9618fea8", 1512600918, 0x1a1c1225},
    {2031142, "893297d89afb7599a3c571ca31a3b80e8353f4cf39872400ad0f57d26c4c5d42", 1514549787, 0x1a15d633},
    {2250000, "0a87a8d4e40dca52763f93812a288741806380cd569537039ee927045c6bc338", 1528215255, 0x1a053477},
    {2510150, "77e3f4a4bcb4a2c15e8015525e3d15b466f6c022f6ca82698f329edef7d9777e", 1544484077, 0x1a0868af},
    {2750000, "d4f8abb835930d3c4f92ca718aaa09bef545076bd872354e0b2b85deefacf2e3", 1559459044, 0x1a0292e2},
    {3000000, "195a83b091fb3ee7ecb56f2e63d01709293f57f971ccf373d93890c8dc1033db", 1575096781, 0x1a07da10},
    {3250000, "7f3e28bf9e309c4b57a4b70aa64d3b2ea5250ae797af84976ddc420d49684034", 1590799169, 0x1a05bfa3},
    {3500000, "eaa303b93c1c64d2b3a2cdcf6ccf21b10cc36626965cc2619661e8e1879abdfb", 1606543340, 0x1a08d505},
    {3606083, "954c7c66dee51f0a3fb1edb26200b735f5275fe54d9505c76ebd2bcabac36f1e", 1613218169, 0x1a03d764},
    {3854173, "e4b4ecda4c022406c502a247c0525480268ce7abbbef632796e8ca1646425e75", 1628934997, 0x1a03ca36},
    {3963597, "2b6927cfaa5e82353d45f02be8aadd3bfd165ece5ce24b9bfa4db20432befb5d", 1635884460, 0x1a037bc9},
    {4303965, "ed7d266dcbd8bb8af80f9ccb8deb3e18f9cc3f6972912680feeb37b090f8cee0", 1657646310, 0x1a0344f5}};

const dogecoin_checkpoint dogecoin_testnet_checkpoint_array[] = {
    {0, "bb0a78264637406b6360aad926284d544d7049f45189db5664f3c4d07350559e", 1391503289, 0x1e0ffff0},
    {483173, "a804201ca0aceb7e937ef7a3c613a9b7589245b10cc095148c4ce4965b0b73b5", 1427629321, 0x1e0fffff},
    {591117, "5f6b93b2c28cedf32467d900369b8be6700f0649388a7dbfd3ebd4a01b1ffad8", 1431705386, 0x1d104d88},
    {658924, "ed6c8324d9a77195ee080f225a0fca6346495e08ded99bcda47a8eea5a8a620b", 1433993669, 0x1e0642c3},
    {703635, "839fa54617adcd582d53030a37455c14a87a806f6615aa8213f13e196230ff7f", 1440601451, 0x1e0fffff},
    {1000000, "1fe4d44ea4d1edb031f52f0d7c635db8190dc871a190654c41d2450086b8ef0e", 1495653305, 0x1e0fffff},
    {1202214, "a2179767a87ee4e95944703976fee63578ec04fa3ac2fc1c9c2c83587d096977", 1514565123, 0x1e0d406a},
    {1250000, "b46affb421872ca8efa30366b09694e2f9bf077f7258213be14adb05a9f41883", 1524645876, 0x1e0fffff},
    {1500000, "0caa041b47b4d18a4f44bdc05cef1a96d5196ce7b2e32ad3e4eb9ba505144917", 1544835691, 0x1e0fffff},
    {1750000, "8042462366d854ad39b8b95ed2ca12e89a526ceee5a90042d55ebb24d5aab7e9", 1549617048, 0x1d027d2a},
    {2000000, "d6acde73e1b42fc17f29dcc76f63946d378ae1bd4eafab44d801a25be784103c", 1563868614, 0x1e0bf5da},
    {2250000, "c4342ae6d9a522a02e5607411df1b00e9329563ef844a758d762d601d42c86dc", 1586841190, 0x1e0fffff},
    {2500000, "3a66ec4933fbb348c9b1889aaf2f732fe429fd9a8f74fee6895eae061ac897e2", 1590565063, 0x1e0e2221},
    {2750000, "473ea9f625d59f534ffcc9738ffc58f7b7b1e0e993078614f5484a9505885563", 1594694481, 0x1d00ee30},
    {3062910, "113c41c00934f940a41f99d18b2ad9aefd183a4b7fe80527e1e6c12779bd0246", 1613218844, 0x1e0e2221},
    {3286675, "07fef07a255d510297c9189dc96da5f4e41a8184bc979df8294487f07fee1cf3", 1628932841, 0x1e0fffff},
    {3445426, "70574db7856bd685abe7b0a8a3e79b29882620645bd763b01459176bceb58cd1", 1635884611, 0x1e0fffff},
    {3976284, "af23c3e750bb4f2ce091235f006e7e4e2af453d4c866282e7870471dcfeb4382", 1657646588, 0x1e0fffff}};

const dogecoin_chainparams* chain_from_b58_prefix(const char* address) {
    /* determine address prefix for network chainparams */
    uint8_t prefix[1];
    memcpy(prefix, address, 1);
    int count = 0;
    switch (prefix[0]) {
        case '9':
            count++;
            break;
        case 'A':
            count++;
            break;
        case 'd':
            count++;
            break;
        case 'D':
            count++;
            break;
        case 'Q':
            count++;
            break;
        case '6':
            count++;
            break;
    }
    return count ? &dogecoin_chainparams_main : &dogecoin_chainparams_test;
}

int chain_from_b58_prefix_bool(char* address) {
    /* determine address prefix for network chainparams */
    uint8_t prefix[1];
    memcpy(prefix, address, 1);
    switch (prefix[0]) {
        case 'd':
            return true;
        case 'D':
            return true;
    }
    return false;
}

/* check if a given address is a testnet address */
dogecoin_bool isTestnetFromB58Prefix(const char address[P2PKHLEN]) {
    /* Determine address prefix for network chainparams */
    const dogecoin_chainparams* chainparams = chain_from_b58_prefix(address);

    /* Check if chainparams is testnet */
    return (chainparams == &dogecoin_chainparams_test);
}

/* check if a given address is a mainnet address */
dogecoin_bool isMainnetFromB58Prefix(const char address[P2PKHLEN]) {
    /* Determine address prefix for network chainparams */
    const dogecoin_chainparams* chainparams = chain_from_b58_prefix(address);

    /* Check if chainparams is mainnet */
    return (chainparams == &dogecoin_chainparams_main);
}
