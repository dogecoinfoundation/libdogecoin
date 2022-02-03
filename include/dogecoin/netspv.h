/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_NETSPV_H__
#define __LIBDOGECOIN_NETSPV_H__

#include "dogecoin.h"
#include "blockchain.h"
#include "headersdb.h"
#include "tx.h"
#include "net.h"

LIBDOGECOIN_BEGIN_DECL

enum SPV_CLIENT_STATE {
    SPV_HEADER_SYNC_FLAG        = (1 << 0),
    SPV_FULLBLOCK_SYNC_FLAG	    = (1 << 1),
};

typedef struct dogecoin_spv_client_
{
    dogecoin_node_group *nodegroup;
    uint64_t last_headersrequest_time;
    uint64_t oldest_item_of_interest; /* oldest key birthday (or similar) */
    dogecoin_bool use_checkpoints; /* if false, the client will create a headers chain starting from genesis */
    const dogecoin_chainparams *chainparams;
    int stateflags;
    uint64_t last_statecheck_time;
    dogecoin_bool called_sync_completed;

    void *headers_db_ctx; /* flexible headers db context */
    const dogecoin_headers_db_interface *headers_db; /* headers db interface */

    /* callbacks */
    /* ========= */

    /* callback when a block(header) was connected */
    void (*header_connected)(struct dogecoin_spv_client_ *client);

    /* callback called when we have reached the ~chaintip
       will be called only once */
    void (*sync_completed)(struct dogecoin_spv_client_ *client);

    /* callback when the header message has been processed */
    /* return false will abort further logic (like continue loading headers, etc.) */
    dogecoin_bool (*header_message_processed)(struct dogecoin_spv_client_ *client, dogecoin_node *node, dogecoin_blockindex *newtip);

    /* callback, executed on each transaction (when getting a block, merkle-block txns or inv txns) */
    void (*sync_transaction)(void *ctx, dogecoin_tx *tx, unsigned int pos, dogecoin_blockindex *blockindex);
    void *sync_transaction_ctx;
} dogecoin_spv_client;


LIBDOGECOIN_API dogecoin_spv_client* dogecoin_spv_client_new(const dogecoin_chainparams *params, dogecoin_bool debug, dogecoin_bool headers_memonly);
LIBDOGECOIN_API void dogecoin_spv_client_free(dogecoin_spv_client *client);

/* load the eventually existing headers db */
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_load(dogecoin_spv_client *client, const char *file_path);

/* discover peers or set peers by IP(s) (CSV) */
LIBDOGECOIN_API void dogecoin_spv_client_discover_peers(dogecoin_spv_client *client, const char *ips);

/* start the spv client main run loop */
LIBDOGECOIN_API void dogecoin_spv_client_runloop(dogecoin_spv_client *client);

/* try to request headers from a single node in the nodegroup */
LIBDOGECOIN_API dogecoin_bool dogecoin_net_spv_request_headers(dogecoin_spv_client *client);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_NETSPV_H__
