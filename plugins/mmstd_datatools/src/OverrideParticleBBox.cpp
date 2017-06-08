/*
 * OverrideParticleBBox.h
 *
 * Copyright (C) 2015 by S. Grottel
 * Alle Rechte vorbehalten.
 */
#include "stdafx.h"
#include "OverrideParticleBBox.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/Vector3fParam.h"
#include "mmcore/param/ButtonParam.h"
#include "vislib/sys/Log.h"
#include "vislib/math/Point.h"
#include "vislib/math/ShallowPoint.h"
#include <float.h>

using namespace megamol;
using namespace megamol::stdplugin;


VISLIB_FORCEINLINE vislib::math::ShallowPoint<float, 3> posFromXYZ(void* const ptr, size_t stride, size_t idx) {
    return vislib::math::ShallowPoint<float, 3>(reinterpret_cast<float *>(reinterpret_cast<char *>(ptr) + idx * stride));
}

VISLIB_FORCEINLINE vislib::math::ShallowPoint<float, 3> posFromXYZR(void* const ptr, size_t stride, size_t idx) {
    return vislib::math::ShallowPoint<float, 3>(reinterpret_cast<float *>(reinterpret_cast<char *>(ptr) + idx * stride));
}

//VISLIB_FORCEINLINE vislib::math::AbstractPoint<float, 3, float[3]> posFromXYZ_SHORT(void* const ptr , size_t stride, size_t idx) {
//    const unsigned short *newPtr = reinterpret_cast< unsigned short *>(reinterpret_cast<const char *>(ptr) + idx * stride);
//    return vislib::math::Point<float, 3>(newPtr[0], newPtr[1], newPtr[2]);
//}

typedef vislib::math::ShallowPoint<float, 3>(*posFromSomethingFunc)(void*, size_t, size_t);

/*
 * datatools::OverrideParticleBBox::OverrideParticleBBox
 */
datatools::OverrideParticleBBox::OverrideParticleBBox(void)
        : AbstractParticleManipulator("outData", "indata"),
        overrideBBoxSlot("override", "Activates the overwrite of the bounding box"),
        bboxMinSlot("bbox::min", "The minimum values of the bounding box"),
        bboxMaxSlot("bbox::max", "The maximum values of the bounding box"),
        resetSlot("reset", "Resets the bounding box values to the incoming data"),
        autocomputeSlot("autocompute::trigger", "Triggers the automatic computation of the bounding box"),
        autocomputeSamplesSlot("autocompute::samples", "Number of samples for the automatic computation"),
        autocomputeXSlot("autocompute::x", "Activates automatic computation of the x size"),
        autocomputeYSlot("autocompute::y", "Activates automatic computation of the y size"),
        autocomputeZSlot("autocompute::z", "Activates automatic computation of the z size")
    {

    this->overrideBBoxSlot.SetParameter(new core::param::BoolParam(false));
    this->MakeSlotAvailable(&this->overrideBBoxSlot);

    this->bboxMinSlot.SetParameter(new core::param::Vector3fParam(vislib::math::Vector<float, 3>(-1.0f, -1.0f, -1.0f)));
    this->MakeSlotAvailable(&this->bboxMinSlot);

    this->bboxMaxSlot.SetParameter(new core::param::Vector3fParam(vislib::math::Vector<float, 3>(1.0f, 1.0f, 1.0f)));
    this->MakeSlotAvailable(&this->bboxMaxSlot);

    this->resetSlot.SetParameter(new core::param::ButtonParam());
    this->MakeSlotAvailable(&this->resetSlot);

    this->autocomputeSlot.SetParameter(new core::param::ButtonParam());
    this->MakeSlotAvailable(&this->autocomputeSlot);

    this->autocomputeSamplesSlot.SetParameter(new core::param::IntParam(10, 0));
    this->MakeSlotAvailable(&this->autocomputeSamplesSlot);

    this->autocomputeXSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->autocomputeXSlot);

    this->autocomputeYSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->autocomputeYSlot);

    this->autocomputeZSlot.SetParameter(new core::param::BoolParam(true));
    this->MakeSlotAvailable(&this->autocomputeZSlot);
}


/*
 * datatools::OverrideParticleBBox::~OverrideParticleBBox
 */
datatools::OverrideParticleBBox::~OverrideParticleBBox(void) {
    this->Release();
}


/*
 * datatools::OverrideParticleBBox::manipulateData
 */
bool datatools::OverrideParticleBBox::manipulateData(
        megamol::core::moldyn::MultiParticleDataCall& outData,
        megamol::core::moldyn::MultiParticleDataCall& inData) {
    return this->manipulateExtent(outData, inData); // because the actual implementations are identical
}


/*
 * datatools::OverrideParticleBBox::manipulateExtent
 */
