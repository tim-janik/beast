<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-BUTTON
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
  .vc-button                  	{ @include vc-buttonshade; display: inline-flex; text-align: center; margin: 0; padding: 3px 1em; }
  .vc-button:focus            	{ outline: $vc-focus-outline; }
  .vc-button:hover            	{ @include vc-buttonhover; }
  .vc-button.active,
  .vc-button:active           	{ @include vc-buttonactive; }
  .vc-button, .vc-button *	{ color: $vc-button-foreground; fill: $vc-button-foreground !important; }
  .vc-button.active *,	/* use '*' + fill!important to include svg elements in buttons */
  .vc-button:active *         	{ color: $vc-button-active-fg;  fill: $vc-button-active-fg !important; }
</style>

<template>
  <button ref="btn"
	  class="vc-button"
	  @click="emit ('click', $event)"
  ><slot class="vc-slot"></slot></button>
</template>

<script>
module.exports = {
  name: 'vc-button',
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
