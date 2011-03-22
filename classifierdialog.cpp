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
#include <QFileDialog>
#include <QMessageBox>

#include "qgscontexthelp.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"

#include "classifierdialog.h"

ClassifierDialog::ClassifierDialog( QWidget* parent, QgisInterface* iface )
    : QDialog( parent ),
    mIface( iface )
{
  setupUi( this );
}

ClassifierDialog::~ClassifierDialog()
{
}

void ClassifierDialog::on_buttonBox_accepted()
{
  accept();
}

/*
void ClassifierDialog::on_buttonBox_rejected()
{
  reject();
}

void ClassifierDialog::on_buttonBox_helpRequested()
{
  QgsContextHelp::run( context_id );
}
*/

QgsVectorLayer* ClassifierDialog::vectorLayerByName( const QString& name )
{
  return 0;
}

void ClassifierDialog::enableOrDisableOkButton()
{
  bool enabled = true;
  buttonBox->button( QDialogButtonBox::Ok )->setEnabled( enabled );
}
