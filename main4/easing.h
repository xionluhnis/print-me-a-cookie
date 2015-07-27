#pragma once

// Mainly from @see https://github.com/mgcrea/arduino_tween

typedef float (*EasingFunc)(float, float, float, float);

namespace Easing {
  
  // constant
  float constant(float t, float b, float c, float d) {
  	return b + c;
  }
  
  // linear
  float linear(float t, float b , float c, float d) {
    // b -> b + c, t, 0 -> d
    //currentTime, from, until, duration
    return c*t/d + b;
  }
  
  // quad
  float quadIn(float t, float b , float c, float d) {
    return c*(t/=d)*t + b;
  }
  float quadOut(float t, float b , float c, float d) {
    return -c *(t/=d)*(t-2) + b;
  }
  float quadInOut(float t, float b , float c, float d) {
    if ((t/=d/2) < 1) return ((c/2)*(t*t)) + b;
    return -c/2 * (((t-2)*(--t)) - 1) + b;
    /*
     originally return -c/2 * (((t-2)*(--t)) - 1) + b;
  
     I've had to swap (--t)*(t-2) due to diffence in behaviour in
     pre-increment operators between java and c++, after hours
     of joy
     */
  
  }
  
  // cubic
  float cubicIn(float t, float b , float c, float d) {
    return c*(t/=d)*t*t + b;
  }
  float cubicOut(float t, float b , float c, float d) {
    return c*((t=t/d-1)*t*t + 1) + b;
  }
  float cubicInOut(float t, float b , float c, float d) {
    if ((t/=d/2) < 1) return c/2*t*t*t + b;
    return c/2*((t-=2)*t*t + 2) + b;
  }
  
  // quart
  float quartIn(float t, float b , float c, float d) {
    return c*(t/=d)*t*t*t + b;
  }
  float quartOut(float t, float b , float c, float d) {
    return -c * ((t=t/d-1)*t*t*t - 1) + b;
  }
  float quartInOut(float t, float b , float c, float d) {
    if ((t/=d/2) < 1) return c/2*t*t*t*t + b;
    return -c/2 * ((t-=2)*t*t*t - 2) + b;
  }
  
  // quint
  float quintIn(float t, float b , float c, float d) {
    return c*(t/=d)*t*t*t*t + b;
  }
  float quintOut(float t, float b , float c, float d) {
    return c*((t=t/d-1)*t*t*t*t + 1) + b;
  }
  float quintInOut(float t, float b , float c, float d) {
    if ((t/=d/2) < 1) return c/2*t*t*t*t*t + b;
    return c/2*((t-=2)*t*t*t*t + 2) + b;
  }
  
  // back
  float backIn(float t, float b , float c, float d) {
    float s = 1.70158f;
    float postFix = t/=d;
    return c*(postFix)*t*((s+1)*t - s) + b;
  }
  float backOut(float t, float b , float c, float d) {
    float s = 1.70158f;
    return c*((t=t/d-1)*t*((s+1)*t + s) + 1) + b;
  }
  float backInOut(float t, float b , float c, float d) {
    float s = 1.70158f;
    if ((t/=d/2) < 1) return c/2*(t*t*(((s*=(1.525f))+1)*t - s)) + b;
    float postFix = t-=2;
    return c/2*((postFix)*t*(((s*=(1.525f))+1)*t + s) + 2) + b;
  }
  
  // bounce
  float bounceIn(float t, float b , float c, float d) {
    return c - BounceOut (d-t, 0, c, d) + b;
  }
  float bounceOut(float t, float b , float c, float d) {
    if ((t/=d) < (1/2.75f)) {
      return c*(7.5625f*t*t) + b;
    } else if (t < (2/2.75f)) {
      float postFix = t-=(1.5f/2.75f);
      return c*(7.5625f*(postFix)*t + .75f) + b;
    } else if (t < (2.5/2.75)) {
      float postFix = t-=(2.25f/2.75f);
      return c*(7.5625f*(postFix)*t + .9375f) + b;
    } else {
      float postFix = t-=(2.625f/2.75f);
      return c*(7.5625f*(postFix)*t + .984375f) + b;
    }
  }
  float bounceInOut(float t, float b , float c, float d) {
    if (t < d/2) return BounceIn (t*2, 0, c, d) * .5f + b;
    else return BounceOut (t*2-d, 0, c, d) * .5f + c*.5f + b;
  }
  
