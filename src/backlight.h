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

typedef struct version_t
{
    int major;
    int minor;
} version_t;

typedef struct backlight_t
{
    xcb_connection_t* connection;
    xcb_atom_t atom_new;
    xcb_atom_t atom_old;
    version_t version;
} backlight_t;

gboolean
backlight_new(backlight_t*);

void
backlight_clear(backlight_t*);

static gboolean
_randr_version(xcb_connection_t* connection, version_t* version, xcb_generic_error_t** error);

static gboolean
_atom(xcb_connection_t* connection,
      char atom_name[],
      xcb_atom_t* atom,
      xcb_generic_error_t** error);

static char*
generic_error_str(int error);

#endif
