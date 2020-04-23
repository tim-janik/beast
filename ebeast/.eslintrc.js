module.exports = {
  "env": {
    "browser": true,
    "es6": true,
    "node": true
  },

  // eslint chokes on `await import();`, see: https://github.com/eslint/eslint/issues/11486
  // and babel-eslint needs special setup for vue: https://eslint.vuejs.org/user-guide/#usage
  // "parser": "babel-eslint",
  "parser": "vue-eslint-parser",
  "parserOptions": { "parser": "babel-eslint",  "sourceType": "module" },

  "globals": {
    "globalThis": false,
    "assert": false,
    "Electron": false,
    "$log": false,
    "debug": false,
    "Vue": false,
    "Bse": false,
    "Shell": false,
    "Util": false,
    "EQ": false,
    "_": false,
    "CONFIG": false,
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
    'vue/require-prop-type-constructor': 'warn',
    "quotes": [ "off", "single" ]
  },
  "plugins": [ "html" ],
  "extends": [ "eslint:recommended", "plugin:vue/recommended" ]
};
