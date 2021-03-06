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
  @import '../app.scss';
  #b-app {
    //* create app stacking context */
    position: relative; z-index: 0;
  }
</style>

<template>
  <div id="b-app" class="b-root" style="display: flex; width: 100%; height: 100%">
    <b-projectshell ref="Vue-prototype-Shell" ></b-projectshell>

    <!-- Generic modal dialogs -->
    <b-modaldialog v-for="d in modal_dialogs" :key="d.uid"
		   :value="d.visible.value" @input="d.input ($event)"
		   :exclusive="true" bwidth="9em" >
      <div slot="header">{{ d.header }}</div>
      <b-hflex slot="default" style="justify-content: flex-start; align-items: center;">
	<b-icon v-bind="d.icon" />
	<div style="flex-grow: 1; white-space: pre-line;" >{{ d.body }}</div>
      </b-hflex>
      <b-hflex slot="footer" :style="d.footerstyle" style="justify-content: space-between" >
	<b-button v-for="(b, i) in d.buttons" :key="i" @click="d.click (i)" :disabled="b.disabled"
		  :canfocus="b.canfocus" :autofocus="b.autofocus" >{{ b.label }}</b-button>
      </b-hflex>
    </b-modaldialog>

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
  const chrome_major_last_buggy = 85;
  CONFIG.dpr_movement = chrome_major >= 37 && chrome_major <= chrome_major_last_buggy;
  console.assert (chrome_major <= chrome_major_last_buggy, `WARNING: Chrome/${chrome_major} has not been tested for the movementX devicePixelRatio bug`);
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
  Vue.mixin (Util.vue_mixins.dom_updates); // Support `dom_update()` and similar hooks
  Vue.mixin (Util.vue_mixins.autodataattrs); // Auto apply data-* props to this.$el
  Object.assign (Vue.prototype, {
    CONFIG: globalThis.CONFIG,
    assert: globalThis.assert,
    debug: globalThis.debug,
    Util: globalThis.Util,
    App: globalThis.App,
    window: globalThis.window,
    document: globalThis.document,
    observable_from_getters: Util.vue_observable_from_getters,
    _: globalThis._,
  });
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

// == modal dialog creation ==
let modal_dialog_counter = 1;
function async_modal_dialog (title, btext, buttons = [], icon) {
  let resolve;
  const promise = new Promise (r => resolve = r);
  const m = {
    uid: modal_dialog_counter++,
    visible: Vue.reactive ({ value: false }),
    input (v) {
      if (!this.visible.value || v)
	return;
      this.visible.value = false;
      resolve (this.result);
      setTimeout (_ => Util.array_remove (App.vm.modal_dialogs, this), CONFIG.transitiondelay);
    },
    result: -1,
    click (r) {
      this.result = r;
      this.input (false);
    },
    header: title,
    body: btext,
    icon: modal_icons[icon] || {},
    footerstyle: '',
    buttons: []
  };
  const is_string = s => typeof s === 'string' || s instanceof String;
  const check_bool = (v, dflt) => v !== undefined ? !!v : dflt;
  for (let i = 0; i < buttons.length; i++)
    {
      const label = is_string (buttons[i]) ? buttons[i] : buttons[i].label;
      const disabled = check_bool (buttons[i].disabled, false);
      const canfocus = check_bool (buttons[i].canfocus, true);
      const autofocus = check_bool (buttons[i].autofocus, false);
      const button = { label, disabled, autofocus, canfocus };
      m.buttons.push (button);
    }
  if (m.buttons.length >= 2)
    m.footerstyle = 'width: 100%;';
  App.vm.modal_dialogs.push (m);
  setTimeout (_ => m.visible.value = true, 0); // changing value triggers animation
  return promise;
}
const modal_icons = {
  QUESTION:	{ fa: "question-circle",	style: "font-size: 300%; padding-right: 1rem; float: left; color: #538cc1" },
  ERROR:	{ fa: "times-circle",		style: "font-size: 300%; padding-right: 1rem; float: left; color: #cc2f2a" },
};

// == export default ==
const export_default = {
  name: 'b-app',
  bootup,
  zmove: ZMove.zmove.bind (ZMove),
  zmovehooks: [],
  data: _ => ({ modal_dialogs: [], }),
  async_modal_dialog,
  created () { App.vm = this; },
  __proto__: Vue.reactive (new App),
};
export default export_default;
Object.defineProperty (globalThis, 'App', { value: export_default });

</script>
