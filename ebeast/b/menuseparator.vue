<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-MENUSEPARATOR
  A menu element that serves as a visual separator between other elements.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  body hr.b-menuseparator {	//* since menus are often embedded, this needs high specificity */
    border-color: $b-menu-separator;
  }
</style>

<template>
  <hr class="b-menuseparator" />
</template>

<script>
export default {
  name: 'b-menuseparator',
};
</script>
