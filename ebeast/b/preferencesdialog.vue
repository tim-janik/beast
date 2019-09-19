<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PREFERENCESDIALOG
  A [b-modaldialog] to edit Beast preferences.
  ## Events:
  *close*
  : A *close* event is emitted once the "Close" button activated.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-preferencesdialog	{
    .b-modaldialog-container	{ max-width: 90em; }
    .b-preferencesdialog-cmenu	{
      span.card  { color: #bbb; }
      span.line1 { font-size: 90%; color: #bbb; }
      span.line2 { font-size: 90%; color: #bbb; }
      span.warning { color: #f80; }
      span.note    { color: #fc0; }
    }
  }
</style>

<template>
  <b-modaldialog class="b-preferencesdialog" @close="$emit ('close')">
    <div
	@click="popdown"
	slot="header">BEAST Preferences</div>
    <slot></slot>
    <b-fed-object :ref="fedobject" :value="prefdata" :default="defaults" :readonly="locked" @input="value_changed" debounce=500>
    </b-fed-object>

    <b-contextmenu class="b-preferencesdialog-cmenu" ref="cmenu" @click="menuactivation" >
      <b-menutitle> PCM / MIDI Device Selection </b-menutitle>

      <b-menuitem v-for="e in Array.prototype.concat (pcmlist, midilist)" :key="e.devid" :role="e.devid" :ic="driver_icon (e)" >
	{{ e.device_name }}
	<span class="card">  </span> <br />
	<span class="line1" v-if="e.capabilities" > {{ e.capabilities }} </span> <br v-if="e.capabilities" />
	<span class="line2" v-if="e.device_info" > {{ e.device_info }} </span> <br v-if="e.device_info" />
	<span class="line2" :class="notice_class (e)" v-if="e.notice" > {{ e.notice }} </span>
      </b-menuitem>

    </b-contextmenu>

  </b-modaldialog>
</template>

<script>
function component_data () {
  const data = {
    defaults: { getter: async c => Object.freeze (await Bse.server.get_config_defaults()), },
    locked:   { getter: async c => Bse.server.locked_config(), },
    prefdata: { getter: async c => {
      const d = await Bse.server.get_config();
      if (d.__typename__)
	d.__typedata__ = await Bse.server.find_typedata (d.__typename__);
      return Object.freeze (d);
    }, },
    pcmlist:  { getter: async c => Object.freeze (await Bse.server.list_pcm_drivers()), },
    midilist: { getter: async c => Object.freeze (await Bse.server.list_midi_drivers()), },
  };
  return this.observable_from_getters (data, () => true);
}

module.exports = {
  name: 'b-preferencesdialog',
  mixins: [ Util.vue_mixins.dom_updates, Util.vue_mixins.hyphen_props ],
  data() { return component_data.call (this); },
  methods: {
    notice_class (entry) {
      if (entry.notice.startsWith ("Warning:"))
	return "warning";
      if (entry.notice.startsWith ("Note:") || entry.notice.startsWith ("Notice:"))
	return "note";
      return "";
    },
    driver_icon (entry, is_midi = false) {
      if (entry.devid.startsWith ("jack="))
	return "mi-graphic_eq";
      if (entry.devid.startsWith ("alsa=pulse"))
	return "mi-speaker_group";
      if (entry.devid.startsWith ("null"))
	return "mi-not_interested"; // "fa-deaf";
      if (entry.devid.startsWith ("auto"))
	return "fa-cog";
      if (entry.device_name.startsWith ("HDMI"))
	return "fa-tv";
      if (entry.device_name.match (/\bMIDI\W*$/))
	return 'fa-music';
      if (!is_midi && (entry.device_name.match (/^USB /) || entry.device_info.match (/at usb-/)))
	return "fa-usb";
      if (entry.devid.match (/^alsa=.*:CARD=/))
	{
	  if (is_midi)
	    return 'fa-music';
	  if (entry.modem)
	    return "uc-☎ ";
	  if (entry.readonly)
	    return "mi-mic";
	  if (entry.writeonly)
	    return "fa-volume-up";
	  return "mi-headset_mic";
	}
      return "mi-not_interested";
    },
    async value_changed (po) {
      const prefs = await Bse.server.get_config();
      Util.assign_forin (prefs, po);
      Bse.server.set_config (prefs);
    },
    popdown (event) {
      this.$refs.cmenu.open (event);
    },
    menuactivation (event) {
    },
  },
};
</script>
