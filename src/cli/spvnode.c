/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
 Copyright (c) 2023 bluezr
 Copyright (c) 2023-2024 The Dogecoin Foundation

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

#ifndef WIN32
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <assert.h>
#endif

#ifndef _MSC_VER
#include <getopt.h>
#include <unistd.h>
#else
#include <win/wingetopt.h>
#endif

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <event2/buffer.h>
#include <event2/http.h>

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include <dogecoin/chainparams.h>
#include <dogecoin/constants.h>
#include <dogecoin/base58.h>
#include <dogecoin/bip39.h>
#include <dogecoin/ecc.h>
#include <dogecoin/headersdb_file.h>
#include <dogecoin/koinu.h>
#include <dogecoin/net.h>
#include <dogecoin/seal.h>
#include <dogecoin/spv.h>
#include <dogecoin/protocol.h>
#include <dogecoin/random.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tool.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>
#include <dogecoin/wallet.h>

#ifndef WIN32
#define BD_NO_CHDIR          01 /* Don't chdir ("/") */
#define BD_NO_CLOSE_FILES    02 /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS 04 /* Don't reopen stdin, stdout, and stderr
                                   to /dev/null */
#define BD_NO_UMASK0        010 /* Don't do a umask(0) */
#define BD_MAX_CLOSE       8192 /* Max file descriptors to close if
                                   sysconf(_SC_OPEN_MAX) is indeterminate */

int // returns 0 on success -1 on error
become_daemon(int flags)
{
  int maxfd, fd;

  /* The first fork will change our pid
   * but the sid and pgid will be the
   * calling process.
   */
  switch(fork())                    // become background process
  {
    case -1: return -1;
    case 0: break;                  // child falls through
    default: _exit(EXIT_SUCCESS);   // parent terminates
  }

  /*
   * Run the process in a new session without a controlling
   * terminal. The process group ID will be the process ID
   * and thus, the process will be the process group leader.
   * After this call the process will be in a new session,
   * and it will be the progress group leader in a new
   * process group.
   */
  if(setsid() == -1)                // become leader of new session
    return -1;

  /*
   * We will fork again, also known as a
   * double fork. This second fork will orphan
   * our process because the parent will exit.
   * When the parent process exits the child
   * process will be adopted by the init process
   * with process ID 1.
   * The result of this second fork is a process
   * with the parent as the init process with an ID
   * of 1. The process will be in it's own session
   * and process group and will have no controlling
   * terminal. Furthermore, the process will not
   * be the process group leader and thus, cannot
   * have the controlling terminal if there was one.
   */
  switch(fork())
  {
    case -1: return -1;
    case 0: break;                  // child breaks out of case
    default: _exit(EXIT_SUCCESS);   // parent process will exit
  }

  if(!(flags & BD_NO_UMASK0))
    umask(0);                       // clear file creation mode mask

//   if(!(flags & BD_NO_CHDIR))
//     chdir("/");                     // change to root directory

  if(!(flags & BD_NO_CLOSE_FILES))  // close all open files
  {
    maxfd = sysconf(_SC_OPEN_MAX);
    if(maxfd == -1)
      maxfd = BD_MAX_CLOSE;         // if we don't know then guess
    for(fd = 0; fd < maxfd; fd++)
      close(fd);
  }

  if(!(flags & BD_NO_REOPEN_STD_FDS))
  {
    /* now time to go "dark"!
     * we'll close stdin
     * then we'll point stdout and stderr
     * to /dev/null
     */
    close(STDIN_FILENO);

    fd = open("/dev/null", O_RDWR);
    if(fd != STDIN_FILENO)
      return -1;
    if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
      return -2;
    if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
      return -3;
  }

  return 0;
}
#endif

