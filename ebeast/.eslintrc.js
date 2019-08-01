module.exports = {
  "env": {
    "browser": true,
    "es6": true,
    "jquery": true,
    "node": true
  },
  "globals": {
    "assert": false,
    "Electron": false,
    "$log": false,
    "Vue": false,
    "Bse": false,
    "Shell": false,
    "Util": false,
    "_": false,
    "MAXINT": false,
    "MAXUINT": false,
    "GLOBALCONFIG": false,
    "module": true /* allow mods */
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
    'no-useless-escape': 'off',
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
    'no-irregular-whitespace': 'off', /* ["error", { 'skipStrings': true, 'skipComments': true, 'skipTemplates': true, 'skipRegExps':true } ], */
    "quotes": [ "off", "single" ]
  },
  "plugins": [ "html" ],
  "extends": [ "eslint:recommended", "plugin:vue/recommended" ]
};
