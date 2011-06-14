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
#include <QProcess>

#include "qgisinterface.h"

#include "rasterfileinfo.h"

#include "ui_classifierdialogbase.h"

class GDALDataset;

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
    void updateStepProgress();
    void toggleDiscreteLabelsCheckBoxState( bool checked );
    void toggleKernelSizeSpinState( int state );
    //void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    //void on_buttonBox_helpRequested();

  private:
    QgisInterface* mIface;
    QStringList mInputRasters;
    QString mOutputFileName;

    RasterFileInfo mFileInfo;

    QgsVectorLayer* extractPoints( QgsVectorLayer* polygonLayer, GDALDataset* inRaster, const QString& layerName );

    void rasterClassification2( const QString& rasterFileName );

    QString createSingleBandRaster();

    void applyRasterStyle( QgsRasterLayer* layer );
    void smoothRaster( const QString& path );

    void manageGui();
    void enableOrDisableOkButton();
};

#endif // CLASSIFIERDIALOG_H
