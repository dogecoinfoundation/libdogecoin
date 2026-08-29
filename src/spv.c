/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
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

#ifdef _WIN32
#include <inttypes.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#else
#include <getopt.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include <dogecoin/block.h>
#include <dogecoin/bip37.h>
#include <dogecoin/portable_endian.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/golomb.h>
#include <dogecoin/headersdb.h>
#include <dogecoin/cfheadersdb_file.h>
#include <dogecoin/headersdb_file.h>
#include <dogecoin/net.h>
#include <dogecoin/pow.h>
#include <dogecoin/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/spv.h>
#include <dogecoin/smpv.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>
#include <dogecoin/vector.h>
#include <dogecoin/pqc_carrier.h>
#include <dogecoin/pqc_dilithium.h>
#include <dogecoin/pqc_falcon.h>
#include <dogecoin/rmd160.h>
#ifdef USE_RACCOON_G
#include <dogecoin/pqc_raccoon.h>
#endif
#ifdef USE_ZK_CARRIER
#include <dogecoin/zk_carrier.h>
#endif
#include <event2/event.h>

/* Optional liboqs (Falcon-only) presence check; compile with -DUSE_LIBOQS */
#ifdef USE_LIBOQS
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <oqs/sig.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

#define DOGECOIN_KOINU_PER_COIN 100000000ULL
/* Dogecoin block subsidy has been 10,000 DOGE for years; good for 24h stats */
#define DOGECOIN_CURRENT_SUBSIDY_KOINU (10000ULL * DOGECOIN_KOINU_PER_COIN)
#define SPV_VARINT_MAX_LEN 9 /* compactSize max bytes; we reserve max for simple one-pass allocation */
#define SPV_FILTERLOAD_FIXED_FIELDS_LEN 9 /* 4-byte nHashFuncs + 4-byte nTweak + 1-byte flags */
#define SPV_TXID_HEX_LEN 65 /* 32-byte hash => 64 hex chars + NUL */
#define SPV_FILTER_HISTORY_MAX_REQUEST_PEERS 5
/* Build and send filterload directly from SPV/network context. */
static dogecoin_bool spv_send_filterload_to_node(dogecoin_node* node,
                                                 const uint8_t* filter,
                                                 uint32_t filter_len,
                                                 uint32_t nHashFuncs,
                                                 uint32_t nTweak,
                                                 uint8_t flags)
{
    if (!node || !filter || filter_len == 0) return false;

    cstring* payload = cstr_new_sz((size_t)filter_len + SPV_VARINT_MAX_LEN + SPV_FILTERLOAD_FIXED_FIELDS_LEN);
    if (!payload) return false;

    ser_varlen(payload, filter_len);
    ser_bytes(payload, filter, filter_len);
    ser_u32(payload, nHashFuncs);
    ser_u32(payload, nTweak);
    ser_bytes(payload, &flags, 1);

    cstring* msg = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_FILTERLOAD,
        (const uint8_t*)payload->str,
        payload->len
    );
    dogecoin_node_send(node, msg);
    cstr_free(msg, true);
    cstr_free(payload, true);
    return true;
}

static uint32_t spv_elapsed(const dogecoin_spv_client *client) {
    return (uint32_t)((uint64_t)time(NULL) - client->start_ts);
}

/* ================================================================ */
/* Parallel cfheaders download helpers                               */
/* ================================================================ */

static dogecoin_bool spv_cf_par_assign(dogecoin_spv_client *client, dogecoin_node *node);
static uint32_t cf_find_checkpoint_stop(const dogecoin_chainparams *params, uint32_t target_height, uint256_t hash_out);

/* Send one GETCFHEADERS batch for a chunk, clamped to batch_max.
 * Fills stop_hash from headers DB / cfcheckpt fallback. */
static void cfh_par_send_batch(dogecoin_spv_client *client, dogecoin_node *node,
                                cfh_par_chunk *ch, dogecoin_blockindex *tip_bi)
{
    uint32_t start = ch->req_next;
    uint32_t end   = start + MAX_GETCFHEADERS_SIZE - 1;
    if (end > ch->end) end = ch->end;

    uint256_t stop_hash;
    if (tip_bi && end == (uint32_t)tip_bi->height) {
        memcpy(stop_hash, tip_bi->hash, 32);
    } else {
        dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
        if (!dogecoin_headers_db_get_block_hash_at_height(hdb, end, stop_hash)) {
            dogecoin_bool aux_found = false;
            if (client->aux_hash_db && client->aux_hash_db_ctx) {
                dogecoin_headers_db *aux = (dogecoin_headers_db *)client->aux_hash_db_ctx;
                aux_found = dogecoin_headers_db_get_block_hash_at_height(aux, end, stop_hash);
            }
            if (!aux_found && tip_bi)
                memcpy(stop_hash, tip_bi->hash, 32);
        }
    }

    dogecoin_getcfheaders_msg msg;
    msg.filter_type  = GCS_BASIC_FILTER_TYPE;
    msg.start_height = start;
    memcpy(msg.stop_hash, stop_hash, 32);

    cstring *payload = cstr_new_sz(64);
    dogecoin_p2p_msg_getcfheaders_ser(&msg, payload);
    cstring *p2pmsg  = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_GETCFHEADERS, payload->str, payload->len);
    cstr_free(payload, true);
    dogecoin_node_send(node, p2pmsg);
    cstr_free(p2pmsg, true);

    ch->req_next = end + 1;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157-cfh-par] node %d: getcfheaders [%u..%u] [%us elapsed]\n",
            node->nodeid, start, end, spv_elapsed(client));
}

/* Assign the next unassigned cfheaders chunk to @node. */
static dogecoin_bool cfh_par_assign(dogecoin_spv_client *client, dogecoin_node *node)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    if (!cfstate || cfstate->cfh_par_n == 0) return false;

    dogecoin_blockindex *tip_bi =
        client->headers_db->getchaintip(client->headers_db_ctx);

    uint8_t wi;
    for (wi = 0; wi < cfstate->cfh_par_n; wi++) {
        cfh_par_chunk *ch = &cfstate->cfh_par_chunks[wi];
        if (ch->node_id != -1) continue;  /* already assigned or no-work */
        ch->node_id = node->nodeid;
        cfh_par_send_batch(client, node, ch, tip_bi);
        return true;
    }
    return false;
}

/* Initialise parallel cfheaders download and assign to available CF nodes. */
static void cfh_par_init(dogecoin_spv_client *client,
                          dogecoin_blockindex *tip_bi,
                          uint32_t cfh_start)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    uint8_t n_workers = client->cf_num_workers;
    uint32_t tip      = (uint32_t)tip_bi->height;

    /* Every chunk after the first anchors on the cfcheckpt filter header at
     * (start - 1), so start - 1 has to land exactly on a CFCHECKPT_INTERVAL
     * boundary. Chunk size is already a multiple of the interval, so that holds
     * for every chunk iff it holds for the first: snap cfh_start down to the
     * nearest k*CFCHECKPT_INTERVAL + 1.
     *
     * Without this, a resume start such as 6314590 gives chunk 1 a start of
     * 6315590, and (6315590 - 1) / 1000 = 6315 selects the checkpoint for
     * height 6315000 -- an anchor 589 blocks off. Every header the chunk
     * derives is then wrong, surfacing as a checkpoint failure at the next
     * boundary rather than at the point the anchor was chosen. A genesis sync
     * starts at 1 and satisfies the precondition by accident, which is why this
     * only appears when resuming. */
    if (cfh_start > 1) {
        uint32_t aligned = ((cfh_start - 1) / CFCHECKPT_INTERVAL) * CFCHECKPT_INTERVAL + 1;
        if (aligned < 1) aligned = 1;
        cfh_start = aligned;
    }

    /* Chunk size rounded up to CFCHECKPT_INTERVAL so boundaries align with
     * cfcheckpt anchors, enabling independent per-chunk validation. */
    uint32_t total     = tip - cfh_start + 1;
    uint32_t raw_chunk = (total + n_workers - 1) / n_workers;
    uint32_t chunk_sz  = ((raw_chunk + CFCHECKPT_INTERVAL - 1) / CFCHECKPT_INTERVAL)
                         * CFCHECKPT_INTERVAL;
    if (chunk_sz < CFCHECKPT_INTERVAL) chunk_sz = CFCHECKPT_INTERVAL;

    cfstate->cfh_par_n      = n_workers;
    cfstate->cfh_par_done   = 0;
    cfstate->cfh_par_base   = cfh_start;
    cfstate->cfh_par_total  = total;
    cfstate->cfh_par_data   = (uint8_t *)dogecoin_calloc(total, 32);
    cfstate->cfh_par_chunks = (cfh_par_chunk *)dogecoin_calloc(n_workers, sizeof(cfh_par_chunk));
    if (!cfstate->cfh_par_data || !cfstate->cfh_par_chunks) {
        /* OOM — fall back to sequential */
        dogecoin_free(cfstate->cfh_par_data);   cfstate->cfh_par_data   = NULL;
        dogecoin_free(cfstate->cfh_par_chunks); cfstate->cfh_par_chunks = NULL;
        cfstate->cfh_par_n = 0;
        return;
    }

    uint8_t wi;
    uint8_t effective_n = 0;
    for (wi = 0; wi < n_workers; wi++) {
        cfh_par_chunk *ch = &cfstate->cfh_par_chunks[wi];
        uint32_t start = cfh_start + (uint32_t)wi * chunk_sz;
        if (start > tip) {
            ch->node_id = -2;  /* no work for this slot */
            cfstate->cfh_par_done++;
            continue;
        }
        /* Settle the anchor before committing to the chunk. Every chunk after
         * the first is validated independently, so it needs the compiled-in
         * checkpoint covering the height before its start. Past the last
         * compiled-in checkpoint there is none, and a chunk anchored on zero
         * produces a header chain that is wrong from its first entry: the
         * cfilter check then fails at the seam. Hand the tail to the last
         * anchored chunk instead, which chains its own batches, so it stays
         * verified and only loses parallelism. */
        uint8_t anchor[32];
        dogecoin_bool anchored = false;
        if (wi == 0) {
            /* Genesis anchor comes from the first CFHEADERS response's
             * prev_filter_header field; initialise to zero for now. */
            dogecoin_mem_zero(anchor, sizeof(anchor));
            anchored = true;
        } else {
            uint256_t cp;
            if (dogecoin_cf_hardcoded_checkpoint_at(client->chainparams, start - 1, cp)) {
                memcpy(anchor, cp, 32);
                anchored = true;
            }
        }
        if (!anchored) {
            if (effective_n > 0)
                cfstate->cfh_par_chunks[effective_n - 1].end = tip;
            ch->node_id = -2;  /* no work for this slot */
            cfstate->cfh_par_done++;
            continue;
        }

        ch->start    = start;
        ch->end      = start + chunk_sz - 1;
        if (ch->end > tip) ch->end = tip;
        ch->req_next = ch->start;
        ch->node_id  = -1;  /* unassigned */
        ch->n_received = 0;
        ch->complete = false;
        memcpy(ch->prev_fh, anchor, 32);
        effective_n++;
    }

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157-cfh-par] starting parallel cfheaders: %u workers, "
            "chunk_sz=%u, heights %u..%u [%us elapsed]\n",
            (unsigned int)effective_n, chunk_sz, cfh_start, tip,
            spv_elapsed(client));

    /* Assign one chunk to each connected CF node */
    unsigned int ni;
    for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
        dogecoin_node *wn = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
        if (!wn || !(wn->state & NODE_CONNECTED) || !wn->version_handshake) continue;
        if (!(wn->services & DOGECOIN_NODE_COMPACT_FILTERS)) continue;
        cfh_par_assign(client, wn);
    }

    cfstate->awaiting_response = false;
}

/* Callback context for spv_rescan_cached_cfilters. */
typedef struct {
    dogecoin_spv_client *client;
    uint32_t             up_to_height; /* only rescan heights < this */
    uint32_t             scanned;
    uint32_t             matched;
} rescan_ctx;

/* Iterator callback: match one cached cfilter against watched_scripts. */
static dogecoin_bool spv_rescan_cb(uint32_t height, const uint256_t block_hash,
                                    const uint8_t *filter_data, uint32_t data_len,
                                    void *ctx_)
{
    rescan_ctx *ctx = (rescan_ctx *)ctx_;
    if (height >= ctx->up_to_height) return false; /* stop; network takes over here */

    dogecoin_compact_filter_state *cfstate = ctx->client->cfilter_state;
    if (!cfstate->watched_scripts || cfstate->watched_scripts->len == 0) return true;

    ctx->scanned++;

    /* The block_hash stored in cfilters.dat may be corrupted by an old deser bug
     * (filter_type byte prepended, last hash byte zeroed).  The GCS SipHash key is
     * derived directly from the block hash, so a wrong hash produces wrong match
     * results.  Look up the authoritative hash from the headers DB BEFORE
     * deserializing the filter so that the correct key is used for matching.
     * Use the _seq variant which never restores the file pointer, keeping sequential
     * ascending-order lookups O(N) rather than O(N²). */
    uint256_t correct_hash;
    dogecoin_bool hash_ok = false;
    if (ctx->client->headers_db && ctx->client->headers_db_ctx) {
        dogecoin_headers_db *hdb = (dogecoin_headers_db *)ctx->client->headers_db_ctx;
        hash_ok = dogecoin_headers_db_get_block_hash_at_height_seq(hdb, height, correct_hash);
    }
    if (!hash_ok && ctx->client->aux_hash_db && ctx->client->aux_hash_db_ctx) {
        dogecoin_headers_db *aux = (dogecoin_headers_db *)ctx->client->aux_hash_db_ctx;
        hash_ok = dogecoin_headers_db_get_block_hash_at_height_seq(aux, height, correct_hash);
    }
    const uint8_t *hash_for_gcs = hash_ok ? (const uint8_t *)correct_hash
                                           : (const uint8_t *)block_hash;

    gcs_filter *gcs = gcs_filter_new();
    struct const_buffer fbuf = { filter_data, data_len };
    if (gcs_filter_deserialize(gcs, GCS_BASIC_FILTER_TYPE, hash_for_gcs, &fbuf)) {
        if (gcs_filter_match_any(gcs, cfstate->watched_scripts)) {
            if (ctx->client->nodegroup && ctx->client->nodegroup->log_write_cb)
                ctx->client->nodegroup->log_write_cb(
                    "[bip157] MATCH (rescan) at height %u\n", height);
            uint256_t *matched_hash = dogecoin_calloc(1, sizeof(uint256_t));
            memcpy(matched_hash, hash_for_gcs, sizeof(uint256_t));
            vector_add(cfstate->matched_block_hashes, matched_hash);
            uint32_t *matched_height = dogecoin_calloc(1, sizeof(uint32_t));
            *matched_height = height;
            vector_add(cfstate->matched_block_heights, matched_height);
            ctx->matched++;
        }
    }
    gcs_filter_free(gcs);
    return true;
}

/* Rescan cached cfilters (heights 1..cf_scan_start-1) against watched_scripts.
 * Called before starting the network download so that already-stored filters
 * are not skipped when scripts were registered after the previous sync. */
/* Does this cfheaders batch continue the chain we already hold?
 *
 * Nothing ties a cfheaders response to the request that asked for it, so with
 * two getcfheaders in flight -- which the CF response timeout causes by
 * re-sending getcfcheckpt without cancelling the first -- both replies get
 * appended and the same range lands twice. Anchoring on prev_filter_header also
 * stops a peer splicing its own chain on above the last compiled-in checkpoint,
 * where no other validation fires at all.
 *
 * An empty chain accepts anything: that batch establishes the anchor. */
LIBDOGECOIN_API dogecoin_bool dogecoin_cfheaders_batch_extends_tip(
    const dogecoin_compact_filter_state *cfstate, const uint8_t *prev_filter_header)
{
    if (!cfstate || !prev_filter_header) return false;
    if (!cfstate->filter_headers || cfstate->filter_headers->len == 0) return true;
    return memcmp(prev_filter_header, cfstate->cfheaders_tip_hash, 32) == 0;
}

/* Where a cfilter scan should pick up: the highest height already scanned this
   session or persisted in the store, else the caller's floor. Called every time
   cfheaders reach the chain tip, which on a live chain is once per block, so it
   has to move forward or the scan restarts for ever. */
static uint32_t spv_cf_resume_from(dogecoin_spv_client *client, uint32_t floor)
{
    dogecoin_compact_filter_state *cfstate = client ? client->cfilter_state : NULL;
    if (!cfstate) return floor;

    uint32_t scanned_to = cfstate->filters_tip_height;
    if (client->cfilters_db && client->cfilters_db->tip_height > scanned_to)
        scanned_to = client->cfilters_db->tip_height;

    if (scanned_to > 0 && floor <= scanned_to)
        return scanned_to + 1;
    return floor;
}

static void spv_rescan_cached_cfilters(dogecoin_spv_client *client, uint32_t cf_scan_start)
{
    if (!client->cfilters_db) return;
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    if (!cfstate || !cfstate->watched_scripts || cfstate->watched_scripts->len == 0) return;

    /* Skip partial rescan if a full startup rescan already covered everything. */
    if (cfstate->rescan_done) return;

    if (cf_scan_start <= 1) return;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] rescanning cached filters heights 1..%u against %u watched scripts\n",
            cf_scan_start - 1, (unsigned int)cfstate->watched_scripts->len);

    rescan_ctx ctx = { client, cf_scan_start, 0, 0 };
    dogecoin_cfilters_db_iterate(client->cfilters_db, spv_rescan_cb, &ctx);

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] rescan complete: %u filters checked, %u matched\n",
            ctx.scanned, ctx.matched);
}

/* Repair cfilters.dat records whose block_hash was written with an off-by-one
 * deserialization bug (old code read the 32-byte hash before the 1-byte
 * filter_type, so stored hashes contain the filter_type byte prepended and the
 * last hash byte replaced with 0x00).  The symptom: stored_hash[31] == 0x00.
 * We correct each such record in-place using the authoritative hash from the
 * headers DB.  Only records opened read-write are touched. */
static void spv_repair_cfilters_hashes(dogecoin_spv_client *client)
{
    dogecoin_cfilters_db *db = client->cfilters_db;
    if (!db || !db->file || !db->read_write) return;

    dogecoin_headers_db *hdb  = client->headers_db     ? (dogecoin_headers_db *)client->headers_db_ctx  : NULL;
    dogecoin_headers_db *aux  = client->aux_hash_db    ? (dogecoin_headers_db *)client->aux_hash_db_ctx : NULL;
    if (!hdb && !aux) return;

    if (fseek(db->file, CF_HEADERS_FILE_HDR_LEN, SEEK_SET) != 0) return;

    uint32_t repaired = 0;
    uint8_t hdr[CF_FILTERS_FILE_REC_HDR_LEN];
    while (fread(hdr, CF_FILTERS_FILE_REC_HDR_LEN, 1, db->file) == 1) {
        uint32_t height, data_len;
        memcpy(&height,   hdr,      4); height   = le32toh(height);
        memcpy(&data_len, hdr + 36, 4); data_len = le32toh(data_len);

        /* Quick corruption check: the old bug always left byte 31 of the hash
         * as 0x00 (zero-initialised, never written).  Real hashes end in 0x00
         * ~1/256 of the time so we'll do a few unnecessary lookups, but those
         * will match and no write will be issued. */
        if (hdr[35] == 0x00) {
            uint256_t correct;
            dogecoin_bool ok = hdb && dogecoin_headers_db_get_block_hash_at_height(hdb, height, correct);
            if (!ok && aux) ok = dogecoin_headers_db_get_block_hash_at_height(aux, height, correct);
            if (ok && memcmp(hdr + 4, correct, 32) != 0) {
                long rec_start = ftell(db->file) - (long)CF_FILTERS_FILE_REC_HDR_LEN;
                if (fseek(db->file, rec_start + 4, SEEK_SET) == 0) {
                    fwrite(correct, 32, 1, db->file);
                    fseek(db->file, rec_start + (long)CF_FILTERS_FILE_REC_HDR_LEN, SEEK_SET);
                    repaired++;
                }
            }
        }

        if (data_len > 0 && fseek(db->file, (long)data_len, SEEK_CUR) != 0) break;
    }

    fseek(db->file, 0, SEEK_END);

    if (repaired > 0 && client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] repaired %u cfilter block hashes (old deserialization bug)\n", repaired);
}

void dogecoin_spv_client_rescan_cached_filters(dogecoin_spv_client *client)
{
    if (!client || !client->cfilters_db) return;
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    if (!cfstate || !cfstate->watched_scripts || cfstate->watched_scripts->len == 0) return;

    uint32_t tip = client->cfilters_db->tip_height;
    if (tip == 0) return;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] startup rescan: checking all %u cached filters against %u watched scripts\n",
            tip, (unsigned int)cfstate->watched_scripts->len);

    rescan_ctx ctx = { client, UINT32_MAX, 0, 0 };
    dogecoin_cfilters_db_iterate(client->cfilters_db, spv_rescan_cb, &ctx);

    cfstate->rescan_done = true;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] startup rescan complete: %u filters checked, %u matched\n",
            ctx.scanned, ctx.matched);
}

/* Called when all cfheaders chunks are complete: populate filter_headers
 * from the flat array and transition to cfilter download. */
static void cfh_par_finish(dogecoin_spv_client *client, dogecoin_node *node)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    dogecoin_blockindex *tip_bi =
        client->headers_db->getchaintip(client->headers_db_ctx);

    cfstate->cfheaders_tip_height  = cfstate->cfh_par_base + cfstate->cfh_par_total - 1;
    cfstate->cfheaders_base_height = cfstate->cfh_par_base;
    memcpy(cfstate->cfheaders_tip_hash,
           cfstate->cfh_par_data + (cfstate->cfh_par_total - 1) * 32, 32);

    /* Persist filter headers to disk — single flush after the batch */
    if (client->cfheaders_db) {
        uint32_t h;
        for (h = 0; h < cfstate->cfh_par_total; h++) {
            dogecoin_cfheaders_db_write(client->cfheaders_db,
                cfstate->cfh_par_base + h,
                cfstate->cfh_par_data + h * 32);
        }
        dogecoin_cfheaders_db_flush(client->cfheaders_db);
    }

    /* Transfer cfh_par_data ownership to filter_headers_flat for O(1) cfilter lookups.
     * This avoids 6M+ individual calloc calls that would otherwise stall here. */
    if (cfstate->filter_headers_flat)
        dogecoin_free(cfstate->filter_headers_flat);
    cfstate->filter_headers_flat      = cfstate->cfh_par_data;
    cfstate->filter_headers_flat_base = cfstate->cfh_par_base;
    cfstate->filter_headers_flat_len  = cfstate->cfh_par_total;

    /* Free parallel cfheaders state (data pointer now owned by filter_headers_flat) */
    cfstate->cfh_par_data = NULL;
    dogecoin_free(cfstate->cfh_par_chunks); cfstate->cfh_par_chunks = NULL;
    cfstate->cfh_par_n = 0;
    cfstate->cfh_par_done = 0;
    cfstate->cfh_par_total = 0;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157-cfh-par] all cfheaders complete (tip=%u), starting cfilter scan [%us elapsed]\n",
            cfstate->cfheaders_tip_height, spv_elapsed(client));

    /* Start cfilter download */
    dogecoin_headers_db *hdb_cf = (dogecoin_headers_db *)client->headers_db_ctx;
    uint32_t cf_scan_start = 1;
    if (client->cf_start_height > 0) {
        /* --cf_from_genesis or API override: honour the requested start height */
        cf_scan_start = client->cf_start_height;
    } else if (hdb_cf && hdb_cf->chainbottom && hdb_cf->chainbottom->height > 0) {
        cf_scan_start = hdb_cf->chainbottom->height;
    }

    /* Clamp cf_scan_start so we only download cfilters we can validate.
     * filter_headers_flat covers cfheaders_base_height..cfheaders_tip_height;
     * heights below that were already scanned by the startup rescan. */
    if (cfstate->cfheaders_base_height > 0 &&
        cf_scan_start < cfstate->cfheaders_base_height)
        cf_scan_start = cfstate->cfheaders_base_height;

    /* Resume from actual progress, not the floor. See the note at the clamp in
       the cfheaders completion path. */
    cf_scan_start = spv_cf_resume_from(client, cf_scan_start);

    /* Rescan any cached filters (heights 1..cf_scan_start-1) that were stored
     * in a prior run before these watched scripts were registered. */
    spv_rescan_cached_cfilters(client, cf_scan_start);

    cfstate->cf_scan_start_height = cf_scan_start;
    if (client->cf_num_workers > 1) {
        cfstate->par_num_workers  = client->cf_num_workers;
        cfstate->par_next_height  = cf_scan_start;
        cfstate->par_flush_height = cf_scan_start;
        cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;
        if (!cfstate->par_bufs) {
            cfstate->par_bufs = (cf_par_buf *)dogecoin_calloc(
                cfstate->par_num_workers, sizeof(cf_par_buf));
            uint8_t pi;
            for (pi = 0; pi < cfstate->par_num_workers; pi++)
                cfstate->par_bufs[pi].node_id = -1;
        }
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[bip157-par] assigning %u parallel cfilter workers from height %u\n",
                (unsigned int)cfstate->par_num_workers, cf_scan_start);
        unsigned int ni;
        for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
            dogecoin_node *wn = (dogecoin_node *)vector_idx(
                client->nodegroup->nodes, ni);
            if (!wn || !(wn->state & NODE_CONNECTED) || !wn->version_handshake)
                continue;
            spv_cf_par_assign(client, wn);
        }
    } else {
        cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;
        dogecoin_spv_request_cfilters(client, node, cf_scan_start, tip_bi->hash);
    }
}

/* Handle a CFHEADERS message in parallel cfheaders download mode. */
static void cfh_par_handle_response(dogecoin_spv_client *client,
                                     dogecoin_node *node,
                                     dogecoin_cfheaders_msg *cfh_msg)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;

    /* Find the chunk assigned to this node */
    cfh_par_chunk *ch = NULL;
    uint8_t wi;
    for (wi = 0; wi < cfstate->cfh_par_n; wi++) {
        if (cfstate->cfh_par_chunks[wi].node_id == node->nodeid) {
            ch = &cfstate->cfh_par_chunks[wi];
            break;
        }
    }
    if (!ch) return;

    /* For chunk 0 (genesis anchor), capture genesis_filter_header from first batch */
    if (ch->n_received == 0 && ch->start == cfstate->cfh_par_base) {
        memcpy(cfstate->genesis_filter_header, cfh_msg->prev_filter_header, 32);
        memcpy(ch->prev_fh, cfh_msg->prev_filter_header, 32);
        if (client->cfheaders_db)
            dogecoin_cfheaders_db_write_genesis(client->cfheaders_db,
                                                cfh_msg->prev_filter_header);
    }

    /* A batch must continue the chunk it belongs to. Nothing ties a cfheaders
       response to the request that asked for it, so a duplicate or reordered
       reply is otherwise appended at ch->n_received and silently corrupts the
       chunk. The height > ch->end check below bounds the damage to one chunk,
       and the checkpoint anchor only fires where the compiled-in table has an
       entry, which is nowhere above 6,239,000 on mainnet. */
    if (ch->n_received > 0 &&
        memcmp(cfh_msg->prev_filter_header, ch->prev_fh, 32) != 0) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[bip157-cfh-par] cfheaders from node %d do not extend chunk %u at %u, dropping %u hashes\n",
                node->nodeid, (unsigned int)wi, ch->start + ch->n_received,
                (unsigned int)cfh_msg->filter_hashes->len);
        return;
    }

    uint8_t prev_fh[32];
    memcpy(prev_fh, ch->prev_fh, 32);

    dogecoin_bool valid = true;
    unsigned int i;
    for (i = 0; i < cfh_msg->filter_hashes->len; i++) {
        uint32_t height = ch->start + ch->n_received;
        if (height > ch->end) { valid = false; break; }

        uint256_t *filter_hash = (uint256_t *)vector_idx(cfh_msg->filter_hashes, i);

        uint8_t combined[64];
        memcpy(combined, filter_hash, 32);
        memcpy(combined + 32, prev_fh, 32);
        uint256_t new_header;
        dogecoin_hash(combined, 64, new_header);

        /* Validate against cfcheckpt at checkpoint boundaries */
        if (height > 0) {
            uint256_t checkpoint;
            if (dogecoin_cf_hardcoded_checkpoint_at(client->chainparams, height, checkpoint)) {
                if (memcmp(new_header, checkpoint, 32) != 0) {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157-cfh-par] cfheader at height %u FAILED checkpoint\n", height);
                    valid = false;
                    break;
                }
            }
        }

        /* Store in flat array */
        uint32_t idx = height - cfstate->cfh_par_base;
        memcpy(cfstate->cfh_par_data + idx * 32, new_header, 32);
        memcpy(prev_fh, new_header, 32);
        ch->n_received++;
    }

    if (!valid) {
        dogecoin_node_misbehave(node);
        ch->node_id = -1;  /* free slot for reassignment */
        return;
    }

    /* Update anchor for next batch */
    memcpy(ch->prev_fh, prev_fh, 32);

    dogecoin_blockindex *tip_bi = client->headers_db->getchaintip(client->headers_db_ctx);

    if (ch->req_next <= ch->end) {
        /* More batches needed for this chunk */
        cfh_par_send_batch(client, node, ch, tip_bi);
    } else {
        /* Chunk complete */
        ch->complete = true;
        cfstate->cfh_par_done++;

        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[bip157-cfh-par] node %d chunk %u..%u done (%u/%u workers) [%us elapsed]\n",
                node->nodeid, ch->start, ch->end,
                (unsigned int)cfstate->cfh_par_done,
                (unsigned int)cfstate->cfh_par_n,
                spv_elapsed(client));

        if (cfstate->cfh_par_done >= cfstate->cfh_par_n)
            cfh_par_finish(client, node);
        else
            cfh_par_assign(client, node);  /* reuse this node for the next unassigned chunk */
    }
}

/* ------------------------------------------------------------------ */
/* Assign the next getcfilters batch to a parallel worker node.        */
/* Returns true if a batch was assigned and getcfilters was sent.      */
/* ------------------------------------------------------------------ */
static dogecoin_bool spv_cf_par_assign(dogecoin_spv_client *client, dogecoin_node *node)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    uint32_t start = cfstate->par_next_height;
    uint32_t tip   = cfstate->cfheaders_tip_height;

    if (start > tip) return false;

    /* Find a free slot */
    int slot = -1;
    uint8_t pi;
    for (pi = 0; pi < cfstate->par_num_workers; pi++) {
        if (cfstate->par_bufs[pi].node_id == -1) { slot = (int)pi; break; }
    }
    if (slot < 0) return false;

    uint32_t end = start + MAX_GETCFILTERS_SIZE - 1;
    if (end > tip) end = tip;

    /* Resolve stop hash BEFORE allocating records so end can be adjusted. */
    uint256_t stop_hash;
    dogecoin_blockindex *tip_bi = client->headers_db->getchaintip(client->headers_db_ctx);
    if (tip_bi && end == (uint32_t)tip_bi->height) {
        memcpy(stop_hash, tip_bi->hash, 32);
    } else {
        dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
        if (!dogecoin_headers_db_get_block_hash_at_height(hdb, end, stop_hash)) {
            dogecoin_bool resolved = false;
            if (client->aux_hash_db && client->aux_hash_db_ctx) {
                dogecoin_headers_db *aux = (dogecoin_headers_db *)client->aux_hash_db_ctx;
                resolved = dogecoin_headers_db_get_block_hash_at_height(aux, end, stop_hash);
            }
            if (!resolved) {
                uint32_t cp_h = cf_find_checkpoint_stop(client->chainparams, end, stop_hash);
                if (cp_h > 0 && cp_h <= tip) {
                    end = cp_h;
                } else if (tip_bi) {
                    /* checkpoint beyond tip or not found — use tip */
                    memcpy(stop_hash, tip_bi->hash, 32);
                    end = (uint32_t)tip_bi->height;
                }
            }
        }
    }

    uint32_t count = end - start + 1;
    cf_par_buf *buf = &cfstate->par_bufs[slot];
    buf->node_id     = node->nodeid;
    buf->batch_start = start;
    buf->batch_end   = end;
    buf->received    = 0;
    buf->complete    = false;
    buf->assign_time = time(NULL);
    buf->records     = (cf_par_record *)dogecoin_calloc(count, sizeof(cf_par_record));
    if (!buf->records) { buf->node_id = -1; return false; }

    node->cf_batch_start = start;
    node->cf_batch_end   = end;
    node->cf_cur_height  = start;
    cfstate->par_next_height = end + 1;

    dogecoin_getcfilters_msg gcf_msg;
    gcf_msg.filter_type  = GCS_BASIC_FILTER_TYPE;
    gcf_msg.start_height = start;
    memcpy(gcf_msg.stop_hash, stop_hash, 32);

    cstring *payload = cstr_new_sz(64);
    dogecoin_p2p_msg_getcfilters_ser(&gcf_msg, payload);
    cstring *p2pmsg = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_GETCFILTERS,
        payload->str, payload->len);
    cstr_free(payload, true);
    dogecoin_node_send(node, p2pmsg);
    cstr_free(p2pmsg, true);

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157-par] node %d: getcfilters [%u..%u] [%us elapsed]\n",
            node->nodeid, start, end, spv_elapsed(client));
    return true;
}

