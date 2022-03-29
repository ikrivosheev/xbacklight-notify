#include "backlight.h"

gboolean
backlight_new(backlight_t* backlight)
{
    int screen_number;
    xcb_generic_error_t* error;

    backlight->connection = xcb_connect(NULL, &screen_number);

    if (_randr_extension(backlight->connection, &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("RANDR Query Version returned error %s", generic_error_str(ec));
        return FALSE;
    }

    if (_randr_version(backlight->connection, &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("RANDR Query Version returned error %s", generic_error_str(ec));
        return FALSE;
    }

    if (_atom(backlight->connection, "Backlight", &backlight->atom_new, &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("New Atom returned error: %s", generic_error_str(ec));
        return FALSE;
    }

    if (_atom(backlight->connection, "BACKLIGHT", &backlight->atom_old, &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("Old Atom returned error: %s", generic_error_str(ec));
        return FALSE;
    }

    xcb_screen_t* screen = NULL;
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(backlight->connection));
    for (; iter.rem; --screen_number, xcb_screen_next(&iter)) {
        if (screen_number == 0) {
            screen = iter.data;
            break;
        }
    }

    if (screen == NULL) {
        g_error("Cannot find screen");
        return FALSE;
    }

    xcb_randr_output_t output;
    if (_randr_outpout(backlight->connection, screen->root, &output, &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("RANDR primary output error: %s", generic_error_str(ec));
        return FALSE;
    }

    if (_randr_range(backlight->connection,
                     output,
                     backlight->atom_new,
                     backlight->atom_old,
                     &backlight->range,
                     &error) == FALSE) {
        int ec = error ? error->error_code : -1;
        g_error("RANDR range error: %s", generic_error_str(ec));
        return FALSE;
    }

    xcb_window_t wid = xcb_generate_id(backlight->connection);
    xcb_void_cookie_t cookie = xcb_create_window_checked(backlight->connection,
                                                         XCB_COPY_FROM_PARENT,
                                                         wid,
                                                         screen->root,
                                                         -1,
                                                         -1,
                                                         1,
                                                         1,
                                                         0,
                                                         XCB_COPY_FROM_PARENT,
                                                         XCB_COPY_FROM_PARENT,
                                                         0,
                                                         NULL);
    if (xcb_flush(backlight->connection) < 0) {
        g_error("Error on flush");
        return FALSE;
    }

    error = xcb_request_check(backlight->connection, cookie);
    if (error != NULL) {
        int ec = error ? error->error_code : -1;
        g_error("Cannot create Window with error: %s", generic_error_str(ec));
        return FALSE;
    }

    cookie = xcb_randr_select_input_checked(
      backlight->connection, wid, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    if (xcb_flush(backlight->connection) < 0) {
        g_error("Error on flush");
        return FALSE;
    }

    error = xcb_request_check(backlight->connection, cookie);
    if (error != NULL) {
        int ec = error ? error->error_code : -1;
        g_error("Cannot select input with error: %s", generic_error_str(ec));
        return FALSE;
    }

    cookie = xcb_randr_select_input_checked(
      backlight->connection, wid, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);
    if (xcb_flush(backlight->connection) < 0) {
        g_error("Error on flush");
        return FALSE;
    }

    error = xcb_request_check(backlight->connection, cookie);
    if (error != NULL) {
        int ec = error ? error->error_code : -1;
        g_error("Cannot create Window with error: %s", generic_error_str(ec));
        return FALSE;
    }

    return TRUE;
}

void
backlight_clear(backlight_t* backlight)
{
    xcb_disconnect(backlight->connection);
}

static gboolean
_randr_extension(xcb_connection_t* connection, xcb_generic_error_t** error)
{
    xcb_query_extension_cookie_t cookie;
    xcb_query_extension_reply_t* reply;

    cookie = xcb_query_extension(connection, sizeof("RANDR"), "RANDR");
    reply = xcb_query_extension_reply(connection, cookie, error);

    if (*error != NULL || reply == NULL) {
        return FALSE;
    }

    free(reply);
    return TRUE;
}

static gboolean
_randr_version(xcb_connection_t* connection, xcb_generic_error_t** error)
{
    xcb_randr_query_version_cookie_t cookie;
    xcb_randr_query_version_reply_t* reply;

    cookie = xcb_randr_query_version(connection, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
    reply = xcb_randr_query_version_reply(connection, cookie, error);
    if (*error != NULL || reply == NULL) {
        return FALSE;
    }
    free(reply);
    return TRUE;
}

static gboolean
_randr_outpout(xcb_connection_t* connection,
               xcb_window_t window,
               xcb_randr_output_t* output,
               xcb_generic_error_t** error)
{
    xcb_randr_get_output_primary_cookie_t cookie;
    xcb_randr_get_output_primary_reply_t* reply;

    cookie = xcb_randr_get_output_primary(connection, window);
    reply = xcb_randr_get_output_primary_reply(connection, cookie, error);

    if (*error != NULL || reply == NULL) {
        return FALSE;
    }

    *output = reply->output;
    free(reply);
    return TRUE;
}

static gboolean
_atom(xcb_connection_t* connection, char atom_name[], xcb_atom_t* atom, xcb_generic_error_t** error)
{
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t* reply;

    cookie = xcb_intern_atom(connection, 1, strlen(atom_name), atom_name);
    reply = xcb_intern_atom_reply(connection, cookie, error);
    if (*error != NULL || reply == NULL) {
        return FALSE;
    }
    *atom = reply->atom;
    free(reply);
    return TRUE;
}

static gboolean
_randr_range(xcb_connection_t* connection,
             xcb_randr_output_t output,
             xcb_atom_t backlight_new,
             xcb_atom_t backlight_old,
             range_t* range,
             xcb_generic_error_t** error)
{
    gboolean result = TRUE;
    xcb_randr_query_output_property_cookie_t prop_cookie;
    xcb_randr_query_output_property_reply_t* prop_reply;

    if (backlight_new != XCB_ATOM_NONE) {
        prop_cookie = xcb_randr_query_output_property(connection, output, backlight_new);
        prop_reply = xcb_randr_query_output_property_reply(connection, prop_cookie, error);
        if (*error != NULL || prop_reply == NULL) {
            free(prop_reply);
            if (backlight_old != XCB_ATOM_NONE) {
                prop_cookie = xcb_randr_query_output_property(connection, output, backlight_old);
                prop_reply = xcb_randr_query_output_property_reply(connection, prop_cookie, error);
                if (*error != NULL || prop_reply == NULL) {
                    result = FALSE;
                }
            }
        }
    }

    if (prop_reply == NULL ||
        !(prop_reply->range &&
          xcb_randr_query_output_property_valid_values_length(prop_reply) == 2)) {
        result = FALSE;
    } else {
        int32_t* values = xcb_randr_query_output_property_valid_values(prop_reply);
        range->min = values[0];
        range->max = values[1];
    }

    free(prop_reply);
    return result;
}

static gboolean
_value(xcb_connection_t* connection,
       xcb_randr_output_t output,
       xcb_atom_t backlight_new,
       xcb_atom_t backlight_old,
       long* value,
       xcb_generic_error_t** error)
{
    gboolean result = TRUE;
    xcb_randr_get_output_property_reply_t* prop_reply = NULL;
    xcb_randr_get_output_property_cookie_t prop_cookie;

    if (backlight_new != XCB_ATOM_NONE) {
        prop_cookie = xcb_randr_get_output_property(
          connection, output, backlight_new, XCB_ATOM_NONE, 0, 4, 0, 0);
        prop_reply = xcb_randr_get_output_property_reply(connection, prop_cookie, error);
        if (error != NULL || prop_reply == NULL) {
            free(prop_reply);
            if (backlight_old != XCB_ATOM_NONE) {
                prop_cookie = xcb_randr_get_output_property(
                  connection, output, backlight_old, XCB_ATOM_NONE, 0, 4, 0, 0);
                prop_reply = xcb_randr_get_output_property_reply(connection, prop_cookie, error);
                if (error != NULL || prop_reply == NULL) {
                    result = FALSE;
                }
            }
        }
    }

    if (prop_reply == NULL || prop_reply->type != XCB_ATOM_INTEGER || prop_reply->num_items != 1 ||
        prop_reply->format != 32) {
        result = FALSE;
    } else {
        *value = *((int32_t*)xcb_randr_get_output_property_data(prop_reply));
    }

    free(prop_reply);
    return result;
}

static char*
generic_error_str(int error_code)
{
    switch (error_code) {
        case XCB_CONN_ERROR:
            return "Socket, pipe or stream error";
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
            return "Unsupported extension";
        case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
            return "Not enough memory";
        case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
            return "Request length exceeded";
        case XCB_CONN_CLOSED_PARSE_ERR:
            return "Can't parse display string";
        case XCB_CONN_CLOSED_INVALID_SCREEN:
            return "Invalid screen";
        case XCB_CONN_CLOSED_FDPASSING_FAILED:
            return "Failed to pass FD";
        default:
            return "Unknown error";
    }
}
