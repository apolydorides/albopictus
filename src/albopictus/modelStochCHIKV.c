#ifdef R#define modelHeader#include "R.h"#include "Rmath.h"#else#include "math.h"#endif#include <time.h>#include <stdio.h>#include <stdlib.h>#include <string.h>#include <gsl/gsl_rng.h>#include <gsl/gsl_randist.h>#include "gamma.h"#include "spop.h"#include "ran_gen.h"#define max(a,b) ((a)>(b)?(a):(b))#define min(a,b) ((a)<(b)?(a):(b))#define ZERO    0#define NumParChk      28#define NumMetChk      13extern gsl_rng *RAND_GSL;// --------------------------------------------// Gamma distribution// --------------------------------------------char gamma_mode = MODE_GAMMA_HASH;void set_gamma_mode(char mode) {  gamma_mode = mode;}// --------------------------------------------// Implementation of the dynamics// --------------------------------------------#define init_S    0#define init_E    1#define init_I    2#define init_Ic   3#define init_C    4#define init_R    5#define init_vAe  6#define init_vAi  7#define scale     8#define gamma     9#define gamma_sd  10#define omega     11#define omega_sd  12#define biteRate  13#define susceptH  14#define prob_chronic 15#define prob_C_to_Ic 16#define prob_C_to_R 17#define susceptV  18#define vec_omega 19#define vec_omega_sd   20#define ovitraps  21#define introduce_time 22#define introduce_location 23#define introduce_infectious 24#define introduce_ivector 25#define fixed_bites 26#define report    27volatile clock_t start = 0, diff;void time2here(int tm,const char *prefix) {  diff = clock() - start;  start = clock();  printf("%d\t%s\t%u\n",tm,prefix,(unsigned int)diff);}double param_biteRate;double param_prob_C_to_R;double param_prob_C_to_Ic;void calculateAeCHIKV(double *mean_air_temp,                      double *daily_vector,                      double *vector_survival_times,                      double *vector_survival_times_sd,                      double *daily_vector_fecundity,                      double *popdens,                      double *param,                      spop   *humE,                      spop   *humI,                      spop   *humIc,                      spop   *vecAe,                      spop   *vecAi,                      int    *nS,                      int    *nE,                      int    *nI,                      int    *nIc,                      int    *nC,                      int    *nR,                      int    *nvAs,                      int    *nvAe,                      int    *nvAi,                      double *npV,                      double *npH,                      double *nbite,                      int    *nInew,                      char   *experiment,                      int    TIME) {  if (TIME==-1) {    (*humE) = spop_init();    (*humI) = spop_init();    (*humIc) = spop_init();    (*vecAe) = spop_init();    (*vecAi) = spop_init();    (*nS) = round((*popdens)*param[init_S]*param[scale]);    (*nE) = round((*popdens)*param[init_E]*param[scale]);    (*nI) = round((*popdens)*param[init_I]*param[scale]);    (*nIc) = round((*popdens)*param[init_Ic]*param[scale]);    (*nC) = round((*popdens)*param[init_C]*param[scale]);    (*nR) = round((*popdens)*param[init_R]*param[scale]);    (*nvAe) = round(param[init_vAe]*param[ovitraps]*param[scale]);    (*nvAi) = round(param[init_vAi]*param[ovitraps]*param[scale]);    (*nInew) = 0;    if ((*nE)>0) spop_add((*humE),0,(*nE));    if ((*nI)>0) spop_add((*humI),0,(*nI));    if ((*nIc)>0) spop_add((*humIc),0,(*nIc));    if ((*nvAe)>0) spop_add((*vecAe),0,(*nvAe));    if ((*nvAi)>0) spop_add((*vecAi),0,(*nvAi));    // Re-scaling is done for the inference    param_biteRate = 1e-2*param[biteRate];    param_prob_C_to_R = param[prob_C_to_R]/365.0;    param_prob_C_to_Ic = param[prob_C_to_Ic]/365.0;    // ---     return;  }  // time2here(TIME,"begin");    // Check the results of the development processes (and survival)  // printf("B: %d, %d, %d, %d, %d, %d\n",TIME,(*humE)->popsize,(*humI)->popsize,(*humIc)->popsize,(*vecAe)->popsize,(*vecAi)->popsize);  int nhumE = spop_survive((*humE),                           param[omega], param[omega_sd], // development                           0.0, 0.0, // survival (immortal humans)                           gamma_mode);  int nhumI = spop_survive((*humI),                           param[gamma], param[gamma_sd], // development                           0.0, 0.0, // survival (immortal humans)                           gamma_mode);  int nhumIc = spop_survive((*humIc),                            param[gamma], param[gamma_sd], // development                            0.0, 0.0, // survival (immortal humans)                            gamma_mode);  int nvecAe = spop_survive((*vecAe),                            param[vec_omega], param[vec_omega_sd], // development                            vector_survival_times[TIME], vector_survival_times_sd[TIME], // survival                            gamma_mode);  int nvecAi = spop_survive((*vecAi),                            0.0, 0.0, // development (persistent infectious stage)                            vector_survival_times[TIME], vector_survival_times_sd[TIME], // survival                            gamma_mode);  if (nvecAi != 0) {    printf("ERROR: Development observed for infectious vector!\n");    exit(1);  }  // printf("A: %d, %d, %d, %d, %d, %d\n",TIME,nhumE,nhumI,nhumIc,nvecAe,nvecAi);  // time2here(TIME,"survive");  // Decide the fate of acute infections  int nhumIC = gsl_ran_binomial(RAND_GSL,param[prob_chronic],nhumI); // end up in the chronic stage  int nhumIR = nhumI - nhumIC; // recover completely  // Update chronic stage recovery and relapse  // This is temperature-dependent relapse  //  double prob_Ic = min(1.0, param_prob_C_to_Ic*max(1.0,26.0-mean_air_temp[TIME]));  //  double prob_R_Ic = min(1.0, param_prob_C_to_R + prob_Ic);  // This is temperature-independent relapse  double prob_R_Ic = min(1.0, param_prob_C_to_R + param_prob_C_to_Ic);  int C_to_R_Ic = gsl_ran_binomial(RAND_GSL,prob_R_Ic,(*nC)); // total exit from the chronic stage  int nhumCR = gsl_ran_binomial(RAND_GSL,param_prob_C_to_R/prob_R_Ic,C_to_R_Ic); // recovery  int nhumCIc = C_to_R_Ic - nhumCR; // relapsed infections  // Perform state transformations  spop_add((*humI),0,nhumE);  spop_add((*humIc),0,nhumCIc);  // Number of newly infectious symptomatic cases which are reported  (*nInew) = gsl_ran_binomial(RAND_GSL,param[report],nhumE);  (*nE) = (*humE)->popsize; // exposed human population  (*nI) = (*humI)->popsize; // infectious human population  (*nIc) = (*humIc)->popsize; // human population with infectious relapse  (*nC) += nhumIc + nhumIC - nhumCIc - nhumCR; // chronic human cases  (*nR) += nhumIR + nhumCR; // recovered human population  (*nvAi) = (*vecAi)->popsize + nvecAe; // newly infected vector population  (*nvAe) = (*vecAe)->popsize; // exposed vector population  // daily_vector from modelDelayAalbopictus is for the end of the day  (*nvAs) = max(0, round(daily_vector[TIME]*param[ovitraps]*param[scale])-(*nvAe)-(*nvAi));  // time2here(TIME,"state transformations");    // Check for transmission  double total_human = (*nS)+(*nE)+(*nI)+(*nIc)+(*nC)+(*nR);  double total_vector = (*nvAs)+(*nvAe)+(*nvAi);  double daily_bites;  char discrete = 0;  if (total_human==0) {    daily_bites = 0;  } else {    double tmp;    if (param[fixed_bites]) {      // Fecundity-independent bite-rate      // tmp = param_biteRate*total_vector*total_human;      tmp = param_biteRate*total_vector*total_human/sqrt((100.0+total_vector)*(1.0+total_human));    } else {      // Fecundity-dependent bite-rate      // tmp = param_biteRate*daily_vector_fecundity[TIME]*total_vector*total_human;      tmp = param_biteRate*daily_vector_fecundity[TIME]*total_vector*total_human/sqrt((100.0+total_vector)*(1.0+total_human));    }    if (tmp>1000.0) {      //printf("tmp=%g,sqrt(tmp)=%g,",tmp,sqrt(tmp));      daily_bites = max(0,tmp + gsl_ran_gaussian(RAND_GSL,sqrt(tmp)));      //printf("daily_bites=%g\n",daily_bites);    } else {      discrete = 1;      daily_bites = gsl_ran_poisson(RAND_GSL,tmp);    }  }  (*nbite) = daily_bites;  //  double probH = 0,    probV = 0,    probHV,    prob;  double counter_nbites = 0,    counter_rng;  //  while (counter_nbites < daily_bites) {    probH = param[susceptH]*(*nvAi)*(*nS)/(total_vector*total_human);    probV = param[susceptV]*(*nvAs)*((*nI)+(*nIc))/(total_vector*total_human);    if (counter_nbites==0) {      (*npV) = probV;      (*npH) = probH;      /*      printf("%d, %g,%g, %g, %g,%g, %g,%d,%d, %g,%d,%d\n",             TIME, total_human,total_vector, daily_bites, probH,probV,             param[susceptH],(*nvAi),(*nS),             param[susceptV],(*nvAs),((*nI)+(*nIc)));      */    }    probHV = probH + probV;    if (probHV <= 0) {      break;    }    // Number of bites until next S->E (probH) or Vs->Vi (probV)    if (discrete) {      counter_rng = gsl_ran_geometric(RAND_GSL,probHV);    } else {      counter_rng = rng_exponential(probHV);    }    if (counter_rng <= 0) {      break;    }    counter_nbites += counter_rng; // nbites represent the successful next bite    if (counter_nbites > daily_bites) {      break;    }    // probH or probV?    prob = gsl_rng_uniform(RAND_GSL);    if (prob < probH/probHV) { // vector->human transmission occured (probH: S->E)      spop_add((*humE),0,1);      (*nE)++;      (*nS)--;    } else { // human->vector transmission occured (probV: Vs->Vi)      spop_add((*vecAe),0,1);      (*nvAe)++;      (*nvAs)--;    }  }  // time2here(TIME,"daily bites");  // Check if an experiment is scheduled  if (param[introduce_time] > 0 &&      param[introduce_infectious] > 0 &&      !(*experiment) &&      TIME >= param[introduce_time]) {    int infect = round(param[introduce_infectious]);    if (param[introduce_ivector]) {      // Introduce an infectious vector      spop_add((*vecAi),0,infect);      (*nvAi) += infect;    } else if ((*nS)>=infect) {      // Introduce an infectious human      spop_add((*humI),0,infect);      (*nI) += infect;      (*nInew) += infect;      (*nS) -= infect;    }    (*experiment) = 1;  }  // time2here(TIME,"end");}// --------------------------------------------// sim_model// --------------------------------------------void numparModel(int *np, int *nm) {  *np = NumParChk;  *nm = NumMetChk;}void param_model(char **names, double *param) {  char temp[NumMetChk+NumParChk][256] = {    "colnS","colnE","colnI","colnIc","colnC","colnR","colnvAs","colnvAe","colnvAi","colnpV","colnpH","colnbite","colnInew",    "init_S","init_E","init_I","init_Ic","init_C","init_R","init_vAe","init_vAi","scale","gamma","gamma_sd","omega","omega_sd","biteRate","susceptH","prob_chronic","prob_C_to_Ic","prob_C_to_R","susceptV","vec_omega","vec_omega_sd","ovitraps","introduce_time","introduce_location","introduce_infectious","introduce_ivector","fixed_bites","report"  };  int i;  for (i=0; i<(NumMetChk+NumParChk); i++)    names[i] = strdup(temp[i]);  // ---  //   param[init_S] = 1.0; // susceptible (% popdens)  param[init_E] = 0; // latent (% popdens)  param[init_I] = 0; // infectious (a/symptomatic) (% popdens)  param[init_Ic] = 0; // infectious relapse (a/symptomatic) (% popdens)  param[init_C] = 0; // chronic stage (% popdens)  param[init_R] = 0; // recovered (% popdens)  param[init_vAe] = 0; // adult vector (latent)  param[init_vAi] = 0; // adult vector (infectious)  param[scale] = 1.0; // simulation span (affects double -> int conversion)  param[gamma] = 4.5; // infectious period in humans (2-7 days)  param[gamma_sd] = 1.0; //  param[omega] = 3.5; // latent period in humans (2-4 days)  param[omega_sd] = 0.5; //  param[biteRate] = 0.25; // num. bites / mosquito / day (?) (x1e-2)  param[susceptH] = 0.65; //0.8; // human susceptibility to infections (50-80%)  param[prob_chronic] = 0.33; // fraction of infections entering into chronic stage  param[prob_C_to_Ic] = 1.0; // daily probability of relapse (x365)  param[prob_C_to_R] = 1.0; // daily probability of recovery from chronic stage (x365)  //                           0 <= prob_C_to_R <= 1  param[susceptV] = 0.85; //1.0; // vector susceptibility to infections (70-100%)  param[vec_omega] = 3.0; // latent period in vectors (2-3 days)  param[vec_omega_sd] = 0.1; //  //  param[ovitraps] = 0.32; // ovitrap / km^2 (~200 ovitraps per grid point, 25*25 km^2)  // Also, each ovitrap samples 10% of the population in the vicinity (?)  //  param[introduce_time] = 0; // when to introduce an infection?  param[introduce_location] = 0; // where to introduce the infection?  param[introduce_infectious] = 0; // how many infected human/vector to be introduced?  param[introduce_ivector] = 0; // introduce infectious vector? or human?  //  param[fixed_bites] = 0; // fixed or fecundity-dependent bite rates?  //  param[report] = 1.0; // fraction of cases reported (scale on the output)}// Accessory function for population movement -------------------------void emigrate(int numreg, double *tprobs, spop *pops, int *nH) {  int regionA, regionB;  int i, swap;  int rgnAB,rgnBA;  int numA;  int *probs = (int *)malloc(numreg*numreg*sizeof(int));  if (probs == NULL) {    printf("ERROR: Memory allocation error!\n");    exit(1);  }  // Calculate the number of emigrations (A -> B)  for (rgnAB=0,regionA=0; regionA<numreg; regionA++) {    numA = 0;    for (regionB=0; regionB<numreg; regionB++,rgnAB++) {      probs[rgnAB] = gsl_ran_binomial(RAND_GSL,                                      tprobs[rgnAB], // probability given previous emigrations                                      nH[regionA]-numA); // number of emigrating humans      // probs[rgnAB] = (int)(round(tprobs[rgnAB] * (nH[regionA]-numA)));      //      // if (regionA!=regionB)      // printf("Propensity %d -> %d: p=%g, nH=%d, %d out of %d\n",regionA,regionB,tprobs[rgnAB],nH[regionA],probs[rgnAB],nH[regionA]-numA);      //      numA += probs[rgnAB];    }  }  //   for (regionA=0; regionA<numreg; regionA++) {    for (regionB=regionA+1; regionB<numreg; regionB++) {      rgnAB = regionA*numreg+regionB;      rgnBA = regionB*numreg+regionA;      swap = probs[rgnAB] - probs[rgnBA];      if (swap==0)        continue;      else { // Swap infectious between regionA and regionB        //        // printf("Swap %d,%d,%d\n",regionA,regionB,swap);        //        nH[regionA] -= swap;        if (nH[regionA]<0) {          printf("ERROR: Negative population size!\n");          free(probs);          exit(1);        }        nH[regionB] += swap;        if (pops) {          for (i=0; i<abs(swap); i++) {            if (swap>0)              spop_swap(pops[regionA],pops[regionB]);            else              spop_swap(pops[regionB],pops[regionA]);          }        }      }    }  }  free(probs);}// --------------------------------------------------------------------void sim_spread(double   *envar,                double  *tprobs,                double   *param,                int     *finalT,                int     *numreg,                double  *result,                int    *success) {  spop *pop_humE = (spop *)malloc((*numreg)*sizeof(spop));  spop *pop_humI = (spop *)malloc((*numreg)*sizeof(spop));  spop *pop_humIc = (spop *)malloc((*numreg)*sizeof(spop));  spop *pop_vecAe = (spop *)malloc((*numreg)*sizeof(spop));  spop *pop_vecAi = (spop *)malloc((*numreg)*sizeof(spop));  char *experiment = (char *)calloc((*numreg),sizeof(char));  int *nS = (int *)calloc((*numreg),sizeof(int));  int *nE = (int *)calloc((*numreg),sizeof(int));  int *nI = (int *)calloc((*numreg),sizeof(int));  int *nIc = (int *)calloc((*numreg),sizeof(int));  int *nC = (int *)calloc((*numreg),sizeof(int));  int *nR = (int *)calloc((*numreg),sizeof(int));  int *nvAs = (int *)calloc((*numreg),sizeof(int));  int *nvAe = (int *)calloc((*numreg),sizeof(int));  int *nvAi = (int *)calloc((*numreg),sizeof(int));  double *npV = (double *)calloc((*numreg),sizeof(double));  double *npH = (double *)calloc((*numreg),sizeof(double));  double *nbite = (double *)calloc((*numreg),sizeof(double));  int *nInew = (int *)calloc((*numreg),sizeof(int));  double *environ;  double *output;  double *mean_air_temp;  double *daily_vector;  double *vector_survival_times;  double *vector_survival_times_sd;  double *daily_vector_fecundity;  double *popdens;  double *colT;  double *colnS;  double *colnE;  double *colnI;  double *colnIc;  double *colnC;  double *colnR;  double *colnvAs;  double *colnvAe;  double *colnvAi;  double *colnpV;  double *colnpH;  double *colnbite;  double *colnInew;  int TIME;  int region;  for (region=0; region<(*numreg); region++) {    if (region != param[introduce_location]) experiment[region] = 1;    success[region] = 2;  }  //  for (TIME=0; TIME<(*finalT); TIME++) {    for (region=0; region<(*numreg); region++) {      /*        WARNING!        ========        Information to decode environmental variables is hard coded here!       */      environ = envar + region*(5*(*finalT)+1);      output = result + region*14*(*finalT);            mean_air_temp             = environ + 0*(*finalT);      daily_vector              = environ + 1*(*finalT);      vector_survival_times     = environ + 2*(*finalT);      vector_survival_times_sd  = environ + 3*(*finalT);      daily_vector_fecundity    = environ + 4*(*finalT);      popdens                   = environ + 5*(*finalT);            colT     = output + 0*(*finalT);      colnS    = output + 1*(*finalT);      colnE    = output + 2*(*finalT);      colnI    = output + 3*(*finalT);      colnIc   = output + 4*(*finalT);      colnC    = output + 5*(*finalT);      colnR    = output + 6*(*finalT);      colnvAs  = output + 7*(*finalT);      colnvAe  = output + 8*(*finalT);      colnvAi  = output + 9*(*finalT);      colnpV   = output + 10*(*finalT);      colnpH   = output + 11*(*finalT);      colnbite = output + 12*(*finalT);      colnInew = output + 13*(*finalT);      calculateAeCHIKV(mean_air_temp,daily_vector,vector_survival_times,vector_survival_times_sd,daily_vector_fecundity,popdens,param,&pop_humE[region],&pop_humI[region],&pop_humIc[region],&pop_vecAe[region],&pop_vecAi[region],&nS[region],&nE[region],&nI[region],&nIc[region],&nC[region],&nR[region],&nvAs[region],&nvAe[region],&nvAi[region],&npV[region],&npH[region],&nbite[region],&nInew[region],&experiment[region],TIME-1);      colT[TIME] = (double)TIME;      colnS[TIME] = (double)nS[region];      colnE[TIME] = (double)nE[region];      colnI[TIME] = (double)nI[region];      colnIc[TIME] = (double)nIc[region];      colnC[TIME] = (double)nC[region];      colnR[TIME] = (double)nR[region];      colnvAs[TIME] = (double)nvAs[region];      colnvAe[TIME] = (double)nvAe[region];      colnvAi[TIME] = (double)nvAi[region];      colnpV[TIME] = npV[region];      colnpH[TIME] = npH[region];      colnbite[TIME] = nbite[region];      colnInew[TIME] = (double)nInew[region];      if (isnan(nS[region]) || isnan(nE[region]) || isnan(nI[region]) || isnan(nIc[region]) || isnan(nC[region]) || isnan(nR[region]) || isnan(nvAs[region]) || isnan(nvAe[region]) || isnan(nvAi[region]) || isnan(npV[region]) || isnan(npH[region]) || (isnan(nbite[region]) || nbite[region]<0) || isnan(nInew[region]) || isnan(experiment[region])) {        success[region] = 0;        goto endall;      }    }    // --------------------------------    // This is where spread is modelled    if ((*numreg)>1) {      // printf("---\n");      // for (region=0; region<(*numreg); region++)      // printf("Before %d: %d %d %d %d (%d) %d (%d) %d (%d)\n",region,nS[region],nC[region],nR[region],nE[region],pop_humE[region]->popsize,nI[region],pop_humI[region]->popsize,nIc[region],pop_humIc[region]->popsize);      emigrate((*numreg), tprobs, 0, nS);      emigrate((*numreg), tprobs, 0, nC);      emigrate((*numreg), tprobs, 0, nR);      emigrate((*numreg), tprobs, pop_humE, nE);      emigrate((*numreg), tprobs, pop_humI, nI);      emigrate((*numreg), tprobs, pop_humIc, nIc);      // for (region=0; region<(*numreg); region++)      // printf("After %d: %d %d %d %d (%d) %d (%d) %d (%d)\n",region,nS[region],nC[region],nR[region],nE[region],pop_humE[region]->popsize,nI[region],pop_humI[region]->popsize,nIc[region],pop_humIc[region]->popsize);      //      for (region=0; region<(*numreg); region++) {        output   = result + region*14*(*finalT);        colnS    = output + 1*(*finalT);        colnE    = output + 2*(*finalT);        colnI    = output + 3*(*finalT);        colnIc   = output + 4*(*finalT);        colnC    = output + 5*(*finalT);        colnR    = output + 6*(*finalT);        colnS[TIME] = (double)nS[region];        colnE[TIME] = (double)nE[region];        colnI[TIME] = (double)nI[region];        colnIc[TIME] = (double)nIc[region];        colnC[TIME] = (double)nC[region];        colnR[TIME] = (double)nR[region];      }      /*        int *popsize = (int *)calloc((*numreg),sizeof(int));        for (region=0; region<(*numreg); region++)        popsize[region] += nS[region]+nC[region]+nR[region]+nE[region]+nI[region]+nIc[region];        printf("TIME: %d, ",TIME);        for (region=0; region<(*numreg); region++)        printf("%s %d",region?",":"",popsize[region]);        printf("\n");        free(popsize);      */    }  }  // endall:  //  for (region=0; region<(*numreg); region++) {    success[region] = (success[region]==0) ? 0 : 1;    spop_destroy(&pop_humE[region]);    spop_destroy(&pop_humI[region]);    spop_destroy(&pop_humIc[region]);    spop_destroy(&pop_vecAe[region]);    spop_destroy(&pop_vecAi[region]);  }  free(pop_humE);  free(pop_humI);  free(pop_humIc);  free(pop_vecAe);  free(pop_vecAi);  free(experiment);  free(nS);  free(nE);  free(nI);  free(nIc);  free(nC);  free(nR);  free(nvAs);  free(nvAe);  free(nvAi);  free(npV);  free(npH);  free(nbite);  free(nInew);  //  gamma_dist_check();}// ---------------------------------------------------------------------------