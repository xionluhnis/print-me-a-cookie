#pragma once

class Stepper {
public:

  typedef void (*Callback)(int state);

  public static const int STEP = 0;
  public static const int DIRECTION = 1;
  public static const int MICROSTEP = 2;
  public static const int ENABLE = 3;

  Stepper(int s, int d, int m1, int m2, int m3, int e)
    : stp(s), dir(d), ms1(m1), ms2(m2), ms3(m3), en(e) {
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
    digitalWrite(ms1, LOW);
    digitalWrite(ms2, LOW);
    digitalWrite(ms3, LOW);
    digitalWrite(en, HIGH); // lock
    callback = NULL;
  }

  void exec(){
    if(moves > 0){
      digitalWrite(en, LOW);
      if(count == 0){
        digitalWrite(stp, HIGH);
      }
    }
  }

  void release() {
    if(moves > 0){
      if(count == 0){
        digitalWrite(stp, LOW);
        digitalWrite(en, HIGH); // lock
        count = steps;
        --moves;
        // callback if there's one
        if(callback != NULL){
          callback(state);
        }
      } else {
        --count;
      }
    }
  }

  void microstep(int m) {
    digitalWrite(en, LOW);
    digitalWrite(ms1, m);
    digitalWrite(ms2, m);
    digitalWrite(ms3, m);
    digitalWrite(en, HIGH);
  }

  int isRunning(){
    return moves > 0;
  }

  void setCallback(Callback cb, int s0){
    callback = cb;
    state = s0;
  }

  void stepBy(long delta, unsigned long speed = 1L, unsigned long init = 0L, Callback cb = NULL, int state0 = 0){
    digitalWrite(en, LOW);
    if(delta > 0){
      moves = (unsigned long)delta;
      digitalWrite(dir, LOW); // forward
    } else {
      moves = (unsigned long)(-delta);
      digitalWrite(dir, HIGH); // backward
    }
    speed = steps;
    count = init;
    callback = cb;
    state = state0;
  }

private:
  // pins
  int stp, dir, ms1, ms2, ms3, en;

  // movement information
  unsigned long moves, steps, count;

  // callback
  Callback callback;
  int state;
}
