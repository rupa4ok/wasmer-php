/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Ivan Enderlin                                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_wasm.h"
#include "wasmer.h"

/**
 * `wasm_read_bytes`.
 */

char* wasm_bytes_resource_name;
int wasm_bytes_resource_number;

wasmer_byte_array *wasm_bytes_from_resource(zend_resource *wasm_bytes_resource)
{
    return (wasmer_byte_array *) zend_fetch_resource(
        wasm_bytes_resource,
        wasm_bytes_resource_name,
        wasm_bytes_resource_number
    );
}

static void wasm_bytes_destructor(zend_resource *resource)
{
    wasmer_byte_array *wasm_byte_array = wasm_bytes_from_resource(resource);
    free((uint8_t *) wasm_byte_array->bytes);
    free(wasm_byte_array);
}

PHP_FUNCTION(wasm_read_bytes)
{
    char *file_path;
    size_t file_path_length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p", &file_path, &file_path_length) == FAILURE) {
        return;
    }

    FILE *wasm_file = fopen(file_path, "r");
    fseek(wasm_file, 0, SEEK_END);

    size_t wasm_file_length = ftell(wasm_file);
    const uint8_t *wasm_bytes = malloc(wasm_file_length);
    fseek(wasm_file, 0, SEEK_SET);

    fread((uint8_t *) wasm_bytes, 1, wasm_file_length, wasm_file);

    fclose(wasm_file);

    wasmer_byte_array *wasm_byte_array = malloc(sizeof(wasmer_byte_array));
    wasm_byte_array->bytes = wasm_bytes;
    wasm_byte_array->bytes_len = (uint32_t) wasm_file_length;

    zend_resource *resource = zend_register_resource((void *) wasm_byte_array, wasm_bytes_resource_number);

    RETURN_RES(resource);
}

