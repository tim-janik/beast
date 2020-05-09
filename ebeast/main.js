// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

// Build time configuration, substituted in Javascript file headers by Makefile.mk
const CONFIG = { MAXINT: 2147483647, MAXUINT: 4294967295,              // plus delayed package.json values
		 files: [], p: '', m: '', norc: false, uiscript: '' };
Object.assign (CONFIG, require ('./package.json').config);

let win;
let bse_proc; // assigned spawn.child
let verbose = false, verbose_binary = false, withgdb = false;

// Load Electron module for BrowserWindow, etc
const Electron = require ('electron');
const ElectronDefaultApp = process.defaultApp !== undefined; // indicates unpackaged electron app
const Eapp = Electron.app;
// Avoid stale sources and storing non-network resources in users home
Eapp.commandLine.appendSwitch ('disable-http-cache');

// create the main ebeast window
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
      devTools:				CONFIG.debug,
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
  let win = new Electron.BrowserWindow (options);
  win.MAINCONFIG = CONFIG;
  win.webContents.once ('crashed', e => {
    seen_crashed = true;
    if (seen_will_quit && seen_crashed)
      Eapp.exit (143); // will-quit + crashed is caused by SIGHUP or SIGTERM
  });
  win.webContents.printout = function (...args) { console.log (...args); };
  win.webContents.printerr = function (...args) { console.error (...args); };
  return win;
}

// Start BeastSoundEngine in private mode
function create_beast_sound_engine (datacb, errorcb) {
  const { spawn, spawnSync } = require ('child_process');
  let args = [ '--embed', '3' ];
  if (verbose)
    args.push ('--verbose');
  if (verbose_binary)
    args.push ('--binary');
  if (CONFIG.norc)
    {
      args.push ('--bse-rcfile');
      args.push ('/dev/null');
    }
  if (CONFIG.p)
    {
      args.push ('--bse-pcm-driver');
      args.push (CONFIG.p);
    }
  if (CONFIG.m)
    {
      args.push ('--bse-midi-driver');
      args.push (CONFIG.m);
    }
  const beastsoundengine = __dirname + '/../lib/BeastSoundEngine-' + CONFIG['version'] + '-rt';
  const bse_proc = spawn (beastsoundengine, args, { stdio: [ 'pipe', 'inherit', 'inherit', 'pipe' ] });
  if (withgdb)
    {
      console.log ('DEBUGGING:\n  gdb --pid', bse_proc.pid, '#', beastsoundengine);
      spawnSync ('/usr/bin/sleep', [ 3 ]);
    }
  bse_proc.stdio[3].once ('data', (bytes) => datacb (bytes.toString()));
  if (errorcb)
    {
      bse_proc.stdio[3].once ('end', () => errorcb());
      bse_proc.stdio[3].once ('error', (err) => errorcb());
    }
  return bse_proc;
}

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

// Print help/usage
function print_help () {
  const lines = [
    `Usage: ${Eapp.getName()} [OPTIONS] [projectfiles...]`,
    'Options:',
    '--help        Print command line option help',
    '--version     Print version information',
    '--gdb         Print command to debug BSE',
    '--norc        Skip reading of ~/.config/beast/bserc.xml',
    '-p DRIVER     Set the default PCM driver (for --norc)',
    '-m DRIVER     Set the default MIDI driver (for --norc)',
  ];
  console.log (lines.join ('\n'));
}

// parse command line arguments
{
  let args = process.argv.slice (1);
  args.skip_defaultapp = ElectronDefaultApp;
  while (args.length) {
    const arg = pop_arg (args);
    if (args.seen_separator) {
      CONFIG.files.push (arg);
      continue;
    }
    switch (arg) {
      case '--help':
	print_help();
	main_exit (0);
	break;
      case '--version':
	console.log (Eapp.getName() + ' ' + Eapp.getVersion());
	main_exit (0);
	break;
      case '--verbose':
	verbose = true;
	break;
      case '--binary':
	verbose_binary = true;
	break;
      case '--gdb':
	withgdb = true;
	break;
      case '-p':
	CONFIG.p = args.length ? pop_arg (args) : '';
	break;
      case '-m':
	CONFIG.m = args.length ? pop_arg (args) : '';
	break;
      case '--norc':
	CONFIG.norc = true;
	break;
      case '--uiscript':
	CONFIG.uiscript = args.length ? pop_arg (args) : '';
	break;
      default:
	if (/^-/.test (arg)) {
	  console.err ('Unknown option: ' + arg);
	  print_help();
	  main_exit (5);
	}
	CONFIG.files.push (arg);
	break;
    }
  }
}

// Start BeastSoundEngine and BrowserWindow
async function startup_components() {
  // configure BrowserWindow with BeastSoundEngine embedding info
  win = create_window();
  win.on ('closed', () => win = null);
  // TODO: when this BrowserWindow is close, also force closing of the beast-manual or other related (child) windows
  let win_ready = new Promise (resolve => win.once ('ready-to-show', () => resolve ()));
  let embedding_info = new Promise (resolve => bse_proc = create_beast_sound_engine (msg => resolve (msg), () => main_exit (3)));
  const auth = JSON.parse (await embedding_info); // yields JSON: { "url": "http://127.0.0.1:<PORT>/index.html?subprotocol=<STRING>" }
  if (!auth.url)
    main_exit (2);
  win.loadURL (auth.url);
  await win_ready;
  win.show();
}
Eapp.once ('ready', startup_components); // create BeastSoundEngine and BrowserWindow once everything is loaded

// End process and main_exit all dependent processes
function main_exit (exitcode) {
  if (main_exit.shuttingdown)
    return;
  main_exit.shuttingdown = true;
  if (win)
    {
      win.destroy();
      win = null;
    }
  if (bse_proc)
    bse_proc.kill();
  Eapp.exit (exitcode ? exitcode : 0);
}

// Valid exit
Eapp.once ('window-all-closed', () => {
  win = null; // this *may* be emitted before win.closed
  main_exit (0);
});

// Invalid exit conditions
process.on ('uncaughtException', function (err) {
  console.error (process.argv[0] + ": uncaughtException (" + (new Date).toUTCString() + '):');
  console.error (err);
  main_exit (7);
});

/* Note, Electron hijacks SIGINT, SIGHUP, SIGTERM to trigger app.quit()
 * which has a 0 exit status. We simply don't use quit() and force a
 * non-0 status if it's used. See also:
 * https://github.com/electron/electron/issues/19650
 * https://github.com/electron/electron/issues/5273
 */
let seen_crashed = false, seen_will_quit = false;
Electron.app.once ('will-quit', e => {
  seen_will_quit = true;
  if (seen_will_quit && seen_crashed)
    Eapp.exit (143);    // will-quit + crashed is caused by SIGHUP or SIGTERM
  Eapp.exit (130);	// assume SIGINT
});
