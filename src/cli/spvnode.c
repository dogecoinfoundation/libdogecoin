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

#include <assert.h>

#ifndef _MSC_VER
#include <getopt.h>
#else
#include <../../contrib/getopt/wingetopt.h>
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include <dogecoin/chainparams.h>
#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/net.h>
#include <dogecoin/spv.h>
#include <dogecoin/protocol.h>
#include <dogecoin/random.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tool.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

/* This is a list of all the options that can be used with the program. */
static struct option long_options[] = {
        {"testnet", no_argument, NULL, 't'},
        {"regtest", no_argument, NULL, 'r'},
        {"ips", no_argument, NULL, 'i'},
        {"debug", no_argument, NULL, 'd'},
        {"maxnodes", no_argument, NULL, 'm'},
        {"dbfile", no_argument, NULL, 'f'},
        {"continuous", no_argument, NULL, 'c'},
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
    UNUSED(client);
    UNUSED(node);
    if (newtip) {
        printf("New headers tip height %d\n", newtip->height);
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

int main(int argc, char* argv[]) {
    int ret = 0;
    int long_index = 0;
    int opt = 0;
    char* data = 0;
    char* ips = 0;
    dogecoin_bool debug = false;
    int timeout = 15;
    int maxnodes = 10;
    char* dbfile = 0;
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    if (argc <= 1 || strlen(argv[argc - 1]) == 0 || argv[argc - 1][0] == '-') {
        /* exit if no command was provided */
        print_usage();
        exit(EXIT_FAILURE);
    }
    data = argv[argc - 1];

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "i:ctrds:m:f:", long_options, &long_index)) != -1) {
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
                case 's':
                    timeout = (int)strtol(optarg, (char**)NULL, 10); // value stored is never read
                    break;
                case 'i':
                    ips = optarg;
                    break;
                case 'm':
                    maxnodes = (int)strtol(optarg, (char**)NULL, 10); // value stored is never read
                    break;
                case 'f':
                    dbfile = optarg;
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
        dogecoin_spv_client* client = dogecoin_spv_client_new(chain, debug, (dbfile && (dbfile[0] == '0' || (strlen(dbfile) > 1 && dbfile[0] == 'n' && dbfile[0] == 'o'))) ? true : false);
        client->header_message_processed = spv_header_message_processed;
        client->sync_completed = spv_sync_completed;
        if (!dogecoin_spv_client_load(client, (dbfile ? dbfile : "headers.db"))) {
            printf("Could not load or create headers database...aborting\n");
            ret = EXIT_FAILURE;
        } else {
            printf("Discover peers...");
            dogecoin_spv_client_discover_peers(client, ips);
            printf("done\n");
            printf("Connecting to the p2p network...\n");
            dogecoin_spv_client_runloop(client);
            dogecoin_spv_client_free(client);
            ret = EXIT_SUCCESS;
        }
        dogecoin_ecc_stop();
        } else {
            printf("Invalid command (use -?)\n");
            ret = EXIT_FAILURE;
        }
    return ret;
}
