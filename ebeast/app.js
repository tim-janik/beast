'use strict';

// == App ==
const AppMethods = {
  __proto__ : Electron.app
};
const App = {	// global App object with defaults
  current_project: undefined,
  project: function () { return this.current_project; },
  show_about: false,
  __proto__ : AppMethods
};
Object.preventExtensions (App);
window.App = App;

// == Load Parts ==
// load and setup application menus
const Menus = require ('./menus.js');
Electron.Menu.setApplicationMenu (Menus.build_menubar());

// == App.status() ==
// update status bar with the given text
AppMethods.status = function (message) {
  console.log ('STATUS:' + message);
};

// == export start_app() ==
module.exports.start_app = function () { // application setup after onready
  if (!App.current_project) {
    App.current_project = Bse.server.create_project ('Untitled');
    let example = __dirname + "/../../" + 'Demos/partymonster.bse';
    if (App.current_project)
      App.current_project.restore_from_file (example);
  }
};
