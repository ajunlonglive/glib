/*
  +----------------------------------------------------------------------+
  | For PHP Version 7                                                    |
  +----------------------------------------------------------------------+
  | Copyright (c) 2017 Elizabeth M Smith                                 |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  | Also available in LICENSE                                            |
  +----------------------------------------------------------------------+
  | Author: Elizabeth M Smith <auroraeosrose@gmail.com>                  |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <php.h>
#include <zend_exceptions.h>
#include <ext/spl/spl_exceptions.h>

#include <glib.h>

#include "php_glib.h"
#include "php_glib_internal.h"

zend_class_entry *ce_glib_main_context;
static zend_object_handlers glib_main_context_object_handlers;

typedef struct _glib_main_context_object {
	GMainContext *main_context;
	zend_object std;
} glib_main_context_object;

static inline glib_main_context_object *glib_main_context_fetch_object(zend_object *object)
{
	return (glib_main_context_object *) ((char*)(object) - XtOffsetOf(glib_main_context_object, std));
}
#define Z_GLIB_MAIN_CONTEXT_P(zv) glib_main_context_fetch_object(Z_OBJ_P(zv))

/* ----------------------------------------------------------------
    Glib\Main\Context class API
------------------------------------------------------------------*/

ZEND_BEGIN_ARG_INFO(Context___construct_args, ZEND_SEND_BY_VAL)
ZEND_END_ARG_INFO()

/* {{{ proto void \Glib\Main\Context->__construct()
        Creates a new main context object, holding a list of sources to be handled in a main loop
		generally you will want to call \Glib\Main\Context::getDefault(); instead
   */
PHP_METHOD(GlibMainContext, __construct)
{
	if (zend_parse_parameters_none_throw() == FAILURE) {
		return;
	}
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO(Context_iteration_args, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, may_block, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

/* {{{ proto bool \Glib\Main\Context->iteration(boolean may_block)
		Runs a single iteration for the given main loop. This involves checking to see if any event sources
		are ready to be processed, then if no events sources are ready and may_block is TRUE, waiting for a
		source to become ready, then dispatching the highest priority events sources that are ready. Otherwise,
		if may_block is FALSE sources are not waited to become ready, only those highest priority events sources
		will be dispatched (if any), that are ready at this given moment without further waiting.
  */ 
PHP_METHOD(GlibMainContext, iteration)
{
	glib_main_context_object *context_object;
	zend_bool may_block = FALSE;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "b", &may_block) == FAILURE) {
		return;
	}

	context_object = Z_GLIB_MAIN_CONTEXT_P(getThis());

	RETURN_BOOL(g_main_context_iteration(context_object->main_context, may_block));
}

/* }}} */

/* {{{ proto \Glib\Main\Context object \Glib\Main\Context::getDefault();
        Returns the default main context. This is the main context used for
		main loop functions when a main loop is not explicitly specified.
   
PHP_METHOD(Glib_Main_Context, getDefault)
{
	glib_maincontext_object *context_object;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	object_init_ex(return_value, glib_ce_maincontext);
	context_object = (glib_maincontext_object *)zend_objects_get_address(return_value TSRMLS_CC);
	context_object->maincontext = g_main_context_new();
	g_main_context_ref(context_object->maincontext);
}
/* }}} */



/* {{{ proto bool \Glib\Main\Context->pending()
		Checks if any sources have pending events for the given context.
   
PHP_METHOD(Glib_Main_Context, pending)
{
	glib_maincontext_object *context_object = (glib_maincontext_object *)glib_maincontext_object_get(getThis() TSRMLS_CC);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_BOOL(g_main_context_pending(context_object->maincontext));
}
/* }}} */

/* {{{ proto void \Glib\Main\Context->wakeup()
		If context is currently waiting in a poll(), interrupt the poll(), and continue the iteration process.
   
PHP_METHOD(Glib_Main_Context, wakeup)
{
	glib_maincontext_object *context_object = (glib_maincontext_object *)glib_maincontext_object_get(getThis() TSRMLS_CC);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	g_main_context_wakeup(context_object->maincontext);
}
/* }}} */

