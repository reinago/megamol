# MegaMol
# Copyright (c) 2021, MegaMol Dev Team
# All rights reserved.
#

megamol_plugin(annotation
  BUILD_DEFAULT OFF
  DEPENDS_FEATURES
    opengl
  DEPENDS_PLUGINS
    mmstd
    mmstd_gl)

if (annotation_PLUGIN_ENABLED)
  find_package(imgui CONFIG REQUIRED)
  target_link_libraries(annotation
    PRIVATE
      imgui::imgui)
endif ()
