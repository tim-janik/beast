'use strict';

// == Application Menu ==
const file_menuitems = [
  { label: _('&New Project'),		id: 'new-project',		accelerator: 'Ctrl+N', },
  { label: _('&Open...'),		id: 'open-file',		accelerator: 'Ctrl+O', },
  { label: _('&Save'),			id: 'save-same',		accelerator: 'Ctrl+S', },
  { label: _('&Save As...'),		id: 'save-as',			accelerator: 'Shift+Ctrl+S', },
  { type:  'separator' },
  { label: _('&Preferences...'),	id: 'preferences-dialog',	accelerator: 'Ctrl+,', },
  { type:  'separator' },
  { label: _('&Quit'),			id: 'quit-app',			accelerator: 'Shift+Ctrl+Q', },
];
const view_menuitems = [
  { label: _('Toggle &Fullscreen'),	id: 'toggle-fulscreen', 	accelerator: 'F11', },
];
const help_menuitems = [
  { label: _('Beast &Manual...'),	id: 'manual-dialog-html',	},
  { type:  'separator' },
  { label: _('&About...'),		id: 'about-dialog',		},
];
const menubar_menuitems = [
  { label: _('&File'), 			submenu: file_menuitems, },
  { label: _('&View'), 			submenu: view_menuitems, },
  { label: _('&Help'), 			submenu: help_menuitems, },
];

let save_same_filename = undefined;
let open_dialog_lastdir = undefined;
let save_dialog_lastdir = undefined;

// assign global app menu
export async function setup_app_menu()
{
  open_dialog_lastdir = await Bse.server.get_demo_path();
  function complete_menu_items (item)
  {
    if (Array.isArray (item))
      for (let i = 0; i < item.length; i++)
	complete_menu_items (item[i]);
    else if (item.submenu !== undefined)
      complete_menu_items (item.submenu);
    else
      {
	if (item.role === undefined && item.id !== undefined)
	  item.role = item.id;
	if (item.click === undefined && item.role !== undefined)
	  item.click = (menuitem, _focusedBrowserWindow, _event) => menu_command (menuitem);
      }
  }
  complete_menu_items (menubar_menuitems);
  const menubar_menu = Electron.Menu.buildFromTemplate (menubar_menuitems);
  Electron.Menu.setApplicationMenu (menubar_menu);
  check_all_menu_items();
}

export function check_all_menu_items()
{
  function check_menu_item (item) {
    if (item.items)
      for (let i = 0; i < item.items.length; i++)
	check_menu_item (item.items[i]);
    else if (item.submenu)
      check_menu_item (item.submenu);
    else
      menu_sentinel (item);
  }
  const app_menu = Electron.Menu.getApplicationMenu();
  if (app_menu)
    check_menu_item (app_menu);
}

function menu_sentinel (menuitem)
{
  switch (menuitem.role) {
    case 'save-same':
      menuitem.enabled = save_same_filename ? true : false;
      break;
  }
}

// handle menu activations
function menu_command (menuitem)
{
  const BrowserWindow = Electron.getCurrentWindow(); // http://electron.atom.io/docs/api/browser-window/
  menu_sentinel (menuitem);
  if (!menuitem.enabled) return;
  switch (menuitem.role) {
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
      let u = new URL (window.location);
      u = u.origin + '/doc/beast-manual.html';
      win.loadURL (u);
      win.setMenu (null);
      win.webContents.on ('before-input-event', (event, input) => {
	if (input.type == 'keyUp' || input.type == 'keyDown')
	  {
	    if (input.alt && input.code == "ArrowLeft" &&
		win.webContents.canGoBack())
	      {
		event.preventDefault();
		if (input.type == 'keyDown')
		  win.webContents.goBack();
	      }
	    if (input.alt && input.code == "ArrowRight" &&
		win.webContents.canGoForward())
	      {
		event.preventDefault();
		if (input.type == 'keyDown')
		  win.webContents.goForward();
	      }
	  }
      });
      break; }
    case 'toggle-fulscreen':
      BrowserWindow.setFullScreen (!BrowserWindow.isFullScreen());
      break;
    case 'quit-app':
      Electron.app.exit (0);
      return false;
    case 'new-project':
      save_same_filename = undefined;
      Shell.load_project();
      break;
    case 'open-file':
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
				      async (result) => {
					if (result && result.length == 1)
					  {
					    const Path = require ('path');
					    open_dialog_lastdir = Path.dirname (result[0]);
					    if (await Shell.load_project (result[0]) == Bse.Error.NONE)
					      {
						save_same_filename = result[0];
						check_all_menu_items();
					      }
					    else
					      console.error ('Failed to load:', result[0]);
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
					let err = Shell.save_project (savepath);
					if (err == Bse.Error.NONE)
					  {
					    save_same_filename = savepath;
					    check_all_menu_items();
					  }
					else
					  console.log ('Save:', savepath, err);
				      });
      break;
    case 'save-same':
      if (save_same_filename)
	{
	  const Fs = require ('fs');
	  if (Fs.existsSync (save_same_filename))
	    Fs.unlinkSync (save_same_filename);
	  // TODO: instead of calling unlinkSync(), BSE should support atomically replacing bse files
	  Shell.save_project (save_same_filename);
	}
      break;
    default:
      console.log ('unhandled menu command: ' + menuitem.role);
      break;
  }
  menu_sentinel (menuitem);
}
