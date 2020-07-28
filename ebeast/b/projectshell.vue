<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PROJECTSHELL
  Shell for editing and display of a Bse.Project.
  ## Props:
  *project*
  : Implicit *Bse.Project*, using App.bse_project().
</docs>

<style lang="scss">
  @import 'mixins.scss';

  .b-projectshell {
    --b-resize-handle-thickness: #{$b-resize-handle-thickness};
    --b-transition-fast-slide: #{$b-transition-fast-slide};
    width: 100%;
    height: 100%;
    justify-content: space-between;
    align-items: stretch;
  }
  .b-projectshell-panel1 {
    @include b-panel-box;
    flex-grow: 1;
  }
  .b-projectshell-panel2 {
    @include b-panel-box;
    height: 19em;
    flex-shrink: 0;
  }
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

<template>
  <b-vflex class="b-projectshell" >

    <!-- play controls, time display -->
    <b-hflex center style="margin: 5px">
      <b-playcontrols :project="project"> </b-playcontrols>
      <span style="width: 3em"><!-- spacer --></span>
      <b-positionview :song="song"> </b-positionview>
    </b-hflex>

    <b-hflex style="overflow: hidden; flex: 1 1 auto">
      <b-vflex grow1 shrink1>
	<!-- upper main area -->
	<b-hflex class="b-projectshell-panel1" >
	  <b-track-list class="grow1" :song="song"></b-track-list>
	</b-hflex>
	<!-- lower main area -->
	<b-hflex class="b-projectshell-panel2" >
	  <b-piano-roll class="grow1" :msrc="App.piano_roll_source" v-show="App.panel2 == 'p'" ></b-piano-roll>
	  <b-devicepanel v-show="App.panel2 == 'd'" :track="current_track" />
	</b-hflex>
      </b-vflex>

      <!-- browser -->
      <b-hflex ref="sidebarcontainer" style="width: 0; flex: 0 0 15%" >
	<div     style="flex-grow: 0; flex-shrink: 0" class="b-projectshell-resizer" @mousedown="sidebar_mouse" ></div>
	<b-vflex class="b-projectshell-sidebar" start shrink1 grow1 >
	  <b-treeselector :tree="o.filetree" v-show="App.panel3 == 'b'" ></b-treeselector>
	  <span v-show="App.panel3 == 'i'" >Info Panel</span>
	</b-vflex>
      </b-hflex>
    </b-hflex>

    <!-- status bar -->
    <b-statusbar />

    <!-- popup dialogs -->
    <b-aboutdialog v-model="show_about_dialog" />
    <b-preferencesdialog v-model="show_preferences_dialog" />

  </b-vflex>
</template>

<script>
async function list_sample_files() {
  const crawler = await Bse.server.resource_crawler();
  const entries = await crawler.list_files ('wave', 'user-downloads');
  return Object.freeze ({ entries: entries });
}

function observable_project_data () { // yields reactive Proxy object
  const data = {
    filetree:	     { default: {}, getter: c => list_sample_files(), },
    usermessagehook: { notify: n => Bse.server.on ("usermessage", this.usermessage), },
    // TODO: tracks: { getter: c => list_tracks.call (this), notify: n => this.song.on ("treechange", n), },
    // update current_track if tracks change
    __update__: Util.observable_force_update,
  };
  return this.observable_from_getters (data, () => this.project);
}

export default {
  name: 'b-projectshell',
  data() { return {
    project: undefined,
    o: observable_project_data.call (this),
    current_track: undefined,
    show_about_dialog: false,
    show_preferences_dialog: false,
    song: undefined,
    notifynameclear: () => 0,
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
    this.switch_panel2 = App.switch_panel2.bind (App);
    Util.add_hotkey ('Backquote', this.switch_panel2);
    this.switch_panel3 = App.switch_panel3.bind (App);
    Util.add_hotkey ('KeyI', this.switch_panel3);
  },
  destroyed() {
    Util.remove_hotkey ('Backquote', this.switch_panel2);
    Util.remove_hotkey ('KeyI', this.switch_panel3);
    this.notifynameclear();
  },
  provide () { return { 'b-projectshell': this }; },
  methods: {
    usermessage (e) {
      let msg = '';
      switch (e.umtype)
      {
	case Bse.UserMessageType.ERROR:		msg += '### '; break;
	case Bse.UserMessageType.WARNING:	msg += '### '; break;
	case Bse.UserMessageType.INFO:		msg += '# '; break;
	case Bse.UserMessageType.DEBUG:		msg += '##### '; break;
      }
      msg += e.title + '\n';
      msg += e.text1 + '\n';
      msg += e.text2 + '\n';
      if (e.text3)
	msg += '  \n  \n**' + e.text3 + '**\n';
      Util.show_note (msg);
    },
    sidebar_mouse (e) {
      const sidebar = this.$refs.sidebarcontainer;
      console.assert (sidebar);
      const html_classes = document.documentElement.classList;
      if (e.type == 'mousedown' && !this.listening)
	{
	  this.listening = Util.debounce (this.sidebar_mouse.bind (this));
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
      const flexwidth = '0 0 ' + (newwidth / pwidth) * 100 + '%';
      if (flexwidth != sidebar.style.flex)
	sidebar.style.flex = flexwidth;
      // Resize via: https://www.w3.org/TR/css-flexbox-1/#flex-common
      e.preventDefault();
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
      newproject.auto_stop (false);
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
	  App.open_piano_roll (undefined);
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
      this.o.__update__();
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
