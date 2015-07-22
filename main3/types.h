#pragma once

#include "Arduino.h"
#include "error.h"

// basic types we need

struct vec2 {
  int x, y;
};

struct vec3 {
  int x, y, z;
};

inline vec2 v2(int x, int y){
  vec2 v = { x, y };
  return v;
}

inline vec3 v3(int x, int y, int z){
  vec3 v = { x, y, z };
  return v;
}

//
// reading utility functions
//
boolean isBreak(char c){
  return c == ',' || c == ';' || c == '\n';
}

int readInt(Stream& input = Serial){
  int val = 0;
  int sign = 1;
  boolean first = true;
  while(input.available()){
    char c = input.read();
    int d = (int)(c - '0');
    if(isBreak(c)){
      break;
    } else if(c == ' '){
      if(first) continue;
      else break;
    } else if(val == 0 && c == '-'){
      sign = -1;
    } else if(d >= 0 && d < 10) {
      val *= 10;
      val += d;
    } else {
      error = ERR_PARSE;
      return val;
    }
    first = false;
  }
  return sign * val;
}

unsigned long readULong(Stream &input = Serial){
  unsigned long val = 0L;
  boolean first = true;
  while(input.available()){
    char c = input.read();
    if(isBreak(c)){
      break;
    } else if(c == ' '){
      if(first) continue;
      else break;
    }
    unsigned long d = (unsigned long)(c - '0');
    if(d < 0 || d > 9){
      error = ERR_PARSE;
      return val;
    }
    val *= 10L;
    val += d;
    first = false;
  }
  return val;
}

long readLong(Stream& input = Serial){
  long val = 0L;
  long sign = 1L;
  boolean first = true;
  while(input.available()){
    char c = input.read();
    if(isBreak(c)){
      break;
    } else if(c == ' '){
      if(first) continue;
      else break;
    }
    long d = (long)(c - '0');
    if(val == 0L && c == '-'){
      sign = -1L;
    } else if(d < 0 || d > 9){
      error = ERR_PARSE;
      return val;
    } else {
      val *= 10L;
      val += d;
    }
    first = false;
  }
  return sign * val;
}

