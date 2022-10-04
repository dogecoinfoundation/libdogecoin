package libdogecoin

/*
#cgo CFLAGS: -I${SRCDIR}/../../.. -I${SRCDIR}/../../../include -I${SRCDIR}/../../../include/dogecoin -I${SRCDIR}/../../../include/dogecoin/ -I${SRCDIR}/../../../src/secp256k1/include -I${SRCDIR}/../../../src/secp256k1/src -I${SRCDIR}/../../../depends/x86_64-pc-linux-gnu/include -fPIC
#cgo LDFLAGS: -L${SRCDIR}/../../../.libs -L${SRCDIR}/../../../src/secp256k1/.libs -L${SRCDIR}/../../../depends/x86_64-pc-linux-gnu/lib -ldogecoin -levent_core -levent -lpthread -lsecp256k1 -lsecp256k1_precomputed -lm -Wl,-rpath=${SRCDIR}/../../../.libs
#include "libdogecoin.h"
*/
import "C"
import (
	"fmt"
	"strconv"
	"unsafe"
)

func W_context_start() {
	C.dogecoin_ecc_start()
}

func W_context_stop() {
	C.dogecoin_ecc_stop()
}

func W_generate_priv_pub_keypair(is_testnet bool) (wif_privkey string, p2pkh_pubkey string) {
	c_wif_privkey := [53]C.char{}
	c_p2pkh_pubkey := [35]C.char{}
	c_is_testnet := C._Bool(is_testnet)
	C.generatePrivPubKeypair((*C.char)(&c_wif_privkey[0]), (*C.char)(&c_p2pkh_pubkey[0]), c_is_testnet)
	wif_privkey = C.GoString((*C.char)(&c_wif_privkey[0]))
	p2pkh_pubkey = C.GoString((*C.char)(&c_p2pkh_pubkey[0]))
	return
}

func W_generate_hd_master_pub_keypair(is_testnet bool) (wif_privkey_master string, p2pkh_pubkey_master string) {
	c_wif_privkey_master := [128]C.char{}
	c_p2pkh_pubkey_master := [35]C.char{}
	c_is_testnet := C._Bool(is_testnet)
	C.generateHDMasterPubKeypair((*C.char)(&c_wif_privkey_master[0]), (*C.char)(&c_p2pkh_pubkey_master[0]), c_is_testnet)
	wif_privkey_master = C.GoString((*C.char)(&c_wif_privkey_master[0]))
	p2pkh_pubkey_master = C.GoString((*C.char)(&c_p2pkh_pubkey_master[0]))
	return
}

func W_generate_derived_hd_pub_key(wif_privkey_master string) (child_p2pkh_pubkey string) {
	c_wif_privkey_master := C.CString(wif_privkey_master)
	c_child_p2pkh_pubkey := [35]C.char{}
	C.generateDerivedHDPubkey(c_wif_privkey_master, (*C.char)(&c_child_p2pkh_pubkey[0]))
	child_p2pkh_pubkey = C.GoString((*C.char)(&c_child_p2pkh_pubkey[0]))
	C.free(unsafe.Pointer(c_wif_privkey_master))
	return
}

func W_verify_priv_pub_keypair(wif_privkey string, p2pkh_pubkey string, is_testnet bool) (result bool) {
	c_wif_privkey := C.CString(wif_privkey)
	c_p2pkh_pubkey := C.CString(p2pkh_pubkey)
	c_is_testnet := C._Bool(is_testnet)
	if C.verifyPrivPubKeypair(c_wif_privkey, c_p2pkh_pubkey, c_is_testnet) == 1 {
		result = true
	} else {
		result = false
	}
	C.free(unsafe.Pointer(c_wif_privkey))
	C.free(unsafe.Pointer(c_p2pkh_pubkey))
	return
}

func W_verify_hd_master_pub_keypair(wif_privkey_master string, p2pkh_pubkey_master string, is_testnet bool) (result bool) {
	c_wif_privkey_master := C.CString(wif_privkey_master)
	c_p2pkh_pubkey_master := C.CString(p2pkh_pubkey_master)
	c_is_testnet := C._Bool(is_testnet)
	if C.verifyHDMasterPubKeypair(c_wif_privkey_master, c_p2pkh_pubkey_master, c_is_testnet) == 1 {
		result = true
	} else {
		result = false
	}
	C.free(unsafe.Pointer(c_wif_privkey_master))
	C.free(unsafe.Pointer(c_p2pkh_pubkey_master))
	return
}

