#pragma  once

class Object;
class Component {
public:
    Component(Object* object) : object_(object) {};
    virtual ~Component() {};

public:
    Object* get_object() { return object_; }

    virtual void awake() {};
    virtual void update() {};
private:
    Object* object_;
};