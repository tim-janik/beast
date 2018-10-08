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
    <b-playcontrols :project="project"> </b-playcontrols>
    <b-hflex>
      <b-vflex style="flex-grow: 1">
	<div class="b-projectshell-track-area">
	  <b-track-list :song="song"></b-track-list>
	</div>
	<b-hflex class="b-projectshell-part-area" style="overflow: hidden;" >
	  <b-piano-roll :part="piano_roll_part" ></b-piano-roll>
	</b-hflex>
      </b-vflex>
      <b-vflex style="justify-content: flex-start" >
	<b-treeselector @close="show_tree_selector = false"></b-treeselector>
      </b-vflex>
    </b-hflex>
    <b-aboutdialog v-if="show_about_dialog" @close="show_about_dialog = false"></b-aboutdialog>
    <b-preferencesdialog v-if="show_preferences_dialog" @close="show_preferences_dialog = false"></b-preferencesdialog>
  </div>
</template>

<style lang="scss">
  @import 'styles.scss';
  .b-projectshell { }
  .b-projectshell-part-area {
    background-color: $b-button-border;
    height: 350px; padding: $b-focus-outline-width; }
</style>

<script>
module.exports = {
  name: 'b-projectshell',
  data_tmpl: {
    piano_roll_part: undefined,
    show_about_dialog: false,
    show_preferences_dialog: false,
    project: undefined,
    song: undefined,
    notifynameclear: () => 0,
  },
  watch: {
    show_about_dialog:       function (newval) { if (newval && this.show_preferences_dialog) this.show_preferences_dialog = false; },
    show_preferences_dialog: function (newval) { if (newval && this.show_about_dialog) this.show_about_dialog = false; },
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
    this.load_project (Bse.server.last_project());
    // load_project() also forces an update with new Shell properties in place
  },
  destroyed() {
    this.notifynameclear();
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
    async load_project (project_or_path)
    {
      project_or_path = await project_or_path;
      // always replace the existing project with a new one
      let newproject = project_or_path instanceof Bse.Project ? project_or_path : null;
      if (!newproject)
	{
	  newproject = await Bse.server.create_project ('Untitled');
	  // load from disk if possible
	  if (project_or_path)
	    {
	      const ret = await newproject.restore_from_file (project_or_path);
	      if (ret != Bse.Error.NONE)
		return ret;
	      const basename = project_or_path.replace (/.*\//, '');
	      await newproject.set_name (basename);
	    }
	}
      // ensure project has a song
      let song, supers = await newproject.get_supers();
      for (let s of supers)
	if (s instanceof Bse.Song)
	  {
	    song = s;
	    break;
	  }
      if (!song)
	song = await newproject.create_song ("Unnamed");
      song.ensure_master_bus();
      let track;
      for (let t of await song.list_children())
	if (t instanceof Bse.Track)
	  {
	    track = t;
	    break;
	  }
      if (!track)
	{
	  const track = await song.create_track ('Master');
	  track.ensure_output();
	}
      // shut down old project
      if (this.project)
	{
	  this.notifynameclear();
	  this.open_part_edit (undefined);
	  this.project.stop();
	  Bse.server.destroy_project (this.project);
	}
      // reaplce project & song without await, to synchronously trigger Vue updates for both
      this.project = newproject;
      this.song = song;
      const update_title = async () => {
	const name = this.project ? await this.project.get_name_or_type() : undefined;
	document.title = Util.format_title ('Beast', name);
      };
      this.notifynameclear = this.project.on ("notify:uname", update_title);
      update_title();
      this.$forceUpdate();
      return Bse.Error.NONE;
    },
    async save_project (projectpath)
    {
      let ret = Bse.Error.INTERNAL;
      if (this.project)
	{
	  const self_contained = true;
	  ret = await this.project.store (projectpath, self_contained);
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
