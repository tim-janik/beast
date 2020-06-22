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
    border: 5px solid #575;
  }
  .b-deviceeditor-sw {
    text-align: center;
    /* FF: writing-mode: sideways-rl; */
    writing-mode: vertical-rl; transform: rotate(180deg);
  }
</style>

<template>
  <b-hflex class="b-deviceeditor" style="width: 100%; height: 100%" @contextmenu.stop="menuopen" >
    <span class="b-deviceeditor-sw" > {{ device_info.uri + ' #' + device.$id }} </span>
    <b-vflex v-for="module in modules" :key="module.$id"
	     class="b-deviceeditor-entry" center style="margin: 5px" >
      <span > Module {{ module.$id }} </span>
    </b-vflex>
    <b-vflex style="flex-wrap: wrap;">
      <b-hflex style="flex-wrap: wrap;" v-for="group in gprops" :key="group.name" >
	<b-pro-input v-for="prop in group.props" :key="prop.$id" :prop="prop" />
      </b-hflex>
    </b-vflex>
    <b-contextmenu ref="cmenu" @click="menuactivation" >
      <b-menutitle> Module </b-menutitle>
      <b-menuitem fa="plus-circle"      uri="add-module" >      Add Module		</b-menuitem>
      <b-menuitem fa="times-circle"     uri="delete-module" >   Delete Module		</b-menuitem>
    </b-contextmenu>
  </b-hflex>
</template>

<script>

async function property_groups (props) {
  props = await props;
  // fetch group of all properties
  const promises = [];
  for (const r of props)
    promises.push (r.group()); // accessing group() yields promise
  const groups = await Promise.all (promises);
  console.assert (groups.length == props.length);
  // split properties into groups
  const grouplists = {}, groupnames = [];
  for (let i = 0; i < props.length; i++)
    {
      const groupname = groups[i];
      if (!grouplists[groupname])
	{
	  groupnames.push (groupname);
	  grouplists[groupname] = [];
	}
      grouplists[groupname].push (props[i]);
    }
  // return list of groups
  const ret = [];
  for (const groupname of groupnames)
    ret.push ({ name: groupname, props: grouplists[groupname] });
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
