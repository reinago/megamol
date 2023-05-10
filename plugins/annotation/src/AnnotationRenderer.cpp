/**
 * MegaMol
 * Copyright (c) 2019, MegaMol Dev Team
 * All rights reserved.
 */

#include "AnnotationRenderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <nlohmann/json.hpp> // Needed for Json
#include <fstream>

#include "mmcore/CoreInstance.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/utility/log/Log.h"
#include "mmcore_gl/utility/ShaderFactory.h"

#include "FrontendResource.h"
#include "CommonTypes.h"

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_tex_inspect.h"

using json = nlohmann::json;

using namespace megamol::mmstd_gl;
using namespace megamol::core::utility;

using namespace megamol::annotation;
struct annotation_struct;
/*
 * AnnotationRenderer::AnnotationRenderer
 */
AnnotationRenderer::AnnotationRenderer()
        : Renderer3DModuleGL()
        , enableAnnotationRendererSlot("AnnotationRenderer", "Enables the rendering of the Annotations")
        , sizeScalingSlot("scaling factor", "Scaling factor for the size of the rendered GL_POINTS")
        , linesColorSlot("linesColor", "Color of the Connection Lines between Annotation and Point in 3D")
        , sphereColorSlot("sphereColor", "Color of the Spheres that show the position of the annotations")
        , drawTextSlot("Draw 3D Text", "Enables the drawing of a 3D Text for each Annotation")
        , titleColorSlot("Title Color", "Color for the title of Annotations")
        , textColorSlot("Text Color", "Color for the shown Annotations in the Data")
        , wrapWidthSlot("Wraping Width for Annotations", "Sets the value for the width of the wraping of the Annotations")
        , saveSlotValuesSlot("saveSlotValues", "Saves the current values of the slots")
        , loadSlotValuesSlot("loadSlotValues", "Loades the saved values of the slots")
        , saveJsonToFileSlot("saveJsonToFile", "Saves the current state of the Annotation to a Json File")
        , loadJsonFromFileSlot("loadJsonFromFile", "Loads the state of the Annotation from a Json File")
        , enableAddingAnnotationWindowSlot("Adding Annotations Window", "Enables the Window for adding new Annotations")
        , enableJsonWindowSlot("Json Window", "Enables the Window for handling saved Annotations") //TODO: better description + name
        , enableListWindowSlot("List Window", "Enables the Window that shows the list of all Annotations")
        , vbo(0)
        , ibo(0)
        , va(0)
        , occlusionQuery()
        , frameType(0)
        , boundingBoxes()
        , totalFrameCount(0.0f)
        , picking_enabled(false)
        , picked_a_point(false)
        , lastX()
        , lastY()
        , my_color()
        , pointWindowSizes()
        , first_win_coordinates_input()
        , first_win_color_input()
        , first_win_color()
        , tryOut(false)
        , anotherWindow(false)
        , annot_win_struct()
        , show_json_window(false)
        , json_file_path()
        , json_point_name_selectedIndex(0)
        , grh(false)
        , all_annotations()
        , json_obj()
        , json_amount(0) {

    this->enableAnnotationRendererSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->enableAnnotationRendererSlot);

    this->sizeScalingSlot.SetParameter(new core::param::FloatParam(0.1f, 0.01f, 1000.0f));
    this->MakeSlotAvailable(&this->sizeScalingSlot);

    this->linesColorSlot.SetParameter(new core::param::ColorParam("#ffffffff"));
    this->MakeSlotAvailable(&this->linesColorSlot);
    
    this->sphereColorSlot.SetParameter(new core::param::ColorParam("#ffffffff"));
    this->MakeSlotAvailable(&this->sphereColorSlot);

    this->drawTextSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->drawTextSlot);
    
    this->titleColorSlot.SetParameter(new core::param::ColorParam("#ffffffff"));
    this->MakeSlotAvailable(&this->titleColorSlot);
    
    this->textColorSlot.SetParameter(new core::param::ColorParam("#ffffffff"));
    this->MakeSlotAvailable(&this->textColorSlot);

    this->wrapWidthSlot.SetParameter(new core::param::FloatParam(15.0f));
    this->MakeSlotAvailable(&this->wrapWidthSlot); // TODO: maybe do this wrapWidthSlot with a slider?
    
    this->saveSlotValuesSlot.SetParameter(
        new core::param::ButtonParam(core::view::Key::KEY_A, core::view::Modifier::SHIFT));
    this->MakeSlotAvailable(&this->saveSlotValuesSlot);

    this->loadSlotValuesSlot.SetParameter(
        new core::param::ButtonParam(core::view::Key::KEY_B, core::view::Modifier::SHIFT));
    this->MakeSlotAvailable(&this->loadSlotValuesSlot);

    this->loadJsonFromFileSlot.SetParameter(
        new core::param::ButtonParam(core::view::Key::KEY_C, core::view::Modifier::SHIFT));
    this->MakeSlotAvailable(&this->loadJsonFromFileSlot);

    this->saveJsonToFileSlot.SetParameter(
        new core::param::ButtonParam(core::view::Key::KEY_D, core::view::Modifier::SHIFT));
    this->MakeSlotAvailable(&this->saveJsonToFileSlot);

    this->enableAddingAnnotationWindowSlot.SetParameter(new core::param::BoolParam(false));
    this->MakeSlotAvailable(&this->enableAddingAnnotationWindowSlot);

    this->enableJsonWindowSlot.SetParameter(new core::param::BoolParam(false));
    this->MakeSlotAvailable(&this->enableJsonWindowSlot);

    this->enableListWindowSlot.SetParameter(new core::param::BoolParam(false));
    this->MakeSlotAvailable(&this->enableListWindowSlot);

    this->json_obj["Points"];
    this->json_obj["SlotValues"];

    // load the json file:
    // TODO: loading jason from file here throws an error
    // load_json_from_file();
}

/*
 * AnnotationRenderer::~AnnotationRenderer
 */
AnnotationRenderer::~AnnotationRenderer() {
    this->Release();
}

/*
* Saves the mouse coordinates after the last move.
*/
bool AnnotationRenderer::OnMouseMove(double x, double y) {
    // IMPORTANT: the x, y from this function count from the TOP left of the screen
    // the x,y in the framebuffer are counted from the BOTTOM left of the screen
    RendererModule::OnMouseMove(x, y);
    this->lastX = x;
    this->lastY = y;
    // printf("lastX: %f, lastY: %f", this->lastX, this->lastY);
    return false;
}

/*
 * Runs when a mouse button is clicked.
 * On left mouse click with no modifieres it will set a flag for saving the current coordinates of the cursor.
 */
bool AnnotationRenderer::OnMouseButton(megamol::core::view::MouseButton button,
    megamol::core::view::MouseButtonAction action, megamol::core::view::Modifiers mods) {
    // Only continue if a Left Mouse Button press is detected
    if (button != core::view::MouseButton::BUTTON_LEFT) {
        return false;
    }

    if (action == core::view::MouseButtonAction::PRESS) {
        printf("Hey you pressed a button\n");
        printf("x: %f, y: %f \n", this->lastX, this->lastY);

        // If picking is enabled then we set the boolean picked_a_point to true, so the other functions can calculate the coordinates with the current mouse position.
        if (picking_enabled) {
            picked_a_point = true;
        }
    }

    return false;
}

/*
 * AnnotationRenderer::create
 */
