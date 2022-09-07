#include "libdogecoin.h"
#include <stdio.h>
#include <string.h>

#define PRIVKEYWIFLEN 53
#define MASTERKEYLEN 200
#define PUBKEYLEN 35

// Example of how to use libdogecoin API functions:
// gcc ./examples/example.c -I./include -L./lib -ldogecoin -o example



int main() {
	dogecoin_ecc_start();

	// CALLING ADDRESS FUNCTIONS
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
	printf("\n\n");

	// keypair verification
	if (verifyPrivPubKeypair(wif_privkey, p2pkh_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 4.\n", wif_privkey, p2pkh_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 4.\n", wif_privkey, p2pkh_pubkey);
	}

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 5.\n", wif_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 5.\n", wif_master_privkey, p2pkh_master_pubkey);
	}

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_child_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 6.\n", wif_master_privkey, p2pkh_child_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 6.\n", wif_master_privkey, p2pkh_child_pubkey);
	}
	printf("\n\n");

	// address verification
	if (verifyP2pkhAddress(p2pkh_pubkey, strlen(p2pkh_pubkey))) {
		printf("Address %s is valid for mainnet 7.\n", p2pkh_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 7.\n", p2pkh_pubkey);
	}

	if (verifyP2pkhAddress(p2pkh_master_pubkey, strlen(p2pkh_master_pubkey))) {
		printf("Address %s is valid for mainnet 8.\n", p2pkh_master_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 8.\n", p2pkh_master_pubkey);
	}

	if (verifyP2pkhAddress(p2pkh_child_pubkey, strlen(p2pkh_child_pubkey))) {
		printf("Address %s is valid for mainnet 9.\n", p2pkh_child_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 9.\n", p2pkh_child_pubkey);
	}
	printf("\n\n");


	
	// CALLING TRANSACTION FUNCTIONS
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
	}

	dogecoin_ecc_stop();
}
