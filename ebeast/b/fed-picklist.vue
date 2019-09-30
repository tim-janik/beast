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
  @import 'styles.scss';
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
      // @include b-style-inset();

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
  .b-fed-picklist-cmenu {
    .b-fed-picklist-line1, .b-fed-picklist-line2, .b-fed-picklist-line3, .b-fed-picklist-line4,
    .b-fed-picklist-line5, .b-fed-picklist-line6, .b-fed-picklist-line7, .b-fed-picklist-line8,
    .b-fed-picklist-line9	{ display: block; font-size: 90%; color: #bbb; }
    .b-fed-picklist-line0	{ display: block; }
  }
</style>

<template>
  <b-hflex class="b-fed-picklist" ref="flexroot" @click="open_menu" @mousedown="open_menu" >
    <b-hflex class="b-fed-picklist-button" start tabindex="0" ref="picklistbutton" >
      <b-icon class="b-fed-picklist-icon" :ic="currentitem.icon" v-if="currentitem.icon" />
      <span class="b-fed-picklist-label"> {{ currentitem.label }} </span>
      <span class="b-fed-picklist-arrow"> ⬍ <!-- ▼ ▽ ▾ ▿ ⇕ ⬍ ⇳ --> </span>
    </b-hflex>

    <b-contextmenu class="b-fed-picklist-cmenu" ref="cmenu" @click="menuactivation" notransitions >
      <b-menutitle v-if="title" > {{ title }} </b-menutitle>

      <b-menuitem v-for="item in picklist_" :key="item.key || item.role" :role="item.role || item.key" :ic="item.icon" >
	<span class="b-fed-picklist-line0" v-if="item.label" > {{ item.label }} </span>
	<span class="b-fed-picklist-line1" v-if="item.line1" > {{ item.line1 }} </span>
	<span class="b-fed-picklist-line2" v-if="item.line2" > {{ item.line2 }} </span>
	<span class="b-fed-picklist-line3" v-if="item.line3" > {{ item.line3 }} </span>
	<span class="b-fed-picklist-line4" v-if="item.line4" > {{ item.line4 }} </span>
	<span class="b-fed-picklist-line5" v-if="item.line5" > {{ item.line5 }} </span>
	<span class="b-fed-picklist-line6" v-if="item.line6" > {{ item.line6 }} </span>
	<span class="b-fed-picklist-line7" v-if="item.line7" > {{ item.line7 }} </span>
	<span class="b-fed-picklist-line8" v-if="item.line8" > {{ item.line8 }} </span>
	<span class="b-fed-picklist-line9" v-if="item.line9" > {{ item.line9 }} </span>
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
      // popup, the context menu takes it from here
      this.$refs.cmenu.popup (event, this.$refs.flexroot);
    },
  },
};
</script>
