<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TREESELECTOR-ITEM
  An element representing an entry of a B-TREESELECTOR, which allows selections.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-treeselector-item {
    user-select: none;
    & > span div:focus { outline: $b-focus-outline; }
    margin: $b-menu-vpad 0;
    &[disabled], &[disabled] * {
      color: $b-menu-disabled;
    }
  }
  .b-treeselector ul { // /* beware, adds styles via parent */
    list-style-type: none;
    margin: 0; padding: 0;
    margin-left: 1em;
    //* li { margin: 0; padding: 0; } */
    .b-treeselector-leaf {
      cursor: pointer; user-select: none;
      margin-left: 0;
      div { text-overflow: ellipsis; white-space: nowrap; overflow-x: hidden; }
    }
    .b-treeselector-caret {
      cursor: pointer; user-select: none;
      position: relative;
      &::before {
	content: "\25B8";
	position: absolute;
	left: -0.8em;
	color: white;
	display: inline;
	transition: all .3s ease;
      }
    }
    .b-treeselector-active > .b-treeselector-caret::before {
      content: "\25B9";
      transform: rotate(90deg);
    }
    .b-treeselector-nested { display: none; }
    .b-treeselector-active > .b-treeselector-nested { display: block; }
  }
</style>

<template>
  <li
      :is="li_or_div()"
      :disabled="isdisabled()"
      class="b-treeselector-item"
      :class="{ 'b-treeselector-active': is_active }" >
    <span class="b-treeselector-leaf"
	  v-if="!(entries && entries.length)">
      <div tabindex="0" @click="leaf_click1" @dblclick="leaf_click2" @keydown="leaf_keydown"
      >{{ label }}</div></span>
    <span class="b-treeselector-caret"
	  v-if="entries && entries.length" @click="caret_click" >
      <div tabindex="0" @click="caret_click" @keydown="caret_keydown"
      >{{ label }}</div></span>
    <ul class="b-treeselector-nested" ref="nested"
	v-if="entries && entries.length" >
      <b-treeselector-item
	  v-for="entry in entries"
          :entries="entry.entries"
          :label="entry.label"
          :uri="entry.uri"
          :disabled="entry.disabled"
	  :key="entry.label + ';' + entry.uri"
      ></b-treeselector-item>
    </ul>
  </li>
</template>

<script>
export default {
  name: 'b-treeselector-item',
  props: { label: 	{ default: '' },
	   uri:		{ default: '' },
	   entries:	{ default: [] },
  },
  data: function() { return { is_active: false, }; },
  inject: { menudata: { from: 'b-contextmenu.menudata',
			default: { showicons: true, showaccels: true, checkeduris: {},
				   isdisabled: () => false, onclick: undefined, }, },
  },
  methods: {
    caret_keydown: function (event) {
      if (Util.match_key_event (event, 'ArrowRight'))
	{
	  if (!this.is_active)
	    this.is_active = true;
	  else if (this.$refs.nested)
	    {
	      const nodes = Util.list_focusables (this.$refs.nested); // selector for focussable elements
	      if (nodes && nodes.length)
		nodes[0].focus();
	    }
	  event.preventDefault();
	}
      if (Util.match_key_event (event, 'ArrowLeft'))
	{
	  if (this.is_active)
	    {
	      this.is_active = false;
	      event.preventDefault();
	    }
	  else
	    this.leaf_keydown (event);
	}
    },
    leaf_keydown (event) {
      if (Util.match_key_event (event, 'ArrowLeft'))
	{
	  if (this.$el.parentElement && this.$el.parentElement.tagName == 'UL' &&
	      this.$el.parentElement.parentElement && this.$el.parentElement.parentElement.tagName == 'LI' &&
	      this.$el.parentElement.parentElement.parentElement &&
	      this.$el.parentElement.parentElement.parentElement.tagName == 'UL')
	    {
	      const nodes = Util.list_focusables (this.$el.parentElement.parentElement); // parent LI
	      if (nodes && nodes.length)
		nodes[0].focus();
	    }
	  event.preventDefault();
	}
    },
    caret_click: function() {
      this.is_active = !this.is_active;
      event.preventDefault();
      event.stopPropagation();
    },
    leaf_click1: function (event) {
      // trigger via focus/keyboard activation
      if (this.isdisabled() || !Util.in_keyboard_click())
	return;
      if (this.menudata.onclick)
	this.menudata.onclick.call (this, event);
      else
	debug ("SELECTED1:", event);
    },
    leaf_click2: function() {
      if (this.isdisabled())
	return;
      // using the mouse, only trigger on double click
      if (this.menudata.onclick)
	this.menudata.onclick.call (this, event);
      else
	debug ("SELECTED2:", event);
    },
    isdisabled ()	{ return this.menudata.isdisabled.call (this); },
    li_or_div: function() { return 'li'; },
  },
};
</script>
