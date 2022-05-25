"""Testing module for wrappers from address.c"""

import inspect
import unittest
import libdogecoin as l


class TestAddressFunctions(unittest.TestCase):

    def setUpClass():
        l.context_start()

    def tearDownClass():
        l.context_stop()

    def test_privkey_gen_mainnet(self):
        """Test that function returns private key for mainnet"""   
        res = l.generate_priv_pub_key_pair()
        self.assertIsNotNone(res[0])

    def test_privkey_gen_testnet(self):
        """Test function returns private key for testnet"""
        res = l.generate_priv_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[0])

    def test_pubkey_gen_mainnet(self):
        """Test function returns public key for mainnet"""
        res = l.generate_priv_pub_key_pair()
        self.assertIsNotNone(res[1])

    def test_pubkey_gen_testnet(self):
        """Test function returns public key for testnet"""
        res = l.generate_priv_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[1])

    def test_p2pkh_addr_format_is_valid_mainnet(self):
        """Test function returns valid address for mainnet"""
        res = l.generate_priv_pub_key_pair()
        self.assertTrue(l.verify_p2pkh_address(res[1]))

    def test_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid address for testnet"""
        res = l.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(l.verify_p2pkh_address(res[1]))

    def test_keypair_is_valid_mainnet(self):
        """Test that the private and public key for mainnet
        are valid and associated to each other"""
        res = l.generate_priv_pub_key_pair()
        self.assertTrue(l.verify_priv_pub_keypair(res[0], res[1]))

    def test_keypair_is_valid_testnet(self):
        """Test that the private and public key for testnet
        are valid and associated to each other"""
        res = l.generate_priv_pub_key_pair(chain_code=1)
        self.assertTrue(l.verify_priv_pub_keypair(res[0], res[1], chain_code=1))

    def test_master_privkey_gen_mainnet(self):
        """Test function returns master private key for mainnet"""
        res = l.generate_hd_master_pub_key_pair()
        self.assertIsNotNone(res[0])

    def test_master_privkey_gen_testnet(self):
        """Test function returns master private key for testnet"""
        res = l.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[0])

    def test_master_pubkey_gen_mainnet(self):
        """Test function returns master public key for mainnet"""
        res = l.generate_hd_master_pub_key_pair()
        self.assertIsNotNone(res[1])

    def test_master_pubkey_gen_testnet(self):
        """Test function returns master public key for testnet"""
        res = l.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertIsNotNone(res[1])

    def test_master_p2pkh_addr_format_is_valid_mainnet(self):
        """Test function returns valid address for mainnet"""
        res = l.generate_hd_master_pub_key_pair()
        self.assertTrue(l.verify_p2pkh_address(res[1]))

    def test_master_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid address for testnet"""
        res = l.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertTrue(l.verify_p2pkh_address(res[1]))
        
    def test_master_keypair_is_valid_mainnet(self):
        """Test function verifies a valid hd keypair for mainnet"""
        res = l.generate_hd_master_pub_key_pair()
        self.assertTrue(l.verify_master_priv_pub_keypair(res[0], res[1], chain_code=0))

    def test_master_keypair_is_valid_testnet(self):
        """Test function verifies a valid hd keypair for testnet"""
        res = l.generate_hd_master_pub_key_pair(chain_code=1)
        self.assertTrue(l.verify_master_priv_pub_keypair(res[0], res[1], chain_code=1))

    def test_derived_hd_pubkey_gen_mainnet(self):
        """Test function returns derived key on mainnet"""
        keypair = l.generate_hd_master_pub_key_pair()
        res = l.generate_derived_hd_pub_key(keypair[0])
        self.assertIsNotNone(res)

    def test_derived_hd_pubkey_gen_testnet(self):
        """Test function returns derived key on mainnet"""
        keypair = l.generate_hd_master_pub_key_pair(chain_code=1)
        res = l.generate_derived_hd_pub_key(keypair[0])
        self.assertIsNotNone(res)

    def test_derived_keypair_is_valid_mainnet(self):
        """Test function verifies a valid derived hd keypair for mainnet"""
        keypair = l.generate_hd_master_pub_key_pair(chain_code=0)
        derived_pubkey = l.generate_derived_hd_pub_key(keypair[0])
        self.assertTrue(l.verify_master_priv_pub_keypair(keypair[0], derived_pubkey, chain_code=0))

    def test_derived_keypair_is_valid_testnet(self):
        """Test function verifies a valid derived hd keypair for testnet"""
        keypair = l.generate_hd_master_pub_key_pair(chain_code=1)
        derived_pubkey = l.generate_derived_hd_pub_key(keypair[0])
        self.assertTrue(l.verify_master_priv_pub_keypair(keypair[0], derived_pubkey, chain_code=1))

    def test_derived_p2pkh_addr_format_is_valid_mainnet(self):
        """Test function returns valid derived pubkey for mainnet"""
        keypair = l.generate_hd_master_pub_key_pair(chain_code=0)
        derived_pubkey = l.generate_derived_hd_pub_key(keypair[0])
        self.assertTrue(l.verify_p2pkh_address(derived_pubkey))

    def test_derived_p2pkh_addr_format_is_valid_testnet(self):
        """Test function returns valid derived pubkey for testnet"""
        keypair = l.generate_hd_master_pub_key_pair(chain_code=1)
        derived_pubkey = l.generate_derived_hd_pub_key(keypair[0])
        self.assertTrue(l.verify_p2pkh_address(derived_pubkey))


if __name__ == "__main__":
    test_src = inspect.getsource(TestAddressFunctions)
    unittest.TestLoader.sortTestMethodsUsing = lambda _, x, y: (
        test_src.index(f"def {x}") - test_src.index(f"def {y}")
    )
    unittest.main()
