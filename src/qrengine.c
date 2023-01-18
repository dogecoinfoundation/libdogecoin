// QR Engine using Project Nanuki QR (MIT) and lodepng (ZLIB) for encoding.

// (c) 2023 michilumin
// (c) 2023 The Dogecoin Foundation

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qr\qr.h"
#include <dogecoin\qrengine.h>



//encode a string to QR byte array with high ECC
int stringToQrArray(const char* inString,uint8_t* qrcode)
{
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, qrcode, qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (codeGenerated) 
    {
        //return 0 on pass/OK.
        return 0;
    } 
    else 
    {
        printf("Error generating QR\n"); 
        return 1;
    }
      
}

//take in a QR byte array and output a formatted string with line breaks ready for console printing
int outputQRStringFromQRBytes(uint8_t qrcode[],char* outstring)
{
    outstring[0] = '\0'; //make sure it starts with a nul so strcat knows what to do 
    int size = qrcodegen_getSize(qrcode);
    int border = 4;
    const char* dark = "  ";
    const char* light = "##";

    for (int y = -border; y < size + border; y++) {
        for (int x = -border; x < size + border; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                strcat(outstring, light);
            } else {
                strcat(outstring, dark);
            }
        }
        strcat(outstring, "\n");
    }
    strcat(outstring,"\n");
    return 0;
}


//macro to glue together string-to-qr and qr-to-string.  String addr in, string QR out.
int qrgen_p2pkh_to_qr_string(const char* in_p2pkh,char* outqr)
{   
    uint8_t temparray[qrcodegen_BUFFER_LEN_MAX];
    stringToQrArray(in_p2pkh, temparray);
    outputQRStringFromQRBytes(temparray, outqr);
}

// For API: Return byte array of QR code "pixels" (byte map) - returns size (L or W) in pixels of QR.
int qrgen_p2pkh_to_qrbits(const char* in_p2pkh,uint8_t* qrbits)
{       
    int size = qrcodegen_getSize(qrbits);
    stringToQrArray(in_p2pkh,qrbits);
    return size;
}

//Creates a QR from the incoming p2pkh and then prints to console.
void qrgen_p2pkh_consoleprint_to_qr(char* in_p2pkh)
{
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    stringToQrArray(in_p2pkh, qrcode);
    printQr(qrcode);
}

//Internal: print a QR byte array to the console in scannable ASCII text.
static void printQr(const uint8_t qrcode[])
{
    int size = qrcodegen_getSize(qrcode);

    int border = 4;
    const char* dark = "  ";
    const char* light = "##";

    for (int y = -border; y < size + border; y++) {
        for (int x = -border; x < size + border; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                printf("%s", light);
            } else {
                printf("%s", dark);
            }
        }
        printf("\n");
    }
    printf("\n");
}


