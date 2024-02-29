#pragma  once

class Object;
class Component {
public:
    Component() : object_(nullptr) {};
    virtual ~Component() {};

public:
    Object* get_object() { return object_; }
    void set_object (Object* object){ object_ = object; }

    virtual void awake() {};
    virtual void update() {};
private:
    Object* object_;
};