<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-CONTEXTMENU
  A modal popup that displays contextmenu choices, see [B-MENUITEM](#b-menuitem),
  [B-MENUSEPARATOR](#b-menuseparator).
  Using the `open()` method, the menu can be popped up from the parent component,
  and setting up an `onclick` handler can be used to handle menuitem actions. Example:
  ```html
  <div @contextmenu.prevent="$refs.cmenu.open">
    <b-contextmenu ref="cmenu" @click="menuactivation">...</b-contextmenu>
  </div>
  ```
  ## Props:
  *origin*
  : Reference DOM element to use for drop-down positioning.
  ## Events:
  *click (role)*
  : Event signaling activation of a submenu item, the `role` of the submenu is provided as argument.
  ## Methods:
  *open (event)*
  : Popup the contextmenu, the `event` coordinates are used for positioning.
  *close()*
  : Hide the contextmenu.
</docs>

<style lang="scss">
  @import 'styles.scss';
  body div.b-contextmenu-area {				    //* constraint area for context menu placement */
    position: absolute; right: 0; bottom: 0;		    //* fixed bottom right */
    display: flex; flex-direction: column; align-items: flex-start;
  }
  body .b-contextmenu-area .b-contextmenu {	//* since menus are often embedded, this needs high specificity */
    position: relative; max-width: 100%; max-height: 100%;  //* constrain to .b-contextmenu-area */
    overflow-y: auto; overflow-x: hidden;		    //* scroll if necessary */
    z-index: 999;					    //* stay atop modal shield */
    padding: $b-menu-padding 0;
    background-color: $b-menu-background; border: 1px outset darken($b-menu-background, 20%);
    color: $b-menu-foreground;
    box-shadow: $b-menu-box-shadow;
    [disabled], [disabled] * { pointer-events: none; }
    //* The template uses flex+start for layouting to scroll without vertical shrinking */
  }
</style>

<template>
  <div class="b-contextmenu-area" v-if="visible" ref="b-contextmenu-area" >
    <b-vflex class="b-contextmenu" ref="b-contextmenu" start>
      <slot />
    </b-vflex>
  </div>
</template>

<script>
module.exports = {
  name: 'b-contextmenu',
  props: [ 'origin', ],
  data_tmpl: { visible: false, doc_x: undefined, doc_y: undefined,
	       resize_observer: undefined, resize_timer: 0,
	       showicons: true, showaccels: true, },
  provide: Util.fwdprovide ('b-contextmenu.menudata',	// context for menuitem descendants
			    [ 'showicons', 'showaccels', 'clicked', 'close' ]),
  mounted () {
    this.update_shield();
    this.position_popup();
    this.resize_observer = new ResizeObserver (entries => {
      if (!this.resize_timer)
	this.resize_timer = setTimeout (() => {
	  this.resize_timer = 0;
	  this.position_popup();
	}, 1);
    });
  },
  beforeUpdate () {
    this.update_shield();
    this.position_popup();
  },
  updated () {
    this.resize_observer.disconnect();
    this.update_shield();
    this.position_popup();
    if (this.$refs['b-contextmenu'] && this.$refs['b-contextmenu'].$el)
      {
	this.resize_observer.observe (this.$refs['b-contextmenu'].$el);
	this.resize_observer.observe (document.body);
	/* adding `origin` to the observer is of little use, for live repositioning,
	 * we would need to observe the origin's size *and* viewport position.
	 */
      }
  },
  beforeDestroy () {
    this.resize_observer.disconnect();
    leaked_observers.push (this.resize_observer); /* workaround */
    this.resize_observer = undefined;
    if (this.resize_timer)
      clearTimeout (this.resize_timer);
    this.resize_timer = 0;
    if (this.shield)
      this.shield.destroy (false);
    this.shield = undefined;
  },
  methods: {
    position_popup() {
      let area_el = this.$refs['b-contextmenu-area'];
      if (area_el && area_el.getBoundingClientRect) // ignore comments
	{
	  const menu_el = this.$refs['b-contextmenu'].$el;
	  // unset size constraints before calculating desired size
	  area_el.style.left = "0px";
	  area_el.style.top = "0px";
	  const p = Util.popup_position (menu_el, { x: this.doc_x,
						    y: this.doc_y,
						    origin: this.origin && this.origin.$el || this.origin });
	  area_el.style.left = p.x + "px";
	  area_el.style.top = p.y + "px";
	}
    },
    update_shield() {
      const contextmenu = this.$refs['b-contextmenu'] && this.$refs['b-contextmenu'].$el;
      if (!contextmenu && this.shield)
	{
	  this.shield.destroy (false);
	  this.shield = undefined;
	}
      if (contextmenu && this.visible && !this.shield)
	this.shield = Util.modal_shield (this.close, contextmenu, { focuscycle: true,
								    background: '#00000000' });
    },
    open (event) {
      if (this.visible) return;
      if (event && event.pageX && event.pageY)
	{
	  this.doc_x = event.pageX;
	  this.doc_y = event.pageY;
	}
      else
	this.doc_x = this.doc_y = undefined;
      this.visible = true;
    },
    close () {
      if (!this.visible) return;
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
const leaked_observers = []; // FIXME: electron-3.1.12 crashes after ResizeObserver destruction
</script>