bool AnnotationRenderer::create() {
    using namespace megamol::core::utility::log;

    auto const shader_options = ::msf::ShaderFactoryOptionsOpenGL(GetCoreInstance()->GetShaderPaths());
    try {
        lineShader = core::utility::make_glowl_shader("simpleLine", shader_options,
            "annotation/simple_line.vert.glsl", "annotation/simple_line.frag.glsl");
        simpleShader = core::utility::make_glowl_shader("simplePoints", shader_options,
            "annotation/simple_points.vert.glsl", "annotation/simple_points.frag.glsl");
        sphereShader =
            core::utility::make_glowl_shader("prettyPoints", shader_options, "annotation/pretty_points.vert.glsl",
                "annotation/pretty_points.geom.glsl", "annotation/pretty_points.frag.glsl");

    } catch (std::exception& e) {
        Log::DefaultLog.WriteError("AnnotationRenderer: {}", e.what());
        return false;
    }

    // the used vertex order resembles the one used in the old View3D
    std::vector<float> vertexCoords = {-1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<unsigned int> vertexIndices = {0, 2, 3, 1, 4, 5, 7, 6, 2, 6, 7, 3, 0, 1, 5, 4, 0, 4, 6, 2, 1, 3, 7, 5};

    glCreateBuffers(1, &this->vbo);
    glCreateBuffers(1, &this->ibo);
    glCreateVertexArrays(1, &this->va);

    glBindVertexArray(this->va);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);

    glEnableVertexAttribArray(0);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCoords.size(), vertexCoords.data(), GL_STATIC_DRAW);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

/*
 * AnnotationRenderer::release
 */
void AnnotationRenderer::release() {
    if (this->va != 0) {
        glDeleteVertexArrays(1, &this->va);
        this->va = 0;
    }
    if (this->vbo != 0) {
        glDeleteBuffers(1, &this->vbo);
        this->vbo = 0;
    }
    if (this->ibo != 0) {
        glDeleteBuffers(1, &this->ibo);
        this->ibo = 0;
    }
}

/*
 * AnnotationRenderer::GetExtents
 */
bool AnnotationRenderer::GetExtents(CallRender3DGL& call) {

    CallRender3DGL* chainedCall = this->chainRenderSlot.CallAs<CallRender3DGL>();
    if (chainedCall != nullptr) {
        *chainedCall = call;
        if ((*chainedCall)(core::view::AbstractCallRender::FnGetExtents)) {
            call = *chainedCall;
            this->boundingBoxes = call.AccessBoundingBoxes(); //TODO: change this, because we have no bounding boxes
            // TODO: save amount of Frames
            this->totalFrameCount = chainedCall->TimeFramesCount();
            // printf("totalFrameCount: %f\n", this->totalFrameCount);
            return true;
        }
    }
    megamol::core::utility::log::Log::DefaultLog.WriteError(
        "The AnnotationRenderer does not work without a renderer attached to its right");
    return false;
}

/*
 * AnnotationRenderer::Render
 */
