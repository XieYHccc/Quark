#pragma once
#include "Asset/BaseAsset.h"



// TODO: Finish the asset manager

using AssetType = u32;
class AssetManager {
public:

    static AssetManager& Instance() {
        static AssetManager instance;
        return instance;
    }
    
    void Init() {};
    void Finalize();

    template<typename T, typename... Args>
    std::shared_ptr<T> AddToPool(const std::string& path, const std::string& name, Args&&... args) 
    {
        auto find = asset_pool_.find(T::GetStaticAssetType());
        if (find == asset_pool_.end()) {
            asset_pool_[T::GetStaticAssetType()] = {};
        }

        std::string uuid = path + ":" + name;
        auto pool = asset_pool_[T::GetStaticAssetType()];
        auto asset = pool.find(uuid);
        if (asset != pool.end()) {
            return std::shared_ptr<T>(static_cast<T*>(asset->second.get()));
        }
        else {
            std::shared_ptr<T> newAsset = std::make_shared<T>(std::forward<Args>(args)...);
            auto base = static_cast<BaseAsset*>(newAsset.get());
            pool[uuid] = std::shared_ptr<BaseAsset>(base);
            return newAsset;
        }
    }
    
    template<typename T>
    std::shared_ptr<T> GetFromPool(const std::string& path, const std::string& name)
    {
        std::string uuid = path + ":" + name;
        auto map = asset_pool_[T::GetStaticAssetType()];
        auto find = map.find(uuid);
        if (find != map.end()) {
            return static_cast<T*>(find->second.get());
        }

        UE_CORE_ERROR("Asset name: {} in file {} doesn't exist.", name, path);
        return nullptr;
    }


private:
    std::unordered_map<AssetType, std::unordered_map<std::string, std::shared_ptr<BaseAsset>>> asset_pool_;

};