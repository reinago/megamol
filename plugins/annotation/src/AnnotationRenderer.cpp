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

#include "mmcore/CoreInstance.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/utility/log/Log.h"
#include "mmcore_gl/utility/ShaderFactory.h"

#include "mmcore/utility/Picking.h"

#include "imgui.h"
#include "imgui_internal.h"
using namespace megamol::mmstd_gl;

using namespace megamol::annotation;

/*
 * AnnotationRenderer::AnnotationRenderer
 */
AnnotationRenderer::AnnotationRenderer()
        : RendererModule<CallRender3DGL, ModuleGL>()
        , enableAnnotationRendererSlot("AnnotationRenderer", "Enables the rendering of the Annotations")
        , vbo(0)
        , ibo(0)
        , va(0)
        , boundingBoxes()
        , f(0.0)
        , buf() {

    this->enableAnnotationRendererSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->enableAnnotationRendererSlot);

    this->MakeSlotAvailable(&this->chainRenderSlot);
    this->MakeSlotAvailable(&this->renderSlot);
}

/*
 * AnnotationRenderer::~AnnotationRenderer
 */
AnnotationRenderer::~AnnotationRenderer() {
    this->Release();
}

/*
 * AnnotationRenderer::create
 */
bool AnnotationRenderer::create() {
    using namespace megamol::core::utility::log;

    auto const shader_options = ::msf::ShaderFactoryOptionsOpenGL(GetCoreInstance()->GetShaderPaths());
    try {
        lineShader = core::utility::make_glowl_shader("boundingbox", shader_options,
            "mmstd_gl/boundingbox/boundingbox.vert.glsl", "mmstd_gl/boundingbox/boundingbox.frag.glsl");
        cubeShader = core::utility::make_glowl_shader("viewcube", shader_options,
            "mmstd_gl/boundingbox/viewcube.vert.glsl", "mmstd_gl/boundingbox/viewcube.frag.glsl");

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
            this->boundingBoxes = call.AccessBoundingBoxes();
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
        test();
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    *chainedCall = call;
    renderRes &= (*chainedCall)(core::view::AbstractCallRender::FnRender);

    lhsFBO->bind();
    glViewport(0, 0, lhsFBO->getWidth(), lhsFBO->getHeight());

    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return renderRes;
}

void AnnotationRenderer::test() {
    bool valid_imgui_scope =
        ((ImGui::GetCurrentContext() != nullptr) ? (ImGui::GetCurrentContext()->WithinFrameScope) : (false));
    if (!valid_imgui_scope)
        return;

    // Creates a Window with the slider
    ImGui::Text("Hello, world %d", 123);
    ImGui::SliderFloat("float", &this->f, 0.0f, 1.0f, "%.3f");
    ImGui::InputText("string", &buf, IM_ARRAYSIZE(&buf));
    ImGui::Text(&buf);

    // Create a window called "My First Tool", with a menu bar.
    ImGui::Begin(this->Name());

    // Generate samples and plot them
    float samples[100];
    for (int n = 0; n < 100; n++)
        samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
    ImGui::PlotLines("Samples", samples, 100);

    // Display contents in a scrolling region
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
    ImGui::BeginChild("Scrolling");
    for (int n = 0; n < 50; n++)
        ImGui::Text("%04d: Some text", n);
    ImGui::EndChild();
    ImGui::End();
}
