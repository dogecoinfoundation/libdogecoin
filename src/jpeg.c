/* Single-file no-dependency JPEG encode based on JPEC munged together by michilumin */

/* Required (C) info as follows */

/*JPEC: */
/**
 * Copyright (c) 2012-2016 Moodstocks SAS
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// (c) 2023 michilumin
// (c) 2023 The Dogecoin Foundation


#include <qr/jpeg.h>

//--buffer defines
#define JPEC_BUFFER_INIT_SIZ 65536

//--huffman defines

#ifdef WORD_BIT
#define JPEC_INT_WIDTH WORD_BIT
#else
#define JPEC_INT_WIDTH (int)(sizeof(int) * CHAR_BIT)
#endif

#if __GNUC__
#define JPEC_HUFF_NBITS(JPEC_nbits, JPEC_val) \
  JPEC_nbits = (!JPEC_val) ? 0 : JPEC_INT_WIDTH - __builtin_clz(JPEC_val)
#else
#define JPEC_HUFF_NBITS(JPEC_nbits, JPEC_val) \
  JPEC_nbits = 0; \
  while (val) { \
    JPEC_nbits++; \
    val >>= 1; \
  }
#endif

//--encoder defines
#define JPEG_ENC_DEF_QUAL   93 /* default quality factor */
#define JPEC_ENC_HEAD_SIZ  330 /* header typical size in bytes */
#define JPEC_ENC_BLOCK_SIZ  30 /* 8x8 entropy coded block typical size in bytes */

/*-------------------- buffer --------------------*/



jpec_buffer_t* jpec_buffer_new(void) {
	return jpec_buffer_new2(-1);
}

jpec_buffer_t* jpec_buffer_new2(int siz) {
	if (siz < 0) siz = 0;
	jpec_buffer_t* b = malloc(sizeof(*b));
	b->stream = siz > 0 ? malloc(siz) : NULL;
	b->siz = siz;
	b->len = 0;
	return b;
}

void jpec_buffer_del(jpec_buffer_t* b) {
	assert(b);
	if (b->stream) free(b->stream);
	free(b);
}

void jpec_buffer_write_byte(jpec_buffer_t* b, int val) {
	assert(b);
	if (b->siz == b->len) {
		int nsiz = (b->siz > 0) ? 2 * b->siz : JPEC_BUFFER_INIT_SIZ;
		void* tmp = realloc(b->stream, nsiz);
		b->stream = (uint8_t*)tmp;
		b->siz = nsiz;
	}
	b->stream[b->len++] = (uint8_t)val;
}

void jpec_buffer_write_2bytes(jpec_buffer_t* b, int val) {
	assert(b);
	jpec_buffer_write_byte(b, (val >> 8) & 0xFF);
	jpec_buffer_write_byte(b, val & 0xFF);
}

/*-------------------- Huffman Enc ------------------*/


/* Private function prototypes */
static void jpec_huff_encode_block_impl(jpec_block_t* block, jpec_huff_state_t* s);
static void jpec_huff_write_bits(jpec_huff_state_t* s, unsigned int bits, int n);

void jpec_huff_skel_init(jpec_huff_skel_t* skel) {
	assert(skel);
	memset(skel, 0, sizeof(*skel));
	skel->opq = jpec_huff_new();
	skel->del = (void (*)(void*))jpec_huff_del;
	skel->encode_block = (void (*)(void*, jpec_block_t*, jpec_buffer_t*))jpec_huff_encode_block;
}

jpec_huff_t* jpec_huff_new(void) {
	jpec_huff_t* h = malloc(sizeof(*h));
	h->state.buffer = 0;
	h->state.nbits = 0;
	h->state.dc = 0;
	h->state.buf = NULL;
	return h;
}

void jpec_huff_del(jpec_huff_t* h) {
	assert(h);
	/* Flush any remaining bits and fill in the incomple byte (if any) with 1-s */
	jpec_huff_write_bits(&h->state, 0x7F, 7);
	free(h);
}

