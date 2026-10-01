#ifndef RESULTWRITER_H
#define RESULTWRITER_H

#include <QString>
#include <QVector>

class ResultWriter
{
public:
    static bool writeResults(const QString& runDir,
                             const QVector<QVector<double>>& y,
                             int steps);
};

#endif // RESULTWRITER_H
