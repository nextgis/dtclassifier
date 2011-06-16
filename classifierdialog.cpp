/***************************************************************************
  classifierdialog.cpp
  Raster classification using decision tree
  -------------------
  begin                : Mar 22, 2011
  copyright            : (C) 2011 by Alexander Bruy
  email                : alexander.bruy@gmail.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QColor>
#include <QApplication>
#include <QList>
#include <QDir>

#include <cmath>

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h"

#include "opencv2/core/core_c.h"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include "qgscontexthelp.h"
#include "qgsgeometry.h"
#include "qgsmaplayerregistry.h"
#include "qgspoint.h"
#include "qgsrasterlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgsvectorfilewriter.h"

#include "classifierdialog.h"
#include "layerselectordialog.h"
#include "classifierutils.h"

ClassifierDialog::ClassifierDialog( QWidget* parent, QgisInterface* iface )
    : QDialog( parent )
    , mIface( iface )
{
  setupUi( this );

  manageGui();

  // need this for working with rasters
  GDALAllRegister();

  connect( btnMultiPresence, SIGNAL( clicked() ), this, SLOT( selectLayers() ) );
  connect( btnMultiAbsence, SIGNAL( clicked() ), this, SLOT( selectLayers() ) );
  connect( btnOutputFile, SIGNAL( clicked() ), this, SLOT( selectOutputFile() ) );
  connect( rastersList, SIGNAL( itemSelectionChanged() ), this, SLOT( updateInputRasters() ) );
  connect( rbDecisionTree, SIGNAL( toggled( bool ) ), this, SLOT( toggleDiscreteLabelsCheckBoxState( bool ) ) );
  connect( generalizeCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( toggleKernelSizeSpinState( int ) ) );

  // use Ok button for starting classification
  disconnect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( doClassification() ) );
}

ClassifierDialog::~ClassifierDialog()
{
}

void ClassifierDialog::selectLayers()
{
  QString senderName = sender()->objectName();

  LayerSelectorDialog dlg( this );

  if ( senderName == "btnMultiPresence" )
  {
    if ( btnMultiPresence->isChecked() )
    {
      cmbPresenceLayer->setEnabled( false );
      dlg.setLayerList( &mPresenceLayers );
    }
    else
    {
      cmbPresenceLayer->setEnabled( true );
      mPresenceLayers.clear();
      return;
    }
  }
  else
  {
    if ( btnMultiAbsence->isChecked() )
    {
      cmbAbsenceLayer->setEnabled( false );
      dlg.setLayerList( &mAbsenceLayers );
    }
    else
    {
      cmbAbsenceLayer->setEnabled( true );
      mAbsenceLayers.clear();
      return;
    }
  }
  dlg.exec();
}

void ClassifierDialog::selectOutputFile()
{
  // get last used directory
  QSettings settings( "NextGIS", "DTclassifier" );
  QString lastDir;

  lastDir = settings.value( "lastUsedDir", "." ).toString();

  QString fileName = QFileDialog::getSaveFileName( this, tr( "Select output file" ), lastDir, "GeoTiff (*.tif *.tiff *.TIF *.TIFF)" );

  if ( fileName.isEmpty() )
  {
    return;
  }

  // ensure the user never ommited the extension from the file name
  if ( !fileName.toLower().endsWith( ".tif" ) && !fileName.toLower().endsWith( ".tiff" ) )
  {
    fileName += ".tif";
  }

  mOutputFileName = fileName;
  leOutputRaster->setText( mOutputFileName );

  // save last used directory
  settings.setValue( "lastUsedDir", QFileInfo( fileName ).absolutePath() );

  enableOrDisableOkButton();
  qDebug() << "OutFileName" << mOutputFileName;
}

void ClassifierDialog::doClassification()
{
  // save checkboxes state
  QSettings settings( "NextGIS", "DTclassifier" );

  settings.setValue( "discreteClasses", discreteLabelsCheckBox->isChecked() );
  settings.setValue( "addToCanvas", addToCanvasCheckBox->isChecked() );
  settings.setValue( "saveTempLayers", savePointLayersCheckBox->isChecked() );

  settings.setValue( "doGeneralization", generalizeCheckBox->isChecked() );
  settings.setValue( "kernelSize", spnKernelSize->value() );

  totalProgress->setFormat( "%p%" );
  totalProgress->setValue( 0 );

  if ( mInputRasters.count() == 1 )
  {
    totalProgress->setRange( 0, 5 );
    //~ qDebug() << "LAYERS PRESENCE" << mPresenceLayers.join( "\n" );
    //~ qDebug() << "LAYERS ABSENCE" << mAbsenceLayers.join( "\n" );
    QString inputRaster = rasterLayerByName( mInputRasters.at( 0 ) )->source();
    mFileInfo.initFromFileName( inputRaster );
    rasterClassification( inputRaster );
  }
  else
  {
    totalProgress->setRange( 0, 7 );
    QString inputRaster = createSingleBandRaster();
    mFileInfo.initFromFileName( inputRaster );
    rasterClassification( inputRaster );
    removeDirectory( QDir().tempPath() + "/dtclassifier" );
  }

  if ( generalizeCheckBox->isChecked() )
  {
    smoothRaster( mOutputFileName );
  }

  totalProgress->setFormat( "Done: %p%" );

  // add classified rasters to map canvas if requested
  if ( addToCanvasCheckBox->isChecked() )
  {
    QgsRasterLayer* newLayer;
    newLayer = new QgsRasterLayer( mOutputFileName, QFileInfo( mOutputFileName ).baseName() );
    applyRasterStyle( newLayer );
    QgsMapLayerRegistry::instance()->addMapLayer( newLayer );

    QFileInfo fi( mOutputFileName );
    QString smoothFileName = fi.absoluteDir().absolutePath() + "/" + fi.baseName() + "_smooth.tif";
    QgsRasterLayer* smoothLayer;
    smoothLayer = new QgsRasterLayer( smoothFileName, QFileInfo( smoothFileName ).baseName() );
    applyRasterStyle( smoothLayer );
    QgsMapLayerRegistry::instance()->addMapLayer( smoothLayer );
  }
}

//~ void ClassifierDialog::on_buttonBox_accepted()
//~ {
  //~ accept();
//~ }

void ClassifierDialog::on_buttonBox_rejected()
{
  reject();
}

//~ void ClassifierDialog::on_buttonBox_helpRequested()
//~ {
  //~ QgsContextHelp::run( context_id );
//~ }

QgsVectorLayer* ClassifierDialog::extractPoints( QgsVectorLayer* polygonLayer, GDALDataset* inRaster, const QString& layerName )
{
  // create memory layer
  QgsVectorLayer* pointsLayer = new QgsVectorLayer( "Point", layerName, "memory" );
  QgsVectorDataProvider *memoryProvider = pointsLayer->dataProvider();
  QgsVectorDataProvider *provider = polygonLayer->dataProvider();

  // add attributes to provider
  QList<QgsField> attrList;
  for ( int i = 0; i < mFileInfo.bandCount(); ++i )
  {
    QgsField* field = new QgsField( QString( "Band_%1").arg( i + 1 ), QVariant::Double );
    attrList.append( *field );
  }
  attrList.append( QgsField( "Class", QVariant::Int ) );

  bool isOk = memoryProvider->addAttributes( attrList );
  qDebug() << "added attributes" << isOk;
  qDebug() << "field count" << memoryProvider->fieldCount();

  // create points
  QgsFeature feat;
  QgsGeometry* geom;
  QgsRectangle bbox;
  double xMin, xMax, yMin, yMax;
  double startCol, startRow, endCol, endRow;
  double x, y;
  QgsPoint* pnt = new QgsPoint();
  QgsFeature* ft;
  QgsFeatureList lstFeatures;

  QVector<float> rasterData( mFileInfo.xSize() * mFileInfo.bandCount() );

  provider->rewind();
  provider->select();

  stepProgress->setRange( 0, provider->featureCount() );
  stepProgress->setValue( 0 );
  stepProgress->setFormat( "Generate points: %p%" );

  while ( provider->nextFeature( feat ) )
  {
    geom = feat.geometry();
    bbox = geom->boundingBox();

    xMin = bbox.xMinimum();
    xMax = bbox.xMaximum();
    yMin = bbox.yMinimum();
    yMax = bbox.yMaximum();

    mFileInfo.mapToPixel( xMin, yMax, startRow, startCol );
    mFileInfo.mapToPixel( xMax, yMin, endRow, endCol );

    for ( int row = startRow; row < endRow + 1; row++ )
    {
      for ( int col = startCol; col < endCol + 1; col++ )
      {
        // create point and test
        mFileInfo.pixelToMap( row - 0.5, col - 0.5, x, y );
        pnt->setX( x );
        pnt->setY( y );
        if ( geom->contains( pnt ) )
        {
          ft = new QgsFeature();
          ft->setGeometry( QgsGeometry::fromPoint( *pnt ) );
          // get pixel value
          inRaster->RasterIO( GF_Read, row - 0.5, col - 0.5, 1, 1, (void*)rasterData.data(), 1, 1, GDT_Float32, mFileInfo.bandCount(), 0, 0, 0, 0 );
          for ( int i = 0; i < mFileInfo.bandCount(); ++i )
          {
            ft->addAttribute( i, QVariant( (double)rasterData[ i ] ) );
          }
          ft->addAttribute( mFileInfo.bandCount(), QVariant( 1 ) );
          lstFeatures.append( *ft );
        }
      }
    }
    // update progress and process messages
    stepProgress->setValue( stepProgress->value() + 1 );
    QApplication::processEvents();
  }
  // write to memory layer
  memoryProvider->addFeatures( lstFeatures );
  pointsLayer->updateExtents();
  // workaround to save added fetures
  pointsLayer->startEditing();
  pointsLayer->commitChanges();

  stepProgress->setFormat( "%p%" );
  stepProgress->setRange( 0, 100 );
  stepProgress->setValue( 0 );

  return pointsLayer;
}

void ClassifierDialog::rasterClassification( const QString& rasterFileName )
{
  GDALDataset *inRaster;
  inRaster = (GDALDataset *) GDALOpen( rasterFileName.toUtf8(), GA_ReadOnly );
  qDebug() << "input raster opened";
  qDebug() << "Band count" << mFileInfo.bandCount();

  totalProgress->setValue( totalProgress->value() + 1 );

  // create layer and populate it with train points
  QgsVectorLayer* trainLayer = createTrainLayer();
  qDebug() << "TRAIN LAYER CREATED";

  if ( !btnMultiPresence->isChecked() )
  {
    mPresenceLayers.append( cmbPresenceLayer->currentText() );
  }
  if ( !btnMultiAbsence->isChecked() )
  {
    mAbsenceLayers.append( cmbAbsenceLayer->currentText() );
  }

  mergeLayers( trainLayer, mPresenceLayers, inRaster, 1 );
  mergeLayers( trainLayer, mAbsenceLayers, inRaster, 0 );
  qDebug() << "MERGE LAYERS OK";

  long featCount = trainLayer->featureCount();
  qDebug() << "Feature count" << featCount;

  // save train layer to disk if requested
  if ( savePointLayersCheckBox->isChecked() )
  {
    QgsMapLayerRegistry::instance()->addMapLayer( trainLayer );

    QFileInfo fi( mOutputFileName );
    QString vectorFilename;
    vectorFilename = fi.absoluteDir().absolutePath() + "/" + "train_points.shp";

    QgsCoordinateReferenceSystem destCRS;
    destCRS = trainLayer->crs();

    QgsVectorFileWriter::WriterError error;
    QString errorMessage;
    error = QgsVectorFileWriter::writeAsVectorFormat(
              trainLayer, vectorFilename, "System", &destCRS,
              "ESRI Shapefile",
              false,
              &errorMessage );

    if ( error != QgsVectorFileWriter::NoError )
    {
      QMessageBox::warning( this, "Save error", tr( "Export to vector file failed.\nError: %1" ).arg( errorMessage ) );
    }
  }

  // create output file
  GDALDriver *driver;
  driver = GetGDALDriverManager()->GetDriverByName( "GTiff" );
  GDALDataset *outRaster;
  outRaster = driver->Create( mOutputFileName.toUtf8(), mFileInfo.xSize(), mFileInfo.ySize(), 1, GDT_Byte, NULL );

  double geotransform[6];
  mFileInfo.geoTransform( geotransform );

  outRaster->SetGeoTransform( geotransform );
  outRaster->SetProjection( mFileInfo.projection().toUtf8() );
  qDebug() << "output raster created";

  // read train data from layers
  QgsFeature feat;
  int i = 0;

  QVector<float> rasterData( mFileInfo.xSize() * mFileInfo.bandCount() );

  CvMat* data = cvCreateMat( featCount, mFileInfo.bandCount(), CV_32F );
  CvMat* responses = cvCreateMat( featCount, 1, CV_32F );

  QgsVectorDataProvider *provider = trainLayer->dataProvider();
  QgsAttributeList attrList = provider->attributeIndexes();
  provider->rewind();
  provider->select( attrList );

  stepProgress->setValue( 0 );
  stepProgress->setFormat( "Collect train data: %p%" );
  stepProgress->setRange( 0, provider->featureCount() );

  totalProgress->setValue( totalProgress->value() + 1 );

  QgsAttributeMap atMap;
  int bc = mFileInfo.bandCount();

  while ( provider->nextFeature( feat ) )
  {
    atMap = feat.attributeMap();
    for (int j = 0; j < bc; j++)
    {
      cvmSet( data, i, j, atMap[ j ].toDouble() );
    }
    cvmSet( responses, i, 0, atMap[ bc ].toDouble() );
    i++;

    stepProgress->setValue( stepProgress->value() + 1 );
    QApplication::processEvents();
  }

  //~ cvSave( "/home/alex/data.yaml", data );
  //~ cvSave( "/home/alex/resp.yaml", responses );

  stepProgress->setValue( 0 );
  stepProgress->setFormat( "%p%" );
  stepProgress->setRange( 0, 100 );

  totalProgress->setValue( totalProgress->value() + 1 );

  CvDTree* dtree = new CvDTree();
  CvRTrees* rtree = new CvRTrees();
  // use decision tree
  if ( rbDecisionTree->isChecked() )
  {
    CvDTreeParams params( 8,     // max depth
                          10,    // min sample count
                          0,     // regression accuracy
                          true,  // use surrogates
                          10,    // max number of categories
                          10,    // prune tree with K fold cross-validation
                          false, // use 1 rule
                          false, // throw away the pruned tree branches
                          0      // the array of priors, the bigger p_weight, the more attention
                         );

    // build decision tree classifier
    if ( discreteLabelsCheckBox->isChecked() )
    {
      CvMat* var_type;
      var_type = cvCreateMat( data->cols + 1, 1, CV_8U );
      cvSet( var_type, cvScalarAll(CV_VAR_CATEGORICAL) );
      dtree->train( data, CV_ROW_SAMPLE, responses, 0, 0, var_type, 0, params );
      cvReleaseMat( &var_type );
    }
    else
    {
      dtree->train( data, CV_ROW_SAMPLE, responses, 0, 0, 0, 0, params );
    }

    QFileInfo fi( mOutputFileName );
    QString treeFileName;
    treeFileName = fi.absoluteDir().absolutePath() + "/" + fi.baseName() + "_tree.yaml";

    dtree->save( treeFileName.toUtf8(), "MyTree" );
  }
  else // or random trees
  {
    // build random trees classifier
    rtree->train( data, CV_ROW_SAMPLE, responses );
  }

  cvReleaseMat( &data );
  cvReleaseMat( &responses );

  // classify raster using tree
  CvMat* sample = cvCreateMat( mFileInfo.bandCount(), 1, CV_32F );
  QVector<unsigned char> outData( mFileInfo.xSize() );

  stepProgress->setFormat( "Classification: %p%" );
  stepProgress->setRange( 0, mFileInfo.ySize() );
  stepProgress->setValue( 0 );

  totalProgress->setValue( totalProgress->value() + 1 );

  for ( int row = 0; row < mFileInfo.ySize(); ++row )
  {
    inRaster->RasterIO( GF_Read, 0, row, mFileInfo.xSize(), 1, (void *)rasterData.data(), mFileInfo.xSize(), 1, GDT_Float32, mFileInfo.bandCount(), 0, 0, 0, 0 );
    for ( int col = 0; col < mFileInfo.xSize(); ++col )
    {
      for ( int j = 0; j < mFileInfo.bandCount(); j++)
      {
        cvmSet( sample, j, 0, rasterData[ mFileInfo.xSize() * j + col ] );
      }

      if ( rbDecisionTree->isChecked() )
      {
        outData[ col ] = (unsigned char)dtree->predict( sample )->value;
      }
      else
      {
        outData[ col ] = (unsigned char)rtree->predict( sample );
      }
    }
    outRaster->RasterIO( GF_Write, 0, row, mFileInfo.xSize(), 1, (void *)outData.data(), mFileInfo.xSize(), 1, GDT_Byte, 1, 0, 0, 0, 0 );
    stepProgress->setValue( stepProgress->value() + 1 );
    QApplication::processEvents();
  }

  totalProgress->setValue( totalProgress->value() + 1 );

  // cleanup
  stepProgress->setFormat( "%p%" );
  stepProgress->setRange( 0, 100 );
  stepProgress->setValue( 0 );

  cvReleaseMat( &sample );
  dtree->clear();
  delete dtree;
  rtree->clear();
  delete rtree;

  GDALClose( (GDALDatasetH) inRaster );
  GDALClose( (GDALDatasetH) outRaster );
}

void ClassifierDialog::manageGui()
{
  // restore ui state from settings
  QSettings settings( "NextGIS", "DTclassifier" );

  addToCanvasCheckBox->setChecked( settings.value( "addToCanvas", false ).toBool() );
  savePointLayersCheckBox->setChecked( settings.value( "saveTempLayers", false ).toBool() );

  generalizeCheckBox->setChecked( settings.value( "doGeneralization", false ).toBool() );
  spnKernelSize->setValue( settings.value( "kernelSize", 3 ).toInt() );
  if ( generalizeCheckBox->isChecked() )
  {
    spnKernelSize->setEnabled( true );
  }
  else
  {
    spnKernelSize->setEnabled( false );
  }

  // classification settings
  QString algorithm = settings.value( "classificationAlg", "dtree" ).toString();
  if ( algorithm == "dtree" )
  {
    rbDecisionTree->setChecked( true );
    discreteLabelsCheckBox->setEnabled( true );
  }
  else
  {
    rbRandomTrees->setChecked( true );
    discreteLabelsCheckBox->setEnabled( false );
  }
  discreteLabelsCheckBox->setChecked( settings.value( "discreteClasses", true ).toBool() );

  // populate vector layers comboboxes
  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  QgsRasterLayer* layer;

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::VectorLayer )
    {
      cmbPresenceLayer->addItem( layer_it.value()->name() );
      cmbAbsenceLayer->addItem( layer_it.value()->name() );
      //~ if ( qobject_cast<QgsVectorLayer *>( layer_it.value() )->geometryType() == QGis::Polygon )
      //~ {
        //~ cmbPresenceLayer->addItem( layer_it.value()->name() );
        //~ cmbAbsenceLayer->addItem( layer_it.value()->name() );
      //~ }
    }
    else if ( layer_it.value()->type() == QgsMapLayer::RasterLayer )
    {
      layer = qobject_cast<QgsRasterLayer *> ( layer_it.value() );
      if ( layer->usesProvider() && layer->providerKey() != "gdal" )
      {
        continue;
      }
      rastersList->addItem( new QListWidgetItem( layer_it.value()->name() ) );
    }
  }
}

void ClassifierDialog::toggleDiscreteLabelsCheckBoxState( bool checked )
{
  if ( checked )
  {
    discreteLabelsCheckBox->setEnabled( true );
  }
  else
  {
    discreteLabelsCheckBox->setEnabled( false );
  }
}

void ClassifierDialog::toggleKernelSizeSpinState( int state )
{
  if ( state == Qt::Checked )
  {
    spnKernelSize->setEnabled( true );
  }
  else
  {
    spnKernelSize->setEnabled( false );
  }
}

void ClassifierDialog::updateInputRasters()
{
  QList<QListWidgetItem *> selection = rastersList->selectedItems();

  mInputRasters.clear();

  for ( int i = 0; i < selection.size(); ++i )
  {
    // write file path instead of layer name ?
    mInputRasters.append( selection.at( i )->text() );
  }
}

void ClassifierDialog::enableOrDisableOkButton()
{
  bool enabled = true;

  if ( mOutputFileName.isEmpty() )
  {
    enabled = false;
  }

  buttonBox->button( QDialogButtonBox::Ok )->setEnabled( enabled );
}

void ClassifierDialog::applyRasterStyle( QgsRasterLayer* layer )
{
  // draw as singleBand image with ColorRampShader
  layer->setDrawingStyle( QgsRasterLayer::SingleBandPseudoColor );
  layer->setColorShadingAlgorithm( QgsRasterLayer::ColorRampShader );

  // create color ramp
  QgsColorRampShader* myRasterShaderFunction = ( QgsColorRampShader* )layer->rasterShader()->rasterShaderFunction();
  QList<QgsColorRampShader::ColorRampItem> myColorRampItems;

  QgsColorRampShader::ColorRampItem absenceItem, presenceItem;
  absenceItem.value = 0;
  absenceItem.color = QColor( Qt::white );
  absenceItem.label = "";

  presenceItem.value = 1;
  presenceItem.color = QColor( Qt::red );
  presenceItem.label = "";
  myColorRampItems.append( absenceItem );
  myColorRampItems.append( presenceItem );

  // sort the shader items
  qSort( myColorRampItems );
  myRasterShaderFunction->setColorRampItemList( myColorRampItems );

  // make 0 transparent
  layer->rasterTransparency()->initializeTransparentPixelList( 0.0 );
}

void ClassifierDialog::smoothRaster( const QString& path )
{
  CvMat* img = cvLoadImageM( path.toUtf8(), CV_LOAD_IMAGE_UNCHANGED );
  CvMat* outImg = cvCreateMat( img->rows, img->cols, CV_8UC1 );

  // TODO: use user defined kernel size
  cvSmooth( img, outImg, CV_MEDIAN );

/*
  int size = spnKernelSize->value();
  IplConvKernel* kernel = cvCreateStructuringElementEx( size * 2 + 1, size * 2 + 1, size, size, CV_SHAPE_RECT, 0 );

  cvDilate( img, img, kernel, 1 );

  cvReleaseStructuringElement( &kernel );
  size += 1;
  kernel = cvCreateStructuringElementEx( size * 2 + 1, size * 2 + 1, size, size, CV_SHAPE_RECT, 0 );
  cvErode( img, img, kernel, 1 );
  cvReleaseStructuringElement( &kernel );
*/

  QFileInfo fi( path );
  QString smoothFileName;
  smoothFileName = fi.absoluteDir().absolutePath() + "/" + fi.baseName() + "_smooth.tif";

  // create output file
  GDALDriver *driver;
  driver = GetGDALDriverManager()->GetDriverByName( "GTiff" );
  GDALDataset *outRaster;
  outRaster = driver->Create( smoothFileName.toUtf8(), mFileInfo.xSize(), mFileInfo.ySize(), 1, GDT_Byte, NULL );

  double geotransform[6];
  mFileInfo.geoTransform( geotransform );

  outRaster->SetGeoTransform( geotransform );
  outRaster->SetProjection( mFileInfo.projection().toUtf8() );

  outRaster->RasterIO( GF_Write, 0, 0, mFileInfo.xSize(), mFileInfo.ySize(), (void*)outImg->data.ptr, mFileInfo.xSize(), mFileInfo.ySize(), GDT_Byte, 1, 0, 0, 0, 0 );

  cvReleaseMat( &img );
  cvReleaseMat( &outImg );
  GDALClose( (GDALDatasetH) outRaster );
}