/* ------------------------------------------------------------------ */
/* Request full blocks for all BIP157 matched block hashes via P2P.   */
/* ------------------------------------------------------------------ */
static void spv_cf_request_matched_blocks(dogecoin_spv_client *client)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    if (!cfstate || !cfstate->matched_block_hashes || cfstate->matched_block_hashes->len == 0)
        return;

    /* Collect all connected peers (prefer CF-capable, then any) */
    /* enum, not `const unsigned int`: in C a const object is not a constant
     * expression, so `peers[MAX_PEERS]` was a variable-length array. MSVC does
     * not implement VLAs and rejected it outright (C2057 / C2133 / C2466). */
    enum { MAX_PEERS = 8 };
    dogecoin_node *peers[MAX_PEERS];
    unsigned int num_peers = 0;
    unsigned int ni;
    for (ni = 0; ni < client->nodegroup->nodes->len && num_peers < MAX_PEERS; ni++) {
        dogecoin_node *n = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
        if (!n || !(n->state & NODE_CONNECTED) || !n->version_handshake) continue;
        if (n->services & DOGECOIN_NODE_COMPACT_FILTERS) {
            /* insert CF peers at the front */
            if (num_peers < MAX_PEERS) { peers[num_peers++] = n; }
        } else {
            if (num_peers < MAX_PEERS) { peers[num_peers++] = n; }
        }
    }
    if (num_peers == 0) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb("[bip157] no peer available for matched block fetch\n");
        return;
    }

    uint32_t total = (uint32_t)cfstate->matched_block_hashes->len;
    cfstate->cf_block_fetch_active = true;
    cfstate->matched_blocks_fetched = 0;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[bip157] requesting %u matched full blocks across %u peers [%us elapsed]\n",
            total, num_peers, spv_elapsed(client));

    /* Distribute blocks round-robin across peers in batches of 8 */
    const uint32_t batch_max = 8;
    uint32_t peer_idx = 0;
    uint32_t sent = 0;
    while (sent < total) {
        dogecoin_node *peer = peers[peer_idx % num_peers];
        peer_idx++;

        uint32_t batch = total - sent;
        if (batch > batch_max) batch = batch_max;

        cstring *payload = cstr_new_sz(9 + (size_t)batch * 36);
        if (!payload) break;

        ser_varlen(payload, batch);
        uint32_t bi;
        for (bi = 0; bi < batch; bi++) {
            uint32_t type = DOGECOIN_INV_TYPE_BLOCK;
            ser_u32(payload, type);
            ser_bytes(payload,
                (const uint8_t *)vector_idx(cfstate->matched_block_hashes, sent + bi), 32);
        }

        cstring *p2p_msg = dogecoin_p2p_message_new(
            peer->nodegroup->chainparams->netmagic,
            DOGECOIN_MSG_GETDATA,
            (const uint8_t *)payload->str, payload->len);
        dogecoin_node_send(peer, p2p_msg);
        cstr_free(p2p_msg, true);
        cstr_free(payload, true);
        sent += batch;
    }
}

/* ------------------------------------------------------------------ */
/* Flush contiguous complete batches to disk in height order.          */
/* ------------------------------------------------------------------ */
static void spv_cf_par_try_flush(dogecoin_spv_client *client)
{
    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    uint8_t n = cfstate->par_num_workers;
    dogecoin_bool flushed;

    do {
        flushed = false;
        uint8_t pi;
        for (pi = 0; pi < n; pi++) {
            cf_par_buf *buf = &cfstate->par_bufs[pi];
            if (buf->node_id == -1 || !buf->complete) continue;
            if (buf->batch_start != cfstate->par_flush_height) continue;

            /* Flush this batch sequentially */
            uint32_t count = buf->batch_end - buf->batch_start + 1;
            uint32_t ri;
            for (ri = 0; ri < count; ri++) {
                uint32_t h = buf->batch_start + ri;
                cf_par_record *rec = &buf->records[ri];
                if (rec->filter_data) {
                    if (client->cfilters_db)
                        dogecoin_cfilters_db_write(client->cfilters_db, h,
                                                   rec->block_hash, rec->filter_data);
                    cstr_free(rec->filter_data, true);
                    rec->filter_data = NULL;
                }
            }

            cfstate->filters_tip_height = buf->batch_end;
            cfstate->par_flush_height   = buf->batch_end + 1;

            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[bip157-par] flushed [%u..%u], progress %u/%u [%us elapsed]\n",
                    buf->batch_start, buf->batch_end,
                    buf->batch_end, cfstate->cfheaders_tip_height,
                    spv_elapsed(client));

            dogecoin_free(buf->records);
            buf->records  = NULL;
            buf->node_id  = -1;
            buf->complete = false;
            flushed = true;
            break;
        }
    } while (flushed);

    if (cfstate->filters_tip_height >= cfstate->cfheaders_tip_height) {
        if (client->nodegroup && client->nodegroup->log_write_cb) {
            uint32_t scan_start = cfstate->cf_scan_start_height > 0 ? cfstate->cf_scan_start_height : 1;
            client->nodegroup->log_write_cb(
                "[bip157] all filters processed: scanned heights %u..%u (of %u), %u matched blocks [%us elapsed]\n",
                scan_start, cfstate->filters_tip_height, cfstate->cfheaders_tip_height,
                (unsigned int)cfstate->matched_block_hashes->len, spv_elapsed(client));
            if (scan_start > 1)
                client->nodegroup->log_write_cb(
                    "[bip157] WARNING: scan started at height %u (checkpoint), not genesis — "
                    "transactions before height %u are not covered; use --filter_hash_db for full history\n",
                    scan_start, scan_start);
        }
        cfstate->awaiting_response = false;
        client->stateflags &= ~SPV_CFILTER_SYNC_FLAG;
        if (cfstate->matched_block_hashes->len > 0 && !cfstate->cf_block_fetch_active) {
            spv_cf_request_matched_blocks(client);
        } else if (!client->called_sync_completed && client->sync_completed) {
            if (client->smpv_enabled) dogecoin_net_spv_request_mempool(client);
            client->sync_completed(client);
            client->called_sync_completed = true;
        }
    }
}

static dogecoin_bool spv_lookup_headersdb_height_by_hash(dogecoin_spv_client* client,
                                                         const uint8_t hash[32],
                                                         uint32_t* out_height)
{
    dogecoin_headers_db* hdb;
    long old_pos;
    dogecoin_bool can_restore_pos;
    uint8_t rec[SPV_HEADERS_FILE_REC_LEN];

    if (!client || !hash || !out_height) return false;
    hdb = (dogecoin_headers_db*)client->headers_db_ctx;
    if (!hdb || !hdb->headers_tree_file) return false;

    old_pos = ftell(hdb->headers_tree_file);
    can_restore_pos = (old_pos >= 0);
    if (fseek(hdb->headers_tree_file, SPV_HEADERS_FILE_HDR_LEN, SEEK_SET) != 0) {
        if (can_restore_pos) fseek(hdb->headers_tree_file, old_pos, SEEK_SET);
        return false;
    }

    uint8_t hash_rev[32];
    size_t ri;
    for (ri = 0; ri < sizeof(hash_rev); ri++) {
        hash_rev[ri] = hash[sizeof(hash_rev) - 1 - ri];
    }

    while (fread(rec, sizeof(rec), 1, hdb->headers_tree_file) == 1) {
        struct const_buffer rec_buf = { rec, sizeof(rec) };
        uint256_t rec_hash;
        uint32_t rec_height = 0;
        uint256_t rec_chainwork_unused;
        deser_u256(rec_hash, &rec_buf);
        deser_u32(&rec_height, &rec_buf);
        deser_u256(rec_chainwork_unused, &rec_buf);
        UNUSED(rec_chainwork_unused);
        if (memcmp(rec_hash, hash, sizeof(uint256_t)) == 0 ||
            memcmp(rec_hash, hash_rev, sizeof(uint256_t)) == 0) {
            *out_height = rec_height;
            if (can_restore_pos) fseek(hdb->headers_tree_file, old_pos, SEEK_SET);
            return true;
        }
    }

    if (can_restore_pos) fseek(hdb->headers_tree_file, old_pos, SEEK_SET);
    return false;
}

typedef struct spv_merkle_log_ctx_ {
    dogecoin_spv_client* client;
    const dogecoin_blockindex* pindex;
} spv_merkle_log_ctx;

static dogecoin_bool spv_log_merkle_match(const uint8_t txid[32], uint32_t pos, dogecoin_bool consumed, void* ctx)
{
    spv_merkle_log_ctx* c = (spv_merkle_log_ctx*)ctx;
    if (!c || !c->client || !c->client->nodegroup || !c->client->nodegroup->log_write_cb) return false;

    char txid_hex[SPV_TXID_HEX_LEN];
    utils_bin_to_hex((unsigned char*)txid, 32, txid_hex);
    int match_height = c->pindex ? (int)c->pindex->height : -1;
    c->client->nodegroup->log_write_cb("[merkle][decode] block_height=%d txid=%s pos=%u consumed=%d\n",
        match_height, txid_hex, pos, consumed ? 1 : 0);
    return true;
}

static const unsigned int HEADERS_MAX_RESPONSE_TIME = 120;
/* Seconds a parallel header segment may sit without a getheaders response
   before it is released back to the pool for another peer to pick up. */
static const uint64_t PAR_HDR_SEG_TIMEOUT = 120;
/* Seconds without the ordered flush advancing before we log loudly. */
static const uint64_t PAR_HDR_STALL_WARN = 300;
/* Ceiling on raw header bytes staged in memory ahead of the ordered flush.
   Segment sizes are uneven (the checkpoint arrays were merged at different
   intervals), so a fixed segment count bounds neither memory nor peer
   utilisation well; budget the thing that actually grows. */
static const uint64_t PAR_HDR_MAX_BUFFERED = 256ull * 1024 * 1024;
/* A segment must have been owned this long before its download rate is judged;
   below this the sample is noise. */
static const uint64_t PAR_HDR_RATE_GRACE = 30;
/* The flush-head owner is preempted when its rate falls below this fraction of
   the median rate across the other active segments, expressed as a divisor. */
static const uint32_t PAR_HDR_SLOW_FACTOR = 4;
static const unsigned int MIN_TIME_DELTA_FOR_STATE_CHECK = 5;
static const unsigned int CF_RESPONSE_TIMEOUT = 30;
static const unsigned int BLOCK_GAP_TO_DEDUCT_TO_START_SCAN_FROM = 5;
static const unsigned int BLOCKS_DELTA_IN_S = 60;
static const unsigned int COMPLETED_WHEN_NUM_NODES_AT_SAME_HEIGHT = 2;

#if defined(USE_LIBOQS) || defined(USE_RACCOON_G)
/* Maximum number of pending PQC OP_RETURN commitments buffered per client
   for cross-TX carrier validation (TX_C has OP_RETURN, TX_R has scriptSig).
   Cap enforced via LRU eviction in spv_pqc_add_pending so a peer flooding
   cheap OP_RETURN commits cannot balloon SPV node memory. */
#define SPV_PQC_PENDING_MAX 16

/* Maximum serialized TX_C size (bytes) we will buffer per pending entry.
   Without this, a peer can submit a max-size P2P message (~4 MiB) and pin
   SPV_PQC_PENDING_MAX * DOGECOIN_MAX_P2P_MSG_SIZE of memory per client.
   Carrier TX_Cs are minimal OP_RETURN-bearing transactions and have no
   legitimate need to approach standardness limits; 100 KiB matches the
   classic standardness MAX_STANDARD_TX_WEIGHT/4 ceiling and bounds total
   buffered carrier memory at SPV_PQC_PENDING_MAX * 100 KiB = 1.6 MiB per
   client per algorithm family. */
#define SPV_PQC_PENDING_TXC_RAW_MAX (100u * 1024u)

typedef struct spv_pqc_pending_commit {
    uint8_t commit[32];           /* 32-byte commit hash - used as hash key */
    dogecoin_pqc_algo_t algo;
    uint32_t txpos;
    uint32_t height;              /* block height where TX_C was seen */
    uint8_t* txc_raw;             /* serialized TX_C for signature verification */
    size_t txc_raw_len;
    UT_hash_handle hh;            /* makes this structure hashable */
} spv_pqc_pending_commit_t;

/* Per-client pending-commit table accessor. Stored as an opaque void* in
   dogecoin_spv_client so the header does not need to expose uthash types.
   uthash tracks insertion order in its hh.next/hh.prev chain, which gives
   us O(1) LRU eviction of the oldest entry without an extra linked list. */
static inline spv_pqc_pending_commit_t* spv_pqc_table(const dogecoin_spv_client* client) {
    return client ? (spv_pqc_pending_commit_t*)client->pqc_pending_commits : NULL;
}

/* Helper: find pending commit by commit hash within this client's table. */
static spv_pqc_pending_commit_t* spv_pqc_find_pending(dogecoin_spv_client* client, const uint8_t* commit) {
    spv_pqc_pending_commit_t* head = spv_pqc_table(client);
    spv_pqc_pending_commit_t* found = NULL;
    if (!head || !commit) return NULL;
    HASH_FIND(hh, head, commit, 32, found);
    return found;
}

/* Helper: add pending commit to this client's hash table. De-duplicates by
   commit hash and enforces SPV_PQC_PENDING_MAX via LRU eviction of the
   oldest insertion (uthash's iteration order is insertion order). */
static void spv_pqc_add_pending(dogecoin_spv_client* client, spv_pqc_pending_commit_t* entry) {
    if (!client || !entry) return;
    spv_pqc_pending_commit_t* head = spv_pqc_table(client);
    /* Replace existing entry with the same commit hash to avoid stale state. */
    spv_pqc_pending_commit_t* existing = NULL;
    if (head) HASH_FIND(hh, head, entry->commit, 32, existing);
    if (existing) {
        HASH_DEL(head, existing);
        if (existing->txc_raw) dogecoin_free(existing->txc_raw);
        dogecoin_free(existing);
    }
    /* Enforce cap: evict oldest entries until there is room for the new one.
       uthash links entries in insertion order via `hh.next` with a tail
       pointer for O(1) append (see UT_hash_table::tail / "tail hh in app
       order, for fast append" in uthash.h). HASH_ITER walks head→tail in
       insertion order, so the table head is always the oldest still-alive
       entry — exact LRU under our usage where we only add and (on TX_R
       match) delete by key. */
    while (head && HASH_COUNT(head) >= SPV_PQC_PENDING_MAX) {
        spv_pqc_pending_commit_t* oldest = head; /* head == oldest insertion */
        HASH_DEL(head, oldest);
        if (oldest->txc_raw) dogecoin_free(oldest->txc_raw);
        dogecoin_free(oldest);
    }
    HASH_ADD(hh, head, commit, 32, entry);
    client->pqc_pending_commits = head;
}

/* Build & insert a pending PQC commit, enforcing both the count cap (via
   spv_pqc_add_pending's LRU eviction) and the per-entry TX_C raw size cap
   SPV_PQC_PENDING_TXC_RAW_MAX. Returns 1 on insert, 0 if the entry was
   refused (oversize tx_raw or alloc failure). Centralising this avoids
   the four near-identical alloc/copy/insert blocks at the call sites and
   ensures every future carrier algorithm picks up the same caps for free. */
static int spv_pqc_buffer_pending(dogecoin_spv_client* client,
                                  const uint8_t commit[32],
                                  dogecoin_pqc_algo_t algo,
                                  uint32_t txpos, uint32_t height,
                                  const uint8_t* tx_raw, size_t tx_raw_len,
                                  const char* log_tag) {
    if (!client || !commit || !tx_raw) return 0;
    if (tx_raw_len == 0 || tx_raw_len > SPV_PQC_PENDING_TXC_RAW_MAX) {
        if (client->nodegroup && client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb(
                "[%s] Skip pending: TX_C size %zu exceeds cap %u\n",
                log_tag ? log_tag : "pqc-commit",
                (size_t)tx_raw_len, (unsigned)SPV_PQC_PENDING_TXC_RAW_MAX);
        }
        return 0;
    }
    spv_pqc_pending_commit_t* entry = dogecoin_calloc(1, sizeof(*entry));
    if (!entry) return 0;
    entry->txc_raw = dogecoin_malloc(tx_raw_len);
    if (!entry->txc_raw) { dogecoin_free(entry); return 0; }
    memcpy(entry->commit, commit, 32);
    entry->algo = algo;
    entry->txpos = txpos;
    entry->height = height;
    memcpy(entry->txc_raw, tx_raw, tx_raw_len);
    entry->txc_raw_len = tx_raw_len;
    spv_pqc_add_pending(client, entry);
    return 1;
}

/* Note: a removal helper used to live here but every call site needs
   special-case logic (the TX_R match path moves out `txc_raw` before
   freeing, the free-all path runs inside a HASH_ITER), so HASH_DEL is
   inlined at the call sites and the helper was removed to avoid dead
   code. */

#endif /* USE_LIBOQS || USE_RACCOON_G (pqc pending) */

#ifdef USE_ZK_CARRIER
/* Maximum number of pending ZK OP_RETURN commitments buffered per client
   for cross-TX carrier validation (TX_C has OP_RETURN, TX_R has scriptSig).
   Cap enforced via LRU eviction in spv_zk_add_pending so a peer flooding
   cheap OP_RETURN commits cannot balloon SPV node memory. Mirrors the
   PQC SPV_PQC_PENDING_MAX cap to keep both carriers' memory ceilings
   identical and to make the LRU semantics easy to audit. */
#define SPV_ZK_PENDING_MAX 16

/* Per-entry TX_C raw size cap (bytes). Mirrors SPV_PQC_PENDING_TXC_RAW_MAX
   so both carrier families share the same per-entry memory ceiling and
   the combined SPV per-client carrier memory is bounded at
   2 * SPV_*_PENDING_MAX * 100 KiB ≈ 3.2 MiB. */
#define SPV_ZK_PENDING_TXC_RAW_MAX (100u * 1024u)

/* Hash table of pending TX_C OP_RETURN commitments awaiting their TX_R
   reveal.  Mirrors the PQC per-client pending table (key: 32-byte commit)
   so the SPV validator can cross-validate ZK carriers in O(1) when the
   reveal arrives in a later block.  Kept separate from the PQC table to
   minimise blast radius — the two key spaces are independent and combining
   them would force every PQC enum case to also know about ZK modes. */
typedef struct spv_zk_pending_commit {
    uint8_t commit[32];                /* 32-byte commit hash - hash key */
    dogecoin_zk_mode_t mode;
    uint32_t txpos;
    uint32_t height;                   /* block height where TX_C was seen */
    uint8_t* txc_raw;                  /* serialized TX_C, kept for diagnostics */
    size_t txc_raw_len;
    UT_hash_handle hh;
} spv_zk_pending_commit_t;

/* Per-client pending-commit table accessor.  Stored as an opaque void* in
   dogecoin_spv_client so the header does not need to expose uthash types. */
static inline spv_zk_pending_commit_t* spv_zk_table(const dogecoin_spv_client* client) {
    return client ? (spv_zk_pending_commit_t*)client->zk_pending_commits : NULL;
}

static spv_zk_pending_commit_t* spv_zk_find_pending(dogecoin_spv_client* client, const uint8_t* commit) {
    spv_zk_pending_commit_t* head = spv_zk_table(client);
    spv_zk_pending_commit_t* found = NULL;
    if (!head || !commit) return NULL;
    HASH_FIND(hh, head, commit, 32, found);
    return found;
}

/* Add pending commit to this client's hash table.  De-duplicates by commit
   hash and enforces SPV_ZK_PENDING_MAX via LRU eviction of the oldest
   insertion (uthash's iteration order is insertion order — see the matching
   note on spv_pqc_add_pending above). */
static void spv_zk_add_pending(dogecoin_spv_client* client, spv_zk_pending_commit_t* entry) {
    if (!client || !entry) return;
    spv_zk_pending_commit_t* head = spv_zk_table(client);
    /* Replace existing entry with the same commit hash to avoid stale state. */
    spv_zk_pending_commit_t* existing = NULL;
    if (head) HASH_FIND(hh, head, entry->commit, 32, existing);
    if (existing) {
        HASH_DEL(head, existing);
        if (existing->txc_raw) dogecoin_free(existing->txc_raw);
        dogecoin_free(existing);
    }
    while (head && HASH_COUNT(head) >= SPV_ZK_PENDING_MAX) {
        spv_zk_pending_commit_t* oldest = head; /* head == oldest insertion */
        HASH_DEL(head, oldest);
        if (oldest->txc_raw) dogecoin_free(oldest->txc_raw);
        dogecoin_free(oldest);
    }
    HASH_ADD(hh, head, commit, 32, entry);
    client->zk_pending_commits = head;
}

static void spv_zk_remove_pending(dogecoin_spv_client* client, spv_zk_pending_commit_t* entry) {
    if (!client || !entry) return;
    spv_zk_pending_commit_t* head = spv_zk_table(client);
    HASH_DEL(head, entry);
    if (entry->txc_raw) dogecoin_free(entry->txc_raw);
    dogecoin_free(entry);
    client->zk_pending_commits = head;
}

/* Build & insert a pending ZK commit, enforcing both the count cap and
   SPV_ZK_PENDING_TXC_RAW_MAX. Mirrors spv_pqc_buffer_pending; see that
   helper for rationale. Returns 1 on insert, 0 if refused. */
static int spv_zk_buffer_pending(dogecoin_spv_client* client,
                                 const uint8_t commit[32],
                                 dogecoin_zk_mode_t mode,
                                 uint32_t txpos, uint32_t height,
                                 const uint8_t* tx_raw, size_t tx_raw_len) {
    if (!client || !commit || !tx_raw) return 0;
    if (tx_raw_len == 0 || tx_raw_len > SPV_ZK_PENDING_TXC_RAW_MAX) {
        if (client->nodegroup && client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb(
                "[zk-commit] Skip pending: TX_C size %zu exceeds cap %u\n",
                (size_t)tx_raw_len, (unsigned)SPV_ZK_PENDING_TXC_RAW_MAX);
        }
        return 0;
    }
    spv_zk_pending_commit_t* entry = dogecoin_calloc(1, sizeof(*entry));
    if (!entry) return 0;
    entry->txc_raw = dogecoin_malloc(tx_raw_len);
    if (!entry->txc_raw) { dogecoin_free(entry); return 0; }
    memcpy(entry->commit, commit, 32);
    entry->mode = mode;
    entry->txpos = txpos;
    entry->height = height;
    memcpy(entry->txc_raw, tx_raw, tx_raw_len);
    entry->txc_raw_len = tx_raw_len;
    spv_zk_add_pending(client, entry);
    return 1;
}

#endif /* USE_ZK_CARRIER */

static dogecoin_bool dogecoin_net_spv_node_timer_callback(dogecoin_node *node, uint64_t *now);
void dogecoin_net_spv_post_cmd(dogecoin_node *node, dogecoin_p2p_msg_hdr *hdr, struct const_buffer *buf);
void dogecoin_net_spv_node_handshake_done(dogecoin_node *node);
void par_hdr_assign(dogecoin_spv_client *client, dogecoin_node *node);
void par_hdr_reclaim(dogecoin_spv_client *client, uint64_t now);
void par_hdr_free(dogecoin_spv_client *client);
static void par_hdr_recv(dogecoin_spv_client *client, dogecoin_node *node,
                         struct const_buffer *buf, uint32_t count);

void dogecoin_node_connection_state_changed_cb(dogecoin_node *node) {
    if (node->nodegroup->should_connect_to_more_nodes_cb) {
        if (node->nodegroup->should_connect_to_more_nodes_cb(node)) {
            dogecoin_spv_client *client = (dogecoin_spv_client*)node->nodegroup->ctx;
            /* Use explicit peer IPs when provided; fall back to DNS seeds otherwise. */
            dogecoin_spv_client_discover_peers(client, client->peer_ips);
            dogecoin_node_group_connect_next_nodes(node->nodegroup);
        }
    }
}

/**
 * @brief This function deterimines if we should connect to more nodes.
 *
 * @param node the node that we are connected to.
 *
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_node_should_connect_to_more_cb(dogecoin_node* node) {
    int connected_amount = dogecoin_node_group_amount_of_connected_nodes(node->nodegroup, NODE_CONNECTED) + dogecoin_node_group_amount_of_connected_nodes(node->nodegroup, NODE_CONNECTING);
    node->nodegroup->log_write_cb("check if more nodes are required (connected to already: %d): %s\n", connected_amount, connected_amount < node->nodegroup->desired_amount_connected_nodes ? "true" : "false");
    if (connected_amount < node->nodegroup->desired_amount_connected_nodes) {
        return true;
    }
    return false;
}

/**
 * The function sets the nodegroup's postcmd_cb to dogecoin_net_spv_post_cmd,
 * the nodegroup's handshake_done_cb to dogecoin_net_spv_node_handshake_done,
 * the nodegroup's node_connection_state_changed_cb to NULL, and the
 * nodegroup's periodic_timer_cb to dogecoin_net_spv_node_timer_callback
 *
 * @param nodegroup The nodegroup to set the callbacks for.
 */
void dogecoin_net_set_spv(dogecoin_node_group *nodegroup)
{
    nodegroup->postcmd_cb = dogecoin_net_spv_post_cmd;
    nodegroup->handshake_done_cb = dogecoin_net_spv_node_handshake_done;
    nodegroup->should_connect_to_more_nodes_cb = dogecoin_node_should_connect_to_more_cb;
    nodegroup->node_connection_state_changed_cb = dogecoin_node_connection_state_changed_cb;
    nodegroup->periodic_timer_cb = dogecoin_net_spv_node_timer_callback;
}

/**
 * The function creates a new dogecoin_spv_client object and initializes it
 *
 * @param params The chainparams struct that we created earlier.
 * @param debug If true, the node will print out debug messages to stdout.
 * @param headers_memonly If true, the headers database will not be loaded from disk.
 * @param use_checkpoints If true, the client will use checkpoints.
 * @param full_sync If true, the client will do a full sync.
 * @param maxnodes The maximum amount of nodes that the client will connect to.
 * @param http_server The IP and port for the HTTP server; if NULL, the HTTP server will not be initialized.
 *
 * @return A pointer to a dogecoin_spv_client object.
 */
dogecoin_spv_client* dogecoin_spv_client_new(const dogecoin_chainparams *params, dogecoin_bool debug, dogecoin_bool headers_memonly, dogecoin_bool use_checkpoints, dogecoin_bool full_sync, int maxnodes, const char* http_server)
{
    dogecoin_spv_client* client;
    client = dogecoin_calloc(1, sizeof(*client));

    client->last_headersrequest_time = 0; //!< time when we requested the last header package
    client->last_statecheck_time = 0;
    client->oldest_item_of_interest = time(NULL)-5*60;
    client->stateflags = full_sync ? SPV_FULLBLOCK_SYNC_FLAG : SPV_HEADER_SYNC_FLAG;

    client->chainparams = params;

    client->nodegroup = dogecoin_node_group_new(params);
    client->nodegroup->ctx = client;
    if (maxnodes > 120) {
        maxnodes = 120;
    }
    client->nodegroup->desired_amount_connected_nodes = maxnodes;

    dogecoin_net_set_spv(client->nodegroup);

    if (debug) {
        client->nodegroup->log_write_cb = net_write_log_printf;
    }

#ifdef USE_LIBOQS
    // Log what PQC variants are present at runtime (minimal, no hard dependency).
    if (client->nodegroup && client->nodegroup->log_write_cb) {
        client->nodegroup->log_write_cb(
            "[oqs] falcon_512=%s falcon_1024=%s ml_dsa_44=%s (liboqs)\n",
            OQS_SIG_alg_is_enabled(OQS_SIG_alg_falcon_512) ? "enabled" : "disabled",
            OQS_SIG_alg_is_enabled(OQS_SIG_alg_falcon_1024) ? "enabled" : "disabled",
            OQS_SIG_alg_is_enabled(OQS_SIG_alg_ml_dsa_44) ? "enabled" : "disabled");
    }
#endif
#ifdef USE_RACCOON_G
    /* Raccoon-G is provided by the in-tree src/raccoon_g/ implementation,
       not by the liboqs fork (which no longer ships Raccoon-G). Log it on
       its own line so the source of the backend is unambiguous. */
    if (client->nodegroup && client->nodegroup->log_write_cb) {
        client->nodegroup->log_write_cb("[raccoon_g] raccoon_g_44=enabled (in-tree)\n");
    }
#endif

    if (params && (strcmp(params->chainname, "main") == 0 ||
                   strcmp(params->chainname, "testnet3") == 0)) {
        client->use_checkpoints = use_checkpoints;
    }
    client->headers_db = &dogecoin_headers_db_interface_file;
    client->headers_db_ctx = client->headers_db->init(params, headers_memonly);

    // set callbacks
    client->header_connected = NULL;
    client->called_sync_completed = false;
    client->sync_completed = NULL;
    client->header_message_processed = NULL;
    client->sync_transaction = NULL;
    client->sync_transaction_ctx = NULL;

    // BIP37 filter state (off by default)
    client->bloom_filter = NULL;
    client->bloom_filter_len = 0;
    client->bloom_nhashfunc = 0;
    client->bloom_ntweak = 0;
    client->bloom_flags = 0;
    client->bloom_filter_debug_dump = NULL;

    // merkleblock -> matched tx state (btree keyed by txid)
    client->merkle_match_tree = NULL;
    client->merkle_match_pending = 0;
    client->merkle_match_active = false;
    client->merkle_match_blockindex = NULL;
    client->rescan_total = 0;
    client->rescan_matched = 0;
    client->filtered_history_last_end_height = -1; /* -1 means no historical filtered range has been requested yet */
    client->filtered_history_tail_rerequest_count = 0;
    dogecoin_hash_clear(client->filtered_history_last_rerequest_txid);
    client->filtered_history_last_rerequest_height = -1;

    if (http_server) {
        // split ip and port
        char* http_server_copy = strdup(http_server);
        char* ip = strtok(http_server_copy, ":");
        char* port = strtok(NULL, ":");

        // HTTP server initialization
        dogecoin_http_server_init(client->nodegroup, ip, atoi(port));
        client->nodegroup->log_write_cb("HTTP server initialized\n");
        free(http_server_copy);
    }

    client->stats_ring_len = 0;
    client->stats_ring_head = 0;
    client->stats_blocks_total = 0;
    client->stats_txs_total = 0;
    client->stats_outputs_total = 0;
    client->stats_out_value_total = 0;
    client->stats_fees_total = 0;
    client->stats_block_bytes_total = 0;
    client->start_ts = (uint64_t)time(NULL);

    // SMPV default off
    client->smpv_ctx = NULL;
    client->smpv_enabled = false;

    // BIP157 compact filter sync (on by default; disable with --no_cfilters)
    client->compact_filters_enabled = true;
    client->cfilter_state = dogecoin_compact_filter_state_new();
    if (client->cfilter_state) {
        client->cfilter_state->enabled = true;
        /* The compiled-in checkpoints are the trust anchor. Load them here so the
           client holds them before it ever speaks to a peer. */
        dogecoin_cf_load_hardcoded_checkpoints(client->cfilter_state, params);
    }
    dogecoin_mem_zero(client->cf_prev_filter_header, sizeof(uint256_t));
    client->cf_computed_height = 0;
    client->cf_export_enabled = false;

    /* BIP157 persistent filter storage (opened in dogecoin_spv_client_load) */
    client->cfheaders_db  = NULL;
    client->cfilters_db   = NULL;
    client->cfheaders_path = NULL;
    client->cfilters_path  = NULL;

    client->peer_ips = NULL;

    client->aux_hash_db_ctx = NULL;
    client->aux_hash_db = NULL;
    client->cf_start_height = 0;
    client->cf_num_workers = 0;

    return client;
}

/**
 * It adds peers to the nodegroup.
 *
 * @param client the dogecoin_spv_client object
 * @param ips A comma-separated list of IPs or seeds to connect to.
 */
void dogecoin_spv_client_discover_peers(dogecoin_spv_client* client, const char *ips)
{

    /* Remember explicit peer IPs so reconnects reuse them instead of DNS seeds. */
    if (ips) {
        if (client->peer_ips) dogecoin_free(client->peer_ips);
        client->peer_ips = strdup(ips);
    }


    dogecoin_node_group_add_peers_by_ip_or_seed(client->nodegroup, ips);
}

/**
 * The function loops through all the nodes in the node group and connects to the next nodes in the
 * node group
 *
 * @param client The dogecoin_spv_client object.
 */
void dogecoin_spv_client_runloop(dogecoin_spv_client* client)
{
    dogecoin_node_group_connect_next_nodes(client->nodegroup);
    dogecoin_node_group_event_loop(client->nodegroup);
}

/**
 * It frees the memory allocated for the client
 *
 * @param client The client object to be freed.
 *
 * @return Nothing.
 */
