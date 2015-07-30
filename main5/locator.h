#pragma once

#include "Arduino.h"
#include "utils.h"
#include "stepper.h"
#include "geom.h"

class Locator {
public:

	typedef void (*Callback)(int state);

	Locator(Stepper *x, Stepper *y) : stpX(x), stpY(y) {
		reset();
	}
	
	vec2 bestFreq(const vec2 &delta){
		vec2 abs = delta.abs();
		long d_max = abs.max();
		vec2 f;
		for(int i = 0; i < 2; ++i){
			if(abs[i] == d_max){
				f[i] = sign(delta[i]) * f_best;
			} else if(abs[i] == 0L) {
				f[i] = 0L;
			} else {
				f[i] = (long)std::round(f_best * d_max / float(delta[i]));
			}
		}
		return f;
	}
	
	void update(){
		// - should we be idle?
		if(!hasTarget() && isMoving()){
			for(int i = 0; i < 2; ++i){
				Stepper *stp = stepper(i);
				if(stp->targetFreq() != Stepper::IDLE_FREQ)
					stp->moveToFreq(Stepper::IDLE_FREQ);
			}
			return;
		}
		
		// target reach state
		bool reached = hasReachedTarget();
		
		// - did we reach the target
		if(reached){
			unsigned long lastID = targetID;
			// callback (mostly to get the new next target)
			if(callback){
				callback(state);
			}
			if(lastID == targetID){
				// shift targets since we have no new target
			  lastTarget = currTarget; // => hasTarget() == false
			}
		}
		
		// - should we stop at the target?
		if(isEnding()){
			long dx = stpX->valueAtFreq(Stepper::IDLE_FREQ),
					 dy = stpY->valueAtFreq(Stepper::IDLE_FREQ);
			// should we start slowing down?
			vec2 targetFreq;
			if(vec2(dx, dy).sqDistTo(realDelta()) <= epsilonSq){
				// it will take us enough time to stop
				// that we should start slowing down now!
				targetFreq = vec2(Stepper::IDLE_FREQ);
			} else {
				targetFreq = bestFreq(realDelta());
			}
			adjustToFreq(targetFreq);
		} else
		
		// - we must make our line as straight as possible
		{
			adjustToFreq(bestFreq(realDelta()));
		}
	}
	
	static unsigned long deltaTime(unsigned long t1, unsigned long t2){
		return std::max(t1, t2) - std::min(t1, t2); // won't underflow
	}
	
	void adjustToFreq(vec2 f_trg){
		unsigned long df[2] = { df_max, df_max };
		unsigned long t[2] = { stpX->timeToFreq(f_trg[0], df[0]), stpY->timeToFreq(f_trg[1], df[1]) };
		unsigned long dt = deltaTime(t[0], t[1]);
		// optimize for df and f_trg, so that abs(t[0] - t[1]) is lowest
		bool opt = true;
		unsigned long it = 0;
		while(opt && it < 1000){
			++it;
			opt = false; // by default, we stop optimizing if nothing changes
			
			// augmenting (first!)
			for(int i = 0; i < 2; ++i){
				if(df[i] < df_max){
					t[i] = stepper(i)->timeToFreq(f_trg[i], df[i] + 1);
					unsigned long dt2 = deltaTime(t[0], t[1]);
					if(dt2 < dt){
						df[i] += 1; // increment since it gets better
						dt = dt2;
						opt = true;
						break;
					}
				}
			}
			// bias towards increase in acceleration
			if(opt) continue;
			
			// reducing (if augmenting does not help)
			for(int i = 0; i < 2; ++i){
				if(df[i] > 1){
					t[i] = stepper(i)->timeToFreq(f_trg[i], df[i] - 1);
					unsigned long dt2 = deltaTime(t[0], t[1]);
					if(dt2 < dt){
						df[i] -= 1; // decrement since it gets better
						dt = dt2;
						opt = true;
						break;
					}
				}
			}
			
			// increasing the period globally
			if(f_trg.x != 0L && f_trg.y != 0L){
				vec2 f_new = f_trg * 2L;
				for(int i = 0; i < 2; ++i)
					t[i] = stepper(i)->timeToFreq(f_new[i], df[i]);
				unsigned long dt2 = deltaTime(t[0], t[1]);
				if(dt2 < dt){
					f_trg = f_new;
					dt = dt2;
					opt = true;
				}
			}
		}
		// done, we update the parameters of both stepper motors
		for(int i = 0; i < 2; ++i){
			stepper(i)->setDeltaFreq(df[i]);
			stepper(i)->moveToFreq(f_trg[i]);
		}
	}
	
	// --- setters ---------------------------------------------------------------
	void setTarget(const vec2 &trg, bool end = true){
		// shift targets
		lastTarget = currTarget;
		currTarget = trg;
		// ending state
		ending = end;
		// update target id
		++targetID;
	}
	void setBestFreq(unsigned long f){
		if(f)
			f_best = f;
	}
	void setMaxDeltaFreq(unsigned long df){
		if(df)
			df_max = df;
	}
	void setPrecision(unsigned long eps){
		if(eps)
			epsilonSq = eps * eps;
	}
	void setCallback(Callback cb){
		callback = cb;
	}
	void setState(int s0){
		state = s0;
	}
	void reset() {
		f_best = 5L;
		df_max = 24L;
		epsilonSq = 400L;
		lastTarget = currTarget = value();
		callback = NULL;
		state = 0;
	}
	
	// --- getters ---------------------------------------------------------------
	vec2 value() const {
		return vec2(
			stpX->value(), stpY->value() //, stpZ->value()
		);
	}
	vec2 target() const {
		return currTarget;
	}
	vec2 currentFreq() const {
		return vec2(
			stpX->currentFreq(), stpY->currentFreq()
		);
	}
	vec2 targetFreq() const {
		return vec2(
			stpX->targetFreq(), stpY->targetFreq()
		);
	}
	vec2 currDelta() const {
		return currTarget - lastTarget;
	}
	vec2 realDelta() const {
		return currTarget - value();
	}
	
	// --- checks ----------------------------------------------------------------
	bool hasTarget() const {
		return lastTarget != currTarget;
	}
	bool isEnding() const {
		return ending;
	}
	bool hasReachedTarget() const {
		vec2 r = realDelta(), d = currDelta();
		return r.dot(d) <= 0L
				|| r.sqLength() <= epsilonSq;
	}
	bool isMoving() const {
		return stpX->isRunning() || stpY->isRunning();
	}
	
protected:
	Stepper *stepper(int i) const {
		switch(i){
			case 0: return stpX;
			case 1: return stpY;
			default:
				error = ERR_INVALID_ACCESSOR;
				return stpY;
		}
	}

private:
	Stepper *stpX, *stpY;
	unsigned long f_best, df_max;
	unsigned long epsilonSq;
	
	// xy target data
	vec2 lastTarget;
	vec2 currTarget;
	bool ending;
	unsigned long targetID;
	
	// callback
	Callback callback;
	int state;
};
