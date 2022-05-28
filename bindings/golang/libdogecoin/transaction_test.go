package libdogecoin

import (
	"testing"
)

// internal keys
var bad_privkey_wif string = "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1f"
var privkey_wif string = "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy"
var p2pkh_addr string = "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy"
var utxo_scriptpubkey string = "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac"

// external keys
var external_p2pkh_addr string = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde"

// expected hashes step by step
var expected_empty_tx_hex string = "01000000000000000000"
var expected_unsigned_single_utxo_tx_hex string = "0100000001746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffff0000000000"
var expected_unsigned_double_utxo_tx_hex string = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff0000000000"
var expected_unsigned_double_utxo_single_output_tx_hex string = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff010065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000"
var expected_unsigned_tx_hex string = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
var expected_signed_single_input_tx_hex string = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"
var expected_signed_raw_tx_hex string = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006a47304402200e19c2a66846109aaae4d29376040fc4f7af1a519156fe8da543dc6f03bb50a102203a27495aba9eead2f154e44c25b52ccbbedef084f0caf1deedaca87efd77e4e70121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000"

// existing transactions
var hash2doge string = "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074"  // transaction worth 2 doge
var hash10doge string = "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2" // transaction worth 10 doge
var vout2doge int = 1                                                                      // vout is the spendable output index from the input transaction
var vout10doge int = 1

// new transaction parameters
var send_amt int = 5
var total_utxo_input int = 12
var fee float64 = 0.00226

func TestTransaction(t *testing.T) {
	t.Run("start_transaction", func(t *testing.T) {
		res := w_start_transaction()
		if res <= 0 {
			t.Errorf("Invalid index received.")
		}
		res2 := w_start_transaction()
		if res2 != res+1 {
			t.Errorf("Transactions are not consecutive.")
		}
		w_clear_transaction(res)
		w_clear_transaction(res2)
	})

	t.Run("get_raw_transaction", func(t *testing.T) {
		idx := w_start_transaction()
		valid_idx_res := w_get_raw_transaction(idx)
		invalid_idx_res := w_get_raw_transaction(idx + 1)
		if valid_idx_res != expected_empty_tx_hex {
			t.Errorf("Transaction hex is incorrect.")
		}
		if invalid_idx_res != "" {
			t.Errorf("Invalid index should receive null transaction.")
		}
		w_clear_transaction(idx)
	})

	t.Run("clear_transaction", func(t *testing.T) {
		idx := w_start_transaction()
		if w_save_raw_transaction(idx, expected_unsigned_double_utxo_single_output_tx_hex) == 0 {
			t.Errorf("Error while deserializing expected transaction.")
		}
		w_clear_transaction(idx)
		rawhex := w_get_raw_transaction(idx)
		if rawhex != "" {
			t.Errorf("Error while erasing current transaction.")
		}
	})

	t.Run("single_utxo", func(t *testing.T) {
		idx := w_start_transaction()
		if w_add_utxo(idx, hash2doge, vout2doge) == 0 {
			t.Errorf("Error while running add_utxo().")
		}
		rawhex := w_get_raw_transaction(idx)
		if rawhex != expected_unsigned_single_utxo_tx_hex {
			t.Errorf("Transaction state does not match expected hash.")
		}
		w_clear_transaction(idx)
	})

	t.Run("double_utxo", func(t *testing.T) {
		idx := w_start_transaction()
		if w_add_utxo(idx, hash2doge, vout2doge) == 0 || w_add_utxo(idx, hash10doge, vout10doge) == 0 {
			t.Errorf("Error while running add_utxo().")
		}
		rawhex := w_get_raw_transaction(idx)
		if rawhex != expected_unsigned_double_utxo_tx_hex {
			t.Errorf("Transaction state does not match expected hash.")
		}
		w_clear_transaction(idx)
	})

	t.Run("save_raw_transaction", func(t *testing.T) {
		// start to build one transaction
		idx := w_start_transaction()
		w_add_utxo(idx, hash2doge, vout2doge)
		rawhex := w_get_raw_transaction(idx)
		// save progress to a new transaction
		idx2 := w_start_transaction()
		w_save_raw_transaction(idx2, rawhex)
		rawhex2 := w_get_raw_transaction(idx2)
		//ensure that both working transactions are equal
		if rawhex != rawhex2 {
			t.Errorf("Saved transaction copy does not match original.")
		}
		w_clear_transaction(idx)
		w_clear_transaction(idx2)
	})

	t.Run("add_output", func(t *testing.T) {
		idx := w_start_transaction()
		if w_save_raw_transaction(idx, expected_unsigned_double_utxo_tx_hex) == 0 {
			t.Errorf("Error while deserializing expected transaction.")
		}
		if w_add_output(idx, external_p2pkh_addr, send_amt) == 0 {
			t.Errorf("Error while adding output.")
		}
		rawhex := w_get_raw_transaction(idx)
		if rawhex != expected_unsigned_double_utxo_single_output_tx_hex {
			t.Errorf("Transaction state does not match expected hash.")
		}
		w_clear_transaction(idx)
	})

	t.Run("finalize_transaction", func(t *testing.T) {
		idx := w_start_transaction()
		if w_save_raw_transaction(idx, expected_unsigned_double_utxo_single_output_tx_hex) == 0 {
			t.Errorf("Error while deserializing expected transaction.")
		}
		rawhex := w_finalize_transaction(idx, external_p2pkh_addr, fee, total_utxo_input, p2pkh_addr)
		if rawhex != expected_unsigned_tx_hex {
			t.Errorf("Transaction state does not match expected hash.")
		}
		w_clear_transaction(idx)
	})

	t.Run("bad_sign_raw_transaction", func(t *testing.T) {
		idx := w_start_transaction()
		if w_save_raw_transaction(idx, expected_unsigned_tx_hex) == 0 {
			t.Errorf("Error while deserializing expected transaction.")
		}
		rawhex := w_get_raw_transaction(idx)
		rawhex = w_sign_raw_transaction(0, rawhex, utxo_scriptpubkey, 1, 2, bad_privkey_wif)
		if rawhex != "" {
			t.Errorf("Bad private key should yield empty transaction.")
		}
		w_clear_transaction(idx)
	})

	t.Run("sign_raw_transaction", func(t *testing.T) {
		idx := w_start_transaction()
		if w_save_raw_transaction(idx, expected_unsigned_tx_hex) == 0 {
			t.Errorf("Error while deserializing expected transaction.")
		}
		rawhex := w_get_raw_transaction(idx)
		rawhex = w_sign_raw_transaction(0, rawhex, utxo_scriptpubkey, 1, 2, privkey_wif)
		if rawhex != expected_signed_single_input_tx_hex {
			t.Errorf("Error signing first input.")
		}
		rawhex = w_sign_raw_transaction(1, rawhex, utxo_scriptpubkey, 1, 10, privkey_wif)
		if rawhex != expected_signed_raw_tx_hex {
			t.Errorf("Error signing second input.")
		}
		w_clear_transaction(idx)
	})
}
