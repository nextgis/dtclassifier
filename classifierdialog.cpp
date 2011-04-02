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

#include <math.h>

#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h"

#include "opencv2/core/core_c.h"
#include "opencv2/ml/ml.hpp"

#include "qgscontexthelp.h"
#include "qgsgeometry.h"
#include "qgsmaplayerregistry.h"
#include "qgspoint.h"
#include "qgsrasterlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"

#include "classifierdialog.h"

ClassifierDialog::ClassifierDialog( QWidget* parent, QgisInterface* iface )
    : QDialog( parent ),
    mIface( iface )
{
  setupUi( this );

  manageGui();

  // need this for working with rasters
  GDALAllRegister();

  connect( btnOutputFile, SIGNAL( clicked() ), this, SLOT( selectOutputFile() ) );
  connect( cmbInputRaster, SIGNAL( currentIndexChanged( const QString& ) ), this, SLOT( updateInputFileName() ) );
  connect( rbDecisionTree, SIGNAL( toggled( bool ) ), this, SLOT( toggleCheckBoxState( bool ) ) );

  // use Ok button for starting classification
  disconnect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( doClassification() ) );
}

ClassifierDialog::~ClassifierDialog()
{
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
  //if ( !fileName.toLower().endsWith( ".tif" ) || !fileName.toLower().endsWith( ".tiff" ) )
  //{
  //  fileName += ".tif";
  //}

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

  mInputFileName = rasterLayerByName( cmbInputRaster->currentText() )->source();
  qDebug() << "InputFileName" << mInputFileName;

  QgsVectorLayer *layerPresence = vectorLayerByName( cmbPresenceLayer->currentText() );
  QgsVectorLayer *layerAbsence = vectorLayerByName( cmbAbsenceLayer->currentText() );

  long featCount = layerAbsence->featureCount() + layerPresence->featureCount();

  // read input raster metadata. We need them to create output raster
  GDALDataset *inRaster;
  inRaster = (GDALDataset *) GDALOpen( mInputFileName.toUtf8(), GA_ReadOnly );
  qDebug() << "input raster opened";

  double geotransform[6];
  int xSize, ySize, bandCount;
  xSize = inRaster->GetRasterXSize();
  ySize = inRaster->GetRasterYSize();
  bandCount = inRaster->GetRasterCount();
  inRaster->GetGeoTransform( geotransform );

  // create output file
  GDALDriver *driver;
  driver = GetGDALDriverManager()->GetDriverByName( "GTiff" );
  GDALDataset *outRaster;
  outRaster = driver->Create( mOutputFileName.toUtf8(), xSize, ySize, 1, GDT_Float32, NULL );
  outRaster->SetGeoTransform( geotransform );
  outRaster->SetProjection( inRaster->GetProjectionRef() );
  GDALRasterBand *outBand;
  outBand = outRaster->GetRasterBand( 1 );
  qDebug() << "output raster created";

  // collect train data
  double mapX, mapY;
  double pixX, pixY;
  int row, col;
  int i = 0;

  QgsFeature feat;
  QgsPoint pnt;

  QVector<float> rasterData( xSize * bandCount );

  CvMat* data = cvCreateMat( featCount, bandCount, CV_32F );
  CvMat* responses = cvCreateMat( featCount, 1, CV_32F );

  QgsVectorDataProvider *provider = layerPresence->dataProvider();
  provider->rewind();
  provider->select();

  while ( provider->nextFeature( feat ) )
  {
    pnt = feat.geometry()->asPoint();
    mapX = pnt.x();
    mapY = pnt.y();
    mapToPixel( mapX, mapY, geotransform, pixX, pixY);
    col = floor( pixX );
    row = floor( pixY );
    inRaster->RasterIO( GF_Read, col , row, 1, 1, (void*)rasterData.data(), 1, 1, GDT_Float32, bandCount, 0, 0, 0 , 0 );
    cvmSet( data, i, 0, rasterData[0] );
    cvmSet( data, i, 1, rasterData[1] );
    cvmSet( data, i, 2, rasterData[2] );
    cvmSet( data, i, 3, rasterData[3] );
    cvmSet( data, i, 4, rasterData[4] );
    cvmSet( data, i, 5, rasterData[5] );
    cvmSet( responses, i, 0, 1 );
    i++;
  }

  provider = layerAbsence->dataProvider();
  provider->rewind();
  provider->select();
  while ( provider->nextFeature( feat ) )
  {
    pnt = feat.geometry()->asPoint();
    mapX = pnt.x();
    mapY = pnt.y();
    mapToPixel( mapX, mapY, geotransform, pixX, pixY);
    col = floor( pixX );
    row = floor( pixY );
    inRaster->RasterIO( GF_Read, col , row, 1, 1, (void*)rasterData.data(), 1, 1, GDT_Float32, bandCount, 0, 0, 0 , 0 );
    cvmSet( data, i, 0, rasterData[0] );
    cvmSet( data, i, 1, rasterData[1] );
    cvmSet( data, i, 2, rasterData[2] );
    cvmSet( data, i, 3, rasterData[3] );
    cvmSet( data, i, 4, rasterData[4] );
    cvmSet( data, i, 5, rasterData[5] );
    cvmSet( responses, i, 0, 0 );
    i++;
  }

  CvDTree* dtree = new CvDTree();
  CvRTrees* rtree = new CvRTrees();
  // use decision tree
  if ( rbDecisionTree->isChecked() )
  {
    // build decision tree classifier
    if ( discreteLabelsCheckBox->isChecked() )
    {
      CvMat* var_type;
      var_type = cvCreateMat( data->cols + 1, 1, CV_8U );
      cvSet( var_type, cvScalarAll(CV_VAR_CATEGORICAL) );
      dtree->train( data, CV_ROW_SAMPLE, responses, 0, 0, var_type );
      cvReleaseMat( &var_type );
    }
    else
    {
      dtree->train( data, CV_ROW_SAMPLE, responses, 0, 0 );
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
  CvMat* sample = cvCreateMat( bandCount, 1, CV_32F );
  QVector<float> outData( xSize );

  progressBar->setRange( 0, ySize );
  progressBar->setValue( 0 );

  for ( int row = 0; row < ySize; ++row )
  {
    inRaster->RasterIO( GF_Read, 0, row, xSize, 1, (void *)rasterData.data(), xSize, 1, GDT_Float32, bandCount, 0, 0, 0 , 0 );
    for ( int col = 0; col < xSize; ++col )
    {
      cvmSet( sample, 0, 0, rasterData[ col ] );
      cvmSet( sample, 1, 0, rasterData[ xSize + col ] );
      cvmSet( sample, 2, 0, rasterData[ xSize*2 + col ] );
      cvmSet( sample, 3, 0, rasterData[ xSize*3 + col ] );
      cvmSet( sample, 4, 0, rasterData[ xSize*4 + col ] );
      cvmSet( sample, 5, 0, rasterData[ xSize*5 + col ] );

      if ( rbDecisionTree->isChecked() )
      {
        outData[ col ] = dtree->predict( sample )->value;
      }
      else
      {
        outData[ col ] = rtree->predict( sample );
      }
    }
    outRaster->RasterIO( GF_Write, 0, row, xSize, 1, (void *)outData.data(), xSize, 1, GDT_Float32, 1, 0, 0, 0 , 0 );
    progressBar->setValue( progressBar->value() + 1 );
  }

  // cleanup
  progressBar->setValue( 0 );
  cvReleaseMat( &sample );
  dtree->clear();
  delete dtree;
  rtree->clear();
  delete rtree;

  GDALClose( (GDALDatasetH) inRaster );
  GDALClose( (GDALDatasetH) outRaster );

  // add classified raster to map canvas if requested
  if ( addToCanvasCheckBox->isChecked() )
  {
    QgsRasterLayer* newLayer;
    newLayer = new QgsRasterLayer( mOutputFileName, QFileInfo( mOutputFileName ).baseName() );
    QgsMapLayerRegistry::instance()->addMapLayer( newLayer );
  }
}

/*
void ClassifierDialog::on_buttonBox_accepted()
{
  accept();
}
*/

void ClassifierDialog::on_buttonBox_rejected()
{
  reject();
}

/*
void ClassifierDialog::on_buttonBox_helpRequested()
{
  QgsContextHelp::run( context_id );
}

QStringList* ClassifierDialog::getVectorLayerNames()
{
  QStringList* layers = new QStringList();

  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::VectorLayer )
    {
      layers->append( layer_it.value()->name() );
    }
  }

  return layers;
}

QStringList* ClassifierDialog::getRasterLayerNames()
{
  QStringList* layers = new QStringList();

  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::RasterLayer )
    {
      if ( layer_it.value()->usesProvider() && layer_it.value()->providerKey() != "gdal" )
      {
        continue;
      }
      layers->append( layer_it.value()->name() );
    }
  }

  return layers;
}
*/

QgsVectorLayer* ClassifierDialog::vectorLayerByName( const QString& name )
{
  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::VectorLayer && layer_it.value()->name() == name )
    {
      return qobject_cast<QgsVectorLayer *>( layer_it.value() );
    }
  }

  return 0;
}

