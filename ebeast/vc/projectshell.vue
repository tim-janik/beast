<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-projectshell - Shell for editing and display of a Bse.Project.
  ### Props:
  - **project** - Implicit, using App.bse_project().
</docs>

<template>
  <div class="vc-projectshell">
    <div class="vc-projectshell-track-area">
      Project Shell. <br />
      <vc-playcontrols :project="project"> </vc-playcontrols>
      <br />
      {{ message }}
      <br />
      <vc-button @click="App.show_about_dialog = true">About</vc-button>
      <vc-button @click="frob (1, $event)">1</vc-button>
      <vc-button @click="frob (2, $event)">2</vc-button>
      <vc-button @click="frob (3, $event)">3</vc-button>
      <br />
      <vc-track-list :song="song"></vc-track-list>
    </div>
    <div class="vc-projectshell-part-area">
      <vc-piano-roll></vc-piano-roll>
    </div>
  </div>
</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-projectshell { }
  .vc-projectshell-part-area {
    white-space: nowrap; overflow-h: hidden; overflow-y: scroll;
    background-color: $vc-button-border;
    height: 350px;
    border: 1px solid red; }
</style>

<script>
module.exports = {
  name: 'vc-projectshell',
  provide () { return { 'vc-projectshell': this }; },
  data: function() {
    let d = {
      message: 'Interpolated text',
    };
    for (const attrname in Electron.app)
      d[attrname] = Electron.app[attrname];
    return d;
  },
  methods: {
    frob (w, e) { App.status (w); },
  },
  computed: {
    project: function () {
      const bse_project = App.bse_project();
      if (!(bse_project instanceof Bse.Project))
	throw new Error ('wrong type, expected: Bse.Project');
      return bse_project;
    },
    song: function () {
      let s, supers = this.project.get_supers();
      for (s of supers) {
	if (s instanceof Bse.Song)
	  return s;
      }
      return undefined;
    },
  },
};
</script>
