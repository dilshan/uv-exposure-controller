//-----------------------------------------------------------------------
// Copyright 2020 Dilshan R Jayakody [jayakody2000lk@gmail.com]
//
// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files 
// (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, 
// publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------

#ifndef _EXSOUURE_CONTROLLER_MAIN_HEADER_
#define _EXSOUURE_CONTROLLER_MAIN_HEADER_

#include <inttypes.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#define MINIMUM_TIME    5
#define MAXIMUM_TIME    999

uint8_t getDigit(uint8_t value, uint8_t showZero);
void setDigitValue(uint8_t value);
void setNumber();
uint8_t getDigitMode(uint8_t pos);

uint8_t extractDigit();
void updateDigit(uint8_t newValue);

#endif /* _EXSOUURE_CONTROLLER_MAIN_HEADER_ */