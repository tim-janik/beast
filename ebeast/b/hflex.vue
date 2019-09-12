<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-HFLEX
  A [B-FLEX](#b-flex) container for horizontal layouting.
</docs>

<script>
const b_flex = require ('./flex.vue');
module.exports = {
  name: 'b-hflex',
  extends: b_flex,
  data_tmpl: { derivedclass: 'flex-row' },
};
</script>
