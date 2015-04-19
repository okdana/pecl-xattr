/*
  +----------------------------------------------------------------------+
  | PHP Version 5, 7                                                     |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Marcin Gibula <mg@iceni.pl>                                  |
  |         Remi Collet <remi@php.net>                                   |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define XATTR_BUFFER_SIZE	1024	/* Initial size for internal buffers, feel free to change it */

#define XATTR_DONTFOLLOW 1

#define XATTR_TRUSTED    2
#define XATTR_SYSTEM     4
#define XATTR_SECURITY   8
#define XATTR_USER      16
#define XATTR_ALL       32
#define XATTR_MASK      (XATTR_TRUSTED|XATTR_SYSTEM|XATTR_SECURITY|XATTR_USER|XATTR_ALL)
#define XATTR_ROOT       XATTR_TRUSTED

/* These prefixes have been taken from attr(5) man page */
#define XATTR_USER_PREFIX      "user."
#define XATTR_TRUSTED_PREFIX   "trusted."
#define XATTR_SYSTEM_PREFIX    "system."
#define XATTR_SECURITY_PREFIX  "security."

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_xattr.h"

#include <stdlib.h>

#include <sys/types.h>
#include <sys/xattr.h>

ZEND_BEGIN_ARG_INFO_EX(xattr_set_arginfo, 0, 0, 3)
  ZEND_ARG_INFO(0, path)
  ZEND_ARG_INFO(0, name)
  ZEND_ARG_INFO(0, value)
  ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(xattr_get_arginfo, 0, 0, 2)
  ZEND_ARG_INFO(0, path)
  ZEND_ARG_INFO(0, name)
  ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(xattr_list_arginfo, 0, 0, 1)
  ZEND_ARG_INFO(0, path)
  ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

/* {{{ xattr_functions[]
 *
 * Every user visible function must have an entry in xattr_functions[].
 */
zend_function_entry xattr_functions[] = {
	PHP_FE(xattr_set,       xattr_set_arginfo)
	PHP_FE(xattr_get,       xattr_get_arginfo)
	PHP_FE(xattr_remove,    xattr_get_arginfo)
	PHP_FE(xattr_list,      xattr_list_arginfo)
	PHP_FE(xattr_supported,	xattr_list_arginfo)
#ifdef PHP_FE_END
	PHP_FE_END
#else
	{ NULL, NULL, NULL }
#endif
};
/* }}} */

/* {{{ xattr_module_entry
 */
