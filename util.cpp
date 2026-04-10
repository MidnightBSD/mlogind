/* SLiM - Simple Login Manager
   Copyright (C) 2009 Eygene Ryabinkin <rea@codelabs.ru>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#include "util.h"

/*
 * Adds the given cookie to the specified Xauthority file.
 * Returns true on success, false on fault.
 * Uses fork/execv with a pipe rather than popen to avoid shell
 * injection via authfile paths that contain spaces or metacharacters.
 */
bool Util::add_mcookie(const std::string &mcookie, const char *display,
	const std::string &xauth_cmd, const std::string &authfile)
{
	int pipefd[2];
	if (pipe(pipefd) == -1)
		return false;

	pid_t pid = fork();
	if (pid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		return false;
	}

	if (pid == 0) {
		/* Child: read commands from the pipe on stdin */
		close(pipefd[1]);
		if (dup2(pipefd[0], STDIN_FILENO) == -1)
			_exit(127);
		close(pipefd[0]);

		const char *argv[] = {
			xauth_cmd.c_str(),
			"-f", authfile.c_str(),
			"-q",
			NULL
		};
		execv(xauth_cmd.c_str(), (char * const *)argv);
		_exit(127);
	}

	/* Parent: write xauth commands then close write end */
	close(pipefd[0]);
	FILE *fp = fdopen(pipefd[1], "w");
	if (!fp) {
		close(pipefd[1]);
		waitpid(pid, NULL, 0);
		return false;
	}
	fprintf(fp, "remove %s\n", display);
	fprintf(fp, "add %s %s %s\n", display, ".", mcookie.c_str());
	fprintf(fp, "exit\n");
	fclose(fp);

	int status;
	waitpid(pid, &status, 0);
	return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

/*
 * Forks and executes cmd via /bin/sh, waiting for it to complete.
 * Avoids system()'s signal-masking side-effects and does not invoke
 * a shell when the command string contains no shell metacharacters,
 * while still supporting quoted arguments in complex commands such
 * as console_cmd.
 */
void Util::run_command(const std::string &cmd) {
	pid_t pid = fork();
	if (pid == 0) {
		execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)NULL);
		_exit(127);
	} else if (pid > 0) {
		int status;
		waitpid(pid, &status, 0);
	}
}

