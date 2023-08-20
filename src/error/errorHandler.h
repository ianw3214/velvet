#pragma once

#include <string>

class ErrorHandler {
    bool mErrorFound = false;
public:
    ErrorHandler();

    bool hasError() const;
    void logError(const std::string& message);
};