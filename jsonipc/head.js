// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

/// WebSocket handling coded needed for the Jsonipc wire marshalling
export const $jsonipc = {
  classes: {},
  observations: {},
  authresult: undefined,
  web_socket: null,
  counter: null,
  idmap: {},

  /// Open the Jsonipc websocket
  open (url, protocols) {
    if (this.web_socket)
      throw "$jsonipc: connection open";
    this.counter = 1000000 * Math.floor (100 + 899 * Math.random());
    this.idmap = {};
    this.web_socket = new WebSocket (url, protocols);
    this.web_socket.binaryType = 'arraybuffer';
    this.web_socket.onerror = (event) => { throw event; };
    this.web_socket.onmessage = this.socket_message.bind (this);
    const promise = new Promise (resolve => {
      this.web_socket.onopen = (event) => {
	const psend = this.send ('$jsonipc.initialize', []);
	psend.then (result => { this.authresult = result; resolve (this.authresult); });
      };
    });
    return promise;
  },

  /// Send a Jsonipc request
  send (methodname, args) {
    if (!this.web_socket)
      throw "$jsonipc: connection closed";
    const unwrap_args = (e, i, a) => {
      if (Array.isArray (e))
	e.forEach (unwrap_args);
      else if ('object' === typeof e)
	{
	  if (e.$id > 0)
	    a[i] = { '$id': e.$id }; // unwrap object
	  else
	    for (let key of Object.keys (e))
	      unwrap_args (e[key], key, e);
	}
    };
    args.forEach (unwrap_args);
    const request_id = ++this.counter;
    const jsondata = JSON.stringify ({
      id: request_id,
      method: methodname,
      params: args,
    });
    this.web_socket.send (jsondata);
    const wrap_args = (e, i, a) => {
      if (Array.isArray (e))
	e.forEach (wrap_args);
      else if (e && 'object' === typeof e)
	{
	  if (e.$class)
	    a[i] = wrap_arg (e);
	  else
	    for (let key of Object.keys (e))
	      wrap_args (e[key], key, e);
	}
    };
    const wrap_arg = (e) => {
      if (e && 'object' === typeof e && e.$class && e.$id)
	{
	  const JsClass = this.classes[e.$class];
	  if (JsClass)
	    return new JsClass (e.$id);
	}
      return e;
    };
    const promise = new Promise ((resolve, reject) => {
      this.idmap[request_id] = (msg) => {
	if (msg.error)
	  reject (msg.error);
	else
	  {
	    let r = msg.result;
	    if (Array.isArray (r))
	      r.forEach (wrap_args);
	    else
	      r = wrap_arg (r);
	    resolve (r);
	  }
      };
    });
    return promise;
  },

  /// Handle a Jsonipc message
  socket_message (event) {
    // Binary message
    if (event.data instanceof ArrayBuffer)
      {
	const handler = this.onbinary;
	if (handler)
	  handler (event.data);
	else
	  console.error ("Unhandled message event:", event);
	return;
      }
    // Text message
    const msg = JSON.parse (event.data);
    if (msg.id)
      {
	const handler = this.idmap[msg.id];
	delete this.idmap[msg.id];
	if (handler)
	  return handler (msg);
      }
    else if ("string" === typeof msg.method && Array.isArray (msg.params)) // notification
      {
	function array_flat_match (short, long) {
	  for (let i = 0; i < short.length; ++i)
	    if (short[i] != long[i])
	      return false;
	  return true;
	}
	const observers = this.observations[msg.method];
	if (observers)
	  for (let i = 0; i < observers.length; ++i)
	    if (array_flat_match (observers[i][0], msg.params))
	      observers[i][1].apply (null, msg.params.slice (observers[i][0].length));
	return;
      }
    console.error ("Unhandled message:", event.data);
  },

  /// Observe Jsonipc notifications
  observe (methodname, args, callback) {
    if (!this.observations[methodname])
      this.observations[methodname] = [];
    this.observations[methodname].push ([args, callback]);
  },

  /// Clear a previous `observe` handler
  unobserve (methodname, args) {
    const jsonargs = JSON.stringify (args);
    if (this.observations[methodname])
      for (const [i, pair] of this.observations[methodname].entries())
	if (JSON.stringify (pair[0]) == jsonargs)
	  {
	    this.observations[methodname].splice (i, 1); // erase at i
	    return true;
	  }
    return false;
  },
};

// ----- End of jsonipc/head.js -----
