/**
 * GCode to Path conversion
 *
 * @author Alexandre Kaspar <akaspar@mit.edu>
 */
function gcode2path(gcode, params){
  if(!params) params = {};
  if(!params.scale) params.scale = 1.0;
  if(!params.EP) params.EP = 10;
  
  var lineReader = new LineReader(gcode || '');
  
  // path driver
  var path = new Path('GCode to Path');
  
  // frame
  var dec = function(v){
    return Math.round(parseFloat(v + '') * params.scale);
  };
  
  var lastFields = {
    X: 0.0, Y: 0.0, Z: 0.0, F: 0.0, E: 0.0, A: 0.0
  };
  var getParam = function(block, key, def){
    if(key in block){
      return block[key];
    } else if(def != undefined){
      return def;
    } else {
      return lastFields[key];
    }
  };

  var relative = false;
  var extrusion = 0;
  
  var parse = {
    // --- Move ----------------------------------------------------------------
    G0: function(fields){
      return parse.G1(fields);
    },
    G1: function(fields){
      // 1 = should we change motion speed?
      // 2 = should we change extrusion speed?
      var E = getParam(fields, 'E');
      var A = getParam(fields, 'A', E);
      if(E != lastFields.E){
        path.extrude(E).and(); // do not stop line here!
      } else if(A != lastFields.A){
        // TODO implement
      }
      
      // 3 = should we move in z?
      var Z = getParam(fields, 'Z');
      if((!relative && Z != lastFields.Z) || (relative && Z)){
        if(relative){
          path.elevateBy(dec(Z));
        } else {
          path.elevateTo(dec(Z));
        }
        path.end();
      }
      
      // 4 = should we move in x/y?
      var X = getParam(fields, 'X');
      var Y = getParam(fields, 'Y');
      if((!relative && (X != lastFields.X || Y != lastFields.Y))
         || (relative && (X || Y))){
        if(relative){
          path.moveBy(dec(X), dec(Y));
        } else {
          path.moveTo(dec(X), dec(Y));
        }
        path.end();
      }
    },
    
    // --- Controlled Arc Move -------------------------------------------------
    G2: function(fields, counterClockwise){
      // TODO implement arc movement
      // G2 Xnnn Ynnn Innn Jnnn Ennn
    },
    G3: function(fields){
      return parse.G2(fields, true);
    },
    
    // --- Dwell ---------------------------------------------------------------
    G4: function(fields){
      if('P' in fields){
        path.wait(fields.P).end();
      } else if('S' in fields){
        path.wait(fields.S).end();
      }
    },
    
    // --- Move to Origin ------------------------------------------------------
    G28: function(fields){
      var X = 'X' in fields;
      var Y = 'Y' in fields;
      var Z = 'Z' in fields;
      if(X || Y || Z){
        var pos = path.currentPosition();
        if(X || Y){
          path.moveTo(X ? 0 : pos.x, Y ? 0 : pos.y).end();
        }
        if(Z){
          path.elevateTo(0).end();
        }
      } else {
        path.moveTo(0, 0).then().elevateTo(0).end();
      }
    },
    
    // --- Positioning ---------------------------------------------------------
    G90: function(fields){
      relative = false;
    },
    G91: function(fields){
      relative = true;
    },
    G92: function(fields){
      var X = 'X' in fields;
      var Y = 'Y' in fields;
      var Z = 'Z' in fields;
      var E = 'E' in fields;
      
      // full
      if(!X && !Y && !Z && !E){
        path.resetPosition(0, 0, 0);
        lastFields.X = lastFields.Y = lastFields.Z = lastFields.E = 0.0;
      } else {
        // partial
        var x = X ? 0 : path.currentPosition().x;
        var y = Y ? 0 : path.currentPosition().y;
        var z = Z ? 0 : path.currentPosition().z;
        path.resetPosition(x, y, z);
        if(E){
          fields.E = 0;
        }
      }
    }
  };
  // parse every line, one by one
  var lastGCommand = null;
  while(lineReader.available()){
    var line = lineReader.next();
    var currFields = {};
    // remove comments and needless spaces
    var free = line.replace(/;.*$/, '')
                   .replace(/\([^)]*\)/g, '')
                   .replace(/( |\t)+/g, '');
    // split into tokens
    var base = free.replace(/([a-zA-Z*]+)([-0-9.]+)/g, function(match, par, val){
      return par + ' ' + val;
    }).replace(/([-0-9.]+)([a-zA-Z*]+)/g, function(match, val, par){
      return val + ' ' + par;
    });
    var tokens = base.split(' ');
    var command = null;
    var extraCommands = [];
    var commandCodes = { G: true, M: true, T: true };
    var implGCode = { X: true, Y: true, Z: true };
    for(var i = 0; i < tokens.length; ++i){
      if(tokens[i].match(/[a-zA-Z]+/)){
        var c = tokens[i].toUpperCase();
        // parameter?
        if(i < tokens.length - 1 && tokens[i+1].match(/[0-9]+/)){
          var v = parseFloat(tokens[i+1]);
          currFields[c] = v;
          ++i; // skip that value since we use it
        } else {
          // no parameter
          currFields[c] = undefined;
        }
        // is it a command?
        if(c in commandCodes){
          var newCommand = c + currFields[c];
          if(command){
            extraCommands.push(newCommand);
          } else {
            command = newCommand;
          }
        } else if(!command && c in implGCode){
          command = lastGCommand; // implicitely generate a command
        }
      } else {
        // token is not a command or field!
        console.log('Invalid token: %s', tokens[i]);
      }
    }
    // default to last command if there is one
    if(!command){
      console.log('No valid command: %s\n', line);
      continue;
    }
    if(command.charAt(0) == 'G'){
      lastGCommand = command;
    }

    // execute command
    if(command in parse){
      parse[command](currFields);
    } else {
      console.log('Unsupported command %s, line %d: %s\n', command, lineReader.line, line);
    }
    // execute extra commands
    for(var i = 0; i < extraCommands.length; ++i){
      var c = extraCommands[i];
      if(c in parse)
        parse[c](currFields);
    }
    // save last params for next command
    for(var k in currFields){
      lastFields[k] = currFields[k];
    }
  }
  
  // return the generated code
  return path;
}

function LineReader(text){
  this.text = text;
  this.i = 0;
  this.line = 0;
}
LineReader.prototype = {
  available: function() {
    return Math.max(0, this.text.length - this.i);
  },
  next: function(){
    var j = this.text.indexOf('\n', this.i);
    var line = null;
    if(j > 0){
      line = this.text.substring(this.i, j);
      this.i = j + 1;
      this.line += 1;
    } else {
      line = this.text.substring(this.i);
      this.i = this.text.length;
    }
    return line;
  }
};
