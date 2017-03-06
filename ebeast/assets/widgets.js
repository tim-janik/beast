'use strict';

// == Keys ==
module.exports.KeyCode = {
  BACKSPACE: 8, TAB: 9, ENTER: 13, RETURN: 13, CAPITAL: 20, CAPSLOCK: 20, ESC: 27, ESCAPE: 27, SPACE: 32,
  PAGEUP: 33, PAGEDOWN: 34, END: 35, HOME: 36, LEFT: 37, UP: 38, RIGHT: 39, DOWN: 40, PRINTSCREEN: 44, INSERT: 45, DELETE: 46,
  F1: 112, F2: 113, F3: 114, F4: 115, F5: 116, F6: 117, F7: 118, F8: 119, F9: 120, F10: 121, F11: 122, F12: 123,
  F13: 124, F14: 125, F15: 126, F16: 127, F17: 128, F18: 129, F19: 130, F20: 131, F21: 132, F22: 133, F23: 134, F24: 135,
  BROWSERBACK: 166, BROWSERFORWARD: 167, PLUS: 187/*FIXME*/, MINUS: 189/*FIXME*/, PAUSE: 230, ALTGR: 255,
  VOLUMEMUTE: 173, VOLUMEDOWN: 174, VOLUMEUP: 175, MEDIANEXTTRACK: 176, MEDIAPREVIOUSTRACK: 177, MEDIASTOP: 178, MEDIAPLAYPAUSE: 179,
};

// == Utility Functions ==

/// Wrap @a callback so it's only called once per AnimationFrame.
function create_animation_callback (callback) {
  return function() {
    if (true === !callback.__create_animation_callback__pending__) {
      const request_id = requestAnimationFrame (function (now_ms) {
	callback.__create_animation_callback__pending__ = undefined;
	callback (now_ms);
      });
      callback.__create_animation_callback__pending__ = request_id;
    }
  };
}
module.exports.create_animation_callback = create_animation_callback;

/// Enqueue @a callback to be executed once at the next AnimationFrame.
function queue_animation_callback (callback) {
  const queue_callback = create_animation_callback (callback);
  queue_callback();
}
module.exports.queue_animation_callback = queue_animation_callback;