bool AnnotationRenderer::Render(CallRender3DGL& call) {

    core::view::Camera cam = call.GetCamera();
    auto const lhsFBO = call.GetFramebuffer();
    // call.Time()

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix();
    glm::mat4 mvp = proj * view;

    CallRender3DGL* chainedCall = this->chainRenderSlot.CallAs<CallRender3DGL>();
    if (chainedCall == nullptr) {
        megamol::core::utility::log::Log::DefaultLog.WriteError(
            "The AnnotationRenderer does not work without a renderer attached to its right");
        return false;
    }

    lhsFBO->bind();
    glViewport(0, 0, lhsFBO->getWidth(), lhsFBO->getHeight());
    


    bool renderRes = true;
    frameType = (frameType + 1) % 2;
    if (this->enableAnnotationRendererSlot.Param<core::param::BoolParam>()->Value()) {
        //test(call);
        // TODO: just testing new main function:
        new_main(call);
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    *chainedCall = call;
    renderRes &= (*chainedCall)(core::view::AbstractCallRender::FnRender);

    lhsFBO->bind();
    glViewport(0, 0, lhsFBO->getWidth(), lhsFBO->getHeight());

    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return renderRes;
}


/*
 * Main function.
 * This function generates the main ImGui window and allows the opening of all other windows.
 */
void AnnotationRenderer::new_main(CallRender3DGL& call) {
    // TODO: With this version it is not possible to close the window with the "x" button
    /* Displays the Window for adding new Annotations */
    if (this->enableAddingAnnotationWindowSlot.Param<core::param::BoolParam>()->Value()) {
        this->anotherWindow = true;
        showAddingAnotationWindow(call, "Adding new Annotations");
    } else {
        this->anotherWindow = false;
    }

    /* Displays the Window for Handling all current Annotations */
    if (this->enableJsonWindowSlot.Param<core::param::BoolParam>()->Value()) {
        this->show_json_window = true;
        determine_points_to_be_shown(call);
    } else {
         this->show_json_window = false;
    }

    if (this->enableListWindowSlot.Param<core::param::BoolParam>()->Value()) {
        list_Window(call);
    }
    
    if (this->loadJsonFromFileSlot.IsDirty()) {
        this->loadJsonFromFileSlot.ResetDirty();
        loadJsonFromFileToVectors(call);
    }

    if (this->saveJsonToFileSlot.IsDirty()) {
        this->saveJsonToFileSlot.ResetDirty();
        // Update the JsonObject with the current state of the all_annotations vector
        for (int i = 0; i < this->all_annotations.size(); i++) {
            updateAnnotationInJsonObj(call, i);
        }
        save_json_to_file();
    }

    if (this->loadSlotValuesSlot.IsDirty()) {
        this->loadSlotValuesSlot.ResetDirty();
        load_slot_values_from_json();
    }

    if (this->saveSlotValuesSlot.IsDirty()) {
        this->saveSlotValuesSlot.ResetDirty();
        save_slot_values_to_json();
    } 
}

/*
* Function for the ImGui window that allows the adding of a new point.
*/
void AnnotationRenderer::showAddingAnotationWindow(CallRender3DGL& call, std::string window_name) {
    // Is this needed here as well?
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    std::string windowNameString = window_name + std::string("##") + window_name;
    bool* p_open = NULL; // for removing the x on the top right corner of the window
    ImGui::Begin(windowNameString.c_str(), p_open);
    ImGui::InputText("Point Name", &this->annot_win_struct.point_name_input);
    ImGui::Text("Write your annotations here:");
    ImGui::InputText("Annotation", &this->annot_win_struct.annotation_input);
    ImGui::Text(this->annot_win_struct.annotation_input.c_str()); // TODO: Allow \n or similar functions to work!
    ImGui::InputFloat3("input coordinates", this->annot_win_struct.coordinates_input);

    // save inputs in global struct variable annot_win_struct
    this->annot_win_struct.annot_struct.coordinates = glm::vec3(this->annot_win_struct.coordinates_input[0],
        this->annot_win_struct.coordinates_input[1], this->annot_win_struct.coordinates_input[2]);
    this->annot_win_struct.annot_struct.name = this->annot_win_struct.point_name_input;
    this->annot_win_struct.annot_struct.annotation = this->annot_win_struct.annotation_input;

    // Button for starting the picking process
    if (ImGui::Button("Click in the viewport to add a new point")) {
        this->picking_enabled = true;
    }

    // Button for saving the current camera position
    if (ImGui::Button("Save current camera position")) {
        this->annot_win_struct.annot_struct.cam_pos = call.GetCamera().getPose().position;
        this->annot_win_struct.annot_struct.cam_orientation = call.GetCamera().getPose().to_quat();
    }

    if (this->picking_enabled) {
        ImGui::Text("Click in the viewport to add a new point");
        // Wait till the user has clicked in the Window and then calculate the coordinates from this point.
        // this uses the variables lastX and lastY that are updated everytime the mouse is moved.
        if (this->picked_a_point) {
            glm::vec3 picked_point = calcClickedPoint(this->lastX, this->lastY, call);
            // are these texts even needed? because they will just vanish after 1 frame
            ImGui::Text("Picked a point!");
            ImGui::Text("x: %f, y: %f, z: %f", picked_point.x, picked_point.y, picked_point.z);
            this->annot_win_struct.annot_struct.coordinates = picked_point;
            // this conversion is needed to show the coordinates in the ImGui window
            this->annot_win_struct.coordinates_input[0] = picked_point.x;
            this->annot_win_struct.coordinates_input[1] = picked_point.y;
            this->annot_win_struct.coordinates_input[2] = picked_point.z;
            this->picked_a_point = false;
            this->picking_enabled = false;
        }
    }

    
    if (ImGui::Button("Toggle Sphere")) {
        if (this->annot_win_struct.show_point) {
            this->annot_win_struct.show_point = false;
        } else {
            this->annot_win_struct.show_point = true;
        }
    }

    ImGui::Text("Save your Timestamps here:");
    if (ImGui::Button("Start")) {
        this->annot_win_struct.annot_struct.start_ts = call.Time();
    }
    if (ImGui::Button("End")) {
        this->annot_win_struct.annot_struct.end_ts = call.Time();
    }
    if (ImGui::Button("Save current coords and Annotation")) {
        // TODO: CLEAR all inputs of the imgui variables in this window after saving the new point (to prevent dupplications etc)?
        // save_new_point_to_json(call, this->annot_win_struct.annot_struct);
        saveNewPoint(call, this->annot_win_struct.annot_struct);
    }

    if (this->annot_win_struct.show_point) {
        showSphereAtPoint(call, this->annot_win_struct.annot_struct.coordinates);
    }
    ImGui::End();
}

/* Print the given vec3 to the console */
void AnnotationRenderer::print_coords(glm::vec3 coords) {
    printf("%lf\n", coords.x);
    printf("%lf\n", coords.y);
    printf("%lf\n", coords.z);
}

/* Draw a sphere at the coordinates given in the vec3 */
void AnnotationRenderer::showSphereAtPoint(CallRender3DGL& call, glm::vec3 coords) {
    // TODO: currently the sphere is ALWAYS in the front? (even when it SHOULD be behind other objects)
    core::view::Camera cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    auto cam_pose = cam.get<core::view::Camera::Pose>();

    auto& colptr = this->sphereColorSlot.Param<core::param::ColorParam>()->Value();

    glm::vec3 current = coords;
    glEnable(GL_DEPTH_TEST);
    /* This enables the use of a Renderer for the generated point at the given coordinates that will stay there no matter the direction of the camera */
    this->sphereShader->use();

    this->sphereShader->setUniform("mvp", mvp);
    this->sphereShader->setUniform("view", view);
    this->sphereShader->setUniform("proj", proj);
    this->sphereShader->setUniform("camRight", cam_pose.right.x, cam_pose.right.y, cam_pose.right.z);
    this->sphereShader->setUniform("camUp", cam_pose.up.x, cam_pose.up.y, cam_pose.up.z);
    this->sphereShader->setUniform("camPos", cam_pose.position.x, cam_pose.position.y, cam_pose.position.z);
    this->sphereShader->setUniform("camDir", cam_pose.direction.x, cam_pose.direction.y, cam_pose.direction.z);
    this->sphereShader->setUniform("scalingFactor", this->sizeScalingSlot.Param<core::param::FloatParam>()->Value());
    this->sphereShader->setUniform("color", colptr[0], colptr[1], colptr[2], colptr[3]);

    // Render a point at the given coordinates
    // TODO: use a different mode
    glBegin(GL_POINTS);
    glVertex3f(current[0], current[1], current[2]); 
    glEnd();


    // TODO: THIS NEEDS SOME WORK
    /*this->lineShader->use();

    glm::vec3 bbmin = current;
    glm::vec3 bbmax = glm::vec3(1.0f, 1.0f, 1.0f);

    this->lineShader->setUniform("mvp", mvp);
    this->lineShader->setUniform("bbMin", bbmin);
    this->lineShader->setUniform("bbMax", bbmax);
    this->lineShader->setUniform("color", colptr[0], colptr[1], colptr[2]);

    glBegin(GL_LINES);
    glVertex3f(current[0], current[1], current[2]);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glColor3f(colptr[0], colptr[1], colptr[2]);
    glEnd();*/
    glDisable(GL_DEPTH_TEST);
}

/* Writes the Names of the currently stored Points in the json_obj to a vector
Add a bool of true, if this function is called after loading a file, this will reset all stored window-bools */
void AnnotationRenderer::write_json_obj_data_to_vectors(CallRender3DGL& call, bool loaded_from_file) {
    if (loaded_from_file) {
        all_annotations.clear(); // when loading from a file then first clear the vector.
        this->pointWindowSizes.clear();
    }
        

    int iterate = 0;
    for (auto& x : this->json_obj["Points"].items()) {
        ++iterate;
        int i = std::stoi(x.key());
        // Generate a temp value for the coordinates array, removes clutter in later calls.
        auto temp = x.value()["Coordinates"];
        auto tempCamPos = x.value()["Camera Position"];
        auto tempCamOrient = x.value()["Camera Orientation"];

        // in case that json_obj holds more points then
        if (i >= all_annotations.size()) {
            this->all_annotations.push_back(
                {x.value()["Annotation"], glm::vec3(temp[0], temp[1], temp[2]), x.value()["Point Name"],
                    false, false, false, x.value()["Start Timestamp"],
                    x.value()["End Timestamp"], glm::vec3(tempCamPos[0], tempCamPos[1], tempCamPos[2]),
                    glm::quat(tempCamOrient[3], tempCamOrient[0], tempCamOrient[1], tempCamOrient[2]), false});
            // For saving the window Sizes
            this->pointWindowSizes.push_back(glm::vec2(0.0f, 0.0f));
        } else {
            this->all_annotations[i].annotation = x.value()["Annotation"];
            this->all_annotations[i].coordinates = glm::vec3(temp[0], temp[1], temp[2]);
            this->all_annotations[i].name = x.value()["Point Name"];
            this->all_annotations[i].show_window = false;
            this->all_annotations[i].show_point = false;
            this->all_annotations[i].aviable_at_current_time = false;
            this->all_annotations[i].start_ts = x.value()["Start Timestamp"];
            this->all_annotations[i].start_ts = x.value()["End Timestamp"];
            this->all_annotations[i].cam_pos = glm::vec3(tempCamPos[0], tempCamPos[1], tempCamPos[2]);
            this->all_annotations[i].cam_orientation =
                glm::quat(tempCamOrient[3], tempCamOrient[0], tempCamOrient[1], tempCamOrient[2]); // 3,0,1,2 because quat in megamol is x,y,z,w and glm::quat is w,x,y,z
            this->all_annotations[i].currently_editing = false;
            // For saving the window Sizes
            this->pointWindowSizes[i] = glm::vec2(0.0f, 0.0f);
        }
        // generates the window for one frame and then saves the size of it to the vector: pointWindowSizes[i]
        // display_visual_points_windows(call, this->all_annotations[i].name, i, this->all_annotations[i].coordinates, ImVec2(0.0f,0.0f), false, true);
    }
    for (int i = 0; i < pointWindowSizes.size(); ++i) {
        display_visual_points_windows(call, this->all_annotations[i].name, i, this->all_annotations[i].coordinates,
            glm::vec2(0.0f, 0.0f), false, true);
    }
    // Case that we have LESS points in json_obj then we have entries in all_annotations:
    // TODO: check if this works, need the remove from json_obj function for this.
    if (iterate < all_annotations.size()) {
        all_annotations.erase(std::next(all_annotations.begin(), iterate + 1), all_annotations.end()); 
    }
    occlusionQuery.query.clear();
    occlusionQuery.query.resize(2 * all_annotations.size());
    occlusionQuery.result.clear();
    occlusionQuery.result.resize(2 * all_annotations.size());
    occlusionQuery.resultAv.clear();
    occlusionQuery.resultAv.resize(2 * all_annotations.size());
    occlusionQuery.queryStarted.clear();
    occlusionQuery.queryStarted.resize(2 * all_annotations.size());
    glGenQueries(occlusionQuery.query.size(), occlusionQuery.query.data());
}

/* Displays an ImGui Window for the selected Point from the Combo of display_json_window()
It shows the values as they are currently in the individual global vectors
On a Button press you can update the values of the current point in the json file */
void AnnotationRenderer::display_window_of_selected_json_point(CallRender3DGL& call, std::string windowName, bool& window_open, int curr_index) {
    // check if the all_annotations vector is empty and if so then exit
    if (this->all_annotations.empty())
        return;
    // TODO: Check for the case that selectedIndex was NOT updated after json_point_names WAS updated => index out of bounds etc!

    ImGui::Begin(windowName.c_str(), &window_open);
    ImGui::Text("Annotation");
    ImGui::InputText("Annotation", &this->all_annotations[curr_index].annotation);
    ImGui::Text(this->all_annotations[curr_index].annotation.c_str());
    ImGui::InputFloat3("Test input", (float*)&this->all_annotations[curr_index].coordinates);

    showSphereAtPoint(call, this->all_annotations[curr_index].coordinates);
    // ImGui::Button
    if (ImGui::Button("Update the json_obj with current values"))
        updateAnnotationInJsonObj(call, curr_index);

    // load camera Position:
    if (ImGui::Button("Load camera position"))
        loadCameraPosition(
            call, this->all_annotations[curr_index].cam_pos, this->all_annotations[curr_index].cam_orientation);
    
    ImGui::End();
}

// TODO: does this cause problems if the current project is NOT loaded BUT thrown together in the editor?
/* TODO : Can it come to problems when a project file is loaded and then mmClearGraph() is called and a new project file is called?
=> also what if instead of loading a new Project a project gets thrown together? (I think in this case it OVERWRITES the Json of the old first loaded Project)... */ 
/*
 * Returns the file Path to the json file.
 * This file will be in the same folder as the given project file.
 */
std::string AnnotationRenderer::determineJsonFilePath(void) const {
    std::string path;

    const auto& paths = frontend_resources.get<megamol::frontend_resources::ScriptPaths>().lua_script_paths;
    if (!paths.empty()) {
        path = paths[0];
    } else {
        return path;
    }

    const auto dotpos = path.find_last_of('.');
    path = path.substr(0, dotpos);
    path.append("_annotation.json");
    return path;
}

/* Takes x,y as Screenspace coordinates and calculates the corresponding worldspace coordinates.
 * This is done by using the inverse of the view and projection matrix.
 * The resulting worldspace coordinates are then used to calculate the corresponding coordinates in the volume.
 * The resulting coordinates are then used to calculate the corresponding index in the volume.
 * The value at this index is then returned.
 */
glm::vec3 AnnotationRenderer::calcClickedPoint(int x, int y, CallRender3DGL& call) {
    // TODO: (what to do if there are holes in the data?)
    // IMPORTANT: the input x,y are counted from the TOP left corner of the screen, NOT from the bottom left corner!

    auto const lhsFBO = call.GetFramebuffer();
    // flip the y coordinates with getHeight from the Framebuffer
    y = lhsFBO->getHeight() - y;
    int dataSize = 1 * 1;
    float* data = new float[dataSize];
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, data); // change 1,1 if bigger area is needed
    float depth = data[0];
    delete[] data;

    float nDepth = 2 * depth - 1;
    
    return getWorldCoordsFromScreenPos(call, x, y, nDepth, false);
}

