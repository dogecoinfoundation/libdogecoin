/*

 The MIT License (MIT)

 Copyright (c) 2022 bluezr
 Copyright (c) 2023 edtubbs
 Copyright (c) 2023 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* basic address functions: return 1 if succesful 
   ----------------------------------------------
*///!init static ecc context
void dogecoin_ecc_start(void);

//!destroys the static ecc context
void dogecoin_ecc_stop(void);

/* generates a private and public keypair (a wallet import format private key and a p2pkh ready-to-use corresponding dogecoin address)*/
int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* generates a hybrid deterministic WIF master key and p2pkh ready-to-use corresponding dogecoin address. */
int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* generates a new dogecoin address from a HD master key */
int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey);

/* verify that a private key and dogecoin address match */
int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* verify that a HD Master key and a dogecoin address matches */
int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* verify that a dogecoin address is valid. */
int verifyP2pkhAddress(char* p2pkh_pubkey, size_t len);

/* Advanced API functions for mnemonic seedphrase generation
--------------------------------------------------------------------------
*/

/* Generates a random (256-bit) English mnemonic phrase */
int generateRandomEnglishMnemonic(char* mnemonic);

/*transaction creation functions - builds a dogecoin transaction
----------------------------------------------------------------
*/

/* create a new dogecoin transaction: Returns the (txindex) in memory of the transaction being worked on. */
int start_transaction();

/* add a utxo to the transaction being worked on at (txindex), specifying the utxo's txid and vout. returns 1 if successful.*/
int add_utxo(int txindex, char* hex_utxo_txid, int vout);

/* add an output to the transaction being worked on at (txindex) of amount (amount) in dogecoins, returns 1 if successful. */
int add_output(int txindex, char* destinationaddress, char* amount);

/* finalize the transaction being worked on at (txindex), with the (destinationaddress) paying a fee of (subtractedfee), */
/* re-specify the amount in dogecoin for verification, and change address for change. If not specified, change will go to the first utxo's address. */
char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress);

/* sign a raw transaction in memory at (txindex), sign (inputindex) with (scripthex) of (sighashtype), with (privkey) */
int sign_transaction(int txindex, char* script_pubkey, char* privkey);

/* clear all internal working transactions */
void remove_all();

/* retrieve the raw transaction at (txindex) as a hex string (char*) */
char* get_raw_transaction(int txindex);

/* clear the transaction at (txindex) in memory */
void clear_transaction(int txindex);

/* QR Code Generation Functions
---------------------------------
*/

//TODO: These are strings not just P2PKH but we need to set a min and max, perhaps only accept wif and p2pkh.

/*populate an array of bits that represent qrcode pixels*/
/* returns size(L or W) in pixels of QR.*/
int qrgen_p2pkh_to_qrbits(const char* in_p2pkh, uint8_t* outQrByteArray);

/* create a QR text formatted string (with line breaks) from an incoming p2pkh*/
int qrgen_p2pkh_to_qr_string(const char* in_p2pkh, char* outString);

/* Prints the given p2pkh addr as QR Code to the console. */
void qrgen_p2pkh_consoleprint_to_qr(char* in_p2pkh);

/* Creates a .png file with the filename outFilename, from string inString, w. size factor of SizeMultiplier.*/
int qrgen_string_to_qr_pngfile(const char* outFilename, const char* inString, uint8_t SizeMultiplier);


/* Advanced API functions for operating on already formed raw transactions
--------------------------------------------------------------------------
*/

/*Sign a raw transaction hexadecimal string using inputindex, scripthex, sighashtype and privkey. */
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey);

/*Store a raw transaction that's already formed, and give it a txindex in memory. (txindex) is returned as int. */
int store_raw_transaction(char* incomingrawtx);
