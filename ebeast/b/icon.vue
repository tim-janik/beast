<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-ICON
  An element to display icons (usually SVGs).
  Note, to style the color of SVG based symbols, apply the `fill` CSS property to this element.
  ## Props:
  *fa*
  : The name of a "Font Awesome" 4 icon, see the [Font Awesome Icons](https://fontawesome.com/v4.7.0/icons/).
  *mi*
  : The name of a "Material Icons" icon, see the [Material Design Icons](https://material.io/tools/icons/).
  *uc*
  : A unicode character literal, see the [Unicode symbols block list](https://en.wikipedia.org/wiki/Unicode_symbols#Symbol_block_list).
  *nosize*
  : Prevent the element from applying default size constraints.
  *fw*
  : Apply fixed-width sizing.
  *lg*
  : Make the icon 33% larger than its container.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-icon {
    margin: 0; padding: 0; text-align: center;
    &.b-icon-dfl { height: 1em; width: 1em; }
    &.b-icon-fw	 { height: 1em; width: 1.28571429em; text-align: center; }
    &.b-icon-lg	 { font-size: 1.33333333em; line-height: 0.75em; vertical-align: -15%; }
    &.material-icons.b-icon-dfl { font-size: 1em; }
  }
</style>

<template>
  <svg v-if="fa" class="b-icon" :class="iconclasses" role="icon" aria-hidden="true">
    <use :href="'assets/fa-sprites.svg#' + fa" />
  </svg>
  <i v-else-if="mi" class="b-icon material-icons" :class="iconclasses" role="icon" aria-hidden="true">{{ mi }}</i>
  <span v-else-if="uc" class="b-icon" :class="iconclasses" role="icon" aria-hidden="true">{{ uc }}</span>
  <span v-else-if="1" class="b-icon" :class="iconclasses" role="icon" aria-hidden="true"><slot /></span>
</template>
<!-- SVG-1.1 notation: <use xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="...svg"></use> -->

<script>
module.exports = {
  name: 'b-icon',
  props: { 'fa': undefined, 'mi': undefined, 'uc': undefined,
	   'nosize': undefined, 'fw': undefined, 'lg': undefined },
  computed: {
    iconclasses() {
      let classes = [];
      if (this.fw || this.fw === '')
	classes.push ('b-icon-fw');
      else if (!(this.nosize || this.nosize == ''))
	classes.push ('b-icon-dfl');
      if (this.lg || this.lg == '')
	classes.push ('b-icon-lg');
      if (this.fa)
	classes.push ('fa-' + this.fa);
      return classes.join (' ');
    },
  },
  methods: {
    emit (what, ev) {
      this.$emit (what, ev);
    },
  },
};
</script>
