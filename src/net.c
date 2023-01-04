/*

 The MIT License (MIT)

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

#ifdef _WIN32
#ifdef _MSC_VER
#include <../contrib/getopt/wingetopt.h>
#else
#include <getopt.h>
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <assert.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//MLUMIN:MSVC
#ifdef _MSC_VER
#define HAVE_STRUCT_TIMESPEC
#include <../../contrib/winpthreads/include/pthread.h>
#else
#include <pthread.h>
#endif
//MLUMIN:MSVC

#include <time.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>


#include <dogecoin/buffer.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/cstr.h>
#include <dogecoin/hash.h>
#include <dogecoin/net.h>
#include <dogecoin/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

#define UNUSED(x) (void)(x)
static const int DOGECOIN_PERIODICAL_NODE_TIMER_S = 3;
static const int DOGECOIN_PING_INTERVAL_S = 120;
static const int DOGECOIN_CONNECT_TIMEOUT_S = 10;

/**
 * This function is used to print debug messages to the log file
 * 
 * @param format The format string.
 * 
 * @return 1
 */
int net_write_log_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("DEBUG: ");
    vprintf(format, args);
    va_end(args);
    return 1;
}

/**
 * This function is used to write to the log file
 * 
 * @param format The format string for the printf function.
 * 
 * @return 1
 */
int net_write_log_null(const char* format, ...)
{
    UNUSED(format);
    return 1;
}

/**
 * If we have a complete message, parse it
 * 
 * @param bev The bufferevent that is being read from.
 * @param ctx The node object.
 * 
 * @return dogecoin_bool (uint8_t)
 */
void read_cb(struct bufferevent* bev, void* ctx)
{
    struct evbuffer* input = bufferevent_get_input(bev);
    if (!input)
        return;

    size_t length = evbuffer_get_length(input);
    dogecoin_node* node = (dogecoin_node*)ctx;

    if ((node->state & NODE_CONNECTED) != NODE_CONNECTED) {
        // ignore messages from disconnected peers
        return;
    }
    // expand the cstring buffer if required
    cstr_alloc_minsize(node->recvBuffer, node->recvBuffer->len + length);

    // copy direct to cstring, avoid another heap buffer
    evbuffer_copyout(input, node->recvBuffer->str + node->recvBuffer->len, length);
    node->recvBuffer->len += length;

    // drain the event buffer
    evbuffer_drain(input, length);

    struct const_buffer buf = {node->recvBuffer->str, node->recvBuffer->len};
    dogecoin_p2p_msg_hdr hdr;
    char* read_upto = NULL;

    do {
        if (buf.len < DOGECOIN_P2P_HDRSZ) {
            break;
        }

        dogecoin_p2p_deser_msghdr(&hdr, &buf);
        if (hdr.data_len > DOGECOIN_MAX_P2P_MSG_SIZE) {
            dogecoin_node_misbehave(node);
            return;
        }
        if (buf.len < hdr.data_len) {
            break;
        }
        if (buf.len >= hdr.data_len) {
            struct const_buffer cmd_data_buf = {buf.p, buf.len};
            if ((node->state & NODE_CONNECTED) != NODE_CONNECTED) {
                return;
            }
            dogecoin_node_parse_message(node, &hdr, &cmd_data_buf);

            buf.p = (const unsigned char*)buf.p + hdr.data_len;
            buf.len -= hdr.data_len;

            read_upto = (void*)buf.p;
        }
        if (buf.len == 0) {
            node->recvBuffer->len = 0;
            break;
        }
    } while (1);

    if (read_upto != NULL && node->recvBuffer->len != 0 && read_upto != (node->recvBuffer->str + node->recvBuffer->len)) {
        char* end = node->recvBuffer->str + node->recvBuffer->len;
        size_t available_chunk_data = end - read_upto;
        cstring* tmp = cstr_new_buf(read_upto, available_chunk_data);
        cstr_free(node->recvBuffer, true);
        node->recvBuffer = tmp;
    }
}

/**
 * This function is called when the client sends data to the server
 * 
 * @param ev The bufferevent object.
 * @param ctx The context parameter is a pointer to the context object that was 
 * passed to bufferevent_socket_new().
 */
void write_cb(struct bufferevent* ev, void* ctx)
{
    UNUSED(ev);
    UNUSED(ctx);
}

/**
 * The node_periodical_timer function is called every second by the node_timer_loop function and checks if the node is connected to the network. 
 * 
 * @param fd The file descriptor of the socket.
 * @param event The event that triggered the callback.
 * @param ctx The node object.
 * 
 * @return dogecoin_bool (uint8_t)
 */
