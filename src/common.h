//
// Created by ncbradley on 16/03/19.
//

#include <stdbool.h>
#include <stddef.h>

#ifndef DESKTOP_NATIVE_COMMON_H
#define DESKTOP_NATIVE_COMMON_H

#define ICON_SIZE 128
#define WINDOW_ID_LEN 255
#define WINDOW_TITLE_LEN 255
#define APP_NAME_LEN 255
#define APP_PATH_LEN 1020

typedef struct DesktopWindow {
    size_t id_size;
    size_t title_size;
    char id[WINDOW_ID_LEN];
    char title[WINDOW_TITLE_LEN];
} window;

typedef struct DesktopApplication {
    size_t name_size;
    size_t path_size;
    size_t icon_size;
    char name[APP_NAME_LEN];
    char path[APP_PATH_LEN];
    char icon[];
} app;

/*
 *
 */
bool activate_window(char *name);

/*
 *
 */
struct DesktopWindow* list_windows(size_t *count);

/*
 * Callers must free each element as well as the entire array.
 */
struct DesktopApplication** list_applications(size_t *count);

#endif //DESKTOP_NATIVE_COMMON_H
