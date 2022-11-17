/**
 * MegaMol
 * Copyright (c) 2016, MegaMol Dev Team
 * All rights reserved.
 */

#include "mmcore/utility/plugins/AbstractPluginInstance.h"
#include "mmcore/utility/plugins/PluginRegister.h"

#include "AnnotationRenderer.h"


namespace megamol::annotation {
class AnnotationPluginInstance : public megamol::core::utility::plugins::AbstractPluginInstance {
    REGISTERPLUGIN(AnnotationPluginInstance)

public:
    AnnotationPluginInstance()
            : megamol::core::utility::plugins::AbstractPluginInstance("annotation", "The annotation plugin."){};

    ~AnnotationPluginInstance() override = default;

    // Registers modules and calls
    void registerClasses() override {

        // register modules
		this->module_descriptions.RegisterAutoDescription<megamol::annotation::AnnotationRenderer>();

        // register calls
    }
};
} // namespace megamol::annotation
