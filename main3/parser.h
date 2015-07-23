#pragma once

#include "Arduino.h"
#include "error.h"

bool isBreak(char c){
  return c == ',' || c == ';';
}

bool isBlankSpace(char c){
  return c == ' ' || c == '\t';
}

class LineParser {
public:

  LineParser(Stream &s, LineParser *p = NULL) : input(&s), last('\0'), valid(true), parent(p) {}

  LineParser subline() {
    return !valid ? *this : LineParser(*input, this);
  }

  bool available(){
    return input->available() && valid;
  }

  void skip(bool echo = false){
    if(!valid) return;
    while(read()){
      if(echo)
        Serial.print(last);
    }
  }

  char readChar(){
    if(valid && read()){
      return last;
    }
    return '\0';
  }

  int readInt(){
    if(!valid)
      return 0;

    // read sequence and sign
    int val = 0;
    int sign = 1;
    bool first = true;
    while(read()){
      char c = last;
      int d = (int)(c - '0');
      if(isBreak(c)){
        break;
      } else if(isBlankSpace(c)){
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

  unsigned long readULong(){
    if(!valid)
      return 0L;
      
    unsigned long val = 0L;
    bool first = true;
    while(read()){
      char c = last;
      if(isBreak(c)){
        break;
      } else if(isBlankSpace(c)){
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

  long readLong(){
    if(!valid)
      return 0L;
      
    long val = 0L;
    long sign = 1L;
    bool first = true;
    while(read()){
      char c = last;
      if(isBreak(c)){
        break;
      } else if(isBlankSpace(c)){
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

protected:
  bool read(){
    if(input->available()){
      last = input->read();
      if(last == '\n' || last == '\r'){
        valid = false;
        if(parent)
          parent->valid = false;
      }
      if(parent && isBreak(last))
        valid = false;
      return valid;
    }
    return false;
  }

private:
  Stream *input;
  char last;
  bool valid;
  LineParser *parent;
};

