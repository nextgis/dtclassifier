/***************************************************************************
  main.cpp
  Raster classification using decision tree
  -------------------
  begin                : Jun 16, 2016
  copyright            : (C) 2016 by Alexander Lisovenko
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
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <QString>
#include <QTextStream>

#include "classifierworker.h"
#include "main_application.h"

#define VERSION "0.0.2"

void usage()
{
  std::cout << "classifier version "<< VERSION << " " << std::endl
            << " Usage: classifier [options]"
              << " [--input_rasters rast1 [rast2, ...]]"
              << " [--presence vect1 [vect2, ...] --absence vect1 [vect2, ...]]" 
              << " [--classify output]" << std::endl
            << "  " << "options:" << std::endl
            << "    " << "[--decision_tree]\tUse decision tree" << std::endl
            << "    " << "[--discrete_classes]\tOutput values are discrete class labels" << std::endl
            << "    " << "[--generalize kernel_size]\tGeneralize resut using kernel size" << std::endl
            << "    " << "[--save_train_layer output]\tCan be used with --classify to save model" << std::endl
            << "    " << "[--save_model output]\tCan be used with --classify and --save_train_layer to save train layer" << std::endl
            << "    " << "[--use_model model_filename]\tUse existing model. Ignore --presence --absence --use_train_layer options" << std::endl
            << "    " << "[--use_train_layer shape_file]\tLoad point layer (train laier). Ignore --presence --absence and --input_rasters if --classify not set" << std::endl
            << "\n Usage examples:" << std::endl
            << "  " << "Classify:" << std::endl
            << "    " << "classifier --input_rasters rast1 [rast2, ...] --presence vect1 [vect2, ...] --absence vect1 [vect2, ...] --classify result.tiff" << std::endl
            << "\n  " << "Classify using a previously saved model:" << std::endl
            << "    " << "classifier --input_rasters rast1 [rast2, ...] --use_model model.yaml --classify result.tiff" << std::endl
            << "\n  " << "Create model only:" << std::endl
            << "    " << "classifier --input_rasters rast1 [rast2, ...] --presence vect1 [vect2, ...] --absence vect1 [vect2, ...] --save_model model.yaml" << std::endl
            << "\n  " << "Create model only using a previously saved train layer:" << std::endl
            << "    " << "classifier --use_train_layer train_layer.shp --save_model model.yaml" << std::endl
            << "\n  " << "Create train layer only:" << std::endl
            << "    " << "classifier --input_rasters rast1 [rast2, ...] --presence vect1 [vect2, ...] --absence vect1 [vect2, ...] --use_train_layer train_layer.shp" << std::endl
            ;
}

void printError(const std::string& msg)
{
  std::cerr << std::endl << msg << std::endl << std::endl;
}

void fileExistValidate(const std::string& fname)
{
  // bool ifExist = ( std::ifstream(fname)!= NULL);
  if (!std::ifstream(fname))
  {
    printError("File not found: " + fname);
    usage();
    exit(1);
  }
}

int main( int argc,
          char *argv[],
          char *envp[] )
{
    ClassifierWorkerConfig config;

    std::string curent_argument = std::string("");

    for( size_t count = 1; count < argc; count++ )
    {
      std::string argument = std::string(argv[count]);
      if (argument == std::string("--help"))
      {
        usage();
        return 0;   
      }
      else if (argument == std::string("--input_rasters"))
      {
        curent_argument = std::string("--input_rasters");
        continue;
      }
      else if (argument == std::string("--presence"))
      {
        curent_argument = std::string("--presence");
        continue;
      }
      else if (argument == std::string("--absence"))
      {
        curent_argument = std::string("--absence");
        continue;
      }
      else if (argument == std::string("--classify"))
      {
        config.mOutputRaster = QString(argv[count+1]);
        count++;
        continue;
      }
      else if (argument == std::string("--save_model"))
      {
        config.mOutputModel = QString(argv[count+1]);
        count++;
        continue;
      }
      else if (argument == std::string("--save_train_layer"))
      {
        config.mOutputTrainLayer = QString(argv[count+1]);
        count++;
        continue;
      }
      // else if (argument == std::string("--save_points"))
      // {
      //   config.save_points_layer_to_disk = true;
      //   continue;
      // }
      else if (argument == std::string("--decision_tree"))
      {
        config.use_decision_tree = true;
        continue;
      }
      else if (argument == std::string("--discrete_classes"))
      {
        config.discrete_classes = true;
        continue;
      }
      else if (argument == std::string("--generalize"))
      {
        config.do_generalization = true;
        config.kernel_size = QString(argv[count+1]).toInt();
        count++;
        continue;
      }
      else if (argument == std::string("--use_model"))
      {
        config.mInputModel = QString(argv[count+1]);
        count++;
        continue;
      }
      else if (argument == std::string("--use_train_layer"))
      {
        config.mInputPoints = QString(argv[count+1]);
        count++;
        continue;
      }
      else
      {
        if (argument.find('-') == 0)
        {
          printError("Unknown option: " + argument);
          usage();
          return 1;
        }
      }

      if (curent_argument == std::string("--input_rasters"))
      {
        fileExistValidate(argv[count]);
        config.mInputRasters << QString(argv[count]);
        continue;
      }
      else if (curent_argument == std::string("--presence"))
      {
        fileExistValidate(argv[count]);
        config.mPresence << QString(argv[count]);
        continue;
      }
      else if (curent_argument == std::string("--absence"))
      {
        fileExistValidate(argv[count]);
        config.mAbsence << QString(argv[count]);
        continue;
      }

      printError("Bad options!");
      usage();
      return 1;
    }

    // ------- Validation ---------------------------
    if (config.mOutputRaster.isEmpty() && config.mOutputModel.isEmpty() && config.mOutputTrainLayer.isEmpty())
    {
        printError("At least one of the arguments (save_train_layer, save_model, classify) must be specified");
        usage();
        return 1;   
    }
    if (config.mInputRasters.size() == 0 && (config.mInputPoints.isEmpty() && !config.mOutputRaster.isEmpty()))
    {
        printError("Must specify at least one input raster");
        usage();
        return 1;
    }
    if (!config.mInputModel.isEmpty())
    {
      fileExistValidate(config.mInputModel.toStdString());
    }
    if (!config.mInputPoints.isEmpty())
    {
      fileExistValidate(config.mInputPoints.toStdString());
    }
    
    // ------- Print input parameters ---------------------------
    std::cout << "Classification arguments" << std::endl;
    std::cout << "\tInput rasters:" << std::endl;
    for(size_t i =0; i < config.mInputRasters.size(); i++)
    {
        std::cout << "\t\t" << config.mInputRasters.at(i).toStdString() << std::endl;
    }
    std::cout << "\tPresence:" << std::endl;
    for(size_t i =0; i < config.mPresence.size(); i++)
    {
        std::cout << "\t\t" << config.mPresence.at(i).toStdString() << std::endl;
    }
    std::cout << "\tAbsence:" << std::endl;
    for(size_t i =0; i < config.mAbsence.size(); i++)
    {
        std::cout << "\t\t" << config.mAbsence.at(i).toStdString() << std::endl;
    }

    // ------- Run application ---------------------------
    ClassifierApplication a(argc,argv);
    a.initQgis();
    
    ClassifierWorker* worker = new ClassifierWorker(config);

    a.connect( worker, SIGNAL( stepCount(int) ), &a, SLOT( setStepCount(int) ) );
    a.connect( worker, SIGNAL( subStepCount(int) ), &a, SLOT( setSubStepCount(int) ) );
    a.connect( worker, SIGNAL( progressStep(int) ), &a, SLOT( showNextStep(int) ) );
    a.connect( worker, SIGNAL( progressSubStep(int) ), &a, SLOT( showNextSubStep(int) ) );
    a.connect( worker, SIGNAL( errorOccured(QString) ), &a, SLOT( showError(QString) ) );
    a.connect( worker, SIGNAL( finished() ), &a, SLOT( showFinish() ) );
    
    worker->process();

    a.exitQgis();
    return 0;
}