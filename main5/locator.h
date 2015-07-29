#pragma once

#include "Arduino.h"
#include "stepper.h"

struct Vec {
	long x, y, z;
	Vec(long a = 0L, long b = 0L, long c = 0L) : x(a), y(b), z(c) {}
	
	long &operator[](int i){
		switch(i){
			case 0:	return x;
			case 1: return y;
			case 2: return z;
			default:
				error = ERR_INVALID_ACCESSOR;
				return z;
	}
	
	Vec operator+(const Vec &v){
		return Vec(x + v.x, y + v.y, z + v.z);
	}
	Vec operator-(const Vec &v){
		return Vec(x - v.x, y - v.y, z - v.z);
	}
	Vec operator-(){
		return Vec(-x, -y, -z);
	}
	Vec operator*(long f){
		return Vec(x * f, y * f, z * f);
	}
	Vec operator/(long f){
		return Vec(x / f, y / f, z / f);
	}
	Vec abs(){
		return Vec(std::abs(x), std::abs(y), std::abs(z));
	}
	long dot(const Vec &v){
		return x * v.x + y * v.y + z * v.z;
	}
	long sqLength(){
		return dot(*this);
	}
	long max(){
		return std::max(x, std::max(y, z));
	}
	long min() {
		return std::min(x, std::min(y, z));
	}
};

class Locator {
public:
	Locator(Stepper *x, Stepper *y, Stepper *z) : stpX(x), stpY(y), stpZ(z) {
		f_best = 5L;
		df_max = 24L;
	}
	
	void moveBy(const Vec &d){
	  // now, we assume that we start completely idle to simplify the problem
	  Vec a = d.abs();
	  long d_max = std::max(a.x, a.y);
	  int k = a.x == d_max ? 0 : 1;
	  int j = 1 - k;
	  // compute best-case frequencies
	  long f_trg[2];
	  for(int i = 0; i < 2; ++i){
	  	if(i == k){
	  		f_trg[i] = f_best;
	  	} else {
	  		f_trg[i] = long(std::round(f_best * d_max / float(d[i])));
	  	}
	 	}
		
		// XXX we must account for the time it takes to reach these frequencies
		unsigned long df[2] = { df_max, df_max };
		unsigned long t[2] = { stpX->timeToFreq(f_trg[0], df[0]), stpY->timeToFreq(f_trg[1], df[1]) };
		// Note: the j=1-k will have a smaller time (since the f_trg[k] > f_trg[j])
		//    => we must slow down j
		while(t[j] < t[k] && df[j] > 1L){
			df[j] -= 1; // slowing down
			t[j] = stepper(j)->timeToFreq(f_trg[j], df[j]);
		}
		
		// XXX instead of finding the best sequence of parameters,
		// 		 use gradient descent to optimize the parameters automatically while moving
	}
	
	Vec bestFreq(const Vec &delta){
		Vec absDelta = delta.abs();
		long d_max = absDelta.max();
		Vec f;
		for(int i = 0; i < 3; ++i){
			if(absDelta[i] == d_max){
				f[i] = sign(delta[i]) * f_best;
			} else if(absDelta[i] == 0L) {
				f[i] = 0L;
			} else {
				f[i] = (long)std::round(f_best * d_max / float(delta[i]));
			}
		}
		return f;
	}
	
	void moveTo(const Vec &delta){
	
	}
	
	Vec currentValue() {
		return Vec(
			stpX->value(), stpY->value(), stpZ->value()
		);
	}
	Vec currentFreq() {
		return Vec(
			stpX->currentFreq(), stpY->currentFreq(), stpZ->currentFreq()
		);
	}
	Vec targetFreq() {
		return Vec(
			stpX->targetFreq(), stpY->targetFreq(), stpZ->targetFreq()
		);
	}
	
	void setBestFreq(unsigned long f){
		if(f)
			f_best = f;
	}
	
	void setMaxDeltaFreq(unsigned long df){
		if(df)
			df_max = df;
	}
	
protected:
	Stepper *stepper(int i){
		switch(i){
			case 0: return stpX;
			case 1: return stpY;
			case 2: return stpZ;
			default:
				error = ERR_INVALID_ACCESSOR;
				return stpZ;
	}

private:
	Stepper *stpX, *stpY, *stpZ;
	unsigned long f_best, df_max;
	
	Vec lastDir;
};
