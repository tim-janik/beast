<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PLAYCONTROLS
  A container holding the play and seek controls for a Bse.song.
  ## Props:
  *project*
  : Injected *Bse.Project*, using `b-projectshell.project`.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-playcontrols {
    .b-button	{ padding: 5px; }
  }
</style>

<template>

  <b-button-bar
      class="b-playcontrols" >
    <b-button disabled @click="pcall ('...Last')"      ><b-icon fw lg fa="fast-backward" /></b-button>
    <b-button disabled @click="pcall ('...Backwards')" ><b-icon fw lg fa="backward"      /></b-button>
    <b-button @click="pcall ('stop')"		hotkey="KeyS"
	      data-tip="**CLICK** Stop playback" >		<b-icon fw lg fa="stop"          /></b-button>
    <b-button @click="toggle_play()"		hotkey="Space"
	      data-tip="**CLICK** Start/stop playback" >	<b-icon fw lg fa="play"          /></b-button>
    <b-button disabled @click="pcall ('...Record')"    ><b-icon fw lg fa="circle"        /></b-button>
    <b-button disabled @click="pcall ('...Forwards')"  ><b-icon fw lg fa="forward"       /></b-button>
    <b-button disabled @click="pcall ('...Next')"      ><b-icon fw lg fa="fast-forward"  /></b-button>
  </b-button-bar>

</template>

<script>
export default {
  name: 'b-playcontrols',
  inject: [ 'b-projectshell' ],
  computed: {
    project: function () { return this['b-projectshell'].project; },
  },
  methods:  {
    pcall (method, e) {
      let project = this.project, m = project[method], message;
      if (m !== undefined) {
	let result = m.call (project);
	if (result == undefined)
	  result = 'ok';
	message = method + ': ' + result;
      }
      else
	message = method + ': unimplemented';
      Shell.status (message);
    },
    async toggle_play() {
      const project = this.project;
      if (!await this.project.is_playing())
	project.play();
      else
	project.stop();
    },
  },
};
</script>
