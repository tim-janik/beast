module.exports = {
  "env": {
    "browser": true,
    "es6": true,
    "jquery": true,
    "node": true
  },
  "globals": {
    "Electron": false,
    "Mithril": false,
    "Vue": false,
    "Bse": false,
    "App": false,
    "Shell": false,
    "T": false,
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
    "quotes": [ "off", "single" ]
  },
  "plugins": [ "html" ],
  "extends": "eslint:recommended"
};
