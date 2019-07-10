// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
'use strict';

exports.vue_mixins = {};

/// Loop over all properties in `source` and assign to `target*
function assign_forin (target, source) {
  for (let p in source)
    target[p] = source[p];
  return target;
}
exports.assign_forin = assign_forin;

/// Loop over all elements of `source` and assign to `target*
function assign_forof (target, source) {
  for (let e of source)
    target[e] = source[e];
  return target;
}
exports.assign_forof = assign_forof;

/// Yield a wrapper that delays calling `callback` until `delay` miliseconds have passed since the last wrapper call.
function debounce (delay, callback) {
  if (!(callback instanceof Function))
    throw new TypeError ('argument `callback` must be of type Function');
  let ctimer, cthis, cargs, creturn; // closure variables
  function invoke_callback() {
    ctimer = undefined;
    const invoke_this = cthis, invoke_args = cargs;
    cthis = undefined;
    cargs = undefined;
    creturn = callback.apply (invoke_this, invoke_args);
    return undefined;
  }
  function cwrapper (...args) {
    cthis = this;
    cargs = args;
    if (ctimer != undefined)
      clearTimeout (ctimer);
    ctimer = setTimeout (invoke_callback, delay);
    const last_return = creturn;
    creturn = undefined;
    return last_return;
  }
  return cwrapper;
}
exports.debounce = debounce;

/** Remove element `item` from `array` */
function array_remove (array, item) {
  for (let i = 0; i < array.length; i++)
    if (item === array[i]) {
      array.splice (i, 1);
      break;
    }
  return array;
}
exports.array_remove = array_remove;

/// Generate map by splittoing the key=value pairs in `kvarray`
function map_from_kvpairs (kvarray) {
  let o = {};
  for (let kv of kvarray) {
    const key = kv.split ('=', 1)[0];
    const value = kv.substr (key.length + 1);
    o[key] = value;
  }
  return o;
}
exports.map_from_kvpairs = map_from_kvpairs;

/** Generate integers [0..`bound`[ if one arg is given or [`bound`..`end`[ by incrementing `step`. */
function* range (bound, end, step = 1) {
  if (end === undefined) {
    end = bound;
    bound = 0;
  }
  for (; bound < end; bound += step)
    yield bound;
}
exports.range = range;

/** Return @a x clamped into @a min and @a max. */
function clamp (x, min, max) {
  return x < min ? min : x > max ? max : x;
}
exports.clamp = clamp;

/** Vue mixin to allow automatic `data` construction (cloning) from `data_tmpl` */
exports.vue_mixins.data_tmpl = {
  beforeCreate: function () {
    // Automatically create `data` (via cloning) from `data_tmpl`
    if (this.$options.data_tmpl)
      this.$options.data = Object.assign ({}, this.$options.data_tmpl,
					  typeof this.$options.data === 'function' ?
					  this.$options.data.call (this) :
					  this.$options.data);
  },
};

/** Vue mixin to create a kebab-case ('two-words') getter proxy for camelCase ('twoWords') props */
exports.vue_mixins.hyphen_props = {
  beforeCreate: function () {
    for (let cc in this.$options.props) {
      const hyphenated = hyphenate (cc);
      if (hyphenated === cc || (this.$options.computed && hyphenated in this.$options.computed))
	continue;
      if (!this.$options.computed)
	this.$options.computed = {};
      Object.defineProperty (this, hyphenated, {
	get() { return this[cc]; },
	enumerable: false,
	configurable: false
      });
    }
  },
};

/** Generate a kebab-case ('two-words') identifier from a camelCase ('twoWords') identifier */
function hyphenate (string) {
  const uppercase_boundary =  /\B([A-Z])/g;
  return string.replace (uppercase_boundary, '-$1').toLowerCase();
}
exports.hyphenate = hyphenate;

/** Vue mixin to provide a `dom_updated` hook.
 * This mixin is a bit of a sledge hammer, usually it's better to have something
 * similar to a paint_canvas() method and just use $watch (this.paint_canvas);
 * to update automatically.
 * A dom_updated() method is only really needed to operate on initialized $refs[].
 */
exports.vue_mixins.dom_updated = {
  mounted: function () { this.dom_updated(); },
  updated: function () { this.dom_updated(); },
};

/** VueifyObject - turn a regular object into a Vue instance.
 * The *object* passed in is used as the Vue `data` object. Properties
 * with a getter (and possibly setter) are turned into Vue `computed`
 * properties, methods are carried over as `methods` on the Vue() instance.
 */
