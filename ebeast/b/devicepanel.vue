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

  $scrollbar-height: 6px; //* Should match Firefox 'scrollbar-width:thin' */

  .b-devicepanel.b-hflex {
    height: 100%; width: 0; flex-grow: 1;
    background: $b-devicepanel-bg;
    border-radius: inherit;
    justify-content: flex-start;
    align-items: center;

    .b-devicepanel-before-scroller {
      text-align: center;
      //* FF: writing-mode: sideways-rl; */
      writing-mode: vertical-rl; transform: rotate(180deg);
      border-right: 7px solid #9c61ff;
      padding: 0 5px;
      border-top-right-radius: inherit;
      border-bottom-right-radius: inherit;
      height: 100%;
      //* Add slight shadow to the right for a soft scroll boundary */
      box-shadow: -2px 0 $b-scroll-shadow-blur 0px #000;
      background: #000000ef;
      z-index: 9; //* raise above scrolled siblings */
    }
    .b-devicepanel-after-scroller {
      height: 100%;
      //* Add slight shadow to the left for a soft scroll boundary */
      box-shadow: 0 0 5px 2px #000;
      width: 1px; background: #000000ef;
      z-index: 9; //* raise above scrolled siblings */
    }
    .b-devicepanel-scroller {
      height: 100%; flex-grow: 1;
      padding-top: $scrollbar-height;
      padding-bottom: 0;
      overflow-y: hidden;
      overflow-x: scroll;
      & > * {
	flex-grow: 0;
      }
      .b-more {
	margin-top: $scrollbar-height;
      }
    }
  }

  // scrollbar styling
  .b-devicepanel-scroller {
    $scrollbar-bg: #111;
    $scrollbar-dd: #222;
    $scrollbar-fg: #777;
    // Chrome
    &::-webkit-scrollbar	{ height: $scrollbar-height; }
    &::-webkit-scrollbar-button { display: none; }
    &::-webkit-scrollbar-track, &::-webkit-scrollbar-track-piece,
    &::-webkit-scrollbar	{
      background: $scrollbar-bg;
      border: none; box-shadow: none;
    }
    &::-webkit-scrollbar-thumb	{
      border-radius: 999px;
      transition: background 0.4s ease;
      background: $scrollbar-dd;
    }
    &:hover::-webkit-scrollbar-thumb {
      transition: background 0.4s ease;
      background: linear-gradient(lighter($scrollbar-fg), darker($scrollbar-fg));
    }
    // Firefox
    html[gecko] & {
      scrollbar-width: thin;
      transition: scrollbar-color 0.4s ease;
      scrollbar-color: $scrollbar-dd $scrollbar-bg;
      &:hover {
	scrollbar-color: $scrollbar-fg $scrollbar-bg;
      }
    }
  }
</style>

<template>
  <b-hflex class="b-devicepanel" >
    <span class="b-devicepanel-before-scroller"> Device Panel </span>
    <b-hflex class="b-devicepanel-scroller" >
      <template v-for="device in devices" >
	<b-more @click.native.stop="menuopen" :sibling="device" :key="device.$id"
		data-tip="**CLICK** Add New Elements" />
	<b-deviceeditor :device="device" center :key="'deviceeditor' + device.$id" />
      </template>
      <b-more @click.native.stop="menuopen" :sibling="null"
	      data-tip="**CLICK** Add New Elements" />
      <b-contextmenu ref="cmenu" @click="menuactivation" yscale="1.6" >
	<b-menutitle> Device </b-menutitle>
	<b-menuitem fa="plus-circle"      uri="EBeast:add-device" >      Add Device		</b-menuitem>
	<b-menuitem fa="times-circle"     uri="EBeast:delete-device" >   Delete Device		</b-menuitem>
	<b-treeselector :tree="devicetypes"> </b-treeselector>
      </b-contextmenu>
    </b-hflex>
    <span class="b-devicepanel-after-scroller"></span>
  </b-hflex>
</template>

<script>
async function list_audio_device_types () {
  if (list_audio_device_types.return_entries)
    return list_audio_device_types.return_entries;
  const crawler = await Bse.server.resource_crawler();
  const entries = await crawler.list_devices (Bse.ResourceType.AUDIO_DEVICE);
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
  list_audio_device_types.return_entries = Object.freeze ({ entries: list });
  return list_audio_device_types.return_entries;
}
list_audio_device_types();

function observable_device_data () {
  const data = {
    devices:	  { default: [],	 notify: n => this.devcon_.on ("notify:devices", n),
		    getter: async c => Object.freeze (await this.devcon_.list_devices()), },
    devicetypes:  { getter: c => list_audio_device_types(), },
  };
  const have_devcon = async () => {
    if (this.last_track_ != this.track)
      {
	this.devcon_ = this.track ? await this.track.device_container() : null;
	this.last_track_ = this.track;
      }
    return this.devcon_;
  };
  return this.observable_from_getters (data, have_devcon);
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
      if (uri == 'EBeast:add-device' || uri == 'EBeast:delete-device')
	debug ("devicepanel.vue:", uri);
      if (this.devcon_ && !uri.startsWith ('EBeast:')) // assuming b-treeselector.devicetypes
	{
	  if (popup_options.device_sibling)
	    this.devcon_.create_device_before (uri, popup_options.device_sibling);
	  else
	    this.devcon_.create_device (uri);
	}
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
      const sibling = event.target?.__vue__?.$attrs?.sibling;
      this.$refs.cmenu.popup (event, { check: this.menucheck.bind (this),
				       device_sibling: sibling });
    },
  },
};
</script>
