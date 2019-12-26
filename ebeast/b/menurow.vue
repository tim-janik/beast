<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-MENUROW
  Menuitems are packed horizontally inside a menurow.
  ## Props:
  *noturn*
  : Avoid turning the icon-label direction in menu items to be upside down.
  ## Slots:
  *default*
  : All contents passed into this slot will be rendered as contents of this element.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  body div.b-menurow { //* since menus are often embedded, this needs high specificity */
    display: inline-flex; flex: 0 0 auto; flex-wrap: nowrap; flex-direction: row;
    justify-content: center;
    align-items: baseline;      //* distribute extra cross-axis space */
    margin: 0; padding: 0; text-align: center;
  }
</style>

<template>
  <div class="b-menurow" :class="classlist">
    <slot />
  </div>
</template>

<script>
export default {
  name: 'b-menurow',
  props: {
    'noturn': false,
  },
  computed: {
    classlist() {
      let classes = [];
      if (this.noturn || this.noturn === '')
	classes.push ('b-menurow-noturn');
      else
	classes.push ('b-menurow-turn');
      return classes.join (' ');
    },
  },
};
</script>
