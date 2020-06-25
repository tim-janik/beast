<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-DEVICEEDITOR
  Editor for editing of modules.
  ## Props:
  *device*
  : Container for the modules.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-deviceeditor {
    .b-deviceeditor-sw {
      background: $b-device-handle;
      border-radius: $b-device-radius; border-top-left-radius: 0; border-bottom-left-radius: 0;
      padding: 0 5px;
      text-align: center;
      /* FF: writing-mode: sideways-rl; */
      writing-mode: vertical-rl; transform: rotate(180deg);
    }
    .b-deviceeditor-areas {
      background: $b-device-bg;
      grid-gap: 3px;
      border: $b-panel-border; /*DEBUG: border-color: #333;*/
      border-radius: $b-device-radius; border-top-left-radius: 0; border-bottom-left-radius: 0;
      justify-content: flex-start;
    }
  }
</style>

<template>
  <b-hflex class="b-deviceeditor" @contextmenu.stop="menuopen" >
    <span class="b-deviceeditor-sw" > {{ device_info.uri + ' #' + device.$id }} </span>
    <b-grid class="b-deviceeditor-areas" >
      <b-pro-group v-for="group in gprops" :key="group.name" :style="group_style (group)"
		   :name="group.name" :props="group.props" />
    </b-grid>
    <b-vflex v-for="module in modules" :key="module.$id"
	     class="b-deviceeditor-entry" center style="margin: 5px" >
      <span > Module {{ module.$id }} </span>
    </b-vflex>
    <b-contextmenu ref="cmenu" @click="menuactivation" >
      <b-menutitle> Module </b-menutitle>
      <b-menuitem fa="plus-circle"      uri="add-module" >      Add Module		</b-menuitem>
      <b-menuitem fa="times-circle"     uri="delete-module" >   Delete Module		</b-menuitem>
    </b-contextmenu>
  </b-hflex>
</template>

<script>

async function cache_properties (propertylist) {
  propertylist = await propertylist;
  const promises = [];
  for (const p of propertylist)
    {
      promises.push (p.label());		// ①
      promises.push (p.nick()); 		// ②
      promises.push (p.unit()); 		// ③
      promises.push (p.hints()); 		// ④
      promises.push (p.group()); 		// ⑤
      promises.push (p.blurb()); 		// ⑥
      promises.push (p.description()); 		// ⑦
      promises.push (p.get_min()); 		// ⑧
      promises.push (p.get_max()); 		// ⑨
      promises.push (p.get_step()); 		// ⑩
      promises.push (p.is_numeric()); 		// ⑪
    }
  const results = await Promise.all (promises);
  console.assert (results.length == 11 * propertylist.length);
  for (let k = 0, i = 0; i < propertylist.length; i++)
    {
      const p = {
	__proto__: propertylist[i],
	label_: results[k++],			// ①
	nick_: results[k++], 			// ②
	unit_: results[k++], 			// ③
	hints_: results[k++], 			// ④
	group_: results[k++], 			// ⑤
	blurb_: results[k++], 			// ⑥
	description_: results[k++], 		// ⑦
	get_min_: results[k++], 		// ⑧
	get_max_: results[k++], 		// ⑨
	get_step_: results[k++], 		// ⑩
	is_numeric_: results[k++], 		// ⑪
      };
      propertylist[i] = p;
    }
  return propertylist;
}

function assign_layout_rows (props) {
  let n_lrows = 1;
  if (props.length > 6)
    n_lrows = 2;
  if (props.length > 12)
    n_lrows = 3;
  if (props.length > 18)
    n_lrows = 4;
  if (props.length > 24)
    ; // n_lrows = 5; is not supported, see rows_from_lrows()
  const run = Math.ceil (props.length / n_lrows);
  for (let i = 0; i < props.length; i++)
    {
      const p = props[i];
      p.lrow_ = Math.trunc (i / run);
    }
  return n_lrows;
}

function prop_visible (prop) {
  const hints = ':' + prop.hints_ + ':';
  if (hints.search (/:G:/) < 0)
    return false;
  return true;
}

