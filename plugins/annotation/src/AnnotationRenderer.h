/**
 * MegaMol
 * Copyright (c) 2019, MegaMol Dev Team
 * All rights reserved.
 */

#pragma once

#include <memory>
#include <vector>
#include <deque>

#include <glowl/glowl.h>

#include <nlohmann/json.hpp>
#include "glm/gtx/quaternion.hpp" // glm::rotate(quat, vector)

#include "mmcore/CalleeSlot.h"
#include "mmcore/CallerSlot.h"
#include "mmcore/param/ParamSlot.h"
#include "mmstd/renderer/RendererModule.h"
#include "mmstd_gl/ModuleGL.h"
#include "mmstd_gl/renderer/CallRender3DGL.h"
#include "mmstd_gl/renderer/Renderer3DModuleGL.h"
#include "ScriptPaths.h"

#include "FrontendResource.h"


#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_tex_inspect.h"

// struct annotation_struct with glm::vec3 coordinates, std::string annotation, std::string name, bool show_window
struct annotation_struct {
    // Annotation of the current point
    std::string annotation;
    // Coordinates of the current point
    glm::vec3 coordinates;
    // Name of the current point
    std::string name;
    // Bool for showing the current point in a window
    bool show_window;
    // Bool for showing the current point in the 3D view
    bool show_point;
    // Bool for showing if the current point might be visible at the current time
    bool aviable_at_current_time;
    // Start Timestamp of the annotation
    float start_ts;
    // End Timestamp of the annotation
    float end_ts;
    // Current camera position
    glm::vec3 cam_pos;
    // Current camera orientation
    glm::quat cam_orientation;

    bool currently_editing;
};

// Struct for saving all variables that are needed for the "Adding Annotation" Window
struct annot_window_struct {
    // Coordinates in the Input Field
    float coordinates_input[3];
    // Annotation in the Input Field
    std::string annotation_input;
    // Sphere Color in the Input Field
    float color_input[3];
    // Point Name in Input Field
    std::string point_name_input;
    // Sphere Color for later use
    glm::vec3 color;
    // Bool for determining if the sphere for the current coordinates should be shown
    bool show_point;
    // annotation_struct for storing all inputs into the json_obj when pressing "save"
    annotation_struct annot_struct;
};

struct occlusionQueries {
    // Query itself
    std::vector<GLuint> query;
    // Query result
    std::vector<GLuint> result;
    // Query if the result is aviable
    std::vector<GLuint> resultAv;
    // Bool if the query was started
    std::vector<bool> queryStarted;
};


namespace megamol::annotation {

/**
 * Renderer responsible for the rendering the currently active Annotations
 * This is a special renderer without the typical structure of other renderers, since it does not inherit from
 * mmstd_gl::Renderer3Dmegamol::mmstd_gl::ModuleGL.
 */
class AnnotationRenderer : public megamol::mmstd_gl::Renderer3DModuleGL {
    //core::view::RendererModule<megamol::mmstd_gl::CallRender3DGL, megamol::mmstd_gl::ModuleGL> {
public:
    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName() {
        return "AnnotationRenderer";
    }

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static const char* Description() {
        return "Renders Annotation Boxes for the current data."; // TODO Need to change this description when it changes.
    }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static bool IsAvailable() {
        return true;
    }

    /** Ctor. */
    AnnotationRenderer();

    /** Dtor. */
    ~AnnotationRenderer() override;

    bool OnMouseButton(megamol::core::view::MouseButton button, megamol::core::view::MouseButtonAction action,
        megamol::core::view::Modifiers mods) override;

    bool OnMouseMove(double x, double y) override;

    std::vector<std::string> requested_lifetime_resources() override {
        std::vector<std::string> resources = megamol::mmstd_gl::Renderer3DModuleGL::requested_lifetime_resources();
        resources.emplace_back("LuaScriptPaths");
        resources.emplace_back("ExecuteLuaScript");
        return resources;
    }
    
protected:
    /**
     * Implementation of 'Create'.
     *
     * @return 'true' on success, 'false' otherwise.
     */
    bool create() override;

    /**
     * Implementation of 'Release'.
     */
    void release() override;

private:
    /*
     * Copies the incoming call to the outgoing one to pass the extents
     *
     * @param call The call containing all relevant parameters
     * @return True on success, false otherwise
     */
    bool GetExtents(megamol::mmstd_gl::CallRender3DGL& call) override;

    /*
     * Renders the Annotation windows
     *
     * @param call The call containing the camera and other parameters
     * @return True on success, false otherwise
     */
    bool Render(megamol::mmstd_gl::CallRender3DGL& call) final;

    /** Parameter that enables or disables the Annotation Renderer */
    core::param::ParamSlot enableAnnotationRendererSlot;

    /** Handle of the vertex buffer object */
    GLuint vbo;

    /** Handle of the index buffer object */
    GLuint ibo;

    /** Handle of the vertex array to be rendered */
    GLuint va;

    /** Shader program for lines */
    std::unique_ptr<glowl::GLSLProgram> lineShader;

    /** Bounding Boxes */
    megamol::core::BoundingBoxes_2 boundingBoxes;

    /* Test function for checking out how ImGUI behaves */
    void test(megamol::mmstd_gl::CallRender3DGL& call);

