/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
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

#ifdef _MSC_VER
#include <stdio.h>
#else
#include <unistd.h>
#endif

#include <test/utest.h>

#include <dogecoin/block.h>
#include <dogecoin/net.h>
#include <dogecoin/spv.h>
#include <dogecoin/utils.h>

void test_spv_sync_completed(dogecoin_spv_client* client) {
    printf("Sync completed, at height %d\n", client->headers_db->getchaintip(client->headers_db_ctx)->height);
    dogecoin_node_group_shutdown(client->nodegroup);
}

dogecoin_bool test_spv_header_message_processed(struct dogecoin_spv_client_ *client, dogecoin_node *node, dogecoin_blockindex *newtip) {
    UNUSED(client);
    UNUSED(node);
    if (newtip) {
        printf("New headers tip height %d\n", newtip->height);
    }
    return true;
}

void test_spv()
{
    // set chain:
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    // concatenate chain to prefix of headers database:
    char* header_suffix = "_headers.db";
    char* header_prefix = (char*)chain->chainname;
    char* headersfile = concat(header_prefix, header_suffix);

    // unlink newly prefixed headers database:
    unlink(headersfile);

    // init new spv client with debugging off and syncing to memory:
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, true, false);
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile);
    dogecoin_free(headersfile);

    printf("Discover peers...");
    dogecoin_spv_client_discover_peers(client, NULL);
    printf("done\n");
    printf("Start interacting with the p2p network...\n");
    dogecoin_spv_client_runloop(client);
    dogecoin_spv_client_free(client);
}
