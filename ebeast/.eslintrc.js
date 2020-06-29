module.exports = {
  "env": {
    "browser": true,
    "es6": true,
    "node": true
  },

  // babel-eslint is needed for stage-3 syntax, see:
  // https://stackoverflow.com/questions/60046847/eslint-does-not-allow-static-class-properties/60464446#60464446
  "parserOptions": {
    "parser": "babel-eslint",
    "sourceType": "module"
  },
  // babel-eslint needs special setup for vue: https://eslint.vuejs.org/user-guide/#usage
  "parser": "vue-eslint-parser",
  // "parser": "babel-eslint", // <- moved to parserOptions under vue-eslint-parser

  "globals": {
    "App": false,
    "Bse": false,
    "CONFIG": false,
    "Electron": false,
    "Shell": false,
    "Util": false,
    "Vue": false,
    "_": false,
    "assert": false,
    "debug": false,
    "globalThis": false,
  },
  "rules": {
    "no-unused-vars": [ "warn", { "args": "none", "argsIgnorePattern": "^__.*", "varsIgnorePattern": "^__.*" } ],
    "no-unreachable": [ "warn" ],
    "semi": [ "error", "always" ],
    "no-extra-semi": [ "warn" ],
    "no-console": [ "off" ],
    "no-constant-condition": [ "warn" ],
    "indent": [ "off", 2 ],
    "linebreak-style": [ "error", "unix" ],
    "no-mixed-spaces-and-tabs": [ "off" ],
    'no-irregular-whitespace': 'off', /* ["error", { 'skipStrings': true, 'skipComments': true, 'skipTemplates': true, 'skipRegExps':true } ], */
    'no-useless-escape': 'off',
    'no-inner-declarations': 'off',
    // 'prefer-const': [ 'warn' ],
    'vue/attributes-order': 'off',
    'vue/html-closing-bracket-newline': 'off',
    'vue/html-closing-bracket-spacing': 'off',
    'vue/html-indent': 'off',
    'vue/html-quotes': 'off',
    'vue/html-self-closing': 'off',
    'vue/max-attributes-per-line': 'off',
    'vue/multiline-html-element-content-newline': 'off',
    'vue/no-multi-spaces': 'off',
    'vue/singleline-html-element-content-newline': 'off',
    'vue/prop-name-casing': 'off',
    'vue/name-property-casing': 'off',
    'vue/require-default-prop': 'off',
    'vue/require-prop-types': 'off',
    'vue/require-prop-type-constructor': 'warn',
    "quotes": [ "off", "single" ]
  },
  "plugins": [ "html" ],
  "extends": [ "eslint:recommended", "plugin:vue/recommended" ]
};
