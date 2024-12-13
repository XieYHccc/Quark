#pragma once
#include "Quark/Asset/MaterialAsset.h"

namespace quark {

class MaterialSerializer {
public:

    void Serialize(const std::string& filePath, const Ref<Material>& materialAsset);
    bool TryLoadData(const std::string& filepath, Ref<Material>& outMaterial);
    
    void Serialize(const std::string& filePath, const Ref<MaterialAsset>& materialAsset);
    bool TryLoadData(const std::string& filepath, Ref<MaterialAsset>& outMaterial);

    void SerializeToAssetPack();
    bool DeserializeFromAssetPack();

private:
    std::string SerializeToYaml(const Ref<Material>& materialAsset);
    bool DeserializeFromYaml(const std::string& yamlString, Ref<Material>& outMaterial);

    std::string SerializeToYaml(const Ref<MaterialAsset>& materialAsset);
    bool DeserializeFromYaml(const std::string& yamlString, Ref<MaterialAsset>& outMaterial);

};
}