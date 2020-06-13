// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"use strict";

// -------- Javascript BSE API (auto-generated) --------
import * as Bse from './bseapi_jsonipc.js';
export * from './bseapi_jsonipc.js';
// -------- Javascript BSE API (auto-generated) --------

// == Object.on ==
Bse.ObjectIface.prototype.on = function (eventselector, callback) {
  const connection = {
    active: true,
    promise: null,
    id: 0,
  };
  const connect = async () => {
    connection.id = await Bse.$jsonipc.send ("Bse/EventHub/connect", [ this, eventselector ]);
    if (connection.id)
      {
	const BseObjectIface_dispatchevent_ = function (...args) {
	  if (connection.active)
	    callback.call (this, ...args);
	}; // this wrapper function needs an accurate name for backtraces
	Bse.$jsonipc.observe ("Bse/EventHub/event", [ connection.id ], BseObjectIface_dispatchevent_);
      }
  };
  connection.promise = connect();
  const disconnect = async () => {
    if (!connection.active)
      return;
    connection.active = false; // deactivate *before* first await
    await connection.promise;
    if (connection.id)
      {
	Bse.$jsonipc.unobserve ("Bse/EventHub/event", [ connection.id ]);
	await Bse.$jsonipc.send ("Bse/EventHub/disconnect", [ connection.id ]);
      }
  };
  // after calling disconnect(), callback() is never executed
  return disconnect;
};

// Connect and fetch Bse.server on startup
export let server = { $promise: null };
server.$promise = new Promise ((resolve, reject) => {
  const hostport = window.location.href.replace (/.*:\/\/([^\/]+).*/, '$1');
  const ws_url = 'ws://' + hostport;
  const url = new URL (window.location);
  const promise = Bse.$jsonipc.open (ws_url, url.searchParams.get ('subprotocol'));
  promise.then (
    result => {
      if (result instanceof Bse.Server)
	{
	  server = new Bse.Server (result['$id']);
	  resolve (server);
	  if (server == "unimplemented") // FIXME: run unit tests again
	    ebeast_test_bse_basics();
	  return server;
	}
      throw Error ('Bse: failed to connect to BeastSoundEngine');
    },
    error => reject (error));
});
Object.freeze (server);

// Handle binary messages for shm updates
import * as Util from './utilities.js';
Bse.$jsonipc.onbinary = Util.shm_receive;

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
