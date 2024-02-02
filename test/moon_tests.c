/**********************************************************************
 * Copyright (c) 2023 qlpqlp (twitter.com/inevitable360)              *
 * Copyright (c) 2022-2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dogecoin/moon.h>

void test_moon()
{
    /* initialize testing */
    char *m = moon();
    assert(strcmp(m, "")!=0);
}
