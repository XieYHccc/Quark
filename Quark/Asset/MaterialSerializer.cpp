#include "Quark/qkpch.h"
#include "Quark/Core/Util/SerializationUtils.h"
#include "Quark/Renderer/Renderer.h"
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

void MaterialSerializer::Serialize(const std::string& filePath, const Ref<Material>& materialAsset)
{
    std::string yamlString = SerializeToYaml(materialAsset);
    std::ofstream fout(filePath);

    fout << yamlString;
}

bool MaterialSerializer::TryLoadData(const std::string& filepath, Ref<Material>& outMaterial)
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

std::string MaterialSerializer::SerializeToYaml(const Ref<Material>& materialAsset)
{
    YAML::Emitter out;
    out << YAML::BeginMap; // Material
    out << YAML::Key << "Material" << YAML::Value;
    {
        out << YAML::BeginMap;

        // Shaders
        QK_SERIALIZE_PROPERTY(VertexShader, materialAsset->shaderProgram->GetSourcePath(graphic::ShaderStage::STAGE_VERTEX), out);
        QK_SERIALIZE_PROPERTY(FragmentShader, materialAsset->shaderProgram->GetSourcePath(graphic::ShaderStage::STAGE_FRAGEMNT), out);

        QK_SERIALIZE_PROPERTY(AlphaMode, AlphaModetoString(materialAsset->alphaMode), out);

        // Uniforms
        QK_SERIALIZE_PROPERTY(BaseColorFactor, materialAsset->uniformBufferData.baseColorFactor, out);
        QK_SERIALIZE_PROPERTY(MetalicFactor, materialAsset->uniformBufferData.metalicFactor, out);
        QK_SERIALIZE_PROPERTY(RoughNessFactor, materialAsset->uniformBufferData.roughNessFactor, out);

        QK_CORE_VERIFY(materialAsset->baseColorTexture->image && materialAsset->metallicRoughnessTexture->image);

        // Textures
        if (materialAsset->baseColorTexture->image != Renderer::Get().image_white)
            QK_SERIALIZE_PROPERTY(BaseColorTexture, materialAsset->baseColorTexture->GetAssetID(), out);
        else 
            QK_SERIALIZE_PROPERTY(BaseColorTexture, 0, out);

        if ( materialAsset->metallicRoughnessTexture && materialAsset->metallicRoughnessTexture->image != Renderer::Get().image_white)
            QK_SERIALIZE_PROPERTY(MetallicRoughnessTexture, materialAsset->baseColorTexture->GetAssetID(), out);
        else 
            QK_SERIALIZE_PROPERTY(MetallicRoughnessTexture, 0, out);
        
        out << YAML::EndMap;
    }

    out << YAML::EndMap; // Material

    return std::string(out.c_str());
}

bool MaterialSerializer::DeserializeFromYaml(const std::string& yamlString, Ref<Material>& outMaterial)
{
    QK_CORE_VERIFY(outMaterial)

    YAML::Node root = YAML::Load(yamlString);
    YAML::Node materialNode = root["Material"];

    if (!materialNode)
    {
        QK_CORE_LOGE_TAG("AssetManager", "MaterialSerializer::DeserializeFromYaml: Material node not found");
        return false;
    }

    QK_DESERIALIZE_PROPERTY(BaseColorFactor, outMaterial->uniformBufferData.baseColorFactor, materialNode, glm::vec4(1.f))
    QK_DESERIALIZE_PROPERTY(MetalicFactor, outMaterial->uniformBufferData.metalicFactor, materialNode, 1.f)
    QK_DESERIALIZE_PROPERTY(RoughNessFactor, outMaterial->uniformBufferData.roughNessFactor, materialNode, 1.f)

    AssetID baseColorTextureId = 0;
    AssetID metallicRoughnessTextureId = 0;
    QK_DESERIALIZE_PROPERTY(BaseColorTexture, baseColorTextureId, materialNode, AssetID(0))
    QK_DESERIALIZE_PROPERTY(MetallicRoughnessTexture, metallicRoughnessTextureId, materialNode, AssetID(0))

    Ref<Texture> defaultTexture = CreateRef<Texture>();
    defaultTexture->image = Renderer::Get().image_white;
    defaultTexture->sampler = Renderer::Get().sampler_linear;

    auto getTextureAsset = [&](AssetID id) -> Ref<Texture>
    {
        if (id != 0)
        {
            auto textureAsset = AssetManager::Get().GetAsset<Texture>(id);
            if (textureAsset)
                return textureAsset;
            else
            {
                QK_CORE_LOGE_TAG("AssetManager", "MaterialSerializer::DeserializeFromYaml: Texture asset not found");
                return defaultTexture;
            }
        }
        else
            return defaultTexture;
    };

    outMaterial->baseColorTexture = getTextureAsset(baseColorTextureId);
    outMaterial->metallicRoughnessTexture = getTextureAsset(metallicRoughnessTextureId);

    std::string AlphaMode;
    QK_DESERIALIZE_PROPERTY(AlphaMode, AlphaMode, materialNode, std::string("Opaque"));

    outMaterial->alphaMode = AlphaMode == "Opaque" ? AlphaMode::MODE_OPAQUE : AlphaMode::MODE_TRANSPARENT;

    std::string vertexShaderPath;
    QK_DESERIALIZE_PROPERTY(VertexShader, vertexShaderPath, materialNode, std::string(""));

    std::string fragmentShaderPath;
    QK_DESERIALIZE_PROPERTY(FragmentShader, fragmentShaderPath, materialNode, std::string(""));

    QK_CORE_VERIFY(!vertexShaderPath.empty() && !fragmentShaderPath.empty())
        //TODO: Remove hardcoded program
    outMaterial->shaderProgram = Renderer::Get().GetShaderLibrary().program_staticMeshEditor;

    return true;
}
}