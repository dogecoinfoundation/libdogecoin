/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr
 Copyright (c) 2023 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_VERSION_H__
#define __LIBDOGECOIN_VERSION_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#if defined(HAVE_CONFIG_H)
#include <config/libdogecoin-config.h>
#endif // HAVE_CONFIG_H

//! These need to be macros, as version.c's voodoo requires it
#define _PKG_VERSION_MAJOR 0
#define _PKG_VERSION_MINOR 1
#define _PKG_VERSION_BUILD 3
#define _LIB_VERSION_REVISION 0

static const int CLIENT_VERSION =
                           1000000 * _PKG_VERSION_MAJOR
                         +   10000 * _PKG_VERSION_MINOR
                         +     100 * _PKG_VERSION_BUILD
                         +       1 * _LIB_VERSION_REVISION;

#endif // __LIBDOGECOIN_VERSION_H__
