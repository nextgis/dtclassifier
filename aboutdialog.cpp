/***************************************************************************
  aboutdialog.h
  Raster classification using decision tree
  -------------------
  begin                : Aug 18, 2015
  copyright            : (C) 2015 by Alexander Lisovenko
  email                : alexander.lisovenko@gmail.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QPixmap>
#include <QLayout>

#include "aboutdialog.h"

AboutDialog::AboutDialog( QWidget *parent)
    : QDialog( parent )
{
  setupUi( this );
  
  verticalLayout->setSizeConstraint(QLayout::SetFixedSize);

  QString title = QString("%1 %2").arg(tr("About")).arg(_name);
  this->setWindowTitle(title);
  
  lIcon->setPixmap(QPixmap(_icon_48x48));
  
  QString html = QString("<div> <h2>%1</h2> </div> <div> %2: <strong>%3</strong></div><div> %4</div> <br/>").arg(title).arg(tr("Version")).arg(_version).arg(_description);
  lAboutText->setText(html);
}