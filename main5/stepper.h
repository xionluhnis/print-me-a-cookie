#pragma once

#include "Arduino.h"
#include "utils.h"

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
  		case MS_1_1 : return 16L;
  		case MS_1_2 : return 8L;
  		case MS_1_4 : return 4L;
  		case MS_1_8 : return 2L;
  		case MS_1_16: return 1L;
  		default:
  			error = ERR_INVALID_MS_MODE;
  			return 0L;
  	}
  }
  
  // exceptional idle frequency case
  static const long IDLE_FREQ = 0L;

  Stepper(int s, int d, int m1, int m2, int m3, int e, char id = '?')
    : stp(s), dir(d), ms1(m1), ms2(m2), ms3(m3), en(e), ident(id) {
      enabled = false;
      // freq data
      count = 0L;
      f_cur = f_mem = 0L;
      f_trg = 0L;
      df = 1L;
      f_safe = 5L;
      // positioning
      steps = 0L;
      stepMode = MS_SLOW;
      stepDelta = stepsForMode(stepMode);
      stepDir = 1L; // = LOW
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
    df = 1L;
    f_safe = 5L;
    count = f_cur = f_trg = f_mem = 0L;
    digitalWrite(stp, LOW);
    digitalWrite(dir, LOW);
    microstep(MS_SLOW);
    disable();
  }

  void exec() {
    if(isFrozen()){
      // Serial.print("Frozen, awaken: ");
      // Serial.println(ident);
      triggerUpdate();
    }
  	if(isTriggering()){
      // arduino::printf("Trigger %c up\n", ident);
      enable();
  		digitalWrite(stp, HIGH);
  		// update position
  		steps += stepDir * stepDelta;
  	}
  }

  void release() {
  	if(isRunning()){
  		if(isTriggering()){
        // arduino::printf("Trigger %c down\n", ident);
  			digitalWrite(stp, LOW);
  			triggerUpdate();
  		}
  		++count;
      // Serial.print("running for count=");
      // Serial.println(count, DEC);
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
  
  // --- getters ---------------------------------------------------------------
  long targetFreq() const {
  	return f_trg;
  }
  long currentFreq() const {
  	return f_cur;
  }
  long value() const {
  	return steps;
  }
  unsigned long stepSize() const {
  	return stepDelta;
  }
  
  // --- estimators ------------------------------------------------------------
  unsigned long timeBetweenFreq(long f_c, long f_t, long df) const {
  	unsigned long t = 0L;
  	long f = f_c;
  	while(f != f_t){
  		t += std::abs(f);
  		f = updateFreq(f, f_t, df);
  	}
  	return t;
  }
  unsigned long timeToFreq(long f_t, long df) const {
  	unsigned long t = timeBetweenFreq(f_cur, f_t, df);
  	if(t)
  		return t + 1L - count; // account for current count
  	else
  		return 0L;
  }
  long valueAtFreq(long f_t, long df) const {
  	long d = steps;
  	long f = f_cur;
  	while(f != f_t){
  		d += sign(f) * stepDelta;
  		f = updateFreq(f, f_t, df);
  	}
  	return d;
  }
  long valueAtFreq(long f_t) const {
  	return valueAtFreq(f_t, df);
  }
  
  // --- checks ----------------------------------------------------------------
  bool isRunning() const {
    return f_trg != IDLE_FREQ || f_cur != IDLE_FREQ;
  }
  bool isEnabled() const {
    return enabled;
  }
  bool isSafeFreq(long f) const {
  	return f == IDLE_FREQ || std::abs(f) >= f_safe;
  }
  bool hasSafeFreq() const {
  	return isSafeFreq(f_cur);
  }
  bool hasCorrectDirection() const {
  	return f_cur * f_trg >= 0L;
  }

protected:

  void triggerUpdate() {
    count = 0L; // reset
    long f_tmp = f_cur;
    f_cur = updateFreq(f_cur, f_trg);
    // prevent oscillation
    if(f_cur != f_tmp && f_cur == f_mem){
      // revert change
      f_cur = f_tmp;
    } else {
      // remember change
      f_mem = f_tmp;
    }
    if(f_cur != f_tmp){
      Serial.print("freqUpdate(");
      Serial.print(ident);
      Serial.print("): ");
      Serial.println(f_cur, DEC);
    }
    // did we change direction?
    if(f_cur * stepDir < 0L){
      stepDir = sign(f_cur);
      // arduino::printf("Changing dir of '%c'.\n", ident);
      digitalWrite(dir, stepDir > 0L ? LOW : HIGH);
    }
  }
  
  bool isTriggering() const {
  	return f_cur && count >= std::abs(f_cur);
  }

  bool isFrozen() const {
    return !f_cur && f_trg;
  }
  
  long updateFreq(long f_c, long f_t, long df) const{
    if(ident == 'e'){
      // arduino::printf("updateFreq: f_c=%d, f_t=%d, df=%d\n", f_c, f_t, df);
    }
  	// update frequency only if needed
		if(f_c == f_t)
			return f_t;
			
		// safety targets
		bool safe_cur = isSafeFreq(f_c);
		bool safe_trg = isSafeFreq(f_t);

    if(ident == 'e'){
      // Serial.print("safe_cur="); Serial.println(safe_cur, DEC);
      // Serial.print("safe_trg="); Serial.println(safe_trg, DEC);
    }
		
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

    if(ident == 'e'){
      // Serial.print("f_c <- "); Serial.println(f_c, DEC);
    }
		return f_c;
  }
  
  long updateFreq(long f_c, long f_t) const {
  	return updateFreq(f_c, f_t, df);
  }

public:
  void debug() {
    Serial.print("debug("); Serial.print(ident); Serial.println("):");
    Serial.print("count  "); Serial.println(count, DEC);
    Serial.print("f_cur  "); Serial.println(f_cur, DEC);
    Serial.print("f_trg  "); Serial.println(f_trg, DEC);
    Serial.print("df     "); Serial.println(df, DEC);
    Serial.print("f_safe "); Serial.println(f_safe, DEC);
    Serial.print(ident); Serial.print(", "); Serial.print(steps, DEC); Serial.print(", "); Serial.print(stepDelta, DEC); Serial.print(", "); Serial.println(stepDir, DEC);
    //arduino::printf("position: stepMode=%d, steps=%d, stepDelta=%d, stepDir=%d\n",
    //                stepMode, steps, stepDelta, stepDir);
  }

private:
  // pins
  int stp, dir, ms1, ms2, ms3, en;

  // ident
  char ident;

  // movement information
  unsigned long count;
  long f_cur, f_trg, f_mem;
  // movement profile
  unsigned long df;  		// maximum absolute delta in frequency, only for f < f_safe
  unsigned long f_safe; // frequency above which a direct speed change is allowed
  
  // positioning information
  byte stepMode;  // step mode
  long steps;			// reference number of steps
  long stepDelta; // step size
  long stepDir;		// step direction

  // state
  bool enabled;
};