bool datatools::OverrideParticleBBox::manipulateExtent(
        megamol::core::moldyn::MultiParticleDataCall& outData,
        megamol::core::moldyn::MultiParticleDataCall& inData) {
    using megamol::core::moldyn::MultiParticleDataCall;

    if (this->resetSlot.IsDirty()) {
        this->resetSlot.ResetDirty();
        this->bboxMinSlot.Param<core::param::Vector3fParam>()->SetValue(inData.AccessBoundingBoxes().ObjectSpaceBBox().GetLeftBottomBack());
        this->bboxMaxSlot.Param<core::param::Vector3fParam>()->SetValue(inData.AccessBoundingBoxes().ObjectSpaceBBox().GetRightTopFront());
    }
    //if (this->autocomputeSlot.IsDirty()) {
    //    this->autocomputeSlot.ResetDirty();

    //    vislib::sys::Log::DefaultLog.WriteError("OverrideParticleBBox::Autocompute not implemented");
    //    // TODO: Implement
    //    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    //    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    //    for (size_t l = 0, max = inData.GetParticleListCount(); l < max; l++) {
    //        for (size_t p = 0, maxP = inData.AccessParticles(l).GetCount(); p < maxP; p++) {
    //            //inData.AccessParticles(l).GetVertexData()
    //        }
    //    }
    //}
    bool doX = this->autocomputeXSlot.Param<core::param::BoolParam>()->Value();
    bool doY = this->autocomputeXSlot.Param<core::param::BoolParam>()->Value();
    bool doZ = this->autocomputeXSlot.Param<core::param::BoolParam>()->Value();
    int samples = this->autocomputeSamplesSlot.Param<core::param::IntParam>()->Value();

    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
    if (this->overrideBBoxSlot.Param<core::param::BoolParam>()->Value() && (doX || doY || doZ)) {
        size_t step;

        for (size_t l = 0, max = inData.GetParticleListCount(); l < max; l++) {
            if (inData.AccessParticles(l).GetVertexDataType() == megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZ
                || inData.AccessParticles(l).GetVertexDataType() == megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZR) {
                posFromSomethingFunc getPoint;
                switch (inData.AccessParticles(l).GetVertexDataType()) {
                    case megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZ:
                        getPoint = posFromXYZ;
                        break;
                    case megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_FLOAT_XYZR:
                        getPoint = posFromXYZR;
                        break;
                }
                size_t maxP = inData.AccessParticles(l).GetCount();
                if (samples == 0) {
                    step = 1;
                } else {
                    step = maxP / samples;
                }
                void *vertPtr = const_cast<void *>(inData.AccessParticles(l).GetVertexData());
                size_t stride = inData.AccessParticles(l).GetVertexDataStride();
                // gief openmp 3.1
                //#pragma omp parallel for reduction(min: minX, max: maxX)
                for (size_t p = 0; p < maxP; p += step) {
                    vislib::math::ShallowPoint<float, 3> sp = getPoint(vertPtr, stride, p);
                    if (sp.GetX() < minX) {
                        minX = sp.GetX();
                    }
                    if (sp.GetX() > maxX) {
                        maxX = sp.GetX();
                    }
                    if (sp.GetY() < minY) {
                        minY = sp.GetY();
                    }
                    if (sp.GetY() > maxY) {
                        maxY = sp.GetY();
                    }
                    if (sp.GetZ() < minZ) {
                        minZ = sp.GetZ();
                    }
                    if (sp.GetZ() > maxZ) {
                        maxZ = sp.GetZ();
                    }
                }
            } else {
                switch (inData.AccessParticles(l).GetVertexDataType()) {
                    case megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_SHORT_XYZ:
                        //getPoint = posFromXYZ_SHORT;
                        vislib::sys::Log::DefaultLog.WriteError("OverrideParticleBBox does not support re-computation of short coordinates");
                        break;
                    case megamol::core::moldyn::MultiParticleDataCall::Particles::VERTDATA_NONE:
                        vislib::sys::Log::DefaultLog.WriteInfo("OverrideParticleBBox: skipping empty vertex data");
                        break;
                }
            }
        }
    }

    outData = inData; // also transfers the unlocker to 'outData'
    inData.SetUnlocker(nullptr, false); // keep original data locked
                                        // original data will be unlocked through outData

    if (this->overrideBBoxSlot.Param<core::param::BoolParam>()->Value()) {
        const vislib::math::Vector<float, 3>& l = this->bboxMinSlot.Param<core::param::Vector3fParam>()->Value();
        const vislib::math::Vector<float, 3>& u = this->bboxMaxSlot.Param<core::param::Vector3fParam>()->Value();
        outData.AccessBoundingBoxes().SetObjectSpaceBBox(doX ? minX : l.X(), doY ? minY : l.Y(), doZ ? minZ : l.Z(),
            doX ? maxX : u.X(), doY ? maxY : u.Y(), doZ ? maxZ : u.Z());
    }

    return true;
}