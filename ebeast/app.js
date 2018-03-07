'use strict';

// == AppWindow ==
/** AppWindow - reactive Application Window class bound by Vue.
 * Properties will be turned into reactive properties on a Vue() instance,
 * and the methods can be invoked on the resulting Vue() instance.
 */
class AppWindow {
  // To avoid clashes, use snake_case when adding methods
  constructor (default_data) {
    this.project_ = null;
    this.show_about_ = false;
    this.log = console.log;
    Object.assign (this, default_data);
  }
  display_about_dialog (...visible) { if (visible.length) this.show_about_ = visible[0]; return this.show_about_; }
  get show_about_dialog ()		{ return this.show_about_; }
  set show_about_dialog (v)		{ this.show_about_ = v; }
  status (...msgs)			{ console.log (msgs.join (' ')); }
  bse_project() {
    if (!this.project_) {
      this.project_ = Bse.server.create_project ('Untitled');
      if (this.project_) {
	let example = __dirname + "/../../" + 'Demos/partymonster.bse';
	this.project_.restore_from_file (example);
      }
    }
    return this.project_;
  }
}

// Export constructor to create AppWindow as Vue component
exports.VueAppWindow = function (vue_options) {
  const util = require ('./vc/utilities.js');
  return util.VueifyObject (new AppWindow (vue_options.data || {}), vue_options);
};

// == Menus ==
const menus = require ('./menus.js');
Electron.Menu.setApplicationMenu (menus.build_menubar());
