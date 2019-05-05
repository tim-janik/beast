<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # VC-FED-OBJECT
  A field-editor for object input.
  A copy of the input value is edited, update notifications are provided via
  an `input` event.
  ## Properties:
  *value*
  : Object with properties to be edited.
  *default*
  : Object with default property values.
  *readonly*
  : Make this component non editable for the user.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .vc-fed-object		{
    .vc-fed-object-clear	{
      font-size: 1.1em; font-weight: bolder;
      color: #888; background: none; padding: 0 0.1em 0;
      outline: none; border: 1px solid rgba(0,0,0,0); border-radius: $vc-button-radius;
      &:hover			{ color: #eb4; }
      &:active			{ color: #3bf; }

    } }
  table.vc-fed-object		{ table-layout: fixed; max-width: 100%;
    & > tr			{
      & > td			{ overflow-wrap: break-word; }
      & > td:first-child	{ max-width: 40%; }
    }
  }
</style>

<!-- field = [ ident, iscomponent, label, attrs, o, handler ] -->
<template>
  <table class="vc-fed-object">
    <template v-for="group in list_fields()">
      <tr :key="'group:' + group[0]">
	<td colspan="3">
	  <div style="display: flex; flex-direction: row; align-items: center; margin-top: 1em">
	    <span style="flex-grow: 0; font-weight: bold">{{ group[0] }}</span>
	    <hr style="flex-grow: 1; margin-left: 0.5em; min-width: 5em" />
	  </div> </td> </tr>
      <tr v-for="field in group[1]" :key="'field:' + field[0]">
	<td style="padding: 0 1em 0; text-align: left" :title="field[3].blurb"
	>{{ field[2] }}</td>
	<td style="text-align: right" :title="field[3].blurb">
	  <component :is="field[1]" v-bind="field[3]"
		     :value="field[4][field[0]]" @input="field[5]"></component></td>
	<td><button class="vc-fed-object-clear" onfocus="this.blur()" @click="clear_field (field[0])"> ⊗  </button></td>
      </tr>
    </template>
  </table>
</template>

<script>
module.exports = {
  name: 'vc-fed-object',
  props: {
    readonly:	{ default: false, },
    value:	{ required: true, },
    default:	{ },
    typedata:	{ },
  },
  data_tmpl: {
    object_:	undefined,
    typedata_:	undefined,
  },
  methods: {
    editable_object() {
      if (this.object_ == undefined) {
	// edit a reactive clone of this.value
	let o = {}, td;
	for (let p in this.value)
	  if (p[0] != '_' && typeof (o[p]) != "function")
	    o[p] = this.value[p];
	this.object_ = o; // Vue makes o reactive
	// determine typedata
	if (typeof (this.value.__typedata__) == "object") {
	  td = {};
	  Util.assign_forin (td, this.value.__typedata__);
	}
	else if (typeof (this.value.__typedata__) == "function")
	  td = Util.map_from_kvpairs (this.value.__typedata__());
	else {
	  td = { fields: [] };
	  for (let p in o)
	    td.fields.push (p);
	  td.fields = td.fields.join (';');
	}
	this.typedata_ = td;
      }
      return [ this.object_, this.typedata_ ];
    },
    list_fields() {
      const [o, td] = this.editable_object();
      const field_typedata = unfold_properties (td); // { foo: { label: 'Foo' }, bar: { step: '5' }, etc }
      const fields = td.fields.split (';');
      const groupmap = {};
      const grouplist = [];
      for (let f of fields) {
	const field_data = field_typedata[f];
	const attrs = {};
	for (let p of ['min', 'max', 'step'])
	  if (field_data[p] != undefined)
	    attrs[p] = field_data[p];
	if (this.readonly || (':' + field_data.hints + ':').indexOf ('ro') >= 0)
	  attrs.readonly = true;
	const handler = (v) => this.apply_field (f, v);
	let ct = '';			// component type
	const ft = typeof (o[f]); // FIXME: use td
	if (ft == "number") {		ct = 'vc-fed-number';
	  if (o[f] != 0|o[f]) // not int // FIXME: use td
	    attrs.allowfloat = true;
	  // min max
	}
	else if (ft == "boolean")	ct = 'vc-fed-switch';
	else if (ft == "string")	ct = 'vc-fed-text';
	let label = td[f + '.label'] || f;
	if (ct)
	  {
	    const group = field_data.group || "Settings";
	    let groupfields;
	    if (groupmap[group] == undefined) {
	      groupfields = [];
	      const newgroup = [ group, groupfields ];
	      groupmap[group] = newgroup;
	      grouplist.push (newgroup);
	    } else {
	      groupfields = groupmap[group][1];
	    }
	    groupfields.push ([ f, ct, label, attrs, o, handler ]);
	  }
      }
      return grouplist; // [ [ 'Group', [ field1, field2 ], [ 'Other', [ field3, field4 ] ] ]
    },
    clear_field (fieldname) {
      const [o] = this.editable_object();
      const old = o[fieldname];
      const ft = typeof (old);
      const dflt = this['default'] != undefined ? this['default'][fieldname] : undefined;
      let cvalue = undefined;
      if (dflt != undefined)		cvalue = dflt;
      else if (ft == "number")		cvalue = 0;
      else if (ft == "boolean")		cvalue = false;
      else if (ft == "string")		cvalue = '';
      this.apply_field (fieldname, cvalue);
    },
    apply_field (fieldname, value) {
      const [o] = this.editable_object();
      Vue.set (o, fieldname, value);
      this.$emit ('input', o);
    },
  },
};

function unfold_properties (nestedpropobject) {
  // turn { a.a: 1, a.b.c: 2, d.e: 3 } into: { a: { a: 1, b.c: 2 }, d: { e: 3 } }
  let oo = {};
  window.nestedpropobject=nestedpropobject;
  for (let p in nestedpropobject) {
    const parts = p.split ('.');
    if (parts.length > 1) {
      const stem = parts[0];
      parts.shift(); // pop stem
      if (oo[stem] == undefined)
	oo[stem] = {};
      oo[stem][parts.join ('.')] = nestedpropobject[p];
    }
  }
  return oo;
}
</script>