/* This is a list of all the options that can be used with the program. */
static struct option long_options[] = {
        {"testnet", no_argument, NULL, 't'},
        {"regtest", no_argument, NULL, 'r'},
        {"ips", no_argument, NULL, 'i'},
        {"debug", no_argument, NULL, 'd'},
        {"maxnodes", no_argument, NULL, 'm'},
        {"mnemonic", no_argument, NULL, 'n'},
        {"pass_phrase", no_argument, NULL, 's'},
        {"dbfile", no_argument, NULL, 'f'},
        {"continuous", no_argument, NULL, 'c'},
        {"address", no_argument, NULL, 'a'},
        {"full_sync", no_argument, NULL, 'b'},
        {"checkpoint", no_argument, NULL, 'p'},
        {"wallet_file", required_argument, NULL, 'w'},
        {"headers_file", required_argument, NULL, 'h'},
        {"no_prompt", no_argument, NULL, 'l'},
        {"encrypted_file", required_argument, NULL, 'y'},
        {"use_tpm", no_argument, NULL, 'j'},
        {"master_key", no_argument, NULL, 'k'},
        {"http_server", required_argument, NULL, 'u'},
        {"daemon", no_argument, NULL, 'z'},
        {NULL, 0, NULL, 0} };

/**
 * Print_version() prints the version of the program
 */
