
########################################################
# Files

SET (CLASSIFIER_SRCS
     classifier.cpp
     classifierdialog.cpp
     classifierutils.cpp
)

SET (CLASSIFIER_UIS
     classifierdialogbase.ui
)

SET (CLASSIFIER_MOC_HDRS
     classifier.h
     classifierdialog.h
)

SET (CLASSIFIER_RCCS  classifier.qrc)

########################################################
# Build

QT4_WRAP_UI (CLASSIFIER_UIS_H  ${CLASSIFIER_UIS})

QT4_WRAP_CPP (CLASSIFIER_MOC_SRCS  ${CLASSIFIER_MOC_HDRS})

QT4_ADD_RESOURCES(CLASSIFIER_RCC_SRCS ${CLASSIFIER_RCCS})

ADD_LIBRARY (classifierplugin MODULE
  ${CLASSIFIER_SRCS}
  ${CLASSIFIER_MOC_SRCS}
  ${CLASSIFIER_RCC_SRCS}
  ${CLASSIFIER_UIS_H}
)

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${GDAL_INCLUDE_DIR}
     ../../core/raster
     ../../core/renderer
     ../../core/symbology
     ../../core
     ../../gui
     ..
     .
     /usr/include/opencv2
)

TARGET_LINK_LIBRARIES(classifierplugin
  qgis_core
  qgis_gui
  /usr/lib/libopencv_core.so
  /usr/lib/libopencv_ml.so
  /usr/lib/libopencv_highgui.so
  /usr/lib/libopencv_imgproc.so
)


########################################################
# Install

INSTALL(TARGETS classifierplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR}
)
