#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nicklib.h>
#include <getpars.h>
#include <globals.h>

#include "admutils.h"
#include "mcio.h"
#include "mcmcpars.h"
#include "regsubs.h"
#include "egsubs.h"
#include "qpsubs.h"

#define Y  0
#define E  1
#define A  2

//  (YRI, CEU, Papua, .... )               


#define WVERSION   "550"

// gsimplify option added

#define MAXFL  50
#define MAXSTR  512
#define MAXPOPS 100

char *parname = NULL;
char *trashdir = "/var/tmp";
int details = NO;
int qtmode = NO;
int fstdmode = NO;
int hires = NO;
int inbreed = NO;

int isinit = NO;

int seed = 0;

char *graphname = NULL;
char *graphoutname = NULL;
char *graphdotname = NULL;
char *poplistname = NULL;

char *dumpname = NULL;
char *loadname = NULL;
char *rootname;
char *delpop = NULL;

char *outpop = NULL;
// list of outliers
char *basepop = NULL;
int basenum = -1;
int calchash = NO;
int gsimp = NO;

// outnum used for weights 
// basenum for f3 status  


double **vmix;
int *lmix, nmix;
int nh2, numeg;
int *ezero = NULL;
double wtmin = .0001;
int xnumeg;
char *newnodename = NULL;
char *edgename = NULL;
double valbreak = 0.5;



char *outputname = NULL;
char *weightname = NULL;
FILE *ofile;
char **eglist;
char **egshort;
char **enames;

void readcommands (int argc, char **argv);
char *get3 (char *ss);
char *getshort (char *ss, int n);
void setwww (double **tmix, double *www, int n);
void getwww (double **tmix, double *www, int n);
void printvals (double **tmix, double *edgelen, int nedge);
void printfit (double *ww);

void setsimp (double *ww, int n);

int
main (int argc, char **argv)
{

  int nedge, t;
  char *psname, *pdfname;
  char sss[MAXSTR];

  readcommands (argc, argv);
  printf ("## qpreroot version: %s\n", WVERSION);

  numeg = loadgraph (graphname, &eglist);

  if (newnodename != NULL) {
    printf ("adding newnode: %s\n", newnodename);
    addnode (newnodename, edgename, valbreak);
  }
  printf ("rootname: %s\n", rootname);
  if (rootname != NULL)
    reroot (rootname);
  if (delpop != NULL) {
    dellabel (delpop);
    gsimplify (0);
  }
  if (calchash) {
    t = hashgraph ();
    printf ("## hash: %x\n", t);
  }

  if (gsimp) {
    printf ("calling gsimplify\n");
    gsimplify (0);
  }

  dumpgraph (graphoutname);
  dumpdotgraph (graphdotname);
  t = 0;
  if (graphdotname != NULL) {
    psname = strdup (graphdotname);
    t = substring (&psname, ".dot", ".ps");
  }
  if (t > 0) {
    sprintf (sss, "dot -T ps < %s  > %s", graphdotname, psname);
    system (sss);
    sprintf(sss, "ps2pdf %s", psname) ;
    system(sss) ;
  }
  printf ("## end of qpreroot\n");

}

void
printvals (double **tmix, double *edgelen, int nedge)
{
  char sss[MAXSTR];
  int k;

  for (k = 0; k < nmix; ++k) {
    getmixstr (k, sss);
    printf ("%40s ", sss);
    printmat (tmix[k], 1, lmix[k]);
  }
  printnl ();

  for (k = 0; k < nedge; ++k) {
    printf ("%9s ", enames[k]);
  }
  printnl ();
  printmatw (edgelen, 1, nedge, nedge);


}

void
setwww (double **tmix, double *www, int n)
// copy tmix to vector 
{
  int k, l;
  double *ww;

  if (n != intsum (lmix, nmix))
    fatalx ("dimension bug\n");

  ww = www;
  for (k = 0; k < nmix; ++k) {
    l = lmix[k];
    copyarr (tmix[k], ww, l);
    bal1 (ww, l);
    ww += l;
  }

}

void
getwww (double **tmix, double *www, int n)
// copy vector to tmix  
{
  int k, l;
  double *ww;

  if (n != intsum (lmix, nmix))
    fatalx ("dimension bug\n");

  ww = www;
  for (k = 0; k < nmix; ++k) {
    l = lmix[k];
    copyarr (ww, tmix[k], l);
    ww += l;
  }
}

void
readcommands (int argc, char **argv)
{
  int i, haploid = 0;
  phandle *ph;
  char str[5000];
  char *tempname;
  int n;

  while ((i = getopt (argc, argv, "p:r:g:o:d:x:hvVs")) != -1) {

    switch (i) {

    case 'p':
      parname = strdup (optarg);
      break;

    case 'r':
      rootname = strdup (optarg);
      break;

    case 'g':
      graphname = strdup (optarg);
      break;

    case 'o':
      graphoutname = strdup (optarg);
      break;

    case 'd':
      graphdotname = strdup (optarg);
      break;

    case 'x':
      delpop = strdup (optarg);
      break;

    case 'h':
      calchash = YES;
      break;

    case 's':
      gsimp = YES;
      break;

    case 'v':
      printf ("version: %s\n", WVERSION);
      break;

    case 'V':
      verbose = YES;
      break;

    case '?':
      printf ("Usage: bad params.... \n");
      fatalx ("bad params\n");
    }
  }


  if (parname == NULL) {
    return;
  }

  printf ("parameter file: %s\n", parname);
  ph = openpars (parname);
  dostrsub (ph);

  getstring (ph, "graphname:", &graphname);
  getstring (ph, "graphoutname:", &graphoutname);
  getstring (ph, "graphdotname:", &graphdotname);
  getstring (ph, "outpop:", &outpop);
  getstring (ph, "output:", &outputname);
  getstring (ph, "delpop:", &delpop);

  getstring (ph, "root:", &rootname);
  getstring (ph, "dumpname:", &dumpname);
  getstring (ph, "loadname:", &loadname);
  getstring (ph, "newnode:", &newnodename);
  getstring (ph, "edgename:", &edgename);
  getdbl (ph, "valbreak:", &valbreak);

  writepars (ph);

}



