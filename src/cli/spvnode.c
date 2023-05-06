/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
 Copyright (c) 2023 bluezr
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include <dogecoin/chainparams.h>
#include <dogecoin/constants.h>
#include <dogecoin/base58.h>
#include <dogecoin/bip39.h>
#include <dogecoin/ecc.h>
#include <dogecoin/koinu.h>
#include <dogecoin/net.h>
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
        {"dbfile", no_argument, NULL, 'f'},
        {"continuous", no_argument, NULL, 'c'},
        {"address", no_argument, NULL, 'a'},
        {"full_sync", no_argument, NULL, 'b'},
        {"checkpoint", no_argument, NULL, 'p'},
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
    printf("Usage: spvnode (-c|continuous) (-i|-ips <ip,ip,...]>) (-m[--maxpeers] <int>) (-t[--testnet]) (-f <headersfile|0 for in mem only>) (-r[--regtest]) (-d[--debug]) (-s[--timeout] <secs>) <command>\n");
    printf("Supported commands:\n");
    printf("        scan      (scan blocks up to the tip, creates header.db file)\n");
    printf("\nExamples: \n");
    printf("Sync up to the chain tip and stores all headers in headers.db (quit once synced):\n");
    printf("> spvnode scan\n\n");
    printf("Sync up to the chain tip and give some debug output during that process:\n");
    printf("> spvnode -d scan\n\n");
    printf("Sync up, show debug info, don't store headers in file (only in memory), wait for new blocks:\n");
    printf("> spvnode -d -f 0 -c scan\n\n");
    }

/**
 * Prints an error message to the screen
 *
 * @param er The error message to display.
 *
 * @return Nothing.
 */
