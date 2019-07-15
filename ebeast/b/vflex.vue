<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

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
