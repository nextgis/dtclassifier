
########################################################
# Files

SET (classifier_SRCS
     classifier.cpp
     classifiergui.cpp
)

SET (classifier_UIS classifierguibase.ui)

SET (classifier_MOC_HDRS
     classifier.h
     classifiergui.h
)

SET (classifier_RCCS  classifier.qrc)

########################################################
# Build

QT4_WRAP_UI (classifier_UIS_H  ${classifier_UIS})

QT4_WRAP_CPP (classifier_MOC_SRCS  ${classifier_MOC_HDRS})

QT4_ADD_RESOURCES(classifier_RCC_SRCS ${classifier_RCCS})

ADD_LIBRARY (classifierplugin MODULE ${classifier_SRCS} ${classifier_MOC_SRCS} ${classifier_RCC_SRCS} ${classifier_UIS_H})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ../../core ../../core/raster ../../core/renderer ../../core/symbology
     ../../gui
     ..
)

TARGET_LINK_LIBRARIES(classifierplugin
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS classifierplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

