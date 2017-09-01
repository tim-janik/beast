<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-icon-button - A Button containing an icon
  ### Props:
  - **icon** - Name of the icon to be displayed by this button
  - **hotkey** - Keyboard accelerator to trigger a ''click'' event.
  ### Events:
  - **click** - A ''click'' event is emitted for activation through mouse buttons, Space or Enter keys.
  ### Slots:
  - **slot** - All contents passed into this element will be rendered as contents besides the icon.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-icon-button			{ margin: 0 0; padding: 7px; }
  .vc-icon-button .vc-icon-button-icon	{ font-size: 1.3em /*1.33333333em*/; }
</style>

<template>
  <vc-button
      class="vc-icon-button"
      :hotkey="hotkey"
      @click="emit ('click', $event)"
  ><span
       :class="iconclasses"
   ></span><slot
	       class="vc-slot"
	   ></slot></vc-button>
</template>

<script>
module.exports = {
  name: 'vc-icon-button',
  props: [ 'icon', 'hotkey' ],
  computed: {
    iconclasses() { return "vc-icon-button-icon fa fa-lg fa-fw " + this.icon; },
  },
  methods: {
    emit (what, ev) {
      this.$emit (what, ev);
    },
  },
};
</script>
