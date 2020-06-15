<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TOGGLE
  This element provides a toggle button for boolean inputs.
  It supports the Vue
  [v-model](https://vuejs.org/v2/guide/components-custom-events.html#Customizing-Component-v-model)
  protocol by emitting an `input` event on value changes and accepting inputs via the `value` prop.
  ## Props:
  *value*
  : Boolean, the toggle value to be displayed, the values are `true` or `false`.
  *label*
  : String, label to be displayed inside the toggle button.
  ## Events:
  *input (value)*
  : Value change notification event, the first argument is the new value.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-toggle {
    display: flex; position: relative;
    margin: 0; padding: 0; text-align: center;
    user-select: none;
    height: 5em;
    &:hover { background: #00f; }
  }
  .b-toggle-label {
    flex-grow: 0;
    .b-toggle-off &            	{ background: #444;
      &.b-toggle-press:hover  	{ background: #383838; } }
    .b-toggle-on &             	{ background: #f55;
      &.b-toggle-press:hover	{ background: #f33; } }
  }
</style>

<!-- NOTE: This implementation assumes the HTML embeds etoggle.svg -->

<template>
  <div    class="b-toggle" ref="btoggle" :style="style (1)"
	  data-tip="**CLICK** Toggle Value" >
    <div  class="b-toggle-label" ref="button" @pointerdown="pointerdown" @dblclick="dblclick" >
      {{ label }}
    </div>
  </div>
</template>

<script>
export default {
  name: 'b-toggle',
  props: { value: { default: false },
	   label: { default: "Toggle" }, },
  data: () => ({
    value_: 0,
    buttondown_: false,
  }),
  beforeDestroy () {
    window.removeEventListener ('pointerup', this.pointerup);
  },
  methods: {
    style (div = 0) {
      return ""; // FIXME
    },
    dom_update() {
      if (!this.$el) // we need a second Vue.render() call for DOM alterations
	return this.$forceUpdate();
      this.value_ = this.value;
      if (this.value_)
	{
	  this.$el.classList.add ('b-toggle-on');
	  this.$el.classList.remove ('b-toggle-off');
	}
      else
	{
	  this.$el.classList.remove ('b-toggle-on');
	  this.$el.classList.add ('b-toggle-off');
	}
    },
    emit_value (v) {
      this.$emit ('input', v);
    },
    dblclick (ev) {
      // prevent double-clicks from propagating, since we always
      // handled it as single click already
      ev.preventDefault();
      ev.stopPropagation();
    },
    pointerdown (ev) {
      // trigger only on primary button press
      if (!this.buttondown_ && ev.buttons == 1)
	{
	  this.buttondown_ = true;
	  this.$refs.button.classList.add ('b-toggle-press');
	  window.addEventListener ('pointerup', this.pointerup);
	  ev.preventDefault();
	  ev.stopPropagation();
	}
    },
    pointerup (ev) {
      window.removeEventListener ('pointerup', this.pointerup);
      this.$refs.button.classList.remove ('b-toggle-press');
      if (this.buttondown_)
	{
	  this.buttondown_ = false;
	  ev.preventDefault();
	  ev.stopPropagation();
	  if (this.$refs.button.matches (':hover'))
	    this.emit_value (!this.value_);
	}
    },
  },
};

</script>
