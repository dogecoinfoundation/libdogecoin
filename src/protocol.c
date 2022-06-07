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

#include <dogecoin/protocol.h>
#include <dogecoin/hash.h>
#include <dogecoin/portable_endian.h>
#include <dogecoin/buffer.h>
#include <dogecoin/mem.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>

#include <assert.h>
#include <time.h>

enum {
    DOGECOIN_ADDR_TIME_VERSION = 31402,
    DOGECOIN_MIN_PROTO_VERSION = 70003,
};

/* IPv4 addresses are mapped to 16bytes with a prefix of 10 x 0x00 + 2 x 0xff */
static const uint8_t DOGECOIN_IPV4_PREFIX[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};

/**
 * If the first 12 bytes of the IP address are the same as the DOGECOIN_IPV4_PREFIX.
 * 
 * @param ipaddr the IP address in network byte order
 * 
 * @return static inline dogecoin_bool (uint8_t)
 */
static inline dogecoin_bool is_ipv4_mapped(const unsigned char* ipaddr)
{
    return memcmp(ipaddr, DOGECOIN_IPV4_PREFIX, 12) == 0;
}

/**
 * Initialize a dogecoin_p2p_address struct
 * 
 * @param addr the address structure to initialize
 */
void dogecoin_p2p_address_init(dogecoin_p2p_address* addr)
{
    dogecoin_mem_zero(addr, sizeof(*addr));
}

/**
 * Create a new cstring object with the header information for a dogecoin p2p message
 * 
 * @param netmagic The magic number is a 4-byte value that identifies the network.
 * @param command The command string.
 * @param data The data to be sent.
 * @param data_len The length of the data payload.
 * 
 * @return A cstring object.
 */
cstring* dogecoin_p2p_message_new(const unsigned char netmagic[4], const char* command, const void* data, uint32_t data_len)
{
    cstring* s = cstr_new_sz(DOGECOIN_P2P_HDRSZ + data_len);

    /* network identifier (magic number) */
    cstr_append_buf(s, netmagic, 4);

    /* command string */
    char command_null[12];
    dogecoin_mem_zero(command_null, 12);
    memcpy_safe(command_null, command, strlen(command));
    //memset(command_null+strlen(command), 0, 12-strlen(command));
    cstr_append_buf(s, command_null, 12);

    /* data length, always 4 bytes */
    uint32_t data_len_le = htole32(data_len);
    cstr_append_buf(s, &data_len_le, 4);

    /* data checksum (first 4 bytes of the double sha256 hash of the pl) */
    uint256 msghash;
    dogecoin_hash(data, data_len, msghash);
    cstr_append_buf(s, &msghash[0], 4);

    /* data payload */
    if (data_len > 0)
        cstr_append_buf(s, data, data_len);

    return s;
}

/**
 * Deserialize an address
 * 
 * @param protocol_version The version of the protocol the node is using.
 * @param addr the address object to be filled in
 * @param buf The buffer to deserialize from.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_p2p_deser_addr(unsigned int protocol_version, dogecoin_p2p_address* addr, struct const_buffer* buf)
{
    if (protocol_version >= DOGECOIN_ADDR_TIME_VERSION) {
        if (!deser_u32(&addr->time, buf))
            return false;
    } else
        addr->time = 0;

    if (!deser_u64(&addr->services, buf))
        return false;
    if (!deser_bytes(&addr->ip, buf, 16))
        return false;
    if (!deser_u16(&addr->port, buf))
        return false;
    return true;
}

/**
 * Serialize a dogecoin_p2p_address struct to a cstring
 * 
 * @param protover The protocol version.
 * @param addr The address to serialize.
 * @param s The cstring to serialize to.
 */
void dogecoin_p2p_ser_addr(unsigned int protover, const dogecoin_p2p_address* addr, cstring* s)
{
    if (protover >= DOGECOIN_ADDR_TIME_VERSION)
        ser_u32(s, addr->time);
    ser_u64(s, addr->services);
    ser_bytes(s, addr->ip, 16);
    ser_u16(s, addr->port);
}


/**
 * Convert a sockaddr to a dogecoin_p2p_address
 * 
 * @param addr The address to convert.
 * @param addr_out the address to be filled in
 */
