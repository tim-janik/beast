// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// assert Vue is present
console.assert (Vue !== undefined);

// load utilities
require ('./hotkeys.js'); // adds $.click_hotkey()
Vue.mixin (Util.vue_mixins.data_tmpl); // Allow automatic `data` construction (cloning) from `data_tmpl`

// load Vue components
const vue_components = [
  require ('./button.vue'),
  require ('./button-bar.vue'),
  require ('./color-picker.vue'),
  require ('./icon.vue'),
  require ('./fed-number.vue'),
  require ('./fed-switch.vue'),
  require ('./fed-text.vue'),
  require ('./fed-object.vue'),
  require ('./piano-roll.vue'),
  require ('./playcontrols.vue'),
  require ('./part-list.vue'),
  require ('./part-thumb.vue'),
  require ('./modaldialog.vue'),
  require ('./aboutdialog.vue'),
  require ('./preferencesdialog.vue'),
  require ('./hscrollbar.vue'),
  require ('./track-list.vue'),
  require ('./track-view.vue'),
  require ('./projectshell.vue'),
  require ('./example.vue'),
];

// register components
vue_components.forEach (c => {
  Vue.component (c.name, c);
});

// run BSE JS unit tests
function ebeast_test_bse_basics() {
  console.assert (Bse.server);
  const proj = Bse.server.create_project ('ebeast_test_bse_basics-A');
  console.assert (proj);
  console.assert (proj.get_name() == 'ebeast_test_bse_basics-A');
  console.assert (proj.get_prop ('uname') == 'ebeast_test_bse_basics-A');
  proj.set_prop ('uname', 'ebeast_test_bse_basics-B2');
  console.assert (proj.get_name() == 'ebeast_test_bse_basics-B2');
  console.assert (proj.get_prop ('uname') == 'ebeast_test_bse_basics-B2');
  let icon = proj.get_prop ('icon');
  console.assert (icon && icon.width == 0 && icon.height == 0);
  proj.set_prop ('icon', { width: 2, height: 2, pixels: [1, "", "3", 4] });
  icon = proj.get_prop ('icon');
  console.assert (icon && icon.width == 2 && icon.height == 2);
  console.assert (JSON.stringify (icon.pixels) == JSON.stringify ([1, 0, 1, 4]));
  console.log ("  COMPLETE  " + 'ebeast_test_bse_basics');
}
ebeast_test_bse_basics();

