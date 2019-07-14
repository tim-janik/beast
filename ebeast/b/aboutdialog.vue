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
  .b-aboutdialog th	{ text-align: right; padding-right: .5em; min-width: 15em; }
  .b-aboutdialog td	{ max-width: 50%; overflow-wrap: break-word; }
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
function about_pairs() {
  const elap = Electron.app;
  const os = require ('os');
  return [
    [ 'Application:',	elap.getName() + ' ' + elap.getVersion() ],
    [ 'Revision:',	Bse.server.get_version_buildid() + ' (' + Bse.server.get_version_date().split (' ')[0] + ')' ],
    [ 'OS:',		process.platform + ' ' + process.arch + ' (' + os.release() + ')' ],
    [ 'Electron:',	process.versions.electron ],
    [ 'Chrome:',	process.versions.chrome ],
    [ 'V8:',		process.versions.v8 ],
    [ 'Node.js:',	process.versions.node ],
    [ 'Vorbis:',	Bse.server.get_vorbis_version() ],
    [ 'Libuv:',		process.versions.uv ],
    [ 'Vuejs:',		Vue.version ],
    [ 'jQuery:',	jQuery.fn.jquery ],
    [ 'User Agent:',	navigator.userAgent ],
    [ 'Executable:',	elap.getPath ('exe') ],
    [ 'Working Dir:',	process.cwd() ],
    [ 'Desktop Dir:',	elap.getPath ('desktop') ],
    [ 'Config Path:',	elap.getPath ('userData') ],
    [ 'Music Path:',	elap.getPath ('music') ],
  ];
}
module.exports = {
  name: 'b-aboutdialog',
  data: function() { return { info_pairs: about_pairs() }; },
  methods: {
    dummy (method, e) {
    },
  },
};
</script>
