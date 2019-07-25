// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
const Electron = require ('electron');
const ElectronDefaultApp = process.defaultApp !== undefined; // indicates unpackaged electron app
const Eapp = Electron.app;

// add development hooks
const HAVE_DEVELOPMENT_TOOLS = true;

// split command line options and return one at a time
function pop_arg (args) {
  if (!args.length)
    return undefined;
  let a = args.shift();
  if (args.__nonoption__) {		// yield saved option argument
    args.__nonoption__--;
    return a;
  }
  if (args.skip_defaultapp) {		// finding defaultApp has precedence
    if (/^[^-]/.test (a)) {
      args.skip_defaultapp = false;	// consume process.defaultApp
      return pop_arg (args);
    }
  }
  if (args.seen_separator)		// yield only non-options after '--'
    return a;
  if (/^-[^-]+/.test (a)) {		// pop single letter option
    const rest = a.slice (2);
    if (/^=/.test (rest)) {		// save single letter option argument
      args.unshift (rest.slice (1));
      args.__nonoption__ = 1;
    } else if (rest)
      args.unshift ('-' + rest);	// save adjunct single letter options
    return '-' + a[1];
  } else if (/^--[^-]+/.test (a)) {	// pop long option
    const eq = a.indexOf ('=');
    if (eq > 0) {
      args.unshift (a.slice (eq + 1));	// save long option argument
      args.__nonoption__ = 1;
      return a.slice (0, eq);
    }
    return a;
  } else if (/^--$/.test (a)) {		// stop option parsing
    args.seen_separator = true;
    return pop_arg (args);
  }
  return a;
}

function print_help () {
  const lines = [
    `Usage: ${Eapp.getName()} [OPTIONS] [projectfiles...]`,
    `Options:`,
    `--help        Print command line option help`,
    `--version     Print version information`,
  ];
  console.log (lines.join ('\n'));
}

// parse command line arguments
const project_files = [];
(function () {
  let args = process.argv.slice (1);
  args.skip_defaultapp = ElectronDefaultApp;
  while (args.length) {
    const arg = pop_arg (args);
    if (args.seen_separator) {
      project_files.push (arg);
      continue;
    }
    switch (arg) {
    case '--help':
      print_help();
      Eapp.exit (0);
      break;
    case '--version':
      console.log (Eapp.getName() + ' ' + Eapp.getVersion());
      Eapp.exit (0);
      break;
    default:
      if (/^-/.test (arg)) {
	console.log ('Unknown option: ' + arg);
	print_help();
	Eapp.exit (129);
      }
      project_files.push (arg);
      break;
    }
  }
}) ();

// create the main ebeast window
var win;
function create_window ()
{
  const FS = require ('fs');
  // avoid menu flicker, leave menu construction to the window
  Electron.Menu.setApplicationMenu (null);
  // find some CSS presets needed as BrowserWindow defaults
  let appcss = FS.readFileSync (__dirname + '/assets/stylesheets.css', 'UTF-8');
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
    autoHideMenuBar:			false,
    webPreferences: {
      nodeIntegration:			true,
      devTools: 			HAVE_DEVELOPMENT_TOOLS,
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
  win.HAVE_DEVELOPMENT_TOOLS = HAVE_DEVELOPMENT_TOOLS;
  win.bse_argv = project_files;
  win.once ('ready-to-show', () => { win.show(); });
  win.loadURL ('file:///' + __dirname + '/window.html');
  // win.webContents.openDevTools();
  win.on ('closed', () => { win = null; });

  win.webContents.on ('crashed', function () { Eapp.exit (-127); });

}
Eapp.on ('ready', create_window); // create window once everything is loaded

// quit when all windows are closed.
Eapp.on ('window-all-closed', Eapp.quit);
