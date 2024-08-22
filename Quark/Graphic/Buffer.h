#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Graphic/Common.h"

namespace quark::graphic {

enum BufferUsageBits {
    BUFFER_USAGE_TRANSFER_FROM_BIT = (1 << 0),
    BUFFER_USAGE_TRANSFER_TO_BIT = (1 << 1),
    BUFFER_USAGE_UNIFORM_BUFFER_BIT = (1 << 4),
    BUFFER_USAGE_STORAGE_BUFFER_BIT = (1 << 5),
    BUFFER_USAGE_INDEX_BUFFER_BIT = (1 << 6),
    BUFFER_USAGE_VERTEX_BUFFER_BIT = (1 << 7),
    BUFFER_USAGE_INDIRECT_BIT = (1 << 8),
};

enum class IndexBufferFormat{
    UINT16,
    UINT32
};

enum class BufferMemoryDomain {
    GPU,
    CPU,
};

struct BufferDesc {
    uint64_t size = 0;
    BufferMemoryDomain domain = BufferMemoryDomain::GPU;
    // BufferType type = BufferType::STORAGE_BUFFER;
    uint32_t usageBits = 0;
};


class Buffer : public GpuResource{
public:
    Buffer(const BufferDesc& desc)
        : m_Desc(desc), m_pMappedData(nullptr), m_GpuAddress(UINT64_MAX) 
    {

    }

    ~Buffer() = default;

    const BufferDesc& GetDesc() const { return m_Desc;}
    void* GetMappedDataPtr() { return m_pMappedData; }
    u64 GetGpuAddress() { return m_GpuAddress;}
    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::BUFFER; }

protected:
    BufferDesc m_Desc;
    void* m_pMappedData;
    u64 m_GpuAddress;
};

}