#if defined(_WIN32) && defined(__x86_64__)
void node_periodical_timer(long long int fd, short int event, void* ctx)
#else
void node_periodical_timer(int fd, short int event, void* ctx)
#endif
{
    UNUSED(fd);
    UNUSED(event);
    dogecoin_node* node = (dogecoin_node*)ctx;
    uint64_t now = time(NULL);

    if (node->nodegroup->periodic_timer_cb)
        if (!node->nodegroup->periodic_timer_cb(node, &now))
            return;

    if (node->time_started_con + DOGECOIN_CONNECT_TIMEOUT_S < now && ((node->state & NODE_CONNECTING) == NODE_CONNECTING)) {
        node->state = 0;
        node->time_started_con = 0;
        node->state |= NODE_ERRORED;
        node->state |= NODE_TIMEOUT;
        dogecoin_node_connection_state_changed(node);
    }

    /* This is checking if the node is connected and if the last ping time is greater than the current
    time plus the ping interval. */
    if (((node->state & NODE_CONNECTED) == NODE_CONNECTED) && node->lastping + DOGECOIN_PING_INTERVAL_S < now) {
        uint64_t nonce;
        dogecoin_cheap_random_bytes((uint8_t*)&nonce, sizeof(nonce));
        cstring* pingmsg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_PING, &nonce, sizeof(nonce));
        dogecoin_node_send(node, pingmsg);
        cstr_free(pingmsg, true);
        node->lastping = now;
    }
}

/**
 * When the event callback is called it sets the node's state with the type of event that happened.
 * 
 * @param ev The bufferevent structure.
 * @param type The event type.
 * @param ctx The node object.
 */
void event_cb(struct bufferevent* ev, short type, void* ctx)
{
    UNUSED(ev);
    dogecoin_node* node = (dogecoin_node*)ctx;
    node->nodegroup->log_write_cb("Event callback on node %d\n", node->nodeid);

    if (((type & BEV_EVENT_TIMEOUT) != 0) && ((node->state & NODE_CONNECTING) == NODE_CONNECTING)) {
        node->nodegroup->log_write_cb("Timout connecting to node %d.\n", node->nodeid);
        node->state = 0;
        node->state |= NODE_ERRORED;
        node->state |= NODE_TIMEOUT;
        dogecoin_node_connection_state_changed(node);
    } else if (((type & BEV_EVENT_EOF) != 0) ||
               ((type & BEV_EVENT_ERROR) != 0)) {
        node->state = 0;
        node->state |= NODE_ERRORED;
        node->state |= NODE_DISCONNECTED;
        if ((type & BEV_EVENT_EOF) != 0) {
            node->nodegroup->log_write_cb("Disconnected from the remote peer %d.\n", node->nodeid);
            node->state |= NODE_DISCONNECTED_FROM_REMOTE_PEER;
        }
        else {
            node->nodegroup->log_write_cb("Error connecting to node %d.\n", node->nodeid);
        }
        dogecoin_node_connection_state_changed(node);
    } else if (type & BEV_EVENT_CONNECTED) {
        node->nodegroup->log_write_cb("Successful connected to node %d.\n", node->nodeid);
        node->state |= NODE_CONNECTED;
        node->state &= ~NODE_CONNECTING;
        node->state &= ~NODE_ERRORED;
        dogecoin_node_connection_state_changed(node);
    }
    node->nodegroup->log_write_cb("Connected nodes: %d\n", dogecoin_node_group_amount_of_connected_nodes(node->nodegroup, NODE_CONNECTED));
}

/**
 * Initializes a new dogecoin_node
 * 
 * @return A pointer to a new dogecoin_node object.
 */
dogecoin_node* dogecoin_node_new()
{
    dogecoin_node* node;
    node = dogecoin_calloc(1, sizeof(*node));
    node->version_handshake = false;
    node->state = 0;
    node->nonce = 0;
    node->services = 0;
    node->lastping = 0;
    node->time_started_con = 0;
    node->time_last_request = 0;
    dogecoin_hash_clear(node->last_requested_inv);

    node->recvBuffer = cstr_new_sz(DOGECOIN_P2P_MESSAGE_CHUNK_SIZE);
    node->hints = 0;
    return node;
}

/**
 * Given a node and a string, set the node's address to the string
 * 
 * @param node the node structure to be filled
 * @param ipport The IP address and port of the node.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_node_set_ipport(dogecoin_node* node, const char* ipport)
{
    int outlen = (int)sizeof(node->addr);

    return (evutil_parse_sockaddr_port(ipport, &node->addr, &outlen) == 0);
}

/**
 * Release the event buffer and timer event
 * 
 * @param node The node that we're releasing the events for.
 */
