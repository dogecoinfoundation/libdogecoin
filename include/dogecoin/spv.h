/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_SPV_H__
#define __LIBDOGECOIN_SPV_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/headersdb.h>
#include <dogecoin/net.h>
#include <dogecoin/tx.h>

#define SPV_STATS_RING 4096

LIBDOGECOIN_BEGIN_DECL

enum SPV_CLIENT_STATE {
    SPV_HEADER_SYNC_FLAG        = (1 << 0),
    SPV_FULLBLOCK_SYNC_FLAG     = (1 << 1),
    SPV_CFILTER_SYNC_FLAG       = (1 << 2),
};

typedef struct spv_block_sample_
{
    uint32_t ts;        // block timestamp
    uint32_t txs;       // number of transactions
    uint32_t outputs;   // number of outputs
    uint64_t out_value; // total output value
    uint32_t size;      // block size
    uint64_t fees;      // total fees
} spv_block_sample;

#define PAR_HDR_RAW_LEN 80

/* How many times a segment may fail to connect at flush before the parallel
 * downloader gives up entirely and lets the sequential path take over. Each
 * retry re-requests the segment from whatever the primary DB actually reached,
 * usually from a different peer. */
#define PAR_HDR_MAX_FLUSH_FAILS 3

/* One parallel header-download segment.  Each segment spans the open-closed
 * height interval (start_height, stop_height] and is assigned to one node. */
typedef struct par_hdr_seg_ {
    uint32_t  start_height;   /* height of start_hash (exclusive lower bound) */
    uint256_t start_hash;     /* block hash at start_height — getheaders locator */
    uint32_t  stop_height;    /* height of stop_hash  (inclusive upper bound)  */
    uint256_t stop_hash;      /* block hash at stop_height — getheaders hash_stop */

    /* download progress */
    int       node_id;        /* assigned node (-1 = unassigned)               */
    int       shadow_id;      /* second node racing this segment (-1 = none)   */
    uint64_t  shadow_at;      /* time the shadow was attached                  */
    uint64_t  requested_at;   /* time of the last getheaders sent for this seg */
    uint64_t  assigned_at;    /* time the current owner took this segment      */
    uint32_t  count_at_assign;/* headers already buffered when it took it      */
    uint32_t  tip_height;     /* highest header received so far in this segment */
    uint256_t tip_hash;       /* hash of that header (next-batch locator)       */

    /* buffered raw 80-byte block headers in ascending height order */
    uint8_t  *buf;
    uint32_t  count;          /* headers buffered */
    uint32_t  cap;            /* buffer capacity  */

    /* The segment's final header, retained whole: the standard 80 bytes plus
     * the AuxPoW blob when the version carries bit 0x100. buf holds only the
     * 80-byte prefix of each header, which is all connect_hdr needs, but it is
     * not enough to verify an AuxPoW proof -- and every mainnet segment above
     * height 371337 is merge-mined. Kept for the last header of the segment
     * only, so this costs one blob per segment rather than one per header. */
    uint8_t  *tail_raw;
    uint32_t  tail_len;

    dogecoin_bool complete;   /* all stop_height - start_height headers received */
    dogecoin_bool flushed;    /* segment has been flushed into the primary DB    */
    uint32_t  flush_fails;    /* times this segment failed to connect at flush   */
} par_hdr_seg;

/* Top-level state for a parallel genesis header download. */
typedef struct par_hdr_state_ {
    par_hdr_seg  *segs;             /* ordered array of segments                */
    uint32_t      num_segs;         /* total segment count                      */
    uint32_t      flush_idx;        /* index of the next segment pending flush  */
    uint32_t      last_flush_idx;   /* flush_idx at the last observed progress  */
    uint64_t      last_progress_time; /* time of that progress                  */
    uint64_t      buffered_bytes;   /* raw header bytes staged across segments  */
    uint32_t      rate_ref;         /* last median peer rate seen while >=3 segs
                                     * were in flight; the tail has no crowd to
                                     * compare against, so it compares to this */
    dogecoin_bool active;           /* download in progress                     */
} par_hdr_state;

