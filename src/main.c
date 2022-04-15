#include "backlight.h"
#include <glib.h>
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <locale.h>
#include <math.h>

#define G_LOG_DOMAIN ((gchar*)0)
#define PROGRAM_NAME "xbacklight-notify"
#define DEFAULT_DEBUG FALSE

typedef struct _Context
{
    NotifyNotification* notification;
    gint last_val;
} Context;

static struct config
{
    gboolean debug;
    gint timeout;
} config = {
    DEFAULT_DEBUG,
    NOTIFY_EXPIRES_DEFAULT,
};

static GOptionEntry option_entries[] = {
    { "debug", 'd', 0, G_OPTION_ARG_NONE, &config.debug, "Enable/disable debug information", NULL },
    { "timeout",
      't',
      0,
      G_OPTION_ARG_INT,
      &config.timeout,
      "Notification timeout in seconds (-1 - default notification timeout, 0 - notification never "
      "expires)",
      NULL },
    { NULL }
};

void
context_init(Context* context)
{
    context->notification = notify_notification_new(NULL, NULL, NULL);
    context->last_val = -1;
}

void
context_free(Context* context)
{}

static void
notify_message(NotifyNotification* notification,
               const gchar* summary,
               const gchar* body,
               NotifyUrgency urgency,
               gint timeout,
               gint brightness)
{
    notify_notification_update(notification, summary, body, NULL);
    notify_notification_set_timeout(notification, timeout);
    notify_notification_set_urgency(notification, urgency);
    if (brightness >= 0) {
        GVariant* g_var = g_variant_new_int32(brightness);
        notify_notification_set_hint(notification, "value", g_var);
    } else {
        notify_notification_set_hint(notification, "value", NULL);
    }

    notify_notification_show(notification, NULL);
}

static gboolean
options_init(int argc, char* argv[])
{
    GError* error = NULL;
    GOptionContext* option_context;

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, PROGRAM_NAME);

    if (g_option_context_parse(option_context, &argc, &argv, &error) == FALSE) {
        g_error("Cannot parse command line arguments: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    g_option_context_free(option_context);
    if (config.debug == TRUE)
        g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, g_log_default_handler, NULL);
    else
        g_log_set_handler(NULL,
                          G_LOG_LEVEL_INFO | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL |
                            G_LOG_LEVEL_ERROR,
                          g_log_default_handler,
                          NULL);

    if (config.timeout > 0) {
        config.timeout *= 1000;
    }

    return TRUE;
}

int
main(int argc, char* argv[])
{
    Context context;
    backlight_t backlight;

    setlocale(LC_ALL, "");

    g_return_val_if_fail(options_init(argc, argv), 1);
    g_info("Options have been initialized");

    context_init(&context);
    g_return_val_if_fail(notify_init(PROGRAM_NAME), 1);
    g_info("Notify has been initialized");

    g_return_val_if_fail(backlight_new(&backlight), 1);
    g_info("X11 randr has been initialized");

    backlight_loop_run(&backlight);

    backlight_clear(&backlight);
    notify_uninit();
    context_free(&context);

    return 0;
}
