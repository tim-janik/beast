<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-FLEX
  A [flex](https://www.w3.org/TR/css-flexbox-1/#flex-containers) container for horizontal or vertical layouting.
  ## Props:
  *inline*
  : Use 'display: inline-flex' as layout mode, e.g. to integrate this container into text flow.
  *reverse*
  : Reverse the layout direction, i.e. layout from bottom → top, or right → left.
  *start*
  : Children are packed towards the start of the flex direction:
  *end*
  : Children are packed towards the end of the flex direction:
  *center*
  : Children are packed towards the center of the flex direction:
  *space-between*
  : Additional space is distributed between the children.
  *space-around*
  : Additional space is evenly distributed between the children and half a spacing around the edges.
  *space-evenly*
  : Children will have additional space evenly distributed around them and the edges.
  *grow*:
  : Grow space for this element if additional space is available.
  *shrink*
  : Shrink space for this element if space is too tight.
  *wrap*
  : Layout of the flexbox children may wrap if the elements use more than 100%.
  *wrap-reverse*
  : Layout of the flexbox children may wrap if the elements use more than 100%, in reverse order.
  ## Slots:
  : The *default* slot holds the contextmenu content.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-flex {
    display: flex; flex-grow: 0; flex-shrink: 0; flex-basis: auto;
    flex-wrap: nowrap;
    justify-content: space-evenly;	//* distribute extra main-axis space */
    align-items: stretch;		//* distribute extra cross-axis space */
    align-content: stretch;		//* distribute extra cross-axis space for multi-line layouts */
    &.inline	{ display: inline-flex; }
    &.flex-row {
      flex-direction: row;
      &.reverse	{ flex-direction: row-reverse; } }
    &.flex-column {
      flex-direction: column;
      &.reverse	{ flex-direction: column-reverse; } }
    &.shrink		{ flex-shrink: 1; flex-grow: 0; }
    &.grow		{ flex-grow: 1; }
    &.wrap		{ flex-wrap: wrap; }
    &.wrap-reverse	{ flex-wrap: wrap-reverse; }
  }
  .b-flex-start		{ justify-content: flex-start; }
  .b-flex-end		{ justify-content: flex-end; }
  .b-flex-center	{ justify-content: center; }
  .b-flex-space-between	{ justify-content: space-between; }
  .b-flex-space-around	{ justify-content: space-around; }
  .b-flex-space-evenly	{ justify-content: space-evenly; }
</style>

<template>
  <div
      class="b-flex"
      :class="classlist()"
  ><slot></slot></div>
</template>

<script>
module.exports = {
  name: 'b-flex',
  methods: {
    classlist() {
      let cl = [];
      if (this.derivedclass)
	cl.push (this.derivedclass);
      const classnames = [
	'flex-column', 'flex-row',
	'start',
	'end',
	'center',
	'space-between',
	'space-around',
	'space-evenly',
	'grow', 'shrink',
	'wrap', 'wrap-reverse',
	'inline',
	'reverse',
      ];
      for (let cssclass of classnames)
	if (this.$attrs[cssclass] == "" || this.$attrs[cssclass])
	  cl.push ('b-flex-' + cssclass);
      return cl;
    },
  },
};
</script>
