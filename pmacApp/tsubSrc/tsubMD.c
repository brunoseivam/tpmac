/* @(#) tsubMD.c 2.2 2004/05/31 -- speed propagation */

/* tsubMD.c - Transformation Subroutines for Modular Motors */
/*            This is for 1-motor assemblies -- Stepanov    */

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<math.h>

#include	<dbDefs.h>
#include	<tsubRecord.h>
#include	<dbCommon.h>
#include	<recSup.h>
#include	<epicsExport.h>		/* Sergey */
#include	<registryFunction.h>	/* Sergey */

volatile int tsubMDDebug = 0;
#define TSUB_MESSAGE	logMsg
#define TSUB_TRACE(level,code) { if ( (pRec->tpro == (level)) || (tsubMDDebug == (level)) ) { code } }

/* ===========================================
 * tsubMDCs - Assembly-X Initialization
 */
static long tsubMDCs (struct tsubRecord *pRec) {
	return (0);
}


/* ===========================================
 * tsubMDCsSync
 *	oa = m1:RqsPos
 *	a  = m1:ActPos
 */
static long tsubMDCsSync (struct tsubRecord *pRec) {
	pRec->oa = pRec->a;
	return (0);
}


/* ===========================================
 * tsubMDCsMtr - Assembly-X Motors
 *	oa1 = d1
 *	a = m1
 *	a0 = d1:Offset
 *	a1 = d1:Scale
 *	nla = (1=wo/Offsets)[rel,vel] (0=w/Offsets)[abs,pos]
 */
static long tsubMDCsMtr (struct tsubRecord *pRec) {
	if (pRec->nla == 0.0)
	{
		pRec->oa1 = pRec->a * pRec->a1 + pRec->a0;         /* d1=m1*scale1+offset1 */
	}
	else
	{
		pRec->oa1 = pRec->a * pRec->a1;                    /* d1=m1*scale1 */
	}
	return (0);
}

/* ===========================================
 * tsubMDCsDrv - Assembly-X Drives
 *	oa0 = m1
 *	a = d1
 *	a0 = d1:Offset
 *	a1 = d1:Scale
 *	nla = (1=wo/Offsets)[rel,vel] (0=w/Offsets)[abs,pos]
 */
static long tsubMDCsDrv (struct tsubRecord *pRec) {
	if ( pRec->a1 == 0.0 )
	{
		return (-1);
	}
	if (pRec->nla == 0.0)
	{
		pRec->oa0 = (pRec->a - pRec->a0) / pRec->a1;       /* m1=(d1-offset1)/scale1 */
	}
	else
	{
		pRec->oa0 = pRec->a / pRec->a1;                    /* m1=d1/scale1 */
	}
	return (0);
}

/* ===========================================
 * tsubMDCsSpeed - Speed propagation spreadsheet
 *	oa0 = m1
 *	oa1 = d1
 *      oj  = sdis (sdis=1 -- disable record processing)
 *	a = m1_max
 *	a0 = m1
 *	a1 = d1
 *	a3 = d1:Scale
 *      nla = Index of input(m1=1, d1=11)
 */
static long tsubMDCsSpeed (struct tsubRecord *pRec) {
	double prcn = 0.0;
	double m1, d1;
	long ifail = 0;
/* Disable next record processing in order to avoid infinite loop.
 * This actually points to a record linked to the SDIS field of the tsub.
 * The SDIS has to be re-enabled before next tsub record call */
   	pRec->oj = 1;                                    /* sdis=1 */

  	if (tsubMDDebug > 1) printf ("tsubMDCsSpeed: called with n=%f \n",pRec->nla);

	if ( pRec->a  == 0.0 || pRec->a3 == 0.0 ) {
	   printf ("tsubMDCsSpeed: exit on zero calc parameters\n");
	   printf ("tsubMDCsSpeed: max1=%5.2f sca1=%g\n",pRec->a,pRec->a3);
           return (-1);
	}

	if      (pRec->nla ==  1.0)  /* ------------------------- m1 speed changed */
	{
	   if ( pRec->a0 == 0.0 ) prcn = 1.0;              /* if 0, then set to max */
	   else prcn = fabs( pRec->a0 / pRec->a );         /* prcn=m1/m1_max */
	}
	else if (pRec->nla == 11.0) /* ------------------------- d1 speed changed */
	{
	   if ( pRec->a1  < 0.0 ) ifail = -1;              /* x,d speeds are always > 0 */
	   if ( pRec->a1 <= 0.0 ) prcn = 1.0;              /* if 0, then set to max */
/* m speed must have same sign as m_max */
	   else prcn = fabs( pRec->a1 / (1000.0 * pRec->a * pRec->a3) );
	}
	else
	{
	   return (-1);
	}

	if ( prcn < 0.0 || prcn > 1.0 ) {
/* If **anything** is wrong set speed to normal */
          ifail = -1;
	  prcn = 1.0;
	}
	m1 = prcn * pRec->a;                             /* m1=prcn*m1_max */
	d1 = fabs( 1000.0 *  m1 * pRec->a3 );            /* d1=m1*scale1 */
	pRec->oa0 = m1;
	pRec->oa1 = d1;
  	if (tsubMDDebug > 1) printf ("tsubMDCsSpeed: m1=%5.2f\n",m1);
	return (ifail);
}

/* ===========================================
 *               Names registration
 *  =========================================== */
static registryFunctionRef tsubMDCsRef[] = {
    {"tsubMDCs",      (REGISTRYFUNCTION)tsubMDCs},
    {"tsubMDCsSync",  (REGISTRYFUNCTION)tsubMDCsSync},
    {"tsubMDCsMtr",   (REGISTRYFUNCTION)tsubMDCsMtr},
    {"tsubMDCsDrv",   (REGISTRYFUNCTION)tsubMDCsDrv},
    {"tsubMDCsSpeed", (REGISTRYFUNCTION)tsubMDCsSpeed}
};

static void tsubMDCsFunc(void) {				/* declare this via registrar in DBD */
    registryFunctionRefAdd(tsubMDCsRef,NELEMENTS(tsubMDCsRef));
}
epicsExportRegistrar(tsubMDCsFunc);

