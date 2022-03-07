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
        res = w.generate_priv_pub_key_pair()
        self.assertTrue(w.verify_p2pkh_address(res[1], 0))

    def test_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid address for testnet"""
        res = w.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(w.verify_p2pkh_address(res[1], 1))

    def test_pubkey_is_valid(self):
        """Test that the pubkey is valid and does hash to returned address"""
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
        # TODO: memmove operation only takes the first 32 bytes and cuts the rest
        # should the is_valid check even return true? seems wrong
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

    def test_master_pubkey_gen_mainnet(self):
        """Test function returns master public key for mainnet"""
        res = w.generate_hd_master_pub_key_pair()
        self.assertIsNotNone(res[1])

    def test_master_pubkey_gen_testnet(self):
        """Test function returns master public key for testnet"""
        res = w.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[1])

    def test_master_keypair_is_valid_mainnet(self):
        """Test function verifies a valid hd keypair for mainnet"""
        res = w.generate_hd_master_pub_key_pair()
        self.assertTrue(w.verify_master_priv_pub_keypair(res[0], res[1], 0))

    # TODO: need support for key derivation on testnet
    # def test_master_keypair_is_valid_testnet(self):
    #     """Test function verifies a valid hd keypair for testnet"""
    #     res = w.generate_hd_master_pub_key_pair()
    #     self.assertTrue(w.verify_master_priv_pub_keypair(res[0], res[1], 1))

    def test_p2pkh_addr_format_is_valid_mainnet(self):
        """Test function returns valid address for mainnet"""
        res = w.generate_hd_master_pub_key_pair()
        self.assertTrue(w.verify_p2pkh_address(res[1], 0))

    # TODO: need support for key derivation on testnet
    # def test_p2pkh_addr_format_is_valid_testnet(self):
    #     """Test function returns valid address for testnet"""
    #     res = w.generate_hd_master_pub_key_pair(chain_code=1)
    #     self.assertTrue(w.verify_p2pkh_address(res[1], 1))



if __name__ == "__main__":
    unittest.main()
