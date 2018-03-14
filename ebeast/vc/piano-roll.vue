<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-piano-roll - Vue template to display a part as a piano roll
  ### Props:
  - **part** - The BsePart to display.
</docs>

<template>

  <div class="vc-piano-roll" :style="{ height: this.piano_height + 'px', 'background-color': 'blue', }" >
    <canvas ref="piano-canvas" class="vc-piano-roll-piano" :style="{ width: 8000 + 'px', height: this.piano_height + 'px', }" ></canvas>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-piano-roll {
  }
  .vc-piano-roll-piano {
    --piano-roll-font: $vc-piano-roll-font;
    --piano-roll-font-color: $vc-piano-roll-font-color;
    --piano-roll-colors: $vc-piano-roll-colors;
  }
</style>

<script>
const Util = require ('./utilities.js');
const floor = Math.floor, round = Math.round;

const tick_quant = 384; // FIXME
module.exports = {
  name: 'vc-piano-roll',
  props: {
    'part': { type: Bse.Part, },
  },
  computed: {
    piano_keys: function() { return 120; },
    piano_height: function() { return 84 * floor ((this.piano_keys + 12 - 1) / 12) + 1; },
    tickscale: function() { return 10 / 384.0; }, // FIXME
    canvas_width: function() {
      const part = this.part;
      let last_tick = part ? part.get_last_tick() : 0;
      return this.tickscale * Math.floor ((last_tick + tick_quant - 1) / tick_quant) * tick_quant;
    },
  },
  customized_render (createElement, result) {
    // queue render_canvas into main loop so $el / canvas are updated
    Promise.resolve().then (() => { if (this.$el) render_piano.call (this); });
    return result;
  },
  beforeDestroy() {
    clearTimeout (this.timer);
  },
};

function piano_layout (csswidth, cssheight) {
  /* By design, each octave consists of 12 aligned rows that are used for note placement.
   * Each row is always pixel aligned. Consequently, the pixel area assigned to an octave
   * can only shrink or grow in 12 screen pixel intervalls.
   * The corresponding white and black keys are also always pixel aligned, variations in
   * mapping the key sizes to screen coordinates are distributed over the widths of the keys.
   */
  let layout = {
    csswidth:		csswidth,
    cssheight:		cssheight,
    line_width:		1,			// if ratio in [0..2]
    width:		csswidth,		// iff ratio==1.0
    height:		cssheight,		// iff ratio==1.0
    oct_start:		cssheight,		// y coordinate of lowest octave
    row:		7,
    oct_length:		84,			// reference size, = 12 * 7
    octaves:		0,			// number of octaves to display
    bkeys:		[], 			// [ [offset,size] * 5 ]
    wkeys:		[], 			// [ [offset,size] * 7 ]
    colors:		[ 1, 2, 1, 2, 1,   1, 2, 1, 2, 1, 2, 1 ],		// distinct key colors
    white_width:	54,			// length of white keys
    black_width:	0.65,			// length of black keys (pre-init factor)
  };
  const black_keyspans = [  [7,7], [21,7],     [43,7], [56.5,7], [70,7]   ]; 	// for 84px octave
  const white_offsets  = [ 0,    12,     24, 36,     48,       60,     72 ]; 	// for 84px octave
  // scale layout
  const zoom = 1.0;
  const ratio = window.devicePixelRatio;
  layout.width = round (ratio * layout.csswidth);
  layout.height = round (ratio * layout.cssheight);
  layout.line_width = Math.max (round (ratio * 0.5), 1);
  layout.oct_start = layout.height - layout.line_width;
  layout.row = round (ratio * layout.row * zoom);
  layout.oct_length = layout.row * 12;
  layout.octaves = round (layout.height / (layout.oct_length + layout.line_width));
  layout.white_width = round (layout.white_width * ratio * zoom);
  layout.black_width = round (layout.white_width * layout.black_width);
  // assign white key positions and aligned sizes
  let last = layout.oct_length;
  for (let i = white_offsets.length - 1; i >= 0; i--) {
    const key_start = round (layout.oct_length * white_offsets[i] / 84.0);
    const key_size = last - key_start;
    layout.wkeys.unshift ( [key_start, key_size] );
    last = key_start;
  }
  // assign black key positions and sizes
  for (let i = 0; i < black_keyspans.length; i++) {
    const key_start = round (layout.oct_length * black_keyspans[i][0] / 84.0);
    const key_end   = round (layout.oct_length * (black_keyspans[i][0] + black_keyspans[i][1]) / 84.0);
    layout.bkeys.push ([key_start, key_end - key_start]);
  }
  return layout;
}

