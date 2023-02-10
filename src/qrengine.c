// QR Engine using Project Nanuki QR (MIT) and lodepng (ZLIB) for encoding.

// (c) 2023 michilumin
// (c) 2023 The Dogecoin Foundation

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qr/qr.h>
#include <qr/png.h>
#include <dogecoin/qrengine.h>



//encode a string to QR byte array with med ECC
int stringToQrArray(const char* inString,uint8_t* qrcode)
{
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
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
int outputQRStringFromQRBytes(const uint8_t qrcode[],char* outstring)
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
    return outputQRStringFromQRBytes(temparray, outqr);
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
void printQr(const uint8_t qrcode[])
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

//internal conversion for PNG

uint8_t* bytesToRgb(uint8_t* qrBytes, size_t multiplier)
{
    size_t side = qrcodegen_getSize(qrBytes);
    size_t pixelRunLength = side * side;
    uint8_t darkPixel[3] = {0, 0, 0};
    uint8_t litePixel[3] = {255, 255, 255};
    // int border = 4; //unused variable
    int step = 3;

    uint8_t* outRgb = (uint8_t*)calloc(pixelRunLength * step * multiplier * multiplier, sizeof(uint8_t));

    int iterator = 0;

    for (int y = 0; y < (int)side; y++) {
        for (int r = 0; r < (int)multiplier; ++r) {
            for (int x = 0; x < (int)side; x++) {
                for (int n = 0; n < (int)multiplier; ++n) {
                    if (qrcodegen_getModule(qrBytes, x, y)) {
                        memcpy(outRgb + iterator, litePixel, step);
                        iterator = iterator + step;
                    } else {
                        memcpy(outRgb + iterator, darkPixel, step);
                        iterator = iterator + step;
                    }
                }
            }
        }
    }

    // should have outRgb now... sized correctly
    return outRgb;
}

// encode a string to PNG file with med ECC
int qrgen_string_to_qr_pngfile(const char *filename, const char* inString, uint8_t multiplier)
{
    if (multiplier<1) 
    {
        multiplier = 1;
    }
    unsigned char* png;
    size_t pngsize;
    unsigned width;
    unsigned height;


    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];

    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
   
    if (codeGenerated) {
        unsigned tempSideDim = qrcodegen_getSize(qrcode);
        width = tempSideDim * multiplier;
        height = tempSideDim * multiplier;

        // bytes to rgb
        uint8_t* image = malloc(width * height);
        image = bytesToRgb(qrcode, multiplier);

        unsigned error = lodepng_encode24(&png, &pngsize, image, width, height);
        if (!error)    
        {
            lodepng_save_file(png, pngsize, filename);
            free(png);
            return 0;
        }
       else 
        {
            printf("png error %u: %s\n", error, lodepng_error_text(error));
            free(png);
            return 1;
        }
    } 
    else 
    {
        printf("Error generating QR\n");
        return 1;
    }
}