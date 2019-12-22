<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-BUTTON-BAR
  A Vue container for tight packing of buttons.
  ## Slots:
  *slot*
  : All contents passed into this element will be packed tightly and styled as buttons.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-button-bar {
      display: inline-flex; background-color: $b-button-border;
      border: 1px solid $b-button-border;
      border-radius: $b-button-radius; }
  .b-button-bar .b-button,
  .b-button-bar button			{ margin: 0 0 0 1px; }
  .b-button-bar .b-button:first-of-type,
  .b-button-bar button:first-of-type	{ margin-left: 0; }
  .b-button-bar > .b-button:first-of-type,
  .b-button-bar > button:first-of-type	{ border-top-left-radius: $b-button-radius; border-bottom-left-radius: $b-button-radius; }
  .b-button-bar > .b-button:last-of-type,
  .b-button-bar > button:last-of-type	{ border-top-right-radius: $b-button-radius; border-bottom-right-radius: $b-button-radius; }
</style>

<template>
  <span class="b-button-bar" ><slot class="b-slot"></slot></span>
</template>

<script>
export default {
  name: 'b-button-bar',
};
</script>