/* Save Slot Values to JSON
 * This function is called when the save button is pressed.
 * It saves the current values of the slots to the json file.
 */
void AnnotationRenderer::save_slot_values_to_json() {
    this->json_obj["SlotValues"]["linesColor"] = this->linesColorSlot.Param<core::param::ColorParam>()->Value();
    this->json_obj["SlotValues"]["sphereColor"] = this->sphereColorSlot.Param<core::param::ColorParam>()->Value();
    this->json_obj["SlotValues"]["sphereSizeScaling"] = this->sizeScalingSlot.Param<core::param::FloatParam>()->Value();
    this->json_obj["SlotValues"]["textColor"] = this->textColorSlot.Param<core::param::ColorParam>()->Value();
    this->json_obj["SlotValues"]["titleColor"] = this->titleColorSlot.Param<core::param::ColorParam>()->Value();
    this->json_obj["SlotValues"]["wrapWidth"] = this->wrapWidthSlot.Param<core::param::FloatParam>()->Value();
}

/* Load the Slot Values from the JSON
 */
void AnnotationRenderer::load_slot_values_from_json() {
    this->linesColorSlot.Param<core::param::ColorParam>()->SetValue(this->json_obj["SlotValues"]["linesColor"]);
    this->sphereColorSlot.Param<core::param::ColorParam>()->SetValue(this->json_obj["SlotValues"]["sphereColor"]);
    this->sizeScalingSlot.Param<core::param::FloatParam>()->SetValue(this->json_obj["SlotValues"]["sphereSizeScaling"]);
    this->titleColorSlot.Param<core::param::ColorParam>()->SetValue(this->json_obj["SlotValues"]["titleColor"]);
    this->textColorSlot.Param<core::param::ColorParam>()->SetValue(this->json_obj["SlotValues"]["textColor"]);
    this->wrapWidthSlot.Param<core::param::FloatParam>()->SetValue(this->json_obj["SlotValues"]["wrapWidth"]);
}

/* Set the Camera to the given Coordinates
*/
void AnnotationRenderer::loadCameraPosition(CallRender3DGL& call, glm::vec3 inputCamPos, glm::quat inputCamOrient) {
    auto thingy = const_cast<frontend_resources::common_types::lua_func_type*>(&frontend_resources.get<frontend_resources::common_types::lua_func_type>());
    std::string camPosString = "mmSetParamValue(\"::view::cam::position\",[=[" + std::to_string(inputCamPos.x) + std::string(";") +
                    std::to_string(inputCamPos.y) + std::string(";") + std::to_string(inputCamPos.z) +
                    std::string("]=])");
    std::string camOrientString = "mmSetParamValue(\"::view::cam::orientation\",[=[" + std::to_string(inputCamOrient[0]) + std::string(";") +
        std::to_string(inputCamOrient[1]) + std::string(";") + std::to_string(inputCamOrient[2]) + std::string(";") +
        std::to_string(inputCamOrient[3]) + std::string("]=])");
    // TODO: This version ONLY works for the "test" project file, because others have different path names...
    (*thingy)(camPosString);
    (*thingy)(camOrientString);
    std::cout << camPosString << std::endl;
    std::cout << camOrientString << std::endl;
    print_coords(inputCamPos);
}