void dogecoin_spv_client_free(dogecoin_spv_client *client)
{
    if (!client)
        return;

    if (client->smpv_enabled && client->smpv_ctx) {
        dogecoin_smpv_stop((dogecoin_smpv_client*)client->smpv_ctx);
        dogecoin_smpv_client_free((dogecoin_smpv_client*)client->smpv_ctx);
        client->smpv_ctx = NULL;
        client->smpv_enabled = false;
    }

    if (client->bloom_filter) {
        dogecoin_free(client->bloom_filter);
        client->bloom_filter = NULL;
    }
    client->bloom_filter_len = 0;
    client->bloom_nhashfunc = 0;
    client->bloom_ntweak = 0;
    client->bloom_flags = 0;
    if (client->bloom_filter_debug_dump) {
        dogecoin_free(client->bloom_filter_debug_dump);
        client->bloom_filter_debug_dump = NULL;
    }

    if (client->merkle_match_tree) {
        dogecoin_btree_tdestroy(client->merkle_match_tree, dogecoin_free);
        client->merkle_match_tree = NULL;
    }
    client->merkle_match_pending = 0;
    client->merkle_match_active = false;
    client->merkle_match_blockindex = NULL;

    if (client->cfilter_state) {
        dogecoin_compact_filter_state_free(client->cfilter_state);
        client->cfilter_state = NULL;
    }
    client->compact_filters_enabled = false;

    if (client->cfheaders_db) {
        dogecoin_cfheaders_db_free(client->cfheaders_db);
        client->cfheaders_db = NULL;
    }
    if (client->cfilters_db) {
        dogecoin_cfilters_db_free(client->cfilters_db);
        client->cfilters_db = NULL;
    }
    if (client->cfheaders_path) {
        dogecoin_free(client->cfheaders_path);
        client->cfheaders_path = NULL;
    }
    if (client->cfilters_path) {
        dogecoin_free(client->cfilters_path);
        client->cfilters_path = NULL;
    }

    if (client->peer_ips) {
        dogecoin_free(client->peer_ips);
        client->peer_ips = NULL;
    }

    if (client->aux_hash_db && client->aux_hash_db_ctx) {
        client->aux_hash_db->free(client->aux_hash_db_ctx);
        client->aux_hash_db_ctx = NULL;
        client->aux_hash_db = NULL;
    }


    par_hdr_free(client);
    if (client->headers_db)
    {
        if (client->headers_db_ctx)
        {
            client->headers_db->free(client->headers_db_ctx);
        }
        client->headers_db_ctx = NULL;
        client->headers_db = NULL;
    }

    if (client->nodegroup) {
        dogecoin_node_group_free(client->nodegroup);
        client->nodegroup = NULL;
    }

#if defined(USE_LIBOQS) || defined(USE_RACCOON_G)
    /* Release any pending PQC carrier commits that were never matched by a TX_R.
       Only the table owned by this client is freed — other live clients keep
       their own per-client tables intact. */
    {
        spv_pqc_pending_commit_t* head = spv_pqc_table(client);
        spv_pqc_pending_commit_t* entry;
        spv_pqc_pending_commit_t* tmp;
        HASH_ITER(hh, head, entry, tmp) {
            HASH_DEL(head, entry);
            if (entry->txc_raw) dogecoin_free(entry->txc_raw);
            dogecoin_free(entry);
        }
        client->pqc_pending_commits = NULL;
    }
#endif

#ifdef USE_ZK_CARRIER
    /* Release any pending ZK carrier commits that were never matched by a TX_R.
       Only the table owned by this client is freed — other live clients keep
       their own per-client tables intact. */
    {
        spv_zk_pending_commit_t* head = spv_zk_table(client);
        spv_zk_pending_commit_t* entry;
        spv_zk_pending_commit_t* tmp;
        HASH_ITER(hh, head, entry, tmp) {
            HASH_DEL(head, entry);
            if (entry->txc_raw) dogecoin_free(entry->txc_raw);
            dogecoin_free(entry);
        }
        client->zk_pending_commits = NULL;
    }
#endif

    dogecoin_free(client);
}

/**
 * Loads the headers database from a file
 *
 * @param client the client object
 * @param file_path The path to the headers database file.
 * @param prompt If true, the user will be prompted to confirm loading the database.
 *
 * @return A boolean value.
 */
static void par_hdr_prune_synced(dogecoin_spv_client *client);

dogecoin_bool dogecoin_spv_client_load(dogecoin_spv_client *client, const char *file_path, dogecoin_bool prompt)
{
    if (!client)
        return false;

    if (!client->headers_db)
        return false;

    if (!client->headers_db->load(client->headers_db_ctx, file_path, prompt))
        return false;

    /* Segments are planned before the DB is loaded, so the covered ones can
       only be dropped here. */
    par_hdr_prune_synced(client);

    /* Open BIP157 persistent filter databases when compact filter sync is enabled */
    if (client->compact_filters_enabled && client->cfilter_state) {
        dogecoin_bool inmem = (file_path && strcmp(file_path, ":memory:") == 0);

        client->cfheaders_db = dogecoin_cfheaders_db_new(client->chainparams, inmem);
        if (!dogecoin_cfheaders_db_load(client->cfheaders_db, client->cfheaders_path, client->cfilter_state)) {
            fprintf(stderr, "spv: failed to open cfheaders.dat; continuing without persistence\n");
            dogecoin_cfheaders_db_free(client->cfheaders_db);
            client->cfheaders_db = NULL;
        }

        /* A cfheaders tip above the chain tip means the on-disk chain took an
           append it should not have. The splice point is not recoverable from
           the file, since heights are assigned from a running counter and stay
           contiguous either way, so the whole cache goes rather than a
           truncation that would leave the bad entries below the chain tip. */
        if (client->cfheaders_db) {
            dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
            dogecoin_compact_filter_state *cfstate = client->cfilter_state;
            /* tip->height == 0 is an unloaded or in-memory headers DB, which
               says nothing about the cfheaders file: judging against it threw
               away a full 6.27M-header cache in test_cfheadersdb. */
            if (tip && tip->height > 0 &&
                cfstate->cfheaders_tip_height > (uint32_t)tip->height) {
                fprintf(stderr,
                        "spv: cfheaders tip %u is above chain tip %d; discarding the cfheaders cache\n",
                        cfstate->cfheaders_tip_height, tip->height);
                if (cfstate->filter_headers->len > 0) {
                    vector_free(cfstate->filter_headers, true);
                    cfstate->filter_headers = vector_new(4096, dogecoin_free);
                }
                if (cfstate->filter_headers_flat) {
                    dogecoin_free(cfstate->filter_headers_flat);
                    cfstate->filter_headers_flat = NULL;
                    cfstate->filter_headers_flat_len = 0;
                }
                dogecoin_mem_zero(cfstate->cfheaders_tip_hash, sizeof(uint256_t));
                dogecoin_mem_zero(cfstate->genesis_filter_header, sizeof(uint256_t));
                cfstate->cfheaders_tip_height  = 0;
                cfstate->cfheaders_base_height = 0;
                dogecoin_cfheaders_db_reset(client->cfheaders_db);
            }
        }

        client->cfilters_db = dogecoin_cfilters_db_new(client->chainparams, inmem);
        if (!dogecoin_cfilters_db_load(client->cfilters_db, client->cfilters_path)) {
            fprintf(stderr, "spv: failed to open cfilters.dat; continuing without persistence\n");
            dogecoin_cfilters_db_free(client->cfilters_db);
            client->cfilters_db = NULL;
        }
    }

    return true;
}

/**
 * If we are in the header sync state, we request headers from a random node
 *
 * @param node the node that we are checking
 * @param now The current time in seconds.
 */
void dogecoin_net_spv_periodic_statecheck(dogecoin_node *node, uint64_t *now)
{
    /* statecheck logic */
    /* ================ */

    dogecoin_spv_client *client = (dogecoin_spv_client*)node->nodegroup->ctx;
    dogecoin_blockindex *pindex = client->headers_db->getchaintip(client->headers_db_ctx);
    client->nodegroup->log_write_cb("Statecheck: amount of connected nodes: %d\nchaintip hash: %s\nchaintip height: %d\n", dogecoin_node_group_amount_of_connected_nodes(client->nodegroup, NODE_CONNECTED), hash_to_string(pindex->hash), pindex->height);

    /* Segments flush in order, so the chaintip does not move until the lowest
       one lands: on a fresh parallel sync the line above reads height 0 for
       minutes while every peer is busy, which is indistinguishable from a
       stall. Report the staging progress that is actually moving. */
    if (client->par_hdr && client->par_hdr->active) {
        client->nodegroup->log_write_cb(
            "par-hdr: segment %u of %u flushed, %llu MB staged\n",
            client->par_hdr->flush_idx, client->par_hdr->num_segs,
            (unsigned long long)(client->par_hdr->buffered_bytes / (1024 * 1024)));
    }

    if (client->last_headersrequest_time > 0 && *now > client->last_headersrequest_time)
    {
        int64_t timedetla = *now - client->last_headersrequest_time;
        client->nodegroup->log_write_cb("No header response in time (used %d) for node %d\n", timedetla, node->nodeid);
        if (timedetla > HEADERS_MAX_RESPONSE_TIME)
        {
            node->state &= ~NODE_HEADERSYNC;
            dogecoin_node_misbehave(node);
        }
    }
    if (node->time_last_request > 0 && *now > node->time_last_request)
    {
        // we are downloading blocks from this peer
        int64_t timedelta = *now - node->time_last_request;
        client->nodegroup->log_write_cb("No block response in time (used %d) for node %d\n", timedelta, node->nodeid);
        if (timedelta > HEADERS_MAX_RESPONSE_TIME)
        {
            node->state &= ~NODE_BLOCKSYNC;
            dogecoin_node_misbehave(node);
        }
    }

    // Do NOT reset client->last_headersrequest_time / node->time_last_request
    // before requesting headers here. These timestamps are the basis for the
    // response-timeout checks above; zeroing them on every periodic statecheck
    // made the "no response in time" detection fire immediately on the next
    // tick regardless of how much time had actually elapsed. They are updated
    // only when an actual request is sent (see dogecoin_net_spv_request_headers
    // and the getheaders/getdata send paths).
    /* Parallel genesis header sync: reclaim segments from dead or silent
       peers and top up idle peers before the normal request path runs. */
    par_hdr_reclaim(client, *now);

    if ((client->stateflags & SPV_HEADER_SYNC_FLAG) == SPV_HEADER_SYNC_FLAG)
    {
        dogecoin_net_spv_request_headers(client);
    }

    if ((client->stateflags & SPV_FULLBLOCK_SYNC_FLAG) == SPV_FULLBLOCK_SYNC_FLAG)
    {
        dogecoin_net_spv_request_headers(client);

        /* Recovery: if our chaintip is far below the global-best peer and no
         * BLOCKSYNC node has a block request in-flight, we're deadlocked —
         * the inv handler won't request blocks because we're too far behind,
         * and dogecoin_net_spv_request_headers early-returns because BLOCKSYNC
         * nodes exist.  Force all nodes back to header sync. */
        dogecoin_blockindex *sc_tip = client->headers_db->getchaintip(client->headers_db_ctx);
        uint32_t sc_best = 0;
        size_t sc_i;
        for (sc_i = 0; sc_i < client->nodegroup->nodes->len; sc_i++) {
            dogecoin_node *sc_n = vector_idx(client->nodegroup->nodes, sc_i);
            if ((sc_n->state & NODE_CONNECTED) && sc_n->bestknownheight > sc_best)
                sc_best = sc_n->bestknownheight;
        }
        if (sc_tip && sc_best > (uint32_t)sc_tip->height + 1440) {
            dogecoin_bool any_pending = false;
            for (sc_i = 0; sc_i < client->nodegroup->nodes->len; sc_i++) {
                dogecoin_node *sc_n = vector_idx(client->nodegroup->nodes, sc_i);
                if ((sc_n->state & NODE_BLOCKSYNC) && (sc_n->state & NODE_CONNECTED) && sc_n->time_last_request > 0)
                    any_pending = true;
            }
            if (!any_pending) {
                for (sc_i = 0; sc_i < client->nodegroup->nodes->len; sc_i++) {
                    dogecoin_node *sc_n = vector_idx(client->nodegroup->nodes, sc_i);
                    if (sc_n->state & NODE_BLOCKSYNC) sc_n->state &= ~NODE_BLOCKSYNC;
                }
                client->stateflags &= ~SPV_FULLBLOCK_SYNC_FLAG;
                client->stateflags |= SPV_HEADER_SYNC_FLAG;
                client->nodegroup->log_write_cb(
                    "[spv] stale BLOCKSYNC (height=%d global_best=%d) — forcing header sync\n",
                    sc_tip->height, sc_best);
                dogecoin_net_spv_request_headers(client);
            }
        }
    }

    /* BIP157: recover from stalled awaiting_response when a peer disconnected
     * mid-download without clearing the flag. */
    if (client->compact_filters_enabled && client->cfilter_state) {
        dogecoin_compact_filter_state *cfstate = client->cfilter_state;
        dogecoin_bool cf_incomplete =
            ((uint32_t)pindex->height > 0) &&
            (cfstate->cfheaders_tip_height < (uint32_t)pindex->height ||
             cfstate->filters_tip_height  < cfstate->cfheaders_tip_height);
        /* Only retry here when not already in a live parallel cfilter download. */
        dogecoin_bool par_active = (cfstate->cfh_par_n > 0) ||
            (cfstate->par_num_workers > 0 &&
             cfstate->filters_tip_height < cfstate->cfheaders_tip_height);
        /* Par cfilter stall recovery: if a worker has been assigned longer than
         * CF_RESPONSE_TIMEOUT and has received nothing, free the slot and
         * re-assign from the next available node. */
        if (cfstate->par_num_workers > 0 && cfstate->par_bufs) {
            int64_t stall_deadline = *now - CF_RESPONSE_TIMEOUT;
            uint8_t pi;
            for (pi = 0; pi < cfstate->par_num_workers; pi++) {
                cf_par_buf *buf = &cfstate->par_bufs[pi];
                if (buf->node_id == -1 || buf->complete) continue;
                if (buf->received > 0) continue; /* making progress */
                if (buf->assign_time == 0 || buf->assign_time > stall_deadline) continue;

                client->nodegroup->log_write_cb(
                    "[bip157-par] worker slot %u (node %d, [%u..%u]) stalled — reassigning\n",
                    (unsigned int)pi, buf->node_id, buf->batch_start, buf->batch_end);

                /* Reset slot and re-issue from the stalled start height */
                int stalled_node_id = buf->node_id;
                uint32_t retry_start = buf->batch_start;
                dogecoin_free(buf->records);
                buf->records  = NULL;
                buf->node_id  = -1;
                buf->complete = false;
                buf->received = 0;
                cfstate->par_next_height = retry_start; /* back up */

                /* Find any connected peer OTHER than the stalled one to take over */
                unsigned int ni;
                dogecoin_bool reassigned = false;
                for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                    dogecoin_node *rn = (dogecoin_node *)vector_idx(
                        client->nodegroup->nodes, ni);
                    if (!rn || !(rn->state & NODE_CONNECTED) || !rn->version_handshake)
                        continue;
                    if (rn->nodeid == stalled_node_id) continue; /* skip stalled peer */
                    if (spv_cf_par_assign(client, rn)) { reassigned = true; break; }
                }
                /* No alternative peer available — fall back to the stalled node */
                if (!reassigned) {
                    for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                        dogecoin_node *rn = (dogecoin_node *)vector_idx(
                            client->nodegroup->nodes, ni);
                        if (!rn || !(rn->state & NODE_CONNECTED) || !rn->version_handshake)
                            continue;
                        if (spv_cf_par_assign(client, rn)) break;
                    }
                }
            }
        }

        if (cf_incomplete && !par_active && cfstate->awaiting_response) {
            /* Fast-path: if no CF-capable peer is connected right now, the peer
             * we sent the request to has disconnected.  Clear awaiting_response
             * immediately so the postcmd trigger can fire as soon as any CF peer
             * reconnects — no need to wait the full CF_RESPONSE_TIMEOUT. */
            dogecoin_bool any_cf_connected = false;
            for (unsigned int ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                dogecoin_node *n = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
                if ((n->state & NODE_CONNECTED) &&
                    (n->services & DOGECOIN_NODE_COMPACT_FILTERS)) {
                    any_cf_connected = true;
                    break;
                }
            }
            if (!any_cf_connected) {
                client->nodegroup->log_write_cb(
                    "[bip157] CF peer gone — clearing awaiting_response to retry on reconnect\n");
                cfstate->awaiting_response = false;
            } else if (cfstate->last_request_time > 0 &&
                       *now > cfstate->last_request_time + CF_RESPONSE_TIMEOUT) {
                /* Full timeout: CF peer is connected but not responding. */
                client->nodegroup->log_write_cb(
                    "[bip157] CF response timeout after %us — retrying\n",
                    (unsigned int)(*now - cfstate->last_request_time));
                cfstate->awaiting_response = false;
                dogecoin_node *cf_node = NULL;
                for (unsigned int ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                    dogecoin_node *n = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
                    if ((n->state & NODE_CONNECTED) &&
                        (n->services & DOGECOIN_NODE_COMPACT_FILTERS)) {
                        cf_node = n;
                        break;
                    }
                }
                if (cf_node)
                    dogecoin_spv_request_cfcheckpt(client, cf_node);
            }
        }
    }

    client->last_statecheck_time = *now;
}

/**
 * This function is called by the dogecoin_node_timer_callback function.
 *
 * It checks if the last_statecheck_time is greater than the minimum time delta for state checks.
 *
 * If it is, it calls the dogecoin_net_spv_periodic_statecheck function.
 *
 * The dogecoin_net_spv_periodic_statecheck function checks if the node is connected to the network.
 *
 * @param node The node that the timer is being called on.
 * @param now the current time in seconds since the epoch
 *
 * @return A boolean value.
 */
static dogecoin_bool dogecoin_net_spv_node_timer_callback(dogecoin_node *node, uint64_t *now)
{
    dogecoin_spv_client *client = (dogecoin_spv_client*)node->nodegroup->ctx;

    if (client->last_statecheck_time + MIN_TIME_DELTA_FOR_STATE_CHECK < *now)
    {
        dogecoin_net_spv_periodic_statecheck(node, now);
    }

    return true;
}

/**
 * Fill up the blocklocators vector_t with the blocklocators from the headers database
 *
 * @param client the spv client
 * @param blocklocators a vector_t of block hashes that we want to scan from
 *
 * @return The blocklocators are being returned.
 */
void dogecoin_net_spv_fill_block_locator(dogecoin_spv_client *client, vector_t *blocklocators) {
    int64_t min_timestamp = client->oldest_item_of_interest - BLOCK_GAP_TO_DEDUCT_TO_START_SCAN_FROM * BLOCKS_DELTA_IN_S; /* ensure we going back ~300 blocks */
    if (client->headers_db->getchaintip(client->headers_db_ctx)->height == 0) {
        if (client->use_checkpoints && client->oldest_item_of_interest > BLOCK_GAP_TO_DEDUCT_TO_START_SCAN_FROM * BLOCKS_DELTA_IN_S) {
            dogecoin_bool is_main = (client->chainparams && strcmp(client->chainparams->chainname, "main") == 0);
            const dogecoin_checkpoint *checkpoint = is_main ? dogecoin_mainnet_checkpoint_array : dogecoin_testnet_checkpoint_array;
            size_t mainnet_checkpoint_size = dogecoin_mainnet_checkpoint_count;
            size_t testnet_checkpoint_size = dogecoin_testnet_checkpoint_count;
            size_t length = is_main ? mainnet_checkpoint_size : testnet_checkpoint_size;
            int i;
            for (i = (int)length - 1; i >= 0; i--) {
                if (checkpoint[i].timestamp < min_timestamp) {
                    uint256_t *hash = dogecoin_calloc(1, sizeof(uint256_t));
                    utils_uint256_sethex((char *)checkpoint[i].hash, (uint8_t *)hash);
                    vector_add(blocklocators, (void *)hash);
                    if (!client->headers_db->has_checkpoint_start(client->headers_db_ctx)) {
                        arith_uint256 checkpoint_chainwork;
                        uint_to_arith(&checkpoint_chainwork, &client->chainparams->minimumchainwork);
                        client->headers_db->set_checkpoint_start(client->headers_db_ctx, *hash, checkpoint[i].height, checkpoint_chainwork);
                    }
                }
            }
            if (blocklocators->len > 0) return; // return if we could fill up the blocklocator with checkpoints
        }
        uint256_t *hash = dogecoin_calloc(1, sizeof(uint256_t));
        memcpy_safe(hash, &client->chainparams->genesisblockhash, sizeof(uint256_t));
        vector_add(blocklocators, (void *)hash);
        client->nodegroup->log_write_cb("Setting blocklocator with genesis block\n");
    } else {
        client->headers_db->fill_blocklocator_tip(client->headers_db_ctx, blocklocators);
    }
}

/**
 * This function is called when a node is in headers sync state. It will request the next block headers
 * from the node
 *
 * @param node The node that is requesting headers or blocks.
 * @param blocks boolean, true if we want to request blocks, false if we want to request headers
 */

void dogecoin_net_spv_node_request_headers_or_blocks(dogecoin_node *node, dogecoin_bool blocks)
{
    // request next headers
    vector_t *blocklocators = vector_new(1, free);

    dogecoin_net_spv_fill_block_locator((dogecoin_spv_client *)node->nodegroup->ctx, blocklocators);

    cstring *getheader_msg = cstr_new_sz(256);
    dogecoin_p2p_msg_getheaders(blocklocators, NULL, getheader_msg);

    cstring *p2p_msg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, (blocks ? DOGECOIN_MSG_GETBLOCKS : DOGECOIN_MSG_GETHEADERS), getheader_msg->str, getheader_msg->len);
    cstr_free(getheader_msg, true);

    dogecoin_node_send(node, p2p_msg);
    node->state |= ( blocks ? NODE_BLOCKSYNC : NODE_HEADERSYNC);

    if (blocks) {
        node->time_last_request = time(NULL);
    } else {
        ((dogecoin_spv_client*)node->nodegroup->ctx)->last_headersrequest_time = time(NULL);
    }

    vector_free(blocklocators, true);
    cstr_free(p2p_msg, true);
}

/**
 * If we have not yet reached the height of the blockchain tip, we request headers from a peer. If we
 * have reached the height of the blockchain tip, we request blocks from a peer
 *
 * @param client the spv client
 *
 * @return dogecoin_bool
 */
/* True while compact-filter sync can still make progress: a peer advertising
 * NODE_COMPACT_FILTERS is connected, or matched blocks from a rescan are still
 * being fetched.  Sync completion is deferred to the filter path only in that
 * case.  When filters are enabled (the default) but no peer serves them, nothing
 * would ever advance the CF state machine, and gating completion purely on
 * compact_filters_enabled left the client waiting forever -- `spvnode scan`
 * never printed "Sync completed" and never exited. */
static dogecoin_bool spv_cf_sync_pending(dogecoin_spv_client *client)
{
    if (!client->compact_filters_enabled || !client->cfilter_state) return false;
    if (client->cfilter_state->cf_block_fetch_active) return true;
    if (!client->nodegroup || !client->nodegroup->nodes) return false;
    unsigned int i;
    for (i = 0; i < client->nodegroup->nodes->len; i++) {
        dogecoin_node *n = (dogecoin_node *)vector_idx(client->nodegroup->nodes, i);
        if (n && (n->state & NODE_CONNECTED) && (n->services & DOGECOIN_NODE_COMPACT_FILTERS))
            return true;
    }
    return false;
}

dogecoin_bool dogecoin_net_spv_request_headers(dogecoin_spv_client *client)
{
    /* Parallel genesis headers in progress -- don't interfere. */
    if (client->par_hdr && client->par_hdr->active) return true;

    size_t i;
    dogecoin_bool new_headers_available = false;
    for(i = 0; i < client->nodegroup->nodes->len; ++i)
    {
        dogecoin_node *check_node = vector_idx(client->nodegroup->nodes, i);
        if (((check_node->state & NODE_HEADERSYNC) == NODE_HEADERSYNC || (check_node->state & NODE_BLOCKSYNC) == NODE_BLOCKSYNC) && (check_node->state & NODE_CONNECTED) == NODE_CONNECTED) { return true; }
    }

    // If in header or block sync state, request headers or blocks from the node with the longest chain
    if ((client->stateflags & SPV_HEADER_SYNC_FLAG) == SPV_HEADER_SYNC_FLAG || (client->stateflags & SPV_FULLBLOCK_SYNC_FLAG) == SPV_FULLBLOCK_SYNC_FLAG)
    {
        dogecoin_node *node_with_longest_chain = NULL;
        unsigned int longest_chain_height = 0;
        for(i = 0; i < client->nodegroup->nodes->len; ++i)
        {
            dogecoin_node *check_node = vector_idx(client->nodegroup->nodes, i);
        if (((check_node->state & NODE_CONNECTED) == NODE_CONNECTED) && check_node->version_handshake)
            {
                if (check_node->bestknownheight > longest_chain_height)
                {
                    longest_chain_height = check_node->bestknownheight;
                    node_with_longest_chain = check_node;
                }
            }
        }

        // Request headers or blocks from the node with the longest chain
        if (node_with_longest_chain != NULL) {
            dogecoin_net_spv_node_request_headers_or_blocks(node_with_longest_chain, (client->stateflags & SPV_FULLBLOCK_SYNC_FLAG) == SPV_FULLBLOCK_SYNC_FLAG);
            new_headers_available = true;
        }
    }

    // Fallback: original logic for handling cases where no suitable node was found
    unsigned int nodes_at_same_height = 0;
    if (!new_headers_available && client->headers_db->getchaintip(client->headers_db_ctx)->header.timestamp < client->oldest_item_of_interest - (BLOCK_GAP_TO_DEDUCT_TO_START_SCAN_FROM * BLOCKS_DELTA_IN_S) && client->stateflags == SPV_HEADER_SYNC_FLAG)
    {
        for(i = 0; i < client->nodegroup->nodes->len; i++)
        {
            dogecoin_node *check_node = vector_idx(client->nodegroup->nodes, i);
            if (((check_node->state & NODE_CONNECTED) == NODE_CONNECTED) && check_node->version_handshake)
            {
                if (check_node->bestknownheight > client->headers_db->getchaintip(client->headers_db_ctx)->height) {
                    dogecoin_net_spv_node_request_headers_or_blocks(check_node, false);
                    new_headers_available = true;
                    break;
                } else if (check_node->bestknownheight == client->headers_db->getchaintip(client->headers_db_ctx)->height) {
                    nodes_at_same_height++;
                }
            }
        }
    }
    if (!new_headers_available && (dogecoin_node_group_amount_of_connected_nodes(client->nodegroup, NODE_CONNECTED) > 0) && client->stateflags == SPV_FULLBLOCK_SYNC_FLAG) {
        // try to fetch blocks if no new headers are available but connected nodes are reachable
        for(i = 0; i< client->nodegroup->nodes->len; i++)
        {
            dogecoin_node *check_node = vector_idx(client->nodegroup->nodes, i);
            if (((check_node->state & NODE_CONNECTED) == NODE_CONNECTED) && check_node->version_handshake)
            {
                if (check_node->bestknownheight == client->headers_db->getchaintip(client->headers_db_ctx)->height) {
                    nodes_at_same_height++;
                }
                dogecoin_net_spv_node_request_headers_or_blocks(check_node, true);
                new_headers_available = true;
            }
        }
    }

    if (nodes_at_same_height >= COMPLETED_WHEN_NUM_NODES_AT_SAME_HEIGHT && !client->called_sync_completed && client->sync_completed
        && !spv_cf_sync_pending(client))
    {
        client->sync_completed(client);
        client->called_sync_completed = true;
    }

    return new_headers_available;
}

/**
 * When the handshake is done, we request the headers
 *
 * @param node The node that just completed the handshake.
 */
void dogecoin_net_spv_node_handshake_done(dogecoin_node *node)
{
    dogecoin_spv_client* client = (dogecoin_spv_client*)node->nodegroup->ctx;

    /* If a BIP37 filter is configured, load it on this peer before requesting blocks. */
    if (client && client->bloom_filter && client->bloom_filter_len > 0) {
        if (spv_send_filterload_to_node(node,
                                        client->bloom_filter,
                                        client->bloom_filter_len,
                                        client->bloom_nhashfunc,
                                        client->bloom_ntweak,
                                        client->bloom_flags) &&
            client->nodegroup && client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb("[spv] sent filterload to node %d (len=%u)\n", node->nodeid, client->bloom_filter_len);
        }
    }

    /* Header download: a segment of the parallel genesis sweep when it is
       running, the serial getheaders walk otherwise. The cfheaders workers
       below are a separate pool and are assigned either way. */
    if (client && client->par_hdr && client->par_hdr->active)
        par_hdr_assign(client, node);
    else
        dogecoin_net_spv_request_headers(client);

    if (client && client->compact_filters_enabled && client->cfilter_state &&
        client->cfilter_state->cfh_par_n > 0 &&
        (node->services & DOGECOIN_NODE_COMPACT_FILTERS))
        cfh_par_assign(client, node);
}

/**
 * The function is called when a new message is received from a peer
 *
 * @param node
 * @param hdr
 * @param buf
 *
 * @return Nothing.
 */
