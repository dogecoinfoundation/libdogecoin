"""Testing module for wrappers from transaction.c"""

import inspect
import time
import unittest
import libdogecoin as l

# internal keys
privkey_wif =       "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy"
pubkey_hex =        "031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075b"
p2pkh_addr =        "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy"
utxo_scriptpubkey =  "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac"

# external keys
external_p2pkh_addr = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde"

# expected hashes step by step
expected_empty_tx_hex =                                 "01000000000000000000"
expected_unsigned_single_utxo_tx_hex =                  "0100000001746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffff0000000000"
expected_unsigned_double_utxo_tx_hex =                  "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff0000000000"
expected_unsigned_double_utxo_single_output_tx_hex =    "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff010065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000"
expected_unsigned_tx_hex =                              "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
expected_signed_single_input_tx_hex =                   "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
expected_signed_raw_tx_hex =                            "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006a47304402200e19c2a66846109aaae4d29376040fc4f7af1a519156fe8da543dc6f03bb50a102203a27495aba9eead2f154e44c25b52ccbbedef084f0caf1deedaca87efd77e4e70121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"

# existing transactions
hash2doge =  "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074" # 2 DOGE
hash10doge = "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2" # 10 DOGE
vout2doge =  1 # vout is the spendable output index from the existing transaction
vout10doge = 1

# new transaction parameters
send_amt = 5
total_utxo_input = 12
fee = .00226

# invalid parameters
bad_privkey_wif =       "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fx"
bad_hash2doge =         "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae07607x"
bad_tx_hex =            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
long_tx_hex =           "x"*((1024*100)+1)
high_send_amt =         15 # max we can spend from the two inputs is 12

