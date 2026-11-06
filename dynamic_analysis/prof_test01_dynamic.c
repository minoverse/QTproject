#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NROWS 10000
#define NCOL 5
#define COL 0
#define NC 100

static int cmp_dbl(
    const void *a,
    const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;

    return (x > y) - (x < y);
}

static double *read_column(
    const char *fname,
    long skip,
    long *n_out)
{
    FILE *f = fopen(fname, "r");

    if (!f) {
        perror(fname);
        return NULL;
    }

    if (skip < 0 || skip >= NROWS) {
        fclose(f);
        return NULL;
    }

    long n = NROWS - skip;

    double *x =
        malloc(n * sizeof *x);

    if (!x) {
        fclose(f);
        return NULL;
    }

    double v[NCOL];

    for (long row = 0;
         row < NROWS;
         row++) {

        if (fscanf(
                f,
                "%lf %lf %lf %lf %lf",
                &v[0],
                &v[1],
                &v[2],
                &v[3],
                &v[4]) != NCOL) {

            free(x);
            fclose(f);
            return NULL;
        }

        if (row >= skip)
            x[row - skip] = v[COL];
    }

    fclose(f);

    *n_out = n;

    return x;
}

static void compute_pq(
    const double *phi,
    long N,
    double c,
    double *p,
    double *q)
{
    double sp = 0.0;
    double sq = 0.0;

    for (long j = 0; j < N; j++) {

        double jc =
            (double)(j + 1) * c;

        sp += phi[j] * cos(jc);
        sq += phi[j] * sin(jc);

        p[j] = sp;
        q[j] = sq;
    }
}

static double compute_Kc(
    const double *phi,
    long N,
    long ncut,
    double c,
    double Ephi2,
    double *p,
    double *q,
    double *D)
{
    compute_pq(
        phi,
        N,
        c,
        p,
        q
    );

    for (long n = 1;
         n <= ncut;
         n++) {

        double s = 0.0;

        long m = N - n;

        for (long j = 0;
             j < m;
             j++) {

            double dp =
                p[j + n] - p[j];

            double dq =
                q[j + n] - q[j];

            s += dp * dp + dq * dq;
        }

        double M =
            s / (double)m;

        double Vosc =
            Ephi2 *
            (1.0 - cos(n * c)) /
            (1.0 - cos(c));

        D[n - 1] =
            M - Vosc;
    }

    double mn = 0.0;
    double mD = 0.0;

    for (long n = 1;
         n <= ncut;
         n++) {

        mn += n;
        mD += D[n - 1];
    }

    mn /= ncut;
    mD /= ncut;

    double cov = 0.0;
    double vn = 0.0;
    double vD = 0.0;

    for (long n = 1;
         n <= ncut;
         n++) {

        double a =
            n - mn;

        double b =
            D[n - 1] - mD;

        cov += a * b;
        vn += a * a;
        vD += b * b;
    }

    if (vD <= 0.0)
        return 0.0;

    return cov /
        sqrt(vn * vD);
}

int main(int argc, char **argv)
{
    if (argc < 3) {

        fprintf(
            stderr,
            "Usage: %s <skip> <file>\n",
            argv[0]
        );

        return 1;
    }

    long skip =
        atol(argv[1]);

    const char *fname =
        argv[2];

    long N;

    double *x =
        read_column(
            fname,
            skip,
            &N
        );

    if (!x)
        return 1;

    double mean = 0.0;

    for (long j = 0;
         j < N;
         j++)
        mean += x[j];

    mean /= N;

    for (long j = 0;
         j < N;
         j++)
        x[j] -= mean;

    double *phi = x;

    double Ephi = 0.0;

    for (long j = 0;
         j < N;
         j++)
        Ephi += phi[j];

    Ephi /= N;

    double Ephi2 =
        Ephi * Ephi;

    long ncut =
        N / 10;

    double *p =
        malloc(N * sizeof *p);

    double *q =
        malloc(N * sizeof *q);

    double *D =
        malloc(ncut * sizeof *D);

    if (!p || !q || !D) {

        free(x);
        free(p);
        free(q);
        free(D);

        return 1;
    }

    double cs[NC];
    double Kc[NC];
    double Ks[NC];

    /*
     * Fixed seed in THIS SEPARATE COPY ONLY.
     * Makes resume/repeated comparison reproducible.
     */
    srand(1);

    for (int k = 0;
         k < NC;
         k++) {

        double r =
            (double)rand() /
            ((double)RAND_MAX + 1.0);

        cs[k] =
            M_PI / 5.0 +
            r * (3.0 * M_PI / 5.0);

        Kc[k] =
            compute_Kc(
                phi,
                N,
                ncut,
                cs[k],
                Ephi2,
                p,
                q,
                D
            );

        Ks[k] = Kc[k];
    }

    qsort(
        Ks,
        NC,
        sizeof *Ks,
        cmp_dbl
    );

    double K =
        0.5 *
        (
            Ks[NC / 2 - 1] +
            Ks[NC / 2]
        );

    printf(
        "K (median)     : %.6f\n",
        K
    );

    printf(
        "K min / max    : %.6f / %.6f\n",
        Ks[0],
        Ks[NC - 1]
    );

    free(x);
    free(p);
    free(q);
    free(D);

    return 0;
}
