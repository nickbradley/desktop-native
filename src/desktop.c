//
// Created by ncbradley on 15/03/19.
//

#include <node_api.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "napi-macros.h"

#ifdef DEBUG
    #define CHECK_NAPI_RESULT(condition) (assert((condition) == napi_ok))
#else
    // Don't assert anything
    #define CHECK_NAPI_RESULT(condition) condition
#endif

NAPI_METHOD(activateWindow) {
    NAPI_ARGV(1)
    NAPI_ARGV_UTF8_MALLOC(name, 0)

    bool result = activate_window(name);

    free(name);

    NAPI_RETURN_UINT32(result);
}

NAPI_METHOD(listWindows) {
    napi_value windows;
    CHECK_NAPI_RESULT(napi_create_array(env, &windows));

    size_t count = 0;
    struct DesktopWindow *window_list = list_windows(&count);

    for (int i = 0; i < count; i++) {
        napi_value window;
        CHECK_NAPI_RESULT(napi_create_object(env, &window));
        napi_value identifier;
        CHECK_NAPI_RESULT(napi_create_string_utf8(env, window_list[i].id, window_list[i].id_size, &identifier));
        napi_value title;
        CHECK_NAPI_RESULT(napi_create_string_utf8(env, window_list[i].title, window_list[i].title_size, &title));
        CHECK_NAPI_RESULT(napi_set_named_property(env, window, "identifier", identifier));
        CHECK_NAPI_RESULT(napi_set_named_property(env, window, "title", title));
        CHECK_NAPI_RESULT(napi_set_element(env, windows, i, window));
    }

    free(window_list);

    return windows;
}

NAPI_METHOD(listApplications) {
    napi_value applications;
    CHECK_NAPI_RESULT(napi_create_array(env, &applications));

    size_t count = 0;
    struct DesktopApplication **apps = list_applications(&count);

    for (int i = 0; i < count; i++) {
        napi_value application;
        CHECK_NAPI_RESULT(napi_create_object(env, &application));
        napi_value name;
        CHECK_NAPI_RESULT(napi_create_string_utf8(env, apps[i]->name, apps[i]->name_size, &name));
        napi_value path;
        CHECK_NAPI_RESULT(napi_create_string_utf8(env, apps[i]->path, apps[i]->path_size, &path));
        napi_value icon;
        CHECK_NAPI_RESULT(napi_create_string_utf8(env, apps[i]->icon, apps[i]->icon_size, &icon));
        CHECK_NAPI_RESULT(napi_set_named_property(env, application, "name", name));
        CHECK_NAPI_RESULT(napi_set_named_property(env, application, "path", path));
        CHECK_NAPI_RESULT(napi_set_named_property(env, application, "icon", icon));
        CHECK_NAPI_RESULT(napi_set_element(env, applications, i, application));

        free(apps[i]);
    }

    free(apps);

    return applications;
}

NAPI_INIT() {
    NAPI_EXPORT_FUNCTION(activateWindow)
    NAPI_EXPORT_FUNCTION(listWindows)
    NAPI_EXPORT_FUNCTION(listApplications)
}

