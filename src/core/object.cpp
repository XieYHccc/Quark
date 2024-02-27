#include "object.h"

#include <cassert>

#include <iostream>
#include <rttr/registration>

#include "component.h"

Object::Object(std::string name) {
    set_name(name);
    object_list.push_back(this);
}

Object::~Object() {

}

Component* Object::add_component(std::string component_type_name) {
    rttr::type t = rttr::type::get_by_name(component_type_name);
    assert(t.is_valid());
    rttr::variant var = t.create();
    Component* component=var.get_value<Component*>();
    component->set_object(this);

    if (component_type_instance_map_.find(component_type_name) == component_type_instance_map_.end()){
        std::vector<Component*> component_vec;
        component_vec.push_back(component);
        component_type_instance_map_[component_type_name] = component_vec;
    }
    else {
        component_type_instance_map_[component_type_name].push_back(component);
    }

    component->awake();
    if (component == nullptr) {
        std::cout << "f" << std::endl;
    }
    return component;

}

std::vector<Component*>& Object::get_components(std::string component_type_name) {
    return component_type_instance_map_[component_type_name];
}

Component* Object::get_component(std::string component_type_name) {
    if (component_type_instance_map_.find(component_type_name)==component_type_instance_map_.end()) {
        return nullptr;
    }
    if (component_type_instance_map_[component_type_name].size() == 0) {
        return nullptr;
    }
    return component_type_instance_map_[component_type_name][0];
}

void Object::foreach_component(std::function<void(Component *)> func) {
    for (auto v : component_type_instance_map_){
        for (auto iter : v.second){
            Component* component=iter;
            func(component);
        }
    }
}