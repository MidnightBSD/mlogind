#include "log.h"
#include <iostream>

bool
LogUnit::openLog(const char * filename)
{
	if (logFile.is_open()) {
		std::cerr << APPNAME
			<< ": opening a new Log file, while another is already open"
			<< std::endl;
		logFile.close();
	}
	logFile.open(filename, std::ios_base::app);

	return !(logFile.fail());
}

void
LogUnit::closeLog()
{
	if (logFile.is_open())
		logFile.close();
}

LogUnit& getLogStream() {
	static LogUnit instance;
	return instance;
}
