#pragma once
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Quark/Asset/Asset.h"

#define QK_SERIALIZE_PROPERTY(propName, propVal, outputNode) outputNode << YAML::Key << #propName << YAML::Value << propVal

#define QK_SERIALIZE_PROPERTY_ASSET(propName, propVal, outputData) outputData << YAML::Key << #propName << YAML::Value << (propVal? (uint64_t)propVal->GetAssetID() : 0);


#define QK_DESERIALIZE_PROPERTY(propertyName, destination, node, defaultValue)	    \
if (node.IsMap())																    \
{																				    \
	if (auto foundNode = node[#propertyName])									    \
	{																			    \
		try																		    \
		{																		    \
			destination = foundNode.as<decltype(defaultValue)>();				    \
		}																		    \
		catch (const std::exception& e)											    \
		{																		    \
			QK_CORE_LOGE_TAG("Core", e.what());										\
																				    \
			destination = defaultValue;											    \
		}																		    \
	}																			    \
	else																		    \
	{																			    \
		destination = defaultValue;												    \
	}																			    \
}																				    \
else																			    \
{																				    \
	destination = defaultValue;													    \
}

namespace YAML {

template<>
struct convert<glm::vec2>
{
    static Node encode(const glm::vec2& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        return node;
    }

    static bool decode(const Node& node, glm::vec2& rhs)
    {
        if (!node.IsSequence() || node.size() != 2)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        return true;
    }
};

template<>
struct convert<glm::vec3>
{
    static Node encode(const glm::vec3& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, glm::vec3& rhs)
    {
        if (!node.IsSequence() || node.size() != 3)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        return true;
    }
};

template<>
struct convert<glm::vec4>
{
    static Node encode(const glm::vec4& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.push_back(rhs.w);
        return node;
    }

    static bool decode(const Node& node, glm::vec4& rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        rhs.w = node[3].as<float>();
        return true;
    }
};
template<>
struct convert<glm::quat>
{
    static Node encode(const glm::quat& rhs)
    {
        Node node;
        node.push_back(rhs.w);
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, glm::quat& rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.w = node[0].as<float>();
        rhs.x = node[1].as<float>();
        rhs.y = node[2].as<float>();
        rhs.z = node[3].as<float>();
        return true;
    }
};

template<>
struct convert<quark::AssetID>
{
    static Node encode(const quark::AssetID& rhs)
    {
        Node node;
        node.push_back((uint64_t)rhs);
        return node;
    }

    static bool decode(const Node& node, quark::AssetID& rhs)
    {
        if(!node.IsScalar())
            return false;

        rhs = node.as<uint64_t>();
        return true;
    }
};
}

namespace quark {

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}


inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.w << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

}