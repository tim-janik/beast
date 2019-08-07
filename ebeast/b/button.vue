<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-BUTTON
  A button element that responds to clicks.
  ## Props:
  *hotkey*
  : Keyboard accelerator to trigger a *click* event.
  ## Events:
  *click*
  : A *click* event is emitted for activation through mouse buttons, Space or Enter keys.
  ## Slots:
  *default*
  : All contents passed into this element will be rendered as contents of the button.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-button                  	{ @include b-buttonshade; display: inline-flex; text-align: center; margin: 0; padding: 3px 1em; }
  .b-button:focus            	{ outline: $b-focus-outline; }
  .b-button:hover            	{ @include b-buttonhover; }
  .b-button.active,
  .b-button:active           	{ @include b-buttonactive; }
  .b-button, .b-button *	{ color: $b-button-foreground; fill: $b-button-foreground !important; }
  .b-button.active *,	/* use '*' + fill!important to include svg elements in buttons */
  .b-button:active *         	{ color: $b-button-active-fg;  fill: $b-button-active-fg !important; }
</style>

<template>
  <button ref="btn"
	  class="b-button"
	  @click="emit ('click', $event)"
  ><slot class="b-slot"></slot></button>
</template>

<script>
module.exports = {
  name: 'b-button',
  props: [ 'hotkey' ],
  methods: {
    emit (what, ev) {
      this.$emit (what, ev);
    },
  },
  mounted: function () { // `this` points to the Vue instance
    if (this.hotkey)
      $(this.$refs.btn).click_hotkey (this.hotkey);
  },
};
</script>