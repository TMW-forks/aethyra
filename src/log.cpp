/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "log.h"
#ifdef WIN32
#include <windows.h>
#endif

FILE* logfile;


Logger::Logger(std::string logFilename)
{
    logFile.open(logFilename.c_str(), std::ios_base::trunc);
    if ( !logFile.is_open() )
    {
        std::cout << "Warning: error while opening log file." << std::endl;
    }

}

Logger::~Logger()
{
    logFile.close();
}

void Logger::log(std::string log_text)
{
    if ( logFile.is_open() )
    {
        // Time container
        time_t t;
        // Get the current system time
        time(&t);
        
        // Print the log entry
        logFile << "[";
        logFile << ((((t / 60) / 60) % 24 < 10) ? "0" : "");
        logFile << (int)(((t / 60) / 60) % 24);
        logFile << ":";
        logFile << (((t / 60) % 60 < 10) ? "0" : "");
        logFile << (int)((t / 60) % 60);
        logFile << ":";
        logFile << ((t % 60 < 10) ? "0" : "");
        logFile << (int)(t % 60);
        logFile << "] ";
        
        // Print the log entry
        logFile << log_text << std::endl;
    }
}

void Logger::log(const char *log_text, ...)
{
    if ( logFile.is_open() )
    {
        char* buf = new char[1024];
        va_list ap;
        time_t t;

        // Use a temporary buffer to fill in the variables
        va_start(ap, log_text);
        vsprintf(buf, log_text, ap);
        va_end(ap);

        // Get the current system time
        time(&t);

        // Print the log entry
        logFile << "[";
        logFile << ((((t / 60) / 60) % 24 < 10) ? "0" : "");
        logFile << (int)(((t / 60) / 60) % 24);
        logFile << ":";
        logFile << (((t / 60) % 60 < 10) ? "0" : "");
        logFile << (int)((t / 60) % 60);
        logFile << ":";
        logFile << ((t % 60 < 10) ? "0" : "");
        logFile << (int)(t % 60);
        logFile << "] ";
        
        logFile << buf << std::endl;

        // Delete temporary buffer
        delete[] buf;
    }
}

void Logger::error(const std::string &error_text)
{
    log(error_text);

#ifdef WIN32
    MessageBox(NULL, error_text.c_str(), "Error", MB_ICONERROR | MB_OK);
#else
    log("Error :");
    log(error_text);
#endif
    exit(1);
}

void init_log()
{
    logfile = fopen("tmw.log", "w");
    if (!logfile) {
        printf("Warning: error while opening log file.\n");
    }
}

void log(const char *log_text, ...)
{
    if (logfile)
    {
        char* buf = new char[1024];
        va_list ap;
        time_t t;

        // Use a temporary buffer to fill in the variables
        va_start(ap, log_text);
        vsprintf(buf, log_text, ap);
        va_end(ap);

        // Get the current system time
        time(&t);

        // Print the log entry
        fprintf(logfile,
                "[%s%d:%s%d:%s%d] %s\n",
                (((t / 60) / 60) % 24 < 10) ? "0" : "",
                (int)(((t / 60) / 60) % 24),
                ((t / 60) % 60 < 10) ? "0" : "",
                (int)((t / 60) % 60),
                (t % 60 < 10) ? "0" : "",
                (int)(t % 60),
                buf
               );

        // Flush the log file
        fflush(logfile);

        // Delete temporary buffer
        delete[] buf;
    }
}

void error(const std::string &error_text)
{
    log(error_text.c_str());

#ifdef WIN32
    MessageBox(NULL, error_text.c_str(), "Error", MB_ICONERROR | MB_OK);
#else
    fprintf(stderr, "Error: %s\n", error_text.c_str());
#endif
    exit(1);
}
