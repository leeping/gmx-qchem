/*
 * $Id$
 * 
 *       This source code is part of
 * 
 *        G   R   O   M   A   C   S
 * 
 * GROningen MAchine for Chemical Simulations
 * 
 *               VERSION 2.0
 * 
 * Copyright (c) 1991-1999
 * BIOSON Research Institute, Dept. of Biophysical Chemistry
 * University of Groningen, The Netherlands
 * 
 * Please refer to:
 * GROMACS: A message-passing parallel molecular dynamics implementation
 * H.J.C. Berendsen, D. van der Spoel and R. van Drunen
 * Comp. Phys. Comm. 91, 43-56 (1995)
 * 
 * Also check out our WWW page:
 * http://md.chem.rug.nl/~gmx
 * or e-mail to:
 * gromacs@chem.rug.nl
 * 
 * And Hey:
 * GRowing Old MAkes el Chrono Sweat
 */
static char *SRCID_eneconv_c = "$Id$";

#include <string.h>
#include <math.h>
#include "string2.h"
#include "typedefs.h"
#include "smalloc.h"
#include "statutil.h"
#include "disre.h"
#include "names.h"
#include "copyrite.h"
#include "macros.h"
#include "enxio.h"

#define TIME_EXPLICIT 0
#define TIME_CONTINUE 1
#define TIME_LAST     2
#ifndef FLT_MAX
#define FLT_MAX 1e36
#endif

static bool same_time(real t1,real t2)
{
  const real tol=1e-5;

  return (fabs(t1-t2) < tol);
}


bool bRgt(double a,double b)
{
  double tol = 1e-6;
  
  if ( a > (b - tol*(a+b)) )
    return TRUE;
  else
    return FALSE;
}

static void sort_files(char **fnms,real *settime,int nfile)
{
    int i,j,minidx;
    real timeswap;
    char *chptr;

    for(i=0;i<nfile;i++) {
	minidx=i;
	for(j=i+1;j<nfile;j++) {
	    if(settime[j]<settime[minidx])
		minidx=j;
	}
	if(minidx!=i) {
	    timeswap=settime[i];
	    settime[i]=settime[minidx];
	    settime[minidx]=timeswap;
	    chptr=fnms[i];
	    fnms[i]=fnms[minidx];
	    fnms[minidx]=chptr;
	}
    }
}


static int scan_ene_files(char **fnms,int nfiles,real *readtime, real *timestep)
{
    /* Check number of energy terms and start time of all files */
    int i,in,nre,ndr,nresav=0,step;
    real t1,t2;
    char      **enm=NULL;
    t_drblock *dr=NULL;
    t_energy  *ee=NULL;
    
    for(i=0;i<nfiles;i++) {
	in = open_enx(fnms[i],"r");
	do_enxnms(in,&nre,&enm);

	if (i == 0) {
	    nresav = nre;
	    snew(ee,nre);
	    do_enx(in,&t1,&step,&nre,ee,&ndr,dr);
	    do_enx(in,&t2,&step,&nre,ee,&ndr,dr);
	    *timestep=t2-t1;
	    readtime[i]=t1;
	    close_enx(in);
	}
	else if (nre != nresav) {
	    fatal_error(0,"Energy files don't match, different number"
			" of energies (%s)",fnms[i]);
	}
	else {
	    do_enx(in,&t1,&step,&nre,ee,&ndr,dr);
	    readtime[i]=t1;
	    close_enx(in);
	}
	fprintf(stderr,"\n");  
    }
    sfree(ee);
    return nre;
}


