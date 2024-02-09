/* Copyright 2012 exMULTI, Inc.
 * Distributed under the MIT/X11 software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include <string.h>

#include <dogecoin/cstr.h>
#include <dogecoin/mem.h>
#include <dogecoin/portable_endian.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>


/**
 * @brief This function appends a buffer of raw bytes
 * to an existing cstring.
 * 
 * @param s The pointer to the cstring to append to.
 * @param p The buffer of raw bytes to be appended.
 * @param len The length of the buffer to be appended.
 * 
 * @return Nothing.
 */
void ser_bytes(cstring* s, const void* p, size_t len)
{
    cstr_append_buf(s, p, len);
}

/**
 * @brief This function takes 2 unsigned bytes and
 * appends them to an existing cstring by converting
 * them to a little endian byte array.
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as a uint16_t.
 * 
 * @return Nothing.
 */
void ser_u16(cstring* s, uint16_t v_)
{
    uint16_t v = htole16(v_);
    cstr_append_buf(s, &v, sizeof(v));
}


/**
 * @brief This function takes 4 unsigned bytes and
 * appends them to an existing cstring by converting
 * them to a little endian byte array.
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as a uint32_t.
 * 
 * @return Nothing.
 */
void ser_u32(cstring* s, uint32_t v_)
{
    uint32_t v = htole32(v_);
    cstr_append_buf(s, &v, sizeof(v));
}


/**
 * @brief This function takes 4 bytes and appends them
 * to an existing cstring by converting them to a 
 * uint32_t and calling ser_u32().
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as an int32_t.
 * 
 * @return Nothing.
 */
void ser_s32(cstring* s, int32_t v_)
{
    ser_u32(s, (uint32_t)v_);
}


/**
 * @brief This function takes 8 unsigned bytes and
 * appends them to an existing cstring by converting
 * them to a little endian byte array.
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as a uint64_t.
 * 
 * @return Nothing.
 */
void ser_u64(cstring* s, uint64_t v_)
{
    uint64_t v = htole64(v_);
    cstr_append_buf(s, &v, sizeof(v));
}


/**
 * @brief This function takes 8 bytes and appends them
 * to an existing cstring by converting them to a
 * uint64_t and calling ser_u64().
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as an int64_t.
 * 
 * @return Nothing.
 */
void ser_s64(cstring* s, int64_t v_)
{
    ser_u64(s, (uint64_t)v_);
}


/**
 * @brief This function takes 32 unsigned bytes and
 * appends them to an existing cstring by converting
 * them to a little endian byte array.
 * 
 * @param s The pointer to the cstring to append to.
 * @param v_ The bytes to be appended as an unsigned char array.
 * 
 * @return Nothing.
 */
void ser_u256(cstring* s, const unsigned char* v_)
{
    ser_bytes(s, v_, 32);
}


/**
 * @brief This function takes a variable length unsigned
 * integer and appends the minimum number of bytes to
 * the cstring in order to preserve the data.
 * 
 * @param s The pointer to the cstring to append to.
 * @param vlen The bytes to be appended as a uint32_t.
 * 
 * @return Nothing.
 */
void ser_varlen(cstring* s, uint32_t vlen)
{
    unsigned char c;

    if (vlen < 253) {
        c = vlen;
        ser_bytes(s, &c, 1);
    }

    else if (vlen < 0x10000) {
        c = 253;
        ser_bytes(s, &c, 1);
        ser_u16(s, (uint16_t)vlen);
    }

    else {
        c = 254;
        ser_bytes(s, &c, 1);
        ser_u32(s, vlen);
    }

    /* u64 case intentionally not implemented */
}


/**
 * @brief This function takes a variable length string
 * and appends up to maxlen bytes to an existing cstring.
 * 
 * @param s The pointer to the cstring to append to.
 * @param s_in The bytes to be appended as a string.
 * @param maxlen The maximum number of bytes to be appended.
 * 
 * @return Nothing.
 */
void ser_str(cstring* s, const char* s_in, size_t maxlen)
{
    size_t slen = strnlen(s_in, maxlen);

    ser_varlen(s, slen);
    ser_bytes(s, s_in, slen);
}


/**
 * @brief This function takes a cstring and appends
 * its contents an existing cstring.
 * 
 * @param s The pointer to the cstring to append to.
 * @param s_in The pointer to the cstring to be appended.
 * 
 * @return Nothing.
 */
void ser_varstr(cstring* s, cstring* s_in)
{
    if (!s_in || !s_in->len) {
        ser_varlen(s, 0);
        return;
    }

    ser_varlen(s, s_in->len);
    ser_bytes(s, s_in->str, s_in->len);
}


/**
 * @brief This function skips the next len bytes of a
 * const_buffer by moving the pointer up by len bytes
 * and subtracting that amount from its len attribute.
 * 
 * @param buf The pointer to the const_buffer whose bytes will be skipped.
 * @param len The amount of bytes to be skipped.
 * 
 * @return 1 if the bytes were skipped successfully, 0 if too many bytes were specified.
 */