QString ClassifierDialog::createSingleBandRaster()
{
  QString layerName, layerPath;
  GDALDataset* raster;
  int bandCount, rasterCount;

  rasterCount = 1;

  // create dir for our temp files
  QString tempDir = QDir().tempPath() + "/dtclassifier";
  if ( !QDir().mkpath( tempDir ) )
  {
    qDebug() << "Can't create temporary directory" << tempDir;
  }

  QString templateName = tempDir + "/raster_";
  QString rasterName;

  QProcess* process = new QProcess();
  QString command = "gdal_translate";
  QStringList args;

  connect( process, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( updateStepProgress() ) );

  totalProgress->setValue( totalProgress->value() + 1 );

  // iterate over rasters
  for (int i = 0; i < mInputRasters.size(); ++i )
  {
    layerName = mInputRasters.at( i );
    layerPath = rasterLayerByName( layerName )->source();

    raster = (GDALDataset*) GDALOpen( layerPath.toUtf8(), GA_ReadOnly );
    bandCount = raster->GetRasterCount();
    GDALClose( (GDALDatasetH)raster );

    stepProgress->setValue( 0 );
    stepProgress->setFormat( "Preparing " + layerName + ": %p%" );
    stepProgress->setRange( 0, bandCount );

    // iterate over bands
    for ( int j = 0; j < bandCount; ++j )
    {
      rasterName = templateName + QString( "%1.tif" ).arg( rasterCount, 2, 10, QChar( 48 ) );

      args.clear();
      args << "-b" << QString::number( j + 1 ) << layerPath << rasterName;

      process->start( command, args, QIODevice::ReadOnly );
      if ( !process->waitForFinished( -1 ) )
      {
        qDebug() << "Failed to extract bands from raster" << layerPath;
        return QString();
      }
      rasterCount++;
    } //for j
  } //for i

  disconnect( process, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( updateStepProgress() ) );
  connect( process, SIGNAL( readyReadStandardOutput() ), this, SLOT( updateStepProgress() ) );

  stepProgress->setValue( 0 );
  stepProgress->setFormat( "Merge bands: %p%" );
  stepProgress->setRange( 0, 20 );

  // get raster files in temp dir
  QDir workDir( tempDir );
  workDir.setFilter( QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot );
  QStringList nameFilter( "*.tif" );
  workDir.setNameFilters( nameFilter );
  QStringList bands = workDir.entryList();
  bands.sort();

  rasterName = tempDir + "/output.img";

  // merge into single raster
  #ifdef Q_OS_WIN32
    command = "gdal_merge.bat";
  #else
    command = "gdal_merge.py";
  #endif
  args.clear();
  args << "-separate" << "-of" << "HFA" << "-o" << rasterName << bands;

  totalProgress->setValue( totalProgress->value() + 1 );

  process->setWorkingDirectory( tempDir );
  process->setReadChannel( QProcess::StandardOutput );
  process->start( command, args, QIODevice::ReadOnly );
  qDebug() << "command:" << command << args.join( " " );
  if ( !process->waitForFinished( -1 ) )
  {
    qDebug() << "Failed to merge bands into single raster";
    return QString();
  }

  stepProgress->setValue( 0 );
  stepProgress->setFormat( "%p%" );
  stepProgress->setRange( 0, 100 );

  return rasterName;
}

