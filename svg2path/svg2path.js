/**
 * SVG to Path conversion
 *
 * @author Alexandre Kaspar <akaspar@mit.edu>
 */
 
 function Path(){
 	this.code = '# SVG2Path \n';
 	
 	// geometry context
 	this.context = new Matrix();
 	this.lastLoc = new Point(0, 0); // absolute location (no context)
 	this.relPos = new Point(0, 0);  // context-dependent relative position
 	this.lastCtrl = null; // last control point
 	
 	// stacks
 	this.contextStack = [];
 	this.codeStack = [];
 }
 
 Path.prototype = {
 // context and stacks
 	storeContext: function(){
 		this.contextStack.push(this.context.clone());
 		this.relPos = new Point(0, 0); // this value is not well defined when changing context
 	},
 	releaseContext: function(){
 		this.context = this.contextStack.pop();
 	},
 	storeCode: function(){
 		this.codeStack.push(this.code + '');
 	},
 	releaseCode: function(){
 		this.codeStack.pop();
 	},
 	restoreCode: function(){
 		this.code = this.codeStack.pop();
 	},
 	
// utils
	project: function(p, c){
		return new Point(2 * c.x - p.x, 2 * c.y - p.y);
	},
	bezierSamples: function(p1, c1, c2, p2){
 		var cubic = !!p2;
 		if(!c2){
 			console.log("Calling bezierSamples with too few arguments!");
 		}
 		var curve = cubic ? new Bezier([p1, c1, c2, p2]) : new Bezier([p1, c1, c2]);
 		var len = curve.length();
 		var N = Math.max(5, Math.ceil(len / 16.0));
 		return {
 			curve: curve,
 			length: len,
 			points: curve.getLUT(N)
 		};
 	},
 	
// getter command
	currentPos: function(){
		return this.relPos;
	},
 	
// path commands
 	m_command: function(dx, dy, dz, sx, sy, sz){
 		if(!dz) dz = 0;
 		this.code += 'm ' + dx + ' ' + dy + ' ' + dz;
 		if(sx) this.code += ' ' + sx;
 		if(sy) this.code += ' ' + sy;
 		if(sz) this.code += ' ' + sz;
 	},

 	
// javascript commands
	shift: function(){
 		var newLoc = this.context.transformPoint(this.relPos); // compute new absolute location
 		var delta = newLoc.sub(this.lastLoc).round(); 		   // compute relative shift in absolute location
 		if(delta.x != 0 || delta.y != 0){
 			var adelta = delta.abs();
 			var deltaMax = Math.max(adelta.x, adelta.y);
 			var sx = Math.ceil(deltaMax / Math.max(adelta.x, 5) * 5);
 			var sy = Math.ceil(deltaMax / Math.max(adelta.y, 5) * 5);
 			this.m_command(delta.x, delta.y, 0, sx, sy);
 			this.lastLoc = newLoc;
 		}
 		return this;
 	},
 	moveBy: function(x, y){
 		this.relPos = this.relPos.translate(x, y);
 		this.lastCtrl = null;
 		return this.shift();
 	},
 	moveTo: function(x, y){
 		this.relPos = new Point(x, y);
 		this.lastCtrl = null;
 		return this.shift();
 	},
 	extrude: function(delta, speed){
 		this.code += 'e ' + delta;
 		if(speed) this.code += ' ' + speed;
 		return this;
 	},
 	wait: function(t){
 		if(!t) t = 500;
 		this.code += 'w ' + t;
 		return this;
 	},
 	lineBy: function(x, y){
 		var loc = this.lastLoc;
 		this.moveBy(x, y).and();
 		var delta = this.lastLoc.sub(loc).abs();
 		var speed = Math.ceil(Math.max(delta.x, delta.y) / 2);
 		this.extrude(speed, 10)
 			.then()
 			.wait();
 		return this;
 	},
 	lineTo: function(x, y){
 		var loc = this.lastLoc;
 		this.moveTo(x, y).and();
 		var delta = this.lastLoc.sub(loc).abs();
 		var speed = Math.ceil(Math.max(delta.x, delta.y) / 2);
 		this.extrude(speed, 10)
 			.then()
 			.wait();
 		return this;
 	},
 	curveBy: function(c1x, c1y, c2x, c2y, x, y){
 		var dx = this.relPos.x; var dy = this.relPos.y;
 		return this.curveTo(c1x + dx, c1y + dy, c2x + dx, c2y + dy, x + dx, y + dy);
 	},
 	curveTo: function(c1x, c1y, c2x, c2y, x, y){
 		// discretize
 		var data = this.bezierSamples(
 			new Point(this.relPos.x, this.relPos.y),
 			new Point(c1x, c1y),
 			new Point(c2x, c2y),
 			new Point(x, y)
 		);
 		this.comment("curveTo: len=" + data.length + ", N=" + data.points.length);
 		// draw segments
 		for(var i = 1; i < data.points.length; ++i){
 			this.lineTo(data.points[i].x, data.points[i].y);
 			if(i < data.points.length - 1) this.end();
 		}
 		// set new relative point
 		this.relPos = new Point(x, y);
 		this.lastCtrl = new Point(c2x, c2y);
 		return this;
 	},
 	smoothCurveBy: function(c2x, c2y, x, y){
 		var dx = this.relPos.x; var dy = this.relPos.y;
 		return this.smoothCurveTo(c2x + dx, c2y + dx, x + dx, y + dy);
 	},
 	smoothCurveTo: function(c2x, c2y, x, y){
 		var c1 = null;
 		if(this.lastCtrl){
 			c1 = this.project(this.lastCtrl, this.relPos);
 		} else {
 			c1 = this.relPos;
 		}
 		return this.curveTo(c1.x, c1.y, c2x, c2y, x, y);
 	},
 	quadBy: function(cx, cy, x, y){
 		var dx = this.relPos.x; var dy = this.relPos.y;
 		return this.quadTo(cx + dx, cy + dy, x + dx, y + dy);
 	},
 	quadTo: function(cx, cy, x, y){
 		// discretize
 		var data = this.bezierSamples(
 			new Point(this.relPos.x, this.relPos.y),
 			new Point(cx, cy),
 			new Point(x, y)
 		);
 		this.comment("quadTo: len=" + data.length + ", N=" + data.points.length);
 		// draw segments
 		for(var i = 1; i < data.points.length; ++i){
 			this.lineTo(data.points[i].x, data.points[i].y);
 			if(i < data.points.length - 1) this.end();
 		}
 		// set new relative point
 		this.relPos = new Point(x, y);
 		this.lastCtrl = new Point(cx, cy);
 		return this;
 	},
 	smoothQuadBy: function(x, y){
 		var dx = this.relPos.x; var dy = this.relPos.y;
 		return this.smoothQuadTo(x + dx, y + dy);
 	},
 	smoothQuadTo: function(x, y){
 		var c = null;
 		if(this.lastCtrl){
 			c = this.project(this.lastCtrl, this.relPos);
 		} else {
 			c = this.relPos;
 		}
 		return this.quadTo(c.x, c.y, x, y);
 	},
 	and: function(){
 		this.code += ', ';
 		return this;
 	},
 	end: this.then = function(){
 		this.code += '\n';
 		return this;
 	},
 	then: function(){
 		return this.end();
 	},
 	comment: function(str) {
 		this.code += '# ' + str + '\n'; 
 		return this;
 	}
 };
 
 function svg2path(root, scale){
 	if(!scale) scale = 100;
 	
 	var path = new Path();
 	
 	// frame
 	var dec = function(v){
 		return Math.round(parseFloat(v) * scale);
 	};
 	var coord = function(node, val){
 		return dec(node.attr(val));
 	};
 	
 	var parse = {
 		transform: function(node) {
 			path.storeContext();
 			// change context matrix
 			var T = node.prop('transform');
 			if(!T || !T.baseVal)
 				return;
 			for(var i = 0; i < T.baseVal.length; ++i){
 				var m = T.baseVal[i].matrix;
 				var M = new Matrix(
 					m.a, m.b, m.c, m.d, m.e, m.f
 				);
 				path.context = path.context.concat(M);
 			}
 		},
 		untransform: function() {
 			path.releaseContext();
 		},
 		g: function(node){
 			path.comment('g');
 			// parse the children
 			parse.children(node);
 		},
 		rect: function(node){
 			var x = coord(node, 'x');
 			var y = coord(node, 'y');
 			var w = coord(node, 'width');
 			var h = coord(node, 'height');
 			path.comment('rect ' + x + ' ' + y + ' ' + w + ' ' + h);
 			path.moveTo(x, y)
 			 	.then().lineBy(w, 0)
 			 	.then().lineBy(0, h)
 			 	.then().lineBy(-w, 0)
 			 	.then().lineBy(0, -h)
 			 	.end();
 		},
 		circ: function(node){
 			path.comment('circ');
 		},
 		path: function(node){
 			path.comment('path');
 			// stack for error cases
 			var error = false;
 			path.storeCode();
 			
 			// process path
 			var data = node.attr('d').replace(/[a-zA-Z]/, function(x){
 				return ' ' + x + ' '; // isolate command letters
 			}).trim() // remove heading and trailing spaces
 			  .replace(/[, ]+/g, ' '); // remove duplicate spaces, replace commas
 			path.comment('Data: ' + data);
 			var firstPos = null;
 			var mode = null;
 			var tokens = data.split(' ');
 			var i = 0;
 			while(i < tokens.length && !error){
 				switch(tokens[i]){
 					case ' ': ++i; break;
 					case 'm':
 					case 'M':
 					case 'l':
 					case 'L':
 					case 'c':
 					case 'C':
 					case 's':
 					case 'S':
 					case 'q':
 					case 'Q':
 					case 't':
 					case 'T':
 					case 'a':
 					case 'A':
 						mode = tokens[i];
 						++i;
 						break;
 					case 'z':
 						path.lineTo(firstPos.x, firstPos.y).end();
 						++i;
 						break;
 					default:
 						var pos = new Point(dec(tokens[i]), dec(tokens[i+1])), q1, q2;
 						if(isNaN(pos.x) || isNaN(pos.y)){
 							console.log('Invalid position: ' + tokens[i] + ',' + tokens[i+1]);
 							error = true;
 						}
 						// potential control points
						q1 = new Point(dec(tokens[i+2]), dec(tokens[i+3])); // /!\ this may be invalid!
						q2 = new Point(dec(tokens[i+4]), dec(tokens[i+5])); //     same here...
						// always shift at least by 2
 						i += 2;
 						switch(mode){
 							// move
 							case 'm': path.moveBy(pos.x, pos.y).end(); if(!firstPos) mode = 'L'; break;
 							case 'M': path.moveTo(pos.x, pos.y).end(); if(!firstPos) mode = 'L'; break;
 							// line
 							case 'l': path.lineBy(pos.x, pos.y).end(); break;
 							case 'L': path.lineTo(pos.x, pos.y).end(); break;
 							// cubic bezier
 							case 'C': path.curveTo(pos.x, pos.y, q1.x, q1.y, q2.x, q2.y).end(); break;
 							case 'c': path.curveBy(pos.x, pos.y, q1.x, q1.y, q2.x, q2.y).end(); break;
 							case 'S': path.smoothCurveTo(pos.x, pos.y, q1.x, q1.y).end(); break;
 							case 's': path.smoothCurveBy(pos.x, pos.y, q1.x, q1.y).end(); break;
 							// quadratic bezier
 							case 'Q': path.quadTo(pos.x, pos.y, q1.x, q1.y).end(); break;
 							case 'q': path.quadBy(pos.x, pos.y, q1.x, q1.y).end(); break;
 							case 'T': path.smoothQuadTo(pos.x, pos.y).end(); break;
 							case 't': path.smoothQuadBy(pos.x, pos.y).end(); break;
 							// elliptical arc
 							case 'A':
 							case 'a': // @see http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
 							// invalid mode
 							default:
 								// not supported
 								console.log("Path command '" + mode + "' not supported!");
 								error = true;
 								break;
 						}
 						if(!firstPos){
 							firstPos = path.currentPos(); // /!\ context position, not real location
 						}
 						break;
 				}
 			}
 			// release / restoration
 			if(error){
 				path.restoreCode();
 			} else {
 				path.releaseCode();
 			}
 		},
 		polygon: function(node){
 			return parse.polyline(node, true);
 		},
 		polyline: function(node, polygon){
 			path.comment(polygon ? 'Polygon' : 'Polyline');
 			
 			// stack for error cases
 			var error = false;
 			path.storeCode();
 			
 			var tokens = node.attr('points').replace(/[, ]+/g, ' ').trim().split(' ');
 			var last = null;
 			if(tokens.length % 2){
 				console.log('Odd polyline data!');
 				error = true;
 			}
 			var firstPos = null;
 			for(var i = 0; i < tokens.length && !error; i += 2){
 				var pos = new Point(dec(tokens[i]), dec(tokens[i+1]));
				if(isNaN(pos.x) || isNaN(pos.y)){
					console.log('Invalid position: ' + tokens[i] + ',' + tokens[i+1]);
					error = true;
				}
				if(i == 0){
					firstPos = pos;
					path.moveTo(pos.x, pos.y);
 				}else
 					path.lineTo(pos.x, pos.y);
 			}
 			// close polygon
 			if(polygon && !error){
 				path.lineTo(firstPos.x, firstPos.y);
 			}
 			if(error){
 				path.restoreCode();
 			} else {
 				path.releaseCode();
 			}
 		},
 		child: function(node){
 			var tag = node.prop('tagName').toLowerCase();
 			if(tag in parse){
 				console.log('Entering <%s>\n', tag);
 				parse.transform(node);
 				parse[tag](node);
 				parse.untransform(node);
 			} else {
 				path.comment(tag + ' not supported.');
 				console.log('Skipping <%s>\n', tag);
 			}
 		},
 		children: function(node){
 			node.children().each(function(){
 				var child = $(this);
 				parse.child(child);
 			});
 		}
 	};
 	// parse the children
 	parse.children(root);
 	
 	// return the generated code
 	return path.code;
 }
 