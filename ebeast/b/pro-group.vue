<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PRO-GROUP
  A property group is a collection of properties.
  ## Properties:
  *name*
  : Group name.
  *props*
  : Contains the properties.
  *readonly*
  : Make this component non editable for the user.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-pro-group        {
    display: grid;
    padding: 5px; grid-gap: 5px;
    justify-content: flex-start;
    border-radius: $b-device-radius;
    margin: 0 0 3px 3px;
    background: $b-device-area1;
    &:nth-child(2n) {
      background: $b-device-area2;
    }
    .b-pro-group-title {
      justify-self: center;
    }
    .b-pro-group-vprop {
      align-items: center;
    }
    .b-pro-group-nick {
      font-size: 90%;
    }
    .b-pro-group-numeric {
      /*max-height: 2em;*/
    }
  }
</style>

<template>
  <div class="b-pro-group tabular-nums" >
    <span class="b-pro-group-title" :style="`grid-column: 1 / ${maxcols}`" > {{ name }} </span>
    <b-vflex class="b-pro-group-vprop" v-for="tp in ptypes" :key="tp.prop.$id" :style="tp.style" >
      <b-pro-input :prop="tp.prop"
		   :class="ptype_class (tp)" :labeled="false"
		   :readonly="readonly" />
      <span class="b-pro-group-nick" >
	{{ tp.nick }}
      </span>
    </b-vflex>
  </div>
</template>

<script>
function prop_visible (nick, is_numeric, hints) {
  hints = ':' + hints + ':';
  if (hints.search (/:G:/) < 0)
    return false;
  return true;
}

function prop_row (idx, count) {
  let nrows = 1;
  if (count > 8)
    nrows = 2;
  if (count > 16)
    nrows = 3;
  if (count > 24)
    nrows = 4;
  const run = Math.ceil (count / nrows);
  let r = Math.trunc (idx / run);
  r += 2; // first row is the title
  return r;
}

async function property_types (props) {
  props = await props;
  // fetch type of all properties
  const promises = [];
  for (const p of props)
    {
      promises.push (p.is_numeric());		// ①
      promises.push (p.hints()); 		// ②
      promises.push (p.nick()); 		// ③
    }
  const results = await Promise.all (promises);
  console.assert (results.length == 3 * props.length);
  // assign ptypes, count UI elements
  const ptypes = [];
  for (let i = 0; i < props.length; i++)
    {
      const is_numeric = results[i * 3 + 0];	// ①
      const hints = results[i * 3 + 1];		// ②
      const nick = results[i * 3 + 2];		// ③
      const visible = prop_visible (nick, is_numeric, hints);
      if (!visible)
	continue;
      const type = {
	is_numeric, hints, nick,
	prop:	    props[i],
	style:      '',
      };
      ptypes.push (type);
    }
  // layout visible props
  const cols = {};
  for (let i = 0; i < ptypes.length; i++)
    {
      const type = ptypes[i];
      const row = prop_row (i, ptypes.length);
      const c = cols[row] || 1;
      cols[row] = c + 1;
      cols.max = Math.max (cols.max | 0, cols[row]);
      type.style = `grid-row: ${row}; grid-column: ${c};`;
    }
  // return list
  this.maxcols = cols.max;
  return Object.freeze (ptypes); // [ { prop, is_numeric, hints }… ]
}

function pro_group_data () {
  const data = {
    ptypes:	{ default: [], getter: async c => property_types.call (this, this.props), },
    maxcols:    { default: 1, },
  };
  return this.observable_from_getters (data, () => this.props);
}

export default {
  name: 'b-pro-group',
  props: {
    name:	{ default: '', },
    props:      { default: () => [], },
    readonly:	{ type: Boolean, default: false, },
  },
  data() { return pro_group_data.call (this); },
  computed: {
    type () {
      return this.is_numeric ? 'knob' : '';
    },
  },
  methods: {
    ptype_class (ptype) {
      const hints = ':' + ptype.hints + ':';
      let c = '';
      if (ptype.is_numeric && hints.search (/:big:/) < 0) // FIXME: test "big" hint
	c += 'b-pro-group-numeric';
      return c;
    },
    dom_update() {
    },
  },
};
</script>
