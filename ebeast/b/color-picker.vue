<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-COLOR-PICKER
  Vue template to display a color picker popup.
  ## Props:
  *initial-color*
  : The initial color to display.
  ## data:
  *color*
  : The currently selected color.
</docs>

<style lang="scss">
  @import 'mixins.scss';

  .b-color-picker {
    * { flex-shrink: 0; }
    button { cursor: pointer; }
  }

  .b-color-picker-dropdown {
    display: flex; flex-direction: column; padding: 10px;
    position: fixed; right: auto; bottom: auto;
    z-index: 9999;
    box-shadow: 3px 3px 16px 0px rgba(0,0,0,0.9);
    background-color: #4e4e4e; color: #f1f1f1;

    $timing: 0.2s;
    &.v-enter-active 		{ transition: opacity $timing ease-out, transform $timing/2 linear; }
    &.v-leave-active		{ transition: opacity $timing ease-in,  transform $timing linear; }
    &.v-enter	 		{ opacity: 0; transform: translateX(-5vw) translateY(-10vh) scale(1); }
    &.v-leave-to	 	{ opacity: 0.5; transform: translateX(-50%) translateY(-50%) scale(0); }

    .b-color-picker-entry {
      margin: 1px;
      width:  20px;
      height: 20px;
      border: 1px solid #777;
    }

    .b-data-tooltip {
      position: relative;
      text-decoration: none;
    }
    .b-data-tooltip:before, .b-data-tooltip:after {
      position: absolute; bottom: 100%;
      visibility: hidden; opacity: 0; pointer-events: none;
      transition: all 0.2s ease-in-out 0.15s;
    }
    .b-data-tooltip:before {
      left: 20%; padding: 5px 11px;
      box-shadow: 0px 0px 3px 0px rgba(0,0,0,0.8) , 0px 0px 6px 1px rgba(255,255,255,0.3) ;
      color: #fef033; background: linear-gradient(#333, #2a2a2a);
      content: attr(data-tooltip); white-space: nowrap;
      border-radius: 7px;
      margin-left: -7px;
      margin-bottom: 9px;
    }
    .b-data-tooltip:after { /* tooltip bubble triangle */
      left: 50%; width: 0; height: 0; content: "";
      border-top: 9px solid #2a2a2a; border-right: 9px solid transparent;
    }
    .b-data-tooltip:hover:after, .b-data-tooltip:hover:before {
      visibility: visible;
      opacity: 1;
    }

  }
</style>

<template>
  <div class="b-color-picker" style="position: relative; display: flex;" >
    <button ref="button" :style="{ 'background-color': color, color: contrast }" @click="visible_dropdown++" ><slot>⁜</slot></button>
    <transition @before-leave="intransition++" @after-leave="end_transitions" >
      <div v-if="visible_dropdown" ref="dropdown" class="b-color-picker-dropdown" >
	<div style="display: flex; flex-direction: row;"
	     v-for="(row, row_index) in color_rows" :key="'row-' + row_index" >
	  <div class="b-color-picker-entry b-data-tooltip"
	       v-for="(item, index) in row"
	       @click="select (item[0])"
	       :data-tooltip=" item[1] + '   ' + item[0] + '' "
	       :key="row_index + '.' + index" :style="{ 'background-color': item[0] }" ></div>
	</div>
      </div>
    </transition>
  </div>
</template>