QgsRasterLayer* ClassifierDialog::rasterLayerByName( const QString& name )
{
  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::RasterLayer )
    {
      if ( qobject_cast<QgsRasterLayer *>( layer_it.value() )->usesProvider() && qobject_cast<QgsRasterLayer *>( layer_it.value() )->providerKey() != "gdal" )
      {
        continue;
      }
      return qobject_cast<QgsRasterLayer *>( layer_it.value() );
    }
  }

  return 0;
}

void ClassifierDialog::manageGui()
{
  // restore ui state from settings
  QSettings settings( "NextGIS", "DTclassifier" );

  addToCanvasCheckBox->setChecked( settings.value( "addToCanvas", false ).toBool() );

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

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::VectorLayer )
    {
      if ( qobject_cast<QgsVectorLayer *>( layer_it.value() )->geometryType() == QGis::Point )
      {
        cmbPresenceLayer->addItem( layer_it.value()->name() );
        cmbAbsenceLayer->addItem( layer_it.value()->name() );
      }
    }
    else if ( layer_it.value()->type() == QgsMapLayer::RasterLayer )
    {
      if ( qobject_cast<QgsRasterLayer *>( layer_it.value() )->usesProvider() && qobject_cast<QgsRasterLayer *>( layer_it.value() )->providerKey() != "gdal" )
      {
        continue;
      }
      cmbInputRaster->addItem( layer_it.value()->name() );
    }
  }
}

