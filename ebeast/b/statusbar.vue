<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-STATUSBAR
  Area for the display of status messages and UI configuration.
</docs>

<style lang="scss">
  @import 'mixins.scss';

  .b-statusbar {
    display: flex; justify-content: space-between; white-space: nowrap;
    padding: 0 $b-statusbar-field-spacing; margin: 5px 0;
    user-select: none;
    .b-statusbar-field {
      display: flex; flex-wrap: nowrap; flex-shrink: 0; flex-grow: 0; white-space: nowrap;
    }
    .b-statusbar-text {
      display: inline-block; overflow-y: visible; //* avoid scrolling */
      overflow-x: hidden; white-space: nowrap;
      flex-shrink: 1; flex-grow: 1;
      margin-left: $b-statusbar-field-spacing * 2;
    }
    .b-statusbar-spacer {
      display: inline; flex-shrink: 1; flex-grow: 0; white-space: nowrap;
      text-align: center;
      margin: 0 $b-statusbar-field-spacing;
      @include b-statusbar-vseparator;
    }
    .b-icon {
      align-items: center;
      padding: 0 $b-statusbar-field-spacing;
      filter: brightness(asfactor($b-statusbar-icon-brightness));
      &:hover:not(.b-active) {
	filter: brightness(1 / asfactor($b-statusbar-icon-brightness));
	transform: scale($b-statusbar-icon-scaleup);
      }
      &.b-active {
	color: $b-color-active;
	transform: scale($b-statusbar-icon-scaleup);
      }
    }

    /* markdown styling for statusbar */
    .b-statusbar-text { /* .b-markdown-it-outer */
      @include b-markdown-it-inlined;
      color: $b-statusbar-text-shade;
      * {
	display: inline-block; overflow-y: visible; //* avoids scrolling */
	padding: 0; margin: 0; font-size: inherit; white-space: nowrap;
      }
      strong { color: $b-main-foreground; font-weight: normal; padding: 0 0.5em; }
      kbd {
	padding: 0 0.4em 1px;
	@include b-kbd-hotkey($border: true);
      }
    }
  }
</style>

<template>
  <b-hflex class="b-statusbar" >
    <span class="b-statusbar-field" >
      <b-icon fa="tasks" style="font-size:110%;" hflip :class="App.panel2 == 'd' && 'b-active'"
	      @click.native="App.switch_panel2 ('d')" data-kbd="^"
	      data-tip="**CLICK** Show Device Stack" />
      <b-icon mi="queue_music" style="font-size:140%" :class="App.panel2 == 'p' && 'b-active'"
	      @click.native="App.switch_panel2 ('p')"
	      data-kbd="^" data-tip="**CLICK** Show Piano Roll Editor" />
    </span>
    <span class="b-statusbar-spacer" />
    <span class="b-statusbar-text" ref="b-statusbar-text" />
    <span class="b-statusbar-text" v-if="!!kbd_" style="flex-grow: 0; margin: 0 0.5em;" >
      <strong>KEY</strong> <kbd>{{ kbd_ }}</kbd> </span>
    <span class="b-statusbar-spacer" />
    <span class="b-statusbar-field" >
      <b-icon fa="info-circle" style="font-size:120%" :class="App.panel3 == 'i' && 'b-active'"
	      @click.native="App.switch_panel3 ('i')" data-kbd="i"
	      data-tip="**CLICK** Show Information View" />
      <b-icon mi="folder_open" style="font-size:120%" :class="App.panel3 == 'b' && 'b-active'"
	      @click.native="App.switch_panel3 ('b')" data-kbd="i"
	      data-tip="**CLICK** Show Browser" />
    </span>
  </b-hflex>
</template>

<script>
export default {
  name: 'b-statusbar',
  data: () => ({ kbd_: '' }),
  mounted() {
    App.zmovehooks.push (this.seen_move);
  },
  destroyed() {
    Util.array_remove (App.zmovehooks, this.seen_move);
  },
  methods: {
    seen_move (ev) {
      if (!ev.buttons)
	{
          const els = document.body.querySelectorAll ('*:hover[data-tip]');
          const rawmsg = els.length ? els[els.length - 1].getAttribute ('data-tip') : '';
          if (rawmsg != this.status_)
            Util.markdown_to_html (this.$refs['b-statusbar-text'], this.status_ = rawmsg);
          const rawkbd = els.length ? els[0].getAttribute ('data-kbd') : '';
          if (rawkbd != this.kbd_)
            this.kbd_ = rawkbd;
	}
    },
  },
};
</script>
