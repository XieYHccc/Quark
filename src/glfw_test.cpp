#include "./Visualization/Viewer.h"
#include "TriMesh.h"
int main(){
    Viewer v("test", 800, 600);

    //FragColor = texture(texture1, TexCoords);
    return v.run();
}                                                                       