exports.VueifyObject = function (object = {}, vue_options = {}) {
  let voptions = Object.assign ({}, vue_options);
  voptions.methods = vue_options.methods || {};
  voptions.computed = vue_options.computed || {};
  voptions.data = voptions.data || object;
  const proto = object.__proto__;
  for (const pname of Object.getOwnPropertyNames (proto)) {
    if (typeof proto[pname] == 'function' && pname != 'constructor')
      voptions.methods[pname] = proto[pname];
    else
      {
	const pd = Object.getOwnPropertyDescriptor (proto, pname);
	if (pd.get)
	  voptions.computed[pname] = pd;
      }
  }
  return new Vue (voptions);
};

/** Copy PropertyDescriptors from source to target, optionally binding handlers against closure. */
exports.clone_descriptors = (target, source, closure) => {
  // See also: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign completeAssign()
  const descriptors = Object.getOwnPropertyNames (source).reduce ((descriptors, pname) => {
    const pd = Object.getOwnPropertyDescriptor (source, pname);
    if (pname != 'constructor')
      {
	if (closure)
	  {
	    if (pd.get)
	      pd.get = pd.get.bind (closure);
	    if (pd.set)
	      pd.set = pd.set.bind (closure);
	    if (typeof pd.value == 'function')
	      pd.value = pd.value.bind (closure);
	  }
	descriptors[pname] = pd;
      }
    return descriptors;
  }, {});
  Object.defineProperties (target, descriptors);
  // in constrast to Object.assign, we ignore getOwnPropertySymbols here
  return target;
  // Usage: clone_descriptors (window, fakewin.__proto__, fakewin);
};

/** Produce hash code from a String, using an FNV-1a variant. */
exports.fnv1a_hash = (str) => {
  let hash = 0x811c9dc5;
  for (let i = 0; i < str.length; ++i) {
    // Note, charCodeAt can return values > 255
    const next = 0x1000193 * (hash ^ str.charCodeAt (i));
    hash = next >>> 0; // % 4294967296, i.e. treat as 32 bit unsigned
  }
  return hash;
};

/** Split a string when encountering a comma, while preserving quoted or parenthesized segments. */
exports.split_comma = (str) => {
  let result = [], item = '', sdepth = 0, ddepth = 0, pdepth = 0, kdepth = 0, cdepth = 0, bslash = 0;
  for (let i = 0; i < str.length; i++) {
    const c = str[i];
    if (c === ',' && 0 == (sdepth + ddepth + pdepth + kdepth + cdepth)) {
      if (item) {
	result.push (item);
	item = '';
      }
    } else {
      item += c;
      if (c === '[') kdepth++;
      if (c === ']' && kdepth) kdepth--;
      if (c === '(') pdepth++;
      if (c === ')' && pdepth) pdepth--;
      if (c === '{') cdepth++;
      if (c === '}' && cdepth) cdepth--;
      if (c === "'" && !bslash) sdepth = !sdepth;
      if (c === '"' && !bslash) ddepth = !ddepth;
    }
    bslash = !bslash && c === '\\' && (sdepth || ddepth);
  }
  if (item)
    result.push (item);
  return result;
};

/** Parse hexadecimal CSS color with 3 or 6 digits into [ R, G, B ]. */
function parse_hex_color (colorstr) {
  if (colorstr.substr (0, 1) == '#') {
    let hex = colorstr.substr (1);
    if (hex.length == 3)
      hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
    return [ parseInt (hex.substr (0, 2), 16),
	     parseInt (hex.substr (2, 2), 16),
	     parseInt (hex.substr (4, 2), 16) ];
  }
  return undefined;
}
exports.parse_hex_color = parse_hex_color;

/** Parse hexadecimal CSS color into luminosity. */
// https://en.wikipedia.org/wiki/Relative_luminance
function parse_hex_luminosity (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
}
exports.parse_hex_luminosity = parse_hex_luminosity;

/** Parse hexadecimal CSS color into brightness. */
// https://www.w3.org/TR/AERT/#color-contrast
function parse_hex_brightness (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2];
}
exports.parse_hex_brightness = parse_hex_brightness;

