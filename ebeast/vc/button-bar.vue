<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-button-bar - A container for tight packing of buttons
  ### Props:
  ### Events:
  ### Slots:
  - **slot** - All contents passed into this element will be packed tightly and styled as buttons.
</docs>

<style lang="less">
  @import 'styles.less';
  .vc-button-bar {
      display: inline-flex; background-color: @vc-button-border;
      border: 1px solid @vc-button-border;
      border-radius: @vc-button-radius; }
  .vc-button-bar .vc-button,
  .vc-button-bar button			{ margin: 0 0 0 1px; }
  .vc-button-bar .vc-button:first-of-type,
  .vc-button-bar button:first-of-type	{ margin-left: 0; }
  .vc-button-bar > .vc-button:first-of-type,
  .vc-button-bar > button:first-of-type	{ border-top-left-radius: @vc-button-radius; border-bottom-left-radius: @vc-button-radius; }
  .vc-button-bar > .vc-button:last-of-type,
  .vc-button-bar > button:last-of-type	{ border-top-right-radius: @vc-button-radius; border-bottom-right-radius: @vc-button-radius; }
</style>

<template>
  <span class="vc-button-bar" ><slot class="vc-slot"></slot></span>
</template>

<script>
module.exports = {
  name: 'vc-button-bar',
};
</script>
