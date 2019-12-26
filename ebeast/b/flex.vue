<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-HFLEX
  A [flex](https://www.w3.org/TR/css-flexbox-1/#flex-containers) container for horizontal layouting
  (or vertical in the case of `B-VFLEX`).
  ## Props:
  *inline*
  : Use 'display: inline-flex' as layout mode, e.g. to integrate this container into text flow.
  *reverse*
  : Reverse the layout direction, i.e. layout from bottom → top, or right → left.
  *start*
  : Children are packed towards the start of the flex direction:
  *end*
  : Children are packed towards the end of the flex direction:
  *center*
  : Children are packed towards the center of the flex direction:
  *space-between*
  : Additional space is distributed between the children.
  *space-around*
  : Additional space is evenly distributed between the children and half a spacing around the edges.
  *space-evenly*
  : Children will have additional space evenly distributed around them and the edges.
  *grow0* *grow1* *grow2* *grow3* *grow4* *grow5* *grow6* *grow7* *grow8* *grow9*
  : Grow space for this element if additional space is available with the given strength (0 = none).
  *shrink0* *shrink1* *shrink2* *shrink3* *shrink4* *shrink5* *shrink6* *shrink7* *shrink8* *shrink9*
  : Shrink space for this element if space is too tight with the given strength (0 = none).
  *wrap*
  : Layout of the flexbox children may wrap if the elements use more than 100%.
  *wrap-reverse*
  : Layout of the flexbox children may wrap if the elements use more than 100%, in reverse order.
  ## Slots:
  : The *default* slot holds the contextmenu content.
  # B-VFLEX
  A vertical variant of the [B-HFLEX](#b-hflex) container for vertical layouting.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-hflex, .b-vflex {
    display: flex; flex-basis: auto;
    flex-wrap: nowrap;
    justify-content: space-evenly;	//* distribute extra main-axis space */
    align-items: stretch;		//* distribute extra cross-axis space */
    align-content: stretch;		//* distribute extra cross-axis space for multi-line layouts */
  }
  .b-hflex {
    flex-direction: row;
    .b-flex-reverse	{ flex-direction: row-reverse; }
  }
  .b-vflex {
    flex-direction: column;
    .b-flex-reverse	{ flex-direction: column-reverse; }
  }
  .b-flex-inline	{ display: inline-flex; }
  .b-flex-start		{ justify-content: flex-start; }
  .b-flex-end		{ justify-content: flex-end; }
  .b-flex-center	{ justify-content: center; }
  .b-flex-space-between	{ justify-content: space-between; }
  .b-flex-space-around	{ justify-content: space-around; }
  .b-flex-space-evenly	{ justify-content: space-evenly; }
  .b-flex-wrap		{ flex-wrap: wrap; }
  .b-flex-wrap-reverse	{ flex-wrap: wrap-reverse; }
  .b-flex-grow1 { flex-grow: 1; } .b-flex-grow2 { flex-grow: 2; } .b-flex-grow3 { flex-grow: 3; }
  .b-flex-grow4 { flex-grow: 4; } .b-flex-grow5 { flex-grow: 5; } .b-flex-grow6 { flex-grow: 6; }
  .b-flex-grow7 { flex-grow: 7; } .b-flex-grow8 { flex-grow: 8; } .b-flex-grow9 { flex-grow: 9; }
  .b-flex-grow0 { flex-grow: 0; }
  .b-flex-shrink1 { flex-shrink: 1; } .b-flex-shrink2 { flex-shrink: 2; } .b-flex-shrink3 { flex-shrink: 3; }
  .b-flex-shrink4 { flex-shrink: 4; } .b-flex-shrink5 { flex-shrink: 5; } .b-flex-shrink6 { flex-shrink: 6; }
  .b-flex-shrink7 { flex-shrink: 7; } .b-flex-shrink8 { flex-shrink: 8; } .b-flex-shrink9 { flex-shrink: 9; }
  .b-flex-shrink0 { flex-shrink: 0; }
</style>

<script>
function classlist (attrs) {
  const classnames = [
    'flex-column', 'flex-row',
    'start',
    'end',
    'center',
    'inline',
    'space-between',
    'space-around',
    'space-evenly',
    'grow0', 'grow1', 'grow2', 'grow3', 'grow4',
    'grow5', 'grow6', 'grow7', 'grow8', 'grow9',
    'reverse',
    'shrink0', 'shrink1', 'shrink2', 'shrink3', 'shrink4',
    'shrink5', 'shrink6', 'shrink7', 'shrink8', 'shrink9',
    'wrap', 'wrap-reverse',
  ];
  const classes = [];
  for (let cssclass of classnames)
    if (attrs[cssclass] === "" || attrs[cssclass])
      classes.push ('b-flex-' + cssclass);
  return classes;
}

const hflex = {
  name: 'b-hflex',
  functional: true,
  render: function (h, context) {
    const classes = Util.join_classes ('b-hflex', context.data.class, classlist (context.data.attrs || {}));
    const data = Object.assign ({}, context.data, { class: classes });
    return h ('div', data, context.children);
  }
};
Vue.component (hflex.name, hflex);

const vflex = {
  name: 'b-vflex',
  functional: true,
  render: function (h, context) {
    const classes = Util.join_classes ('b-vflex', context.data.class, classlist (context.data.attrs || {}));
    const data = Object.assign ({}, context.data, { class: classes });
    return h ('div', data, context.children);
  }
};
Vue.component (vflex.name, vflex);

export default {};
</script>
