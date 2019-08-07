<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-ABOUTDIALOG
  A [b-modaldialog] that displays version information about Beast.
  ## Events:
  *close*
  : A *close* event is emitted once the "Close" button activated.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .b-aboutdialog .b-modaldialog-container	{ max-width: 70em; }
  .b-aboutdialog table	{ table-layout: fixed; max-width: 100%; }
  .b-aboutdialog th	{
    text-align: right; vertical-align: top;
    padding-right: .5em; min-width: 15em; }
  .b-aboutdialog td	{
    max-width: 50%; overflow-wrap: break-word;
    display: inline-block; white-space: pre; }
</style>

<template>
  <b-modaldialog class="b-aboutdialog" @close="$emit ('close')">
    <div slot="header">About BEAST</div>
    <slot></slot>
    <table>
      <tr v-for="p in info_pairs" :key="p[0]" >
	<th>{{ p[0] }}</th>
	<td>{{ p[1] }}</td>
      </tr>
    </table>
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
    [ 'jQuery:',		jQuery.fn.jquery ],
    [ 'User Agent:',		user_agent ],
  ];
  if (window.Electron)
    {
      const Eapp = Electron.app;
      const os = require ('os');
      const parray = [
	[ 'Executable:',	Eapp.getPath ('exe') ],
	[ 'Revision:',		Eapp.getName() + ' ' + Eapp.getVersion() ],
	[ 'OS:',		process.platform + ' ' + process.arch + ' (' + os.release() + ')' ],
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
module.exports = {
  name: 'b-aboutdialog',
  data_tmpl: { info_pairs: [] },
  async created() {
    this.info_pairs = await about_pairs();
  },
};
</script>