void dogecoin_addr_to_p2paddr(struct sockaddr* addr, dogecoin_p2p_address* addr_out)
{
    if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6* saddr = (struct sockaddr_in6*)addr;
        memcpy_safe(&addr_out->ip, &saddr->sin6_addr, 16);
        addr_out->port = ntohs(saddr->sin6_port);
    } else if (addr->sa_family == AF_INET) {
        struct sockaddr_in* saddr = (struct sockaddr_in*)addr;
        dogecoin_mem_zero(&addr_out->ip[0], 10);
        memset(&addr_out->ip[10], 0xff, 2);
        memcpy_safe(&addr_out->ip[12], &saddr->sin_addr, 4);
        addr_out->port = ntohs(saddr->sin_port);
    }
}

/**
 * Convert a dogecoin_p2p_address struct to a sockaddr struct
 * 
 * @param p2p_addr the p2p_address struct to convert
 * @param addr_out The address to write the result to.
 * 
 * @return a pointer to a dogecoin_p2p_address structure.
 */
void dogecoin_p2paddr_to_addr(dogecoin_p2p_address* p2p_addr, struct sockaddr* addr_out)
{
    if (!p2p_addr || !addr_out)
        return;

    if (!is_ipv4_mapped(p2p_addr->ip)) {
        /* ipv6 */
        struct sockaddr_in6* saddr = (struct sockaddr_in6*)addr_out;
        memcpy_safe(&saddr->sin6_addr, p2p_addr->ip, 16);
        saddr->sin6_port = htons(p2p_addr->port);
    } else {
        struct sockaddr_in* saddr = (struct sockaddr_in*)addr_out;
        memcpy_safe(&saddr->sin_addr, &p2p_addr->ip[12], 4);
        saddr->sin_port = htons(p2p_addr->port);
    }
}

/**
 * Initialize a dogecoin_p2p_version_msg struct with the given parameters
 * 
 * @param msg the message object
 * @param addrFrom The address of the node that sent the message.
 * @param addrTo The address to which the message is being sent.
 * @param strSubVer The user agent string.
 * @param relay If set to true, the node will relay transactions and blocks to other nodes.
 */
void dogecoin_p2p_msg_version_init(dogecoin_p2p_version_msg* msg, const dogecoin_p2p_address* addrFrom, const dogecoin_p2p_address* addrTo, const char* strSubVer, dogecoin_bool relay)
{
    msg->version = DOGECOIN_PROTOCOL_VERSION;
    msg->services = 0;
    msg->timestamp = time(NULL);
    msg->timestamp = time(NULL);
    if (addrTo)
        msg->addr_recv = *addrTo;
    else
        dogecoin_p2p_address_init(&msg->addr_recv);
    if (addrFrom)
        msg->addr_from = *addrFrom;
    else
        dogecoin_p2p_address_init(&msg->addr_from);
    dogecoin_cheap_random_bytes((uint8_t*)&msg->nonce, sizeof(msg->nonce));
    if (strSubVer && strlen(strSubVer) < 128)
        memcpy_safe(msg->useragent, strSubVer, strlen(strSubVer));

    msg->start_height = 0;
    msg->relay = relay;
}

/**
 * It serializes a dogecoin_p2p_version_msg object into a cstring
 * 
 * @param msg the message object
 * @param buf the buffer to serialize into
 */
void dogecoin_p2p_msg_version_ser(dogecoin_p2p_version_msg* msg, cstring* buf)
{
    ser_s32(buf, msg->version);
    ser_u64(buf, msg->services);
    ser_s64(buf, msg->timestamp);
    dogecoin_p2p_ser_addr(0, &msg->addr_recv, buf);
    dogecoin_p2p_ser_addr(0, &msg->addr_from, buf);
    ser_u64(buf, msg->nonce);
    ser_str(buf, msg->useragent, 1024);
    ser_s32(buf, msg->start_height);
    cstr_append_c(buf, msg->relay);
}

/**
 * Deserialize a version message
 * 
 * @param msg the message object to be filled
 * @param buf the buffer to deserialize from
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_p2p_msg_version_deser(dogecoin_p2p_version_msg* msg, struct const_buffer* buf)
{
    dogecoin_mem_zero(msg, sizeof(*msg));
    if (!deser_s32(&msg->version, buf))
        return false;
    if (!deser_u64(&msg->services, buf))
        return false;
    if (!deser_s64(&msg->timestamp, buf))
        return false;
    if (!dogecoin_p2p_deser_addr(0, &msg->addr_recv, buf))
        return false;
    if (!dogecoin_p2p_deser_addr(0, &msg->addr_from, buf))
        return false;
    if (!deser_u64(&msg->nonce, buf))
        return false;

    uint32_t ua_len;
    uint32_t cpy_len;
    if (!deser_varlen(&ua_len, buf))
        return false;

    cpy_len = ua_len;
    if (cpy_len > 128)
        cpy_len = 128;
    /* we only support user agent strings up to 1024 chars */
    /* we only copy 128bytes to the message structure */
    if (ua_len > 1024)
        return false;

    char ua_str[1024];
    if (!deser_bytes(ua_str, buf, ua_len))
        return false;
    dogecoin_mem_zero(msg->useragent, sizeof(msg->useragent));
    memcpy_safe(&msg->useragent, ua_str, cpy_len);

    if (!deser_s32(&msg->start_height, buf))
        return false;
    if (msg->version > 70003)
        if (!deser_bytes(&msg->relay, buf, 1))
            return false;

    return true;
}

