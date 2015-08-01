# PATHR

Webapp to compute path from svg or gcode files.

Makes use of

* [Bezier.js](https://github.com/Pomax/bezierjs)
* [Simplify.js](https://github.com/mourner/simplify-js)

## Supported:
**SVG**: (only outlines, no style information)
* `g` tags
* `transform` attributes
* `rect` tags
* `polygon` and `polyline` tags
* `path` tags (M, m, L, l, T, t, C, c, Q, q, S, s supported, A and a not supported)

**GCodes**:
* G0, G1 - move
* G4 - dwell
* G28 - move to origin
* G90, G91, G92 - set positioning (absolute, relative, reset position)

See [GCode](http://reprap.org/wiki/G-code) wiki.

## TODO

* update svg2path.js to use the new path commands
* implement arcTo / arcBy to be used for A/a paths and G2/G3 codes
* add parameter panel to specify speeds, acceleration and various path parameters
* implement shape filling for svg?
