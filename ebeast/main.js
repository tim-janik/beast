// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
const Electron = require ('electron');

// create the main ebeast window
var win;
function create_window ()
{
  const FS = require ('fs');
  // avoid menu flicker, leave menu construction to the window
  Electron.Menu.setApplicationMenu (null);
  // find some CSS presets needed as BrowserWindow defaults
  let appcss = FS.readFileSync ('objects/app.css', 'UTF-8');
  let rex = /\bBrowserWindowDefaults\s*{([^}]*)}/m;
  appcss = rex.exec (appcss)[1];       // extract BrowserWindowDefaults section
  rex = /\/\*([^*]|\*[^\/])*\*\//;
  appcss = appcss.replace (rex, ' ');  // remove comments
  rex = /\bbackgroundColor\s*:\s*([^;}]*)\s*[;}]/mi;
  const backgroundColor = rex.exec (appcss)[1];
  rex = /\bdefaultFontSize\s*:\s*([^;}]*)\s*[;}]/mi;
  const defaultFontSize = rex.exec (appcss)[1];
  rex = /\bdefaultMonospaceFontSize\s*:\s*([^;}]*)\s*[;}]/mi;
  const defaultMonospaceFontSize = rex.exec (appcss)[1];
  // window configuraiton
  const options = {
    width: 				1820, // calling win.maximize() causes flicker
    height: 				1365, // using a big initial size avoids flickering
    backgroundColor: 			backgroundColor,
    webPreferences: {
      defaultEncoding:			'UTF-8',
      defaultFontSize:			parseInt (defaultFontSize),
      defaultMonospaceFontSize:		parseInt (defaultMonospaceFontSize),
      defaultFontFamily: {
	standard:	'sans',		// 'Times New Roman',
	serif:		'Constantia',	// 'Times New Roman',
	sansSerif:	'Candara',	// 'Arial',
	monospace:	'Consolas',	// 'Courier New',
	cursive:	'Script',	// 'Script',
	fantasy:	'Impact',	// 'Impact',
      },
    },
    show: false, // avoid incremental load effect, see 'ready-to-show'
    darkTheme: true,
  };
  win = new Electron.BrowserWindow (options);
  win.once ('ready-to-show', () => { win.show(); });
  win.loadURL ('file:///' + __dirname + '/index.html');
  // win.webContents.openDevTools();
  win.on ('closed', () => { win = null; });
}
Electron.app.on ('ready', create_window); // create window once everything is loaded

// quit when all windows are closed.
Electron.app.on ('window-all-closed', Electron.app.quit);