zend_module_entry xattr_module_entry = {
	STANDARD_MODULE_HEADER,
	"xattr",
	xattr_functions,
	PHP_MINIT(xattr),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(xattr),
	PHP_XATTR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_XATTR
ZEND_GET_MODULE(xattr)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(xattr)
{
	REGISTER_LONG_CONSTANT("XATTR_USER",       XATTR_USER,       CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_ROOT",       XATTR_ROOT,       CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_TRUSTED",    XATTR_TRUSTED,    CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_SYSTEM",     XATTR_SYSTEM,     CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_SECURITY",   XATTR_SECURITY,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_ALL",        XATTR_ALL,        CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("XATTR_DONTFOLLOW", XATTR_DONTFOLLOW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_CREATE",     XATTR_CREATE,     CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_REPLACE",    XATTR_REPLACE,    CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(xattr)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "xattr support", "enabled");
	php_info_print_table_row(2, "PECL module version", PHP_XATTR_VERSION);
	php_info_print_table_end();
}
/* }}} */

#define check_prefix(flags) add_prefix(NULL, flags TSRMLS_DC)

static char *add_prefix(char *name, zend_long flags TSRMLS_DC) {
	char *ret;

	if ((flags & XATTR_MASK) > 0 &&
	    (flags & XATTR_MASK) != XATTR_TRUSTED &&
	    (flags & XATTR_MASK) != XATTR_SYSTEM &&
	    (flags & XATTR_MASK) != XATTR_SECURITY &&
	    (flags & XATTR_MASK) != XATTR_USER &&
	    (flags & XATTR_MASK) != XATTR_ALL) {
		php_error(E_NOTICE, "%s Bad option, single namespace expected", get_active_function_name(TSRMLS_C));
	}
	if (!name) {
		return NULL;
	}
	if ((flags & XATTR_MASK) == XATTR_ALL && !strchr(name, '.')) {
		php_error(E_NOTICE, "%s Bad option, missing namespace, XATTR_ALL ignored", get_active_function_name(TSRMLS_C));
	}

	if (flags & XATTR_TRUSTED) {
		spprintf(&ret, 0, "%s%s", XATTR_TRUSTED_PREFIX, name);

	} else if (flags & XATTR_SYSTEM) {
		spprintf(&ret, 0, "%s%s", XATTR_SYSTEM_PREFIX, name);

	} else if (flags & XATTR_SECURITY) {
		spprintf(&ret, 0, "%s%s", XATTR_SECURITY_PREFIX, name);

	} else if ((flags & XATTR_ALL) && strchr(name, '.')) {
		/* prefix provided in input */
		ret = name;

	} else {
		spprintf(&ret, 0, "%s%s", XATTR_USER_PREFIX, name);
	}
	return ret;
}

/* {{{ proto bool xattr_set(string path, string name, string value [, int flags])
   Set an extended attribute of file */
PHP_FUNCTION(xattr_set)
{
	char *attr_name = NULL, *prefixed_name;
	char *attr_value = NULL;
	char *path = NULL;
	int error;
	zend_long flags = 0;
	strsize_t tmp, value_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|l", &path, &tmp, &attr_name, &tmp, &attr_value, &value_len, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC)
#if PHP_VERSION_ID < 50400
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}

	prefixed_name = add_prefix(attr_name, flags TSRMLS_CC);
	/* Attempt to set an attribute, warn if failed. */
	if (flags & XATTR_DONTFOLLOW) {
		error = lsetxattr(path, prefixed_name, attr_value, (int)value_len, (int)(flags & (XATTR_CREATE | XATTR_REPLACE)));
	} else {
		error = setxattr(path, prefixed_name, attr_value, (int)value_len, (int)(flags & (XATTR_CREATE | XATTR_REPLACE)));
	}
	if (error == -1) {
		switch (errno) {
			case E2BIG:
				php_error(E_WARNING, "%s The value of the given attribute is too large", get_active_function_name(TSRMLS_C));
				break;
			case EPERM:
			case EACCES:
				php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
				break;
			case EOPNOTSUPP:
				php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
				break;
			case ENOENT:
			case ENOTDIR:
				php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
				break;
			case EEXIST:
				php_error(E_WARNING, "%s Attribute %s already exists", get_active_function_name(TSRMLS_C), prefixed_name);
				break;
			case ENODATA:
				php_error(E_WARNING, "%s Attribute %s doesn't exists", get_active_function_name(TSRMLS_C), prefixed_name);
				break;
		}

		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}
	if (prefixed_name != attr_name) {
		efree(prefixed_name);
	}
}
/* }}} */

/* {{{ proto string xattr_get(string path, string name [, int flags])
   Returns a value of an extended attribute */
PHP_FUNCTION(xattr_get)
{
	char *attr_name = NULL, *prefixed_name;
	char *attr_value = NULL;
	char *path = NULL;
	strsize_t tmp;
	zend_long flags = 0;
	size_t buffer_size;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC)
#if PHP_VERSION_ID < 50400
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}

	prefixed_name = add_prefix(attr_name, flags TSRMLS_CC);

	/*
	 * If buffer is too small then attr_get sets errno to E2BIG and tells us
	 * how many bytes are required by setting buffer_size variable.
	 */
	if (flags & XATTR_DONTFOLLOW) {
		buffer_size = lgetxattr(path, prefixed_name, attr_value, 0);
	} else {
		buffer_size = getxattr(path, prefixed_name, attr_value, 0);
	}
	if (buffer_size != (size_t)-1) {
		attr_value = emalloc(buffer_size+1);

		if (flags & XATTR_DONTFOLLOW) {
			buffer_size = lgetxattr(path, prefixed_name, attr_value, buffer_size);
		} else {
			buffer_size = getxattr(path, prefixed_name, attr_value, buffer_size);
		}
		attr_value[buffer_size] = 0;
	}

	if (prefixed_name != attr_name) {
		efree(prefixed_name);
	}

	/* Return a string if everything is ok */
	if (buffer_size != (size_t)-1) {
		_RETVAL_STRINGL(attr_value, buffer_size, 1); /* copy + free instead of realloc */
		efree(attr_value);
		return;
	}

	/* Give warning for some common error conditions */
	switch (errno) {
		case ENODATA:
			break;
		case ENOENT:
		case ENOTDIR:
			php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
			break;
		case EPERM:
		case EACCES:
			php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
			break;
		case EOPNOTSUPP:
			php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
			break;
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool xattr_supported(string path [, int flags])
   Checks if filesystem supports extended attributes */
PHP_FUNCTION(xattr_supported)
{
	char *buffer="", *path = NULL;
	int error;
	strsize_t tmp;
	zend_long flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &tmp, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC)
#if PHP_VERSION_ID < 50400
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_NULL();
	}

	/* Is "user.test.is.supported" a good name? */
	if (flags & XATTR_DONTFOLLOW) {
		error = lgetxattr(path, "user.test.is.supported", buffer, 0);
	} else {
		error = getxattr(path, "user.test.is.supported", buffer, 0);
	}

	if (error >= 0) {
		RETURN_TRUE;
	}

	switch (errno) {
		case ENODATA:
			RETURN_TRUE;
		case ENOTSUP:
			RETURN_FALSE;
		case ENOENT:
		case ENOTDIR:
			php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
			break;
		case EACCES:
			php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
			break;
	}

	RETURN_NULL();
}
/* }}} */

/* {{{ proto string xattr_remove(string path, string name [, int flags])
   Remove an extended attribute of file */
