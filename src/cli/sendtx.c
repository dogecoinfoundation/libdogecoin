/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

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
//MLUMIN:MSVC
#ifndef _MSC_VER
#include <getopt.h>
#else
#include <../contrib/getopt/wingetopt.h>
#endif

#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <dogecoin/winunistd.h>
#endif

#ifdef WITH_NET
#include <event2/event.h>
#include <dogecoin/net.h>
#include <dogecoin/protocol.h>
#endif

#include <dogecoin/chainparams.h>
#include <dogecoin/ecc.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tool.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

static struct option long_options[] = {
        {"testnet", no_argument, NULL, 't'},
        {"regtest", no_argument, NULL, 'r'},
        {"ips", no_argument, NULL, 'i'},
        {"debug", no_argument, NULL, 'd'},
        {"timeout", no_argument, NULL, 's'},
        {"maxnodes", no_argument, NULL, 'm'},
        {NULL, 0, NULL, 0} };

static void print_version() {
    printf("Version: %s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    }

static void print_usage() {
    print_version();
    printf("Usage: sendtx (-i|-ips <ip,ip,...]>) (-m[--maxpeers] <int>) (-t[--testnet]) (-r[--regtest]) (-d[--debug]) (-s[--timeout] <secs>) <txhex>\n");
    printf("\nExamples: \n");
    printf("Send a TX to random peers on testnet:\n");
    printf("> sendtx --testnet <txhex>\n\n");
    printf("Send a TX to specific peers on mainnet:\n");
    printf("> sendtx -i 127.0.0.1:22556,192.168.0.1:22556 <txhex>\n\n");
    }

static bool showError(const char* er) {
    printf("Error: %s\n", er);
    return 1;
    }

int main(int argc, char* argv[]) {
    int ret = 0;
    int long_index = 0;
    int opt = 0;
    char* data = 0;
    char* ips = 0;
    int debug = 0;
    int timeout = 15;
    int maxnodes = 10;
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    if (argc <= 1 || strlen(argv[argc - 1]) == 0 || argv[argc - 1][0] == '-') {
        /* exit if no command was provided */
        print_usage();
        exit(EXIT_FAILURE);
        }
    data = argv[argc - 1];

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "i:trds:m:", long_options, &long_index)) != -1) {
        switch (opt) {
                case 't':
                    chain = &dogecoin_chainparams_test;
                    break;
                case 'r':
                    chain = &dogecoin_chainparams_regtest;
                    break;
                case 'd':
                    debug = 1;
                    break;
                case 's':
                    timeout = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'i':
                    ips = optarg;
                    break;
                case 'm':
                    maxnodes = (int)strtol(optarg, (char**)NULL, 10);
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

    /* The above code is checking if the data is NULL, empty or larger than the maximum
    size of a p2p message. */
    if (data == NULL || strlen(data) == 0 || strlen(data) > DOGECOIN_MAX_P2P_MSG_SIZE) {
        return showError("Transaction in invalid or to large.\n");
        }
    uint8_t* data_bin = dogecoin_malloc(strlen(data) / 2 + 1);
    size_t outlen = 0;
    utils_hex_to_bin(data, data_bin, strlen(data), &outlen);

    dogecoin_tx* tx = dogecoin_tx_new();
    /* Deserializing the transaction and broadcasting it to the network. */
    if (dogecoin_tx_deserialize(data_bin, outlen, tx, NULL)) {
        broadcast_tx(chain, tx, ips, maxnodes, timeout, debug);
        }
    else {
        showError("Transaction is invalid\n");
        ret = 1;
        }
    dogecoin_free(data_bin);
    dogecoin_tx_free(tx);

    return ret;
    }