//const Value* invoke_imported_function_from_host(
//    const /* zend_fcall_info */ void *function_implementation,
//    const /* zend_fcall_info_cache */ void *function_implementation_cache,
//    const FunctionInputs *function_inputs
//) {
//    zend_fcall_info *fci = (zend_fcall_info *) function_implementation;
//    zend_fcall_info_cache *fci_cache = (zend_fcall_info_cache *) function_implementation_cache;
//    zval output;
//
//    uintptr_t inputs_length = function_inputs->inputs_length;
//    zval *inputs = (zval *) malloc(inputs_length * sizeof(zval));
//
//    for (uintptr_t nth = 0; nth < inputs_length; ++nth) {
//        const Value value = function_inputs->inputs_buffer[nth];
//        Value_Tag tag = value.tag;
//
//        if (tag == I32) {
//            ZVAL_LONG(&inputs[nth], value.i32._0);
//        } else if (tag == I64) {
//            ZVAL_LONG(&inputs[nth], value.i64._0);
//        } else if (tag == F32) {
//            ZVAL_DOUBLE(&inputs[nth], value.f32._0);
//        } else if (tag == F64) {
//            ZVAL_DOUBLE(&inputs[nth], value.f64._0);
//        } else if (tag == None) {
//            free(inputs);
//
//            return NULL;
//        }
//    }
//
//    fci->retval = &output;
//    fci->param_count = inputs_length;
//    fci->params = inputs;
//    fci->no_separation = 0;
//
//    if (zend_call_function(fci, fci_cache) != SUCCESS) {
//        free(inputs);
//
//        return NULL;
//    }
//
//    free(inputs);
//
//    switch (Z_TYPE_P(&output)) {
//        case IS_LONG: {
//            I32_Body value = { output.value.lval };
//            Value* function_output = (Value*) malloc(sizeof(Value));
//            function_output->tag = I32;
//            function_output->i32 = value;
//
//            return function_output;
//        }
//
//        case IS_DOUBLE: {
//            F32_Body value = { output.value.dval };
//            Value* function_output = (Value*) malloc(sizeof(Value));
//            function_output->tag = F32;
//            function_output->f32 = value;
//
//            return function_output;
//        }
//
//        default:
//            return NULL;
//    }
//}
//
///**
// * `wasm_runtime_add_function`.
// */
//
//PHP_FUNCTION(wasm_runtime_add_function)
//{
//    zval *wasm_runtime_resource;
//    double index;
//    char *function_name;
//    size_t function_name_length;
//    HashTable *signature;
//    zend_fcall_info *function_implementation = (zend_fcall_info *) malloc(sizeof(zend_fcall_info));
//    zend_fcall_info_cache *function_implementation_cache =  (zend_fcall_info_cache *) malloc(sizeof(zend_fcall_info_cache));
//
//    if (
//        zend_parse_parameters(
//            ZEND_NUM_ARGS() TSRMLS_CC,
//            "rdshf",
//            &wasm_runtime_resource,
//            &index,
//            &function_name,
//            &function_name_length,
//            &signature,
//            function_implementation,
//            function_implementation_cache
//        ) == FAILURE
//    ) {
//        free(&function_implementation);
//        free(&function_implementation_cache);
//
//        return;
//    }
//
//    size_t signature_length = zend_hash_num_elements(signature);
//
//    // Must at least contain the output type.
//    if (signature_length == 0) {
//        free(&function_implementation);
//        free(&function_implementation_cache);
//
//        RETURN_FALSE
//    }
//
//    ValueType *ffi_signature = (ValueType*) malloc(signature_length * sizeof(ValueType));
//
//    {
//        zend_ulong key;
//        zval *value;
//
//        ZEND_HASH_FOREACH_NUM_KEY_VAL(signature, key, value)
//            ffi_signature[key] = value->value.lval;
//        ZEND_HASH_FOREACH_END();
//    }
//
//    const Value* (*callback)(const void*, const void*, const FunctionInputs*);
//    callback = &invoke_imported_function_from_host;
//
//    Runtime *wasm_runtime = wasm_runtime_from_resource(Z_RES_P(wasm_runtime_resource));
//    wasm_runtime_add_function(
//        wasm_runtime,
//        index,
//        function_name,
//        ffi_signature,
//        signature_length,
//        &callback,
//        function_implementation,
//        function_implementation_cache
//    );
//
//    free(&ffi_signature);
//
//    RETURN_TRUE
//}

/**
 * `wasm_new_function_import`.
 */

char* wasm_function_import_resource_name;
int wasm_function_import_resource_number;

typedef struct {
    wasmer_import_t *wasmer_import;
    zend_fcall_info *function_implementation;
    zend_fcall_info_cache *function_implementation_cache;
} wasm_function_import_t;

wasm_function_import_t *wasm_function_import_from_resource(zend_resource *wasm_function_import_resource)
{
    return (wasm_function_import_t *) zend_fetch_resource(
        wasm_function_import_resource,
        wasm_function_import_resource_name,
        wasm_function_import_resource_number
    );
}

static void wasm_function_import_destructor(zend_resource *resource)
{
    wasm_function_import_t *wasm_function_import = wasm_function_import_from_resource(resource);
    wasmer_import_func_destroy((wasmer_import_func_t *) wasm_function_import->wasmer_import->value.func);
    free(wasm_function_import->wasmer_import);
    free(wasm_function_import->function_implementation);
    free(wasm_function_import->function_implementation_cache);
    free(wasm_function_import);
}

typedef struct {
    zend_llist imported_function_implementations;
    int i;
} wasm_instance_context_data;

int32_t foo(const wasmer_instance_context_t *context, int32_t x, int32_t y)
{
    wasm_instance_context_data *wasm_context_data = wasmer_instance_context_data_get(context);
    printf("here %d %d!\n", x, y);
    printf("context %d\n", wasm_context_data->i);
    return x + y + 1;
}

