#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "./Visualization/Viewer.h"
#include "model.h"

int main()
{
    Viewer viewer("test", 1500, 1000);


    // load model
    Model m("D:/courses/LearnGames103/src/resources/objects/cyborg/cyborg.obj");
    viewer.render_queue.push_back(&m);

    return viewer.run();

}