/*Determines which points have to be drawn and which not.
 * It does this by checking if the current frame is in the time span of the point.
 * Afterwards it also checks if the point is in the foreground or background.
*/
void AnnotationRenderer::determine_points_to_be_shown(CallRender3DGL& call) {
    if (this->all_annotations.size() == 0) {
        return;
    }
    float currentTimeStamp = call.Time();
    for (int i = 0; i < all_annotations.size(); ++i) {
        // TODO: check if this works so now with the Edge case...
        // This is a long if because it has the two cases: start_ts <= end_ts and start_ts > end_ts and each case needs a different check
        // for if the current time is in the time span
        if ((all_annotations[i].start_ts <= all_annotations[i].end_ts &&
            currentTimeStamp >= all_annotations[i].start_ts && currentTimeStamp <= all_annotations[i].end_ts)
            ||
            (all_annotations[i].end_ts < all_annotations[i].start_ts &&
            (currentTimeStamp >= all_annotations[i].start_ts || currentTimeStamp <= all_annotations[i].end_ts)))
        {
            showSphereAtPointIndex(call, this->all_annotations[i].coordinates, i);
            this->all_annotations[i].aviable_at_current_time = true;

            int t = (frameType + 1) % 2;
            if (occlusionQuery.queryStarted[2 * i + t]) {
                glGetQueryObjectuiv(
                    occlusionQuery.query[2 * i + t], GL_QUERY_RESULT_AVAILABLE, &occlusionQuery.resultAv[2 * i + t]);
                if (occlusionQuery.resultAv[2 * i + t] == GL_TRUE) {
                    glGetQueryObjectuiv(
                        occlusionQuery.query[2 * i + t], GL_QUERY_RESULT, &occlusionQuery.result[2 * i + t]);
                    if (occlusionQuery.result[2 * i + t] == GL_TRUE) {
                        this->all_annotations[i].show_point = true;
                    } else {
                        this->all_annotations[i].show_point = false;
                    }
                }
            }
        } else {
            this->all_annotations[i].show_point = false;
            this->all_annotations[i].aviable_at_current_time = false;
        }
    }
    forceDirectedLayout(call);
}

/*Converts the given 3D world coordinates into 2D screen coordinates.
 * This is needed to draw the text at the correct position.
 */
glm::vec2 AnnotationRenderer::getScreenPosFromWorldCoords(CallRender3DGL& call, glm::vec3 input_coords) {
    auto const lhsFBO = call.GetFramebuffer();
    core::view::Camera cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    glm::vec4 screenCoords = mvp * glm::vec4(input_coords, 1.0f);
    
    screenCoords.x /= screenCoords.w;
    screenCoords.y /= screenCoords.w;
    screenCoords.x = lhsFBO->getWidth() * (screenCoords.x + 1.0f) / 2.0f;
    screenCoords.y = lhsFBO->getHeight() * (1.0f - (screenCoords.y + 1.0f) / 2.0f);
    
    return glm::vec2(screenCoords.x, screenCoords.y);    
}

/* Converts Screen Position with given z Value into Worldspace coordinates.
 * z Value is already in World Space coordinates form.
 * x, y Values are the Screen Position in Screen Space coordinates form.
 * flipY is needed because the y Position needs to be flipped
 * Set it to true if this is still needed and to false if previous operation already did the flip
 */
glm::vec3 AnnotationRenderer::getWorldCoordsFromScreenPos(CallRender3DGL& call, int x, int y, float z, bool flipY) {
    auto const lhsFBO = call.GetFramebuffer();
    // flip the y coordinates with getHeight from the Framebuffer
    if (flipY) {
        y = lhsFBO->getHeight() - y;
    }

    auto cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    auto invMVP = glm::inverse(mvp);

    float nx = (2 * ((float)x / lhsFBO->getWidth())) - 1; //Einheitswürfel
    float ny = (2 * ((float)y / lhsFBO->getHeight())) - 1;

    glm::vec4 h = invMVP * glm::vec4(nx, ny, z, 1.0f);
    glm::vec3 result = glm::vec3(h) / h.w;
    return result;
}

/* Displays an ImGui Window at the position the corresponding point is located.
 * It shows the name of the point and its Annotation.
 * It is fixed in place.
 * It is only shown when the corresponding point is visible in the current frame.
 */
void AnnotationRenderer::display_visual_points_windows(
    CallRender3DGL& call, std::string windowName, int curr_index, glm::vec3 point_pos, glm::vec2 offset, bool drawLine, bool saveSize) {
    // check if the all_annotations vector is empty and if so then exit
    if (this->all_annotations.empty())
        return;
    // check for index out of bounds
    if (this->all_annotations.size() <= curr_index)
        return;

    glm::vec2 screenPos = getScreenPosFromWorldCoords(call, point_pos);
    screenPos = screenPos + glm::vec2(offset.x, offset.y); // add offset to the screen position to prevent overlapping windows

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoBackground;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    bool* p_open = NULL;
    std::string windowNameString = windowName + std::string("##") + std::to_string(curr_index); // Needed to differenciate between this window and the other windows

    float wrap_width = this->wrapWidthSlot.Param<core::param::FloatParam>()->Value();
    
    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y), 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetFontSize() * wrap_width, -1.0f));

    megamol::core::param::ColorParam::ColorType textColorIn = this->textColorSlot.Param<core::param::ColorParam>()->Value();
    ImVec4 textColor = ImVec4(textColorIn[0], textColorIn[1], textColorIn[2], textColorIn[3]);

    megamol::core::param::ColorParam::ColorType titleColorIn =
        this->titleColorSlot.Param<core::param::ColorParam>()->Value();
    ImVec4 titleColor = ImVec4(titleColorIn[0], titleColorIn[1], titleColorIn[2], titleColorIn[3]);
    
    ImGui::Begin(windowNameString.c_str(), p_open, window_flags);
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * wrap_width); // TODO: change 15.0f out with: wrap_width
    ImGui::TextColored(titleColor, windowName.c_str());
    ImGui::TextColored(textColor, this->all_annotations[curr_index].annotation.c_str());
    ImGui::PopTextWrapPos();

    // save the current window size
    ImVec2 winSize = ImGui::GetWindowSize();
    if (saveSize) {
        pointWindowSizes[curr_index] = glm::vec2(winSize.x, winSize.y);
    }
    
    // Drawing connecting line between Point and the center of the window:
    if (drawLine) {
        ImVec2 currWinPos = ImGui::GetWindowPos();
        currWinPos.x = currWinPos.x + 0.5f * ImGui::GetWindowWidth();
        currWinPos.y = currWinPos.y + 0.5f * ImGui::GetWindowHeight();
        drawConnectionLine(call, glm::vec2(currWinPos.x, currWinPos.y), point_pos);
    }
    ImGui::End();
}


