# print-me-a-cookie
Software related to our food printing project

## Links

* [Sparkfun EasyDriver](https://github.com/sparkfun/Big_Easy_Driver) for the stepper motors, and the [hookup guide](https://learn.sparkfun.com/tutorials/big-easy-driver-hookup-guide)
* [Avr-Libc](http://www.nongnu.org/avr-libc/user-manual/index.html) for the LibC implementation that Arduino makes use of

## TODO

1. Convert move+steps into move+speed formulation
2. Make use of all partial speeds (ms1-3 => 8 different speeds)
3. Add acceleration profile function (change speed at start/end)
4. Read from SD card in a loop fashion

```
# my file comment
command 1 # serial
command 2 # serial
command 3, command 4, ... # parallel
```

## Commands (to implement)

* `u pin microstep` - set the microstep mode for a stepper motor
* `s pin delta speed init` - step for delta steps at a given speed
* `m x y z sx [sy sz ix iy iz]` - move in x/y/z at a given speed

