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
      border: $b-panel-border; /*DEBUG: border-color: #333;*/
      border-radius: $b-device-radius; border-top-left-radius: 0; border-bottom-left-radius: 0;
      justify-content: flex-start;
    }
  }
  /* As of 2020, 'flex-flow: column wrap;' is still broken in FF and Chrome:
   * https://stackoverflow.com/questions/33891709/when-flexbox-items-wrap-in-column-mode-container-does-not-grow-its-width/33899301#33899301
   * https://stackoverflow.com/questions/23408539/how-can-i-make-a-displayflex-container-expand-horizontally-with-its-wrapped-con/41209186#41209186
   */
  .b-deviceeditor-gwrap-ascflex {
    /* Buggy in 2020: the flex container doesn't extend its with beyond the first wrapped child */
    display: flex; flex-flow: column wrap;
  }
  .b-deviceeditor-areas,
  .b-deviceeditor-gwrap-asvflex {
    /* Hacky workaround, use flex row but vertical writing mode; 2020: has reflow bugs in FF and Chrome */
    display: inline-flex; flex-flow: row wrap; writing-mode: vertical-lr;
    align-content: flex-start;
    & > * { writing-mode: horizontal-tb; } //* restore writing mode */
  }
  .b-deviceeditor-gwrap-asgrid {
    display: grid;
    grid-auto-flow: column;
    grid-template-rows: repeat(2, auto);
  }
</style>

<template>
  <b-hflex class="b-deviceeditor" @contextmenu.stop="menuopen" >
    <span class="b-deviceeditor-sw" > {{ device_info.uri + ' #' + device.$id }} </span>
    <div class="b-deviceeditor-areas" >
      <b-pro-group v-for="group in gprops" :key="group.name" :name="group.name" :props="group.props" />
    </div>
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
  let nrows = 1;
  if (props.length > 8)
    nrows = 2;
  if (props.length > 16)
    nrows = 3;
  if (props.length > 24)
    nrows = 4;
  const run = Math.ceil (props.length / nrows);
  for (let i = 0; i < props.length; i++)
    {
      const p = props[i];
      p.lrow_ = Math.trunc (i / run);
    }
  return nrows;
}

function prop_visible (prop) {
  const hints = ':' + prop.hints_ + ':';
  if (hints.search (/:G:/) < 0)
    return false;
  return true;
}

async function property_groups (props) {
  props = await cache_properties (props);
  // split properties into groups
  const grouplists = {}, groupnames = [];
  for (const p of props)
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
  // return list of groups
  const ret = [];
  for (const groupname of groupnames)
    {
      const gprops = grouplists[groupname];
      const nrows = assign_layout_rows (gprops);
      ret.push ({ name: groupname, props: gprops, nrows: nrows });
    }
  return Object.freeze (ret); // list of groups: [ { name, props: [ Prop... ] }... ]
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
