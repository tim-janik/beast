# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
include $(wildcard $>/beast-gtk/*.d)
CLEANDIRS += $(wildcard $>/beast-gtk/)

include beast-gtk/gxk/Makefile.mk
