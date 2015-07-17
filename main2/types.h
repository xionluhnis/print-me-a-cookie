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
int readInt(){
  int val = 0;
  int sign = 1;
  while(Serial.available()){
    char c = Serial.read();
    int d = (int)(c - '0');
    if(val == 0 && c == '-'){
      sign = -1;
    } else if(d >= 0 && d < 10) {
      val *= 10;
      val += d;
    } else {
      error = ERR_PARSE;
      return val;
    }
  }
  return sign * val;
}

unsigned long readULong(){
  unsigned long val = 0L;
  while(Serial.available()){
    char c = Serial.read();
    if(c == ' '){
      break;
    }
    unsigned long d = (unsigned long)(c - '0');
    if(d < 0 || d > 9){
      error = ERR_PARSE;
      return val;
    }
    val *= 10L;
    val += d;
  }
  return val;
}

long readLong(){
  long val = 0L;
  long sign = 1L;
  while(Serial.available()){
    char c = Serial.read();
    if(c == ' '){
      break;
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
  }
  return sign * val;
}

