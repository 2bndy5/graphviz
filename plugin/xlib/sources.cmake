# $Id$ $Revision$

INCLUDE( FindX11 )
INCLUDE( UsePkgConfig )
PKGCONFIG( pangocairo PANGOCAIRO_INCLUDE_DIR PANGOCAIRO_LIB_DIR PANGOCAIRO_LINK_FLAGS PANGOCAIRO_CFLAGS )
ADD_DEFINITIONS(${PANGOCAIRO_CFLAGS})

INCLUDE_DIRECTORIES(
	${gvplugin_xlib_SRCDIR}
	${top_SRCDIR}
	${common_SRCDIR}
	${pathplan_SRCDIR}
	${gvc_SRCDIR}
	${graph_SRCDIR}
	${cdt_SRCDIR}
)

SET(gvplugin_xlib_SRCS
	${gvplugin_xlib_SRCDIR}/gvdevice_xlib.c
	${gvplugin_xlib_SRCDIR}/gvplugin_xlib.c
)
