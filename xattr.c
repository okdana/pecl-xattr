/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Marcin Gibula <mg@iceni.pl>                                  |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define XATTR_BUFFER_SIZE	1024

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_xattr.h"

#include <stdlib.h>
#include <attr/attributes.h>

#include <sys/types.h>
#include <attr/xattr.h>

/* {{{ xattr_functions[]
 *
 * Every user visible function must have an entry in xattr_functions[].
 */
function_entry xattr_functions[] = {
	PHP_FE(xattr_set,		NULL)
	PHP_FE(xattr_get,		NULL)
	PHP_FE(xattr_remove,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in xattr_functions[] */
};
/* }}} */

/* {{{ xattr_module_entry
 */
zend_module_entry xattr_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"xattr",
	xattr_functions,
	PHP_MINIT(xattr),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(xattr),
#if ZEND_MODULE_API_NO >= 20010901
	"1.0",
#endif
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
	REGISTER_LONG_CONSTANT("XATTR_ROOT", ATTR_ROOT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_DONTFOLLOW", ATTR_DONTFOLLOW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_CREATE", ATTR_CREATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XATTR_REPLACE", ATTR_REPLACE, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(xattr)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(xattr)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(xattr)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(xattr)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "xattr support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto bool xattr_set(string path, string name, string value [, int flags])
   Set an extended attribute for a file */
PHP_FUNCTION(xattr_set)
{
	char *attr_name = NULL;
	char *attr_value = NULL;
	char *path = NULL;
	int error, tmp, value_len, flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|l", &path, &tmp, &attr_name, &tmp, &attr_value, &value_len, &flags) == FAILURE) {
		return;
	}

	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | ATTR_DONTFOLLOW | ATTR_CREATE | ATTR_REPLACE; 
	
	/* Try to set an attribute and give some warning if it fails... */
	error = attr_set(path, attr_name, attr_value, value_len, flags);
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

/* {{{ proto string xattr_get(string path, string name [, int flags])
   Return a value of an extended attribute */
PHP_FUNCTION(xattr_get)
{
	char *attr_name = NULL;
	char *attr_value = NULL;
	char *path = NULL;
	int error, tmp, flags = 0;
	size_t buffer_size = XATTR_BUFFER_SIZE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}

	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | ATTR_DONTFOLLOW; 
	
	/* Allocate a buffer with starting size XATTR_BUFFER_SIZE bytes */
	attr_value = emalloc(buffer_size);
	if (!attr_value)
		RETURN_FALSE;
	
	/* 
	 * If buffer is too small then attr_get sets errno to E2BIG and tells us
	 * how many bytes is required by setting buffer_size variable.
	 */
	error = attr_get(path, attr_name, attr_value, &buffer_size, flags);

	/* 
	 * Loop is necessary in case that someone edited extended attributes
	 * and our estimated buffer size is no longer correct.
	 */
	while (error && errno == E2BIG) {
		attr_value = erealloc(attr_value, buffer_size);
		if (!attr_value)
			RETURN_FALSE;
		
		error = attr_get(path, attr_name, attr_value, &buffer_size, flags);		
	}

	/* Return a string if everything is ok */
	if (!error) {
		attr_value = erealloc(attr_value, buffer_size);
		RETURN_STRINGL(attr_value, buffer_size, 0);
	}
	
	/* Error handling part */
	efree(attr_value);
	
	/* Give warning for some common error conditions */
	switch (errno) {
		case ENOATTR:
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

/* {{{ proto string xattr_get(string path, string name [, int flags])
   Return a value of an extended attribute */
PHP_FUNCTION(xattr_remove)
{
	char *attr_name = NULL;
	char *path = NULL;
	int error, tmp, flags = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &tmp, &attr_name, &tmp, &flags) == FAILURE) {
		return;
	}
	
	/* Ensure that only allowed bits are set */
	flags &= ATTR_ROOT | ATTR_DONTFOLLOW; 
	
	/* Try to remove an attribute and give some warning if it fails... */
	error = attr_remove(path, attr_name, flags);
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

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
