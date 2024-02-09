#include "libdogecoin.h"
#include <stdio.h>
#include <string.h>

// Example of how to use libdogecoin API functions:
// gcc ./examples/example.c -I./include -L./lib -ldogecoin -lunistring -o example

// (or in the case of this project's directory structure, and if you want to build statically):
// (after build, from the /libdogecoin project root directory)
// gcc ./contrib/examples/example.c ./.libs/libdogecoin.a -I./include/dogecoin -L./.libs -ldogecoin -lunistring -o example
// then run 'example'.

//  for windows, from the command line: (after build, from the /libdogecoin project root directory) run:
//  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" first to set up the environment.
//  then run: cl.exe contrib/examples/example.c /I"include\dogecoin" /link "build\Debug\dogecoin.lib" ncrypt.lib tbs.lib msvcrt.lib advapi32.lib /out:example.exe
//  then run: example.exe

int main() {
	dogecoin_ecc_start();

	// BASIC ADDRESS EXAMPLES
	printf("\n\nBEGIN BASIC ADDRESSING:\n\n");
	// create variables

	char wif_privkey[PRIVKEYWIFLEN];
	char p2pkh_pubkey[P2PKHLEN];
	char hd_master_privkey[HDKEYLEN];
	char p2pkh_master_pubkey[P2PKHLEN];
	char p2pkh_child_pubkey[P2PKHLEN];

	// keypair generation
	if (generatePrivPubKeypair(wif_privkey, p2pkh_pubkey, 0)) {
		printf("Mainnet keypair 1:\n===============================\nPrivate: %s\nPublic:  %s\n\n", wif_privkey, p2pkh_pubkey);
	}
	else {
		printf("Error occurred 1.\n");
		return -1;
	}

	if (generateHDMasterPubKeypair(hd_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Mainnet master keypair 2:\n===============================\nPrivate: %s\nPublic:  %s\n\n", hd_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Error occurred 2.\n");
		return -1;
	}


	if (generateDerivedHDPubkey((const char*)hd_master_privkey, (char*)p2pkh_child_pubkey)) {
		printf("Mainnet master derived keypair 3:\n===============================\nPrivate: %s\nPublic:  %s\n\n", hd_master_privkey, p2pkh_child_pubkey);
	}
	else {
		printf("Error occurred 3.\n");
		return -1;
	}
	printf("\n");

	// keypair verification
	if (verifyPrivPubKeypair(wif_privkey, p2pkh_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 4.\n\n", wif_privkey, p2pkh_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 4.\n", wif_privkey, p2pkh_pubkey);
		return -1;
	}

	if (verifyHDMasterPubKeypair(hd_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 5.\n\n", hd_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 5.\n", hd_master_privkey, p2pkh_master_pubkey);
		return -1;
	}

	if (verifyHDMasterPubKeypair(hd_master_privkey, p2pkh_child_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 6.\n\n", hd_master_privkey, p2pkh_child_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 6.\n", hd_master_privkey, p2pkh_child_pubkey);
		return -1;
	}
	printf("\n");

	// address verification
	if (verifyP2pkhAddress(p2pkh_pubkey, strlen(p2pkh_pubkey))) {
		printf("Address %s is valid for mainnet 7.\n\n", p2pkh_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 7.\n", p2pkh_pubkey);
		return -1;
	}

	if (verifyP2pkhAddress(p2pkh_master_pubkey, strlen(p2pkh_master_pubkey))) {
		printf("Address %s is valid for mainnet 8.\n\n", p2pkh_master_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 8.\n", p2pkh_master_pubkey);
		return -1;
	}

	if (verifyP2pkhAddress(p2pkh_child_pubkey, strlen(p2pkh_child_pubkey))) {
		printf("Address %s is valid for mainnet 9.\n", p2pkh_child_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 9.\n", p2pkh_child_pubkey);
		return -1;
	}
	printf("\n");
	// END ===========================================


	// DERIVED HD ADDRESS EXAMPLE
	printf("\n\nBEGIN HD ADDRESS DERIVATION EXAMPLE:\n\n");
	size_t extoutsize = HDKEYLEN;
	char* extout = dogecoin_char_vla(extoutsize);
	char* masterkey_main_ext = "dgpv51eADS3spNJh8h13wso3DdDAw3EJRqWvftZyjTNCFEG7gqV6zsZmucmJR6xZfvgfmzUthVC6LNicBeNNDQdLiqjQJjPeZnxG8uW3Q3gCA3e";

	if (getDerivedHDAddress(masterkey_main_ext, 0, false, 0, extout, true)) {
		printf("Derived HD Addresses:\n%s\n%s\n\n", extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
	} else {
		printf("getDerviedHDAddress failed!\n");
		return -1;
	}

	char keypath[BIP44_KEY_PATH_MAX_SIZE] = "m/44'/3'/0'/0/0";
	if (getDerivedHDAddressByPath(masterkey_main_ext, keypath, extout, true)) {
		printf("Derived HD Addresses:\n%s\n%s\n", extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
	} else {
		printf("getDerivedHDAddressByPath failed!\n");
		return -1;
	}

	// test getHDNodeAndExtKeyByPath
	size_t wiflen = PRIVKEYWIFLEN;
	char privkeywif_main[PRIVKEYWIFLEN];
	dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, keypath, extout, true);
	if (strcmp(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "09648faa2fa89d84c7eb3c622e06ed2c1c67df223bc85ee206b30178deea7927") != 0) {
		printf("getHDNodeAndExtKeyByPath!\n");
	}
	dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
	if (strcmp(privkeywif_main, "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG") != 0) {
		printf("privkeywif_main does not match!\n");
	}
	if (strcmp(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7") != 0) {
		printf("extout does not match!\n");
	}
	char* privkeywifbypath = getHDNodePrivateKeyWIFByPath(masterkey_main_ext, keypath, extout, true);
	if (strcmp(privkeywifbypath, "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG") != 0) {
		printf("private key WIF does not match!\n");
	}
	if (strcmp(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7") != 0) {
		printf("extout does not match!\n");
	}
	dogecoin_hdnode_free(hdnode);
	dogecoin_free(extout);
	dogecoin_free(privkeywifbypath);
	// END ===========================================

	// TOOLS EXAMPLE
	printf("\n\nTOOLS EXAMPLE:\n\n");
	char addr[P2PKHLEN];
	if (addresses_from_pubkey(&dogecoin_chainparams_main, "039ca1fdedbe160cb7b14df2a798c8fed41ad4ed30b06a85ad23e03abe43c413b2", addr)) {
		printf ("addr: %s\n", addr);
	}

	size_t pubkeylen = PUBKEYHEXLEN;
	char* pubkey=dogecoin_char_vla(pubkeylen);
	if (pubkey_from_privatekey(&dogecoin_chainparams_main, "QUaohmokNWroj71dRtmPSses5eRw5SGLKsYSRSVisJHyZdxhdDCZ", pubkey, &pubkeylen)) {
		printf ("pubkey: %s\n", pubkey);
	}
	dogecoin_free(pubkey);

	size_t privkeywiflen = PRIVKEYWIFLEN;
	char* privkeywif=dogecoin_char_vla(privkeywiflen);
	char privkeyhex[PRIVKEYHEXLEN];
	if (gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, NULL)) {
			if (gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, privkeyhex)) {
			printf ("privkeywif: %s\n", privkeywif);
			printf ("privkeyhex: %s\n", privkeyhex);
		}
	}
	dogecoin_free(privkeywif);
	// END ===========================================

	// BIP44 EXAMPLE
	printf("\n\nBIP44 EXAMPLE:\n\n");

	int result;
	dogecoin_hdnode node;
	dogecoin_hdnode bip44_key;
	size_t size;

	dogecoin_hdnode_from_seed(utils_hex_to_uint8("000102030405060708090a0b0c0d0e0f"), 16, &node);
	printf ("seed: 000102030405060708090a0b0c0d0e0f\n");

	char master_key_str[HDKEYLEN];

	// Print the master key (MAINNET)
	dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, master_key_str, sizeof(master_key_str));
	printf("BIP32 master pub key: %s\n", master_key_str);
	dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, master_key_str, sizeof(master_key_str));
	printf("BIP32 master prv key: %s\n", master_key_str);

	char* change_level = BIP44_CHANGE_EXTERNAL;
	uint32_t account = BIP44_FIRST_ACCOUNT_NODE;

	// Derive the BIP 44 extended key
	result = derive_bip44_extended_key(&node, &account, NULL, change_level, NULL, false, keypath, &bip44_key);

	// Print the BIP 44 extended key
	char bip44_private_key[HDKEYLEN];
	dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_main, bip44_private_key, sizeof(bip44_private_key));
	printf("BIP44 extended key: %s\n", bip44_private_key);

	char str[P2PKHLEN];

	// Print the BIP 44 extended public key
	char bip44_public_key[HDKEYLEN];
	dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_main, bip44_public_key, sizeof(bip44_public_key));
	printf("BIP44 extended public key: %s\n", bip44_public_key);

	printf("%s", "Derived Addresses\n");

		char wifstr[PRIVKEYWIFLEN];
		wiflen = PRIVKEYWIFLEN;

	for (uint32_t index = BIP44_FIRST_ADDRESS_INDEX; index < BIP44_ADDRESS_GAP_LIMIT; index++) {
		// Derive the addresses
		result = derive_bip44_extended_key(&node, &account, &index, change_level, NULL, false, keypath, &bip44_key);

		// Print the private key
		dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_main, bip44_private_key, sizeof(bip44_private_key));
		printf("private key (serialized): %s\n", bip44_private_key);

		// Print the public key
		dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_main, bip44_public_key, sizeof(bip44_public_key));
		printf("public key (serialized): %s\n", bip44_public_key);

		// Print the wif private key
		dogecoin_privkey_encode_wif((dogecoin_key*) bip44_key.private_key, &dogecoin_chainparams_main, wifstr, &wiflen);
		printf("private key (wif): %s\n", wifstr);

		// Print the p2pkh address
		dogecoin_hdnode_get_p2pkh_address(&bip44_key, &dogecoin_chainparams_main, str, sizeof(str));
		printf("Address: %s\n", str);
	}

	// EXTENDED PUBLIC KEY DERIVATION EXAMPLE
	printf("\n\nBEGIN EXTENDED PUBLIC KEY DERIVATION:\n\n");

	// Generate the Master key from a seed
	char masterkey[HDKEYLEN];
	getHDRootKeyFromSeed(utils_hex_to_uint8("5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc19a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"), MAX_SEED_SIZE, false, masterkey);
	printf("Master key: %s\n", masterkey);

	// Serialize and print the Master public key
	char master_public_key[HDKEYLEN] = {0};
	getHDPubKey(masterkey, false, master_public_key);
	printf("Master public key: %s\n", master_public_key);

	// Derive an extended normal (non-hardened) public key
	char extkeypath[KEYPATHMAXLEN] = "m/0/0/0/0/0";
	char extpubkey[HDKEYLEN] = {0};
	getHDNodeAndExtKeyByPath(master_public_key, extkeypath, extpubkey, false);
	printf("Keypath: %s\nExtended public key: %s\n", extkeypath, extpubkey);
	if (strcmp(extpubkey, "dgub8wcWPRxhthgZYftisbirNJ5Ae3navJCCEfd6SzyL5SK44GC4tok3BGkNWbhrM4KeJ8o9ZAkXiVdLTnUyzz89ah1izJjWTo5pv7eboGtzktJ") != 0) {
			printf("extpubkey does not match!\n");
	}

	// BASIC TRANSACTION FORMATION EXAMPLE
	printf("\n\nBEGIN TRANSACTION FORMATION AND SIGNING:\n\n");
	// declare keys and previous hashes
	char *external_p2pkh_addr = 	"nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";
	char *hash_2_doge = 			"b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";
	char *hash_10_doge = 			"42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2";
        char myscriptpubkey [PUBKEYHASHLEN];
        dogecoin_p2pkh_address_to_pubkey_hash (str, myscriptpubkey);

	// build transaction
	int idx = start_transaction();
	printf("Empty transaction created at index %d.\n", idx);

	if (add_utxo(idx, hash_2_doge, 1)) {
		printf("Input of value 2 dogecoin added to the transaction.\n");
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}

	if (add_utxo(idx, hash_10_doge, 1)) {
		printf("Input of value 10 dogecoin added to the transaction.\n");
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}

	if (add_output(idx, external_p2pkh_addr, "5.0")) {
		printf("Output of value 5 dogecoin added to the transaction.\n");
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}

	// save the finalized unsigned transaction to a new index in the hash table
	int idx2 = store_raw_transaction(finalize_transaction(idx, external_p2pkh_addr, "0.00226", "12", str));
	if (idx2 > 0) {
		printf("Change returned to address %s and finalized unsigned transaction saved at index %d.\n", str, idx2);
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}

	// sign transaction
	if (sign_transaction(idx, myscriptpubkey, wifstr)) {
		printf("\nAll transaction inputs signed successfully. \nFinal transaction hex: %s\n.", get_raw_transaction(idx));
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}
    remove_all();
	// END ===========================================


	// BASIC MESSAGE SIGNING EXAMPLE
	printf("\n\nBEGIN BASIC MESSAGE SIGNING:\n\n");
	char* msg = "This is just a test message";
    char* privkey = "QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2";
    char* address = "D6a52RGbfvKDzKTh8carkGd1vNdAurHmaS";
    char* sig = sign_message(privkey, msg);
	if (verify_message(sig, msg, address)) {
		printf("Addresses match!\n");
	} else {
		printf("Addresses do not match!\n");
		return -1;
	}

    // testcase 2:
    // assert modified msg will cause verification failure:
    msg = "This is a new test message";
	if (!verify_message(sig, msg, address)) {
		printf("Addresses do not match!\n");
	} else {
		printf("Addresses match!\n");
		return -1;
	}

	// testcase 3:
    msg = "This is just a test message";
	if (verify_message(sig, msg, address)) {
		printf("Addresses match!\n");
	} else {
		printf("Addresses do not match!\n");
		return -1;
	}
    dogecoin_free(sig);
	// END ===========================================


	// ADVANCED MESSAGE SIGNING EXAMPLE
	printf("\n\nBEGIN ADVANCED MESSAGE SIGNING:\n\n");
    for (int i = 0; i < 10; i++) {
        // key 1:
        int key_id = start_key(false);
        eckey* key = find_eckey(key_id);
        char* msg = "This is a test message";
        char* sig = sign_message(key->private_key_wif, msg);
        if (verify_message(sig, msg, key->address)) {
			printf("Addresses match!\n");
		} else {
			printf("Message verification failed!\n");
			return -1;
		}
        remove_eckey(key);
        dogecoin_free(sig);

        // key 2:
        int key_id2 = start_key(true);
        eckey* key2 = find_eckey(key_id2);
        char* msg2 = "This is a test message";
        char* sig2 = sign_message(key2->private_key_wif, msg2);
        if (verify_message(sig2, msg2, key2->address)) {
			printf("Addresses match!\n");
		} else {
			printf("Message verification failed!\n");
			return -1;
		}

        // test message signature verification failure:
        msg2 = "This is an altered test message";
        if (!verify_message(sig2, msg2, key2->address)) {
			printf("Addresses do not match!\n");
		} else {
			printf("Message verification failed!\n");
			return -1;
		}
        remove_eckey(key2);
        dogecoin_free(sig2);
    }

