#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>

static GtkWidget               *window;
static WebKitWebView           *web_view;
static WebKitWebSettings       *webkitsettings;
static WebKitWebWindowFeatures *webkitwindowfeatures;
static SoupSession             *session;
static SoupCookieJar           *jar;

void load_uri(gchar *uri) {
    gchar *u;
    u = g_strrstr(uri, "://") ? g_strdup(uri) : g_strdup_printf("http://%s", uri);
//  webkit_web_view_set_zoom_level(web_view, 1);
    webkit_web_view_load_uri(web_view, u);
    g_free(u);
}

void setup_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 480);
    gtk_widget_show(window);
    webkitsettings       = webkit_web_settings_new();
    webkitwindowfeatures = webkit_web_window_features_new();
    session              = webkit_get_default_session();
    jar                  = soup_cookie_jar_text_new(
                             g_build_filename(g_get_home_dir(),
		               ".sb_cookies",
		               NULL),
                             FALSE);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(jar));
    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(web_view));
    gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);
    setup_window();
    if (argc == 2) 
    	load_uri(argv[1]);
    gtk_main();
    // Gdk::PROXIMITY_IN
    return 0;
}
