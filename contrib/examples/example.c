#include "libdogecoin.h"
#include <stdio.h>
#include <string.h>

//#define PRIVKEYWIFLEN 51 //WIF length for uncompressed keys is 51 and should start with Q. This can be 52 also for compressed keys. 53 internally to lib (+stringterm)
#define PRIVKEYWIFLEN 53 //Function takes 53 but needs to be fixed to take 51.

//#define MASTERKEYLEN 111 //should be chaincode + privkey; starts with dgpv51eADS3spNJh8 or dgpv51eADS3spNJh9 (112 internally including stringterm? often 128. check this.)
#define MASTERKEYLEN 128 // Function expects 128 but needs to be fixed to take 111.

//#define PUBKEYLEN 34 //our mainnet addresses are 34 chars if p2pkh and start with D.  Internally this is cited as 35 for strings that represent it because +stringterm.
#define PUBKEYLEN 35 // Function expects 35, but needs to be fixed to take 34.


// Example of how to use libdogecoin API functions:
// gcc ./examples/example.c -I./include -L./lib -ldogecoin -o example

// (or in the case of this project's directory structure, and if you want to build statically):
// (after build, from the /libdogecoin project root directory)
// gcc ./contrib/examples/example.c ./.libs/libdogecoin.a -I./include/dogecoin -L./.libs -ldogecoin -lunistring -o example
// then run 'example'. 

int main() {
	dogecoin_ecc_start();

	// BASIC ADDRESS EXAMPLES
	printf("\n\nBEGIN BASIC ADDRESSING:\n\n");
	// create variables
	
	char wif_privkey[PRIVKEYWIFLEN];
	char p2pkh_pubkey[PUBKEYLEN];
	char wif_master_privkey[MASTERKEYLEN];
	char p2pkh_master_pubkey[PUBKEYLEN];
	char p2pkh_child_pubkey[PUBKEYLEN];
	
	// keypair generation
	if (generatePrivPubKeypair(wif_privkey, p2pkh_pubkey, 0)) {
		printf("Mainnet keypair 1:\n===============================\nPrivate: %s\nPublic:  %s\n\n", wif_privkey, p2pkh_pubkey);
	}
	else {
		printf("Error occurred 1.\n");
		return -1;
	}

	if (generateHDMasterPubKeypair(wif_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Mainnet master keypair 2:\n===============================\nPrivate: %s\nPublic:  %s\n\n", wif_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Error occurred 2.\n");
		return -1;
	}


	if (generateDerivedHDPubkey((const char*)wif_master_privkey, (char*)p2pkh_child_pubkey)) {
		printf("Mainnet master derived keypair 3:\n===============================\nPrivate: %s\nPublic:  %s\n\n", wif_master_privkey, p2pkh_child_pubkey);
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

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 5.\n\n", wif_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 5.\n", wif_master_privkey, p2pkh_master_pubkey);
		return -1;
	}

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_child_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 6.\n\n", wif_master_privkey, p2pkh_child_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 6.\n", wif_master_privkey, p2pkh_child_pubkey);
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
	size_t extoutsize = 112;
	char* extout = dogecoin_char_vla(extoutsize);
	char* masterkey_main_ext = "dgpv51eADS3spNJh8h13wso3DdDAw3EJRqWvftZyjTNCFEG7gqV6zsZmucmJR6xZfvgfmzUthVC6LNicBeNNDQdLiqjQJjPeZnxG8uW3Q3gCA3e";
	
	if (getDerivedHDAddress(masterkey_main_ext, 0, false, 0, extout, true)) {
		printf("Derived HD Addresses:\n%s\n%s\n\n", extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
	} else {
		printf("getDerviedHDAddress failed!\n");
		return -1;
	}

	if (getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true)) {
		printf("Derived HD Addresses:\n%s\n%s\n", extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
	} else {
		printf("getDerivedHDAddressByPath failed!\n");
		return -1;
	}
	dogecoin_free(extout);
	// END ===========================================


	// BASIC TRANSACTION FORMATION EXAMPLE
	printf("\n\nBEGIN TRANSACTION FORMATION AND SIGNING:\n\n");
	// declare keys and previous hashes
	char *external_p2pkh_addr = 	"nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";
	char *myprivkey = 				"ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy";
	char *mypubkey = 				"noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy";
	char *myscriptpubkey = 			"76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac";
	char *hash_2_doge = 			"b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";
	char *hash_10_doge = 			"42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2";

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
	// save the finalized unsigned transaction to a new index in the hash table
	int idx2 = store_raw_transaction(finalize_transaction(idx, external_p2pkh_addr, "0.00226", "12", mypubkey));
	if (idx2 > 0) {
		printf("Change returned to address %s and finalized unsigned transaction saved at index %d.\n", mypubkey, idx2);
	}
	else {
		printf("Error occurred.\n");
		return -1;
	}

	// sign transaction
	if (sign_transaction(idx, myscriptpubkey, myprivkey)) {
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

	printf("\nTESTS COMPLETE!\n");
	dogecoin_ecc_stop();
}
