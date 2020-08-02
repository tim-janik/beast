<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-APP
  Global application instance for Beast.
  ## Public Exports:
  *zmovehooks*
  : An array of callbacks to be notified on pointer moves.
  *zmove (event)*
  : Trigger the callback list `zmovehooks`, passing `event` along. This is useful to get debounced
  notifications for pointer movements, including 0-distance moves after significant UI changes.
</docs>

<style lang="scss">
  @import 'mixins.scss';
</style>

<template>
  <div id="b-app" class="b-root" style="display: flex; width: 100%; height: 100%">
    <b-projectshell ref="Vue-prototype-Shell" ></b-projectshell>
  </div>
</template>

<script>
// == zmove() ==
class ZMove {
  static
  zmove (ev) {
    if (ev && ev.screenX && ev.screenY &&
	ev.timeStamp > (ZMove.last_event?.timeStamp || 0))
      ZMove.last_event = ev;
    ZMove.trigger();
  }
  static
  trigger_hooks() {
    if (ZMove.last_event)
      for (const hook of export_default.zmovehooks)
        hook (ZMove.last_event);
  }
  static trigger = Util.debounce (ZMove.trigger_hooks);
}

// == Bootup ==
// Vue mounting and dynamic import() calls to control module loading side-effects
async function bootup() {
  bootlog ("App bootup");
  // Setup global CONFIG
  const packagejson = JSON.parse (document.getElementById ('--EMBEDD-package_json').innerHTML);
  const mainconfig = globalThis.Electron?.getCurrentWindow()?.MAINCONFIG || {
    // defaults for non-Electron runtimes
    MAXINT: 2147483647, MAXUINT: 4294967295, mainjs: false,
    dpr_movement: false, // Chrome bug, movementX *should* match screenX units
    files: [], p: '', m: '', norc: false, uiscript: '',
  };
  Object.assign (CONFIG, packagejson.config, mainconfig);
  // Chrome Bug: https://bugs.chromium.org/p/chromium/issues/detail?id=1092358 https://github.com/w3c/pointerlock/issues/42
  const chrome_major = parseInt (( /\bChrome\/([0-9]+)\./.exec (navigator.userAgent) || [0,0] )[1]);
  CONFIG.dpr_movement = chrome_major >= 37 && chrome_major <= 83;
  console.assert (chrome_major <= 83, `WARNING: Chrome/${chrome_major} has not been tested for the movementX devicePixelRatio bug`);
  if (CONFIG.dpr_movement)
    bootlog ("Detected Chrome bug #1092358...");
  // Electron specifics
  if (globalThis.Electron)
    {
      bootlog ("Configure Electron...");
      const navigate_external = (ev, url) => {
	if (ev.defaultPrevented) // <- main.js causes prevention
	  {
	    debug ("NAVIGATE-EXTERNAL:", url);
	    Electron.shell.openExternal (url);
	  }
      };
      const win = Electron.getCurrentWindow();
      win.webContents.addListener ('will-navigate', navigate_external);
      win.webContents.addListener ('new-window', navigate_external);
      if (CONFIG.debug) // show DevTools on hotkey
	document.addEventListener ("keydown", (event) => {
	  if (event.shiftKey && event.ctrlKey && event.keyCode == 73) // Shift+Ctrl+I
	    Electron.getCurrentWindow().toggleDevTools();
	});
    }
  // Add helpers for device specific CSS
  document.body.parentNode.style.setProperty ('--device-pixel-ratio', window.devicePixelRatio);
  // Add helpers for browser specific CSS (needs document.body to exists)
  document.body.parentElement.toggleAttribute ('gecko', navigator.userAgent.indexOf ('Gecko/') >= 0);
  document.body.parentElement.toggleAttribute ('chrome', navigator.userAgent.indexOf ('Chrome/') >= 0);
  bootlog ("Enable <html/> UA attributes:",
	   [ document.body.parentElement.hasAttribute ('gecko') ? 'gecko' : '',
	     document.body.parentElement.hasAttribute ('chrome') ? 'chrome' : '' ].join (' '));
  // Bse - import Jsonipc based libbse bindings
  bootlog ("Importing Bse...");
  Object.defineProperty (globalThis, 'Bse', { value: await import ('/jsbse.js') });
  bootlog ("Connecting to Bse.server...");
  await Bse.server['$promise'];
  console.assert (Bse.server['$id'] > 0);
  // MarkdownIt - import MarkdownIt ESM wrapper, needed by Util
  bootlog ("Importing Markdown...");
  Object.defineProperty (globalThis, 'MarkdownIt', { value: (await import ('/markdown-it.mjs')).default });
  // _() - add translation indicator
  bootlog ("Configure i18n...");
  // globalThis._ = Bse.server.gettext (txtstring);
  Object.defineProperty (globalThis, '_', { value: txtstring => txtstring });
  // Setup Vue utilities and shortcuts inside Vue component handlers
  for (let directivename in Util.vue_directives) // register all utility directives
    Vue.directive (directivename, Util.vue_directives[directivename]);
  Vue.mixin (Util.vue_mixins.data_tmpl);   // Allow automatic `data` construction (cloning) from `data_tmpl`
  Vue.mixin (Util.vue_mixins.dom_updates); // Support `dom_create`, `dom_update`, `dom_destroy` hooks
  Vue.mixin (Util.vue_mixins.autodataattrs); // Auto apply data-* props to this.$el
  Object.assign (Vue.prototype, {
    CONFIG: globalThis.CONFIG,
    debug: globalThis.debug,
    Util: globalThis.Util,
    App: globalThis.App,
    observable_from_getters: Util.vue_observable_from_getters,
    _: globalThis._,
  });
  // Menus - import if Electron is present
  if (globalThis.Electron)
    {
      const menus = await import ('/menus.js');
      await menus.setup_app_menu();
      bootlog ("Loaded Electron.Menus...");
    }
  // SFC - import list of b/ Vue components and register with Vue
  bootlog ("Importing MJS components...");
  for (const link of document.querySelectorAll ('head link[data-autoload][href]'))
    {
      const exports = await import (link.getAttribute ('href'));
      if (exports.default && exports.default.name)
	Vue.component (exports.default.name, exports.default);
    }
  // Fonts - wait for fonts before Vue components are mounted and compute sizes
  await document.fonts.ready;
  bootlog ("Loaded Fonts...");
  // Shell will inject itself into all Vue components, make it global
  Object.defineProperty (globalThis, 'Shell', { get: () => Vue.prototype.Shell, });
  // Mount Vue App, this creates b-projectshell as Vue.Shell
  bootlog ("Mounting Vue App...");
  const vue_root = new Vue ({
    el:   '#b-app',
    data: { Shell: { Shell_placeholder: true, }, },	// flag causing replacement later on
  });
  console.assert (vue_root); // Vue.$root
  App.$forceUpdate = () => {
    function vue_update (component) {
      component.$forceUpdate();
      for (let child of component.$children)
	vue_update (child);
    }
    vue_update (vue_root);
  };
  // Load external BSE plugins
  if (CONFIG.mainjs)
    {
      bootlog ("Loading LADSPA plugins...");
      await Bse.server.load_ladspa();
    }
  // UI-Script
  if (CONFIG.mainjs && CONFIG.uiscript)
    {
      bootlog ("Loading '" + window.CONFIG.uiscript + "'...");
      window.uiscript = await import (window.CONFIG.uiscript);
      try {
	await window.uiscript.run();
      } catch (e) {
	console.error (e.stack ? e.stack : e.message ? e.message : e, '\n',
		       'UIScript failed, aborting...');
	Electron.app.exit (123);
      }
    }
  // Load command line files
  if (CONFIG.mainjs && CONFIG.files.length)
    {
      bootlog ("Loading", window.CONFIG.files.length, "command line files...");
      for (let arg of window.CONFIG.files)
	{
	  const error = await Shell.load_project (arg);
	  if (error != Bse.Error.NONE)
	    console.error ('Failed to load:', arg + ":", error);
	}
    }
  // Clear temporary files if *not* in script mode
  if (CONFIG.mainjs && !CONFIG.uiscript)
    {
      const ms = 2000;
      bootlog ("Will clean Bse cachedirs in", ms + "ms...");
      setTimeout (async () => {
	await Bse.server.purge_stale_cachedirs();
	// bootlog ("Cleaned Bse cachedirs...");
      }, ms);
    }
  // Dismiss startup messages
  bootlog ("Finished bootup.\n");
}

// == App Globals ==
class App {
  data_bubble = null;
  panel3_types = [ 'i' /*info*/, 'b' /*browser*/ ];
  panel3 = 'i';
  switch_panel3 (n) {
    const a = this.panel3_types;
    if ('string' == typeof n)
      this.panel3 = n;
    else
      this.panel3 = a[(a.indexOf (this.panel3) + 1) % a.length];
  }
  panel2_types = [ 'd' /*devices*/, 'p' /*pianoroll*/ ];
  panel2 = 'p';
  switch_panel2 (n) {
    const a = this.panel2_types;
    if ('string' == typeof n)
      this.panel2 = n;
    else
      this.panel2 = a[(a.indexOf (this.panel2) + 1) % a.length];
  }
  piano_roll_source = undefined;
  open_piano_roll (midi_source) {
    this.piano_roll_source = midi_source;
    if (this.piano_roll_source)
      this.switch_panel2 ('p');
  }
}

// == export default ==
const export_default = {
  name: 'b-app',
  bootup,
  zmove: ZMove.zmove.bind (ZMove),
  zmovehooks: [],
  __proto__: Vue.reactive (new App),
};
export default export_default;
Object.defineProperty (globalThis, 'App', { value: export_default });

</script>
