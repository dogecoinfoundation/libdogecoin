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

//(or in the case of this project's directory structure, and if you want to build statically):
//(after build, from the /libdogecoin project root directory)
//gcc ./contrib/examples/example.c ./.libs/libdogecoin.a -I./include/dogecoin -L./.libs -ldogecoin -o example
//then run 'example'. 

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
		return -1;
	}

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_master_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 5.\n", wif_master_privkey, p2pkh_master_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 5.\n", wif_master_privkey, p2pkh_master_pubkey);
		return -1;
	}

	if (verifyHDMasterPubKeypair(wif_master_privkey, p2pkh_child_pubkey, 0)) {
		printf("Keypair (%s, %s) is valid for mainnet 6.\n", wif_master_privkey, p2pkh_child_pubkey);
	}
	else {
		printf("Keypair (%s, %s) is not valid for mainnet 6.\n", wif_master_privkey, p2pkh_child_pubkey);
		return -1;
	}
	printf("\n\n");

	// address verification
	if (verifyP2pkhAddress(p2pkh_pubkey, strlen(p2pkh_pubkey))) {
		printf("Address %s is valid for mainnet 7.\n", p2pkh_pubkey);
	}
	else {
		printf("Address %s is not valid for mainnet 7.\n", p2pkh_pubkey);
		return -1;
	}

	if (verifyP2pkhAddress(p2pkh_master_pubkey, strlen(p2pkh_master_pubkey))) {
		printf("Address %s is valid for mainnet 8.\n", p2pkh_master_pubkey);
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
		return -1;
	}

	dogecoin_ecc_stop();
}