/** Parse hexadecimal CSS color into perception corrected grey. */
// http://alienryderflex.com/hsp.html
function parse_hex_pgrey (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return Math.sqrt (0.299 * rgb[0] * rgb[0] + 0.587 * rgb[1] * rgb[1] + 0.114 * rgb[2] * rgb[2]);
}
exports.parse_hex_pgrey = parse_hex_pgrey;

/** Parse hexadecimal CSS color into average grey. */
function parse_hex_average (colorstr) {
  const rgb = parse_hex_color (colorstr);
  return 0.3333 * rgb[0] + 0.3333 * rgb[1] + 0.3333 * rgb[2];
}
exports.parse_hex_average = parse_hex_average;

/** Parse CSS colors (via invisible DOM element) and yield an array of rgba tuples. */
function parse_colors (colorstr) {
  let result = [];
  if (!parse_colors.span) {
    parse_colors.span = document.createElement ('span');
    parse_colors.span.style.display = 'none';
    document.body.appendChild (parse_colors.span);
  }
  for (const c of exports.split_comma (colorstr)) {
    parse_colors.span.style.color = c;
    const style = getComputedStyle (parse_colors.span);
    let m = style.color.match (/^rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);
    if (m)
      result.push ([m[1], m[2], m[3], 1]);
    else {
      m = style.color.match (/^rgba\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)\s*\)$/i);
      if (m)
	result.push ([m[1], m[2], m[3], m[4]]);
    }
  }
  return result;
}
exports.parse_colors = parse_colors;

/** Retrieve a new object with the properties of `obj` resolved against the style of `el` */
function compute_style_properties (el, obj) {
  const style = getComputedStyle (el);
  let props = {};
  for (let key in obj) {
    const result = style.getPropertyValue (obj[key]);
    if (result !== undefined)
      props[key] = result;
  }
  return props;
}
exports.compute_style_properties = compute_style_properties;

const modal_mouse_guard = function (ev) {
  for (let shield of document._vc_modal_shields)
    if (!ev.cancelBubble) {
      if (ev.target == shield) {
	ev.preventDefault();
	ev.stopPropagation();
	shield.destroy();
      }
    }
};

const modal_keyboard_guard = function (ev) {
  const ESCAPE = 27;
  for (let shield of document._vc_modal_shields)
    if (!ev.cancelBubble) {
      if (event.keyCode == ESCAPE) {
	ev.preventDefault();
	ev.stopPropagation();
	shield.destroy();
      }
    }
};

/** Add a modal overlay to \<body/>, prevent DOM clicks and focus movements */
function modal_shield (close_handler, preserve_element) {
  // prevent focus during modal shield
  const focus_guard = install_focus_guard (preserve_element);
  // keep a shield list and handle keyboard / mouse events on the shield
  if (!(document._vc_modal_shields instanceof Array)) {
    document._vc_modal_shields = [];
    document.addEventListener ('mousedown', modal_mouse_guard);
    document.addEventListener ('keydown', modal_keyboard_guard);
  }
  // install shield element on <body/>
  const shield = document.createElement ("div");
  document._vc_modal_shields.unshift (shield);
  shield.style = 'display: flex; position: fixed; z-index: 90; left: 0; top: 0; width: 100%; height: 100%;' +
		 'background-color: rgba(0,0,0,0.60);';
  document.body.appendChild (shield);
  // destroying the shield
  shield.destroy = function (call_handler = true) {
    if (shield.parentNode)
      shield.parentNode.removeChild (shield);
    array_remove (document._vc_modal_shields, shield);
    focus_guard.restore();
    if (close_handler && call_handler) {
      const close_handler_once = close_handler;
      close_handler = undefined; // guard against recursion
      close_handler_once();
    }
  };
  return shield;
}
exports.modal_shield = modal_shield;

/** Recursively prevent `node` from being focussed */
const prevent_focus = (array, node, preserve) => {
  if (node == preserve)
    return;
  if (node.tabIndex > -1)
    {
      if (node._vc_focus_guard > 0)
	node._vc_focus_guard += 1;
      else {
	node._vc_focus_guard = 1;
	node._vc_focus_guard_tabIndex = node.tabIndex;
	node.tabIndex = -1;
      }
      array.push (node);
    }
  else if (node._vc_focus_guard > 0)
    {
      node._vc_focus_guard += 1;
      array.push (node);
    }
  if (node.firstChild)
    prevent_focus (array, node.firstChild, preserve);
  if (node.nextSibling)
    prevent_focus (array, node.nextSibling, preserve);
};

