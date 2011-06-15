/***************************************************************************
  layerselector.cpp
  Raster classification using decision tree
  -------------------
  begin                : Jun 14, 2011
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

#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"

#include "layerselectordialog.h"

LayerSelectorDialog::LayerSelectorDialog( QWidget *parent, QStringList *layers )
    : QDialog( parent )
    , mLayers( layers )
{
  setupUi( this );

  // additional buttons
  //~ QPushButton *pb;
  //~ pb = new QPushButton( tr( "Select all" ) );
  //~ buttonBox->addButton( pb, QDialogButtonBox::ActionRole );
  //~ connect( pb, SIGNAL( clicked() ), this, SLOT( selectAll() ) );
//~
  //~ pb = new QPushButton( tr( "Clear selection" ) );
  //~ buttonBox->addButton( pb, QDialogButtonBox::ActionRole );
  //~ connect( pb, SIGNAL( clicked() ), this, SLOT( clearSelection() ) );

  connect( layersList, SIGNAL( itemSelectionChanged() ), this, SLOT( updateSelectedLayers() ) );

  populateLayers();
}

void LayerSelectorDialog::on_buttonBox_accepted()
{
  accept();
}

void LayerSelectorDialog::on_buttonBox_rejected()
{
  reject();
}

void LayerSelectorDialog::populateLayers()
{
  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

  for ( ; layer_it != mapLayers.end(); ++layer_it )
  {
    if ( layer_it.value()->type() == QgsMapLayer::VectorLayer )
    {
      layersList->addItem( new QListWidgetItem( layer_it.value()->name() ) );
    }
  }
}

void LayerSelectorDialog::updateSelectedLayers()
{
  QList<QListWidgetItem *> selection = layersList->selectedItems();

  mLayers->clear();

  for ( int i = 0; i < selection.size(); ++i )
  {
    // write file path instead of layer name ?
    mLayers->append( selection.at( i )->text() );
  }
}

//~ void LayerSelectorDialog::selectAll()
//~ {
  //~ layersList->selectAll();
//~ }
//~
//~ void LayerSelectorDialog::clearSelection()
//~ {
  //~ layersList->clearSelection();
//~ }
