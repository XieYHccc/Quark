#pragma once

#include "../Object/Components/Component.h"

class Object;
class Collider : public Component {
public:
	Collider(Object* object) : Component(object) {}
	virtual ~Collider() {}


};
