
clarisworks_pkgs="$gsf_req"

if test "$enable_clarisworks" == "yes"; then

PKG_CHECK_MODULES(CLARISWORKS,[ $clarisworks_pkgs ])

CLARISWORKS_CFLAGS="$CLARISWORKS_CFLAGS "'${PLUGIN_CFLAGS}'
CLARISWORKS_LIBS="$CLARISWORKS_LIBS "'${PLUGIN_LIBS}'

if test "$enable_clarisworks_builtin" == "yes"; then
	CLARISWORKS_CFLAGS="$CLARISWORKS_CFLAGS -DABI_PLUGIN_BUILTIN"
fi

fi

AC_SUBST([CLARISWORKS_CFLAGS])
AC_SUBST([CLARISWORKS_LIBS])

