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
 * Great Red Oystrich Makes All Chemists Sane
 */
static char *SRCID_mkice_c = "$Id$";

#include <stdio.h>
#include <math.h>
#include "typedefs.h"
#include "statutil.h"
#include "copyrite.h"
#include "fatal.h"
#include "xvgr.h"
#include "pdbio.h"
#include "macros.h"
#include "smalloc.h"
#include "vec.h"
#include "pbc.h"
#include "physics.h"
#include "names.h"
#include "txtdump.h"
#include "trnio.h"
#include "symtab.h"
#include "confio.h"

real pot(real x,real qq,real c6,real c12)
{
  return c12*pow(x,-12)-c6*pow(x,-6)+qq*ONE_4PI_EPS0/x;
}

real dpot(real x,real qq,real c6,real c12)
{
  return -(12*c12*pow(x,-13)-6*c6*pow(x,-7)+qq*ONE_4PI_EPS0/sqr(x));
}

int main(int argc,char *argv[])
{
  static char *desc[] = {
    "Plot the potential"
  };
  static real c6=1.0e-3,c12=1.0e-6,qi=1,qj=2,sig=0.3,eps=1,sigfac=0.7;
  t_pargs pa[] = {
    { "-c6",   FALSE,  etREAL,  {&c6},  "c6"   },
    { "-c12",  FALSE,  etREAL,  {&c12}, "c12"  },
    { "-sig",  FALSE,  etREAL,  {&sig}, "sig"  },
    { "-eps",  FALSE,  etREAL,  {&eps}, "eps"  },
    { "-qi",   FALSE,  etREAL,  {&qi},  "qi"   },
    { "-qj",   FALSE,  etREAL,  {&qj},  "qj"   },
    { "-sigfac", FALSE, etREAL, {&sigfac}, "Factor in front of sigma for starting the plot" }
  };
  t_filenm fnm[] = {
    { efXVG, "-o", "potje", ffWRITE }
  };
#define NFILE asize(fnm)

  FILE      *fp;
  int       i;
  real      qq,x,oldx,minimum,mval,dp[2],pp[2];
  int       cur=0;
#define next (1-cur)
  
  /* CopyRight(stdout,argv[0]);*/
  parse_common_args(&argc,argv,PCA_CAN_VIEW,
		    FALSE,NFILE,fnm,asize(pa),pa,asize(desc),
		    desc,0,NULL);

  if (opt2parg_bSet("-sig",asize(pa),pa) ||
      opt2parg_bSet("-eps",asize(pa),pa)) {
    c6  = 4*eps*pow(sig,6);
    c12 = 4*eps*pow(sig,12);
  }
  else if ((c6 != 0) && (c12 != 0)) {
    sig = pow(c12/c6,1.0/6.0);
    eps = c6*c6/(4*c12);
  }
  else {
    sig = eps = 0;
  }
  printf("c6    = %12.5e, c12     = %12.5e\n",c6,c12);
  printf("sigma = %12.5f, epsilon = %12.5f\n",sig,eps);
  qq = qi*qj;
      
  fp = xvgropen(ftp2fn(efXVG,NFILE,fnm),"Potential","r (nm)","E (kJ/mol)");
  if (sig == 0)
    sig=0.25;
  minimum = -1;
  mval    = 0;
  oldx    = 0;
  for(i=0; (i<100); i++) {
    x    = sigfac*sig+sig*i*0.02;
    dp[next] = dpot(x,qq,c6,c12);
    fprintf(fp,"%10g  %10g  %10g\n",x,pot(x,qq,c6,c12),
	    dp[next]);
    if ((i > 0) && (dp[cur]*dp[next] < 0)) {
      minimum = oldx + dp[cur]*(x-oldx)/(dp[cur]-dp[next]);
      mval    = pot(minimum,qq,c6,c12);
      /*fprintf(stdout,"dp[cur] = %g, dp[next] = %g  oldx = %g, dx = %g\n",
	dp[cur],dp[next],oldx,x-oldx);*/
      printf("Minimum at r = %g (nm). Value = %g (kJ/mol)\n",
	      minimum,mval);
    }
    cur = next;
    oldx = x;
      
  }
  fclose(fp);
  
  xvgr_file(ftp2fn(efXVG,NFILE,fnm),NULL);

  thanx(stderr);  
	       
  return 0;
}