PHP_FUNCTION(xattr_remove)
{
	char *attr_name = NULL, *prefixed_name;
	char *path = NULL;
	int error;
	strsize_t tmp;
	zend_long flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC)
#if PHP_VERSION_ID < 50400
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}

	prefixed_name = add_prefix(attr_name, flags TSRMLS_CC);

	/* Attempt to remove an attribute, warn if failed. */
	if (flags & XATTR_DONTFOLLOW) {
		error = lremovexattr(path, prefixed_name);
	} else {
		error = removexattr(path, prefixed_name);
	}
	if (prefixed_name != attr_name) {
		efree(prefixed_name);
	}

	if (error == -1) {
		switch (errno) {
			case E2BIG:
				php_error(E_WARNING, "%s The value of the given attribute is too large", get_active_function_name(TSRMLS_C));
				break;
			case EPERM:
			case EACCES:
				php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
				break;
			case EOPNOTSUPP:
				php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
				break;
			case ENOENT:
			case ENOTDIR:
				php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
				break;
		}

		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array xattr_list(string path [, int flags])
   Get list of extended attributes of file */
PHP_FUNCTION(xattr_list)
{
	char *buffer, *path = NULL;
	char *p, *prefix;
	int error;
	strsize_t tmp;
	zend_long flags = 0;
	size_t i = 0, buffer_size = XATTR_BUFFER_SIZE;
	size_t len, prefix_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &tmp, &flags) == FAILURE) {
		return;
	}

	check_prefix(flags);

	/* Enforce open_basedir and safe_mode */
	if (php_check_open_basedir(path TSRMLS_CC)
#if PHP_VERSION_ID < 50400
	|| (PG(safe_mode) && !php_checkuid(path, NULL, CHECKUID_DISALLOW_FILE_NOT_EXISTS))
#endif
	) {
		RETURN_FALSE;
	}

	buffer = emalloc(buffer_size);

	/* Loop is required to get a list reliably */
	do {
		/*
		 * Call to this function with zero size buffer will return us
		 * required size of our buffer in return (or an error).
		 */
		if (flags & XATTR_DONTFOLLOW) {
			error = llistxattr(path, buffer, 0);
		} else {
			error = listxattr(path, buffer, 0);
		}

		/* Print warning on common errors */
		if (error == -1) {
			switch (errno) {
				case ENOTSUP:
					php_error(E_WARNING, "%s Operation not supported", get_active_function_name(TSRMLS_C));
					break;
				case ENOENT:
				case ENOTDIR:
					php_error(E_WARNING, "%s File %s doesn't exists", get_active_function_name(TSRMLS_C), path);
					break;
				case EACCES:
					php_error(E_WARNING, "%s Permission denied", get_active_function_name(TSRMLS_C));
					break;
			}

			efree(buffer);
			RETURN_FALSE;
		}

		/* Resize buffer to the required size */
		buffer_size = error;
		buffer = erealloc(buffer, buffer_size);

		if (flags & XATTR_DONTFOLLOW) {
			error = llistxattr(path, buffer, buffer_size);
		} else {
			error = listxattr(path, buffer, buffer_size);
		}

		/*
		 * Preceding functions may fail if extended attributes
		 * have been changed after we read required buffer size.
		 * That's why we will retry if errno is ERANGE.
		 */
	} while (error == -1 && errno == ERANGE);

	/* If there is still an error and it's not ERANGE then return false */
	if (error == -1) {
		efree(buffer);
		RETURN_FALSE;
	}

	buffer_size = error;
	buffer = erealloc(buffer, buffer_size);

	array_init(return_value);
	p = buffer;

	/*
	 * Root namespace has the prefix "trusted." and users namespace "user.".
	 */
	if (flags & XATTR_SYSTEM) {
		prefix = XATTR_SYSTEM_PREFIX;
	} else if (flags & XATTR_SECURITY) {
		prefix = XATTR_SECURITY_PREFIX;
	} else if (flags & XATTR_TRUSTED) {
		prefix = XATTR_TRUSTED_PREFIX;
	} else {
		prefix = XATTR_USER_PREFIX;
	}

	prefix_len = strlen(prefix);

	/*
	 * We go through the whole list and add entries beginning with selected
	 * prefix to the return_value array.
	 */
	while (i != buffer_size) {
		len = strlen(p) + 1;	/* +1 for NULL */
		if (flags & XATTR_ALL) {
#if PHP_MAJOR_VERSION < 7
			add_next_index_stringl(return_value, p, len - 1, 1);
#else
			add_next_index_stringl(return_value, p, len - 1);
#endif
		} else if (strstr(p, prefix) == p) {
#if PHP_MAJOR_VERSION < 7
			add_next_index_stringl(return_value, p + prefix_len, len - 1 - prefix_len, 1);
#else
			add_next_index_stringl(return_value, p + prefix_len, len - 1 - prefix_len);
#endif
		}

		p += len;
		i += len;
	}

	efree(buffer);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
