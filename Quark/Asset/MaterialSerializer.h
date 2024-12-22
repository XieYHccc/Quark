#pragma once
#include "Quark/Asset/MaterialAsset.h"

namespace quark {

class MaterialSerializer {
public:
    void Serialize(const std::string& filePath, const Ref<MaterialAsset>& materialAsset);
    bool TryLoadData(const std::string& filepath, Ref<MaterialAsset>& outMaterial);

    void SerializeToAssetPack();
    bool DeserializeFromAssetPack();

private:
    std::string SerializeToYaml(const Ref<MaterialAsset>& materialAsset);
    bool DeserializeFromYaml(const std::string& yamlString, Ref<MaterialAsset>& outMaterial);

};
}