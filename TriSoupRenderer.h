/*
 * TriSoupRenderer.h
 *
 * Copyright (C) 2010 by Sebastian Grottel
 * Copyright (C) 2008-2010 by VISUS (Universitaet Stuttgart)
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOLCORE_TRISOUPRENDERER_H_INCLUDED
#define MEGAMOLCORE_TRISOUPRENDERER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "view/Renderer3DModule.h"
#include "Call.h"
#include "CallerSlot.h"
#include "param/ParamSlot.h"
#include "vislib/Cuboid.h"
#include "vislib/memutils.h"


namespace megamol {
namespace trisoup {


    /**
     * Renderer for tri-mesh data
     */
    class TriSoupRenderer : public core::view::Renderer3DModule {
    public:

        /**
         * Answer the name of this module.
         *
         * @return The name of this module.
         */
        static const char *ClassName(void) {
            return "TriSoupRenderer";
        }

        /**
         * Answer a human readable description of this module.
         *
         * @return A human readable description of this module.
         */
        static const char *Description(void) {
            return "Renderer for tri-mesh data";
        }

        /**
         * Answers whether this module is available on the current system.
         *
         * @return 'true' if the module is available, 'false' otherwise.
         */
        static bool IsAvailable(void) {
            return true;
        }

        /** Ctor. */
        TriSoupRenderer(void);

        /** Dtor. */
        virtual ~TriSoupRenderer(void);

    protected:

        /**
         * Implementation of 'Create'.
         *
         * @return 'true' on success, 'false' otherwise.
         */
        virtual bool create(void);

        /**
         * The get capabilities callback. The module should set the members
         * of 'call' to tell the caller its capabilities.
         *
         * @param call The calling call.
         *
         * @return The return value of the function.
         */
        virtual bool GetCapabilities(core::Call& call);

        /**
         * The get extents callback. The module should set the members of
         * 'call' to tell the caller the extents of its data (bounding boxes
         * and times).
         *
         * @param call The calling call.
         *
         * @return The return value of the function.
         */
        virtual bool GetExtents(core::Call& call);

        /**
         * Implementation of 'Release'.
         */
        virtual void release(void);

        /**
         * The render callback.
         *
         * @param call The calling call.
         *
         * @return The return value of the function.
         */
        virtual bool Render(core::Call& call);

    private:

        /** The slot to fetch the data */
        core::CallerSlot getDataSlot;

        /** Flag whether or not to show vertices */
        core::param::ParamSlot showVertices;

        /** Flag whether or not use lighting for the surface */
        core::param::ParamSlot lighting;

        /** Flag whether or not use face culling for the surface */
        core::param::ParamSlot cullface;

        /** The rendering style for the surface */
        core::param::ParamSlot surStyle;

    };


} /* end namespace trisoup */
} /* end namespace megamol */

#endif /* MEGAMOLCORE_TRISOUPRENDERER_H_INCLUDED */
