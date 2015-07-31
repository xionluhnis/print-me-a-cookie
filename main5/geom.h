#pragma once

#include "Arduino.h"
#include "utils.h"

/**
 * Rounding type
 */
enum Rounding {
	CAST 	= 0, 
	DOWN 	= 1,
	UP 		= 2,
	ROUND = 3
};

/**
 * 2D Vector
 */
template <typename S>
struct vec2_ {
	typedef vec2_<S> vec;
	
	S x, y;
	vec2_(S a, S b) : x(a), y(b) {}
	explicit vec2_(S a = 0L) : x(a), y(a) {}
	template <typename S2>
	vec2_(const vec2_<S2> &o) : x(o.x), y(o.y) {}
	
	S &operator[](int i){
		switch(i){
			case 0:	return x;
			case 1: return y;
			default:
				error = ERR_INVALID_ACCESSOR;
				return y;
		}
	}
  S operator[](int i) const {
    switch(i){
      case 0: return x;
      case 1: return y;
      default:
        error = ERR_INVALID_ACCESSOR;
        return y;
    }
  }
	
	bool operator==(const vec &v) const {
		return v.x == x && v.y == y;
	}
	bool operator!=(const vec &v) const {
		return v.x != x || v.y != y;
	}
	vec operator+(const vec &v) const {
		return vec(x + v.x, y + v.y);
	}
  vec &operator+=(const vec &v) {
    x += v.x;
    y += v.y;
    return *this;
  }
	vec operator-(const vec &v) const {
		return vec(x - v.x, y - v.y);
	}
	vec operator-() const {
		return vec(-x, -y);
	}
	vec operator*(S f) const {
		return vec(x * f, y * f);
	}
 /*
	vec divideBy(S f, Rounding r = CAST){
		switch(r){
			case CAST:  return vec(x / f, y / f);
			case DOWN:  return vec(std::floor(x / float(f)), std::floor(y / float(f)));
			case UP: 	  return vec(std::ceil(x / float(f)), std::ceil(y / float(f)));
			case ROUND: return vec(std::round(x / float(f)), std::round(y / float(f)));
			default:
				error = ERR_INVALID_ROUNDING;
				return vec();
		}
	}
 */
	vec abs() const {
		return vec(std::abs(x), std::abs(y));
	}
	S dot(const vec &v) const {
		return x * v.x + y * v.y;
	}
	S sqLength() const {
		return dot(*this);
	}
	S sqDistTo(const vec &v) const {
		return (v - *this).sqLength();
	}
	S max() const {
		return std::max(x, y);
	}
	S min() const {
		return std::min(x, y);
	}
	static vec max(const vec &v1, const vec &v2){
		return vec(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
	}
	static vec min(const vec &v1, const vec &v2){
		return vec(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
	}
};

typedef vec2_<long> vec2;
typedef vec2_<unsigned long> uvec2;

