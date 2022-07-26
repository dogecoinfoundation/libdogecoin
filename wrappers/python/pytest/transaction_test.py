"""Testing module for wrappers from transaction.c"""

import unittest
import libdogecoin as l

# internal keys (set 1)
privkey_wif =       "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy"
p2pkh_addr =        "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy"
utxo_scriptpubkey = "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac"

# internal keys (set 2)
privkey_wif2 =          "cf6dS36Erx417gQgYiWXyXwQL5u4yxkEhUVuD65u3EKSbT3Bfaor"
p2pkh_addr2 =           "ncMfpXNvKzeNBoJwuAdSjz949oWLyieWee"
utxo_scriptpubkey2 =    "76a914598d6c0f67763fa68235edcd60ab939fc242875c88ac"

# external keys
external_p2pkh_addr = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde"

# expected hashes step by step for integer test amount (using set 1)
expected_empty_tx_hex =                                 "01000000000000000000"
expected_unsigned_single_utxo_tx_hex =                  "0100000001746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffff0000000000"
expected_unsigned_double_utxo_tx_hex =                  "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff0000000000"
expected_unsigned_double_utxo_single_output_tx_hex =    "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff010065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000"
expected_unsigned_tx_hex =                              "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
expected_signed_single_input_tx_hex =                   "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
expected_signed_raw_tx_hex =                            "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006a47304402200e19c2a66846109aaae4d29376040fc4f7af1a519156fe8da543dc6f03bb50a102203a27495aba9eead2f154e44c25b52ccbbedef084f0caf1deedaca87efd77e4e70121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"

# expected hashes step by step for decimal test amount (using set 2)
expected_unsigned_single_utxo_tx_hex2 =                 "01000000018746fb1cc227513513063245373c2d16533ca638b6911e34b01b7970681f099b0100000000ffffffff0000000000"
expected_unsigned_single_utxo_single_output_tx_hex2 =   "01000000018746fb1cc227513513063245373c2d16533ca638b6911e34b01b7970681f099b0100000000ffffffff01cc74e0c7020000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000"
expected_signed_raw_tx_hex2 =                           "01000000018746fb1cc227513513063245373c2d16533ca638b6911e34b01b7970681f099b010000006b483045022100f78f4b911b74c8769d3c6824d048c7d6813265d8599dfcc19bc12e17fcf0207b02206ae0a48e4319767cce4579f4852e179ba9cd6061f1c337eac782facdbff42a49012102eab8cb0125caff77443d47d1d6bbc8f753fe17b86c2ea3332622b3db82afe6f4ffffffff01cc74e0c7020000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000"

# existing transactions
hash_2_doge =       "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074"  # 2 DOGE
hash_10_doge =      "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2"  # 10 DOGE
hash_decimal_doge = "9b091f6870791bb0341e91b638a63c53162d3c3745320613355127c21cfb4687"  # 119.43536540 DOGE
vout_2_doge =       1 # vout is the spendable output index from the existing transaction
vout_10_doge =      1
vout_decimal_doge = 1

# transaction amounts
input1_amt =        "2"
input2_amt =        "10"
send_amt =          "5"
total_utxo_input =  "12"

decimal_input_amt =         "119.43536540"
decimal_send_amt =          "119.43310540"# fee of 0.00226 deducted
decimal_total_utxo_input =  "119.43536540" 

fee = "0.00226"

# invalid parameters
bad_privkey_wif =       "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fx"
long_tx_hex =           "x"*((1024*100)+1)
high_send_amt =         "15" # max we can spend from the two inputs is 12

