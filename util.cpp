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

#include <sstream>
#include <vector>

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
 * Forks and executes cmd, waiting for it to complete.
 * Avoids system()'s signal-masking side-effects (system() blocks SIGCHLD
 * and ignores SIGINT/SIGQUIT in the parent).
 *
 * If cmd contains no shell metacharacters it is split on whitespace and
 * executed directly via execvp, keeping the shell entirely out of the
 * picture and eliminating any injection surface for those commands.
 * If shell metacharacters are present (e.g. quotes, pipes, redirections)
 * it falls back to /bin/sh -c so that quoting and pipelines work correctly,
 * as required by console_cmd's default value.
 */
void Util::run_command(const std::string &cmd) {
	static const char shell_metachars[] = "|&;<>()$`\\\"'{[*?~!#\n";
	bool needs_shell = (cmd.find_first_of(shell_metachars) != std::string::npos);

	pid_t pid = fork();
	if (pid == 0) {
		if (needs_shell) {
			execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)NULL);
		} else {
			/* tokenize on whitespace and exec directly, no shell involved */
			std::vector<std::string> args;
			std::istringstream iss(cmd);
			std::string token;
			while (iss >> token)
				args.push_back(token);
			if (args.empty())
				_exit(0);
			std::vector<char *> argv;
			for (std::vector<std::string>::iterator it = args.begin();
			     it != args.end(); ++it)
				argv.push_back(&(*it)[0]);
			argv.push_back(NULL);
			execvp(argv[0], argv.data());
		}
		_exit(127);
	} else if (pid > 0) {
		int status;
		waitpid(pid, &status, 0);
	}
}

