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
#include "mmcore/utility/log/Log.h"
#include "mmcore_gl/utility/ShaderFactory.h"

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
        , linesColorSlot("linesColor", "Color of the Connection Lines between Annotation and Point in 3D")
        , vbo(0)
        , ibo(0)
        , va(0)
        , boundingBoxes()
        , picking_enabled(false)
        , picked_a_point(false)
        , lastX()
        , lastY()
        , my_color()
        , first_win_coordinates_input()
        , first_win_color_input()
        , first_win_color()
        , tryOut(false)
        , anotherWindow(false)
        , annot_win_struct()
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
        //test(call);
        // TODO: just testing new main function:
        new_main(call);
    }
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
* Main function. OLD!!!
* This function generates the main ImGui window and allows the opening of all other windows.
*/
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
        save_json_to_file();
    }

    if (anotherWindow) {
        showAddingAnotationWindow(call, "Second Window", this->anotherWindow);
    }

    if (show_json_window) {
        display_json_window(call);
    }

    if (this->tryOut) {
        showSphereAtPoint(call, first_win_coordinates);
    }
    ImGui::End();


}


/*
 * Main function.
 * This function generates the main ImGui window and allows the opening of all other windows.
 */
void AnnotationRenderer::new_main(CallRender3DGL& call) {
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    // TODO: With this version it is not possible to close the window with the "x" button
    /* Displays the Window for adding new Annotations */
    if (this->enableAddingAnnotationWindowSlot.Param<core::param::BoolParam>()->Value()) {
        this->anotherWindow = true;
        showAddingAnotationWindow(call, "Second Window", this->anotherWindow);
    } else {
        this->anotherWindow = false;
    }

    /* Displays the Window for Handling all current Annotations */
    if (this->enableJsonWindowSlot.Param<core::param::BoolParam>()->Value()) {
        this->show_json_window = true;
        display_json_window(call);
    } else {
         this->show_json_window = false;
    }
        
    
    // TODO: Add saving all points to JSON file in the main list? OR is it better to just have it in the "JSON window"?
    // // Save the current state of the json_obj to a json file
    // if (ImGui::Button("Save annotations to Json File")) {
    //     save_json_to_file();
    // }
    // TODO: DEBUG ONLY
    showSphereAtPoint(call, glm::vec3(0.0f, 0.0f, 0.0f));
    drawConnectionLine(call, glm::vec2(1000.0f, 100.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    
}

/*
* Function for the ImGui window that allows the adding of a new point.
*/
void AnnotationRenderer::showAddingAnotationWindow(CallRender3DGL& call, std::string window_name, bool &window_open) {
    // Is this needed here as well?
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    ImGui::Begin(window_name.c_str(), &window_open);
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
        save_new_point_to_json(this->annot_win_struct.annot_struct);
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

    auto colptr = this->linesColorSlot.Param<core::param::ColorParam>()->Value();

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
void AnnotationRenderer::save_new_point_to_json(annotation_struct input) {
    // TODO: add saving of color?

    // update the amount of points with annotations
    ++json_amount;

    // save the current point and annotation
    json_obj["Points"][json_amount - 1] = {
        {"Coordinates", {input.coordinates.x, input.coordinates.y, input.coordinates.z}},
        {"Annotation", input.annotation},
        {"Point Number", json_amount},// to be able to get the correct number of the point again later TODO: is this really needed?
        {"Point Name", input.name},
        {"Show Window", false},
        {"Start Timestamp", input.start_ts},
        {"End Timestamp", input.end_ts}
    };

    // TODO: maybe save the json every time this function is called
    // TODO: In case of saving every time a new entry was made, maybe add a _temp file that is deleted after the user saves to the real file
    // write json to vectors every time a new entry was made?
    // TODO: Calling write to vector CLOSES ALL opened windows!!!!!!
    write_json_obj_data_to_vectors(false);
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
    // showAddingAnotationWindow(call, "Second Window", this->show_json_window);
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
            display_window_of_selected_json_point(call, all_annotations[index].name, all_annotations[index].show_window, index);
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
            this->all_annotations.push_back({x.value()["Annotation"], glm::vec3(temp[0], temp[1], temp[2]), x.value()["Point Name"],
                    x.value()["Show Window"], x.value()["Start Timestamp"], x.value()["End Timestamp"]});
        } else {
            this->all_annotations[i].annotation = x.value()["Annotation"];
            this->all_annotations[i].coordinates = glm::vec3(temp[0], temp[1], temp[2]);
            this->all_annotations[i].name = x.value()["Point Name"];
            this->all_annotations[i].show_window = x.value()["Show Window"];
            this->all_annotations[i].start_ts = x.value()["Start Timestamp"];
            this->all_annotations[i].start_ts = x.value()["End Timestamp"];
        }
    }
    // Case that we have LESS points in json_obj then we have entries in all_annotations:
    // TODO: check if this works, need the remove from json_obj function for this.
    if (iterate < all_annotations.size()) {
        all_annotations.erase(std::next(all_annotations.begin(), iterate + 1), all_annotations.end()); 
    }
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

/* Loads the json file that is found under its path into the json_obj and the vector all_annotatins */
void AnnotationRenderer::load_json_from_file() {
    // TODO: change the path to the path of the json file
    // std::ifstream i("C:\\Dateien\\megamol\\pretty.json");
    std::string file_path = determineJsonFilePath();
    if (file_path.empty()) {
        // TODO: Add warning message that there is no file to be loaded!
        // this is just a warning message on the console that is not nessecerily something for the "regular" user
        std::cout << "There is no file to be loaded" << std::endl;
        return;
    }
    std::ifstream i(file_path);
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

/*
* Saves the current state of the variable "json_obj" into the json file for the currently used project.
*/
void AnnotationRenderer::save_json_to_file() {
    std::ofstream o(determineJsonFilePath());
    o << std::setw(4) << this->json_obj << std::endl;
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
/* Function that draws a connection line between an ImGui window and the given coordinates in 3D. */
void AnnotationRenderer::drawConnectionLine(CallRender3DGL& call, glm::vec2 windowPos, glm::vec3 worldPos) {
    // TODO: add variable for changing the line color?
    auto& colptr = this->linesColorSlot.Param<core::param::ColorParam>()->Value();
    glm::vec3 lineColor = glm::vec3(1.0f, 1.0f, 1.0f);
    auto cam = call.GetCamera();
    auto view = cam.getViewMatrix();
    auto proj = cam.getProjectionMatrix();
    auto mvp = proj * view;

    // TODO: get correct z Value from nearPlane (But I do not see how it can be accessed at least not from call.camera, because it is a private member)
    float z = 0.0f;

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
