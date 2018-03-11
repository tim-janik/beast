<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-part-thumb - Vue template to display a part thumbnail
  ### Props:
  - **part** - The BsePart to display.
</docs>

<template>

  <span class="vc-part-thumb" @click="$forceUpdate()">
    <canvas ref="canvas" width="100%" height="20px"></canvas>
  </span>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-part-thumb {
    font-size: 80%;
    border: 1px solid $vc-button-border;
    border-radius: $vc-button-radius; }
  .vc-part-thumb > *	{ margin: 0; }
  .vc-part-thumb canvas {
    --background-color: hsv(190, 75%, 67%);
    --font-color: hsv(0, 0%, 100%, 0.75);
    --font: bold 8px sans-serif; /* italic small-caps bold 12px serif */
  }
</style>

<script>
function render_canvas () {
  const canvas = this.$refs['canvas'], ctx = canvas.getContext ('2d');
  const style = getComputedStyle (canvas);
  const width = canvas.clientWidth, height = canvas.clientHeight;
  ctx.clearRect (0, 0, width, height);
  // paint part
  ctx.fillStyle = style.getPropertyValue ('--background-color');
  ctx.fillRect (0, 0, width, height);
  // draw name
  ctx.fillStyle = style.getPropertyValue ('--font-color');
  ctx.font = style.getPropertyValue ('--font');
  ctx.textAlign = 'left';
  ctx.textBaseline = 'top';
  ctx.fillText (this.part.get_name(), 1.5, .5);
}
module.exports = {
  name: 'vc-part-thumb',
  props: {
    'part': { type: Bse.Part, },
  },
  methods: {
  },
  data_tmpl: {
    name: "Part-Label-XYZ",
  },
  customized_render (createElement, result) {
    // queue render_canvas as async µtask so $el and the canvas are present
    Promise.resolve().then (() => render_canvas.call (this));
    return result;
  },
};
</script>
