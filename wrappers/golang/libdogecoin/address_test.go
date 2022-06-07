package libdogecoin

import (
	"testing"
)

func TestAddress(t *testing.T) {
	t.Run("keypair_gen_mainnet", func(t *testing.T) {
		priv, pub := w_generate_priv_pub_keypair(false)
		if len(priv) == 0 {
			t.Errorf("Private key does not exist.")
		}
		if len(pub) == 0 {
			t.Errorf("Public key does not exist.")
		}
	})

	t.Run("keypair_gen_testnet", func(t *testing.T) {
		priv, pub := w_generate_priv_pub_keypair(true)
		if len(priv) == 0 {
			t.Errorf("Private key does not exist.")
		}
		if len(pub) == 0 {
			t.Errorf("Public key does not exist.")
		}
	})

	t.Run("p2pkh_addr_format_is_valid_mainnet", func(t *testing.T) {
		_, pub := w_generate_priv_pub_keypair(false)
		len := len(pub)
		if !w_verify_p2pkh_address(pub, len) {
			t.Errorf("P2PKH address is not valid.")
		}
	})

	t.Run("p2pkh_addr_format_is_valid_testnet", func(t *testing.T) {
		_, pub := w_generate_priv_pub_keypair(true)
		len := len(pub)
		if !w_verify_p2pkh_address(pub, len) {
			t.Errorf("P2PKH address is not valid.")
		}
	})

	t.Run("keypair_is_valid_mainnet", func(t *testing.T) {
		priv, pub := w_generate_priv_pub_keypair(false)
		if !w_verify_priv_pub_keypair(priv, pub, false) {
			t.Errorf("Keypair is not valid.")
		}
	})

	t.Run("keypair_is_valid_testnet", func(t *testing.T) {
		priv, pub := w_generate_priv_pub_keypair(true)
		if !w_verify_priv_pub_keypair(priv, pub, true) {
			t.Errorf("Keypair is not valid.")
		}
	})

	t.Run("master_keypair_gen_mainnet", func(t *testing.T) {
		priv, pub := w_generate_hd_master_pub_keypair(false)
		if len(priv) == 0 {
			t.Errorf("Master private key does not exist.")
		}
		if len(pub) == 0 {
			t.Errorf("Master public key does not exist.")
		}
	})

	t.Run("master_keypair_gen_testnet", func(t *testing.T) {
		priv, pub := w_generate_hd_master_pub_keypair(true)
		if len(priv) == 0 {
			t.Errorf("Master private key does not exist.")
		}
		if len(pub) == 0 {
			t.Errorf("Master public key does not exist.")
		}
	})

	t.Run("master_p2pkh_addr_format_is_valid_mainnet", func(t *testing.T) {
		_, pub := w_generate_hd_master_pub_keypair(false)
		len := len(pub)
		if !w_verify_p2pkh_address(pub, len) {
			t.Errorf("Master P2PKH address is not valid.")
		}
	})

	t.Run("master_p2pkh_addr_format_is_valid_testnet", func(t *testing.T) {
		_, pub := w_generate_hd_master_pub_keypair(true)
		len := len(pub)
		if !w_verify_p2pkh_address(pub, len) {
			t.Errorf("Master P2PKH address is not valid.")
		}
	})

	t.Run("master_keypair_is_valid_mainnet", func(t *testing.T) {
		priv, pub := w_generate_hd_master_pub_keypair(false)
		if !w_verify_hd_master_pub_keypair(priv, pub, false) {
			t.Errorf("Master keypair is not valid.")
		}
	})

	t.Run("master_keypair_is_valid_testnet", func(t *testing.T) {
		priv, pub := w_generate_hd_master_pub_keypair(true)
		if !w_verify_hd_master_pub_keypair(priv, pub, true) {
			t.Errorf("Master keypair is not valid.")
		}
	})

	t.Run("derived_hd_pubkey_gen_mainnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(false)
		child_pub := w_generate_derived_hd_pub_key(priv)
		if len(child_pub) == 0 {
			t.Errorf("Derived public key does not exist.")
		}
	})

	t.Run("derived_hd_pubkey_gen_testnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(true)
		child_pub := w_generate_derived_hd_pub_key(priv)
		if len(child_pub) == 0 {
			t.Errorf("Derived public key does not exist.")
		}
	})

	t.Run("derived_keypair_is_valid_mainnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(false)
		child_pub := w_generate_derived_hd_pub_key(priv)
		if !w_verify_hd_master_pub_keypair(priv, child_pub, false) {
			t.Errorf("Derived keypair is not valid.")
		}
	})

	t.Run("derived_keypair_is_valid_testnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(true)
		child_pub := w_generate_derived_hd_pub_key(priv)
		if !w_verify_hd_master_pub_keypair(priv, child_pub, true) {
			t.Errorf("Derived keypair is not valid.")
		}
	})

	t.Run("derived_p2pkh_addr_is_valid_mainnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(false)
		child_pub := w_generate_derived_hd_pub_key(priv)
		len := len(child_pub)
		if !w_verify_p2pkh_address(child_pub, len) {
			t.Errorf("Derived P2PKH address is not valid.")
		}
	})

	t.Run("derived_p2pkh_addr_is_valid_testnet", func(t *testing.T) {
		priv, _ := w_generate_hd_master_pub_keypair(true)
		child_pub := w_generate_derived_hd_pub_key(priv)
		len := len(child_pub)
		if !w_verify_p2pkh_address(child_pub, len) {
			t.Errorf("Derived P2PKH address is not valid.")
		}
	})
}
