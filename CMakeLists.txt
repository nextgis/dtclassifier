# set path to additional CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

FIND_PACKAGE(OpenCV REQUIRED)

########################################################
# Files
SET (CLASSIFIER_COMMON_SRCS
    classifierworker.cpp
    rasterfileinfo.cpp
    classifierutils.cpp
)
SET (CLASSIFIER_PLUGIN_SRCS
     classifier.cpp
     classifierdialog.cpp
     layerselectordialog.cpp
     aboutdialog.cpp
)
SET (CLASSIFIER_UTIL_SRCS
     main_application.cpp
     main.cpp
)

SET (CLASSIFIER_UIS
     classifierdialogbase.ui
     layerselectordialogbase.ui
     about.ui
)

SET (CLASSIFIER_COMMON_MOC_HDRS
     classifierworker.h
)
SET (CLASSIFIER_PLUGIN_MOC_HDRS
     classifier.h
     classifierdialog.h
     layerselectordialog.h
     aboutdialog.h
)
SET (CLASSIFIER_UTIL_MOC_HDRS
     main_application.h
)

SET (CLASSIFIER_RCCS  classifier.qrc)

########################################################
# Build

QT4_WRAP_UI (CLASSIFIER_UIS_H  ${CLASSIFIER_UIS})

QT4_WRAP_CPP (CLASSIFIER_COMMON_MOC_SRCS  ${CLASSIFIER_COMMON_MOC_HDRS})
QT4_WRAP_CPP (CLASSIFIER_PLUGIN_MOC_SRCS  ${CLASSIFIER_PLUGIN_MOC_HDRS})
QT4_WRAP_CPP (CLASSIFIER_UTIL_MOC_SRCS  ${CLASSIFIER_UTIL_MOC_HDRS})

QT4_ADD_RESOURCES(CLASSIFIER_RCC_SRCS ${CLASSIFIER_RCCS})

ADD_LIBRARY (classifierplugin MODULE
  ${CLASSIFIER_COMMON_SRCS}
  ${CLASSIFIER_PLUGIN_SRCS}
  ${CLASSIFIER_COMMON_MOC_SRCS}
  ${CLASSIFIER_PLUGIN_MOC_SRCS}
  ${CLASSIFIER_RCC_SRCS}
  ${CLASSIFIER_UIS_H}
)

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_BINARY_DIR}
     ${QGIS_INCLUDE_DIR}
     ${GDAL_INCLUDE_DIR}
     ${OpenCV_INCLUDE_DIR}
     ../../core/raster
     ../../core/renderer
     ../../core/symbology
     ../../core/symbology-ng
     ../../core/geometry
     ../../core
     ../../gui
     ..
     .
)

TARGET_LINK_LIBRARIES(classifierplugin
  ${QGIS_CORE_TARGET_NAME}
  ${QGIS_GUI_TARGET_NAME}
  ${OpenCV_LIBS}
)
add_executable(classifier
  ${CLASSIFIER_COMMON_SRCS}
  ${CLASSIFIER_UTIL_SRCS}
  ${CLASSIFIER_COMMON_MOC_SRCS}
  ${CLASSIFIER_UTIL_MOC_SRCS}
)
set_target_properties( classifier PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/${QGIS_BIN_SUBDIR})

TARGET_LINK_LIBRARIES(classifier
  ${QGIS_CORE_TARGET_NAME}
  ${QGIS_GUI_TARGET_NAME}
  ${OpenCV_LIBS}
)

########################################################
# Install

INSTALL(TARGETS classifierplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR}
)

INSTALL(TARGETS classifier
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
  LIBRARY DESTINATION ${QGIS_BIN_DIR}
)

if (WIN32)
  get_filename_component(OpenCV_BUILD_DIR ${OpenCV_LIB_DIR_OPT} DIRECTORY)
  file(GLOB OpenCV_DLLs 
    ${OpenCV_BUILD_DIR}/bin/opencv_core[0-9][0-9][0-9][0-9].dll
    ${OpenCV_BUILD_DIR}/bin/opencv_core[0-9][0-9][0-9][0-9].dll
    ${OpenCV_BUILD_DIR}/bin/opencv_highgui[0-9][0-9][0-9][0-9].dll
    ${OpenCV_BUILD_DIR}/bin/opencv_imgproc[0-9][0-9][0-9][0-9].dll
    ${OpenCV_BUILD_DIR}/bin/opencv_legacy[0-9][0-9][0-9][0-9].dll
    ${OpenCV_BUILD_DIR}/bin/opencv_ml[0-9][0-9][0-9][0-9].dll
  )

  install(FILES ${OpenCV_DLLs} DESTINATION ${QGIS_BIN_DIR})
endif()