void jpec_huff_encode_block(jpec_huff_t* h, jpec_block_t* block, jpec_buffer_t* buf) {
	assert(h && block && buf);
	jpec_huff_state_t state;
	state.buffer = h->state.buffer;
	state.nbits = h->state.nbits;
	state.dc = h->state.dc;
	state.buf = buf;
	jpec_huff_encode_block_impl(block, &state);
	h->state.buffer = state.buffer;
	h->state.nbits = state.nbits;
	h->state.dc = state.dc;
	h->state.buf = state.buf;
}

static void jpec_huff_encode_block_impl(jpec_block_t* block, jpec_huff_state_t* s) {
	assert(block && s);
	int val, bits, nbits;
	/* DC coefficient encoding */
	if (block->len > 0) {
		val = block->zz[0] - s->dc;
		s->dc = block->zz[0];
	}
	else {
		val = -s->dc;
		s->dc = 0;
	}
	bits = val;
	if (val < 0) {
		val = -val;
		bits = ~val;
	}
	JPEC_HUFF_NBITS(nbits, val);
	jpec_huff_write_bits(s, jpec_dc_code[nbits], jpec_dc_len[nbits]);
	if (nbits) jpec_huff_write_bits(s, (unsigned int)bits, nbits);
	/* AC coefficients encoding (w/ RLE of zeros) */
	int nz = 0;
	for (int i = 1; i < block->len; i++) {
		if ((val = block->zz[i]) == 0) nz++;
		else {
			while (nz >= 16) {
				jpec_huff_write_bits(s, jpec_ac_code[0xF0], jpec_ac_len[0xF0]); /* ZRL code */
				nz -= 16;
			}
			bits = val;
			if (val < 0) {
				val = -val;
				bits = ~val;
			}
			JPEC_HUFF_NBITS(nbits, val);
			int j = (nz << 4) + nbits;
			jpec_huff_write_bits(s, jpec_ac_code[j], jpec_ac_len[j]);
			if (nbits) jpec_huff_write_bits(s, (unsigned int)bits, nbits);
			nz = 0;
		}
	}
	if (block->len < 64) {
		jpec_huff_write_bits(s, jpec_ac_code[0x00], jpec_ac_len[0x00]); /* EOB marker */
	}
}

/* Write n bits into the JPEG buffer, with 0 < n <= 16.
 *
 * == Details
 * - 16 bits are large enough to hold any zig-zag coeff or the longest AC code
 * - bits are chunked into bytes before being written into the JPEG buffer
 * - any remaining bits are kept in memory by the Huffman state
 * - at most 7 bits can be kept in memory
 * - a 32-bit integer buffer is used internally
 * - only the right 24 bits part of this buffer are used
 * - the input bits and remaining bits (if any) are left-justified on this part
 * - a mask is used to mask off any extra bits: useful when the input value has been
 *   first transformed by bitwise complement(|initial value|)
 * - if an 0xFF byte is detected a 0x00 stuff byte is automatically written right after
 */
static void jpec_huff_write_bits(jpec_huff_state_t* s, unsigned int bits, int n) {
	assert(s && n > 0 && n <= 16);
	int32_t mask = (((int32_t)1) << n) - 1;
	int32_t buffer = (int32_t)bits;
	int nbits = s->nbits + n;
	buffer &= mask;
	buffer <<= 24 - nbits;
	buffer |= s->buffer;
	while (nbits >= 8) {
		int chunk = (int)((buffer >> 16) & 0xFF);
		jpec_buffer_write_byte(s->buf, chunk);
		if (chunk == 0xFF) jpec_buffer_write_byte(s->buf, 0x00);
		buffer <<= 8;
		nbits -= 8;
	}
	s->buffer = buffer;
	s->nbits = nbits;
}

/*----------------------- conf/const -------------------------*/

const uint8_t jpec_qzr[64] = {
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68,109,103, 77,
	24, 35, 55, 64, 81,104,113, 92,
	49, 64, 78, 87,103,121,120,101,
	72, 92, 95, 98,112,100,103, 99 };

const float jpec_dct[7] = {
	0.49039, 0.46194, 0.41573, 0.35355,
	0.27779, 0.19134, 0.09755 };

const int jpec_zz[64] = {
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63 };

const uint8_t jpec_dc_nodes[17] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
const int jpec_dc_nb_vals = 12; /* sum of dc_nodes */
const uint8_t jpec_dc_vals[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };

const uint8_t jpec_ac_nodes[17] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
const int jpec_ac_nb_vals = 162; /* sum of ac_nodes */
const uint8_t jpec_ac_vals[162] = {
	0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12, /* 0x00: EOB */
	0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
	0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
	0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0, /* 0xf0: ZRL */
	0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,
	0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
	0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,
	0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
	0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
	0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
	0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,
	0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
	0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
	0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
	0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,
	0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
	0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,
	0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
	0xf9,0xfa };

const uint8_t jpec_dc_len[12] = { 2,3,3,3,3,3,4,5,6,7,8,9 };
const int jpec_dc_code[12] = {
	0x000,0x002,0x003,0x004,0x005,0x006,
	0x00e,0x01e,0x03e,0x07e,0x0fe,0x1fe };

const int8_t jpec_ac_len[256] = {
	 4, 2, 2, 3, 4, 5, 7, 8,
	10,16,16, 0, 0, 0, 0, 0,
	 0, 4, 5, 7, 9,11,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 5, 8,10,12,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 6, 9,12,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 6,10,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 7,11,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 7,12,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 8,12,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 9,15,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 9,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0, 9,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0,10,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0,10,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0,11,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	 0,16,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0,
	11,16,16,16,16,16,16,16,
	16,16,16, 0, 0, 0, 0, 0 };

