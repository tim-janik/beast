// MIT licensed, based on https://github.com/bugsnag/chromatic-sass
"use strict";

const chroma = require ("chroma-js");
const sass = require ("sass");
const T = sass.types;

function assertion_error (msg, new_error) {
  msg = msg + ": " + new_error.message;
  const callerregexp = RegExp (/^[ \t]+at (.+) \(([^)]*)\)$/gm), stack = new_error.stack.toString();
  const caller = callerregexp.exec (stack) && callerregexp.exec (stack);
  const prefix = caller ? caller[2] + ': ' + caller[1] + ': ': '';
  if (prefix)
    console.error (prefix + msg);
  return new Error (msg);
}

function sasstype (obj) {
  if (obj instanceof T.Null)	return 'Null';		// T.Null.NULL
  if (obj instanceof T.Boolean)	return 'Boolean';	// T.Boolean.TRUE T.Boolean.FALSE getValue()
  if (obj instanceof T.Color)	return 'Color';		// getR() getG() getB() getA() setR() setG() setB() setA() toString()
  if (obj instanceof T.List)	return 'List';		// getValue(N) setValue() getSeparator() setSeparator() getLength() toString()
  if (obj instanceof T.Map)	return 'Map';		// getKey() getValue() getLength() setKey() setValue() toString()
  if (obj instanceof T.Number)	return 'Number';	// getValue() setValue() getUnit() setUnit() toString()
  if (obj instanceof T.String)	return 'String';	// getValue() setValue() toString()
  if (obj instanceof T.Error)	return 'Error';		// T.Error (msg) toString()
  return typeof obj;
}

/// Compare by `===` or per item for Array objects.
function equals_recursively (a, b) {
  if (a === b)			return true;
  if (typeof a != typeof b)	return false;
  if (typeof a == 'number' &&
      isNaN (a) && isNaN (b))	return true;
  if (!(a instanceof Object &&                  // null
	b instanceof Object))	return false;
  if (a instanceof Function)	return false;	// should match ===
  const ak = Object.getOwnPropertyNames (a), bk = Object.getOwnPropertyNames (b);
  if (ak.length != bk.length)	return false;
  for (let i = 0; i < ak.length; i++)
    {
      const k = ak[i];
      if (k != bk[i])
	return false;
      const av = a[k], bv = b[k];
      if (!equals_recursively (av, bv))
	return false;
    }
  return true;
}

/// Turn SassList into Array
function list2array (sasslist) {
  const l = sasslist.getLength(), array = new Array (l);
  for (let i = 0; i < l; i++)
    array[i] = sasslist.getValue (i);
  array.separator = sasslist.getSeparator();
  return array;
}

/// Extract JS values from SassValue (usually by calling getValue())
function V (obj) {
  if (obj instanceof T.List)	// getValue(N)
    {
      const l = obj.getLength(), array = new Array (l);
      for (let i = 0; i < l; i++)
	array[i] = V (obj.getValue (i));
      array.separator = obj.getSeparator();
      return array;
    }
  if (obj.getValue)
    return obj.getValue();	// Boolean Number String
  switch (sasstype (obj)) {
    case 'Null':
      return null;
    default:
      return obj;
  }
}

function assert_type (obj, type) {
  if (type != sasstype (obj))
    throw assertion_error ("not a `" + type + "` type object", new Error (obj));
}

function assert_value (obj) {
  if (V (obj) === undefined)
    throw assertion_error ("not a SassValue object", new Error (JSON.stringify (obj)));
}

/// Generate a hex color string from a sass color
function sass2hex (color) {
  assert_type (color, 'Color');
  return chroma (color.getR(), color.getG(), color.getB()).hex();
}

/// Generate a rgb array from a sass color
function sass2rgb (color) {
  assert_type (color, 'Color');
  return [color.getR(), color.getG(), color.getB(), color.getA()];
}

