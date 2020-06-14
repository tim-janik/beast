<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-FED-OBJECT
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
  *debounce*
  : Delay in milliseconds for `input` event notifications.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-fed-object		{
    .b-fed-object-clear	{
      font-size: 1.1em; font-weight: bolder;
      color: #888; background: none; padding: 0 0.1em 0;
      outline: none; border: 1px solid rgba(0,0,0,0); border-radius: $b-button-radius;
      &:hover			{ color: #eb4; }
      &:active			{ color: #3bf; }

    } }
  .b-fed-object > * { //* avoid visible overflow for worst-case resizing */
    min-width: 0;
    overflow-wrap: break-word;
  }
</style>

<!-- field = [ ident, iscomponent, label, attrs, o, handler ] -->
<template>
  <b-grid class="b-fed-object" style="grid-gap: 0.6em 0.5em;">
    <template v-for="group in list_fields()">

      <b-hflex style="grid-column: 1 / span 3; align-items: center; margin-top: 1em" :key="'group:' + group[0]">
	<span style="flex-grow: 0; font-weight: bold">{{ group[0] }}</span>
	<hr style="flex-grow: 1; margin-left: 0.5em; min-width: 5em" />
      </b-hflex>

      <template v-for="f in group[1]" >
	<span style="grid-column: 1; padding: 0 1em 0; text-align: left" :data-bubble="f.blurb" :key="'f1:' + f.ident"
	>{{ f.label }}</span>
	<span style="text-align: right" :data-bubble="f.blurb" :key="'f2:' + f.ident">
	  <component :is="f.ctype" v-bind="f.attrs" :class="'b-fed-object--' + f.ident"
		     :value="f.odata[f.ident]" @input="f.apply" ></component></span>
	<span :key="'f3:' + f.ident">
	  <button class="b-fed-object-clear" tabindex="-1" @click="clear_field (f.ident)" > ⊗  </button></span>
      </template>

    </template>
  </b-grid>
</template>

<script>

async function editable_object () {
  // provide an editable and reactive clone of this.value
  let o = { __typedata__: undefined, __fieldhooks__: undefined, }, td;
  for (const p in this.value)
    if (p[0] != '_' && typeof (this.value[p]) != "function")
      o[p] = this.value[p];
  // determine __typedata__
  if (Array.isArray (this.value.__typedata__))
    td = Util.map_from_kvpairs (this.value.__typedata__);
  else if (typeof (this.value.__typedata__) == "function")
    td = Util.map_from_kvpairs (this.value.__typedata__());
  else if (typeof (this.value.__typedata__) == "object")
    {
      td = {};
      Util.assign_forin (td, this.value.__typedata__);
    }
  else
    {
      const fields = [];
      for (let p in o)
	fields.push (p);
      td = { fields: fields.join (';'), };
    }
  o.__typedata__ = td;
  // transport __fieldhooks__
  o.__fieldhooks__ = Object.freeze (Object.assign ({}, this.value.__fieldhooks__ || {}));
  return o;
}

function component_data () {
  const data = {
    object: { getter: c => editable_object.call (this, c), },
  };
  return this.observable_from_getters (data, () => this.value);
}

export default {
  name: 'b-fed-object',
  mixins: [ Util.vue_mixins.hyphen_props ],
  data() { return component_data.call (this); },
  props: {
    readonly:	{ default: false, },
    debounce:	{ default: 0, },
    value:	{ required: true, },
    default:	{},
  },
  methods: {
    list_fields() {
      if (!this.object)
	return [];
      const o = this.object, td = o.__typedata__, fieldhooks = o.__fieldhooks__ || {};
      const field_typedata = unfold_properties (td); // { foo: { label: 'Foo' }, bar: { step: '5' }, etc }
      const fields = td.fields ? td.fields.split (';') : [];
      const groupmap = {};
      const grouplist = [];
      for (let fieldname of fields)
	{
	  const field_data = field_typedata[fieldname];
	  const attrs = {};
	  for (let p of ['min', 'max', 'step'])
	    if (field_data[p] != undefined)
	      attrs[p] = field_data[p];
	  if (this.readonly || (':' + field_data.hints + ':').indexOf ('ro') >= 0)
	    attrs.readonly = true;
	  const handler = (v) => this.apply_field (fieldname, v);
	  let label = td[fieldname + '.label'] || fieldname;
	  let ct = '';			// component type
	  const ft = typeof (o[fieldname]); // FIXME: use td
	  if (ft == "number")
	    {
	      ct = 'b-fed-number';
	      if (o[fieldname] != 0 | o[fieldname]) // not int // FIXME: use td
		attrs.allowfloat = true;
	      // min max
	    }
	  else if (ft == "boolean")
	    ct = 'b-fed-switch';
	  else if (ft == "string")
	    {
	      const picklistitems = fieldhooks[fieldname + '.picklistitems'];
	      if (picklistitems)
		{
		  attrs.picklistitems = picklistitems;
		  attrs.title = label + " " + _("Selection");
		  ct = 'b-fed-picklist';
		}
	      else
		ct = 'b-fed-text';
	    }
	  if (ct)
	    {
	      const group = field_data.group || "Settings";
	      let groupfields;
	      if (groupmap[group] == undefined)
		{
		  groupfields = [];
		  const newgroup = [ group, groupfields ];
		  groupmap[group] = newgroup;
		  grouplist.push (newgroup);
		}
	      else
		groupfields = groupmap[group][1];
	      const blurb = td[fieldname + '.blurb'] || undefined;
	      groupfields.push ({ ident: fieldname,
				  ctype: ct,
				  label: label,
				  attrs: attrs,
				  odata: o,
				  blurb: blurb,
				  apply: handler });
	    }
	}
      return grouplist; // [ [ 'Group', [ field1, field2 ], [ 'Other', [ field3, field4 ] ] ]
    },
    clear_field (fieldname) {
      const o = this.object;
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
      const o = this.object;
      Vue.set (o, fieldname, value);
      if (this.emit_update_ == undefined)
	this.emit_update_ = Util.debounce (() => this.$emit ('input', this.object),
					   { wait: this.debounce,
					     restart: true,
					   });
      this.emit_update_();
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