func W_verify_p2pkh_address(p2pkh_pubkey string) (result bool) {
	c_p2pkh_pubkey := C.CString(p2pkh_pubkey)
	len := len(p2pkh_pubkey)
	c_len := C.size_t(len)
	if C.verifyP2pkhAddress(c_p2pkh_pubkey, c_len) == 1 {
		result = true
	} else {
		result = false
	}
	C.free(unsafe.Pointer(c_p2pkh_pubkey))
	return
}

func W_start_transaction() (result int) {
	result = int(C.start_transaction())
	return
}

func W_add_utxo(tx_index int, hex_utxo_txid string, vout int) (result int) {
	c_tx_index := C.int(tx_index)
	c_hex_utxo_txid := C.CString(hex_utxo_txid)
	c_vout := C.int(vout)
	result = int(C.add_utxo(c_tx_index, c_hex_utxo_txid, c_vout))
	C.free(unsafe.Pointer(c_hex_utxo_txid))
	return
}

func W_add_output(tx_index int, destination_address string, amount string) (result int) {
	c_tx_index := C.int(tx_index)
	c_destination_address := C.CString(destination_address)
	_, err := strconv.ParseFloat(amount, 64)
	if err != nil {
		fmt.Println("Error: amount is not numeric.")
		return 0
	}
	c_amount := C.CString(amount)
	result = int(C.add_output(c_tx_index, c_destination_address, c_amount))
	C.free(unsafe.Pointer(c_destination_address))
	return
}

func W_finalize_transaction(tx_index int, destination_address string, subtracted_fee string, out_doge_amount_for_verification string, public_key string) (result string) {
	c_tx_index := C.int(tx_index)
	c_destination_address := C.CString(destination_address)
	_, err1 := strconv.ParseFloat(subtracted_fee, 64)
	if err1 != nil {
		fmt.Println("Error: subtracted fee is not numeric.")
		return ""
	}
	c_subtracted_fee := C.CString(subtracted_fee)
	_, err2 := strconv.ParseFloat(out_doge_amount_for_verification, 64)
	if err2 != nil {
		fmt.Println("Error: send amount is not numeric.")
		return ""
	}
	c_out_doge_amount_for_verification := C.CString(out_doge_amount_for_verification)
	c_public_key := C.CString(public_key)
	result = C.GoString(C.finalize_transaction(c_tx_index, c_destination_address, c_subtracted_fee, c_out_doge_amount_for_verification, c_public_key))
	C.free(unsafe.Pointer(c_destination_address))
	C.free(unsafe.Pointer(c_public_key))
	return
}

func W_get_raw_transaction(tx_index int) (result string) {
	c_tx_index := C.int(tx_index)
	result = C.GoString(C.get_raw_transaction(c_tx_index))
	return
}

func W_clear_transaction(tx_index int) {
	c_tx_index := C.int(tx_index)
	C.clear_transaction(c_tx_index)
}

func W_sign_raw_transaction(input_index int, incoming_raw_tx string, script_hex string, sig_hash_type int, privkey string) (result string) {
	c_input_index := C.int(input_index)
	c_incoming_raw_tx := [1024 * 100]C.char{}
	for i := 0; i < len(incoming_raw_tx) && i < 1024; i++ {
		c_incoming_raw_tx[i] = C.char(incoming_raw_tx[i])
	}
	c_script_hex := C.CString(script_hex)
	c_sig_hash_type := C.int(sig_hash_type)

	c_privkey := C.CString(privkey)

	if C.sign_raw_transaction(c_input_index, &c_incoming_raw_tx[0], c_script_hex, c_sig_hash_type, c_privkey) == 1 {
		result = C.GoString(&c_incoming_raw_tx[0])
	} else {
		result = ""
	}
	C.free(unsafe.Pointer(c_script_hex))
	C.free(unsafe.Pointer(c_privkey))
	return
}

func W_sign_transaction(tx_index int, script_pubkey string, privkey string) (result int) {
	c_tx_index := C.int(tx_index)
	c_script_pubkey := C.CString(script_pubkey)
	c_privkey := C.CString(privkey)
	result = int(C.sign_transaction(c_tx_index, c_script_pubkey, c_privkey))
	C.free(unsafe.Pointer(c_script_pubkey))
	C.free(unsafe.Pointer(c_privkey))
	return
}

func W_store_raw_transaction(incoming_raw_tx string) (result int) {
	c_incoming_raw_tx := C.CString(incoming_raw_tx)
	result = int(C.store_raw_transaction(c_incoming_raw_tx))
	return
}
