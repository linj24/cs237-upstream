/*! \file scene.cpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * Code to load a scene description.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 1 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hpp"
#include "scene.hpp"
#include <map>
#include <functional>
#include <iostream>

/* helper functions to make extracting values from the json easier */

//! load a vec3f from a json object.
//! \return false if okay, true if there is an error.
static bool loadVec3 (json::Object const *jv, glm::vec3 &vec)
{
    const json::Number *x = jv->fieldAsNumber("x");
    const json::Number *y = jv->fieldAsNumber("y");
    const json::Number *z = jv->fieldAsNumber("z");
    if ((x == nullptr) || (y == nullptr) || (z == nullptr)) {
        return true;
    }

    vec = glm::vec3 (
        static_cast<float>(x->realVal()),
        static_cast<float>(y->realVal()),
        static_cast<float>(z->realVal()));

    return false;
}

//! load a color3f from a json object.
//! \return false if okay, true if there is an error.
static bool loadColor (json::Object const *jv, glm::vec3 &color)
{
    const json::Number *r = jv->fieldAsNumber("r");
    const json::Number *g = jv->fieldAsNumber("g");
    const json::Number *b = jv->fieldAsNumber("b");
    if ((r == nullptr) || (g == nullptr) || (b == nullptr)) {
        return true;
    }

    color = glm::vec3 (
        static_cast<float>(r->realVal()),
        static_cast<float>(g->realVal()),
        static_cast<float>(b->realVal()));

    return false;
}

//! load a window size from a json object.
//! \return false if okay, true if there is an error.
static bool loadSize (json::Object const *jv, int &wid, int &ht)
{
    const json::Integer *w = jv->fieldAsInteger ("wid");
    const json::Integer *h = jv->fieldAsInteger ("ht");

    if ((w == nullptr) || (h == nullptr)) {
        return true;
    }
    else {
        wid = static_cast<int>(w->intVal());
        ht = static_cast<int>(h->intVal());
        return false;
    }

}

//! load a ground size from a json object.
//! \return false if okay, true if there is an error.
bool loadSize (json::Object const *jv, float &wid, float &ht)
{
    if (jv == nullptr) {
        return true;
    }

    const json::Number *w = jv->fieldAsNumber ("wid");
    const json::Number *h = jv->fieldAsNumber ("ht");

    if ((w == nullptr) || (h == nullptr)) {
        return true;
    }
    else {
        wid = static_cast<float>(w->realVal());
        ht = static_cast<float>(h->realVal());
        return false;
    }

}

//! load a float from a json object.
//! \return false if okay, true if there is an error.
static bool loadFloat (json::Value const *jv, float &f)
{
    if ((jv == nullptr) || (! jv->isNumber())) {
        return true;
    }
    else {
        f = static_cast<float>(jv->asNumber()->realVal());
        return false;
    }
}

/***** class Scene member functions *****/

