dnl $Id$
dnl config.m4 for extension xattr

PHP_ARG_WITH(xattr, for xattr support,
Make sure that the comment is aligned:
[  --with-xattr             Include xattr support])

if test "$PHP_XATTR" != "no"; then
  AC_CHECK_HEADER([sys/xattr.h], [
  ], [
     AC_MSG_ERROR([You need to install glibc development package])
  ])

  PHP_NEW_EXTENSION(xattr, xattr.c, $ext_shared)
fi
