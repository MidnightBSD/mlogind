#ifndef _LOG_H_
#define _LOG_H_

#ifdef USE_CONSOLEKIT
#include "Ck.h" 
#endif
#ifdef USE_PAM
#include "PAM.h"
#endif
#include "const.h"
#include <fstream>

class LogUnit {
	std::ofstream logFile;
public:
	bool openLog(const char * filename);
	void closeLog();

	~LogUnit() { closeLog(); }

	template<typename Type>
	LogUnit & operator<<(const Type & text) {
		if (logFile.is_open()) {
			logFile << text;
			logFile.flush();
		}
		return *this;
	}

	LogUnit & operator<<(std::ostream & (*fp)(std::ostream&)) {
		if (logFile.is_open()) {
			logFile << fp;
			logFile.flush();
		}
		return *this;
	}

	LogUnit & operator<<(std::ios_base & (*fp)(std::ios_base&)) {
		if (logFile.is_open()) {
			logFile << fp;
			logFile.flush();
		}
		return *this;
	}
};

/* Function-local static avoids cross-TU static initialization order issues. */
LogUnit& getLogStream();
#define logStream getLogStream()

#endif /* _LOG_H_ */