static bool showError(const char* er) {
    printf("Error: %s\n", er);
    return 1;
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

dogecoin_wallet* dogecoin_wallet_init(const dogecoin_chainparams* chain, char* address, char* mnemonic_in) {
    dogecoin_wallet* wallet = dogecoin_wallet_new(chain);
    int error;
    dogecoin_bool created;
    // prefix chain to wallet file name:
    char* wallet_suffix = "_wallet.db";
    char* wallet_prefix = (char*)chain->chainname;
    char* walletfile = concat(wallet_prefix, wallet_suffix);
    dogecoin_bool res = dogecoin_wallet_load(wallet, walletfile, &error, &created);
    dogecoin_free(walletfile);
    if (!res) {
        showError("Loading wallet failed\n");
        exit(EXIT_FAILURE);
        }
    if (created) {
        // create a new key
        dogecoin_hdnode node;
#ifdef WITH_UNISTRING
        SEED seed;
#else
        uint8_t seed[64];    
#endif
        if (mnemonic_in) {
            // generate seed from mnemonic
            dogecoin_seed_from_mnemonic(mnemonic_in, NULL, seed);
        } else {
            res = dogecoin_random_bytes(seed, sizeof(seed), true);
            if (!res) {
                showError("Generating random bytes failed\n");
                exit(EXIT_FAILURE);
            }
        }
        dogecoin_hdnode_from_seed(seed, sizeof(seed), &node);
        dogecoin_wallet_set_master_key_copy(wallet, &node);
    } else {
        // ensure we have a key
        // TODO
    }

    dogecoin_wallet_addr* waddr;

    if (address != NULL) {
        char delim[] = " ";

        char *ptr = strtok(address, delim);

        while(ptr != NULL)
        {
            waddr = dogecoin_wallet_addr_new();
            if (!dogecoin_p2pkh_address_to_wallet_pubkeyhash(ptr, waddr, wallet)) {
                exit(EXIT_FAILURE);
            }
            ptr = strtok(NULL, delim);
        }
    } 
#ifdef USE_UNISTRING  
    else if (wallet->waddr_vector->len == 0) {
        int i=0;
        for(;i<20;i++) {
            waddr = dogecoin_wallet_next_bip44_addr(wallet);
        }
        char str[P2PKH_ADDR_STRINGLEN];
        dogecoin_p2pkh_addr_from_hash160(waddr->pubkeyhash, wallet->chain, str, P2PKH_ADDR_STRINGLEN);
        printf("Wallet addr: %s (child %d)\n", str, waddr->childindex);
    }
#else
    else if (wallet->waddr_vector->len == 0) {
        waddr = dogecoin_wallet_next_addr(wallet);
    }
#endif

    /* Creating a vector of addresses and storing them in the wallet. */
    vector* addrs = vector_new(1, free);
    dogecoin_wallet_get_addresses(wallet, addrs);
    unsigned int i;
    for (i = 0; i < addrs->len; i++) {
        char* addr = vector_idx(addrs, i);
        printf("Address: %s\n", addr);
        }
    vector_free(addrs, true);

    vector* unspent = vector_new(1, free);
    dogecoin_wallet_get_unspent(wallet, unspent);
    if (unspent->len) {
        char wallet_total[21];
        uint64_t wallet_total_u64 = 0;
        for (i = 0; i < unspent->len; i++) {
            dogecoin_utxo* utxo = vector_idx(unspent, i);
            printf("%s\n", "----------------------");
            printf("txid:           %s\n", utils_uint8_to_hex(utxo->txid, sizeof utxo->txid));
            printf("vout:           %d\n", utxo->vout);
            printf("address:        %s\n", utxo->address);
            printf("script_pubkey:  %s\n", utxo->script_pubkey);
            printf("amount:         %s\n", utxo->amount);
            debug_print("confirmations:  %d\n", utxo->confirmations);
            printf("spendable:      %d\n", utxo->spendable);
            printf("solvable:       %d\n", utxo->solvable);
            wallet_total_u64 += coins_to_koinu_str(utxo->amount);
        }
        koinu_to_coins_str(wallet_total_u64, wallet_total);
        printf("Balance: %s\n", wallet_total);
    }
    vector_free(unspent, true);
    return wallet;
}

int main(int argc, char* argv[]) {
    int ret = 0;
    int long_index = 0;
    int opt = 0;
    char* data = 0;
    char* ips = 0;
    dogecoin_bool debug = false;
    char* dbfile = 0;
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;
    char* address = NULL;
    dogecoin_bool use_checkpoint = false;
    char* mnemonic_in = 0;
    dogecoin_bool full_sync = false;
    dogecoin_bool have_decl_daemon = false;

    if (argc <= 1 || strlen(argv[argc - 1]) == 0 || argv[argc - 1][0] == '-') {
        /* exit if no command was provided */
        print_usage();
        exit(EXIT_FAILURE);
        }
    data = argv[argc - 1];

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "i:ctrds:m:n:f:a:bpz:", long_options, &long_index)) != -1) {
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
        dogecoin_spv_client* client = dogecoin_spv_client_new(chain, debug, (dbfile && (dbfile[0] == '0' || (strlen(dbfile) > 1 && dbfile[0] == 'n' && dbfile[0] == 'o'))) ? true : false, use_checkpoint, full_sync);
        client->header_message_processed = spv_header_message_processed;
        client->sync_completed = spv_sync_completed;

#if WITH_WALLET
        dogecoin_wallet* wallet = dogecoin_wallet_init(chain, address, mnemonic_in);
        client->sync_transaction = dogecoin_wallet_check_transaction;
        client->sync_transaction_ctx = wallet;
#endif
        char* header_suffix = "_headers.db";
        char* header_prefix = (char*)chain->chainname;
        char* headersfile = concat(header_prefix, header_suffix);

        dogecoin_bool response = dogecoin_spv_client_load(client, (dbfile ? dbfile : headersfile));
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
            ret = EXIT_SUCCESS;
#if WITH_WALLET
            dogecoin_wallet_free(wallet);
#endif
            }
        dogecoin_ecc_stop();
    } else if (strcmp(data, "wallet") == 0) {
#if WITH_WALLET
        if (address != NULL) {
            uint64_t amount = dogecoin_get_balance(address);
            if (amount > 0) {
                printf("amount:         %s\n", dogecoin_get_balance_str(address));
                unsigned int utxo_count = dogecoin_get_utxos_length(address);
                if (utxo_count) {
                    printf("utxo count:     %d\n", utxo_count);
                    unsigned int i = 1;
                    for (; i <= utxo_count; i++) {
                        printf("txid:           %s\n", dogecoin_get_utxo_txid_str(address, i));
                    }
                }
            }
        }
#endif
    } else {
        printf("Invalid command (use -?)\n");
        ret = EXIT_FAILURE;
        }
    return ret;
    }
