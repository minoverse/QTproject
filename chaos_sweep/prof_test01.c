#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define NROWS  10000    /* number of rows in the file */
#define NCOL   5        /* number of columns in the file */
#define COL    0        /* analyzed column (0 = first) */
#define NC     100      /* number of random c values */

static int cmp_dbl(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;

    return (x > y) - (x < y);
}

static double *read_column(const char *fname, long skip, long *n_out) {
    FILE *f = fopen(fname, "r");
    if (!f) {
        perror(fname);
        return NULL;
    }

    long n = NROWS - skip;

    double *x = malloc(n * sizeof *x);
    if (x) {
        double v[NCOL];

        for (long row = 0; row < NROWS; row++) {
            if (fscanf(f, "%lf %lf %lf %lf %lf", &v[0], &v[1], &v[2], &v[3], &v[4]) != NCOL) {
                free(x);
                fclose(f);

                return NULL;
            }

            if (row >= skip)
                x[row - skip] = v[COL];
        }
        fclose(f);
        *n_out = n;
    }

    return x;
}

static void compute_pq(const double *phi, long N, double c, double *p, double *q){
    double sp = 0.0, sq = 0.0;
    for (long j = 0; j < N; j++) {
        double jc = (double)(j + 1) * c;

        sp += phi[j] * cos(jc);
        sq += phi[j] * sin(jc);

        p[j] = sp;
        q[j] = sq;
    }
}

static double compute_Kc(const double *phi, long N, long ncut, double c, double Ephi2, double *p, double *q, double *D){

    compute_pq(phi, N, c, p, q);

    for (long n = 1; n <= ncut; n++) {
        double s = 0.0;
        long m = N - n;
        for (long j = 0; j < m; j++) {
            double dp = p[j + n] - p[j];
            double dq = q[j + n] - q[j];

            s += dp * dp + dq * dq;
        }

        double M = s / (double) m;
        double Vosc = Ephi2 * (1.0 - cos(n * c)) / (1.0 - cos(c));

        D[n - 1] = M - Vosc;
    }

    double mn = 0.0, mD = 0.0;
    for (long n = 1; n <= ncut; n++) {
        mn += n;
        mD += D[n - 1];
    }

    mn /= ncut;
    mD /= ncut;

    double cov = 0.0, vn = 0.0, vD = 0.0;

    for (long n = 1; n <= ncut; n++) {
        double a = n - mn;
        double b = D[n - 1] - mD;

        cov += a * b;
        vn += a * a;
        vD += b * b;
    }

    if (vD <= 0.0)
        return 0.0;

    return cov / sqrt(vn * vD);
}

int main(int argc, char **argv){

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <skip> [pq] [file]\n", argv[0]);
        return 1;
    }

    long skip = atol(argv[1]);

    int want_pq = 0;

    const char *fname = "result_a2_3.000000.dat";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "pq") == 0)
            want_pq = 1;
        else
            fname = argv[i];
    }

    long N;
    double *x = read_column(fname, skip, &N);
    if (!x)
        return 1;

    double mean = 0.0;
    for (long j = 0; j < N; j++)
        mean += x[j];
    mean /= N;

    for (long j = 0; j < N; j++)
        x[j] -= mean;

    double *phi = x;

    double Ephi = 0.0;

    for (long j = 0; j < N; j++)
        Ephi += phi[j];

    Ephi /= N;

    double Ephi2 = Ephi * Ephi;

    long ncut = N / 10;

    double *p = malloc(N * sizeof *p);
    double *q = malloc(N * sizeof *q);
    double *D = malloc(ncut * sizeof *D);
    if (p == NULL || q == NULL || D == NULL) {
        fprintf(stderr, "Memory allocation error (p, q or D)\n");
        free(p);
        free(q);
        free(D);
        free(x);
        return 1;
    }

    double cs[NC], Kc[NC], Ks[NC];

    srand(1); /* fixed seed for reproducible K and P-Q */

    for (int k = 0; k < NC; k++) {
        double r = (double)rand() / ((double)RAND_MAX + 1.0);

        cs[k] = M_PI / 5.0 + r * (3.0 * M_PI / 5.0);   /* c in (pi/5, 4pi/5) */

        Kc[k] = compute_Kc(phi, N, ncut, cs[k], Ephi2, p, q, D);
        Ks[k] = Kc[k];
    }

    qsort(Ks, NC, sizeof *Ks, cmp_dbl);
    double K = 0.5 * (Ks[NC / 2 - 1] + Ks[NC / 2]);
    printf("file           : %s\n", fname);
    printf("skipped        : %ld\n", skip);
    printf("N              : %ld\n", N);
    printf("ncut           : %ld\n", ncut);
    printf("K (median)     : %.6f\n", K);
    printf("K min / max    : %.6f / %.6f\n", Ks[0], Ks[NC - 1]);

    FILE *fk = fopen("Kc.dat", "w");
    if (fk) {
        fprintf(fk, "# c  K_c\n");

        for (int k = 0; k < NC; k++)
            fprintf(fk, "%.10f %.10f\n", cs[k], Kc[k]);

        fclose(fk);
    }

    if (want_pq) {
        int best = 0;
        for (int k = 1; k < NC; k++)
            if (fabs(Kc[k] - K) < fabs(Kc[best] - K))
                best = k;

        compute_pq(phi, N, cs[best], p, q);

        FILE *fo = fopen("pq.dat", "w");
        if (!fo)
            perror("pq.dat");
        else {
            fprintf(fo, "# c = %.10f  K_c = %.6f  K = %.6f\n", cs[best], Kc[best], K);
            fprintf(fo, "# n  p(n)  q(n)\n");

            for (long j = 0; j < N; j++)
                fprintf(fo, "%ld %.10e %.10e\n", j + 1, p[j], q[j]);

            fclose(fo);

            printf("pq.dat         : c = %.6f, K_c = %.6f\n", cs[best], Kc[best]);
        }
    }

    free(x);
    free(p);
    free(q);
    free(D);

    return 0;
}
