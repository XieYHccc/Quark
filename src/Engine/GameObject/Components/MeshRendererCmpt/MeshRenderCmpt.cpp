#include "GameObject/Components/MeshRendererCmpt/MeshRenderCmpt.h"
#include "GameObject/GameObject.h"

void MeshRendererCmpt::GenerateRenderObjects()
{
    renderObjects.clear();
    
    for (auto& s : gpuMesh->submeshes) {
        RenderObject newObject;

        newObject.indexCount = s.count;
        newObject.firstIndex = s.startIndex;
        newObject.indexBuffer = gpuMesh->meshBuffers.indexBuffer.buffer;
        newObject.material = s.material;
        newObject.bounds = s.bounds;
        newObject.transform = GetOwner()->transformCmpt->GetTRSMatrix();
        newObject.vertexBufferAddress = gpuMesh->meshBuffers.vertexBufferAddress;

        renderObjects.push_back(newObject);
    }
}

const std::vector<RenderObject>& MeshRendererCmpt::GetRenderObjects()
{
    if (gpuMesh->Dynamic) {
        GenerateRenderObjects();
        return renderObjects;
    }

    if (isDirty) {
        GenerateRenderObjects();
        isDirty = false;
    }

    return renderObjects;
}


