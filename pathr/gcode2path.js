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
 	var path = new Path();
 	
 	// frame
 	var dec = function(v){
 		return Math.round(parseFloat(v) * scale);
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
	 			path.extrude(
	 		}
	 		
	 		// 3 = should we move in z?
	 		var Z = getParam(fields, 'Z');
	 		if((!relative && Z != lastFields.Z) || (relative && Z)){
	 			if(relative){
	 				path.elevateBy(Z);
	 			} else {
	 				path.elevateTo(Z);
	 			}
	 			path.end();
	 		}
	 		
	 		// 4 = should we move in x/y?
	 		var X = getParam(field, 'X');
	 		var Y = getParam(field, 'Y');
	 		if((!relative && (X != lastFields.X || Y != lastFields.Y))
	 			 || (relative && (X || Y))){
	 			if(relative){
	 				path.moveBy(X, Y);
	 			} else {
	 				path.moveTo(X, Y);
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
 	while(lineReader.available()){
 		var line = lineReader.next();
 		currFields = 
 		// remove comments and needless spaces
 		var free = line.replace(/;.*$/, '')
 							     .replace(/( |\t)+/g, '');
 		// split into tokens
 		var base = line.replace(/([a-zA-Z*]+)([-0-9.]+)/g, function(match, par, val){
 			return par + ' ' + val;
 		}).replace(/([-0-9.]+)([a-zA-Z*]+)/g, function(match, val, par){
 			return val + ' ' + par;
 		});
 		var tokens = base.split(' ');
 		for(var i = 0; i < tokens.length; ++i){
 			var c = tokens[i].charAt(0);
 			var v = parseFloat(tokens[i].substring(1));
 			currFields[c] = v;
 		}
 		// find command
 		var command = null;
 		var commandCodes = ['G', 'M', 'T'];
 		for(var i = 0; i < commandCodes.length; ++i){
 			var commandCode = commandCodes[i];
	 		if(commandCode in currParams){
	 			command = commandCode + currFields[commandCode];
	 		}
	 	}
 		// compute path
 		if(command in parse){
 			parse[command](currFields);
 			// save last params for next command
 			for(var k in currFields){
 				lastFields[k] = currFields[k];
 			}
 		} else {
 			console.log('Unsupported command %s, line %d: %s\n', tokens[0], lineReader.line-1, line);
 		}
 	}
 	
 	// return the generated code
 	return path.code;
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
			line = this.substring(this.i, j);
			this.i = j + 1;
			this.line += 1;
		} else {
			line = this.substring(this.i);
			this.i = this.text.length;
		}
		return line;
	}
};