int deser_skip(struct const_buffer* buf, size_t len)
{
    char* p;
    if (buf->len < len) {
        return false;
    }

    p = (char*)buf->p;
    p += len;
    buf->p = p;
    buf->len -= len;

    return true;
}


/**
 * @brief This function takes a const_buffer and consumes
 * the first len bytes, which are then placed into a void
 * buffer.
 * 
 * @param po The pointer to the object to deserialize into.
 * @param buf The pointer to the const_buffer to deserialize from.
 * @param len The number of bytes to be deserialized.
 * 
 * @return 1 if the bytes were deserialized successfully, 0 if too many bytes were specified.
 */
int deser_bytes(void* po, struct const_buffer* buf, size_t len)
{
    char* p;
    if (buf->len < len) {
        return false;
    }

    memcpy_safe(po, buf->p, len);
    p = (char*)buf->p;
    p += len;
    buf->p = p;
    buf->len -= len;

    return true;
}


/**
 * @brief This function deserializes 2 bytes from a 
 * const_buffer into a uint16_t object.
 * 
 * @param vo The pointer to the uint16_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if the bytes were deserialized successfully, 0 if too many bytes were specified.
 */
int deser_u16(uint16_t* vo, struct const_buffer* buf)
{
    uint16_t v;

    if (!deser_bytes(&v, buf, sizeof(v))) {
        return false;
    }

    *vo = le16toh(v);
    return true;
}


/**
 * @brief This function deserializes 4 bytes from a
 * const_buffer into an int32_t object.
 * 
 * @param vo The pointer to the int32_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if bytes were deserialized correctly, 0 otherwise.
 */
int deser_s32(int32_t* vo, struct const_buffer* buf)
{
    int32_t v;

    if (!deser_bytes(&v, buf, sizeof(v))) {
        return false;
    }
    *vo = le32toh(v);
    return true;
}


/**
 * @brief This function deserializes 4 bytes from a
 * const_buffer into a uint32_t object.
 * 
 * @param vo The pointer to the uint32_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if the bytes were deserialized correctly, 0 otherwise.
 */
int deser_u32(uint32_t* vo, struct const_buffer* buf)
{
    uint32_t v;

    if (!deser_bytes(&v, buf, sizeof(v))) {
        return false;
    }
    *vo = le32toh(v);
    return true;
}


/**
 * @brief This function deserializes 8 bytes from a
 * const_buffer into a uint64_t object.
 * 
 * @param vo The pointer to the uint64_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if the bytes were deserialized correctly, 0 otherwise.
 */
int deser_u64(uint64_t* vo, struct const_buffer* buf)
{
    uint64_t v;

    if (!deser_bytes(&v, buf, sizeof(v))) {
        return false;
    }

    *vo = le64toh(v);
    return true;
}


/**
 * @brief This function deserializes 32 bytes from a
 * const_buffer into an unsigned byte array.
 * 
 * @param vo The unsigned byte array to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if the bytes were deserialized correctly, 0 otherwise.
 */
int deser_u256(uint8_t* vo, struct const_buffer* buf)
{
    return deser_bytes(vo, buf, 32);
}


/**
 * @brief This function deserializes a variable number
 * of bytes from a const_buffer into an variable length
 * unsigned integer. Only the minimum bytes needed to 
 * preserve its value are deserialized.
 * 
 * @param lo The pointer to the uint32_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if bytes were deserialized successfully, 0 otherwise.
 */
int deser_varlen(uint32_t* lo, struct const_buffer* buf)
{
    uint32_t len;

    unsigned char c;
    if (!deser_bytes(&c, buf, 1))
        return false;

    if (c == 253) {
        uint16_t v16;
        if (!deser_u16(&v16, buf))
            return false;
        len = v16;
    } else if (c == 254) {
        uint32_t v32;
        if (!deser_u32(&v32, buf))
            return false;
        len = v32;
    } else if (c == 255) {
        uint64_t v64;
        if (!deser_u64(&v64, buf))
            return false;
        len = (uint32_t)v64; /* WARNING: truncate */
    } else {
        len = c;
    }

    *lo = len;
    return true;
}

/**
 * Deserialize variable length from file.
 * 
 * @param lo the length of the variable-length data
 * @param file the file to read from
 * 
 * @return Nothing.
 */
int deser_varlen_from_file(uint32_t* lo, FILE* file)
{
    uint32_t len;
    struct const_buffer buf;
    unsigned char c;
    const unsigned char bufp[sizeof(uint64_t)];

    if (fread(&c, 1, 1, file) != 1)
        return false;

    buf.p = (void*)bufp;
    buf.len = sizeof(uint64_t);

    if (c == 253) {
        uint16_t v16;
        if (fread((void*)buf.p, 1, sizeof(v16), file) != sizeof(v16))
            return false;
        if (!deser_u16(&v16, &buf))
            return false;
        len = v16;
    } else if (c == 254) {
        uint32_t v32;
        if (fread((void*)buf.p, 1, sizeof(v32), file) != sizeof(v32))
            return false;
        if (!deser_u32(&v32, &buf))
            return false;
        len = v32;
    } else if (c == 255) {
        uint64_t v64;
        if (fread((void*)buf.p, 1, sizeof(v64), file) != sizeof(v64))
            return false;
        if (!deser_u64(&v64, &buf))
            return false;
        len = (uint32_t)v64; /* WARNING: truncate */
    } else
        len = c;

    *lo = len;
    return true;
}

