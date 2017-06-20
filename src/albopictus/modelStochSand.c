#ifdef R#define modelHeader#include "R.h"#include "Rmath.h"#else#include "math.h"#endif#include <time.h>#include <stdio.h>#include <stdlib.h>#include <string.h>#include <gsl/gsl_rng.h>#include <gsl/gsl_randist.h>#include "gamma.h"#include "spop.h"#include "ran_gen.h"#define max(a,b) ((a)>(b)?(a):(b))#define min(a,b) ((a)<(b)?(a):(b))#define ZERO    0extern gsl_rng *RAND_GSL;// --------------------------------------------// Gamma distribution// --------------------------------------------char gamma_mode = MODE_GAMMA_HASH;void set_gamma_mode(char mode) {  gamma_mode = mode;}// --------------------------------------------// Implementation of the dynamics// --------------------------------------------double *param;#define NumPar      30#define NumMet      4#define init_E      0#define init_L      1#define init_P      2#define init_A      3#define param_d1_1  4#define param_d1_2  5#define param_d1_3  6#define param_d2_1  7#define param_d2_2  8#define param_d2_3  9#define param_d3_1  10#define param_d3_2  11#define param_d3_3  12#define param_d4_1  13#define param_p1_1  14#define param_p1_2  15#define param_p1_3  16#define param_p2_1  17#define param_p3_1  18#define param_p3_2  19#define param_p3_3  20#define param_p4_1  21#define param_p4_2  22#define param_p4_3  23#define param_F4_1  24#define param_sex   25#define param_PP_ta 26#define param_PP_hu 27#define param_PP_d2 28#define param_PP_p2 29void numparModel(int *np, int *nm) {  *np = NumPar;  *nm = NumMet;}void param_model(char **names, double *par) {  char temp[NumMet+NumPar][256] = {    "colnvE","colnvL","colnvP","colnvA",    "init_E","init_L","init_P","init_A","param_d1_1","param_d1_2","param_d1_3","param_d2_1","param_d2_2","param_d2_3","param_d3_1","param_d3_2","param_d3_3","param_d4_1","param_p1_1","param_p1_2","param_p1_3","param_p2_1","param_p3_1","param_p3_2","param_p3_3","param_p4_1","param_p4_2","param_p4_3","param_F4_1","param_sex","param_PP_ta","param_PP_hu","param_PP_d2","param_PP_p2"  };  int i;  for (i=0; i<(NumMet+NumPar); i++)    names[i] = strdup(temp[i]);  // ---  //   par[init_E]      = 100; // initial number of egg  par[init_L]      = 0; // initial number of larva  par[init_P]      = 0; // initial number of pupa  par[init_A]      = 0; // initial number of adult females  par[param_d1_1]  = 0.03;  par[param_d1_2]  = 35.0;  par[param_d1_3]  = 5.0;  par[param_d2_1]  = 0.01;  par[param_d2_2]  = 30.0;  par[param_d2_3]  = 18.0;  par[param_d3_1]  = 0.1;  par[param_d3_2]  = 32.0;  par[param_d3_3]  = 5.0;  par[param_d4_1]  = 6.0;  par[param_p1_1]  = -0.0014;  par[param_p1_2]  = 22.0;  par[param_p1_3]  = 0.98;  par[param_p2_1]  = 0.98;  par[param_p3_1]  = -0.000275;  par[param_p3_2]  = 18.0;  par[param_p3_3]  = 0.995;  par[param_p4_1]  = 0.8;  par[param_p4_2]  = 20.25;  par[param_p4_3]  = 0.4;  par[param_F4_1]  = 80.0; // eggs per female in 4-8 days  par[param_sex]   = 0.4;  par[param_PP_ta] = 19.81;  par[param_PP_hu] = 0.7;  par[param_PP_d2] = 1000.0;  par[param_PP_p2] = 0.99;}// --------------------------------------------------------------------void fun_devsur(double temp,                double hum,                double *par,                double *vec) {  // Diapause flag  char diapause = temp < par[param_PP_ta] || hum < par[param_PP_hu];  // Calculate development and survival times  /* d1 */ vec[0] = temp < par[param_d1_2] ? par[param_d1_1]*pow(temp-par[param_d1_2],2)+par[param_d1_3] : par[param_d1_3];  /* d2 */ vec[1] = diapause ? par[param_PP_d2] : (temp < par[param_d2_2] ? par[param_d2_1]*pow(temp-par[param_d2_2],4)+par[param_d2_3] : par[param_d2_3]);  /* d3 */ vec[2] = temp < par[param_d3_2] ? par[param_d3_1]*pow(temp-par[param_d3_2],2)+par[param_d3_3] : par[param_d3_3];  /* d4 */ vec[3] = par[param_d4_1];  //  /* p1 */ vec[4] = 1.0-(temp < 11.6 ? 0.0 : max(0,min(par[param_p1_1]*pow(temp-par[param_p1_2],2)+par[param_p1_3],1.0)));  /* p2 */ vec[5] = 1.0-(diapause ? par[param_PP_p2] : par[param_p2_1]);  /* p3 */ vec[6] = 1.0-(temp < par[param_p3_2] ? 0.0 : max(0,min(par[param_p3_1]*pow(temp-par[param_p3_2],2)+par[param_p3_3],1.0)));  /* p4 */ vec[7] = 1.0-(temp < par[param_p4_2] ? par[param_p4_3] : par[param_p4_1]);  //  /* F4 */ vec[8] = par[param_F4_1];}// --------------------------------------------------------------------void calculateSand(double mean_air_temp,                   double daily_humidity,                   spop   *vecE,                   spop   *vecL,                   spop   *vecP,                   spop   *vecA,                   int    *nvE,                   int    *nvL,                   int    *nvP,                   int    *nvA) {  // Calculate development and survival times  double vec[9];  fun_devsur(mean_air_temp,             daily_humidity,             param,             vec);  double d1=vec[0],    d2=vec[1],    d3=vec[2],    d4=vec[3],    p1=vec[4],    p2=vec[5],    p3=vec[6],    p4=vec[7],    F4=vec[8];  // Check the results of the development processes and survival  int nvecE = spop_survive((*vecE),                           d1, sqrt(d1), // development                           -p1, 0, // survival                           gamma_mode);  int nvecL = spop_survive((*vecL),                           d2, sqrt(d2), // development                           -p2, 0, // survival                           gamma_mode);  int nvecP = spop_survive((*vecP),                           d3, sqrt(d3), // development                           -p3, 0, // survival                           gamma_mode);  int nvecA = spop_survive((*vecA),                           d4, sqrt(d4), // development (oviposition)                           -p4, 0, // survival                           gamma_mode);  double tmp = F4*nvecA;  int nF4 = (tmp>1000.0) ? max(0.0,round(tmp + gsl_ran_gaussian(RAND_GSL,sqrt(tmp)))) : gsl_ran_poisson(RAND_GSL,tmp);  //  spop_add((*vecE),0,nF4);  spop_add((*vecL),0,nvecE);  spop_add((*vecP),0,nvecL);  spop_add((*vecA),0,param[param_sex]*nvecP+nvecA);  //  (*nvE) = (*vecE)->popsize;  (*nvL) = (*vecL)->popsize;  (*nvP) = (*vecP)->popsize;  (*nvA) = (*vecA)->popsize;  //  // printf("%d %d %d %d\n",(*nvE),(*nvL),(*nvP),(*nvA));}// --------------------------------------------// sim_model// --------------------------------------------void sim_model(double   *envar,               double     *par,               int     *finalT,               int    *control,               double  *result,               int    *success) {  param = par;  double *controlpar = 0;  if ((*control)) {    controlpar = param + NumPar;  }  double *mean_air_temp = envar + 0*(*finalT);;  double *daily_humidity = envar + 1*(*finalT);;  double *colT   = result + 0*(*finalT);;  double *colnvE = result + 1*(*finalT);;  double *colnvL = result + 2*(*finalT);;  double *colnvP = result + 3*(*finalT);;  double *colnvA = result + 4*(*finalT);;  int TIME = 0;  (*success) = 2;  // Set initial conditions  int nvE = round(param[init_E]);  int nvL = round(param[init_L]);  int nvP = round(param[init_P]);  int nvA = round(param[init_A]);  //  spop pop_vecE = spop_init();  spop pop_vecL = spop_init();  spop pop_vecP = spop_init();  spop pop_vecA = spop_init();  if (nvE>0) spop_add(pop_vecE,0,nvE);  if (nvL>0) spop_add(pop_vecL,0,nvL);  if (nvP>0) spop_add(pop_vecP,0,nvP);  if (nvA>0) spop_add(pop_vecA,0,nvA);  // Record state  colT[TIME] = (double)TIME;  colnvE[TIME] = (double)nvE;  colnvL[TIME] = (double)nvL;  colnvP[TIME] = (double)nvP;  colnvA[TIME] = (double)nvA;  //  for (TIME=1; TIME<(*finalT); TIME++) {    // Take a step    calculateSand(mean_air_temp[TIME-1],                  daily_humidity[TIME-1],                  &pop_vecE,                  &pop_vecL,                  &pop_vecP,                  &pop_vecA,                  &nvE,                  &nvL,                  &nvP,                  &nvA);    // Record state    colT[TIME] = (double)TIME;    colnvE[TIME] = (double)nvE;    colnvL[TIME] = (double)nvL;    colnvP[TIME] = (double)nvP;    colnvA[TIME] = (double)nvA;    if (isnan(nvE) || isnan(nvL) || isnan(nvP) || isnan(nvA)) {      (*success) = 0;      goto endall;    }  }  // endall:  //  (*success) = ((*success)==0) ? 0 : 1;  spop_destroy(&pop_vecE);  spop_destroy(&pop_vecL);  spop_destroy(&pop_vecP);  spop_destroy(&pop_vecA);  //  gamma_dist_check();}// ---------------------------------------------------------------------------