bool Scene::load (std::string const &path)
{
    if (this->_loaded) {
        std::cerr << "Scene is already loaded" << std::endl;
        return true;
    }
    this->_loaded = true;

    std::string sceneDir = path + "/";

    // load the scene description file
    json::Value *root = json::parseFile(sceneDir + "scene.json");

    // check for errors
    if (root == nullptr) {
        std::cerr << "Unable to load scene \"" << path << "\"\n";
        return true;
    } else if (! root->isObject()) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; root is not an object" << std::endl;
        return true;
    }
    const json::Object *rootObj = root->asObject();

    // load the camera info
    const json::Object *cam = rootObj->fieldAsObject ("camera");
    if ((cam == nullptr)
    ||  loadSize (cam->fieldAsObject ("size"), this->_wid, this->_ht)
    ||  loadFloat (cam->fieldAsNumber ("fov"), this->_fov)
    ||  loadVec3 (cam->fieldAsObject ("pos"), this->_camPos)
    ||  loadVec3 (cam->fieldAsObject ("look-at"), this->_camAt)
    ||  loadVec3 (cam->fieldAsObject ("up"), this->_camUp)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad camera" << std::endl;
        return true;
    }

    // load the lighting information
    const json::Object *lighting = rootObj->fieldAsObject ("lighting");
    if ((lighting == nullptr)
    ||  loadColor (lighting->fieldAsObject ("ambient"), this->_ambI)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lighting\n";
        return true;
    }
    // make sure that the ambient-light intensity is in 0..1 range
    this->_ambI = glm::clamp(this->_ambI, 0.0f, 1.0f);
    // get the array of point lights; we allow at most 4 lights
    json::Array const *lights = lighting->fieldAsArray("lights");
    if ((lights == nullptr) || (lights->length() == 0) || (lights->length() > 4)) {
        std::cerr << "Invalid scene description in \"" << path
            << "\"; bad lights array\n";
        return true;
    }
    // allocate space for the lights in the scene
    this->_lights.resize(lights->length());
    for (int i = 0;  i < lights->length();  i++) {
        json::Object const *light = (*lights)[i]->asObject();
        if (loadVec3 (light->fieldAsObject ("pos"), this->_lights[i].pos)
        ||  loadColor (light->fieldAsObject ("intensity"), this->_lights[i].intensity)) {
            std::cerr << "Invalid scene description in \"" << path
                << "\"; bad lighting\n";
            return true;
        }
        // get attenuation coefficients
        json::Array const *aten = light->fieldAsArray("attenuation");
        if ((aten == nullptr)
        ||  (aten->length() != 3)
        ||  loadFloat((*aten)[0], this->_lights[i].k0)
        ||  loadFloat((*aten)[1], this->_lights[i].k1)
        ||  loadFloat((*aten)[2], this->_lights[i].k2)) {
            std::cerr << "Invalid scene description in \"" << path
                << "\"; bad attenuation array\n";
            return true;
        }
        // make sure that the light intensity is in 0..1 range
        this->_lights[i].intensity = glm::clamp(this->_lights[i].intensity, 0.0f, 1.0f);
    }

    // get the object array from the json tree and check that it is non-empty
    json::Array const *objs = rootObj->fieldAsArray("objects");
    if ((objs != nullptr) && (objs->length() >= 0)) {
        // allocate space for the objects in the scene
        this->_objs.resize(objs->length());

        // we use a map to keep track of which models have already been loaded
        std::map<std::string, int> objMap;
        std::map<std::string, int>::iterator it;

        // load the objects in the scene
        int numModels = 0;
        for (int i = 0;  i < objs->length();  i++) {
            json::Object const *object = (*objs)[i]->asObject();
            if (object == nullptr) {
                std::cerr << "Expected array of json objects for field 'objects' in \""
                    << path << "\"\n";
                return true;
            }
            json::String const *file = object->fieldAsString("file");
            json::Object const *frame = object->fieldAsObject("frame");
            glm::vec3 pos, xAxis, yAxis, zAxis;
            if ((file == nullptr) || (frame == nullptr)
            ||  loadVec3 (object->fieldAsObject("pos"), pos)
            ||  loadVec3 (frame->fieldAsObject("x-axis"), xAxis)
            ||  loadVec3 (frame->fieldAsObject("y-axis"), yAxis)
            ||  loadVec3 (frame->fieldAsObject("z-axis"), zAxis)
            ||  loadColor (object->fieldAsObject("color"), this->_objs[i].color)) {
                std::cerr << "Invalid objects description in \"" << path << "\"\n";
                return true;
            }
            // have we already loaded this model?
            it = objMap.find(file->value());
            int modelId;
            if (it != objMap.end()) {
                modelId = it->second;
            }
            else {
                // load the model from the file sytem and add it to the map
                modelId = numModels++;
                OBJ::Model *model = new OBJ::Model (sceneDir + file->value());
                this->_models.push_back(model);
                objMap.insert (std::pair<std::string, int> (file->value(), modelId));
            }
            this->_objs[i].model = modelId;
            // set the object-space to world-space transform
            this->_objs[i].toWorld = glm::mat4 (
                glm::vec4 (xAxis, 0.0f),
                glm::vec4 (yAxis, 0.0f),
                glm::vec4 (zAxis, 0.0f),
                glm::vec4 (pos, 1.0f));
        }
    }

    // load the texture images used by the materials in the models
    for (auto modIt = this->_models.begin();  modIt != this->_models.end();  modIt++) {
        const OBJ::Model *model = *modIt;
        for (auto grpIt = model->beginGroups();  grpIt != model->endGroups();  grpIt++) {
            const OBJ::Material *mat = &model->Material((*grpIt).material);
            this->_loadTexture (sceneDir, mat->diffuseMap);
            this->_loadTexture (sceneDir, mat->normalMap, true);
        }
    }

    // load the ground information (if present)
    const json::Object *ground = rootObj->fieldAsObject ("ground");
    if (ground != nullptr) {
        float wid, ht;
        float vScale;
        glm::vec3 color;
        json::String const *hf = ground->fieldAsString("height-field");
        json::String const *cmap = ground->fieldAsString("color-map");
        json::String const *nmap = ground->fieldAsString("normal-map");
        if ((hf == nullptr) || (cmap == nullptr) || (nmap == nullptr)
        ||  loadSize (ground->fieldAsObject("size"), wid, ht)
        ||  loadFloat (ground->fieldAsNumber ("v-scale"), vScale)
        ||  loadColor (ground->fieldAsObject("color"), color)) {
            std::cerr << "Invalid ground description in \"" << path << "\"\n";
            return true;
        }
        // load the color-map and normal-map textures
        this->_loadTexture (sceneDir, cmap->value());
        cs237::Image2D *cmapImg = this->textureByName (cmap->value());
        this->_loadTexture (sceneDir, nmap->value(), true);
        cs237::Image2D *nmapImg = this->textureByName (nmap->value());
        this->_hf = new HeightField (sceneDir + hf->value(), wid, ht, vScale, color, cmapImg, nmapImg);
    }
    else {
        this->_hf = nullptr;
    }

    if ((ground == nullptr) && (objs == nullptr || objs->length() == 0)) {
        std::cerr << "Invalid empty scene description in \"" << path << "\"\n";
        return true;
    }

    // free up the space used by the json object
    delete root;

    return false;
}

void Scene::_loadTexture (std::string path, std::string name, bool nMap)
{
    if (name.empty()) {
        return;
    }
    // have we already loaded this texture?
    if (this->_texs.find(name) != this->_texs.end()) {
        return;
    }
    // load the image data;
    cs237::Image2D *img;
    if (nMap) {
        // normal data should not be sRGB encoded!
        img = new cs237::DataImage2D(path + name);
    } else {
        img = new cs237::Image2D(path + name);
    }
    // add to _texs map
    this->_texs.insert (std::pair<std::string, cs237::Image2D *>(name, img));

}

cs237::Image2D *Scene::textureByName (std::string name) const
{
    if (! name.empty()) {
        auto it = this->_texs.find(name);
        if (it != this->_texs.end()) {
            return it->second;
        }
    }
    return nullptr;

}

Scene::Scene ()
    : _loaded(false), _wid(0), _ht(0), _fov(0),
      _camPos(), _camAt(), _camUp(),
      _models(), _objs()
{ }

Scene::~Scene ()
{
    for (auto it = this->_models.begin();  it != this->_models.end();  it++) {
        delete *it;
    }
    for (auto it = this->_texs.begin();  it != this->_texs.end();  it++) {
        delete it->second;
    }
}
