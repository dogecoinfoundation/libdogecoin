/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_NET_NET_H__
#define __LIBDOGECOIN_NET_NET_H__

#include <dogecoin/dogecoin.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <dogecoin/buffer.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/cstr.h>
#include <dogecoin/net/protocol.h>
#include <dogecoin/vector.h>

LIBDOGECOIN_BEGIN_DECL

static const unsigned int DOGECOIN_P2P_MESSAGE_CHUNK_SIZE = 4000;

enum NODE_STATE {
    NODE_CONNECTING = (1 << 0),
    NODE_CONNECTED = (1 << 1),
    NODE_ERRORED = (1 << 2),
    NODE_TIMEOUT = (1 << 3),
    NODE_HEADERSYNC = (1 << 4),
    NODE_BLOCKSYNC	= (1 << 5),
    NODE_MISSBEHAVED = (1 << 6),
    NODE_DISCONNECTED = (1 << 7),
    NODE_DISCONNECTED_FROM_REMOTE_PEER = (1 << 8),
};

/* basic group-of-nodes structure */
struct dogecoin_node_;
typedef struct dogecoin_node_group_ {
    void* ctx; /* flexible context usefull in conjunction with the callbacks */
    struct event_base* event_base;
    vector* nodes; /* the groups nodes */
    char clientstr[1024];
    int desired_amount_connected_nodes;
    const dogecoin_chainparams* chainparams;

    /* callbacks */
    int (*log_write_cb)(const char* format, ...); /* log callback, default=printf */
    dogecoin_bool (*parse_cmd_cb)(struct dogecoin_node_* node, dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf);
    void (*postcmd_cb)(struct dogecoin_node_* node, dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf);
    void (*node_connection_state_changed_cb)(struct dogecoin_node_* node);
    dogecoin_bool (*should_connect_to_more_nodes_cb)(struct dogecoin_node_* node);
    void (*handshake_done_cb)(struct dogecoin_node_* node);
    dogecoin_bool (*periodic_timer_cb)(struct dogecoin_node_* node, uint64_t* time); // return false will cancle the internal logic
} dogecoin_node_group;

enum {
    NODE_CONNECTIONSTATE_DISCONNECTED = 0,
    NODE_CONNECTIONSTATE_CONNECTING = 5,
    NODE_CONNECTIONSTATE_CONNECTED = 50,
    NODE_CONNECTIONSTATE_ERRORED = 100,
    NODE_CONNECTIONSTATE_ERRORED_TIMEOUT = 101,
};

/* basic node structure */
typedef struct dogecoin_node_ {
    struct sockaddr addr;
    struct bufferevent* event_bev;
    struct event* timer_event;
    dogecoin_node_group* nodegroup;
    int nodeid;
    uint64_t lastping;
    uint64_t time_started_con;
    uint64_t time_last_request;
    uint256 last_requested_inv;

    cstring* recvBuffer;
    uint64_t nonce;
    uint64_t services;
    uint32_t state;
    int missbehavescore;
    dogecoin_bool version_handshake;

    unsigned int bestknownheight;

    uint32_t hints; /* can be use for user defined state */
} dogecoin_node;

LIBDOGECOIN_API int net_write_log_printf(const char* format, ...);
LIBDOGECOIN_API int net_write_log_null(const char* format, ...);

/* =================================== */
/* NODES */
/* =================================== */

/* create new node object */
LIBDOGECOIN_API dogecoin_node* dogecoin_node_new();
LIBDOGECOIN_API void dogecoin_node_free(dogecoin_node* node);

/* set the nodes ip address and port (ipv4 or ipv6)*/
LIBDOGECOIN_API dogecoin_bool dogecoin_node_set_ipport(dogecoin_node* node, const char* ipport);

/* disconnect a node */
LIBDOGECOIN_API void dogecoin_node_disconnect(dogecoin_node* node);

/* mark a node missbehave and disconnect */
LIBDOGECOIN_API dogecoin_bool dogecoin_node_misbehave(dogecoin_node* node);

/* =================================== */
/* NODE GROUPS */
/* =================================== */

/* create a new node group */
LIBDOGECOIN_API dogecoin_node_group* dogecoin_node_group_new(const dogecoin_chainparams* chainparams);
LIBDOGECOIN_API void dogecoin_node_group_free(dogecoin_node_group* group);

/* disconnect all peers */
LIBDOGECOIN_API void dogecoin_node_group_shutdown(dogecoin_node_group* group);

/* add a node to a node group */
LIBDOGECOIN_API void dogecoin_node_group_add_node(dogecoin_node_group* group, dogecoin_node* node);

/* start node groups event loop */
LIBDOGECOIN_API void dogecoin_node_group_event_loop(dogecoin_node_group* group);

/* connect to more nodes */
LIBDOGECOIN_API dogecoin_bool dogecoin_node_group_connect_next_nodes(dogecoin_node_group* group);

/* get the amount of connected nodes */
LIBDOGECOIN_API int dogecoin_node_group_amount_of_connected_nodes(dogecoin_node_group* group, enum NODE_STATE state);

/* sends version command to node */
LIBDOGECOIN_API void dogecoin_node_send_version(dogecoin_node* node);

/* send arbitrary data to node */
LIBDOGECOIN_API void dogecoin_node_send(dogecoin_node* node, cstring* data);

LIBDOGECOIN_API int dogecoin_node_parse_message(dogecoin_node* node, dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf);
LIBDOGECOIN_API void dogecoin_node_connection_state_changed(dogecoin_node* node);

/* =================================== */
/* DNS */
/* =================================== */

LIBDOGECOIN_API dogecoin_bool dogecoin_node_group_add_peers_by_ip_or_seed(dogecoin_node_group *group, const char *ips);
LIBDOGECOIN_API int dogecoin_get_peers_from_dns(const char* seed, vector* ips_out, int port, int family);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_NET_NET_H__
