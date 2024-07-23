#include "pch.h"
#include "Scene/Node.h"
#include "Scene/Scene.h"

namespace scene {
Node::Node(Scene* scene, Entity* entity, size_t poolOffset)
    :scene_(scene), entity_(entity), parent_(nullptr), poolOffset_(poolOffset)
{
    
}

void Node::RemoveChild(Node* child)
{
    CORE_DEBUG_ASSERT(child != nullptr)

    if (child->parent_ != this) {
        CORE_LOGW("Node::RemoveChild()::You can't remove a child which is not your child.")
        return;     
    }

    for (auto& c : children_) {
        if (c == child) {
            std::swap(c, children_.back());
            children_.pop_back();
            child->parent_ = nullptr;
            return;
        }
    }

    CORE_LOGW("Node::RemoveChild()::You can't remove a child which is not your child.")
    
}

void Node::ClearChildren()
{
    for (auto* c: children_)
        RemoveChild(c);
}

void Node::AddChild(Node *child)
{
    CORE_DEBUG_ASSERT(child != nullptr)
    CORE_DEBUG_ASSERT(child != this)

    if (child->parent_ != nullptr) {
        CORE_LOGW("Node::AddChild()::You can't add a child which has a parent.")
        return;
    }

    for (const auto* c : children_) {
        if (c == child) {
            CORE_LOGW("Node::AddChild()::You can't add a child which has existed.")
            return;       
        }
    }

    child->parent_ = this;
    children_.push_back(child);
}
}