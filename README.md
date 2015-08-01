# print-me-a-cookie
Software related to our food printing project

## Links

* [Sparkfun EasyDriver](https://github.com/sparkfun/Big_Easy_Driver) for the stepper motors, and the [hookup guide](https://learn.sparkfun.com/tutorials/big-easy-driver-hookup-guide)
* [Avr-Libc](http://www.nongnu.org/avr-libc/user-manual/index.html) for the LibC implementation that Arduino makes use of

## Available Arduino commands

* `u pin microstep` - set the microstep mode for a stepper motor
* `p pin delta [speed init]` - step for delta steps at a given speed
* `m x y z [sx sy sz ix iy iz]` - move in x/y/z at a given speed
* `e delta [speed init]` - extrude for delta steps at a given speed
* `w [time]` - wait for a specific amount of time (ms for lowercase, s for uppercase)
* `l` - list files in the sd card with their id
* `o id` - run the file corresponding to the given id

Each stepper motor can be stepped using its corresponding pin command such as
```
x 1000 10 # steps in x for 1000 steps every 10 time steps
y 1000 10 # steps in y for 1000 steps every 10 time steps
z -1000 1 # steps in z for 1000 reversed steps every time step
```

## Path program

See Pathr.

## TODO

* Convert move+steps into move+speed formulation
* Add acceleration profile function (change speed at start/end)
* Fix saccade in motor movement for complex paths
