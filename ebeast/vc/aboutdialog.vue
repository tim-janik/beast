<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-ABOUTDIALOG
  A [vc-modaldialog] that displays version information about Beast.
  ## Events:
  *close*
  : A *close* event is emitted once the "Close" button activated.
</docs>

<style lang="scss">
  @import 'styles.scss';
  .vc-aboutdialog .vc-modaldialog-container	{ max-width: 70em; }
  .vc-aboutdialog table	{ table-layout: fixed; max-width: 100%; }
  .vc-aboutdialog th	{ text-align: right; padding-right: .5em; min-width: 15em; }
  .vc-aboutdialog td	{ max-width: 50%; overflow-wrap: break-word; }
</style>

<template>
  <vc-modaldialog class="vc-aboutdialog" @close="$emit ('close')">
    <div slot="header">About BEAST</div>
    <slot></slot>
    <table>
      <tr v-for="p in info_pairs" :key="p[0]" >
	<th>{{ p[0] }}</th>
	<td>{{ p[1] }}</td>
      </tr>
    </table>
  </vc-modaldialog>
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
  name: 'vc-aboutdialog',
  data: function() { return { info_pairs: about_pairs() }; },
  methods: {
    dummy (method, e) {
    },
  },
};
</script>