  // circ
  float circIn(float t, float b , float c, float d) {
    return -c * (sqrt(1 - (t/=d)*t) - 1) + b;
  }
  float circOut(float t, float b , float c, float d) {
    return c * sqrt(1 - (t=t/d-1)*t) + b;
  }
  float circInOut(float t, float b , float c, float d) {
    if ((t/=d/2) < 1) return -c/2 * (sqrt(1 - t*t) - 1) + b;
    return c/2 * (sqrt(1 - t*(t-=2)) + 1) + b;
  }
  
  // elastic
  float elasticIn(float t, float b , float c, float d) {
    if (t==0) return b;  if ((t/=d)==1) return b+c;
    float p=d*.3f;
    float a=c;
    float s=p/4;
    float postFix =a*pow(2,10*(t-=1)); // this is a fix, again, with post-increment operators
    return -(postFix * sin((t*d-s)*(2*M_PI)/p )) + b;
  }
  float elasticOut(float t, float b , float c, float d) {
    if (t==0) return b;  if ((t/=d)==1) return b+c;
    float p=d*.3f;
    float a=c;
    float s=p/4;
    return (a*pow(2,-10*t) * sin( (t*d-s)*(2*M_PI)/p ) + c + b);
  }
  float elasticInOut(float t, float b , float c, float d) {
    if (t==0) return b;  if ((t/=d/2)==2) return b+c;
    float p=d*(.3f*1.5f);
    float a=c;
    float s=p/4;
  
    if (t < 1) {
      float postFix =a*pow(2,10*(t-=1)); // postIncrement is evil
      return -.5f*(postFix* sin( (t*d-s)*(2*M_PI)/p )) + b;
    }
    float postFix =  a*pow(2,-10*(t-=1)); // postIncrement is evil
    return postFix * sin( (t*d-s)*(2*M_PI)/p )*.5f + c + b;
  }
  
  // expo
  float expoIn(float t, float b , float c, float d) {
    return (t==0) ? b : c * pow(2, 10 * (t/d - 1)) + b;
  }
  float expoOut(float t, float b , float c, float d) {
    return (t==d) ? b+c : c * (-pow(2, -10 * t/d) + 1) + b;
  }
  float expoInOut(float t, float b , float c, float d) {
    if (t==0) return b;
    if (t==d) return b+c;
    if ((t/=d/2) < 1) return c/2 * pow(2, 10 * (t - 1)) + b;
    return c/2 * (-pow(2, -10 * --t) + 2) + b;
  }
  
  // sine
  float sineIn(float t, float b , float c, float d) {
    return -c * cos(t/d * (M_PI/2)) + c + b;
  }
  float sineOut(float t, float b , float c, float d) {
    return c * sin(t/d * (M_PI/2)) + b;
  }
  float sineInOut(float t, float b , float c, float d) {
    return -c/2 * (cos(t/d * M_PI) - 1) + b;
  }
  
  // breathe
  // f(x) = (exp(sin(x)) - 1/e ) * 255 / (e - 1/e)
  // min 0 at x = -π/2 + 2nπ
  // max 255 at x = π/2 + 2nπ
  float breatheIn(float t, float b , float c, float d) {
    return -c * (exp(cos(t / d * M_PI)) - 0.36787944) / 2.35040238 + c + b;
  }
  float breatheOut(float t, float b , float c, float d) {
    return c * (exp(sin(t / d * M_PI - M_PI / 2)) - 0.36787944) / 2.35040238 + b;
  }
  float breatheInOut(float t, float b , float c, float d) {
    return -c / 2 * ((exp(cos(t / d * 2 * M_PI)) - 0.36787944) / 2.35040238 - 1) + b;
  }

}
