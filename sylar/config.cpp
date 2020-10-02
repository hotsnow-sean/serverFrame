#include "./config.h"

namespace sylar {

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string &prefix, const nlohmann::json &node,
                          std::list<std::pair<std::string, const nlohmann::json>> &output) {
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.emplace_back(prefix, node);
    if (node.is_object()) {
        for (auto &it : node.items()) {
            ListAllMember(prefix.empty() ? it.key() : prefix + "." + it.key(), it.value(), output);
        }
    }
}

void Config::LoadFromFile(const std::string &file) {
    std::ifstream jsf(file);
    nlohmann::json root;
    jsf >> root;
    LoadFromJson(root);
}

void Config::LoadFromJson(const nlohmann::json &j) {
    std::list<std::pair<std::string, const nlohmann::json>> all_nodes;
    ListAllMember("", j, all_nodes);
    for (auto &i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) continue;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
        if (var) {
            var->fromJson(i.second);
        }
    }
}

Config::ConfigVarMap &Config::GetDatas() {
    static Config::ConfigVarMap s_datas;
    return s_datas;
}

} // namespace sylar