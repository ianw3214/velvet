#include "errorHandler.h"

#include <iostream>

ErrorHandler::ErrorHandler() {}

bool ErrorHandler::hasError() const {
    return mErrorFound;
}

void ErrorHandler::logError(const std::string& message) {
    mErrorFound = true;
    std::cout << "ERROR: " << message << std::endl;
}