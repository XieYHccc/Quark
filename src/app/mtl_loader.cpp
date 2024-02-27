#include "mtl_loader.h"

#include <fstream>
#include <algorithm>
#include "material.h"

bool LoadMaterials(std::string path) {
    // If the file is not a material file return false
    if (path.substr(path.size() - 4, path.size()) != ".mtl")
        return false;

    std::ifstream file(path);

    // If the file is not found return false
    if (!file.is_open())
        return false;

    Material*  material = new Material();

    bool listening = false;

    // Go through each line looking for material variables
    std::string curline;
    while (std::getline(file, curline))
    {
        // new material and material name
        if (algorithm::firstToken(curline) == "newmtl")
        {
            if (!listening)
            {
                listening = true;

                if (curline.size() > 7)
                {
                    tempMaterial.name = algorithm::tail(curline);
                }
                else
                {
                    tempMaterial.name = "none";
                }
            }
            else
            {
                // Generate the material

                // Push Back loaded Material
                LoadedMaterials.push_back(tempMaterial);

                // Clear Loaded Material
                tempMaterial = Material();

                if (curline.size() > 7)
                {
                    tempMaterial.name = algorithm::tail(curline);
                }
                else
                {
                    tempMaterial.name = "none";
                }
            }
        }
        // Ambient Color
        if (algorithm::firstToken(curline) == "Ka")
        {
            std::vector<std::string> temp;
            algorithm::split(algorithm::tail(curline), temp, " ");

            if (temp.size() != 3)
                continue;

            tempMaterial.Ka.X = std::stof(temp[0]);
            tempMaterial.Ka.Y = std::stof(temp[1]);
            tempMaterial.Ka.Z = std::stof(temp[2]);
        }
        // Diffuse Color
        if (algorithm::firstToken(curline) == "Kd")
        {
            std::vector<std::string> temp;
            algorithm::split(algorithm::tail(curline), temp, " ");

            if (temp.size() != 3)
                continue;

            tempMaterial.Kd.X = std::stof(temp[0]);
            tempMaterial.Kd.Y = std::stof(temp[1]);
            tempMaterial.Kd.Z = std::stof(temp[2]);
        }
        // Specular Color
        if (algorithm::firstToken(curline) == "Ks")
        {
            std::vector<std::string> temp;
            algorithm::split(algorithm::tail(curline), temp, " ");

            if (temp.size() != 3)
                continue;

            tempMaterial.Ks.X = std::stof(temp[0]);
            tempMaterial.Ks.Y = std::stof(temp[1]);
            tempMaterial.Ks.Z = std::stof(temp[2]);
        }
        // Specular Exponent
        if (algorithm::firstToken(curline) == "Ns")
        {
            tempMaterial.Ns = std::stof(algorithm::tail(curline));
        }
        // Optical Density
        if (algorithm::firstToken(curline) == "Ni")
        {
            tempMaterial.Ni = std::stof(algorithm::tail(curline));
        }
        // Dissolve
        if (algorithm::firstToken(curline) == "d")
        {
            tempMaterial.d = std::stof(algorithm::tail(curline));
        }
        // Illumination
        if (algorithm::firstToken(curline) == "illum")
        {
            tempMaterial.illum = std::stoi(algorithm::tail(curline));
        }
        // Ambient Texture Map
        if (algorithm::firstToken(curline) == "map_Ka")
        {
            tempMaterial.map_Ka = algorithm::tail(curline);
        }
        // Diffuse Texture Map
        if (algorithm::firstToken(curline) == "map_Kd")
        {
            tempMaterial.map_Kd = algorithm::tail(curline);
        }
        // Specular Texture Map
        if (algorithm::firstToken(curline) == "map_Ks")
        {
            tempMaterial.map_Ks = algorithm::tail(curline);
        }
        // Specular Hightlight Map
        if (algorithm::firstToken(curline) == "map_Ns")
        {
            tempMaterial.map_Ns = algorithm::tail(curline);
        }
        // Alpha Texture Map
        if (algorithm::firstToken(curline) == "map_d")
        {
            tempMaterial.map_d = algorithm::tail(curline);
        }
        // Bump Map
        if (algorithm::firstToken(curline) == "map_Bump" || algorithm::firstToken(curline) == "map_bump" || algorithm::firstToken(curline) == "bump")
        {
            tempMaterial.map_bump = algorithm::tail(curline);
        }
    }

    // Deal with last material

    // Push Back loaded Material
    LoadedMaterials.push_back(tempMaterial);

    // Test to see if anything was loaded
    // If not return false
    if (LoadedMaterials.empty())
        return false;
    // If so return true
    else
        return true;
}