/**
 * Initialize a dogecoin_p2p_inv_msg object
 * 
 * @param msg The message object to initialize.
 * @param type The type of the inventory message.
 * @param hash The hash of the item being advertised.
 */
void dogecoin_p2p_msg_inv_init(dogecoin_p2p_inv_msg* msg, uint32_t type, uint256 hash)
{
    msg->type = type;
    memcpy_safe(&msg->hash, hash, DOGECOIN_HASH_LENGTH);
}

/**
 * Serialize a dogecoin_p2p_inv_msg to a cstring.
 * 
 * @param msg The message object to serialize.
 * @param buf The buffer to serialize into.
 */
void dogecoin_p2p_msg_inv_ser(dogecoin_p2p_inv_msg* msg, cstring* buf)
{
    ser_u32(buf, msg->type);
    ser_bytes(buf, msg->hash, DOGECOIN_HASH_LENGTH);
}

/**
 * Deserialize a dogecoin_p2p_inv_msg from a const_buffer
 * 
 * @param msg The message object to be filled in.
 * @param buf The buffer to deserialize from.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_p2p_msg_inv_deser(dogecoin_p2p_inv_msg* msg, struct const_buffer* buf)
{
    dogecoin_mem_zero(msg, sizeof(*msg));
    if (!deser_u32(&msg->type, buf))
        return false;
    if (!deser_u256(msg->hash, buf))
        return false;
    return true;
}

/**
 * This function serializes a getheaders message
 * 
 * @param blocklocators a vector of block hashes
 * @param hashstop The hash of the last block in the chain that we want to download.
 * @param s the serialized message
 */
void dogecoin_p2p_msg_getheaders(vector* blocklocators, uint256 hashstop, cstring* s)
{
    unsigned int i;

    ser_u32(s, DOGECOIN_PROTOCOL_VERSION);
    ser_varlen(s, blocklocators->len);
    for (i = 0; i < blocklocators->len; i++) {
        uint256 *hash = vector_idx(blocklocators, i);
        ser_bytes(s, hash, DOGECOIN_HASH_LENGTH);
    }
    if (hashstop)
        ser_bytes(s, hashstop, DOGECOIN_HASH_LENGTH);
    else
        ser_bytes(s, NULLHASH, DOGECOIN_HASH_LENGTH);
}

/**
 * Deserialize a getheaders message
 * 
 * @param blocklocators a vector of block hashes
 * @param hashstop The hash of the last block that we want to request.
 * @param buf the buffer to deserialize from
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool dogecoin_p2p_deser_msg_getheaders(vector* blocklocators, uint256 hashstop, struct const_buffer* buf)
{
    int32_t version;
    uint32_t vsize;
    if (!deser_s32(&version, buf))
        return false;
    if (!deser_varlen(&vsize, buf))
        return false;
    vector_resize(blocklocators, vsize);
    for (unsigned int i = 0; i < vsize; i++) {
        uint256 *hash = dogecoin_malloc(DOGECOIN_HASH_LENGTH);
        if (!deser_u256(*hash, buf)) {
            dogecoin_free(hash);
            return false;
        }
        vector_add(blocklocators, hash);
    }
    if (!deser_u256(hashstop, buf))
        return false;
    return true;
}

/**
 * Deserialize the dogecoin_p2p_msg_hdr structure
 * 
 * @param hdr The dogecoin_p2p_msg_hdr struct that we're deserializing into.
 * @param buf The buffer to deserialize from.
 */
void dogecoin_p2p_deser_msghdr(dogecoin_p2p_msg_hdr* hdr, struct const_buffer* buf)
{
    deser_bytes(hdr->netmagic, buf, 4);
    deser_bytes(hdr->command, buf, 12);
    deser_u32(&hdr->data_len, buf);
    deser_bytes(hdr->hash, buf, 4);
}