void dogecoin_node_release_events(dogecoin_node* node)
{
    if (node->event_bev) {
        bufferevent_free(node->event_bev);
        node->event_bev = NULL;
    }

    if (node->timer_event) {
        event_del(node->timer_event);
        event_free(node->timer_event);
        node->timer_event = NULL;
    }
}

/**
 * Mark the node as misbehaved
 * 
 * @param node The node that is being marked as misbehaved.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_node_misbehave(dogecoin_node* node)
{
    node->nodegroup->log_write_cb("Mark node %d as missbehaved\n", node->nodeid);
    node->state |= NODE_MISSBEHAVED;
    dogecoin_node_connection_state_changed(node);
    return 0;
}

/**
 * If the node is connected, disconnect it
 * 
 * @param node The node that is being disconnected.
 */
void dogecoin_node_disconnect(dogecoin_node* node)
{
    if ((node->state & NODE_CONNECTED) == NODE_CONNECTED || (node->state & NODE_CONNECTING) == NODE_CONNECTING) {
        node->nodegroup->log_write_cb("Disconnect node %d\n", node->nodeid);
    }
    dogecoin_node_release_events(node);

    node->state &= ~NODE_CONNECTING;
    node->state &= ~NODE_CONNECTED;
    node->state |= NODE_DISCONNECTED;

    node->time_started_con = 0;
}

/**
 * It frees the memory allocated to the node
 * 
 * @param node The node to disconnect from.
 */
void dogecoin_node_free(dogecoin_node* node)
{
    dogecoin_node_disconnect(node);
    cstr_free(node->recvBuffer, true);
    dogecoin_free(node);
}

/**
 * It takes a pointer to a dogecoin_node object and frees it
 * 
 * @param obj The object to be freed.
 */
void dogecoin_node_free_cb(void* obj)
{
    dogecoin_node* node = (dogecoin_node*)obj;
    dogecoin_node_free(node);
}

/**
 * Creates a new dogecoin_node_group object
 * 
 * @param chainparams The chainparams to use. If NULL, use the mainnet.
 * 
 * @return A dogecoin_node_group object.
 */
dogecoin_node_group* dogecoin_node_group_new(const dogecoin_chainparams* chainparams)
{
    dogecoin_node_group* node_group;
    node_group = dogecoin_calloc(1, sizeof(*node_group));
#ifdef _WIN32
    WSADATA wsaData;
    WORD wVersionRequested;
    int err;
    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
	printf("\ninitializing winsock...");
	if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed. error code : %d", err);
	}
	
	printf("initialized.\n");
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
    }
    else
        printf("winsock 2.2 dll was found okay\n");
#endif
    struct event_base *base = event_base_new();
    node_group->event_base = base;
    if (!base) {
        dogecoin_free(node_group);
        printf("node_group->event_base not created\n");
        return NULL;
    };

    node_group->nodes = vector_new(1, dogecoin_node_free_cb);
    node_group->chainparams = (chainparams ? chainparams : &dogecoin_chainparams_main);
    node_group->parse_cmd_cb = NULL;
    strcpy(node_group->clientstr, "libdogecoin 0.1");

    /* nullify callbacks */
    node_group->postcmd_cb = NULL;
    node_group->node_connection_state_changed_cb = NULL;
    node_group->should_connect_to_more_nodes_cb = NULL;
    node_group->handshake_done_cb = NULL;
    node_group->log_write_cb = net_write_log_null;
    node_group->desired_amount_connected_nodes = 25;

    return node_group;
}

/**
 * Shut down all the nodes in the group.
 * 
 * @param group The group to shutdown.
 */
void dogecoin_node_group_shutdown(dogecoin_node_group *group) {
    for (size_t i = 0; i < group->nodes->len; i++) {
        dogecoin_node* node = vector_idx(group->nodes, i);
        dogecoin_node_disconnect(node);
    }
}

/**
 * It takes a pointer to a dogecoin_node_group, and frees the memory allocated to it
 * 
 * @param group The group to free.
 */
void dogecoin_node_group_free(dogecoin_node_group* group)
{
    if (!group)
        return;

    if (group->event_base) {
        event_base_free(group->event_base);
    }

    if (group->nodes) {
        vector_free(group->nodes, true);
    }
    dogecoin_free(group);
}

/**
 * The event loop is the core of the event-driven networking library
 * 
 * @param group The dogecoin_node_group object.
 */
