<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PROJECTSHELL
  Shell for editing and display of a Bse.Project.
  ## Props:
  *project*
  : Implicit *Bse.Project*, using App.bse_project().
</docs>

<template>
  <b-vflex class="b-projectshell" style="width: 100%; height: 100%" >
    <b-hflex center style="margin: 5px">
      <b-playcontrols :project="project"> </b-playcontrols>
      <span style="width: 3em"><!-- spacer --></span>
      <b-positionview :song="song"> </b-positionview>
    </b-hflex>

    <b-hflex grow1 style="overflow-y: hidden">
      <b-vflex grow1 shrink1>
	<b-hflex class="b-projectshell-track-area" style="height: 50%">
	  <b-track-list class="grow1" :song="song"></b-track-list>
	</b-hflex>
	<b-hflex class="b-projectshell-part-area" style="height: 50%" >
	  <b-piano-roll class="grow1" :part="piano_roll_part" v-show="panel2 == 0" ></b-piano-roll>
	  <b-vflex v-show="panel2 == 1">
	    Device Panel
	    <b-devicepanel :track="current_track" />
	  </b-vflex>
	</b-hflex>
      </b-vflex>

      <b-hflex ref="sidebarcontainer" style="width:15%" >
	<div     style="flex-grow: 0; flex-shrink: 0" class="b-projectshell-resizer" @mousedown="sidebar_mouse" ></div>
	<b-vflex class="b-projectshell-sidebar" start shrink1 grow1 >
	  <b-treeselector></b-treeselector>
	</b-vflex>
      </b-hflex>
    </b-hflex>

    <b-aboutdialog v-model="show_about_dialog" />
    <b-preferencesdialog v-model="show_preferences_dialog" />
  </b-vflex>
</template>

<style lang="scss">
  @import 'mixins.scss';
  .b-projectshell {
    border: 5px solid #322;
    --b-resize-handle-thickness: #{$b-resize-handle-thickness};
    --b-transition-fast-slide: #{$b-transition-fast-slide};
  }
  .b-projectshell-part-area {
    background-color: $b-button-border;
    padding: $b-focus-outline-width; }
  .b-projectshell-sidebar {
    padding: 3px;
    overflow: hidden scroll;
    &, * { text-overflow: ellipsis; white-space: nowrap; }
  }
  .b-projectshell-resizer {
    width: var(--b-resize-handle-thickness);
    background: $b-resize-handle-bgcolor;
    border-left: $b-resize-handle-border;
    border-right: $b-resize-handle-border;
    cursor: col-resize;
  }
  html.b-projectshell-during-drag .b-root {
    .b-projectshell-resizer { background: $b-resize-handle-hvcolor; }
    * { cursor: col-resize !important; user-select: none !important; }
  }
</style>

<script>
function project_data () {
  const pdata = {
    // TODO: tracks: { getter: c => list_tracks.call (this), notify: n => this.song.on ("treechange", n), },
    // update current_track if tracks change
  };
  return this.observable_from_getters (pdata, () => this.project);
}

export default {
  name: 'b-projectshell',
  data() { return {
    project: undefined,
    pdata: project_data.call (this),
    current_track: undefined,
    piano_roll_part: undefined,
    show_about_dialog: false,
    show_preferences_dialog: false,
    song: undefined,
    notifynameclear: () => 0,
    panel2: 0,
  }; },
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
    console.assert (this === p.Shell);
    // provide default project
    this.load_project (Bse.server.last_project());
    // load_project() also forces an update with new Shell properties in place
  },
  mounted() {
    Util.add_hotkey ('Backquote', this.switch_panel2);
  },
  destroyed() {
    Util.remove_hotkey ('Backquote', this.switch_panel2);
    this.notifynameclear();
  },
  provide () { return { 'b-projectshell': this }; },
  methods: {
    switch_panel2() {
      this.panel2 = (this.panel2 + 1) % 2;
    },
    sidebar_mouse (e) {
      const sidebar = this.$refs.sidebarcontainer;
      console.assert (sidebar);
      const html_classes = document.documentElement.classList;
      if (e.type == 'mousedown' && !this.listening)
	{
	  this.listening = this.sidebar_mouse.bind (this);
	  document.addEventListener ('mousemove', this.listening);
	  document.addEventListener ('mouseup', this.listening);
	  this.startx = e.clientX; //  - e.offsetX;
	  this.startwidth = sidebar.getBoundingClientRect().width;
	  html_classes.add ('b-projectshell-during-drag');
	}
      if (this.listening && e.type == 'mouseup')
	{
	  document.removeEventListener ('mousemove', this.listening);
	  document.removeEventListener ('mouseup', this.listening);
	  this.listening = undefined;
	  html_classes.remove ('b-projectshell-during-drag');
	}
      let newwidth = this.startwidth - (e.clientX - this.startx);
      const pwidth = sidebar.parentElement.getBoundingClientRect().width;
      const maxwidth = pwidth * 0.6 |0, minwidth = 120;
      if (newwidth < minwidth / 2)
	{
	  const cs = getComputedStyle (sidebar);
	  newwidth = parseInt (cs.getPropertyValue ('--b-resize-handle-thickness'), 10);
	}
      else
	newwidth = Util.clamp (newwidth, minwidth, maxwidth);
      sidebar.style.transition = newwidth > minwidth ? "" : "width var(--b-transition-fast-slide)";
      const stylewidth = (newwidth / pwidth) * 100 + '%';
      if (stylewidth != sidebar.style.width)
	sidebar.style.width = stylewidth;
      e.preventDefault();
    },
    open_part_edit (part) {
      console.assert (part == undefined || part instanceof Bse.Part);
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
		{
		  const Path = require ('path');
		  Electron.dialog.showMessageBox (Electron.getCurrentWindow(),
						  {
						    type: 'error',
						    title: Util.format_title ('Beast', 'Failed to Load Project'),
						    message: 'Failed to load project from "' + Path.basename (project_or_path) + '"',
						    detail:  'Error during loading: ' + ret,
						    buttons: [ '&Dismiss', ],
						    cancelId: 1, defaultId: 1, normalizeAccessKeys: true,
						  },
						  (response, checked) => {
						    // nothing
						  });
		  return ret;
		}
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
	  track = await song.create_track ('Master');
	  track.ensure_output();
	}
      // shut down old project
      if (this.project)
	{
	  this.notifynameclear();
	  this.open_part_edit (undefined);
	  this.project.stop();
	  this.project = null; // TODO: should trigger FinalizationGroup
	}
      // reaplce project & song without await, to synchronously trigger Vue updates for both
      this.project = newproject;
      this.song = song;
      this.current_track = track;
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
