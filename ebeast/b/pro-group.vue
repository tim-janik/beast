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
    display: flex; justify-content: flex-start;
    .b-pro-group-numeric {
      /*max-height: 2em;*/
    }
  }
</style>

<template>
  <b-hflex class="b-pro-group tabular-nums" :data-bubble="name" >
    <b-pro-input v-for="tp in ptypes" :key="tp.prop.$id" :prop="tp.prop"
		 :class="ptype_class (tp)" :labeled="false"
		 :readonly="readonly" />
  </b-hflex>
</template>

<script>

async function property_types (props) {
  props = await props;
  // fetch type of all properties
  const promises = [];
  for (const p of props)
    {
      promises.push (p.is_numeric());	// ①
      promises.push (p.hints()); 	// ②
    }
  const results = await Promise.all (promises);
  console.assert (results.length == 2 * props.length);
  // assign ptypes
  const ptypes = [];
  for (let i = 0; i < props.length; i++)
    {
      const type = {
	is_numeric: results[i * 2 + 0],	// ①
	hints:      results[i * 2 + 1],	// ②
	prop:	    props[i],
      };
      ptypes.push (type);
    }
  // return list
  return Object.freeze (ptypes); // [ { prop, is_numeric, hints }… ]
}

function pro_group_data () {
  const data = {
    ptypes:	{ default: [], getter: async c => property_types (this.props), },
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
