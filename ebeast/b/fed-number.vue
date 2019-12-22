<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-FED-NUMBER
  A field-editor for integer or floating point number ranges.
  The input `value` will be constrained to take on an amount between `min` and `max` inclusively.
  ## Properties:
  *value*
  : Contains the number being edited.
  *min*
  : The minimum amount that `value` can take on.
  *max*
  : The maximum amount that `value` can take on.
  *step*
  : A useful amount for stepwise increments.
  *allowfloat*
  : Unless this setting is `true`, numbers are constrained to integer values.
  *readonly*
  : Make this component non editable for the user.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-fed-number        {
    display: flex; justify-content: flex-end;
    input[type='number'] {
      text-align: right;
      outline-width: 0; border: none;
      @include b-style-number-input;
    }
    input[type='range'] {
      margin: auto 1em auto 0;
      @include b-style-hrange-input;
    }
  }
</style>

<template>
  <label class="b-fed-number tabular-nums">
    <input ref="slidertype" v-if="withslider" type="range" onfocus="this.blur()" tabindex="-1"
	   :min="fmin()" :max="fmax()" :step="slidersteps()" :disabled="readonly"
	   :value="value" @input="emit_input_value ($event.target.value)" >
    <input ref="numbertype" type="number" :style="numberstyle()"
	   :min="fmin()" :max="fmax()" :step="fstep()" :readonly="readonly"
	   :value="value" @input="emit_input_value ($event.target.value)" >
  </label>
</template>

<script>
export default {
  name: 'b-fed-number',
  // https://vuejs.org/v2/guide/components.html#Using-v-model-on-Components
  props: {
    value:	{ type: [ String, Number ], },
    allowfloat:	{ type: Boolean, default: false, },
    min:	{ type: [ String, Number ], default: -Number.MAX_SAFE_INTEGER, },
    max:	{ type: [ String, Number ], default: +Number.MAX_SAFE_INTEGER, },
    step:	{ type: [ String, Number ], default: 1, },
    readonly:	{ type: Boolean, default: false, },
  },
  data_tmpl: {
    withslider:	true,
  },
  watch: {
    value: function (val, old) {		// enforce constrain() on *outside* changes
      const constrainedvalue = this.constrain (val);
      if (val != constrainedvalue) {
	const expected = String (constrainedvalue);
	if (expected != String (this.value))
	  this.emit_input_value (this.value);	// enforce constrain()
      }
    },
  },
  methods: {
    emit_input_value (inputvalue) {		// emit 'input' with constrained value
      const constrainedvalue = this.constrain (inputvalue);
      const expected = String (constrainedvalue);
      if (this.$refs.numbertype && String (this.$refs.numbertype.value) != expected)
	this.$refs.numbertype.value = expected;
      if (this.$refs.slidertype && String (this.$refs.slidertype.value) != expected)
	this.$refs.slidertype.value = expected;
      if (String (this.value) != expected)
	this.$emit ('input', constrainedvalue);
    },
    constrain (v) {
      v = parseFloat (v);
      if (isNaN (v))
	v = 0;
      if (!this.allowfloat)			// use v|0 to cast to int
	v = 0 | (v > 0 ? v + 0.5 : v - 0.5);
      return Util.clamp (v, this.fmin(), this.fmax());
    },
    fmin()  { return parseFloat (this.min); },
    fmax()  { return parseFloat (this.max); },
    fstep() { return parseFloat (this.step); },
    slidersteps() {				// aproximate slider steps to slider pixels
      if (!this.allowfloat)
	return 1;				// use integer stepping for slider
      // slider float stepping, should roughly amount to the granularity the slider offers in pixels
      const sliderlength = 250;			// uneducated approximation of maximum slider length
      const delta = Math.abs (this.fmax() - this.fmin());
      if (delta > 0)
	{
	  const l10 = Math.log (10);
	  const deltalog = Math.log (delta / sliderlength) / l10;
	  const steps = Math.pow (10, Math.floor (deltalog));
	  return steps;
	}
      return 0.1;				// float fallback
    },
    numberstyle() {				// determine width for numeric inputs
      const l10 = Math.log (10);
      const minimum = 123456;			// minimum number of digits always representable
      const delta = Math.max (minimum, Math.abs (this.fmax()), Math.abs (this.fmin()),
			      Math.abs (this.fmax() - this.fmin()));
      const digits = Math.log (Math.max (delta, 618)) / l10;
      const width = Math.ceil (digits * 0.9 + 1); // margin + digits + spin-arrows
      return `width: ${width}.5em;`;		// add 0.5em margin
    },
  },
};
</script>
