// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

import vue from 'rollup-plugin-vue';		   // compile SFC.vue files
import babel from 'rollup-plugin-babel';	   // see babel.config.js
import resolve from '@rollup/plugin-node-resolve'; // find vue-runtime-helpers/dist/normalize-component
import scss from 'rollup-plugin-scss';		   // compile scss to css

export default {
  // https://rollupjs.org/guide/en/#big-list-of-options
  output: {
    sourcemap: 'inline',
    format: 'esm',
  },
  plugins: [
    vue ({
      // https://rollup-plugin-vue.vuejs.org/options.html
      css: false,
      needMap: false,
      // normalizeComponent needs ./node_modules/vue-runtime-helpers/dist/index.mjs import
      normalizer: "globalThis['vue-runtime-helpers'].normalizeComponent",
      // data: { css: '/* globals... */', }, // increases line numbers in source maps
    }),
    babel(),
    scss ({
      // https://github.com/thgh/rollup-plugin-scss#options https://github.com/sass/node-sass#options
      sourceMapEmbed: true,
      sourceComments: true,
      outputStyle: 'nested', // nested, expanded, compact, compressed
    }),
    resolve(),
  ],
  external: [ './jsbse.js', ],
};