const int jpec_ac_code[256] = {
	0x000a,0x0000,0x0001,0x0004,0x000b,0x001a,0x0078,0x00f8,
	0x03f6,0xff82,0xff83,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x000c,0x001b,0x0079,0x01f6,0x07f6,0xff84,0xff85,
	0xff86,0xff87,0xff88,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x001c,0x00f9,0x03f7,0x0ff4,0xff89,0xff8a,0xff8b,
	0xff8c,0xff8d,0xff8e,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x003a,0x01f7,0x0ff5,0xff8f,0xff90,0xff91,0xff92,
	0xff93,0xff94,0xff95,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x003b,0x03f8,0xff96,0xff97,0xff98,0xff99,0xff9a,
	0xff9b,0xff9c,0xff9d,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x007a,0x07f7,0xff9e,0xff9f,0xffa0,0xffa1,0xffa2,
	0xffa3,0xffa4,0xffa5,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x007b,0x0ff6,0xffa6,0xffa7,0xffa8,0xffa9,0xffaa,
	0xffab,0xffac,0xffad,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x00fa,0x0ff7,0xffae,0xffaf,0xffb0,0xffb1,0xffb2,
	0xffb3,0xffb4,0xffb5,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x01f8,0x7fc0,0xffb6,0xffb7,0xffb8,0xffb9,0xffba,
	0xffbb,0xffbc,0xffbd,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x01f9,0xffbe,0xffbf,0xffc0,0xffc1,0xffc2,0xffc3,
	0xffc4,0xffc5,0xffc6,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x01fa,0xffc7,0xffc8,0xffc9,0xffca,0xffcb,0xffcc,
	0xffcd,0xffce,0xffcf,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x03f9,0xffd0,0xffd1,0xffd2,0xffd3,0xffd4,0xffd5,
	0xffd6,0xffd7,0xffd8,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x03fa,0xffd9,0xffda,0xffdb,0xffdc,0xffdd,0xffde,
	0xffdf,0xffe0,0xffe1,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x07f8,0xffe2,0xffe3,0xffe4,0xffe5,0xffe6,0xffe7,
	0xffe8,0xffe9,0xffea,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0xffeb,0xffec,0xffed,0xffee,0xffef,0xfff0,0xfff1,
	0xfff2,0xfff3,0xfff4,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x07f9,0xfff5,0xfff6,0xfff7,0xfff8,0xfff9,0xfffa,0xfffb,
	0xfffc,0xfffd,0xfffe,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*------------------------ encode -------------------------*/


/* Private function prototypes */
static void jpec_enc_init_dqt(jpec_enc_t* e);
static void jpec_enc_open(jpec_enc_t* e);
static void jpec_enc_close(jpec_enc_t* e);
static void jpec_enc_write_soi(jpec_enc_t* e);
static void jpec_enc_write_app0(jpec_enc_t* e);
static void jpec_enc_write_dqt(jpec_enc_t* e);
static void jpec_enc_write_sof0(jpec_enc_t* e);
static void jpec_enc_write_dht(jpec_enc_t* e);
static void jpec_enc_write_sos(jpec_enc_t* e);
static int jpec_enc_next_block(jpec_enc_t* e);
static void jpec_enc_block_dct(jpec_enc_t* e);
static void jpec_enc_block_quant(jpec_enc_t* e);
static void jpec_enc_block_zz(jpec_enc_t* e);


jpec_enc_t* jpec_enc_new(const uint8_t* img, uint16_t w, uint16_t h) {
	return jpec_enc_new2(img, w, h, JPEG_ENC_DEF_QUAL);
}

jpec_enc_t* jpec_enc_new2(const uint8_t* img, uint16_t w, uint16_t h, int q) {
	if (img && w > 0 && !(w & 0x7) && h > 0 && !(h & 0x7)) 
	{
                printf("width and height assert failed\n");
	}
	jpec_enc_t* e = malloc(sizeof(*e));
	e->img = img;
	e->w = w;
	e->w8 = (((w - 1) >> 3) + 1) << 3;
	e->h = h;
	e->qual = q;
	e->bmax = (((w - 1) >> 3) + 1) * (((h - 1) >> 3) + 1);
	e->bnum = -1;
	e->bx = -1;
	e->by = -1;
	int bsiz = JPEC_ENC_HEAD_SIZ + e->bmax * JPEC_ENC_BLOCK_SIZ;
	e->buf = jpec_buffer_new2(bsiz);
	e->hskel = malloc(sizeof(*e->hskel));
	return e;
}

void jpec_enc_del(jpec_enc_t* e) {
	assert(e);
	if (e->buf) jpec_buffer_del(e->buf);
	free(e->hskel);
	free(e);
}

const uint8_t* jpec_enc_run(jpec_enc_t* e, int* len) {
	assert(e && len);
	jpec_enc_open(e);
	while (jpec_enc_next_block(e)) {
		jpec_enc_block_dct(e);
		jpec_enc_block_quant(e);
		jpec_enc_block_zz(e);
		e->hskel->encode_block(e->hskel->opq, &e->block, e->buf);
	}
	jpec_enc_close(e);
	*len = e->buf->len;
	return e->buf->stream;
}

/* Update the internal quantization matrix according to the asked quality */
static void jpec_enc_init_dqt(jpec_enc_t* e) {
	assert(e);
	float qualf = (float)e->qual;
	float scale = (e->qual < 50) ? (50 / qualf) : (2 - qualf / 50);
	for (int i = 0; i < 64; i++) {
		int a = (int)((float)jpec_qzr[i] * scale + 0.5);
		a = (a < 1) ? 1 : ((a > 255) ? 255 : a);
		e->dqt[i] = a;
	}
}

static void jpec_enc_open(jpec_enc_t* e) {
	assert(e);
	jpec_huff_skel_init(e->hskel);
	jpec_enc_init_dqt(e);
	jpec_enc_write_soi(e);
	jpec_enc_write_app0(e);
	jpec_enc_write_dqt(e);
	jpec_enc_write_sof0(e);
	jpec_enc_write_dht(e);
	jpec_enc_write_sos(e);
}

static void jpec_enc_close(jpec_enc_t* e) {
	assert(e);
	e->hskel->del(e->hskel->opq);
	jpec_buffer_write_2bytes(e->buf, 0xFFD9); /* EOI marker */
}

static void jpec_enc_write_soi(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFD8); /* SOI marker */
}

static void jpec_enc_write_app0(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFE0); /* APP0 marker */
	jpec_buffer_write_2bytes(e->buf, 0x0010); /* segment length */
	jpec_buffer_write_byte(e->buf, 0x4A);     /* 'J' */
	jpec_buffer_write_byte(e->buf, 0x46);     /* 'F' */
	jpec_buffer_write_byte(e->buf, 0x49);     /* 'I' */
	jpec_buffer_write_byte(e->buf, 0x46);     /* 'F' */
	jpec_buffer_write_byte(e->buf, 0x00);     /* '\0' */
	jpec_buffer_write_2bytes(e->buf, 0x0101); /* v1.1 */
	jpec_buffer_write_byte(e->buf, 0x00);     /* no density unit */
	jpec_buffer_write_2bytes(e->buf, 0x0001); /* X density = 1 */
	jpec_buffer_write_2bytes(e->buf, 0x0001); /* Y density = 1 */
	jpec_buffer_write_byte(e->buf, 0x00);     /* thumbnail width = 0 */
	jpec_buffer_write_byte(e->buf, 0x00);     /* thumbnail height = 0 */
}

