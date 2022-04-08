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

#ifndef __LIBDOGECOIN_NET_PROTOCOL_H__
#define __LIBDOGECOIN_NET_PROTOCOL_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/buffer.h>
#include <dogecoin/cstr.h>
#include <dogecoin/vector.h>

LIBDOGECOIN_BEGIN_DECL

#ifdef _WIN32
#  ifndef _MSC_VER
#    include <getopt.h>
#  endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

static const unsigned int DOGECOIN_MAX_P2P_MSG_SIZE = 0x003D0900; // 4 * 1000 * 1000

static const unsigned int DOGECOIN_P2P_HDRSZ = 24; //(4 + 12 + 4 + 4)  magic, command, length, checksum

DISABLE_WARNING_PUSH
DISABLE_WARNING(-Wunused-variable)
static uint256 NULLHASH = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

enum service_bits {
    // Nothing
    DOGECOIN_NODE_NONE = 0,
    // NODE_NETWORK means that the node is capable of serving the block chain. It is currently
    // set by all Bitcoin Core nodes, and is unset by SPV clients or other peers that just want
    // network services but don't provide them.
    DOGECOIN_NODE_NETWORK = (1 << 0),
    // NODE_GETUTXO means the node is capable of responding to the getutxo protocol request.
    // Bitcoin Core does not support this but a patch set called Bitcoin XT does.
    // See BIP 64 for details on how this is implemented.
    DOGECOIN_NODE_GETUTXO = (1 << 1),
    // NODE_BLOOM means the node is capable and willing to handle bloom-filtered connections.
    // Bitcoin Core nodes used to support this by default, without advertising this bit,
    // but no longer do as of protocol version 70011 (= NO_BLOOM_VERSION)
    DOGECOIN_NODE_BLOOM = (1 << 2),
    // NODE_WITNESS indicates that a node can be asked for blocks and transactions including
    // witness data.
    DOGECOIN_NODE_WITNESS = (1 << 3),
    // NODE_XTHIN means the node supports Xtreme Thinblocks
    // If this is turned off then the node will not service nor make xthin requests
    DOGECOIN_NODE_XTHIN = (1 << 4),
    DOGECOIN_NODE_COMPACT_FILTERS = (1 << 6),
};

static const char* DOGECOIN_MSG_VERSION = "version";
static const char* DOGECOIN_MSG_VERACK = "verack";
static const char* DOGECOIN_MSG_PING = "ping";
static const char* DOGECOIN_MSG_PONG = "pong";
static const char* DOGECOIN_MSG_GETDATA = "getdata";
static const char* DOGECOIN_MSG_GETHEADERS = "getheaders";
static const char* DOGECOIN_MSG_HEADERS = "headers";
static const char* DOGECOIN_MSG_GETBLOCKS = "getblocks";
static const char* DOGECOIN_MSG_BLOCK = "block";
static const char* DOGECOIN_MSG_INV = "inv";
static const char* DOGECOIN_MSG_TX = "tx";
DISABLE_WARNING_POP

enum DOGECOIN_INV_TYPE {
    DOGECOIN_INV_TYPE_ERROR = 0,
    DOGECOIN_INV_TYPE_TX = 1,
    DOGECOIN_INV_TYPE_BLOCK = 2,
    DOGECOIN_INV_TYPE_FILTERED_BLOCK = 3,
    DOGECOIN_INV_TYPE_CMPCT_BLOCK = 4,
};

static const unsigned int MAX_HEADERS_RESULTS = 2000;
static const int DOGECOIN_PROTOCOL_VERSION = 70015;

typedef struct dogecoin_p2p_msg_hdr_ {
    unsigned char netmagic[4];
    char command[12];
    uint32_t data_len;
    unsigned char hash[4];
} dogecoin_p2p_msg_hdr;

typedef struct dogecoin_p2p_inv_msg_ {
    uint32_t type;
    uint256 hash;
} dogecoin_p2p_inv_msg;

typedef struct dogecoin_p2p_address_ {
    uint32_t time;
    uint64_t services;
    unsigned char ip[16];
    uint16_t port;
} dogecoin_p2p_address;

typedef struct dogecoin_p2p_version_msg_ {
    int32_t version;
    uint64_t services;
    int64_t timestamp;
    dogecoin_p2p_address addr_recv;
    dogecoin_p2p_address addr_from;
    uint64_t nonce;
    char useragent[128];
    int32_t start_height;
    uint8_t relay;
} dogecoin_p2p_version_msg;

/** getdata message type flags */
static const uint32_t MSG_TYPE_MASK    = 0xffffffff >> 2;

