'use strict';
const m = Mithril;

// == ModalDialog ==
/**
 * Show a popup dialog.
 * Attributes:
 * - title: dialog title vnode
 * - buttons: list of vnodes
 * - onclose: callback for dialog closing
 */
const ModalDialog = {
  view: function (vnode) {
    const child_attrs = {
      title: undefined,
      buttons: undefined,
      onclose: undefined,
      __proto__: vnode.attrs
    };
    let divs = [
      m ('.ModalDialogArea.vbox', [
	m ('.maxbox'), /* eat space for vertical centering */
	m ('.minbox', [
	  m ('.DialogSpacer.hbox', [
	    m ('.maxbox.w100'), /* eat space for horizontal centering */
	    m ('.minbox.shrinkbox', [
	      m ('.DialogRack', [
		m ('.Dialog.vbox', child_attrs, [
		  m ('.DialogInside.vbox', [
		    m ('.Header.minbox', [
		      m ('.hbox.w100', [
			m ('.Title.maxbox', vnode.attrs.title),
			m ('div.minbox.Closer[autofocus]', {
			  style: 'cursor: default',
			  onclick: (event) => {
			    if (vnode.attrs.onclose)
			      vnode.attrs.onclose (event);
			  }
			}, 'X'),
		      ]),
		    ]),
		    m ('.Contents.maxbox', [
		      m ('.DialogHS', vnode.children),
		    ]),
		    m ('.DialogControls.minbox', [
		      m ('.DialogButtons', [
			vnode.attrs.buttons,
		      ])
		    ]),
		  ]),
		]),
	      ]),
	    ]),
	    m ('.maxbox.w100'), /* eat space for horizontal centering */
	  ]),
	]),
	m ('.maxbox'), /* eat space for vertical centering */
      ]),
    ];
    return m.fragment ({}, divs);
  }
};
module.exports.ModalDialog = ModalDialog;

// == About ==
function version_rows () {
  const App = Electron.app;
  const os = require ('os');
  return [
    m ('tr', [ m ('th', 'Application:'),	m ('td', App.getName() + ' ' + App.getVersion()) ]),
    m ('tr', [ m ('th', 'OS:'),			m ('td', process.platform + ' ' + process.arch + ' (' + os.release() + ')') ]),
    m ('tr', [ m ('th', 'Electron:'),		m ('td', process.versions.electron) ]),
    m ('tr', [ m ('th', 'Chrome:'),		m ('td', process.versions.chrome) ]),
    m ('tr', [ m ('th', 'User Agent:'),		m ('td', navigator.userAgent) ]),
    m ('tr', [ m ('th', 'V8:'),			m ('td', process.versions.v8) ]),
    m ('tr', [ m ('th', 'Node.js:'),		m ('td', process.versions.node) ]),
    m ('tr', [ m ('th', 'Bse:'),		m ('td', Bse.server.get_version()) ]),
    m ('tr', [ m ('th', 'Vorbis:'),		m ('td', Bse.server.get_vorbis_version()) ]),
    m ('tr', [ m ('th', 'Libuv:'),		m ('td', process.versions.uv) ]),
    m ('tr', [ m ('th', 'Executable:'),		m ('td', m('p',App.getPath ('exe'))) ]),
    m ('tr', [ m ('th', 'Mithril'),		m ('td', Mithril.version) ]),
    m ('tr', [ m ('th', 'jQuery'),		m ('td', jQuery.fn.jquery) ]),
    m ('tr', [ m ('th', 'Working Dir:'),	m ('td', App.getAppPath()) ]),
    m ('tr', [ m ('th', 'Desktop Dir:'),	m ('td', App.getPath ('desktop')) ]),
    m ('tr', [ m ('th', 'Config Path:'),	m ('td', App.getPath ('userData')) ]),
    m ('tr', [ m ('th', 'Music Path:'),		m ('td', App.getPath ('music')) ]),
  ];
}
module.exports.about_dialog = function () {
  if (!App.show_about)
    return null;
  function doclose (ev) {
    App.show_about = false;
  }
  return (
    m (ModalDialog, { class: 'About',
		      title: 'About BEAST',
		      onclose: doclose,
		      buttons: [
			m ('button', [ 'Toggle', ]),
			m ('button[autofocus]', { onclick: doclose }, [ 'Close', ]),
			m ('button', [ 'Help', ]),
		      ] },
       m ('table', version_rows())
      ));
};
