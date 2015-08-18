#pragma once

#include "error.h"

// callback type
typedef void (*Callback)(int state);

class EventSource {
public:
  EventSource() : count(0){
  }

  void listen(Callback cb){
    if(count >= 10){
      error = ERR_MAX_LISTENERS;
    } else {
      listeners[count] = cb;
      ++count;
    }
  }

  void trigger(int state = 0) {
    for(int i = 0; i < count; ++i){
      if(listeners[i])
        listeners[i](state);
    }
  }

private:
  Callback listeners[10];
  unsigned int count;
};