/**
 * @brief This function reads the first byte of the file
 * as an indicator of how many bytes to deserialize. 
 * 
 * @param lo The length of the unsigned integer read from the file.
 * @param file The file which contains the bytes to deserialize from.
 * @param rawdata The buffer to be parsed.
 * @param buflen_inout The number of total bytes read from the file, including the first byte.
 * 
 * @return 1 if file is read successfully, 0 otherwise. 
 */
int deser_varlen_file(uint32_t* lo, FILE* file, uint8_t* rawdata, size_t* buflen_inout)
{
    uint32_t len;
    struct const_buffer buf;
    unsigned char c;
    const unsigned char bufp[sizeof(uint64_t)];

    /* check min size of the buffer */
    if (*buflen_inout < sizeof(len))
        return false;

    if (fread(&c, 1, 1, file) != 1)
        return false;

    rawdata[0] = c;
    *buflen_inout = 1;

    buf.p = (void*)bufp;
    buf.len = sizeof(uint64_t);

    if (c == 253) {
        uint16_t v16;
        if (fread((void*)buf.p, 1, sizeof(v16), file) != sizeof(v16))
            return false;
        memcpy_safe(rawdata + 1, buf.p, sizeof(v16));
        *buflen_inout += sizeof(v16);
        if (!deser_u16(&v16, &buf))
            return false;
        len = v16;
    } else if (c == 254) {
        uint32_t v32;
        if (fread((void*)buf.p, 1, sizeof(v32), file) != sizeof(v32))
            return false;
        memcpy_safe(rawdata + 1, buf.p, sizeof(v32));
        *buflen_inout += sizeof(v32);
        if (!deser_u32(&v32, &buf))
            return false;
        len = v32;
    } else if (c == 255) {
        uint64_t v64;
        if (fread((void*)buf.p, 1, sizeof(v64), file) != sizeof(v64))
            return false;
        memcpy_safe(rawdata + 1, buf.p, sizeof(uint32_t)); /* warning, truncate! */
        *buflen_inout += sizeof(uint32_t);
        if (!deser_u64(&v64, &buf))
            return false;
        len = (uint32_t)v64; /* WARNING: truncate */
    } else {
        len = c;
    }

    *lo = len;
    return true;
}


/**
 * @brief This function deserializes maxlen bytes from
 * a const_buffer into a string object.
 * 
 * @param so The string to deserialize into.
 * @param buf The pointer to the const_buffer to deserialize from.
 * @param maxlen The maximum number of bytes to deserialize.
 * 
 * @return 1 if deserialized successfully, 0 otherwise.
 */
int deser_str(char* so, struct const_buffer* buf, size_t maxlen)
{
    uint32_t len;
    uint32_t skip_len = 0;
    if (!deser_varlen(&len, buf)) {
        return false;
    }

    /* if input larger than buffer, truncate copy, skip remainder */
    if (len > maxlen) {
        skip_len = len - maxlen;
        len = maxlen;
    }

    if (!deser_bytes(so, buf, len)) {
        return false;
    }
    if (!deser_skip(buf, skip_len)) {
        return false;
    }

    /* add C string null */
    if (len < maxlen) {
        so[len] = 0;
    }
    else {
        so[maxlen - 1] = 0;
    }

    return true;
}


/**
 * @brief This function deserializes a variable length
 * string 
 * 
 * @param so The pointer to the pointer to the cstring to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if string deserialized successfully, 0 otherwise. 
 */
int deser_varstr(cstring** so, struct const_buffer* buf)
{
    uint32_t len;
    cstring* s;
    char* p;

    if (*so) {
        cstr_free(*so, 1);
        *so = NULL;
    }

    if (!deser_varlen(&len, buf))
        return false;

    if (buf->len < len)
        return false;

    s = cstr_new_sz(len);
    cstr_append_buf(s, buf->p, len);

    p = (char*)buf->p;
    p += len;
    buf->p = p;
    buf->len -= len;

    *so = s;

    return true;
}


/**
 * @brief This function deserializes 8 bytes from a
 * const_buffer into an int64_t object.
 * 
 * @param vo The pointer to the int64_t object to deserialize into.
 * @param buf The const_buffer to deserialize from.
 * 
 * @return 1 if bytes were deserialized correctly, 0 otherwise.
 */
int deser_s64(int64_t* vo, struct const_buffer* buf)
{
    return deser_u64((uint64_t*)vo, buf);
}
