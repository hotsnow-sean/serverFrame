#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include "./log.h"
#include "./nlohmann/json.hpp"
#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace sylar {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name), m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string &val) = 0;
    virtual bool fromJson(const nlohmann::json &node) = 0;
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};

template <typename T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
        : ConfigVarBase(name, description), m_val(default_value) {
    }

    std::string toString() override {
        try {
            return nlohmann::json(m_val).dump();
        } catch (const std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                              << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }
    bool fromString(const std::string &val) override {
        auto j = nlohmann::json::parse(val);
        return fromJson(j);
    }
    bool fromJson(const nlohmann::json &node) override {
        try {
            setValue(node.get<T>());
        } catch (const std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception" << e.what()
                                              << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getValue() const { return m_val; }
    void setValue(const T &v) {
        if (m_val == v) return;
        for (auto &i : m_cbs) {
            i.second(m_val, v);
        }
        m_val = v;
    }
    std::string getTypeName() const override { return typeid(T).name(); }

    void addListerner(uint64_t key, on_change_cb cb) {
        m_cbs[key] = cb;
    }
    void delListerner(uint64_t key) {
        m_cbs.erase(key);
    }
    on_change_cb getListerner(uint64_t key) {
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }
    void clearListerner() { m_cbs.clear(); }

private:
    T m_val;
    // 变更回调函数组，uint64_t key 要求唯一 一般用 hash
    std::map<uint64_t, on_change_cb> m_cbs;
};

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template <typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value,
                                             const std::string &description = "") {
        auto it = s_datas.find(name);
        if (it != s_datas.end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            } else {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exsits but type not "
                                                  << typeid(T).name() << " real type=" << it->second->getTypeName()
                                                  << " " << it->second->toString();
                return nullptr;
            }
        }
        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;
    }
    template <typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        auto it = s_datas.find(name);
        if (it == s_datas.end()) return nullptr;
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string &name);
    static void LoadFromJson(const std::string &file);

private:
    static ConfigVarMap s_datas;
};

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    auto it = s_datas.find(name);
    return it == s_datas.end() ? nullptr : it->second;
}

static void ListAllMember(const std::string &prefix, const nlohmann::json &node,
                          std::list<std::pair<std::string, const nlohmann::json>> &output) {
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.is_object()) {
        for (auto &it : node.items()) {
            ListAllMember(prefix.empty() ? it.key() : prefix + "." + it.key(), it.value(), output);
        }
    }
}

void Config::LoadFromJson(const std::string &file) {
    std::ifstream jsf(file);
    nlohmann::json root;
    jsf >> root;
    std::list<std::pair<std::string, const nlohmann::json>> all_nodes;
    ListAllMember("", root, all_nodes);
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

Config::ConfigVarMap Config::s_datas;

} // namespace sylar

#endif // __SYLAR_CONFIG_H__