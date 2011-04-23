/***************************************************************************
  classifierdialog.h
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

#ifndef CLASSIFIERDIALOG_H
#define CLASSIFIERDIALOG_H

#include <QDialog>

#include "qgisinterface.h"

#include "ui_classifierdialogbase.h"

class ClassifierDialog : public QDialog, private Ui::ClassifierDialogBase
{
    Q_OBJECT
  public:
    ClassifierDialog( QWidget* parent, QgisInterface* iface  );
    ~ClassifierDialog();

  private:
    static const int context_id = 0;

  private slots:
    void selectOutputFile();
    void doClassification();
    void updateInputRasters();
    //void updateFirstFileName();
    //void updateSecondFileName();
    void toggleCheckBoxState( bool checked );
    //void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    //void on_buttonBox_helpRequested();

  private:
    QgisInterface* mIface;
    QStringList mInputRasters;
    //QString mFirstRaster;
    //QString mSecondRaster;
    QString mOutputFileName;

    //QStringList* getVectorLayerNames();
    //QStringList* getRasterLayerNames();
    QgsVectorLayer* vectorLayerByName( const QString& name );
    QgsRasterLayer* rasterLayerByName( const QString& name );

    QgsVectorLayer* pointsFromPolygons( QgsVectorLayer* polygonLayer, double* geoTransform, const QString& layerName );
    
    void singleRasterClassification( const QString& rasterFileName );
    void multipleRastersClassification();

    void mapToPixel( double mX, double mY, double* geoTransform, double& outX, double& outY );
    void pixelToMap( double pX, double pY, double* geoTransform, double& outX, double& outY );
    void applyGeoTransform( double inX, double inY, double* geoTransform, double& outX, double& outY );
    void invertGeoTransform( double* inGeoTransform, double* outGeoTransform);

    void applyRasterStyle( QgsRasterLayer* layer );

    void manageGui();
    void enableOrDisableOkButton();
};

#endif // CLASSIFIERDIALOG_H