/** getdata / inv message types.
 * These numbers are defined by the protocol. When adding a new value, be sure
 * to mention it in the respective BIP.
 */
enum GetDataMsg
{
    MSG_TX = 1,
    MSG_BLOCK = 2,
    // ORed into other flags to add witness
    MSG_WITNESS_FLAG = 1 << 30,
    // The following can only occur in getdata. Invs always use TX or BLOCK.
    MSG_FILTERED_BLOCK = 3,  //!< Defined in BIP37
    MSG_CMPCT_BLOCK = 4,     //!< Defined in BIP152
    MSG_WITNESS_BLOCK = MSG_BLOCK | MSG_WITNESS_FLAG, //!< Defined in BIP144
    MSG_WITNESS_TX = MSG_TX | MSG_WITNESS_FLAG,       //!< Defined in BIP144
    MSG_FILTERED_WITNESS_BLOCK = MSG_FILTERED_BLOCK | MSG_WITNESS_FLAG,
};

/* =================================== */
/* VERSION MESSAGE */
/* =================================== */

/* sets a version message*/
LIBDOGECOIN_API void dogecoin_p2p_msg_version_init(dogecoin_p2p_version_msg* msg, const dogecoin_p2p_address* addrFrom, const dogecoin_p2p_address* addrTo, const char* strSubVer, dogecoin_bool relay);

/* serialize a p2p "version" message to an existing cstring */
LIBDOGECOIN_API void dogecoin_p2p_msg_version_ser(dogecoin_p2p_version_msg* msg, cstring* buf);

/* deserialize a p2p "version" message */
LIBDOGECOIN_API dogecoin_bool dogecoin_p2p_msg_version_deser(dogecoin_p2p_version_msg* msg, struct const_buffer* buf);

/* =================================== */
/* INV MESSAGE */
/* =================================== */

/* sets an inv message-element*/
LIBDOGECOIN_API void dogecoin_p2p_msg_inv_init(dogecoin_p2p_inv_msg* msg, uint32_t type, uint256 hash);

/* serialize a p2p "inv" message to an existing cstring */
LIBDOGECOIN_API void dogecoin_p2p_msg_inv_ser(dogecoin_p2p_inv_msg* msg, cstring* buf);

/* deserialize a p2p "inv" message-element */
LIBDOGECOIN_API dogecoin_bool dogecoin_p2p_msg_inv_deser(dogecoin_p2p_inv_msg* msg, struct const_buffer* buf);


/* =================================== */
/* ADDR MESSAGE */
/* =================================== */

/* initializes a p2p address structure */
LIBDOGECOIN_API void dogecoin_p2p_address_init(dogecoin_p2p_address* addr);

/* copy over a sockaddr (IPv4/IPv6) to a p2p address struct */
LIBDOGECOIN_API void dogecoin_addr_to_p2paddr(struct sockaddr* addr, dogecoin_p2p_address* addr_out);

/* deserialize a p2p address */
LIBDOGECOIN_API dogecoin_bool dogecoin_p2p_deser_addr(unsigned int protocol_version, dogecoin_p2p_address* addr, struct const_buffer* buf);

/* serialize a p2p addr */
LIBDOGECOIN_API void dogecoin_p2p_ser_addr(unsigned int protover, const dogecoin_p2p_address* addr, cstring* str_out);

/* copy over a p2p addr to a sockaddr object */
LIBDOGECOIN_API void dogecoin_p2paddr_to_addr(dogecoin_p2p_address* p2p_addr, struct sockaddr* addr_out);


/* =================================== */
/* P2P MSG-HDR */
/* =================================== */

/* deserialize the p2p message header from a buffer */
LIBDOGECOIN_API void dogecoin_p2p_deser_msghdr(dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf);

/* dogecoin_p2p_message_new does malloc a cstring, needs cleanup afterwards! */
LIBDOGECOIN_API cstring* dogecoin_p2p_message_new(const unsigned char netmagic[4], const char* command, const void* data, uint32_t data_len);


/* =================================== */
/* GETHEADER MESSAGE */
/* =================================== */

/* creates a getheader message */
LIBDOGECOIN_API void dogecoin_p2p_msg_getheaders(vector* blocklocators, uint256 hashstop, cstring* str_out);

/* directly deserialize a getheaders message to blocklocators, hashstop */
LIBDOGECOIN_API dogecoin_bool dogecoin_p2p_deser_msg_getheaders(vector* blocklocators, uint256 hashstop, struct const_buffer* buf);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_NET_PROTOCOL_H__