async function property_groups (asyncpropertylist) {
  const propertylist = await cache_properties (asyncpropertylist);
  // split properties into group lists
  const grouplists = {}, groupnames = [];
  for (const p of propertylist)
    {
      if (!prop_visible (p))
	continue;
      const groupname = p.group_;
      if (!grouplists[groupname])
	{
	  groupnames.push (groupname);
	  grouplists[groupname] = [];
	}
      grouplists[groupname].push (p);
    }
  // create group objects
  const grouplist = [];
  for (const name of groupnames)
    {
      const props = grouplists[name];
      const n_lrows = assign_layout_rows (props);
      const group = {
	name, props, n_lrows,
	col: undefined,
	cspan: undefined,
	row: undefined,
	rspan: undefined,
      };
      grouplist.push (group);
    }
  // determine grid rows from group internal layout rows
  const rows_from_lrows = (group) => {
    /* Available vertical panel areas:
     * 1lrow   2lrows    3lrows       4lrows
     * 123456 123456789 123456789012 123456789012345
     * TT kkk TT kkkqqq TT kkkqqqkkk TT kkkqqqkkkqqq
     *
     * Supporting 5 lrows would not leave room for another panel after a 4 lrow panel,
     * so the 4 lrows and 5 lrows panels would always be stretched to same grid rows.
     */
    if (group.n_lrows == 1)
      return 2;			// title + knobs
    if (group.n_lrows == 2)
      return 3;			// title + knobs + knobs
    if (group.n_lrows == 3)
      return 4;			// title + knobs + knobs + knobs
    if (group.n_lrows == 4)
      return 5;			// title + knobs + knobs + knobs + knobs
  };
  // wrap groups into columns
  const maxrows = 5, cols = {};
  let c = 0, r = 0;
  for (let i = 0; i < grouplist.length; i++)
    {
      const group = grouplist[i];
      let rspan = rows_from_lrows (group);
      if (r > 1 && r + rspan > maxrows)
	{
	  c += 1;
	  r = 0;
	}
      group.col = c;
      group.row = r;
      group.rspan = rspan;
      r += rspan;
      if (!cols[c])
	cols[c] = [];
      cols[c].push (group);
    }
  // distribute excess column space
  for (const c in cols)  // forall columns
    {
      const cgroups = cols[c];
      let r = 0;
      for (const g of cgroups) // forall groups in column
	r += g.rspan;
      const extra = Math.trunc ((maxrows - r) / cgroups.length);
      // distribute extra space evenly
      cgroups[0].rspan += extra;
      r = cgroups[0].rspan;
      for (let i = 1; i < cgroups.length; i++)
	{
	  const prev = cgroups[i - 1];
	  cgroups[i].row = prev.row + prev.rspan;
	  cgroups[i].rspan += extra;
	  r += cgroups[i].rspan;
	}
      // close gap of last row to bottom
      if (r < maxrows)
	cgroups[cgroups.length - 1].rspan += maxrows - r;
    }
  return Object.freeze (grouplist); // list of groups: [ { name, props: [ Prop... ] }... ]
}

function observable_device_data () {
  const data = {
    modules:	{ default: [],  	notify: n => this.device.on ("notify:modules", n),
		  getter: async c => Object.freeze (await this.device.list_modules()), },
    gprops:     { default: [], getter: async c => property_groups (this.device.access_properties ("")), },
    device_info: { default: "",		notify: n => this.device.on ("notify:device_info", n),
		  getter: async c => Object.freeze (await this.device.device_info()), },
  };
  return this.observable_from_getters (data, () => this.device);
}

export default {
  name: 'b-deviceeditor',
  props: {
    device: { type: Bse.Device, },
  },
  data() { return observable_device_data.call (this); },
  methods: {
    group_style (group) {
      let s = '';
      if (group.row !== undefined)
	{
	  s += 'grid-row:' + (1 + group.row);
	  if (group.rspan)
	    s += '/span ' + group.rspan;
	  s += ';';
	}
      if (group.col !== undefined)
	{
	  s += 'grid-column:' + (1 + group.col);
	  if (group.cspan)
	    s += '/span ' + group.cspan;
	  s += ';';
	}
      return s;
    },
    menuactivation (uri) {
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      if (uri == 'add-module')
	this.device.create_module ('Dummy');
    },
    menucheck (uri, component) {
      console.log ("deviceeditor.vue", this.device);
      if (!this.device)
	return false;
      switch (uri)
      {
	case 'add-module':   return true;
      }
      return false;
    },
    menuopen (event) {
      this.$refs.cmenu.popup (event, { check: this.menucheck.bind (this) });
    },
  },
};
</script>
