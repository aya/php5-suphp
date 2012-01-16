/*
  +---------------------------------------------------------------------+
  | PHP Version 5 - Copyright (c) 1997-2007 The PHP Group               |
  +---------------------------------------------------------------------+
  | Author: Simon Uyttendaele                                           |
  +---------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "php_suphp.h"

ZEND_DECLARE_MODULE_GLOBALS(suphp)
static int le_suphp;

zend_function_entry suphp_functions[] = {
	{NULL, NULL, NULL}
};

zend_module_entry suphp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"suphp",
	suphp_functions,
	PHP_MINIT(suphp),
	PHP_MSHUTDOWN(suphp),
	PHP_RINIT(suphp),
	PHP_RSHUTDOWN(suphp),
	PHP_MINFO(suphp),
#if ZEND_MODULE_API_NO >= 20010901
	"1.0", /* module version number */
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SUPHP
ZEND_GET_MODULE(suphp)
#endif

// ======================== PHP.ini ========================
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("suphp.enabled", "1", PHP_INI_SYSTEM, OnUpdateLong, global_enabled, zend_suphp_globals, suphp_globals)
	STD_PHP_INI_ENTRY("suphp.min_uid", "33", PHP_INI_SYSTEM, OnUpdateLong, global_min_uid, zend_suphp_globals, suphp_globals)
	STD_PHP_INI_ENTRY("suphp.use_effective_uid", "0", PHP_INI_SYSTEM, OnUpdateLong, global_use_effective_uid, zend_suphp_globals, suphp_globals)
	STD_PHP_INI_ENTRY("suphp.chown_uploads", "1", PHP_INI_SYSTEM, OnUpdateLong, global_chown_uploads, zend_suphp_globals, suphp_globals)
PHP_INI_END()

static void php_suphp_init_globals(zend_suphp_globals *suphp_globals)
{
	suphp_globals->global_enabled = 1;
	suphp_globals->global_min_uid = 33;
	suphp_globals->global_use_effective_uid = 0;
	suphp_globals->global_chown_uploads = 1;
}

int check_uploads(long st_uid)
{
	// uploads are handled by apache and hence do
	// not have the proper file ownership
	// solution : seteuid to root then chown

	HashTable *ht = SG(rfc1867_uploaded_files);
	char *filename;
	ulong length;
		
	if( ht == NULL || zend_hash_num_elements(ht) == 0 )
		return 0;

	if( setegid(0) != 0 || seteuid(0) != 0 )
		return -1;

	zend_hash_internal_pointer_reset(ht);
	while( zend_hash_has_more_elements(ht) == SUCCESS )
	{
		if( zend_hash_get_current_key(ht, &filename, &length, 0) != HASH_KEY_IS_STRING )
			continue;

		if( chown(filename, st_uid, st_uid) != 0 )
			return -1;

		if( zend_hash_move_forward(ht) != SUCCESS )
                        break;
	}
	
	return 0;
}

int suphp()
{
	struct stat st;
	char *filename = SG(request_info).path_translated;
	
	if( stat(filename, &st) != 0 )
		return -1;

	if( st.st_uid < suphp_globals.global_min_uid )
		return -1;

	if( suphp_globals.global_chown_uploads == 1 )
		check_uploads(st.st_uid);

	if( suphp_globals.global_use_effective_uid == 1 )
	{
		if( setegid(st.st_uid) != 0 )
			return -1;
		if( seteuid(st.st_uid) != 0 )
			return -1;
	}
	else
	{
		if( setgid(st.st_uid) != 0 )
			return -1;
		if( setuid(st.st_uid) != 0 )
			return -1;
	}

	return 0;
}

PHP_MINIT_FUNCTION(suphp)
{
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(suphp)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(suphp)
{
	if( suphp_globals.global_enabled > 0 && suphp() < 0 )
	{
		zend_bailout();
		return FAILURE;
	}
	else
		return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(suphp)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(suphp)
{
	php_info_print_table_start();
	php_info_print_table_header(1, "The suphp PHP module has been developped explicitely for the Olympe Network. It allows to execute PHP with privileges of the script file owner. Be sure to chown root and chmod +s the PHP executable.");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "Suphp module", "Loaded");
	php_info_print_table_row(2, "Developper", "Simon Uyttendaele");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