/** Restore `node`s focus ability when the last focus guard is destroyed */
const restore_focus = (node) => {
  if (node._vc_focus_guard > 0) {
    node._vc_focus_guard -= 1;
    if (node._vc_focus_guard == 0) {
      const tabIndex = node._vc_focus_guard_tabIndex;
      delete node._vc_focus_guard_tabIndex;
      delete node._vc_focus_guard;
      node.tabIndex = tabIndex;
    }
  }
};

/** Prevent all DOM elements from getting focus.
 * Preseve focus ability for `preserve_element` and its descendants.
 * Returns a `guard` object on which guard.restore() must be called to
 * restore the DOM elements.
 */
function install_focus_guard (preserve_element) {
  const guard = {
    elements: [],
    restore: () => {
      if (!guard.elements)
	return;
      for (let node of guard.elements)
	restore_focus (node);
      guard.elements = undefined;
      if (guard.last_focus)
	guard.last_focus.focus();
      guard.last_focus = undefined;
    },
  };
  // save last focussed element
  guard.last_focus = document.activeElement;
  // disable focusable elements outside of preserve_element
  prevent_focus (guard.elements, document, preserve_element);
  // remove focus if the current focus is a guarded element
  if (document.activeElement && document.activeElement._vc_focus_guard > 0)
    document.activeElement.blur();
  return guard;
}

/** Resize canvas display size (CSS size) and resize backing store to match hardware pixels */
exports.resize_canvas = function (canvas, csswidth, cssheight, fill_style = false) {
  /* Here we fixate the canvas display size at (csswidth,cssheight) and then setup the
   * backing store to match the hardware screen pixel size.
   * Note, just assigning canvas.width *without* assigning canvas.style.width may lead to
   * resizes in the absence of other constraints. So to render at screen pixel size, we
   * always have to assign canvas.style.{width|height}.
   */
  const cw = Math.round (csswidth), ch = Math.round (cssheight);
  const pw = Math.round (window.devicePixelRatio * cw);
  const ph = Math.round (window.devicePixelRatio * ch);
  if (cw != canvas.style.width || ch != canvas.style.height ||
      pw != canvas.width || ph != canvas.height || fill_style) {
    canvas.style.width = cw + 'px';
    canvas.style.height = ch + 'px';
    canvas.width = pw;
    canvas.height = ph;
    const ctx = canvas.getContext ('2d');
    if (!fill_style || fill_style === true)
      ctx.clearRect (0, 0, canvas.width, canvas.height);
    else {
      ctx.fillStyle = fill_style;
      ctx.fillRect (0, 0, canvas.width, canvas.height);
    }
    return true;
  }
  return false;
};

/** Draw a horizontal line from (x,y) of width `w` with dashes `d` */
exports.dash_xto = (ctx, x, y, w, d) => {
  for (let i = 0, p = x; p < x + w; p = p + d[i++ % d.length]) {
    if (i % 2)
      ctx.lineTo (p, y);
    else
      ctx.moveTo (p, y);
  }
};

/** Draw a horizontal rect `(x,y,width,height)` with pixel gaps of width `stipple` */
exports.hstippleRect = function (ctx, x, y, width, height, stipple) {
  for (let s = x; s + stipple < x + width; s += 2 * stipple)
    ctx.fillRect (s, y, stipple, height);
};

/** Fill and stroke a canvas rectangle with rounded corners. */
exports.roundRect = (ctx, x, y, width, height, radius, fill = true, stroke = true) => {
  if (typeof radius === 'number')
    radius = [ radius, radius, radius, radius ];
  else if (typeof radius === 'object' && radius.length == 4)
    ; // top-left, top-right, bottom-right, bottom-left
  else
    throw new Error ('invalid or missing radius');
  ctx.beginPath();
  ctx.moveTo           (x + radius[0],         y);
  ctx.lineTo           (x + width - radius[1], y);
  ctx.quadraticCurveTo (x + width,             y,                      x + width,             y + radius[1]);
  ctx.lineTo           (x + width,             y + height - radius[2]);
  ctx.quadraticCurveTo (x + width,             y + height,             x + width - radius[2], y + height);
  ctx.lineTo           (x + radius[3],         y + height);
  ctx.quadraticCurveTo (x,                     y + height,             x,                     y + height - radius[3]);
  ctx.lineTo           (x,                     y + radius[0]);
  ctx.quadraticCurveTo (x,                     y,                      x + radius[0],         y);
  ctx.closePath();
  if (fill)
    ctx.fill();
  if (stroke)
    ctx.stroke();
};