class TestTransactionFunctions(unittest.TestCase):

    def setUpClass():
        l.context_start()

    def tearDownClass():
        l.context_stop()

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

    def test_save_raw_transaction(self):
        """Test that function successfully saves a
        transaction hex string to working transaction
        object."""
        idx = l.w_start_transaction()
        res = l.w_save_raw_transaction(idx, expected_unsigned_single_utxo_tx_hex)
        self.assertTrue(res)
    
    def test_save_raw_transaction_value(self):
        """Test that the saved copy of the transaction is
        the same as the original."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        rawhex = l.w_get_raw_transaction(idx)
        idx2 = l.w_start_transaction()
        l.w_save_raw_transaction(idx2, rawhex)
        rawhex2 = l.w_get_raw_transaction(idx2)
        self.assertTrue(rawhex==rawhex2)
        l.w_clear_transaction(idx)
        l.w_clear_transaction(idx2)

    def test_save_long_raw_transaction(self):
        """Test that inputting a transaction hex of more
        than 100 kilobytes returns zero and does not save."""
        idx = l.w_start_transaction()
        res = l.w_save_raw_transaction(idx, long_tx_hex)
        self.assertFalse(res)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_empty_tx_hex)

    def test_get_raw_transaction(self):
        """Test that function correctly returns the current
        state of the specified working transaction at various
        steps."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_single_utxo_tx_hex)
        l.w_add_utxo(idx, hash10doge, vout10doge)
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
        res = l.w_add_utxo(idx, hash2doge, vout2doge)
        self.assertTrue(res)
        l.w_clear_transaction(idx)

    def test_add_single_utxo_value(self):
        """Test that the updated transaction matches the
        expected hex after one input has been added."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_single_utxo_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_double_utxo_value(self):
        """Test that the updated transaction matches the
        expected hex after two inputs have been added."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        l.w_add_utxo(idx, hash10doge, vout10doge)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_utxo_bad_index(self):
        """Test that function returns zero when given an
        index that does not exist in the hash table."""
        idx = l.w_start_transaction()
        res = l.w_add_utxo(idx+1, hash2doge, vout2doge) # out of bounds
        self.assertFalse(res)

    def test_add_output(self):
        """Test that function successfully updates a
        working transaction to include a new output."""
        idx = l.w_start_transaction()
        l.w_save_raw_transaction(idx, expected_unsigned_double_utxo_tx_hex)
        res = l.w_add_output(idx, external_p2pkh_addr, send_amt)
        self.assertTrue(res)
        l.w_clear_transaction(idx)
        
    def test_add_output_value(self):
        """Test that the updated transaction matches the
        expected hex after two inputs and one output have
        been added."""
        idx = l.w_start_transaction()
        l.w_save_raw_transaction(idx, expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_single_output_tx_hex)
        l.w_clear_transaction(idx)

    def test_add_output_bad_index(self):
        """Test that function returns zero when given an
        index that does not exist in the hash table."""
        idx = l.w_start_transaction()
        l.w_save_raw_transaction(idx, expected_unsigned_double_utxo_tx_hex)
        res = l.w_add_output(idx+1, external_p2pkh_addr, send_amt) # out of bounds
        self.assertFalse(res)

    def test_finalize_transaction(self):
        """Test that the updated transaction matches the
        expected hex after the inputs have been closed."""
        idx = l.w_start_transaction()
        l.w_save_raw_transaction(idx, expected_unsigned_double_utxo_single_output_tx_hex)
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
        rawhex = l.w_sign_raw_transaction(0, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, 2, privkey_wif)
        self.assertTrue(rawhex==expected_signed_single_input_tx_hex)
        rawhex = l.w_sign_raw_transaction(1, expected_signed_single_input_tx_hex, utxo_scriptpubkey, 1, 10, privkey_wif)
        self.assertTrue(rawhex==expected_signed_raw_tx_hex)

    def test_sign_raw_transaction_bad_privkey(self):
        """Test that a transaction cannot be signed by
        an incorrect private key."""
        self.assertFalse(l.w_sign_raw_transaction(0, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, 2, bad_privkey_wif))
        self.assertFalse(l.w_sign_raw_transaction(1, expected_unsigned_tx_hex, utxo_scriptpubkey, 1, 10, bad_privkey_wif))

    def test_sign_raw_transaction_high_send_amt(self):
        """Test that a transaction cannot be signed if
        it contains illegal input."""
        idx = l.w_start_transaction()
        l.w_save_raw_transaction(idx, expected_unsigned_double_utxo_tx_hex)
        l.w_add_output(idx, external_p2pkh_addr, high_send_amt) # spending more than received
        finalized_rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, 15, p2pkh_addr)
        half_signed_rawhex = l.w_sign_raw_transaction(0, finalized_rawhex, utxo_scriptpubkey, 1, 2, privkey_wif)
        full_signed_rawhex = l.w_sign_raw_transaction(1, half_signed_rawhex, utxo_scriptpubkey, 1, 10, privkey_wif)
        print("\nTransaction looks valid but will get rejected:", full_signed_rawhex)

    def test_full_transaction_build(self):
        """Test that a transaction built from scratch
        matches the expected hash when everything has
        been finalized and signed."""
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        l.w_add_utxo(idx, hash10doge, vout10doge)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        finalized_rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        self.assertTrue(finalized_rawhex==expected_unsigned_tx_hex)
        half_signed_rawhex = l.w_sign_raw_transaction(0, finalized_rawhex, utxo_scriptpubkey, 1, 2, privkey_wif)
        self.assertTrue(half_signed_rawhex==expected_signed_single_input_tx_hex)
        full_signed_rawhex = l.w_sign_raw_transaction(1, half_signed_rawhex, utxo_scriptpubkey, 1, 10, privkey_wif)
        self.assertTrue(full_signed_rawhex==expected_signed_raw_tx_hex)
        l.w_clear_transaction(idx)

        
if __name__ == "__main__":
    test_src = inspect.getsource(TestTransactionFunctions)
    unittest.TestLoader.sortTestMethodsUsing = lambda _, x, y: (
        test_src.index(f"def {x}") - test_src.index(f"def {y}")
    )
    unittest.main()
    l.w_remove_all() # free all transactions in the hash table.
