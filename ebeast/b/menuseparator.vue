<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-MENUSEPARATOR
  A menu element that serves as a visual separator between other elements.
</docs>

<style lang="scss">
  @import 'styles.scss';
  body hr.b-menuseparator {	//* since menus are often embedded, this needs high specificity */
    border-color: $b-menu-separator;
  }
</style>

<template>
  <hr class="b-menuseparator" />
</template>

<script>
module.exports = {
  name: 'b-menuseparator',
};
</script>
