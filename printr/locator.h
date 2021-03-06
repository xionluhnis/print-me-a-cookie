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
	
	vec2 bestFreq(const vec2 &delta, unsigned long f_best){
		vec2 abs = delta.abs();
		long d_max = abs.max();
		vec2 f;
		for(int i = 0; i < 2; ++i){
			if(abs[i] == d_max){
				f[i] = sign(delta[i]) * f_best;
			} else if(abs[i] <= stepper(i)->stepSize()) {
				f[i] = 0L;
			} else {
				f[i] = (long)std::round(f_best * d_max / float(delta[i]));
        // /!\ this can raise values lower than f_best
        // => replace by (+/-)f_best in these cases
        if(std::abs(f[i]) < f_best){
          f[i] = sign(delta[i]) * f_best;
        }
			}
		}
		return f;
	}
  vec2 bestFreq(const vec2 &delta){
   return bestFreq(delta, f_best);
  }
	
	void update(){
    // should we work or not?
    if(!enabled) return;
    
		// - should we be idle?
		if(!hasTarget()){
      if(debugMode > 1) Serial.println("No target.");
      // just go idle if not already
		  if(isMoving()){
  			for(int i = 0; i < 2; ++i){
  				Stepper *stp = stepper(i);
          if(!stp->lowMicrostep()){
            stp->microstep(Stepper::MS_SLOW);
          }
  				if(stp->targetFreq() != Stepper::IDLE_FREQ)
  					stp->moveToFreq(Stepper::IDLE_FREQ);
  			}
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
		
    vec2 delta = realDelta();
		// - should we stop at the target?
		if(isEnding()){
			long x0 = stpX->stepsToFreq(Stepper::IDLE_FREQ),
					 y0 = stpY->stepsToFreq(Stepper::IDLE_FREQ);
			// should we start slowing down?
			vec2 targetFreq;
			if((currTarget - vec2(x0, y0)).sqLength() < epsilonSq){
        if(debugMode > 1){
          Serial.print("Ending stop in "); Serial.print(x0); Serial.print(", "); Serial.print(y0);
          Serial.print(" for target "); Serial.print(currTarget.x); Serial.print(", "); Serial.print(currTarget.y);
          Serial.print(" | curFreq "); Serial.print(stpX->currentFreq()); Serial.print(", "); Serial.println(stpY->currentFreq());
        }
        // targetFreq = bestFreq(delta);
				// it will take us enough time to stop
				// that we should start slowing down now!
				targetFreq = bestFreq(delta, currentFreq().abs().max() + 1L); // vec2(Stepper::IDLE_FREQ);
        // TODO this change is a HACK, it's not intended (most likely valueAtFreq is wrong instead)
			} else {
				targetFreq = bestFreq(delta);
        if(debugMode > 1){
          Serial.print("delta: "); Serial.print(delta.x); Serial.print(", "); Serial.print(delta.y);
          Serial.print(" => trgFreq "); Serial.print(targetFreq.x); Serial.print(", "); Serial.print(targetFreq.y);
          Serial.print(" | curFreq "); Serial.print(stpX->currentFreq()); Serial.print(", "); Serial.println(stpY->currentFreq());
        }
			}
			adjustToFreq(targetFreq, delta);
		} else
		
		// - we must make our line as straight as possible
		{
      if(debugMode > 1) Serial.println("Straight line");
			adjustToFreq(bestFreq(delta), delta);
		}
	}
	
	static unsigned long deltaTime(unsigned long t1, unsigned long t2){
		return std::max(t1, t2) - std::min(t1, t2); // won't underflow
	}
	
	void adjustToFreq(vec2 f_trg, const vec2 &delta){
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

      /*
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
     */
		}

    /*
    // acceleration using ms
    long f_x = std::abs(stpX->currentFreq());
    long f_y = std::abs(stpY->currentFreq());
    vec2 d = delta.abs();
    bool msX = f_x == 1L && std::abs(stpX->targetFreq()) == 1L;
    bool msY = f_y == 1L && std::abs(stpY->targetFreq()) == 1L;
    if(d.sqLength() > 5000L * 5000L){
      if(d.x == 0L || d.y > 5L * d.x){
        stpY->microstep(Stepper::MS_1_4);
        f_trg[0] /= 4;
      } else if(d.y == 0L || d.x > 5L * d.y){
        stpX->microstep(Stepper::MS_1_4);
        f_trg[1] /= 4;
      } else {
        msX = msY = false;
      }
    } else if(d.sqLength() > 1000L * 1000L){
      if(d.x == 0L || d.y > 5L * d.x){
        stpY->microstep(Stepper::MS_1_8);
        f_trg[0] /= 2;
      } else if(d.y == 0L || d.x > 5L * d.y){
        stpX->microstep(Stepper::MS_1_8);
        f_trg[1] /= 2;
      } else {
        msX = msY = false;
      }
    } else {
      msX = msY = false;
    }
    
    if(!msX && !stpX->lowMicrostep()){
      stpX->microstep(Stepper::MS_SLOW);
    }
    if(!msY && !stpY->lowMicrostep()){
      stpY->microstep(Stepper::MS_SLOW);
    }
    */
    
		// done, we update the parameters of both stepper motors
		for(int i = 0; i < 2; ++i){
      if(debugMode > 2){
        Serial.print("dt "); Serial.print(df[i], DEC); Serial.print(", f_trg "); Serial.println(f_trg[i]);
      }
			stepper(i)->setDeltaFreq(df[i]);
			stepper(i)->moveToFreq(f_trg[i]); // TODO these don't work correctly! => bug in updateFreq
		}
	}
	
	// --- setters ---------------------------------------------------------------
	void setTarget(const vec2 &trg, bool end = true){
		// shift targets
		lastTarget = currTarget;
		currTarget = trg;
   
		// movement state
		ending = end;
   
    // reset memory so that we can move optimally
    stpX->resetMemory();
    stpY->resetMemory();
    
		// update target id
		++targetID;
   
   if(debugMode){
      Serial.print("New targets: ");
      Serial.print("from "); Serial.print(lastTarget.x, DEC); Serial.print(", "); Serial.print(lastTarget.y, DEC);
      Serial.println(" ->"); Serial.print(currTarget.x, DEC); Serial.print(", "); Serial.println(currTarget.y, DEC);
      Serial.print("Current: "); Serial.print(stpX->value()); Serial.print(", "); Serial.println(stpY->value());
    }
	}
  void resetX(long x){
    stpX->resetPosition(x);
    lastTarget.x = currTarget.x = x;
  }
  void resetY(long y){
    stpY->resetPosition(y);
    lastTarget.y = currTarget.y = y;
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
		epsilon = eps;
    epsilonSq = std::max(1UL, eps * eps);
	}
	void setCallback(Callback cb){
		callback = cb;
	}
	void setState(int s0){
		state = s0;
	}
	void reset() {
		f_best = 1L;
		df_max = 1L;
		setPrecision(5UL);
		lastTarget = currTarget = value();
    ending = true;
		callback = NULL;
		state = 0;
    enabled = true;
	}
  void toggle(){
    enabled = !enabled;
  }
  void enable(){
    enabled = true;
  }
  void disable(){
    enabled = false;
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
		return lastTarget != currTarget || !hasReachedTarget();
	}
	bool isEnding() const {
		return ending;
	}
	bool hasReachedTarget() const {
		vec2 r = realDelta(), d = currDelta();
		return r.dot(d) < 0L
				|| r.sqLength() <= epsilonSq;
	}
	bool isMoving() const {
		return stpX->isRunning() || stpY->isRunning();
	}
  bool isEnabled() const {
    return enabled;
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

public:
  void debug() {
    Serial.println("debug(m):");
    Serial.print("f_best "); Serial.println(f_best, DEC);
    Serial.print("df_max "); Serial.println(df_max, DEC);
    Serial.print("eps    "); Serial.println(epsilon, DEC);
    Serial.print("lastTg "); Serial.print(lastTarget.x, DEC); Serial.print(", "); Serial.println(lastTarget.y, DEC);
    Serial.print("currTg "); Serial.print(currTarget.x, DEC); Serial.print(", "); Serial.println(currTarget.y, DEC);
  }

  void setDebugMode(int m){
    debugMode = m;
  }

private:
	Stepper *stpX, *stpY;
	unsigned long f_best, df_max;
	unsigned long epsilon, epsilonSq;
	
	// xy target data
	vec2 lastTarget;
	vec2 currTarget;
	bool ending;
	unsigned long targetID;
	
	// callback
	Callback callback;
	int state;

  // state
  bool enabled;
  int debugMode;
};


