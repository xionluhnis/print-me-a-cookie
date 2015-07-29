#pragma once

#include "Arduino.h"
#include <cmath>

long sign(long l){
	return l < 0 ? -1 : 1;
}

class Stepper {
public:

  typedef void (*Callback)(int state);

  // Microstep modes
  static const byte MS_FULL = B000;
  static const byte MS_1_1  = B000;
  static const byte MS_1_2  = B100;
  static const byte MS_1_4  = B010;
  static const byte MS_1_8  = B110;
  static const byte MS_1_16 = B111;
  static const byte MS_SLOW = B111; 
  
  // convert microstep mode into number of base steps
  static long stepsForMode(byte msMode){
  	switch(msMode){
  		case MS_FULL:
  		case MS_1_1 : return 16L;
  		case MS_1_2 : return 8L;
  		case MS_1_4 : return 4L;
  		case MS_1_8 : return 2L;
  		case MS_1_16:
  		case MS_SLOW: return 1L;
  		default:
  			error = ERR_INVALID_MS_MODE;
  			return 0L;
  	}
  }
  
  // exceptional idle frequency case
  static const long IDLE_FREQ = 0L;

  Stepper(int s, int d, int m1, int m2, int m3, int e)
    : stp(s), dir(d), ms1(m1), ms2(m2), ms3(m3), en(e) {
      enabled = false;
      // freq data
      count = 0L;
      f_cur = 0L;
      f_trg = 0L;
      df = 1L;
      f_safe = 100L;
      // positioning
      steps = 0L;
      stepMode = MS_SLOW;
      stepDelta = stepsForMode(stepMode);
      stepDir = 1L; // = LOW
      // cb
      callback = NULL;
      state = 0L;
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
    enable();
    f_c = f_t = 0L;
    enable();
    digitalWrite(stp, LOW);
    digitalWrite(dir, LOW);
    microstep(MS_SLOW);
    disable();
    callback = NULL;
  }

  void exec(){
  	if(isTriggering()){
  		digitalWrite(stp, HIGH);
  		// update position
  		steps += stepDir * stepDelta;
  	}
  }

  void release() {
  	if(isRunning()){
  		if(isTriggering()){
  			digitalWrite(stp, LOW);
  			count = 0L; // reset
  			f_cur = updateFreq(f_cur, f_trg);
  			// did we change direction?
  			if(f_cur * stepDir < 0L){
  				stepDir = sign(f_cur);
  				digitalWrite(stepDir > 0L ? LOW : HIGH);
  			}
  		}
  		++count;
  	}
  }
  
  void enable(){
    if(!enabled){
      digitalWrite(en, LOW);
      enabled = true;
      Serial.println("enable");
    }
  }

  void disable(){
    if(enabled && !isRunning()){
      digitalWrite(en, HIGH);
      enabled = false;
      Serial.println("disable");
    }
  }

  void microstep(byte mode = MS_SLOW, bool forceDisable = false) {
    // Serial.println("Microstep");
    enable();
    stepMode = mode;
    stepDelta = stepsForMode(mode);
    int  ms[] = { ms1, ms2, ms3 };
    byte mask[] = { B100, B010, B001 };
    for(int i = 0; i < 3; ++i){
      digitalWrite(ms[i], mask[i] & mode ? HIGH : LOW);
    }
    if(forceDisable)
      disable();
  }
  
  // --- setters ---------------------------------------------------------------
  void moveToFreq(long f = IDLE_FREQ){
  	f_trg = f; // this is our new target
  }
  void resetPosition(long absoluteSteps = 0){
  	steps = absoluteSteps;
  }
  void setDeltaFreq(unsigned long deltaF = 1L){
  	df = deltaF;
  }
  void setSafeFreq(unsigned long f0 = 100L){
  	f_safe = f0;
  }
  void setCallback(Callback cb, int s0){
    callback = cb;
    state = s0;
  }
  
  // --- getters ---------------------------------------------------------------
  long targetFreq(){
  	return f_trg;
  }
  long currentFreq(){
  	return f_cur;
  }
  long value(){
  	return steps;
  }
  
  // --- estimators ------------------------------------------------------------
  unsigned long timeBetweenFreq(long f_c, long f_t, long df){
  	unsigned long t = 0L;
  	long f = f_c;
  	while(f != f_t){
  		t += std::abs(f);
  		f = updateFreq(f, f_t, df);
  	}
  	return t;
  }
  unsigned long timeToFreq(long f_t, long df){
  	unsigned long t = timeBetweenFreq(f_cur, f_t, df);
  	if(t)
  		return t + 1L - count; // account for current count
  	else
  		return 0L;
  }
  long valueAtFreq(long f_t, long df){
  	long d = steps;
  	long f = f_cur;
  	while(f != f_trg){
  		d += sign(f) * stepDelta;
  		f = updateFreq(f, f_t, df);
  	}
  	return d;
  }
  
  // --- checks ----------------------------------------------------------------
  bool isRunning(){
    return f_t != IDLE_FREQ && f_c == IDLE_FREQ;
  }
  bool isEnabled(){
    return enabled;
  }
  bool isSafeFreq(long f){
  	return f == IDLE_FREQ || std::abs(f) >= f_safe;
  }
  bool hasSafeFreq(){
  	return isSafeFreq(f_cur);
  }
  bool hasCorrectDirection(){
  	return f_cur * f_trg >= 0L;
  }

protected:
  
  bool isTriggering(){
  	return f_cur && count >= std::abs(f_cur);
  }
  
  long updateFreq(long f_c, long f_t, long df){
  	// update frequency only if needed
		if(f_c == f_t)
			return f_t;
			
		// safety targets
		bool safe_cur = isSafeFreq(f_c);
		bool safe_trg = isSafeFreq(f_t);
		
		// are both safe frequencies?
		if(safe_cur && safe_trg){
			f_c = f_t; // safe to change directly
		} else
		
		// are both frequencies on the same direction?
		if(f_c * f_t > 0){
			// get closer
			long s0 = sign(f_t - f_c);
			f_c += s0 * df;
			if(sign(f_t - f_c) != s0){
				// we reach the target (or went past)
				f_c = f_t;
			}
		} else
		
		// we must change direction first, but is it safe?
		if(safe_cur){
			f_c = f_safe * sign(f_t);
		} else
		
		// we must go to a safe frequency before changing direction
		{
			f_c += sign(f_c) * df;
		}
		return f_c;
  }
  
  long updateFreq(long f_c, long f_t){
  	return updateFreq(f_c, f_t, df);
  }

private:
  // pins
  int stp, dir, ms1, ms2, ms3, en;

  // movement information
  unsigned long count;
  long f_cur, f_trg;
  // movement profile
  unsigned long df;  		// maximum absolute delta in frequency, only for f < f_safe
  unsigned long f_safe; // frequency above which a direct speed change is allowed
  
  // positioning information
  byte stepMode;  // step mode
  long steps;			// reference number of steps
  long stepDelta; // step size
  long stepDir;		// step direction

  // callback
  Callback callback;
  int state;

  // state
  bool enabled;
};
