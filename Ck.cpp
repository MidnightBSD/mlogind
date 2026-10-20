/* SLiM - Simple Login Manager
   Copyright (C) 2011 David Hauweele

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <cstdio>
#include <iostream>

#include <ck-connector.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdarg.h>

#include "Ck.h"

namespace Ck {
  Exception::Exception(const std::string &func,
					   const std::string &errstr):
	func(func),
	errstr(errstr)
  {}

  dbus_bool_t Session::ck_connector_open_graphic_session(const std::string &display,
														 uid_t uid)
  {
	dbus_bool_t local		= true;
	const char *session_type = "x11";
	const char *x11_display  = display.c_str();
	const char *x11_device   = get_x11_device(display);
	const char *remote_host  = "";
	const char *display_dev  = "";

	return ck_connector_open_session_with_parameters(ckc, &error,
													 "unix-user", &uid,
													 "session-type", &session_type,
													 "x11-display", &x11_display,
													 "x11-display-device", &x11_device,
													 "display-device", &display_dev,
													 "remote-host-name", &remote_host,
													 "is-local", &local,
													 NULL);
  }

  const char * Session::get_x11_device(const std::string &display)
  {
	static char device[64];

	Display *xdisplay = XOpenDisplay(display.c_str());

	if(!xdisplay)
	  throw Exception(__func__, "cannot open display");

	/* Close this private connection on every return and throw path */
	struct DisplayCloser {
	  Display *dpy;
	  ~DisplayCloser() { XCloseDisplay(dpy); }
	} closer = { xdisplay };

	Window root;
	Atom xfree86_vt_atom;
	Atom return_type_atom;
	int return_format;
	unsigned long return_count;
	unsigned long bytes_left;
	unsigned char *return_value = NULL;
	long vt;
	const char *problem = NULL;

	xfree86_vt_atom = XInternAtom(xdisplay, "XFree86_VT", true);

	if(xfree86_vt_atom == None)
	  throw Exception(__func__, "cannot get XFree86_VT");

	root = DefaultRootWindow(xdisplay);

	if(XGetWindowProperty(xdisplay, root, xfree86_vt_atom,
						  0L, 1L, false, XA_INTEGER,
						  &return_type_atom, &return_format,
						  &return_count, &bytes_left,
						  &return_value) != Success)
	  throw Exception(__func__, "cannot get root window property");

	if(return_type_atom != XA_INTEGER)
	  problem = "bad atom type";
	else if(return_format != 32)
	  problem = "invalid return format";
	else if(return_count != 1)
	  problem = "invalid count";
	else if(bytes_left != 0)
	  problem = "invalid bytes left";
	else if(!return_value)
	  problem = "null return value from XGetWindowProperty";

	if(problem) {
	  if(return_value)
		XFree(return_value);
	  throw Exception(__func__, problem);
	}

	vt = *((long *)return_value);
	XFree(return_value);

	snprintf(device, sizeof(device), "/dev/ttyv%ld", vt - 1);

	return device;
  }

  /* Drop the connector, which also closes its private system bus
   * connection.  The session is considered open only while ckc is set. */
  void Session::release()
  {
	if(ckc) {
	  ck_connector_unref(ckc);
	  ckc = nullptr;
	}
  }

  void Session::open_session(const std::string &display, uid_t uid)
  {
	release();
	ckc = ck_connector_new();

	if(!ckc)
	  throw Exception(__func__, "error setting up connection to ConsoleKit");

	dbus_bool_t opened;
	try {
	  opened = ck_connector_open_graphic_session(display, uid);
	}
	catch(Exception &) {
	  release();
	  throw;
	}

	if(!opened) {
	  std::string errstr = dbus_error_is_set(&error) ? error.message :
		"cannot open ConsoleKit session: OOM, DBus system bus "
		" not available or insufficient privileges";
	  dbus_error_free(&error);
	  release();
	  throw Exception(__func__, errstr);
	}
  }

  bool Session::is_open() const
  {
	return ckc != nullptr;
  }

  const char * Session::get_xdg_session_cookie()
  {
	return ckc ? ck_connector_get_cookie(ckc) : NULL;
  }

  /* Return a copy of the XDG runtime directory ConsoleKit manages for the
   * session, or an empty string if it does not provide one. */
  std::string Session::get_runtime_dir()
  {
	if(!ckc)
	  return "";

	DBusError local_error;
	dbus_error_init(&local_error);
	const char *runtime_dir = ck_connector_get_runtime_dir(ckc, &local_error);
	std::string result = runtime_dir ? runtime_dir : "";
	std::string errstr = dbus_error_is_set(&local_error) ? local_error.message : "";
	dbus_error_free(&local_error);

	if(!runtime_dir && !errstr.empty())
	  throw Exception(__func__, errstr);
	return result;
  }

  void Session::close_session()
  {
	if(!ckc)
	  return;

	if(!ck_connector_close_session(ckc, &error)) {
	  std::string errstr = dbus_error_is_set(&error) ? error.message :
		"cannot close ConsoleKit session: OOM, DBus system bus "
		" not available or insufficient privileges";
	  dbus_error_free(&error);
	  release();
	  throw Exception(__func__, errstr);
	}
	release();
  }

  Session::Session()
    : ckc(nullptr)
  {
	dbus_error_init(&error);
  }

  Session::~Session()
  {
	release();
	dbus_error_free(&error);
  }
}

std::ostream& operator<<( std::ostream& os, const Ck::Exception& e)
{
  os << e.func << ": " << e.errstr;
  return os;
}
