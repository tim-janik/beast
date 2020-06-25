<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-PRO-GROUP
  A property group is a collection of properties.
  ## Properties:
  *name*
  : Group name.
  *props*
  : List of properties with cached information and layout rows.
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
    .b-pro-group-big {
      /*max-height: 2em;*/
    }
  }
</style>

<template>
  <div class="b-pro-group tabular-nums" >
    <span class="b-pro-group-title" :style="`grid-column: 1 / ${maxcols}`" > {{ name }} </span>
    <b-vflex class="b-pro-group-vprop" v-for="p in lprops" :key="p.$id" :style="p.style_" >
      <b-pro-input :prop="p"
		   :class="prop_class (p)" :labeled="false"
		   :readonly="readonly" />
      <span class="b-pro-group-nick" >
	{{ p.nick_ }}
      </span>
    </b-vflex>
  </div>
</template>

<script>

async function assign_layout_cols (props) {
  props = await props;
  const cols = {};
  // restart column count per new layout row
  for (let i = 0; i < props.length; i++)
    {
      const p = props[i];
      const row = p.lrow_ + 2; // add offset for title row
      const c = cols[row] || 1;
      cols[row] = c + 1;
      cols.max = Math.max (cols.max | 0, cols[row]);
      p.style_ = `grid-row: ${row}; grid-column: ${c};`;
    }
  // return list
  this.maxcols = cols.max;
  return Object.freeze (props);
}

function pro_group_data () {
  const data = {
    lprops:	{ default: [], getter: async c => assign_layout_cols.call (this, this.props), },
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
    prop_class (prop) {
      const hints = ':' + prop.hints_ + ':';
      let c = '';
      if (prop.is_numeric_ && hints.search (/:big:/) < 0) // FIXME: test "big" hint
	c += 'b-pro-group-big';
      return c;
    },
    dom_update() {
    },
  },
};
</script>
