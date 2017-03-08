'use strict';

// == Essential Modules ==
// window.$ = preloaded jQuery
window.Electron = require ('electron').remote;
window.Bse = require ('./v8bse/v8bse.node');
window.Mithril = require ('mithril');

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
// load parts that depend on App
const Widgets = require ('./assets/widgets.js');
const Dialogs = require ('./assets/dialogs.js');
const m = Mithril;

// == App.status() ==
// update status bar with the given text
AppMethods.status = function (message) {
  $('#statusfield').text (message);
};
const StatusBar = {
  view (_vnode) {
    return (
      m ('#statusarea.StatusArea.hbox.w100', [
	m ('#statusbar.StatusBar.maxbox', [
	  'Status: ',
	  m ('#statusfield.StatusField'),
	]),
      ])
    );
  },
};

// == PlayControls ==
const PlayControls = {
  view (vnode) {
    const B = Widgets.IconButton;
    return (
      m ('#playcontrols.PlayControls', vnode.attrs, [
	m ('span', { style: 'margin: 0 2em' }, [
	  m (B, { icon: 'fa-cog fa-spin0' }),
	]),
	m ('span.ButtonBox', [
	  m (B, { icon: 'fa-fast-backward', hotkey: 'L',     onclick: () => { App.status ('Last...'); } }),
	  m (B, { icon: 'fa-backward',      hotkey: 'B',     onclick: () => { App.status ('Backwards...'); } }),
	  m (B, { icon: 'fa-stop',          hotkey: 'Space', onclick: () => { App.status ('Stop...'); } }),
	  m (B, { icon: 'fa-play',          hotkey: 'P',     onclick: () => { App.status ('Play...'); } }), // toggle fa-play / fa-pause
	  m (B, { icon: 'fa-circle',        hotkey: 'R',     onclick: () => { App.status ('...Record...'); } }),
	  m (B, { icon: 'fa-forward',       hotkey: 'F',     onclick: () => { App.status ('Forwards...'); } }),
	  m (B, { icon: 'fa-fast-forward',  hotkey: 'N',     onclick: () => { App.status ('Next...'); } }),
	]),
	m ('span.ButtonBox', { style: 'margin: 0 2em' }, [
	  m (B, { icon: 'fa-music' }),
	]),
	m ('span', { style: 'margin: 0 2em' }, [
	  m (B, { icon: 'fa-info-circle' }),
	]),
      ])
    );
  },
};

// == Toolbar ==
const Toolbar = {
  view (vnode) {
    return (
      m ('#toolbar.ToolBar', vnode.attrs, vnode.children, [
	m (PlayControls),
      ])
    );
  },
};

// == export start_app() ==
module.exports.start_app = function () { // application setup after onready
  const MithrilApp = {
    view: function () {
      const m = Mithril;
      const app_body = [
	m (Toolbar),
	m ('.maxbox'),
	m (StatusBar),
	Dialogs.about_dialog(),
      ];
      return (
	m ('div#mithril.Mithril.vbox', {
	  style: 'position: absolute; top: 0; left: 0; right: 0; bottom: 0;',
	}, app_body)
      );
    }
  };
  m.mount (document.body, MithrilApp);
};
