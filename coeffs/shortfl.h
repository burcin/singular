#ifndef SHORTFL_H
#define SHORTFL_H
/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/*
* ABSTRACT
*/
/* $Id$ */
#include "coeffs.h"

BOOLEAN nrGreaterZero (number k, const coeffs r);
number  nrMult        (number a, number b, const coeffs r);
number  nrInit        (int i, const coeffs r);
int     nrInt         (number &n, const coeffs r);
number  nrAdd         (number a, number b, const coeffs r);
number  nrSub         (number a, number b, const coeffs r);
void    nrPower       (number a, int i, number * result, const coeffs r);
BOOLEAN nrIsZero      (number a, const coeffs r);
BOOLEAN nrIsOne       (number a, const coeffs r);
BOOLEAN nrIsMOne      (number a, const coeffs r);
number  nrDiv         (number a, number b, const coeffs r);
number  nrNeg         (number c, const coeffs r);
number  nrInvers      (number c, const coeffs r);
BOOLEAN nrGreater     (number a, number b, const coeffs r);
BOOLEAN nrEqual       (number a, number b, const coeffs r);
void    nrWrite       (number &a, const coeffs r);
const char *  nrRead  (const char *s, number *a, const coeffs r);
int     nrGetChar();
#ifdef LDEBUG
BOOLEAN nrDBTest(number a, const coeffs, rconst char *f, const int l);
#endif

nMapFunc nrSetMap(const coeffs src, const coeffs dst);

float   nrFloat(number n);
number nrMapQ(number from, const coeffs r);
#endif

