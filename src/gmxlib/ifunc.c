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
 * Green Red Orange Magenta Azure Cyan Skyblue
 */
static char *SRCID_ifunc_c = "$Id$";

#include "typedefs.h"
#include "bondf.h"
#include "disre.h"

#define def_bonded(str,lstr,nra,nrpa,nrpb,ind,func)\
   {str,lstr,(nra),(nrpa),(nrpb),IF_BOND,                        (ind),(func)}
   
#define  def_angle(str,lstr,nra,nrpa,nrpb,ind,func)\
   {str,lstr,(nra),(nrpa),(nrpb),IF_BOND | IF_ATYPE,(ind),(func)}
   
#define   def_bond(str,lstr,nra,nrpa,nrpb,ind,func)\
   {str,lstr,(nra),(nrpa),(nrpb),IF_BOND | IF_CONNECT | IF_BTYPE,(ind),(func)}
   
#define  def_dummy(str,lstr,nra,nrpa)\
   {str,lstr,(nra),(nrpa),     0,IF_DUMMY | IF_CONNECT,     -1, unimplemented}
   
#define    def_shk(str,lstr,nra,nrpa,nrpb)\
   {str,lstr,(nra),(nrpa),(nrpb),IF_CONSTRAINT,             -1, unimplemented}

#define def_shkcon(str,lstr,nra,nrpa,nrpb)\
   {str,lstr,(nra),(nrpa),(nrpb),IF_CONSTRAINT | IF_CONNECT,-1, unimplemented}
   
#define     def_nb(str,lstr,nra, nrp)\
   {str,lstr,(nra), (nrp),     0,IF_NULL,                    -1,unimplemented}
   
#define   def_nofc(str,lstr)\
   {str,lstr,    0,     0,     0,IF_NULL,                    -1,unimplemented}

/* this MUST correspond to the enum in include/types/idef.h */
t_interaction_function interaction_function[F_NRE]=
{
  def_bond   ("BONDS",    "Bond",            2, 2, 2,  eNR_BONDS,  bonds    ),
  def_bond   ("G96BONDS", "G96Bond",         2, 2, 2,  eNR_BONDS,  g96bonds ),
  def_bond   ("MORSE",    "Morse",           2, 3, 0,  eNR_MORSE, morsebonds),
  def_angle  ("ANGLES",   "Angle",           3, 2, 2,  eNR_ANGLES, angles   ),
  def_angle  ("G96ANGLES","G96Angle",        3, 2, 2,  eNR_ANGLES, g96angles),
  def_bonded ("PDIHS",    "Proper Dih.",     4, 3, 3,  eNR_PROPER, pdihs    ),
  def_bonded ("RBDIHS",   "Ryckaert-Bell.",  4, 6, 0,  eNR_RB, rbdihs       ),
  def_bonded ("IDIHS",    "Improper Dih.",   4, 2, 2,  eNR_IMPROPER,idihs   ),
  def_bonded ("LJ14",     "LJ-14",           2, 2, 2,  eNR_LJC, do_14       ),
  def_nofc   ("COUL14",   "Coulomb-14"       ),
  def_nb     ("LJ",       "LJ (SR)",         2, 2      ),
  def_nb     ("BHAM",     "BuckingHam",      2, 3      ),
  def_nofc   ("LJLR",     "LJ (LR)"          ),
  def_nofc   ("DISPCORR", "Disper. corr."    ),
  def_nofc   ("SR",       "Coulomb (SR)"     ),
  def_nofc   ("LR",       "Coulomb (LR)"     ),
  def_bonded ("WATERPOL", "Water Pol.",      1, 6, 0,  eNR_WPOL,   water_pol),
  def_bonded ("POSRES",   "Position Rest.",  1, 3, 0,  eNR_POSRES, posres   ),
  def_bonded ("DISRES",   "Dis. Res",        2, 6, 0,  eNR_DISRES, ta_disres),
  def_bonded ("ANGRES",   "Angle Rest.",     4, 3, 3,  eNR_ANGRES, angres   ),
  def_bonded ("ANGRESZ",  "Angle Rest. Z",   2, 3, 3,  eNR_ANGRESZ,angresz  ),
  def_shkcon ("CONSTR",   "Constraint",      2, 1, 1   ),
  def_shk    ("CONSTRNC", "Constr. No Conn.",2, 1, 1   ),
  def_shk    ("SETTLE",   "Settle",          1, 2, 0   ),
  def_dummy  ("DUMMY2",   "Dummy2",          3, 1      ),
  def_dummy  ("DUMMY3",   "Dummy3",          4, 2      ),
  def_dummy  ("DUMMY3FD", "Dummy3fd",        4, 2      ),
  def_dummy  ("DUMMY3FAD","Dummy3fad",       4, 2      ),
  def_dummy  ("DUMMY3OUT","Dummy3out",       4, 3      ),
  def_dummy  ("DUMMY4FD", "Dummy4fd",        5, 3      ),
  def_nofc   ("EPOT",     "Potential"        ),
  def_nofc   ("EKIN",     "Kinetic En."      ),
  def_nofc   ("ETOT",     "Total Energy"     ),
  def_nofc   ("TEMP",     "Temperature"      ),
  def_nofc   ("PRES",     "Pressure (bar)"   ),
  def_nofc   ("DV/DL",    "dVpot/dlambda"    ),
  def_nofc   ("DK/DL",    "dEkin/dlambda"    )
};

bool have_interaction(t_idef *idef,int ftype)
{
  int i;
  
  for(i=0; (i<idef->ntypes); i++)
    if (idef->functype[i] == ftype)
      return TRUE;
  return FALSE;
}
