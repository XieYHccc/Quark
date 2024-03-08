#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <typeindex>

class Component;
class Object {

public:
    Object() = delete;
    Object(const std::string& name) : name_(name) {}

public:
    std::string get_name() { return name_; }
    void set_name(std::string name) { name_ = name; }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
    std::shared_ptr<T> add_component() 
    {
        auto cmpt = std::make_shared<T>(this);
        cmpt->awake();

        auto base = std::static_pointer_cast<Component>(cmpt);
        auto pair = std::make_pair(std::type_index(typeid(T)), base);
        components_.insert(pair);
        return cmpt;
    }

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
    std::shared_ptr<T> get_component() 
    {
        auto it = components_.find(std::type_index(typeid(T)));
        if (it == components_.end()) return nullptr;
        return std::dynamic_pointer_cast<T>(it->second);
    }
    
private:
    std::string name_;
    std::multimap<std::type_index, std::shared_ptr<Component>> components_;

};