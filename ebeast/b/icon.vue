<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-ICON
  An element to display icons (usually SVGs).
  Note, to style the color of SVG based symbols, apply the `fill` CSS property to this element.
  ## Props:
  *fa*
  : The name of a "Font Awesome" 4 icon, see the [Font Awesome Icons](https://fontawesome.com/v4.7.0/icons/).
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
    margin: 0; padding: 0;
    &.b-icon-dfl	{ height: 1em; width: 1em; height: 1em; }
    &.b-icon-fw	{ height: 1em; width: 1.28571429em; text-align: center; }
    &.b-icon-lg	{ font-size: 1.33333333em; line-height: 0.75em; vertical-align: -15%; }
  }
</style>

<template>
  <svg class="b-icon" :class="iconclasses" role="icon" :aria-label="fa">
    <use v-if="fa" :href="'assets/fa-sprites.svg#' + fa" />
  </svg>
</template>
<!-- SVG-1.1 notation: <use xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="...svg"></use> -->

<script>
module.exports = {
  name: 'b-icon',
  props: { 'fa': undefined,
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
