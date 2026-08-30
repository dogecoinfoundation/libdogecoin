/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022-2024 The Dogecoin Foundation

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

#include <dogecoin/base58.h>
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
    {0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    22556,
    {{"seed.multidoge.org"}, {{"seed2.multidoge.org"}}},
    true,
    0x0062,
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00}, // pow limit 0x00000fffff..ff, internal byte order
    {0x9b, 0xa4, 0x46, 0xf2, 0x6c, 0xa8, 0x2a, 0x3d, 0x99, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
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
    {0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    44556,
    {{"testseed.jrn.me.uk"}, {{0}}},
    false,
    0x0062,
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00}, // pow limit 0x00000fffff..ff, internal byte order
    {0x26, 0x9a, 0xff, 0x62, 0x2f, 0x0f, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
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
    {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    18332,
    {{"testseed.jrn.me.uk"}, {{0}}},
    true,
    0x0062,
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f}, // pow limit
    {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

const dogecoin_checkpoint dogecoin_mainnet_checkpoint_array[] = {
    {0, "1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691", 1386325540, 0x1e0ffff0},
    {100000, "13ab3b961fcc500c03f51279385c42e9f055d48a37dfa72d0073c0d3f595036b", 1392346781, 0x1b267eeb},
    {104679, "35eb87ae90d44b98898fec8c39577b76cb1eb08e1261cfc10706c8ce9a1d01cf", 1392637497, 0x1b41676b},
    {145000, "cc47cae70d7c5c92828d3214a266331dde59087d4a39071fa76ddfff9b7bde72", 1395094679, 0x1b499dfd},
    {200000, "092fd3e76db5ff35fbfefe48d5c53ca26e799f0654a4036ddd5fd78de77418c2", 1398695540, 0x1b42fb92},
    {300000, "7c2b3b8cb1629fa014a29e85cd7b0fe7876ce3def8899ba4c0945c0de2ad7902", 1405442852, 0x1b2e13bb},
    {371337, "60323982f9c5ff1b5a954eac9dc1269352835f47c2c5222691d80f0d50dcf053", 1410464577, 0x1b364184},
    {450000, "d279277f8f846a224d776450aa04da3cf978991a182c6f3075db4c48b173bbd7", 1415413000, 0x1b03efda},
    {500000, "92ae1083b7b3c22fd7b4ce2eae121d518f8e1aa81d0be9432ce5aa20a2954fcc", 1418570422, 0x1b04f203},
    {600000, "caa5446c05c8e51cf7985a6cb4da4dc2b730e4ec399dc95b2a094df6bdd4f268", 1424881308, 0x1b03f04d},
    {700000, "eac853ae22d59a498386241a3de69a36739ccc9e0a6acfd617b64c5ea4a0f4b3", 1431191204, 0x1b05d766},
    {771275, "1b7d789ed82cbdc640952e7e7a54966c6488a32eaad54fc39dff83f310dbaaed", 1435666139, 0x1b0444d2},
    {800000, "773fbb34e1bfe82467eb24cda8769dfdcd13a5b4dac4c8f9f6534c40301f7fbf", 1437473213, 0x1b06b43a},
    {900000, "5235e97ba5ae95c04300daed8d75dca2ed6eef9be42d433d985d3203639a4a66", 1443761100, 0x1b028fe5},
    {1000000, "6aae55bea74235f0c80bd066349d4440c31f2d0f27d54265ecd484d8c1d11b47", 1450031952, 0x1b02dcf8},
    {1100000, "d48fef240e48d9395d08c2ec4fe9c6d5554091a107e1c0fddedf5db3a435d96e", 1456288963, 0x1b033eb4},
    {1200000, "1f8ef813b31ec896e3f7f064d6637e251242ba95c1bb12ca339e55b0614a53db", 1462554308, 0x1b036f1b},
    {1250000, "00c7a442055c1a990e11eea5371ca5c1c02a0677b33cc88ec728c45edc4ec060", 1465690401, 0x1b045d00},
    {1300000, "80be4067b5dc8e3db9b787ca4a69c09b3478b6434edc51bc8984db1a79fa620f", 1468822277, 0x1b02fe44},
    {1400000, "18c7078848109e3d95706f639f1501550765e4675a02ddb8bfe3a703bb49fb8f", 1475060460, 0x1b039462},
    {1500000, "f1d32d6920de7b617d51e74bdf4e58adccaa582ffdc8657464454f16a952fca6", 1481313507, 0x1b0306ef},
    {1600000, "ca5eb72f1e0d160f1481f74d56d7cc4a27d91aa585ba012da8018a5fe934d61b", 1487574651, 0x1b01b66a},
    {1700000, "c151a40f121a4f0ee0078e0268563c8299ad12652f939d9c6880aab9a93c1969", 1493860971, 0x1b016434},
    {1750000, "5c8e7327984f0d6f59447d89d143e5f6eafc524c82ad95d176c5cec082ae2001", 1496985750, 0x1b00d4d4},
    {1800000, "c90be6e4d4dfd4f6cfa37e17afe2cb423d82a1af726f402244b4607d155bd021", 1500113228, 0x1b009c6d},
    {1900000, "6ab174805b47f1c93d69fbd37876375cacf57c075b3039561ddb05759f0853c0", 1506361934, 0x1a46b9c8},
    {2000000, "9914f0e82e39bbf21950792e8816620d71b9965bdbbc14e72a95e3ab9618fea8", 1512600918, 0x1a1c1225},
    {2031142, "893297d89afb7599a3c571ca31a3b80e8353f4cf39872400ad0f57d26c4c5d42", 1514549787, 0x1a15d633},
    {2100000, "4f95b8f837f139f512dd8ba26fe4dd702271394daccb29007dc52938c96ccf85", 1518833207, 0x1a08f167},
    {2200000, "7027f0e32f0566f39367580dfb9d42157608fc7ce4483bf2471beb68b3b3f26e", 1525088854, 0x1a07034c},
    {2250000, "0a87a8d4e40dca52763f93812a288741806380cd569537039ee927045c6bc338", 1528215255, 0x1a053477},
    {2300000, "2f391edf30851d6ab71e1100a907d314190a15764da600e478730369db422f93", 1531345450, 0x1a04439c},
    {2400000, "41a17417233ec887dfd5950c731e64abfab5fe230353d226a6a3de580e59c3bf", 1537593624, 0x1a059c04},
    {2500000, "3352c854b77d6946fabed44b4451bc998a48dbbcc2dbc1d313f47b0e8a782d18", 1543848050, 0x1a05d53a},
    {2510150, "77e3f4a4bcb4a2c15e8015525e3d15b466f6c022f6ca82698f329edef7d9777e", 1544484077, 0x1a0868af},
    {2600000, "37bd4bb2b345ca3bc7035adf8850632159d965878f7ab59fbcb4ffd382895a03", 1550097077, 0x1a05f4f3},
    {2700000, "e76b5a769f05c868c831e067946ce32221ad3e163b4bfd96ae2745233587e7cd", 1556337998, 0x1a03ef42},
    {2750000, "d4f8abb835930d3c4f92ca718aaa09bef545076bd872354e0b2b85deefacf2e3", 1559459044, 0x1a0292e2},
    {2800000, "5af3d9109a9c3a75179a9a85f385ca5e045a40dcb4f716ad907beae3d5ee662b", 1562584608, 0x1a0336c7},
    {2900000, "7bc6c24835317d4944a468a27a112eae5d564fc53cf366e34c8c7452f30e09e5", 1568837724, 0x1a045469},
    {3000000, "195a83b091fb3ee7ecb56f2e63d01709293f57f971ccf373d93890c8dc1033db", 1575096781, 0x1a07da10},
    {3100000, "c7970179a8433e85b13a4930f80367a05a50c1f4b8878b31cb4b0ca7506f04c1", 1581365323, 0x1a0a5662},
    {3200000, "1026822a0313b2466ddc13c251dd98072430708cc53c56e43e71dd9ab0625437", 1587656622, 0x1a0786ac},
    {3250000, "7f3e28bf9e309c4b57a4b70aa64d3b2ea5250ae797af84976ddc420d49684034", 1590799169, 0x1a05bfa3},
    {3300000, "76f7f333faf010117d9ca7dd0c6200456dcbffa03b31dee66541966347589b8b", 1593941349, 0x1a050d2a},
    {3400000, "20588e8f8ce89cab1f894ef80faf935c631df24229a1a42bafee781d9a57a454", 1600234275, 0x1a04bf31},
    {3500000, "eaa303b93c1c64d2b3a2cdcf6ccf21b10cc36626965cc2619661e8e1879abdfb", 1606543340, 0x1a08d505},
    {3600000, "3319474fec2b5cbc57df8a8b976fbbb4907d5315b23943daf58bc6ce0bba9347", 1612832557, 0x1a03ecc0},
    {3606083, "954c7c66dee51f0a3fb1edb26200b735f5275fe54d9505c76ebd2bcabac36f1e", 1613218169, 0x1a03d764},
    {3700000, "d0f0af23aadcf6b8d4a681ee930e39d1e64aca967187fa8a0c655c6dacfa22ce", 1619169876, 0x1a043028},
    {3800000, "0e1a1b524408f5d3e23fc461351daf9469976679e37cfc321ab2e06a3cb4f01a", 1625505238, 0x1a03d014},
    {3854173, "e4b4ecda4c022406c502a247c0525480268ce7abbbef632796e8ca1646425e75", 1628934997, 0x1a03ca36},
    {3900000, "3815e66fe50a8c9770c04306626fa6f6be7a087a5e03c0b014ffee470dbb215e", 1631833148, 0x1a032198},
    {3963597, "2b6927cfaa5e82353d45f02be8aadd3bfd165ece5ce24b9bfa4db20432befb5d", 1635884460, 0x1a037bc9},
    {4000000, "47f227cd54c270aa18c5136635fc003a11cb82d6f8319dd8dab180a2f5555a9e", 1638207091, 0x1a0350f2},
    {4100000, "8112b8a11dd3d1965cab6ccc0175abca62ec6b1225ad303e72e94efac635007e", 1644587549, 0x1a02450e},
    {4200000, "93da94c346ff5299ff400be4c1008aae99e9b70ef74db22820e42049ce14c6b4", 1650989758, 0x1a03efe7},
    {4300000, "337d3b4bf937e2f9513e064b58253c9d0f188b7b928d570b07c03651d47e250f", 1657392921, 0x1a0461ea},
    {4303965, "ed7d266dcbd8bb8af80f9ccb8deb3e18f9cc3f6972912680feeb37b090f8cee0", 1657646310, 0x1a0344f5},
    {4400000, "367bdf79d7c527a3f79430d72cf62a8201ba52edd608410a6da1b1ab7311e04d", 1663792227, 0x1a028e13},
    {4500000, "66d27fdeb0f694d06d1276b2d3d72c4dbefe294651ddc1ade220951823243396", 1670200610, 0x1a020cfc},
    {4600000, "75323db271259899e66cbf4cfab33dcc034527e34bdc75b9a70e1b2deec1aad1", 1676603347, 0x1a016fd5},
    {4700000, "748844093ba9fd6ba002ed2d0c9857c0292f1fb4aeb949d2fa8f0d438a18387f", 1682991374, 0x1a014ec5},
    {4800000, "a1e58e27594bbc04f9c021c9f73771852260fce5e09e86071d34c74a8a116d01", 1689360368, 0x1a01b637},
    {4900000, "8bc08384ae8f17e0891159f3cd5783c07a62bd37c3a67d8d5e1bd292addced46", 1695779125, 0x1a02888f},
    {5000000, "5ae6eadfe5fd98fa7f388b671c4863db4b62e46eecc054465a276d419500e469", 1702188235, 0x1a010be6},
    {5050000, "e7d4577405223918491477db725a393bcfc349d8ee63b0a4fde23cbfbfd81dea", 1705383360, 0x1a019541},
    {5100000, "a60110809b403aecd06185ec73753ddfd21dbc243e82c5ef76c88648d1b7b718", 1708571081, 0x1a01114a},
    {5200000, "06381a6241f120951a2f751851f50320e059b084ebc639c6b9d7d8b076d1f0bc", 1714956041, 0x1a01a709},
    {5300000, "8ccc7ebe3d4a97bfdf1993145114f654c537a9b06bc89bb12beabc954fb3110f", 1721347471, 0x1a00f17c},
    {5400000, "cbb1f4ae807da83e13bdf9c28188982938c9ee6bf560c1023f51adac229eef87", 1727704957, 0x0106DAC9},
    {5500000, "a2c15e69513051e6c01cce388718efa7a803f52766e6dc91624812e32250c56e", 1734026997, 0x1a00d1c5},
    {5600000, "39c02091aa7b8ad483a87957b12bdf4c7c6ebcc5dfe2c4648c4d235b40961c8d", 1740360175, 0x1a0093c1},
    {5700000, "28ef0c3aa091c839048bba16bcee4df442961e2bc4e42bda407906f71e5b594d", 1746679266, 0x1a00adee},
    {5800000, "8d1540c92ec87451d73573fec3720ca7e835e630538096e3e11c56dec8205e2e", 1753007429, 0x1a00d815},
    {5900000, "9eb4809b6bf358a5bf9fbddf82da9ff4f047ef76577d2939efabefc7bd6ced07", 1759340677, 0x1a00801a},
    {6000000, "7af46caeb390c15e5d92b4aa58854b55351b43fe5fc714e359b31d0ce019a187", 1765676839, 0x192fa398},
    {6100000, "bdeff3efe99a8203ad2baae74e50c29f5158227ff03d0bb7ffa3ff9fbd25dee3", 1772013218, 0x1a0082ee},
    {6148124, "0f4d009c402553dbec50012ace7e224018595e4b788d1af78cea72eb6401d959", 1775065954, 0x1a00b3ec},
    {6154988, "8ca6972c34a2154b6ae6f15996de8d1a794477ec0add439ee1279bd9a21b2eed", 1775506602, 0x1a008107},
    {6156000, "e8257a8929faef98929dc4e006bf405ec0fdfa93cb29b6d336a04e6b4d7bd02e", 1775571096, 0x1a0088a2},
    {6156600, "2b17dfd7a717852058940928fd87b438971c29573474589abb427480ea7db83f", 1775609059, 0x19654a1c},
    {6169760, "7296ff3bbbc905a0a6d9ca290fb4fcead9b5e9ddbc5be752aa20cd0d789d0316", 1776448183, 0x197c8a95},
    {6173706, "1a59af827274d479127434728f9e1885a45b9a252bab246d039045822e671aa5", 1776698026, 0x197935DD},
    {6189700, "a6ed6d6b03668d0bb9baddfbc52565b2514d02f118d6d0298a6ca48d511d7804", 1777713485, 0x19429592},
    {6191600, "a8eeeadc81e8def69167f0d82c50c4d05fdb0c6ffd17a6af955f80d6925e0f0d", 1777835169, 0x197c3bc0},
    {6201360, "0bb1dbb971969f89f1c4c6305396042de1d6a390d71a44b066255c844da9256c", 1778454835, 0x19555f03},
    {6250000, "197f31497b1e3b506da36eb6126738e3206e42fdff240844f8b75ae4ebc29519", 1781539362, 0x19614D63},
    {6275000, "63c89f44fe3ce35d6abaa297d32d75826ce4368027e098f816432f68c2bedbd6", 1783120876, 0x1979F333},
    {6300000, "e35560d7a6dda44da8fd5d3fea4025319ac5d6287dc3c1308cde650e51e12e58", 1784704739, 0x19671029}};

/* Counts defined next to the arrays. sizeof() on the extern declarations
   yields whatever bound the header states, not the real length: chainparams.h
   said [87] and libdogecoin.h said [33] while this file defined 89, so callers
   silently saw a truncated array. */
const size_t dogecoin_mainnet_checkpoint_count =
    sizeof(dogecoin_mainnet_checkpoint_array) / sizeof(dogecoin_mainnet_checkpoint_array[0]);

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
    {3976284, "af23c3e750bb4f2ce091235f006e7e4e2af453d4c866282e7870471dcfeb4382", 1657646588, 0x1e0fffff},
    {5900000, "199bea6a442310589cbb50a193a30b097c228bd5a0f21af21e4e53dd57c382d3", 1703511130, 0x1e0fffff},
    {41595117, "ff15d3837501029b27f34419ceb76c283be15159b6aecca38e11cd9f55f2ec85", 1773263888, 0x1e03ee2d},
    {41598400, "5fab0f828e47da930d5a010ab6d6204b185362e062f70b24b16f696135fdc9e5", 1773338705, 0x1d01f16d},
    {44518499, "11d7c46c746b7bd3b52e519cfa736dee5c301046ee44a25f80c34f32cc2950ae", 1774975684, 0x1e0fffff},
    {44531497, "274f5820c16f7fd40719c3018648a51efc97a608c0cc069a108c0fd1ef8da091", 1774978992, 0x1e0fffff},
    {46533515, "c5e11996abc86dfaabffbe3c653301ff106bf689e8f57f6d20377c0fccd86a3a", 1775506692, 0x1e0806cb}};

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
        case 'n': {
            /* Ambiguous by construction. Regtest's 0x6f encodes to 'm' or 'n'
               depending on the hash160, and testnet's 0x71 is only ever 'n', so
               an 'n' address is either one and the character cannot say which.
               Read the version byte, which can. */
            uint8_t decoded[64];
            if (dogecoin_base58_decode_check(address, decoded, sizeof(decoded)) == 25 &&
                decoded[0] == dogecoin_chainparams_regtest.b58prefix_pubkey_address) {
                return &dogecoin_chainparams_regtest;
            }
            return &dogecoin_chainparams_test;
        }
        case 'm':
            /* 0x71 never reaches 'm', so this one is unambiguous */
            return &dogecoin_chainparams_regtest;
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

const size_t dogecoin_testnet_checkpoint_count =
    sizeof(dogecoin_testnet_checkpoint_array) / sizeof(dogecoin_testnet_checkpoint_array[0]);
