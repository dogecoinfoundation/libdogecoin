import unittest
from ctypes import *

import wrappers as w

DOGE_LIB = w.get_lib("libdogecoin.so")


class PrivateKeyGenTestCase(unittest.TestCase):
    def test_privkeyisvalid_main(self):
        # wrapper answer (privkey wif, privkey hex)
        res = w.py_gen_privkey(DOGE_LIB, 0)
        
        #convert hex to binary
        privkey_hex = c_char_p(res[1].encode('utf-8'))
        privkey_bin = (c_ubyte * 1000)() #temporary fix to prevent segfaults
        inlen = c_int32(len(res[1]))
        outlen = c_int32()
        DOGE_LIB.utils_hex_to_bin(privkey_hex, privkey_bin, inlen, byref(outlen))

        # test privkey wif is correctly encoded
        privkey_wif = c_char_p(bytes(res[0], 'utf-8'))
        datalen = c_ulong(outlen.value)
        decoded_key_bin = (c_ubyte * 100)()
        DOGE_LIB.dogecoin_base58_decode_check(privkey_wif, byref(decoded_key_bin), datalen)
        
        self.assertEqual(bytes(decoded_key_bin)[1:33], bytes(privkey_bin)[:32], 'utf-8')
  

    def test_privkeyisvalid_testnet(self):
                # wrapper answer (privkey wif, privkey hex)
        res = w.py_gen_privkey(DOGE_LIB, 1)
        
        #convert hex to binary
        privkey_hex = c_char_p(res[1].encode('utf-8'))
        privkey_bin = (c_ubyte * 1000)() #temporary fix to prevent segfaults
        inlen = c_int32(len(res[1]))
        outlen = c_int32()
        DOGE_LIB.utils_hex_to_bin(privkey_hex, privkey_bin, inlen, byref(outlen))

        # test privkey wif is correctly encoded
        privkey_wif = c_char_p(bytes(res[0], 'utf-8'))
        datalen = c_ulong(outlen.value)
        decoded_key_bin = (c_ubyte * 100)()
        DOGE_LIB.dogecoin_base58_decode_check(privkey_wif, byref(decoded_key_bin), datalen)
        
        self.assertEqual(bytes(decoded_key_bin)[1:33], bytes(privkey_bin)[:32], 'utf-8')


    def test_privkeyisvalid_regtest(self):
                # wrapper answer (privkey wif, privkey hex)
        res = w.py_gen_privkey(DOGE_LIB, 2)
        
        #convert hex to binary
        privkey_hex = c_char_p(res[1].encode('utf-8'))
        privkey_bin = (c_ubyte * 1000)() #temporary fix to prevent segfaults
        inlen = c_int32(len(res[1]))
        outlen = c_int32()
        DOGE_LIB.utils_hex_to_bin(privkey_hex, privkey_bin, inlen, byref(outlen))

        # test privkey wif is correctly encoded
        privkey_wif = c_char_p(bytes(res[0], 'utf-8'))
        datalen = c_ulong(outlen.value)
        decoded_key_bin = (c_ubyte * 100)()
        DOGE_LIB.dogecoin_base58_decode_check(privkey_wif, byref(decoded_key_bin), datalen)
        
        self.assertEqual(bytes(decoded_key_bin)[1:33], bytes(privkey_bin)[:32], 'utf-8')




class PublicKeyGenTestCase(unittest.TestCase):
    def test_pubkeyisvalid_main(self):
        res = w.py_gen_privkey(DOGE_LIB, 0)
        privkey_bytes = bytes.fromhex(res[1][:64])
        privkey = w.Dogecoin_key()
        privkey.privkey = privkey_bytes


    def test_pubkeyisvalid_testnet(self):
        pass

    def test_pubkeyisvalid_regtest(self):
        pass