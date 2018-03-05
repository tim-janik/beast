'use strict';

// == App ==
const AppMethods = {
  __proto__ : Electron.app
};
const App = {	// global App object with defaults
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
};
