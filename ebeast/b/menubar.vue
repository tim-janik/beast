<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-MENUBAR
  The main menu bar at the top of the window.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-menubar {
    margin: 5px;
    .-stack {
      display: inline-block;
      position: relative;
      vertical-align: middle;
      width: 1em; height: 1em;
      & :not(:first-child) { position: absolute; top: 0; left: 0; }
    }
  }
</style>

<template>

  <!-- main menu & controlbar -->
  <b-hflex class="b-menubar" space-between >

    <!-- menubar left -->
    <b-button-bar class="-menubar" >
      <!-- File Menu -->
      <b-button data-tip="**CLICK** File Menu" hotkey="Alt+KeyF" @click="Util.dropdown ($refs.filemenu, $event)" >
	<div class="-stack" >
	  <b-icon bc="folder" />
	  <b-icon bc="menumore" />
	</div>
	<b-contextmenu ref="filemenu" @click="activation" startfocus keepmounted >
	  <b-menuitem fa="file-o"	kbd="Ctrl+KeyN"	uri="loadnew" >	New Project		</b-menuitem>
	  <b-menuitem disabled fa="file-audio-o"	kbd="Ctrl+KeyO"	uri="load" >	Open Project…		</b-menuitem>
	  <b-menuitem disabled mi="save_alt"	kbd="Ctrl+KeyS"	uri="save" >	Save Project		</b-menuitem>
	  <b-menuitem disabled fa="save"	  kbd="Shift+Ctrl+KeyS" uri="save-as">	Save As…		</b-menuitem>
	  <b-menuseparator style="margin: 7px"  />
	  <b-menuitem fa="cog"         kbd="Ctrl+Comma"	uri="prefs">	Preferences		</b-menuitem>
	  <b-menuseparator style="margin: 7px"  />
	  <b-menuitem mi="close" kbd="Shift+Ctrl+KeyQ" uri="quit">	Quit			</b-menuitem>
	</b-contextmenu>
      </b-button>

      <!-- View Menu -->
      <b-button data-tip="**CLICK** View Menu" hotkey="Alt+KeyV" @click="Util.dropdown ($refs.viewmenu, $event)" >
	<div class="-stack" >
	  <b-icon fa="eye" />
	  <b-icon bc="menumore" />
	</div>
	<b-contextmenu ref="viewmenu" @click="activation" startfocus keepmounted >
	  <b-menuitem mi="fullscreen" :disabled="!document.fullscreenEnabled"
		      kbd="F11" uri="fullscreen">	Toggle Fullscreen	</b-menuitem>
	</b-contextmenu>
      </b-button>

    </b-button-bar>

    <!-- playcontrols -->
    <b-hflex>
      <b-playcontrols :project="project"> </b-playcontrols>
      <b-positionview :song="song"> </b-positionview>
    </b-hflex>

    <!-- menubar right -->
    <b-button-bar class="-menubar" >
      <!-- Help Menu -->
      <b-button data-tip="**CLICK** Help Menu" hotkey="Alt+KeyH"
		@click="Util.dropdown ($refs.helpmenu, $event)" >
	<div class="-stack" >
	  <b-icon fa="life-ring" />
	  <b-icon bc="menumore" />
	</div>
	<b-contextmenu ref="helpmenu" @click="activation" startfocus >
	  <b-menuitem mi="chrome_reader_mode"	uri="manual">	Beast Manual…		</b-menuitem>
	  <b-menuseparator style="margin: 7px"  />
	  <b-menuitem fa="id-card-o"		uri="about">	About…			</b-menuitem>
	</b-contextmenu>
      </b-button>
    </b-button-bar>

    <b-modaldialog v-model="discard_save" bwidth="11em" >
      <div slot="header">
	Save Project?
      </div>
      <b-icon fa="question-circle" style="font-size: 250%; padding-right: 1em; float: left; color: #538cc1" />
      The current project may contain unsaved changes. <br />
      Save changes to the project before closing?
      <b-icon fa="warning" style="display: none; font-size: 250%; padding-right: 1em; float: left; color: #d92" />
      <div slot="footer">
	<button @click="activation ('quit_discard')" >Discard Changes</button>
	<button autofocus @click="discard_save = false" >Cancel</button>
	<button >Save</button>
      </div>
    </b-modaldialog>

  </b-hflex>

</template>

<script>
export default {
  name: 'b-menubar',
  props: {
    song: { type: Bse.Song, },
    project: { type: Bse.Project, },
  },
  data: _ => ({ discard_save: false }),
  mounted() {
    this.$refs.filemenu.map_kbd_hotkeys (true);
    this.$refs.viewmenu.map_kbd_hotkeys (true);
  },
  methods: {
    activation (uri, event) {
      let u;
      switch (uri) {
	case 'quit_discard':
	  window.close();
	  break;
	case 'quit':
	  //d = document.querySelector ('dialog#testdialog');
	  //dialogPolyfill.registerDialog (d);
	  //d.showModal();
	  this.discard_save = true;
	  break;
	case 'about':
	  Shell.show_about_dialog = !Shell.show_about_dialog;
	  break;
	case 'manual':
	  u = new URL (window.location);
	  u = u.origin + '/doc/beast-manual.html';
	  window.open (u, '_blank');
	  break;
	case 'prefs':
	  Shell.show_preferences_dialog = !Shell.show_preferences_dialog;
	  break;
	case 'fullscreen':
	  if (document.fullscreen)
	    document.exitFullscreen();
	  else
	    document.body.requestFullscreen();
	  break;
	case 'loadnew':
	  Shell.load_project();
	  break;
      }
    },
  },
};
</script>
