<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TREESELECTOR-ITEM
  An element representing an entry of a B-TREESELECTOR, which allows selections.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-treeselector-item {
    user-select: none;
    & > span:focus { outline: $b-focus-outline; }
  }
  .b-treeselector ul { // /* beware, adds styles via parent */
    list-style-type: none;
    margin: 0; padding: 0;
    margin-left: 1em;
    li { margin: 0; padding: 0; }
    .b-treeselector-leaf {
      cursor: pointer; user-select: none;
      margin-left: 0;
    }
    .b-treeselector-caret {
      cursor: pointer; user-select: none;
      position: relative;
      &::before {
	content: "\25B6";
	position: absolute;
	left: -1em;
	color: white;
	display: inline;
	transition: all .3s ease;
      }
    }
    .b-treeselector-active > .b-treeselector-caret::before {
      transform: rotate(90deg);
    }
    .b-treeselector-nested { display: none; }
    .b-treeselector-active > .b-treeselector-nested { display: block; }
  }
</style>

<template>
  <li
      :is="li_or_div()"
      class="b-treeselector-item"
      :class="{ 'b-treeselector-active': is_active }" >
    <span class="b-treeselector-leaf"
	  tabindex="0"
	  @click="leaf_click1" @dblclick="leaf_click2"
	  v-if="!(entries && entries.length)">{{ label }}</span>
    <span class="b-treeselector-caret"
	  tabindex="0"
	  @click="caret_click"
	  @keydown="caret_keydown"
	  v-if="entries && entries.length" >{{ label }}</span>
    <ul class="b-treeselector-nested"
	v-if="entries && entries.length" >
      <b-treeselector-item
	  v-for="entry in entries"
          :entries="entry.entries"
          :label="entry.label"
          :key="entry.label"
      ></b-treeselector-item>
    </ul>
  </li>
</template>

<script>
module.exports = {
  name: 'b-treeselector-item',
  props: [ 'label', 'entries' ],
  data: function() { return { is_active: false, }; },
  methods: {
    caret_click: function() {
      this.is_active = !this.is_active;
    },
    caret_keydown: function (event) {
      if ((!this.is_active && Util.match_key_event (event, 'right')) ||
	  (this.is_active && Util.match_key_event (event, 'left')))
	this.is_active = !this.is_active;
    },
    leaf_click1: function (event) {
      // trigger via focus/keyboard activation
      if (Util.in_keyboard_click())
	console.log ("SELECTED:", event);
    },
    leaf_click2: function() {
      // using the mouse, only trigger on double click
      console.log ("SELECTED:", this.label);
    },
    li_or_div: function() { return 'li'; },
  },
};
</script>
