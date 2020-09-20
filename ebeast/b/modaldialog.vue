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
    max-height: 99%; max-width: 99%;
    * { flex-shrink: 0; }
    @include b-flex-vbox; flex-shrink: 1;
    min-width: 16em; padding: 0;
    border: 2px solid $b-modal-bordercol;
    border-radius: 5px;
    background: $b-modal-background;
    color: $b-modal-foreground;
    padding: 1em;
    @include b-popup-box-shadow;
    /* fix vscrolling: https://stackoverflow.com/a/33455342 */
    justify-content: space-between;
    margin: auto;
    overflow: auto;
    &.v-enter-active 		{ transition: opacity 0.15s ease-out, transform 0.15s linear; }
    &.v-leave-active		{ transition: opacity 0.15s ease-in,  transform 0.15s linear; }
    &.v-enter, &.v-leave-to 	{ opacity: 0.15; transform: scale(0.15); }

    .b-button {
      @include b-singlebutton;
      margin: 0 1em;
      border-radius: 3px;
    }
    .-footer .b-button {
      &:first-child { margin-left: 1px; }
      &:last-child  { margin-right: 1px; }
    }
  }
  .b-modaldialog-shield		{ transition: background 0.15s ease-in; background: $b-style-modal-overlay; }
  .b-modaldialog-shield-leave	{ background: #0000; }
  .-header {
    font-size: 1.5em; font-weight: bold;
    justify-content: space-evenly;
    padding-bottom: 0.5em;
    border-bottom: 1px solid $b-modal-bordercol;
  }
  .-header, .-footer {
    align-self: stretch;
    align-items: center; text-align: center;
  }
  .-footer {
    justify-content: space-evenly;
    padding-top: 1em;
    border-top: 1px solid $b-modal-bordercol;
    &.-empty { display: none; }
  }
  .-body {
    padding: 1em 1em;
    align-self: stretch;
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
    <div class="b-modaldialog" @click.stop ref='modaldialog' v-if='value' >

      <b-hflex class="-header">
	<slot name="header">
	  Modal Dialog
	</slot>
      </b-hflex>

      <b-vflex class="-body">
	<slot name="default"></slot>
      </b-vflex>

      <b-hflex class="-footer" :class="footerclass" ref="footer">
	<slot name="footer"/>
      </b-hflex>

    </div>
  </transition>
</template>

<script>
export default {
  name: 'b-modaldialog',
  props:     { value: { type: Boolean },
	       exclusive: { type: Boolean },
	       bwidth: { default: '9em' }, },
  data_tmpl: { intransition: 0, footerclass: '', },
  beforeDestroy () {
    if (this.shield)
      this.shield.destroy (false);
  },
  methods: {
    dom_hidden() {
      this.update_shield();
      this.re_autofocus = true;
    },
    dom_update() {
      this.update_shield();
      this.footerclass = this.$refs.footer && this.$refs.footer.innerHTML ? '' : '-empty';
      if (this.value && this.re_autofocus)
	{
	  const e = document.querySelector ('[autofocus]:not([disabled]):not([display="none"])');
	  e?.focus();
	  this.re_autofocus = false;
	}
      if (this.value)
	{
	  const sel = Util.vm_scope_selector (this);
	  const css = [];
	  if (this.bwidth)
	    css.push (`${sel}.b-modaldialog .-footer .b-button { min-width: ${this.bwidth}; }\n`);
	  Util.vm_attach_style (this, css.join ('\n'));
	}
    },
    update_shield() {
      const modaldialog = this.$refs.modaldialog;
      if (!modaldialog && this.shield && !this.intransition)
	{
	  this.shield.destroy (false);
	  this.shield = undefined;
	}
      if (modaldialog && !this.shield)
	this.shield = Util.modal_shield (this.$refs.modaldialog, { class: 'b-modaldialog-shield',
								   exclusive: this.exclusive,
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
