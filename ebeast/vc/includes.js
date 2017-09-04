// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// pull in jQuery
console.assert (window.jQuery === undefined);
window.$ = window.jQuery = require ('jquery');

// load utilities
require ('./hotkeys.js'); // adds $.click_hotkey()

// load Vue core
console.assert (window.Vue === undefined);
const Vue = require ('vue/dist/vue.common.js');
window.Vue = Vue;

// load Vue components, bundled from ./bundle.js
require ('../objects/vc-bundle.js');
