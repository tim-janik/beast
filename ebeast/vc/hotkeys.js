// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// == Keys ==
const KeyCode = {
  BACKSPACE: 8, TAB: 9, ENTER: 13, RETURN: 13, CAPITAL: 20, CAPSLOCK: 20, ESC: 27, ESCAPE: 27, SPACE: 32,
  PAGEUP: 33, PAGEDOWN: 34, END: 35, HOME: 36, LEFT: 37, UP: 38, RIGHT: 39, DOWN: 40, PRINTSCREEN: 44, INSERT: 45, DELETE: 46,
  F1: 112, F2: 113, F3: 114, F4: 115, F5: 116, F6: 117, F7: 118, F8: 119, F9: 120, F10: 121, F11: 122, F12: 123,
  F13: 124, F14: 125, F15: 126, F16: 127, F17: 128, F18: 129, F19: 130, F20: 131, F21: 132, F22: 133, F23: 134, F24: 135,
  BROWSERBACK: 166, BROWSERFORWARD: 167, PLUS: 187/*FIXME*/, MINUS: 189/*FIXME*/, PAUSE: 230, ALTGR: 255,
  VOLUMEMUTE: 173, VOLUMEDOWN: 174, VOLUMEUP: 175, MEDIANEXTTRACK: 176, MEDIAPREVIOUSTRACK: 177, MEDIASTOP: 178, MEDIAPLAYPAUSE: 179,
};
exports.KeyCode = KeyCode;

// == Hotkey handling ==
let split_hotkey = function (hotkey) {
  let rex = new RegExp (/\s*[+]\s*/); // Split 'Shift+Ctrl+Alt+Meta+SPACE'
  return hotkey.toLowerCase().split (rex);
};

/** Execute *callbacks* when *hotkey* is pressed inside *document*.
 */
$.fn.add_hotkey = function (hotkey, callbacks) {
  let keyname = split_hotkey (hotkey).sort().join ('+');
  let hotkeys = $(document).data()['hotkeys'];
  if (hotkeys === undefined)
    hotkeys = $(document).data()['hotkeys'] = {};
  let hotkey_callbacks = hotkeys[keyname];
  if (hotkey_callbacks === undefined)
    hotkey_callbacks = hotkeys[keyname] = $.Callbacks ('unique stopOnFalse');
  hotkey_callbacks.add (callbacks);
};

/** Remove *callbacks* previously installed for *hotkey*.
 */
$.fn.remove_hotkey = function (hotkey, callbacks) {
  const keyname = split_hotkey (hotkey).sort().join ('+');
  const hotkeys = $(document).data()['hotkeys'];
  if (hotkeys === undefined)
    return;
  const hotkey_callbacks = hotkeys[keyname];
  if (hotkey_callbacks === undefined)
    return;
  hotkey_callbacks.remove (callbacks);
};

/** Install a *hotkey* handler that executes $.click() and adds the .vc-fakeactive CSS class.
 */
$.fn.click_hotkey = function (hotkey) {
  let self = this;
  $(document).add_hotkey (hotkey, function (/*event*/) {
    self.addClass ('vc-fakeactive');
    self.click();
    setTimeout (function () {
      self.removeClass ('vc-fakeactive');
    }, 45);
  });
  return self;
};

// Install *document* key handler for all installed hotkeys.
$(document).keydown (function (event) {
  const textarea_types = [ 'date', 'datetime-local', 'email', 'month', 'number', 'password', 'search',
			 'tel', 'text', 'textarea', 'time', 'url', 'week', ];
  const navigation_types = [ 'button', 'checkbox', 'color', 'file', 'hidden', 'image', 'radio', 'range', 'submit', 'reset', ];
  if (document.activeElement) {
    if ($.inArray (document.activeElement.type, textarea_types) >= 0) {
      $('#statusarea').text ('IGNORE-TEXT: ' + event.keyCode + ' (' + document.activeElement.type + ')');
      return; // no hotkey activation possible when in text input
    }
    if ($.inArray (document.activeElement.type, navigation_types) >= 0 &&
	$.inArray (event.keyCode, match_hotkey_event.navigation_keys) >= 0) {
      $('#statusarea').text ('IGNORE-NAV: ' + event.keyCode + ' (' + document.activeElement.tagName + ')');
      return; // no navigation hotkey possible when a navigatable element has focu
    }
    const hotkeys = $(document).data()['hotkeys'];
    if (hotkeys === undefined)
      return;
    App.status ('HOTKEY: ' + event.keyCode + ' ' + event.which + ' ' + event.charCode +
		' (' + document.activeElement.tagName + ')');
    for (let key in hotkeys) {
      if (match_hotkey_event (event, key)) {
	const hotkey_callbacks = hotkeys[key];
	hotkey_callbacks.fire (event);
	return;
      }
    }
  }
});

let match_hotkey_event = function (event, keyname) {
  // SEE: http://unixpapa.com/js/key.html & https://developer.mozilla.org/en-US/docs/Mozilla/Gecko/Gecko_keypress_event
  const parts = split_hotkey (keyname);
  const char = String.fromCharCode (event.which || event.keyCode);
  let need_meta = 0, need_alt = 0, need_ctrl = 0, need_shift = 0;
  for (let i = 0; i < parts.length; i++) {
    // collect meta keys
    switch (parts[i]) {
    case 'cmd': case 'command':
    case 'super': case 'meta':	  need_meta  = 1; continue;
    case 'option': case 'alt':	  need_alt   = 1; continue;
    case 'control': case 'ctrl':  need_ctrl  = 1; continue;
    case 'shift':		  need_shift = 1; continue;
    }
    // match named keys (special)
    const key_val = KeyCode[parts[i].toUpperCase()];
    if (key_val !== undefined && char.length == 1 && key_val == char.charCodeAt(0))
      continue;
    // match characters
    if (char.toLowerCase() == parts[i])
      continue;
    // failed to match
    return false;
  }
  // ignore shift for case insensitive characters (except for navigations)
  if (char.toLowerCase() == char.toUpperCase() &&
      $.inArray (event.keyCode, match_hotkey_event.navigation_keys) == -1)
    need_shift = 2;
  // match meta keys
  if (need_meta   != 0 + event.metaKey ||
      need_alt    != 0 + event.altKey ||
      need_ctrl   != 0 + event.ctrlKey ||
      (need_shift != 0 + event.shiftKey && need_shift != 2))
    return false;
  return true;
};

match_hotkey_event.navigation_keys = [
  KeyCode.UP, KeyCode.DOWN, KeyCode.LEFT, KeyCode.RIGHT,
  KeyCode.TAB, KeyCode.SPACE, KeyCode.ENTER /*13*/, 10 /*LINEFEED*/,
  KeyCode.PAGE_UP, KeyCode.PAGE_DOWN, KeyCode.HOME, KeyCode.END,
  93 /*CONTEXT_MENU*/, KeyCode.ESCAPE, ];
