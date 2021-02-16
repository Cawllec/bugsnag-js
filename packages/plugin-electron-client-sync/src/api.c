#include <assert.h>
#include <node_api.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bugsnag_electron_client_sync.h"
#include "js_native_api_types.h"

static napi_value json_stringify(napi_env env, napi_value json_obj) {
  napi_value global;
  napi_status status = napi_get_global(env, &global);
  assert(status == napi_ok);

  napi_value JSON;
  status = napi_get_named_property(env, global, "JSON", &JSON);
  assert(status == napi_ok);

  napi_value stringify;
  status = napi_get_named_property(env, JSON, "stringify", &stringify);
  assert(status == napi_ok);

  napi_value argv[] = {json_obj};
  napi_value result;
  status = napi_call_function(env, JSON, stringify, 1, argv, &result);
  assert(status == napi_ok);

  return result;
}

static char *read_string_value(napi_env env, napi_value arg, bool allow_null) {
  napi_valuetype valuetype;
  napi_status status = napi_typeof(env, arg, &valuetype);
  assert(status == napi_ok);

  switch (valuetype) {
  case napi_string: {
    size_t len;
    status = napi_get_value_string_utf8(env, arg, NULL, 0, &len);
    assert(status == napi_ok);

    char *string = calloc(len + 1, sizeof(char));
    status = napi_get_value_string_utf8(env, arg, string, len + 1, NULL);
    assert(status == napi_ok);

    return string;
  }
  case napi_null:
    if (allow_null) {
      return NULL;
    }
    // fall through
  default:
    napi_throw_type_error(env, NULL, "Wrong argument type, expected string");
  }

  return NULL;
}

static char *read_json_string_value(napi_env env, napi_value arg,
                                    bool allow_null) {
  napi_valuetype valuetype;
  napi_status status = napi_typeof(env, arg, &valuetype);
  assert(status == napi_ok);

  switch (valuetype) {
  case napi_object:
    return read_string_value(env, json_stringify(env, arg), allow_null);
  case napi_string:
    return read_string_value(env, arg, allow_null);
  case napi_null:
    if (allow_null) {
      return NULL;
    }
  default:
    napi_throw_type_error(env, NULL,
                          "Wrong argument type, expected object or string");
    return NULL;
  }
}

static void set_object_or_null(napi_env env, napi_value obj,
                               BECS_STATUS (*setter)(const char *)) {
  napi_valuetype valuetype;
  napi_status status = napi_typeof(env, obj, &valuetype);
  assert(status == napi_ok);

  switch (valuetype) {
  case napi_null:
    setter(NULL);
    break;
  case napi_object: {
    char *value = read_string_value(env, json_stringify(env, obj), false);
    if (value) {
      setter(value);
      free(value);
    }
  } break;
  default:
    napi_throw_type_error(env, NULL, "Wrong argument type, expected object");
  }
}

static napi_value Uninstall(napi_env env, napi_callback_info info) {
  becs_uninstall();
  return NULL;
}

static napi_value Install(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 2");
    return NULL;
  }

  napi_valuetype valuetype0;
  status = napi_typeof(env, args[0], &valuetype0);
  assert(status == napi_ok);

  napi_valuetype valuetype1;
  status = napi_typeof(env, args[1], &valuetype1);
  assert(status == napi_ok);

  if (valuetype0 != napi_string || valuetype1 != napi_number) {
    napi_throw_type_error(env, NULL,
                          "Wrong argument types, expected (string, number)");
    return NULL;
  }

  char *filepath = read_string_value(env, args[0], false);
  if (!filepath) {
    return NULL;
  }

  double max_crumbs;
  status = napi_get_value_double(env, args[1], &max_crumbs);
  assert(status == napi_ok);

  becs_install(filepath, max_crumbs);
  free(filepath);

  return NULL;
}

static napi_value UpdateContext(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 1");
    return NULL;
  }

  napi_valuetype valuetype0;
  status = napi_typeof(env, args[0], &valuetype0);
  assert(status == napi_ok);

  if (valuetype0 == napi_string) {
    char *context = read_string_value(env, args[0], false);
    becs_set_context(context);
    free(context);
  } else if (valuetype0 == napi_null) {
    becs_set_context(NULL);
  } else {
    napi_throw_type_error(env, NULL,
                          "Wrong argument type, expected string or null");
  }

  return NULL;
}

static napi_value UpdateUser(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 3");
    return NULL;
  }

  char *id = read_string_value(env, args[0], true);
  char *email = read_string_value(env, args[1], true);
  char *name = read_string_value(env, args[2], true);
  becs_set_user(id, email, name);

  free(id);
  free(email);
  free(name);

  return NULL;
}

static napi_value AddMetadata(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 3");
    return NULL;
  }

  char *tab = read_string_value(env, args[0], false);
  char *key = read_string_value(env, args[1], false);
  char *value = read_string_value(env, json_stringify(env, args[2]), true);
  if (tab && key && value) {
    becs_set_metadata(tab, key, value);
  }

  free(tab);
  free(key);
  free(value);

  return NULL;
}

static napi_value SetApp(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 1");
    return NULL;
  }

  set_object_or_null(env, args[0], becs_set_app);

  return NULL;
}

static napi_value SetDevice(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 1");
    return NULL;
  }

  set_object_or_null(env, args[0], becs_set_device);

  return NULL;
}

static napi_value SetSession(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 1");
    return NULL;
  }

  set_object_or_null(env, args[0], becs_set_session);

  return NULL;
}

static napi_value ClearMetadata(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 2");
    return NULL;
  }

  char *tab = read_json_string_value(env, args[0], false);
  char *key = read_json_string_value(env, args[1], false);

  if (tab && key) {
    becs_set_metadata(tab, key, NULL);
  }

  free(tab);
  free(key);

  return NULL;
}

static napi_value LeaveBreadcrumb(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, NULL, "Wrong number of arguments, expected 1");
    return NULL;
  }

  char *breadcrumb = read_json_string_value(env, args[0], false);
  if (breadcrumb) {
    becs_add_breadcrumb(breadcrumb);
    free(breadcrumb);
  }

  return NULL;
}

static napi_value PersistState(napi_env env, napi_callback_info info) {
  becs_persist_to_disk();
  return NULL;
}

#define DECLARE_NAPI_METHOD(name, func)                                        \
  (napi_property_descriptor) { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc = DECLARE_NAPI_METHOD("install", Install);
  napi_status status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("addMetadata", AddMetadata);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("clearMetadata", ClearMetadata);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("leaveBreadcrumb", LeaveBreadcrumb);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("updateContext", UpdateContext);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("updateUser", UpdateUser);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("persistState", PersistState);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("setApp", SetApp);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("setDevice", SetDevice);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("setSession", SetSession);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  desc = DECLARE_NAPI_METHOD("uninstall", Uninstall);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