void dogecoin_net_spv_post_cmd(dogecoin_node *node, dogecoin_p2p_msg_hdr *hdr, struct const_buffer *buf)
{
    dogecoin_spv_client *client = (dogecoin_spv_client *)node->nodegroup->ctx;

    if (strcmp(hdr->command, DOGECOIN_MSG_INV) == 0 && (node->state & NODE_BLOCKSYNC) == NODE_BLOCKSYNC)
    {
        struct const_buffer original_inv = { buf->p, buf->len };
        uint32_t varlen;
        deser_varlen(&varlen, buf);
        dogecoin_bool contains_block = false;
        dogecoin_bool contains_tx = false;

        client->nodegroup->log_write_cb("Get inv request with %d items\n", varlen);

        unsigned int i;
        for (i = 0; i < varlen; i++)
        {
            uint32_t type;
            deser_u32(&type, buf);
            if (type == DOGECOIN_INV_TYPE_BLOCK && ((varlen >= 500) || (client->headers_db->getchaintip(client->headers_db_ctx)->height > node->bestknownheight - 1440))) {
                contains_block = true;
                deser_u256(node->last_requested_inv, buf);
           } else if (type == DOGECOIN_INV_TYPE_TX) {
                contains_tx = true;
                deser_skip(buf, 32);
            } else {
                deser_skip(buf, 32);
            }
        }

        if (contains_block) {
            node->time_last_request = time(NULL);

            dogecoin_bool use_filtered = (client->bloom_filter && client->bloom_filter_len > 0);

            if (use_filtered) {
                /* Rebuild getdata payload, rewriting BLOCK -> FILTERED_BLOCK so peer answers:
                   merkleblock + (matched) tx messages, and we drive sync_transaction only for matches. */
                uint8_t* out = NULL;
                uint32_t out_len = 0;
                uint32_t n = 0;
                if (!dogecoin_bip37_build_filtered_getdata_payload(&original_inv, &out, &out_len, &n)) {
                    node->state &= ~NODE_BLOCKSYNC;
                    node->nodegroup->node_connection_state_changed_cb(node);
                    return;
                }

                client->nodegroup->log_write_cb("Requesting %d filtered blocks (merkleblock+tx)\n", n);
                cstring *p2p_msg = dogecoin_p2p_message_new(
                    node->nodegroup->chainparams->netmagic,
                    DOGECOIN_MSG_GETDATA,
                    out,
                    out_len
                );
                dogecoin_node_send(node, p2p_msg);
                cstr_free(p2p_msg, true);
                dogecoin_free(out);
            } else {
                client->nodegroup->log_write_cb("Requesting %d blocks\n", varlen);
                cstring *p2p_msg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_GETDATA, original_inv.p, original_inv.len);
                dogecoin_node_send(node, p2p_msg);
                cstr_free(p2p_msg, true);
            }
        }

        if (contains_tx && client->smpv_enabled) {
            client->nodegroup->log_write_cb("Requesting %d tx (mempool INV)\n", varlen);
            cstring *p2p_msg = dogecoin_p2p_message_new(
                node->nodegroup->chainparams->netmagic,
                DOGECOIN_MSG_GETDATA,
                original_inv.p, original_inv.len);
            dogecoin_node_send(node, p2p_msg);
            cstr_free(p2p_msg, true);
        }
    }

    if (strcmp(hdr->command, DOGECOIN_MSG_BLOCK) == 0)
    {
        dogecoin_bool connected;
        dogecoin_blockindex *pindex = client->headers_db->connect_hdr(client->headers_db_ctx, buf, false, &connected);

        node->time_last_request = time(NULL);

        if (connected) {
            if (client->header_connected) { client->header_connected(client); }

            // for now, turn of stall checks if we are near the tip
            if (pindex->header.timestamp > node->time_last_request - 30*60) {
                node->time_last_request = 0;
            }

            time_t lasttime = pindex->header.timestamp;
            char s[1000];
            time_t t = lasttime;
            struct tm tmv;
            struct tm *p = dogecoin_localtime(&t, &tmv);
            if (p) strftime(s, sizeof s, "%Y-%m-%d %H:%M:%S", p);
            else s[0] = '\0';
            char *ctime_no_newline;
            ctime_no_newline = strtok(s, "\n");
            printf("%s|%" PRIu32 "|%s|%" PRIu32 "\n", hash_to_string(pindex->hash), pindex->height, ctime_no_newline, hdr->data_len);
            uint64_t start = time(NULL);

            uint32_t amount_of_txs;
            if (!deser_varlen(&amount_of_txs, buf)) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
                client->nodegroup->log_write_cb("Error deserializing amount of transactions from node %d\n", node->nodeid);
                node->state &= ~NODE_BLOCKSYNC;
                node->nodegroup->node_connection_state_changed_cb(node);
                return;
            }

            client->nodegroup->log_write_cb("Start parsing %d transactions...\n", (int)amount_of_txs);

            // update the last block info for the client
            client->last_block_tx_count = amount_of_txs;
            client->last_block_size = hdr->data_len;

            uint64_t total_tx_size = 0;

            // per-block accumulation for stats
            uint64_t block_outputs_value = 0;
            uint32_t block_outputs_count = 0;
            uint64_t coinbase_value = 0;

            size_t consumedlength = 0;
            unsigned int i;
            for (i = 0; i < amount_of_txs; i++)
            {
                const unsigned char* tx_raw = (const unsigned char*)buf->p;

                dogecoin_tx* tx = dogecoin_tx_new();
                if (!dogecoin_tx_deserialize(buf->p, buf->len, tx, &consumedlength)) {
                    client->nodegroup->log_write_cb("Error deserializing transaction\n");
                    if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                        dogecoin_free(pindex);
                    }
                    dogecoin_tx_free(tx);
                    node->state &= ~NODE_BLOCKSYNC;
                    node->nodegroup->node_connection_state_changed_cb(node);
                    return;
                }

                deser_skip(buf, consumedlength);

                // update smpv status for this tx as confirmed in a block
                if (client->smpv_enabled && client->smpv_ctx) {
                    uint256_t h;
                    dogecoin_dblhash(tx_raw, consumedlength, h);
                    char txid_hex[65];
                    utils_bin_to_hex((unsigned char*)h, 32, txid_hex);
                    utils_reverse_hex(txid_hex, 64);
                    char block_hash_hex[65];
                    utils_bin_to_hex((unsigned char*)pindex->hash, 32, block_hash_hex);
                    utils_reverse_hex(block_hash_hex, 64);
                    dogecoin_smpv_update_tx_status(
                        (dogecoin_smpv_client*)client->smpv_ctx,
                        txid_hex,
                        true,
                        block_hash_hex,
                        (uint32_t)pindex->height
                    );
                }

                if (client->sync_transaction) { client->sync_transaction(client->sync_transaction_ctx, tx, i, pindex); }
                
#if defined(USE_LIBOQS) || defined(USE_RACCOON_G)
                /* --- Phase 1: Detect OP_RETURN commitments --- */
#ifdef USE_LIBOQS
                /* Falcon-512: buffer for cross-TX carrier match (TX_C → pending, TX_R validates) */
                uint8_t falcon_commit_data[32];
                if (dogecoin_tx_extract_falcon512_commit(tx, falcon_commit_data)) {
                    char falcon_commit_hex[65];
                    utils_bin_to_hex(falcon_commit_data, 32, falcon_commit_hex);
                    client->nodegroup->log_write_cb("[falcon-commit] Pending at height=%d txpos=%u commit=%s source=op_return\n",
                                                     pindex->height, i, falcon_commit_hex);
                    spv_pqc_buffer_pending(client, falcon_commit_data,
                                           DOGECOIN_PQC_ALGO_FALCON,
                                           i, pindex->height,
                                           tx_raw, consumedlength,
                                           "falcon-commit");
                }
                /* Dilithium2: buffer for cross-TX carrier match (TX_C → pending, TX_R validates via multi-part carrier) */
                uint8_t dilithium_commit_data[32];
                if (dogecoin_tx_extract_dilithium2_commit(tx, dilithium_commit_data)) {
                    char dilithium_commit_hex[65];
                    utils_bin_to_hex(dilithium_commit_data, 32, dilithium_commit_hex);
                    client->nodegroup->log_write_cb("[dilithium-commit] Pending at height=%d txpos=%u commit=%s source=op_return\n",
                                                     pindex->height, i, dilithium_commit_hex);
                    spv_pqc_buffer_pending(client, dilithium_commit_data,
                                           DOGECOIN_PQC_ALGO_DILITHIUM,
                                           i, pindex->height,
                                           tx_raw, consumedlength,
                                           "dilithium-commit");
                }
                /* Raccoon-G-44: buffer for cross-TX carrier match (TX_C → pending, TX_R validates via multi-part carrier) */
#endif /* USE_LIBOQS Falcon/Dilithium Phase 1 */
#ifdef USE_RACCOON_G
                uint8_t raccoong_commit_data[32];
                if (dogecoin_tx_extract_raccoong44_commit(tx, raccoong_commit_data)) {
                    char raccoong_commit_hex[65];
                    utils_bin_to_hex(raccoong_commit_data, 32, raccoong_commit_hex);
                    client->nodegroup->log_write_cb("[raccoong-commit] Pending at height=%d txpos=%u commit=%s source=op_return\n",
                                                     pindex->height, i, raccoong_commit_hex);
                    spv_pqc_buffer_pending(client, raccoong_commit_data,
                                           DOGECOIN_PQC_ALGO_RACCOONG,
                                           i, pindex->height,
                                           tx_raw, consumedlength,
                                           "raccoong-commit");
                }
#endif

                /* --- Phase 2: Scan every TX for carrier-format scriptSigs (cross-TX carrier match) --- */
                {
                    dogecoin_pqc_algo_t carrier_algo;
                    const uint8_t* carrier_pk = NULL;
                    const uint8_t* carrier_sig = NULL;
                    size_t carrier_pk_len = 0, carrier_sig_len = 0, carrier_vin = 0;
                    uint8_t* carrier_buf = NULL;
                    size_t carrier_buf_len = 0;
                    if (dogecoin_pqc_carrier_extract_scriptsig(tx, &carrier_algo, &carrier_pk, &carrier_pk_len,
                                                      &carrier_sig, &carrier_sig_len, &carrier_vin,
                                                      &carrier_buf, &carrier_buf_len)) {
                        /* Compute TX_R (reveal) txid for logging (display byte order) */
                        uint256_t txr_hash;
                        dogecoin_tx_hash(tx, txr_hash);
                        uint8_t txr_hash_rev[32];
                        for (int rb = 0; rb < 32; rb++) txr_hash_rev[rb] = ((uint8_t*)txr_hash)[31 - rb];
                        char txr_txid_hex[65];
                        utils_bin_to_hex(txr_hash_rev, 32, txr_txid_hex);

                        uint8_t computed_commit[32];
                        const char* algo_label = (carrier_algo == DOGECOIN_PQC_ALGO_FALCON) ? "falcon-commit" :
                                                 (carrier_algo == DOGECOIN_PQC_ALGO_DILITHIUM) ? "dilithium-commit" :
#ifdef USE_RACCOON_G
                                                 (carrier_algo == DOGECOIN_PQC_ALGO_RACCOONG) ? "raccoong-commit" :
#endif
                                                 "unknown-pqc";
                        dogecoin_bool commit_ok = false;
#ifdef USE_LIBOQS
                        if (carrier_algo == DOGECOIN_PQC_ALGO_FALCON)
                            commit_ok = dogecoin_falcon512_commit_bytes(carrier_pk, carrier_pk_len, carrier_sig, carrier_sig_len, computed_commit);
                        else if (carrier_algo == DOGECOIN_PQC_ALGO_DILITHIUM)
                            commit_ok = dogecoin_dilithium2_commit_bytes(carrier_pk, carrier_pk_len, carrier_sig, carrier_sig_len, computed_commit);
#endif
#ifdef USE_RACCOON_G
                        if (carrier_algo == DOGECOIN_PQC_ALGO_RACCOONG)
                            commit_ok = dogecoin_raccoong44_commit_bytes(carrier_pk, carrier_pk_len, carrier_sig, carrier_sig_len, computed_commit);
#endif

                        if (commit_ok) {
                            char commit_hex[65];
                            utils_bin_to_hex(computed_commit, 32, commit_hex);
                            char pk_prefix_hex[33] = {0};
                            size_t pk_prefix_len = carrier_pk_len < 16 ? carrier_pk_len : 16;
                            utils_bin_to_hex((unsigned char*)carrier_pk, pk_prefix_len, pk_prefix_hex);
                            char sig_prefix_hex[33] = {0};
                            size_t sig_prefix_len = carrier_sig_len < 16 ? carrier_sig_len : 16;
                            utils_bin_to_hex((unsigned char*)carrier_sig, sig_prefix_len, sig_prefix_hex);

                            /* Cross-validate: O(1) hash table lookup for matching OP_RETURN commitment */
                            spv_pqc_pending_commit_t* matched_entry = spv_pqc_find_pending(client, computed_commit);
                            dogecoin_bool matched = (matched_entry != NULL && matched_entry->algo == carrier_algo);
                            uint32_t matched_txpos = 0;
                            uint8_t* matched_txc_raw = NULL;
                            size_t matched_txc_raw_len = 0;
                            if (matched) {
                                matched_txpos = matched_entry->txpos;
                                matched_txc_raw = matched_entry->txc_raw;
                                matched_txc_raw_len = matched_entry->txc_raw_len;
                                /* Remove from hash table but don't free txc_raw yet (used below) */
                                matched_entry->txc_raw = NULL;
                                {
                                    spv_pqc_pending_commit_t* head = spv_pqc_table(client);
                                    HASH_DEL(head, matched_entry);
                                    client->pqc_pending_commits = head;
                                }
                                dogecoin_free(matched_entry);
                            }

                            if (matched) {
                                client->nodegroup->log_write_cb("[%s] Valid at height=%d txpos=%u commit=%s carrier_vin=%zu source=carrier_scriptsig matched_txc_txpos=%u pk_len=%zu sig_len=%zu pk_prefix=%s sig_prefix=%s txr_txid=%s\n",
                                                                 algo_label, pindex->height, i, commit_hex, carrier_vin, matched_txpos, carrier_pk_len, carrier_sig_len, pk_prefix_hex, sig_prefix_hex, txr_txid_hex);

                                /* Phase 2: Verify PQC signature over TX_C sighash32 (via pqc_carrier module) */
                                if (matched_txc_raw && matched_txc_raw_len > 0) {
                                    uint8_t verify_sighash[32] = {0};
                                    dogecoin_bool sig_verified = dogecoin_pqc_carrier_verify_reveal(
                                        (dogecoin_pqc_algo_t)carrier_algo,
                                        matched_txc_raw, matched_txc_raw_len,
                                        carrier_pk, carrier_pk_len,
                                        carrier_sig, carrier_sig_len,
                                        verify_sighash);
                                    char sighash_hex[65];
                                    utils_bin_to_hex(verify_sighash, 32, sighash_hex);
                                    client->nodegroup->log_write_cb("[%s] PQC signature verification %s at height=%d txpos=%u sighash=%s\n",
                                        algo_label, sig_verified ? "PASSED" : "FAILED", pindex->height, i, sighash_hex);
                                    if (sig_verified) {
                                        client->nodegroup->log_write_cb("[%s] Reveal validated: TX_R=%s commit=%s pk_len=%zu sig_len=%zu height=%d\n",
                                            algo_label, txr_txid_hex, commit_hex, carrier_pk_len, carrier_sig_len, pindex->height);
                                    }
                                    dogecoin_free(matched_txc_raw);
                                }
                            } else {
                                client->nodegroup->log_write_cb("[%s] Unmatched at height=%d txpos=%u commit=%s carrier_vin=%zu source=carrier_scriptsig pk_len=%zu sig_len=%zu pk_prefix=%s sig_prefix=%s txr_txid=%s\n",
                                                                 algo_label, pindex->height, i, commit_hex, carrier_vin, carrier_pk_len, carrier_sig_len, pk_prefix_hex, sig_prefix_hex, txr_txid_hex);
                            }
                        } else {
                            client->nodegroup->log_write_cb("[%s] carrier found but commit_bytes failed at height=%d txpos=%u carrier_vin=%zu pk_len=%zu sig_len=%zu buf_len=%zu\n",
                                                             algo_label, pindex->height, i, carrier_vin, carrier_pk_len, carrier_sig_len, carrier_buf_len);
                        }
                        if (carrier_buf) dogecoin_free(carrier_buf);
                    }
                }
#endif

#ifdef USE_ZK_CARRIER
                /* --- ZK Phase 1: Detect TX_C OP_RETURN commitments --- */
                {
                    dogecoin_zk_mode_t zk_mode = DOGECOIN_ZK_MODE_GROTH16;
                    uint8_t zk_commit_data[32];
                    if (dogecoin_tx_extract_zk_commit(tx, &zk_mode, zk_commit_data)) {
                        char zk_commit_hex[65];
                        utils_bin_to_hex(zk_commit_data, 32, zk_commit_hex);
                        client->nodegroup->log_write_cb("[zk-commit] Pending at height=%d txpos=%u commit=%s mode=%u source=op_return\n",
                                                         pindex->height, i, zk_commit_hex, (unsigned)zk_mode);
                        spv_zk_buffer_pending(client, zk_commit_data, zk_mode,
                                              i, pindex->height,
                                              tx_raw, consumedlength);
                    }
                }

                /* --- ZK Phase 2: Detect TX_R carrier scriptSigs and validate --- */
                {
                    uint8_t* zk_payload = NULL;
                    size_t zk_payload_len = 0;
                    if (dogecoin_zk_extract_carrier_payload(tx, &zk_payload, &zk_payload_len) == DOGECOIN_ZK_OK
                        && zk_payload && zk_payload_len > 0) {
                        /* Compute TX_R (reveal) txid for logging (display byte order) */
                        uint256_t txr_hash;
                        dogecoin_tx_hash(tx, txr_hash);
                        uint8_t txr_hash_rev[32];
                        for (int rb = 0; rb < 32; rb++) txr_hash_rev[rb] = ((uint8_t*)txr_hash)[31 - rb];
                        char txr_txid_hex[65];
                        utils_bin_to_hex(txr_hash_rev, 32, txr_txid_hex);

                        uint8_t computed_commit[32];
                        if (dogecoin_zk_get_commitment_hash(zk_payload, zk_payload_len, computed_commit) == DOGECOIN_ZK_OK) {
                            char commit_hex[65];
                            utils_bin_to_hex(computed_commit, 32, commit_hex);

                            spv_zk_pending_commit_t* matched_entry = spv_zk_find_pending(client, computed_commit);
                            if (matched_entry) {
                                uint32_t matched_txpos = matched_entry->txpos;
                                dogecoin_zk_mode_t matched_mode = matched_entry->mode;
                                /* Mirror the PQC path above: stash txc_raw / txc_raw_len
                                 * locally, NULL them out on the entry so spv_zk_remove_pending
                                 * doesn't free the buffer we still need for the tx_binding
                                 * recompute below, then drop the hash-table entry. The
                                 * stashed buffer is freed at the end of this branch. */
                                uint8_t* matched_txc_raw     = matched_entry->txc_raw;
                                size_t   matched_txc_raw_len = matched_entry->txc_raw_len;
                                matched_entry->txc_raw = NULL;
                                matched_entry->txc_raw_len = 0;
                                spv_zk_remove_pending(client, matched_entry);
                                matched_entry = NULL; /* poison: do NOT dereference past this point. */

                                client->nodegroup->log_write_cb("[zk-commit] Valid at height=%d txpos=%u commit=%s mode=%u source=carrier_scriptsig matched_txc_txpos=%u payload_len=%zu txr_txid=%s\n",
                                                                 pindex->height, i, commit_hex, (unsigned)matched_mode, matched_txpos, zk_payload_len, txr_txid_hex);

                                /* Fully decode and log every ZKP1 field for log-level auditability.
                                   The reveal payload format is:
                                       magic(4) || mode(1) || reserved(1) || circuit_id(4 BE) ||
                                       public_len(2 BE) || public_inputs[public_len] ||
                                       proof_len(4 BE)  || proof[proof_len]
                                   Each field is dumped as hex (with an ASCII preview when the
                                   bytes are printable, e.g. snarkjs JSON proofs). */
                                /* tx_binding outcome:
                                 *   0 = not checked (decode failed, no cached TX_C, sighash
                                 *       recompute failed, or fewer than 3 public inputs —
                                 *       legacy / payload predates tx-base binding)
                                 *   1 = matched
                                 *  -1 = mismatch (proof was lifted from another funding tx and
                                 *       replayed under this commit)
                                 *
                                 * Threaded out of the decode block so the gating below the
                                 * verify_proof call can refuse to log "Reveal validated" on a
                                 * mismatch.  Without this, a replayed historic proof with the
                                 * right commit32 would still print Reveal validated whenever
                                 * the verifier returned DELEGATED — negating the BIP's only
                                 * replay-protection mechanism for the no-in-process-verifier
                                 * (mobile / spv-only) build configuration. */
                                int zk_binding_state = 0;
                                {
                                    dogecoin_zk_mode_t dec_mode = (dogecoin_zk_mode_t)0;
                                    uint32_t dec_circuit_id = 0;
                                    const uint8_t* dec_pub = NULL; size_t dec_pub_len = 0;
                                    const uint8_t* dec_proof = NULL; size_t dec_proof_len = 0;
                                    const uint8_t* dec_vk = NULL; size_t dec_vk_len = 0;
                                    if (dogecoin_zk_decode_payload(zk_payload, zk_payload_len,
                                                                   &dec_mode, &dec_circuit_id,
                                                                   &dec_pub, &dec_pub_len,
                                                                   &dec_proof, &dec_proof_len,
                                                                   &dec_vk, &dec_vk_len) == DOGECOIN_ZK_OK) {
                                        uint8_t reserved_byte = zk_payload[DOGECOIN_ZK_CARRIER_MAGIC_LEN + 1];
                                        const char* mode_label =
                                            (dec_mode == DOGECOIN_ZK_MODE_GROTH16) ? "groth16-bn254" :
                                            (dec_mode == DOGECOIN_ZK_MODE_PLONK) ? "plonk" :
                                            (dec_mode == DOGECOIN_ZK_MODE_STARK_S2) ? "stark-s2" : "unknown";
                                        client->nodegroup->log_write_cb(
                                            "[zk-commit] reveal_decoded: magic=\"ZKP1\" mode=%u(%s) version=0x%02x circuit_id=0x%08x public_len=%zu proof_len=%zu vk_len=%zu total_payload_len=%zu txr_txid=%s\n",
                                            (unsigned)dec_mode, mode_label, (unsigned)reserved_byte,
                                            (unsigned)dec_circuit_id, dec_pub_len, dec_proof_len,
                                            dec_vk_len, zk_payload_len, txr_txid_hex);

                                        /* Helper: dump up to N bytes as hex + ASCII preview onto the log. */
                                        #define ZK_LOG_DUMP_FIELD(label, ptr, len) do { \
                                            const uint8_t* _p = (const uint8_t*)(ptr); size_t _l = (size_t)(len); \
                                            size_t _hex_max = 256; /* full dump up to 256 bytes; head+tail beyond */ \
                                            if (_l == 0) { \
                                                client->nodegroup->log_write_cb("[zk-commit] reveal_decoded.%s: len=0 (empty)\n", (label)); \
                                            } else if (_l <= _hex_max) { \
                                                char* _hex = (char*)dogecoin_calloc(1, _l * 2 + 1); \
                                                char* _asc = (char*)dogecoin_calloc(1, _l + 1); \
                                                if (_hex && _asc) { \
                                                    utils_bin_to_hex((unsigned char*)_p, _l, _hex); \
                                                    int _printable = 1; \
                                                    for (size_t _ai = 0; _ai < _l; _ai++) { \
                                                        unsigned char _c = _p[_ai]; \
                                                        if (_c == '\n' || _c == '\t' || _c == '\r') _asc[_ai] = ' '; \
                                                        else if (_c >= 0x20 && _c < 0x7f) _asc[_ai] = (char)_c; \
                                                        else { _asc[_ai] = '.'; _printable = 0; } \
                                                    } \
                                                    if (_printable) { \
                                                        client->nodegroup->log_write_cb("[zk-commit] reveal_decoded.%s: len=%zu hex=%s ascii=\"%s\"\n", (label), _l, _hex, _asc); \
                                                    } else { \
                                                        client->nodegroup->log_write_cb("[zk-commit] reveal_decoded.%s: len=%zu hex=%s\n", (label), _l, _hex); \
                                                    } \
                                                } \
                                                dogecoin_free(_hex); dogecoin_free(_asc); \
                                            } else { \
                                                /* Long field: emit head+tail hex (64 bytes each) plus ASCII head. */ \
                                                size_t _head = 64, _tail = 64; \
                                                char _hh[129] = {0}; char _tt[129] = {0}; \
                                                utils_bin_to_hex((unsigned char*)_p, _head, _hh); \
                                                utils_bin_to_hex((unsigned char*)(_p + _l - _tail), _tail, _tt); \
                                                size_t _ascii_n = _l < 96 ? _l : 96; \
                                                char _asc[97] = {0}; int _printable = 1; \
                                                for (size_t _ai = 0; _ai < _ascii_n; _ai++) { \
                                                    unsigned char _c = _p[_ai]; \
                                                    if (_c == '\n' || _c == '\t' || _c == '\r') _asc[_ai] = ' '; \
                                                    else if (_c >= 0x20 && _c < 0x7f) _asc[_ai] = (char)_c; \
                                                    else { _asc[_ai] = '.'; _printable = 0; } \
                                                } \
                                                if (_printable) { \
                                                    client->nodegroup->log_write_cb("[zk-commit] reveal_decoded.%s: len=%zu hex_head=%s hex_tail=%s ascii_head=\"%s\"\n", (label), _l, _hh, _tt, _asc); \
                                                } else { \
                                                    client->nodegroup->log_write_cb("[zk-commit] reveal_decoded.%s: len=%zu hex_head=%s hex_tail=%s\n", (label), _l, _hh, _tt); \
                                                } \
                                            } \
                                        } while (0)

                                        ZK_LOG_DUMP_FIELD("public_inputs", dec_pub, dec_pub_len);
                                        ZK_LOG_DUMP_FIELD("proof", dec_proof, dec_proof_len);
                                        if (dec_vk_len > 0) {
                                            ZK_LOG_DUMP_FIELD("vk", dec_vk, dec_vk_len);
                                        }

                                        /* tx_binding replay-protection check.
                                         * Mirroring the PQC carrier model where the signature is
                                         * over a tx_base sighash, the ZK proof's third snarkjs
                                         * public input is the same tx_base sighash (zero-top-byte
                                         * 248-bit BN254 field element).  Recompute it from the
                                         * matched TX_C bytes the SPV node cached during phase-1
                                         * pending-commit detection, parse the third element of the
                                         * snarkjs public.json array out of `dec_pub`, and compare.
                                         * A mismatch means the proof was lifted from another
                                         * funding tx and replayed under this commit — we log it
                                         * loudly so the operator (and any external auditor) can
                                         * see the binding match independent of the snarkjs verify
                                         * step. */
                                        if (matched_txc_raw && matched_txc_raw_len > 0) {
                                            dogecoin_tx* txc = dogecoin_tx_new();
                                            size_t txc_consumed = 0;
                                            if (txc && dogecoin_tx_deserialize(matched_txc_raw, matched_txc_raw_len, txc, &txc_consumed)) {
                                                cstring* signer_spk = dogecoin_zk_extract_signer_p2pkh_spk(txc);
                                                cstring* carrier_spk = NULL;
                                                /* Re-derive the canonical 23-byte P2SH carrier spk for
                                                 * the matched payload.  The carrier scriptPubKey is
                                                 * payload-independent — it only depends on the fixed
                                                 * carrier redeem script — so build it directly via the
                                                 * PQC helpers instead of allocating a throwaway tx and
                                                 * appending OP_RETURN + 0-value P2SH outputs to it. */
                                                cstring* carrier_redeem = NULL;
                                                if (dogecoin_pqc_carrier_build_redeemscript(&carrier_redeem) && carrier_redeem) {
                                                    if (!dogecoin_pqc_carrier_build_p2sh_scriptpubkey(carrier_redeem, &carrier_spk)) {
                                                        carrier_spk = NULL;
                                                    }
                                                    cstr_free(carrier_redeem, true);
                                                }
                                                if (signer_spk && carrier_spk) {
                                                    uint8_t recomputed[32];
                                                    if (dogecoin_zk_compute_tx_base_sighash(txc, signer_spk, carrier_spk, recomputed) == DOGECOIN_ZK_OK) {
                                                        char recomputed_hex[65] = {0};
                                                        utils_bin_to_hex(recomputed, 32, recomputed_hex);
                                                        /* Parse the snarkjs public-inputs array and pick out the
                                                         * 3rd quoted string (index 2) — the canonical tx_binding
                                                         * field element, see the BIP §"Phase 1: Base Transaction
                                                         * Binding".  Layout: ["...","...","<decimal>"]. */
                                                        uint8_t parsed[32] = {0};
                                                        size_t binding_token_count = 0;
                                                        dogecoin_zk_err_t bind_e = dogecoin_zk_parse_public_input_be32(
                                                            dec_pub, dec_pub_len, 2, parsed, &binding_token_count);
                                                        if (bind_e == DOGECOIN_ZK_OK) {
                                                            char parsed_hex[65] = {0};
                                                            utils_bin_to_hex(parsed, 32, parsed_hex);
                                                            int match = (memcmp(parsed, recomputed, 32) == 0);
                                                            zk_binding_state = match ? 1 : -1;
                                                            client->nodegroup->log_write_cb(
                                                                "[zk-commit] tx_binding %s txr_txid=%s recomputed=%s public_input[2]=%s\n",
                                                                match ? "match" : "mismatch",
                                                                txr_txid_hex, recomputed_hex, parsed_hex);
                                                        } else {
                                                            client->nodegroup->log_write_cb(
                                                                "[zk-commit] tx_binding skipped: expected >=3 public inputs (got %zu) — payload predates tx-base binding\n",
                                                                binding_token_count);
                                                        }
                                                    } else {
                                                        client->nodegroup->log_write_cb(
                                                            "[zk-commit] tx_binding skipped: tx_base sighash recompute failed for txr_txid=%s\n",
                                                            txr_txid_hex);
                                                    }
                                                } else {
                                                    client->nodegroup->log_write_cb(
                                                        "[zk-commit] tx_binding skipped: cannot extract signer/carrier spk from cached TX_C (txr_txid=%s)\n",
                                                        txr_txid_hex);
                                                }
                                                if (signer_spk) cstr_free(signer_spk, true);
                                                if (carrier_spk) cstr_free(carrier_spk, true);
                                            }
                                            if (txc) dogecoin_tx_free(txc);
                                        }

                                        #undef ZK_LOG_DUMP_FIELD
                                    } else {
                                        client->nodegroup->log_write_cb(
                                            "[zk-commit] reveal_decoded: decode_failed payload_len=%zu txr_txid=%s\n",
                                            zk_payload_len, txr_txid_hex);
                                    }
                                }

                                /* In-process proof verification uses only bytes carried by the
                                   reveal payload (v1 embedded vk). Without a compiled verifier
                                   this returns DOGECOIN_ZK_ERR_NOT_IMPLEMENTED / DELEGATED. */
                                dogecoin_zk_err_t verify_e = dogecoin_zk_verify_proof(
                                    zk_payload, zk_payload_len, NULL, 0);
                                const char* verify_status =
                                    (verify_e == DOGECOIN_ZK_OK) ? "PASSED" :
                                    (verify_e == DOGECOIN_ZK_ERR_VERIFY_FAIL) ? "FAILED" :
                                    (verify_e == DOGECOIN_ZK_ERR_NOT_IMPLEMENTED ||
                                     verify_e == DOGECOIN_ZK_ERR_DELEGATED) ? "DELEGATED" :
                                    "ERROR";
                                client->nodegroup->log_write_cb("[zk-commit] ZK verification %s at height=%d txpos=%u mode=%u err=%d\n",
                                    verify_status, pindex->height, i, (unsigned)matched_mode, (int)verify_e);

                                /* Reveal succeeds when the proof verified in-process, OR
                                   verification was delegated and the on-chain commit matched.
                                   When the binding check ran AND came back as a mismatch we
                                   refuse to log Reveal validated regardless of verify_e — a
                                   binding mismatch means the proof was lifted from another
                                   funding tx and replayed under this commit, which is the one
                                   thing this SPV-side check exists to catch.  When the binding
                                   check was skipped (legacy payload, decode failure, or the
                                   cached TX_C wasn't available) we fall back to the previous
                                   behaviour so v0 / pre-binding payloads still validate. */
                                if ((verify_e == DOGECOIN_ZK_OK ||
                                     verify_e == DOGECOIN_ZK_ERR_NOT_IMPLEMENTED ||
                                     verify_e == DOGECOIN_ZK_ERR_DELEGATED) &&
                                    zk_binding_state >= 0) {
                                    client->nodegroup->log_write_cb("[zk-commit] Reveal validated: TX_R=%s commit=%s payload_len=%zu mode=%u height=%d\n",
                                        txr_txid_hex, commit_hex, zk_payload_len, (unsigned)matched_mode, pindex->height);
                                } else if (zk_binding_state < 0) {
                                    client->nodegroup->log_write_cb("[zk-commit] Reveal REJECTED (tx_binding mismatch): TX_R=%s commit=%s payload_len=%zu mode=%u height=%d\n",
                                        txr_txid_hex, commit_hex, zk_payload_len, (unsigned)matched_mode, pindex->height);
                                }

                                /* Release the cached TX_C buffer we stashed before
                                 * spv_zk_remove_pending freed the hash-table entry. */
                                if (matched_txc_raw) dogecoin_free(matched_txc_raw);
                            } else {
                                client->nodegroup->log_write_cb("[zk-commit] Unmatched at height=%d txpos=%u commit=%s payload_len=%zu source=carrier_scriptsig txr_txid=%s\n",
                                                                 pindex->height, i, commit_hex, zk_payload_len, txr_txid_hex);
                            }
                        }
                        dogecoin_free(zk_payload);
                    }
                }