/** Add color stops from `stoparray` to `grad`, `stoparray` is an array: [(offset,color)...] */
function gradient_apply_stops (grad, stoparray) {
  for (const g of stoparray)
    grad.addColorStop (g[0], g[1]);
}
exports.gradient_apply_stops = gradient_apply_stops;

/** Create a new linear gradient at (x1,y1,x2,y2) with color stops `stoparray` */
function linear_gradient_from (ctx, stoparray, x1, y1, x2, y2) {
  const grad = ctx.createLinearGradient (x1, y1, x2, y2);
  gradient_apply_stops (grad, stoparray);
  return grad;
}
exports.linear_gradient_from = linear_gradient_from;

/** Measure ink span of a canvas text string or an array */
function canvas_ink_vspan (font_style, textish = 'gM') {
  let canvas, ctx, cwidth = 64, cheight = 64;
  function measure_vspan (text) {
    const cache_key = font_style + ':' + text;
    let result = canvas_ink_vspan.cache[cache_key];
    if (!result)
      {
	if (canvas === undefined) {
	  canvas = document.createElement ('canvas');
	  ctx = canvas.getContext ('2d');
	}
	/* BUG: electron-1.8.3 (chrome-59) is unstable (shows canvas memory
	 * corruption) at tiny zoom levels without canvas size assignments.
	 */
	const text_em = ctx.measureText ("MW").width;
	const twidth = Math.max (text_em * 2, ctx.measureText (text).width);
	cwidth = Math.max (cwidth, 2 * twidth);
	cheight = Math.max (cheight, 3 * text_em);
	canvas.width = cwidth;
	canvas.height = cheight;
	ctx.fillStyle = '#000000';
	ctx.fillRect (0, 0, canvas.width, canvas.height);
	ctx.font = font_style;
	ctx.fillStyle = '#ffffff';
	ctx.textBaseline = 'top';
	ctx.fillText (text, 0, 0);
	const pixels = ctx.getImageData (0, 0, canvas.width, canvas.height).data;
	let start = -1, end = -1;
	for (let row = 0; row < canvas.height; row++)
	  for (let col = 0; col < canvas.width; col++) {
	    let index = (row * canvas.width + col) * 4; // RGBA
	    if (pixels[index + 0] > 0) {
	      if (start < 0)
		start = row;
	      else
		end = row;
	      break;
	    }
	  }
	result = start >= 0 && end >= 0 ? [start, end - start] : [0, 0];
	canvas_ink_vspan.cache[cache_key] = result;
      }
    return result;
  }
  return Array.isArray (textish) ? textish.map (txt => measure_vspan (txt)) : measure_vspan (textish);
}
canvas_ink_vspan.cache = [];
exports.canvas_ink_vspan = canvas_ink_vspan;