function roundRgb (rgb) {
  const arr = [Math.round (rgb[0]), Math.round (rgb[1]), Math.round (rgb[2])];
  if (rgb[3])
    arr.push (rgb[3]);
  return arr;
}

function rgb2sass (rgb) {
  rgb = roundRgb (rgb);
  const color = new T.Color (0xff000000);
  color.setR (rgb[0]);
  color.setG (rgb[1]);
  color.setB (rgb[2]);
  if (rgb[3] && rgb[3] != 1)
    return color.setA (rgb[3]);
  return color;
}

function rgb2str (rgb) {
  rgb = roundRgb (rgb);
  if (rgb[3] && rgb[3] !== 1)
    return "rgba(" + rgb[0] + ", " + rgb[1] + ", " + rgb[2] + ", " + rgb[3] + ")";
  else
    return "rgb(" + rgb[0] + ", " + rgb[1] + ", " + rgb[2] + ")";
}

module.exports = {
  "chromatic-color-luminance($color, $luminance: '', $mode: '')": (color, luminance, mode) => {
    assert_type (color, 'Color');
    const rgb = sass2rgb (color);
    luminance = V (luminance);
    mode = V (mode);
    if (luminance)
      {
	if (mode)
	  return rgb2sass (chroma (rgb).luminance (luminance, mode)._rgb);
	else
	  return rgb2sass (chroma (rgb).luminance (luminance)._rgb);
      }
    else
      return new T.Number (chroma (rgb).luminance());
  },
  "chromatic-color-set($color, $channel, $value)": (color, channel, value) => {
    assert_type (color, 'Color');
    assert_value (channel);
    assert_value (value);
    channel = V (channel);
    value = V (value);
    const rgb = sass2rgb (color);
    return rgb2sass (chroma (rgb).set (channel, value)._rgb);
  },
  "chromatic-color-get($color, $channel)": (color, channel) => {
    assert_type (color, 'Color');
    assert_value (channel);
    channel = V (channel);
    const rgb = sass2rgb (color);
    return new T.Number (chroma (rgb).get (channel));
  },
  "chromatic-contrast($color0, $color1)": (color0, color1) => {
    assert_type (color0, 'Color');
    assert_type (color1, 'Color');
    return new T.Number (chroma.contrast (sass2hex (color0), sass2hex (color1)));
  },
  "chromatic-hsv($x, $y, $z, $alpha: 1)": (x, y, z, alpha) => {
    return rgb2sass (chroma.hsv (V (x), V (y), V (z), V (alpha))._rgb);
  },
  "chromatic-gradient($argslist...)": (argslist) => {
    assert_type (argslist, 'List');
    const defaults = {
      mode: "lab",
      stops: 7,
      type: "linear"
    };
    const options = {};
    const colors = [];
    const initPositions = [];
    const positions = [];
    const argslistLength = argslist.getLength();

    // Set direction if provided
    let direction = null;
    const firstArg = argslist.getValue (0);
    const firstArgType = sasstype (firstArg);
    if (firstArgType === "List")
      {
	const firstArgJs = V (firstArg);
	if (firstArgJs.every (item => typeof (item) === "string"))
          direction = firstArgJs.join (" ");
      }
    else if (firstArgType === "Number")
      direction = firstArg.getValue() + firstArg.getUnit();
    else if (firstArgType === "String")
      direction = firstArg.getValue();

    // Set options if provided and init settings
    const lastArg = argslist.getValue (argslistLength - 1);
    const lastArgType = sasstype (lastArg);
    if (lastArgType === "Map")
      // Unpacks options map assuming k/v types string, unitless number, or boolean
      for (let i = 0; i < lastArg.getLength(); i++)
        options[lastArg.getKey (i).getValue()] = lastArg.getValue (i).getValue();
    const settings = Object.assign ({}, defaults, options);

    // Set color stops
    const startIndex = direction ? 1 : 0;
    const endIndex = lastArgType === "Map" ? argslistLength - 1 : argslistLength;
    for (let i = startIndex; i < endIndex; i++)
      {
	let arg = argslist.getValue (i);
	const argType = sasstype (arg);
	// Unpack color stops
	if (argType === "List")
	  {
            arg = list2array (arg);
            if (sasstype (arg[0]) === "Color" && sasstype (arg[1]) === "Number" && arg.length === 2)
	      {
		if (arg[1].getUnit() !== "%")
		  return T.Error ("Chromatic gradient color-stop initPositions must be provided as percentages");
		colors.push (sass2rgb (arg[0]));
		positions.push (arg[1].getValue() / 100);
              }
	    else
              return new T.Error ("Chromatic gradient color stops must take the form: <color> [<percentage>]?");
	  }
	else if (argType === "Color")
	  {
            colors.push (sass2rgb (arg));
            positions.push (null);
	  }
      }

    // Set defaults for positions start and end
    if (positions[0] === null)
      positions[0] = 0;
    if (positions[positions.length - 1] === null)
      positions[positions.length - 1] = 1;

    // Populate null initPositions
    let lastNonnullIndex = 0;
    let numberOfNulls = 0;
    let nullIndex = 0;
    let maxValue = 0;
    for (let [index, value] of positions.entries())
      {
	if (value === null)
	  {
            let increment = 0;
            nullIndex += 1;
	    for (const [nextIndex, nextValue] of positions.slice (index + 1, positions.length).entries())
	      if (nextValue)
		{
		  numberOfNulls = nextIndex + index - lastNonnullIndex;
		  if (nextValue <= maxValue)
		    increment = maxValue;
		  else
		    increment = ((nextValue - positions[lastNonnullIndex]) * 1.0) / (numberOfNulls + 1);
		  break;
		}
            positions[index] = increment * nullIndex;
	    // Force rendering of inferred non 0, 1 initial positions in case we add points asymetrically
            initPositions[index] = increment * nullIndex;
	  }
	else
	  {
            nullIndex = 0;
            lastNonnullIndex = index;
            if (value < maxValue)
	      {
		value = maxValue;
		positions[index] = value;
              }
	    else if (value > maxValue)
	      maxValue = value;
	  }
      }

    // Interpolate additional points in specified color space
    while (colors.length < settings.stops)
      {
	let maxDistance = 0;
	let maxDistanceStartIndex = null;
	for (let i = 0; i < colors.length - 1; i++)
	  {
            const distance = positions[i + 1] - positions[i];
            if (distance > maxDistance && !equals_recursively (colors[i], colors[i + 1]))
	      {
		maxDistanceStartIndex = i;
		maxDistance = distance;
              }
	  }
	if (maxDistanceStartIndex === null)
	  break;
        const newPosition = maxDistance / 2 + positions[maxDistanceStartIndex];
        const newColor = chroma.mix (colors[maxDistanceStartIndex], colors[maxDistanceStartIndex + 1], .5, settings.mode)._rgb;
        colors.splice (maxDistanceStartIndex + 1, 0, newColor);
        positions.splice (maxDistanceStartIndex + 1, 0, newPosition);
      }

    // Build string
    let str = settings.type + "-gradient(";
    if (direction)
      str += direction + ", ";
    for (const [i, color] of colors.entries())
      {
	str += rgb2str (color);
	str += " " + positions[i] * 100 + "%";
	if (i < colors.length - 1)
          str += ", ";
      }
    str += ")";
    return new T.String (str);
  },
};

/* Print JS stack traces for exceptions caught by sass:
 * Add to node_modules/sass/sass.dart.js: completeError$2(): console.error ('STACKTRACE:', st);
 * Text snipet:
   "use strict";
   const sass = require ('sass');
   const r = sass.renderSync ({
   file: 'ebeast/app.scss',
   includePaths: [ 'ebeast/', 'out/ebeast/', ],
   functions: require ("./chromatic-sass2"),
   });
   console.log (r.css.toString ('utf8'));
 */