void dogecoin_node_group_event_loop(dogecoin_node_group* group)
{
    event_base_dispatch(group->event_base);
}

/**
 * Adds a node to a node group
 * 
 * @param group The node group to add the node to.
 * @param node The node to add to the group.
 */
void dogecoin_node_group_add_node(dogecoin_node_group* group, dogecoin_node* node)
{
    vector_add(group->nodes, node);
    node->nodegroup = group;
    node->nodeid = group->nodes->len;
}

/**
 * The function is used to count the number of nodes in the group that are in the given state
 * 
 * @param group the group to check
 * @param state The state of the node.
 * 
 * @return The number of nodes in the group that are in the given state.
 */
int dogecoin_node_group_amount_of_connected_nodes(dogecoin_node_group* group, enum NODE_STATE state)
{
    int count = 0;
    for (size_t i = 0; i < group->nodes->len; i++) {
        dogecoin_node* node = vector_idx(group->nodes, i);
        if ((node->state & state) == state)
            count++;
    }
    return count;
}

/**
 * Try to connect to a node that is not connected, not in connecting state, not errored, and has not
 * been connected for more than DOGECOIN_PERIODICAL_NODE_TIMER_S seconds.
 * 
 * @param group the node group we're connecting to
 * 
 * @return A boolean value.
 */
dogecoin_bool dogecoin_node_group_connect_next_nodes(dogecoin_node_group* group)
{
    dogecoin_bool connected_at_least_to_one_node = false;
    int connect_amount = group->desired_amount_connected_nodes - dogecoin_node_group_amount_of_connected_nodes(group, NODE_CONNECTED);
    if (connect_amount <= 0)
        return true;

    connect_amount = connect_amount*3;
    for (size_t i = 0; i < group->nodes->len; i++) {
        dogecoin_node* node = vector_idx(group->nodes, i);
        if (
            !((node->state & NODE_CONNECTED) == NODE_CONNECTED) &&
            !((node->state & NODE_CONNECTING) == NODE_CONNECTING) &&
            !((node->state & NODE_DISCONNECTED) == NODE_DISCONNECTED) &&
            !((node->state & NODE_ERRORED) == NODE_ERRORED)) {
            /* setup buffer event */
            node->event_bev = bufferevent_socket_new(group->event_base, -1, BEV_OPT_CLOSE_ON_FREE);
            bufferevent_setcb(node->event_bev, read_cb, write_cb, event_cb, node);
            bufferevent_enable(node->event_bev, EV_READ | EV_WRITE);
            if (bufferevent_socket_connect(node->event_bev, (struct sockaddr*)&node->addr, sizeof(node->addr)) < 0) {
                if (node->event_bev) {
                    bufferevent_free(node->event_bev);
                    node->event_bev = NULL;
                }
                return false;
            }

            /* setup periodic timer */
            node->time_started_con = time(NULL);
            struct timeval tv;
            tv.tv_sec = DOGECOIN_PERIODICAL_NODE_TIMER_S;
            tv.tv_usec = 0;
            node->timer_event = event_new(group->event_base, 0, EV_TIMEOUT | EV_PERSIST, node_periodical_timer, node);
            event_add(node->timer_event, &tv);
            node->state |= NODE_CONNECTING;
            connected_at_least_to_one_node = true;
            node->nodegroup->log_write_cb("Trying to connect to %d...\n", node->nodeid);
            connect_amount--;
            if (connect_amount <= 0)
                return true;
        }
    }
    return connected_at_least_to_one_node;
}

/**
 * If the node is in an error state or misbehaving state we disconnect it. If the 
 * node is in a connecting state we do nothing. Otherwise we send a version message to it. 
 * 
 * @param node The node that has changed state.
 */
void dogecoin_node_connection_state_changed(dogecoin_node* node)
{
    if (node->nodegroup->node_connection_state_changed_cb)
        node->nodegroup->node_connection_state_changed_cb(node);

    if ((node->state & NODE_ERRORED) == NODE_ERRORED) {
        dogecoin_node_release_events(node);

        /* connect to more nodes are required */
        dogecoin_bool should_connect_to_more_nodes = true;
        if (node->nodegroup->should_connect_to_more_nodes_cb)
            should_connect_to_more_nodes = node->nodegroup->should_connect_to_more_nodes_cb(node);

        if (should_connect_to_more_nodes && (dogecoin_node_group_amount_of_connected_nodes(node->nodegroup, NODE_CONNECTED) + dogecoin_node_group_amount_of_connected_nodes(node->nodegroup, NODE_CONNECTING) < node->nodegroup->desired_amount_connected_nodes))
            dogecoin_node_group_connect_next_nodes(node->nodegroup);
    }
    if ((node->state & NODE_MISSBEHAVED) == NODE_MISSBEHAVED) {
        if ((node->state & NODE_CONNECTED) == NODE_CONNECTED || (node->state & NODE_CONNECTING) == NODE_CONNECTING) {
            dogecoin_node_disconnect(node);
        }
    } else
        dogecoin_node_send_version(node);
}