/* Draw a sphere at the coordinates given in the vec3
* Additionally also start a glQuery of GL_ANY_SAMPLES_PASSED for these coordinates.
* This is used to check if the drawn point is visible or not.
*/
void AnnotationRenderer::showSphereAtPointIndex(CallRender3DGL& call, glm::vec3 coords, int index) {
    // TODO: currently the sphere is ALWAYS in the front? (even when it SHOULD be behind other objects)
    core::view::Camera cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    auto cam_pose = cam.get<core::view::Camera::Pose>();

    auto& colptr = this->sphereColorSlot.Param<core::param::ColorParam>()->Value();

    glm::vec3 current = coords;
    glEnable(GL_DEPTH_TEST);
    /* This enables the use of a Renderer for the generated point at the given coordinates that will stay there no matter the direction of the camera */
    this->sphereShader->use();

    this->sphereShader->setUniform("mvp", mvp);
    this->sphereShader->setUniform("view", view);
    this->sphereShader->setUniform("proj", proj);
    this->sphereShader->setUniform("camRight", cam_pose.right.x, cam_pose.right.y, cam_pose.right.z);
    this->sphereShader->setUniform("camUp", cam_pose.up.x, cam_pose.up.y, cam_pose.up.z);
    this->sphereShader->setUniform("camPos", cam_pose.position.x, cam_pose.position.y, cam_pose.position.z);
    this->sphereShader->setUniform("camDir", cam_pose.direction.x, cam_pose.direction.y, cam_pose.direction.z);
    this->sphereShader->setUniform("scalingFactor", this->sizeScalingSlot.Param<core::param::FloatParam>()->Value());
    this->sphereShader->setUniform("color", colptr[0], colptr[1], colptr[2], colptr[3]);

    // Render a point at the given coordinates
    // TODO: use a different mode
    glBeginQuery(GL_ANY_SAMPLES_PASSED, this->occlusionQuery.query[2 * index + frameType]);
    glBegin(GL_POINTS);
    glVertex3f(current[0], current[1], current[2]);
    glEnd();
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    occlusionQuery.queryStarted[2 * index + frameType] = true;
    glDisable(GL_DEPTH_TEST);
}

/* ImGui Window that holds all annotations.
 * This includes a list of all annotations  TODO: (and a text field to add new annotations.)
 * TODO: Also a button to delete the selected annotation.
 * A timeline next to each annotation that shows when the annotation is visible timewise
 * Different colors for the names depending on if the annotation is visible in time and space.
 */
