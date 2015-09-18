dnl $Id$
dnl config.m4 for extension xattr

PHP_ARG_WITH(xattr, for xattr support,
Make sure that the comment is aligned:
[  --with-xattr             Include xattr support])

if test "$PHP_XATTR" != "no"; then
  AC_CHECK_HEADER([sys/xattr.h], [HAVE_XATTR_H="yes"], [HAVE_XATTR_H="no"])
  if test $HAVE_XATTR_H = "no" ; then
     AC_MSG_ERROR([You need to install glibc development package])
  fi

  PHP_NEW_EXTENSION(xattr, xattr.c, $ext_shared)
fi
