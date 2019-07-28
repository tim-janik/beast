// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"use strict";

/// Jsonipc centrum for Javascript communication.
function $jsonipc (this_, methodname, args) {
  if (!$jsonipc.ws)
    throw "$jsonipc: connection closed";
  const id = ++$jsonipc.counter;
  return new Promise ((resolve, reject) => {
    $jsonipc.idmap[id] = (msg) => {
      if (msg.error)
	reject (msg.error);
      else
	resolve (msg.result);
    };
    const jsondata = JSON.stringify ({
      id: $jsonipc.counter,
      this: this_['$id'],
      method: methodname,
      args: args,
    });
    $jsonipc.ws.send (jsondata);
  });
}
$jsonipc.counter = 10000 * Math.floor (10 + 89 * Math.random());
$jsonipc.idmap = {};
$jsonipc.handler = (event) => {
  const msg = JSON.parse (event.data);
  if (msg.id)
    {
      const handler = $jsonipc.idmap[msg.id];
      delete $jsonipc.idmap[msg.id];
      if (handler)
	handler (msg);
      // else ignore unknown messages
    }
};

// -------- Javascript BSE API (auto-generated) --------
import * as Bse from './js_bseapi.js';
Bse.$jsonipc.send = $jsonipc;
export * from './js_bseapi.js';
// -------- Javascript BSE API (auto-generated) --------

export const server = new Bse.Server (null);
server['$promise'] = new Promise ((resolve, reject) =>
  {
    const ws = new WebSocket ("ws://localhost:27239/", 'auth123');
    ws.onerror = (event) => reject (event);
    // ws.onclose = (event) => {};
    ws.onmessage = $jsonipc.handler;
    ws.onopen = (event) => {
      $jsonipc.ws = ws;
      let p = $jsonipc ({ '$id': null }, 'BeastSoundEngine/init_jsonipc', []);
      p.then (server_id => {
	if (!Number.isInteger (server_id) || !(server_id > 0))
	  throw Error ('Bse: failed to authenticate to BeastSoundEngine');
	// TODO: check that the return value is { '$id': <int>, '$class': 'Bse.Server' }
	server['$id'] = server_id;
	server['$promise'] = undefined;
	resolve (server);
      }, err => reject (Error ('Jsonipc:' + err.code + ': ' + err.message)));
    };
  });

// TODO: run BSE JS unit tests
function ebeast_test_bse_basics() {
  console.assert (Bse.server);
  const proj = Bse.server.create_project ('ebeast_test_bse_basics-A');
  console.assert (proj);
  console.assert (proj.get_name() == 'ebeast_test_bse_basics-A');
  console.assert (proj.get_prop ('uname') == 'ebeast_test_bse_basics-A');
  proj.set_prop ('uname', 'ebeast_test_bse_basics-B2');
  console.assert (proj.get_name() == 'ebeast_test_bse_basics-B2');
  console.assert (proj.get_prop ('uname') == 'ebeast_test_bse_basics-B2');
  let icon = proj.get_prop ('icon');
  console.assert (icon && icon.width == 0 && icon.height == 0);
  proj.set_prop ('icon', { width: 2, height: 2, pixels: [1, "", "3", 4] });
  icon = proj.get_prop ('icon');
  console.assert (icon && icon.width == 2 && icon.height == 2);
  console.assert (JSON.stringify (icon.pixels) == JSON.stringify ([1, 0, 1, 4]));
  console.log ("  COMPLETE  " + 'ebeast_test_bse_basics');
}
if (Bse.Project && Bse.Project.get_name) // FIXME: run unconditionally
  ebeast_test_bse_basics();
