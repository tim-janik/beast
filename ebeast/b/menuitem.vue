<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-MENUITEM
  A menuitem element to be used as a descendant of a [B-CONTEXTMENU](#b-contextmenu).
  The menuitem can be activated via keyboard focus or mouse click and will notify
  its B-CONTEXTMENU about the click and its `role`, unless the `@click.prevent`
  modifier is used.
  If no `role` is specified, the B-CONTEXTMENU will still be notified to be closed,
  unless `$event.preventDefault()` is called.
  ## Props:
  *role*
  : Descriptor for this menuitem that is passed on to its B-CONTEXTMENU `onclick`.
  *disabled*
  : Boolean flag indicating disabled state.
  *fa*, *mi*, *uc*
  : Shorthands icon properties that are forwarded to a [B-ICON](#b-icon) used inside the menuitem.
  ## Events:
  *click*
  : Event emitted on keyboard/mouse activation, use `preventDefault()` to avoid closing the menu on clicks.
  ## Slots:
  *default*
  : All contents passed into this slot will be rendered as contents of this element.
</docs>

<style lang="scss">
  @import 'styles.scss';
  body button.b-menuitem { //* since menus are often embedded, this needs high specificity */
    display: inline-flex; flex: 0 0 auto; flex-wrap: nowrap;
    margin: 0; padding: 5px 1em; text-align: left;
    //* button-reset */
    background: transparent; cursor: pointer; user-select: none; outline: none;
    border: 1px solid transparent;
    color: $b-menu-foreground;
    &:not([disabled]) {
      .b-icon { color: $b-menu-fill; }
      &:focus {
	background-color: $b-menu-focus-bg; color: $b-menu-focus-fg; outline: none;
	border: 1px solid darken($b-menu-focus-bg, 50%);
	.b-icon { color: $b-menu-focus-fg; }
      }
      &.active, &:active, &:focus.active, &:focus:active {
	background-color: $b-menu-active-bg; color: $b-menu-active-fg; outline: none;
	border: 1px solid darken($b-menu-active-bg, 50%);
	.b-icon { color: $b-menu-active-fg; }
      }
    }
    &[disabled], &[disabled] * {
      color: $b-menu-disabled;
      .b-icon { color: $b-menu-disabled-fill; }
    }
    flex-direction: row; align-items: baseline;
    & > .b-icon:first-child { margin: 0 $b-menu-spacing 0 0; }
  }
  body .b-menurow-turn button.b-menuitem {
    flex-direction: column; align-items: center;
    & > .b-icon:first-child { margin: 0 0 $b-menu-spacing 0; }
  }
  body .b-menurow-noturn button.b-menuitem {
    .menulabel { min-width: 2em; } //* this aligns blocks of 2-digit numbers */
    & > .b-icon:first-child { margin: 0 $b-menu-tightspace 0 0; }
  }
</style>

<template>
  <button class="b-menuitem"
	  :disabled="isdisabled()"
	  @mouseenter="focus"
	  @click="clicked" >
    <b-icon class="menuicon" :fa="fa" :mi="mi" :uc="uc" v-if="menudata.showicons" />
    <span class="menulabel"><slot /></span>
  </button>
</template>

<script>
module.exports = {
  name: 'b-menuitem',
  props: [ 'role', 'disabled', 'fa', 'mi', 'uc' ],
  inject: { menudata: { from: 'b-contextmenu.menudata',
			default: { 'showicons': true, 'showaccels': true, }, },
  },
  methods: {
    clicked (event) {
      this.$emit ('click', event, this.role);
      if (!event.defaultPrevented)
	{
	  if (this.role && this.menudata.clicked)
	    this.menudata.clicked (this.role);
	  else if (this.menudata.close)
	    this.menudata.close();
	}
      event.stopPropagation();
      event.preventDefault(); // avoid submit, etc
    },
    isdisabled() {
      if (this.disabled == "" || !!this.disabled ||
	  this.$attrs['this.disabled'] == "" || !!this.$attrs['this.disabled'])
	return true;
      return false;
    },
    focus() {
      if (this.$el && !this.isdisabled())
	this.$el.focus();
    },
  },
};
</script>