#endif /* USE_ZK_CARRIER */
                
                total_tx_size += consumedlength;

                // accumulate outputs for this tx
                uint64_t tx_out_sum = 0;
                unsigned int oi;
                for (oi = 0; oi < tx->vout->len; oi++)
                {
                    dogecoin_tx_out *txout = vector_idx(tx->vout, oi);
                    tx_out_sum += txout->value;
                    block_outputs_value += txout->value;
                    block_outputs_count++;
                }
                if (i == 0 && dogecoin_tx_is_coinbase(tx)) {
                    coinbase_value = tx_out_sum; // coinbase tx is always the first tx in a block
                }
                dogecoin_tx_free(tx);
            }
            client->last_block_total_tx_size = total_tx_size;

            // update smpv tip
            if (client->smpv_enabled && client->smpv_ctx) {
                dogecoin_smpv_tip_update(
                    (dogecoin_smpv_client*)client->smpv_ctx,
                    (uint32_t)pindex->height
                );
            }

            // approximate fees (OK for recent blocks where subsidy is 10k0 DOGE)
            uint64_t block_fees = 0;
            if (coinbase_value > DOGECOIN_CURRENT_SUBSIDY_KOINU) {
                block_fees = coinbase_value - DOGECOIN_CURRENT_SUBSIDY_KOINU;
            }

            // session totals
            client->stats_blocks_total++;
            client->stats_txs_total += amount_of_txs;
            client->stats_outputs_total += block_outputs_count;
            client->stats_out_value_total += block_outputs_value;
            client->stats_fees_total += block_fees;
            client->stats_block_bytes_total += hdr->data_len;

            // ring insert (latest)
            spv_block_sample *smp = &client->stats_ring[client->stats_ring_head];
            smp->ts = pindex->header.timestamp;
            smp->txs = amount_of_txs;
            smp->outputs = block_outputs_count;
            smp->out_value = block_outputs_value;
            smp->size = hdr->data_len;
            smp->fees = block_fees;

            client->stats_ring_head = (client->stats_ring_head + 1) % SPV_STATS_RING;
            if (client->stats_ring_len < SPV_STATS_RING) {
                client->stats_ring_len++;
            }

            client->nodegroup->log_write_cb("done (took %lld secs)\n", (unsigned long long)(time(NULL) - start));
        }
        else
        {
            /* BIP157 matched-block path: block already in headers DB; process its txns */
            dogecoin_compact_filter_state *cfstate_mb =
                (client->compact_filters_enabled && client->cfilter_state)
                    ? client->cfilter_state : NULL;
            dogecoin_bool is_cf_match = false;
            unsigned int cf_match_idx = 0;
            if (cfstate_mb && cfstate_mb->cf_block_fetch_active && pindex) {
                unsigned int mbi;
                for (mbi = 0; mbi < cfstate_mb->matched_block_hashes->len; mbi++) {
                    if (memcmp(vector_idx(cfstate_mb->matched_block_hashes, mbi),
                               pindex->hash, 32) == 0) {
                        is_cf_match = true;
                        cf_match_idx = mbi;
                        break;
                    }
                }
            }

            if (is_cf_match) {
                /* Get real in-memory pindex with correct height from B-tree.
                 * For historical blocks pruned from the tree, recover the height
                 * from the stored matched_block_heights vector instead. */
                dogecoin_headers_db *db_mb = (dogecoin_headers_db *)client->headers_db_ctx;
                dogecoin_blockindex *real_pindex = dogecoin_headersdb_find(db_mb, pindex->hash);
                dogecoin_bool pindex_is_heap = false; /* track if we need to free pindex */
                if (real_pindex) {
                    dogecoin_free(pindex); /* dummy from connect_hdr; tree owns real_pindex */
                    pindex = real_pindex;
                } else {
                    /* Block pruned from in-memory tree — recover height from stored value */
                    if (cfstate_mb->matched_block_heights &&
                        cf_match_idx < cfstate_mb->matched_block_heights->len) {
                        uint32_t *h = (uint32_t *)vector_idx(cfstate_mb->matched_block_heights, cf_match_idx);
                        if (h) pindex->height = *h;
                    }
                    pindex_is_heap = true; /* we own this allocation */
                }

                uint32_t amount_of_txs_cf = 0;
                if (!pindex || !deser_varlen(&amount_of_txs_cf, buf)) {
                    if (pindex_is_heap) dogecoin_free(pindex);
                    return;
                }

                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb(
                        "[bip157] processing %u txs from matched block height=%u [%us elapsed]\n",
                        amount_of_txs_cf, (unsigned)pindex->height, spv_elapsed(client));

                size_t cf_consumed = 0;
                unsigned int cfi;
                for (cfi = 0; cfi < amount_of_txs_cf; cfi++) {
                    dogecoin_tx *tx = dogecoin_tx_new();
                    if (!dogecoin_tx_deserialize(buf->p, buf->len, tx, &cf_consumed)) {
                        dogecoin_tx_free(tx);
                        break;
                    }
                    deser_skip(buf, cf_consumed);
                    if (client->sync_transaction)
                        client->sync_transaction(client->sync_transaction_ctx, tx, cfi, pindex);
                    dogecoin_tx_free(tx);
                }

                cfstate_mb->matched_blocks_fetched++;
                if (cfstate_mb->matched_blocks_fetched >=
                        (uint32_t)cfstate_mb->matched_block_hashes->len) {
                    cfstate_mb->cf_block_fetch_active = false;
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] all %u matched blocks processed [%us elapsed]\n",
                            cfstate_mb->matched_blocks_fetched, spv_elapsed(client));
                    if (!client->called_sync_completed && client->sync_completed) {
                        if (client->smpv_enabled) dogecoin_net_spv_request_mempool(client);
                        client->sync_completed(client);
                        client->called_sync_completed = true;
                    }
                }
                if (pindex_is_heap) dogecoin_free(pindex);
                return;
            }

            client->nodegroup->log_write_cb("Got invalid block (not in sequence) from node %d\n", node->nodeid);
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            dogecoin_free(pindex);
            return;
        }

        if (dogecoin_hash_equal((uint8_t *)node->last_requested_inv, (uint8_t *)pindex->hash)) {
            // instead of querying whether the last connected header timestamp is greater than the oldest item of interest
            // we check if the height is greater than or equal to the node's bestknown height minus 5 minutes
            if (client->headers_db->getchaintip(client->headers_db_ctx)->height >= node->bestknownheight - 5) {
                // last requested block reached, consider stop syncing
                if (!client->called_sync_completed && client->sync_completed
                    && !spv_cf_sync_pending(client)) { /* BIP157: defer only while filter sync can progress */
                    // enable mempool requests if smpv is enabled
                    if (client->smpv_enabled) dogecoin_net_spv_request_mempool(client);
                    client->sync_completed(client);
                    client->called_sync_completed = true;
                }
            } else if (client->headers_db->getchaintip(client->headers_db_ctx)->height < node->bestknownheight - 1440) {
                node->time_last_request = time(NULL);
                dogecoin_net_spv_node_request_headers_or_blocks(node, true);
            }
        }
    }

    if (strcmp(hdr->command, DOGECOIN_MSG_HEADERS) == 0)
    {
        uint32_t amount_of_headers;
        if (!deser_varlen(&amount_of_headers, buf)) return;
        uint64_t now = time(NULL);
        /* last_headersrequest_time is only armed by the serial path; in
           parallel mode it stays 0 and the subtraction printed a raw epoch.
           Deliberately not armed here — it drives a per-node stall check that
           would misbehave every peer at once if the whole sync went quiet. */
        if (client->par_hdr && client->par_hdr->active)
            client->nodegroup->log_write_cb("Got %d headers from node %d\n",
                amount_of_headers, node->nodeid);
        else
            client->nodegroup->log_write_cb("Got %d headers (took %d s) from node %d [%us elapsed]\n", amount_of_headers, now - client->last_headersrequest_time, node->nodeid, spv_elapsed(client));

        /* Parallel genesis headers mode -- buffer into the owning segment. */
        if (client->par_hdr && client->par_hdr->active) {
            par_hdr_recv(client, node, buf, amount_of_headers);
            return;
        }

        // flag off the request stall check
        client->last_headersrequest_time = 0;

        /* Use global-best height so we don't switch to BLOCKSYNC prematurely when
         * the peer we're syncing from has a low tip but other peers are further ahead. */
        uint32_t global_best_height = node->bestknownheight;
        {
            unsigned int gni;
            for (gni = 0; gni < (unsigned int)client->nodegroup->nodes->len; gni++) {
                dogecoin_node *gn = (dogecoin_node*)vector_idx(client->nodegroup->nodes, gni);
                if ((gn->state & NODE_CONNECTED) && gn->bestknownheight > global_best_height)
                    global_best_height = gn->bestknownheight;
            }
        }

        unsigned int connected_headers = 0;
        unsigned int i;
        for (i = 0; i < amount_of_headers; i++)
        {
            dogecoin_bool connected;
            dogecoin_blockindex *pindex = client->headers_db->connect_hdr(client->headers_db_ctx, buf, false, &connected);
            if (!pindex)
            {
                client->nodegroup->log_write_cb("Header deserialization failed (node %d)\n", node->nodeid);
            }
            /* Per-header transaction count. Always zero in a headers
               message, so the old 1-byte skip was right in practice, but the
               wire type is a varint and reading it as one keeps a
               non-minimally-encoded zero from desynchronising the parse one
               byte into the next header. */
            uint32_t hdr_tx_count = 0;
            if (!deser_varlen(&hdr_tx_count, buf)) {
                client->nodegroup->log_write_cb("Header deserialization (tx count) failed (node %d)\n", node->nodeid);
            }

            if (!connected)
            {
                client->nodegroup->log_write_cb("Got invalid headers (not in sequence) from node %d\n", node->nodeid);
                node->state &= ~NODE_HEADERSYNC;
                node->nodegroup->node_connection_state_changed_cb(node);
                dogecoin_free(pindex);
                break;
            } else {
                if (client->header_connected) { client->header_connected(client); }
                connected_headers++;
                if (pindex->height >= global_best_height - 5) {
                    client->stateflags &= ~SPV_HEADER_SYNC_FLAG;
                    client->stateflags |= SPV_FULLBLOCK_SYNC_FLAG;
                    node->state &= ~NODE_HEADERSYNC;
                    node->state |= NODE_BLOCKSYNC;
                    client->nodegroup->log_write_cb("start loading block from node %d at height %d at time: %ld\n", node->nodeid, client->headers_db->getchaintip(client->headers_db_ctx)->height, client->headers_db->getchaintip(client->headers_db_ctx)->header.timestamp);
                    dogecoin_net_spv_node_request_headers_or_blocks(node, true);
                    break;
                }
            }
        }
        dogecoin_blockindex *chaintip = client->headers_db->getchaintip(client->headers_db_ctx);

        client->nodegroup->log_write_cb("Connected %d headers\n", connected_headers);
        client->nodegroup->log_write_cb("Chaintip at height %d [%us elapsed]\n", chaintip->height, spv_elapsed(client));

        if (client->header_message_processed && client->header_message_processed(client, node, chaintip) == false)
            return;

        if (amount_of_headers == MAX_HEADERS_RESULTS && ((node->state & NODE_BLOCKSYNC) != NODE_BLOCKSYNC))
        {
            time_t lasttime = chaintip->header.timestamp;
            char lasttime_str[DOGECOIN_CTIME_LEN] = {0};
            dogecoin_ctime(&lasttime, lasttime_str, sizeof lasttime_str);
            client->nodegroup->log_write_cb("chain size: %d, last time %s", chaintip->height, lasttime_str);
            dogecoin_net_spv_node_request_headers_or_blocks(node, false);
        }
        else if (client->compact_filters_enabled && client->cfilter_state &&
                 !client->cfilter_state->awaiting_response &&
                 /* CF sync not yet complete: cfheaders or cfilters behind chain tip */
                 (client->cfilter_state->cfheaders_tip_height < (uint32_t)chaintip->height ||
                  client->cfilter_state->filters_tip_height  < client->cfilter_state->cfheaders_tip_height) &&
                 /* Not already in a live parallel cfilter download */
                 !(client->cfilter_state->par_num_workers > 0 &&
                   client->cfilter_state->filters_tip_height < client->cfilter_state->cfheaders_tip_height) &&
                 chaintip->height > 0 &&
                 chaintip->height >= node->bestknownheight - 5) {
            /* Headers are at tip — find a BIP157-capable peer (NODE_COMPACT_FILTERS) */
            dogecoin_node *cf_node = NULL;
            for (unsigned int ni = 0; ni < (unsigned int)client->nodegroup->nodes->len; ni++) {
                dogecoin_node *n = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
                if ((n->state & NODE_CONNECTED) && (n->services & DOGECOIN_NODE_COMPACT_FILTERS)) {
                    cf_node = n;
                    break;
                }
            }
            if (cf_node) {
                client->nodegroup->log_write_cb("[bip157] headers synced to tip (height=%d), sending getcfcheckpt to node %d (compact-filters peer)\n",
                                                chaintip->height, cf_node->nodeid);
                dogecoin_spv_request_cfcheckpt(client, cf_node);
            } else {
                client->nodegroup->log_write_cb("[bip157] headers at tip (height=%d) but no NODE_COMPACT_FILTERS peer connected\n",
                                                chaintip->height);
                /* Completion is not handled here on purpose.  Headers reaching the tip
                 * is not the end of the sync -- the last few blocks still arrive over
                 * BLOCKSYNC afterwards -- so finishing at this point reports a height
                 * short of the real tip.  spv_cf_sync_pending() lets the normal
                 * headers/blocks completion run instead, at the right moment. */
            }
        }

        /* Blocks matched by the startup rescan of already-cached cfilters have to be
         * requested independently of the cfheaders/cfilters network sync above.  That
         * sync only covers heights not already on disk, so when the cache already spans
         * the chain its completion hook (spv_cf_par_try_flush) never fires and the
         * matched blocks would never be fetched -- the scan reported matches but the
         * wallet stayed empty.  Trigger here once headers are at tip, whichever CF
         * branch above was taken; cf_block_fetch_active keeps it to one dispatch. */
        if (client->compact_filters_enabled && client->cfilter_state &&
            client->cfilter_state->rescan_done &&
            client->cfilter_state->matched_block_hashes &&
            client->cfilter_state->matched_block_hashes->len > 0 &&
            !client->cfilter_state->cf_block_fetch_active &&
            chaintip->height > 0 &&
            chaintip->height >= node->bestknownheight - 5) {
            spv_cf_request_matched_blocks(client);
        }
    }

    if (strcmp(hdr->command, DOGECOIN_MSG_MERKLEBLOCK) == 0) {
        dogecoin_bool connected = false;
        if (client->bloom_filter_debug_dump && client->nodegroup && client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb("%s", client->bloom_filter_debug_dump);
        }

        /* connect header first (advances buf past 80-byte header) */
        const unsigned char* merkleblock_start = (const unsigned char*)buf->p;
        dogecoin_blockindex *pindex = client->headers_db->connect_hdr(client->headers_db_ctx, buf, false, &connected);
        node->time_last_request = time(NULL);

        /* If not newly connected, the block may already be in the chain (historical rescan).
           Look it up to get the real blockindex with correct height. */
        dogecoin_bool is_historical = false;
        if (!connected && pindex) {
            dogecoin_headers_db* db = (dogecoin_headers_db*)client->headers_db_ctx;
            dogecoin_blockindex* found = dogecoin_headersdb_find(db, pindex->hash);
            dogecoin_bool recovered_historical_height = false;
            if (found) {
                dogecoin_free(pindex);
                pindex = found;
            } else if (client->filtered_history_last_end_height >= 0 && client->bloom_filter) {
                uint32_t historical_height = 0;
                if (spv_lookup_headersdb_height_by_hash(client, pindex->hash, &historical_height)) {
                    pindex->height = historical_height;
                    recovered_historical_height = true;
                }
            }
            /* During filtered-history rescans, allow processing even if the header is pruned from in-memory tree. */
            if (found || recovered_historical_height) {
                is_historical = true;
            }
        }

        if (!connected && !is_historical) {
            if (!pindex) {
                client->nodegroup->log_write_cb("Got invalid merkleblock (not in sequence) from node %d\n", node->nodeid);
            } else {
                client->nodegroup->log_write_cb("Got unknown merkleblock from node %d\n", node->nodeid);
            }
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            if (pindex && !is_historical) dogecoin_free(pindex);
            return;
        }

        if (client->header_connected && connected) { client->header_connected(client); }

        /* reset prior match state */
        if (client->merkle_match_tree) {
            dogecoin_btree_tdestroy(client->merkle_match_tree, dogecoin_free);
            client->merkle_match_tree = NULL;
        }
        client->merkle_match_pending = 0;
        client->merkle_match_active = false;
        client->merkle_match_blockindex = NULL;

        /* header merkle root (from original 80-byte header at message start) */
        uint256_t header_merkle;
        memcpy(header_merkle, merkleblock_start + 36, 32);

        /* after connect_hdr, buf points at nTx (uint32) */
        uint32_t nTx = 0;
        if (!deser_u32(&nTx, buf)) {
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            client->nodegroup->log_write_cb("Merkleblock nTx deser failed (node %d)\n", node->nodeid);
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        uint32_t hashCount = 0;
        if (!deser_varlen(&hashCount, buf) || hashCount == 0) {
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            client->nodegroup->log_write_cb("Merkleblock hashCount deser failed/zero (node %d)\n", node->nodeid);
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        uint256_t* hashes = (uint256_t*)dogecoin_calloc(hashCount, sizeof(uint256_t));
        if (!hashes) {
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        unsigned int i;
        for (i = 0; i < hashCount; i++) {
            if (!deser_u256(hashes[i], buf)) {
                dogecoin_free(hashes);
                if (!is_historical) {
                    if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                        dogecoin_free(pindex);
                    }
                }
                client->nodegroup->log_write_cb("Merkleblock hash deser failed (node %d)\n", node->nodeid);
                node->state &= ~NODE_BLOCKSYNC;
                node->nodegroup->node_connection_state_changed_cb(node);
                return;
            }
        }

        uint32_t flags_len = 0;
        if (!deser_varlen(&flags_len, buf) || flags_len == 0 || flags_len > buf->len) {
            dogecoin_free(hashes);
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            client->nodegroup->log_write_cb("Merkleblock flags deser failed/badlen (node %d)\n", node->nodeid);
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        uint8_t* flags = (uint8_t*)dogecoin_calloc(flags_len, 1);
        if (!flags) {
            dogecoin_free(hashes);
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }
        memcpy(flags, buf->p, flags_len);
        if (!deser_skip(buf, flags_len)) {
            dogecoin_free(flags);
            dogecoin_free(hashes);
            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        if (!dogecoin_bip37_merkle_extract_match_tree(nTx,
                                                      (const uint8_t*)hashes,
                                                      hashCount,
                                                      flags,
                                                      flags_len,
                                                      header_merkle,
                                                      (const uint8_t*)pindex->hash,
                                                      (int)pindex->height,
                                                      client->bloom_filter_debug_dump,
                                                      &client->merkle_match_tree,
                                                      &client->merkle_match_pending,
                                                      client->nodegroup ? client->nodegroup->log_write_cb : NULL)) {
            if (client->merkle_match_tree) {
                dogecoin_btree_tdestroy(client->merkle_match_tree, dogecoin_free);
                client->merkle_match_tree = NULL;
            }
            client->merkle_match_pending = 0;
            client->merkle_match_active = false;
            client->merkle_match_blockindex = NULL;

            dogecoin_free(flags);
            dogecoin_free(hashes);

            if (!is_historical) {
                if (!client->headers_db->disconnect_tip(client->headers_db_ctx)) {
                    dogecoin_free(pindex);
                }
            }

            client->nodegroup->log_write_cb("Merkleblock verify failed (node %d)\n", node->nodeid);
            node->state &= ~NODE_BLOCKSYNC;
            node->nodegroup->node_connection_state_changed_cb(node);
            return;
        }

        client->merkle_match_active = (client->merkle_match_pending > 0);
        client->merkle_match_blockindex = pindex;

        if (client->merkle_match_pending > 0 && client->nodegroup && client->nodegroup->log_write_cb) {
            spv_merkle_log_ctx log_ctx;
            log_ctx.client = client;
            log_ctx.pindex = pindex;
            dogecoin_bip37_merkle_for_each_match(client->merkle_match_tree, spv_log_merkle_match, &log_ctx);
        }

        /* Update rescan progress counters. */
        client->rescan_total++;
        static const uint64_t early_rescan_log_threshold = 50;
        static const uint64_t periodic_rescan_log_interval = 1000;
        if (client->merkle_match_pending > 0) {
            client->rescan_matched++;
            /* Log individual blocks only when they have matches. */
            client->nodegroup->log_write_cb("[merkle] MATCH at height %d: nTx=%u matched=%u\n",
                pindex->height, nTx, client->merkle_match_pending);
        } else if (client->nodegroup->log_write_cb &&
                   client->filtered_history_last_end_height >= 0 &&
                   (client->rescan_total <= early_rescan_log_threshold ||
                    (client->rescan_total % periodic_rescan_log_interval) == 0)) {
            /* During historical scans, emit early + periodic parse logs so we can confirm merkleblocks are being processed. */
            client->nodegroup->log_write_cb("[merkle] parsed height %d: nTx=%u matched=0 (scanned=%llu)\n",
                pindex->height, nTx, (unsigned long long)client->rescan_total);
        }
        /* Log periodic progress every 10,000 blocks. */
        if (client->rescan_total % 10000 == 0) {
            client->nodegroup->log_write_cb("[merkle] progress: %llu blocks scanned, %llu with matches\n",
                (unsigned long long)client->rescan_total,
                (unsigned long long)client->rescan_matched);
        }

        /* Advance bounded historical scan windows only after the current
           requested end-height has actually been parsed. This avoids
           pre-queuing many future windows with a stale bloom filter state. */
        if (client->called_sync_completed &&
            client->headers_db &&
            client->bloom_filter && client->bloom_filter_len > 0 &&
            client->filtered_history_last_end_height >= 0 &&
            !client->merkle_match_active &&
            client->merkle_match_pending == 0 &&
            pindex &&
            (int32_t)pindex->height >= client->filtered_history_last_end_height) {
            dogecoin_blockindex* tip_now = client->headers_db->getchaintip(client->headers_db_ctx);
            if (tip_now && (uint32_t)client->filtered_history_last_end_height < tip_now->height) {
                client->filtered_history_tail_rerequest_count = 0;
                dogecoin_hash_clear(client->filtered_history_last_rerequest_txid);
                client->filtered_history_last_rerequest_height = -1;
                dogecoin_net_spv_request_filtered_history(client, 0);
            }
        }

        dogecoin_free(flags);
        dogecoin_free(hashes);

        if (dogecoin_hash_equal((uint8_t *)node->last_requested_inv, (uint8_t *)pindex->hash)) {
            if (client->headers_db->getchaintip(client->headers_db_ctx)->height >= node->bestknownheight - 5) {
                if (!client->called_sync_completed && client->sync_completed
                    && !spv_cf_sync_pending(client)) { /* BIP157: defer only while filter sync can progress */
                    if (client->smpv_enabled) dogecoin_net_spv_request_mempool(client);
                    client->sync_completed(client);
                    client->called_sync_completed = true;
                }
            } else if (client->headers_db->getchaintip(client->headers_db_ctx)->height < node->bestknownheight - 1440) {
                node->time_last_request = time(NULL);
                dogecoin_net_spv_node_request_headers_or_blocks(node, true);
            }
        }
    }

    if (strcmp(hdr->command, DOGECOIN_MSG_TX) == 0) {
        if (client && client->merkle_match_active && client->merkle_match_pending > 0 &&
            client->merkle_match_tree && client->sync_transaction) {
            size_t consumedlength = 0;
            dogecoin_tx* tx = dogecoin_tx_new();
            if (tx && dogecoin_tx_deserialize(buf->p, buf->len, tx, &consumedlength)) {
                uint256_t txid;
                dogecoin_tx_hash(tx, txid);

                char txid_hex[65];
                utils_bin_to_hex(txid, 32, txid_hex);
                client->nodegroup->log_write_cb("[merkle-tx] received tx %s (match_pending=%u)\n",
                    txid_hex, client->merkle_match_pending);

                uint32_t match_pos = 0;
                if (dogecoin_bip37_merkle_match_consume(&client->merkle_match_tree,
                                                        &client->merkle_match_pending,
                                                        txid,
                                                        &match_pos)) {
                    unsigned int pos = (unsigned int)match_pos;
                    dogecoin_blockindex* bi = client->merkle_match_blockindex;
                    uint32_t vout_i = 0;

                    if (client->bloom_filter && client->bloom_filter_len > 0) {
                        for (vout_i = 0; vout_i < (uint32_t)tx->vout->len; vout_i++) {
                            uint8_t outpoint[36];
                            unsigned int b = 0;
                            /* COutPoint serializes txid little-endian on the wire. */
                            for (b = 0; b < 32; b++) outpoint[b] = txid[31 - b];
                            outpoint[32] = (uint8_t)(vout_i & 0xffu);
                            outpoint[33] = (uint8_t)((vout_i >> 8) & 0xffu);
                            outpoint[34] = (uint8_t)((vout_i >> 16) & 0xffu);
                            outpoint[35] = (uint8_t)((vout_i >> 24) & 0xffu);
                            dogecoin_spv_client_filteradd(client, outpoint, (uint32_t)sizeof(outpoint));
                        }
                    }

                    client->nodegroup->log_write_cb("[merkle-tx] MATCH at pos %u, calling sync_transaction (height=%d)\n",
                        pos, bi ? (int)bi->height : -1);
                    client->sync_transaction(client->sync_transaction_ctx, tx, pos, bi);

                    if (client->filtered_history_tail_rerequest_count < 8 &&
                        client->filtered_history_last_end_height >= 0 &&
                        bi &&
                        (int32_t)bi->height < client->filtered_history_last_end_height) {
                        dogecoin_blockindex* tip_now = client->headers_db->getchaintip(client->headers_db_ctx);
                        if (tip_now) {
                            int64_t height_delta = (int64_t)tip_now->height - (int64_t)bi->height;
                            if (height_delta > 0) {
                                dogecoin_bool is_duplicate_tail_trigger =
                                    (client->filtered_history_last_rerequest_height >= 0) &&
                                    ((int32_t)bi->height == client->filtered_history_last_rerequest_height) &&
                                    (memcmp(client->filtered_history_last_rerequest_txid, txid, sizeof(uint256_t)) == 0);
                                if (is_duplicate_tail_trigger) {
                                    if (client->nodegroup && client->nodegroup->log_write_cb) {
                                        client->nodegroup->log_write_cb("[spv] skipping duplicate historical tail re-request for tx at height %d\n",
                                            (int)bi->height);
                                    }
                                } else {
                                int32_t previous_end = client->filtered_history_last_end_height;
                                int depth_to_tip = (height_delta > (int64_t)INT_MAX) ? INT_MAX : (int)height_delta;
                                client->filtered_history_tail_rerequest_count++;
                                dogecoin_hash_set(client->filtered_history_last_rerequest_txid, txid);
                                client->filtered_history_last_rerequest_height = (int32_t)bi->height;
                                client->filtered_history_last_end_height = (int32_t)bi->height;
                                if (client->nodegroup && client->nodegroup->log_write_cb) {
                                    client->nodegroup->log_write_cb("[spv] re-requesting historical tail heights %d-%d after new matched tx to catch spends (attempt %u/8)\n",
                                        (int)bi->height + 1, (int)previous_end,
                                        (unsigned int)client->filtered_history_tail_rerequest_count);
                                }
                                if (depth_to_tip > 0) {
                                    dogecoin_net_spv_request_filtered_history(client, depth_to_tip);
                                }
                                }
                            }
                        }
                    }

                    if (client->merkle_match_pending == 0) {
                        client->merkle_match_active = false;
                        client->merkle_match_blockindex = NULL;
                        if (client->merkle_match_tree) {
                            dogecoin_btree_tdestroy(client->merkle_match_tree, dogecoin_free);
                            client->merkle_match_tree = NULL;
                        }
                    }
                } else {
                    client->nodegroup->log_write_cb("[merkle-tx] tx NOT found in match tree\n");
                }
            }
            if (tx) dogecoin_tx_free(tx);
        }

        if (client && client->smpv_enabled && client->smpv_ctx) {
            // allocate hex buffer (2 chars per byte + NUL)
            size_t hex_len = ((size_t)hdr->data_len * 2) + 1;
            char* hex = (char*)dogecoin_calloc(1, hex_len);
            if (hex) {
                // convert raw bytes to hex using utils.c
                utils_bin_to_hex((unsigned char*)buf->p, (size_t)hdr->data_len, hex);

                dogecoin_bool ok = dogecoin_spv_handle_mempool_tx_hex(client, hex);

                if (client->nodegroup && client->nodegroup->log_write_cb) {
                    client->nodegroup->log_write_cb(
                        "[smpv] mempool tx seen len=%u dispatched=%s\n",
                        hdr->data_len, ok ? "true" : "false"
                    );
                }
                dogecoin_free(hex);
            } else {
                if (client->nodegroup && client->nodegroup->log_write_cb) {
                    client->nodegroup->log_write_cb(
                        "[smpv] hex alloc failed for len=%u\n",
                        hdr->data_len
                    );
                }
            }
        }
    }

    /* ================================================================ */
    /* BIP157: cfilter response handler                                 */
    /* ================================================================ */
    if (strcmp(hdr->command, DOGECOIN_MSG_CFILTER) == 0)
    {
        if (!client->compact_filters_enabled || !client->cfilter_state) {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[bip157] compact filters not enabled, ignoring cfilter\n");
        } else {
            dogecoin_cfilter_msg cfilter_msg;
            dogecoin_cfilter_msg_init(&cfilter_msg);

            struct const_buffer deser_buf = { buf->p, buf->len };
            if (dogecoin_p2p_msg_cfilter_deser(&cfilter_msg, &deser_buf)) {
                dogecoin_compact_filter_state *cfstate = client->cfilter_state;

                if (cfilter_msg.filter_data) {
                    dogecoin_bool parallel = (cfstate->par_num_workers > 1);

                    /* Determine the height this cfilter corresponds to. */
                    uint32_t filter_height;
                    cf_par_buf *par_buf = NULL;

                    if (parallel) {
                        /* Per-node height tracking: validate node has an active batch */
                        if (node->cf_batch_end == 0 ||
                            node->cf_cur_height > node->cf_batch_end) {
                            /* Stale or unassigned — drop */
                            goto cfilter_handler_done;
                        }
                        filter_height = node->cf_cur_height;
                        node->cf_cur_height++;

                        /* Find the buffer for this node whose range covers filter_height.
                         * A node can have two slots temporarily: one complete-but-pending-flush
                         * and one newly-assigned. Match by height range to avoid OOB write. */
                        uint8_t pi;
                        for (pi = 0; pi < cfstate->par_num_workers; pi++) {
                            if (cfstate->par_bufs[pi].node_id == node->nodeid &&
                                filter_height >= cfstate->par_bufs[pi].batch_start &&
                                filter_height <= cfstate->par_bufs[pi].batch_end) {
                                par_buf = &cfstate->par_bufs[pi];
                                break;
                            }
                        }
                        if (!par_buf) goto cfilter_handler_done;
                    } else {
                        if (cfstate->filters_tip_height >= cfstate->cfheaders_tip_height)
                            goto cfilter_handler_done;
                        filter_height = cfstate->filters_tip_height + 1;
                    }

                    /*
                     * filter_headers[i] = filter header for block at height (cfheaders_base_height + i).
                     * For block at height h:
                     *   vec_idx     = h - cfheaders_base_height
                     *   expected_fh = filter_headers[vec_idx]
                     *   prev_fh     = filter_headers[vec_idx - 1]  (or genesis_filter_header when vec_idx == 0)
                     */
                    uint256_t prev_fh;
                    uint32_t base = cfstate->cfheaders_base_height;
                    dogecoin_bool have_fh = cfstate->filter_headers_flat
                                           ? (cfstate->filter_headers_flat_len > 0)
                                           : (cfstate->filter_headers->len > 0);
                    if (filter_height <= base || !have_fh) {
                        memcpy(prev_fh, cfstate->genesis_filter_header, 32);
                    } else {
                        uint32_t prev_idx = filter_height - base - 1;
                        if (cfstate->filter_headers_flat) {
                            if (prev_idx < cfstate->filter_headers_flat_len)
                                memcpy(prev_fh, cfstate->filter_headers_flat + prev_idx * 32, 32);
                            else
                                memcpy(prev_fh, cfstate->cfheaders_tip_hash, 32);
                        } else {
                            if (prev_idx < cfstate->filter_headers->len)
                                memcpy(prev_fh, vector_idx(cfstate->filter_headers, prev_idx), 32);
                            else
                                memcpy(prev_fh, cfstate->cfheaders_tip_hash, 32);
                        }
                    }

                    uint32_t vec_idx = (filter_height >= base) ? filter_height - base : UINT32_MAX;
                    uint256_t flat_fh;
                    uint256_t *expected_fh_ptr = NULL;
                    if (cfstate->filter_headers_flat) {
                        if (vec_idx < cfstate->filter_headers_flat_len) {
                            memcpy(flat_fh, cfstate->filter_headers_flat + vec_idx * 32, 32);
                            expected_fh_ptr = &flat_fh;
                        }
                    } else if (vec_idx < cfstate->filter_headers->len) {
                        expected_fh_ptr = (uint256_t *)vector_idx(cfstate->filter_headers, vec_idx);
                    }
                    if (expected_fh_ptr) {
                        uint256_t *expected_fh = expected_fh_ptr;

                        if (dogecoin_compact_filter_validate(cfilter_msg.filter_data, prev_fh, *expected_fh)) {
                            if (parallel) {
                                /* Buffer the validated record; disk flush happens in-order via spv_cf_par_try_flush */
                                uint32_t offset = filter_height - par_buf->batch_start;
                                cf_par_record *rec = &par_buf->records[offset];
                                rec->filter_data = cstr_new_buf(cfilter_msg.filter_data->str,
                                                                cfilter_msg.filter_data->len);
                                memcpy(rec->block_hash, cfilter_msg.block_hash, 32);

                                /* Match watched scripts now (before buffering loses the data reference) */
                                if (cfstate->watched_scripts && cfstate->watched_scripts->len > 0) {
                                    gcs_filter *gcs = gcs_filter_new();
                                    struct const_buffer fbuf = { cfilter_msg.filter_data->str,
                                                                  cfilter_msg.filter_data->len };
                                    if (gcs_filter_deserialize(gcs, cfilter_msg.filter_type,
                                                                cfilter_msg.block_hash, &fbuf)) {
                                        if (gcs_filter_match_any(gcs, cfstate->watched_scripts)) {
                                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                                client->nodegroup->log_write_cb(
                                                    "[bip157] MATCH at height %u\n", filter_height);
                                            uint256_t *matched_hash = dogecoin_calloc(1, sizeof(uint256_t));
                                            memcpy(matched_hash, cfilter_msg.block_hash, sizeof(uint256_t));
                                            vector_add(cfstate->matched_block_hashes, matched_hash);
                                            uint32_t *matched_height = dogecoin_calloc(1, sizeof(uint32_t));
                                            *matched_height = filter_height;
                                            vector_add(cfstate->matched_block_heights, matched_height);
                                        }
                                    }
                                    gcs_filter_free(gcs);
                                }

                                par_buf->received++;
                                if (par_buf->received == par_buf->batch_end - par_buf->batch_start + 1) {
                                    par_buf->complete = true;
                                    spv_cf_par_try_flush(client);
                                    /* Get next batch for this worker (no-op if all work assigned) */
                                    spv_cf_par_assign(client, node);
                                }
                            } else {
                                /* Sequential mode: write immediately */
                                cfstate->filters_tip_height = filter_height;

                                if (client->cfilters_db)
                                    dogecoin_cfilters_db_write(client->cfilters_db,
                                                               filter_height,
                                                               cfilter_msg.block_hash,
                                                               cfilter_msg.filter_data);

                                /* Match watched scripts */
                                if (cfstate->watched_scripts && cfstate->watched_scripts->len > 0) {
                                    gcs_filter *gcs = gcs_filter_new();
                                    struct const_buffer fbuf = { cfilter_msg.filter_data->str,
                                                                  cfilter_msg.filter_data->len };
                                    if (gcs_filter_deserialize(gcs, cfilter_msg.filter_type,
                                                                cfilter_msg.block_hash, &fbuf)) {
                                        if (gcs_filter_match_any(gcs, cfstate->watched_scripts)) {
                                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                                client->nodegroup->log_write_cb(
                                                    "[bip157] MATCH at height %u\n", filter_height);
                                            uint256_t *matched_hash = dogecoin_calloc(1, sizeof(uint256_t));
                                            memcpy(matched_hash, cfilter_msg.block_hash, sizeof(uint256_t));
                                            vector_add(cfstate->matched_block_hashes, matched_hash);
                                            uint32_t *matched_height = dogecoin_calloc(1, sizeof(uint32_t));
                                            *matched_height = filter_height;
                                            vector_add(cfstate->matched_block_heights, matched_height);
                                        }
                                    }
                                    gcs_filter_free(gcs);
                                }

                                /* Request next batch when current is consumed */
                                dogecoin_blockindex *cf_tip =
                                    client->headers_db->getchaintip(client->headers_db_ctx);
                                if (cfstate->filters_tip_height >= cfstate->cfheaders_tip_height) {
                                    if (client->nodegroup && client->nodegroup->log_write_cb) {
                                        uint32_t scan_start = cfstate->cf_scan_start_height > 0 ? cfstate->cf_scan_start_height : 1;
                                        client->nodegroup->log_write_cb(
                                            "[bip157] all filters processed: scanned heights %u..%u, %u matched blocks [%us elapsed]\n",
                                            scan_start, cfstate->filters_tip_height,
                                            (unsigned int)cfstate->matched_block_hashes->len,
                                            spv_elapsed(client));
                                        if (scan_start > 1)
                                            client->nodegroup->log_write_cb(
                                                "[bip157] WARNING: scan started at height %u (checkpoint), not genesis — "
                                                "transactions before height %u are not covered; use --filter_hash_db for full history\n",
                                                scan_start, scan_start);
                                    }
                                    cfstate->awaiting_response = false;
                                    client->stateflags &= ~SPV_CFILTER_SYNC_FLAG;
                                    if (cfstate->matched_block_hashes->len > 0) {
                                        spv_cf_request_matched_blocks(client);
                                    } else if (!client->called_sync_completed && client->sync_completed) {
                                        if (client->smpv_enabled) dogecoin_net_spv_request_mempool(client);
                                        client->sync_completed(client);
                                        client->called_sync_completed = true;
                                    }
                                } else if (cfstate->filters_tip_height >= cfstate->cfilter_batch_end) {
                                    cfstate->awaiting_response = false;
                                    dogecoin_spv_request_cfilters(client, node,
                                        cfstate->filters_tip_height + 1, cf_tip->hash);
                                }
                            }
                        } else {
                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                client->nodegroup->log_write_cb(
                                    "[bip157] filter at height %u FAILED validation\n", filter_height);
                            dogecoin_node_misbehave(node);
                        }
                    }
                }
                cfilter_handler_done:;
            } else {
                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb("[bip157] failed to deserialize cfilter from node %d\n", node->nodeid);
            }
            dogecoin_cfilter_msg_free(&cfilter_msg);
        }
    }

    /* ================================================================ */
    /* BIP157: cfheaders response handler                               */
    /* ================================================================ */
    if (strcmp(hdr->command, DOGECOIN_MSG_CFHEADERS) == 0)
    {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb("[bip157] received cfheaders from node %d\n", node->nodeid);

        if (!client->compact_filters_enabled || !client->cfilter_state) {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[bip157] compact filters not enabled, ignoring cfheaders\n");
        } else {
            dogecoin_cfheaders_msg cfh_msg;
            dogecoin_cfheaders_msg_init(&cfh_msg);

            struct const_buffer deser_buf = { buf->p, buf->len };
            if (dogecoin_p2p_msg_cfheaders_deser(&cfh_msg, &deser_buf)) {
                dogecoin_compact_filter_state *cfstate = client->cfilter_state;

                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb("[bip157] cfheaders: type=%u n_hashes=%u\n",
                        cfh_msg.filter_type, (unsigned int)cfh_msg.filter_hashes->len);

                if (cfstate->cfh_par_n > 0) {
                    cfh_par_handle_response(client, node, &cfh_msg);
                    dogecoin_cfheaders_msg_free(&cfh_msg);
                    return;
                }

                if (!dogecoin_cfheaders_batch_extends_tip(cfstate,
                                                          cfh_msg.prev_filter_header)) {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] cfheaders from node %d do not extend tip %u, dropping %u hashes\n",
                            node->nodeid, cfstate->cfheaders_tip_height,
                            (unsigned int)cfh_msg.filter_hashes->len);
                    dogecoin_cfheaders_msg_free(&cfh_msg);
                    return;
                }

                uint256_t prev_header;
                memcpy(prev_header, cfh_msg.prev_filter_header, 32);
                uint32_t height = cfstate->cfheaders_tip_height;

                /* On the first cfheaders batch, prev_filter_header is the filter header
                 * immediately before our starting height — store it for cfilter validation
                 * and persist it so restarts don't need to re-download cfheaders. */
                if (cfstate->filter_headers->len == 0) {
                    memcpy(cfstate->genesis_filter_header, cfh_msg.prev_filter_header, 32);
                    if (client->cfheaders_db)
                        dogecoin_cfheaders_db_write_genesis(client->cfheaders_db,
                                                           cfh_msg.prev_filter_header);
                }

                unsigned int i;
                dogecoin_bool valid = true;

                for (i = 0; i < cfh_msg.filter_hashes->len; i++) {
                    uint256_t *filter_hash = (uint256_t *)vector_idx(cfh_msg.filter_hashes, i);
                    height++;

                    /* filter_header = dbl_sha256(filter_hash || prev_header) */
                    uint8_t combined[64];
                    memcpy(combined, filter_hash, 32);
                    memcpy(combined + 32, prev_header, 32);

                    uint256_t *new_header = dogecoin_calloc(1, sizeof(uint256_t));
                    dogecoin_hash(combined, 64, *new_header);

                    /* Anchor on the compiled-in table, looked up by height.
                     *
                     * This used to index cfstate->checkpoints, which the cfcheckpt
                     * handler filled from the peer's response -- so a peer that
                     * answered with a short list was validated only over the prefix
                     * it chose to send, and everything above that was accepted with
                     * no anchor at all. The table is ours and a peer cannot shorten
                     * it. Looking up by height rather than by index also keeps this
                     * correct on testnet, whose checkpoints are one per ten
                     * intervals rather than one per interval. */
                    uint256_t anchor;
                    if (dogecoin_cf_hardcoded_checkpoint_at(client->chainparams, height, anchor)) {
                        if (memcmp(*new_header, anchor, 32) != 0) {
                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                client->nodegroup->log_write_cb("[bip157] cfheader at height %u does NOT match checkpoint!\n", height);
                            valid = false;
                            dogecoin_free(new_header);
                            break;
                        }
                    }

                    vector_add(cfstate->filter_headers, new_header);
                    memcpy(prev_header, *new_header, 32);

                    /* Persist as we go. Only the genesis header was written
                       here, so cfheaders.dat kept just its file header and the
                       chain was refetched on every start; the flush below had
                       nothing to write. The parallel path already does this in
                       cfh_par_finish(). */
                    if (client->cfheaders_db)
                        dogecoin_cfheaders_db_write(client->cfheaders_db, height, *new_header);

                    if (client->cf_export_enabled &&
                        height > 0 && (height % CF_EXPORT_INTERVAL) == 0) {
                        char *hex = utils_uint8_to_hex(*new_header, 32);
                        printf("[cfcheckpt-export] { %u, \"%s\" },\n", height, hex);
                        fflush(stdout);
                    }
                }

                if (valid) {
                    cfstate->cfheaders_tip_height = height;
                    memcpy(cfstate->cfheaders_tip_hash, prev_header, 32);

                    if (client->cfheaders_db)
                        dogecoin_cfheaders_db_flush(client->cfheaders_db);

                    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
                    cfstate->awaiting_response = false;
                    if (height < (uint32_t)tip->height) {
                        dogecoin_spv_request_cfheaders(client, node, height + 1, tip->hash);
                    } else {
                        dogecoin_headers_db *hdb_cf = (dogecoin_headers_db *)client->headers_db_ctx;
                        uint32_t cf_scan_start = 1;
                        if (client->cf_start_height > 0) {
                            cf_scan_start = client->cf_start_height;
                        } else if (hdb_cf && hdb_cf->chainbottom && hdb_cf->chainbottom->height > 0) {
                            cf_scan_start = hdb_cf->chainbottom->height;
                        }

                        /* Clamp to cfheaders_base_height: filters below that were scanned at startup. */
                        if (cfstate->cfheaders_base_height > 0 &&
                            cf_scan_start < cfstate->cfheaders_base_height)
                            cf_scan_start = cfstate->cfheaders_base_height;

                        /* Resume from what has actually been scanned rather than from
                           the floor. This ran only when rescan_done, which the startup
                           rescan sets and which stays false on an empty filter store, so
                           a fresh scan rewound to cf_scan_start every time cfheaders
                           reached the tip -- once per block. Measured against a live
                           chain from 6300000: 108000 filters downloaded, 12 restarts,
                           6300000..6309000 covered, and it could never outrun the chain. */
                        cf_scan_start = spv_cf_resume_from(client, cf_scan_start);

                        /* Rescan any cached filters stored before these scripts were registered. */
                        spv_rescan_cached_cfilters(client, cf_scan_start);

                        if (client->nodegroup && client->nodegroup->log_write_cb)
                            client->nodegroup->log_write_cb(
                                "[bip157] all cfheaders received (tip=%u), starting cfilter scan from %u with %u workers [%us elapsed]\n",
                                cfstate->cfheaders_tip_height, cf_scan_start,
                                (unsigned int)(client->cf_num_workers > 1 ? client->cf_num_workers : 1),
                                spv_elapsed(client));

                        /* Record where the scan actually began. The completion log
                         * falls back to 1 when this is unset, which reported
                         * "scanned heights 1..tip" for a run that only covered the
                         * checkpoint tail -- overstating coverage in exactly the way
                         * that hides a gap. Both arms below need it. */
                        cfstate->cf_scan_start_height = cf_scan_start;

                        if (client->cf_num_workers > 1) {
                            /* Parallel mode: assign batches to all connected nodes */
                            cfstate->par_num_workers = client->cf_num_workers;
                            cfstate->par_next_height  = cf_scan_start;
                            cfstate->par_flush_height = cf_scan_start;
                            cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;

                            if (!cfstate->par_bufs) {
                                cfstate->par_bufs = (cf_par_buf *)dogecoin_calloc(
                                    cfstate->par_num_workers, sizeof(cf_par_buf));
                                uint8_t pi;
                                for (pi = 0; pi < cfstate->par_num_workers; pi++)
                                    cfstate->par_bufs[pi].node_id = -1;
                            }

                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                client->nodegroup->log_write_cb(
                                    "[bip157-par] assigning %u parallel workers from height %u\n",
                                    (unsigned int)cfstate->par_num_workers, cf_scan_start);

                            unsigned int ni;
                            for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                                dogecoin_node *wn = (dogecoin_node *)vector_idx(client->nodegroup->nodes, ni);
                                if (!wn || !(wn->state & NODE_CONNECTED) || !wn->version_handshake)
                                    continue;
                                spv_cf_par_assign(client, wn);
                            }
                        } else {
                            /* Sequential mode: single node, existing path */
                            cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;
                            dogecoin_spv_request_cfilters(client, node, cf_scan_start, tip->hash);
                        }
                    }
                } else {
                    dogecoin_node_misbehave(node);
                    cfstate->awaiting_response = false;
                }
            } else {
                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb("[bip157] failed to deserialize cfheaders\n");
            }
            dogecoin_cfheaders_msg_free(&cfh_msg);
        }
    }

    /* ================================================================ */
    /* BIP157: cfcheckpt response handler                               */
    /* ================================================================ */
    if (strcmp(hdr->command, DOGECOIN_MSG_CFCHECKPT) == 0)
    {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb("[bip157] received cfcheckpt from node %d\n", node->nodeid);

        if (!client->compact_filters_enabled || !client->cfilter_state) {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[bip157] compact filters not enabled, ignoring cfcheckpt\n");
        } else {
            dogecoin_cfcheckpt_msg cfcp_msg;
            dogecoin_cfcheckpt_msg_init(&cfcp_msg);

            struct const_buffer deser_buf = { buf->p, buf->len };
            if (dogecoin_p2p_msg_cfcheckpt_deser(&cfcp_msg, &deser_buf)) {
                dogecoin_compact_filter_state *cfstate = client->cfilter_state;

                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb("[bip157] cfcheckpt: type=%u n_checkpoints=%u\n",
                        cfcp_msg.filter_type, (unsigned int)cfcp_msg.filter_headers->len);

                /* Validate the peer's list without adopting it. Overwriting
                   cfstate->checkpoints here is what let a truncated response
                   disable anchoring: the replacement was only checked over the
                   indices the peer supplied, and the cfheaders path then trusted
                   its length. The compiled-in set loaded at client construction
                   stays put; this message is a misbehaviour signal, nothing more. */
                if (!dogecoin_cf_validate_checkpoints(client->chainparams, cfcp_msg.filter_headers)) {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb("[bip157] cfcheckpt FAILED hardcoded validation — misbehaving node %d\n", node->nodeid);
                    dogecoin_node_misbehave(node);
                    dogecoin_cfcheckpt_msg_free(&cfcp_msg);
                    return;
                }

                /* Export checkpoint filter headers directly from cfcheckpt response.
                 * Checkpoints are at heights 999, 1999, 2999 ... (cp_idx * 1000 + 999). */
                /* Export filter headers directly from cfcheckpt response.
                 * Dogecoin Core checkpoints are at heights 1000, 2000, ...
                 * peer_checkpoints[i] = filter header at height (i+1)*CFCHECKPT_INTERVAL. */
                if (client->cf_export_enabled) {
                    printf("[cfcheckpt-export] /* Dogecoin Core cfcheckpt: %u entries */\n",
                           (unsigned int)cfcp_msg.filter_headers->len);
                    for (unsigned int ci = 0; ci < (unsigned int)cfcp_msg.filter_headers->len; ci++) {
                        uint32_t cp_height = (ci + 1) * CFCHECKPT_INTERVAL;
                        uint256_t *fh = (uint256_t *)vector_idx(cfcp_msg.filter_headers, ci);
                        /* P2P bytes are LE; chainparams.c uses big-endian display (same as RPC).
                         * Reverse the 32 bytes before hex-encoding. */
                        uint8_t reversed[32];
                        for (int ri = 0; ri < 32; ri++) reversed[ri] = (*fh)[31 - ri];
                        char *hex = utils_uint8_to_hex(reversed, 32);
                        printf("[cfcheckpt-export] { %u, \"%s\" },\n", cp_height, hex);
                        fflush(stdout);
                    }
                    printf("[cfcheckpt-export] /* end */\n");
                    fflush(stdout);
                }

                dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);

                /* Determine where cfheaders download starts.
                 * cf_start_height overrides everything (set via --cf-from-genesis or API).
                 * Otherwise: start at chainbottom (checkpoint-based sync) unless an aux
                 * hash-lookup DB is available, in which case we can go back to genesis. */
                uint32_t cfh_start;
                if (client->cf_start_height > 0) {
                    cfh_start = client->cf_start_height;
                } else if (client->aux_hash_db && client->aux_hash_db_ctx) {
                    /* aux DB covers early heights — download filters from genesis. */
                    cfh_start = 1;
                } else {
                    dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
                    cfh_start = (hdb && hdb->chainbottom && hdb->chainbottom->height > 0)
                                ? hdb->chainbottom->height : 1;
                }

                /* Use cfheaders already loaded from disk if they are valid and cover
                 * the needed range.  The v2 DB stores genesis_filter_header so we can
                 * validate cfilters without re-downloading all cfheaders. */
                uint8_t gfh_zeros[32];
                memset(gfh_zeros, 0, 32);
                dogecoin_bool genesis_valid =
                    (memcmp(cfstate->genesis_filter_header, gfh_zeros, 32) != 0);
                /* base_height <= cfh_start matters: the loaded range has to start at or
                 * below where we want filters from, because the resume path can only
                 * append forward and cannot fill a gap *below* the loaded base.
                 *
                 * A gap *above* the loaded tip is not a reason to discard anything --
                 * that is precisely what resuming is for. Requiring
                 * cfheaders_tip_height >= cfh_start - 1 here meant that whenever the
                 * on-disk cfheaders stopped short of cfh_start, the whole cache was
                 * declared unusable and the fresh-start branch below wiped it, along
                 * with genesis_filter_header -- after which every subsequent cfilter
                 * failed validation. That is the normal case with -p/--checkpoint,
                 * where cfh_start comes from chainbottom near the chain tip while the
                 * cfheaders file legitimately ends wherever the last sync stopped:
                 * headers 1..6269574 on disk against a cfh_start of 6311582 threw away
                 * 6.27M valid headers to re-download a 42k-block gap. */
                dogecoin_bool have_loaded =
                    (cfstate->filter_headers->len > 0 &&
                     cfstate->cfheaders_base_height > 0 &&
                     cfstate->cfheaders_base_height <= cfh_start);

                if (have_loaded && genesis_valid) {
                    uint32_t cfh_resume = cfstate->cfheaders_tip_height + 1;
                    if (cfh_resume > (uint32_t)tip->height) {
                        /* cfheaders already at chain tip: skip directly to cfilters */
                        dogecoin_headers_db *hdb_cf = (dogecoin_headers_db *)client->headers_db_ctx;
                        uint32_t cf_scan_start = 1;
                        if (client->cf_start_height > 0) {
                            cf_scan_start = client->cf_start_height;
                        } else if (hdb_cf && hdb_cf->chainbottom && hdb_cf->chainbottom->height > 0) {
                            cf_scan_start = hdb_cf->chainbottom->height;
                        }
                        /* Clamp to cfheaders_base_height: filters below that were scanned at startup. */
                        if (cfstate->cfheaders_base_height > 0 &&
                            cf_scan_start < cfstate->cfheaders_base_height)
                            cf_scan_start = cfstate->cfheaders_base_height;
                        /* Resume from actual progress, not the floor. Gating this on
                           rescan_done left it false on an empty filter store, so every
                           new block reset the scan to cf_scan_start. Applies to the
                           parallel path too: par_next_height and par_flush_height are
                           both seeded from cf_scan_start just below. */
                        cf_scan_start = spv_cf_resume_from(client, cf_scan_start);
                        if (client->nodegroup && client->nodegroup->log_write_cb)
                            client->nodegroup->log_write_cb(
                                "[bip157] cfheaders at tip (base=%u tip=%u), starting cfilter scan from %u with %u workers [%us elapsed]\n",
                                cfstate->cfheaders_base_height, cfstate->cfheaders_tip_height,
                                cf_scan_start,
                                (unsigned int)(client->cf_num_workers > 1 ? client->cf_num_workers : 1),
                                spv_elapsed(client));
                        cfstate->cf_scan_start_height = cf_scan_start;
                        cfstate->awaiting_response = false;
                        if (client->cf_num_workers > 1) {
                            cfstate->par_num_workers   = client->cf_num_workers;
                            cfstate->par_next_height   = cf_scan_start;
                            cfstate->par_flush_height  = cf_scan_start;
                            cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;
                            if (!cfstate->par_bufs) {
                                cfstate->par_bufs = (cf_par_buf *)dogecoin_calloc(
                                    cfstate->par_num_workers, sizeof(cf_par_buf));
                                uint8_t pi;
                                for (pi = 0; pi < cfstate->par_num_workers; pi++)
                                    cfstate->par_bufs[pi].node_id = -1;
                            }
                            if (client->nodegroup && client->nodegroup->log_write_cb)
                                client->nodegroup->log_write_cb(
                                    "[bip157-par] assigning %u parallel workers from height %u\n",
                                    (unsigned int)cfstate->par_num_workers, cf_scan_start);
                            unsigned int ni;
                            for (ni = 0; ni < client->nodegroup->nodes->len; ni++) {
                                dogecoin_node *wn = (dogecoin_node *)vector_idx(
                                    client->nodegroup->nodes, ni);
                                if (!wn || !(wn->state & NODE_CONNECTED) ||
                                    !wn->version_handshake)
                                    continue;
                                spv_cf_par_assign(client, wn);
                            }
                        } else {
                            cfstate->filters_tip_height = (cf_scan_start > 1) ? cf_scan_start - 1 : 0;
                            dogecoin_spv_request_cfilters(client, node, cf_scan_start, tip->hash);
                        }
                    } else {
                        /* Resume cfheaders download from where the DB left off */
                        if (client->nodegroup && client->nodegroup->log_write_cb)
                            client->nodegroup->log_write_cb(
                                "[bip157] resuming cfheaders from %u (base=%u disk_tip=%u) [%us elapsed]\n",
                                cfh_resume, cfstate->cfheaders_base_height,
                                cfstate->cfheaders_tip_height, spv_elapsed(client));
                        cfstate->awaiting_response = false;
                        dogecoin_spv_request_cfheaders(client, node, cfh_resume, tip->hash);
                    }
                } else {
                    /* Fresh start: clear any stale loaded data and re-download */
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] cfheaders fresh start at height %u [%us elapsed]\n",
                            cfh_start, spv_elapsed(client));
                    if (cfstate->filter_headers->len > 0) {
                        vector_free(cfstate->filter_headers, true);
                        cfstate->filter_headers = vector_new(4096, dogecoin_free);
                    }
                    if (cfstate->filter_headers_flat) {
                        dogecoin_free(cfstate->filter_headers_flat);
                        cfstate->filter_headers_flat = NULL;
                        cfstate->filter_headers_flat_len = 0;
                    }
                    dogecoin_mem_zero(cfstate->cfheaders_tip_hash, sizeof(uint256_t));
                    dogecoin_mem_zero(cfstate->genesis_filter_header, sizeof(uint256_t));
                    cfstate->cfheaders_tip_height  = (cfh_start > 1) ? cfh_start - 1 : 0;
                    cfstate->cfheaders_base_height = cfh_start;
                    if (client->cfheaders_db)
                        dogecoin_cfheaders_db_reset(client->cfheaders_db);
                    cfstate->awaiting_response = false;
                    if (client->cf_num_workers > 1)
                        cfh_par_init(client, tip, cfh_start);
                    else
                        dogecoin_spv_request_cfheaders(client, node, cfh_start, tip->hash);
                }
            } else {
                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb("[bip157] failed to deserialize cfcheckpt\n");
            }
            dogecoin_cfcheckpt_msg_free(&cfcp_msg);
        }
    }

}

