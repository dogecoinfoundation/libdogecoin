/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David L. Whitehurst
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * conversion.c (source)
 * Reusable conversion (beyond casting) functions.
 *
 * author: David L. Whitehurst
 * date: June 8, 2018
 *
 * Find this code useful? Please donate:
 *  Bitcoin: 1Mxt427mTF3XGf8BiJ8HjkhbiSVvJbkDFY
 *
 */

#include "conversion.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * This function converts a null terminated hex string
 * to a pointer of unsigned character bytes
 */

unsigned char *hexstr_to_char(const char *hexstr) {

    size_t len = strlen(hexstr);
    size_t final_len = len / 2;

    unsigned char *chrs = (unsigned char *) malloc((final_len + 1) * sizeof(*chrs));

    size_t i, j;

    for (i = 0, j = 0; j < final_len; i += 2, j++)
        chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25;
    chrs[final_len] = '\0';

    return chrs;
}