static void print_version() {
    printf("Version: %s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    }

/**
 * This function prints the usage of the spvnode command
 */
static void print_usage() {
    print_version();
    printf("Usage: spvnode (-c|continuous) (-i|--ips <ip,ip,...>) (-m[--maxpeers] <int>) (-f <headersfile|0 for in mem only>) \
(-a|--address <address>) (-n|--mnemonic <seed_phrase>) (-s|[--pass_phrase]) (-y|--encrypted_file <file_num 0-999>) \
(-w|--wallet_file <filename>) (-h|--headers_file <filename>) (-l|[--no_prompt]) (-b[--full_sync]) (-p[--checkpoint]) (-k[--master_key]) (-j[--use_tpm]) \
(-u|--http_server <ip:port>) (-t[--testnet]) (-r[--regtest]) (-d[--debug]) <command>\n");
    printf("Supported commands:\n");
    printf("        scan      (scan blocks up to the tip, creates header.db file)\n");
    printf("\nExamples: \n");
    printf("Sync up to the chain tip and stores all headers in headers.db (quit once synced):\n");
    printf("> ./spvnode scan\n\n");
    printf("Sync up to the chain tip and give some debug output during that process:\n");
    printf("> ./spvnode -d scan\n\n");
    printf("Sync up, show debug info, don't store headers in file (only in memory), wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -b scan\n\n");
    printf("Sync up, with an address, show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -a \"DSVw8wkkTXccdq78etZ3UwELrmpfvAiVt1\" -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -w \"./main_wallet.db\" -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", show debug info, with a headers file \"main_headers.db\", wait for new blocks:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -h \"./main_headers.db\" -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with an address, show debug info, with a headers file, with a headers file \"main_headers.db\", wait for new blocks:\n");
    printf("> ./spvnode -d -c -a \"DSVw8wkkTXccdq78etZ3UwELrmpfvAiVt1\" -w \"./main_wallet.db\" -h \"./main_headers.db\" -b scan\n\n");
    printf("Sync up, with encrypted mnemonic 0, show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -y 0 -b scan\n\n");
    printf("Sync up, with encrypted mnemonic 0, BIP39 passphrase, show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -y 0 -s -b scan\n\n");
    printf("Sync up, with encrypted mnemonic 0, BIP39 passphrase, show debug info, don't store headers in file, wait for new blocks, use TPM:\n");
    printf("> ./spvnode -d -f 0 -c -y 0 -s -j -b scan\n\n");
    printf("Sync up, with encrypted key 0, show debug info, don't store headers in file, wait for new blocks, use master key:\n");
    printf("> ./spvnode -d -f 0 -c -y 0 -k -b scan\n\n");
    printf("Sync up, with encrypted key 0, show debug info, don't store headers in file, wait for new blocks, use master key, use TPM:\n");
    printf("> ./spvnode -d -f 0 -c -y 0 -k -j -b scan\n\n");
    printf("Sync up, with mnemonic \"test\", BIP39 passphrase, show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -n \"test\" -s -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with encrypted mnemonic 0, show debug info, don't store headers in file, wait for new blocks:\n");
    printf("> ./spvnode -d -f 0 -c -w \"./main_wallet.db\" -y 0 -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with encrypted mnemonic 0, show debug info, with a headers file \"main_headers.db\", wait for new blocks:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -h \"./main_headers.db\" -y 0 -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with encrypted mnemonic 0, show debug info, with a headers file \"main_headers.db\", wait for new blocks, use TPM:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -h \"./main_headers.db\" -y 0 -j -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with encrypted mnemonic 0, show debug info, with a headers file \"main_headers.db\", wait for new blocks, use master key:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -h \"./main_headers.db\" -y 0 -k -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", with encrypted mnemonic 0, show debug info, with a headers file \"main_headers.db\", wait for new blocks, use master key, use TPM:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -h \"./main_headers.db\" -y 0 -k -j -b scan\n\n");
    printf("Sync up, with a wallet file \"main_wallet.db\", show debug info, wait for new blocks, enable http server:\n");
    printf("> ./spvnode -d -c -w \"./main_wallet.db\" -u \"0.0.0.0:8080\" -b scan\n\n");
    }


/**
 * When a new block is added to the blockchain, this function is called
 *
 * @param client The client object.
 * @param node The node that sent the message.
 * @param newtip The new tip of the headers chain.
 *
 * @return A boolean value.
 */
dogecoin_bool spv_header_message_processed(struct dogecoin_spv_client_* client, dogecoin_node* node, dogecoin_blockindex* newtip) {
    UNUSED(node);
    if (newtip) {
        time_t timestamp = client->headers_db->getchaintip(client->headers_db_ctx)->header.timestamp;
        printf("New headers tip height %d from %s\n", newtip->height, ctime(&timestamp));
        }
    return true;
    }

static dogecoin_bool quit_when_synced = true;
/**
 * When the sync is complete, print a message and either exit or wait for new blocks or relevant
 * transactions
 *
 * @param client The client object.
 */
void spv_sync_completed(dogecoin_spv_client* client) {
    printf("Sync completed, at height %d\n", client->headers_db->getchaintip(client->headers_db_ctx)->height);
    if (quit_when_synced) {
        dogecoin_node_group_shutdown(client->nodegroup);
    } else {
        printf("Waiting for new blocks or relevant transactions...\n");
    }
}

// Signal handler for SIGINT
void handle_sigint() {
    // Reset standard input back to blocking mode
#ifndef _WIN32
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags & ~O_NONBLOCK);
#endif
    exit(0);
}

/**
 * This function is called when an http request is received
 * It handles the request and sends a response
 *
 * @param req the request
 * @param arg the client
 *
 * @return Nothing.
 */

void dogecoin_http_request_cb(struct evhttp_request *req, void *arg) {
    dogecoin_spv_client* client = (dogecoin_spv_client*)arg;
    dogecoin_wallet* wallet = (dogecoin_wallet*)client->sync_transaction_ctx;
    if (!wallet) {
        evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
        return;
    }

    const struct evhttp_uri* uri = evhttp_request_get_evhttp_uri(req);
    const char* path = evhttp_uri_get_path(uri);

    struct evbuffer *evb = NULL;
    evb = evbuffer_new();
    if (!evb) {
        evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
        return;
    }

    if (strcmp(path, "/getBalance") == 0) {
        int64_t balance = dogecoin_wallet_get_balance(wallet);
        char balance_str[32] = {0};
        koinu_to_coins_str(balance, balance_str);
        evbuffer_add_printf(evb, "Wallet balance: %s\n", balance_str);
    } else if (strcmp(path, "/getAddresses") == 0) {
        vector* addresses = vector_new(10, dogecoin_free);
        dogecoin_wallet_get_addresses(wallet, addresses);
        for (unsigned int i = 0; i < addresses->len; i++) {
            char* address = vector_idx(addresses, i);
            evbuffer_add_printf(evb, "address: %s\n", address);
        }
        vector_free(addresses, true);
    } else if (strcmp(path, "/getTransactions") == 0) {
        char wallet_total[21];
        dogecoin_mem_zero(wallet_total, 21);
        uint64_t wallet_total_u64 = 0;

        if (HASH_COUNT(utxos) > 0) {
            dogecoin_utxo* utxo;
            dogecoin_utxo* tmp;
            HASH_ITER(hh, utxos, utxo, tmp) {
                if (!utxo->spendable) {
                    // For spent UTXOs
                    evbuffer_add_printf(evb, "%s\n", "----------------------");
                    evbuffer_add_printf(evb, "txid:           %s\n", utils_uint8_to_hex(utxo->txid, sizeof utxo->txid));
                    evbuffer_add_printf(evb, "vout:           %d\n", utxo->vout);
                    evbuffer_add_printf(evb, "address:        %s\n", utxo->address);
                    evbuffer_add_printf(evb, "script_pubkey:  %s\n", utxo->script_pubkey);
                    evbuffer_add_printf(evb, "amount:         %s\n", utxo->amount);
                    evbuffer_add_printf(evb, "confirmations:  %d\n", utxo->confirmations);
                    evbuffer_add_printf(evb, "spendable:      %d\n", utxo->spendable);
                    evbuffer_add_printf(evb, "solvable:       %d\n", utxo->solvable);
                    wallet_total_u64 += coins_to_koinu_str(utxo->amount);
                }
            }
        }

        // Convert and print totals for spent UTXOs.
        koinu_to_coins_str(wallet_total_u64, wallet_total);
        evbuffer_add_printf(evb, "Spent Balance: %s\n", wallet_total);
    } else if (strcmp(path, "/getUTXOs") == 0) {
        char wallet_total[21];
        dogecoin_mem_zero(wallet_total, 21);
        uint64_t wallet_total_u64_unspent = 0, wallet_total_u64_spent = 0;

        dogecoin_utxo* utxo;
        dogecoin_utxo* tmp;

        HASH_ITER(hh, wallet->utxos, utxo, tmp) {
            if (utxo->spendable) {
                // For unspent UTXOs
                evbuffer_add_printf(evb, "----------------------\n");
                evbuffer_add_printf(evb, "Unspent UTXO:\n");
                evbuffer_add_printf(evb, "txid:           %s\n", utils_uint8_to_hex(utxo->txid, sizeof(utxo->txid)));
                evbuffer_add_printf(evb, "vout:           %d\n", utxo->vout);
                evbuffer_add_printf(evb, "address:        %s\n", utxo->address);
                evbuffer_add_printf(evb, "script_pubkey:  %s\n", utxo->script_pubkey);
                evbuffer_add_printf(evb, "amount:         %s\n", utxo->amount);
                evbuffer_add_printf(evb, "spendable:      %d\n", utxo->spendable);
                evbuffer_add_printf(evb, "solvable:       %d\n", utxo->solvable);
                wallet_total_u64_unspent += coins_to_koinu_str(utxo->amount);
            }
        }

        // Convert and print totals for unspent UTXOs.
        koinu_to_coins_str(wallet_total_u64_unspent, wallet_total);
        evbuffer_add_printf(evb, "Total Unspent: %s\n", wallet_total);
    } else if (strcmp(path, "/getWallet") == 0) {
        // Get the wallet file
        FILE* file = wallet->dbfile;
        if (file == NULL) {
            evhttp_send_error(req, HTTP_NOTFOUND, "Wallet file not found");
            evbuffer_free(evb);
            return;
        }

        // Get the size of the wallet file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        // Read the wallet file into a buffer
        char* buffer = malloc(file_size);
        if (buffer == NULL) {
            fclose(file);
            evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
            evbuffer_free(evb);
            return;
        }
        fread(buffer, 1, file_size, file);

        // Add the buffer to the response buffer
        evbuffer_add(evb, buffer, file_size);

        // Set the Content-Type header to "application/octet-stream" for binary data
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");

        // Clean up
        free(buffer);
    } else if (strcmp(path, "/getHeaders") == 0) {
        // Get the headers file
        dogecoin_headers_db* headers_db = (dogecoin_headers_db *)(client->headers_db_ctx);
        FILE* file = headers_db->headers_tree_file;
        if (file == NULL) {
            evhttp_send_error(req, HTTP_NOTFOUND, "Headers file not found");
            evbuffer_free(evb);
            return;
        }

        // Get the size of the headers file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        // Read the headers file into a buffer
        char* buffer = malloc(file_size);
        if (buffer == NULL) {
            fclose(file);
            evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
            evbuffer_free(evb);
            return;
        }
        fread(buffer, 1, file_size, file);

        // Add the buffer to the response buffer
        evbuffer_add(evb, buffer, file_size);

        // Set the Content-Type header to "application/octet-stream" for binary data
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");

        // Clean up
        free(buffer);
    } else if (strcmp(path, "/getChaintip") == 0) {
        dogecoin_blockindex* tip = client->headers_db->getchaintip(client->headers_db_ctx);
        evbuffer_add_printf(evb, "Chain tip: %d\n", tip->height);
    } else {
        evhttp_send_error(req, HTTP_NOTFOUND, "Not Found");
        evbuffer_free(evb);
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");
    evhttp_send_reply(req, HTTP_OK, "OK", evb);
    evbuffer_free(evb);
}

int main(int argc, char* argv[]) {
    int ret = 0;
    int long_index = 0;
    int opt = 0;
    char* data = 0;
    char* ips = 0;
    dogecoin_bool debug = false;
    int maxnodes = 10;
    char* dbfile = 0;
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;
    char* address = NULL;
    dogecoin_bool use_checkpoint = false;
    char* pass = 0;
    char* mnemonic_in = 0;
    char* name = 0;
    char* headers_name = 0;
    dogecoin_bool full_sync = false;
    dogecoin_bool have_decl_daemon = false;
    dogecoin_bool prompt = true;
    dogecoin_bool encrypted = false;
    dogecoin_bool master_key = false;
    dogecoin_bool tpm = false;
    char* http_server = NULL;
    int file_num = NO_FILE;

    if (argc <= 1 || strlen(argv[argc - 1]) == 0 || argv[argc - 1][0] == '-') {
        /* exit if no command was provided */
        print_usage();
        exit(EXIT_FAILURE);
        }
    data = argv[argc - 1];

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "i:ctrdsm:n:f:y:u:w:h:a:lbpzkj:", long_options, &long_index)) != -1) {
        switch (opt) {
                case 'c':
                    quit_when_synced = false;
                    break;
                case 't':
                    chain = &dogecoin_chainparams_test;
                    break;
                case 'r':
                    chain = &dogecoin_chainparams_regtest;
                    break;
                case 'd':
                    debug = true;
                    break;
                case 'i':
                    ips = optarg;
                    break;
                case 's':
                    pass = getpass("BIP39 passphrase: \n");
                    break;
                case 'm':
                    maxnodes = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'n':
                    mnemonic_in = optarg;
                    break;
                case 'f':
                    dbfile = optarg;
                    break;
                case 'a':
                    address = optarg;
                    break;
                case 'b':
                    full_sync = true;
                    break;
                case 'p':
                    use_checkpoint = true;
                    break;
                case 'h':
                    headers_name = optarg;
                    break;
                case 'l':
                    prompt = false;
                    break;
                case 'y':
                    encrypted = true;
                    file_num = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'k':
                    master_key = true;
                    break;
                case 'j':
                    tpm = true;
                    break;
                case 'w':
                    name = optarg;
                    break;
                case 'u':
                    http_server = optarg;
                    break;
                case 'z':
                    have_decl_daemon = true;
                    break;
                case 'v':
                    print_version();
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    print_usage();
                    exit(EXIT_FAILURE);
            }
        }

    if (strcmp(data, "scan") == 0) {
        dogecoin_ecc_start();
        dogecoin_spv_client* client = dogecoin_spv_client_new(chain, debug, (dbfile && (dbfile[0] == '0' || (strlen(dbfile) > 1 && dbfile[0] == 'n' && dbfile[0] == 'o'))) ? true : false, use_checkpoint, full_sync, maxnodes, http_server);
        if (http_server) {
            evhttp_set_gencb(client->nodegroup->http_server, dogecoin_http_request_cb, client);
        }
        client->header_message_processed = spv_header_message_processed;
        client->sync_completed = spv_sync_completed;
        signal(SIGINT, handle_sigint);

#if WITH_WALLET
        dogecoin_wallet* wallet = dogecoin_wallet_init(chain, address, name, mnemonic_in, pass, encrypted, tpm, file_num, master_key, prompt);
        if (!wallet) {
            printf("Could not initialize wallet...\n");
            // clear and free the passphrase
            if (pass) {
                dogecoin_mem_zero (pass, strlen(pass));
                dogecoin_free(pass);
                }
            dogecoin_spv_client_free(client);
            dogecoin_ecc_stop();
            return EXIT_FAILURE;
        }
        // clear and free the passphrase
        if (pass) {
            dogecoin_mem_zero (pass, strlen(pass));
            dogecoin_free(pass);
            }
        print_utxos(wallet);
        client->sync_transaction = dogecoin_wallet_check_transaction;
        client->sync_transaction_ctx = wallet;
#endif
        char* header_suffix = "_headers.db";
        char* header_prefix = (char*)chain->chainname;
        char* headersfile = NULL;
        dogecoin_bool response = false;
        if (mnemonic_in) {
            // mnemonic was provided, so store headers in separate file
            char* wallet_type = "_mnemonic";
            char* header_type_prefix = concat(header_prefix, wallet_type);
            headersfile = concat(header_type_prefix, header_suffix);
            dogecoin_free(header_type_prefix);
            if (headers_name) {
                // Load headers file name with headers name:
                response = dogecoin_spv_client_load(client, (dbfile ? dbfile : headers_name), prompt);
            } else {
                // Otherwise, use default headers file name:
                response = dogecoin_spv_client_load(client, (dbfile ? dbfile : headersfile), prompt);
            }
        }
        else if (headers_name) {
            // Load headers file name with headers name:
            response = dogecoin_spv_client_load(client, (dbfile ? dbfile : headers_name), prompt);
        } else {
            // Otherwise, use default headers file name:
            headersfile = concat(header_prefix, header_suffix);
            response = dogecoin_spv_client_load(client, (dbfile ? dbfile : headersfile), prompt);
        }

        dogecoin_free(headersfile);
        if (!response) {
            printf("Could not load or create headers database...aborting\n");
            ret = EXIT_FAILURE;
        } else {
            if (have_decl_daemon) {
#if defined(HAVE_DECL_DAEMON) && !defined(WIN32)
                const char *LOGNAME = "libdogecoin-spvnode";

                // turn this process into a daemon
                ret = become_daemon(0);
                if(ret)
                {
                    syslog(LOG_USER | LOG_ERR, "error starting");
                    closelog();
                    return EXIT_FAILURE;
                }

                // we are now a daemon!
                // printf now will go to /dev/null

                // open up the system log
                openlog(LOGNAME, LOG_PID, LOG_USER);
                syslog(LOG_USER | LOG_INFO, "starting");

                // run forever in the background
                while(1)
                {
                    sleep(60);
                    syslog(LOG_USER | LOG_INFO, "running");
                }
#else
            fprintf(stderr, "Error: -z | --daemon is not supported on this operating system\n");
            return false;
#endif
            }
            printf("done\n");
            printf("Discover peers...\n");
            dogecoin_spv_client_discover_peers(client, ips);
            printf("Connecting to the p2p network...\n");
            dogecoin_spv_client_runloop(client);
            dogecoin_spv_client_free(client);
            printf("done\n");
            ret = EXIT_SUCCESS;
#if WITH_WALLET
            dogecoin_wallet_free(wallet);
#endif
            }
        dogecoin_ecc_stop();
    } else if (strcmp(data, "sanity") == 0) {
#if WITH_WALLET
    dogecoin_ecc_start();
    if (address != NULL) {
        char delim[] = " ";
        // copy address into a new string, strtok modifies the string
        char* address_copy = strdup(address);

        // backup existing default wallet file prior to radio doge functions test
        const dogecoin_chainparams *params = chain_from_b58_prefix(address_copy);
        dogecoin_wallet *tmp = dogecoin_wallet_new(params);
        int result;
        FILE *file;
        if ((file = fopen(tmp->filename, "r")))
        {
            fclose(file);
#ifdef WIN32
            #include <winbase.h>
            result = CopyFile((char*)tmp->filename, "tmp.bin", true);
            if (result == 1) result = 0;
#else
            result = file_copy((char *)tmp->filename, "tmp.bin");
#endif
            if (result != 0) {
                printf( "could not copy '%s' %d\n", tmp->filename, result );
            } else {
                printf( "File '%s' copied to 'tmp.bin'\n", tmp->filename);
            }
        }

        char *ptr;
        char* temp_address_copy = address_copy;

        while((ptr = strtok_r(temp_address_copy, delim, &temp_address_copy))) {
            int res = dogecoin_register_watch_address_with_node(ptr);
            printf("registered:     %d %s\n", res, ptr);
            uint64_t amount = dogecoin_get_balance(ptr);
            if (amount > 0) {
                char* amount_str = dogecoin_get_balance_str(ptr);
                printf("total:          %s\n", amount_str);
                unsigned int utxo_count = dogecoin_get_utxos_length(ptr);
                if (utxo_count) {
                    printf("utxo count:     %d\n", utxo_count);
                    unsigned int i = 1;
                    for (; i <= utxo_count; i++) {
                        printf("txid:           %s\n", dogecoin_get_utxo_txid_str(ptr, i));
                        printf("vout:           %d\n", dogecoin_get_utxo_vout(ptr, i));
                        char* utxo_amount_str = dogecoin_get_utxo_amount(ptr, i);
                        printf("amount:         %s\n", utxo_amount_str);
                        dogecoin_free(utxo_amount_str);
                    }
                }
                dogecoin_free(amount_str);
            }
            res = dogecoin_unregister_watch_address_with_node(ptr);
            printf("unregistered:   %s\n", res ? "true" : "false");
        }

        if ((file = fopen("tmp.bin", "r"))) {
            fclose(file);
#ifdef WIN32
            #include <winbase.h>
            char *tmp_filename = _strdup((char *)tmp->filename);
            char *filename = _strdup((char *)tmp->filename);
            replace_last_after_delim(filename, "\\", "tmp.bin");
            LPVOID message;
            result = DeleteFile(tmp->filename);
            if (!result) {
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);
                printf("ERROR: %s\n", (char *)message);
            }
            result = rename(filename, tmp->filename);
            dogecoin_free(filename);
            dogecoin_free(tmp_filename);
#else
            result = rename("tmp.bin", tmp->filename);
#endif
            if( result != 0 ) {
                printf( "could not copy 'tmp.bin' %d\n", result );
            } else {
                printf( "File 'tmp.bin' copied to '%s'\n", tmp->filename);
            }
        }
        dogecoin_wallet_free(tmp);
        dogecoin_free(address_copy);
    }

    dogecoin_ecc_stop();
#endif
    } else {
        printf("Invalid command (use -?)\n");
        ret = EXIT_FAILURE;
        }
    return ret;
    }
