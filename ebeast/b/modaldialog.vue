<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-MODALDIALOG
  A dialog component that disables and dims everything else for exclusive dialog use.
  Using *b-modaldialog* with *v-if* enables a modal dialog that dims all other
  elements while it is visible and constrains the focus chain. A *close* event is
  emitted on clicks outside of the dialog area, if *Escape* is pressed or the default
  *Close* button is actiavted.
  ## Props:
  *value*
  : A boolean value to control the visibility of the dialog, suitable for `v-model` bindings.
  ## Events:
  *input*
  : An *input* event with value `false` is emitted when the "Close" button activated.
  ## Slots:
  *header*
  : The *header* slot can be used to hold the modal dialog title.
  *default*
  : The *default* slot holds the main content.
  *footer*
  : By default, the *footer* slot holds a *Close* button which emits the *close* event.
  ## CSS Classes:
  *b-modaldialog*
  : The *b-modaldialog* CSS class contains styling for the actual dialog contents.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  @mixin b-flex-vbox() {
    display: flex; flex-basis: auto; align-items: center; justify-content: center;
    flex-shrink: 0; flex-flow: column; height: auto;
  }
  @mixin b-flex-hbox() {
    display: flex; flex-basis: auto; align-items: center; justify-content: center;
    flex-shrink: 0; flex-flow: row; width: auto;
  }
  .b-modaldialog {
    position: fixed; top: auto; left: auto; right: auto; bottom: auto;
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
    &.v-enter-active 		{ transition: opacity 0.15s ease-out, transform 0.15s linear; }
    &.v-leave-active		{ transition: opacity 0.15s ease-in,  transform 0.15s linear; }
    &.v-enter, &.v-leave-to 	{ opacity: 0.5; transform: translateY(-100%) scale(1); }
  }
  .b-modaldialog-shield		{ transition: background 0.15s ease-in; background: $b-style-modal-overlay; }
  .b-modaldialog-shield-leave	{ background: #0000; }
  .b-modaldialog-header {
    font-size: 1.5em; font-weight: bold;
    padding: 1rem 0 1.1rem;
    justify-content: space-evenly;
  }
  .b-modaldialog-header, .b-modaldialog-footer {
    width: 100%; align-items: center; text-align: center;
    background-color: darken($b-theme-background, 8%);
  }
  .b-modaldialog-footer {
    padding: 1.2rem 0 1rem;
    justify-content: space-evenly;
  }
  .b-modaldialog-body {
    padding: 1.5em 2em;
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
  <transition @after-leave="end_transitions"
	      @before-leave="intransition = shield && shield.toggle ('b-modaldialog-shield-leave')" >
    <div class="b-modaldialog" @click.stop ref='b-modaldialog' v-if='value' >

      <b-hflex class="b-modaldialog-header">
	<slot name="header">
	  Modal Dialog
	</slot>
      </b-hflex>
      <div class="b-modaldialog-body">
	<slot name="default"></slot>
      </div>
      <b-hflex class="b-modaldialog-footer">
	<slot name="footer">
	  <b-button class="b-modaldialog-button" ref="bclose" @click="close">
	    Close
	  </b-button>
	</slot>
      </b-hflex>

    </div>
  </transition>
</template>

<script>
export default {
  name: 'b-modaldialog',
  props:     { value: false, },
  data_tmpl: { focus_close: true, intransition: 0, },
  mounted () {
    this.update_shield();
    if (this.focus_close && this.$refs.bclose)
      {
	this.$refs.bclose.focus();
	this.focus_close = false;
      }
  },
  updated () {
    this.update_shield();
    if (this.focus_close && this.$refs.bclose)
      {
	this.$refs.bclose.focus();
	this.focus_close = false;
      }
    if (!this.value)
      this.focus_close = true; // re-focus next time
  },
  beforeDestroy () {
    if (this.shield)
      this.shield.destroy (false);
  },
  methods: {
    update_shield() {
      const modaldialog = this.$refs['b-modaldialog'];
      if (!modaldialog && this.shield && !this.intransition)
	{
	  this.shield.destroy (false);
	  this.shield = undefined;
	}
      if (modaldialog && !this.shield)
	this.shield = Util.modal_shield (this.$refs['b-modaldialog'], { class: 'b-modaldialog-shield',
									close: this.close });
    },
    end_transitions() {
      this.intransition = 0;
      this.update_shield();
    },
    close (event) {
      this.$emit ('input', false); // value = false
    },
  },
};
</script>
