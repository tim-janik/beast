// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"use strict";

const required_markdown_it = require ("./node_modules/markdown-it/dist/markdown-it.js");

const ESM6EXPORT = { default: required_markdown_it };

console.assert (ESM6EXPORT.default);

module.exports = ESM6EXPORT.default;