static void edit_files(char **fnms,int nfiles,real *readtime, 
		       real *settime,int *cont_type,bool bSetTime,bool bSort)
{
  int i;
  bool ok;
  char inputstring[STRLEN],*chptr;
  
  if(bSetTime) {
    if(nfiles==1)
      fprintf(stderr,"\n\nEnter the new start time:\n\n");
    else
      fprintf(stderr,"\n\nEnter the new start time for each file.\n"
	      "There are two special options, both disables sorting:\n\n"
	      "c (continue) - The start time is taken from the end\n"
	      "of the previous file. Use it when your continuation run\n"
	      "restarts with t=0 and there is no overlap.\n\n"
	      "l (last) - The time in this file will be changed the\n"
	      "same amount as in the previous. Use it when the time in the\n"
		"new run continues from the end of the previous one,\n"
	      "since this takes possible overlap into account.\n\n");
    
    fprintf(stderr,"          File             Current start       New start\n"
	    "---------------------------------------------------------\n");
    
    for(i=0;i<nfiles;i++) {
      fprintf(stderr,"%25s   %10.3f             ",fnms[i],readtime[i]);
      ok=FALSE;
      do {
	fgets(inputstring,STRLEN-1,stdin);
	inputstring[strlen(inputstring)-1]=0;
	
	if(inputstring[0]=='c' || inputstring[0]=='C') {
	  cont_type[i]=TIME_CONTINUE;
	  bSort=FALSE;
	  ok=TRUE;
	  settime[i]=FLT_MAX;
	}
	else if(inputstring[0]=='l' ||
		inputstring[0]=='L') {
	  cont_type[i]=TIME_LAST;
	  bSort=FALSE;
	  ok=TRUE;
	  settime[i]=FLT_MAX;			  
	}
	else {
	  settime[i]=strtod(inputstring,&chptr);
	  if(chptr==inputstring) {
	    fprintf(stderr,"Try that again: ");
	  }
	  else {
	    cont_type[i]=TIME_EXPLICIT;
	    ok=TRUE;
	  }
	}
      } while (!ok);
    }
    if(cont_type[0]!=TIME_EXPLICIT) {
      cont_type[0]=TIME_EXPLICIT;
      settime[0]=0;
    }
  }
  else 
    for(i=0;i<nfiles;i++)
      settime[i]=readtime[i];
  
  if(bSort && (nfiles>1)) 
    sort_files(fnms,settime,nfiles);
  else
    fprintf(stderr,"Sorting disabled.\n");
  
  
  /* Write out the new order and start times */
  fprintf(stderr,"\nSummary of files and start times used:\n\n"
	  "          File                Start time\n"
	  "-----------------------------------------\n");
  for(i=0;i<nfiles;i++)
    switch(cont_type[i]) {
    case TIME_EXPLICIT:
      fprintf(stderr,"%25s   %10.3f\n",fnms[i],settime[i]);
      break;
    case TIME_CONTINUE:
      fprintf(stderr,"%25s        Continue from end of last file\n",fnms[i]);
      break;	      
    case TIME_LAST:
      fprintf(stderr,"%25s        Change by same amount as last file\n",fnms[i]);
      break;
    }
  fprintf(stderr,"\n");
  
  settime[nfiles]=FLT_MAX;
  cont_type[nfiles]=TIME_EXPLICIT;
  readtime[nfiles]=FLT_MAX;
}


static void copy_ee(t_energy *src, t_energy *dst, int nre)
{
  int i;

  for(i=0;i<nre;i++) {
    dst[i].e=src[i].e;
    dst[i].esum=src[i].esum;  
    dst[i].eav=src[i].eav;
  }
}


static void remove_last_eeframe(t_energy *lastee, int laststep,
				t_energy *ee, int nre)
{
    int i;
    int p=laststep+1;
    double sigmacorr;
    
    for(i=0;i<nre;i++) {
	lastee[i].esum-=ee[i].e;
	sigmacorr=lastee[i].esum-(p-1)*ee[i].e;
	lastee[i].eav-=(sigmacorr*sigmacorr)/((p-1)*p);
    }
}



static void update_ee(t_energy *lastee,int laststep,
		      t_energy *startee,int startstep,
		      t_energy *ee, int step,
		      t_energy *outee, int nre)
{
  int i; 
  double sigmacorr,nom,denom;
  double prestart_esum;
  double prestart_sigma;
  
  for(i=0;i<nre;i++) {
	  outee[i].e=ee[i].e;
      /* add statistics from earlier file if present */
      if(laststep>0) {
	  outee[i].esum=lastee[i].esum+ee[i].esum;
	  nom=(lastee[i].esum*(step+1)-ee[i].esum*(laststep));
	  denom=((step+1.0)*(laststep)*(step+1.0+laststep));	
	  sigmacorr=nom*nom/denom;
	  outee[i].eav=lastee[i].eav+ee[i].eav+sigmacorr;
      }  
      else {
	  /* otherwise just copy to output */
	  outee[i].esum=ee[i].esum;
	  outee[i].eav=ee[i].eav; 
      }
      
      /* if we didnt start to write at the first frame
       * we must compensate the statistics for this
       * there are laststep frames in the earlier file
       * and step+1 frames in this one.
       */
    if(startstep>0) {
      int q=laststep+step; 
      int p=startstep+1;
      prestart_esum=startee[i].esum-startee[i].e;
      sigmacorr=prestart_esum-(p-1)*startee[i].e;
      prestart_sigma=startee[i].eav-
	sigmacorr*sigmacorr/(p*(p-1));
      sigmacorr=prestart_esum/(p-1)-
	outee[i].esum/(q);
      outee[i].esum-=prestart_esum;
      outee[i].eav=outee[i].eav-prestart_sigma-
	sigmacorr*sigmacorr*((p-1)*q)/(q-p+1);
    }
 
    if((outee[i].eav/(laststep+step+1))<(GMX_REAL_EPS))
      outee[i].eav=0;
  }
}


