/* SLiM - Simple Login Manager
   Copyright (C) 2009 Eygene Ryabinkin <rea@codelabs.ru>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/
#ifndef _UTIL_H__
#define _UTIL_H__

#include <string>

namespace Util {
	bool add_mcookie(const std::string &mcookie, const char *display,
		const std::string &xauth_cmd, const std::string &authfile);

	void run_command(const std::string &cmd);

	long makeseed();
	void srandom(long seed);
	long random();

	std::string shell_escape(const std::string &s);

	/* Keyboard text is kept as UTF-8. set_utf8_ctype() selects a UTF-8
	 * LC_CTYPE (the environment's, else C.UTF-8) so Xlib input methods
	 * produce UTF-8; it returns false if none is available. */
	bool set_utf8_ctype();
	/* True if s is valid UTF-8 made only of printable characters (no C0,
	 * DEL or C1 controls); chars receives the number of characters. */
	bool utf8_printable(const std::string &s, size_t &chars);
	/* Remove the last UTF-8 character of s; false if s is empty. */
	bool utf8_erase_last(std::string &s);
	std::string latin1_to_utf8(const std::string &s);
}

#endif /* _UTIL_H__ */
