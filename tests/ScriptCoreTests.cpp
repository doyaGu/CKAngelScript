#include "ScriptCoreSelfTests.h"

#include <exception>
#include <iostream>
#include <string>

namespace {

struct CoreTestCase {
    const char *Name;
    bool (*Run)(std::string &error);
};

} // namespace

int main() {
    const CoreTestCase tests[] = {
        {"Initial lifecycle", RunScriptInitialLifecycleSelfTest},
        {"Module generation", RunScriptModuleGenerationSelfTest},
        {"Runtime metadata", ScriptRuntimeMetadata::RunScriptRuntimeMetadataSelfTest},
        {"Runtime dependencies", ScriptRuntimeDependencyResolver::RunScriptRuntimeDependencySelfTest},
    };

    for (const CoreTestCase &test : tests) {
        std::string error;
        try {
            if (!test.Run(error)) {
                std::cerr << test.Name << " failed";
                if (!error.empty()) {
                    std::cerr << ": " << error;
                }
                std::cerr << '\n';
                return 1;
            }
        } catch (const std::exception &exception) {
            std::cerr << test.Name << " threw an exception: " << exception.what() << '\n';
            return 1;
        } catch (...) {
            std::cerr << test.Name << " threw an unknown exception.\n";
            return 1;
        }
    }

    std::cout << "All CKAngelScript core tests passed.\n";
    return 0;
}
