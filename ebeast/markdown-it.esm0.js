// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"use strict";

// Rollup tends to mangle the target variable of require() assignments
const required_markdown_it = require ("markdown-it");

const ESM6EXPORT = { default: required_markdown_it };

// A Makefile rule turns the ESM6EXPORT.default assertion into a real export
console.assert (ESM6EXPORT.default);
