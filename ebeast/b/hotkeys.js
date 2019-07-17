// GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html
'use strict';

// assert jQuery
console.assert ($ !== undefined);

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

/** Install a *hotkey* handler that executes $.click() and adds the .b-fakeactive CSS class.
 */
$.fn.click_hotkey = function (hotkey) {
  let self = this;
  $(document).add_hotkey (hotkey, function (/*event*/) {
    self.addClass ('b-fakeactive');
    Util.keyboard_click (self.get (0));
    setTimeout (function () {
      self.removeClass ('b-fakeactive');
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
	KeyCode.ENTER != event.keyCode && Util.is_navigation_key_code (event.keyCode))
      {
	$('#statusarea').text ('IGNORE-NAV: ' + event.keyCode + ' (' + document.activeElement.tagName + ')');
	return; // no navigation hotkey possible when a navigatable element has focus
      }
    const hotkeys = $(document).data()['hotkeys'];
    if (hotkeys === undefined)
      return;
    Shell.status ('HOTKEY: ' + event.keyCode + ' ' + event.which + ' ' + event.charCode +
		  ' (' + document.activeElement.tagName + ')');
    for (let key in hotkeys) {
      if (Util.match_key_event (event, key)) {
	const hotkey_callbacks = hotkeys[key];
	hotkey_callbacks.fire (event);
	return;
      }
    }
    if (Util.match_key_event (event, 'Enter') &&
	document.activeElement && document.activeElement != document.body)
      {
	event.preventDefault();
	Util.keyboard_click (document.activeElement);
      }
  }
});
