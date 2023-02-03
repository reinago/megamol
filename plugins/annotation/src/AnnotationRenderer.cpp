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

/* Needed for Json */
#include <nlohmann/json.hpp>
#include <fstream>


#include "mmcore/CoreInstance.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/utility/log/Log.h"
#include "mmcore_gl/utility/ShaderFactory.h"

#include "mmcore/utility/Picking.h"

#include "imgui.h"
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
        , linesColorSlot("linesColor", "Color of the lines")
        , vbo(0)
        , ibo(0)
        , va(0)
        , boundingBoxes()
        , picking_buffer()
        , my_color()
        , first_win_coordinates_input()
        , first_win_color_input()
        , first_win_color()
        , tryOut(false)
        , anotherWindow(false)
        , second_win_coordinates_input()
        , second_win_annotation_input()
        , second_win_color_input()
        , second_win_color()
        , second_win_point_name_input()
        , show_second_win_point(false)
        , warning_popup_bool(false)
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

    this->json_obj["Points"];
}

/*
 * AnnotationRenderer::~AnnotationRenderer
 */
AnnotationRenderer::~AnnotationRenderer() {
    this->Release();
}

//bool PickingBuffer::ProcessMouseClick(megamol::core::view::MouseButton button,
//    megamol::core::view::MouseButtonAction action, megamol::core::view::Modifiers mods) {
//
//    RendererModule::OnMouseButton(button, action, mods);
//    this->picking_buffer.ProcessMouseClick(button, action, mods);
//    return false;
//}

