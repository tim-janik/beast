# Beastycons - Cursors & icons for Beast

SVG based cursor and icon drawings for the Beast project.

## SCSS Cursor Variables

Cursors from [Beast/images/cursors/](https://github.com/tim-janik/beast/tree/master/images/cursors/)
are transformed into SCSS variables holding `CSS cursor` values. For instance:

```SCSS
// SCSS cursor use:
@import 'beastycons/bc-cursors.scss';
* { cursor: $bc-cursor-pen; }
// CSS result:
* { cursor: url(data:image/svg+xml;base64,Abc…xyZ) 3 28, default; }
```

The cursors are SVG drawings, layed out at 32x32 pixels,
and `beastycons.sh` contains the hotspot coordinates.

## Icon font

Icons from [Beast/images/icons/](https://github.com/tim-janik/beast/tree/master/images/icons/)
are transformed into a WOFF2 font and a CSS file `Beastycons.css` to be used like this:

```HTML
<!-- HEAD: Load Beastycons, preload woff2 for speedups -->
<link rel="stylesheet" href="/assets/Beastycons.css" crossorigin>
<link rel="preload"    href="/assets/Beastycons.woff2" as="font" type="font/woff2" crossorigin>
<!-- BODY: Display engine1.svg -->
<i class="Beastycons-engine1"></i>
```

The icons are SVG drawings and layed out at 24x24 pixels.

The build is slightly involved, using Inkscape, ImageMagick and various npm
packages, so prebuilt artifacts are uploaded and can be fetched from:
	https://github.com/tim-janik/assets/releases/.

## LICENSE

Unless noted otherwise, the Beastycons source code forms are licensed
[MPL-2.0](http://mozilla.org/MPL/2.0)
