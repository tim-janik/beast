// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

import resolve  from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import rjson    from '@rollup/plugin-json';

export default {
  plugins: [
    resolve ({ preferBuiltins: false }),
    rjson (),
    commonjs (),
  ]
};