void ClassifierDialog::updateStepProgress()
{
  stepProgress->setValue( stepProgress->value() + 1 );
  QApplication::processEvents();
}

//////////////////////////////////////////////////
//
// helper methods

QgsVectorLayer* ClassifierDialog::createTrainLayer()
{
  // create memory layer
  QgsVectorLayer* vl = new QgsVectorLayer( "Point", "train_points", "memory" );
  QgsVectorDataProvider* provider = vl->dataProvider();

  // add attributes to provider
  QList<QgsField> attrList;
  for ( int i = 0; i < mFileInfo.bandCount(); ++i )
  {
    QgsField* field = new QgsField( QString( "Band_%1").arg( i + 1 ), QVariant::Double );
    attrList.append( *field );
  }
  attrList.append( QgsField( "Class", QVariant::Int ) );

  bool isOk = provider->addAttributes( attrList );
  qDebug() << "IN FUNCTION added attributes" << isOk;
  qDebug() << "IN FUNCTION field count" << provider->fieldCount();

  return vl;
}

void ClassifierDialog::mergeLayers( QgsVectorLayer *outLayer, const QStringList& layers, GDALDataset* raster, int layerType )
{
  QgsVectorLayer *vl;
  // iterate over layers
  for (int i = 0; i < layers.size(); ++i )
  {
    vl = vectorLayerByName( layers.at( i ) );
    switch ( vl->wkbType() )
    {
      // polygon
      case QGis::WKBPolygon:
      case QGis::WKBPolygon25D:
      {
        pointsFromPolygons( vl, outLayer, raster, layerType );
        break;
      }
      // point
      case QGis::WKBPoint:
      case QGis::WKBPoint25D:
      {
        copyPoints( vl, outLayer, raster, layerType );
        break;
      }
      // line
      case QGis::WKBLineString:
      case QGis::WKBLineString25D:
      {
        QgsVectorLayer* poly = createBuffer( vl );
        pointsFromPolygons( poly, outLayer, raster, layerType );
        delete poly;
        break;
      }
      default:
        break;
    } // switch
  }
}

