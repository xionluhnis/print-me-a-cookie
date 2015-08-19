#pragma once

#include "Arduino.h"
#include "utils.h"
#include "stepper.h"

class Elevator {
public:

	typedef void (*Callback)(int state);

	explicit Elevator(Stepper *z) : stpZ(z) {
		reset();
	}
	
	void update(){
    if(!enabled) return;
		// were we moving?
		if(!hasTarget()){
      // Serial.println("No Z target");
			stpZ->moveToFreq(Stepper::IDLE_FREQ);
			return;
		}
		// are we done moving?
		if(hasReachedTarget()){
      // Serial.println("Reached H target");
			stpZ->moveToFreq(Stepper::IDLE_FREQ);
			if(callback){
				callback(state);
			}
			lastTarget = currTarget;
		} else {
			// move at the best frequency (directly)
			stpZ->moveToFreq(bestFreq(realDelta()));
			stpZ->setSafeFreq(f_best);
			stpZ->setDeltaFreq(df_max);
      // Serial.print("Freq: "); Serial.println(stpZ->targetFreq());
		}
	}

  long bestFreq(long delta) const {
    return sign(delta) * f_best;
  }
	
	// --- setters ---------------------------------------------------------------
	void setTarget(long z){
		lastTarget = currTarget;
		currTarget = z;
    //Serial.print("New targets: ");
    //Serial.print(lastTarget); Serial.print(" -> "); Serial.println(currTarget);
    //Serial.print("Current: "); Serial.println(stpZ->value());
	}
	void setBestFreq(unsigned long f){
		if(f)
			f_best = f;
	}
	void setMaxDeltaFreq(unsigned long df){
		if(df)
			df_max = df;
	}
	void setCallback(Callback cb){
		callback = cb;
	}
	void setState(int s0){
		state = s0;
	}
	void reset(){
		f_best = 5L;
		df_max = 42L;
		lastTarget = currTarget = stpZ->value();
		callback = NULL;
		state = 0;
    enabled = true;
	}
  void resetZ(long z){
    stpZ->resetPosition(z);
  }
  void toggle(){
    enabled = !enabled;
  }
  void enable(){
    enabled = true;
  }
  void disable(){
    enabled = true;
  }
	
	// --- getters ---------------------------------------------------------------
	long target() const {
		return currTarget;
	}
  bool isEnabled() const {
    return enabled;
  }
  long realDelta() const {
    return currTarget - stpZ->value();
  }
  long currDelta() const {
    return currTarget - lastTarget;
  }

  // --- checks ---
  bool hasTarget() const {
    return lastTarget != currTarget || !hasReachedTarget();
  }
  bool hasReachedTarget() const {
    long currDelta = stpZ->value() - currTarget;
    long fullDelta = lastTarget - currTarget;
    return currDelta * fullDelta < 0L || std::abs(currDelta) < stpZ->stepSize();
  }

public:
  void debug() {
    Serial.println("debug(h):");
    Serial.print("f_best "); Serial.println(f_best, DEC);
    Serial.print("df_max "); Serial.println(df_max, DEC);
    Serial.print("lastTg "); Serial.println(lastTarget, DEC);
    Serial.print("currTg "); Serial.println(currTarget, DEC);
  }

private:
	Stepper *stpZ;
	unsigned long f_best, df_max;
	
	// xy target data
	long lastTarget;
	long currTarget;
	
	// callback
	Callback callback;
	int state;

  // state
  bool enabled;
};


