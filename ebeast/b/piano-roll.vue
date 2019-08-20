<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-PIANO-ROLL
  A Vue template to display a Bse.part as a piano roll.
  ## Props:
  *part*
  : The Bse.Part to display.
  ## Data:
  *hscrollbar*
  : The horizontal scrollbar component, used to provide the virtual scrolling position for the notes canvas.
  *part*
  : Retrieve the *Bse.Part* set via the `part` property.
</docs>

<template>

  <div class="b-piano-roll" style="display: flex; flex-direction: column; width: 100%;">
    <div style="display: flex; width: 100%; flex-shrink: 0;">
      <div ref="piano-roll-buttons" class="b-piano-roll-buttons" style="flex-shrink: 0; display: flex" >
	<button >I</button>
	<button >m</button>
	<button >q</button>
	<b-color-picker style="flex-shrink: 1" ></b-color-picker>
      </div>
      <b-hscrollbar ref="hscrollbar" slider-size='45' style="width: 100%;" ></b-hscrollbar>
    </div>
    <div ref="scrollcontainer" style="overflow-y: scroll" >
      <div ref="scrollarea" style="display: flex; flex-direction: column; overflow: hidden;" >
	<div :style="{
		     display: 'flex',
		     'flex-direction': 'row',
		     height: piano_roll_cssheight + 'px',
		     width: '100%',
		     'background-color': 'blue',
		     }" >
	  <canvas ref="piano-canvas" class="b-piano-roll-piano tabular-nums" :style="{ height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
	  <canvas ref="notes-canvas" class="b-piano-roll-notes tabular-nums" :style="{ height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
	</div>
      </div>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  .b-piano-roll {
    border: 1px solid red;
  }
  .b-piano-roll-buttons {
    font: $b-piano-roll-buttons-font;
  }
  .b-piano-roll canvas {
    display: block;
    image-rendering: pixelated /*ff: crisp-edges*/;
    //* Make scss variables available to JS via getComputedStyle() */
    --piano-roll-light-row:    		#{$b-piano-roll-light-row};
    --piano-roll-dark-row:     		#{$b-piano-roll-dark-row};
    --piano-roll-semitone12:   		#{$b-piano-roll-semitone12};
    --piano-roll-semitone6:    		#{$b-piano-roll-semitone6};
    --piano-roll-grid-main1:   		#{$b-piano-roll-grid-main1};
    --piano-roll-grid-sub1:    		#{$b-piano-roll-grid-sub1};
    --piano-roll-white-base:   		#{$b-piano-roll-white-base};
    --piano-roll-white-glint:  		#{$b-piano-roll-white-glint};
    --piano-roll-white-border: 		#{$b-piano-roll-white-border};
    --piano-roll-black-base:   		#{$b-piano-roll-black-base};
    --piano-roll-black-glint:  		#{$b-piano-roll-black-glint};
    --piano-roll-black-shine:  		#{$b-piano-roll-black-shine};
    --piano-roll-black-border: 		#{$b-piano-roll-black-border};
    --piano-roll-key-font:     		#{$b-piano-roll-key-font};
    --piano-roll-key-font-color: 	#{$b-piano-roll-key-font-color};
    --piano-roll-note-font:    		#{$b-piano-roll-note-font};
    --piano-roll-note-font-color:	#{$b-piano-roll-note-font-color};
  }
  .b-piano-roll-piano {
    --piano-roll-font: #{$b-piano-roll-font};
    --piano-roll-font-color: #{$b-piano-roll-font-color};
    --piano-roll-key-length: #{$b-piano-roll-key-length};
    width: $b-piano-roll-key-length;
  }
  .b-piano-roll-notes {
  }
</style>

<script>
const floor = Math.floor, round = Math.round;
const piano_roll_cssheight = 841;

function observable_part_data () {
  const data = {
    qnpt:          { default:   4, },	// quarter notes per tact
    tpqn:          { default: 384, },	// ticks per quarter note
    min_last_tick: { default: 384, },
    vzoom:         { default: 1.0, },
    hzoom:         { default: 1.0, },
    auto_scrollto: { default:  -2, },
    last_tick:     { getter: c => this.part.get_last_tick(), notify: n => this.part.on ("notify:last_tick", n), },
    pnotes:        { default: [],                            notify: n => this.part.on ("noteschanged", n),
                     getter: c => this.part.list_notes_crossing (0, CONFIG.MAXINT), },
  };
  return this.observable_from_getters (data, () => this.part);
}

