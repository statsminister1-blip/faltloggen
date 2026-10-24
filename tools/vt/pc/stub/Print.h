// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
class Print { public:
  virtual size_t write(uint8_t)=0;
  virtual size_t write(const uint8_t*b,size_t n){size_t r=0;while(n--)r+=write(*b++);return r;}
  size_t write(const char*s){return write((const uint8_t*)s,strlen(s));}
  size_t print(const char*s){return write(s);}
  size_t print(char c){return write((uint8_t)c);}
  size_t print(int v){char b[16];snprintf(b,16,"%d",v);return write(b);}
  size_t println(const char*s){return print(s)+write('\n');}
  virtual ~Print(){}
};
