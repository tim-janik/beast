<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-FED-SWITCH
  A field-editor switch to change between on and off.
  ## Properties:
  *value*
  : Contains a boolean indicating whether the switch is on or off.
  *readonly*
  : Make this component non editable for the user.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-fed-switch        { position: relative; display: inline-block; width: 2.6em; height: 1.4em;
    input               { opacity: 0; width: 0; height: 0;
      &:focus   + .b-fed-switch-trough		{ box-shadow: $b-focus-box-shadow; }
      &:checked + .b-fed-switch-trough         { background-color: $b-switch-active; /*cursor: ew-resize;*/ }
      &:checked + .b-fed-switch-trough:before	{ opacity: 1; /* checkmark */ }
      &:checked + .b-fed-switch-trough .b-fed-switch-knob { transform: translateX(1.2em); }
    }
  }
  .b-fed-switch-knob	{ position: absolute; height: 1em; width: 1em; left: 0.2em; bottom: 0.2em;
    content: ""; transition: .3s; background-color: $b-switch-knob; border-radius: $b-button-radius;
  }
  .b-fed-switch-trough { position: absolute; top: 0; left: 0; right: 0; bottom: 0;
    transition: .3s; background-color: $b-switch-inactive; border-radius: $b-button-radius;
    &:before		{ position: absolute; top: 0.1em; left: 0.3em;
      font-size: 1em; text-transform: none; text-decoration: none !important; speak: none;
      content: "\2713"; transition: .3s; color: $b-switch-knob; opacity: 0;
    }
  }
</style>

<template>
  <label class="b-fed-switch">
    <input ref="checkboxtype" type="checkbox" :disabled="readonly"
	   :checked="value" @change="emit_input_value ($event.target.checked)" >
    <span class="b-fed-switch-trough"><span class="b-fed-switch-knob"></span></span>
  </label>
</template>

<script>
export default {
  name: 'b-fed-switch',
  props: {
    value:	{ type: [ String, Boolean ], },
    readonly:	{ type: Boolean, default: false, },
  },
  watch: {
    value: function (val, old) {		// enforce constrain() on *outside* changes
      const constrainedvalue = this.constrain (val);
      if (this.value != constrainedvalue)
	this.emit_input_value (this.value);	// enforce constrain()
    },
  },
  methods: {
    emit_input_value (inputvalue) {		// emit 'input' with constrained value
      const constrainedvalue = this.constrain (inputvalue);
      const expected = String (constrainedvalue);
      if (this.$refs.checkboxtype && String (this.$refs.checkboxtype.checked) != expected)
	this.$refs.checkboxtype.checked = expected;
      if (String (this.value) != expected)
	this.$emit ('input', constrainedvalue);
    },
    constrain (v) {
      if (typeof (v) != "string")
	return !!v;
      if (v.length < 1)
	return false;
      if (v[0] == 'f' || v[0] == 'F' || v[0] == 'n' || v[0] == 'N')
	return false;
      return true;
    },
  },
};
</script>
