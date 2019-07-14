<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-PLAYCONTROLS
  A container holding the play and seek controls for a Bse.song.
  ## Props:
  *project*
  : Injected *Bse.Project*, using `b-projectshell.project`.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-playcontrols {
    .b-button	{ padding: 5px; }
    .b-icon	{ fill: $b-button-foreground; }
  }
</style>

<template>

  <b-button-bar
      class="b-playcontrols" >
    <b-button hotkey="L"     @click="pcall ('...Last')"      ><b-icon fw lg fa="fast-backward" /></b-button>
    <b-button hotkey="B"     @click="pcall ('...Backwards')" ><b-icon fw lg fa="backward"      /></b-button>
    <b-button hotkey="Space" @click="pcall ('stop')"         ><b-icon fw lg fa="stop"          /></b-button>
    <b-button hotkey="P"     @click="pcall ('play')"         ><b-icon fw lg fa="play"          /></b-button>
    <b-button hotkey="R"     @click="pcall ('...Record')"    ><b-icon fw lg fa="circle"        /></b-button>
    <b-button hotkey="F"     @click="pcall ('...Forwards')"  ><b-icon fw lg fa="forward"       /></b-button>
    <b-button hotkey="N"     @click="pcall ('...Next')"      ><b-icon fw lg fa="fast-forward"  /></b-button>
  </b-button-bar>

</template>

<script>
module.exports = {
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
  },
};
</script>
