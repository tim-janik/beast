// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// assert Vue is present
console.assert (Vue !== undefined);

// load Vue components
const vue_components = [
  require ('./aboutdialog.vue'),
  require ('./button-bar.vue'),
  require ('./button.vue'),
  require ('./color-picker.vue'),
  require ('./contextmenu.vue'),
  require ('./devicepanel.vue'),
  require ('./deviceeditor.vue'),
  require ('./example.vue'),
  require ('./icon.vue'),
  require ('./menuitem.vue'),
  require ('./menurow.vue'),
  require ('./menuseparator.vue'),
  require ('./menutitle.vue'),
  require ('./more.vue'),
  require ('./fed-number.vue'),
  require ('./fed-object.vue'),
  require ('./fed-picklist.vue'),
  require ('./fed-switch.vue'),
  require ('./fed-text.vue'),
  require ('./flex.vue'),
  require ('./grid.vue'),
  require ('./hscrollbar.vue'),
  require ('./modaldialog.vue'),
  require ('./part-list.vue'),
  require ('./part-thumb.vue'),
  require ('./piano-roll.vue'),
  require ('./playcontrols.vue'),
  require ('./preferencesdialog.vue'),
  require ('./projectshell.vue'),
  require ('./positionview.vue'),
  require ('./track-list.vue'),
  require ('./track-view.vue'),
  require ('./treeselector.vue'),
  require ('./treeselector-item.vue'),
];

// register components
vue_components.forEach (c => {
  if (c.name)
    Vue.component (c.name, c);
});