typedef struct dogecoin_spv_client_
{
    dogecoin_node_group *nodegroup;
    uint64_t last_headersrequest_time;
    uint64_t oldest_item_of_interest;
    dogecoin_bool use_checkpoints;
    struct par_hdr_state_ *par_hdr;  /* parallel genesis header sync state */
    const dogecoin_chainparams *chainparams;
    int stateflags;
    uint64_t last_statecheck_time;
    dogecoin_bool called_sync_completed;
    void *headers_db_ctx;
    const dogecoin_headers_db_interface *headers_db;
    uint64_t last_block_size;
    uint64_t last_block_tx_count;
    uint64_t last_block_total_tx_size;
    spv_block_sample stats_ring[SPV_STATS_RING];
    int stats_ring_len;
    int stats_ring_head;
    uint64_t stats_blocks_total;
    uint64_t stats_txs_total;
    uint64_t stats_outputs_total;
    uint64_t stats_out_value_total;
    uint64_t stats_fees_total;
    uint64_t stats_block_bytes_total;
    uint64_t start_ts;

    void* smpv_ctx;
    dogecoin_bool smpv_enabled;

    /* BIP37 bloom filter (optional). When set, getdata requests are rewritten
       to request FILTERED_BLOCK so peers answer with merkleblock + matched tx. */
    uint8_t* bloom_filter;
    uint32_t bloom_filter_len;
    uint32_t bloom_nhashfunc;
    uint32_t bloom_ntweak;
    uint8_t  bloom_flags;
    char*    bloom_filter_debug_dump;

   /* merkleblock -> matched tx state
      stored as a btree keyed by txid so tx lookup is O(log n) */
   void*      merkle_match_tree;
   uint32_t   merkle_match_pending;
    dogecoin_bool merkle_match_active;
    dogecoin_blockindex* merkle_match_blockindex;

    /* historical rescan progress counters */
    uint64_t rescan_total;      /* total merkle blocks received during rescan */
    uint64_t rescan_matched;    /* merkle blocks with at least one matched tx */
    int32_t  filtered_history_last_end_height; /* highest historical height already requested via getdata(FILTERED_BLOCK) */
    uint8_t  filtered_history_tail_rerequest_count; /* bounded historical tail re-requests after new matches to catch spends */
    uint256_t filtered_history_last_rerequest_txid; /* dedupe repeated tail re-requests for the same matched tx */
    int32_t  filtered_history_last_rerequest_height;

    /* Comma-separated "host:port" strings supplied by the caller via
     * dogecoin_spv_client_discover_peers().  When non-NULL, the reconnect
     * callback reuses these peers instead of querying DNS seeds, so the
     * client stays connected to the explicitly-named hosts only. */
    char *peer_ips;

    /* BIP157 genesis-filter support: optional read-only secondary headers DB
     * used only for block-hash lookup at early heights when the primary DB is
     * checkpoint-based.  Populated via --filter-hash-db CLI option. */
    void                                *aux_hash_db_ctx; /**< Context for aux block-hash lookup DB */
    const dogecoin_headers_db_interface *aux_hash_db;     /**< Interface for aux block-hash lookup DB */

    /* When non-zero, overrides the cfheaders start height (auto default:
     * chainbottom for checkpoint-based sync, 1 for genesis sync). */
    uint32_t cf_start_height;

    /* Number of parallel getcfilters workers (independent TCP connections).
     * 0 or 1 means sequential (default); >1 activates parallel mode. */
    uint8_t cf_num_workers;

    /* BIP157: compact filter sync state */
    dogecoin_bool compact_filters_enabled; /**< Whether compact filter sync is active */
    dogecoin_compact_filter_state *cfilter_state; /**< BIP157 per-client compact filter state */
    struct dogecoin_cfheaders_db_ *cfheaders_db;  /**< Persistent storage for BIP157 filter headers */
    struct dogecoin_cfilters_db_  *cfilters_db;   /**< Persistent storage for BIP157 filter data */
    char *cfheaders_path;  /**< Override path for cfheaders.dat (NULL = default datadir) */
    char *cfilters_path;   /**< Override path for cfilters.dat (NULL = default datadir) */

    /* BIP158 filter header computation during full block sync */
    uint256_t cf_prev_filter_header;   /**< Running filter header chain for BIP158 */
    uint32_t  cf_computed_height;      /**< Last height for which a filter header was computed */
    dogecoin_bool cf_export_enabled;   /**< Log filter header checkpoints during sync */

    /* callbacks */
    /* ========= */
    void (*header_connected)(struct dogecoin_spv_client_ *client);
    void (*sync_completed)(struct dogecoin_spv_client_ *client);
    dogecoin_bool (*header_message_processed)(struct dogecoin_spv_client_ *client, dogecoin_node *node, dogecoin_blockindex *newtip);
    void (*sync_transaction)(void *ctx, dogecoin_tx *tx, unsigned int pos, dogecoin_blockindex *blockindex);
    void *sync_transaction_ctx;

    /* Per-client pending PQC OP_RETURN commitments awaiting carrier TX_R match.
       Opaque pointer (spv_pqc_pending_commit_t* internally); NULL when no PQC
       backends are compiled in. Owned by the client and freed on
       dogecoin_spv_client_free so multiple clients do not share state. */
    void* pqc_pending_commits;

    /* Per-client pending ZK OP_RETURN commitments awaiting carrier TX_R match.
       Opaque pointer (spv_zk_pending_commit_t* internally); NULL when the ZK
       carrier module is not compiled in. Owned by the client and freed on
       dogecoin_spv_client_free so multiple clients do not share state and so
       a long-running node cannot accumulate unbounded entries from cheap
       OP_RETURN spam (capped + LRU-evicted by spv_zk_add_pending). */
    void* zk_pending_commits;
} dogecoin_spv_client;

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_enable_genesis_headers(dogecoin_spv_client *client);

