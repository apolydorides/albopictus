#ifdef R#define modelHeader#include "R.h"#include "Rmath.h"#else#include "math.h"#endif#include <time.h>#include <stdio.h>#include <stdlib.h>#include <string.h>#include <gsl/gsl_rng.h>#include <gsl/gsl_randist.h>#include "gamma.h"#include "spop01.h"#include "ran_gen.h"#define max(a,b) ((a)>(b)?(a):(b))#define min(a,b) ((a)<(b)?(a):(b))#define ZERO    0#define MAX_DEV 1000#define CHECK(x) ((x)<0)extern gsl_rng *RAND_GSL;// --------------------------------------------// Gamma distribution// --------------------------------------------char gamma_mode = MODE_GAMMA_HASH;void set_gamma_mode(char mode) {  gamma_mode = mode;}// --------------------------------------------// Implementation of the dynamics// --------------------------------------------double *param;double prob_capture;double prob_BS;#define NumPar      37#define NumMet      7#define init_E      0#define init_L      1#define init_P      2#define init_A      3#define param_dE_1  4#define param_dE_2  5#define param_dE_3  6#define param_dL_1  7#define param_dL_2  8#define param_dL_3  9#define param_dP_1  10#define param_dP_2  11#define param_dP_3  12#define param_dA_1  13#define param_dA_2  14#define param_dA_3  15#define param_pE_1  16#define param_pE_2  17#define param_pE_3  18#define param_pL_1  19#define param_pL_2  20#define param_pL_3  21#define param_pP_1  22#define param_pP_2  23#define param_pP_3  24#define param_pA_1  25#define param_pA_2  26#define param_pA_3  27#define param_FA_1  28#define param_FA_2  29#define param_FA_3  30#define param_ovipos 31#define param_female 32#define param_PP_ta 33#define param_PP_hu 34#define param_BS    35#define param_eta   36void numparModel(int *np, int *nm) {  *np = NumPar;  *nm = NumMet;}void param_model(char **names, double *par) {  char temp[NumMet+NumPar][256] = {    "colnvE","colnvL","colnvP","colnvAf","colnvAfcap","colnvAm","colnvAmcap",    "init_E","init_L","init_P","init_A","param_dE_1","param_dE_2","param_dE_3","param_dL_1","param_dL_2","param_dL_3","param_dP_1","param_dP_2","param_dP_3","param_dA_1","param_dA_2","param_dA_3","param_pE_1","param_pE_2","param_pE_3","param_pL_1","param_pL_2","param_pL_3","param_pP_1","param_pP_2","param_pP_3","param_pA_1","param_pA_2","param_pA_3","param_FA_1","param_FA_2","param_FA_3","param_ovipos","param_female","param_PP_ta","param_PP_hu","param_BS","param_eta"  };  int i;  for (i=0; i<(NumMet+NumPar); i++)    names[i] = strdup(temp[i]);  // ---  //   par[init_E]      = 1e4; // initial number of egg  par[init_L]      = 1e4; // initial number of larva  par[init_P]      = 1e4; // initial number of pupa  par[init_A]      = 1e4; // initial number of adult    par[param_dE_1]  = 5.0; // min  par[param_dE_2]  = 35.0; // max  par[param_dE_3]  = 20.0; // Tmean    par[param_dL_1]  = 18.0;  par[param_dL_2]  = 30.0;  par[param_dL_3]  = 20.0;  par[param_dP_1]  = 5.0;  par[param_dP_2]  = 32.0;  par[param_dP_3]  = 20.0;    par[param_dA_1]  = 5.0; // min  par[param_dA_2]  = 10.0; // max  par[param_dA_3]  = 25.0; // Tmean  par[param_pE_1]  = 50.0; // max  par[param_pE_2]  = 11.6; // Tmin  par[param_pE_3]  = 22.0; // Tmax    par[param_pL_1]  = 50.0;  par[param_pL_2]  = 11.6;  par[param_pL_3]  = 22.0;    par[param_pP_1]  = 50.0;  par[param_pP_2]  = 11.6;  par[param_pP_3]  = 22.0;    par[param_pA_1]  = 50.0;  par[param_pA_2]  = 11.6;  par[param_pA_3]  = 20.25;    par[param_FA_1]  = 80.0; // eggs per female in 4-8 days  par[param_FA_2]  = 0.0; // Tmin  par[param_FA_3]  = 30.0; // Tmax  par[param_ovipos] = 0.5; // surviving oviposition    par[param_female] = 0.5;    par[param_PP_ta] = 19.81;  par[param_PP_hu] = 0.3;  par[param_BS]    = 1.0;  par[param_eta]   = -3.0;}// --------------------------------------------------------------------#define dsigL(x, a2) (max(0.0,min(1.0, 1.0/(1.0+exp(((a2)-(x))/0.01)) )))#define dsig(x, a1, a2, a3) (max(0.0,min(0.999, (a1)/((1.0+exp((a2)-(x)))*(1.0+exp((x)-(a3)))) )))#define dsigi(x, a1l, a1h, a2, a3) (max((a1l),min((a1h), (a1h)-((a1h)-(a1l))/((1.0+exp((a2)-(x)))*(1.0+exp((x)-(a3)))) )))#define dsigiR(x, a1l, a1h, a2) (max((a1l),min((a1h), (a1h)-((a1h)-(a1l))/(1.0+exp((a2)-(x))) )))#define dsigm(x, a1, a2, a3) (max(0.0, (a1)/((1.0+exp((a2)-(x)))*(1.0+exp((x)-(a3)))) ))int fun_capture(int    *nvA,                double *par) {  return gsl_ran_binomial(RAND_GSL,prob_capture,*nvA);}int fun_female(int    *nvP,               double *par) {  return gsl_ran_binomial(RAND_GSL,par[param_female],*nvP);}void fun_devsur(double temp,                double hum,                char   all,                double *vec) {  // Diapause flag  char diapause = temp < param[param_PP_ta];  //  /* p12hu: probability to die of low humidity */  vec[9] = 1.0-dsigL(hum,param[param_PP_hu]);  //  if (!all) return;  //  // Calculate development and survival times  /* d1: probability to develop */  vec[0] = dsigiR(temp,param[param_dE_1],param[param_dE_2],param[param_dE_3]);  /* d2: probability to develop */  vec[1] = diapause ? MAX_DEV : dsigiR(temp,param[param_dL_1],param[param_dL_2],param[param_dL_3]);  /* d3: probability to develop */  vec[2] = dsigiR(temp,param[param_dP_1],param[param_dP_2],param[param_dP_3]);  /* d4: probability to oviposition */  vec[3] = dsigiR(temp,param[param_dA_1],param[param_dA_2],param[param_dA_3]);  //  /* p1: probability to die */  vec[4] = 1.0-dsig(temp,1.0-(1.0/(1.0+param[param_pE_1])),param[param_pE_2],param[param_pE_3]);  /* p2: probability to die */  vec[5] = 1.0-dsig(temp,1.0-(1.0/(1.0+param[param_pL_1])),param[param_pL_2],param[param_pL_3]);  /* p3: probability to die */  vec[6] = 1.0-dsig(temp,1.0-(1.0/(1.0+param[param_pP_1])),param[param_pP_2],param[param_pP_3]);  /* p4: probability to die */  vec[7] = 1.0-dsig(temp,1.0-(1.0/(1.0+param[param_pA_1])),param[param_pA_2],param[param_pA_3]);  //  /* FA */  vec[8] = dsigm(temp,param[param_FA_1],param[param_FA_2],param[param_FA_3]);  //  // development: d2, d3, d4 (temperature and starvation)  if (prob_BS > 0) {    vec[1] /= prob_BS;    vec[2] /= prob_BS;    vec[3] /= prob_BS;  }  // death: p1, p2 (temperature or humidity)  vec[4] = vec[4] + vec[9] - vec[4]*vec[9];  vec[5] = vec[5] + vec[9] - vec[5]*vec[9];  // fecundity (temperature and starvation)  vec[8] *= prob_BS;}// --------------------------------------------------------------------void calculateSand(double mean_air_temp,                   double daily_humidity,                   double daily_capture,                   spop   *vecE,                   spop   *vecL,                   spop   *vecP,                   spop   *vecAf,                   int    *nvE,                   int    *nvL,                   int    *nvP,                   int    *nvAf,                   int    *nvAfcap,                   int    *nvAm,                   int    *nvAmcap) {  // Calculate development and survival times  double vec[10];  fun_devsur(mean_air_temp,             daily_humidity,             1,             vec);  double d1=vec[0],    d2=vec[1],    d3=vec[2],    d4=vec[3],    p1=vec[4],    p2=vec[5],    p3=vec[6],    p4=vec[7],    FA=vec[8],    p12hu=vec[9]; // death due to low humidity  //  // Check the results of the development processes and survival  int nvecE = spop_survive((*vecE),                           d1, sqrt(d1), // development                           -p1, 0, // death                           gamma_mode);  int nvecL = spop_survive((*vecL),                           d2, sqrt(d2), // development                           -p2, 0, // death                           gamma_mode);  int nvecP = spop_survive((*vecP),                           d3, sqrt(d3), // development                           -p3, 0, // death                           gamma_mode);  int nvecAf = spop_survive((*vecAf),                           d4, sqrt(d4), // egg development                           -p4, 0, // death                           gamma_mode);  int nvecAm = gsl_ran_binomial(RAND_GSL,p4,(*nvAm)); // number of dead males  // prob_BS affects survival  /*  double pp = p12hu + prob_BS - p12hu*prob_BS; // death due to humidity or starvation  int nvecE = spop_survive((*vecE),                           d1, sqrt(d1), // development (temperature)                           -(p1 + p12hu - p1*p12hu), 0, // death (temperature or humidity)                           gamma_mode);  int nvecL = spop_survive((*vecL),                           d2, sqrt(d2), // development (temperature)                           -(p2 + pp - p2*pp), 0, // death (temperature or humidity or starvation)                           gamma_mode);  int nvecP = spop_survive((*vecP),                           d3, sqrt(d3), // development (temperature)                           -(p3 + prob_BS - p3*prob_BS), 0, // death (temperature or starvation)                           gamma_mode);  int nvecAf = spop_survive((*vecAf),                           d4, sqrt(d4), // egg development (temperature)                            -(p4 + prob_BS - p4*prob_BS), 0, // death (temperature or starvation)                           gamma_mode);  int nvecAm = gsl_ran_binomial(RAND_GSL,(p4 + prob_BS - p4*prob_BS),(*nvAm));  */  //  if (CHECK(nvecE) || CHECK(nvecL) || CHECK(nvecAf) || CHECK(nvecAm)) {    (*nvE) = -1;    (*nvL) = -1;    (*nvP) = -1;    (*nvAf) = -1;    return;   }  //  int tmp;  double tmpd = FA*nvecAf; // expected number of viable eggs laid at the right location by all females  int nFA = (tmpd>1000.0) ? max(0.0,round(tmpd + gsl_ran_gaussian(RAND_GSL,sqrt(tmpd)))) : gsl_ran_poisson(RAND_GSL,tmpd);  // Newly emerged adult females  int newf = fun_female(&nvecP,param);  //  spop_add((*vecE),0,nFA);  spop_add((*vecL),0,nvecE);  spop_add((*vecP),0,nvecL);  //  tmp = gsl_ran_binomial(RAND_GSL,param[param_ovipos],nvecAf); // females surviving oviposition  spop_add((*vecAf),0,newf+tmp); // newly emerged ones and  // subsequent gonotrophic cycle of the oviposited ones  //  char capture = daily_capture > 0.5;  //  if (capture) {    tmp = spop_kill((*vecAf),                    prob_capture); // probability of capture    (*nvAfcap) = tmp;  } else {    (*nvAfcap) = 0;  }  //  (*nvAm) += nvecP - newf - nvecAm;  //  if (capture) {    tmp = fun_capture(nvAm,param);    (*nvAmcap) = tmp;    (*nvAm) -= tmp;  } else {    (*nvAmcap) = 0;  }  //  (*nvE) = (*vecE)->popsize;  (*nvL) = (*vecL)->popsize;  (*nvP) = (*vecP)->popsize;  (*nvAf) = (*vecAf)->popsize;  //  // printf("%d %d %d %d\n",(*nvE),(*nvL),(*nvP),(*nvAf));}// --------------------------------------------------------------------void set_param(double *par) {  param = par;  prob_capture = pow(10,param[param_eta]);  prob_BS      = param[param_BS];}// --------------------------------------------// sim_model// --------------------------------------------void sim_model(double   *envar,               double     *par,               int     *finalT,               int    *control,               double  *result,               int    *success) {  set_param(par);  double *controlpar = 0;  if ((*control)) {    controlpar = param + NumPar;  }  double *mean_air_temp = envar + 0*(*finalT);;  double *daily_humidity = envar + 1*(*finalT);;  double *daily_capture = envar + 2*(*finalT);;  double *colT   = result + 0*(*finalT);;  double *colnvE = result + 1*(*finalT);;  double *colnvL = result + 2*(*finalT);;  double *colnvP = result + 3*(*finalT);;  double *colnvAf = result + 4*(*finalT);;  double *colnvAfcap = result + 5*(*finalT);;  double *colnvAm = result + 6*(*finalT);;  double *colnvAmcap = result + 7*(*finalT);;  int TIME = 0;  (*success) = 2;  // Set initial conditions  int nvE = round(param[init_E]);  int nvL = round(param[init_L]);  int nvP = round(param[init_P]);  int nvA = round(param[init_A]);  int nvAf = fun_female(&nvA,param);  int nvAm = nvA - nvAf;  //  int nvAfcap;  int nvAmcap;  if (daily_capture[0] > 0.5) {    nvAfcap = fun_capture(&nvAf,param);    nvAmcap = fun_capture(&nvAm,param);    nvAf -= nvAfcap;    nvAm -= nvAmcap;  }  //  spop pop_vecE = spop_init();  spop pop_vecL = spop_init();  spop pop_vecP = spop_init();  spop pop_vecAf = spop_init();  if (nvE>0) spop_add(pop_vecE,0,nvE);  if (nvL>0) spop_add(pop_vecL,0,nvL);  if (nvP>0) spop_add(pop_vecP,0,nvP);  if (nvAf>0) spop_add(pop_vecAf,0,nvAf);  // Record state  colT[TIME] = (double)TIME;  colnvE[TIME] = (double)nvE;  colnvL[TIME] = (double)nvL;  colnvP[TIME] = (double)nvP;  colnvAf[TIME] = (double)nvAf;  colnvAfcap[TIME] = (double)nvAfcap;  colnvAm[TIME] = (double)nvAm;  colnvAmcap[TIME] = (double)nvAmcap;  //  for (TIME=1; TIME<(*finalT); TIME++) {    // Take a step    calculateSand(mean_air_temp[TIME-1],                  daily_humidity[TIME-1],                  daily_capture[TIME], // Capture using the previous day's results (i.e. traps stay overnight)                  &pop_vecE,                  &pop_vecL,                  &pop_vecP,                  &pop_vecAf,                  &nvE,                  &nvL,                  &nvP,                  &nvAf,                  &nvAfcap,                  &nvAm,                  &nvAmcap);    // Record state    colT[TIME] = (double)TIME;    colnvE[TIME] = (double)nvE;    colnvL[TIME] = (double)nvL;    colnvP[TIME] = (double)nvP;    colnvAf[TIME] = (double)nvAf;    colnvAfcap[TIME] = (double)nvAfcap;    colnvAm[TIME] = (double)nvAm;    colnvAmcap[TIME] = (double)nvAmcap;    if (CHECK(nvE) ||        CHECK(nvL) ||        CHECK(nvP) ||        CHECK(nvAf) ||        CHECK(nvAfcap) ||        CHECK(nvAm) ||        CHECK(nvAmcap)) {      (*success) = 0;      goto endall;    }  }  // endall:  //  (*success) = ((*success)==0) ? 0 : 1;  spop_destroy(&pop_vecE);  spop_destroy(&pop_vecL);  spop_destroy(&pop_vecP);  spop_destroy(&pop_vecAf);  //  gamma_dist_check();}// ---------------------------------------------------------------------------