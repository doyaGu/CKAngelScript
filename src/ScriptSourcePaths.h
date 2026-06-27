#ifndef CK_SCRIPT_SOURCE_PATHS_H
#define CK_SCRIPT_SOURCE_PATHS_H

#include <string>

namespace ScriptSourcePaths {

std::string NormalizeSectionName(std::string path);
std::string ResolveSnapshotIncludeName(const char *include, const char *from);
std::string ResolveFileIncludeName(const char *include, const char *from);

} // namespace ScriptSourcePaths

#endif // CK_SCRIPT_SOURCE_PATHS_H
