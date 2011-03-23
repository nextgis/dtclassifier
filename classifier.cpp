/***************************************************************************
  classifier.cpp
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

#include <QAction>
#include <QToolBar>

#include <qgisinterface.h>
#include <qgisgui.h>

#include "classifier.h"
#include "classifierdialog.h"

static const QString _name = QObject::tr( "Classifier" );
static const QString _description = QObject::tr( "Raster classification using decision tree" );
static const QString _version = QObject::tr( "Version 0.1.0" );
static const QString _icon = ":/classifier/icons/classifier.png";

Classifier::Classifier( QgisInterface* iface )
    : mIface( iface ), mActionClassify( 0 ), mActionAbout( 0 )
{
}

Classifier::~Classifier()
{
}

void Classifier::initGui()
{
  mActionClassify = new QAction( QIcon( ":/classifier/icons/classifier.png" ), tr( "Classifier" ), this );
  mActionClassify->setWhatsThis( tr( "Raster classification using decision tree" ) );
  connect( mActionClassify, SIGNAL( triggered() ), this, SLOT( showMainDialog() ) );

  mActionAbout = new QAction( QIcon( ":/classifier/icons/about.png" ), tr( "About" ), this );
  mActionAbout->setWhatsThis( tr( "About Classifier" ) );
  connect( mActionAbout, SIGNAL( triggered() ), this, SLOT( showAboutDialog() ) );

  // Add the icon to the toolbar
  mIface->addToolBarIcon( mActionClassify );
  mIface->addPluginToMenu( tr( "Classifier" ), mActionClassify );
  mIface->addPluginToMenu( tr( "Classifier" ), mActionAbout );
}

void Classifier::help()
{
  // implement me!
}

void Classifier::showMainDialog()
{
  ClassifierDialog dlg( 0, mIface );
  dlg.exec();
}

void Classifier::showAboutDialog()
{
  // implement me!
}

void Classifier::unload()
{
  mIface->removePluginMenu( "Classifier", mActionClassify );
  mIface->removePluginMenu( "Classifier", mActionAbout );
  mIface->removeToolBarIcon( mActionClassify );
  delete mActionClassify;
  delete mActionAbout;
}

QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new Classifier( theQgisInterfacePointer );
}

QGISEXTERN QString name()
{
  return _name;
}

QGISEXTERN QString description()
{
  return _description;
}

QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

QGISEXTERN QString version()
{
  return _version;
}

QGISEXTERN QString icon()
{
  return _icon;
}

QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
