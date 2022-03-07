<<<<<<< HEAD
=======

>>>>>>> 68cee85... added address and keypair verification along with wrappers for them
"""Testing module for wrappers from address.c"""

import unittest
import ctypes as ct
import sys
sys.path.append("../")
import wrappers as w
lib = w.load_libdogecoin()


class TestGeneratePrivPubKeyPair(unittest.TestCase):
    """Test class for function generate_priv_pub_key_pair()"""

    def test_privkey_gen_mainnet(self):
        """Test that function returns private key for mainnet"""
        res = w.generate_priv_pub_key_pair()
        self.assertIsNotNone(res[0])

    def test_privkey_gen_testnet(self):
        """Test function returns private key for testnet"""
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[0])

    def test_privkey_is_valid_mainnet(self):
        """Test function returns valid private key"""
        res = w.generate_priv_pub_key_pair(as_bytes=True)
        privkey = (ct.c_ubyte * 32)()
        ct.memmove(privkey, res[0], 32)
        dogecoin_key = w.DogecoinKey(privkey)
        lib.dogecoin_ecc_start()
        self.assertTrue(lib.dogecoin_privkey_is_valid(ct.byref(dogecoin_key)))
        lib.dogecoin_ecc_stop()

    def test_privkey_is_valid_testnet(self):
        """Test function returns valid private key"""
        res = w.generate_priv_pub_key_pair(chain_code=1, as_bytes=True)
        privkey = (ct.c_ubyte * 32)()
        ct.memmove(privkey, res[0], 32)
        dogecoin_key = w.DogecoinKey(privkey)
        lib.dogecoin_ecc_start()
        self.assertTrue(lib.dogecoin_privkey_is_valid(ct.byref(dogecoin_key)))
        lib.dogecoin_ecc_stop()

    def test_pubkey_gen_mainnet(self):
        """Test function returns public key for mainnet"""
        res = w.generate_priv_pub_key_pair()
        self.assertIsNotNone(res[1])

    def test_pubkey_gen_testnet(self):
        """Test function returns public key for testnet"""
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[1])

    def test_p2pkh_addr_format_is_valid_mainnet(self):
        """Test function returns valid address for mainnet"""
<<<<<<< HEAD
        # TODO: make rule changes later, for now simple verify
        res = w.generate_priv_pub_key_pair()
        self.assertTrue(res[1][0] == "D")
        self.assertTrue(len(res[1])==34)

    def test_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid address for testnet"""
        # TODO: make rule changes later, for now simple verify
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(res[1][0] == "n")
        self.assertTrue(len(res[1])==34)

    def test_pubkey_is_valid(self):
        """Test that the pubkey is valid and does hash to returned address"""
=======
        res = w.generate_priv_pub_key_pair()
        self.assertTrue(w.verify_p2pkh_address(res[1], 0))

    def test_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid address for testnet"""
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(w.verify_p2pkh_address(res[1], 1))

    def test_keypair_is_valid_mainnet(self):
        """Test that the private and public key for mainnet
        are valid and associated to each other"""
        res = w.generate_priv_pub_key_pair()
        self.assertTrue(w.verify_priv_pub_keypair(res[0], res[1]))

    def test_keypair_is_valid_testnet(self):
        """Test that the private and public key for testnet
        are valid and associated to each other"""
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(w.verify_priv_pub_keypair(res[0], res[1], chain_code=1))
>>>>>>> 68cee85... added address and keypair verification along with wrappers for them


class TestGenerateHDMasterPrivPubKeyPair(unittest.TestCase):
    """Test class for function generate_hd_master_pub_key_pair"""

    def test_master_privkey_gen_mainnet(self):
        """Test function returns master private key for mainnet"""
        res = w.generate_hd_master_pub_key_pair()
        self.assertIsNotNone(res[0])

    def test_master_privkey_gen_testnet(self):
        """Test function returns amster private key for testnet"""
        res = w.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[0])

    def test_privkey_is_valid_mainnet(self):
        """Test function returns valid master private key for mainnet"""
        res = w.generate_hd_master_pub_key_pair(as_bytes=True)
        privkey = (ct.c_ubyte * 32)()
        ct.memmove(privkey, res[0], 32)
        dogecoin_key = w.DogecoinKey(privkey)
        lib.dogecoin_ecc_start()
        self.assertTrue(lib.dogecoin_privkey_is_valid(ct.byref(dogecoin_key)))
        lib.dogecoin_ecc_stop()

    def test_master_pubkey_gen_mainnet(self):
        """Test function returns master public key for mainnet"""
        res = w.generate_hd_master_pub_key_pair()
        self.assertIsNotNone(res[1])

    def test_master_pubkey_gen_testnet(self):
        """Test function returns master public key for testnet"""
        res = w.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[1])


if __name__ == "__main__":
    unittest.main()
