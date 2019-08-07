<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-PROJECTSHELL
  Shell for editing and display of a Bse.Project.
  ## Props:
  *project*
  : Implicit *Bse.Project*, using App.bse_project().
</docs>

<template>
  <div class="b-projectshell">
    <div class="b-projectshell-track-area">
      Project Shell:
      <b-playcontrols :project="project"> </b-playcontrols>
      <b-track-list :song="song"></b-track-list>
    </div>
    <div class="b-projectshell-part-area" style="display: flex; overflow: hidden;" >
      <b-piano-roll :piano-part="piano_roll_part" ></b-piano-roll>
    </div>
  </div>
</template>

<style lang="scss">
  @import 'styles.scss';
  .b-projectshell { }
  .b-projectshell-part-area {
    background-color: $b-button-border;
    height: 350px; }
</style>

<script>
module.exports = {
  name: 'b-projectshell',
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
  provide () { return { 'b-projectshell': this }; },
  methods: {
    open_part_edit (part) {
      assert (part == undefined || part instanceof Bse.Part);
      this.piano_roll_part = part;
    },
    status (...args) {
      console.log (...args);
    },
    load_project (projectpath)
    {
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
	  Bse.server.destroy_project (this.project);
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
    save_project (projectpath)
    {
      let ret = Bse.Error.INTERNAL;
      if (this.project)
	{
	  const self_contained = true;
	  ret = this.project.store (projectpath, self_contained);
	  const Path = require ('path');
	  if (ret != Bse.Error.NONE)
	    Electron.dialog.showMessageBox (Electron.getCurrentWindow(),
					    {
					      type: 'error',
					      title: Util.format_title ('Beast', 'Failed to Save Project'),
					      message: 'Failed to save project as "' + Path.basename (projectpath) + '"',
					      detail:  'Error during saving: ' + ret,
					      buttons: [ '&Dismiss', ],
					      cancelId: 1, defaultId: 1, normalizeAccessKeys: true,
					    },
					    (response, checked) => {
					      // nothing
					    });
	}
      return ret;
    },
  },
};
</script>