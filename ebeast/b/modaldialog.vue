<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-MODALDIALOG
  A dialog component that disables and dims everything else for exclusive dialog use.
  Using *b-modaldialog* with *v-if* enables a modal dialog that dims all other
  elements while it is visible and constrains the focus chain. A *close* event is
  emitted on clicks outside of the dialog area, if *Escape* is pressed or the default
  *Close* button is actiavted.
  ## Events:
  *close*
  : A *close* event is emitted once the "Close" button activated.
  ## Slots:
  *header*
  : The *header* slot can be used to hold the modal dialog title.
  *default*
  : The *default* slot holds the main content.
  *footer*
  : By default, the *footer* slot holds a *Close* button which emits the *close* event.
  ## CSS Classes:
  *b-modaldialog-mask*
  : The *b-modaldialog-mask* CSS class is used as backdrop beneath the dialog contents to mask non-modal contents.
  *b-modaldialog*
  : The *b-modaldialog* CSS class contains styling for the actual dialog contents.
</docs>

<style lang="scss">
  @import 'styles.scss';
  @mixin b-flex-vbox() {
    display: flex; flex-basis: auto; align-items: center; justify-content: center;
    flex-shrink: 0; flex-flow: column; height: auto;
  }
  @mixin b-flex-hbox() {
    display: flex; flex-basis: auto; align-items: center; justify-content: center;
    flex-shrink: 0; flex-flow: row; width: auto;
  }
  .b-modaldialog-mask {
    @include b-flex-vbox;
    position: fixed; top: 0; left: 0; right: 0; bottom: 0;
    z-index: 99998;
    background-color: fadeout(darken($b-theme-background, 15%), 15%); /* backdrop */
  }
  .b-modaldialog {
    $dist: 1em; z-index: 99999;
    position: fixed; top: $dist; left: $dist; right: $dist; bottom: $dist;
    max-height: 100%; max-width: 100%;
    * { flex-shrink: 0; }
    @include b-flex-vbox; flex-shrink: 1;
    min-width: 16em; padding: 0;
    background-color: $b-theme-background;
    border-radius: $b-theme-border-radius;
    border-top: 3px solid $b-main-border-light; border-left: 3px solid $b-main-border-light;
    border-bottom: 3px solid $b-main-border-dark; border-right: 3px solid $b-main-border-dark;
    @include b-popup-box-shadow;
    /* fix vscrolling: https://stackoverflow.com/a/33455342 */
    justify-content: space-between;
    margin: auto;
    overflow: auto;
    transition: all .4s ease-out;
  }
  .b-modaldialog-header {
    font-size: 1.5em; font-weight: bold;
    padding: 1rem 0 1.1rem;
  }
  .b-modaldialog-header, .b-modaldialog-footer {
    display: block; width: 100%; align-items: center; text-align: center;
    background-color: darken($b-theme-background, 8%);
  }
  .b-modaldialog-footer {
    padding: 1.2rem 0 1rem;
  }
  .b-modaldialog-body {
    padding: 1.5em 2em;
  }
  .b-modaldialog-mask {
    transition: all .4s ease;
  }
  .b-modaldialog-transition-enter {
    opacity: 0;
  }
  .b-modaldialog-transition-leave-active {
    opacity: 0;
  }
  .b-modaldialog-transition-enter .b-modaldialog
  { transform: translateY(-100%) scale(.5); }
  .b-modaldialog-transition-leave-active .b-modaldialog
  { transform: translateY(-100%) scale(.5); }
</style>

<template>
  <transition name="b-modaldialog-transition">
    <div class="b-modaldialog" @click.stop ref="container">

      <div class="b-modaldialog-header">
	<slot name="header">
	  Modal Dialog
	</slot>
      </div>
      <div class="b-modaldialog-body">
	<slot name="default"></slot>
      </div>
      <div class="b-modaldialog-footer">
	<slot name="footer">
	  <b-button class="b-modaldialog-button" ref="bclose" @click="close">
	    Close
	  </b-button>
	</slot>
      </div>

    </div>
  </transition>
</template>

<script>
module.exports = {
  name: 'b-modaldialog',
  mounted () {
    this.shield = Util.modal_shield (this.close, this.$refs['container'], { focuscycle: true });
    const focus_close = true;
    if (focus_close)
      this.$refs.bclose.$el.focus();
  },
  beforeDestroy () {
    if (this.shield)
      this.shield.destroy (false);
  },
  methods: {
    close (event) {
      if (this.shield)
	this.shield.destroy (false);
      if (event instanceof Event)
	event.preventDefault();
      this.$emit ('close');
    },
  },
};
</script>