bool AnnotationRenderer::OnMouseButton(megamol::core::view::MouseButton button,
    megamol::core::view::MouseButtonAction action, megamol::core::view::Modifiers mods) {

    RendererModule::OnMouseButton(button, action, mods);
    this->picking_buffer.ProcessMouseClick(button, action, mods);
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
    if (this->enableAnnotationRendererSlot.Param<core::param::BoolParam>()->Value()) {
        test(call);
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    *chainedCall = call;
    renderRes &= (*chainedCall)(core::view::AbstractCallRender::FnRender);

    lhsFBO->bind();
    glViewport(0, 0, lhsFBO->getWidth(), lhsFBO->getHeight());

    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return renderRes;
}

void AnnotationRenderer::test(CallRender3DGL &call) {
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    // Creates a Window with the slider
    ImGui::Begin("Basic Functions");

    ImGui::InputFloat3("input coordinates", this->first_win_coordinates_input);
    glm::vec3 first_win_coordinates =
        glm::vec3(first_win_coordinates_input[0], first_win_coordinates_input[1], first_win_coordinates_input[2]);

    /* TODO: COLOR does NOT yet work! */
    // ImGui::InputFloat3("input Color", this->first_win_color_input);

    if (ImGui::Button("print current coordinates to console"))
        print_coords(first_win_coordinates);

    if (ImGui::Button("Toggle Sphere")) {
        if (this->tryOut) {
            this->tryOut = false;
        } else {
            this->tryOut = true;
        }
    }

    if (ImGui::Button("Show Create new Annotations Window")) {
        this->anotherWindow = true;
    }

    if (ImGui::Button("Show Json Window")) {
        this->show_json_window = true;
    }

    // Save the current state of the json_obj to a json file
    if (ImGui::Button("Save annotations to Json File")) {
        std::ofstream o(determineJsonFilePath());
        o << std::setw(4) << json_obj << std::endl;
    }

    if (anotherWindow) {
        showAnotherWindow(call, "Second Window", this->anotherWindow);
    }

    if (show_json_window) {
        display_json_window(call);
    }

    if (this->tryOut) {
        showSphereAtPoint(call, first_win_coordinates);
    }
    ImGui::End();
}

void AnnotationRenderer::showAnotherWindow(CallRender3DGL& call, std::string window_name, bool &window_open) {
    // Is this needed here as well?
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    ImGui::Begin(window_name.c_str(), &window_open);
    ImGui::InputText("Point Name", &this->second_win_point_name_input);
    ImGui::Text("Write your annotations here:");
    ImGui::InputText("Annotation", &this->second_win_annotation_input);
    ImGui::Text(this->second_win_annotation_input.c_str()); // TODO: Allow \n or similar functions to work!
    ImGui::InputFloat3("input coordinates", this->second_win_coordinates_input);

    // save inputs in local variables
    glm::vec3 second_win_coordinates =
        glm::vec3(second_win_coordinates_input[0], second_win_coordinates_input[1], second_win_coordinates_input[2]);
    std::string second_win_annotation = second_win_annotation_input;
    std::string second_win_point_name = second_win_point_name_input;

    if (ImGui::Button("Toggle Sphere")) {
        if (this->show_second_win_point) {
            this->show_second_win_point = false;
        } else {
            this->show_second_win_point = true;
        }
    }

    if (ImGui::Button("Save current coords and Annotation")) {
        save_new_point_to_json(second_win_coordinates, second_win_annotation, second_win_point_name);
    }

    if (this->show_second_win_point) {
        showSphereAtPoint(call, second_win_coordinates);
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
    core::view::Camera cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    auto cam_pose = cam.get<core::view::Camera::Pose>();

    auto colptr = this->linesColorSlot.Param<core::param::ColorParam>()->Value();

    glm::vec3 current = coords;

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

    // Render a point at the given coordinates
    // TODO: use a different mode
    glBegin(GL_POINTS);
    glVertex3f(current[0], current[1], current[2]);
    // TODO: Color the point with a user given color
    // glColor3f(colptr[0], colptr[1], colptr[2]);
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
}

/* Function to write the current coordinates and annotation to a json file */
void AnnotationRenderer::save_new_point_to_json(glm::vec3 coords, std::string annotation, std::string point_name) {
    // TODO: add saving of color?

    // update the amount of points with annotations
    ++json_amount;

    // save the current point and annotation
    json_obj["Points"][json_amount - 1] = {
        {"Coordinates", {coords.x, coords.y, coords.z}},
        {"Annotation", annotation},
        {"Point Number", json_amount},// to be able to get the correct number of the point again later TODO: is this really needed?
        {"Point Name", point_name},
        {"Show Window", false}
    };

    // TODO: maybe save the json every time this function is called
    // TODO: In case of saving every time a new entry was made, maybe add a _temp file that is deleted after the user saves to the real file
    // TODO: write json to vectors every time a new entry was made?
    std::cout << json_obj.dump(4) << std::endl;
}

/* Function to update the values of the given point in the json_obj and the corresponding vectors.
Only updates Annotation and Coordinates. */
void AnnotationRenderer::update_point_in_json(glm::vec3 coords, std::string annotation, int point_index) {
    // update the json object
    this->json_obj["Points"][point_index]["Coordinates"] = {coords.x, coords.y, coords.z}; // update the Coordinates
    this->json_obj["Points"][point_index]["Annotation"] = annotation;                      // update the annotation
    std::cout << json_obj.dump(4) << std::endl;
}

/* Opens a new window for loading from json file or loading from the current json_obj and display all aviable Points. */
void AnnotationRenderer::display_json_window(CallRender3DGL& call) {
    ImGui::Begin("test", &this->show_json_window);
    // this->anotherWindow = true;
    // showAnotherWindow(call, "Second Window", this->show_json_window);
    ImGui::Text("WARNING: Importing a file WILL overvrite everything you currently have!");
    if (ImGui::Button("Load json from file")) {
        // TODO: add popup
        warning_popup_bool = true;
        // load_json_from_file();
    }

    if (warning_popup_bool)
        warning_popup();

    if (ImGui::Button("Print the current state of json_obj to console")) {
        std::cout << std::setw(4) << json_obj << std::endl;
    }

    // std::vector items{"a", "b", "c"}; // defined somewhere
    // int selectedIndex = 0;            // you need to store this state somewhere
    static const char* current_item = NULL;

    // later in your code...
    if (ImGui::BeginCombo("combo", current_item)) {
        for (int i = 0; i < this->all_annotations.size(); ++i) {
            const bool isSelected = (json_point_name_selectedIndex == i);
            if (ImGui::Selectable(this->all_annotations[i].name.c_str(), isSelected)) {
                json_point_name_selectedIndex = i;
                current_item = this->all_annotations[i].name.c_str();
            }

            // Set the initial focus when opening the combo
            // (scrolling + keyboard navigation focus)
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // TODO: Get a good name for this new bool variable
    // TODO: Maybe make a global vector for all to be opened windows of points
    // => can dynamically work with more windows BUT then we have the problem of how to keep the windows open when the function will no longer be called...

    if (ImGui::Button("show current point")) {
        this->all_annotations[json_point_name_selectedIndex].show_window = true;
    }

    for (int index = 0; index < all_annotations.size(); ++index) {
        if (all_annotations[index].show_window)
            load_selected_json_point(call, all_annotations[index].name, all_annotations[index].show_window, index);
    }
    

    if (ImGui::Button("Save Names to vector")) {
        write_json_obj_data_to_vectors();
        /*for (std::string i : this->json_points_names)
            std::cout << i << ' ';*/
    }

    if (ImGui::Button("print currently selected item")) {
        std::cout << current_item << std::endl;
    }

    ImGui::End();
}

/* Writes the Names of the currently stored Points in the json_obj to a vector
Add a bool of true, if this function is called after loading a file, this will reset all stored window-bools */
void AnnotationRenderer::write_json_obj_data_to_vectors(bool loaded_from_file) {
    if (loaded_from_file)
        all_annotations.clear(); // when loading from a file then first clear the vector.

    int iterate = 0;
    for (auto& x : this->json_obj["Points"].items()) {
        ++iterate;
        int i = std::stoi(x.key());
        // Generate a temp value for the coordinates array, removes clutter in later calls.
        auto temp = x.value()["Coordinates"];

        // in case that json_obj holds more points then
        if (i >= all_annotations.size()) {
            this->all_annotations.push_back({x.value()["Annotation"], glm::vec3(temp[0], temp[1], temp[2]),
                x.value()["Point Name"], x.value()["Show Window"]});
        } else {
            this->all_annotations[i].annotation = x.value()["Annotation"];
            this->all_annotations[i].coordinates = glm::vec3(temp[0], temp[1], temp[2]);
            this->all_annotations[i].name = x.value()["Point Name"];
            this->all_annotations[i].show_window = x.value()["Show Window"];
        }
    }
    // Case that we have LESS points in json_obj then we have entries in all_annotations:
    // TODO: check if this works, need the remove from json_obj function for this.
    if (iterate < all_annotations.size()) {
        all_annotations.erase(std::next(all_annotations.begin(), iterate + 1), all_annotations.end()); 
    }
}

/* Loads the selected Point from the Combo of display_json_window()
It shows the values as they are currently in the individual global vectors
On a Button press you can update the values of the current point in the json file */
void AnnotationRenderer::load_selected_json_point(CallRender3DGL& call, std::string windowName, bool& window_open, int curr_index) {
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
        update_point_in_json(this->all_annotations[curr_index].coordinates, this->all_annotations[curr_index].annotation, curr_index);
    ImGui::End();
}

/* Makes a popup in the current frame with a warning message.
It sets the value of warning_popup_bool to false when a button is pressed. */
void AnnotationRenderer::warning_popup() {
    ImGui::OpenPopup("Warning");
    if (ImGui::BeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // TODO: make the warning popup more general... (add warning text and possible functions as inputs)
        ImGui::Text("Do you want to load the json file?\nThis will overwrite any current data that was not saved.\n\n");
        ImGui::Separator();

        if (ImGui::Button("YES", ImVec2(120, 0))) {
            load_json_from_file();
            warning_popup_bool = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("NO", ImVec2(120, 0))) {
            warning_popup_bool = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

}


void AnnotationRenderer::load_json_from_file() {
    // TODO: change the path to the path of the json file
    // std::ifstream i("C:\\Dateien\\megamol\\pretty.json");
    std::ifstream i(determineJsonFilePath());
    i >> json_obj;
    // TODO: this line is ONLY for debugging...
    if (IsDebuggerPresent)
        std::cout << std::setw(4) << json_obj << std::endl;
    // temp is for counting the amount of points in the json file
    int temp = 0;
    for (auto& x : json_obj["Points"].items()) {
        ++temp;
    }
    json_amount = temp;
    // now write the names to the and bools to the vector
    write_json_obj_data_to_vectors(true);
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
    int dataSize = 1 * 1;
    float* data = new float[dataSize];
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, data); // change 1,1 if bigger area is needed
    float depth = data[0];
    delete[] data;
    core::view::Camera cam = call.GetCamera();
    auto const lhsFBO = call.GetFramebuffer();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;
    auto invMVP = glm::inverse(mvp);
    float trararara = 2 * depth - 1;
    float nx = (2 * (x / lhsFBO->getWidth())) - 1; //Einheitswürfel
    float ny = (2 * (y / lhsFBO->getHeight())) - 1;
    glm::vec4 h = invMVP * glm::vec4(nx, ny, trararara, 1.0);
    glm::vec3 result = glm::vec3(h)/h.w;
    return result;
}
