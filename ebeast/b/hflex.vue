<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

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
