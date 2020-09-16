<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-ABOUTDIALOG
  A [b-modaldialog] that displays version information about Beast.
  ## Events:
  *close*
  : A *close* event is emitted once the "Close" button activated.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-aboutdialog .b-grid {
    max-width: 100%;
    & > * { //* avoid visible overflow for worst-case resizing */
      overflow-wrap: break-word;
      min-width: 0; }
  }
  .b-aboutdialog .b-aboutdialog-header {
    grid-column: 1;
    text-align: right; vertical-align: top; font-weight: bold;
    padding-right: .5em; min-width: 15em; }
  .b-aboutdialog .b-aboutdialog-field {
    grid-column: 2;
    overflow-wrap: break-word;
    display: inline-block; white-space: pre-wrap; }
</style>

<template>
  <b-modaldialog class="b-aboutdialog"
		 :value="value" @input="$emit ('input', $event)" >
    <div slot="header">About BEAST</div>
    <slot></slot>
    <b-grid>
      <template v-for="p in info_pairs">
	<span class="b-aboutdialog-header" :key="'h' + p[0]" >{{ p[0] }}</span>
	<span class="b-aboutdialog-field"  :key="'f' + p[0]" >{{ p[1] }}</span>
      </template>
    </b-grid>
    <div slot="footer"><b-button autofocus canfocus onclick="Shell.show_about_dialog = false" > Close </b-button></div>
  </b-modaldialog>
</template>

<script>
async function about_pairs() {
  const user_agent = navigator.userAgent.replace (/([)0-9]) ([A-Z])/gi, '$1\n$2');
  let array = [
    [ 'Beast:',			CONFIG.revision + ' (' + CONFIG.revdate.split (' ')[0] + ')' ],
    [ 'BSE:',			await Bse.server.get_version() ],
    [ 'Vorbis:',		await Bse.server.get_vorbis_version() ],
    [ 'Vuejs:',			Vue.version ],
    [ 'User Agent:',		user_agent ],
  ];
  if (window.Electron)
    {
      const Eapp = Electron.app;
      const os = require ('os');
      const parray = [
	[ 'OS:',		process.platform + ' ' + process.arch + ' (' + os.release() + ')' ],
	[ 'Executable:',	Eapp.getPath ('exe') ],
	[ 'Application:',	Eapp.getName() + ' ' + Eapp.getVersion() ],
	[ 'Electron:',		process.versions.electron ],
	[ 'Chrome:',		process.versions.chrome ],
	[ 'Node.js:',		process.versions.node ],
	[ 'Libuv:',		process.versions.uv ],
	[ 'V8:',		process.versions.v8 ],
	[ 'Working Dir:',	process.cwd() ],
	[ 'Desktop Dir:',	Eapp.getPath ('desktop') ],
	[ 'Config Path:',	Eapp.getPath ('userData') ],
	[ 'Music Path:',	Eapp.getPath ('music') ],
      ];
      array = Array.prototype.concat (array, parray);
    }
  return array;
}

export default {
  name: 'b-aboutdialog',
  props: {
    value: { default: false },
  },
  data_tmpl: { info_pairs: [] },
  async created() {
    this.info_pairs = await about_pairs();
  },
};
</script>
