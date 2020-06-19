<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-DATABUBBLE
  A mechanism to display data-bubble="" tooltip popups on mouse hover.
</docs>

<style lang="scss">
  @import 'mixins.scss';

  /* Bubble color setup */
  $b-data-bubble-hue: 52;
  $b-data-bubble-fg:  color($b-data-bubble-hue, 1%);
  $b-data-bubble-bg:  color($b-data-bubble-hue, 90%);
  $b-data-bubble-bg2: lighter($b-data-bubble-bg, 3%);
  $b-data-bubble-br:  $b-data-bubble-bg2;

  /* Tooltips via CSS, using the data-bubble="" attribute */
  .b-data-bubble {
    position: absolute; z-index: 9999999999; pointer-events: none;
    display: flex; max-width: 95vw;
    margin: 0; padding: 0 0 8px 0; /* make room for triangle: 5px + 3px */
    transition: visibility 0.1s, opacity 0.09s ease-in-out;
    visibility: hidden; opacity: 0;

    .b-data-bubble-inner {
      display: flex; overflow: hidden; position: relative;
      white-space: normal; margin: 0;
      max-width: 40em; border-radius: 3px;
      // border: dppx(2) solid lighter($b-data-bubble-bg2, 5%);
      box-shadow:
      0 0 0 1px change-color($b-data-bubble-br, $alpha: 0.8),
      0px 0px 2px 1px black;
      color: $b-data-bubble-fg; padding: 2px 3px;
      background: $b-data-bubble-bg;
      background-image: chromatic-gradient(to bottom right, $b-data-bubble-bg, $b-data-bubble-bg2);
      font-variant-numeric: tabular-nums;
    }
    &.b-data-bubble-visible {
      visibility: visible; opacity: 0.95;
    }
    /* Triangle acting as bubble pointer */
    &:after {
      position: absolute; bottom: 5px - 2; /* room below triangle: 5px */
      left: calc(50% - 5px); width: 0; height: 0; content: "";
      border-top: 5px solid $b-data-bubble-br;
      border-left: 5px solid transparent;
      border-right: 5px solid transparent;
    }

    /* markdown styling for data-bubble */
    .b-markdown-it-outer {
      @include b-markdown-it-inlined;
    }
  }
</style>

<script>
const UNSET = Symbol ('UNSET');

