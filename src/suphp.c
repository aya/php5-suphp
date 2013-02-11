/*
  +---------------------------------------------------------------------+
  | PHP Version 5 - Copyright (c) 1997-2007 The PHP Group               |
  +---------------------------------------------------------------------+
  | Author: Simon Uyttendaele                                           |
  | Parts of this source file may be subject to version 3.01 of the PHP |
  | license. http://www.php.net/license/3_01.txt                        |
  |                                                                     |
  | Uncovered parts from the above license are subject to the FreeBSD   |
  | license below with respective copyrights.                           |
  +---------------------------------------------------------------------+
 
Copyright (c) 2012, Simon Uyttendaele <simon@olympe-network.com>
Granted Copyright (c) 2012, Olympe Network <contact@olympe-network.com>
Granted Copyright (c) 2012, SYS S.A.S. <contact@anotherservice.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

zend_function_entry suphp_functions[] = {
	{NULL, NULL, NULL}
};

zend_module_entry suphp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_SUPHP_EXTNAME,
	suphp_functions,
	PHP_MINIT(suphp),
	PHP_MSHUTDOWN(suphp),
	PHP_RINIT(suphp),
	PHP_RSHUTDOWN(suphp),
	PHP_MINFO(suphp),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_SUPHP_EXTVER, /* module version number */
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
		return SUCCESS;

	if( setegid(0) != 0 || seteuid(0) != 0 )
		return FAILURE;

	zend_hash_internal_pointer_reset(ht);
	while( zend_hash_has_more_elements(ht) == SUCCESS )
	{
		if( zend_hash_get_current_key(ht, &filename, &length, 0) != HASH_KEY_IS_STRING )
			continue;

		if( chown(filename, st_uid, st_uid) != 0 )
			return FAILURE;

		if( zend_hash_move_forward(ht) != SUCCESS )
                        break;
	}
	
	return SUCCESS;
}

int suphp()
{
	struct stat st;
	char *filename = SG(request_info).path_translated;

	// we are running php without a filename (like -i or -v)
	if( filename == NULL )
		return SUCCESS;
	
	if( stat(filename, &st) != 0 )
		return FAILURE;

	if( st.st_uid < suphp_globals.global_min_uid )
		return FAILURE;

	if( suphp_globals.global_chown_uploads == 1 )
		check_uploads(st.st_uid);

	if( suphp_globals.global_use_effective_uid == 1 )
	{
		if( setegid(st.st_uid) != 0 )
			return FAILURE;
		if( seteuid(st.st_uid) != 0 )
			return FAILURE;
	}
	else
	{
		if( setgid(st.st_uid) != 0 )
			return FAILURE;
		if( setuid(st.st_uid) != 0 )
			return FAILURE;
	}

	return SUCCESS;
}

PHP_MINIT_FUNCTION(suphp)
{
	ZEND_INIT_MODULE_GLOBALS(suphp, php_suphp_init_globals, NULL);

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