static void jpec_enc_write_dqt(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFDB); /* DQT marker */
	jpec_buffer_write_2bytes(e->buf, 0x0043); /* segment length */
	jpec_buffer_write_byte(e->buf, 0x00);     /* table 0, 8-bit precision (0) */
	for (int i = 0; i < 64; i++) {
		jpec_buffer_write_byte(e->buf, e->dqt[jpec_zz[i]]);
	}
}

static void jpec_enc_write_sof0(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFC0); /* SOF0 marker */
	jpec_buffer_write_2bytes(e->buf, 0x000B); /* segment length */
	jpec_buffer_write_byte(e->buf, 0x08);     /* 8-bit precision */
	jpec_buffer_write_2bytes(e->buf, e->h);
	jpec_buffer_write_2bytes(e->buf, e->w);
	jpec_buffer_write_byte(e->buf, 0x01);     /* 1 component only (grayscale) */
	jpec_buffer_write_byte(e->buf, 0x01);     /* component ID = 1 */
	jpec_buffer_write_byte(e->buf, 0x11);     /* no subsampling */
	jpec_buffer_write_byte(e->buf, 0x00);     /* quantization table 0 */
}

static void jpec_enc_write_dht(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFC4);          /* DHT marker */
	jpec_buffer_write_2bytes(e->buf, 19 + jpec_dc_nb_vals); /* segment length */
	jpec_buffer_write_byte(e->buf, 0x00);              /* table 0 (DC), type 0 (0 = Y, 1 = UV) */
	for (int i = 0; i < 16; i++) {
		jpec_buffer_write_byte(e->buf, jpec_dc_nodes[i + 1]);
	}
	for (int i = 0; i < jpec_dc_nb_vals; i++) {
		jpec_buffer_write_byte(e->buf, jpec_dc_vals[i]);
	}
	jpec_buffer_write_2bytes(e->buf, 0xFFC4);           /* DHT marker */
	jpec_buffer_write_2bytes(e->buf, 19 + jpec_ac_nb_vals);
	jpec_buffer_write_byte(e->buf, 0x10);               /* table 1 (AC), type 0 (0 = Y, 1 = UV) */
	for (int i = 0; i < 16; i++) {
		jpec_buffer_write_byte(e->buf, jpec_ac_nodes[i + 1]);
	}
	for (int i = 0; i < jpec_ac_nb_vals; i++) {
		jpec_buffer_write_byte(e->buf, jpec_ac_vals[i]);
	}
}

static void jpec_enc_write_sos(jpec_enc_t* e) {
	assert(e);
	jpec_buffer_write_2bytes(e->buf, 0xFFDA); /* SOS marker */
	jpec_buffer_write_2bytes(e->buf, 8);      /* segment length */
	jpec_buffer_write_byte(e->buf, 0x01);     /* nb. components */
	jpec_buffer_write_byte(e->buf, 0x01);     /* Y component ID */
	jpec_buffer_write_byte(e->buf, 0x00);     /* Y HT = 0 */
	/* segment end */
	jpec_buffer_write_byte(e->buf, 0x00);
	jpec_buffer_write_byte(e->buf, 0x3F);
	jpec_buffer_write_byte(e->buf, 0x00);
}

static int jpec_enc_next_block(jpec_enc_t* e) {
	assert(e);
	int rv = (++e->bnum >= e->bmax) ? 0 : 1;
	if (rv) {
		e->bx = (e->bnum << 3) % e->w8;
		e->by = ((e->bnum << 3) / e->w8) << 3;
	}
	return rv;
}

