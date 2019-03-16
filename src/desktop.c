//
// Created by ncbradley on 15/03/19.
//

#include <node_api.h>
#include <assert.h>
#include <stdio.h>

#ifdef __linux__
#include "desktop_linux.h"
#endif


#define CHECK_NAPI_RESULT(condition) (assert((condition) == napi_ok))

napi_value ActivateWindow(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_value *argv = NULL;
    CHECK_NAPI_RESULT(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    char *name = argv[0];
    printf("ActivateWindow called with: %s\n", name);
    bool activationResult = activate_window(name);

    napi_value result = NULL;
    CHECK_NAPI_RESULT(napi_get_boolean(env, activationResult, &result));

    return result;
}


napi_value ListWindows(napi_env env, napi_callback_info info) {
    return NULL;
}

napi_value ListApplications(napi_env env, napi_callback_info info) {
    return NULL;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_value activateWindow;
    napi_value listWindows;
    napi_value listApplications;

    CHECK_NAPI_RESULT(napi_create_function(env, "activateWindow", -1, ActivateWindow, NULL, &activateWindow));
    CHECK_NAPI_RESULT(napi_create_function(env, "listWindows", -1, ListWindows, NULL, &listWindows));
    CHECK_NAPI_RESULT(napi_create_function(env, NULL, -1, ListApplications, NULL, &listApplications));

    CHECK_NAPI_RESULT(napi_set_named_property(env, exports, "activateWindow", activateWindow));
    CHECK_NAPI_RESULT(napi_set_named_property(env, exports, "listWindows", listWindows));
    CHECK_NAPI_RESULT(napi_set_named_property(env, exports, "listApplications", listApplications));

    return exports;
}

NAPI_MODULE(PROJECT_NAME, Init)