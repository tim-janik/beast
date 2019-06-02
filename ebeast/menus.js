'use strict';

// == Application Menu ==
const file_menu = [
  { label: _('&Open...'),		role: 'open-file',		accelerator: 'Ctrl+O', },
  { label: _('&Save As...'),		role: 'save-as',		accelerator: 'Shift+Ctrl+S', },
  { label: _('&Preferences...'),	role: 'preferences-dialog',	accelerator: 'Ctrl+,', },
  { label: _('&Quit'),			role: 'quit-app',		accelerator: 'Shift+Ctrl+Q', },
];
const view_menu = [
  { label: _('Toggle &Fullscreen'), 	 role: 'toggle-fulscreen', 	accelerator: 'F11', },
];
const help_menu = [
  { label: _('Beast &Manual...'),	role: 'manual-dialog-html',	},
  { label: _('&About...'),		role: 'about-dialog',		},
];
const menubar_menus = [
  { label: _('&File'), 			submenu: file_menu,		accelerator: 'Alt+F' },
  { label: _('&View'), 			submenu: view_menu,		accelerator: 'Alt+W' },
  { label: _('&Help'), 			submenu: help_menu,		accelerator: 'Alt+H' },
];

// add 'click' handlers to menu templates
function assign_click (item, func) {
  if (Array.isArray (item)) {
    for (let i = 0; i < item.length; i++)
      assign_click (item[i], func);
  } else if (item.submenu !== undefined) {
    assign_click (item.submenu, func);
  } else if (item.click === undefined)
    item.click = func;
}

// assign global app menu
assign_click (menubar_menus, (menuitem, _focusedBrowserWindow, _event) => {
  menu_command (menuitem.role, menuitem.data);
});
module.exports.build_menubar = function () {
  return Electron.Menu.buildFromTemplate (menubar_menus);
};

// handle menu activations
function menu_command (role, _data) {
  const BrowserWindow = Electron.getCurrentWindow(); // http://electron.atom.io/docs/api/browser-window/
  switch (role) {
    case 'about-dialog':
      Shell.show_about_dialog = !Shell.show_about_dialog;
      break;
    case 'preferences-dialog':
      Shell.show_preferences_dialog = !Shell.show_preferences_dialog;
      break;
    case 'manual-dialog-html': {
      const win = new Electron.BrowserWindow ({
	backgroundColor: '#fefdfc',
	webPreferences: { contextIsolation: true,
			  nodeIntegration: false,
			  plugins: true, // needed for pdf_viewer
			  sandbox: true } });
      win.setMenu (null);
      win.loadURL ('file:///' + __dirname + '/../doc/beast-manual.html');
      win.webContents.on ('before-input-event', (event, input) => {
	if (input.alt && input.code=="ArrowLeft" &&
	    win.webContents.canGoBack())
	  win.webContents.goBack();
	if (input.alt && input.code=="ArrowRight" &&
	    win.webContents.canGoForward())
	  win.webContents.goForward();
      });
      break; }
    case 'toggle-fulscreen':
      BrowserWindow.setFullScreen (!BrowserWindow.isFullScreen());
      break;
    case 'quit-app':
      Electron.app.quit();
      return false;
    case 'open-file':
      if (!open_dialog_lastdir)
	open_dialog_lastdir = Bse.server.get_demo_path();
      Electron.dialog.showOpenDialog (Electron.getCurrentWindow(),
				      {
					title: Util.format_title ('Beast', 'Select File To Open'),
					buttonLabel: 'Open File',
					defaultPath: open_dialog_lastdir,
					properties: ['openFile', ], // 'multiSelections' 'openDirectory' 'showHiddenFiles'
					filters: [ { name: 'BSE Projects', extensions: ['bse'] },
						   { name: 'Audio Files', extensions: [ 'bse', 'mid', 'wav', 'mp3', 'ogg' ] },
						   { name: 'All Files', extensions: [ '*' ] }, ],
				      },
				      (result) => {
					if (result && result.length == 1)
					  {
					    const Path = require ('path');
					    open_dialog_lastdir = Path.dirname (result[0]);
					    Shell.load_project (result[0]);
					  }
				      });
      break;
    case 'save-as':
      if (!save_dialog_lastdir)
	{
	  const Path = require ('path');
	  save_dialog_lastdir = Electron.app.getPath ('music') || Path.resolve ('.');
	}
      Electron.dialog.showSaveDialog (Electron.getCurrentWindow(),
				      {
					title: Util.format_title ('Beast', 'Save Project'),
					buttonLabel: 'Save As',
					defaultPath: save_dialog_lastdir,
					filters: [ { name: 'BSE Projects', extensions: ['bse'] }, ],
				      },
				      (savepath) => {
					if (!savepath)
					  return;
					const Path = require ('path');
					save_dialog_lastdir = Path.dirname (savepath);
					if (!savepath.endsWith ('.bse'))
					  savepath += '.bse';
					const Fs = require ('fs');
					if (Fs.existsSync (savepath))
					  Fs.unlinkSync (savepath);
					Shell.save_project (savepath);
				      });
      break;
    default:
      console.log ('unhandled menu command: ' + role);
      break;
  }
}

let open_dialog_lastdir = undefined;
let save_dialog_lastdir = undefined;