module.exports = {
  name: 'b-piano-roll',
  mixins: [ Util.vue_mixins.dom_updates, Util.vue_mixins.hyphen_props ],
  props: {
    part: [Bse.Part],
  },
  data() { const PIANO_KEYS = 120;
    return { piano_keys:   PIANO_KEYS,
	     piano_height: 84 * floor ((PIANO_KEYS + 12 - 1) / 12) + 1,
	     adata:        observable_part_data.call (this) }; },
  watch: {
    part (nv, ov) { this.sync_scrollpos (nv, ov); },
  },
  mounted () {
    // keep vertical scroll position for each part, non-reactive
    this.auto_scrolls = {};
    // observer to watch for canvas size changes
    this.resize_observer = Util.resize_observer (this, () => this.$forceUpdate());
  },
  destroyed() {
    this.resize_observer.disconnect();
  },
  methods: {
    sync_scrollpos (newpart, oldpart) {
      if (!this.$refs.scrollarea)
	return;
      this.$refs.hscrollbar.value = 0;
      const vbr = this.$refs.scrollarea.parentElement.getBoundingClientRect();
      const sbr = this.$refs.scrollarea.getBoundingClientRect();
      if (oldpart && sbr.height > vbr.height)
	this.auto_scrolls[oldpart.$id] = this.$refs.scrollarea.parentElement.scrollTop / (sbr.height - vbr.height);
      if (newpart)
	{
	  const auto_scrollto = this.auto_scrolls[newpart.$id];
	  this.adata.auto_scrollto = auto_scrollto == undefined ? -1 : auto_scrollto;
	}
    },
    dom_update() {
      // Guard against the initial VNode Vue.render() call, after which DOM elements are created and assigned to $el and $ref
      if (!this.$el) // we need a second Vue.render() call for canvas drawing
	return this.$forceUpdate();
      // DOM, $el and $refs are in place now
      if (this.resizable_scrollarea != this.$refs.scrollarea)
	{
	  if (this.resizable_scrollarea)
	    this.resize_observer.unobserve (this.resizable_scrollarea);
	  this.resizable_scrollarea = this.$refs.scrollarea;
	  this.resize_observer.observe (this.resizable_scrollarea);
	}
      // make sure scroll events in the canvas are forwarded to the scrollbar
      if (!this.$refs.scrollarea.forwarding_wheel)
	{
	  this.$refs.scrollarea.addEventListener ('wheel', e => this.$refs.hscrollbar.wheel_event (e));
	  this.$refs.scrollarea.forwarding_wheel = true;
	}
      // canvas setup
      const piano_canvas = this.$refs['piano-canvas'], piano_style = getComputedStyle (piano_canvas);
      const notes_canvas = this.$refs['notes-canvas'], notes_style = getComputedStyle (notes_canvas);
      const layout = this.piano_layout (piano_canvas, piano_style, notes_canvas, notes_style);
      // render piano first, it fills some caches that render_notes utilizes
      this.render_piano (piano_canvas, piano_style, layout);
      this.render_notes (notes_canvas, notes_style, layout);
      // scrollto an area with visible notes
      if (this.adata.auto_scrollto !== undefined) {
	const vbr = this.$refs.scrollarea.parentElement.getBoundingClientRect();
	const sbr = this.$refs.scrollarea.getBoundingClientRect();
	if (sbr.height > vbr.height) {
	  let scroll_top, scroll_behavior = 'smooth';
	  if (this.adata.auto_scrollto >= 0)
	    scroll_top = (sbr.height - vbr.height) * this.adata.auto_scrollto;
	  else {
	    scroll_top = (sbr.height - vbr.height) / 2;
	    if (this.adata.auto_scrollto < -1)
	      scroll_behavior = 'auto'; // initial scroll pos, avoid animation
	  }
	  this.$refs.scrollarea.parentElement.scroll ({ top: scroll_top, behavior: scroll_behavior });
	}
	this.adata.auto_scrollto = undefined;
      }
    },
    piano_layout: piano_layout,
    render_piano: render_piano,
    render_notes: render_notes,
  },
};