static void smpv_tx_cb(const dogecoin_smpv_tx* tx, const char* addr, void* user)
{
    dogecoin_spv_client* client = (dogecoin_spv_client*)user;
    if (!client || !client->nodegroup || !client->nodegroup->log_write_cb || !tx) return;

    client->nodegroup->log_write_cb(
        "[smpv] tx=%s size=%lluB vin=%u vout=%u coinbase=%d "
        "outval=%llu koinu types{p2pk=%u,p2pkh=%u,p2sh=%u,multi=%u,opret=%u,nonstd=%u}%s%s\n",
        tx->txid ? tx->txid : "(null)",
        (unsigned long long)tx->size,
        tx->vin_count, tx->vout_count,
        tx->is_coinbase ? 1 : 0,
        (unsigned long long)tx->total_output_value,
        tx->pubkey_out, tx->p2pkh_out, tx->p2sh_out,
        tx->multisig_out, tx->opreturn_out, tx->nonstandard_out,
        addr ? " addr=" : "", addr ? addr : ""
    );
}

LIBDOGECOIN_API void dogecoin_spv_enable_smpv(dogecoin_spv_client* client, dogecoin_bool enable)
{
    if (!client) return;

    if (enable && !client->smpv_enabled) {
        client->smpv_ctx = dogecoin_smpv_client_new(client->chainparams);
        if (client->smpv_ctx && dogecoin_smpv_start((dogecoin_smpv_client*)client->smpv_ctx)) {
            client->smpv_enabled = true;
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[smpv] enabled\n");
        } else {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[smpv] failed to enable (alloc/start)\n");
            if (client->smpv_ctx) {
                dogecoin_smpv_client_free((dogecoin_smpv_client*)client->smpv_ctx);
                client->smpv_ctx = NULL;
            }
        }
        return;
    }

    if (!enable && client->smpv_enabled) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb("[smpv] disabling\n");
        dogecoin_smpv_stop((dogecoin_smpv_client*)client->smpv_ctx);
        dogecoin_smpv_client_free((dogecoin_smpv_client*)client->smpv_ctx);
        client->smpv_ctx = NULL;
        client->smpv_enabled = false;
    }
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_handle_mempool_tx_hex(dogecoin_spv_client* client, const char* raw_tx_hex)
{
    if (!client || !client->smpv_enabled || !client->smpv_ctx || !raw_tx_hex) return false;
    return dogecoin_smpv_process_tx(
        (dogecoin_smpv_client*)client->smpv_ctx,
        raw_tx_hex,
        smpv_tx_cb,
        client
    );
}

LIBDOGECOIN_API void dogecoin_spv_get_smpv_stats(dogecoin_spv_client* client, uint32_t* total_txs, uint32_t* watched_addrs)
{
    if (total_txs) *total_txs = 0;
    if (watched_addrs) *watched_addrs = 0;
    if (!client || !client->smpv_enabled || !client->smpv_ctx) return;
    dogecoin_smpv_get_stats(
        (dogecoin_smpv_client*)client->smpv_ctx,
        total_txs, watched_addrs);
}

LIBDOGECOIN_API void dogecoin_net_spv_request_mempool(dogecoin_spv_client *client)
{
    if (!client || !client->nodegroup || !client->nodegroup->nodes) return;
    vector_t* nodes = client->nodegroup->nodes;
    for (unsigned int i = 0; i < (unsigned int)nodes->len; i++) {
        dogecoin_node* n = (dogecoin_node*)vector_idx(nodes, i);
        if (!n) continue;
        cstring* payload = cstr_new_sz(0);
        cstring* msg = dogecoin_p2p_message_new(
            n->nodegroup->chainparams->netmagic,
            DOGECOIN_MSG_MEMPOOL,
            payload->str,
            payload->len
        );
        cstr_free(payload, true);
        dogecoin_node_send(n, msg);
        cstr_free(msg, true);
    }
    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[spv] sent 'mempool' to peers\n");
}

LIBDOGECOIN_API void dogecoin_net_spv_request_filtered_history(dogecoin_spv_client *client, int depth)
{
    /* BIP37-oriented historical scan: only request FILTERED_BLOCK when bloom is active. */
    if (!client || !client->nodegroup || !client->nodegroup->nodes) return;
    if (!client->bloom_filter || client->bloom_filter_len == 0) return;

    /* Walk backwards from the chain tip collecting block hashes. */
    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
    if (!tip) return;

    int tip_height = (int)tip->height;
    int request_end_height = tip_height;
    int start_height = tip_height;
    dogecoin_blockindex* cur = tip;

    /* depth <= 0 means "scan all available blocks (to checkpoint/genesis)". */
    if (depth > 0) {
        start_height = tip_height - depth + 1;
        if (start_height < 0) start_height = 0;
    } else {
        dogecoin_headers_db* hdb = (dogecoin_headers_db*)client->headers_db_ctx;
        int file_start_height = -1;

        while (cur && cur->prev) cur = cur->prev;
        if (cur) start_height = (int)cur->height;

        if (hdb && hdb->headers_tree_file) {
            long old_pos = ftell(hdb->headers_tree_file);
            dogecoin_bool can_restore_pos = (old_pos >= 0);
            uint8_t rec[SPV_HEADERS_FILE_REC_LEN];
            if (fseek(hdb->headers_tree_file, SPV_HEADERS_FILE_HDR_LEN, SEEK_SET) == 0 &&
                fread(rec, sizeof(rec), 1, hdb->headers_tree_file) == 1) {
                struct const_buffer rec_buf = { rec, sizeof(rec) };
                uint256_t first_block_hash;
                uint32_t h = 0;
                uint256_t first_block_chainwork;
                deser_u256(first_block_hash, &rec_buf);
                deser_u32(&h, &rec_buf);
                deser_u256(first_block_chainwork, &rec_buf);
                file_start_height = (int)h;
            }
            if (can_restore_pos) fseek(hdb->headers_tree_file, old_pos, SEEK_SET);
        }
        if (file_start_height >= 0 && file_start_height < start_height) start_height = file_start_height;
        cur = tip;
    }

    /* Avoid re-requesting historical filtered blocks already requested in this process. */
    if (client->filtered_history_last_end_height >= start_height) {
        start_height = client->filtered_history_last_end_height + 1;
    }
    if (start_height > tip_height) return;

    int total = (tip_height - start_height) + 1;
    if (total == 0) return;

    /* Bound each historical request window so peers can realistically serve it. */
    {
        const int max_historical_request_blocks = 50000;
        if (total > max_historical_request_blocks) {
            request_end_height = start_height + max_historical_request_blocks - 1;
            total = max_historical_request_blocks;
        }
    }

    /* Find one connected bloom-capable peer and keep historical requests serialized on it.
       This avoids interleaved merkleblock streams that can constantly reset match state. */
    dogecoin_node* history_peer = NULL;
    unsigned int ni;
    for (ni = 0; ni < (unsigned int)client->nodegroup->nodes->len; ni++) {
        dogecoin_node* n = (dogecoin_node*)vector_idx(client->nodegroup->nodes, ni);
        if (!n || ((n->state & NODE_CONNECTED) != NODE_CONNECTED) ||
            ((n->state & NODE_MISSBEHAVED) == NODE_MISSBEHAVED) ||
            !n->version_handshake) continue;
        if ((n->services & DOGECOIN_NODE_BLOOM) != 0) {
            history_peer = n;
            break;
        }
    }
    if (!history_peer) {
        if (client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb("[spv] skipped historical filtered request: no connected bloom-capable peer available\n");
        }
        return;
    }

    /* Ensure selected historical peer has our current bloom filter loaded. */
    spv_send_filterload_to_node(history_peer,
                                client->bloom_filter,
                                client->bloom_filter_len,
                                client->bloom_nhashfunc,
                                client->bloom_ntweak,
                                client->bloom_flags);

    /* Reset rescan progress counters. */
    client->rescan_total = 0;
    client->rescan_matched = 0;

    if (client->nodegroup->log_write_cb) {
        client->nodegroup->log_write_cb("[spv] scanning %d historical blocks for UTXO discovery (only matches will be logged)\n", total);
    }

    /* Persist requested historical end before dispatching getdata so
       follow-up tail re-requests can be scheduled immediately from
       early matched transactions. */
    client->filtered_history_last_end_height = request_end_height;

    /* Send getdata in batches to avoid huge allocations and messages.
       We walk backwards collecting a batch of hashes, reverse them (oldest first),
       then send and repeat. */
    int batch_max = 500;
    cur = tip;
    while (cur && (int)cur->height > request_end_height) cur = cur->prev;

    /* Collect block hashes oldest->newest for requested range. */
    uint8_t* block_hashes = (uint8_t*)dogecoin_calloc((size_t)total, 32);
    if (!block_hashes) return;

    dogecoin_bool collected = true;
    cur = tip;
    while (cur && (int)cur->height > request_end_height) cur = cur->prev;
    int idx = total - 1;
    while (cur && idx >= 0) {
        if ((int)cur->height < start_height) break;
        memcpy(block_hashes + ((size_t)idx * 32), cur->hash, 32);
        idx--;
        cur = cur->prev;
    }

    /* If in-memory chain is truncated, backfill range from headers file records. */
    if (idx >= 0) {
        dogecoin_headers_db* hdb = (dogecoin_headers_db*)client->headers_db_ctx;
        uint8_t* slot_filled = NULL;
        int found = 0;

        collected = false;
        if (hdb && hdb->headers_tree_file) {
            long old_pos = ftell(hdb->headers_tree_file);
            dogecoin_bool can_restore_pos = (old_pos >= 0);
            uint8_t rec[SPV_HEADERS_FILE_REC_LEN];
            slot_filled = (uint8_t*)dogecoin_calloc((size_t)total, 1);
            if (slot_filled && fseek(hdb->headers_tree_file, SPV_HEADERS_FILE_HDR_LEN, SEEK_SET) == 0) {
                while (fread(rec, sizeof(rec), 1, hdb->headers_tree_file) == 1) {
                    struct const_buffer rec_buf = { rec, sizeof(rec) };
                    uint256_t hash;
                    uint32_t h = 0;
                    uint256_t cw_unused;
                    int offset;
                    deser_u256(hash, &rec_buf);
                    deser_u32(&h, &rec_buf);
                    deser_u256(cw_unused, &rec_buf);
                    if ((int)h < start_height || (int)h > request_end_height) continue;
                    offset = (int)h - start_height;
                    if (!slot_filled[offset]) {
                        memcpy(block_hashes + ((size_t)offset * 32), hash, 32);
                        slot_filled[offset] = 1;
                        found++;
                        if (found == total) break;
                    }
                }
            }
            if (can_restore_pos) fseek(hdb->headers_tree_file, old_pos, SEEK_SET);
            if (found == total) collected = true;
        }
        if (slot_filled) dogecoin_free(slot_filled);
    }
    if (!collected) {
        if (client->nodegroup->log_write_cb) {
            client->nodegroup->log_write_cb("[spv] skipped historical filtered request due to incomplete local header range (start_height=%d, end=%d)\n",
                start_height, request_end_height);
        }
        dogecoin_free(block_hashes);
        return;
    }

    {
        int blocks_sent = 0;
        while (blocks_sent < total) {
            int batch = total - blocks_sent;
            if (batch > batch_max) batch = batch_max;

            /* Build getdata payload: varint(count) + count * (uint32 type + uint256 hash) */
            cstring* payload = cstr_new_sz(9 + (size_t)batch * 36);
            if (!payload) break;

            ser_varlen(payload, (uint32_t)batch);

            int bi;
            for (bi = 0; bi < batch; bi++) {
                uint32_t type = DOGECOIN_INV_TYPE_FILTERED_BLOCK;
                ser_u32(payload, type);
                ser_bytes(payload, block_hashes + ((size_t)(blocks_sent + bi) * 32), 32);
            }

            cstring *p2p_msg = dogecoin_p2p_message_new(
                history_peer->nodegroup->chainparams->netmagic,
                DOGECOIN_MSG_GETDATA,
                (const uint8_t*)payload->str, payload->len);
            dogecoin_node_send(history_peer, p2p_msg);
            cstr_free(p2p_msg, true);
            cstr_free(payload, true);

            blocks_sent += batch;
        }
    }

    if (client->nodegroup->log_write_cb) {
        client->nodegroup->log_write_cb("[spv] requested %d historical filtered blocks (heights %d-%d) via bloom peer %d\n",
            total, start_height, request_end_height, history_peer->nodeid);
    }

    dogecoin_free(block_hashes);
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filterload(
    dogecoin_spv_client* client,
    const uint8_t* filter,
    uint32_t filter_len,
    uint32_t nHashFuncs,
    uint32_t nTweak,
    uint8_t flags)
{
    if (!client || !filter || filter_len == 0) return false;

    /* BIP37 and BIP157 are mutually exclusive, and this is a privacy control,
     * not an optimisation: a FILTERLOAD hands the peer a bloom filter of the
     * exact scripts being watched, which is the fingerprint compact filters
     * exist to avoid.  Compact filters are enabled by default, so silently
     * honouring a filterload here would leak the wallet to every connected
     * peer of a client that chose BIP157 precisely to prevent that.  Fail
     * closed and send nothing; callers wanting BIP37 must first disable
     * compact filters via dogecoin_spv_enable_compact_filters(client, false).
     * dogecoin_spv_client_filteradd() applies the same rule. */
    if (client->compact_filters_enabled) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[spv] refusing filterload: compact filters (BIP157) are enabled; "
                "a BIP37 bloom filter would leak the watched scripts to peers. "
                "To use BIP37 anyway, call dogecoin_spv_enable_compact_filters(client, false) "
                "first (spvnode: -e/--no_cfilters).\n");
        return false;
    }

    if (client->bloom_filter) {
        dogecoin_free(client->bloom_filter);
        client->bloom_filter = NULL;
    }

    client->bloom_filter = (uint8_t*)dogecoin_calloc(filter_len, 1);
    if (!client->bloom_filter) return false;

    memcpy(client->bloom_filter, filter, filter_len);
    client->bloom_filter_len = filter_len;
    client->bloom_nhashfunc = nHashFuncs;
    client->bloom_ntweak = nTweak;
    client->bloom_flags = flags;

    if (!client->nodegroup || !client->nodegroup->nodes) return true;

    for (unsigned int i = 0; i < (unsigned int)client->nodegroup->nodes->len; i++) {
        dogecoin_node* n = (dogecoin_node*)vector_idx(client->nodegroup->nodes, i);
        if (!n) continue;
        if (((n->state & NODE_CONNECTED) != NODE_CONNECTED) || !n->version_handshake) continue;
        spv_send_filterload_to_node(n, filter, filter_len, nHashFuncs, nTweak, flags);
    }

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[spv] filterload set (len=%u)\n", filter_len);

    return true;
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filteradd(
    dogecoin_spv_client* client,
    const uint8_t* data,
    uint32_t data_len)
{
    if (!client || !data || data_len == 0) return false;

    /* BIP157: populate watched_scripts for compact filter matching */
    if (client->compact_filters_enabled && client->cfilter_state &&
        client->cfilter_state->watched_scripts) {
        cstring *script = cstr_new_buf(data, data_len);
        if (script)
            vector_add(client->cfilter_state->watched_scripts, script);
        return true;  /* skip BIP37 bloom filter and P2P FILTERADD when using compact filters */
    }

    if (client->bloom_filter && client->bloom_filter_len > 0) {
        dogecoin_bip37_filter local_filter;
        memset(&local_filter, 0, sizeof(local_filter));
        local_filter.data = client->bloom_filter;
        local_filter.data_len = client->bloom_filter_len;
        local_filter.n_hash_funcs = client->bloom_nhashfunc;
        local_filter.n_tweak = client->bloom_ntweak;
        local_filter.n_flags = client->bloom_flags;
        dogecoin_bip37_filter_add(&local_filter, data, data_len);
    }
    if (!client->nodegroup || !client->nodegroup->nodes) return true;

    cstring* payload = cstr_new_sz((size_t)data_len + SPV_VARINT_MAX_LEN);
    if (!payload) return false;

    ser_varlen(payload, data_len);
    ser_bytes(payload, data, data_len);

    for (unsigned int i = 0; i < (unsigned int)client->nodegroup->nodes->len; i++) {
        dogecoin_node* n = (dogecoin_node*)vector_idx(client->nodegroup->nodes, i);
        if (!n) continue;
        if (((n->state & NODE_CONNECTED) != NODE_CONNECTED) || !n->version_handshake) continue;

        cstring* msg = dogecoin_p2p_message_new(
            n->nodegroup->chainparams->netmagic,
            DOGECOIN_MSG_FILTERADD,
            (const uint8_t*)payload->str,
            payload->len
        );
        dogecoin_node_send(n, msg);
        cstr_free(msg, true);
    }

    cstr_free(payload, true);

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[spv] sent filteradd (len=%u)\n", data_len);

    return true;
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_filterclear(dogecoin_spv_client* client)
{
    if (!client) return false;

    if (client->bloom_filter) {
        dogecoin_free(client->bloom_filter);
        client->bloom_filter = NULL;
    }
    client->bloom_filter_len = 0;
    client->bloom_nhashfunc = 0;
    client->bloom_ntweak = 0;
    client->bloom_flags = 0;

    if (!client->nodegroup || !client->nodegroup->nodes) return true;

    for (unsigned int i = 0; i < (unsigned int)client->nodegroup->nodes->len; i++) {
        dogecoin_node* n = (dogecoin_node*)vector_idx(client->nodegroup->nodes, i);
        if (!n) continue;
        if (((n->state & NODE_CONNECTED) != NODE_CONNECTED) || !n->version_handshake) continue;

        cstring* payload = cstr_new_sz(0);
        cstring* msg = dogecoin_p2p_message_new(
            n->nodegroup->chainparams->netmagic,
            DOGECOIN_MSG_FILTERCLEAR,
            payload->str,
            payload->len
        );
        cstr_free(payload, true);
        dogecoin_node_send(n, msg);
        cstr_free(msg, true);
    }

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[spv] sent filterclear\n");

    return true;
}


/* ================================================================ */
/* Parallel genesis header download                                  */
/* ================================================================ */

/* Build segments from the chainparams block-header checkpoint array.
 * Segment i covers the open-closed height range (prev_checkpoint, checkpoint[i]].
 * Segment 0 starts from the genesis block hash (height 0).
 * Returns a newly allocated par_hdr_state, or NULL for chains with no checkpoints. */
static par_hdr_state *par_hdr_init(const dogecoin_chainparams *params)
{
    const dogecoin_checkpoint *arr = NULL;
    size_t cnt = 0;
    if (strcmp(params->chainname, "main") == 0) {
        arr = dogecoin_mainnet_checkpoint_array;
        cnt = dogecoin_mainnet_checkpoint_count;
    } else if (strcmp(params->chainname, "test") == 0) {
        arr = dogecoin_testnet_checkpoint_array;
        cnt = dogecoin_testnet_checkpoint_count;
    }
    if (!cnt) return NULL;

    /* arr[0] is the genesis entry (height=0) — it is the start anchor for the
     * first segment, not an endpoint to download toward.  Skip it so segment 0
     * covers the range genesis..arr[1] instead of the degenerate 0..0 range. */
    size_t start_i = (cnt > 0 && arr[0].height == 0) ? 1 : 0;
    size_t num_segs = cnt - start_i;
    if (!num_segs) return NULL;

    par_hdr_state *s = dogecoin_calloc(1, sizeof(par_hdr_state));
    s->segs = dogecoin_calloc(num_segs, sizeof(par_hdr_seg));
    s->num_segs = (uint32_t)num_segs;
    s->active = true;
    s->last_flush_idx     = 0;
    s->last_progress_time = (uint64_t)time(NULL);

    for (uint32_t i = 0; i < (uint32_t)num_segs; i++) {
        uint32_t arr_i = (uint32_t)(start_i + i);
        par_hdr_seg *seg = &s->segs[i];

        /* stop = checkpoint[arr_i] */
        seg->stop_height = arr[arr_i].height;
        utils_uint256_sethex((char *)arr[arr_i].hash, seg->stop_hash);


        /* start = genesis when i==0, else checkpoint[arr_i-1] */
        if (i == 0) {
            seg->start_height = 0;
            memcpy(seg->start_hash, params->genesisblockhash, sizeof(uint256_t));
        } else {
            seg->start_height = arr[arr_i - 1].height;
            utils_uint256_sethex((char *)arr[arr_i - 1].hash, seg->start_hash);
        }

        seg->node_id    = -1;
        seg->shadow_id  = -1;
        seg->shadow_at  = 0;
        seg->tip_height = seg->start_height;
        memcpy(seg->tip_hash, seg->start_hash, sizeof(uint256_t));

        seg->cap = 2048;
        seg->buf = dogecoin_malloc((size_t)seg->cap * PAR_HDR_RAW_LEN);
    }
    return s;
}

/* Send a getheaders request to @node for the next batch in @seg. */
static dogecoin_node *par_hdr_node_by_id(dogecoin_spv_client *client, int node_id);

static void par_hdr_send_getheaders(dogecoin_node *node, par_hdr_seg *seg)
{
    seg->requested_at = (uint64_t)time(NULL);

    vector_t *locators = vector_new(1, free);
    uint256_t *loc = dogecoin_calloc(1, sizeof(uint256_t));
    memcpy(loc, seg->tip_hash, sizeof(uint256_t));
    vector_add(locators, loc);

    cstring *msg = cstr_new_sz(512);
    dogecoin_p2p_msg_getheaders(locators, (uint8_t *)seg->stop_hash, msg);
    vector_free(locators, true);

    cstring *p2p = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_GETHEADERS, msg->str, msg->len);
    cstr_free(msg, true);
    dogecoin_node_send(node, p2p);
    cstr_free(p2p, true);

    node->state |= NODE_HEADERSYNC;
}

