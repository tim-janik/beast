<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-BUTTON
  A button element that responds to clicks.
  ## Props:
  *hotkey*
  : Keyboard accelerator to trigger a *click* event.
  ## Events:
  *click*
  : A *click* event is emitted for activation through mouse buttons, Space or Enter keys.
  ## Slots:
  *default*
  : All contents passed into this element will be rendered as contents of the button.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-button {
    @include b-buttonshade;
    text-align: center; margin: 0; padding: 3px 1em;
    display: flex;
    align-items: center;
    &.b-button-disabled {
      filter: saturate(80%) brightness(80%);
      pointer-events: none;
    }
  }
  .b-button:focus            	{ outline: $b-focus-outline; }
  .b-button:hover            	{ @include b-buttonhover; }
  .b-button.active, .b-button[data-contextmenu=true],
  .b-button:active           	{ @include b-buttonactive; }
  .b-button			{ color: $b-button-foreground; }
  /* use '*' + fill!important to include svg elements in buttons */
  .b-button.active, .b-button[data-contextmenu=true],
  .b-button:active         	{ color: $b-button-active-fg; }
</style>

<script>
export default {};

const b_button = {
  name: 'b-button',
  props: { hotkey: String, disabled: Boolean, canfocus: Boolean },
  functional: true,
  render: function (h, context) {
    const attrs = {
      'data-hotkey': context.props.hotkey,
      disabled: context.props.disabled,
    };
    const localdata = {
      class: Util.join_classes ('b-button', context.data.class,
				context.props.disabled ? 'b-button-disabled' : ''),
      attrs: Object.assign ({}, context.data.attrs, attrs),
    };
    const data = Object.assign ({}, context.data, localdata);
    return h (context.props.canfocus ? 'button' : 'span', data, context.children);
  }
};
Vue.component (b_button.name, b_button);

</script>
