#include "composer/composer.h"
#include "error/errorHandler.h"

int main(int argc, char* argv[]) {
    ErrorHandler handler;
    Composer composer(handler);
    for (int index = 1; index < argc; ++index) {
        composer.addInputFile(argv[index]);
    }

    composer.buildAllFiles();
    if (handler.hasError()) {
        return 1;
    }
    composer.generateExecutable();

    return 0;
};