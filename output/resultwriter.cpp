#include "resultwriter.h"

#include <QFile>
#include <QTextStream>

bool ResultWriter::writeResults(const QString& runDir,
                                const QVector<QVector<double>>& y,
                                int steps)
{
    const QString prefix = runDir.isEmpty() ? QString() : runDir + "/";

    QFile f(prefix + "result.dat");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(17);

    for (int t = 0; t <= steps; ++t) {
        out << y[0][t] << " "
            << y[1][t] << " "
            << y[2][t] << " "
            << y[3][t] << " "
            << y[4][t] << "\n";
    }
    f.close();

    QFile stream(prefix + "result_stream.csv");
    if (stream.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&stream);
        s << "t,y1,y2,y3,y4,y5\n";

        for (int t = 0; t <= steps; ++t) {
            s << t << ","
              << y[0][t] << ","
              << y[1][t] << ","
              << y[2][t] << ","
              << y[3][t] << ","
              << y[4][t] << "\n";
        }
    }

    QFile fin(prefix + "result_final.csv");
    if (fin.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&fin);
        s << "y1,y2,y3,y4,y5\n";
        s << y[0][steps] << ","
          << y[1][steps] << ","
          << y[2][steps] << ","
          << y[3][steps] << ","
          << y[4][steps] << "\n";
    }

    QFile table(prefix + "table.txt");
    if (table.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream t(&table);
        t << "Output Table (result.dat)\n";
        t << "Rows: " << (steps + 1) << "\n\n";
        t << "y1 y2 y3 y4 y5\n";

        for (int i = 0; i <= steps; ++i) {
            t << y[0][i] << " "
              << y[1][i] << " "
              << y[2][i] << " "
              << y[3][i] << " "
              << y[4][i] << "\n";
        }
    }

    return true;
}