/** A mechanism to display data-bubble="" tooltip popups */
class DataBubbleImpl {
  constructor() {
    // create one toplevel div.data-bubble element to deal with all popups
    this.bubble = document.createElement ('div');
    this.bubble.classList.add ('b-data-bubble');
    this.bubblediv = document.createElement ('div');
    this.bubblediv.classList.add ('b-data-bubble-inner');
    this.bubble.appendChild (this.bubblediv);
    document.body.appendChild (this.bubble);
    this.current = null; // current element showing a data-bubble
    this.stack = []; // element stack to force bubble
    this.lasttext = "";
    this.last_event = null;
    this.buttonsdown = 0;
    this.coords = {};
    // milliseconds to wait to detect idle time after mouse moves
    const IDLE_DELAY = 115;
    // trigger popup handling after mouse rests
    this.restart_bubble_timer = Util.debounce (() => this.check_showtime (true),
					       { restart: true, wait: IDLE_DELAY });
    this.debounced_check = Util.debounce (this.check_showtime);
    this.queue_update = Util.debounce (this.update_now);
    const recheck_event = (ev, newmove) => {
      this.last_event = ev;
      this.debounced_check();
    };
    document.addEventListener ("mousemove", recheck_event, { capture: true, passive: true });
    document.addEventListener ("mousedown", recheck_event, { capture: true, passive: true });
    document.addEventListener ("mouseleave",
			       ev => (ev.target === this.current) && this.debounced_check(),
			       { capture: true, passive: true });
    this.resizeob = new ResizeObserver (() => !!this.current && this.debounced_check());
  }
  check_showtime (showtime = false) {
    if (this.last_event)
      {
	const coords = { x: this.last_event.screenX, y: this.last_event.screenY };
	this.buttonsdown = !!this.last_event.buttons;
	if (!this.buttonsdown && !this.stack.length &&
	    !Util.equals_recursively (coords, this.coords) &&
	    this.last_event.type === "mousemove")
	  {
	    this.restart_bubble_timer();
	    for (let i = 0; i < App.onpointermoves.length; i += 1)
	      App.onpointermoves[i] (this.last_event);
	  }
	this.coords = coords; // needed to ignore 0-distance moves
	this.last_event = null;
      }
    if (this.stack.length) // stack takes precedence over events
      {
	const next = this.stack[0];
	if (next != this.current)
	  {
	    this.hide();
	    this.restart_bubble_timer.cancel();
	  }
	if (!this.current)
	  this.show (next);
	return;
      }
    if (this.buttonsdown)
      {
	this.hide();
	this.restart_bubble_timer.cancel();
	return;
      }
    if (!showtime && !this.current)
      return;
    const els = document.body.querySelectorAll ('*:hover[data-bubble]');
    const next = els.length ? els[els.length - 1] : null;
    if (next != this.current)
      this.hide();
    if (next && showtime && !this.current)
      this.show (next);
  }
  hide() {
    if (this.current)
      {
	delete this.current.data_bubble_active;
	this.current = null;
	this.resizeob.disconnect();
	this.bubble.classList.remove ('b-data-bubble-visible');
      }
    // keep textContent for fade-outs
  }
  update_now() {
    if (this.current)
      {
	let cbtext;
	if (this.current.data_bubble_callback)
	  {
	    cbtext = this.current.data_bubble_callback();
	    this.current.setAttribute ('data-bubble', cbtext);
	  }
	const newtext = cbtext || this.current.getAttribute ('data-bubble');
	if (!newtext)
	  this.hide();
	else if (newtext != this.lasttext)
	  {
	    this.lasttext = newtext;
	    Util.markdown_to_html (this.bubblediv, this.lasttext);
	  }
      }
  }
  show (element) {
    console.assert (!this.current);
    this.current = element;
    this.current.data_bubble_active = true;
    this.resizeob.observe (this.current);
    this.bubble.classList.add ('b-data-bubble-visible');
    this.update_now(); // might hide()
    if (this.current) // resizing
      {
	document.body.appendChild (this.bubble); // restack the bubble
	const viewport = {
	  width:  Math.max (document.documentElement.clientWidth || 0, window.innerWidth || 0),
	  height: Math.max (document.documentElement.clientHeight || 0, window.innerHeight || 0),
	};
	// request ideal layout
	const r = this.current.getBoundingClientRect();
	this.bubble.style.top = '0px';
	this.bubble.style.right = '0px';
	let s = this.bubble.getBoundingClientRect();
	let top = Math.max (0, r.y - s.height);
	let right = Math.max (0, viewport.width - (r.x + r.width / 2 + s.width / 2));
	this.bubble.style.top = top + 'px';
	this.bubble.style.right = right + 'px';
	s = this.bubble.getBoundingClientRect();
	// constrain layout
	if (s.left < 0)
	  {
	    right += s.left;
	    right = Math.max (0, right);
	    this.bubble.style.right = right + 'px';
	  }
      }
  }
}

class DataBubbleIface {
  constructor() {
    this.data_bubble = new DataBubbleImpl();
  }
  /// Set the `data-bubble` attribute of `element` to `text` or force its callback
  update (element, text = UNSET) {
    if (text !== UNSET && !element.data_bubble_callback)
      {
	if (text)
	  element.setAttribute ('data-bubble', text);
	else
	  element.removeAttribute ('data-bubble');
      }
    this.data_bubble.queue_update();
  }
  /// Assign a callback function to fetch the `data-bubble` attribute of `element`
  callback (element, callback) {
    element.data_bubble_callback = callback;
    if (callback)
      element.setAttribute ('data-bubble', "");	// [data-bubble] selector needs existing attribute
    this.data_bubble.queue_update();
  }
  /// Force `data-bubble` to be shown for `element`
  force (element) {
    this.data_bubble.stack.unshift (element);
    this.data_bubble.debounced_check();
  }
  /// Reset the `data-bubble` attribute, its callback and cancel a forced bubble
  clear (element) {
    if (this.data_bubble.stack.length)
      Util.array_remove (this.data_bubble.stack, element);
    if (element.data_bubble_active)
      this.data_bubble.hide();
    if (element.data_bubble_callback)
      element.data_bubble_callback = undefined;
    element.removeAttribute ('data-bubble');
    this.data_bubble.debounced_check();
  }
}
console.assert (App.data_bubble == null);
App.data_bubble = new DataBubbleIface();

export default {};

</script>