void ClassifierDialog::toggleCheckBoxState( bool checked )
{
  //if ( rbDecisionTree->isChecked() )
  if ( checked )
  {
    discreteLabelsCheckBox->setEnabled( true );
  }
  else
  {
    discreteLabelsCheckBox->setEnabled( false );
  }
}

void ClassifierDialog::updateInputFileName()
{
  mInputFileName = rasterLayerByName( cmbInputRaster->currentText() )->source();
  qDebug() << "InputFileName" << mInputFileName;
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

void ClassifierDialog::mapToPixel( double mX, double mY, double* geoTransform, double& outX, double& outY )
{
  if ( geoTransform[ 2 ] + geoTransform[ 4 ] == 0 )
  {
    outX = (int)( mX - geoTransform[ 0 ] ) / geoTransform[ 1 ];
    outY = (int)( mY - geoTransform[ 3 ] ) / geoTransform[ 5 ];
  }
  else
  {
    double invGeoTransform [ 6 ] = { 0, 0, 0, 0, 0, 0 };
    invertGeoTransform( geoTransform, invGeoTransform );
    applyGeoTransform( mX, mY, invGeoTransform, outX, outY );
  }
}

void ClassifierDialog::pixelToMap( double pX, double pY, double* geoTransform, double& outX, double& outY )
{
  applyGeoTransform( pX, pY, geoTransform, outX, outY );
}

void ClassifierDialog::applyGeoTransform( double inX, double inY, double* geoTransform, double& outX, double& outY )
{
  outX = geoTransform[ 0 ] + inX * geoTransform[ 1 ] + inY * geoTransform[ 2 ];
  outY = geoTransform[ 3 ] + inX * geoTransform[ 4 ] + inY * geoTransform[ 5 ];
}

void ClassifierDialog::invertGeoTransform( double* inGeoTransform, double* outGeoTransform )
{
  double det = inGeoTransform[ 1 ] * inGeoTransform[ 5 ] - inGeoTransform[ 2 ] * inGeoTransform[ 4 ];

  if ( fabs( det ) < 0.000000000000001 )
  {
    return;
  }

  double invDet = 1.0 / det;

  outGeoTransform[ 1 ] = inGeoTransform[ 5 ] * invDet;
  outGeoTransform[ 4 ] = -inGeoTransform[ 4 ] * invDet;

  outGeoTransform[ 2 ] = -inGeoTransform[ 2 ] * invDet;
  outGeoTransform[ 5 ] = inGeoTransform[ 1 ] * invDet;

  outGeoTransform[ 0 ] = ( inGeoTransform[ 2 ] * inGeoTransform[ 3 ] - inGeoTransform[ 0 ] * inGeoTransform[ 5 ] ) * invDet;
  outGeoTransform[ 3 ] = ( -inGeoTransform[ 1 ] * inGeoTransform[ 3 ] + inGeoTransform[ 0 ] * inGeoTransform[ 4 ] ) * invDet;
}
