// QR Engine using Project Nanuki QR (MIT) and lodepng (ZLIB) for PNG encoding and jpec (MIT).
// see qr.h, png.h and jpeg.h for other (c) and attribution notes


// (c) 2023 michilumin
// (c) 2023 The Dogecoin Foundation

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qr/qr.h>
#include <qr/png.h>
#include <qr/jpeg.h>
#include <dogecoin/qrengine.h>



//encode a string to QR byte array with med ECC
int stringToQrArray(const char* inString, uint8_t* outQrBytes)
{
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, outQrBytes, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
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
int outputQRStringFromQRBytes(const uint8_t* inQrBytes, char* outString)
{
    outString[0] = '\0'; //make sure it starts with a nul so strcat knows what to do 
    int size = qrcodegen_getSize(inQrBytes);
    int border = 4;
    const char* dark = "  ";
    const char* light = "##";

    for (int y = -border; y < size + border; y++) {
        for (int x = -border; x < size + border; x++) {
            if (qrcodegen_getModule(inQrBytes, x, y)) {
                strcat(outString, light);
            } else {
                strcat(outString, dark);
            }
        }
        strcat(outString, "\n");
    }
    strcat(outString,"\n");
    return 0;
}


//macro to glue together string-to-qr and qr-to-string.  String addr in, string QR out.
int qrgen_p2pkh_to_qr_string(const char* in_p2pkh, char* outString)
{   
    uint8_t temparray[qrcodegen_BUFFER_LEN_MAX];
    stringToQrArray(in_p2pkh, temparray);
    outputQRStringFromQRBytes(temparray, outString);
    return 0;
}

// For API: Return byte array of QR code "pixels" (byte map) - returns size (L or W) in pixels of QR.
int qrgen_p2pkh_to_qrbits(const char* in_p2pkh, uint8_t* outQrByteArray)
{       
    int size = qrcodegen_getSize(outQrByteArray);
    stringToQrArray(in_p2pkh,outQrByteArray);
    return size;
}

//Creates a QR from the incoming p2pkh and then prints to console.
void qrgen_p2pkh_consoleprint_to_qr(const char* in_p2pkh)
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

uint8_t* bytesToMono(uint8_t* qrBytes, size_t multiplier)
{
    size_t side = qrcodegen_getSize(qrBytes);
    size_t pixelRunLength = side * side;
    uint8_t darkPixel[3] = {0, 0, 0};
    uint8_t litePixel[3] = {255, 255, 255};
    int step = 1;

    uint8_t* outMono = (uint8_t*)calloc(pixelRunLength * step * multiplier * multiplier, sizeof(uint8_t));

    int iterator = 0;
    
    for (int y = 0; y < (int)side; y++) {
        for (int r = 0; r < (int)multiplier; ++r) {
            for (int x = 0; x < (int)side; x++) {
                for (int n = 0; n < (int)multiplier; ++n) {
                    if (qrcodegen_getModule(qrBytes, x, y)) {
                        memcpy(outMono + iterator, litePixel, step);
                        iterator = iterator + step;
                    } else {
                        memcpy(outMono + iterator, darkPixel, step);
                        iterator = iterator + step;
                    }
                }
            }
        }
    }
    return outMono;
}

// encode a string to PNG file with med ECC
int qrgen_string_to_qr_pngfile(const char * outFilename, const char* inString, uint8_t sizeMultiplier)
{
    if (sizeMultiplier<1) 
    {
        sizeMultiplier = 1;
    }
    unsigned char* png;
    size_t pngsize;
    unsigned width;
    unsigned height;

    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];

    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
   
    if (codeGenerated) 
    {
        unsigned tempSideDim = qrcodegen_getSize(qrcode);
        width = tempSideDim * sizeMultiplier;
        height = tempSideDim * sizeMultiplier;

        // bytes to rgb
        uint8_t* image = bytesToRgb(qrcode, sizeMultiplier);
      
        unsigned error = lodepng_encode24(&png, &pngsize, image, width, height);
        if (!error)    
        {
            lodepng_save_file(png, pngsize, outFilename);
            free(png);
            free(image);
            return (int)pngsize;
        }
       else 
        {
            printf("png error %u: %s\n", error, lodepng_error_text(error));
            free(png);
            free(image);
            return -1;
        }
    } 
    else 
    {
        printf("Error generating QR\n");
        return -1;
    }
}


// encode a string to JPEG file with med ECC
int qrgen_string_to_qr_jpgfile(const char* outFilename, const char* inString, uint8_t sizeMultiplier)
{
    if (sizeMultiplier < 1) 
    {
        sizeMultiplier = 1;
    }

    const uint8_t* jpg;
    int jpgsize;
    unsigned width;
    unsigned height;

    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];

    bool codeGenerated = qrcodegen_encodeText(inString, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    if (codeGenerated) 
    {
        unsigned tempSideDim = qrcodegen_getSize(qrcode);
        width = tempSideDim * sizeMultiplier;
        height = tempSideDim * sizeMultiplier;

        // bytes to monobitmap with sizemodifier
        uint8_t* image = bytesToMono(qrcode, sizeMultiplier);

        //encode jpeg
        jpec_enc_t* enc = jpec_enc_new(image, width, height);
        jpg = jpec_enc_run(enc, &jpgsize);

        FILE* file = fopen(outFilename, "wb");
        fwrite(jpg, sizeof(uint8_t), jpgsize, file);
        fclose(file);
        jpec_enc_del(enc);
        free(image);
        return jpgsize;
    } 
    else 
    {
        printf("Error generating QR\n");
        return -1;
    }
}