void ClassifierDialog::pointsFromPolygons( QgsVectorLayer* src, QgsVectorLayer* dst, GDALDataset* raster, int layerType )
{
  QgsVectorDataProvider *srcProvider = src->dataProvider();
  QgsVectorDataProvider *dstProvider = dst->dataProvider();

  int bandCount = mFileInfo.bandCount();

  // create points
  QgsFeature feat;
  QgsFeature* newFeat;
  QgsGeometry* geom;
  QgsRectangle bbox;
  double xMin, xMax, yMin, yMax;
  double startCol, startRow, endCol, endRow;
  double x, y;
  QgsPoint* pnt = new QgsPoint();
  QgsFeatureList lstFeatures;

  QVector<float> rasterData( mFileInfo.xSize() * bandCount );

  srcProvider->rewind();
  srcProvider->select();

  while ( srcProvider->nextFeature( feat ) )
  {
    geom = feat.geometry();
    bbox = geom->boundingBox();

    xMin = bbox.xMinimum();
    xMax = bbox.xMaximum();
    yMin = bbox.yMinimum();
    yMax = bbox.yMaximum();

    mFileInfo.mapToPixel( xMin, yMax, startRow, startCol );
    mFileInfo.mapToPixel( xMax, yMin, endRow, endCol );

    for ( int row = startRow; row < endRow + 1; row++ )
    {
      for ( int col = startCol; col < endCol + 1; col++ )
      {
        // create point and test
        mFileInfo.pixelToMap( row - 0.5, col - 0.5, x, y );
        pnt->setX( x );
        pnt->setY( y );
        if ( geom->contains( pnt ) )
        {
          newFeat = new QgsFeature();
          newFeat->setGeometry( QgsGeometry::fromPoint( *pnt ) );
          // get pixel value
          raster->RasterIO( GF_Read, row - 0.5, col - 0.5, 1, 1, (void*)rasterData.data(), 1, 1, GDT_Float32, bandCount, 0, 0, 0, 0 );
          for ( int i = 0; i < bandCount; ++i )
          {
            newFeat->addAttribute( i, QVariant( (double)rasterData[ i ] ) );
          }
          newFeat->addAttribute( bandCount, QVariant( layerType ) );
          lstFeatures.append( *newFeat );
        }
      }
    }
    // update progress and process messages
    //~ stepProgress->setValue( stepProgress->value() + 1 );
    QApplication::processEvents();
  }
  // write to memory layer
  dstProvider->addFeatures( lstFeatures );
  dst->updateExtents();
  // workaround to save added fetures
  dst->startEditing();
  dst->commitChanges();
}

