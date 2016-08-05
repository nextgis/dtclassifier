#include <iostream>
#include <QString>

#include "main_application.h"

ClassifierApplication::ClassifierApplication(int & argc, char ** argv)
    :   QgsApplication(argc, argv, false),
        mTextStream(stdout)
{
    mutex = new QMutex();
}

ClassifierApplication::~ClassifierApplication()
{}

void ClassifierApplication::setStepCount(int stepCount)
{
    mutex->lock();
    mStepCount = stepCount;
    mutex->unlock();
}

void ClassifierApplication::setSubStepCount(int stepCount)
{
    mutex->lock();
    mSubStepCount = stepCount;
    mutex->unlock();
}

void ClassifierApplication::showNextStep(int step)
{
    mutex->lock();
    mStep = step;
    mCurrentSubProgress = -1;
    mutex->unlock();
}

void ClassifierApplication::showNextSubStep(int subStep)
{
    mutex->lock();
    mSubStep = subStep;
    int progress = 0;
    if (mSubStepCount != 0)
        progress = int(10 * mSubStep / mSubStepCount);

    if (mCurrentSubProgress < progress)
    {
        mCurrentSubProgress = progress;
        printProgress();
    }
    mutex->unlock();
}

void ClassifierApplication::printProgress()
{   
    std::cout << "step finished " << mStep << " from " << mStepCount << " (substep finished " << mSubStep << " from " << mSubStepCount << ")";
    if (mSubStep < mSubStepCount)
        std::cout << "\r";
    else
        std::cout << std::endl;
}

void ClassifierApplication::showFinish()
{   
    mutex->lock();
    std::cout << "Finish!" << std::endl;
    mutex->unlock();
}

void ClassifierApplication::showError(QString msg)
{
    mutex->lock();
    std::cout << "Exceeption occured: " << msg.toStdString() << std::endl;
    mutex->unlock();   
}