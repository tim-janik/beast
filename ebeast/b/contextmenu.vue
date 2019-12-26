<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-CONTEXTMENU
  A modal popup that displays contextmenu choices, see [B-MENUITEM](#b-menuitem),
  [B-MENUSEPARATOR](#b-menuseparator).
  Using the `popup()` method, the menu can be popped up from the parent component,
  and setting up an `onclick` handler can be used to handle menuitem actions. Example:
  ```html
  <div @contextmenu.prevent="$refs.cmenu.popup">
    <b-contextmenu ref="cmenu" @click="menuactivation">...</b-contextmenu>
  </div>
  ```
  ## Props:
  ## Events:
  *click (role)*
  : Event signaling activation of a submenu item, the `role` of the submenu is provided as argument.
  ## Methods:
  *popup (event, origin)*
  : Popup the contextmenu, the `event` coordinates are used for positioning, the `origin` is a
  : reference DOM element to use for drop-down positioning.
  *close()*
  : Hide the contextmenu.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  body div.b-contextmenu-area {				    //* constraint area for context menu placement */
    position: fixed; right: 0; bottom: 0;		    //* fixed bottom right */
    display: flex; flex-direction: column; align-items: flex-start;
    //* transition */
    $timing: 0.1s;
    &.v-enter-active	{ transition: opacity $timing ease-out, transform $timing linear; }
    &.v-leave-active	{ transition: opacity $timing ease-in,  transform $timing linear; }
    &.v-enter	 	{ opacity: 0.3; transform: translateX(-0.3vw) translateY(-1vh); }
    &.v-leave-to	{ opacity: 0; transform: translateX(-0%) translateY(-0%) scale(1); }
    &.b-contextmenu-notransitions { transition: none !important; }
  }
  body .b-contextmenu-shield {
    background: #0001;
    transition: background 0.1s;
  }
  body .b-contextmenu-area .b-contextmenu {	//* since menus are often embedded, this needs high specificity */
    //* The template uses flex+start for layouting to scroll without vertical shrinking */
    position: relative; max-width: 100%; max-height: 100%;  //* constrain to .b-contextmenu-area */
    overflow-y: auto; overflow-x: hidden;		    //* scroll if necessary */
    padding: $b-menu-padding 0;
    background-color: $b-menu-background; border: 1px outset darken($b-menu-background, 20%);
    color: $b-menu-foreground;
    box-shadow: $b-menu-box-shadow;
    [disabled], [disabled] * { pointer-events: none; }
  }
</style>

<template>
  <transition>
    <div class='b-contextmenu-area' :class='cmenu_class' ref='contextmenuarea' v-if='visible' >
      <b-vflex class='b-contextmenu' ref='cmenu' start >
	<slot />
      </b-vflex>
    </div>
  </transition>
</template>

<script>
export default {
  name: 'b-contextmenu',
  props: { 'notransitions': { default: false }, },
  computed: {
    cmenu_class() { return this.notransitions !== false ? 'b-contextmenu-notransitions' : ''; },
  },
  data_tmpl: { visible: false, doc_x: undefined, doc_y: undefined,
	       resize_observer: undefined, checkedroles: {},
	       showicons: true, showaccels: true, checker: undefined, },
  provide: Util.fwdprovide ('b-contextmenu.menudata',	// context for menuitem descendants
			    [ 'checkedroles', 'showicons', 'showaccels', 'clicked', 'close' ]),
  methods: {
    dom_update () {
      if (!this.resize_observer)
	{
	  this.resize_observer = new Util.ResizeObserver ((e, ro) => {
	    if (this.resize_timer)
	      return;
	    this.resize_timer = setTimeout (() => {
	      this.resize_timer = 0;
	      this.position_popup();
	    }, 1);
	  });
	}
      else if (this.resize_timer)
	{
	  clearTimeout (this.resize_timer);
	  this.resize_timer = 0;
	}
      this.resize_observer.disconnect();
      this.update_shield();
      if (this.$refs.cmenu)
	{
	  this.checkitems();
	  this.position_popup();
	  this.resize_observer.observe (document.body);
	  if (this.origin)
	    this.resize_observer.observe (this.origin.$el || this.origin);
	}
      else if (this.tieclass)
	{
	  this.tieclass.element.classList.remove (this.tieclass.class);
	  this.tieclass = undefined;
	}
    },
    dom_destroy () {
      this.clear_dragging();
      if (this.tieclass)
	{
	  this.tieclass.element.classList.remove (this.tieclass.class);
	  this.tieclass = undefined;
	}
      this.resize_observer.disconnect();
      this.resize_observer = undefined;
      if (this.resize_timer)
	clearTimeout (this.resize_timer);
      this.resize_timer = 0;
      if (this.shield)
	this.shield.destroy (false);
      this.shield = undefined;
    },
    position_popup() {
      const area_el = this.$refs.contextmenuarea;
      if (area_el && area_el.getBoundingClientRect) // ignore Vue placeholder (html comment)
	{
	  const menu_el = this.$refs.cmenu;
	  // unset size constraints before calculating desired size, otherwise resizing
	  // can take dozens of resize_observer/popup_position frame iterations
	  area_el.style.left = '0px';
	  area_el.style.top = '0px';
	  const p = Util.popup_position (menu_el, { x: this.doc_x,
						    y: this.doc_y,
						    origin: this.origin && this.origin.$el || this.origin });
	  area_el.style.left = p.x + "px";
	  area_el.style.top = p.y + "px";
	}
    },
    checkitems() {
      if (!this.checker)
	return;
      const checkrecursive = component => {
	if (component instanceof Element &&
	    component.__vue__)
	  component = component.__vue__;
	if (component.$options && component.$options.propsData && component.$options.propsData.role)
	  {
	    let result = this.checker.call (null, component.$options.propsData.role, component);
	    if ('boolean' !== typeof result)
	      result = undefined;
	    if (result != this.checkedroles[component.$options.propsData.role])
	      {
		this.$set (this.checkedroles, component.$options.propsData.role, result); // Vue reactivity
		component.$forceUpdate();
	      }
	  }
	if (component.$children)
	  for (let child of component.$children)
	    checkrecursive (child);
	else if (component.children) // DOM element, possibly a function component
	  for (let child of component.children)
	    checkrecursive (child);
      };
      if (this.$refs.cmenu)
	checkrecursive (this.$refs.cmenu);
    },
    update_shield() {
      const contextmenu = this.$refs.cmenu;
      if (!contextmenu && this.shield)
	{
	  this.shield.destroy (false);
	  this.shield = undefined;
	}
      if (contextmenu && this.visible && !this.shield)
	this.shield = Util.modal_shield (contextmenu, { class: 'b-contextmenu-shield',
							root: this.$refs.contextmenuarea,
							close: this.close });
    },
    popup (event, options) {
      const { origin, check, tieclass } = options || {};
      this.visible = false;
      this.origin = origin;
      this.checker = check;
      this.tieclass = tieclass;
      this.clear_dragging();
      if (event && event.pageX && event.pageY)
	{
	  this.doc_x = event.pageX;
	  this.doc_y = event.pageY;
	}
      else
	this.doc_x = this.doc_y = undefined;
      if (event.type == "mousedown")
	{
	  console.assert (!this.dragging);
	  this.dragging = {
	    button: event.button,
	    ignoreclick: true,
	    timer: setTimeout (() => { if (this.dragging) this.dragging.ignoreclick = false; }, 500),
	    handler: (e) => this.drag_event (e),
	    evpassive: { capture: true, passive: true },
	    evactive: { capture: false, passive: false }
	  };
	  window.addEventListener ('mouseup',   this.dragging.handler, this.dragging.evactive);
	  window.addEventListener ('mousedown', this.dragging.handler, this.dragging.evpassive);
	  window.addEventListener ('keydown',   this.dragging.handler, this.dragging.evpassive);
	}
      if (this.tieclass)
	this.tieclass.element.classList.add (this.tieclass.class);
      this.visible = true;
    },
    clear_dragging() {
      if (!this.dragging)
	return;
      if (this.dragging.ignoreclick)
	clearTimeout (this.dragging.timer);
      window.removeEventListener ('mouseup',   this.dragging.handler, this.dragging.evactive);
      window.removeEventListener ('mousedown', this.dragging.handler, this.dragging.evpassive);
      window.removeEventListener ('keydown',   this.dragging.handler, this.dragging.evpassive);
      this.dragging = undefined;
    },
    drag_event (event) {
      console.assert (this.dragging);
      let clickit = null;
      if (this.visible && !this.dragging.ignoreclick &&
	  event.type == 'mouseup' && event.button == this.dragging.button &&
	  this.$refs.cmenu.contains (document.activeElement) &&
	  document.activeElement.contains (event.target))
	{
	  clickit = document.activeElement;
	  event.stopPropagation(); // avoid other 'mouseup' handlers
	  event.preventDefault();  // avoid generating 'click' from 'mouseup'
	}
      // clean up `dragging` state
      this.clear_dragging();
      // activate due to valid drag-selection
      if (clickit)
	Util.keyboard_click (clickit);
    },
    close () {
      this.clear_dragging();
      if (!this.visible)
	return;
      this.visible = false;
      // take down shield immediately, to remove focus guards
      if (this.shield)
	this.shield.destroy (false);
      this.shield = undefined;
    },
    clicked (role) {
      this.$emit ('click', role);
      this.close();
    },
  },
};
</script>