#if defined(USE_TPM2)
	// TPM2 TESTS
	printf("\n\nBEGIN TPM2 TESTS:\n\n");

	// test dogecoin_encrypt_seed_with_tpm
	SEED seed = {0};
	if (dogecoin_encrypt_seed_with_tpm(seed, sizeof(seed), TEST_FILE, true)) {
		printf("Seed encrypted with TPM2.\n");
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test dogecoin_generate_mnemonic_encrypt_with_tpm
	MNEMONIC mnemonic = {0};
	if (dogecoin_generate_mnemonic_encrypt_with_tpm(mnemonic, TEST_FILE, true, "eng", " ", NULL)) {
		printf("Mnemonic generated and encrypted with TPM2.\n");
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test dogecoin_generate_hdnode_encrypt_with_tpm
	dogecoin_hdnode out;
	if (dogecoin_generate_hdnode_encrypt_with_tpm(&out, TEST_FILE, true)) {
		printf("HD node generated and encrypted with TPM2.\n");
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test generateRandomEnglishMnemonicTPM
	if (generateRandomEnglishMnemonicTPM(mnemonic, TEST_FILE, true)) {
		printf("Mnemonic: %s\n", mnemonic);
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test getDerivedHDAddressFromEncryptedSeed
	char derived_address[P2PKHLEN];
	if (getDerivedHDAddressFromEncryptedSeed(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0) {
		printf("Derived address: %s\n", derived_address);
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test getDerivedHDAddressFromEncryptedMnemonic
	if (getDerivedHDAddressFromEncryptedMnemonic(0, 0, BIP44_CHANGE_EXTERNAL, NULL, derived_address, false, TEST_FILE) == 0) {
		printf("Derived address: %s\n", derived_address);
	} else {
		printf("Error occurred.\n");
		return -1;
	}

	// test getDerivedHDAddressFromEncryptedHDNode
	if (getDerivedHDAddressFromEncryptedHDNode(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0) {
		printf("Derived address: %s\n", derived_address);
	} else {
		printf("Error occurred.\n");
		return -1;
	}

#endif

	printf("\nTESTS COMPLETE!\n");
	dogecoin_ecc_stop();
}
