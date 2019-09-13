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
  }
</style>

<template>
  <b-modaldialog class="b-preferencesdialog" @close="$emit ('close')">
    <div slot="header">BEAST Preferences</div>
    <slot></slot>
    <b-fed-object :ref="fedobject" :value="prefdata" :default="defaults" :readonly="locked" @input="value_changed" debounce=500>
    </b-fed-object>
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
  };
  return this.observable_from_getters (data, () => true);
}

module.exports = {
  name: 'b-preferencesdialog',
  mixins: [ Util.vue_mixins.dom_updates, Util.vue_mixins.hyphen_props ],
  data() { return component_data.call (this); },
  methods: {
    async value_changed (po) {
      const prefs = await Bse.server.get_config();
      Util.assign_forin (prefs, po);
      Bse.server.set_config (prefs);
    },
  },
};
</script>
