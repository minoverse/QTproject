//Compilation 
//gcc solve_dwa.c -O2 -lgsl -lm

#include <stdio.h>
#include <math.h>

#include <stdio.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv2.h>

struct rparams {
  double s12,s13,s14;
  double s21,s23,s25;
  double s31,s32,s33;
  double s41,s52;
  double a1,a2,a3;
};

int func (double t, const double y[], double f[], void *params){
	(void)(t); /* avoid unused parameter warning */
	double s12 = ((struct rparams *) params)->s12;
	double s13 = ((struct rparams *) params)->s13;
	double s14 = ((struct rparams *) params)->s14;
  
	double s21 = ((struct rparams *) params)->s21;
	double s23 = ((struct rparams *) params)->s23;
	double s25 = ((struct rparams *) params)->s25;
  
	double s31 = ((struct rparams *) params)->s31;
	double s32 = ((struct rparams *) params)->s32;
	double s33 = ((struct rparams *) params)->s33;
  
	double s41 = ((struct rparams *) params)->s41;
	double s52 = ((struct rparams *) params)->s52;

	double a1 = ((struct rparams *) params)->a1;
	double a2 = ((struct rparams *) params)->a2;
	double a3 = ((struct rparams *) params)->a3;
	
	f[0] = -y[0] + s12*tanh(y[1]) + s13*sin(y[2]) + s14*sin(y[3]);
	f[1] = -y[1] + s21*sin(y[0])  + s23*sin(y[2]) + s25*sin(y[4]);
	f[2] = -y[2] + s31*tanh(y[0]) + s32*tanh(y[1]) + s33*sin(y[2]);
	f[3] = -y[3] + s41*tanh(y[0]) + (a2-a3*sin(y[4]))*tanh(y[3]);
	f[4] = -y[4] + s52*tanh(y[1]) + (1-a1*tanh(y[2]))*tanh(y[4]);
	
	return GSL_SUCCESS;
}

int jac (double t, const double y[], double *dfdy, double dfdt[], void *params){
	(void)(t); /* avoid unused parameter warning */
	double s12 = ((struct rparams *) params)->s12;
	double s13 = ((struct rparams *) params)->s13;
	double s14 = ((struct rparams *) params)->s14;
  
	double s21 = ((struct rparams *) params)->s21;
	double s23 = ((struct rparams *) params)->s23;
	double s25 = ((struct rparams *) params)->s25;
  
	double s31 = ((struct rparams *) params)->s31;
	double s32 = ((struct rparams *) params)->s32;
	double s33 = ((struct rparams *) params)->s33;
  
	double s41 = ((struct rparams *) params)->s41;
	double s52 = ((struct rparams *) params)->s52;

	double a1 = ((struct rparams *) params)->a1;
	double a2 = ((struct rparams *) params)->a2;
	double a3 = ((struct rparams *) params)->a3;

	gsl_matrix_view dfdy_mat = gsl_matrix_view_array (dfdy, 5, 5);
	gsl_matrix *m = &dfdy_mat.matrix;
	
	gsl_matrix_set (m, 0, 0, -1.0 );
	gsl_matrix_set (m, 0, 1, s12/(cosh(y[1])*cosh(y[1])) );
	gsl_matrix_set (m, 0, 2, s13*cos(y[2]) );
	gsl_matrix_set (m, 0, 3, s14*cos(y[3]) );
	gsl_matrix_set (m, 0, 4, 0.0 );

	gsl_matrix_set (m, 1, 0, s21*cos(y[0]) );
	gsl_matrix_set (m, 1, 1, -1.0 );
	gsl_matrix_set (m, 1, 2, s23*cos(y[2]) );
	gsl_matrix_set (m, 1, 3, 0.0);
	gsl_matrix_set (m, 1, 4, s25*cos(y[4]) );
	
	gsl_matrix_set (m, 2, 0, s31/(cosh(y[0])*cosh(y[0])) );
	gsl_matrix_set (m, 2, 1, s32/(cosh(y[1])*cosh(y[1])) );
	gsl_matrix_set (m, 2, 2, -1.0 + s33*cos(y[2]) );
	gsl_matrix_set (m, 2, 3, 0.0);
	gsl_matrix_set (m, 2, 4, 0.0);
	
	gsl_matrix_set (m, 3, 0, s41/(cosh(y[0])*cosh(y[0])) );
	gsl_matrix_set (m, 3, 1, 0.0 );
	gsl_matrix_set (m, 3, 2, 0.0 );
	gsl_matrix_set (m, 3, 3, -1.0 + (a2 - a3*sin(y[4]))/(cosh(y[3])*cosh(y[3])) );
	gsl_matrix_set (m, 3, 4, -a3*cos(y[4])*tanh(y[3]) );

	gsl_matrix_set (m, 4, 0, 0.0);
	gsl_matrix_set (m, 4, 1, s52/(cosh(y[1])*cosh(y[1])) );
	gsl_matrix_set (m, 4, 2,-a1*tanh(y[4])/(cosh(y[2])*cosh(y[2])) );
	gsl_matrix_set (m, 4, 3, 0.0);
	gsl_matrix_set (m, 4, 4, -1.0 + (1.0-a1*tanh(y[2]))/(cosh(y[4])*cosh(y[4])));
	
	dfdt[0] = 0.0;
	dfdt[1] = 0.0;
	dfdt[2] = 0.0;
	dfdt[3] = 0.0;
	dfdt[4] = 0.0;
	
	return GSL_SUCCESS;
}

int main (void) {
	//..........266_11.......
	double s12=-0.3, s13=-0.8, s14=-0.6;
	double s21=-3.0, s23=2.0, s25=0.4;
	double s31=1.7, s32=-0.4, s33=3.0;
	double s41=0.7, s52=1.7;
  
	double a1=-2.2, a3=1.2;

	double a2=2.23; 


		struct rparams p = {s12, s13, s14, s21, s23, s25, s31, s32, s33, s41, s52, a1, a2, a3};

		gsl_odeiv2_system sys = {func, jac, 5, &p};
		gsl_odeiv2_driver * d = gsl_odeiv2_driver_alloc_y_new (&sys, gsl_odeiv2_step_rk8pd, 1e-6, 1e-6, 0.0);
  
		double t = 0.0, t1 = 5000.0;
		double y[5] = { 0.8, 0.3, 0.4, 0.6, 0.7 };

		char fname[20];
		sprintf(fname, "alfa_%0.0f.dat", 10*a2);
		FILE *fp = fopen(fname, "w");
		if (fp){
			for (long int i = 1; i <= 250000; i++) {
				double ti = i * t1 / 250000.0;
				int status = gsl_odeiv2_driver_apply (d, &t, ti, y);
				if (status != GSL_SUCCESS) {
					printf ("error, return value=%d\n", status);
					break;
				}
				fprintf (fp, "%.5f %.5e %.5e %.5e %.5e %.5e\n", t, y[0], y[1], y[2], y[3], y[4]);
			}
			gsl_odeiv2_driver_free (d);
			fclose(fp);
	}
	return 0;
}
