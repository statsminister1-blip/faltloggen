// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <algorithm>
typedef bool boolean;
typedef uint8_t byte;
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define pgm_read_word(a) (*(const uint16_t*)(a))
#define pgm_read_dword(a) (*(const uint32_t*)(a))
#define pgm_read_pointer(a) (*(void* const*)(a))
#define F(x) x
#define DEC 10
#define HEX 16
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define INPUT 0
using std::min; using std::max;
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
inline void delay(unsigned long){}
inline unsigned long millis(){return 0;}
inline void pinMode(int,int){} inline void digitalWrite(int,int){}
class __FlashStringHelper;
#include <string>
class String : public std::string { public: using std::string::string; String(const std::string&s):std::string(s){} String(){} unsigned int length() const {return size();} };
#include "Print.h"
