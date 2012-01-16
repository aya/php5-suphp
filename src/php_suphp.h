/*
  +---------------------------------------------------------------------+
  | PHP Version 5 - Copyright (c) 1997-2007 The PHP Group               |
  +---------------------------------------------------------------------+
  | Author: Simon Uyttendaele                                           |
  +---------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifndef PHP_SUPHP_H
#define PHP_SUPHP_H

extern zend_module_entry suphp_module_entry;
#define phpext_suphp_ptr &suphp_module_entry

#ifdef PHP_WIN32
#define PHP_SUPHP_API __declspec(dllexport)
#else
#define PHP_SUPHP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(suphp);
PHP_MSHUTDOWN_FUNCTION(suphp);
PHP_RINIT_FUNCTION(suphp);
PHP_RSHUTDOWN_FUNCTION(suphp);
PHP_MINFO_FUNCTION(suphp);

ZEND_BEGIN_MODULE_GLOBALS(suphp)
	long  global_enabled;
	long  global_min_uid;
	long  global_use_effective_uid;
	long  global_chown_uploads;
ZEND_END_MODULE_GLOBALS(suphp)

#endif	/* PHP_SUPHP_H */