    /* Main function that holds all the connections to calling other functions */
    void new_main(megamol::mmstd_gl::CallRender3DGL& call);



    /** The simple shader for the drawing of GL_POINTS */
    std::unique_ptr<glowl::GLSLProgram> simpleShader;

    /** The pretty shader that draws spheres*/
    std::unique_ptr<glowl::GLSLProgram> sphereShader;


    void print_coords(glm::vec3 coords);

    void showSphereAtPoint(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 coords);

    void showAddingAnotationWindow(megamol::mmstd_gl::CallRender3DGL& call, std::string window_name, bool& window_open);

    void save_new_point_to_json(megamol::mmstd_gl::CallRender3DGL& call, annotation_struct input);

    void display_json_window(megamol::mmstd_gl::CallRender3DGL& call);

    void write_json_obj_data_to_vectors(megamol::mmstd_gl::CallRender3DGL& call, bool loaded_from_file = false);

    void display_window_of_selected_json_point(
        megamol::mmstd_gl::CallRender3DGL& call, std::string windowName, bool& window_open, int curr_index);

    void update_point_in_json(
        megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 coords, std::string annotation, int point_index);

    void load_json_from_file(megamol::mmstd_gl::CallRender3DGL& call);

    void save_json_to_file();

    void drawPointNames(megamol::mmstd_gl::CallRender3DGL& call);
    void save_slot_values_to_json();

    void load_slot_values_from_json();

    /* Calculate the coordinates for a given clicked */
    glm::vec3 calcClickedPoint(int x, int y, megamol::mmstd_gl::CallRender3DGL& call);

    std::string determineJsonFilePath() const;

    void loadCameraPosition(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 inputCamPos, glm::quat inputCamOrient);

    void determine_points_to_be_shown(megamol::mmstd_gl::CallRender3DGL& call);

    glm::vec2 getScreenPosFromWorldCoords(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 input_coords);

    void display_visual_points_windows(megamol::mmstd_gl::CallRender3DGL& call, std::string windowName, int curr_index,
        glm::vec3 point_pos, glm::vec2 offset = glm::vec2(0.0f, 0.0f), bool drawLine = true, bool saveSize = false);



    void showSphereAtPointIndex(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 coords, int index);

    void list_Window(megamol::mmstd_gl::CallRender3DGL& call);

    void drawConnectionLine(megamol::mmstd_gl::CallRender3DGL& call, glm::vec2 windowPos, glm::vec3 worldPos);

    glm::vec3 getWorldCoordsFromScreenPos(megamol::mmstd_gl::CallRender3DGL& call, int x, int y, float z, bool flipY);

    void editing_Annotations_Window(megamol::mmstd_gl::CallRender3DGL& call, int index);

    /* Parameters */
    /** Slot for the scaling factor of the pointsize*/
    core::param::ParamSlot sizeScalingSlot;

    core::param::ParamSlot sphereColorSlot;

    core::param::ParamSlot linesColorSlot;

    /* Slot for enabling the drawing with Fonts */
    core::param::ParamSlot drawTextSlot;

    /*Slot for setting the Text Scaling of Fonts */
    core::param::ParamSlot textScalingSlot;

    // gives the depth buffer to the renderer
    // megamol::core::CallerSlot get_depth_buffer;

    core::param::ParamSlot saveSlotValuesSlot;

    core::param::ParamSlot loadSlotValuesSlot;

    core::param::ParamSlot loadJsonFromFileSlot;

    core::param::ParamSlot saveJsonToFileSlot;

    /* Slot for enabling the Window for adding new Annotations */
    core::param::ParamSlot enableAddingAnnotationWindowSlot;

    /* Slot for enabling the Window for JSON things */
    core::param::ParamSlot enableJsonWindowSlot;

    /* Slot for enabling the Window for showing the list with all Annotations */
    core::param::ParamSlot enableListWindowSlot;

    core::param::ParamSlot wrappWidthSlot;
    
    /*
    VARIABLES
    */
    /* Frames Variables */
    // stores the total number of frames of the animation
    float totalFrameCount;

    // stores if the frame is even or uneven (0 or 1)
    int frameType;

    /* GL variables */
    occlusionQueries occlusionQuery;
    
    /** Picking Variables */
    bool picking_enabled;
    bool picked_a_point;

    /** Last mouse position (for deprecation mapping) */
    float lastX, lastY;
    
    /** ImGUI Variables */
    float my_color;
    float first_win_coordinates_input[3];
    float first_win_color_input[3];
    glm::vec3 first_win_color;

    bool tryOut;

    bool anotherWindow;

    bool show_json_window;

    /* ImGui Second Window Variables */
    //float annot_win_coordinates_input[3];
    //std::string annot_win_annotation_input;
    //float annot_win_color_input[3];
    //glm::vec3 annot_win_color;
    //bool show_annot_win_point;
    //std::string annot_win_point_name_input;
    //annotation_struct annot_win_struct;
    annot_window_struct annot_win_struct;

    std::vector<glm::vec2> pointWindowSizes;

    bool grh;

    std::vector<annotation_struct> all_annotations;


    /* json Variables */

    nlohmann::json json_obj;
    int json_amount;
    std::string json_file_path;

    int json_point_name_selectedIndex;

};
} // namespace megamol::annotation
