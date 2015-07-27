#pragma once

#include <Arduino.h>
#include "easing.h"

class TweenTimer {

public:

  int state;
  float created, time, duration;
  unsigned int loop, count;

  TweenTimer() {
    loop = 0;
    reset(0.0);
  }

  void reset(float _duration) {
    duration = _duration;
    reset();
  }

  void reset() {
    created = millis();
    count = 0;
    time = 0.0;
    state = 0;
  }

  void tick() {

    if(state == 0) {
      return;
    }

    int ctime = millis();
    if(state == 1) {
      time = ctime - created;
    } else if(state == -1) {
      time = (created + duration) - ctime;
    }

    if (time > duration) {
      count++;
      time = duration;
      created = ctime;

      if(loop == 0) {
        state = 0;
      } else if(loop == 1) {
        time = 0;
        state = 1;
      } else if(loop >= 2) {
        state = -1;
      }
    } else if(time < 0) {
      count++;
      time = 0;
      created = ctime;

      if(loop == 0) {
        state = 0;
      } else if(loop > 0) {
        state = 1;
      }
    }
  }
};

class Tween {

public:

  Tween() : value(0.0), percent(0.0), _ease(Easing::constant), _verbose(false){}
  void setup(int duration, float start, float change, EasingFunc ease, int delay = 0, int loop = 0) {
		_tween.reset(duration);
		_tween.loop = loop;
		_delay.reset(delay);

		_begin = begin;
		_finish = finish;
		_change = finish - begin;

		value = begin;
		percent = 0;
		_ease = ease ? ease : Easing::constant; // should never be NULL!
  }
  void play(){
		_tween.created = millis();
		_delay.created = millis();

		if(_delay.duration > 0 && _delay.time != _delay.duration) {
		  _delay.state = 1;
		} else {
		  _tween.state = 1;
		}
  }
  void pause(){
  	_delay.state = 0;
 	 	_tween.state = 0;
  }
  void stop(){
  	pause();

		_delay.time = 0;
		_tween.time = 0;
  }

  bool isRunning(){
  	return _delay.state != 0 || _tween.state != 0 ? true : false;
  }
  bool isFinished(){
  	return _tween.time == _tween.duration ? true : false;
  }
  void setVerbosity(bool verbose){
  	_verbose = verbose;
  }

  void update() {
		if (_delay.duration > 0) {
			_delay.tick();
		}

		_tween.tick();

		percent = _tween.time / _tween.duration;

		if (_tween.state != 0 && _tween.time == _tween.duration) {
			if(_verbose){
				Serial.println("! _tween.time == _tween.duration"); Serial.print("/!\\"); debug();
			}
			if (_delay.duration > 0) {
				if(_verbose) {
					Serial.println("! Setup downward delay"); Serial.print("/!\\"); debug();
				}
				// Setup downward delay
				_delay.reset();
				_delay.state = 1;
				_tween.state = 0;
			} else {
				if (_tween.loop == 2) {
					_tween.state = -1;
				}
			}
		} else if (_tween.state != 0 && _tween.time == 0) {
			if(_verbose){
				Serial.println("! _tween.time == 0"); Serial.print("/!\\"); debug();
			}
			if (_delay.duration > 0) {
				if(_verbose){
					Serial.println("! Setup upward delay"); Serial.print("/!\\"); debug();
				}
				// Setup upward delay
				_delay.reset();
				_delay.state = 1;
				_tween.state = 0;
			}
		}

		if (_delay.duration > 0) {
			if (_delay.time == _delay.duration && _delay.state == 0) {
				if(_verbose){
					Serial.println("! delay finished"); Serial.print("/!\\"); debug();
				}
				_delay.time = 0;
				_tween.created = millis();
				if (_tween.loop == 2) {
				_tween.state = _tween.count % 2 == 0 ? 1 : -1;
				} else {
				_tween.state = 1;
				}
			}
		}

		exec();
  }
  int getCount(){
  	return _tween.count;
  }
  void debug() {
  	// Debug
		Serial.print("value=");
		Serial.print(value);
		Serial.print(",tween_state=");
		Serial.print(_tween.state);
		Serial.print(",tween_count=");
		Serial.print(_tween.count);
		Serial.print(",tween_time=");
		Serial.print(static_cast<int>(_tween.time));
		Serial.print("/");
		Serial.print(static_cast<int>(_tween.duration));
		Serial.print(",delay_state=");
		Serial.print(_delay.state);
		Serial.print(",delay_count=");
		Serial.print(_delay.count);
		Serial.print(",delay_time=");
		Serial.print(static_cast<int>(_delay.time));
		Serial.print("/");
		Serial.println(static_cast<int>(_delay.duration));
  }

  float value;
  float percent;

private:

  TweenTimer _tween;
  TweenTimer _delay;

  float _begin;
  float _finish;
  float _change;
  EasingFunc _ease;
  bool _verbose;

  void exec(){
  	value = _ease(_tween.time, _begin, _change, _tween.duration);
  }
};

