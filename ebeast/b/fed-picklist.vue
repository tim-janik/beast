<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-FED-PICKLIST
  A field-editor for picklist input.
  ## Properties:
  *value*
  : Contains the picklist string being edited.
  *readonly*
  : Make this component non editable for the user.
  ## Events:
  *input*
  : This event is emitted whenever the value changes through user input or needs to be constrained.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-fed-picklist		{
    $b-fed-picklist-width: 23em;
    .b-fed-picklist-button {
      width: 1.5 + $b-fed-picklist-width + 1.5;
      white-space: nowrap;
      text-overflow: ellipsis;
      overflow-x: hidden;
      user-select: none;

      padding: .1em .3em .1em .3em;
      @include b-style-outset();

      .b-fed-picklist-icon {
	flex: 0 0 auto; width: 1em;
	margin: 0 .5em 0 0;
      }

      .b-fed-picklist-label {
	flex: 1 1 auto;
	// width: $b-fed-picklist-width;
	text-align: left;
	overflow-x: hidden;
      }

      .b-fed-picklist-arrow {
	flex: 0 0 auto; width: 1em;
	margin: 0 0 0 .5em;
      }
    }
  }
  .b-fed-picklist-contextmenu {
    $b-fed-picklist-lines: ".b-fed-picklist-line1, .b-fed-picklist-line2, .b-fed-picklist-line3, .b-fed-picklist-line4, \
      .b-fed-picklist-line5, .b-fed-picklist-line6, .b-fed-picklist-line7, .b-fed-picklist-line8, .b-fed-picklist-line9";
    .b-fed-picklist-label    { display: block; }
    #{$b-fed-picklist-lines} { display: block; font-size: 90%; color: $b-style-fg-secondary; }
    .b-menuitem {
      &:focus, &.active, &:active {
	#{$b-fed-picklist-lines} { filter: $b-style-fg-filter; } //* adjust to inverted menuitem */
      } }
  }
</style>

<template>
  <b-hflex class="b-fed-picklist" ref="flexroot" @click="open_menu" @mousedown="open_menu" >
    <b-hflex class="b-fed-picklist-button" start tabindex="0" ref="picklistbutton" >
      <b-icon class="b-fed-picklist-icon" v-if="currentitem.icon"
	      :ic="currentitem.icon" :iconclass='currentitem.iconclass' />
      <span class="b-fed-picklist-label" :class='currentitem.labelclass' > {{ currentitem.label }} </span>
      <span class="b-fed-picklist-arrow" :class='currentitem.arrowclass' > ⬍ <!-- ▼ ▽ ▾ ▿ ⇕ ⬍ ⇳ --> </span>
    </b-hflex>

    <b-contextmenu class="b-fed-picklist-contextmenu" ref="cmenu" @click="menuactivation" notransitions >
      <b-menutitle v-if="title" > {{ title }} </b-menutitle>

      <b-menuitem v-for="item in picklist_" :key="item.key || item.role" :role="item.role || item.key"
		  :ic="item.icon"      :iconclass='item.iconclass' >
	<span class="b-fed-picklist-label" :class='item.labelclass' v-if="item.label" > {{ item.label }} </span>
	<span class="b-fed-picklist-line1" :class='item.line1class' v-if="item.line1" > {{ item.line1 }} </span>
	<span class="b-fed-picklist-line2" :class='item.line2class' v-if="item.line2" > {{ item.line2 }} </span>
	<span class="b-fed-picklist-line3" :class='item.line3class' v-if="item.line3" > {{ item.line3 }} </span>
	<span class="b-fed-picklist-line4" :class='item.line4class' v-if="item.line4" > {{ item.line4 }} </span>
	<span class="b-fed-picklist-line5" :class='item.line5class' v-if="item.line5" > {{ item.line5 }} </span>
	<span class="b-fed-picklist-line6" :class='item.line6class' v-if="item.line6" > {{ item.line6 }} </span>
	<span class="b-fed-picklist-line7" :class='item.line7class' v-if="item.line7" > {{ item.line7 }} </span>
	<span class="b-fed-picklist-line8" :class='item.line8class' v-if="item.line8" > {{ item.line8 }} </span>
	<span class="b-fed-picklist-line9" :class='item.line9class' v-if="item.line9" > {{ item.line9 }} </span>
      </b-menuitem>

    </b-contextmenu>

  </b-hflex>
</template>

<script>
module.exports = {
  name: 'b-fed-picklist',
  props: {
    value:	   { type: String, },
    title:	   { type: String, },
    readonly:	   { type: Boolean, default: false, },
    picklistitems: { type: Function, required: true, },
  },
  data_tmpl: {
    placeholder: '',
    opencount: 0, // counter to force invalidation on user-triggered menu popup
  },
  computed: {
    picklist_() {
      // *use* this.opencount, so changing it causes Vue to see invalidation
      void this.opencount;
      return this.picklistitems (this.opencount);
    },
    currentitem() {
      for (const item of this.picklist_)
	if (item.key == this.value || item.role == this.value)
	  return item;
      // fallback for unlisted value
      return { label: this.value, icon: 'fa-question-circle-o' };	// � ⟁
    },
  },
  methods: {
    menuactivation (role) {
      this.emit_input_value (role);
    },
    emit_input_value (inputvalue) {		// emit 'input' with constrained value
      const constrainedvalue = this.constrain (inputvalue);
      if (this.$refs.texttype && this.$refs.texttype.value != constrainedvalue)
	this.$refs.texttype.value = constrainedvalue;
      if (this.value != constrainedvalue)
	this.$emit ('input', constrainedvalue);
    },
    constrain (v) {
      return String (v);
    },
    pickliststyle() {
      // determine width for input types
      const width = 1 + 15; // padding + characters
      return `width: ${width}em;`;
    },
    open_menu (event) {
      // Firefox-68 and Electron-6.0.1 generate a 'click' for mousedown+ESC+mouseup inside `picklistbutton`
      if (event.type == 'click' && event.detail > 0)  // click events originating from the mouse have a non-0 detail
	return;                                       // ignore, we use 'mousedown' for popups via mouse
      // when opening through @mousedown, force focus *before* the menu's modal_shield is installed
      if (event.type == 'mousedown')
	this.$refs.picklistbutton.focus();
      // in any case, avoid propagation or default processing once the menu is up
      event.stopPropagation(); // avoid other 'mouseup' handlers
      event.preventDefault();  // avoid generating 'click' from 'mousedown'
      // trigger getter() refresh on each menu popup
      this.opencount++;
      // keep an 'active' indicator for picklistbutton during menu popup
      Util.clear_keyboard_click (this.$refs.picklistbutton); // reset cleanup handler from potential keyboard_click
      const tieclass = { element: this.$refs.picklistbutton, class: 'active' };
      // popup, the context menu takes it from here
      this.$refs.cmenu.popup (event, { origin: this.$refs.flexroot, tieclass: tieclass });
    },
  },
};
</script>
