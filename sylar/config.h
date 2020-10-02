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
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
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
        GetDatas().emplace(name, v);
        return v;
    }
    template <typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) return nullptr;
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string &name);
    static void LoadFromFile(const std::string &file);
    static void LoadFromJson(const nlohmann::json& j);

private:
    static ConfigVarMap &GetDatas();
};

} // namespace sylar

#endif // __SYLAR_CONFIG_H__