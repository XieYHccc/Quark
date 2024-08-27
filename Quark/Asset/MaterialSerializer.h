#pragma once
#include "Quark/Asset/Material.h"

namespace quark {

class MaterialSerializer {
public:

    void Serialize(const std::string& filePath, const Ref<Material>& materialAsset);
    bool TryLoadData(const std::string& filepath, Ref<Material>& outMaterial);
    
    void SerializeBinary();
    bool DeserializeBinary();

private:
    std::string SerializeToYaml(const Ref<Material>& materialAsset);
    bool DeserializeFromYaml(const std::string& yamlString, Ref<Material>& outMaterial);

};
}