/**
 * Send a message to a node
 * 
 * @param node the node that is sending the message
 * @param data The data to send.
 * 
 * @return dogecoin_bool (uint8_t)
 */
void dogecoin_node_send(dogecoin_node* node, cstring* data)
{
    if ((node->state & NODE_CONNECTED) != NODE_CONNECTED)
        return;

    bufferevent_write(node->event_bev, data->str, data->len);
    char* dummy = data->str + 4;
    node->nodegroup->log_write_cb("sending message to node %d: %s\n", node->nodeid, dummy);
}

/**
 * Send a version message to the remote node
 * 
 * @param node The node that is sending the message.
 */
void dogecoin_node_send_version(dogecoin_node* node)
{
    if (!node)
        return;

    cstring* version_msg_cstr = cstr_new_sz(256);

    /* copy socket_addr to p2p addr */
    dogecoin_p2p_address fromAddr;
    dogecoin_p2p_address_init(&fromAddr);
    dogecoin_p2p_address toAddr;
    dogecoin_p2p_address_init(&toAddr);
    dogecoin_addr_to_p2paddr(&node->addr, &toAddr);

    /* create a version message struct */
    dogecoin_p2p_version_msg version_msg;
    dogecoin_mem_zero(&version_msg, sizeof(version_msg));

    /* create a serialized version message */
    dogecoin_p2p_msg_version_init(&version_msg, &fromAddr, &toAddr, node->nodegroup->clientstr, true);
    dogecoin_p2p_msg_version_ser(&version_msg, version_msg_cstr);

    /* create p2p message */
    cstring* p2p_msg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_VERSION, version_msg_cstr->str, version_msg_cstr->len);

    /* send message */
    dogecoin_node_send(node, p2p_msg);

    /* cleanup */
    cstr_free(version_msg_cstr, true);
    cstr_free(p2p_msg, true);
}

/**
 * This function parses a command message received from another node.
 * 
 * @param node The node that received the message.
 * @param hdr The header of the message.
 * @param buf The buffer containing the message.
 * 
 * @return int
 */
int dogecoin_node_parse_message(dogecoin_node* node, dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf)
{
    node->nodegroup->log_write_cb("received command from node %d: %s\n", node->nodeid, hdr->command);
    if (memcmp(hdr->netmagic, node->nodegroup->chainparams->netmagic, sizeof(node->nodegroup->chainparams->netmagic)) != 0) {
        return dogecoin_node_misbehave(node);
    }

    /* send the header and buffer to the possible callback */
    if (!node->nodegroup->parse_cmd_cb || node->nodegroup->parse_cmd_cb(node, hdr, buf)) {
        if (strcmp(hdr->command, DOGECOIN_MSG_VERSION) == 0) {
            dogecoin_p2p_version_msg v_msg_check;
            if (!dogecoin_p2p_msg_version_deser(&v_msg_check, buf)) {
                return dogecoin_node_misbehave(node);
            }
            if ((v_msg_check.services & DOGECOIN_NODE_NETWORK) != DOGECOIN_NODE_NETWORK) {
                dogecoin_node_disconnect(node);
            }
            node->bestknownheight = v_msg_check.start_height;
            node->nodegroup->log_write_cb("Connected to node %d: %s (%d)\n", node->nodeid, v_msg_check.useragent, v_msg_check.start_height);
            /* confirm version via verack */
            cstring* verack = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_VERACK, NULL, 0);
            dogecoin_node_send(node, verack);
            cstr_free(verack, true);
        } else if (strcmp(hdr->command, DOGECOIN_MSG_VERACK) == 0) {
            /* complete handshake if verack has been received */
            node->version_handshake = true;
            if (node->nodegroup->handshake_done_cb)
                node->nodegroup->handshake_done_cb(node);
        } else if (strcmp(hdr->command, DOGECOIN_MSG_PING) == 0) {
            uint64_t nonce = 0;
            if (!deser_u64(&nonce, buf)) {
                return dogecoin_node_misbehave(node);
            }
            cstring* pongmsg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_PONG, &nonce, 8);
            dogecoin_node_send(node, pongmsg);
            cstr_free(pongmsg, true);
        }
    }

    if (node->nodegroup->postcmd_cb)
        node->nodegroup->postcmd_cb(node, hdr, buf);

    return true;
}

