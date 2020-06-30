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
  .b-pro-group {
    display: flex;
    padding: 5px;
    justify-content: space-evenly;
    border-radius: $b-device-radius;
    background: $b-device-area1;
    &:nth-child(2n) {
      background: $b-device-area2;
    }
    .b-pro-group-title {
      display: flex;
      justify-content: center;
      flex-grow: 1;
    }
    .b-pro-group-row {
      justify-content: space-evenly;
      .b-pro-group-vprop {
	align-items: center;
	.b-pro-group-nick {
	  font-size: 90%;
	}
      }
      //* emulate: .b-pro-group-row { gap: ...; } */
      & > *:not(:last-child) { margin-right: 7px; }
    }
    //* emulate: .b-pro-group { gap: ...; } */
    & > *:not(:last-child) { margin-bottom: 5px; }
    .b-pro-group-big {
      /*max-height: 2em;*/
    }
  }
</style>

<template>
  <b-vflex class="b-pro-group tabular-nums" >
    <span class="b-pro-group-title" > {{ name }} </span>
    <b-hflex class="b-pro-group-row" v-for="row in proprows" :key="row.index" >
      <b-vflex class="b-pro-group-vprop" v-for="prop in row" :key="prop.$id" :class="prop_class (prop)" >
	<b-pro-input class="b-pro-group-input" :prop="prop" :labeled="false" :readonly="readonly" />
	<span class="b-pro-group-nick" > {{ prop.nick_ }} </span>
      </b-vflex>
    </b-hflex>
  </b-vflex>
</template>

<script>

async function assign_layout_rows (props) {
  props = await props;
  // split properties into rows, according to lrow_
  const rows = [];
  for (const prop of props)
    {
      console.assert ('number' == typeof prop.lrow_);
      if (!rows[prop.lrow_])
	{
	  rows[prop.lrow_] = [];
	  rows[prop.lrow_].index = prop.lrow_;
	}
      rows[prop.lrow_].push (prop);
    }
  // return list
  return Object.freeze (rows);
}

function pro_group_data () {
  const data = {
    proprows:	{ default: [], getter: async c => assign_layout_rows.call (this, this.props), },
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
