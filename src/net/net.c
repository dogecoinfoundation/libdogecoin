#include <dogecoin/net/net.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <dogecoin/buffer.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/cstr.h>
#include <dogecoin/crypto/hash.h>
#include <dogecoin/net/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>

#ifdef _WIN32
#include <getopt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

/* The code below is a macro that is used to suppress compiler warnings. */
#define UNUSED(x) (void)(x)

/* The code below is defining a constant called DOGECOIN_PERIODICAL_NODE_TIMER_S.
        This constant is used to define the number of seconds between each node timer.
        The node timer is used to check if a node is still online.
        If a node is not online, it will be removed from the list of nodes. */
static const int DOGECOIN_PERIODICAL_NODE_TIMER_S = 3;
/* The code below is a function that returns a constant value. */
static const int DOGECOIN_PING_INTERVAL_S = 120;
/* The code below is a function definition. It defines a function called DOGECOIN_CONNECT_TIMEOUT_S.
The function returns the value 10.
*/
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
    /* Getting the input buffer of the bufferevent. */
    struct evbuffer* input = bufferevent_get_input(bev);
    if (!input)
        return;

    /* Getting the length of the input buffer. */
    size_t length = evbuffer_get_length(input);

    dogecoin_node* node = (dogecoin_node*)ctx;

    /* Checking if the node is connected. */
    if ((node->state & NODE_CONNECTED) != NODE_CONNECTED) {
        // ignore messages from disconnected peers
        return;
    }
    // expand the cstring buffer if required
    /* Allocating a new buffer with a size that is at least as large as the current buffer. */
    cstr_alloc_minsize(node->recvBuffer, node->recvBuffer->len + length);

    // copy direct to cstring, avoid another heap buffer
    /* Copying the data from the input buffer to the node's receive buffer. */
    evbuffer_copyout(input, node->recvBuffer->str + node->recvBuffer->len, length);
    node->recvBuffer->len += length;

    // drain the event buffer
    /* Draining the input buffer of the data that was read. */
    evbuffer_drain(input, length);

    struct const_buffer buf = {node->recvBuffer->str, node->recvBuffer->len};
    dogecoin_p2p_msg_hdr hdr;
    char* read_upto = NULL;

    /* The code below is parsing the message received from the dogecoin node. */
    do {
        //check if message is complete
        /* This is checking if the buffer is less than the size of the header. */
        if (buf.len < DOGECOIN_P2P_HDRSZ) {
            break;
        }

        /* Deserializing the message header. */
        dogecoin_p2p_deser_msghdr(&hdr, &buf);
        /* This is checking if the data length is greater than the maximum allowed size. */
        if (hdr.data_len > DOGECOIN_MAX_P2P_MSG_SIZE) {
            // check for invalid message lengths
            dogecoin_node_misbehave(node);
            return;
        }
        /* Checking if the buffer is smaller than the data length. */
        if (buf.len < hdr.data_len) {
            //if we haven't read the whole message, continue and wait for the next chunk
            break;
        }
        /* Checking if the buffer has enough data to read the data length specified in the header. */
        if (buf.len >= hdr.data_len) {
            //at least one message is complete

            struct const_buffer cmd_data_buf = {buf.p, buf.len};
            /* Checking if the node is connected. */
            if ((node->state & NODE_CONNECTED) != NODE_CONNECTED) {
                // ignore messages from disconnected peers
                return;
            }
            /* The code below is parsing the message received from the dogecoin node. */
            dogecoin_node_parse_message(node, &hdr, &cmd_data_buf);

            //skip the size of the whole message
            /* The code below is reading the data from the buffer and advancing the buffer pointer. */
            buf.p = (const unsigned char*)buf.p + hdr.data_len;
            /* Checking to see if the buffer has enough data to read the header. If it does it reads the header. */
            buf.len -= hdr.data_len;

            read_upto = (void*)buf.p;
        }
        if (buf.len == 0) {
            // if we have "consumed" the whole buffer
            node->recvBuffer->len = 0;
            break;
        }
    } while (1);

    if (read_upto != NULL && node->recvBuffer->len != 0 && read_upto != (node->recvBuffer->str + node->recvBuffer->len)) {
        /* Checking if the end of the buffer is at the end of the string. */
        char* end = node->recvBuffer->str + node->recvBuffer->len;
        /* Checking the number of bytes available in the chunk. */
        size_t available_chunk_data = end - read_upto;
        // partial message
        cstring* tmp = cstr_new_buf(read_upto, available_chunk_data);
        /* Freeing the memory allocated to the node->recvBuffer. */
        cstr_free(node->recvBuffer, true);
        /* Allocating memory for the receive buffer. */
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
void node_periodical_timer(int fd, short event, void* ctx)
{
    UNUSED(fd);
    UNUSED(event);
    dogecoin_node* node = (dogecoin_node*)ctx;
    uint64_t now = time(NULL);

    if (node->nodegroup->periodic_timer_cb)
        if (!node->nodegroup->periodic_timer_cb(node, &now))
            return;

    /* Checking if the time that the node has been trying to connect to the network for is greater than
    the maximum time allowed for a node to connect to the network. */
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
        // time for a ping
        uint64_t nonce;
        /* This code is generating a random nonce. */
        dogecoin_cheap_random_bytes((uint8_t*)&nonce, sizeof(nonce));
        /* Creating a new message object and initializing it with the network magic, message type, and
        nonce. */
        cstring* pingmsg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_PING, &nonce, sizeof(nonce));
        /* Sending a ping message to the node. */
        dogecoin_node_send(node, pingmsg);
        /* The code below is freeing the memory allocated to pingmsg. */
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
    /* The code below is a callback function that writes event to log and is called when a node receives an event. */
    node->nodegroup->log_write_cb("Event callback on node %d\n", node->nodeid);

    /* Checking if the event is a timeout event and if the node is in the connecting state. */
    if (((type & BEV_EVENT_TIMEOUT) != 0) && ((node->state & NODE_CONNECTING) == NODE_CONNECTING)) {
        node->nodegroup->log_write_cb("Timout connecting to node %d.\n", node->nodeid);
        node->state = 0;
        node->state |= NODE_ERRORED;
        node->state |= NODE_TIMEOUT;
        dogecoin_node_connection_state_changed(node);
    /* Checking to see if the event is an EOF or an error. */
    } else if (((type & BEV_EVENT_EOF) != 0) ||
               ((type & BEV_EVENT_ERROR) != 0)) {
        node->state = 0;
        node->state |= NODE_ERRORED;
        node->state |= NODE_DISCONNECTED;
        /* Checking to see if the event is an EOF event. */
        if ((type & BEV_EVENT_EOF) != 0) {
            node->nodegroup->log_write_cb("Disconnected from the remote peer %d.\n", node->nodeid);
            node->state |= NODE_DISCONNECTED_FROM_REMOTE_PEER;
        }
        else {
            node->nodegroup->log_write_cb("Error connecting to node %d.\n", node->nodeid);
        }
        dogecoin_node_connection_state_changed(node);
    /* This code is checking to see if the event is a connection event and if so will update the connection state accordingly */
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

    //return true in case of success (0 == no error)
    /* Parsing the IP address and port number from the string. */
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

    /* Checking if the node has a timer event. If it does, it will
            remove the timer event from the node. */
    if (node->timer_event) {
        /* Deleting the timer event from the event loop. */
        event_del(node->timer_event);
        /* Freeing the timer event. */
        event_free(node->timer_event);
        node->timer_event = NULL;
    }
}

/**
 * Mark the node as missbehaved
 * 
 * @param node The node that is being marked as missbehaved.
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
    /* Checking if the node is connected or connecting and writes message to the log callback function. */
    if ((node->state & NODE_CONNECTED) == NODE_CONNECTED || (node->state & NODE_CONNECTING) == NODE_CONNECTING) {
        node->nodegroup->log_write_cb("Disconnect node %d\n", node->nodeid);
    }
    /* release buffer and timer event */
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
    /* Creating a new dogecoin_node object and assigning it to the variable node. */
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
    node_group->event_base = event_base_new();
    if (!node_group->event_base) {
        dogecoin_free(node_group);
        return NULL;
    };

    /* Creating a new vector of nodes. */
    node_group->nodes = vector_new(1, dogecoin_node_free_cb);
    /* Setting the chainparams to either the main dogecoin_chainparams_main or the testnet
    dogecoin_chainparams_test. */
    node_group->chainparams = (chainparams ? chainparams : &dogecoin_chainparams_main);
    node_group->parse_cmd_cb = NULL;
    strcpy(node_group->clientstr, "libdogecoin 0.1");

    /* nullify callbacks */
    node_group->postcmd_cb = NULL;
    node_group->node_connection_state_changed_cb = NULL;
    node_group->should_connect_to_more_nodes_cb = NULL;
    node_group->handshake_done_cb = NULL;
    node_group->log_write_cb = net_write_log_null;
    node_group->desired_amount_connected_nodes = 3;

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
    /* Get the kernel event notification mechanism used by Libevent. */
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
    /* Adding a node to the group. */
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
    /* Iterating through the nodes in the group and checking if the node is a leaf. */
    for (size_t i = 0; i < group->nodes->len; i++) {
        /* Creating a dogecoin_node object and adding it to the vector of nodes. */
        dogecoin_node* node = vector_idx(group->nodes, i);
        /* Checking if the node is in the state we want. */
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
    /* The code below is checking if the amount of connected nodes is less than the desired amount of
    connected nodes. */
    int connect_amount = group->desired_amount_connected_nodes - dogecoin_node_group_amount_of_connected_nodes(group, NODE_CONNECTED);
    /* Checking if the connect_amount is less than or equal to 0. */
    if (connect_amount <= 0)
        return true;

    connect_amount = connect_amount*3;
    /* search for a potential node that has not errored and is not connected or in connecting state */
    for (size_t i = 0; i < group->nodes->len; i++) {
        dogecoin_node* node = vector_idx(group->nodes, i);
        /* Checking if the node is connected, connecting, disconnected or errored. */
        if (
            !((node->state & NODE_CONNECTED) == NODE_CONNECTED) &&
            !((node->state & NODE_CONNECTING) == NODE_CONNECTING) &&
            !((node->state & NODE_DISCONNECTED) == NODE_DISCONNECTED) &&
            !((node->state & NODE_ERRORED) == NODE_ERRORED)) {
            /* setup buffer event */
            /* It creates a new bufferevent object and stores it in the node->event_bev variable and launch a connect() attempt with a socket-based bufferevent. */
            node->event_bev = bufferevent_socket_new(group->event_base, -1, BEV_OPT_CLOSE_ON_FREE);
            /* Setting the callbacks for the bufferevent. */
            bufferevent_setcb(node->event_bev, read_cb, write_cb, event_cb, node);
            /* Enabling the bufferevent for reading and writing. */
            bufferevent_enable(node->event_bev, EV_READ | EV_WRITE);
            /* Resolve the hostname 'hostname' and connect to it as with
   bufferevent_socket_connect(). */
            if (bufferevent_socket_connect(node->event_bev, (struct sockaddr*)&node->addr, sizeof(node->addr)) < 0) {
                if (node->event_bev) {
                    /* Freeing the bufferevent. */
                    bufferevent_free(node->event_bev);
                    node->event_bev = NULL;
                }
                return false;
            }

            /* setup periodic timer */
            node->time_started_con = time(NULL);
            struct timeval tv;
            /* Setting the timer to run every DOGECOIN_PERIODICAL_NODE_TIMER_S seconds. */
            tv.tv_sec = DOGECOIN_PERIODICAL_NODE_TIMER_S;
            /* Setting the microseconds to 0. */
            tv.tv_usec = 0;
            /* Prepare a new, already-allocated event structure to be added.
            The function event_assign() prepares the event structure ev to be used */
            node->timer_event = event_new(group->event_base, 0, EV_TIMEOUT | EV_PERSIST, node_periodical_timer,
                                          (void*)node);
            /* Adding the timer event to the event queue. */
            event_add(node->timer_event, &tv);
            /* Setting the state of the node to NODE_CONNECTING. */
            node->state |= NODE_CONNECTING;
            connected_at_least_to_one_node = true;

            /* Writing a message to the log file. */
            node->nodegroup->log_write_cb("Trying to connect to %d...\n", node->nodeid);

            connect_amount--;
            /* Checking if the connect_amount is less than or equal to 0. */
            if (connect_amount <= 0)
                return true;
        }
    }
    /* node group misses a node to connect to */
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

    /* get new string buffer */
    cstring* version_msg_cstr = cstr_new_sz(256);

    /* copy socket_addr to p2p addr */
    dogecoin_p2p_address fromAddr;
    dogecoin_p2p_address_init(&fromAddr);
    dogecoin_p2p_address toAddr;
    dogecoin_p2p_address_init(&toAddr);
    dogecoin_addr_to_p2paddr(&node->addr, &toAddr);

    /* create a version message struct */
    dogecoin_p2p_version_msg version_msg;
    memset(&version_msg, 0, sizeof(version_msg));

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
    /* callback can decide to run the internal base message logic */
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

            /* execute callback and inform that the node is ready for custom message logic */
            if (node->nodegroup->handshake_done_cb)
                node->nodegroup->handshake_done_cb(node);
        } else if (strcmp(hdr->command, DOGECOIN_MSG_PING) == 0) {
            /* response pings */
            uint64_t nonce = 0;
            if (!deser_u64(&nonce, buf)) {
                return dogecoin_node_misbehave(node);
            }
            cstring* pongmsg = dogecoin_p2p_message_new(node->nodegroup->chainparams->netmagic, DOGECOIN_MSG_PONG, &nonce, 8);
            dogecoin_node_send(node, pongmsg);
            cstr_free(pongmsg, true);
        }
    }

    /* pass data to the "post command" callback */
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
 * @return int
 */
