"""Testing module for wrappers from transaction.c"""

import inspect
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
vout2doge =  1 #vout is the spendable output index from the existing transaction
vout10doge = 1

# new transaction parameters
send_amt = 5
total_utxo_input = 12
fee = .00226

class TestTransactionFunctions(unittest.TestCase):

    def setUpClass():
        l.context_start()

    def tearDownClass():
        l.context_stop()

    def test_w_start_transaction(self):
        res = l.w_start_transaction()
        self.assertTrue(type(res)==int and res>=0)

    def test_w_save_raw_transaction(self):
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        rawhex = l.w_get_raw_transaction(idx)
        idx2 = l.w_start_transaction()
        l.w_save_raw_transaction(idx2, rawhex)
        rawhex2 = l.w_get_raw_transaction(idx2)
        self.assertTrue(rawhex==rawhex2)

    def test_w_get_raw_transaction(self):
        idx = l.w_start_transaction()
        valid_idx_res = l.w_get_raw_transaction(idx)
        invalid_idx_res = l.w_get_raw_transaction(idx+1)
        self.assertTrue(valid_idx_res==expected_empty_tx_hex)
        self.assertFalse(invalid_idx_res)

    def test_single_utxo(self):
        idx = l.w_start_transaction()
        check = l.w_add_utxo(idx, hash2doge, vout2doge)
        self.assertTrue(check==1)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_single_utxo_tx_hex)

    def test_double_utxo(self):
        checks = []
        idx = l.w_start_transaction()
        checks.append(l.w_add_utxo(idx, hash2doge, vout2doge))
        checks.append(l.w_add_utxo(idx, hash10doge, vout10doge))
        for x in checks:
            self.assertTrue(x==1)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_tx_hex)

    def test_w_add_output(self):
        checks = []
        idx = l.w_start_transaction()
        checks.append(l.w_add_utxo(idx, hash2doge, vout2doge))
        checks.append(l.w_add_utxo(idx, hash10doge, vout10doge))
        checks.append(l.w_add_output(idx, external_p2pkh_addr, send_amt))
        for x in checks:
            self.assertTrue(x==1)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_unsigned_double_utxo_single_output_tx_hex)

    def test_w_finalize_transaction(self):
        checks = []
        idx = l.w_start_transaction()
        checks.append(l.w_add_utxo(idx, hash2doge, vout2doge))
        checks.append(l.w_add_utxo(idx, hash10doge, vout10doge))
        checks.append(l.w_add_output(idx, external_p2pkh_addr, send_amt))
        for x in checks:
            self.assertTrue(x==1)
        rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        self.assertTrue(rawhex==expected_unsigned_tx_hex)

    def test_w_clear_transaction(self):
        idx = l.w_start_transaction()
        rawhex = l.w_get_raw_transaction(idx)
        self.assertTrue(rawhex==expected_empty_tx_hex)
        l.w_clear_transaction(idx)
        rawhex = l.w_get_raw_transaction(idx)
        self.assertFalse(rawhex)

    def test_w_sign_indexed_raw_transaction(self):
        idx = l.w_start_transaction()
        l.w_add_utxo(idx, hash2doge, vout2doge)
        l.w_add_utxo(idx, hash10doge, vout10doge)
        l.w_add_output(idx, external_p2pkh_addr, send_amt)
        rawhex = l.w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
        self.assertTrue(rawhex==expected_unsigned_tx_hex)
        l.w_save_raw_transaction(idx, rawhex)
        rawhex = l.w_sign_indexed_raw_transaction(idx, 0, l.w_get_raw_transaction(idx), utxo_scriptpubkey, 1, 2, privkey_wif)
        self.assertTrue(l.w_get_raw_transaction(idx)==expected_signed_single_input_tx_hex)
        rawhex = l.w_sign_raw_transaction(1, rawhex, utxo_scriptpubkey, 1, 10, privkey_wif)
        self.assertTrue(rawhex==expected_signed_raw_tx_hex)
        
if __name__ == "__main__":
    test_src = inspect.getsource(TestTransactionFunctions)
    unittest.TestLoader.sortTestMethodsUsing = lambda _, x, y: (
        test_src.index(f"def {x}") - test_src.index(f"def {y}")
    )
    unittest.main()
