<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-projectshell - Shell for editing and display of a Bse.Project.
  ### Props:
  - **project** - Implicit, using App.bse_project().
</docs>

<template>
  <div class="vc-projectshell">
    <div class="vc-projectshell-track-area">
      Project Shell:
      <vc-playcontrols :project="project"> </vc-playcontrols>
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
  methods: {
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
