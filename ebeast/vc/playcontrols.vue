<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-playcontrols - A container holding the play and seek controls for a song
  ### Props:
  - **project** - The BSE object providing playback API.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-playcontrols { }
</style>

<template>

  <vc-button-bar
      class="vc-playcontrols" >
    <vc-icon-button hotkey="L"     icon="fa-fast-backward" @click="bclick ('...Last')"></vc-icon-button>
    <vc-icon-button hotkey="B"     icon="fa-backward"      @click="bclick ('...Backwards')"></vc-icon-button>
    <vc-icon-button hotkey="Space" icon="fa-stop"          @click="bclick ('stop')"></vc-icon-button>
    <vc-icon-button hotkey="P"     icon="fa-play"          @click="bclick ('play')"></vc-icon-button>
    <vc-icon-button hotkey="R"     icon="fa-circle"        @click="bclick ('...Record')"></vc-icon-button>
    <vc-icon-button hotkey="F"     icon="fa-forward"       @click="bclick ('...Forwards')"></vc-icon-button>
    <vc-icon-button hotkey="N"     icon="fa-fast-forward"  @click="bclick ('...Next')"></vc-icon-button>
  </vc-button-bar>

</template>

<script>
module.exports = {
  name: 'vc-playcontrols',
  props: [ 'project' ],
  methods:  {
    bclick (method, e) {
      let project = this.project, m = project[method], message;
      if (m !== undefined) {
	let result = m.call (project);
	if (result == undefined)
	  result = 'ok';
	message = method + ': ' + result;
      }
      else
	message = method + ': unimplemented';
      App.status (message);
    },
  },
};
</script>
