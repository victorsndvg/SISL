/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/* (c) Copyright 1989,1990,1991,1992 by                                      */
/*     Senter for Industriforskning, Oslo, Norway                            */
/*     All rights reserved. See the copyright.h for more details.            */
/*                                                                           */
/*****************************************************************************/

#include "copyright.h"

/*
 *
 * $Id: s1333.c,v 1.4 1994-08-02 11:21:08 pfu Exp $
 *
 */


#define S1333

#include "sislP.h"

#if defined(SISLNEEDPROTOTYPES)
void
s1333(int inbcrv,SISLCurve *vpcurv[],int nctyp[],double astpar,
	   int iopen,int iord2,int iflag,
	   SISLSurf **rsurf,double **gpar,int *jstat)
#else
void s1333(inbcrv,vpcurv,nctyp,astpar,iopen,iord2,
           iflag,rsurf,gpar,jstat)
     int    	inbcrv;
     SISLCurve  *vpcurv[];
     int   	nctyp[];
     double	astpar;
     int    	iopen;
     int    	iord2;
     int    	iflag;
     SISLSurf   **rsurf;
     double 	**gpar;
     int    	*jstat;
#endif
/*
*********************************************************************
*
* PURPOSE    : To create a spline lofted surface
*              from a set of input-curves.
*
* INPUT      : inbcrv - Number of curves in the curve-set.
*              vpcurv  - Array (length inbcrv) of pointers to the
*                       curves in the curve-set.
*              nctyp  - Array (length inbcrv) containing the types
*                       of curves in the curve-set.
*                        1 - Ordinary curve.
*                        2 - Knuckle curve. Treated as ordinary curve.
*                        3 - Tangent to next curve.
*                        4 - Tangent to prior curve.
*                       (5 - Double derivative to prior curve.)
*                       (6 - Double derivative to next curve.)
*                       13 - SISLCurve giving start of tangent to next curve.
*                       14 - SISLCurve giving end of tengent to prior curve.
*              astpar - Start-parameter for spline lofting direction.
*              iopen  - Flag telling if the resulting surface should
*                       be closed or open.
*                       -1 - The surface should be closed and periodic.
*                        0 - The surface should be closed.
*                        1 - The surface should be open.
*              iord2  - Maximal order of the B-spline basis in the
*                       lofting direction.
*              iflag  - Flag telling if the size of the tangents in the
*                       derivative curves should be adjusted or not.
*                        0 - Do not adjust tangent-sizes.
*                        1 - Adjust tangent-sizes.
*
* OUTPUT     : rsurf  - Pointer to the surface produced.
*              gpar   - The input-curves are constant parameter-lines
*                       in the parameter-plane of the produced surface.
*                       (i) - contains the (constant) value of this
*                             parameter of input-curve no. i.
*              jstat  - status messages
*                                         > 0      : warning
*                                         = 0      : ok
*                                         < 0      : error
*
* METHOD     : A common basis for all the B-spline curves are found.
*              The curves are represented using this basis.
*              The resulting curves are given to an interpolation
*              routine that calculates the B-spline vertices of the
*              resulting spline lofted surface.
*              Throughout these routines, first parameterdirection
*              will be the interpolating direction, second parameter-
*              direction will be along the input curves.
*-
* CALLS      : s1931,s1917,s1918,s1358,s6err.
*
* WRITTEN BY : A. M. Ytrehus   SI  Oslo,Norway. Sep. 1988
* Revised by : Tor Dokken, SI, Oslo, Norway, 26-feb-1989
* Revised by : Trond Vidar Stensby, SI, 91-08
* Revised by : Paal Fugelli, SINTEF, Oslo 02/08-1994. Fixed memory leak.
*********************************************************************
*/
{
  int kind,kcopy,kdim;
  int kn1,kord1,knbcrv;
  int kcnsta,kcnend;         /* Interpolation condition at start or end */
  int ki,kj,kl,km;
  int kleng;                 /* Number of doubles describing a curve  */
  int ktype;                 /* Kind of interpolation condition.      */
  int kopen;                 /* Open/closed parameter in curve direction. */
  SISLCurve *qc;             /* Pointer to curve representing surface */
  int *lder = NULL;	     /* Derivative indicators from s1915. */
  double *spar=NULL; 	     /* Param. values of point conditions. */
  double *spar2=NULL; 	     /* Parameter values from s1915. */
  double *sknot1=NULL;       /* Knot vector.                 */
  double *scoef2=NULL;       /* Pointer to vertices expressed in same basis  */
  double tstpar;             /* Parameter value of last curve                */
  int kstat = 0;             /* Status variable. */
  int kpos = 0;              /* Position of error. */
  int knbpar;                /* Number of parameter values produced          */
  int kdimcrv;               /* kdim multiplied with number of vertices kn1  */
  int kcont;                 /* Continuity at end of curves */
  double *etyp = NULL;

/* -> new statement guen & ujk Thu Jul  2 14:59:05 MESZ 1992 */

/* make compatible to old use of s1604 */
  if (iopen==SISL_CRV_CLOSED) iopen = SISL_CRV_PERIODIC;

/* a new version with input-iopen == rc->cuopen
   should be made, with a name different from any
   other.
  NOTE: There is an error in this function when iopen = 0
        qc as input to s1713 (and s1750) then has wrong flag !!*/

/* <- new statement guen & ujk Thu Jul  2 14:59:05 MESZ 1992 */

/* -> guen & ujk Wed Jul  1 17:06:46 MESZ 1992 */

  etyp = newarray (inbcrv, DOUBLE);
  if (etyp == NULL)
    goto err101;
/* <- guen & ujk Wed Jul  1 17:06:46 MESZ 1992 */

  /* Initiate variables. */

  kdim = vpcurv[0]->idim;

  if (inbcrv < 2) goto err179;

  /* Put the curves into common basis. */

  s1931 (inbcrv, vpcurv, &sknot1, &scoef2, &kn1, &kord1, &kstat);
  if (kstat < 0)
    goto error;

  /* Create the parameter-values for the knot-vector
     (in lofting direction) for a lofted surface, allocate array for
     parameter values.    */

  s1917 (inbcrv, scoef2, kn1, kdim, nctyp, astpar, iopen,
	 &spar2, &lder, &knbcrv, &kstat);

  if (kstat < 0)
    goto error;

  /* Convert condition 13 and 14 to 3 and 4 */

  kleng = kn1*kdim;
  for (ki=0 ; ki<knbcrv ; ki++)
    {
       ktype = nctyp[ki];

      if (ktype == 13 && ki+1<knbcrv)
        {
	  /*
	   * Start of tangent to next curve,
	   * make difference of next curve and this curve
	   */

	  for (kj=ki*kleng,kl=kj+kleng,km=0; km <kleng ; kj++,kl++,km++)
	      scoef2[kj] = scoef2[kl] - scoef2[kj];
	  nctyp[ki] = 3;
        }
      else if (ktype == 14 && ki>0)
        {
	  /* End of tangent to prior curve,
	   * make difference of this curve
	   * and prior curve
	   */

	  for (kj=ki*kleng,kl=kj-kleng,km=0; km <kleng ; kj++,kl++,km++)
	      scoef2[kj] = scoef2[kj] - scoef2[kl];
	  nctyp[ki] = 4;
        }
    }

  spar = newarray(knbcrv+1,DOUBLE);
  if (spar==NULL) goto err101;

  /*  Only copy parameter values of point conditions */

  for (ki=0,kl=0; ki<knbcrv ; ki++)
    {
      if (nctyp[ki] == 1 || nctyp[ki] == 2)
        {
	  spar[kl] = spar2[ki];
	  kl++;
        }
    }

  /* Add one extra parameter value if closed curve */

  if (iopen != SISL_CRV_OPEN) spar[kl] = spar2[knbcrv];

  /* Adjust tangent-lengths if wanted. */

  if (iflag)
    {
      s1918 (knbcrv, sknot1, scoef2, kn1, kord1, kdim, spar2, lder, &kstat);
      if (kstat < 0) goto error;
    }

  /* Interpolate with point interpolation method */

  kcnsta = 0;
  kcnend = 0;
  kdimcrv = kdim*kn1;

/* -> changed guen & ujk Wed Jul  1 19:45:25 MESZ 1992 */
/*  s1358(scoef2,knbcrv,kdimcrv,nctyp,spar,kcnsta,kcnend,iopen,iord2,astpar,*/
/*	&tstpar,&qc,gpar,&knbpar,&kstat); 			*/

  for (ki=0; ki < inbcrv; ki++) etyp[ki] = (double)nctyp[ki];

  *gpar = NULL;  /* PFU 02/08-1994 */
  s1358(scoef2,knbcrv,kdimcrv,etyp,spar,kcnsta,kcnend,iopen,iord2,astpar,
	&tstpar,&qc,gpar,&knbpar,&kstat);
  if (kstat<0) goto error;

  if (*gpar) freearray(*gpar);  /* 'gpar' not used.  PFU 02/08-1994. */

/* -> changed guen & ujk Wed Jul  1 19:45:25 MESZ 1992 */

  /* The knot vector in the lofting direction and the coefficients are
     now contained in the curve object pointed to by qc */

  /* Create the surface */

  kind = 1;
  kcopy = 1;
  *rsurf = newSurf(kn1,qc->in,kord1,qc->ik,sknot1,qc->et,qc->ecoef,
		   kind,kdim,kcopy);
  if (*rsurf == NULL) goto err101;

  /* Copy cuopen flag from curve */
  (*rsurf)->cuopen_2 = qc->cuopen;

  /* Release the curve object */

  freeCurve(qc);

  /* Output parametervalues according to the input curves  */

  *gpar = spar;

  /* Decide if the surface should have a cyclic behaviour in first
     parameter direction i.e. the direction of the curves */

  s1333_count(inbcrv,vpcurv,&kcont,&kstat);
  if (kstat<0) goto error;

  if (kcont>=0)
      {
        s1333_cyclic(*rsurf,kcont,&kstat);
	if (kstat<0) goto error;

	/* Set periodic flag */
	(*rsurf)->cuopen_1 = SISL_SURF_PERIODIC;
      }
      else
      {
         /* Test if the surface should be closed and non-periodic.  */

         for (kopen=-2, ki=0; ki<inbcrv; ki++)
           kopen = MAX(kopen,vpcurv[ki]->cuopen);
         if (kopen == SISL_CRV_CLOSED) (*rsurf)->cuopen_1 = SISL_SURF_CLOSED;
      }

  /* Task done */

  *jstat = 0;
  goto out;

  /* Error in allocation. */

 err101:
  *jstat = -101;
  s6err("s1333",*jstat,kpos);
  goto out;


  /* Error in interpolation conditions. No. of curves < 2. */

 err179:
  *jstat = -179;
  s6err("s1333",*jstat,kpos);
  goto out;


  /* Error in lower level routine.  */

  error :
    *jstat = kstat;
  s6err("s1333",*jstat,kpos);
  goto out;
 out:

  /* Free allocated scratch  */

  if (sknot1 != NULL) freearray(sknot1);
  if (scoef2 != NULL) freearray(scoef2);
  if (spar2 != NULL) freearray(spar2);
  if (lder != NULL) freearray(lder);

/* -> added, guen & ujk Wed Jul  1 18:48:27 MESZ 1992 */
  if (etyp != NULL)
    freearray (etyp);
/* <- added, guen & ujk Wed Jul  1 18:48:27 MESZ 1992 */

  return;
}
