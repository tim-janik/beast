'use strict';

// == AppWindow ==
/** AppWindow - reactive Application Window class bound by Vue.
 * Properties will be turned into reactive properties on a Vue() instance,
 * and the methods can be invoked on the resulting Vue() instance.
 */
class AppWindow {
  // To avoid clashes, use snake_case when adding methods
  constructor (default_data) {
    this.show_about_ = false;
    Object.assign (this, default_data);
  }
  display_about_dialog (...visible) { if (visible.length) this.show_about_ = visible[0]; return this.show_about_; }
  get about_dialog ()	{ return this.show_about_; }
  set about_dialog (v)	{ this.show_about_ = v; }
}

// Export constructor to create AppWindow as Vue component
exports.VueAppWindow = function (vue_options) {
  const util = require ('./vc/utilities.js');
  return util.VueifyObject (new AppWindow (vue_options.data || {}), vue_options);
};

// == Menus ==
const menus = require ('./menus.js');
Electron.Menu.setApplicationMenu (menus.build_menubar());

// == App *OLD* ==
const AppMethods = {
  __proto__ : Electron.app
};
const App = {	// global App object with defaults
  show_about: false,
  __proto__ : AppMethods
};
Object.preventExtensions (App);
window.App = App;

// == App.status() ==
// update status bar with the given text
AppMethods.status = function (message) {
  console.log ('STATUS:' + message);
};
