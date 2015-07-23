#pragma once

#include "Arduino.h"

class Stepper {
public:

  typedef void (*Callback)(int state);

  static const int STEP = 0;
  static const int DIRECTION = 1;
  static const int MICROSTEP = 2;
  static const int ENABLE = 3;

  // Microstep modes
  static const byte MS_FULL = B000;
  static const byte MS_1_1  = B000;
  static const byte MS_1_2  = B100;
  static const byte MS_1_4  = B010;
  static const byte MS_1_8  = B110;
  static const byte MS_1_16 = B111;

  Stepper(int s, int d, int m1, int m2, int m3, int e)
    : stp(s), dir(d), ms1(m1), ms2(m2), ms3(m3), en(e) {
      enabled = false;
  }
  void setup() {
    pinMode(stp, OUTPUT);
    pinMode(dir, OUTPUT);
    pinMode(ms1, OUTPUT);
    pinMode(ms2, OUTPUT);
    pinMode(ms3, OUTPUT);
    pinMode(en, OUTPUT);
    reset();
  }

  void reset() {
    moves = 0;
    steps = 1;
    count = 0;
    digitalWrite(en, LOW);  // unlock
    digitalWrite(stp, LOW);
    digitalWrite(dir, LOW);
    digitalWrite(ms1, HIGH);
    digitalWrite(ms2, HIGH);
    digitalWrite(ms3, HIGH);
    digitalWrite(en, HIGH); // lock
    callback = NULL;
  }

  void exec(){
    if(moves > 0){
      // Serial.println("exec");
      if(count == 0){
        // Serial.println("step");
        digitalWrite(stp, HIGH);
      }
    }
  }

  void release() {
    if(moves > 0){
      // Serial.println("release");
      if(count == 0){
        digitalWrite(stp, LOW);
        count = steps;
        --moves;
      } else {
        --count;
      }
      if(moves == 0){
        disable();
        // callback if there's one
        if(callback != NULL){
          callback(state);
        }
      }
    }
  }

  void microstep(byte mode = MS_FULL) {
    // Serial.println("Microstep");
    enable();
    int  ms[] = { ms1, ms2, ms3 };
    byte mask[] = { B100, B010, B001 };
    for(int i = 0; i < 3; ++i){
      digitalWrite(ms[i], mask[i] & mode ? HIGH : LOW);
    }
    digitalWrite(en, HIGH);
    disable();
  }

  boolean isRunning(){
    return moves > 0;
  }

  boolean isEnabled(){
    return enabled;
  }

  void setCallback(Callback cb, int s0){
    callback = cb;
    state = s0;
  }

  void stepBy(long delta, unsigned long speed = 1L, unsigned long init = 0L, Callback cb = NULL, int state0 = 0){
    Serial.print("stepBy ");
    Serial.print(delta);
    Serial.print(" ");
    Serial.println(speed);
    enable();
    if(delta > 0){
      moves = (unsigned long)delta;
      digitalWrite(dir, LOW); // forward
    } else {
      moves = (unsigned long)(-delta);
      digitalWrite(dir, HIGH); // backward
    }
    steps = speed;
    count = init;
    callback = cb;
    state = state0;
  }

protected:
  void enable(){
    digitalWrite(en, LOW);
    enabled = true;
    //Serial.println("enable");
  }

  void disable(){
    digitalWrite(en, HIGH);
    enabled = false;
    //Serial.println("disable");
  }

private:
  // pins
  int stp, dir, ms1, ms2, ms3, en;

  // movement information
  unsigned long moves, steps, count;

  // callback
  Callback callback;
  int state;

  // state
  boolean enabled;
};