void AnnotationRenderer::list_Window(CallRender3DGL& call) {
    // Using those as a base value to create width/height that are factor of the size of our font
    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    bool* p_open = NULL;
    ImGui::Begin("Annotation List", p_open);
    // list all annotation names
    //ImGui::BeginTabBar("#Lists");
    //for (int i = 0; i < this->all_annotations.size(); i++) {

    //}
    // Taken From IMGUI DEMO:
    // Just for seeing the code right now
    static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                   ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                   ImGuiTableFlags_NoBordersInBody;
    static ImGuiTableFlags flags_Slider = 0;


    if (ImGui::BeginTable("3ways", 4, flags)) {
        // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
        ImGui::TableSetupColumn("Timeline", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
        ImGui::TableSetupColumn("Start Time", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
        ImGui::TableHeadersRow();

        // TODO: Allow sorting of entries!

        // This is a line for the Explanations of the different columns
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Tooltips:");
        ImGui::TableNextColumn();
        ImGui::SmallButton("Visibility");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            //std::string tooltipText = "There are three possible colors and names for the visibility of an Annotation.\n\n"
            //                          "Visible - Green: This Annotation is visible on screen right now.\n"
            //                          "Obscurred - Yellow: This Annotation is currently behind Objects in the scene.\n"
            //                          "Hidden - Red: This Annotation is currently not visible in any way."; // TODO: maybe change these lines a bit...
            //ImGui::TextUnformatted(tooltipText.c_str());
            ImGui::TextUnformatted("There are three possible colors and names for the visibility of an Annotation.");
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Visible");
            ImGui::SameLine();
            ImGui::Text("This Annotation is visbile on screen right now.");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Obscurred");
            ImGui::SameLine();
            ImGui::Text("This Annotation is currently behind Objects in the scene.");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Hidden");
            ImGui::SameLine();
            ImGui::Text("This Annotation is currently not visible in any way.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        } // TODO: ADD COLORBLIND MODE!!!!

        ImGui::TableNextColumn();
        int amountLines = 100.0f;
        float currentTime = call.Time();
        float timeSpan = this->totalFrameCount / (float)amountLines;
        int currentTimeL = currentTime / timeSpan;
        float arr[100];
        for (int i = 0; i < amountLines; i++) {
            if (i == (int)currentTimeL) {
                arr[i] = 1.0f;
            } else {
                arr[i] = 0.0f;
            }
        }

        ImGui::PlotLines("", arr, IM_ARRAYSIZE(arr));
        
        // loop over all annotations in all_annotations
        // entries will be: name, timeline
        // collapsed for each entry: annotation text, change annotation, jump to annotation
        for (int i = 0; i < this->all_annotations.size(); i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            std::string treeNodeName = this->all_annotations[i].name + "##" + std::to_string(i);
            bool open = ImGui::TreeNodeEx(treeNodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
            
            ImGui::TableNextColumn();
            std::string text1 = "";
            ImVec4 color1 = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            if (this->all_annotations[i].show_point) {
                text1 = "Visible";
                color1 = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // green
            } else if (this->all_annotations[i].aviable_at_current_time) {
                text1 = "Obscurred";
                color1 = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // yellow
            } else {
                text1 = "Hidden"; // TODO: better word... it is after all not just hidden, but also just not in the current timeframe...
                color1 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // red
            }
            ImGui::TextColored(color1, text1.c_str());
            
            ImGui::TableNextColumn();
            // ImGui::TextDisabled("--");
            // std::string tempText = createTimelineArt(call, this->all_annotations[i].start_ts, this->all_annotations[i].end_ts);
            // ImGui::Text(tempText.c_str());

            float startTime = this->all_annotations[i].start_ts;
            float endTime = this->all_annotations[i].end_ts;

            float startLine = startTime / timeSpan;
            float endLine = endTime / timeSpan;

            float xs5[100];
            for (int k = 0; k < amountLines; k++) {
                if (startTime <= endTime) {
                    if (k >= startLine && k <= endLine) {
                        xs5[k] = 1.0f;
                    } else {
                        xs5[k] = 0.0f;
                    }
                } else if (startTime > endTime) {
                    if (k >= endLine && k <= startLine) {
                        xs5[k] = 1.0f;
                    } else {
                        xs5[k] = 0.0f;
                    }
                }
            }
            ImGui::PlotLines("", xs5, IM_ARRAYSIZE(xs5));
            
            ImGui::TableNextColumn();
            // show start time, for sorting
            // TODO: allow sorting in the table
            ImGui::Text(std::to_string(this->all_annotations[i].start_ts).c_str());
            if (open) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped(this->all_annotations[i].annotation.c_str());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button("Load Camera Position")) {
                    loadCameraPosition(
                        call, this->all_annotations[i].cam_pos, this->all_annotations[i].cam_orientation);
                }
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button("Load Time Position")) {
                    auto thingy = const_cast<frontend_resources::common_types::lua_func_type*>(
                        &frontend_resources.get<frontend_resources::common_types::lua_func_type>());
                    std::string tttt = " mmSetParamValue(\"::view::anim::time\", [=[" +
                                       std::to_string(this->all_annotations[i].start_ts) + "]=])";
                    (*thingy)(tttt);
                    // TODO: Set Time does not work this way?
                }
                std::string open_string = "Open Change Options##" + std::to_string(i);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                //bool open_changes = ImGui::TreeNodeEx(open_string.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                //if (open_changes) {
                //    ImGui::TableNextRow();
                //    ImGui::TableNextColumn();
                //    ImGui::Text("Change Annotation");
                //    ImGui::TableNextColumn();
                //    ImGui::InputText("Annotation", &this->all_annotations[i].annotation);
                //    //ImGui::Text(this->all_annotations[i].annotation.c_str());
                //    ImGui::TableNextRow();
                //    ImGui::TableNextColumn();
                //    ImGui::Text("Change Coordinate of Point");
                //    ImGui::TableNextColumn();
                //    // Button for starting the picking process
                //    if (ImGui::Button("Click in the viewport to add a new point##2")) {
                //        this->picking_enabled = true;
                //    }

                //    if (this->picking_enabled) {
                //    ImGui::Text("Click in the viewport to add a new point");
                //    // Wait till the user has clicked in the Window and then calculate the coordinates from this point.
                //    // this uses the variables lastX and lastY that are updated everytime the mouse is moved.
                //    if (this->picked_a_point) {
                //        glm::vec3 picked_point = calcClickedPoint(this->lastX, this->lastY, call);
                //        // are these texts even needed? because they will just vanish after 1 frame
                //        ImGui::Text("Picked a point!");
                //        ImGui::Text("x: %f, y: %f, z: %f", picked_point.x, picked_point.y, picked_point.z);
                //        this->annot_win_struct.annot_struct.coordinates = picked_point;
                //        // this conversion is needed to show the coordinates in the ImGui window
                //        this->all_annotations[i].coordinates = picked_point;
                //        this->picked_a_point = false;
                //        this->picking_enabled = false;
                //        }
                //    }
                //    
                //    if (ImGui::Button("Update the json_obj with current values")) {
                //        updateAnnotationInJsonObj(call, i); // TODO: add struct as import => can change as wanted
                //    }
                //    ImGui::TreePop();
                //}

                if (ImGui::Button("Edit this Annotation")) {
                    this->all_annotations[i].currently_editing = true;
                }
                if (this->all_annotations[i].currently_editing) {
                    editing_Annotations_Window(call, i);
                }
                    

                ImGui::TreePop();
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}


/* Function that implements a sort of Spring Embedder for the Shown Annotations on screen to prevent overlapping. */
void AnnotationRenderer::forceDirectedLayout(CallRender3DGL& call) {
    // Idea: Place first window as normal.
    // Then, for every other window, check if it overlaps with the first window.
    // If it does, move it to the right.
    // Then, check if it overlaps with any other window.
    // If it does, move it to the right.
    // Repeat until no overlap is detected.
    // Then, place the next window as normal.
    // Repeat until all windows are placed.
    struct tempStruct {
        int indexAllAnnot;        // index of the annotation in the all_annotations vector
        glm::vec2 screenPosition; // screen position of the annotation (middle point)
        glm::vec2 windowSize;     // window size of the ImGui window
        glm::vec2 offset;         // offset for the current loop
        glm::vec2 totalOffset;    // offset that is sum of all offets
    };

    std::vector<tempStruct> tempStructVector;

    for (int i = 0; i < this->all_annotations.size(); i++) {
        if (this->all_annotations[i].show_point) {
            glm::vec2 currentPointScreenPos = getScreenPosFromWorldCoords(call, this->all_annotations[i].coordinates);
            glm::vec2 currentPointScreenSize = pointWindowSizes[i];
            tempStructVector.push_back(tempStruct{
                i, currentPointScreenPos, currentPointScreenSize, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f)});
        }
    }

    // iterate over the triangle of all Windows that need to be placed...
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < tempStructVector.size(); i++) {
            for (int j = i + 1; j < tempStructVector.size(); j++) {
                float sumWidth = (tempStructVector[i].windowSize.x + tempStructVector[j].windowSize.x) * 0.5f;
                float sumHeight = (tempStructVector[i].windowSize.y + tempStructVector[j].windowSize.y) * 0.5f;
                float differenceX = tempStructVector[i].screenPosition.x - tempStructVector[j].screenPosition.x;
                float differenceY = tempStructVector[i].screenPosition.y - tempStructVector[j].screenPosition.y;

                if (std::abs(differenceX) <= sumWidth && std::abs(differenceY) <= sumHeight) {
                    // Calculate left and right points of the rectangles
                    glm::vec2 l1 = tempStructVector[i].screenPosition - (tempStructVector[i].windowSize * 0.5f);
                    glm::vec2 r1 = tempStructVector[i].screenPosition + (tempStructVector[i].windowSize * 0.5f);
                    glm::vec2 l2 = tempStructVector[j].screenPosition - (tempStructVector[j].windowSize * 0.5f);
                    glm::vec2 r2 = tempStructVector[j].screenPosition + (tempStructVector[j].windowSize * 0.5f);

                    // Calculate Offsets
                    float x_dist = std::min(r1.x, r2.x) - std::max(l1.x, l2.x);
                    float y_dist = (std::min(r1.y, r2.y) - std::max(l1.y, l2.y));

                    if (differenceX >= 0.0f && differenceY >= 0.0f) { // i is on the right and up?
                        tempStructVector[i].offset += glm::vec2(x_dist * 0.5f, y_dist * 0.5f);
                        tempStructVector[j].offset += glm::vec2(x_dist * -0.5f, y_dist * -0.5f);
                    } else if (differenceX < 0.0f && differenceY >= 0.0f) { // i is on the left and up?
                        tempStructVector[i].offset += glm::vec2(x_dist * -0.5f, y_dist * 0.5f);
                        tempStructVector[j].offset += glm::vec2(x_dist * 0.5f, y_dist * -0.5f);
                    } else if (differenceX >= 0.0f && differenceY >= 0.0f) { // i is on the right and down?
                        tempStructVector[i].offset += glm::vec2(x_dist * 0.5f, y_dist * -0.5f);
                        tempStructVector[j].offset += glm::vec2(x_dist * -0.5f, y_dist * 0.5f);
                    } else { // i is on the left and down?
                        tempStructVector[i].offset += glm::vec2(x_dist * -0.5f, y_dist * -0.5f);
                        tempStructVector[j].offset += glm::vec2(x_dist * 0.5f, y_dist * 0.5f);
                    }
                }
            }
        }
        for (int i = 0; i < tempStructVector.size(); i++) {
            tempStructVector[i].screenPosition += tempStructVector[i].offset;
            tempStructVector[i].totalOffset += tempStructVector[i].offset;
            tempStructVector[i].offset = glm::vec2(0.0f, 0.0f);
        }
    }
    for (int i = 0; i < tempStructVector.size(); i++) {
        int index = tempStructVector[i].indexAllAnnot;
        display_visual_points_windows(call, this->all_annotations[index].name, index, this->all_annotations[index].coordinates, tempStructVector[i].totalOffset, true, true);
    }
}

/* Function that draws a connection line between an ImGui window and the given coordinates in 3D. */
void AnnotationRenderer::drawConnectionLine(CallRender3DGL& call, glm::vec2 windowPos, glm::vec3 worldPos) {
    // TODO: add variable for changing the line color?
    auto& colptr = this->linesColorSlot.Param<core::param::ColorParam>()->Value();
    glm::vec3 lineColor = glm::vec3(1.0f, 1.0f, 1.0f);
    auto cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    float z = -1.0f;

    /*Line too short when using NearPlane as z.
    When using z = 0.0f then the line IS drawn to the correct location AND has a good length
    BUT then it can be obscurred by other objects in the scene, that should not be in front of the line.
    BUT with z = -1.0f it somehow works... (at least for data sets, that are inside of the "Einheitswürfel")*/

    // convert windowPos to world Space coordinates
    glm::vec3 convertedScreenPos = getWorldCoordsFromScreenPos(call, windowPos.x, windowPos.y, z, true);

    this->lineShader->use();
    this->lineShader->setUniform("mvp", mvp);
    this->lineShader->setUniform("color", colptr[0], colptr[1], colptr[2], colptr[3]);

    // draw line
    glEnable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glVertex3f(convertedScreenPos[0], convertedScreenPos[1], convertedScreenPos[2]);
    glVertex3f(worldPos[0], worldPos[1], worldPos[2]);
    glEnd();
    glDisable(GL_DEPTH_TEST);
}

void AnnotationRenderer::editing_Annotations_Window(CallRender3DGL& call, int index) {
    ImGuiWindowFlags window_flags = 0;
    
    std::string windowNameString =
        "Editing Annotation: " + this->all_annotations[index].name + std::string("##") +
        std::to_string(index); // Needed to differenciate between this window and the other windows (does not work)
    float wrap_width = this->wrapWidthSlot.Param<core::param::FloatParam>()->Value();
    
    ImGui::Begin(windowNameString.c_str(), &this->all_annotations[index].currently_editing , window_flags);
    ImGui::Text("Change Annotation");
    ImGui::InputText("Annotation", &this->all_annotations[index].annotation);
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * wrap_width); 
    ImGui::TextUnformatted(
        this->all_annotations[index].annotation.c_str()); // This allows the user to see the whole annotation text
    ImGui::PopTextWrapPos();
    ImGui::Text("Change Coordinate of Point");
    
    // Button for starting the picking process
    if (ImGui::Button("Click in the viewport##2")) { // TODO: change description of this Button?
        this->picking_enabled = true;
    }

    if (this->picking_enabled) {
        ImGui::Text("Click in the viewport to add a new point");
        // Wait till the user has clicked in the Window and then calculate the coordinates from this point.
        // this uses the variables lastX and lastY that are updated everytime the mouse is moved.
        if (this->picked_a_point) {
            glm::vec3 picked_point = calcClickedPoint(this->lastX, this->lastY, call);
            // are these texts even needed? because they will just vanish after 1 frame
            ImGui::Text("Picked a point!");
            ImGui::Text("x: %f, y: %f, z: %f", picked_point.x, picked_point.y, picked_point.z);
            this->annot_win_struct.annot_struct.coordinates = picked_point;
            // this conversion is needed to show the coordinates in the ImGui window
            this->all_annotations[index].coordinates = picked_point;
            this->picked_a_point = false;
            this->picking_enabled = false;
        }
    }

    // Button for saving the current camera position
    if (ImGui::Button("Update camera to current camera position")) { // TODO: better wording?
        this->all_annotations[index].cam_pos = call.GetCamera().getPose().position;
        this->all_annotations[index].cam_orientation = call.GetCamera().getPose().to_quat();
    }

    if (ImGui::Button("Update the json_obj with current values")) { // TODO: this is USELESS!!!! because any changes are already being done the moment they happen.
        updateAnnotationInJsonObj(call, index); // TODO: add struct as import => can change as wanted
    }
    ImVec2 currWinPos = ImGui::GetWindowPos();
    drawConnectionLine(call, glm::vec2(currWinPos.x, currWinPos.y), this->all_annotations[index].coordinates);
    ImGui::End();
}