LIBDOGECOIN_API dogecoin_spv_client* dogecoin_spv_client_new(const dogecoin_chainparams *params, dogecoin_bool debug, dogecoin_bool headers_memonly, dogecoin_bool use_checkpoints, dogecoin_bool full_sync, int maxnodes, const char *http_server);
LIBDOGECOIN_API void dogecoin_spv_client_free(dogecoin_spv_client *client);
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_load(dogecoin_spv_client *client, const char *file_path, dogecoin_bool prompt);
LIBDOGECOIN_API void dogecoin_spv_client_discover_peers(dogecoin_spv_client *client, const char *ips);
LIBDOGECOIN_API void dogecoin_spv_client_runloop(dogecoin_spv_client *client);
LIBDOGECOIN_API dogecoin_bool dogecoin_net_spv_request_headers(dogecoin_spv_client *client);
LIBDOGECOIN_API void dogecoin_net_spv_node_request_headers_or_blocks(dogecoin_node *node, dogecoin_bool blocks);

LIBDOGECOIN_API void dogecoin_spv_enable_smpv(dogecoin_spv_client* client, dogecoin_bool enable);
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_handle_mempool_tx_hex(dogecoin_spv_client* client, const char* raw_tx_hex);
LIBDOGECOIN_API void dogecoin_spv_get_smpv_stats(dogecoin_spv_client* client, uint32_t* total_txs, uint32_t* watched_addrs);
LIBDOGECOIN_API void dogecoin_net_spv_request_mempool(dogecoin_spv_client *client);
LIBDOGECOIN_API void dogecoin_net_spv_request_filtered_history(dogecoin_spv_client *client, int depth);

/* BIP37: caller supplies a bloom filter payload (as built elsewhere).
   Insert script-relevant data (e.g. pubkey hashes, script bytes, outpoints),
   not base58 address strings. txids/outpoints are only useful when known. */
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filterload(
    dogecoin_spv_client* client,
    const uint8_t* filter,
    uint32_t filter_len,
    uint32_t nHashFuncs,
    uint32_t nTweak,
    uint8_t flags);

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filteradd(
    dogecoin_spv_client* client,
    const uint8_t* data,
    uint32_t data_len);

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filterclear(dogecoin_spv_client* client);

/* BIP157: enable or disable compact filter sync for this client */
LIBDOGECOIN_API void dogecoin_spv_enable_compact_filters(dogecoin_spv_client *client, dogecoin_bool enable);

/* BIP157: rescan all cached cfilters in cfilters.dat against watched scripts.
 * Call after dogecoin_spv_client_filteradd and before dogecoin_spv_client_runloop
 * to check locally-stored filters immediately without waiting for cfheaders sync. */
LIBDOGECOIN_API void dogecoin_spv_client_rescan_cached_filters(dogecoin_spv_client *client);


/* BIP157: send getcfcheckpt, getcfheaders, getcfilters to a peer */
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfcheckpt(dogecoin_spv_client *client, dogecoin_node *node);
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfheaders(dogecoin_spv_client *client, dogecoin_node *node, uint32_t start_height, const uint256_t stop_hash);
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfilters(dogecoin_spv_client *client, dogecoin_node *node, uint32_t start_height, const uint256_t stop_hash);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SPV_H__
