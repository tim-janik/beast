// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// assert Vue is present
console.assert (window.Vue !== undefined);

// load Vue components
const vue_components = [
  require ('./button.vue'),
  require ('./icon-button.vue'),
  require ('./button-bar.vue'),
];

// register components
vue_components.forEach ((c) => {
  Vue.component (c.name, c);
});
