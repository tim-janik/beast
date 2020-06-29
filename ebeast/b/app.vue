<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-APP
  Global application instance for Beast.
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
export default {
  name: 'b-app',
  bootup,
};

const bootlog = debug || (() => undefined);

// == Bootup ==
// Vue mounting and dynamic import() calls to control module loading side-effects
async function bootup() {
  bootlog ("App bootup");
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
  // Vue - import and configure
  bootlog ("Importing Vue...");
  const vue_config = {
    productionTip: false,
    devtools: false,	// prevent warning about VUEJS_DEVTOOLS missing
    silent: !CONFIG.debug,
    performance: false,
  };
  const vue_options = {
    inheritAttrs: false, // Add non-props attributes to $attrs (default in Vue-3)
  };
  Object.defineProperty (globalThis, 'Vue', { value: (await import ('/vue.mjs')).default });
  Object.assign (Vue.config, vue_config);
  Object.assign (Vue.options, vue_options);
  console.assert (!Vue.reactive); // provided by Vue-3
  Vue.reactive = Vue.observable;  // rename by Vue-3
  // Setup global App object
  const app_tmpl = {
    data_bubble: null,
    onpointermoves: [],
    panel3_types: [ 'i' /*info*/, 'b' /*browser*/ ],
    panel3: 'i',
    switch_panel3 (n) {
      const a = this.panel3_types;
      if ('string' == typeof n)
	this.panel3 = n;
      else
	this.panel3 = a[(a.indexOf (this.panel3) + 1) % a.length];
    },
    panel2_types: [ 'd' /*devices*/, 'p' /*pianoroll*/ ],
    panel2: 'd',
    switch_panel2 (n) {
      const a = this.panel2_types;
      if ('string' == typeof n)
	this.panel2 = n;
      else
	this.panel2 = a[(a.indexOf (this.panel2) + 1) % a.length];
    }
  };
  Object.defineProperty (globalThis, 'App', { value: Vue.reactive (app_tmpl) });
  // MarkdownIt - import MarkdownIt ESM wrapper, needed by Util
  bootlog ("Importing Markdown...");
  Object.defineProperty (globalThis, 'MarkdownIt', { value: (await import ('/markdown-it.mjs')).default });
  // Util - import, depends on Vue and MarkdownIt
  bootlog ("Importing Util...");
  Object.defineProperty (globalThis, 'Util', { value: await import ('/util.js') });
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
  if (window.Electron)
    {
      const menus = await import ('/menus.js');
      await menus.setup_app_menu();
      bootlog ("Loaded Menus...");
    }
  else
    bootlog ("Skipping Electron Menus...");
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
  bootlog ("Loading LADSPA plugins...");
  await Bse.server.load_ladspa();
  // UI-Script
  if (window.CONFIG.uiscript)
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
  if (window.CONFIG.files.length)
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
  if (!window.CONFIG.uiscript)
    {
      const ms = 2000;
      bootlog ("Will clean Bse cachedirs in", ms + "ms...");
      setTimeout (async () => {
	await Bse.server.purge_stale_cachedirs();
	bootlog ("Cleaned Bse cachedirs...");
      }, ms);
    }
  // Dismiss startup messages
  bootlog ("Finished bootup.\n");
}

</script>