static void jpec_enc_block_dct(jpec_enc_t* e) {
	assert(e && e->bnum >= 0);
#define JPEC_BLOCK(col,row) e->img[(((e->by + row) < e->h) ? e->by + row : e->h-1) * \
                            e->w + (((e->bx + col) < e->w) ? e->bx + col : e->w-1)]
	const float* coeff = jpec_dct;
	float tmp[64];
	for (int row = 0; row < 8; row++) {
		/* NOTE: the shift by 256 allows resampling from [0 255] to [-128 127] */
		float s0 = (float)(JPEC_BLOCK(0, row) + JPEC_BLOCK(7, row) - 256);
		float s1 = (float)(JPEC_BLOCK(1, row) + JPEC_BLOCK(6, row) - 256);
		float s2 = (float)(JPEC_BLOCK(2, row) + JPEC_BLOCK(5, row) - 256);
		float s3 = (float)(JPEC_BLOCK(3, row) + JPEC_BLOCK(4, row) - 256);

		float d0 = (float)(JPEC_BLOCK(0, row) - JPEC_BLOCK(7, row));
		float d1 = (float)(JPEC_BLOCK(1, row) - JPEC_BLOCK(6, row));
		float d2 = (float)(JPEC_BLOCK(2, row) - JPEC_BLOCK(5, row));
		float d3 = (float)(JPEC_BLOCK(3, row) - JPEC_BLOCK(4, row));

		tmp[8 * row] = coeff[3] * (s0 + s1 + s2 + s3);
		tmp[8 * row + 1] = coeff[0] * d0 + coeff[2] * d1 + coeff[4] * d2 + coeff[6] * d3;
		tmp[8 * row + 2] = coeff[1] * (s0 - s3) + coeff[5] * (s1 - s2);
		tmp[8 * row + 3] = coeff[2] * d0 - coeff[6] * d1 - coeff[0] * d2 - coeff[4] * d3;
		tmp[8 * row + 4] = coeff[3] * (s0 - s1 - s2 + s3);
		tmp[8 * row + 5] = coeff[4] * d0 - coeff[0] * d1 + coeff[6] * d2 + coeff[2] * d3;
		tmp[8 * row + 6] = coeff[5] * (s0 - s3) + coeff[1] * (s2 - s1);
		tmp[8 * row + 7] = coeff[6] * d0 - coeff[4] * d1 + coeff[2] * d2 - coeff[0] * d3;
	}
	for (int col = 0; col < 8; col++) {
		float s0 = tmp[col] + tmp[56 + col];
		float s1 = tmp[8 + col] + tmp[48 + col];
		float s2 = tmp[16 + col] + tmp[40 + col];
		float s3 = tmp[24 + col] + tmp[32 + col];

		float d0 = tmp[col] - tmp[56 + col];
		float d1 = tmp[8 + col] - tmp[48 + col];
		float d2 = tmp[16 + col] - tmp[40 + col];
		float d3 = tmp[24 + col] - tmp[32 + col];

		e->block.dct[col] = coeff[3] * (s0 + s1 + s2 + s3);
		e->block.dct[8 + col] = coeff[0] * d0 + coeff[2] * d1 + coeff[4] * d2 + coeff[6] * d3;
		e->block.dct[16 + col] = coeff[1] * (s0 - s3) + coeff[5] * (s1 - s2);
		e->block.dct[24 + col] = coeff[2] * d0 - coeff[6] * d1 - coeff[0] * d2 - coeff[4] * d3;
		e->block.dct[32 + col] = coeff[3] * (s0 - s1 - s2 + s3);
		e->block.dct[40 + col] = coeff[4] * d0 - coeff[0] * d1 + coeff[6] * d2 + coeff[2] * d3;
		e->block.dct[48 + col] = coeff[5] * (s0 - s3) + coeff[1] * (s2 - s1);
		e->block.dct[56 + col] = coeff[6] * d0 - coeff[4] * d1 + coeff[2] * d2 - coeff[0] * d3;
	}
#undef JPEC_BLOCK
}

static void jpec_enc_block_quant(jpec_enc_t* e) {
	assert(e && e->bnum >= 0);
	for (int i = 0; i < 64; i++) {
		e->block.quant[i] = (int)(e->block.dct[i] / e->dqt[i]);
	}
}

static void jpec_enc_block_zz(jpec_enc_t* e) {
	assert(e && e->bnum >= 0);
	e->block.len = 0;
	for (int i = 0; i < 64; i++) {
		if ((e->block.zz[i] = e->block.quant[jpec_zz[i]])) e->block.len = i + 1;
	}
}

/*-----------------end encode--------------------*/