int dogecoin_get_peers_from_dns(const char* seed, vector* ips_out, int port, int family)
{
    if (!seed || !ips_out || (family != AF_INET && family != AF_INET6) || port > 99999) {
        return 0;
    }
    char def_port[12] = {0};
    sprintf(def_port, "%d", port);
    struct evutil_addrinfo hints, *aiTrav = NULL, *aiRes = NULL;
    memset(&hints, 0, sizeof(hints));
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

        memcpy(ipaddr + strlen(ipaddr), ":", 1);
        memcpy(ipaddr + strlen(ipaddr), def_port, strlen(def_port));

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
        /* get a couple of peers from a seed */
        vector* ips_dns = vector_new(10, free);
        const dogecoin_dns_seed seed = group->chainparams->dnsseeds[0];
        if (strlen(seed.domain) == 0) {
            return false;
        }
        /* todo: make sure we have enought peers, eventually */
        /* call another seeder */
        dogecoin_get_peers_from_dns(seed.domain, ips_dns, group->chainparams->default_port, AF_INET);
        for (unsigned int i = 0; i < ips_dns->len; i++) {
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
        memset(working_str, 0, sizeof(working_str));
        size_t offset = 0;
        for (unsigned int i = 0; i <= strlen(ips); i++) {
            if (i == strlen(ips) || ips[i] == ',') {
                dogecoin_node* node = dogecoin_node_new();
                if (dogecoin_node_set_ipport(node, working_str) > 0) {
                    dogecoin_node_group_add_node(group, node);
                }
                offset = 0;
                memset(working_str, 0, sizeof(working_str));
            } else if (ips[i] != ' ' && offset < sizeof(working_str)) {
                working_str[offset] = ips[i];
                offset++;
            }
        }
    }
    return true;
}
