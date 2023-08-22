#pragma once

#include <vector>
#include <string>

class ErrorHandler;

class Composer {
    std::vector<std::string> mInputFiles;
    std::vector<std::string> mObjectFiles;
    ErrorHandler& mErrorHandler;
public:
    Composer(ErrorHandler& errorHandler);

    void addInputFile(const std::string& fileName);

    void buildAllFiles();
    void generateExecutable();
};