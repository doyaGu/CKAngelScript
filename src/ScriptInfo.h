#ifndef SCRIPTINFO_H
#define SCRIPTINFO_H

#include <angelscript.h>

#include <string>

bool ExportScriptApiIfRequested(asIScriptEngine *engine, std::string &error);

#endif //SCRIPTINFO_H
