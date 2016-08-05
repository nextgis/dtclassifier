#ifndef CLASSIFIEAPPLICATION_H
#define CLASSIFIEAPPLICATION_H

#include <QCoreApplication>
#include <QTextStream>
#include <QMutex>

#include <qgsapplication.h>

class ClassifierApplication : public QgsApplication
{
    Q_OBJECT
    public:
        ClassifierApplication(int & argc, char ** argv);
        ~ClassifierApplication();

    public slots:
    	void setStepCount(int stepCount);
        void showNextStep(int step);
        void setSubStepCount(int stepCount);
        void showNextSubStep(int subbStep);
        void showError(QString msg);
        void showFinish();
    
    private:
    	int mStepCount;
    	int mSubStepCount;
    	int mStep;
    	int mSubStep;
    	int mCurrentSubProgress;

    	QMutex* mutex;
    	QTextStream mTextStream;

    	void printProgress();
};
#endif //CLASSIFIEAPPLICATION_H