<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-piano-roll - Vue template to display a part as a piano roll
  ### Props:
  - **piano-part** - The `BsePart` to display.
  ### data:
  - **hscrollbar** - The horizontal scrollbar component, used to provide the virtual scrolling position for the notes canvas.
  - **part** - Retrieve the `BsePart` set via the `piano-part` property.
</docs>

<template>
  <div class="vc-piano-roll" style="display: flex; flex-direction: column; width: 100%;" >
    <vc-hscrollbar ref="hscrollbar" slider-size='45' style='margin:10px' ></vc-hscrollbar>
    <div ref="scrollcontainer" style="overflow-y: scroll" >
      <div ref="scrollarea" style="display: flex; flex-direction: column; overflow: hidden;" >
	<div :style="{
		     display: 'flex',
		     'flex-direction': 'row',
		     height: piano_roll_cssheight + 'px',
		     width: '100%',
		     'background-color': 'blue',
		     }" >
	  <canvas ref="piano-canvas" class="vc-piano-roll-piano" :style="{ width:  54 + 'px', height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
	  <canvas ref="notes-canvas" class="vc-piano-roll-notes" :style="{ width: 384 + 'px', height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
	</div>
      </div>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-piano-roll {
    border: 1px solid red;
  }
  .vc-piano-roll canvas {
    display: block;
    image-rendering: pixelated /*ff: crisp-edges*/;
    --piano-roll-light-row:    		$vc-piano-roll-light-row;
    --piano-roll-dark-row:     		$vc-piano-roll-dark-row;
    --piano-roll-semitone12:   		$vc-piano-roll-semitone12;
    --piano-roll-semitone6:    		$vc-piano-roll-semitone6;
    --piano-roll-grid-main1:   		$vc-piano-roll-grid-main1;
    --piano-roll-grid-sub1:    		$vc-piano-roll-grid-sub1;
    --piano-roll-white-base:   		$vc-piano-roll-white-base;
    --piano-roll-white-glint:  		$vc-piano-roll-white-glint;
    --piano-roll-white-border: 		$vc-piano-roll-white-border;
    --piano-roll-black-base:   		$vc-piano-roll-black-base;
    --piano-roll-black-glint:  		$vc-piano-roll-black-glint;
    --piano-roll-black-shine:  		$vc-piano-roll-black-shine;
    --piano-roll-black-border: 		$vc-piano-roll-black-border;
    --piano-roll-key-font:     		$vc-piano-roll-key-font;
    --piano-roll-key-font-color: 	$vc-piano-roll-key-font-color;
    --piano-roll-note-font:    		$vc-piano-roll-note-font;
    --piano-roll-note-font-color:	$vc-piano-roll-note-font-color;
  }
  .vc-piano-roll-piano {
    --piano-roll-font: $vc-piano-roll-font;
    --piano-roll-font-color: $vc-piano-roll-font-color;
  }
  .vc-piano-roll-notes {
  }
</style>

<script>
const Util = require ('./utilities.js');
const floor = Math.floor, round = Math.round;
const piano_roll_cssheight = 841;