PHP_FUNCTION(wasm_new_function_import)
{
    char *function_name;
    size_t function_name_length;
    HashTable *inputs_signature;
    zend_long *output_signature;
    zend_fcall_info *function_implementation = malloc(sizeof(zend_fcall_info));
    zend_fcall_info_cache *function_implementation_cache = malloc(sizeof(zend_fcall_info_cache));

    if (
        zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC,
            "shlf",
            &function_name,
            &function_name_length,
            &inputs_signature,
            &output_signature,
            function_implementation,
            function_implementation_cache
        ) == FAILURE
    ) {
        return;
    }

    size_t inputs_signature_length = zend_hash_num_elements(inputs_signature);
    size_t outputs_signature_length = 1;

    wasmer_value_tag *wasm_function_inputs_signature = malloc(sizeof(wasmer_value_tag) * inputs_signature_length);
    wasmer_value_tag wasm_function_outputs_signature[] = {(uint32_t) output_signature};

    {
        zend_ulong key;
        zval *value;

        ZEND_HASH_FOREACH_NUM_KEY_VAL(inputs_signature, key, value)
            wasm_function_inputs_signature[key] = value->value.lval;
        ZEND_HASH_FOREACH_END();
    }

    const wasmer_import_func_t *wasm_function_import = wasmer_import_func_new(
        (void (*)(void *)) foo,
        wasm_function_inputs_signature,
        inputs_signature_length,
        wasm_function_outputs_signature,
        outputs_signature_length
    );

    const char *module_name = "env";
    wasmer_byte_array wasm_module_name;
    wasm_module_name.bytes = (const uint8_t *) module_name;
    wasm_module_name.bytes_len = strlen(module_name);

    wasmer_byte_array wasm_function_name;
    wasm_function_name.bytes = (const uint8_t *) function_name;
    wasm_function_name.bytes_len = function_name_length;

    wasmer_import_t *wasm_import = malloc(sizeof(wasmer_import_t));
    wasm_import->module_name = wasm_module_name;
    wasm_import->import_name = wasm_function_name;
    wasm_import->tag = WASM_FUNCTION;
    wasm_import->value.func = wasm_function_import;

    wasm_function_import_t *function_import = malloc(sizeof(wasm_function_import_t));
    function_import->wasmer_import = wasm_import;
    function_import->function_implementation = function_implementation;
    function_import->function_implementation_cache = function_implementation_cache;

    zend_resource *resource = zend_register_resource((void *) function_import, wasm_function_import_resource_number);

    RETURN_RES(resource);
}

/**
 * `wasm_new_instance`.
 */

char* wasm_instance_resource_name;
int wasm_instance_resource_number;

wasmer_instance_t *wasm_instance_from_resource(zend_resource *wasm_instance_resource)
{
    return (wasmer_instance_t *) zend_fetch_resource(
        wasm_instance_resource,
        wasm_instance_resource_name,
        wasm_instance_resource_number
    );
}

static void wasm_instance_destructor(zend_resource *resource)
{
    wasmer_instance_t *wasm_instance = wasm_instance_from_resource(resource);
    wasmer_instance_destroy(wasm_instance);
}

PHP_FUNCTION(wasm_new_instance)
{
    zval *wasm_bytes_resource;
    HashTable *imports = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|h", &wasm_bytes_resource, &imports) == FAILURE) {
        return;
    }

    wasmer_byte_array *wasm_byte_array = wasm_bytes_from_resource(Z_RES_P(wasm_bytes_resource));
    wasmer_import_t *wasm_imports;
    int number_of_imports = 0;

    if (imports == NULL) {
        wasm_imports = NULL;
    } else {
        number_of_imports = zend_hash_num_elements(imports);
        wasm_imports = malloc(sizeof(wasmer_import_t) * number_of_imports);

        {
            zend_ulong key;
            zval *value;

            ZEND_HASH_FOREACH_NUM_KEY_VAL(imports, key, value)
                wasm_imports[key] = *wasm_function_import_from_resource(Z_RES_P(value))->wasmer_import;
            ZEND_HASH_FOREACH_END();
        }
    }

    wasmer_instance_t *wasm_instance = NULL;
    wasmer_result_t wasm_instantiation_result = wasmer_instantiate(
        &wasm_instance,
        (uint8_t *) wasm_byte_array->bytes,
        wasm_byte_array->bytes_len,
        wasm_imports,
        number_of_imports
    );

    if (wasm_instantiation_result != WASMER_OK) {
        free(wasm_instance);

        RETURN_NULL();
    }

    wasm_instance_context_data *wasm_context_data = malloc(sizeof(wasm_instance_context_data));
    wasm_context_data->i = 42;

    wasmer_instance_context_data_set(wasm_instance, wasm_context_data);

    zend_resource *resource = zend_register_resource((void *) wasm_instance, wasm_instance_resource_number);

    RETURN_RES(resource);
}

