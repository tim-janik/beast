<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-PROJECTSHELL
  Shell for editing and display of a Bse.Project.
  ## Props:
  *project*
  : Implicit *Bse.Project*, using App.bse_project().
</docs>

<template>
  <div class="vc-projectshell">
    <div class="vc-projectshell-track-area">
      Project Shell:
      <vc-playcontrols :project="project"> </vc-playcontrols>
      <vc-track-list :song="song"></vc-track-list>
    </div>
    <div class="vc-projectshell-part-area" style="display: flex; overflow: hidden;" >
      <vc-piano-roll :piano-part="piano_roll_part" ></vc-piano-roll>
    </div>
  </div>
</template>

<style lang="scss">
  @import 'styles.scss';
  .vc-projectshell { }
  .vc-projectshell-part-area {
    background-color: $vc-button-border;
    height: 350px; }
</style>

<script>
module.exports = {
  name: 'vc-projectshell',
  data_tmpl: {
    piano_roll_part: undefined,
    show_about_dialog: false,
    show_preferences_dialog: false,
    project: undefined,
  },
  watch: {
    show_about_dialog:       function (newval) { if (newval && this.show_preferences_dialog) this.show_preferences_dialog = false; },
    show_preferences_dialog: function (newval) { if (newval && this.show_about_dialog) this.show_about_dialog = false; },
  },
  computed: {
    song: function () {
      let s, supers = this.project.get_supers();
      for (s of supers) {
	if (s instanceof Bse.Song)
	  return s;
      }
      return undefined;
    },
  },
  created() {
    // delete `Shell` dummy on toplebel if present
    let p = this;
    while (p.$parent)
      p = p.$parent;
    if (p.Shell && p.Shell.Shell_placeholder)
      delete p.Shell;
    // inject Shell into all Vue components
    Vue.prototype.Shell = this;
    assert (this === p.Shell);
    // provide default project
    this.load_project();
    // load_project() also forces an update with new Shell properties in place
  },
  provide () { return { 'vc-projectshell': this }; },
  methods: {
    open_part_edit (part) {
      assert (part == undefined || part instanceof Bse.Part);
      this.piano_roll_part = part;
    },
    status (...args) {
      console.log (...args);
    },
    load_project (projectpath) {
      let newproject = Bse.server.create_project ('Untitled');
      if (projectpath != undefined)
	{
	  const ret = newproject.restore_from_file (projectpath);
	  if (ret != Bse.Error.NONE)
	    return ret;
	  const path = require ('path');
	  newproject.set_name (path.basename (projectpath));
	}
      if (this.project)
	{
	  this.project.stop();
	  this.open_part_edit (undefined);
	  this.title_off_();
	}
      this.project = newproject;
      const update_title = () => {
	const name = this.project ? this.project.get_name_or_type() : undefined;
	document.title = Util.format_title ('Beast', name);
      };
      this.title_off_ = this.project.on ("notify:uname", update_title);
      update_title();
      this.$forceUpdate();
      return Bse.Error.NONE;
    },
  },
};
</script>
