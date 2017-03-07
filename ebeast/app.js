'use strict';

// load external libraries
window.Electron = require ('electron').remote;
window.Bse = require ('./v8bse/v8bse.node');
// window.$ = preloaded jQuery
window.Mithril = require ('mithril');

// Assign global App object with defaults
window.App = {
  show_about: false,
  __proto__ : Electron.app
};
Object.preventExtensions (App);

// load application parts that depend on App
const Widgets = require ('./assets/widgets.js');
const Dialogs = require ('./assets/dialogs.js');
const Menus = require ('./menus.js');
const m = Mithril;

// assign BrowserWindow's menubar
Electron.Menu.setApplicationMenu (Menus.build_menubar());

// Application start onready
module.exports.start_app = function () {
  Dialogs.toggle_about = function() {
    const m = Mithril;
    return (
      m ('button',
	 { onclick: function () { App.show_about = !App.show_about; } },
	 'Toggle About')
    );
  };

  const MithrilApp = {
    view: function () {
      const m = Mithril;
      const app_body = [
	Dialogs.toggle_about(),
	Dialogs.about_dialog(),
      ];
      return (
	m ('div#mithril.Mithril', {
	  style: 'position: absolute; top: 0; left: 0; right: 0; bottom: 0;',
	}, app_body)
      );
    }
  };
  m.mount (document.body, MithrilApp);
};
