<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-EXAMPLE
  A collection of Vue documentation pointers and notes on idiomatic use.
  ## Attr:
  Attributes set on a component are by default transferred to its root element, see:
  [inheritAttrs: false](https://vuejs.org/v2/guide/components-props.html#Disabling-Attribute-Inheritance).
  The component can access the attributes through `this.$attrs`, this can e.g. be used for initial prop values.
  ## Props:
  Parents can set values on child component via the `props` array, the child itself must not change its `props`, see:
  [Passing Data to Child Components with Props](https://vuejs.org/v2/guide/components.html#Passing-Data-to-Child-Components-with-Props)
  ## Property:
  Child components can contain computed properties that are get/set-able from the parent via the `computed` arraay, see:
  [Computed Properties and Watchers](https://vuejs.org/v2/guide/computed.html)
  ## Slots:
  Child components can embed parent DOM contents via the `v-slot` mechanism, see:
  [Content Distribution with Slots](https://vuejs.org/v2/guide/components-slots.html)
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-example		{ /* component specific CSS */ }
  .b-example-slot	{ /* all classes must use component prefixes */ }
</style>

<template>
  <span class="b-example">
    <slot class="b-example-slot"></slot>
  </span>
</template>

<script>
export default {
  name: 'b-example',
  props: { valuefromparent: String, },
  computed: {
    sample:	{ set (v) { this.sample_ = v; },
		  get ()  { return this.sample_; }, },
  },
  watch: {
    sample:	function (val, old) { console.log ('sample changed:', val, old); },
  },
  data_tmpl: {	// Create per-component `data` via the global `Util.vue_mixins.data_tmpl`
    sample_:	'Some default',
  },
  methods: {	// Custom methods
    dom_update() {},	// See Util.vue_mixins.dom_updates
  },
};
</script>
