<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-DEVICEPANEL
  Panel for editing of devices.
  ## Props:
  *track*
  : Container for the devices.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-devicepanel.b-hflex {
    border: 3px solid #777;
    border-radius: $b-theme-border-radius;
    justify-content: flex-start;
    align-items: center;
  }
</style>

<template>
  <b-hflex class="b-devicepanel" style="width: 100%; height: 100%" >
    <b-more @click.native.stop="menuopen" :sibling="null" />
    <template v-for="device in devices" >
      <b-deviceeditor :device="device" center style="margin: 5px" :key="'deviceeditor' + device.$id" />
      <b-more @click.native.stop="menuopen" :sibling="device" :key="device.$id" />
    </template>
    <b-contextmenu ref="cmenu" @click="menuactivation" yscale="1.6" >
      <b-menutitle> Device </b-menutitle>
      <b-menuitem fa="plus-circle"      uri="add-device" >      Add Device		</b-menuitem>
      <b-menuitem fa="times-circle"     uri="delete-device" >   Delete Device		</b-menuitem>
      <b-treeselector :tree="devicetypes"> </b-treeselector>
    </b-contextmenu>
  </b-hflex>
</template>

<script>
async function list_audio_device_types () {
  if (list_audio_device_types.return_entries)
    return list_audio_device_types.return_entries;
  const crawler = await Bse.server.resource_crawler();
  const entries = await crawler.list_devices ('audio-device');
  const cats = {};
  for (const e of entries)
    {
      const category = e.category || 'Other';
      cats[category] = cats[category] || { label: category, type: 'bse-resource-type-folder', entries: [] };
      cats[category].entries.push (e);
    }
  const list = [];
  for (const c of Object.keys (cats).sort())
    list.push (cats[c]);
  list_audio_device_types.return_entries = { entries: list };
  return list_audio_device_types.return_entries;
}
list_audio_device_types();

function observable_device_data () {
  const data = {
    devices:	{ default: [],  	notify: n => this.track.on ("notify:devices", n),
		  getter: async c => Object.freeze (await this.track.list_devices()), },
    devicetypes: { getter: async c => Object.freeze (await list_audio_device_types()), },
  };
  return this.observable_from_getters (data, () => this.track);
}

export default {
  name: 'b-devicepanel',
  props: {
    track: { type: Bse.Track, },
  },
  data() { return observable_device_data.call (this); },
  methods: {
    menuactivation (uri) {
      const popup_options = this.$refs.cmenu.popup_options;
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      if (uri == 'add-device')
	console.log ("create_device: after:", popup_options.device_sibling);
      if (uri == 'add-device')
	this.track.create_device ('Dummy');
    },
    menucheck (uri, component) {
      if (!this.track)
	return false;
      switch (uri)
      {
	case 'add-device':   return true;
      }
      return false;
    },
    menuopen (event) {
      const sibling = event?.path[0]?.__vue__?.$attrs?.sibling;
      this.$refs.cmenu.popup (event, { check: this.menucheck.bind (this),
				       device_sibling: sibling });
    },
  },
};
</script>