/**
 * `wasm_get_function_signature`.
 */

PHP_FUNCTION(wasm_get_function_signature)
{
    zval *wasm_instance_resource;
    char *function_name;
    size_t function_name_length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &wasm_instance_resource, &function_name, &function_name_length) == FAILURE) {
        return;
    }

    wasmer_instance_t *wasm_instance = wasm_instance_from_resource(Z_RES_P(wasm_instance_resource));
    wasmer_exports_t *wasm_exports = NULL;

    wasmer_instance_exports(wasm_instance, &wasm_exports);

    int number_of_exports = wasmer_exports_len(wasm_exports);

    if (number_of_exports == 0) {
        wasmer_exports_destroy(wasm_exports);

        RETURN_NULL();
    }

    const wasmer_export_func_t *wasm_function = NULL;

    for (uint32_t nth = 0; nth < number_of_exports; ++nth) {
        wasmer_export_t *wasm_export = wasmer_exports_get(wasm_exports, nth);
        wasmer_import_export_kind wasm_export_kind = wasmer_export_kind(wasm_export);

        if (wasm_export_kind != WASM_FUNCTION) {
            continue;
        }

        wasmer_byte_array wasm_export_name = wasmer_export_name(wasm_export);

        if (wasm_export_name.bytes_len != function_name_length) {
            continue;
        }

        if (strncmp(function_name, (const char *) wasm_export_name.bytes, wasm_export_name.bytes_len) == 0) {
            wasm_function = wasmer_export_to_func(wasm_export);

            break;
        }
    }

    if (wasm_function == NULL) {
        wasmer_exports_destroy(wasm_exports);

        RETURN_NULL();
    }

    uint32_t wasm_function_inputs_arity;
    wasmer_export_func_params_arity(wasm_function, &wasm_function_inputs_arity);

    array_init_size(return_value, wasm_function_inputs_arity + /* output */ 1);

    wasmer_value_tag *wasm_function_input_signatures = malloc(sizeof(wasmer_value_tag) * wasm_function_inputs_arity);
    wasmer_export_func_params(wasm_function, wasm_function_input_signatures, wasm_function_inputs_arity);

    for (uint32_t nth = 0; nth < wasm_function_inputs_arity; ++nth) {
        add_next_index_long(return_value, wasm_function_input_signatures[nth]);
    }

    free(wasm_function_input_signatures);

    uint32_t wasm_function_outputs_arity;
    wasmer_export_func_returns_arity(wasm_function, &wasm_function_outputs_arity);

    if (wasm_function_outputs_arity == 0) {
        RETURN_NULL();
    }

    wasmer_value_tag *wasm_function_output_signatures = malloc(sizeof(wasmer_value_tag) * wasm_function_outputs_arity);
    wasmer_export_func_returns(wasm_function, wasm_function_output_signatures, wasm_function_outputs_arity);

    add_next_index_long(return_value, wasm_function_output_signatures[0]);

    free(wasm_function_output_signatures);
    wasmer_exports_destroy(wasm_exports);
}

/**
 * `wasm_value`.
 */

