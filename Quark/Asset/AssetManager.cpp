#include "Asset/AssetManager.h"


void AssetManager::Finalize()
{
    for (auto& [k, value] : asset_pool_) {
        for (auto& [id, asset] : value) {
            asset.reset();
        }
    }
}