/* {{{ proto array(boolean, int) \Glib\Main\Context->prepare()
		Checks if any sources have pending events for the given context.
		Returns an array of status (true if some source is ready to be dispatched prior to polling)
		and integer priority (priority of highest priority source already ready)
   
PHP_METHOD(Glib_Main_Context, prepare)
{
	gint priority;
	gboolean status;
	glib_maincontext_object *context_object = (glib_maincontext_object *)glib_maincontext_object_get(getThis() TSRMLS_CC);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	status = g_main_context_prepare(context_object->maincontext, &priority);
	array_init(return_value);
	add_next_index_bool(return_value, status);
	add_next_index_long(return_value, priority);
}
/* }}} */

/* {{{ proto void \Glib\Main\Context->dispatch()
		Dispatches all pending sources
   
PHP_METHOD(Glib_Main_Context, dispatch)
{
	glib_maincontext_object *context_object = (glib_maincontext_object *)glib_maincontext_object_get(getThis() TSRMLS_CC);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	g_main_context_dispatch(context_object->maincontext);
}
/* }}} */

/* ----------------------------------------------------------------
    Glib\Main\Context Object management
------------------------------------------------------------------*/

/* Custom Object Destruction - unrefs our context */
void glib_main_context_free_obj(zend_object *object)
{
	glib_main_context_object *intern = glib_main_context_fetch_object(object);

	if(!intern) {
		return;
	}
	if(intern->main_context) {
		g_main_context_unref(intern->main_context);
	}
	intern->main_context = NULL;

	zend_object_std_dtor(&intern->std);
}

/* Custom object creation - calls g_main_context_new and ref */
static zend_object* glib_main_context_create_object(zend_class_entry *ce)
{
	glib_main_context_object *intern = NULL;

	intern = ecalloc(1, sizeof(glib_main_context_object) + zend_object_properties_size(ce));
	intern->main_context = g_main_context_new();
	intern->main_context = g_main_context_ref(intern->main_context);

	zend_object_std_init(&(intern->std), ce);
	object_properties_init(&(intern->std), ce);
	intern->std.handlers = &glib_main_context_object_handlers;
	return &(intern->std);
}

/* ----------------------------------------------------------------
    Glib\MainContext Class Definition and Registration
------------------------------------------------------------------*/

static const zend_function_entry glib_main_context_methods[] = {
	PHP_ME(GlibMainContext, __construct, Context___construct_args, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	//PHP_ME(GlibMainContext, getDefault, Context_getDefault_args, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	//PHP_ME(GlibTimer, start, Timer_start_args, ZEND_ACC_PUBLIC)
	//PHP_ME(GlibTimer, stop, Timer_stop_args, ZEND_ACC_PUBLIC)
	//PHP_ME(GlibTimer, continue, Timer_continue_args, ZEND_ACC_PUBLIC)
	PHP_ME(GlibMainContext, iteration, Context_iteration_args, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

PHP_MINIT_FUNCTION(glib_main_context)
{
	zend_class_entry ce;

	memcpy(&glib_main_context_object_handlers, zend_get_std_object_handlers(),
		sizeof(zend_object_handlers));
	glib_main_context_object_handlers.offset = XtOffsetOf(glib_main_context_object, std);
	glib_main_context_object_handlers.clone_obj = NULL;
	glib_main_context_object_handlers.free_obj = glib_main_context_free_obj;

	INIT_NS_CLASS_ENTRY(ce, GLIB_NAMESPACE, ZEND_NS_NAME("Main", "Context"), glib_main_context_methods);
	ce.create_object = glib_main_context_create_object;
	ce_glib_main_context = zend_register_internal_class(&ce);

	return SUCCESS;
}
