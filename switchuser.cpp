/* SLiM - Simple Login Manager
   Copyright (C) 1997, 1998 Per Liden
   Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
   Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <sys/types.h>
#include <login_cap.h>
#include <unistd.h>
#include <grp.h>
#include <errno.h>
#include <cstring>

#include <cstdio>
#include "switchuser.h"
#include "util.h"

using namespace std;

SwitchUser::SwitchUser(struct passwd *pw, Cfg *c, const string& display,
					   char** _env)
	: cfg(c),
	  Pw(pw),
	  displayName(display),
	  env(_env)
{
}

SwitchUser::~SwitchUser() {
	/* Never called */
}

void SwitchUser::Login(const char* cmd, const char* mcookie) {
	SetUserId();
	SetClientAuth(mcookie);
	Execute(cmd);
}

void SwitchUser::SetUserId() {
	if (Pw == 0) {
		logStream << APPNAME << ": SwitchUser::SetUserId: passwd struct is NULL" << endl;
		exit(ERR_EXIT);
	}

	if (setsid() == -1) {
		logStream << APPNAME << ": setsid failed: " << strerror(errno) << endl;
		exit(ERR_EXIT);
	}

	/* Try BSD-style user context setting (preferred on FreeBSD/MidnightBSD) */
	login_cap_t *lc = login_getpwclass(Pw);
	if (lc != NULL) {
		if (setusercontext(lc, Pw, Pw->pw_uid, LOGIN_SETALL) == 0) {
			login_close(lc);
			return;
		}
		logStream << APPNAME << ": setusercontext failed: " << strerror(errno) << endl;
		login_close(lc);
	} else {
		logStream << APPNAME << ": login_getpwclass failed for user " << Pw->pw_name << endl;
	}

	/* Fallback to manual privilege drop if setusercontext fails or is unavailable */
	logStream << APPNAME << ": attempting fallback privilege drop for user " << Pw->pw_name << endl;

	if (setlogin(Pw->pw_name) != 0) {
		logStream << APPNAME << ": setlogin failed: " << strerror(errno) << endl;
		exit(ERR_EXIT);
	}

	if (initgroups(Pw->pw_name, Pw->pw_gid) != 0) {
		logStream << APPNAME << ": initgroups failed: " << strerror(errno) << endl;
		exit(ERR_EXIT);
	}

	if (setgid(Pw->pw_gid) != 0) {
		logStream << APPNAME << ": setgid failed: " << strerror(errno) << endl;
		exit(ERR_EXIT);
	}

	if (setuid(Pw->pw_uid) != 0) {
		logStream << APPNAME << ": setuid failed: " << strerror(errno) << endl;
		exit(ERR_EXIT);
	}
}

void SwitchUser::Execute(const char* cmd) {
	chdir(Pw->pw_dir);
	execle(Pw->pw_shell, Pw->pw_shell, "-c", cmd, NULL, env);
	logStream << APPNAME << ": could not execute login command" << endl;
}

void SwitchUser::SetClientAuth(const char* mcookie) {
	string home = string(Pw->pw_dir);
	string authfile = home + "/.Xauthority";
	remove(authfile.c_str());
	Util::add_mcookie(mcookie, displayName.c_str(), cfg->getOption("xauth_path"),
	  authfile);
}
