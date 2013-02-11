PHP_ARG_ENABLE(suphp, whether to enable suphp module,
[  --enable-suphp          Enable suphp module])

if test "$PHP_SUPHP" = "yes"; then
  AC_DEFINE(HAVE_PHP_SUPHP, 1, [Whether you have suphp module])
  PHP_NEW_EXTENSION(suphp, suphp.c, $ext_shared)
fi
