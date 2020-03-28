/*
 * Group.h
 *
 * Copyright (C) 2020 by Universitaet Stuttgart (VISUS).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_GUI_GRAPH_GROUP_H_INCLUDED
#define MEGAMOL_GUI_GRAPH_GROUP_H_INCLUDED


#include "Module.h"
#include "CallSlot.h"
#include "GUIUtils.h"


namespace megamol {
namespace gui {
namespace configurator {


/**
 * Defines module data structure for graph.
 */
class Group {
public:
  
    Group(ImGuiID uid);
    ~Group();

    const ImGuiID uid;

    // Init when adding group to graph
    std::string name;

    bool AddModule(const ModulePtrType& module_ptr);
    bool RemoveModule(ImGuiID module_uid);
    bool ContainsModule(ImGuiID module_uid);

    bool AddCallSlot(const CallSlotPtrType& callslot_ptr);
    bool RemoveCallSlot(ImGuiID callslot_uid);
    bool ContainsCallSlot(ImGuiID callslot_uid);

    const ModulePtrVectorType& GetGroupModules(void) { return this->modules; }

    bool Empty(void) { return (this->modules.size() == 0); }

    // GUI Presentation -------------------------------------------------------

    void GUI_Present(GraphItemsStateType& state) { this->present.Present(*this, state); }

    void GUI_Update(const GraphCanvasType& in_canvas) { this->present.UpdatePositionSize(*this, in_canvas); }
    bool GUI_ModulesVisible(void) { return this->present.ModulesVisible(); }

private:

    // VARIABLES --------------------------------------------------------------

    ModulePtrVectorType modules;
    std::map<CallSlot::CallSlotType, CallSlotPtrVectorType> interface_callslots;

    /**
     * Defines GUI group presentation.
     */
    class Presentation {
    public:
        Presentation(void);

        ~Presentation(void);

        void Present(Group& inout_group, GraphItemsStateType& state);

        void UpdatePositionSize(Group& inout_group, const GraphCanvasType& in_canvas);
        bool ModulesVisible(void) { return !this->collapsed_view; }

        void ApplyUpdate(void) { this->update = true; }

    private:
        const float BORDER;

        // Relative position without considering canvas offset and zooming
        ImVec2 position;
        // Relative size without considering zooming
        ImVec2 size;
        GUIUtils utils;
        std::string name_label;
        bool collapsed_view;
        bool selected;
        bool update;        

    } present;

    // FUNCTIONS --------------------------------------------------------------


};


} // namespace configurator
} // namespace gui
} // namespace megamol

#endif // MEGAMOL_GUI_GRAPH_GROUP_H_INCLUDED