module.exports = {
  name: 'vc-piano-roll',
  mixins: [ Util.vue_mixins.dom_updated, Util.vue_mixins.hyphen_props ],
  props: {
    'piano-part': [Bse.Part],
  },
  data_tmpl: {
    qnpt:		4,	// quarter notes per tact
    tpqn:		384,	// ticks per quarter note
    last_tick:		384,
    vzoom:		1.0,
    hzoom:		1.0,
    hscrollbar:		undefined,
    part_:		undefined,
  },
  computed: {
    piano_keys: function() { return 120; }, // FIXME
    piano_height: function() { return 84 * floor ((this.piano_keys + 12 - 1) / 12) + 1; }, // FIXME
    part:		function () { return this['piano-part']; },
  },
  beforeDestroy() {
    if (this.unwatch_render_canvas) {
      this.unwatch_render_canvas();
      this.unwatch_render_canvas = undefined;
    }
  },
  methods: {
    piano_layout: piano_layout,
    render_piano: render_piano,
    render_notes: render_notes,
    render_canvas () {
      const layout = this.piano_layout();
      this.render_piano (layout);	// render piano first, it fills some caches that render_notes utilizes
      this.render_notes (layout);
    },
    dom_updated() {
      if (this.hscrollbar != this.$refs.hscrollbar) {
	this.hscrollbar = this.$refs.hscrollbar;
	this.$forceUpdate(); // setup hscrollbar percentage
      }
      /* DOM and $el is in place, now:
       * a) render into the canvases, we call render_canvas() for this;
       * b) re-render the canvases if anything changes, for this we install a watcher
       */
      if (!this.unwatch_render_canvas)
	this.unwatch_render_canvas = this.$watch (this.render_canvas, () => this.$forceUpdate());
      this.render_canvas();
    },
  },
};

function piano_layout () {
  this.last_tick = Math.max (this.last_tick, this.part ? this.part.get_last_tick() : 0);
  /* By design, each octave consists of 12 aligned rows that are used for note placement.
   * Each row is always pixel aligned. Consequently, the pixel area assigned to an octave
   * can only shrink or grow in 12 screen pixel intervalls.
   * The corresponding white and black keys are also always pixel aligned, variations in
   * mapping the key sizes to screen coordinates are distributed over the widths of the keys.
   */
  let layout = {
    cssheight:		piano_roll_cssheight,
    piano_csswidth:	0,			// derived from white_width
    notes_csswidth:	0,			// display width, determined by parent
    virt_width:		0,			// virtual width in CSS pixels, derived from last_tick
    oct_length:		84,			// initially css pixels, = 12 * 7
    octaves:		0,			// number of octaves to display
    thickness:		1,			// if ratio in [0..2]
    yoffset:		piano_roll_cssheight,	// y coordinate of lowest octave
    xscroll:		0,			// horizontal scroll position
    beat_pixels:	50,			// pixels per quarter note
    row:		7,
    bkeys:		[], 			// [ [offset,size] * 5 ]
    wkeys:		[], 			// [ [offset,size] * 7 ]
    row_colors:		[ 1, 2, 1, 2, 1,   1, 2, 1, 2, 1, 2, 1 ],		// distinct key colors
    white_width:	54,			// length of white keys
    black_width:	0.55,			// length of black keys (pre-init factor)
    label_keys:		1,			// 0=none, 1=roots, 2=whites
  };
  const black_keyspans = [  [7,7], [21,7],     [43,7], [56.5,7], [70,7]   ]; 	// for 84px octave
  const white_offsets  = [ 0,    12,     24, 36,     48,       60,     72 ]; 	// for 84px octave
  // scale layout
  const sbr = this.$refs.scrollarea.getBoundingClientRect();
  const ratio = window.devicePixelRatio;
  const layout_height = round (ratio * layout.cssheight);
  layout.piano_csswidth = layout.white_width;
  layout.notes_csswidth = Math.max (layout.piano_csswidth + 1, sbr.width - layout.piano_csswidth);
  layout.beat_pixels = round (layout.beat_pixels * ratio * this.hzoom);
  layout.virt_width = floor (384 * floor (this.last_tick / 384));
  layout.octaves = round (layout.cssheight / layout.oct_length);
  layout.thickness = Math.max (round (ratio * 0.5), 1);
  layout.yoffset = layout_height - layout.thickness;		// leave a pixel for overlapping piano key borders
  layout.row = round (ratio * layout.row * this.vzoom);
  layout.oct_length = layout.row * 12;
  layout.white_width = round (layout.white_width * ratio);
  layout.black_width = round (layout.white_width * layout.black_width);
  // hscrollbar setup
  if (this.hscrollbar) {
    if (this.hscrollbar.percentage !== undefined)
      this.hscrollbar.percentage = layout.notes_csswidth / layout.virt_width;
    layout.xscroll = this.hscrollbar.value * layout.virt_width;
  }
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
  return Object.freeze (layout); // effectively 'const'
}

