/**
 * Path driver to produce PTH commands
 *
 * @author Alexandre Kaspar <akaspar@mit.edu>
 */
function Path(welcome){
	this.code = '';
	if(welcome)
		this.comment(welcome);
	else
		this.comment('Personalized path');
 	
 	// geometry context
 	this.context = new Matrix();
 	this.lastLoc = new Point(0, 0); // absolute location (no context)
 	this.relPos = new Point(0, 0);  // context-dependent relative position
 	this.lastZ = this.relZ = 0;
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
 		// 1 = create bezier curve
 		var curve = cubic ? new Bezier([p1, c1, c2, p2]) : new Bezier([p1, c1, c2]);
 		// 2 = sample points
 		var len = curve.length();
 		var N = Math.max(5, Math.ceil(len / 16.0));
 		var origPoints = curve.getLUT(N);
 		// 3 = simplify polyline
 		var tolerance = 1.0;
 		var points = simplify(origPoints, tolerance, true);
 		// done! be happy!
 		return {
 			curve: curve,
 			length: len,
 			points: points
 		};
 	},
 	
// getter command
	currentPos: function(){
		return this.context.transformPoint(this.relPos);
	},
	currentZ: function(){
		return this.relZ;
	},
	
// path commands
	__command: function(){
		for(var i = 0; i < arguments.length; ++i){
			this.code += (i > 0 ? ' ' : '') + arguments[i];
		}
		return this;
	},
 	
// javascript commands
	resetPosition: function(x, y, z){
		this.__command('sxp', x)
				.and
				.__command('syp', y)
				.and
				.__command('szp', z).end();
		this.relPos = new Point(x || 0, y || 0);
		this.relZ = z || 0;
		return this; 
	},
	shift: function(){
 		var newLoc = this.context.transformPoint(this.relPos); // compute new absolute location
 		var delta = newLoc.sub(this.lastLoc).round(); 		   // compute relative shift in absolute location
 		if(delta.x != 0 || delta.y != 0){
 			this.__command('m', delta.x, delta.y);
 			this.lastLoc = newLoc;
 		}
 		var deltaZ = this.relZ - this.lastZ;
 		if(deltaZ != 0){
 			this.__command('z', deltaZ);
 			this.lastZ = this.relZ;
 		}
 		return this;
 	},
 	moveBy: function(x, y){
 		return this.moveTo(this.relPos.x + x, this.relPos.y + y);
 	},
 	moveTo: function(x, y){
 		this.relPos = new Point(x, y);
 		this.lastCtrl = null;
 		return this.shift();
 	},
 	elevateBy: function(dz){
 		return this.elevateTo(this.relZ + dz);
 	},
 	elevateTo: function(z){
 		this.relZ = z;
 		this.lastCtrl = null;
 		return this.shift();
 	},
 	extrude: function(speed){
 		this.code += 'e ' + speed;
 		return this;
 	},
 	wait: function(t){
 		if(!t) t = 500;
 		this.code += 'w ' + t;
 		return this;
 	},
 	longWait: function(t){
 		if(!t) t = 1;
 		this.code += 'W ' + t;
 		return this;
 	},
 	lineBy: function(x, y){
 		return this.lineTo(this.relPos.x + x, this.relPos.y + y);
 	},
 	_lineTo: function(x, y){
 		var loc = this.lastLoc;
 		this.moveTo(x, y).and();
 		var delta = this.lastLoc.sub(loc).abs();
 		var speed = Math.ceil(Math.max(delta.x, delta.y) / 2);
 		this.extrude(speed, 10).end();
 		return this;
 	},
 	lineTo: function(x, y){
 		return this._lineTo(x, y).then().wait();
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
 			this._lineTo(data.points[i].x, data.points[i].y);
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
 			this._lineTo(data.points[i].x, data.points[i].y);
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
