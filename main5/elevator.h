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
		// were we moving?
		if(lastTarget == currTarget){
			stpZ->moveToFreq(Stepper::IDLE_FREQ);
			return;
		}
		// are we done moving?
		long currDelta = stpZ->value() - currTarget;
		long fullDelta = lastTarget - currTarget;
		if(currDelta * fullDelta <= 0L
		|| std::abs(currDelta) < stpZ->stepSize()){
			stpZ->moveToFreq(Stepper::IDLE_FREQ);
			if(callback){
				callback(state);
			}
			lastTarget = currTarget;
		} else {
			// move at the best frequency (directly)
			stpZ->moveToFreq(f_best);
			stpZ->setSafeFreq(f_best);
			stpZ->setDeltaFreq(df_max);
		}
	}
	
	// --- setters ---------------------------------------------------------------
	void setTarget(long z){
		lastTarget = currTarget;
		currTarget = z;
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
	}
	
	// --- getters ---------------------------------------------------------------
	long target() const {
		return currTarget;
	}

public:
  void debug() {
    arduino::printf("debug(h): f_best=%d, df_max=%d\n", f_best, df_max);
    arduino::printf("position: lastTarget=%d, currTarget=%d\n", lastTarget, currTarget);
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
};