/**
 * Given a seed DNS name, return a vector of IP addresses and ports. 
 * (utility function to get peers (ips/port as char*) from a seed)
 * 
 * @param seed The seed DNS name.
 * @param ips_out A vector of strings.
 * @param port The port to connect to.
 * @param family The address family of the socket. AF_INET or AF_INET6.
 * 
 * @return size_t
 */
size_t dogecoin_get_peers_from_dns(const char* seed, vector* ips_out, int port, int family)
{
    if (!seed || !ips_out || (family != AF_INET && family != AF_INET6) || port > 99999) {
        return 0;
    }
    char def_port[12] = {0};
    sprintf(def_port, "%d", port);
    struct evutil_addrinfo hints, *aiTrav = NULL, *aiRes = NULL;
    dogecoin_mem_zero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int err = evutil_getaddrinfo(seed, NULL, &hints, &aiRes);
    if (err)
        return 0;

    aiTrav = aiRes;
    while (aiTrav != NULL) {
        int maxlen = 256;
        char* ipaddr = dogecoin_calloc(1, maxlen);
        if (aiTrav->ai_family == AF_INET) {
            assert(aiTrav->ai_addrlen >= sizeof(struct sockaddr_in));
            evutil_inet_ntop(aiTrav->ai_family, &((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr, ipaddr, maxlen);
        }

        if (aiTrav->ai_family == AF_INET6) {
            assert(aiTrav->ai_addrlen >= sizeof(struct sockaddr_in6));
            evutil_inet_ntop(aiTrav->ai_family, &((struct sockaddr_in6*)(aiTrav->ai_addr))->sin6_addr, ipaddr, maxlen);
        }

        memcpy_safe(ipaddr + strlen(ipaddr), ":", 1);
        memcpy_safe(ipaddr + strlen(ipaddr), def_port, strlen(def_port));

        vector_add(ips_out, ipaddr);

        aiTrav = aiTrav->ai_next;
    }
    evutil_freeaddrinfo(aiRes);
    return ips_out->len;
}

/**
 * It takes a comma seperated list of IPs and adds them to the group
 * 
 * @param group the node group to add the nodes to
 * @param ips comma seperated list of ip addresses
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_node_group_add_peers_by_ip_or_seed(dogecoin_node_group *group, const char *ips) {
    if (ips == NULL) {
        /* === DNS QUERY === */
        vector* ips_dns = vector_new(10, free);
        const dogecoin_dns_seed seed = group->chainparams->dnsseeds[0];
        if (strlen(seed.domain) == 0) {
            return false;
        }
        /* todo: make sure we have enough peers, eventually */
        dogecoin_get_peers_from_dns(seed.domain, ips_dns, group->chainparams->default_port, AF_INET);
        unsigned int i;
        for (i = 0; i < ips_dns->len; i++) {
            char* ip = (char*)vector_idx(ips_dns, i);

            /* create a node */
            dogecoin_node* node = dogecoin_node_new();
            if (dogecoin_node_set_ipport(node, ip) > 0) {
                /* add the node to the group */
                dogecoin_node_group_add_node(group, node);
            }
        }
        vector_free(ips_dns, true);
    } else {
        // add comma seperated ips (nodes)
        char working_str[64];
        dogecoin_mem_zero(working_str, sizeof(working_str));
        size_t offset = 0;
        unsigned int i;
        for (i = 0; i <= strlen(ips); i++) {
            if (i == strlen(ips) || ips[i] == ',') {
                dogecoin_node* node = dogecoin_node_new();
                if (dogecoin_node_set_ipport(node, working_str) > 0) {
                    dogecoin_node_group_add_node(group, node);
                }
                offset = 0;
                dogecoin_mem_zero(working_str, sizeof(working_str));
            } else if (ips[i] != ' ' && offset < sizeof(working_str)) {
                working_str[offset] = ips[i];
                offset++;
            }
        }
    }
    return true;
}

/**
 * @brief Disconnect node if it has been connected for more than the timeout.
 *
 * @param node the node that is being checked
 * @param now the current time in seconds since the epoch
 *
 * @return static dogecoin_bool (uint8_t)
 */