const cs_values = {
  light_row: 		'--piano-roll-light-row',
  dark_row:  		'--piano-roll-dark-row',
  semitone6:		'--piano-roll-semitone6',
  semitone12:		'--piano-roll-semitone12',
  grid_main1:		'--piano-roll-grid-main1',
  grid_sub1:		'--piano-roll-grid-sub1',
  white_base:   	'--piano-roll-white-base',
  white_glint:  	'--piano-roll-white-glint',
  white_border: 	'--piano-roll-white-border',
  black_base:   	'--piano-roll-black-base',
  black_glint:  	'--piano-roll-black-glint',
  black_shine:  	'--piano-roll-black-shine',
  black_border: 	'--piano-roll-black-border',
  key_font:		'--piano-roll-key-font',
  key_font_color:	'--piano-roll-key-font-color',
  note_font:		'--piano-roll-note-font',
  note_font_color:	'--piano-roll-note-font-color',
};

function render_notes (layout) {
  // canvas setup
  const canvas = this.$refs['notes-canvas'], ctx = canvas.getContext ('2d');
  // computed styles
  const cs = Util.compute_style_properties (canvas, cs_values);
  // resize canvas to match onscreen pixels, paint bg with white key row color
  Util.resize_canvas (canvas, layout.notes_csswidth, layout.cssheight, cs.light_row);
  // we draw piano keys verticaly overlapping by one th and align octave separators accordingly
  const th = layout.thickness;

  // paint black key rows
  ctx.fillStyle = cs.dark_row;
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    for (let r = 0; r < layout.row_colors.length; r++) {
      if (layout.row_colors[r] > 1) {
	ctx.fillRect (0, oy - r * layout.row - layout.row, canvas.width, layout.row);
      }
    }
  }

  // line thickness and line cap
  ctx.lineWidth = th;
  ctx.lineCap = 'butt'; // chrome 'butt' has a 0.5 pixel bug, so we use fillRect
  const lsx = layout.xscroll;

  // draw half octave separators
  ctx.fillStyle = cs.semitone6;
  const stipple = round (3 * window.devicePixelRatio), stipple2 = 2 * stipple;
  const qy = layout.wkeys[3][0]; // separator between F|G
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    Util.hstippleRect (ctx, th - lsx % stipple2, oy - qy, canvas.width, th, stipple);
  }

  // draw vertical grid lines
  const gy1 = layout.yoffset - layout.octaves * layout.oct_length + th;
  const gy2 = layout.yoffset; // align with outer piano border
  const beat_dist = layout.beat_pixels;
  const grid_divider = 4 * beat_dist; // largest grid divider
  let grid = floor (lsx / grid_divider);
  for (let gx = 0; gx - lsx < canvas.width; grid++) {
    gx = grid * beat_dist;
    if (grid % 4 == 0) {
      ctx.fillStyle = cs.grid_main1;
    } else {
      ctx.fillStyle = cs.grid_sub1;
    }
    ctx.fillRect (gx - lsx, gy1, th, gy2 - gy1);
  }

  // draw octave separators
  ctx.fillStyle = cs.semitone12;
  for (let oct = 0; oct <= layout.octaves; oct++) {	// condiiton +1 to include top border
    const oy = layout.yoffset - oct * layout.oct_length;
    ctx.fillRect (0, oy, canvas.width, th);
  }

  // paint notes
  const part = this.part;
  if (part) {
    const fpx_parts = cs.key_font.split (/\s*\d+px\s*/i); // 'bold 10px sans' -> [ ['bold', 'sans']
    const fpx = layout.row - 2;
    ctx.font = fpx_parts[0] + ' ' + fpx + 'px ' + (fpx_parts[1] || '');
    ctx.fillStyle = cs.key_font_color;
    // draw notes
    const pnotes = part.list_notes_crossing (0, MAXINT);
    const tickscale = layout.beat_pixels / 384;
    for (const note of pnotes) {
      const oct = floor (note.note / 12), key = note.note - oct * 12;
      const ny = layout.yoffset - oct * layout.oct_length - key * layout.row;
      const nx = round (note.tick * tickscale), nw = Math.max (1, round (note.duration * tickscale));
      ctx.fillRect (nx - lsx, ny - layout.row, nw, layout.row);
    }
  }
}

