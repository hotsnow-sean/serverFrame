#include "../sylar/config.hpp"
#include "../sylar/log.h"
#include <iostream>

#if 0
sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_int_valuex_config =
    sylar::Config::Lookup("system.port", (float)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int_vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int_list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int_set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int_uset");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 1}}, "system str_int_map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config =
    sylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 1}}, "system str_int_umap");

void test_config() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto &v = g_var->getValue(); \
        for (auto &i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " json: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto &v = g_var->getValue(); \
        for (auto &i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": { " << i.first << " - " << i.second << " }"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " json: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    sylar::Config::LoadFromJson("D:\\MyCode\\CPPSTUDY\\Hserver\\bin\\conf\\test.json");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);

#undef XX_M
#undef XX
}
#endif

class Person {
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;
    friend std::ostream &operator<<(std::ostream &os, const Person &p);

    bool operator==(const Person &p) const {
        return p.m_name == m_name && p.m_age == m_age && p.m_sex == m_sex;
    }
};
std::ostream &operator<<(std::ostream &os, const Person &p) {
    os << "[Person name=" << p.m_name << " age=" << p.m_age << " sex=" << p.m_sex << "]";
    return os;
}

void to_json(nlohmann::json &j, const Person &p) {
    j = nlohmann::json{{"name", p.m_name}, {"age", p.m_age}, {"sex", p.m_sex}};
}

void from_json(const nlohmann::json &j, Person &p) {
    j.at("name").get_to(p.m_name);
    j.at("age").get_to(p.m_age);
    j.at("sex").get_to(p.m_sex);
}

sylar::ConfigVar<Person>::ptr g_person =
    sylar::Config::Lookup("class.person", Person(), "system person");

sylar::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    sylar::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map =
    sylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system person");

void test_class() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for (auto &i : m) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix ": " << i.first << " - " << i.second; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix ": size=" << m.size(); \
    }

    g_person->addListerner(10, [](const Person &o, const Person &n) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << o << " new_value=" << n;
    });

    XX_PM(g_person_map, before);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    sylar::Config::LoadFromJson("D:\\MyCode\\CPPSTUDY\\Hserver\\bin\\conf\\test.json");

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->toString();
    XX_PM(g_person_map, after);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_vec_map->toString();

#undef XX_PM
}

int main() {
    // test_config();
    test_class();

    return 0;
}