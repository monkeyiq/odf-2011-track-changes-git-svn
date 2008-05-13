
openwriter_pkgs="$gsf_req"

if test "$enable_openwriter" == "yes"; then

PKG_CHECK_MODULES(OPENWRITER,[ $openwriter_pkgs ])

OPENWRITER_CFLAGS="$OPENWRITER_CFLAGS "'${PLUGIN_CFLAGS}'
OPENWRITER_LIBS="$OPENWRITER_LIBS "'${PLUGIN_LIBS}'

if test "$enable_openwriter_builtin" == "yes"; then
	OPENWRITER_CFLAGS="$OPENWRITER_CFLAGS -DABI_PLUGIN_BUILTIN"
fi

fi

AC_SUBST([OPENWRITER_CFLAGS])
AC_SUBST([OPENWRITER_LIBS])