char* wasm_value_resource_name;
int wasm_value_resource_number;

wasmer_value_t *wasm_value_from_resource(zend_resource *wasm_value_resource)
{
    return (wasmer_value_t *) zend_fetch_resource(
        wasm_value_resource,
        wasm_value_resource_name,
        wasm_value_resource_number
    );
}

static void wasm_value_destructor(zend_resource *resource)
{
    wasmer_value_t *wasm_value = wasm_value_from_resource(resource);
    free(wasm_value);
}

PHP_FUNCTION(wasm_value)
{
    zend_long value_type;
    zval *value;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &value_type, &value) == FAILURE) {
        return;
    }

    if (value_type < 0) {
        RETURN_NULL();
    }

    uint32_t type = (uint32_t) value_type;
    wasmer_value_t *wasm_value = malloc(sizeof(wasmer_value_t));

    if (type == WASM_I32) {
        wasm_value->tag = type;
        wasm_value->value.I32 = (int32_t) value->value.lval;
    } else if (type == WASM_I64) {
        wasm_value->tag = type;
        wasm_value->value.I64 = (int64_t) value->value.lval;
    } else if (type == WASM_F32) {
        wasm_value->tag = type;
        wasm_value->value.F32 = (float) value->value.dval;
    } else if (type == WASM_F64) {
        wasm_value->tag = type;
        wasm_value->value.F64 = (double) value->value.dval;
    } else {
        free(wasm_value);

        RETURN_NULL();
    }

    zend_resource *resource = zend_register_resource((void *) wasm_value, wasm_value_resource_number);

    RETURN_RES(resource);
}

/**
 * `wasm_invoke_function`.
 */

PHP_FUNCTION(wasm_invoke_function)
{
    zval *wasm_instance_resource;
    char *function_name;
    size_t function_name_length;
    HashTable *inputs;

    if (
        zend_parse_parameters(
            ZEND_NUM_ARGS() TSRMLS_CC,
            "rsh",
            &wasm_instance_resource,
            &function_name,
            &function_name_length,
            &inputs
        ) == FAILURE
    ) {
        return;
    }

    wasmer_instance_t *wasm_instance = wasm_instance_from_resource(Z_RES_P(wasm_instance_resource));

    size_t function_input_length = zend_hash_num_elements(inputs);
    wasmer_value_t *function_inputs = malloc(sizeof(wasmer_value_t) * function_input_length);

    {
        zend_ulong key;
        zval *value;

        ZEND_HASH_FOREACH_NUM_KEY_VAL(inputs, key, value)
            function_inputs[key] = *wasm_value_from_resource(Z_RES_P(value));
        ZEND_HASH_FOREACH_END();
    }

    size_t function_output_length = 1;
    wasmer_value_t output;
    wasmer_value_t function_outputs[] = {output};

    wasmer_result_t function_call_result = wasmer_instance_call(
        wasm_instance,
        function_name,
        function_inputs,
        function_input_length,
        function_outputs,
        function_output_length
    );

    if (function_call_result != WASMER_OK) {
        RETURN_FALSE
    }

    wasmer_value_t function_output = function_outputs[0];

    if (function_output.tag == WASM_I32) {
        RETURN_LONG(function_output.value.I32);
    } else if (function_output.tag == WASM_I64) {
        RETURN_LONG(function_output.value.I64);
    } else if (function_output.tag == WASM_F32) {
        RETURN_DOUBLE(function_output.value.F32);
    } else if (function_output.tag == WASM_F64) {
        RETURN_DOUBLE(function_output.value.F64);
    } else {
        RETURN_NULL();
    }
}

