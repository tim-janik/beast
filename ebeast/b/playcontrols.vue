<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-PLAYCONTROLS
  A container holding the play and seek controls for a Bse.song.
  ## Props:
  *project*
  : Injected *Bse.Project*, using `vc-projectshell.project`.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .vc-playcontrols {
    .vc-button	{ padding: 5px; }
    .vc-icon	{ fill: $vc-button-foreground; }
  }
</style>

<template>

  <vc-button-bar
      class="vc-playcontrols" >
    <vc-button hotkey="L"     @click="pcall ('...Last')"      ><vc-icon fw lg fa="fast-backward" /></vc-button>
    <vc-button hotkey="B"     @click="pcall ('...Backwards')" ><vc-icon fw lg fa="backward"      /></vc-button>
    <vc-button hotkey="Space" @click="pcall ('stop')"         ><vc-icon fw lg fa="stop"          /></vc-button>
    <vc-button hotkey="P"     @click="pcall ('play')"         ><vc-icon fw lg fa="play"          /></vc-button>
    <vc-button hotkey="R"     @click="pcall ('...Record')"    ><vc-icon fw lg fa="circle"        /></vc-button>
    <vc-button hotkey="F"     @click="pcall ('...Forwards')"  ><vc-icon fw lg fa="forward"       /></vc-button>
    <vc-button hotkey="N"     @click="pcall ('...Next')"      ><vc-icon fw lg fa="fast-forward"  /></vc-button>
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
