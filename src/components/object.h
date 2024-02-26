#pragma once

#include <string>
#include <list>

class Component;
class Object {
public:
    Object(std::string name);
    ~Object();

public:
    std::string& name() { return name_; }
    void set_name(std::string name) { name_ = name; }
    Component* add_component(std::string component_type_name);
    Component* get_component(std::string component_type_name);
    std::vector<Component*>& get_components(std::string component_type_name);
    void foreach_component(std::function<void(Component* component)> func);

private:
    std::string name_;
    std::unordered_map<std::string,std::vector<Component*>> component_type_instance_map_;

private:
    // Store all objects
    static std::list<Object*> object_list_;
};