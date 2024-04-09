//
// Created by xiongqimin on 2023/12/14.
//

#include "logger.h"
#include <iostream>

namespace xmppclient {
/* Class variables. */

Logger::LogHandlerInterface *Logger::handler{nullptr};
char Logger::buffer[Logger::bufferSize];
Logger::LLevel Logger::logLevel = Logger::LLevel::LOG_NONE;

/* Class methods. */

void Logger::SetLogLevel(Logger::LLevel level) {
    Logger::logLevel = level;
}

void Logger::SetHandler(LogHandlerInterface *handler) {
    Logger::handler = handler;
}

void Logger::SetDefaultHandler() {
    Logger::handler = new Logger::DefaultLogHandler();
}

/* DefaultLogHandler */

void Logger::DefaultLogHandler::OnLog(Logger::LLevel /*level*/, char *payload, size_t /*len*/) {
    std::cout << payload << std::endl;
}
} // gmeetclient