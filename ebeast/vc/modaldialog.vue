<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-modaldialog - A dialog component that dims everything else
  Using *vc-modaldialog* with *v-if* enables a modal dialog that dims all other
  elements while it is visible and constrains the focus chain. A *close* event is
  emitted on clicks outside of the dialog area, if *Escape* is pressed or the default
  *Close* button is actiavted.
  ### Events:
  - **close** - A *close* event is emitted once the "Close" button activated.
  ### Slots:
  - **header** - Slot to hold the modal dialog title.
  - **default** - Slot for the main content.
  - **footer** - Slot that by defaults holds a *Close* button which emits the *close* event.
  ### CSS:
  - **vc-modaldialog-mask** - Css class for the backdrop beneath the dialog contents.
  - **vc-modaldialog-container** - Css class for the actual dialog contents.
</docs>

<style lang="less">
  @import 'mixins.less';
  .vc-modaldialog * { flex-shrink: 0; }
  .vc-flex-vbox() {
      display: flex; flex-basis: auto; align-items: center; justify-content: center;
      flex-shrink: 0; flex-flow: column; height: auto;
  }
  .vc-flex-hbox() {
      display: flex; flex-basis: auto; align-items: center; justify-content: center;
      flex-shrink: 0; flex-flow: row; width: auto;
  }
  .vc-modaldialog-mask {
      .vc-flex-vbox;
      position: fixed; top: 0; left: 0; right: 0; bottom: 0;
      z-index: 99998;
      background-color: fadeout(darken(@vc-theme-background, 15%), 15%); /* backdrop */
  }
  .vc-modaldialog-wrapper {
      .vc-flex-vbox;
      max-height: 100%; max-width: 100%;
      padding: 1em;
  }
  .vc-modaldialog-container {
      .vc-flex-vbox; flex-shrink: 1;
      min-width: 25em; padding: 0;
      background-color: @vc-theme-background;
      border-radius: @vc-theme-border-radius;
      border-top: 3px solid @vc-panel-background-light; border-left: 3px solid @vc-panel-background-light;
      border-bottom: 3px solid @vc-panel-background-dark; border-right: 3px solid @vc-panel-background-dark;
      .vc-popup-box-shadow;
      /* fix vscrolling: https://stackoverflow.com/a/33455342 */
      justify-content: space-between;
      margin: auto;
      overflow: auto;
  }
  .vc-modaldialog-header {
      font-size: 1.5em; font-weight: bold;
      padding: 1rem 0 1.1rem;
  }
  .vc-modaldialog-header, .vc-modaldialog-footer {
      display: block; width: 100%; align-items: center; text-align: center;
      background-color: darken(@vc-theme-background, 8%);
  }
  .vc-modaldialog-footer {
      padding: 1.2rem 0 1rem;
  }
  .vc-modaldialog-body {
      padding: 1.5em 2em;
  }
  .vc-modaldialog-mask {
      transition: all .4s ease;
  }
  .vc-modaldialog-container {
      transition: all .4s ease-out;
  }
  .vc-modaldialog-transition-enter {
      opacity: 0;
  }
  .vc-modaldialog-transition-leave-active {
      opacity: 0;
  }
  .vc-modaldialog-transition-enter .vc-modaldialog-container
  { transform: translateY(-100%) scale(.5); }
  .vc-modaldialog-transition-leave-active .vc-modaldialog-container
  { transform: translateY(-100%) scale(.5); }
</style>

<template>
  <transition name="vc-modaldialog-transition">
    <div class="vc-modaldialog vc-modaldialog-mask" @click="close">
      <div class="vc-modaldialog-wrapper">
	<div class="vc-modaldialog-container" @click.stop>

	  <div class="vc-modaldialog-header">
	    <slot name="header">
	      Modal Dialog
	    </slot>
	  </div>
	  <div class="vc-modaldialog-body">
	    <slot name="default"></slot>
	  </div>
	  <div class="vc-modaldialog-footer">
	    <slot name="footer">
	      <vc-button class="vc-modaldialog-button" ref="bclose" @click="close">
		Close
	      </vc-button>
	    </slot>
	  </div>

	</div>
      </div>
    </div>
  </transition>
</template>

<script>

function focus_chain_constrain() {
  function is_descendant (ancestor, descendant) {
    do {
      if (descendant === ancestor)
	return true;
    } while ((descendant = descendant.parentNode));
    return false;
  }
  // save last focussed element
  this.focus_chain_last_active = document.activeElement;
  // find focusable children which are outside of this.$el
  const saved_elements =
    Array.prototype.filter.call (document.all,
				 o => o.tabIndex > -1 && !is_descendant (this.$el, o));
  // unset their tabIndex to prevent focus and create a restoration closure
  this.focus_chain_restores =
    saved_elements.map (o => {
      const oldindex = o.tabIndex;
      o.tabIndex = -1;
      return () => o.tabIndex = oldindex;
    });
}
function focus_chain_restore () {
  // restore saved tabIndex fields
  if (this.focus_chain_restores)
    this.focus_chain_restores.forEach (f => f());
  this.focus_chain_restores = null;
  // restore last focussed element
  if (this.focus_chain_last_active)
    this.focus_chain_last_active.focus();
  this.focus_chain_last_active = null;
}

module.exports = {
  name: 'vc-modaldialog',
  created () {
    window.addEventListener ('keydown', this.global_keydown);
  },
  mounted () {
    focus_chain_constrain.call (this);
    this.$refs.bclose.$el.focus();
  },
  beforeDestroy () {
    focus_chain_restore.call (this);
  },
  destroyed () {
    window.removeEventListener ('keydown', this.global_keydown);
  },
  methods: {
    close (event) {
      if (event instanceof Event)
	event.preventDefault();
      this.$emit ('close');
    },
    global_keydown (event) {
      const ESCAPE = 27;
      // TODO: ignore ESCAPE if document.activeElement is in hotkeys.js:textarea_types
      if (event.keyCode == ESCAPE)
	this.close();
      // non-global alternative: @keydown.esc="close"
    },
  },
};
</script>
