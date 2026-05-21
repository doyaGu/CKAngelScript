#include "ScriptRuntimeDependency.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

#include <fmt/format.h>

namespace ScriptRuntimeDependencyInternal {

struct NodeOrder {
    int Order = 1000;
    std::string Id;
    int Index = 0;
};

struct ReadyCompare {
    bool operator()(const NodeOrder &lhs, const NodeOrder &rhs) const {
        if (lhs.Order != rhs.Order) {
            return lhs.Order > rhs.Order;
        }
        if (lhs.Id != rhs.Id) {
            return lhs.Id > rhs.Id;
        }
        return lhs.Index > rhs.Index;
    }
};

std::string JoinIds(const std::vector<std::string> &ids) {
    std::string result;
    for (const std::string &id : ids) {
        if (!result.empty()) {
            result += " -> ";
        }
        result += id;
    }
    return result;
}

bool HasId(const std::vector<std::string> &ids, const std::string &id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void AddEdge(int from,
             int to,
             std::vector<std::vector<int>> &edges,
             std::vector<int> &indegree) {
    if (from == to) {
        return;
    }
    std::vector<int> &targets = edges[from];
    if (std::find(targets.begin(), targets.end(), to) == targets.end()) {
        targets.push_back(to);
        ++indegree[to];
    }
}

} // namespace ScriptRuntimeDependencyInternal

namespace ScriptRuntimeDependencyResolver {

ScriptRuntimeLoadPlan Resolve(const std::vector<ScriptRuntimeManifest> &scripts) {
    ScriptRuntimeLoadPlan plan;
    std::string diagnostics;
    std::vector<ScriptRuntimeManifest> enabled;
    enabled.reserve(scripts.size());
    for (const ScriptRuntimeManifest &script : scripts) {
        if (script.Enabled) {
            enabled.push_back(script);
        }
    }

    std::map<std::string, int> indexById;
    for (int i = 0; i < static_cast<int>(enabled.size()); ++i) {
        indexById[enabled[i].Id] = i;
    }

    std::vector<bool> skipped(enabled.size(), false);
    for (int i = 0; i < static_cast<int>(enabled.size()); ++i) {
        const ScriptRuntimeManifest &script = enabled[i];
        for (const ScriptRuntimeDependency &dependency : script.RequiredDependencies) {
            const auto depIt = indexById.find(dependency.Id);
            if (depIt == indexById.end()) {
                diagnostics += fmt::format("Skipping runtime script '{}': required dependency '{}' is missing.\n",
                                           script.Id,
                                           ScriptRuntimeMetadata::VersionRequirementText(dependency));
                skipped[i] = true;
                continue;
            }
            const ScriptRuntimeManifest &actual = enabled[depIt->second];
            if (!ScriptRuntimeMetadata::SatisfiesVersion(actual.Version, dependency)) {
                diagnostics += fmt::format("Skipping runtime script '{}': dependency '{}' has version '{}', expected '{}'.\n",
                                           script.Id,
                                           dependency.Id,
                                           actual.VersionText,
                                           ScriptRuntimeMetadata::VersionRequirementText(dependency));
                skipped[i] = true;
            }
        }
        for (const ScriptRuntimeDependency &dependency : script.OptionalDependencies) {
            const auto depIt = indexById.find(dependency.Id);
            if (depIt == indexById.end()) {
                diagnostics += fmt::format("Runtime script '{}': optional dependency '{}' is not present.\n",
                                           script.Id,
                                           ScriptRuntimeMetadata::VersionRequirementText(dependency));
                continue;
            }
            const ScriptRuntimeManifest &actual = enabled[depIt->second];
            if (!ScriptRuntimeMetadata::SatisfiesVersion(actual.Version, dependency)) {
                diagnostics += fmt::format("Runtime script '{}': optional dependency '{}' has version '{}', expected '{}'.\n",
                                           script.Id,
                                           dependency.Id,
                                           actual.VersionText,
                                           ScriptRuntimeMetadata::VersionRequirementText(dependency));
            }
        }
        for (const std::string &id : script.Before) {
            if (!id.empty() && indexById.find(id) == indexById.end()) {
                diagnostics += fmt::format("Runtime script '{}': before target '{}' is not present.\n", script.Id, id);
            }
        }
        for (const std::string &id : script.After) {
            if (!id.empty() && indexById.find(id) == indexById.end()) {
                diagnostics += fmt::format("Runtime script '{}': after target '{}' is not present.\n", script.Id, id);
            }
        }
    }

    std::vector<int> activeToOriginal;
    std::vector<int> originalToActive(enabled.size(), -1);
    for (int i = 0; i < static_cast<int>(enabled.size()); ++i) {
        if (!skipped[i]) {
            originalToActive[i] = static_cast<int>(activeToOriginal.size());
            activeToOriginal.push_back(i);
        }
    }

    std::vector<std::vector<int>> edges(activeToOriginal.size());
    std::vector<int> indegree(activeToOriginal.size(), 0);
    auto addById = [&](int fromOriginal, const std::string &toId) {
        const auto toIt = indexById.find(toId);
        if (toIt == indexById.end()) {
            return;
        }
        const int from = originalToActive[fromOriginal];
        const int to = originalToActive[toIt->second];
        if (from >= 0 && to >= 0) {
            ScriptRuntimeDependencyInternal::AddEdge(from, to, edges, indegree);
        }
    };

    for (int activeIndex = 0; activeIndex < static_cast<int>(activeToOriginal.size()); ++activeIndex) {
        const int originalIndex = activeToOriginal[activeIndex];
        const ScriptRuntimeManifest &script = enabled[originalIndex];
        for (const ScriptRuntimeDependency &dependency : script.RequiredDependencies) {
            const auto depIt = indexById.find(dependency.Id);
            if (depIt != indexById.end()) {
                const int depActive = originalToActive[depIt->second];
                if (depActive >= 0) {
                    ScriptRuntimeDependencyInternal::AddEdge(depActive, activeIndex, edges, indegree);
                }
            }
        }
        for (const ScriptRuntimeDependency &dependency : script.OptionalDependencies) {
            const auto depIt = indexById.find(dependency.Id);
            if (depIt != indexById.end() &&
                ScriptRuntimeMetadata::SatisfiesVersion(enabled[depIt->second].Version, dependency)) {
                const int depActive = originalToActive[depIt->second];
                if (depActive >= 0) {
                    ScriptRuntimeDependencyInternal::AddEdge(depActive, activeIndex, edges, indegree);
                }
            }
        }
        for (const std::string &before : script.Before) {
            addById(originalIndex, before);
        }
        for (const std::string &after : script.After) {
            const auto afterIt = indexById.find(after);
            if (afterIt != indexById.end()) {
                const int afterActive = originalToActive[afterIt->second];
                if (afterActive >= 0) {
                    ScriptRuntimeDependencyInternal::AddEdge(afterActive, activeIndex, edges, indegree);
                }
            }
        }
    }

    std::priority_queue<ScriptRuntimeDependencyInternal::NodeOrder,
                        std::vector<ScriptRuntimeDependencyInternal::NodeOrder>,
                        ScriptRuntimeDependencyInternal::ReadyCompare> ready;
    for (int i = 0; i < static_cast<int>(activeToOriginal.size()); ++i) {
        if (indegree[i] == 0) {
            const ScriptRuntimeManifest &script = enabled[activeToOriginal[i]];
            ready.push({script.Order, script.Id, i});
        }
    }

    std::vector<int> sortedActive;
    while (!ready.empty()) {
        const int index = ready.top().Index;
        ready.pop();
        sortedActive.push_back(index);
        for (const int target : edges[index]) {
            --indegree[target];
            if (indegree[target] == 0) {
                const ScriptRuntimeManifest &script = enabled[activeToOriginal[target]];
                ready.push({script.Order, script.Id, target});
            }
        }
    }

    if (sortedActive.size() != activeToOriginal.size()) {
        std::vector<std::string> cycleIds;
        for (int i = 0; i < static_cast<int>(activeToOriginal.size()); ++i) {
            if (indegree[i] > 0) {
                cycleIds.push_back(enabled[activeToOriginal[i]].Id);
            }
        }
        std::sort(cycleIds.begin(), cycleIds.end());
        diagnostics += fmt::format("Skipping runtime scripts with dependency cycle: {}.\n",
                                   ScriptRuntimeDependencyInternal::JoinIds(cycleIds));
        std::set<int> cycleActive;
        for (int i = 0; i < static_cast<int>(activeToOriginal.size()); ++i) {
            if (indegree[i] > 0) {
                cycleActive.insert(i);
            }
        }
        for (const int activeIndex : sortedActive) {
            if (cycleActive.find(activeIndex) == cycleActive.end()) {
                plan.Scripts.push_back(enabled[activeToOriginal[activeIndex]]);
            }
        }
    } else {
        for (const int activeIndex : sortedActive) {
            plan.Scripts.push_back(enabled[activeToOriginal[activeIndex]]);
        }
    }

    plan.Diagnostics = std::move(diagnostics);
    return plan;
}

bool HasDependencyFailure(const ScriptRuntimeManifest &script,
                          const std::vector<std::string> &failedIds,
                          std::string &error) {
    for (const ScriptRuntimeDependency &dependency : script.RequiredDependencies) {
        if (ScriptRuntimeDependencyInternal::HasId(failedIds, dependency.Id)) {
            error = fmt::format("Runtime script '{}' skipped because required dependency '{}' failed to load.",
                                script.Id,
                                dependency.Id);
            return true;
        }
    }
    return false;
}

#if CKAS_BUILD_SELF_TESTS
bool RunScriptRuntimeDependencySelfTest(std::string &error) {
    ScriptRuntimeManifest core;
    core.Id = "core";
    core.Name = "core";
    core.VersionText = "1.2.0";
    core.Version = ScriptRuntimeMetadata::ParseVersion(core.VersionText);
    core.Order = 10;

    ScriptRuntimeManifest smoke;
    smoke.Id = "smoke";
    smoke.Name = "smoke";
    smoke.VersionText = "1.0.0";
    smoke.Version = ScriptRuntimeMetadata::ParseVersion(smoke.VersionText);
    smoke.Order = 0;
    ScriptRuntimeDependency required;
    std::string parseError;
    if (!ScriptRuntimeMetadata::ParseDependencySpec("core>=1.0.0", required, parseError)) {
        error = parseError;
        return false;
    }
    smoke.RequiredDependencies.push_back(required);

    ScriptRuntimeLoadPlan plan = Resolve({smoke, core});
    if (plan.Scripts.size() != 2 || plan.Scripts[0].Id != "core" || plan.Scripts[1].Id != "smoke") {
        error = "Runtime dependency resolver did not sort required dependencies before dependents.";
        return false;
    }

    ScriptRuntimeManifest missing = smoke;
    missing.Id = "missing";
    missing.RequiredDependencies[0].Id = "not.present";
    plan = Resolve({missing});
    if (!plan.Scripts.empty() || plan.Diagnostics.find("required dependency") == std::string::npos) {
        error = "Runtime dependency resolver did not reject missing required dependencies.";
        return false;
    }

    ScriptRuntimeManifest a;
    a.Id = "a";
    a.Name = "a";
    a.Version = ScriptRuntimeMetadata::ParseVersion("1.0.0");
    ScriptRuntimeManifest b = a;
    b.Id = "b";
    ScriptRuntimeDependency depA;
    ScriptRuntimeDependency depB;
    if (!ScriptRuntimeMetadata::ParseDependencySpec("b", depA, parseError) ||
        !ScriptRuntimeMetadata::ParseDependencySpec("a", depB, parseError)) {
        error = parseError;
        return false;
    }
    a.RequiredDependencies.push_back(depA);
    b.RequiredDependencies.push_back(depB);
    plan = Resolve({a, b});
    if (!plan.Scripts.empty() || plan.Diagnostics.find("cycle") == std::string::npos) {
        error = "Runtime dependency resolver did not reject dependency cycles.";
        return false;
    }
    return true;
}
#endif

} // namespace ScriptRuntimeDependencyResolver
