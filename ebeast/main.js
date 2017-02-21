// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
const {app, BrowserWindow, ipcMain} = require ('electron');

// create the main ebeast window
var win;
function create_window ()
{
  options = {
    width: 1820, height: 1024, // calling win.maximize() flickers, using a big size not
    webPreferences: {
      defaultEncoding:	'UTF-8',
      defaultFontSize:   	15,
      defaultMonospaceFontSize:	14,
      defaultFontFamily: {
	standard:	'Candara',	// 'Times New Roman',
	serif:		'Constantia',	// 'Times New Roman',
	sansSerif:	'Candara',	// 'Arial',
	monospace:	'Courier',	// 'Courier New',
	cursive:	'Script',	// 'Script',
	fantasy:	'Impact',	// 'Impact',
      },
    },
    show: false, // avoid incremental load effect, see 'ready-to-show'
    darkTheme: true,
    backgroundColor: '#333333',
  };
  win = new BrowserWindow (options);
  win.once ('ready-to-show', () => { win.show(); });
  win.loadURL ('file:///' + __dirname + '/ebeast.html');
  // win.webContents.openDevTools();
  win.on ('closed', () => { win = null });
}
app.on ('ready', create_window); // create window once everything is loaded

// quit when all windows are closed.
app.on ('window-all-closed', app.quit);
