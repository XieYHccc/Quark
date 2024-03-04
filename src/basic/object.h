#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <functional>
#include <memory>

class Component;
class Object {

public:
    Object() = delete;
    Object(std::string name);
    ~Object();

public:
    std::string get_name() { return name_; }
    void set_name(std::string name) { name_ = name; }
    Component* add_component(std::string component_type_name);
    Component* get_component(std::string component_type_name);
    
private:
    std::string name_;
    std::unordered_map<std::string,std::vector<Component*>> components_;

};