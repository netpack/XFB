// Quick test to verify ErrorHandler compilation
#include "src/services/ErrorHandler.h"

int main() {
    // Just test that ErrorHandler can be instantiated
    // This will verify the mutex include is working
    auto& handler = ErrorHandler::instance();
    (void)handler; // Suppress unused variable warning
    return 0;
}