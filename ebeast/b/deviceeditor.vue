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
</style>

<template>
  <b-hflex class="b-deviceeditor" style="width: 100%; height: 100%" @contextmenu.stop="menuopen" >
    <slot />
    <b-vflex v-for="module in modules" :key="module.$id"
	     class="b-deviceeditor-entry" center style="margin: 5px" >
      <span > Module {{ module.$id }} </span>
    </b-vflex>
    <b-contextmenu ref="cmenu" @click="menuactivation" >
      <b-menutitle> Module </b-menutitle>
      <b-menuitem fa="plus-circle"      role="add-module" >      Add Module		</b-menuitem>
      <b-menuitem fa="times-circle"     role="delete-module" >   Delete Module		</b-menuitem>
    </b-contextmenu>
  </b-hflex>
</template>

<script>
function observable_device_data () {
  const data = {
    modules:	{ default: [],  	notify: n => this.device.on ("notify:modules", n),
		  getter: async c => Object.freeze (await this.device.list_modules()), },
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
    menuactivation (role) {
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      if (role == 'add-module')
	this.device.create_module ('Dummy');
    },
    menucheck (role, component) {
      console.log ("deviceeditor.vue", this.device);
      if (!this.device)
	return false;
      switch (role)
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
