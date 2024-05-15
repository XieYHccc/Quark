#include "GameObject/Components/MeshCmpt.h"
#include "GameObject/GameObject.h"

void MeshCmpt::GenerateRenderObjects()
{
    renderObjects.clear();
    
    for (auto& s : mesh->GetSubMeshes()) {
        RenderObject newObject;

        newObject.indexCount = s.count;
        newObject.firstIndex = s.startIndex;
        newObject.indexBuffer = mesh->GetIndexBuffer().vkBuffer;
        newObject.material = s.material;
        newObject.bounds = s.bounds;
        newObject.transform = GetOwner()->transformCmpt->GetTRSMatrix();
        newObject.vertexBufferAddress = mesh->GetVertBufferAddress();

        renderObjects.push_back(newObject);
    }
}

const std::vector<RenderObject>& MeshCmpt::GetRenderObjects()
{
    if (mesh->IsDynamic()) {
        GenerateRenderObjects();
        return renderObjects;
    }

    if (isDirty) {
        GenerateRenderObjects();
        isDirty = false;
    }

    return renderObjects;
}


