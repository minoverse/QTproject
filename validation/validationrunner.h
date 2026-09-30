#ifndef VALIDATIONRUNNER_H
#define VALIDATIONRUNNER_H

#include <QString>
#include <QVector>

#include "model/fivenodeparameters.h"

struct ValidationConnection
{
    int from;
    int to;
    double weight;
    QString function;
};

class ValidationRunner
{
public:
    static void configureFiveNodePreset(FiveNodeParameters& params,
                                        double alpha2);

    static QVector<ValidationConnection> fiveNodeConnections();
};

#endif // VALIDATIONRUNNER_H