void ClassifierDialog::copyPoints( QgsVectorLayer* src, QgsVectorLayer* dst, GDALDataset* raster, int layerType )
{
  QgsVectorDataProvider* dstProvider = dst->dataProvider();
  QgsVectorDataProvider* srcProvider = src->dataProvider();

  int bandCount = mFileInfo.bandCount();

  QgsFeature inFeat;
  QgsFeature* outFeat;
  QgsGeometry* geom;
  QgsFeatureList lstFeatures;
  double col, row;

  QVector<float> rasterData( mFileInfo.xSize() * bandCount );

  srcProvider->rewind();
  srcProvider->select();
  while ( srcProvider->nextFeature( inFeat ) )
  {
    geom = inFeat.geometry();
    outFeat = new QgsFeature();
    outFeat->setGeometry( geom );

    mFileInfo.mapToPixel( geom->asPoint().x(), geom->asPoint().y(), row, col );
    raster->RasterIO( GF_Read, row - 0.5, col - 0.5, 1, 1, (void*)rasterData.data(), 1, 1, GDT_Float32, bandCount, 0, 0, 0, 0 );
    for ( int i = 0; i < bandCount; ++i )
    {
      outFeat->addAttribute( i, QVariant( (double)rasterData[ i ] ) );
    }
    outFeat->addAttribute( bandCount, QVariant( layerType ) );
    lstFeatures.append( *outFeat );
  }
  // write to memory layer
  dstProvider->addFeatures( lstFeatures );
  dst->updateExtents();
  // workaround to save added fetures
  dst->startEditing();
  dst->commitChanges();
}

QgsVectorLayer* ClassifierDialog::createBuffer( QgsVectorLayer* src )
{
  QgsVectorLayer* dst = new QgsVectorLayer( "Polygon", "buffer", "memory" );
  QgsVectorDataProvider* dstProvider = dst->dataProvider();
  QgsVectorDataProvider* srcProvider = src->dataProvider();

  double dist = mFileInfo.pixelSize() / 2;

  QgsFeature inFeat;
  QgsFeature* outFeat;
  QgsGeometry* inGeom;
  QgsGeometry* outGeom;
  QgsFeatureList lstFeatures;

  srcProvider->rewind();
  srcProvider->select();
  while ( srcProvider->nextFeature( inFeat ) )
  {
    inGeom = inFeat.geometry();
    outGeom = inGeom->buffer( dist, 5 );
    outFeat = new QgsFeature();
    outFeat->setGeometry( outGeom );
    lstFeatures.append( *outFeat );
  }
  // write to memory layer
  dstProvider->addFeatures( lstFeatures );
  dst->updateExtents();
  // workaround to save added fetures
  dst->startEditing();
  dst->commitChanges();

  return dst;
}