function render_piano () {
  // canvas setup
  const canvas = this.$refs['piano-canvas'], ctx = canvas.getContext ('2d');
  const layout = piano_layout (canvas.clientWidth, canvas.clientHeight);
  // FIXME: const style = getComputedStyle (canvas), part = this.part;
  const lw = layout.line_width;
  // resize canvas to match onscreen pixels
  canvas.width = layout.width;
  canvas.height = layout.height;
  ctx.clearRect (0, 0, layout.width, layout.height);
  // paint area with white key row color
  ctx.fillStyle = '#303030';
  const piano_height = layout.octaves * layout.oct_length;
  ctx.fillRect (0, layout.oct_start - piano_height, layout.width, piano_height + 1);
  // paint black key rows
  ctx.fillStyle = '#272727';
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.oct_start - oct * layout.oct_length;
    for (let r = 0; r < layout.colors.length; r++) {
      if (layout.colors[r] > 1) {
	ctx.fillRect (0, oy - r * layout.row - layout.row, layout.width, layout.row);
      }
    }
  }

  // draw grid lines
  ctx.lineWidth = lw;
  const grid_dist = 25;
  for (let gx = 0, g = 0; gx < layout.width; g++) {
    const oh = layout.octaves * layout.oct_length;
    const oy = layout.oct_start + 0.5;
    gx = layout.white_width + g * grid_dist + 0.5;
    if (g % 4 == 0) {
      ctx.strokeStyle = '#555';
    } else {
      ctx.strokeStyle = '#3b3b3b';
    }
    ctx.beginPath();
    ctx.moveTo (gx, oy);
    ctx.lineTo (gx, oy - oh);
    ctx.closePath();
    ctx.stroke();
  }

  // draw octave separators
  ctx.strokeStyle = '#555';
  ctx.lineWidth = lw;
  const qy = layout.wkeys[3][0];
  for (let oct = 0; oct <= layout.octaves; oct++) {
    const oy = layout.oct_start + lw - oct * layout.oct_length - 0.5;
    ctx.beginPath();
    ctx.moveTo (0, oy);
    ctx.lineTo (layout.width, oy);
    if (oct < layout.octaves)
      Util.dash_xto (ctx, 0, oy - qy, layout.width, [ 8, 8, ]);
    ctx.closePath();
    ctx.stroke();
  }

  // draw piano keys
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.oct_start + lw - oct * layout.oct_length;
    // draw white keys
    ctx.fillStyle = '#ccc';
    ctx.lineWidth = lw;
    for (let k = 0; k < layout.wkeys.length; k++) {
      const x = -0.5, w = layout.white_width;
      const p = layout.wkeys[k];
      const y = oy - p[0] - 0.5, h = p[1];
      ctx.fillRect   (x, y - h, w, h);
      ctx.strokeStyle = '#eeeeee';
      ctx.strokeRect (x -lw, y - h +lw, w, h -lw);
      ctx.strokeStyle = '#111111';
      ctx.strokeRect (x, y - h, w, h);
    }
    // draw black keys
    ctx.fillStyle = '#181818';
    ctx.lineWidth = 1;
    for (let k = 0; k < layout.bkeys.length; k++) {
      const x = -0.5, w = layout.black_width;
      const p = layout.bkeys[k];
      const y = oy - p[0] - 0.5, h = p[1];
      const grad = ctx.createLinearGradient (x + w, y - h, x, y - h);
      const gradient = [ [0, '#181818'], [.85, '#555'], [.92, '#181818'], [1, '#181818']  ]; // FIXME
      for (const g of gradient)
	grad.addColorStop (g[0], g[1]);
      ctx.fillStyle = grad;
      ctx.fillRect   (x, y - h, w, h);
      ctx.strokeStyle = '#3a3a3a';
      ctx.strokeRect (x -lw, y - h +lw, w, h -lw);
      ctx.strokeStyle = '#222222';
      ctx.strokeRect (x, y - h, w, h);
    }
    ctx.fillStyle = '#000';
  }
}

</script>
