<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PRO-INPUT
  A property input element, usually for numbers, toggles or menus.
  ## Properties:
  *value*
  : Contains the input being edited.
  *readonly*
  : Make this component non editable for the user.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-pro-input        {
    display: flex; justify-content: flex-end;
    .b-pro-input-ldiv[class]:before { content: "\200b"; /* zero width character to force line height */ }
    .b-pro-input-knob { height: 2em; }
    .b-pro-input-toggle { height: 2em; }
    .b-pro-input-choice { height: 2em; }
    .b-pro-input-span {
      pointer-events: none; user-select: none;
      max-width: 100%;
      white-space: nowrap;
      text-align: center;
      margin-right: -999em; /* avoid widening parent */
      overflow: hidden;
      z-index: 9;
    }
    &.b-pro-input:hover .b-pro-input-span { overflow: visible; }
  }
</style>

<template>
  <b-vflex class="b-pro-input tabular-nums" :data-bubble="bubble()" >
    <b-knob class="b-pro-input-knob" v-if="type() == 'knob'" :hscroll="false"
	    :value="get_num()" :label="vtext" :bidir="is_bidir()" @input="set_num ($event)" />
    <b-toggle class="b-pro-input-toggle" v-if="type() == 'toggle'" label=""
	      :value="get_num()" @input="set_num ($event)" />
    <b-choice class="b-pro-input-choice" v-if="type() == 'choice'" :choices="choices"
	      :value="get_index()" @input="set_index ($event)" />
    <span   class="b-pro-input-span" v-if="labeled && !!nick">{{ nick }}</span>
  </b-vflex>
</template>

<script>
function pro_input_data () {
  const data = {
    // ident:   { default: '', getter: async c => await this.prop.identifier(), },
    // group:   { default: '', getter: async c => await this.prop.group(), },
    is_numeric: { default:  1, getter: async c => await this.prop.is_numeric(), },
    label:      { default: '', getter: async c => await this.prop.label(), },
    choices:    { default: [], getter: async c => Object.freeze (await this.prop.choices()), },
    nick:       { default: '', getter: async c => await this.prop.nick(), },
    unit:       { default: '', getter: async c => await this.prop.unit(), },
    hints:      { default: '', getter: async c => await this.prop.hints(), },
    blurb:	{ default: '', getter: async c => await this.prop.blurb(), },
    vmin:       { getter: async c => await this.prop.get_min(), },
    vmax:       { getter: async c => await this.prop.get_max(), },
    vstep:      { getter: async c => await this.prop.get_step(), },
    vnum:       { getter: async c => await this.prop.get_num(),
		  notify: n => this.n1=n /*this.prop.on ("change", n)*/, },
    vtext:      { getter: async c => await this.prop.get_text(),
		  notify: n => this.n2=n /*this.prop.on ("change", n)*/, },
  };
  return this.observable_from_getters (data, () => this.prop);
}

export default {
  name: 'b-pro-input',
  props: {
    prop:       { default: null, },
    labeled:	{ type: Boolean, default: true, },
    readonly:	{ type: Boolean, default: false, },
  },
  data() { return pro_input_data.call (this); },
  methods: {
    bubble() {
      let b = '';
      if (this.label && this.blurb)
	b += '###### '; // label to title
      if (this.label)
	b += this.label;
      if (this.unit)
	b += '  (**' + this.unit + '**)';
      if (this.label || this.unit)
	b += '\n';
      if (this.blurb)
	b += this.blurb;
      App.zmove(); // force changes to be picked up
      return b;
    },
    type () {
      if (this.is_numeric)
	{
	  const hints = ':' + this.hints + ':';
	  if (hints.search (/:toggle:/) >= 0)
	    return 'toggle';
	  if (hints.search (/:choice:/) >= 0)
	    return 'choice';
	  return 'knob';
	}
      return '';
    },
    set_num (v) {
      if (this.readonly)
	return;
      this.update_hints();
      if (this.vmin !== undefined && this.vmax !== undefined)
	{
	  if (this.bidir_)
	    v = (v + 1) * 0.5;
	  const next = this.scale (v);
	  this.prop.set_num (next);
	}
      setTimeout (this.n1, 15); // FIXME : need real notification
      setTimeout (this.n2, 15); // FIXME : need real notification
    },
    get_num() {
      if (this.vnum === undefined || this.vmin === undefined || this.vmax === undefined)
	return 0;
      this.update_hints();
      let v = this.iscale (this.vnum);
      if (this.bidir_)
	v = 2 * v - 1;
      return v;
    },
    update_hints()
    {
      if (this.hints == this.hints_ &&
	  this.vmin == this.vmin_ &&
	  this.vmax == this.vmax_)
	return;
      this.hints_ = this.hints;
      this.vmin_ = this.vmin;
      this.vmax_ = this.vmax;
      let m = this.hints_.match (/:logcenter=([0-9.]+):/);
      if (m)
	{
	  const center = m[1];
	  // Determine exponent, so that:
	  //   begin + pow (0.0, exponent) * (end - begin) == begin  ← exponent is irrelevant here
	  //   begin + pow (0.5, exponent) * (end - begin) == center
	  //   begin + pow (1.0, exponent) * (end - begin) == end    ← exponent is irrelevant here
	  // I.e. desired: log_0.5 ((center - begin) / (end - begin))
	  this.exp_ = Math.log2 ((this.vmax_ - this.vmin_) / (center - this.vmin_));
	  /* Example in gnuplot:
	   * begin=32.7; end=8372; center=523; e=log((end-begin) / (center-begin)) / log(2)
	   * print e; set logscale y; plot [0:1] begin + x**e * (end-begin), center
	   */
	}
      else
	this.exp_ = undefined;
    },
    scale (v)
    {
      // apply logscale if set
      if (this.exp_)
	{
	  // Implements logarithmic mapping (or exponential within [0…1]) as requested by
	  // @swesterfeld in: https://github.com/tim-janik/beast/issues/5#issuecomment-303974829
	  // The slope is determined by giving `scenter` at the midpoint within `[nmin … nmax]`.
	  v = this.vmin + Math.pow (v, this.exp_) * (this.vmax - this.vmin);
	}
      else
	v = this.vmin + v * (this.vmax - this.vmin);
      return v;
    },
    iscale (v)
    {
      // invert logscale if set
      if (this.exp_)
	{
	  // @swesterfeld in: https://github.com/tim-janik/beast/issues/5#issuecomment-303974829
	  // > Using f(x)=x^slope is not only always monotonically increasing and adjustable,
	  // > it is also trivial to invert (x^(1.0/slope)).
	  v = Math.pow ((v - this.vmin) / (this.vmax - this.vmin), 1 / this.exp_);
	}
      else
	v = (v - this.vmin) / (this.vmax - this.vmin);
      return v;
    },
    set_index (v) {
      if (this.vmin !== undefined && this.vstep !== undefined)
	{
	  const next = this.vmin + v * this.vstep;
	  this.prop.set_num (next);
	}
      setTimeout (this.n1, 15); // FIXME : need real notification
      setTimeout (this.n2, 15); // FIXME : need real notification
    },
    get_index() {
      if (this.vnum === undefined || this.vmin === undefined || this.vstep === undefined)
	return 0;
      return (this.vnum - this.vmin) / this.vstep;
    },
    is_bidir() {
      this.bidir_ = this.hints.search (/:bidir:/) >= 0;
      return this.bidir_;
    },
  },
};
</script>