static dogecoin_bool broadcast_timer_cb(dogecoin_node* node, uint64_t* now) {
    struct broadcast_ctx* ctx = (struct broadcast_ctx*)node->nodegroup->ctx;

    if (node->time_started_con > 0) {
        node->nodegroup->log_write_cb("timer node %d, delta: %" PRIu64 " secs (timeout is: %d)\n", node->nodeid, (*now - ctx->start_time), ctx->timeout);
        }
    if ((*now - ctx->start_time) > ctx->timeout)
        dogecoin_node_disconnect(node);

    if ((node->hints & (1 << 1)) == (1 << 1)) {
        dogecoin_node_disconnect(node);
        }

    if ((node->hints & (1 << 2)) == (1 << 2)) {
        dogecoin_node_disconnect(node);
        }

    /* return true = run internal timer logic (ping, disconnect-timeout, etc.) */
    return true;
    }

/**
 * @brief We send an INV message to the peer and set hints flag to 0 to indicate that we've sent an INV
 * message
 *
 * @param node The node that is broadcasting the handshake.
 */
void broadcast_handshake_done(struct dogecoin_node_* node) {
    char ipaddr[256];
    struct sockaddr_in* ad = (struct sockaddr_in*)&node->addr;
    evutil_inet_ntop(node->addr.sa_family, &ad->sin_addr, ipaddr, sizeof(ipaddr));

    printf("Successfully connected to peer %d (%s)\n", node->nodeid, ipaddr);
    struct broadcast_ctx* ctx = (struct broadcast_ctx*)node->nodegroup->ctx;
    ctx->connected_to_peers++;

    if (ctx->inved_to_peers >= ctx->max_peers_to_inv) {
        return;
        }

    /* create a INV */
    cstring* inv_msg_cstr = cstr_new_sz(256);
    dogecoin_p2p_inv_msg inv_msg;
    dogecoin_mem_zero(&inv_msg, sizeof(inv_msg));

    uint256 hash;
    dogecoin_tx_hash(ctx->tx, hash);
    dogecoin_p2p_msg_inv_init(&inv_msg, DOGECOIN_INV_TYPE_TX, hash);

    /* serialize the inv count (1) */
    ser_varlen(inv_msg_cstr, 1);
    dogecoin_p2p_msg_inv_ser(&inv_msg, inv_msg_cstr);

    cstring* p2p_msg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_INV, inv_msg_cstr->str, inv_msg_cstr->len);
    cstr_free(inv_msg_cstr, true);
    dogecoin_node_send(node, p2p_msg);
    cstr_free(p2p_msg, true);

    /* INV sent */
    node->hints |= (1 << 0);
    ctx->inved_to_peers++;
    }

/**
 * @brief This function deterimines if we have connected to the maximum number of peers
 * (true) or not (false).
 *
 * @param node the node that we are broadcasting to.
 *
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool broadcast_should_connect_more(dogecoin_node* node) {
    struct broadcast_ctx* ctx = (struct broadcast_ctx*)node->nodegroup->ctx;
    node->nodegroup->log_write_cb("check if more nodes are required (connected to already: %d)\n", ctx->connected_to_peers);
    if (ctx->connected_to_peers >= ctx->max_peers_to_connect) {
        return false;
        }
    return true;
    }

/**
 * @brief If we receive an INV message we check if the transaction is in the message.
 * If true we indicate that we have the transaction. If we receive a GETDATA message
 * we check if the transaction is in the message. If true we broadcast the transaction.
 *
 * @param node the node that received the message
 * @param hdr The header of the message.
 * @param buf the buffer containing the message
 */
void broadcast_post_cmd(struct dogecoin_node_* node, dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf) {
    struct broadcast_ctx* ctx = (struct broadcast_ctx*)node->nodegroup->ctx;
    if (strcmp(hdr->command, DOGECOIN_MSG_INV) == 0) {
        /* hash the tx */
        /* TODO: cache the hash */
        uint256 hash;
        dogecoin_tx_hash(ctx->tx, hash);

        uint32_t vsize;
        if (!deser_varlen(&vsize, buf)) {
            dogecoin_node_misbehave(node);
            return;
            };
        for (unsigned int i = 0; i < vsize; i++) {
            dogecoin_p2p_inv_msg inv_msg;
            if (!dogecoin_p2p_msg_inv_deser(&inv_msg, buf)) {
                dogecoin_node_misbehave(node);
                return;
                }
            if (memcmp(hash, inv_msg.hash, sizeof(hash)) == 0) {
                /* tx found on peer */
                node->hints |= (1 << 2);
                printf("node %d has the tx\n", node->nodeid);
                ctx->found_on_non_inved_peers++;
                printf("tx successfully seen on node %d\n", node->nodeid);
                }
            }
        }
    else if (strcmp(hdr->command, DOGECOIN_MSG_GETDATA) == 0 && ((node->hints & (1 << 1)) != (1 << 1))) {
        ctx->getdata_from_peers++;
        // only allow a single object in GETDATA for the broadcaster
        uint32_t vsize;
        if (!deser_varlen(&vsize, buf) || vsize != 1) {
            dogecoin_node_misbehave(node);
            return;
            }

        dogecoin_p2p_inv_msg inv_msg;
        dogecoin_mem_zero(&inv_msg, sizeof(inv_msg));
        if (!dogecoin_p2p_msg_inv_deser(&inv_msg, buf) || inv_msg.type != DOGECOIN_INV_TYPE_TX) {
            dogecoin_node_misbehave(node);
            return;
            };

        /* send the tx */
        cstring* tx_ser = cstr_new_sz(1024);
        dogecoin_tx_serialize(tx_ser, ctx->tx);
        cstring* p2p_msg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_TX, tx_ser->str, tx_ser->len);
        cstr_free(tx_ser, true);
        dogecoin_node_send(node, p2p_msg);
        cstr_free(p2p_msg, true);

        /* tx sent */
        node->hints |= (1 << 1);

        printf("tx successfully sent to node %d\n", node->nodeid);
        }
    }