class TestTransactionFunctions(unittest.TestCase):

    def setUpClass():
        l.w_context_start()

    def tearDownClass():
        l.w_context_stop()

    def suite():
        suite = unittest.TestSuite()
        suite.addTest(TestTransactionFunctions("test_start_transaction"))
        suite.addTest(TestTransactionFunctions("test_start_transaction_value"))
        suite.addTest(TestTransactionFunctions("test_store_raw_transaction"))
        suite.addTest(TestTransactionFunctions("test_store_raw_transaction_value"))
        suite.addTest(TestTransactionFunctions("test_store_long_raw_transaction"))
        suite.addTest(TestTransactionFunctions("test_get_raw_transaction"))
        suite.addTest(TestTransactionFunctions("test_get_raw_transaction_bad_index"))
        suite.addTest(TestTransactionFunctions("test_add_utxo"))
        suite.addTest(TestTransactionFunctions("test_add_single_utxo_value"))
        suite.addTest(TestTransactionFunctions("test_add_double_utxo_value"))
        suite.addTest(TestTransactionFunctions("test_add_utxo_bad_index"))
        suite.addTest(TestTransactionFunctions("test_add_output"))
        suite.addTest(TestTransactionFunctions("test_add_output_value"))
        suite.addTest(TestTransactionFunctions("test_add_output_bad_index"))
        suite.addTest(TestTransactionFunctions("test_finalize_transaction"))
        suite.addTest(TestTransactionFunctions("test_clear_transaction"))
        suite.addTest(TestTransactionFunctions("test_sign_raw_transaction"))
        suite.addTest(TestTransactionFunctions("test_sign_raw_transaction_bad_privkey"))
        suite.addTest(TestTransactionFunctions("test_sign_raw_transaction_high_send_amt"))
        suite.addTest(TestTransactionFunctions("test_sign_transaction"))
        suite.addTest(TestTransactionFunctions("test_sign_transaction_value"))
        suite.addTest(TestTransactionFunctions("test_full_int_amt_transaction_build"))
        suite.addTest(TestTransactionFunctions("test_full_decimal_amt_transaction_build"))
        return suite

    def test_start_transaction(self):
        """Test that function successfully creates a new
        working transaction at the next available slot 
        in the hash table."""
        res = l.w_start_transaction()
        self.assertTrue(type(res)==int and res>=0)
        res2 = l.w_start_transaction()
        self.assertTrue(type(res2)==int and res2>=0)
        self.assertTrue(res2==res+1)
        l.w_clear_transaction(res)
        l.w_clear_transaction(res2)

    def test_start_transaction_value(self):
        """Test that start transaction yields the expected
        hex for an empty working transaction."""
        idx = l.w_start_transaction()
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_empty_tx_hex)
        l.w_clear_transaction(idx)

    def test_store_raw_transaction(self):
        """Test that store transaction successfully saves
        a transaction hex string to the next available
        working transaction in memory."""
        idx = l.w_start_transaction()
        idx2 = l.w_store_raw_transaction(expected_unsigned_single_utxo_tx_hex)
        self.assertTrue(idx2==idx+1)

    def test_store_raw_transaction_value(self):
        """Test that store transaction successfully saves
        a transaction hex string to the next available
        working transaction in memory."""
        idx = l.w_store_raw_transaction(expected_unsigned_single_utxo_tx_hex)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_unsigned_single_utxo_tx_hex)

    def test_store_long_raw_transaction(self):
        """Test that inputting a transaction hex of more
        than 100 kilobytes returns zero and does not save."""
        res = l.w_store_raw_transaction(long_tx_hex)
        self.assertFalse(res)

    def test_get_raw_transaction(self):
        """Test that function correctly returns the current
        state of the specified working transaction at various
        steps."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash_2_doge, vout_2_doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_single_utxo_tx_hex)
        l.w_add_utxo(idx, hash_10_doge, vout_10_doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_single_output_tx_hex)
        l.w_clear_transaction(idx)
    
    def test_get_raw_transaction_bad_index(self):
        """Test that function returns zero when given an
        index that does not exist in the hash table."""
        idx = l.w_start_transaction()
        res = l.w_get_raw_transaction(idx+1) # out of bounds
        self.assertFalse(res)
        l.w_clear_transaction(idx)

    def test_add_utxo(self):
        """Test that function successfully adds a new
        input to a working transaction."""
        idx = l.w_start_transaction()
        res = l.w_add_utxo(idx, hash_2_doge, vout_2_doge)
        self.assertTrue(res)
        l.w_clear_transaction(idx)

    def test_add_single_utxo_value(self):
        """Test that the updated transaction matches the
        expected hex after one input has been added."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash_2_doge, vout_2_doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_single_utxo_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_double_utxo_value(self):
        """Test that the updated transaction matches the
        expected hex after two inputs have been added."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash_2_doge, vout_2_doge)
        l.w_add_utxo(idx, hash_10_doge, vout_10_doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_utxo_bad_index(self):
        """Test that function returns zero when given an
        index that does not exist in the hash table."""
        idx = l.w_start_transaction()
        res = l.w_add_utxo(idx+1, hash_2_doge, vout_2_doge) # out of bounds
        self.assertFalse(res)

    def test_add_output(self):
        """Test that function successfully updates a
        working transaction to include a new output."""
        idx = l.w_store_raw_transaction(expected_unsigned_double_utxo_tx_hex)
        res = l.w_add_output(idx, external_p2pkh_addr, send_amt)
        self.assertTrue(res)
        l.w_clear_transaction(idx)
        
    def test_add_output_value(self):
        """Test that the updated transaction matches the
        expected hex after two inputs and one output have
        been added."""
        idx = l.w_store_raw_transaction(expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_single_output_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_output_bad_index(self):
        """Test that function returns zero when given an
        index that does not exist in the hash table."""
        idx = l.w_store_raw_transaction(expected_unsigned_double_utxo_tx_hex)
        res = l.w_add_output(idx+1, external_p2pkh_addr, send_amt) # out of bounds
        self.assertFalse(res)

    def test_finalize_transaction(self):
        """Test that the updated transaction matches the
        expected hex after the inputs have been closed."""
        idx = l.w_store_raw_transaction(expected_unsigned_double_utxo_single_output_tx_hex)
        rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        self.assertTrue(rawhex==expected_unsigned_tx_hex)
        l.w_clear_transaction(idx)

    def test_clear_transaction(self):
        """Test that function successfully erases a 
        transaction from the hash table."""
        idx = l.w_start_transaction()
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_empty_tx_hex)
        l.w_clear_transaction(idx)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertFalse(rawhex)

    def test_sign_raw_transaction(self):
        """Test that the updated transaction matches the
        expected hex after each input has been signed."""
        rawhex = l.w_sign_raw_transaction(0, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, privkey_wif)
        self.assertTrue(rawhex==expected_signed_single_input_tx_hex)
        rawhex = l.w_sign_raw_transaction(1, expected_signed_single_input_tx_hex, utxo_scriptpubkey, 1, privkey_wif)
        self.assertTrue(rawhex==expected_signed_raw_tx_hex)

    def test_sign_raw_transaction_bad_privkey(self):
        """Test that a transaction cannot be signed by
        an incorrect private key."""
        self.assertFalse(l.w_sign_raw_transaction(0, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, bad_privkey_wif))
        self.assertFalse(l.w_sign_raw_transaction(1, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, bad_privkey_wif))

    def test_sign_raw_transaction_high_send_amt(self):
        """Test that a transaction cannot be signed if
        it contains illegal input."""
        idx = l.w_store_raw_transaction(expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, high_send_amt) # spending more than received
        finalized_rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        half_signed_rawhex = l.w_sign_raw_transaction(0, finalized_rawhex, utxo_scriptpubkey, 1, privkey_wif)
        full_signed_rawhex = l.w_sign_raw_transaction(1, half_signed_rawhex, utxo_scriptpubkey, 1, privkey_wif)
        print("\nTransaction looks valid but will get rejected:", full_signed_rawhex)
        l.w_clear_transaction(idx)

    def test_sign_transaction(self):
        """Test that the updated transaction matches the
        expected hex after each input has been signed."""
        idx = l.w_store_raw_transaction(expected_unsigned_tx_hex)
        res = l.w_sign_transaction(idx, utxo_scriptpubkey, privkey_wif)
        self.assertTrue(res)
        l.w_clear_transaction(idx)
        
    def test_sign_transaction_value(self):
        """Test that the updated transaction matches the
        expected hex after each input has been signed."""
        idx = l.w_store_raw_transaction(expected_unsigned_tx_hex)
        l.w_sign_transaction(idx, utxo_scriptpubkey, privkey_wif)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_signed_raw_tx_hex)
        l.w_clear_transaction(idx)

    def test_full_int_amt_transaction_build(self):
        """Test that a transaction built from scratch
        using integer values for inputs and output amounts
        matches the expected hash when everything has
        been finalized and signed."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash_2_doge, vout_2_doge)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_unsigned_single_utxo_tx_hex)
        l.w_add_utxo(idx, hash_10_doge, vout_10_doge)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_unsigned_double_utxo_single_output_tx_hex)
        finalized_rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        self.assertTrue(finalized_rawhex==expected_unsigned_tx_hex)
        l.w_sign_transaction(idx, utxo_scriptpubkey, privkey_wif)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_signed_raw_tx_hex)
        l.w_clear_transaction(idx)

    def test_full_decimal_amt_transaction_build(self):
        """Test that a transaction built from scratch
        using decimal values for inputs and output amounts
        matches the expected hash when everything has
        been finalized and signed."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash_decimal_doge, vout_decimal_doge)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_unsigned_single_utxo_tx_hex2)
        l.w_add_output(idx, external_p2pkh_addr, decimal_send_amt)
        finalized_rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(finalized_rawhex==expected_unsigned_single_utxo_single_output_tx_hex2)
        l.w_sign_transaction(idx, utxo_scriptpubkey2, privkey_wif2)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_signed_raw_tx_hex2)
        l.w_clear_transaction(idx)

        
if __name__ == "__main__":
    r = unittest.TextTestRunner()
    r.run(TestTransactionFunctions.suite())