static void update_last_ee(t_energy *lastee, int laststep,
			   t_energy *ee,int step,int nre)
{
    t_energy *tmp;
    snew(tmp,nre);
    update_ee(lastee,laststep,NULL,0,ee,step,tmp,nre);
    copy_ee(tmp,lastee,nre);
    sfree(tmp);
}


int main(int argc,char *argv[])
{
  static char *desc[] = {
    "When [TT]-f[tt] is [IT]not[it] specified:[BR]",
    "Concatenates several energy files in sorted order.",
    "In case of double time frames the one",
    "in the later file is used. By specifying [TT]-settime[tt] you will be",
    "asked for the start time of each file. The input files are taken",
    "from the command line,",
    "such that the command [TT]eneconv -o fixed.edr *.edr[tt] should do",
    "the trick. [PAR]",
    "With [TT]-f[tt] specified:[BR]",
    "Reads one energy file and writes another, applying the [TT]-dt[tt],",
    "[TT]-offset[tt], [TT]-t0[tt] and [TT]-settime[tt] options and",
    "converting to a different format if necessary (indicated by file",
    "extentions).[PAR]",
    "[TT]-settime[tt] is applied first, then [TT]-dt[tt]/[TT]-offset[tt]",
    "followed by [TT]-b[tt] and [TT]-e[tt] to select which frames to write."
  };
  static char *bugs[] = {
    "When combining trajectories the sigma and E^2 (necessary for statistics) are not updated correctly. Only the actual energy is correct. One thus has to compute statistics in another way."
  };
  int       in,out=0;
  t_energy  *ee,*lastee,*outee,*startee;
  int       step,laststep,outstep,startstep;
  int       nre,nfile,i,j,ndr;
  real      t=0,outt=-1; 
  char      **fnms;
  char      **enm=NULL;
  t_drblock *dr=NULL;
  real      *readtime,*settime,timestep,t1,tadjust;
  char      inputstring[STRLEN],*chptr;
  bool      ok;
  int       *cont_type;
  bool      bNewFile,bFirst,bNewOutput;
  
  t_filenm fnm[] = {
    { efENX, "-f", NULL,    ffREAD  },
    { efENX, "-o", "fixed", ffOPTWR },
  };

#define NFILE asize(fnm)  
  bool   bWrite;
  static real  delta_t=0.0, toffset=0;
  static bool  bSetTime=FALSE;
  static bool  bSort=TRUE,bError=TRUE;
  static real  begin=-1;
  static real  end=-1;
  
  t_pargs pa[] = {
    { "-b",        FALSE, etREAL, {&begin},
      "First time to use"},
    { "-e",        FALSE, etREAL, {&end},
      "Last time to use"},
    { "-dt",       FALSE, etREAL, {&delta_t},
      "Only write out frame when t MOD dt = offset" },
    { "-offset",   FALSE, etREAL, {&toffset},
      "Time offset for -dt option" }, 
    { "-settime",  FALSE, etBOOL, {&bSetTime}, 
      "Change starting time interactively" },
    { "-sort",     FALSE, etBOOL, {&bSort},
      "Sort energy files (not frames)"},
    { "-error",    FALSE, etBOOL, {&bError},
      "Stop on errors in the file" }
  };
  
  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,PCA_NOEXIT_ON_ARGS,TRUE,
		    NFILE,fnm,asize(pa),pa,asize(desc),desc,asize(bugs),bugs);
  tadjust=0;
  snew(fnms,argc);
  nfile=0;
  outstep=laststep=startstep=0;
  
  for(i=1; (i<argc); i++)
    fnms[nfile++]=argv[i];
  if(nfile==0)
    nfile=1;
  snew(settime,nfile+1);
  snew(readtime,nfile+1);
  snew(cont_type,nfile+1);
  
  if (!opt2bSet("-f",NFILE,fnm)) {
    if(!nfile)
      fatal_error(0,"No input files!");
 }
  else {
    /* get the single filename */
    fnms[0]=opt2fn("-f",NFILE,fnm);
  }

  nre=scan_ene_files(fnms,nfile,readtime,&timestep);   
  edit_files(fnms,nfile,readtime,settime,cont_type,bSetTime,bSort);     

  snew(ee,nre);
  snew(outee,nre);

  if(nfile>1)
    snew(lastee,nre);
  else
    lastee=NULL;

  if(begin>0)
    snew(startee,nre);
  else
    startee=NULL;

  bFirst=TRUE;

  for(i=0;i<nfile;i++) {
    bNewFile=TRUE;
    bNewOutput=TRUE;
    in=open_enx(fnms[i],"r");
    do_enxnms(in,&nre,&enm);
    if(i==0) {
      /* write names to the output file */
      out=open_enx(opt2fn("-o",NFILE,fnm),"w");  
      do_enxnms(out,&nre,&enm);
    }
    
    /* start reading from the next file */
    while((t<(settime[i+1]-GMX_REAL_EPS)) &&
	  do_enx(in,&t1,&step,&nre,ee,&ndr,dr)) {
      if(bNewFile) {
	tadjust=settime[i]-t1;	  
	if(cont_type[i+1]==TIME_LAST) {
	  settime[i+1]=readtime[i+1]-readtime[i]+settime[i];
	  cont_type[i+1]=TIME_EXPLICIT;
	}
	bNewFile=FALSE;
      }
      t=tadjust+t1;

      bWrite = ((begin < 0) || ((begin >= 0) && (t >= (begin-GMX_REAL_EPS)))&& 
		(end   < 0) || ((end   >= 0) && (t <= (end+GMX_REAL_EPS))));
		
      if (bError)      
	if((end > 0) && (t>(end+GMX_REAL_EPS))) {
	  i=nfile;
	  break;
	}
      
      if (t >= (begin-GMX_REAL_EPS)) {
	if((bFirst)) {
	  bFirst=FALSE;
	  if(startee!=NULL)
	    copy_ee(ee,startee,nre);
	  startstep=step;		
	}
	update_ee(lastee,laststep,startee,startstep,ee,step,outee,nre);
	outstep=laststep+step-startstep;
      }	  
      
      /* determine if we should write it */
      if (bWrite && ((delta_t==0) || (bRmod(t-toffset,delta_t)))) {
	outt=t;
	if(bNewOutput) {
	  bNewOutput=FALSE;
	  fprintf(stderr,"\nContinue writing frames from t=%g, step=%d\n",
		  t,outstep);
	}
	do_enx(out,&outt,&outstep,&nre,outee,&ndr,dr);
	fprintf(stderr,"\rWriting step %d, time %f        ",outstep,outt);
      }
    }
    /* copy statistics to old */
    if(lastee!=NULL) {
	update_last_ee(lastee,laststep,ee,step,nre);
	laststep+=step;
	/* remove the last frame from statistics since gromacs2.0 
	 * repeats it in the next file 
	 */
	remove_last_eeframe(lastee,laststep,ee,nre);
	/* the old part now has (laststep) values, and the new (step+1) */
	printf("laststep=%d step=%d\n",laststep,step);
    }
    
    /* set the next time from the last in previous file */
    if(cont_type[i+1]==TIME_CONTINUE) {
	settime[i+1]=outt;
	/* in this case we have already written the last frame of
	 * previous file, so update begin to avoid doubling it
	 * with the start of the next file
	 */
	begin=outt+0.5*timestep;
	/* cont_type[i+1]==TIME_EXPLICIT; */
    }
    
    if((outt<end) && (i<(nfile-1)) &&
       (outt<(settime[i+1]-1.5*timestep))) 
      fprintf(stderr,
	      "\nWARNING: There might be a gap around t=%g\n",t);
    
    /* move energies to lastee */
    close_enx(in);
    
    fprintf(stderr,"\n");
  }
  if(outstep==0)
      fprintf(stderr,"No frames written.\n");
  else
      fprintf(stderr,"Last frame written was at step %d, time %f\n",outstep,outt);
  
  thanx(stderr);
  return 0;
}
