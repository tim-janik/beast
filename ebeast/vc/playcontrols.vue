<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-playcontrols - A container holding the play and seek controls for a song
  ### Props:
  - **project** - Injected, using vc-projectshell.project.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-playcontrols { }
</style>

<template>

  <vc-button-bar
      class="vc-playcontrols" >
    <vc-icon-button hotkey="L"     icon="fa-fast-backward" @click="pcall ('...Last')"></vc-icon-button>
    <vc-icon-button hotkey="B"     icon="fa-backward"      @click="pcall ('...Backwards')"></vc-icon-button>
    <vc-icon-button hotkey="Space" icon="fa-stop"          @click="pcall ('stop')"></vc-icon-button>
    <vc-icon-button hotkey="P"     icon="fa-play"          @click="pcall ('play')"></vc-icon-button>
    <vc-icon-button hotkey="R"     icon="fa-circle"        @click="pcall ('...Record')"></vc-icon-button>
    <vc-icon-button hotkey="F"     icon="fa-forward"       @click="pcall ('...Forwards')"></vc-icon-button>
    <vc-icon-button hotkey="N"     icon="fa-fast-forward"  @click="pcall ('...Next')"></vc-icon-button>
  </vc-button-bar>

</template>

<script>
module.exports = {
  name: 'vc-playcontrols',
  inject: [ 'vc-projectshell' ],
  computed: {
    project: function () { return this['vc-projectshell'].project; },
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
  },
};
</script>
