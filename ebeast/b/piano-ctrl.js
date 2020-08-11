// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
//import * as PianoRoll from "./piano-roll.mjs";

export const PIANO_OCTAVES = 11;
export const PIANO_KEYS = PIANO_OCTAVES * 12;
export const PPQN = 384;			// ticks per quarter note

export class PianoCtrl {
  constructor (piano_roll)
  {
    this.piano_roll = piano_roll;
  }
  dom_update()
  {
  }
  keydown (event)
  {
    const roll = this.piano_roll, msrc = roll.msrc;
    const LEFT = Util.KeyCode.LEFT, UP = Util.KeyCode.UP, RIGHT = Util.KeyCode.RIGHT, DOWN = Util.KeyCode.DOWN;
    const idx = find_note (roll.adata.pnotes, n => roll.adata.focus_noteid == n.id);
    let note = idx >= 0 ? roll.adata.pnotes[idx] : {};
    const big = 9e12; // assert: big * 1000 + 999 < Number.MAX_SAFE_INTEGER
    let nextdist = +Number.MAX_VALUE, nextid = -1, pred, score;
    switch (event.keyCode) {
      case RIGHT:
	if (idx < 0)
	  note = { id: -1, tick: -1, note: -1, duration: 0 };
	pred  = n => n.tick > note.tick || (n.tick == note.tick && n.key >= note.key);
	score = n => (n.tick - note.tick) * 1000 + n.key;
	// nextid = idx >= 0 && idx + 1 < roll.adata.pnotes.length ? roll.adata.pnotes[idx + 1].id : -1;
	break;
      case LEFT:
	if (idx < 0)
	  note = { id: -1, tick: +big, note: 1000, duration: 0 };
	pred  = n => n.tick < note.tick || (n.tick == note.tick && n.key <= note.key);
	score = n => (note.tick - n.tick) * 1000 + 1000 - n.key;
	// nextid = idx > 0 ? roll.adata.pnotes[idx - 1].id : -1;
	break;
      case DOWN:
	if (note.id)
	  {
	    msrc.change_note (note.id, note.tick, note.duration, Math.max (note.key - 1, 0), note.fine_tune, note.velocity);
	    event.preventDefault();
	  }
	break;
      case UP:
	if (note.id)
	  {
	    msrc.change_note (note.id, note.tick, note.duration, Math.min (note.key + 1, PIANO_KEYS - 1), note.fine_tune, note.velocity);
	    event.preventDefault();
	  }
	break;
    }
    if (note.id && pred && score)
      {
	roll.adata.pnotes.forEach (n => {
	  if (n.id != note.id && pred (n))
	    {
	      const dist = score (n);
	      if (dist < nextdist)
		{
		  nextdist = dist;
		  nextid = n.id;
		}
	    }
	});
      }
    if (nextid > 0)
      {
	roll.adata.focus_noteid = nextid;
	event.preventDefault();
      }
  }
  async notes_click (event) {
    const roll = this.piano_roll, msrc = roll.msrc, layout = roll.layout;
    if (!msrc)
      return;
    event.preventDefault();
    const tick = layout.tick_from_x (event.offsetX);
    const midinote = layout.midinote_from_y (event.offsetY);
    const idx = find_note (roll.adata.pnotes,
			   n => tick >= n.tick && tick < n.tick + n.duration && n.key == midinote);
    if (idx >= 0 && roll.pianotool == 'E')
      {
	let note = roll.adata.pnotes[idx];
	msrc.change_note (note.id, note.tick, 0, note.key, note.fine_tune, note.velocity);
      }
    else if (idx >= 0)
      {
	const note = roll.adata.pnotes[idx];
	roll.adata.focus_noteid = note.id;
      }
    else if (roll.pianotool == 'P')
      {
	roll.adata.focus_noteid = undefined;
	const note_id = await msrc.change_note (-1, tick, PPQN * 0.25, midinote, 0, 1);
	if (roll.adata.focus_noteid === undefined)
	  roll.adata.focus_noteid = note_id;
      }
  }
}

function find_note (allnotes, predicate) {
  for (let i = 0; i < allnotes.length; ++i)
    {
      const n = allnotes[i];
      if (predicate (n))
	return i;
    }
  return -1;
}
