'use strict';

// == Application Menu ==
const file_menu = [
  { label: 'Open...',			role: 'open-file',		accelerator: 'Ctrl+O', },
  { label: 'Quit',			role: 'quit-app',		accelerator: 'Shift+Ctrl+Q', },
];
const view_menu = [
  { label: 'Toggle Fullscreen', 	role: 'toggle-fulscreen', 	accelerator: 'F11', },
  { label: 'Toggle Developer Tools', 	role: 'toggle-devtools',  	accelerator: 'Shift+Ctrl+I', },
];
const help_menu = [
  { label: '&About...',			role: 'about-dialog',		},
];
const menubar_menus = [
  { label: '&File', 			submenu: file_menu,		accelerator: 'Alt+F' },
  { label: '&View', 			submenu: view_menu,		accelerator: 'Alt+W' },
  { label: '&Help', 			submenu: help_menu,		accelerator: 'Alt+H' },
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
  app_command (menuitem.role, menuitem.data);
});
Electron.Menu.setApplicationMenu (Electron.Menu.buildFromTemplate (menubar_menus));

// handle menu activations
function app_command (role, _data) {
  const BrowserWindow = Electron.getCurrentWindow(); // http://electron.atom.io/docs/api/browser-window/
  const WebContents = BrowserWindow.webContents;     // http://electron.atom.io/docs/api/web-contents/
  switch (role) {
  case 'toggle-devtools':
    WebContents.toggleDevTools();
    break;
  case 'toggle-fulscreen':
    BrowserWindow.setFullScreen (!BrowserWindow.isFullScreen());
    break;
  case 'quit-app':
    Electron.app.quit();
    return false;
  case 'open-file':
    Electron.dialog.showOpenDialog ({
      properties: ['openFile', 'openDirectory', 'multiSelections'],
    }, (result) => {
      console.log ('open-file: ' + result);
    });
    break;
  case 'about-dialog':
    App.show_about = !App.show_about;
    Mithril.redraw();
    break;
  }
}
