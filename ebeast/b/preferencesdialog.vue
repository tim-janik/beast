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
    <b-fed-object :ref="fedobject" :value="preferences()" :default="defaults()" :readonly="locked()" @input="value_changed" debounce=500>
    </b-fed-object>
  </b-modaldialog>
</template>

<script>
module.exports = {
  name: 'b-preferencesdialog',
  methods: {
    defaults() {
      return Bse.server.get_config_defaults();
    },
    locked() {
      return Bse.server.locked_config();
    },
    preferences() {
      return Bse.server.get_config();
    },
    value_changed (po) {
      const prefs = Bse.server.get_config();
      Util.assign_forin (prefs, po);
      Bse.server.set_config (prefs);
    },
  },
};
</script>