function render_piano (layout) {
  // canvas setup
  const canvas = this.$refs['piano-canvas'], ctx = canvas.getContext ('2d');
  // computed styles
  const cs = Util.compute_style_properties (canvas, cs_values);
  // resize canvas to match onscreen pixels, paint bg with white key row color
  Util.resize_canvas (canvas, layout.piano_csswidth, layout.cssheight, cs.light_row);
  // we draw piano keys horizontally within their boundaries, but verticaly overlapping by one th
  const th = layout.thickness, hf = th * 0.5; // thickness/2 fraction

  // draw piano keys
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    // draw white keys
    ctx.fillStyle = cs.white_base;
    ctx.lineWidth = th;
    for (let k = 0; k < layout.wkeys.length; k++) {
      const p = layout.wkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.white_width, h = p[1];
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = cs.white_glint;		// highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = cs.white_border;	// border
      ctx.strokeRect (sx, sy, w - th, h);			// v-overlap by 1*th
    }
    // draw black keys
    ctx.fillStyle = cs.black_base;
    ctx.lineWidth = th;
    for (let k = 0; k < layout.bkeys.length; k++) {
      const p = layout.bkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.black_width, h = p[1];
      const gradient = [ [0, cs.black_base], [.08, cs.black_base], [.15, cs.black_shine],   [1, cs.black_base] ];
      ctx.fillStyle = Util.linear_gradient_from (ctx, gradient, x + th, y - h / 2, x + w - 2 * th, y - h / 2);
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = cs.black_glint;		// highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = cs.black_border;	// border
      ctx.strokeRect (sx, sy, w - th, h);
    }
  }

  // figure font size for piano key labels
  const avg_height = layout.wkeys.reduce ((a, p) => a += p[1], 0) / layout.wkeys.length;
  const fpx = avg_height - 2 * (th + 1);	// base font size on average white key size
  if (fpx >= 6) {
    const fpx_parts = cs.key_font.split (/\s*\d+px\s*/i); // 'bold 10px sans' -> [ ['bold', 'sans']
    ctx.font = fpx_parts[0] + ' ' + fpx + 'px ' + (fpx_parts[1] || '');
    ctx.fillStyle = cs.key_font_color;
    // measure Midi labels, faster if batched into an array
    const midi_labels = Util.midi_label ([...Util.range (0, layout.octaves * (layout.wkeys.length + layout.bkeys.length))]);
    const label_spans = Util.canvas_ink_vspan (ctx.font, midi_labels);
    // draw names
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    const white2midi = [ 0, 2, 4,   5, 7, 9, 11 ];
    // TODO: use actualBoundingBoxAscent once measureText() becomes more sophisticated
    for (let oct = 0; oct < layout.octaves; oct++) {
      const oy = layout.yoffset - oct * layout.oct_length;
      // skip non-roots / roots according to configuration
      for (let k = 0; k < layout.wkeys.length; k++) {
	if ((k && layout.label_keys < 2) || layout.label_keys < 1)
	  continue;
	// draw white key
	const p = layout.wkeys[k];
	const x = 0, y = oy - p[0];
	const w = layout.white_width, h = p[1];
	const midi_key = oct * 12 + white2midi[k];
	const label = midi_labels[midi_key], vspan = label_spans[midi_key];
	const twidth = ctx.measureText (label).width;
	const tx = x + w - 2 * (th + 1) - twidth, ty = y - h + (h - vspan[1]) / 2 - vspan[0];
	ctx.fillText (label, tx, ty);
      }
    }
  }
}

</script>