/* Assign the next unassigned segment to @node and send the first getheaders. */
LIBDOGECOIN_API void par_hdr_assign(dogecoin_spv_client *client, dogecoin_node *node)
{
    par_hdr_state *s = client->par_hdr;
    if (!s || !s->active) return;
    if (!(node->state & NODE_CONNECTED) || !node->version_handshake) return;

    /* Skip nodes already working on a segment */
    for (uint32_t i = 0; i < s->num_segs; i++) {
        if (s->segs[i].node_id == (int)node->nodeid ||
            s->segs[i].shadow_id == (int)node->nodeid) return;
    }

    /* Lowest-index segment that is neither complete nor currently owned.  A
       released segment keeps its buffered headers and tip_hash, so a new owner
       resumes where the previous one stopped rather than restarting. */
    par_hdr_seg *seg = NULL;
    uint32_t seg_idx = 0;
    for (uint32_t i = s->flush_idx; i < s->num_segs; i++) {
        if (!s->segs[i].complete && s->segs[i].node_id == -1) {
            /* The flush head is always allowed: refusing it would deadlock,
               since nothing can drain while it is unowned. */
            if (i != s->flush_idx && s->buffered_bytes >= PAR_HDR_MAX_BUFFERED)
                return;
            seg = &s->segs[i];
            seg_idx = i;
            break;
        }
    }
    if (!seg) return; /* nothing outstanding */

    seg->node_id         = (int)node->nodeid;
    seg->assigned_at     = (uint64_t)time(NULL);
    seg->count_at_assign = seg->count;

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[par-hdr] assigned node %d to segment %u (heights %u..%u)\n",
            node->nodeid, seg_idx,
            seg->tip_height + 1, seg->stop_height);

    par_hdr_send_getheaders(node, seg);
}

/* Flush completed segments (in order) into the primary headers DB.
 * Returns the number of segments flushed. */
/* Check the invariants the segment array is supposed to hold, and log any that
 * do not. Returns the number of violations, 0 when healthy.
 *
 * Deliberately not assert(). This codebase compiles with NDEBUG in release, so
 * an assert here would document the invariants while checking them only in
 * builds nobody ships. These conditions are cheap -- one pass over ~89
 * segments -- and the failures they describe are silent ones, which is exactly
 * the case for paying at runtime.
 *
 * Called after the operations that mutate segment ownership or flush state.
 * It reports rather than aborts: a violation means a bug in this file, and
 * killing a node mid-sync is a worse outcome than finishing with a diagnostic
 * in the log. */
static uint32_t par_hdr_check(dogecoin_spv_client *client, const char *where)
{
    par_hdr_state *s = client->par_hdr;
    uint32_t bad = 0;
    uint32_t i;

    if (!s) return 0;

#define PAR_HDR_BAD(fmt, ...)                                                  \
    do {                                                                       \
        bad++;                                                                 \
        if (client->nodegroup && client->nodegroup->log_write_cb)              \
            client->nodegroup->log_write_cb("[par-hdr][INVARIANT] %s: " fmt,   \
                                            where, __VA_ARGS__);               \
    } while (0)

    /* flush_idx only ever moves forward, and never past the end. */
    if (s->flush_idx > s->num_segs)
        PAR_HDR_BAD("flush_idx %u exceeds num_segs %u\n",
                    s->flush_idx, s->num_segs);

    for (i = 0; i < s->num_segs; i++) {
        const par_hdr_seg *seg = &s->segs[i];

        /* A segment below flush_idx has been written to the DB; one at or
         * above it has not. Ordered flushing is what lets the downloader skip
         * proof-of-work, so a hole here breaks the anchoring argument. */
        if (i < s->flush_idx && !seg->flushed)
            PAR_HDR_BAD("segment %u is below flush_idx %u but not flushed\n",
                        i, s->flush_idx);
        if (i >= s->flush_idx && seg->flushed)
            PAR_HDR_BAD("segment %u is at/above flush_idx %u but flushed\n",
                        i, s->flush_idx);

        /* Flushed implies complete, and implies the staging buffer is gone --
         * that release is what keeps peak RSS off the size of the chain. */
        if (seg->flushed && !seg->complete)
            PAR_HDR_BAD("segment %u flushed but not complete\n", i);
        if (seg->flushed && seg->buf)
            PAR_HDR_BAD("segment %u flushed but still holds its buffer\n", i);

        /* A completed segment is owned by nobody: par_hdr_assign only
         * considers !complete && node_id == -1, so a complete segment with an
         * owner leaks that peer out of the assignment pool for the rest of the
         * run. */
        if (seg->complete && seg->node_id != -1)
            PAR_HDR_BAD("segment %u complete but still owned by node %d\n",
                        i, seg->node_id);
        if (seg->complete && seg->shadow_id != -1)
            PAR_HDR_BAD("segment %u complete but still shadowed by node %d\n",
                        i, seg->shadow_id);

        /* A shadow races the owner; it is meaningless without one, and a peer
         * cannot race itself. */
        if (seg->shadow_id != -1 && seg->node_id == -1)
            PAR_HDR_BAD("segment %u has shadow %d but no owner\n",
                        i, seg->shadow_id);
        if (seg->shadow_id != -1 && seg->shadow_id == seg->node_id)
            PAR_HDR_BAD("segment %u shadowed by its own owner %d\n",
                        i, seg->node_id);

        /* Heights bracket the range this segment is responsible for. */
        if (seg->stop_height <= seg->start_height)
            PAR_HDR_BAD("segment %u stop %u is not above start %u\n",
                        i, seg->stop_height, seg->start_height);
        if (seg->tip_height < seg->start_height)
            PAR_HDR_BAD("segment %u tip %u is below start %u\n",
                        i, seg->tip_height, seg->start_height);

        /* count is the number of headers staged in buf -- but only until the
         * segment is flushed, after which the buffer is released and count is
         * kept as the record of how many headers went in. So this pair only
         * describes unflushed segments. */
        if (!seg->flushed) {
            if (seg->count > 0 && !seg->buf)
                PAR_HDR_BAD("segment %u claims %u staged headers with no buffer\n",
                            i, seg->count);
            if (seg->count > seg->cap)
                PAR_HDR_BAD("segment %u count %u exceeds capacity %u\n",
                            i, seg->count, seg->cap);
        }

        /* One peer, one segment. Two segments owned by the same node means
         * par_hdr_recv cannot tell which one a batch belongs to -- it matches
         * on node_id and takes the first. */
        if (seg->node_id != -1) {
            uint32_t j;
            for (j = i + 1; j < s->num_segs; j++) {
                if (s->segs[j].complete) continue;
                if (s->segs[j].node_id == seg->node_id)
                    PAR_HDR_BAD("node %d owns both segment %u and %u\n",
                                seg->node_id, i, j);
            }
        }
    }

#undef PAR_HDR_BAD

    return bad;
}

/* Verify proof of work on the header that lands on a segment's checkpoint.
 *
 * The flush loop turns skip_pow on for the whole batch. The justification is
 * that both ends of a segment are pinned to a compiled-in checkpoint, so the
 * headers between them are implied by the hash chain -- but that argument only
 * holds if the segment really reaches its checkpoint with real work behind it,
 * and until now nothing checked the work at all. The terminal hash comparison
 * in par_hdr_recv proves the segment arrived at the right block; this proves
 * the block was mined.
 *
 * One header per segment: 114 checks on mainnet, against ~6.2M headers.
 * Merge-mined headers are the overwhelming majority (everything above 371337),
 * and for those the scrypt target applies to the parent block, so the check is
 * check_auxpow over the retained proof rather than a hash of the 80 bytes. */
static dogecoin_bool par_hdr_verify_tail(dogecoin_spv_client *client,
                                         par_hdr_seg *seg, uint32_t seg_idx)
{
    if (!seg->tail_raw || seg->tail_len < PAR_HDR_RAW_LEN) {
        /* A complete segment always retained its final header. Missing means
           the segment was rebuilt after a failed flush without re-reaching the
           checkpoint, so refuse rather than write it on trust. */
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] segment %u: no retained terminal header to verify\n",
                seg_idx);
        return false;
    }

    dogecoin_chainparams *params = (dogecoin_chainparams *)client->chainparams;
    arith_uint256 chainwork = {{0}};
    dogecoin_block_header hdr;
    dogecoin_mem_zero(&hdr, sizeof(hdr));

    /* Deserializing the whole record with params runs check_auxpow on the
       AuxPoW branch; the 80-byte-only call in par_hdr_recv cannot, because the
       proof is not in that span. */
    struct const_buffer tbuf = { seg->tail_raw, seg->tail_len };
    if (!dogecoin_block_header_deserialize(&hdr, &tbuf, params, &chainwork)) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] segment %u: terminal header failed validation at height %u\n",
                seg_idx, seg->stop_height);
        return false;
    }

    if (!is_auxpow(hdr.version)) {
        uint256_t hash = {0};
        cstring *s80 = cstr_new_sz(96);
        dogecoin_block_header_serialize(s80, &hdr);
        dogecoin_block_header_scrypt_hash(s80, &hash);
        cstr_free(s80, true);
        if (!check_pow(&hash, hdr.bits, client->chainparams, &chainwork)) {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: terminal header fails proof of work at height %u\n",
                    seg_idx, seg->stop_height);
            return false;
        }
    }
    return true;
}

static uint32_t par_hdr_flush(dogecoin_spv_client *client)
{
    par_hdr_state *s = client->par_hdr;
    uint32_t flushed = 0;

    /* Batch-optimise writes: suppress per-record fdatasync and skip scrypt
     * PoW verification.  Segments are checkpoint-anchored so the chain is
     * implicitly validated.  A single fsync at the end suffices. */
    dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
    dogecoin_bool prev_batch = hdb ? hdb->batch_write : false;
    dogecoin_bool prev_skip  = hdb ? hdb->skip_pow    : false;
    if (hdb) { hdb->batch_write = true; hdb->skip_pow = true; }

    uint32_t flush_idx_before = s->flush_idx;

    while (s->flush_idx < s->num_segs && s->segs[s->flush_idx].complete &&
           !s->segs[s->flush_idx].flushed) {
        par_hdr_seg *seg = &s->segs[s->flush_idx];

        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] flushing segment %u (%u headers, heights %u..%u)\n",
                s->flush_idx, seg->count,
                seg->start_height + 1, seg->stop_height);

        /* Check the work behind the checkpoint header before writing any of
         * this segment. A failure takes the same recovery path as a connect
         * failure below: nothing is committed, the segment is dropped and
         * re-requested, usually from a different peer. */
        uint32_t bad = par_hdr_verify_tail(client, seg, s->flush_idx) ? 0 : 1;
        for (uint32_t j = 0; !bad && j < seg->count; j++) {
            struct const_buffer cbuf = {
                (const void *)(seg->buf + (size_t)j * PAR_HDR_RAW_LEN),
                PAR_HDR_RAW_LEN
            };
            dogecoin_bool connected;
            dogecoin_blockindex *pindex =
                client->headers_db->connect_hdr(client->headers_db_ctx, &cbuf, false, &connected);
            if (!connected) {
                if (bad == 0 && client->nodegroup && client->nodegroup->log_write_cb) {
                    /* Decode prev_block from the raw header for diagnostics */
                    const uint8_t *raw = seg->buf + (size_t)j * PAR_HDR_RAW_LEN;
                    /* Standard header layout: version(4) + prev_block(32) */
                    char prev_hex[65] = {0};
                    for (int _k = 0; _k < 32; _k++)
                        snprintf(prev_hex + _k*2, 3, "%02x", raw[4 + (31-_k)]);
                    char tip_hex[65] = {0};
                    if (hdb && hdb->chaintip)
                        for (int _k = 0; _k < 32; _k++)
                            snprintf(tip_hex + _k*2, 3, "%02x", ((uint8_t*)hdb->chaintip->hash)[_k]);
                    client->nodegroup->log_write_cb(
                        "[par-hdr] segment %u: first connect failure at j=%u\n"
                        "  chaintip  height=%d hash=%s\n"
                        "  prev_block in hdr=%s\n",
                        s->flush_idx, j,
                        hdb && hdb->chaintip ? (int)hdb->chaintip->height : -1, tip_hex,
                        prev_hex);
                }
                bad++;
                dogecoin_free(pindex); /* orphan — not in DB */
                /* Stop at the first failure. Header j+1 chains off header j,
                 * which is not in the DB, so nothing after this point can
                 * connect either -- continuing would allocate and free an
                 * orphan per remaining header and blur where the chain broke. */
                break;
            } else {
                if (pindex && client->header_connected)
                    client->header_connected(client);
                /* pindex is now db->chaintip — owned by the DB, do NOT free */
            }
        }

        if (bad) {
            /* Do NOT mark this segment flushed or advance flush_idx. The old
             * code did both unconditionally and freed the staging buffer, so a
             * mid-segment connect failure lost those headers permanently, left
             * a hole in the chain, and let every later segment fail the same
             * way until flush_idx ran off the end and the sync reported "all
             * segments complete" over a broken chain.
             *
             * Instead, resume the segment from what the DB actually reached.
             * Headers 0..j-1 did connect, so the chaintip is the correct
             * locator; re-requesting from start_hash would replay those and
             * fail immediately. The buffer is dropped and the segment is put
             * back up for assignment, usually landing on a different peer. */
            seg->flush_fails++;

            uint64_t staged = (uint64_t)seg->count * PAR_HDR_RAW_LEN;
            s->buffered_bytes = (s->buffered_bytes > staged)
                              ? s->buffered_bytes - staged : 0;
            dogecoin_free(seg->buf);
            seg->buf   = NULL;
            seg->cap   = 0;
            seg->count = 0;
            dogecoin_free(seg->tail_raw);
            seg->tail_raw = NULL;
            seg->tail_len = 0;

            if (hdb && hdb->chaintip) {
                memcpy(seg->tip_hash, hdb->chaintip->hash, DOGECOIN_HASH_LENGTH);
                seg->tip_height = (uint32_t)hdb->chaintip->height;
            } else {
                memcpy(seg->tip_hash, seg->start_hash, DOGECOIN_HASH_LENGTH);
                seg->tip_height = seg->start_height;
            }

            seg->complete        = false;
            seg->node_id         = -1;
            seg->shadow_id       = -1;
            seg->shadow_at       = 0;
            seg->count_at_assign = 0;

            if (seg->flush_fails >= PAR_HDR_MAX_FLUSH_FAILS) {
                /* Retrying has not helped, so the range is not merely a bad
                 * peer. Hand the rest of the chain to the sequential path,
                 * which verifies AUXPoW and can make progress from the real
                 * chaintip without trusting segment boundaries. */
                s->active = false;

                /* Segments past flush_idx may be complete and still holding
                 * their staging buffers. Nothing will flush them now, and on
                 * mainnet that is most of the chain resident for the rest of
                 * the run, so release them here rather than at teardown. */
                for (uint32_t k = s->flush_idx + 1; k < s->num_segs; k++) {
                    if (!s->segs[k].buf) continue;
                    uint64_t held = (uint64_t)s->segs[k].count * PAR_HDR_RAW_LEN;
                    s->buffered_bytes = (s->buffered_bytes > held)
                                      ? s->buffered_bytes - held : 0;
                    dogecoin_free(s->segs[k].buf);
                    s->segs[k].buf   = NULL;
                    s->segs[k].cap   = 0;
                    s->segs[k].count = 0;
                }

                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb(
                        "[par-hdr] segment %u failed to connect %u times — "
                        "disabling parallel download, falling back to "
                        "sequential from height %u\n",
                        s->flush_idx, seg->flush_fails, seg->tip_height);
            } else if (client->nodegroup && client->nodegroup->log_write_cb) {
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: connect failed, re-requesting from "
                    "height %u (attempt %u of %u)\n",
                    s->flush_idx, seg->tip_height,
                    seg->flush_fails, (uint32_t)PAR_HDR_MAX_FLUSH_FAILS);
            }
            break;
        }

        seg->flushed = true;
        s->flush_idx++;
        flushed++;

        /* The headers are on disk now — release the staging buffer.  Without
           this every flushed segment keeps its raw headers resident until
           par_hdr_free at teardown, which on mainnet is the whole chain. */
        uint64_t freed = (uint64_t)seg->count * PAR_HDR_RAW_LEN;
        s->buffered_bytes = (s->buffered_bytes > freed)
                          ? s->buffered_bytes - freed : 0;
        dogecoin_free(seg->buf);
        seg->buf = NULL;
        seg->cap = 0;
        dogecoin_free(seg->tail_raw);
        seg->tail_raw = NULL;
        seg->tail_len = 0;
    }

    if (hdb) {
        hdb->batch_write = prev_batch;
        hdb->skip_pow    = prev_skip;
        if (flushed > 0 && hdb->headers_tree_file)
            dogecoin_file_commit(hdb->headers_tree_file);
    }
    if (s->flush_idx != flush_idx_before) {
        s->last_flush_idx     = s->flush_idx;
        s->last_progress_time = (uint64_t)time(NULL);
    }

    par_hdr_check(client, "after flush");

    return flushed;
}

/* Advance @buf past the AUXPoW chain data that follows the 80-byte standard
 * header in a P2P headers message for AUXPoW blocks (version & 0x100).
 * Mirrors deserialize_dogecoin_auxpow_block's buffer consumption but skips
 * check_auxpow — PoW is guaranteed by checkpoint anchors at segment boundaries. */
static dogecoin_bool par_hdr_skip_auxpow(struct const_buffer *buf) {
    /* parent coinbase tx (variable length) */
    size_t cb_len = 0;
    dogecoin_tx *dummy = dogecoin_tx_new();
    dogecoin_bool ok = (dogecoin_bool)dogecoin_tx_deserialize(buf->p, buf->len, dummy, &cb_len);
    dogecoin_tx_free(dummy);
    if (!ok || cb_len == 0 || !deser_skip(buf, cb_len)) return false;

    /* parent_hash (32 bytes) */
    if (!deser_skip(buf, 32)) return false;

    /* parent merkle branch: count (varint) + count×32 bytes */
    uint32_t merkle_count = 0;
    if (!deser_varlen(&merkle_count, buf)) return false;
    if (merkle_count > 0 && !deser_skip(buf, (size_t)merkle_count * 32)) return false;

    /* parent_merkle_index (uint32) */
    if (!deser_skip(buf, 4)) return false;

    /* aux merkle branch: count (varint) + count×32 bytes */
    uint32_t aux_count = 0;
    if (!deser_varlen(&aux_count, buf)) return false;
    if (aux_count > 0 && !deser_skip(buf, (size_t)aux_count * 32)) return false;

    /* aux_merkle_index (uint32) */
    if (!deser_skip(buf, 4)) return false;

    /* parent block header: version(4) + prev_block(32) + merkle_root(32) + time(4) + bits(4) + nonce(4) */
    if (!deser_skip(buf, 80)) return false;

    return true;
}

/* Called from the DOGECOIN_MSG_HEADERS handler when par_hdr is active.
 * @buf points just past the varint that gave @count. */
static void par_hdr_recv(dogecoin_spv_client *client, dogecoin_node *node,
                         struct const_buffer *buf, uint32_t count)
{
    par_hdr_state *s = client->par_hdr;