<script>
export default {
  name: 'b-color-picker',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    'initial-color': { type: String, default: '#808080' },
  },
  data: function() {
    return {
      value: this['initial-color'],
      intransition: 0,
      visible_dropdown: 0,
  }; },
  computed: {
    contrast: function () { return Util.parse_hex_pgrey (this.value) > 0x7f ? 'rgba(0,0,0,.5)' : 'rgba(255,255,255,.5)'; },
    color: {
      get ()  { return this.value; },
      set (v) {
	if (v.match (/#[a-f0-9]{6}/i)) {
	  this.value = v;
	  this.$emit ('input', this.value);
	}
      } },
  },
  created () {
    this.color_rows = smooth_colors;
  },
  mounted () {
    // sample usage:
    // this.$on ('input', hex => console.log (hex));
  },
  beforeDestroy () {
    if (this.shield)
      this.shield.destroy (false);
  },
  methods: {
    update_shield() {
      if (this.visible_dropdown && !this.shield)
	this.shield = Util.modal_shield (this.$refs.dropdown, { close: this.hide });
      else if (!this.visible_dropdown && this.shield && !this.intransition)
	{
	  this.shield.destroy();
	  this.shield = null;
	}
    },
    dom_update() {
      this.update_shield();
      if (this.$refs.dropdown)
	{
	  const p = Util.popup_position (this.$refs.dropdown, { origin: this.$refs.button });
	  this.$refs.dropdown.style.left = p.x + "px";
	  this.$refs.dropdown.style.top = p.y + "px";
	}
    },
    end_transitions() {
      this.intransition = 0;
      this.update_shield();
    },
    hide() {
      this.visible_dropdown = 0;
      if (this.shield)
	{
	  this.shield.destroy (false); // false prevents recursion
	  this.shield = undefined;
	}
    },
    select (hex) {
      if (hex.match (/#[a-f0-9]{6}/i))
	this.color = hex;
      this.hide();
    },
  },
};

// Color palette, based on https://forum.openoffice.org/en/forum/viewtopic.php?f=6&t=14529#p77662
const smooth_colors = [ [
  ['#000000', 'Black'], ['#0f0f0f', 'Gray Bri   6%'], ['#1f1f1f', 'Gray Bri  12%'], ['#2f2f2f', 'Gray Bri  19%'], ['#3f3f3f', 'Gray Bri  25%'],
  ['#4f4f4f', 'Gray Bri  31%'], ['#5f5f5f', 'Gray Bri  37%'], ['#6f6f6f', 'Gray Bri  44%'], ['#7f7f7f', 'Gray Bri  50%'],
  ['#8f8f8f', 'Gray Bri  57%'], ['#9f9f9f', 'Gray Bri  63%'], ['#afafaf', 'Gray Bri  70%'], ['#bfbfbf', 'Gray Bri  76%'],
  ['#cfcfcf', 'Gray Bri  82%'], ['#dfdfdf', 'Gray Bri  88%'], ['#efefef', 'Gray Bri  94%'], ['#ffffff', 'White'],
], [
  ['#4f4f00', 'Yellow Bri  31%'], ['#5f5f00', 'Yellow Bri  37%'], ['#6f6f00', 'Yellow Bri  44%'], ['#7f7f00', 'Yellow Bri  50%'], ['#8f8f00', 'Yellow Bri  57%'],
  ['#9f9f00', 'Yellow Bri  63%'], ['#afaf00', 'Yellow Bri  70%'], ['#bfbf00', 'Yellow Bri  76%'], ['#cfcf00', 'Yellow Bri  82%'], ['#dfdf00', 'Yellow Bri  88%'],
  ['#ffff00', 'Yellow 100%'], ['#ffff50', 'Yellow Sat 68%'], ['#ffff70', 'Yellow Sat 56%'], ['#ffff80', 'Yellow Sat 50%'], ['#ffff90', 'Yellow Sat 44%'],
  ['#ffffa0', 'Yellow Sat 38%'], ['#ffffb0', 'Yellow Sat 32%'], ['#ffffc0', 'Yellow Sat 25%'], ['#ffffd0', 'Yellow Sat 19%'], ['#fffff0', 'Yellow Sat  5%'],
], [
  ['#4f2700', 'Orange Bri  31%'], ['#5f2f00', 'Orange Bri  37%'], ['#6f3700', 'Orange Bri  44%'], ['#7f3f00', 'Orange Bri  50%'], ['#8f4700', 'Orange Bri  57%'],
  ['#9f4f00', 'Orange Bri  63%'], ['#af5700', 'Orange Bri  70%'], ['#bf5f00', 'Orange Bri  76%'], ['#cf6700', 'Orange Bri  82%'], ['#df6f00', 'Orange Bri  88%'],
  ['#ff7f00', 'Orange 100%'], ['#ff9f3f', 'Orange Sat  76%'], ['#ffa74f', 'Orange Sat  70%'], ['#ffaf5f', 'Orange Sat  63%'], ['#ffb76f', 'Orange Sat  57%'],
  ['#ffbf7f', 'Orange Sat  50%'], ['#ffc78f', 'Orange Sat  44%'], ['#ffcf9f', 'Orange Sat  37%'], ['#ffd7af', 'Orange Sat  31%'], ['#ffefdf', 'Orange Sat  12%'],
], [
  ['#4f0000', 'Red Bri  31%'], ['#5f0000', 'Red Bri  37%'], ['#6f0000', 'Red Bri  44%'], ['#7f0000', 'Red Bri  50%'], ['#8f0000', 'Red Bri  57%'],
  ['#9f0000', 'Red Bri  63%'], ['#af0000', 'Red Bri  70%'], ['#bf0000', 'Red Bri  76%'], ['#cf0000', 'Red Bri  82%'], ['#df0000', 'Red Bri  88%'],
  ['#ff0000', 'Red 100%'], ['#ff5050', 'Red Sat 68%'], ['#ff7070', 'Red Sat 56%'], ['#ff8080', 'Red Sat 50%'], ['#ff9090', 'Red Sat 44%'],
  ['#ffa0a0', 'Red Sat 38%'], ['#ffb0b0', 'Red Sat 32%'], ['#ffc0c0', 'Red Sat 25%'], ['#ffd0d0', 'Red Sat 19%'], ['#fff0f0', 'Red Sat  5%'],
], [
  ['#4f0027', 'Rose Bri  31%'], ['#5f002f', 'Rose Bri  37%'], ['#6f0037', 'Rose Bri  44%'], ['#7f003f', 'Rose Bri  50%'], ['#8f0047', 'Rose Bri  57%'],
  ['#9f004f', 'Rose Bri  63%'], ['#af0057', 'Rose Bri  70%'], ['#bf005f', 'Rose Bri  76%'], ['#cf0067', 'Rose Bri  82%'], ['#df006f', 'Rose Bri  88%'],
  ['#ff007f', 'Rose 100%'], ['#ff4fa7', 'Rose Sat  70%'], ['#ff5faf', 'Rose Sat  63%'], ['#ff6fb7', 'Rose Sat  57%'], ['#ff7fbf', 'Rose Sat  50%'],
  ['#ff8fc7', 'Rose Sat  44%'], ['#ff9fcf', 'Rose Sat  37%'], ['#ffafd7', 'Rose Sat  31%'], ['#ffcfe7', 'Rose Sat  19%'], ['#ffdfef', 'Rose Sat  12%'],
], [
  ['#4f004f', 'Magenta Bri  31%'], ['#5f005f', 'Magenta Bri  37%'], ['#6f006f', 'Magenta Bri  44%'], ['#7f007f', 'Magenta Bri  50%'], ['#8f008f', 'Magenta Bri  57%'],
  ['#9f009f', 'Magenta Bri  63%'], ['#af00af', 'Magenta Bri  70%'], ['#bf00bf', 'Magenta Bri  76%'], ['#cf00cf', 'Magenta Bri  82%'], ['#df00df', 'Magenta Bri  88%'],
  ['#ff00ff', 'Magenta 100%'], ['#ff50ff', 'Magenta Sat 68%'], ['#ff70ff', 'Magenta Sat 56%'], ['#ff80ff', 'Magenta Sat 50%'], ['#ff90ff', 'Magenta Sat 44%'],
  ['#ffa0ff', 'Magenta Sat 38%'], ['#ffb0ff', 'Magenta Sat 32%'], ['#ffc0ff', 'Magenta Sat 25%'], ['#ffd0ff', 'Magenta Sat 19%'], ['#fff0ff', 'Magenta Sat  5%'],
], [
  ['#27004f', 'Violet Bri  31%'], ['#2f005f', 'Violet Bri  37%'], ['#37006f', 'Violet Bri  44%'], ['#3f007f', 'Violet Bri  50%'], ['#47008f', 'Violet Bri  57%'],
  ['#4f009f', 'Violet Bri  63%'], ['#5700af', 'Violet Bri  70%'], ['#5f00bf', 'Violet Bri  76%'], ['#6700cf', 'Violet Bri  82%'], ['#6f00df', 'Violet Bri  88%'],
  ['#7f00ff', 'Violet 100%'], ['#a74fff', 'Violet Sat  70%'], ['#af5fff', 'Violet Sat  63%'], ['#b76fff', 'Violet Sat  57%'], ['#bf7fff', 'Violet Sat  50%'],
  ['#c78fff', 'Violet Sat  44%'], ['#cf9fff', 'Violet Sat  37%'], ['#d7afff', 'Violet Sat  31%'], ['#e7cfff', 'Violet Sat  19%'], ['#efdfff', 'Violet Sat  12%'],
], [
  ['#00004f', 'Blue Bri  31%'], ['#00005f', 'Blue Bri  37%'], ['#00006f', 'Blue Bri  44%'], ['#00007f', 'Blue Bri  50%'], ['#00008f', 'Blue Bri  57%'],
  ['#00009f', 'Blue Bri  63%'], ['#0000af', 'Blue Bri  70%'], ['#0000bf', 'Blue Bri  76%'], ['#0000cf', 'Blue Bri  82%'], ['#0000df', 'Blue Bri  88%'],
  ['#0000ff', 'Blue 100%'], ['#5050ff', 'Blue Sat 68%'], ['#7070ff', 'Blue Sat 56%'], ['#8080ff', 'Blue Sat 50%'], ['#9090ff', 'Blue Sat 44%'],
  ['#a0a0ff', 'Blue Sat 38%'], ['#b0b0ff', 'Blue Sat 32%'], ['#c0c0ff', 'Blue Sat 25%'], ['#d0d0ff', 'Blue Sat 19%'], ['#f0f0ff', 'Blue Sat  5%'],
], [
  ['#00274f', 'Azure Bri  31%'], ['#002f5f', 'Azure Bri  37%'], ['#00376f', 'Azure Bri  44%'], ['#003f7f', 'Azure Bri  50%'], ['#00478f', 'Azure Bri  57%'],
  ['#004f9f', 'Azure Bri  63%'], ['#0057af', 'Azure Bri  70%'], ['#005fbf', 'Azure Bri  76%'], ['#0067cf', 'Azure Bri  82%'], ['#006fdf', 'Azure Bri  88%'],
  ['#007fff', 'Azure 100%'], ['#4fa7ff', 'Azure Sat  70%'], ['#5fafff', 'Azure Sat  63%'], ['#6fb7ff', 'Azure Sat  57%'], ['#7fbfff', 'Azure Sat  50%'],
  ['#8fc7ff', 'Azure Sat  44%'], ['#9fcfff', 'Azure Sat  37%'], ['#afd7ff', 'Azure Sat  31%'], ['#cfe7ff', 'Azure Sat  19%'], ['#dfefff', 'Azure Sat  12%'],
], [
  ['#004f4f', 'Cyan Bri  31%'], ['#005f5f', 'Cyan Bri  37%'], ['#006f6f', 'Cyan Bri  44%'], ['#007f7f', 'Cyan Bri  50%'], ['#008f8f', 'Cyan Bri  57%'],
  ['#009f9f', 'Cyan Bri  63%'], ['#00afaf', 'Cyan Bri  70%'], ['#00bfbf', 'Cyan Bri  76%'], ['#00cfcf', 'Cyan Bri  82%'], ['#00dfdf', 'Cyan Bri  88%'],
  ['#00ffff', 'Cyan 100%'], ['#50ffff', 'Cyan Sat 68%'], ['#70ffff', 'Cyan Sat 56%'], ['#80ffff', 'Cyan Sat 50%'], ['#90ffff', 'Cyan Sat 44%'],
  ['#a0ffff', 'Cyan Sat 38%'], ['#b0ffff', 'Cyan Sat 32%'], ['#c0ffff', 'Cyan Sat 25%'], ['#d0ffff', 'Cyan Sat 19%'], ['#f0ffff', 'Cyan Sat  5%'],
], [
  ['#004f27', 'Aquamarine Bri  31%'], ['#005f2f', 'Aquamarine Bri  37%'], ['#006f37', 'Aquamarine Bri  44%'], ['#007f3f', 'Aquamarine Bri  50%'], ['#008f47', 'Aquamarine Bri  57%'],
  ['#009f4f', 'Aquamarine Bri  63%'], ['#00af57', 'Aquamarine Bri  70%'], ['#00bf5f', 'Aquamarine Bri  76%'], ['#00cf67', 'Aquamarine Bri  82%'], ['#00df6f', 'Aquamarine Bri  88%'],
  ['#00ff7f', 'Aquamarine 100%'], ['#4fffa7', 'Aquamarine Sat  70%'], ['#5fffaf', 'Aquamarine Sat  63%'], ['#6fffb7', 'Aquamarine Sat  57%'], ['#7fffbf', 'Aquamarine Sat  50%'],
  ['#8fffc7', 'Aquamarine Sat  44%'], ['#9fffcf', 'Aquamarine Sat  37%'], ['#afffd7', 'Aquamarine Sat  31%'], ['#cfffe7', 'Aquamarine Sat  19%'], ['#dfffef', 'Aquamarine Sat  12%'],
], [
  ['#274f00', 'Chartreuse Bri  31%'], ['#2f5f00', 'Chartreuse Bri  37%'], ['#376f00', 'Chartreuse Bri  44%'], ['#3f7f00', 'Chartreuse Bri  50%'], ['#478f00', 'Chartreuse Bri  57%'],
  ['#4f9f00', 'Chartreuse Bri  63%'], ['#57af00', 'Chartreuse Bri  70%'], ['#5fbf00', 'Chartreuse Bri  76%'], ['#67cf00', 'Chartreuse Bri  82%'], ['#6fdf00', 'Chartreuse Bri  88%'],
  ['#7fff00', 'Chartreuse 100%'], ['#a7ff4f', 'Chartreuse Sat  70%'], ['#afff5f', 'Chartreuse Sat  63%'], ['#b7ff6f', 'Chartreuse Sat  57%'], ['#bfff7f', 'Chartreuse Sat  50%'],
  ['#c7ff8f', 'Chartreuse Sat  44%'], ['#cfff9f', 'Chartreuse Sat  37%'], ['#d7ffaf', 'Chartreuse Sat  31%'], ['#e7ffcf', 'Chartreuse Sat  19%'], ['#efffdf', 'Chartreuse Sat  12%'],
], [
  ['#004f00', 'Green Bri  31%'], ['#005f00', 'Green Bri  37%'], ['#006f00', 'Green Bri  44%'], ['#007f00', 'Green Bri  50%'], ['#008f00', 'Green Bri  57%'],
  ['#009f00', 'Green Bri  63%'], ['#00af00', 'Green Bri  70%'], ['#00bf00', 'Green Bri  76%'], ['#00cf00', 'Green Bri  82%'], ['#00df00', 'Green Bri  88%'],
  ['#00ff00', 'Green 100%'], ['#50ff50', 'Green Sat 68%'], ['#70ff70', 'Green Sat 56%'], ['#80ff80', 'Green Sat 50%'], ['#90ff90', 'Green Sat 44%'],
  ['#a0ffa0', 'Green Sat 38%'], ['#b0ffb0', 'Green Sat 32%'], ['#c0ffc0', 'Green Sat 25%'], ['#d0ffd0', 'Green Sat 19%'], ['#f0fff0', 'Green Sat  5%'],
] ];
// patch smooth_colors to throw away too bright/dark colors and make it square
for (let r = 1; r < smooth_colors.length; r++) {
  smooth_colors[r] = smooth_colors[r].splice (2, smooth_colors[r].length - 2 - 1);
}
for (let r = 0; r < smooth_colors.length; r++)
  smooth_colors[r].reverse();
</script>