/** Retrieve the 'C-1' .. 'G8' label for midi note numbers */
function midi_label (numish) {
  function one_label (num) {
    const letter = [ 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B' ];
    const oct = Math.floor (num / letter.length) - 1;
    const key = num % letter.length;
    return letter[key] + oct;
  }
  return Array.isArray (numish) ? numish.map (n => one_label (n)) : one_label (numish);
}
exports.midi_label = midi_label;

let frame_handler_id = 0x200000,
    frame_handler_active = false,
    frame_handler_callback_id = undefined,
    frame_handler_cur = 0,
    frame_handler_max = 0,
    frame_handler_array = undefined;

function call_frame_handlers () {
  const active = frame_handler_active;
  frame_handler_max = frame_handler_array.length;
  for (frame_handler_cur = 0; frame_handler_cur < frame_handler_max; frame_handler_cur++) {
    const handler_id = frame_handler_array[frame_handler_cur][1];
    const handler_result = frame_handler_array[frame_handler_cur][0] (active);
    if (handler_result !== undefined && !handler_result)
      remove_frame_handler (handler_id);
  }
  if (frame_handler_active)
    frame_handler_callback_id = window.requestAnimationFrame (call_frame_handlers);
}

function reinstall_frame_handler() {
  if (frame_handler_callback_id !== undefined) {
    window.cancelAnimationFrame (frame_handler_callback_id);
    frame_handler_callback_id = undefined;
  }
  if (frame_handler_active)
    frame_handler_callback_id = window.requestAnimationFrame (call_frame_handlers);
  else
    call_frame_handlers(); // call one last time with frame_handler_active == false
}

function remove_frame_handler (handler_id) {
  for (let i = frame_handler_array.length - 1; i >= 0; i--)
    if (frame_handler_array[i][1] == handler_id) {
      frame_handler_array.splice (i, 1);
      if (i < frame_handler_cur)
	frame_handler_cur--;
      frame_handler_max--;
      return;
    }
  console.log ("remove_frame_handler(" + handler_id + "): invalid id");
}

/// Install a permanent redraw handler, to run as long as the DSP engine is active.
function add_frame_handler (handlerfunc) {
  if (frame_handler_array === undefined) { // must initialize
    frame_handler_active = Boolean (Bse.server.engine_active());
    frame_handler_array = [];
    Bse.server.on ('enginechange', (ev) => {
      frame_handler_active = Boolean (ev.active);
      reinstall_frame_handler();
    } );
  }
  const handler_id = frame_handler_id++;
  frame_handler_array.push ([handlerfunc, handler_id]);
  reinstall_frame_handler();
  return function() { remove_frame_handler (handler_id); };
}
exports.add_frame_handler = add_frame_handler;

let bse_server_shared_arrays = [];

/// Retrieve shared memory arrays from BSE shared memory ids.
function array_fields_from_shm (shm_id, shm_offset) {
  if (bse_server_shared_arrays[shm_id] === undefined) {
    const array_buffer = Bse.server.create_shared_memory_array_buffer (shm_id);
    console.assert (array_buffer.byteLength > 0);
    bse_server_shared_arrays[shm_id] = {
      'array_buffer':  array_buffer,
      'int32_array':   new Int32Array (array_buffer),
      'float32_array': new Float32Array (array_buffer),
      'float64_array': new Float64Array (array_buffer),
    };
  }
  console.assert ((shm_offset & 0xf) == 0);
  let afields = Object.assign ({}, bse_server_shared_arrays[shm_id]);
  afields.int32_offset = shm_offset / afields.int32_array.BYTES_PER_ELEMENT; // 4
  afields.float32_offset = shm_offset / afields.float32_array.BYTES_PER_ELEMENT; // 4
  afields.float64_offset = shm_offset / afields.float64_array.BYTES_PER_ELEMENT; // 8
  return afields;
}
exports.array_fields_from_shm = array_fields_from_shm;

/// Access int32 subarray within arrays returned from array_fields_from_shm().
function array_fields_i32 (afields, byte_offset) {
  console.assert ((byte_offset & 0x3) == 0); // 4 - 1
  return afields.int32_array.subarray (afields.int32_offset + byte_offset / afields.int32_array.BYTES_PER_ELEMENT);
}
exports.array_fields_i32 = array_fields_i32;

/// Access float32 subarray within arrays returned from array_fields_from_shm().
function array_fields_f32 (afields, byte_offset) {
  console.assert ((byte_offset & 0x3) == 0); // 4 - 1
  return afields.float32_array.subarray (afields.float32_offset + byte_offset / afields.float32_array.BYTES_PER_ELEMENT);
}
exports.array_fields_f32 = array_fields_f32;

/// Access float64 subarray within arrays returned from array_fields_from_shm().
function array_fields_f64 (afields, byte_offset) {
  console.assert ((byte_offset & 0x7) == 0); // 8 - 1
  return afields.float64_array.subarray (afields.float64_offset + byte_offset / afields.float64_array.BYTES_PER_ELEMENT);
}
exports.array_fields_f64 = array_fields_f64;

// Format window titles
function format_title (prgname, entity = undefined, infos = undefined, extras = undefined) {
  let title = prgname;
  if (entity)
    {
      let sub = entity;
      if (infos)
	{
	  sub += ' - ' + infos;
	  if (extras)
	    sub += ' (' + extras + ')';
	}
      title = sub + ' – ' + title;
    }
  return title;
}
exports.format_title = format_title;

let keyboard_click_state = { inclick: 0 };

/// Check if the current click event originates from keyboard activation.
function in_keyboard_click()
{
  return keyboard_click_state.inclick > 0;
}
exports.in_keyboard_click = in_keyboard_click;

/// Trigger elemtn click via keyboard.
function keyboard_click (element)
{
  if (element)
    {
      keyboard_click_state.inclick += 1;
      if (!element.classList.contains ('active'))
	{
	  const e = element;
	  e.classList.toggle ('active', true);
	  setTimeout (() => e.classList.toggle ('active', false), 42); // exceed 1/24th frame
	}
      element.click();
      keyboard_click_state.inclick -= 1;
      return true;
    }
  return false;
}
exports.keyboard_click = keyboard_click;

/// Check if an element can be found in a given array.
function in_array (element, array)
{
  return array.indexOf (element) >= 0;
}

/// Export key codes
const KeyCode = {
  BACKSPACE: 8, TAB: 9, ENTER: 13, RETURN: 13, CAPITAL: 20, CAPSLOCK: 20, ESC: 27, ESCAPE: 27, SPACE: 32,
  PAGEUP: 33, PAGEDOWN: 34, END: 35, HOME: 36, LEFT: 37, UP: 38, RIGHT: 39, DOWN: 40, PRINTSCREEN: 44, INSERT: 45, DELETE: 46,
  F1: 112, F2: 113, F3: 114, F4: 115, F5: 116, F6: 117, F7: 118, F8: 119, F9: 120, F10: 121, F11: 122, F12: 123,
  F13: 124, F14: 125, F15: 126, F16: 127, F17: 128, F18: 129, F19: 130, F20: 131, F21: 132, F22: 133, F23: 134, F24: 135,
  BROWSERBACK: 166, BROWSERFORWARD: 167, PLUS: 187/*FIXME*/, MINUS: 189/*FIXME*/, PAUSE: 230, ALTGR: 255,
  VOLUMEMUTE: 173, VOLUMEDOWN: 174, VOLUMEUP: 175, MEDIANEXTTRACK: 176, MEDIAPREVIOUSTRACK: 177, MEDIASTOP: 178, MEDIAPLAYPAUSE: 179,
};
Object.assign (exports, KeyCode);

const navigation_keys = [
  KeyCode.UP, KeyCode.DOWN, KeyCode.LEFT, KeyCode.RIGHT,
  KeyCode.TAB, KeyCode.SPACE, KeyCode.ENTER /*13*/, 10 /*LINEFEED*/,
  KeyCode.PAGE_UP, KeyCode.PAGE_DOWN, KeyCode.HOME, KeyCode.END,
  93 /*CONTEXT_MENU*/, KeyCode.ESCAPE,
];

/// Check if a key code is used of rnavigaiton (and non alphanumeric).
function is_navigation_key_code (keycode)
{
  return in_array (keycode, navigation_keys);
}
exports.is_navigation_key_code = is_navigation_key_code;

/// Match an event's key code, considering modifiers.
function match_key_event (event, keyname)
{
  // SEE: http://unixpapa.com/js/key.html & https://developer.mozilla.org/en-US/docs/Mozilla/Gecko/Gecko_keypress_event
  // split_hotkey (hotkey)
  const rex = new RegExp (/\s*[+]\s*/); // Split 'Shift+Ctrl+Alt+Meta+SPACE'
  const parts = keyname.toLowerCase().split (rex);
  const char = String.fromCharCode (event.which || event.keyCode);
  let need_meta = 0, need_alt = 0, need_ctrl = 0, need_shift = 0;
  for (let i = 0; i < parts.length; i++)
    {
      // collect meta keys
      switch (parts[i])
      {
	case 'cmd': case 'command':
	case 'super': case 'meta':	need_meta  = 1; continue;
	case 'option': case 'alt':	need_alt   = 1; continue;
	case 'control': case 'ctrl':	need_ctrl  = 1; continue;
	case 'shift':		  	need_shift = 1; continue;
      }
      // match named keys (special)
      const key_val = KeyCode[parts[i].toUpperCase()];
      if (key_val !== undefined && char.length == 1 && key_val == char.charCodeAt (0))
	continue;
      // match characters
      if (char.toLowerCase() == parts[i])
	continue;
      // failed to match
      return false;
    }
  // ignore shift for case insensitive characters (except for navigations)
  if (char.toLowerCase() == char.toUpperCase() &&
      !is_navigation_key_code (event.keyCode))
    need_shift = -1;
  // match meta keys
  if (need_meta   != 0 + event.metaKey ||
      need_alt    != 0 + event.altKey ||
      need_ctrl   != 0 + event.ctrlKey ||
      (need_shift != 0 + event.shiftKey && need_shift >= 0))
    return false;
  return true;
}
exports.match_key_event = match_key_event;
