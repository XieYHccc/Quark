#include "Quark/qkpch.h"
#include "Quark/Core/Util/SerializationUtils.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/MaterialSerializer.h"
#include "Quark/Asset/AssetManager.h"

namespace quark {
static std::string AlphaModetoString(AlphaMode mode)
{
    if (mode == AlphaMode::MODE_OPAQUE)
        return "Opaque";
    else
        return "Transparent";
}

void MaterialSerializer::Serialize(const std::string& filePath, const Ref<MaterialAsset>& materialAsset)
{
    std::string yamlString = SerializeToYaml(materialAsset);
    std::ofstream fout(filePath);

    fout << yamlString;
}

bool MaterialSerializer::TryLoadData(const std::string& filepath, Ref<MaterialAsset>& outMaterial)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        QK_CORE_LOGE_TAG("AssetManger", "MaterialSerializer::TryLoadData: Failed to open file {0}", filepath);
        return false;
    }

    std::stringstream strStream;
    strStream << stream.rdbuf();

    return DeserializeFromYaml(strStream.str(), outMaterial);
}


std::string MaterialSerializer::SerializeToYaml(const Ref<MaterialAsset>& materialAsset)
{
    YAML::Emitter out;
    out << YAML::BeginMap; // Material
    out << YAML::Key << "Material" << YAML::Value;
    {
        out << YAML::BeginMap;

        QK_SERIALIZE_PROPERTY(BaseColorFactor, materialAsset->baseColorFactor, out);
        QK_SERIALIZE_PROPERTY(MetalicFactor, materialAsset->metalicFactor, out);
        QK_SERIALIZE_PROPERTY(RoughNessFactor, materialAsset->roughNessFactor, out);

        QK_SERIALIZE_PROPERTY(AlphaMode, AlphaModetoString(materialAsset->alphaMode), out);

        QK_SERIALIZE_PROPERTY(BaseColorImage, materialAsset->baseColorImage, out);
        QK_SERIALIZE_PROPERTY(MetallicRoughnessmage, materialAsset->metallicRoughnessImage, out);
        QK_SERIALIZE_PROPERTY(NormalImage, materialAsset->normalImage, out);

        QK_SERIALIZE_PROPERTY(VertexShader, materialAsset->vertexShaderPath, out);
        QK_SERIALIZE_PROPERTY(FragmentShader, materialAsset->fragmentShaderPath, out);

        out << YAML::EndMap;
    }

    out << YAML::EndMap; // Material

    return std::string(out.c_str());
}

bool MaterialSerializer::DeserializeFromYaml(const std::string& yamlString, Ref<MaterialAsset>& outMaterial)
{
    QK_CORE_VERIFY(outMaterial)

    YAML::Node root = YAML::Load(yamlString);
    YAML::Node materialNode = root["Material"];

    if (!materialNode)
    {
        QK_CORE_LOGE_TAG("AssetManager", "MaterialSerializer::DeserializeFromYaml: Material node not found");
        return false;
    }

    QK_DESERIALIZE_PROPERTY(BaseColorFactor, outMaterial->baseColorFactor, materialNode, glm::vec4(1.f))
    QK_DESERIALIZE_PROPERTY(MetalicFactor, outMaterial->metalicFactor, materialNode, 1.f)
    QK_DESERIALIZE_PROPERTY(RoughNessFactor, outMaterial->roughNessFactor, materialNode, 1.f)
    QK_DESERIALIZE_PROPERTY(BaseColorImage, outMaterial->baseColorImage, materialNode, AssetID(0))
    QK_DESERIALIZE_PROPERTY(MetallicRoughnessImage, outMaterial->metallicRoughnessImage, materialNode, AssetID(0))
    QK_DESERIALIZE_PROPERTY(VertexShader, outMaterial->vertexShaderPath, materialNode, std::string(""));
    QK_DESERIALIZE_PROPERTY(FragmentShader, outMaterial->fragmentShaderPath, materialNode, std::string(""));

    std::string AlphaMode;
    QK_DESERIALIZE_PROPERTY(AlphaMode, AlphaMode, materialNode, std::string("Opaque"));
    outMaterial->alphaMode = AlphaMode == "Opaque" ? AlphaMode::MODE_OPAQUE : AlphaMode::MODE_TRANSPARENT;

    return true;
}
}