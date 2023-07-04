/**
 * MegaMol
 * Copyright (c) 2023, MegaMol Dev Team
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
    std::string annotation = "";
    // Coordinates of the current point
    glm::vec3 coordinates = glm::vec3(0.0f,0.0f,0.0f);
    // Name of the current point
    std::string name = "";
    // Bool for showing the current point in a window
    bool show_window = false;
    // Bool for showing the current point in the 3D view
    bool show_point = false;
    // Bool for showing if the current point might be visible at the current time
    bool aviable_at_current_time = false;
    // Start Timestamp of the annotation
    float start_ts = 0.0f;
    // End Timestamp of the annotation
    float end_ts = 0.0f;
    // Current camera position
    glm::vec3 cam_pos = glm::vec3(0.0f,0.0f,0.0f);
    // Current camera orientation
    glm::quat cam_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool currently_editing = false;
    // String for Tags of the Annotations for filtering
    std::string tag = "";
    
};

// Struct for saving all variables that are needed for the "Adding Annotation" Window
struct annot_window_struct {
    // Coordinates in the Input Field
    float coordinates_input[3] = {0.0f, 0.0f, 0.0f};
    // Annotation in the Input Field
    std::string annotation_input = "";
    // Point Name in Input Field
    std::string point_name_input = "";
    // Bool for determining if the sphere for the current coordinates should be shown
    bool show_point = false;
    // annotation_struct for storing all inputs into the json_obj when pressing "save"
    annotation_struct annot_struct = {};
    // Bool for saving if the Start TS was set by the user
    bool start_ts_set = false;
    // Bool for saving if the End TS was set by the user
    bool end_ts_set = false;
    // Bool for saving if the Camera Position was set by the user
    bool camera_set = false;
    // Color for the sphere of the current point
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    // String for Tags of the Annotations for filtering
    std::string tag = "";
    bool tag_set = false;
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

struct listWindowStruct {
    // This is for enabeling and disabeling Deletion of annotations.
    bool allowDeletion = false;
    // This is for enabling the autoResize feature of the List window
    bool autoResize = true;
    // This allows the user to show or hide the background of the point windows
    bool opaqueWindowsOfPoints = true;
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

    /** The pretty shader that draws spheres*/
    std::unique_ptr<glowl::GLSLProgram> sphereShader;
    

    /* Main function that holds all the connections to calling other functions */
    void new_main(megamol::mmstd_gl::CallRender3DGL& call);

    void print_coords(glm::vec3 coords);

    void showSphereAtPoint(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 coords, float color[4]);

    void showAddingAnotationWindow(megamol::mmstd_gl::CallRender3DGL& call, std::string window_name);

    void write_json_obj_data_to_vectors(megamol::mmstd_gl::CallRender3DGL& call, bool loaded_from_file = false);

    void display_window_of_selected_json_point(
        megamol::mmstd_gl::CallRender3DGL& call, std::string windowName, bool& window_open, int curr_index);

    void save_json_to_file();

    void save_slot_values_to_json();

    void load_slot_values_from_json();

    glm::vec3 calcClickedPoint(int x, int y, megamol::mmstd_gl::CallRender3DGL& call);

    std::string determineJsonFilePath() const;

    void loadCameraPosition(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 inputCamPos, glm::quat inputCamOrient);

    void determine_points_to_be_shown(megamol::mmstd_gl::CallRender3DGL& call);

    glm::vec2 getScreenPosFromWorldCoords(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 input_coords);

    void display_visual_points_windows(megamol::mmstd_gl::CallRender3DGL& call, std::string windowName, int curr_index,
        glm::vec3 point_pos, glm::vec2 offset = glm::vec2(0.0f, 0.0f), bool drawLine = true, bool saveSize = false);

    void showSphereAtPointIndex(megamol::mmstd_gl::CallRender3DGL& call, glm::vec3 coords, int index);

    void list_Window(megamol::mmstd_gl::CallRender3DGL& call);

    void forceDirectedLayout(
        megamol::mmstd_gl::CallRender3DGL& call, std::string tagName);

    void drawConnectionLine(megamol::mmstd_gl::CallRender3DGL& call, glm::vec2 windowPos, glm::vec3 worldPos);

    glm::vec3 getWorldCoordsFromScreenPos(megamol::mmstd_gl::CallRender3DGL& call, int x, int y, float z, bool flipY);

    void editing_Annotations_Window(megamol::mmstd_gl::CallRender3DGL& call, int index);

    void loadJsonFromFileToVectors(megamol::mmstd_gl::CallRender3DGL& call);

    void updateAnnotationInJsonObj(megamol::mmstd_gl::CallRender3DGL& call, int i);

    void saveNewPoint(megamol::mmstd_gl::CallRender3DGL& call, annotation_struct input);

    void deleteAnnotation(megamol::mmstd_gl::CallRender3DGL& call, int i);

    void loadOldValuesFromJsonobj(megamol::mmstd_gl::CallRender3DGL& call, int i);

    void testingFunction(megamol::mmstd_gl::CallRender3DGL& call, std::string wantedTag);

    
    /* Parameters */
    
    /** Slot for the scaling factor of the pointsize*/
    core::param::ParamSlot sizeScalingSlot;

    /* Slot for the color of the Spheres */
    core::param::ParamSlot sphereColorSlot;

    /* Slot for the color of the Lines */
    core::param::ParamSlot linesColorSlot;

    /* Slot for enabling the drawing of the Point Windows */
    core::param::ParamSlot drawTextSlot;

    /* Slot for changing the Color of the Text of drawn Annotations */
    core::param::ParamSlot textColorSlot;

    /* Slot for changing the Color of the Title of drawn Annotations */
    core::param::ParamSlot titleColorSlot;

    /* Slot for saving the Values of some Slots */
    core::param::ParamSlot saveSlotValuesSlot;

    /* Slot for loading the Values of some Slots */
    core::param::ParamSlot loadSlotValuesSlot;

    /* Slot for loading the Annotations from a Json file */
    core::param::ParamSlot loadJsonFromFileSlot;

    /* Slot for saving the Annotations to a Json file */
    core::param::ParamSlot saveJsonToFileSlot;

    /* Slot for enabling the Window for adding new Annotations */
    core::param::ParamSlot enableAddingAnnotationWindowSlot;

    /* Slot for enabling the Window for JSON things */
    core::param::ParamSlot enableJsonWindowSlot;

    /* Slot for enabling the Window for showing the list with all Annotations */
    core::param::ParamSlot enableListWindowSlot;

    /* Slot for changing the Wraping in the Annotations */
    core::param::ParamSlot wrapWidthSlot;

    /* Slot for setting the Path to the Json file */
    core::param::ParamSlot filenameSlot;

    /* Slot for setting the Tag that is shown */
    core::param::ParamSlot shownTagsSlot;

    
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

    
    /* Picking Variables */
    bool picking_enabled;
    bool picked_a_point;

    
    /* Last mouse position (for deprecation mapping) */
    float lastX, lastY;

    
    /* ImGui Adding Annotations Window Variables */
    annot_window_struct annot_win_struct;

    
    /* Annotations */
    // Stores all Annotations and their data
    std::vector<annotation_struct> all_annotations;
    
    // Stores the window sizes of all Annotations
    std::vector<glm::vec2> pointWindowSizes;

    
    /* ImGui List Variables */
    listWindowStruct listWindowBooleans;

    /* json Variables */
    // Stores the json object
    nlohmann::json json_obj;
    
    // Stores the path to the json file
    std::string json_file_path;
    
    // Stores the toggle, if a json path was given by the user
    bool json_file_path_set;
};
} // namespace megamol::annotation
