// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"use strict";

// -------- Javascript BSE API (auto-generated) --------
import * as Bse from './bseapi_jsonipc.js';
export * from './bseapi_jsonipc.js';
// -------- Javascript BSE API (auto-generated) --------

// == Object.on ==
Bse.ObjectIface.prototype.on = async function (eventselector, callback) {
  const connection_id = await Bse.$jsonipc.send ("Bse/EventHub/connect", [ this, eventselector ]);
  if (connection_id)
    {
      Bse.$jsonipc.observe ("Bse/EventHub/event", [ connection_id ], callback);
      return connection_id;
    }
  return false;
};

// == Object.off ==
Bse.ObjectIface.prototype.off = async function (connection_id) {
  if (connection_id)
    {
      Bse.$jsonipc.unobserve ("Bse/EventHub/event", [ connection_id ]);
      return await Bse.$jsonipc.send ("Bse/EventHub/disconnect", [ connection_id ]);
    }
  return false;
};

// Connect and fetch Bse.server on startup
export const server = new Bse.Server (undefined);
Object.defineProperty (server, '$promise', { writable: true });
server['$promise'] = new Promise ((resolve, reject) => {
  const promise = Bse.$jsonipc.open ("ws://localhost:27239/", 'auth123');
  promise.then (
    result => {
      if (result instanceof Bse.Server)
	{
	  Object.defineProperty (server, '$id', { value: result['$id'], configurable: true });
	  resolve (server);
	  if (server == "unimplemented") // FIXME: run unit tests again
	    ebeast_test_bse_basics();
	  return server;
	}
      throw Error ('Bse: failed to connect to BeastSoundEngine');
    },
    error => reject (error));
});

// Define BSE JS unit tests
async function ebeast_test_bse_basics() {
  console.assert (server && server.$id > 0);
  const proj = await server.create_project ('ebeast_test_bse_basics-A');
  console.assert (proj);
  console.assert ('ebeast_test_bse_basics-A' == await proj.get_name());
  console.assert ('ebeast_test_bse_basics-A' == await proj.get_prop ('uname'));
  await proj.set_prop ('uname', 'ebeast_test_bse_basics-B2');
  console.assert ('ebeast_test_bse_basics-B2' == await proj.get_name());
  console.assert ('ebeast_test_bse_basics-B2' == await proj.get_prop ('uname'));
  let icon = await proj.get_prop ('icon');
  console.assert (icon && icon.width == 0 && icon.height == 0);
  await proj.set_prop ('icon', { width: 2, height: 2, pixels: [1, "", "3", 4] });
  icon = await proj.get_prop ('icon');
  console.assert (icon && icon.width == 2 && icon.height == 2);
  console.assert (JSON.stringify (icon.pixels) == JSON.stringify ([1, 0, 1, 4]));
  console.log ("  COMPLETE  " + 'ebeast_test_bse_basics');
}
