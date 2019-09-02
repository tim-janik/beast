<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-VFLEX
  A [B-FLEX](#b-flex) container for vertical layouting.
</docs>

<script>
const b_flex = require ('./flex.vue');
module.exports = {
  name: 'b-vflex',
  extends: b_flex,
  data_tmpl: { derivedclass: 'flex-column' },
};
</script>