function piano_layout (piano_canvas, piano_style, notes_canvas, notes_style) {
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
    white_width:	54,			// length of white keys, --piano-roll-key-length
    black_width:	0.55,			// length of black keys (pre-init factor)
    label_keys:		1,			// 0=none, 1=roots, 2=whites
  };
  const black_keyspans = [  [7,7], [21,7],     [43,7], [56.5,7], [70,7]   ]; 	// for 84px octave
  const white_offsets  = [ 0,    12,     24, 36,     48,       60,     72 ]; 	// for 84px octave
  const key_length = parseFloat (piano_style.getPropertyValue ('--piano-roll-key-length'));
  // scale layout
  const sbr = this.$refs.scrollarea.getBoundingClientRect();
  const ratio = window.devicePixelRatio;
  const layout_height = round (ratio * layout.cssheight);
  layout.white_width = key_length || layout.white_width; // allow CSS override
  layout.piano_csswidth = layout.white_width;
  layout.notes_csswidth = Math.max (layout.piano_csswidth + 1, sbr.width - layout.piano_csswidth);
  layout.beat_pixels = round (layout.beat_pixels * ratio * this.adata.hzoom);
  layout.virt_width = floor (384 * floor (this.adata.last_tick / 384));
  layout.octaves = round (layout.cssheight / layout.oct_length);
  layout.thickness = Math.max (round (ratio * 0.5), 1);
  layout.yoffset = layout_height - layout.thickness;		// leave a pixel for overlapping piano key borders
  layout.row = round (ratio * layout.row * this.adata.vzoom);
  layout.oct_length = layout.row * 12;
  layout.white_width = round (layout.white_width * ratio);
  layout.black_width = round (layout.white_width * layout.black_width);
  // hscrollbar setup
  if (this.$refs.hscrollbar)
    {
      if (this.$refs.hscrollbar.percentage !== undefined)
	this.$refs.hscrollbar.percentage = layout.notes_csswidth / layout.virt_width;
      layout.xscroll = this.$refs.hscrollbar.value * layout.virt_width;
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

function render_piano (canvas, cstyle, layout) {
  const ctx = canvas.getContext ('2d'), csp = cstyle.getPropertyValue.bind (cstyle);
  // resize canvas to match onscreen pixels, paint bg with white key row color
  const light_row = csp ('--piano-roll-light-row');
  Util.resize_canvas (canvas, layout.piano_csswidth, layout.cssheight, light_row);
  const piano_buttons = this.$refs['piano-roll-buttons'];
  piano_buttons.style.width = layout.piano_csswidth + 'px';
  // we draw piano keys horizontally within their boundaries, but verticaly overlapping by one th
  const th = layout.thickness, hf = th * 0.5; // thickness/2 fraction

  // draw piano keys
  const white_base = csp ('--piano-roll-white-base');
  const white_glint = csp ('--piano-roll-white-glint');
  const white_border = csp ('--piano-roll-white-border');
  const black_base = csp ('--piano-roll-black-base');
  const black_glint = csp ('--piano-roll-black-glint');
  const black_shine = csp ('--piano-roll-black-shine');
  const black_border = csp ('--piano-roll-black-border');
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    // draw white keys
    ctx.fillStyle = white_base;
    ctx.lineWidth = th;
    for (let k = 0; k < layout.wkeys.length; k++) {
      const p = layout.wkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.white_width, h = p[1];
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = white_glint;		// highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = white_border;		// border
      ctx.strokeRect (sx, sy, w - th, h);			// v-overlap by 1*th
    }
    // draw black keys
    ctx.fillStyle = black_base;
    ctx.lineWidth = th;
    for (let k = 0; k < layout.bkeys.length; k++) {
      const p = layout.bkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.black_width, h = p[1];
      const gradient = [ [0, black_base], [.08, black_base], [.15, black_shine],   [1, black_base] ];
      ctx.fillStyle = Util.linear_gradient_from (ctx, gradient, x + th, y - h / 2, x + w - 2 * th, y - h / 2);
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = black_glint;		// highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = black_border;		// border
      ctx.strokeRect (sx, sy, w - th, h);
    }
  }

  // figure font size for piano key labels
  const avg_height = layout.wkeys.reduce ((a, p) => a += p[1], 0) / layout.wkeys.length;
  const fpx = avg_height - 2 * (th + 1);	// base font size on average white key size
  if (fpx >= 6) {
    const key_font = csp ('--piano-roll-key-font');
    const key_font_color = csp ('--piano-roll-key-font-color');
    const fpx_parts = key_font.split (/\s*\d+px\s*/i); // 'bold 10px sans' -> [ ['bold', 'sans']
    ctx.font = fpx_parts[0] + ' ' + fpx + 'px ' + (fpx_parts[1] || '');
    ctx.fillStyle = key_font_color;
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

function render_notes (canvas, cstyle, layout) {
  const ctx = canvas.getContext ('2d'), csp = cstyle.getPropertyValue.bind (cstyle);
  const light_row = cstyle.getPropertyValue ('--piano-roll-light-row');
  // resize canvas to match onscreen pixels, paint bg with white key row color
  Util.resize_canvas (canvas, layout.notes_csswidth, layout.cssheight, light_row);
  // we draw piano keys verticaly overlapping by one th and align octave separators accordingly
  const th = layout.thickness;

  // paint black key rows
  const dark_row = csp ('--piano-roll-dark-row');
  ctx.fillStyle = dark_row;
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
  const semitone6 = csp ('--piano-roll-semitone6');
  ctx.fillStyle = semitone6;
  const stipple = round (3 * window.devicePixelRatio), stipple2 = 2 * stipple;
  const qy = layout.wkeys[3][0]; // separator between F|G
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    Util.hstippleRect (ctx, th - lsx % stipple2, oy - qy, canvas.width, th, stipple);
  }

  // draw vertical grid lines
  const grid_main1 = csp ('--piano-roll-grid-main1');
  const grid_sub1 = csp ('--piano-roll-grid-sub1');
  const gy1 = layout.yoffset - layout.octaves * layout.oct_length + th;
  const gy2 = layout.yoffset; // align with outer piano border
  const beat_dist = layout.beat_pixels;
  const grid_divider = 4 * beat_dist; // largest grid divider
  let grid = floor (lsx / grid_divider);
  for (let gx = 0; gx - lsx < canvas.width; grid++) {
    gx = grid * beat_dist;
    if (grid % 4 == 0) {
      ctx.fillStyle = grid_main1;
    } else {
      ctx.fillStyle = grid_sub1;
    }
    ctx.fillRect (gx - lsx, gy1, th, gy2 - gy1);
  }

  // draw octave separators
  const semitone12 = csp ('--piano-roll-semitone12');
  ctx.fillStyle = semitone12;
  for (let oct = 0; oct <= layout.octaves; oct++) {	// condiiton +1 to include top border
    const oy = layout.yoffset - oct * layout.oct_length;
    ctx.fillRect (0, oy, canvas.width, th);
  }

  // paint notes
  if (!this.adata.pnotes)
    return;
  const note_font = csp ('--piano-roll-note-font');
  const note_font_color = csp ('--piano-roll-note-font-color');
  const fpx_parts = note_font.split (/\s*\d+px\s*/i); // 'bold 10px sans' -> [ ['bold', 'sans']
  const fpx = layout.row - 2;
  ctx.font = fpx_parts[0] + ' ' + fpx + 'px ' + (fpx_parts[1] || '');
  ctx.fillStyle = note_font_color;
  // draw notes
  const tickscale = layout.beat_pixels / 384;
  for (const note of this.adata.pnotes)
    {
      const oct = floor (note.note / 12), key = note.note - oct * 12;
      const ny = layout.yoffset - oct * layout.oct_length - key * layout.row;
      const nx = round (note.tick * tickscale), nw = Math.max (1, round (note.duration * tickscale));
      ctx.fillRect (nx - lsx, ny - layout.row, nw, layout.row);
    }
}

</script>
