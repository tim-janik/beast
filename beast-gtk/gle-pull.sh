#!/bin/bash
set -v

GLE=/usr/src/gle/gle
SOURCES="gtkwrapbox gtkcluehunter gtkhwrapbox gtkvwrapbox"
TEMPH=glewidgets.h-tmp

# pull together glewidgets.h body
echo >glewidgets.tmp
for i in $SOURCES; do
	cat $GLE/$i.h >>glewidgets.tmp
done

# put guarding header with includes
cat >$TEMPH <<EOF
#ifndef __GLE_WIDGETS_H__
#define __GLE_WIDGETS_H__
#include <gtk/gtk.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
EOF

# disable includes and put body
sed >>$TEMPH <glewidgets.tmp -e 's,^\(#include.*\),/* \1 */,'

# put trailer
cat >>$TEMPH <<EOF
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GLE_WIDGETS_H__ */
EOF

cmp -s $TEMPH glewidgets.h || mv $TEMPH glewidgets.h
rm -f $TEMPH


# pull together glewidgets.c body
echo >glewidgets.tmp
for i in $SOURCES; do
	sed <$GLE/$i.c >>glewidgets.tmp \
	  -e 's,parent_class,parent_'$i'_class,g' \
	  -e 's,\([^_]ARG_\),\1_'$i'_,g' \
	  -e 's,gtk_widget_get_child_requisition,gtk_widget_get__child_requisition,g' \
	  -e 's,get_child_requisition,get_'$i'_child_requisition,g' \
	  -e 's,gtk_widget_get__child_requisition,gtk_widget_get_child_requisition,g' \
	  -e 's,get_layout_size,get_'$i'_layout_size,g' \
	  -e 's,Line,Line_'$i'_,g'
done

# put header with includes
cat >glewidgets.c <<EOF
#include "glewidgets.h"
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
EOF

# disable includes and put body
sed >>glewidgets.c <glewidgets.tmp -e 's,^\(#include.*\),/* \1 */,'

# remove temporary
rm glewidgets.tmp