    /* Find the segment assigned to this node */
    par_hdr_seg *seg = NULL;
    uint32_t seg_idx = 0;
    for (uint32_t i = 0; i < s->num_segs; i++) {
        if (s->segs[i].complete) continue;
        if (s->segs[i].node_id == (int)node->nodeid) {
            seg = &s->segs[i];
            seg_idx = i;
            break;
        }
        /* A racing shadow delivering before the owner takes the segment over.
         * Whichever peer answers first is by definition the faster one on this
         * range, and the buffered headers are shared, so the swap costs
         * nothing already downloaded. */
        if (s->segs[i].shadow_id == (int)node->nodeid) {
            dogecoin_node *old_owner = par_hdr_node_by_id(client, s->segs[i].node_id);
            if (old_owner) old_owner->state &= ~NODE_HEADERSYNC;
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: shadow node %d won the race from node %d\n",
                    i, (int)node->nodeid, s->segs[i].node_id);
            s->segs[i].node_id         = (int)node->nodeid;
            s->segs[i].shadow_id       = -1;
            s->segs[i].shadow_at       = 0;
            s->segs[i].assigned_at     = (uint64_t)time(NULL);
            s->segs[i].count_at_assign = s->segs[i].count;
            seg = &s->segs[i];
            seg_idx = i;
            break;
        }
    }
    if (!seg) {
        /* Unsolicited response — discard */
        node->state &= ~NODE_HEADERSYNC;
        return;
    }

    /* Buffer each raw 80-byte standard header.
     *
     * AUXPoW blocks (version & 0x100) carry variable-length AUXPoW chain data
     * between the standard 80-byte header and the 1-byte tx_count varint in the
     * P2P headers message.  We copy the standard 80 bytes, advance buf by 80,
     * then call par_hdr_skip_auxpow to consume the AUXPoW data without running
     * check_auxpow (checkpoint anchors at segment boundaries guarantee validity). */
    /* A batch must continue from what this segment already holds.
     *
     * Two peers can be attached to one segment while a race is resolving, and
     * a getheaders is answered relative to the locator sent at request time.
     * The loser's reply can therefore arrive after the winner has advanced the
     * segment, carrying headers that start below seg->tip_height. Appending
     * those at seg->count leaves a discontinuity in the middle of the buffer
     * that only surfaces at flush, as a connect failure.
     *
     * Compare the first header's prev_block against the hash this segment
     * expects next and drop the whole batch if it does not match. Worth doing
     * even without racing: it also rejects a peer that answers with something
     * other than the continuation it was asked for. */
    if (count > 0 && buf->len >= PAR_HDR_RAW_LEN) {
        const uint8_t *first_prev = (const uint8_t *)buf->p + 4; /* version(4) */
        /* tip_hash is always the right anchor: it is seeded from start_hash when
         * the segment is built, advanced per header received, and re-seeded from
         * the primary DB chaintip when a flush fails. Selecting start_hash on
         * count == 0 instead would be wrong in that last case -- the segment has
         * no buffered headers but the DB is already past start_height, so every
         * correctly-served batch would be rejected and the segment would be
         * re-requested forever. */
        const uint8_t *expected   = (const uint8_t *)seg->tip_hash;
        if (memcmp(first_prev, expected, DOGECOIN_HASH_LENGTH) != 0) {
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: dropping %u headers from node %d, "
                    "does not continue from height %u\n",
                    seg_idx, count, (int)node->nodeid, seg->tip_height);
            return;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (buf->len < PAR_HDR_RAW_LEN) break;

        /* Start of this header's whole wire record, kept so the segment's last
         * header can be retained with its AuxPoW proof attached. */
        const uint8_t *rec_start = (const uint8_t *)buf->p;

        /* Peek at version to detect AUXPoW (wire format: little-endian int32) */
        uint32_t wire_ver;
        memcpy(&wire_ver, buf->p, 4);
        const dogecoin_bool is_aux = (le32toh(wire_ver) & 0x100) != 0;

        /* Grow segment buffer if needed */
        if (seg->count >= seg->cap) {
            /* cap can be 0 if the buffer was released after a flush; a
               flushed segment should never be written to again, but doubling
               0 would silently never grow. */
            seg->cap  = seg->cap ? seg->cap * 2 : 2048;
            seg->buf  = dogecoin_realloc(seg->buf, (size_t)seg->cap * PAR_HDR_RAW_LEN);
        }

        /* Copy the standard 80-byte header and advance past it */
        s->buffered_bytes += PAR_HDR_RAW_LEN;
        memcpy(seg->buf + (size_t)seg->count * PAR_HDR_RAW_LEN, buf->p, PAR_HDR_RAW_LEN);
        buf->p   = (const uint8_t *)buf->p + PAR_HDR_RAW_LEN;
        buf->len -= PAR_HDR_RAW_LEN;

        /* For AUXPoW blocks, skip the variable-length chain data */
        if (is_aux && !par_hdr_skip_auxpow(buf)) {
            /* The same correction the deserialize failure below makes: the 80
               bytes were counted but the header is never staged, and
               buffered_bytes is what gates segment assignment. Left uncorrected
               a peer sending one malformed AuxPoW blob per headers message
               inflates it 80 at a time until it passes PAR_HDR_MAX_BUFFERED,
               after which no segment but the flush head is ever assigned. */
            s->buffered_bytes -= PAR_HDR_RAW_LEN;
            break;
        }

        /* Compute header hash from the buffered 80 bytes */
        dogecoin_block_header hdr;
        struct const_buffer hbuf = { seg->buf + (size_t)seg->count * PAR_HDR_RAW_LEN,
                                     PAR_HDR_RAW_LEN };
        if (!dogecoin_block_header_deserialize(&hdr, &hbuf, client->chainparams, NULL)) {
            /* An 80-byte header that will not deserialize cannot advance the
             * continuity anchor. Counting it anyway would leave tip_hash and
             * tip_height describing header n-1 while seg->count moved to n, so
             * the next batch's prev_block check would compare against the wrong
             * hash and the discontinuity would only surface at flush, as a
             * connect failure well away from its cause. Drop the remainder of
             * the batch and let the segment be re-requested from tip_hash. */
            s->buffered_bytes -= PAR_HDR_RAW_LEN;
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: undeserializable header at index %u "
                    "from node %d, dropping rest of batch\n",
                    seg_idx, seg->count, (int)node->nodeid);
            break;
        }
        dogecoin_block_header_hash(&hdr, (uint8_t *)seg->tip_hash);
        seg->tip_height++;
        seg->count++;

        /* Retain the whole record for the header that lands on the checkpoint.
         * par_hdr_flush verifies its proof of work before writing any of the
         * segment, and for a merge-mined header that needs the AuxPoW blob,
         * which buf does not carry. Do it before the tx_count varint is
         * consumed so the span is exactly the header record. */
        if (seg->tip_height == seg->stop_height) {
            size_t rec_len = (size_t)((const uint8_t *)buf->p - rec_start);
            dogecoin_free(seg->tail_raw);
            seg->tail_raw = dogecoin_malloc(rec_len);
            memcpy(seg->tail_raw, rec_start, rec_len);
            seg->tail_len = (uint32_t)rec_len;
        }

        /* Consume the per-header transaction count. A headers message carries
         * one after every header and it is always zero, so a 1-byte skip is
         * correct in practice -- but the wire type is a varint, and saying so
         * in code rather than in a comment means a peer that encodes the zero
         * non-minimally desynchronises the parse loudly here instead of
         * silently one byte into the next header. */
        uint32_t tx_count = 0;
        if (!deser_varlen(&tx_count, buf)) break;
    }

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[par-hdr] seg %u: buffered %u, tip_height=%u, stop=%u\n",
            seg_idx, seg->count, seg->tip_height, seg->stop_height);

    if (seg->tip_height >= seg->stop_height) {
        /* The segment claims to have reached its checkpoint. Verify that it
         * actually landed on it before trusting the range.
         *
         * stop_hash comes from the checkpoint array and is sent as getheaders
         * hash_stop, but nothing compared it against what arrived. Safety was
         * emergent: a divergent segment left a chaintip the *next* segment
         * could not extend, so it surfaced one segment late, as a connect
         * failure away from its cause -- and the final segment has no next
         * segment, so nothing caught it at all.
         *
         * This is the whole justification for skipping proof-of-work between
         * checkpoints, so it is worth checking directly rather than inferring.
         * The segment has never been flushed at this point (flush only touches
         * complete && !flushed segments, in order), so rejecting it costs
         * nothing already written. */
        if (memcmp(seg->tip_hash, seg->stop_hash, DOGECOIN_HASH_LENGTH) != 0) {
            dogecoin_headers_db *hdb =
                (dogecoin_headers_db *)client->headers_db_ctx;

            seg->flush_fails++;

            char got_hex[65] = {0}, want_hex[65] = {0};
            for (int _k = 0; _k < 32; _k++) {
                snprintf(got_hex  + _k*2, 3, "%02x", ((const uint8_t *)seg->tip_hash)[31-_k]);
                snprintf(want_hex + _k*2, 3, "%02x", ((const uint8_t *)seg->stop_hash)[31-_k]);
            }
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] segment %u: terminal hash mismatch at height %u "
                    "from node %d (attempt %u)\n"
                    "  expected %s\n"
                    "  got      %s\n",
                    seg_idx, seg->tip_height, (int)node->nodeid,
                    seg->flush_fails, want_hex, got_hex);

            uint64_t staged = (uint64_t)seg->count * PAR_HDR_RAW_LEN;
            s->buffered_bytes = (s->buffered_bytes > staged)
                              ? s->buffered_bytes - staged : 0;
            dogecoin_free(seg->buf);
            seg->buf   = NULL;
            seg->cap   = 0;
            seg->count = 0;
            dogecoin_free(seg->tail_raw);
            seg->tail_raw = NULL;
            seg->tail_len = 0;

            /* Resume from the primary DB if it is already inside this segment
             * -- that only happens when an earlier flush failed partway and
             * left the chaintip mid-range -- otherwise from the checkpoint
             * anchor, since nothing from this segment reached the DB. */
            if (hdb && hdb->chaintip &&
                (uint32_t)hdb->chaintip->height >  seg->start_height &&
                (uint32_t)hdb->chaintip->height <= seg->stop_height) {
                memcpy(seg->tip_hash, hdb->chaintip->hash, DOGECOIN_HASH_LENGTH);
                seg->tip_height = (uint32_t)hdb->chaintip->height;
            } else {
                memcpy(seg->tip_hash, seg->start_hash, DOGECOIN_HASH_LENGTH);
                seg->tip_height = seg->start_height;
            }

            seg->complete        = false;
            seg->node_id         = -1;
            seg->shadow_id       = -1;
            seg->shadow_at       = 0;
            seg->count_at_assign = 0;
            node->state &= ~NODE_HEADERSYNC;

            if (seg->flush_fails >= PAR_HDR_MAX_FLUSH_FAILS) {
                s->active = false;
                for (uint32_t k = 0; k < s->num_segs; k++) {
                    if (k == seg_idx || !s->segs[k].buf) continue;
                    uint64_t held = (uint64_t)s->segs[k].count * PAR_HDR_RAW_LEN;
                    s->buffered_bytes = (s->buffered_bytes > held)
                                      ? s->buffered_bytes - held : 0;
                    dogecoin_free(s->segs[k].buf);
                    s->segs[k].buf   = NULL;
                    s->segs[k].cap   = 0;
                    s->segs[k].count = 0;
                }
                if (client->nodegroup && client->nodegroup->log_write_cb)
                    client->nodegroup->log_write_cb(
                        "[par-hdr] segment %u failed terminal check %u times — "
                        "disabling parallel download, falling back to "
                        "sequential from height %u\n",
                        seg_idx, seg->flush_fails, seg->tip_height);
            } else {
                par_hdr_assign(client, node);
            }
            return;
        }

        /* Segment complete */
        seg->complete = true;
        seg->node_id  = -1;
        seg->shadow_id = -1;
        seg->shadow_at = 0;
        node->state  &= ~NODE_HEADERSYNC;

        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] segment %u complete (%u headers)\n", seg_idx, seg->count);

        /* Try to flush as many ordered completed segments as possible */
        par_hdr_flush(client);

        /* Re-assign this node to the next segment */
        par_hdr_assign(client, node);

        par_hdr_check(client, "after segment complete");

        /* Check if all segments are done */
        if (s->flush_idx >= s->num_segs) {
            s->active = false;
            dogecoin_blockindex *tip =
                client->headers_db->getchaintip(client->headers_db_ctx);
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb(
                    "[par-hdr] all segments complete — primary DB height %d [%us elapsed]\n",
                    tip ? (int)tip->height : -1, spv_elapsed(client));
        }
    } else {
        /* More headers needed for this segment */
        par_hdr_send_getheaders(node, seg);
    }
}

/* Look up a connected node by its nodeid, or NULL if it is gone. */
static dogecoin_node *par_hdr_node_by_id(dogecoin_spv_client *client, int nodeid)
{
    if (!client || !client->nodegroup || !client->nodegroup->nodes) return NULL;
    for (size_t i = 0; i < client->nodegroup->nodes->len; i++) {
        dogecoin_node *n = vector_idx(client->nodegroup->nodes, i);
        if (n && (int)n->nodeid == nodeid) return n;
    }
    return NULL;
}

/* Headers per second delivered by the current owner of @seg, or 0 if the
 * segment has not been owned long enough for the sample to mean anything. */
static uint32_t par_hdr_seg_rate(const par_hdr_seg *seg, uint64_t now)
{
    if (seg->node_id == -1 || seg->assigned_at == 0) return 0;
    if (now <= seg->assigned_at) return 0;
    uint64_t elapsed = now - seg->assigned_at;
    if (elapsed < PAR_HDR_RATE_GRACE) return 0;
    if (seg->count <= seg->count_at_assign) return 0;
    return (uint32_t)((seg->count - seg->count_at_assign) / elapsed);
}

/* Release the flush-head segment when its owner is alive but far slower than
 * its peers and someone else is free to take over.
 *
 * A dead owner is handled by the timeout in par_hdr_reclaim.  A merely slow
 * one is not: it keeps answering inside the deadline while every segment above
 * it finishes into memory and their owners go idle, because the ordered flush
 * cannot advance past the head.  One slow peer therefore throttles the whole
 * sync.  Resuming from tip_hash means preemption costs nothing already
 * downloaded.
 */

/* Attach a second peer to an already-owned segment.
 *
 * Segments are the unit of parallelism and cannot be subdivided in flight, so
 * once fewer segments remain than there are peers, the extra peers idle and
 * the sync runs at the speed of whoever holds the last one. Preemption helps
 * only if the owner is a clear outlier; a merely mediocre peer keeps the
 * segment while faster ones sit unused.
 *
 * When there is genuine spare capacity -- strictly more idle peers than
 * incomplete segments -- hand the segment to a second peer as well and take
 * whichever reaches the stop height first. The cost is duplicated bandwidth on
 * the last few segments; the benefit is that tail latency stops depending on
 * which peer happened to draw the final segment.
 */
static void par_hdr_race_tail(dogecoin_spv_client *client, uint64_t now)
{
    par_hdr_state *s = client->par_hdr;
    if (!client->nodegroup || !client->nodegroup->nodes) return;

    uint32_t incomplete = 0;
    for (uint32_t i = 0; i < s->num_segs; i++)
        if (!s->segs[i].complete) incomplete++;
    if (incomplete == 0) return;

    /* Count peers not currently owning or shadowing anything. */
    uint32_t idle_count = 0;
    for (size_t i = 0; i < client->nodegroup->nodes->len; i++) {
        dogecoin_node *n = vector_idx(client->nodegroup->nodes, i);
        if (!n || !(n->state & NODE_CONNECTED) || !n->version_handshake) continue;
        dogecoin_bool busy = false;
        for (uint32_t k = 0; k < s->num_segs && !busy; k++)
            if (s->segs[k].node_id == (int)n->nodeid ||
                s->segs[k].shadow_id == (int)n->nodeid) busy = true;
        if (!busy) idle_count++;
    }
    if (idle_count == 0) return; /* nobody free to help */

    /* Racing used to require idle_count > incomplete, which meant it only
     * engaged once nearly every segment was done. A full mainnet run showed
     * that is far too late: the first race fired about three quarters of the
     * way through the log, so every peer that finished early sat idle while
     * mid-run segments crawled. Any idle peer is spare capacity; the question
     * is only which segment is worth spending it on.
     *
     * Shadow a segment only when its owner is running below the reference
     * rate. A peer keeping pace does not need help, and shadowing it would
     * just duplicate bandwidth. Being merely below median is enough here --
     * preemption already handles the clear outliers, and this is the cheaper
     * intervention because the original owner keeps working. */
    uint32_t ref = s->rate_ref;
    if (ref == 0) {
        /* No reference yet: sample the segments currently in flight. */
        uint32_t sum = 0, n = 0;
        for (uint32_t i = 0; i < s->num_segs; i++) {
            uint32_t r = par_hdr_seg_rate(&s->segs[i], now);
            if (r > 0) { sum += r; n++; }
        }
        if (n == 0) return;
        ref = sum / n;
    }

    /* Shadow the segments nearest the flush head first: those gate everything. */
    for (uint32_t i = s->flush_idx; i < s->num_segs && idle_count > 0; i++) {
        par_hdr_seg *seg = &s->segs[i];
        if (seg->complete || seg->node_id == -1 || seg->shadow_id != -1) continue;

        /* Leave owners that are keeping up alone. A rate of 0 means the owner
         * is still inside the grace window, which also counts as no evidence
         * of trouble. */
        uint32_t rate = par_hdr_seg_rate(seg, now);
        if (rate == 0 || rate >= ref) continue;

        dogecoin_node *cand = NULL;
        for (size_t j = 0; j < client->nodegroup->nodes->len && !cand; j++) {
            dogecoin_node *n = vector_idx(client->nodegroup->nodes, j);
            if (!n || !(n->state & NODE_CONNECTED) || !n->version_handshake) continue;
            if ((int)n->nodeid == seg->node_id) continue;
            dogecoin_bool busy = false;
            for (uint32_t k = 0; k < s->num_segs && !busy; k++)
                if (s->segs[k].node_id == (int)n->nodeid ||
                    s->segs[k].shadow_id == (int)n->nodeid) busy = true;
            if (!busy) cand = n;
        }
        if (!cand) return;

        seg->shadow_id = (int)cand->nodeid;
        seg->shadow_at = now;
        idle_count--;

        if (client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] racing segment %u: node %d shadowing node %d "
                "(%u hdr/s vs ref %u, resuming at %u)\n",
                i, seg->shadow_id, seg->node_id, rate, ref,
                seg->tip_height + 1);

        par_hdr_send_getheaders(cand, seg);
    }
}

static void par_hdr_preempt_head(dogecoin_spv_client *client, uint64_t now)
{
    par_hdr_state *s = client->par_hdr;
    if (s->flush_idx >= s->num_segs) return;

    par_hdr_seg *head = &s->segs[s->flush_idx];
    if (head->complete || head->node_id == -1) return;

    uint32_t head_rate = par_hdr_seg_rate(head, now);
    if (head_rate == 0) return; /* still inside the grace window */

    /* Median rate across the other segments currently being downloaded.
     *
     * Sized to num_segs rather than a fixed 64. The old bound stopped sampling
     * at 64 without saying so, and mainnet is already at 89 segments, so once
     * more than 64 are in flight the median is drawn from an arbitrary prefix
     * of the peer set -- and that median is the reference every preemption and
     * racing decision keys off. Truncation here is invisible in the logs and
     * would look like a tuning problem, not a sampling one. */
    uint32_t *rates = dogecoin_malloc((size_t)s->num_segs * sizeof(*rates));
    if (!rates) return;
    uint32_t n = 0;
    for (uint32_t i = 0; i < s->num_segs; i++) {
        if (i == s->flush_idx) continue;
        uint32_t r = par_hdr_seg_rate(&s->segs[i], now);
        if (r > 0) rates[n++] = r;
    }
    /* Remember the median while a crowd exists, because the tail has none:
     * once every other segment has completed their node_id is -1 and
     * par_hdr_seg_rate() reports 0 for all of them, so n falls to 0 exactly
     * when the head is the only thing left and preemption matters most.
     * Fall back to the last crowd-derived median in that case. */
    uint32_t median;
    if (n >= 3) {
        for (uint32_t i = 1; i < n; i++) {
            uint32_t v = rates[i], j = i;
            while (j > 0 && rates[j - 1] > v) { rates[j] = rates[j - 1]; j--; }
            rates[j] = v;
        }
        median = rates[n / 2];
        s->rate_ref = median;
    } else if (s->rate_ref > 0) {
        median = s->rate_ref;
    } else {
        dogecoin_free(rates);
        return; /* never saw a crowd; nothing to call an outlier against */
    }
    dogecoin_free(rates);

    if (head_rate * PAR_HDR_SLOW_FACTOR >= median) return; /* not an outlier */

    /* Only preempt if someone is actually free to pick it up. */
    dogecoin_node *idle = NULL;
    if (client->nodegroup && client->nodegroup->nodes) {
        for (size_t i = 0; i < client->nodegroup->nodes->len && !idle; i++) {
            dogecoin_node *cand = vector_idx(client->nodegroup->nodes, i);
            if (!cand) continue;
            if (!(cand->state & NODE_CONNECTED) || !cand->version_handshake) continue;
            if ((int)cand->nodeid == head->node_id) continue;
            dogecoin_bool busy = false;
            for (uint32_t k = 0; k < s->num_segs; k++) {
                if (s->segs[k].node_id == (int)cand->nodeid) { busy = true; break; }
            }
            if (!busy) idle = cand;
        }
    }
    if (!idle) return;

    dogecoin_node *owner = par_hdr_node_by_id(client, head->node_id);

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[par-hdr] preempting segment %u from node %d "
            "(%u hdr/s vs median %u); %u headers kept, resuming at %u\n",
            s->flush_idx, head->node_id, head_rate, median,
            head->count, head->tip_height + 1);

    if (owner) owner->state &= ~NODE_HEADERSYNC;
    head->node_id      = -1;
    head->requested_at = 0;
    head->assigned_at  = 0;

    par_hdr_assign(client, idle);
}

/* Periodic maintenance for a parallel header sync.
 *
 * Segments are handed out one per peer and are only cleared on completion, so
 * a peer that disconnects or goes silent mid-segment would otherwise hold it
 * forever.  Because par_hdr_flush only advances across contiguously complete
 * segments, a single stuck segment blocks every finished segment above it and
 * the sync parks with no getheaders on the wire.
 *
 * This reclaims those segments, hands free segments to idle peers, and warns
 * if the ordered flush stops advancing.
 */
LIBDOGECOIN_API void par_hdr_reclaim(dogecoin_spv_client *client, uint64_t now)
{
    if (!client || !client->par_hdr || !client->par_hdr->active) return;
    par_hdr_state *s = client->par_hdr;

    /* 1. Release segments whose owner disconnected or stopped responding. */
    for (uint32_t i = 0; i < s->num_segs; i++) {
        par_hdr_seg *seg = &s->segs[i];
        if (seg->complete || seg->node_id == -1) continue;

        dogecoin_node *owner = par_hdr_node_by_id(client, seg->node_id);
        dogecoin_bool gone = (owner == NULL) ||
                             !(owner->state & NODE_CONNECTED) ||
                             !owner->version_handshake;
        dogecoin_bool stalled = !gone && seg->requested_at > 0 &&
                                now > seg->requested_at &&
                                (now - seg->requested_at) > PAR_HDR_SEG_TIMEOUT;

        if (!gone && !stalled) continue;

        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] releasing segment %u from node %d (%s); "
                "%u headers kept, will resume at %u\n",
                i, seg->node_id, gone ? "disconnected" : "timed out",
                seg->count, seg->tip_height + 1);

        if (owner) owner->state &= ~NODE_HEADERSYNC;
        seg->node_id      = -1;
        seg->requested_at = 0;
        seg->shadow_id    = -1;
        seg->shadow_at    = 0;
    }

    /* 2. Hand any free segment to a peer that is not already working one. */
    if (client->nodegroup && client->nodegroup->nodes) {
        for (size_t i = 0; i < client->nodegroup->nodes->len; i++) {
            dogecoin_node *n = vector_idx(client->nodegroup->nodes, i);
            if (!n) continue;
            if (!(n->state & NODE_CONNECTED) || !n->version_handshake) continue;
            par_hdr_assign(client, n); /* no-ops if n already owns a segment */
        }
    }

    /* 3. A live but slow owner of the flush head throttles everything above it. */
    par_hdr_preempt_head(client, now);
    par_hdr_race_tail(client, now);

    /* 4. Warn if the ordered flush has stopped advancing. */
    if (s->flush_idx != s->last_flush_idx) {
        s->last_flush_idx     = s->flush_idx;
        s->last_progress_time = now;
    } else if (s->last_progress_time > 0 && now > s->last_progress_time &&
               (now - s->last_progress_time) > PAR_HDR_STALL_WARN) {
        uint32_t assigned = 0, blocked = 0;
        for (uint32_t i = 0; i < s->num_segs; i++) {
            if (s->segs[i].node_id != -1) assigned++;
            if (s->segs[i].complete && !s->segs[i].flushed) blocked++;
        }
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] WARNING: no flush progress for %us — "
                "flush_idx=%u/%u, %u segments assigned, %u complete but blocked\n",
                (unsigned int)(now - s->last_progress_time),
                s->flush_idx, s->num_segs, assigned, blocked);
        s->last_progress_time = now; /* rate-limit the warning */
    }
}

/* Release all par_hdr memory. */
LIBDOGECOIN_API void par_hdr_free(dogecoin_spv_client *client)
{
    if (!client || !client->par_hdr) return;
    par_hdr_state *s = client->par_hdr;
    for (uint32_t i = 0; i < s->num_segs; i++) {
        dogecoin_free(s->segs[i].buf);
        dogecoin_free(s->segs[i].tail_raw);
    }
    dogecoin_free(s->segs);
    dogecoin_free(s);
    client->par_hdr = NULL;
}

/* Drop the segments a loaded headers DB already covers.
 *
 * par_hdr_init() plans from genesis off the checkpoint array alone, and
 * spvnode enables it before loading the DB, so -H on a synced DB re-downloaded
 * segment 0 from height 1, failed to connect it to a chaintip millions of
 * blocks above, and only fell back to sequential after three attempts. */
static void par_hdr_prune_synced(dogecoin_spv_client *client)
{
    par_hdr_state *s = client ? client->par_hdr : NULL;
    if (!s || !client->headers_db) return;

    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
    if (!tip || tip->height <= 0) return;
    uint32_t tip_height = (uint32_t)tip->height;

    uint32_t pruned = 0;
    for (uint32_t i = 0; i < s->num_segs; i++) {
        par_hdr_seg *seg = &s->segs[i];
        if (seg->stop_height > tip_height) break;
        seg->complete = true;
        seg->flushed  = true;
        dogecoin_free(seg->buf);
        seg->buf   = NULL;
        seg->count = 0;
        seg->cap   = 0;
        dogecoin_free(seg->tail_raw);
        seg->tail_raw = NULL;
        seg->tail_len = 0;
        pruned = i + 1;
    }

    if (pruned == s->num_segs) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[par-hdr] headers DB at height %u covers every segment, syncing the tail sequentially\n",
                tip_height);
        par_hdr_free(client);
        return;
    }

    s->flush_idx      = pruned;
    s->last_flush_idx = pruned;

    /* The segment straddling the tip resumes from it rather than from its
       checkpoint start, so its flush connects onto the chaintip. */
    par_hdr_seg *head = &s->segs[pruned];
    if (head->start_height < tip_height) {
        head->tip_height = tip_height;
        memcpy(head->tip_hash, tip->hash, sizeof(uint256_t));
    }

    if (pruned && client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb(
            "[par-hdr] headers DB at height %u, skipping %u already-synced segments\n",
            tip_height, pruned);
}

/* Initialise parallel genesis header download for @client.
 * Builds segments from the chainparams block checkpoint array.
 * Returns true on success, false if the chain has no checkpoints. */
LIBDOGECOIN_API dogecoin_bool dogecoin_spv_client_enable_genesis_headers(
    dogecoin_spv_client *client)
{
    if (!client || !client->chainparams) return false;
    par_hdr_free(client);
    client->par_hdr = par_hdr_init(client->chainparams);
    if (!client->par_hdr) return false;
    return true;
}

/* ================================================================ */
/* BIP157: enable/disable compact filter sync                        */
/* ================================================================ */
LIBDOGECOIN_API void dogecoin_spv_enable_compact_filters(dogecoin_spv_client *client, dogecoin_bool enable)
{
    if (!client) return;

    if (enable && !client->compact_filters_enabled) {
        client->cfilter_state = dogecoin_compact_filter_state_new();
        if (client->cfilter_state) {
            client->cfilter_state->enabled = true;
            client->compact_filters_enabled = true;
            dogecoin_cf_load_hardcoded_checkpoints(client->cfilter_state, client->chainparams);
            if (client->nodegroup && client->nodegroup->log_write_cb)
                client->nodegroup->log_write_cb("[bip157] compact filter sync enabled\n");
        }
        return;
    }

    if (!enable && client->compact_filters_enabled) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb("[bip157] compact filter sync disabled\n");
        if (client->cfilter_state) {
            dogecoin_compact_filter_state_free(client->cfilter_state);
            client->cfilter_state = NULL;
        }
        client->compact_filters_enabled = false;
    }
}

/* ================================================================ */
/* BIP157: request helper functions                                  */
/* ================================================================ */

/* Find the first block-header checkpoint at height >= target_height and write
 * its hash in P2P (internal LE) byte order to hash_out.
 * Returns the checkpoint height, or 0 if no suitable checkpoint exists. */
static uint32_t cf_find_checkpoint_stop(const dogecoin_chainparams *params,
    uint32_t target_height, uint256_t hash_out)
{
    const dogecoin_checkpoint *arr = NULL;
    size_t cnt = 0;
    if (!params) return 0;
    if (strcmp(params->chainname, "main") == 0) {
        arr = dogecoin_mainnet_checkpoint_array;
        cnt = dogecoin_mainnet_checkpoint_count;
    } else if (strcmp(params->chainname, "test") == 0) {
        arr = dogecoin_testnet_checkpoint_array;
        cnt = dogecoin_testnet_checkpoint_count;
    }
    for (size_t i = 0; i < cnt; i++) {
        if (arr[i].height >= target_height) {
            /* chainparams hashes are display-order hex; utils_uint256_sethex reverses to LE */
            utils_uint256_sethex((char *)arr[i].hash, hash_out);
            return arr[i].height;
        }
    }
    return 0;
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfcheckpt(dogecoin_spv_client *client, dogecoin_node *node)
{
    if (!client || !node || !client->cfilter_state) return false;

    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
    if (!tip) return false;

    dogecoin_getcfcheckpt_msg msg;
    msg.filter_type = GCS_BASIC_FILTER_TYPE;
    memcpy(msg.stop_hash, tip->hash, sizeof(uint256_t));

    cstring *payload = cstr_new_sz(64);
    dogecoin_p2p_msg_getcfcheckpt_ser(&msg, payload);

    cstring *p2p_msg = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_GETCFCHECKPT,
        payload->str, payload->len);
    cstr_free(payload, true);
    dogecoin_node_send(node, p2p_msg);
    cstr_free(p2p_msg, true);

    client->cfilter_state->awaiting_response = true;
    client->cfilter_state->last_request_time = (uint64_t)time(NULL);

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[bip157] sent getcfcheckpt to node %d\n", node->nodeid);
    return true;
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfheaders(dogecoin_spv_client *client, dogecoin_node *node, uint32_t start_height, const uint256_t stop_hash)
{
    if (!client || !node || !client->cfilter_state) return false;

    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
    if (!tip) return false;

    uint32_t tip_height = (uint32_t)tip->height;

    /* BIP157: servers reject requests with stop_height - start_height >= MAX_GETCFHEADERS_SIZE.
     * Clamp the stop to one batch of MAX_GETCFHEADERS_SIZE filter headers. */
    uint32_t batch_stop_height = start_height + MAX_GETCFHEADERS_SIZE - 1;
    if (batch_stop_height >= tip_height)
        batch_stop_height = tip_height;

    uint256_t batch_stop_hash;
    if (batch_stop_height == tip_height) {
        /* Last (or only) batch ends exactly at the tip. */
        memcpy(batch_stop_hash, tip->hash, sizeof(uint256_t));
    } else {
        /* Find the block hash at batch_stop_height.
         * Try the in-memory prev chain first; fall back to a file scan. */
        dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
        if (!dogecoin_headers_db_get_block_hash_at_height(hdb, batch_stop_height, batch_stop_hash)) {
            /* Primary DB miss — try aux hash-lookup DB (genesis headers for filter IBD). */
            dogecoin_bool aux_found = false;
            if (client->aux_hash_db && client->aux_hash_db_ctx) {
                dogecoin_headers_db *aux = (dogecoin_headers_db *)client->aux_hash_db_ctx;
                aux_found = dogecoin_headers_db_get_block_hash_at_height(aux, batch_stop_height, batch_stop_hash);
            }
            if (!aux_found) {
                uint32_t cp_h = cf_find_checkpoint_stop(client->chainparams,
                    batch_stop_height, batch_stop_hash);
                if (cp_h > 0) {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] getcfheaders: no hash at %u, using checkpoint %u\n",
                            batch_stop_height, cp_h);
                    batch_stop_height = cp_h;
                } else {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] getcfheaders: cannot find hash at height %u, using tip\n",
                            batch_stop_height);
                    memcpy(batch_stop_hash, tip->hash, sizeof(uint256_t));
                    batch_stop_height = tip_height;
                }
            }
        }
    }

    dogecoin_getcfheaders_msg msg;
    msg.filter_type = GCS_BASIC_FILTER_TYPE;
    msg.start_height = start_height;
    memcpy(msg.stop_hash, batch_stop_hash, sizeof(uint256_t));

    cstring *payload = cstr_new_sz(64);
    dogecoin_p2p_msg_getcfheaders_ser(&msg, payload);

    cstring *p2p_msg = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_GETCFHEADERS,
        payload->str, payload->len);
    cstr_free(payload, true);
    dogecoin_node_send(node, p2p_msg);
    cstr_free(p2p_msg, true);

    client->cfilter_state->awaiting_response = true;
    client->cfilter_state->last_request_time = (uint64_t)time(NULL);
    client->cfilter_state->pending_start_height = start_height;
    memcpy(client->cfilter_state->pending_stop_hash, batch_stop_hash, sizeof(uint256_t));

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[bip157] sent getcfheaders (start=%u stop_height=%u) to node %d [%us elapsed]\n",
            start_height, batch_stop_height, node->nodeid, spv_elapsed(client));
    return true;
}

LIBDOGECOIN_API dogecoin_bool dogecoin_spv_request_cfilters(dogecoin_spv_client *client, dogecoin_node *node, uint32_t start_height, const uint256_t stop_hash)
{
    if (!client || !node || !client->cfilter_state) return false;

    dogecoin_compact_filter_state *cfstate = client->cfilter_state;
    uint32_t cfheaders_tip = cfstate->cfheaders_tip_height;

    /* BIP157: servers reject requests with stop_height - start_height >= MAX_GETCFILTERS_SIZE.
     * Clamp the batch to at most MAX_GETCFILTERS_SIZE cfilters. */
    uint32_t batch_end = start_height + MAX_GETCFILTERS_SIZE - 1;
    if (cfheaders_tip > 0 && batch_end > cfheaders_tip)
        batch_end = cfheaders_tip;

    /* Find the block hash at batch_end. */
    uint256_t batch_stop_hash;
    dogecoin_blockindex *tip = client->headers_db->getchaintip(client->headers_db_ctx);
    if (tip && batch_end == (uint32_t)tip->height) {
        memcpy(batch_stop_hash, tip->hash, sizeof(uint256_t));
    } else {
        dogecoin_headers_db *hdb = (dogecoin_headers_db *)client->headers_db_ctx;
        if (!dogecoin_headers_db_get_block_hash_at_height(hdb, batch_end, batch_stop_hash)) {
            /* Primary DB miss — try aux hash-lookup DB (genesis headers for filter IBD). */
            dogecoin_bool aux_found = false;
            if (client->aux_hash_db && client->aux_hash_db_ctx) {
                dogecoin_headers_db *aux = (dogecoin_headers_db *)client->aux_hash_db_ctx;
                aux_found = dogecoin_headers_db_get_block_hash_at_height(aux, batch_end, batch_stop_hash);
            }
            if (!aux_found) {
                uint32_t cp_h = cf_find_checkpoint_stop(client->chainparams,
                    batch_end, batch_stop_hash);
                if (cp_h > 0) {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] getcfilters: no hash at %u, using checkpoint %u\n",
                            batch_end, cp_h);
                    batch_end = cp_h;
                } else {
                    if (client->nodegroup && client->nodegroup->log_write_cb)
                        client->nodegroup->log_write_cb(
                            "[bip157] getcfilters: cannot find hash at height %u, using tip\n",
                            batch_end);
                    memcpy(batch_stop_hash, stop_hash, sizeof(uint256_t));
                    if (tip) batch_end = (uint32_t)tip->height;
                }
            }
        }
    }

    /* Every fallback above reassigns batch_end after the cap was applied: the
       checkpoint branch to an arbitrary checkpoint, the last one to the chain
       tip. A tip substitution asked for 6309000..6349751, 40751 filters against
       a limit of 1000, and the peer dropped us for it every time. Refuse rather
       than send a request the server must reject; the CF timeout retries, and a
       stalled scan is recoverable where a disconnect loop is not. */
    if (batch_end < start_height ||
        batch_end - start_height + 1 > MAX_GETCFILTERS_SIZE) {
        if (client->nodegroup && client->nodegroup->log_write_cb)
            client->nodegroup->log_write_cb(
                "[bip157] getcfilters: refusing %u..%u, over the %u limit\n",
                start_height, batch_end, (unsigned int)MAX_GETCFILTERS_SIZE);
        return false;
    }

    dogecoin_getcfilters_msg msg;
    msg.filter_type = GCS_BASIC_FILTER_TYPE;
    msg.start_height = start_height;
    memcpy(msg.stop_hash, batch_stop_hash, sizeof(uint256_t));

    cstring *payload = cstr_new_sz(64);
    dogecoin_p2p_msg_getcfilters_ser(&msg, payload);

    cstring *p2p_msg = dogecoin_p2p_message_new(
        node->nodegroup->chainparams->netmagic,
        DOGECOIN_MSG_GETCFILTERS,
        payload->str, payload->len);
    cstr_free(payload, true);
    dogecoin_node_send(node, p2p_msg);
    cstr_free(p2p_msg, true);

    cfstate->cfilter_batch_end = batch_end;
    cfstate->awaiting_response = true;
    cfstate->last_request_time = (uint64_t)time(NULL);
    cfstate->pending_start_height = start_height;
    memcpy(cfstate->pending_stop_hash, batch_stop_hash, sizeof(uint256_t));

    if (client->nodegroup && client->nodegroup->log_write_cb)
        client->nodegroup->log_write_cb("[bip157] sent getcfilters (start=%u..%u) to node %d [%us elapsed]\n",
            start_height, batch_end, node->nodeid, spv_elapsed(client));
    return true;
}
