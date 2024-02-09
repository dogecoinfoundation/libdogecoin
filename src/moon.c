/*

 The MIT License (MIT)

 Copyright (c) 2023 qlpqlp (twitter.com/inevitable360)
 Copyright (c) 2022-2024 The Dogecoin Foundation

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

#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * @brief Calling the moon, it returns the current moon phase
 * @return moon icon
 */

char* moon()
{
  int D,c,e;
  double jd;                      // Julian days

  time_t t = time(NULL);          // get time
  struct tm tm = *localtime(&t);  // get localtime
  int y = tm.tm_year + 1900;      // year, because its only above 1900, we add 1900 to the year
  int m = tm.tm_mon;              // month
  int d = tm.tm_mday - 2;         // day, because it can show the moon with 2 days in advance, we subtract

  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;

  c = 365.25*y;                   // 365.25 total days in a year
  e = 30.6*m;
  jd = c+e+d-694039.09;           // Julian days
  jd /= 29.53;                    // moons cycle 29.53 days
  D = jd;	                        // convert decimal to int 	   
  jd -= D;                        // get negative of Julian days
  D = jd*8 + 1.5;	                // get fraction from 0-8 by rounding 0.5 to it               

  switch (D & 7) {                // because the new moon = 0 is the same on 8 we convert to 0
    case 0:
      return "🌑";               // New Moon
    case 1:
      return "🌒";               // Waxing Crescent
    case 2:
      return "🌓";               // First Quarter Moon
    case 3:
      return "🌔";               // Waxing Gibbous
    case 4:
      return "🌕";               // Full Moon
    case 5:
      return "🌖";               // Waning Gibbous
    case 6:
      return "🌗";               // Third Quarter
    case 7:
      return "🌘";               // Waning Crescent
  }
  return "";
}
