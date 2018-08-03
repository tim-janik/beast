// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// assert Vue is present
console.assert (Vue !== undefined);

// load utilities
require ('./hotkeys.js'); // adds $.click_hotkey()
const Util = require ('./utilities.js');
Vue.mixin (Util.vue_mixins.data_tmpl); // Allow automatic `data` construction (cloning) from `data_tmpl`

// load Vue components
const vue_components = [
  require ('./button.vue'),
  require ('./button-bar.vue'),
  require ('./color-picker.vue'),
  require ('./icon-button.vue'),
  require ('./piano-roll.vue'),
  require ('./playcontrols.vue'),
  require ('./part-thumb.vue'),
  require ('./modaldialog.vue'),
  require ('./aboutdialog.vue'),
  require ('./hscrollbar.vue'),
  require ('./track-list.vue'),
  require ('./track-view.vue'),
  require ('./projectshell.vue'),
];

// register components
vue_components.forEach (c => {
  Vue.component (c.name, c);
});
