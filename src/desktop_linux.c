//
// Created by ncbradley on 27/02/19.
// Much of the code was taken from https://github.com/dancor/wmctrl/blob/0181e8e4a7ed98255f6ef930875e56560f19aa95/main.c
//

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <stdint.h>
#include <netinet/in.h>
#include <glib.h>

#include "common.h"

#define MAX_PROPERTY_VALUE_LEN 4096
#define SELECT_WINDOW_MAGIC ":SELECT:"
#define ACTIVE_WINDOW_MAGIC ":ACTIVE:"

#define p_verbose(...) if (True) { \
    fprintf(stderr, __VA_ARGS__); \
}

static int client_msg(Display *disp, Window win, char *msg,
                      unsigned long data0, unsigned long data1,
                      unsigned long data2, unsigned long data3,
                      unsigned long data4) {
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;

    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}

static gchar *get_property (Display *disp, Window win, Atom xa_prop_type, gchar *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    gchar *ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     */
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
                           xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }

    if (xa_ret_type != xa_prop_type) {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / (32 / sizeof(long))) * ret_nitems;
    ret = g_malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}

static Window *get_client_list (Display *disp, unsigned long *size) {
    Window *client_list;

    if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                    XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
        if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp),
                        XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
            fputs("Cannot get client list properties. \n"
                  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
                  "\n", stderr);
            return NULL;
        }
    }

    return client_list;
}

static gchar *get_window_title (Display *disp, Window win) {
    gchar *title_utf8;
    gchar *wm_name;
    gchar *net_wm_name;

    wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = get_property(disp, win,
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name) {
        title_utf8 = g_strdup(net_wm_name);
    }
    else {
        if (wm_name) {
            title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
        }
        else {
            title_utf8 = NULL;
        }
    }

    g_free(wm_name);
    g_free(net_wm_name);

    return title_utf8;
}

static int _activate_window (Display *disp, Window win, gboolean switch_desktop) {
    unsigned long *desktop;

    /* desktop ID */
    if ((desktop = (unsigned long *)get_property(disp, win,
                                                 XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        if ((desktop = (unsigned long *)get_property(disp, win,
                                                     XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
            p_verbose("Cannot find desktop ID of the window.\n");
        }
    }

    if (switch_desktop && desktop) {
        if (client_msg(disp, DefaultRootWindow(disp),
                       "_NET_CURRENT_DESKTOP",
                       *desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
            p_verbose("Cannot switch desktop.\n");
        }
        g_free(desktop);
    }

    client_msg(disp, win, "_NET_ACTIVE_WINDOW",
               0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    return EXIT_SUCCESS;
}


static Display *open_display(void) {
    return XOpenDisplay(NULL);
}

static void close_display(Display *disp) {
    XCloseDisplay(disp);
}

/**
 *
 */
static GdkPixbuf *get_pixbuf_from_icon(GIcon *icon, GtkIconSize size)
{
    GdkPixbuf *result=NULL;
    GtkIconTheme *theme;
    GtkIconInfo *info;
    gint width;

    if (!icon)
        return NULL;

    theme=gtk_icon_theme_get_default();
    gtk_icon_size_lookup(size,&width,NULL);
    info=gtk_icon_theme_lookup_by_gicon(theme,
                                        icon,
                                        size,
                                        GTK_ICON_LOOKUP_USE_BUILTIN);

    if (!info)
        return NULL;

    result=gtk_icon_info_load_icon(info,NULL);

    g_object_unref(info);

    return result;
}

/**
 *
 */
static GdkPixbuf *get_pixbuf_from_file(GFile *file, GtkIconSize size)
{
    GIcon *icon;
    GFileInfo *info;

    GdkPixbuf *result=NULL;

    info=g_file_query_info(file,
                           G_FILE_ATTRIBUTE_STANDARD_ICON,
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);

    if (!info)
        return NULL;

    icon=g_file_info_get_icon(info);

    if (icon!=NULL) {
        result=get_pixbuf_from_icon(icon,size);
    }

    g_object_unref(info);

    return result;
}

bool activate_window(char *name) {
    Display *disp = open_display();
    gboolean switch_desktop = True;
    Window win = strtoul(name, NULL, 0);

    int result = _activate_window(disp, win, switch_desktop);

    close_display(disp);

    return result == 0;
}

struct DesktopApplication** list_applications(size_t *count) {
    gtk_init(0, NULL);

    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();

    GList *app_list = g_app_info_get_all();
    GList *l;
    gint list_count = g_list_length(app_list);
    int pos = -1;

    struct DesktopApplication **apps = malloc(list_count * sizeof(struct DesktopApplication *));

    for (l = app_list; l != NULL; l = l->next) {
        pos++;
        GAppInfo *app = l->data;
        const char *disp_name = g_app_info_get_name(app);  // can also use display_name but this seems more expected
        const char *commandline = g_app_info_get_commandline(app);
        GIcon *icon = g_app_info_get_icon(app);
        gchar *dataStr = NULL;

        if (icon != NULL) {
            GdkPixbuf *pixBuf = get_pixbuf_from_icon(icon, ICON_SIZE);
            if (pixBuf != NULL) {
                gchar *buffer = NULL;
                gsize buf_size = 0;
                GError *error = NULL;

                if (gdk_pixbuf_save_to_buffer(pixBuf, &buffer, &buf_size, "png", &error, NULL)) {
                    gchar *base64 = g_base64_encode(buffer, buf_size);
                    dataStr = g_strconcat("data:image/png;base64,", base64, NULL);

                    g_free(base64);
                } else {
                    g_warning ("Failed load icon: %s", error->message);
                    g_error_free(error);
                }
            } else {
                g_warning("Failed to get pixel buffer from icon.");
            }
        } else {
            g_warning("Failed to get default app icon.");
        }

        if (dataStr != NULL) {
            const size_t icon_size = strlen(dataStr);
            apps[pos] = malloc(sizeof(struct DesktopApplication) + sizeof(char) * icon_size);
            strcpy(apps[pos]->icon, dataStr);
            apps[pos]->icon_size = icon_size;
        } else {
            apps[pos] = malloc(sizeof(struct DesktopApplication));
            apps[pos]->icon_size = 0;
        }

        size_t size;
        size = g_strlcpy(apps[pos]->name, disp_name, APP_NAME_LEN);
        apps[pos]->name_size = MIN(size, APP_NAME_LEN); // We might have truncated the string
        size = g_strlcpy(apps[pos]->path, commandline, APP_PATH_LEN);
        apps[pos]->path_size = MIN(size, APP_PATH_LEN);
    }

    *count = (size_t)list_count;
    return apps;
}

struct DesktopWindow* list_windows(size_t *count) {
    unsigned long client_list_size;
    Display *disp = open_display();
    Window *client_list = get_client_list(disp, &client_list_size);
    unsigned long list_count = client_list_size / sizeof(Window);

    struct DesktopWindow *window_list = malloc(list_count * sizeof(struct DesktopWindow));

    for (uint i = 0; i < list_count; i++) {
        char *title = get_window_title(disp, client_list[i]);
        char str[10];
        sprintf(str, "0x0%x", client_list[i]);

        strcpy(window_list[i].id, str);
        strcpy(window_list[i].title, title);
        window_list[i].id_size = 10;
        window_list[i].title_size = (size_t)g_utf8_strlen(title, WINDOW_TITLE_LEN);
    }

    close_display(disp);
    *count = list_count;
    return window_list;
}
