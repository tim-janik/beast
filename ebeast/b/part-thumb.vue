<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PART-THUMB
  A Vue template to display a thumbnail for a Bse.Part.
  ## Props:
  *part*
  : The Bse.Part to display.
  *tick*
  : The Bse.Track tick position.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-part-thumb {
    display: inline-block; position: absolute; top: 0px; bottom: 0px;
    height: $b-trackrow-height;
    border-radius: $b-theme-border-radius * 0.66;
    --part-thumb-font-color: #{$b-part-thumb-font-color}; --part-thumb-font: #{$b-part-thumb-font};
    --part-thumb-note-color: #{$b-part-thumb-note-color}; --part-thumb-colors: #{$b-part-thumb-colors};
    background-color: #222;
    box-shadow: inset 0px 0 1px #fff9, inset -1px 0 1px #000;
  }
</style>

<template>
  <canvas ref="canvas" class="b-part-thumb" @click="App.open_piano_roll (part)"
	  :style="{ left: pxoffset + 'px', width: canvas_width + 'px', }" ></canvas>
</template>

<script>
const tick_quant = 384; // FIXME

function observable_part_data () {
  const data = {
    partname: { getter: c => this.part.get_name(),      notify: n => this.part.on ("notify:uname", n), },
    lasttick: { getter: c => this.part.get_last_tick(), notify: n => this.part.on ("notify:last_tick", n), },
    allnotes: { default: [],                            notify: n => this.part.on ("noteschanged", n),
		getter: async c => Object.freeze (await this.part.list_notes_crossing (0, CONFIG.MAXINT)), },
  };
  return this.observable_from_getters (data, () => this.part);
}

export default {
  name: 'b-part-thumb',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    part: { type: Bse.Part, },
    tick: { type: Number, },
    index: { type: Number, },
    trackindex: { type: Number, },
  },
  data() { return observable_part_data.call (this); },
  computed: {
    tickscale:    function() { return 10 / 384.0; }, // FIXME
    pxoffset:     function() { return this.tick * this.tickscale; }, // FIXME
    canvas_width: function() {
      return this.tickscale * Math.floor ((this.lasttick + tick_quant - 1) / tick_quant) * tick_quant;
    },
  },
  methods: {
    dom_update() {
      if (this.lasttick)
	render_canvas.call (this);
    },
  },
};

function render_canvas () {
  // canvas setup
  const canvas = this.$refs['canvas'];
  const pixelratio = Util.resize_canvas (canvas, canvas.clientWidth, canvas.clientHeight, true);
  const ctx = canvas.getContext ('2d'), cstyle = getComputedStyle (canvas), csp = cstyle.getPropertyValue.bind (cstyle);
  const width = canvas.width, height = canvas.height;
  const tickscale = this.tickscale * pixelratio;
  //const width = canvas.clientWidth, height = canvas.clientHeight;
  //canvas.width = width; canvas.height = height;
  ctx.clearRect (0, 0, width, height);
  // color setup
  const colors = Util.split_comma (csp ('--part-thumb-colors'));
  let cindex;
  cindex = this.trackindex;			// - color per track
  cindex = (cindex + 1013904223) * 1664557;	//   LCG randomization step
  cindex = this.index;				// - color per part
  cindex = Util.fnv1a_hash (this.partname);	// - color from part name
  const bgcol = colors[(cindex >>> 0) % colors.length];
  // paint part background
  ctx.fillStyle = bgcol;
  ctx.fillRect (0, 0, width, height);
  // draw name
  const fpx = height / 3;
  const note_font = csp ('--part-thumb-font');
  const fpx_parts = note_font.split (/\s*\d+px\s*/i); // 'bold 10px sans' -> [ ['bold', 'sans']
  ctx.font = fpx_parts[0] + ' ' + fpx + 'px ' + (fpx_parts[1] || '');
  ctx.fillStyle = csp ('--part-thumb-font-color');
  ctx.textAlign = 'left';
  ctx.textBaseline = 'top';
  ctx.fillText (this.partname, 1.5, .5);
  // paint notes
  ctx.fillStyle = csp ('--part-thumb-note-color');
  const pnotes = this.allnotes; // await part.list_notes_crossing (0, MAXINT);
  const noteoffset = 12;
  const notescale = height / (123.0 - 2 * noteoffset); // MAX_NOTE
  for (const note of pnotes) {
    ctx.fillRect (note.tick * tickscale, height - (note.key - noteoffset) * notescale, note.duration * tickscale, 1 * pixelratio);
  }
}

</script>
