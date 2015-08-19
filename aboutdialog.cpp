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
  QString version = QString("%2: <strong>%3</strong>").arg(tr("Version"), _version);
  QString developers = QString("%1: <a href=\"%3\"> %2 </a>").arg(tr("Developers"), _developers_name,_developers_site);
  QString homepage = QString("%1: <a href=\"%2\"> %2 </a>").arg(tr("Homepage"), _home_page);
  QString bagtraking = QString("%1 <a href=\"%3\"> %2 </a>").arg(tr("Please report bugs at"), tr("bugtracker"), _bagtracker_page);

  QString html("");
  html += QString("<h2> %1 </h2>").arg(title);
  html += QString("<div> %1 </div>").arg(version);
  html += QString("<div> %1 </div>").arg(_description);
  html += QString("<div> %1 </div>").arg(_ext_description);
  html += QString("<br/><div> %1 </div>").arg(developers);
  html += QString("<div> %1 </div>").arg(homepage);
  html += QString("<div> %1 </div>").arg(bagtraking);

  lAboutText->setText(html);

  this->setWindowTitle(title);

  lIcon->setPixmap(QPixmap(_icon_48x48));
}