/**
 * Connect to a set of nodes and then wait for the transaction to be broadcasted.
 *
 * @param chain the chain parameters
 * @param tx The transaction to broadcast
 * @param ips a comma separated list of IPs or DNS seeds to connect to.
 * @param maxpeers The maximum number of peers to connect to.
 * @param timeout How long to wait for the transaction to be broadcasted.
 * @param debug if set to 1, will print out debug messages
 *
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool broadcast_tx(const dogecoin_chainparams* chain, const dogecoin_tx* tx, const char* ips, int maxpeers, int timeout, dogecoin_bool debug) {
    struct broadcast_ctx ctx;
    ctx.tx = tx;
    ctx.debuglevel = debug;
    ctx.timeout = timeout;
    ctx.max_peers_to_inv = 2;
    ctx.found_on_non_inved_peers = 0;
    ctx.getdata_from_peers = 0;
    ctx.inved_to_peers = 0;
    ctx.connected_to_peers = 0;
    ctx.max_peers_to_connect = maxpeers;
    /* create a node group */
    dogecoin_node_group* group = dogecoin_node_group_new(chain);
    group->desired_amount_connected_nodes = ctx.max_peers_to_connect;
    group->ctx = &ctx;

    /* set the timeout callback */
    group->periodic_timer_cb = broadcast_timer_cb;

    /* set a individual log print function */
    if (debug) {
        group->log_write_cb = net_write_log_printf;
        }
    group->postcmd_cb = broadcast_post_cmd;
    group->handshake_done_cb = broadcast_handshake_done;
    group->should_connect_to_more_nodes_cb = broadcast_should_connect_more;

    dogecoin_node_group_add_peers_by_ip_or_seed(group, ips);

    uint256 txhash;
    dogecoin_tx_hash(tx, txhash);
    char hexout[sizeof(txhash) * 2 + 1];
    utils_bin_to_hex(txhash, sizeof(txhash), hexout);
    hexout[sizeof(txhash) * 2] = 0;
    utils_reverse_hex(hexout, strlen(hexout));
    printf("Start broadcasting transaction: %s with timeout %d seconds\n", hexout, timeout);
    /* connect to the next node */
    ctx.start_time = time(NULL);
    printf("Trying to connect to nodes...\n");
    dogecoin_node_group_connect_next_nodes(group);

    /* start the event loop */
    dogecoin_node_group_event_loop(group);

    /* cleanup (free) nodes structures from the heap */
    dogecoin_node_group_free(group);

    printf("\n\nResult:\n=============\n");
    printf("Max nodes to connect to: %d\n", ctx.max_peers_to_connect);
    printf("Successfully connected to nodes: %d\n", ctx.connected_to_peers);
    printf("Informed nodes: %d\n", ctx.inved_to_peers);
    printf("Requested from nodes: %d\n", ctx.getdata_from_peers);
    printf("Seen on other nodes: %d\n", ctx.found_on_non_inved_peers);

    if (ctx.getdata_from_peers == 0)
        {
        printf("\nError: The transaction was not requested by the informed nodes. This usually happens when the transaction has already been broadcasted\n");
        }
    else if (ctx.found_on_non_inved_peers == 0)
        {
        printf("\nError: The transaction was not relayed back. Your transaction is very likely invalid (or was already broadcasted and picked up by an invalid node)\n");
        }
    return 1;
    }
