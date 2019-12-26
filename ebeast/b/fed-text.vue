<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-FED-TEXT
  A field-editor for text input.
  ## Properties:
  *value*
  : Contains the text string being edited.
  *readonly*
  : Make this component non editable for the user.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-fed-text		{
    input		{
      outline-width: 0; border: none; border-radius: $b-button-radius;
      text-align: left; background-color: rgba(255,255,255,.3); color: #fff;
      padding-left: $b-button-radius; padding-right: $b-button-radius;
      @include b-style-inset;
      &:focus		{ box-shadow: $b-focus-box-shadow; }
    }
  }
</style>

<template>
  <label class="b-fed-text">
    <input ref="texttype" type="text" :readonly="readonly" :style="textstyle()" :placeholder="placeholder"
	   :value="value" @input="emit_input_value ($event.target.value)" >
  </label>
</template>

<script>
export default {
  name: 'b-fed-text',
  props: {
    value:	{ type: String, },
    readonly:	{ type: Boolean, default: false, },
  },
  data_tmpl: {
    placeholder: '',
  },
  methods: {
    emit_input_value (inputvalue) {		// emit 'input' with constrained value
      const constrainedvalue = this.constrain (inputvalue);
      if (this.$refs.texttype && this.$refs.texttype.value != constrainedvalue)
	this.$refs.texttype.value = constrainedvalue;
      if (this.value != constrainedvalue)
	this.$emit ('input', constrainedvalue);
    },
    constrain (v) {
      return String (v);
    },
    textstyle() {
      // determine width for input types
      const width = 1 + 15; // padding + characters
      return `width: ${width}em;`;
    },
  },
};
</script>
