<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-CHOICE
  This element provides a choice popup to choose from a set of options.
  It supports the Vue
  [v-model](https://vuejs.org/v2/guide/components-custom-events.html#Customizing-Component-v-model)
  protocol by emitting an `input` event on value changes and accepting inputs via the `value` prop.
  ## Props:
  *value*
  : Integer, the index of the choice value to be displayed.
  *choices*
  : List of choices: `[ { icon, label, subject }... ]`
  ## Events:
  *input (value)*
  : Value change notification event, the first argument is the new value.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-choice {
    display: flex; position: relative;
    margin: 0; padding: 0; text-align: center;
    user-select: none;
  }
  .b-choice-label {
    //* flex-grow: 1; */
    white-space: nowrap; overflow: hidden;
    height: 1.33em;
    width: 2.2em;
    padding: 0 2px;
    align-self: center;
    border-radius: 3px;
    box-shadow: 0 0 3px #00000077;
    background-color: $b-choice-0-bg;
    background: linear-gradient(177deg, $b-choice-0-bh, $b-choice-0-bl 20%, $b-choice-0-bd);
    .b-choice-press &  	{
      filter: brightness(90%);
      background: linear-gradient(-3deg, $b-choice-0-bh, $b-choice-0-bl 20%, $b-choice-0-bd);
    }
  }
  .b-choice-label:focus { outline: 0.5px dashed #00adb3; }
</style>

<!-- NOTE: This implementation assumes the HTML embeds echoice.svg -->

<template>
  <div    class="b-choice" ref="bchoice" :style="style (1)"
	  data-tip="**CLICK** Choice Value" >
    <div  class="b-choice-label" :data-tip="bubbletip()" ref="label" @pointerdown="pointerdown"
	  @keydown="keydown" tabindex="0" >
      {{ nick() }}
      <b-contextmenu ref="cmenu" @click="menuactivation" @close="menugone" >
	<b-menuitem v-for="(e, i) in choices" :uri="i" :key="i" >
	  <b>{{ e.label }}</b>
	  <span v-if="e.subject" >{{ e.subject }}</span>
	</b-menuitem>
      </b-contextmenu>
    </div>
  </div>
</template>

<script>
export default {
  name: 'b-choice',
  props: { value:   { default: false },
	   choices: { default: [] }, },
  data: () => ({
    value_: 0,
    buttondown_: false,
  }),
  beforeDestroy () {
    window.removeEventListener ('pointerup', this.pointerup);
  },
  methods: {
    style (div = 0) {
      return ""; // FIXME
    },
    dom_update() {
      if (!this.$el) // we need a second Vue.render() call for DOM alterations
	return this.$forceUpdate();
      this.value_ = this.value;
      if (this.value_)
	{
	  this.$el.classList.add ('b-choice-on');
	  this.$el.classList.remove ('b-choice-off');
	}
      else
	{
	  this.$el.classList.remove ('b-choice-on');
	  this.$el.classList.add ('b-choice-off');
	}
    },
    bubbletip() {
      const n = this.value >>> 0;
      if (!this.choices || n >= this.choices.length)
	return "";
      const c = this.choices[n];
      let tip = "**" + c.label + "**";
      if (c.subject)
	tip += " " + c.subject;
      return tip;
    },
    nick() {
      const n = this.value >>> 0;
      if (!this.choices || n >= this.choices.length)
	return "";
      const c = this.choices[n];
      if (!c.label)
	return '';
      let nick = c.label;
      if (nick.length > 4)
	nick = nick.substr (0, 4) + '…';
      return nick;
    },
    menuactivation (uri) {
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      debug("emit", uri >>> 0);
      this.$emit ('input', uri >>> 0);
    },
    pointerdown (ev) {
      this.$refs.label.focus();
      // trigger only on primary button press
      if (!this.buttondown_ && ev.buttons == 1)
	{
	  this.buttondown_ = true;
	  this.$el.classList.add ('b-choice-press');
	  ev.preventDefault();
	  ev.stopPropagation();
	  this.$refs.cmenu.popup (event, { origin: event.target });
	}
    },
    menugone () {
      this.$el.classList.remove ('b-choice-press');
      this.buttondown_ = false;
    },
    keydown (ev) {
      if (event.keyCode == Util.KeyCode.DOWN || event.keyCode == Util.KeyCode.UP)
	{
	  ev.preventDefault();
	  ev.stopPropagation();
	  const n = this.value >>> 0;
	  if (this.choices)
	    {
	      const v = n + (event.keyCode == Util.KeyCode.DOWN ? +1 : -1);
	      if (v >= 0 && v < this.choices.length)
		this.$emit ('input', v);
	    }
	}
      else if (event.keyCode == Util.KeyCode.ENTER)
	{
	  ev.preventDefault();
	  ev.stopPropagation();
	  this.$refs.cmenu.popup (event, { origin: event.target });
	}
    },
  },
};

</script>
