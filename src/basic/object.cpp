#include "object.h"

#include <cassert>

#include <iostream>
#include <rttr/registration>

#include "component.h"


Object::Object(std::string name) {
    set_name(name);
}

Object::~Object() {

}

Component* Object::add_component(std::string component_type_name) {
    rttr::type t = rttr::type::get_by_name(component_type_name);
    assert(t.is_valid());
    rttr::variant var = t.create();
    Component* component=var.get_value<Component*>();
    component->set_object(this);

    if (components_.find(component_type_name) == components_.end()){
        std::vector<Component*> component_vec;
        component_vec.push_back(component);
        components_[component_type_name] = component_vec;
    }
    else {
        components_[component_type_name].push_back(component);
    }

    component->awake();

    return component;

}


Component* Object::get_component(std::string component_type_name) {
    if (components_.find(component_type_name) == components_.end()) {
        return nullptr;
    }
    if (components_[component_type_name].size() == 0) {
        return nullptr;
    }
    return components_[component_type_name][0];
}