/* Load the Json file and then call another function to save the data to all_annotations vector.
This function preserves all previously aviable data. */
void AnnotationRenderer::loadJsonFromFileToVectors(CallRender3DGL& call) {
    // TODO: change the path to the path of the json file
    std::string file_path = determineJsonFilePath();
    if (file_path.empty()) {
        // this is just a warning message on the console that is not nessecerily something for the "regular" user
        std::cout << "There is no file to be loaded" << std::endl;
        return;
    }
    std::ifstream i(file_path);
    nlohmann::json tempJson;
    i >> tempJson;
    // TODO: this line is ONLY for debugging...
    if (IsDebuggerPresent)
        std::cout << std::setw(4) << tempJson << std::endl;

    for (auto& element : tempJson["Points"].items()) {
        this->json_obj["Points"].push_back(element.value());
    }
    this->json_obj["SlotValues"].clear();
    for (auto& element : tempJson["SlotValues"].items()) {
        this->json_obj["SlotValues"][element.key()] = element.value();
    }

    // now write the names to the and bools to the vector
    write_json_obj_data_to_vectors(call, true);
}

/* Updates the json_obj for the annotation at index i, with the current values of this annotation in the vector all_annotations. */
void AnnotationRenderer::updateAnnotationInJsonObj(CallRender3DGL& call, int i) {
    // Needs at least one Annotation to be able to save
    annotation_struct input = this->all_annotations[i];
    json_obj["Points"][i] = {{"Coordinates", {input.coordinates.x, input.coordinates.y, input.coordinates.z}},
        {"Annotation", input.annotation}, {"Point Name", input.name}, {"Start Timestamp", input.start_ts},
        {"End Timestamp", input.end_ts}, {"Camera Position", {input.cam_pos.x, input.cam_pos.y, input.cam_pos.z}},
        {"Camera Orientation",
            {input.cam_orientation.x, input.cam_orientation.y, input.cam_orientation.z, input.cam_orientation.w}}};
}

/* Saves the current state of the variable "json_obj" into the json file for the currently used project.
 */
void AnnotationRenderer::save_json_to_file() {
    std::ofstream o(determineJsonFilePath());
    o << std::setw(4) << this->json_obj << std::endl;
}

/* Saves the given struct to the Vector all_annotations
This function is only to be called after this new */
void AnnotationRenderer::saveNewPoint(CallRender3DGL& call, annotation_struct input) {

    this->all_annotations.push_back(input);
    this->pointWindowSizes.push_back(glm::vec2(0.0f, 0.0f));

    json_obj["Points"][this->all_annotations.size() - 1] = {
        {"Coordinates", {input.coordinates.x, input.coordinates.y, input.coordinates.z}},
        {"Annotation", input.annotation}, {"Point Name", input.name}, {"Start Timestamp", input.start_ts},
        {"End Timestamp", input.end_ts}, {"Camera Position", {input.cam_pos.x, input.cam_pos.y, input.cam_pos.z}},
        {"Camera Orientation",
            {input.cam_orientation.x, input.cam_orientation.y, input.cam_orientation.z, input.cam_orientation.w}}};

    // Add entries to occlusionQuery and oqResults vectors:
    occlusionQuery.query.resize(occlusionQuery.query.size() + 2);
    occlusionQuery.result.resize(occlusionQuery.result.size() + 2);
    occlusionQuery.resultAv.resize(occlusionQuery.resultAv.size() + 2);
    occlusionQuery.queryStarted.resize(occlusionQuery.queryStarted.size() + 2);
    // glDeleteQueries(occlusionQuery.size(), occlusionQuery.data());
    glGenQueries(occlusionQuery.query.size(),
        occlusionQuery.query.data() + occlusionQuery.query.size() - 2); // TODO: does this still work?
}
