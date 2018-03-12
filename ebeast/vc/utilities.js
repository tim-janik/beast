// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
'use strict';

/** VueifyObject - turn a regular object into a Vue instance.
 * The *object* passed in is used as the Vue `data` object. Properties
 * with a getter (and possibly setter) are turned into Vue `computed`
 * properties, methods are carried over as `methods` on the Vue() instance.
 */
exports.VueifyObject = function (object = {}, vue_options = {}) {
  let voptions = Object.assign ({}, vue_options);
  voptions.methods = vue_options.methods || {};
  voptions.computed = vue_options.computed || {};
  voptions.data = voptions.data || object;
  const proto = object.__proto__;
  for (const pname of Object.getOwnPropertyNames (proto)) {
    if (typeof proto[pname] == 'function' && pname != 'constructor')
      voptions.methods[pname] = proto[pname];
    else
      {
	const pd = Object.getOwnPropertyDescriptor (proto, pname);
	if (pd.get)
	  voptions.computed[pname] = pd;
      }
  }
  return new Vue (voptions);
};

/** Copy PropertyDescriptors from source to target, optionally binding handlers against closure. */
exports.clone_descriptors = (target, source, closure) => {
  // See also: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign completeAssign()
  const descriptors = Object.getOwnPropertyNames (source).reduce ((descriptors, pname) => {
    const pd = Object.getOwnPropertyDescriptor (source, pname);
    if (pname != 'constructor')
      {
	if (closure)
	  {
	    if (pd.get)
	      pd.get = pd.get.bind (closure);
	    if (pd.set)
	      pd.set = pd.set.bind (closure);
	    if (typeof pd.value == 'function')
	      pd.value = pd.value.bind (closure);
	  }
	descriptors[pname] = pd;
      }
    return descriptors;
  }, {});
  Object.defineProperties (target, descriptors);
  // in constrast to Object.assign, we ignore getOwnPropertySymbols here
  return target;
  // Usage: clone_descriptors (window, fakewin.__proto__, fakewin);
};
