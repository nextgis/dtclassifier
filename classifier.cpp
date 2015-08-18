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
#include <qgsmessagelog.h>

#include "classifier.h"
#include "classifierdialog.h"
#include "aboutdialog.h"

static const QString _name = QObject::tr( "DTclassifier" );
static const QString _description = QObject::tr( "Raster classification using decision tree" );
static const QString _category = QObject::tr( "Raster" );
static const QString _version = QString( "0.4.1" );
static const QString _icon = ":/classifier/icons/classifier.png";
static const QString _icon_48x48 = ":/classifier/icons/classifier_48x48.png";

Classifier::Classifier( QgisInterface* iface ):
	QgisPlugin( _name, _description, _category, _version, QgisPlugin::UI ),
	mIface( iface ),
	mActionClassify( 0 ),
	mActionAbout( 0 )
{
}

Classifier::~Classifier()
{
}

void Classifier::initGui()
{
  mActionClassify = new QAction( QIcon( ":/classifier/icons/classifier.png" ), tr( "DTclassifier" ), this );
  mActionClassify->setWhatsThis( tr( "Raster classification using decision tree" ) );
  connect( mActionClassify, SIGNAL( triggered() ), this, SLOT( showMainDialog() ) );

  mActionAbout = new QAction( QIcon( ":/classifier/icons/about.png" ), tr( "About" ), this );
  mActionAbout->setWhatsThis( tr( "About DTclassifier" ) );
  connect( mActionAbout, SIGNAL( triggered() ), this, SLOT( showAboutDialog() ) );

  // Add the icon to the toolbar
  mIface->addToolBarIcon( mActionClassify );
  mIface->addPluginToMenu( tr( "DTclassifier" ), mActionClassify );
  mIface->addPluginToMenu( tr( "DTclassifier" ), mActionAbout );
}

void Classifier::help()
{
  // TODO: implement me!
}

void Classifier::showMainDialog()
{
  ClassifierDialog dlg( 0, mIface );
  dlg.exec();
}

void Classifier::showAboutDialog()
{
  AboutDialog dlg(0);
  dlg.exec();
}

void Classifier::unload()
{
  mIface->removePluginMenu( "DTclassifier", mActionClassify );
  mIface->removePluginMenu( "DTclassifier", mActionAbout );
  mIface->removeToolBarIcon( mActionClassify );
  delete mActionClassify;
  delete mActionAbout;
}

QGISEXTERN QgisPlugin* classFactory( QgisInterface * theQgisInterfacePointer )
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

QGISEXTERN QString category()
{
  return _category;
}

QGISEXTERN QString version()
{
  return _version;
}

QGISEXTERN QString icon()
{
  return _icon;
}

QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
