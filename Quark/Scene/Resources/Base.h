#pragma once
#include "Quark/Core/Base.h"

namespace quark {
class Resource {
public:
    Resource() = default;
    virtual ~Resource() = default;

    void SetName(const std::string& name) { name_ = name; }
    std::string GetName() const { return name_; }
private:
    std::string name_;
};

}