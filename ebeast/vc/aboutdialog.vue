<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-aboutdialog - Modal dialog listing version informatoin about Beast
  ### Events:
  - **close** - A *close* event is emitted once the "Close" button activated.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-aboutdialog .vc-modaldialog-container	{ max-width: 70em; }
  .vc-aboutdialog table	{ table-layout: fixed; max-width: 100%; }
  .vc-aboutdialog th	{ text-align: right; padding-right: .5em; min-width: 15em; }
  .vc-aboutdialog td	{ font-family: monospace; max-width: 50%; overflow-wrap: break-word; }
</style>

<template>
  <vc-modaldialog class="vc-aboutdialog" @close="$emit ('close')">
    <div slot="header">About BEAST</div>
    <slot></slot>
    <table>
      <tr v-for="p in info_pairs">
	<th>{{ p[0] }}</th>
	<td>{{ p[1] }}</td>
      </tr>
    </table>
  </vc-modaldialog>
</template>

<script>
function about_pairs() {
  const App = Electron.app;
  const os = require ('os');
  return [
    [ 'Application:',	App.getName() + ' ' + App.getVersion() ],
    [ 'OS:',		process.platform + ' ' + process.arch + ' (' + os.release() + ')' ],
    [ 'Electron:',	process.versions.electron ],
    [ 'Chrome:',	process.versions.chrome ],
    [ 'User Agent:',	navigator.userAgent ],
    [ 'V8:',		process.versions.v8 ],
    [ 'Node.js:',	process.versions.node ],
    [ 'Bse:',		Bse.server.get_version() ],
    [ 'Vorbis:',	Bse.server.get_vorbis_version() ],
    [ 'Libuv:',		process.versions.uv ],
    [ 'Executable:',	App.getPath ('exe') ],
    [ 'jQuery:',	jQuery.fn.jquery ],
    [ 'Vuejs:',		Vue.version ],
    [ 'Working Dir:',	App.getAppPath() ],
    [ 'Desktop Dir:',	App.getPath ('desktop') ],
    [ 'Config Path:',	App.getPath ('userData') ],
    [ 'Music Path:',	App.getPath ('music') ],
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
