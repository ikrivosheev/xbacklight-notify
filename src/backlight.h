#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <glib.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>

typedef struct range_t
{
    long min;
    long max;
} range_t;

typedef struct backlight_t
{
    xcb_connection_t* connection;
    xcb_atom_t atom;
    xcb_screen_t* screen;
    xcb_randr_output_t output;
    xcb_window_t window;
    range_t range;
} backlight_t;

typedef void (*update_callback_t)(backlight_t* backlight, gint value, void* userdata);

gboolean
backlight_new(backlight_t*);

void
backlight_clear(backlight_t*);

static gboolean
_randr_value(backlight_t* backlight, long* value, xcb_generic_error_t** error);

void
backlight_loop_run(backlight_t* backlight, update_callback_t callback, void* userdata);

static gboolean
_randr_extension(xcb_connection_t* connection, xcb_generic_error_t** error);

static gboolean
_randr_version(xcb_connection_t* connection, xcb_generic_error_t** error);

static gboolean
_randr_outpout(backlight_t* backlight, xcb_window_t window, xcb_generic_error_t** error);

static gboolean
_randr_atom(xcb_connection_t* connection,
            char atom_name[],
            xcb_atom_t* atom,
            xcb_generic_error_t** error);

static gboolean
_randr_range(backlight_t* backlight,
             xcb_atom_t atom_new,
             xcb_atom_t atom_old,
             xcb_generic_error_t** error);

static const char*
generic_error_str(int error_code);

#endif
