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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#include "ui_about.h"
#include "classifier.h"

extern const QString _name;
extern const QString _version;
extern const QString _description;
extern const QString _ext_description;
extern const QString _icon_48x48;
extern const QString _developers_name;
extern const QString _developers_site;
extern const QString _home_page;
extern const QString _bagtracker_page;


class AboutDialog : public QDialog, private Ui::AboutDialog
{
    Q_OBJECT
  public:
    AboutDialog( QWidget* parent);

};

#endif // ABOUTDIALOG_H
