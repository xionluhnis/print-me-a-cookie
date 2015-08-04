/**
 * SVG to Path conversion
 *
 * @author Alexandre Kaspar <akaspar@mit.edu>
 */
function svg2path(root, params){
  if(!params) params = {};
  if(!params.scale) params.scale = 100;
  
  // path driver
  var path = new Path('SVG to Path');
  
  // frame
  var dec = function(v){
    return Math.round(parseFloat(v) * params.scale);
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
              case 'm': path.moveBy(pos.x, pos.y).end(); if(!firstPos) mode = 'l'; break;
              case 'M': path.moveTo(pos.x, pos.y).end(); if(!firstPos) mode = 'L'; break;
              // line
              case 'l': path.lineBy(pos.x, pos.y).end(); break;
              case 'L': path.lineTo(pos.x, pos.y).end(); break;
              // cubic bezier
              case 'C': path.curveTo(pos.x, pos.y, q1.x, q1.y, q2.x, q2.y).end(); i += 4; break;
              case 'c': path.curveBy(pos.x, pos.y, q1.x, q1.y, q2.x, q2.y).end(); i += 4; break;
              case 'S': path.smoothCurveTo(pos.x, pos.y, q1.x, q1.y).end(); i += 2; break;
              case 's': path.smoothCurveBy(pos.x, pos.y, q1.x, q1.y).end(); i += 2; break;
              // quadratic bezier
              case 'Q': path.quadTo(pos.x, pos.y, q1.x, q1.y).end(); i += 2; break;
              case 'q': path.quadBy(pos.x, pos.y, q1.x, q1.y).end(); i += 2; break;
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
              firstPos = path.currentPosition(); // /!\ context position, not real location
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
  return path;
}
 
