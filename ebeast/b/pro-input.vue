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
    .b-pro-input-knob { height: 4.5em; }
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
    <b-knob class="b-pro-input-knob" v-if="type=='knob'" :value="get_num()" @input="set_num ($event)" />
    <span   class="b-pro-input-span" v-if="!!nick">{{ nick }}</span>
  </b-vflex>
</template>

<script>
function pro_input_data () {
  const data = {
    // ident:   { default: '', getter: async c => await this.prop.identifier(), },
    // group:   { default: '', getter: async c => await this.prop.group(), },
    is_numeric: { default:  0, getter: async c => await this.prop.is_numeric(), },
    label:      { default: '', getter: async c => await this.prop.label(), },
    nick:       { default: '', getter: async c => await this.prop.nick(), },
    unit:       { default: '', getter: async c => await this.prop.unit(), },
    hints:      { default: '', getter: async c => await this.prop.hints(), },
    blurb:	{ default: '', getter: async c => await this.prop.blurb(), },
    vmin:       { getter: async c => await this.prop.get_min(), },
    vmax:       { getter: async c => await this.prop.get_max(), },
    vstep:      { getter: async c => await this.prop.get_step(), },
    vnum:       { getter: async c => await this.prop.get_num(),
		  notify: n => this.n=n /*this.prop.on ("change", n)*/, },
  };
  return this.observable_from_getters (data, () => this.prop);
}

export default {
  name: 'b-pro-input',
  props: {
    prop:       { default: null, },
    readonly:	{ type: Boolean, default: false, },
  },
  data() { return pro_input_data.call (this); },
  computed: {
    type () {
      return this.is_numeric ? 'knob' : '';
    },
  },
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
      return b;
    },
    set_num (v) {
      if (this.readonly)
	return;
      if (this.vmin !== undefined && this.vmax !== undefined)
	{
	  let next = this.vmin + v * (this.vmax - this.vmin);
	  this.prop.set_num (next);
	}
      this.n (); // FIXME : need real notification
    },
    get_num() {
      if (this.vnum === undefined) return 0;
      let v;
      if (this.vstep)
	v = this.vstep * Math.round (this.vnum / this.vstep);
      else
	v = this.vnum;
      const c = v;
      if (this.vmin !== undefined && this.vmax !== undefined)
	v = (Util.clamp (v, this.vmin, this.vmax) - this.vmin) / (this.vmax - this.vmin);
      return v;
    },
  },
};
</script>
