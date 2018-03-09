<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-part-thumb - Vue template to display a part thumbnail
  ### Props:
  - **part** - The BsePart to display.
</docs>

<template>

  <span class="vc-part-thumb" >
    [{{ part.get_name() }}]
  </span>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-part-thumb {
    background-color: hsv(190, 75%, 55%);
    font-size: 80%;
    border: 1px solid $vc-button-border;
    border-radius: $vc-button-radius; }
  .vc-part-thumb > *	{ margin: 0; }
</style>

<script>
module.exports = {
  name: 'vc-part-thumb',
  props: {
    'part': { type: Bse.Part, },
  },
  methods: {
  },
  data_tmpl: {
    name: "Part-Label-XYZ",
  },
};
</script>
