#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gsl/gsl_sf_gamma.h>

#define N 5

// === Global Config (can be overridden)
int OMEGA = 1000;

// === Fractional Derivative Weight using Gamma
double BG(int om, int r, double ni) {
    return gsl_sf_gamma(om - r + ni) / gsl_sf_gamma(om - r + 1) / gsl_sf_gamma(ni);
}

// === G1 = 1 - a1 * tanh(y)
double G1(double y, double a1) {
    return 1.0 - a1 * tanh(y);
}

// === G2 = a2 - a3 * sin(y)
double G2(double y, double a2, double a3) {
    return a2 - a3 * sin(y);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <tmax> <input_file>\n", argv[0]);
        return 1;
    }

    OMEGA = atoi(argv[1]);  // dynamic time steps

    FILE *inp = fopen(argv[2], "r");
    if (!inp) {
        perror("Failed to open input file");
        return 1;
    }

    double a1, a3;
    fscanf(inp, "%lf %lf", &a1, &a3);

    double s[N][N] = {{0}};
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            fscanf(inp, "%lf", &s[i][j]);
    fclose(inp);

    double ni = 0.7;
    double f1 = 0.0, f3 = 0.0;

    double *y0 = calloc(OMEGA + 1, sizeof(double));
    double *y1 = calloc(OMEGA + 1, sizeof(double));
    double *y2 = calloc(OMEGA + 1, sizeof(double));
    double *y3 = calloc(OMEGA + 1, sizeof(double));
    double *y4 = calloc(OMEGA + 1, sizeof(double));

    y0[0] = 0.8; y1[0] = 0.3; y2[0] = 0.4; y3[0] = 0.6; y4[0] = 0.7;

    for (int om = 1; om <= OMEGA; om++) {
        for (int r = 1; r <= om; r++) {
            double bg = BG(om, r, ni);
            y0[om] += (-y0[r-1] + s[0][1]*tanh(y1[r-1]) + s[0][2]*sin(y2[r-1]) + s[0][3]*sin(y3[r-1])) * bg;
            y1[om] += (-y1[r-1] + s[1][0]*sin(y0[r-1]) + s[1][2]*sin(y2[r-1]) + s[1][4]*sin(y4[r-1]) + f1) * bg;
            y2[om] += (-y2[r-1] + s[2][0]*tanh(y0[r-1]) + s[2][1]*tanh(y1[r-1]) + s[2][2]*sin(y2[r-1])) * bg;
            y3[om] += (-y3[r-1] + s[3][0]*tanh(y0[r-1]) + G2(y4[r-1], OMEGA/1000.0, a3) * tanh(y3[r-1]) + f3) * bg;
            y4[om] += (-y4[r-1] + s[4][1]*tanh(y1[r-1]) + G1(y2[r-1], a1) * tanh(y4[r-1])) * bg;
        }

        y0[om] += y0[0];
        y1[om] += y1[0];
        y2[om] += y2[0];
        y3[om] += y3[0];
        y4[om] += y4[0];
    }

    FILE *out = fopen("result.dat", "w");
    if (!out) {
        perror("Cannot write result.dat");
        return 1;
    }
    for (int i = 0; i <= OMEGA; i++)
        fprintf(out, "%.10lf %.10lf %.10lf %.10lf %.10lf\n", y0[i], y1[i], y2[i], y3[i], y4[i]);
    fclose(out);

    free(y0); free(y1); free(y2); free(y3); free(y4);
    return 0;
}

