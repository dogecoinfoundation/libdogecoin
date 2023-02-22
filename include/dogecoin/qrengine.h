// QR Engine using Project Nanuki QR (MIT) and lodepng (ZLIB) for encoding.

// (c) 2023 michilumin
// (c) 2023 The Dogecoin Foundation

#ifndef __LIBDOGECOIN_QRENGINE_H__
#define __LIBDOGECOIN_QRENGINE_H__

#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>

// internal funcs not exported
void printQr(const uint8_t qrcode[]);
int outputQRStringFromQRBytes(const uint8_t* inQrBytes, char* outString);
int stringToQrArray(const char* inString, uint8_t* outQrBytes);

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API int qrgen_p2pkh_to_qr_string(const char* in_p2pkh, char* outString);
LIBDOGECOIN_API void qrgen_p2pkh_consoleprint_to_qr(const char* in_p2pkh);
LIBDOGECOIN_API int qrgen_p2pkh_to_qrbits(const char* in_p2pkh, uint8_t* outQrByteArray);
LIBDOGECOIN_API int qrgen_string_to_qr_pngfile(const char* outFilename, const char* inString, uint8_t sizeMultiplier);
LIBDOGECOIN_API int qrgen_string_to_qr_jpgfile(const char* outFilename, const char* inString, uint8_t sizeMultiplier);
LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_QRENGINE_H__
