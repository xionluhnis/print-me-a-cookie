/**
 * SVG to Path conversion
 *
 * @author Alexandre Kaspar <akaspar@mit.edu>
 */
 
 function Path(){
 	this.code = '# SVG2Path \n';
 	
 	// geometry context
 	this.context = new Matrix();
 	this.lastPos = new Point(0, 0);
 	
 	// stacks
 	this.contextStack = [];
 	this.codeStack = [];
 }
 
 Path.prototype = {
 // context and stacks
 	storeContext: function(){
 		this.contextStack.push(this.context.clone());
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
 	
// getter command
	currentPos: function(){
		return new Point(this.context.tx, this.context.ty);
	},
 	
// path commands
 	m_command: function(dx, dy, dz, sx, sy, sz){
 		if(!dz) dz = 0;
 		this.code += 'm ' + dx + ' ' + dy + ' ' + dz;
 		if(sx) code += ' ' + sx;
 		if(sy) code += ' ' + sy;
 		if(sz) code += ' ' + sz;
 	},
 	
// javascript commands
	shift: function(){
 		var newPos = new Point(this.context.tx, this.context.ty);
 		var delta = newPos.sub(this.lastPos).round();
 		if(delta.x != 0 || delta.y != 0){
 			this.m_command(delta.x, delta.y);
 			this.lastPos = newPos;
 		}
 		return this;
 	},
 	moveBy: function(x, y){
 		this.context = this.context.translate(x, y);
 		return this.shift();
 	},
 	moveTo: function(x, y){
 		this.context.tx = x;
 		this.context.ty = y;
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
 		var pos = this.lastPos;
 		this.moveBy(x, y).and();
 		var delta = this.lastPos.sub(pos).abs();
 		var speed = Math.ceil(Math.max(delta.x, delta.y) / 2);
 		this.extrude(speed, 10)
        .then()
        .wait();
 		return this;
 	},
 	lineTo: function(x, y){
 		var pos = this.lastPos;
 		this.moveTo(x, y).and();
 		var delta = this.lastPos.sub(pos).abs();
 		var speed = Math.ceil(Math.max(delta.x, delta.y) / 2);
 		this.extrude(speed, 10)
        .then()
        .wait();
 		return this;
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
 		return Math.ceil(parseFloat(v) * scale);
 	};
 	var coord = function(node, val){
 		return dec(node.attr(val));
 	};
 	
 	var parse = {
 		transform: function(node) {
 			path.storeContext();
 			// TODO change context matrix
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
 			path.storeContext();
 			path.storeCode();
 			
 			// process path
 			var data = node.attr('d').replace(/[, ]+/g, ' ');
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
 						mode = tokens[i];
 						++i;
 						break;
 					case 'z':
 						path.lineTo(firstPos.x, firstPos.y).end();
 						++i;
 						break;
 					default:
 						var pos = new Point(dec(tokens[i]), dec(tokens[i+1]));
 						if(isNaN(pos.x) || isNaN(pos.y)){
 							console.log('Invalid position');
 						}
 						i += 2;
 						switch(mode){
 							case 'm': path.moveBy(pos.x, pos.y).end(); if(!firstPos) mode = 'L'; break;
 							case 'M': path.moveTo(pos.x, pos.y).end(); if(!firstPos) mode = 'L'; break;
 							case 'l': path.lineBy(pos.x, pos.y).end(); break;
 							case 'L': path.lineTo(pos.x, pos.y).end(); break;
 							default:
 								// not supported
 								console.log("Path command '" + mode + "' not supported!");
 								error = true;
 								break;
 						}
 						if(!firstPos){
 							firstPos = path.currentPos();
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
 			path.releaseContext();
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
 
