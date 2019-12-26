<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-GRID
  A [grid](https://www.w3.org/TR/css-grid-1/#grid-containers) container for grid layouting
  ([visual cheatsheet](http://grid.malven.co/).
  ## Props:
  *inline*
  : Use 'display: inline-grid' as layout mode, e.g. to integrate this container into text flow.
  ## Slots:
  : The *default* slot holds the contextmenu content.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-grid {
    display: grid;
  }
  .b-grid-inline { display: inline-grid; }
  /* Alignment: https://www.w3.org/TR/css-align-3/#overview
   * - grid within parent, vertically:   align-content:   start end center stretch space-around space-between space-evenly;
   * - grid within parent, horizontally: justify-content: start end center stretch space-around space-between space-evenly;
   * - items within grid, vertically:    align-items:     baseline start end center stretch;
   * - items within grid, horizontally:  justify-items:   baseline start end center stretch space-around space-between space-evenly;
   * - per-element: align-self justify-self
   */
</style>

<script>
function classlist (classlist, attrs) {
  const classnames = [
    'inline',
  ];
  const classes = (classlist || '').split (/ +/);
  for (let cssclass of classnames)
    if (attrs[cssclass] === "" || attrs[cssclass])
      classes.push ('b-grid-' + cssclass);
  return classes;
}

const grid = {
  name: 'b-grid',
  functional: true,
  render: function (h, context) {
    const classes = classlist ('b-grid ' + (context.data.class || ''), context.data.attrs || {});
    const data = Object.assign ({}, context.data, { class: classes.join (' ') });
    return h ('div', data, context.children);
  }
};
Vue.component (grid.name, grid);

export default {};
</script>
