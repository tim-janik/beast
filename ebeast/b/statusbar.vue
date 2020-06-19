<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-STATUSBAR
  Area for the display of status messages and UI configuration.
</docs>

<style lang="scss">
  @import 'mixins.scss';

  $status-spacing: 0.5em;
  .b-statusbar {
    display: flex; justify-content: space-between; white-space: nowrap;
    padding: 0 $status-spacing; margin: 5px 0;
    user-select: none;
    .b-statusbar-field {
      display: flex; flex-wrap: nowrap; flex-shrink: 0; flex-grow: 0; white-space: nowrap;
    }
    .b-statusbar-text {
      display: inline; overflow: hidden; flex-shrink: 1; flex-grow: 1; white-space: nowrap;
      margin-left: $status-spacing * 2;
    }
    .b-statusbar-spacer {
      display: inline; flex-shrink: 1; flex-grow: 0; white-space: nowrap;
      text-align: center;
      margin: 0 $status-spacing;
      border-left: 1px solid darker($b-main-foreground, 50%);
      width: 1px;
    }
    .b-icon {
      align-items: center;
      padding: 0 $status-spacing;
      filter: brightness(0.8);
      &:hover {
	filter: brightness(1.2);
	transform: scale(1.1);
      }
    }

    // markdown styling for statusbar
    .b-statusbar-text {
      color: darker($b-main-foreground, 35%);
      * { padding: 0; margin: 0; font-size: inherit; }
      strong { color: $b-main-foreground; font-weight: normal; padding: 0 0.5em; }
    }
  }
</style>

<template>
  <b-hflex class="b-statusbar" >
    <span class="b-statusbar-field" >
      <b-icon fa="tasks" style="font-size:110%;" hflip data-tip="**CLICK** Show Devices" />
      <b-icon mi="queue_music" style="font-size:140%" data-tip="**CLICK** Show Piano Roll Editor" />
    </span>
    <span class="b-statusbar-spacer" />
    <span class="b-statusbar-text" ref="b-statusbar-text" />
    <span class="b-statusbar-spacer" />
    <span class="b-statusbar-field" >
      <b-icon fa="info-circle" style="font-size:120% ; color: #71cff2" data-tip="**CLICK** Show Information View" />
      <b-icon mi="folder_open" style="font-size:120%" data-tip="**CLICK** Show Browser" />
    </span>
  </b-hflex>
</template>

<script>
export default {
  name: 'b-statusbar',
  mounted() {
    App.onpointermoves.push (this.seen_move);
  },
  destroyed() {
    Util.array_remove (App.onpointermoves, this.seen_move);
  },
  methods: {
    seen_move (ev) {
      const coords = this.coords || { x: -1, y: -1 };
      this.coords = { x: ev.screenX, y: ev.screenY };
      if (ev.buttons || (coords.x == this.coords.x && coords.y == this.coords.y))
	return;
      const els = document.body.querySelectorAll ('*:hover[data-tip]');
      const rawmsg = els.length ? els[0].getAttribute ('data-tip') : '';
      if (rawmsg != this.status_)
	Util.markdown_to_html (this.$refs['b-statusbar-text'], this.status_ = rawmsg);
    },
  },
};
</script>
