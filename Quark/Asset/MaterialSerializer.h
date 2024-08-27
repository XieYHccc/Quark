#pragma once
#include "Quark/Asset/Material.h"

namespace quark {

class MaterialSerializer {
public:

    void Serialize(const std::string& fePath, const Ref<Material>& materialAsset);
    bool TryLoadData(const std::string& fililepath, Ref<Material>& outMaterial);
    
    void SerializeToAssetPack();
    bool DeserializeFromAssetPack();

private:
    std::string SerializeToYaml(const Ref<Material>& materialAsset);
    bool DeserializeFromYaml(const std::string& yamlString, Ref<Material>& outMaterial);

};
}