PHP_RINIT_FUNCTION(wasm)
{
#if defined(ZTS) && defined(COMPILE_DL_WASM)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}

PHP_MINIT_FUNCTION(wasm)
{
    REGISTER_LONG_CONSTANT("WASM_TYPE_I32", WASM_I32, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WASM_TYPE_I64", WASM_I64, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WASM_TYPE_F32", WASM_F32, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WASM_TYPE_F64", WASM_F64, CONST_CS | CONST_PERSISTENT);

    wasm_bytes_resource_name = "wasm_bytes";
    wasm_bytes_resource_number = zend_register_list_destructors_ex(
        wasm_bytes_destructor,
        NULL,
        wasm_bytes_resource_name,
        module_number
    );

    wasm_function_import_resource_name = "wasm_function_import";
    wasm_function_import_resource_number = zend_register_list_destructors_ex(
        wasm_function_import_destructor,
        NULL,
        wasm_function_import_resource_name,
        module_number
    );

    wasm_instance_resource_name = "wasm_instance";
    wasm_instance_resource_number = zend_register_list_destructors_ex(
        wasm_instance_destructor,
        NULL,
        wasm_instance_resource_name,
        module_number
    );

    wasm_instance_resource_name = "wasm_instance";
    wasm_instance_resource_number = zend_register_list_destructors_ex(
        wasm_instance_destructor,
        NULL,
        wasm_instance_resource_name,
        module_number
    );

    wasm_value_resource_name = "wasm_value";
    wasm_value_resource_number = zend_register_list_destructors_ex(
        wasm_value_destructor,
        NULL,
        wasm_value_resource_name,
        module_number
    );

    return SUCCESS;
}

PHP_MINFO_FUNCTION(wasm)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "wasm support", "enabled");
    php_info_print_table_end();
}

ZEND_BEGIN_ARG_INFO(arginfo_wasm_read_bytes, 0)
    ZEND_ARG_TYPE_INFO(0, file_path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wasm_new_function_import, 0)
    ZEND_ARG_TYPE_INFO(0, function_name, IS_STRING, 0)
    ZEND_ARG_ARRAY_INFO(0, inputs_signature, 0)
    ZEND_ARG_TYPE_INFO(0, output_signature, IS_LONG, 0)
    ZEND_ARG_CALLABLE_INFO(0, function_implementation, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_wasm_new_instance, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, wasm_bytes, IS_RESOURCE, 0)
    ZEND_ARG_ARRAY_INFO(0, wasm_imports, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wasm_get_function_signature, 0)
    ZEND_ARG_TYPE_INFO(0, wasm_instance, IS_RESOURCE, 0)
    ZEND_ARG_TYPE_INFO(0, function_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wasm_value, 0)
    ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wasm_invoke_function, 0)
    ZEND_ARG_TYPE_INFO(0, wasm_instance, IS_RESOURCE, 0)
    ZEND_ARG_TYPE_INFO(0, function_name, IS_STRING, 0)
    ZEND_ARG_ARRAY_INFO(0, inputs, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry wasm_functions[] = {
    PHP_FE(wasm_read_bytes,				arginfo_wasm_read_bytes)
    PHP_FE(wasm_new_function_import,	arginfo_wasm_new_function_import)
    PHP_FE(wasm_new_instance,			arginfo_wasm_new_instance)
    PHP_FE(wasm_get_function_signature,	arginfo_wasm_get_function_signature)
    PHP_FE(wasm_value,					arginfo_wasm_value)
    PHP_FE(wasm_invoke_function,		arginfo_wasm_invoke_function)
    PHP_FE_END
};

zend_module_entry wasm_module_entry = {
    STANDARD_MODULE_HEADER,
    "wasm",					/* Extension name */
    wasm_functions,			/* zend_function_entry */
    PHP_MINIT(wasm),		/* PHP_MINIT - Module initialization */
    NULL,					/* PHP_MSHUTDOWN - Module shutdown */
    PHP_RINIT(wasm),		/* PHP_RINIT - Request initialization */
    NULL,					/* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(wasm),		/* PHP_MINFO - Module info */
    PHP_WASM_VERSION,		/* Version */
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_WASM
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
    ZEND_GET_MODULE(wasm)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
