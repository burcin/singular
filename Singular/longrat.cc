/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/* $Id: longrat.cc,v 1.18 1999-04-29 11:38:49 Singular Exp $ */
/*
* ABSTRACT: computation with long rational numbers (Hubert Grassmann)
*/

#include <string.h>
#include "mod2.h"
#include "tok.h"
#include "mmemory.h"
#include "febase.h"
#include "numbers.h"
#include "modulop.h"
#include "longrat.h"

#ifndef BYTES_PER_MP_LIMB
#ifdef HAVE_LIBGMP2
#define BYTES_PER_MP_LIMB sizeof(mp_limb_t)
#else
#define BYTES_PER_MP_LIMB sizeof(mp_limb)
#endif
#endif

static int nlPrimeM;
static number nlMapP(number from)
{
  number to;
  int save=npPrimeM;
  npPrimeM=nlPrimeM;
  to = nlInit(npInt(from));
  npPrimeM=save;
  return to;
}

BOOLEAN nlSetMap(int c, char ** par, int nop, number minpol)
{
  if (c == 0)
  {
    nMap = nlCopy;   /*Q -> Q*/
    return TRUE;
  }
  if (c>1)
  {
    if (par==NULL)
    {
      nlPrimeM=c;
      nMap = nlMapP; /* Z/p -> Q */
      return TRUE;
    }
    else
     return FALSE;  /* GF(q) ->Q */
  }
  else if (c<0)
     return FALSE; /* Z/p(a) -> Q*/
  else if (c==1)
     return FALSE; /* Q(a) -> Q */
  return FALSE;
}

/*-----------------------------------------------------------------*/
/*3
* parameter s in number:
* 0 (or FALSE): not normalised rational
* 1 (or TRUE):  normalised rational
* 3          :  integer with n==NULL
*/
/*3
**  'SR_INT' is the type of those integers small enough to fit into  29  bits.
**  Therefor the value range of this small integers is: $-2^{28}...2^{28}-1$.
**
**  Small integers are represented by an immediate integer handle, containing
**  the value instead of pointing  to  it,  which  has  the  following  form:
**
**      +-------+-------+-------+-------+- - - -+-------+-------+-------+
**      | guard | sign  | bit   | bit   |       | bit   | tag   | tag   |
**      | bit   | bit   | 27    | 26    |       | 0     | 0     | 1     |
**      +-------+-------+-------+-------+- - - -+-------+-------+-------+
**
**  Immediate integers handles carry the tag 'SR_INT', i.e. the last bit is 1.
**  This distuingishes immediate integers from other handles which  point  to
**  structures aligned on 4 byte boundaries and therefor have last bit  zero.
**  (The second bit is reserved as tag to allow extensions of  this  scheme.)
**  Using immediates as pointers and dereferencing them gives address errors.
**
**  To aid overflow check the most significant two bits must always be equal,
**  that is to say that the sign bit of immediate integers has a  guard  bit.
**
**  The macros 'INT_TO_SR' and 'SR_TO_INT' should be used to convert  between
**  a small integer value and its representation as immediate integer handle.
**
**  Large integers and rationals are represented by z and n
**  where n may be undefined (if s==3)
**  NULL represents only deleted values
*/
#define SR_HDL(A) ((long)(A))
/*#define SR_INT    1*/
/*#define INT_TO_SR(INT)  ((number) (((long)INT << 2) + SR_INT))*/
#define SR_TO_INT(SR)   (((long)SR) >> 2)

#define MP_SMALL 1
//#define mpz_isNeg(A) (mpz_cmp_si(A,(long)0)<0)
#ifdef HAVE_LIBGMP1
#define mpz_isNeg(A) ((A)->size<0)
#define mpz_limb_size(A) ((A)->size)
#define mpz_limb_d(A) ((A)->d)
#define MPZ_DIV(A,B,C) mpz_div((A),(B),(C))
#define MPZ_EXACTDIV(A,B,C) mpz_div((A),(B),(C))
#else
#define mpz_isNeg(A) ((A)->_mp_size<0)
#define mpz_limb_size(A) ((A)->_mp_size)
#define mpz_limb_d(A) ((A)->_mp_d)
#define MPZ_DIV(A,B,C) mpz_tdiv_q((A),(B),(C))
#ifdef HAVE_SMALLGMP
#define MPZ_EXACTDIV(A,B,C) mpz_tdiv_q((A),(B),(C))
#else
#define MPZ_EXACTDIV(A,B,C) mpz_divexact((A),(B),(C))
#endif
#define nlGmpSimple(A)
#endif


#ifdef LDEBUG
#define nlTest(a) nlDBTest(a,__FILE__,__LINE__)
BOOLEAN nlDBTest(number a, char *f,int l)
{
  if (a==NULL)
  {
    Print("!!longrat: NULL in %s:%d\n",f,l);
    return FALSE;
  }
  //if ((int)a==1) Print("!! 0x1 as number ? %s %d\n",f,l);
  if ((((int)a)&3)==3)
  {
    Print(" !!longrat:ptr(3) in %s:%d\n",f,l);
    return FALSE;
  }
  if ((((int)a)&3)==1)
  {
    if (((((int)a)<<1)>>1)!=((int)a))
    {
      Print(" !!longrat:arith:%x in %s:%d\n",(int)a, f,l);
      return FALSE;
    }
    return TRUE;
  }
#ifdef MDEBUG
  mmDBTestBlock(a,sizeof(*a),f,l);
#endif
  if (a->debug!=123456)
  {
    Print("!!longrat:debug:%d in %s:%d\n",a->debug,f,l);
    a->debug=123456;
    return FALSE;
  }
  if ((a->s<0)||(a->s>4))
  {
    Print("!!longrat:s=%d in %s:%d\n",a->s,f,l);
    return FALSE;
  }
#ifdef MDEBUG
#ifdef HAVE_LIBGMP2
  mmDBTestBlock(a->z._mp_d,a->z._mp_alloc*BYTES_PER_MP_LIMB,f,l);
  if (a->z._mp_alloc==0)
#else
  mmDBTestBlock(a->z.d,a->z.alloc*BYTES_PER_MP_LIMB,f,l);
  if(a->z.alloc==0)
#endif
    Print("!!longrat:z->alloc=0 in %s:%l\n",f,l);
#endif
  if (a->s<2)
  {
#ifdef MDEBUG
#ifdef HAVE_LIBGMP2
    mmDBTestBlock(a->n._mp_d,a->n._mp_alloc*BYTES_PER_MP_LIMB,f,-l);
    if (a->z._mp_alloc==0)
#else
    mmDBTestBlock(a->n.d,a->n.alloc*BYTES_PER_MP_LIMB,f,-l);
    if(a->z.alloc==0)
#endif
      Print("!!longrat:n->alloc=0 in %s:%l\n",f,l);
#endif
    if (mpz_cmp_si(&a->n,(long)1)==0)
    {
      Print("!!longrat:integer as rational in %s:%d\n",f,l);
      mpz_clear(&a->n);
      a->s=3;
      return FALSE;
    }
    else if (mpz_isNeg(&a->n))
    {
      Print("!!longrat:div. by negative in %s:%d\n",f,l);
      mpz_neg(&a->z,&a->z);
      mpz_neg(&a->n,&a->n);
      return FALSE;
    }
    return TRUE;
  }
  if (a->s==2)
  {
    Print("!!longrat:s=2 in %s:%d\n",f,l);
    return FALSE;
  }
  if (mpz_size1(&a->z)>MP_SMALL) return TRUE;
  int ui=(int)mpz_get_si(&a->z);
  if ((((ui<<3)>>3)==ui)
  && (mpz_cmp_si(&a->z,(long)ui)==0))
  {
    Print("!!longrat:im int %d in %s:%d\n",ui,f,l);
    f=NULL;
    return FALSE;
  }
  return TRUE;
}
#endif

void nlNew (number * r)
{
  *r=NULL;
}

/*2
* z := i
*/
#ifdef MDEBUG
#define nlRInit(A) nlRDBInit(A,__LINE__)
static inline number nlRDBInit(int i, int n)
#else
static inline number nlRInit (int i)
#endif
{
#ifdef MDEBUG
  number z=(number)mmDBAllocBlock(sizeof(rnumber),"longrat.cc",n);
#else
  number z=(number)Alloc(sizeof(rnumber));
#endif
#ifdef LDEBUG
  z->debug=123456;
#endif
  mpz_init_set_si(&z->z,(long)i);
  z->s = 3;
  return z;
}

number nlInit (int i)
{
  number n;
  if ( ((i << 3) >> 3) == i ) n=INT_TO_SR(i);
  else                        n=nlRInit(i);
#ifdef LDEBUG
  nlTest(n);
#endif
  return n;
}


number nlInit (number u)
{
  if (u->s == 3 && mpz_size1(&u->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&u->z);
    if ((((ui<<3)>>3)==ui)
        && (mpz_cmp_si(&u->z,(long)ui)==0))
    {
      mpz_clear(&u->z);
      Free((ADDRESS)u,sizeof(rnumber));
      return INT_TO_SR(ui);
    }
  }
  return u;
}

int nlSize(number a)
{
  if (a==INT_TO_SR(0))
     return 0; /* rational 0*/
  if (SR_HDL(a) & SR_INT)
     return 1; /* immidiate int */
#ifdef HAVE_LIBGMP2
  int s=a->z._mp_alloc;
#else
  int s=a->z.alloc;
#endif
  if (a->s<2)
  {
#ifdef HAVE_LIBGMP2
    s+=a->n._mp_alloc;
#else
    s+=a->n.alloc;
#endif
  }
  return s;
}

/*
* delete leading 0s from a gmp number
*/
#ifdef HAVE_LIBGMP1
static void nlGmpSimple(MP_INT *z)
{
  int k;
  if (mpz_limb_size(z)<0)
  {
    k=-mpz_limb_size(z)-1;
    while ((k>0) && (mpz_limb_d(z)[k]==0))
    { k--;printf("X"); }
    k++;
    mpz_limb_size(z)=-k;
  }
  else
  {
    k=mpz_limb_size(z)-1;
    while ((k>0) && (mpz_limb_d(z)[k]==0))
    { k--;printf("X"); }
    k++;
    mpz_limb_size(z)=k;
  }
}
#endif

/*2
* convert number to int
*/
int nlInt(number &i)
{
#ifdef LDEBUG
  nlTest(i);
#endif
  nlNormalize(i);
  if (SR_HDL(i) &SR_INT) return SR_TO_INT(i);
  nlGmpSimple(&i->z);
  if ((i->s!=3)||(mpz_size1(&i->z)>MP_SMALL)) return 0;
  int ul=(int)mpz_get_si(&i->z);
  if (mpz_cmp_si(&i->z,(long)ul)!=0) return 0;
  return ul;
}

/*2
* delete a
*/
#ifdef LDEBUG
void nlDBDelete (number * a,char *f, int l)
#else
void nlDelete (number * a)
#endif
{
  if (*a!=NULL)
  {
#ifdef LDEBUG
    nlTest(*a);
#endif
    if ((SR_HDL(*a) & SR_INT)==0)
    {
      switch ((*a)->s)
      {
        case 0:
        case 1:
          mpz_clear(&(*a)->n);
        case 3:
          mpz_clear(&(*a)->z);
#ifdef LDEBUG
          (*a)->s=2;
          (*a)->debug=654321;
#endif
      }
#if defined(MDEBUG) && defined(LDEBUG)
      mmDBFreeBlock((ADDRESS) *a,sizeof(rnumber),f,l);
#else
      Free((ADDRESS) *a,sizeof(rnumber));
#endif
    }
    *a=NULL;
  }
}

/*2
* copy a to b
*/
number nlCopy(number a)
{
  number b;
  if ((SR_HDL(a) & SR_INT)||(a==NULL))
  {
    return a;
  }
#ifdef LDEBUG
  nlTest(a);
#endif
  b=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  b->debug=123456;
#endif
  switch (a->s)
  {
    case 0:
    case 1:
            nlGmpSimple(&a->n);
            mpz_init_set(&b->n,&a->n);
    case 3:
            nlGmpSimple(&a->z);
            mpz_init_set(&b->z,&a->z);
            break;
  }
  b->s = a->s;
#ifdef LDEBUG
  nlTest(b);
#endif
  return b;
}

/*2
* za:= - za
*/
number nlNeg (number a)
{
#ifdef LDEBUG
  nlTest(a);
#endif
  if(SR_HDL(a) &SR_INT)
  {
    int r=SR_TO_INT(a);
    if (r==(-(1<<28))) a=nlRInit(1<<28);
    else               a=INT_TO_SR(-r);
  }
  else
  {
    mpz_neg(&a->z,&a->z);
    if ((a->s==3) && (mpz_size1(&a->z)<=MP_SMALL))
    {
      int ui=(int)mpz_get_si(&a->z);
      if ((((ui<<3)>>3)==ui)
      && (mpz_cmp_si(&a->z,(long)ui)==0))
      {
        mpz_clear(&a->z);
        Free((ADDRESS)a,sizeof(rnumber));
        a=INT_TO_SR(ui);
      }
    }
  }
#ifdef LDEBUG
  nlTest(a);
#endif
  return a;
}

/*
* 1/a
*/
number nlInvers(number a)
{
#ifdef LDEBUG
  nlTest(a);
#endif
  number n;
  if (SR_HDL(a) & SR_INT)
  {
    if ((a==INT_TO_SR(1)) || (a==INT_TO_SR(-1)))
    {
      return a;
    }
    if (nlIsZero(a))
    {
      WerrorS("div. 1/0");
      return INT_TO_SR(0);
    }
    n=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
    n->debug=123456;
#endif
    n->s=1;
    if ((int)a>0)
    {
      mpz_init_set_si(&n->z,(long)1);
      mpz_init_set_si(&n->n,(long)SR_TO_INT(a));
    }
    else
    {
      mpz_init_set_si(&n->z,(long)-1);
      mpz_init_set_si(&n->n,(long)-SR_TO_INT(a));
#ifdef LDEBUG
    nlTest(n);
#endif
    }
#ifdef LDEBUG
    nlTest(n);
#endif
    return n;
  }
  n=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  n->debug=123456;
#endif
  {
    n->s=a->s;
    mpz_init_set(&n->n,&a->z);
    switch (a->s)
    {
      case 0:
      case 1:
              mpz_init_set(&n->z,&a->n);
              if (mpz_isNeg(&n->n)) /* && n->s<2*/
              {
                mpz_neg(&n->z,&n->z);
                mpz_neg(&n->n,&n->n);
              }
              if (mpz_cmp_si(&n->n,(long)1)==0)
              {
                mpz_clear(&n->n);
                n->s=3;
                if (mpz_size1(&n->z)<=MP_SMALL)
                {
                  int ui=(int)mpz_get_si(&n->z);
                  if ((((ui<<3)>>3)==ui)
                  && (mpz_cmp_si(&n->z,(long)ui)==0))
                  {
                    mpz_clear(&n->z);
                    Free((ADDRESS)n,sizeof(rnumber));
                    n=INT_TO_SR(ui);
                  }
                }
              }
              break;
      case 3:
              n->s=1;
              if (mpz_isNeg(&n->n)) /* && n->s<2*/
              {
                mpz_neg(&n->n,&n->n);
                mpz_init_set_si(&n->z,(long)-1);
              }
              else
              {
                mpz_init_set_si(&n->z,(long)1);
              }
              break;
    }
  }
#ifdef LDEBUG
  nlTest(n);
#endif
  return n;
}

/*2
* u:= a + b
*/
number nlAdd (number a, number b)
{
  number u;
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    int r=SR_HDL(a)+SR_HDL(b)-1;
    if ( ((r << 1) >> 1) == r )
    {
      return (number)r;
    }
    u=(number)Alloc(sizeof(rnumber));
    u->s=3;
    mpz_init_set_si(&u->z,(long)SR_TO_INT(r));
#ifdef LDEBUG
    u->debug=123456;
    nlTest(u);
#endif
    return u;
  }
  u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  u->debug=123456;
#endif
  mpz_init(&u->z);
  if (SR_HDL(b) & SR_INT)
  {
    number x=a;
    a=b;
    b=x;
  }
  if (SR_HDL(a) & SR_INT)
  {
    switch (b->s)
    {
      case 0:
      case 1:/* a:short, b:1 */
      {
        MP_INT x;
        mpz_init(&x);
        if ((int)a>0)
        {
          mpz_mul_ui(&x,&b->n,SR_TO_INT(a));
        }
        else
        {
          mpz_mul_ui(&x,&b->n,-SR_TO_INT(a));
          mpz_neg(&x,&x);
        }
        mpz_add(&u->z,&b->z,&x);
        nlGmpSimple(&u->z);
        mpz_clear(&x);
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        if (mpz_cmp(&u->z,&b->n)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(1);
        }
        mpz_init_set(&u->n,&b->n);
        u->s = 0;
        break;
      }
      case 3:
      {
        if ((int)a>0)
          mpz_add_ui(&u->z,&b->z,SR_TO_INT(a));
        else
          mpz_sub_ui(&u->z,&b->z,-SR_TO_INT(a));
        nlGmpSimple(&u->z);
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        //u->s = 3;
        if (mpz_size1(&u->z)<=MP_SMALL)
        {
          int ui=(int)mpz_get_si(&u->z);
          if ((((ui<<3)>>3)==ui)
          && (mpz_cmp_si(&u->z,(long)ui)==0))
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(ui);
          }
        }
        u->s = 3;
        break;
      }
    }
  }
  else
  {
    switch (a->s)
    {
      case 0:
      case 1:
      {
        switch(b->s)
        {
          case 0:
          case 1:
          {
            MP_INT x;
            MP_INT y;
            mpz_init(&x);
            mpz_init(&y);
            mpz_mul(&x,&b->z,&a->n);
            mpz_mul(&y,&a->z,&b->n);
            mpz_add(&u->z,&x,&y);
            nlGmpSimple(&u->z);
            mpz_clear(&x);
            mpz_clear(&y);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            mpz_init(&u->n);
            mpz_mul(&u->n,&a->n,&b->n);
            nlGmpSimple(&u->n);
            if (mpz_cmp(&u->z,&u->n)==0)
            {
               mpz_clear(&u->z);
               mpz_clear(&u->n);
               Free((ADDRESS)u,sizeof(rnumber));
               return INT_TO_SR(1);
            }
            u->s = 0;
            break;
          }
          case 3: /* a:1 b:3 */
          {
            MP_INT x;
            mpz_init(&x);
            mpz_mul(&x,&b->z,&a->n);
            mpz_add(&u->z,&a->z,&x);
            nlGmpSimple(&u->z);
            mpz_clear(&x);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            if (mpz_cmp(&u->z,&a->n)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(1);
            }
            mpz_init_set(&u->n,&a->n);
            u->s = 0;
            break;
          }
        } /*switch (b->s) */
        break;
      }
      case 3:
      {
        switch(b->s)
        {
          case 0:
          case 1:/* a:3, b:1 */
          {
            MP_INT x;
            mpz_init(&x);
            mpz_mul(&x,&a->z,&b->n);
            mpz_add(&u->z,&b->z,&x);
            nlGmpSimple(&u->z);
            mpz_clear(&x);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            if (mpz_cmp(&u->z,&b->n)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(1);
            }
            mpz_init_set(&u->n,&b->n);
            u->s = 0;
            break;
          }
          case 3:
          {
            mpz_add(&u->z,&a->z,&b->z);
            nlGmpSimple(&u->z);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            if (mpz_size1(&u->z)<=MP_SMALL)
            {
              int ui=(int)mpz_get_si(&u->z);
              if ((((ui<<3)>>3)==ui)
              && (mpz_cmp_si(&u->z,(long)ui)==0))
              {
                mpz_clear(&u->z);
                Free((ADDRESS)u,sizeof(rnumber));
                return INT_TO_SR(ui);
              }
            }
            u->s = 3;
            break;
          }
        }
        break;
      }
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u:= a - b
*/
number nlSub (number a, number b)
{
  number u;
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    int r=SR_HDL(a)-SR_HDL(b)+1;
    if ( ((r << 1) >> 1) == r )
    {
      return (number)r;
    }
    u=(number)Alloc(sizeof(rnumber));
    u->s=3;
    mpz_init_set_si(&u->z,(long)SR_TO_INT(r));
#ifdef LDEBUG
    u->debug=123456;
    nlTest(u);
#endif
    return u;
  }
  u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  u->debug=123456;
#endif
  mpz_init(&u->z);
  if (SR_HDL(a) & SR_INT)
  {
    switch (b->s)
    {
      case 0:
      case 1:/* a:short, b:1 */
      {
        MP_INT x;
        mpz_init(&x);
        if ((int)a>0)
        {
          mpz_mul_ui(&x,&b->n,SR_TO_INT(a));
        }
        else
        {
          mpz_mul_ui(&x,&b->n,-SR_TO_INT(a));
          mpz_neg(&x,&x);
        }
        mpz_sub(&u->z,&x,&b->z);
        mpz_clear(&x);
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        if (mpz_cmp(&u->z,&b->n)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(1);
        }
        mpz_init_set(&u->n,&b->n);
        u->s = 0;
        break;
      }
      case 3:
      {
        if ((int)a>0)
        {
          mpz_sub_ui(&u->z,&b->z,SR_TO_INT(a));
          mpz_neg(&u->z,&u->z);
        }
        else
        {
          mpz_add_ui(&u->z,&b->z,-SR_TO_INT(a));
          mpz_neg(&u->z,&u->z);
        }
        nlGmpSimple(&u->z);
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        //u->s = 3;
        if (mpz_size1(&u->z)<=MP_SMALL)
        {
          int ui=(int)mpz_get_si(&u->z);
          if ((((ui<<3)>>3)==ui)
          && (mpz_cmp_si(&u->z,(long)ui)==0))
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(ui);
          }
        }
        u->s = 3;
        break;
      }
    }
  }
  else if (SR_HDL(b) & SR_INT)
  {
    switch (a->s)
    {
      case 0:
      case 1:/* b:short, a:1 */
      {
        MP_INT x;
        mpz_init(&x);
        if ((int)b>0)
        {
          mpz_mul_ui(&x,&a->n,SR_TO_INT(b));
        }
        else
        {
          mpz_mul_ui(&x,&a->n,-SR_TO_INT(b));
          mpz_neg(&x,&x);
        }
        mpz_sub(&u->z,&a->z,&x);
        mpz_clear(&x);
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        if (mpz_cmp(&u->z,&a->n)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(1);
        }
        mpz_init_set(&u->n,&a->n);
        u->s = 0;
        break;
      }
      case 3:
      {
        if ((int)b>0)
        {
          mpz_sub_ui(&u->z,&a->z,SR_TO_INT(b));
        }
        else
        {
          mpz_add_ui(&u->z,&a->z,-SR_TO_INT(b));
        }
        if (mpz_cmp_ui(&u->z,(long)0)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(0);
        }
        //u->s = 3;
        nlGmpSimple(&u->z);
        if (mpz_size1(&u->z)<=MP_SMALL)
        {
          int ui=(int)mpz_get_si(&u->z);
          if ((((ui<<3)>>3)==ui)
          && (mpz_cmp_si(&u->z,(long)ui)==0))
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(ui);
          }
        }
        u->s = 3;
        break;
      }
    }
  }
  else
  {
    switch (a->s)
    {
      case 0:
      case 1:
      {
        switch(b->s)
        {
          case 0:
          case 1:
          {
            MP_INT x;
            MP_INT y;
            mpz_init(&x);
            mpz_init(&y);
            mpz_mul(&x,&b->z,&a->n);
            mpz_mul(&y,&a->z,&b->n);
            mpz_sub(&u->z,&y,&x);
            mpz_clear(&x);
            mpz_clear(&y);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            mpz_init(&u->n);
            mpz_mul(&u->n,&a->n,&b->n);
            if (mpz_cmp(&u->z,&u->n)==0)
            {
              mpz_clear(&u->z);
              mpz_clear(&u->n);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(1);
            }
            u->s = 0;
            break;
          }
          case 3: /* a:1, b:3 */
          {
            MP_INT x;
            mpz_init(&x);
            mpz_mul(&x,&b->z,&a->n);
            mpz_sub(&u->z,&a->z,&x);
            mpz_clear(&x);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            if (mpz_cmp(&u->z,&a->n)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(1);
            }
            mpz_init_set(&u->n,&a->n);
            u->s = 0;
            break;
          }
        }
        break;
      }
      case 3:
      {
        switch(b->s)
        {
          case 0:
          case 1: /* a:3, b:1 */
          {
            MP_INT x;
            mpz_init(&x);
            mpz_mul(&x,&a->z,&b->n);
            mpz_sub(&u->z,&x,&b->z);
            mpz_clear(&x);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            if (mpz_cmp(&u->z,&b->n)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(1);
            }
            mpz_init_set(&u->n,&b->n);
            u->s = 0;
            break;
          }
          case 3: /* a:3 , b:3 */
          {
            mpz_sub(&u->z,&a->z,&b->z);
            nlGmpSimple(&u->z);
            if (mpz_cmp_ui(&u->z,(long)0)==0)
            {
              mpz_clear(&u->z);
              Free((ADDRESS)u,sizeof(rnumber));
              return INT_TO_SR(0);
            }
            //u->s = 3;
            if (mpz_size1(&u->z)<=MP_SMALL)
            {
              int ui=(int)mpz_get_si(&u->z);
              if ((((ui<<3)>>3)==ui)
              && (mpz_cmp_si(&u->z,(long)ui)==0))
              {
                mpz_clear(&u->z);
                Free((ADDRESS)u,sizeof(rnumber));
                return INT_TO_SR(ui);
              }
            }
            u->s = 3;
            break;
          }
        }
        break;
      }
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u := a * b
*/
number nlMult (number a, number b)
{
  number u;

#ifdef LDEBUG
  nlTest(a);
  nlTest(b);
#endif
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    if (b==INT_TO_SR(0)) return INT_TO_SR(0);
    int r=(SR_HDL(a)-1)*(SR_HDL(b)>>1);
    if ((r/(SR_HDL(b)>>1))==(SR_HDL(a)-1))
    {
      u=((number) ((r>>1)+SR_INT));
      if (((SR_HDL(u)<<1)>>1)==SR_HDL(u)) return (u);
      return nlRInit(SR_HDL(u)>>2);
    }
    u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
    u->debug=123456;
#endif
    u->s=3;
    if ((int)b>0)
    {
      mpz_init_set_si(&u->z,(long)SR_TO_INT(a));
      mpz_mul_ui(&u->z,&u->z,(unsigned long)SR_TO_INT(b));
    }
//    else if ((int)a>=0)
//    {
//      mpz_init_set_si(&u->z,(long)SR_TO_INT(b));
//      mpz_mul_ui(&u->z,&u->z,(unsigned long)SR_TO_INT(a));
//    }
    else
    {
      mpz_init_set_si(&u->z,(long)(-SR_TO_INT(a)));
      mpz_mul_ui(&u->z,&u->z,(long)(-SR_TO_INT(b)));
    }
#ifdef LDEBUG
    nlTest(u);
#endif
    return u;
  }
  else
  {
    if ((a==INT_TO_SR(0))
    ||(b==INT_TO_SR(0)))
      return INT_TO_SR(0);
    u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
    u->debug=123456;
#endif
    mpz_init(&u->z);
    if (SR_HDL(b) & SR_INT)
    {
      number x=a;
      a=b;
      b=x;
    }
    if (SR_HDL(a) & SR_INT)
    {
      u->s=b->s;
      if (u->s==1) u->s=0;
      if ((int)a>0)
      {
        mpz_mul_ui(&u->z,&b->z,(unsigned long)SR_TO_INT(a));
      }
      else
      {
        if (a==INT_TO_SR(-1))
        {
          mpz_set(&u->z,&b->z);
          mpz_neg(&u->z,&u->z);
          u->s=b->s;
        }
        else
        {
          mpz_mul_ui(&u->z,&b->z,(unsigned long)-SR_TO_INT(a));
          mpz_neg(&u->z,&u->z);
        }
      }
      nlGmpSimple(&u->z);
      if (u->s<2)
      {
        if (mpz_cmp(&u->z,&b->n)==0)
        {
          mpz_clear(&u->z);
          Free((ADDRESS)u,sizeof(rnumber));
          return INT_TO_SR(1);
        }
        mpz_init_set(&u->n,&b->n);
      }
      else //u->s==3
      {
        if (mpz_size1(&u->z)<=MP_SMALL)
        {
          int ui=(int)mpz_get_si(&u->z);
          if ((((ui<<3)>>3)==ui)
          && (mpz_cmp_si(&u->z,(long)ui)==0))
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(ui);
          }
        }
      }
    }
    else
    {
      mpz_mul(&u->z,&a->z,&b->z);
      u->s = 0;
      if(a->s==3)
      {
        if(b->s==3)
        {
          u->s = 3;
        }
        else
        {
          if (mpz_cmp(&u->z,&b->n)==0)
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(1);
          }
          mpz_init_set(&u->n,&b->n);
        }
      }
      else
      {
        if(b->s==3)
        {
          if (mpz_cmp(&u->z,&a->n)==0)
          {
            mpz_clear(&u->z);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(1);
          }
          mpz_init_set(&u->n,&a->n);
        }
        else
        {
          mpz_init(&u->n);
          mpz_mul(&u->n,&a->n,&b->n);
          if (mpz_cmp(&u->z,&u->n)==0)
          {
            mpz_clear(&u->z);
            mpz_clear(&u->n);
            Free((ADDRESS)u,sizeof(rnumber));
            return INT_TO_SR(1);
          }
        }
      }
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u := a / b in Z, if b | a (else undefined)
*/
number   nlExactDiv(number a, number b)
{
  if (b==INT_TO_SR(0))
  {
    WerrorS("div. by 0");
    return INT_TO_SR(0);
  }
  number u;
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    /* the small int -(1<<28) divided by -1 is the large int (1<<28)   */
    if ((a==INT_TO_SR(-(1<<28)))&&(b==INT_TO_SR(-1)))
    {
      return nlRInit(1<<28);
    }
    int aa=SR_TO_INT(a);
    int bb=SR_TO_INT(b);
    return INT_TO_SR(aa/bb);
  }
  number bb=NULL;
  if (SR_HDL(b) & SR_INT)
  {
    bb=nlRInit(SR_TO_INT(b));
    b=bb;
  }
  u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  u->debug=123456;
#endif
  mpz_init(&u->z);
  /* u=a/b */
  u->s = 3;
  MPZ_EXACTDIV(&u->z,&a->z,&b->z);
  if (bb!=NULL)
  {
    mpz_clear(&bb->z);
    Free((ADDRESS)bb,sizeof(rnumber));
  }
  if (mpz_size1(&u->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&u->z);
    if ((((ui<<3)>>3)==ui)
    && (mpz_cmp_si(&u->z,(long)ui)==0))
    {
      mpz_clear(&u->z);
      Free((ADDRESS)u,sizeof(rnumber));
      u=INT_TO_SR(ui);
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u := a / b in Z
*/
number nlIntDiv (number a, number b)
{
  if (b==INT_TO_SR(0))
  {
    WerrorS("div. by 0");
    return INT_TO_SR(0);
  }
  number u;
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    /* the small int -(1<<28) divided by -1 is the large int (1<<28)   */
    if ((a==INT_TO_SR(-(1<<28)))&&(b==INT_TO_SR(-1)))
    {
      return nlRInit(1<<28);
    }
    /* consider the signs of a and b:
    *  + + -> u=a+b-1
    *  + - -> u=a-b-1
    *  - + -> u=a-b+1
    *  - - -> u=a+b+1
    */
    int aa=SR_TO_INT(a);
    int bb=SR_TO_INT(b);
    if (aa>0)
    {
      if (bb>0)
        return INT_TO_SR((aa+bb-1)/bb);
      else
        return INT_TO_SR((aa-bb-1)/bb);
    }
    else
    {
      if (bb>0)
      {
        return INT_TO_SR((aa-bb+1)/bb);
      }
      else
      {
        return INT_TO_SR((aa+bb+1)/bb);
      }
    }
  }
  if (SR_HDL(a) & SR_INT)
  {
    /* the small int -(1<<28) divided by 2^28 is 1   */
    if (a==INT_TO_SR(-(1<<28)))
    {
      if(mpz_cmp_si(&b->z,(long)(1<<28))==0)
      {
        return INT_TO_SR(-1);
      }
    }
    /* a is a small and b is a large int: -> 0 */
    return INT_TO_SR(0);
  }
  number bb=NULL;
  if (SR_HDL(b) & SR_INT)
  {
    bb=nlRInit(SR_TO_INT(b));
    b=bb;
  }
  u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  u->debug=123456;
#endif
  mpz_init_set(&u->z,&a->z);
  /* consider the signs of a and b:
  *  + + -> u=a+b-1
  *  + - -> u=a-b-1
  *  - + -> u=a-b+1
  *  - - -> u=a+b+1
  */
  if (mpz_isNeg(&a->z))
  {
    if (mpz_isNeg(&b->z))
    {
      mpz_add(&u->z,&u->z,&b->z);
    }
    else
    {
      mpz_sub(&u->z,&u->z,&b->z);
    }
    mpz_add_ui(&u->z,&u->z,1);
  }
  else
  {
    if (mpz_isNeg(&b->z))
    {
      mpz_sub(&u->z,&u->z,&b->z);
    }
    else
    {
      mpz_add(&u->z,&u->z,&b->z);
    }
    mpz_sub_ui(&u->z,&u->z,1);
  }
  /* u=u/b */
  u->s = 3;
  MPZ_DIV(&u->z,&u->z,&b->z);
  nlGmpSimple(&u->z);
  if (bb!=NULL)
  {
    mpz_clear(&bb->z);
    Free((ADDRESS)bb,sizeof(rnumber));
  }
  if (mpz_size1(&u->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&u->z);
    if ((((ui<<3)>>3)==ui)
    && (mpz_cmp_si(&u->z,(long)ui)==0))
    {
      mpz_clear(&u->z);
      Free((ADDRESS)u,sizeof(rnumber));
      u=INT_TO_SR(ui);
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u := a mod b in Z, u>=0
*/
number nlIntMod (number a, number b)
{
  if (b==INT_TO_SR(0))
  {
    WerrorS("div. by 0");
    return INT_TO_SR(0);
  }
  if (a==INT_TO_SR(0))
      return INT_TO_SR(0);
  number u;
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    if ((int)a>0)
    {
      if ((int)b>0)
        return INT_TO_SR(SR_TO_INT(a)%SR_TO_INT(b));
      else
        return INT_TO_SR(SR_TO_INT(a)%(-SR_TO_INT(b)));
    }
    else
    {
      if ((int)b>0)
      {
        int i=(-SR_TO_INT(a))%SR_TO_INT(b);
        if ( i != 0 ) i = (SR_TO_INT(b))-i;
        return INT_TO_SR(i);
      }
      else
      {
        int i=(-SR_TO_INT(a))%(-SR_TO_INT(b));
        if ( i != 0 ) i = (-SR_TO_INT(b))-i;
        return INT_TO_SR(i);
      }
    }
  }
  if (SR_HDL(a) & SR_INT)
  {
    /* a is a small and b is a large int: -> a or (a+b) or (a-b) */
    if ((int)a<0)
    {
      if (mpz_isNeg(&b->z))
        return nlSub(a,b);
      /*else*/
        return nlAdd(a,b);
    }
    /*else*/
      return a;
  }
  number bb=NULL;
  if (SR_HDL(b) & SR_INT)
  {
    bb=nlRInit(SR_TO_INT(b));
    b=bb;
  }
  u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  u->debug=123456;
#endif
  mpz_init(&u->z);
  u->s = 3;
  mpz_mod(&u->z,&a->z,&b->z);
  if (bb!=NULL)
  {
    mpz_clear(&bb->z);
    Free((ADDRESS)bb,sizeof(rnumber));
  }
  if (mpz_isNeg(&u->z))
  {
    if (mpz_isNeg(&b->z))
      mpz_sub(&u->z,&u->z,&b->z);
    else
      mpz_add(&u->z,&u->z,&b->z);
  }
  nlGmpSimple(&u->z);
  if (mpz_size1(&u->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&u->z);
    if ((((ui<<3)>>3)==ui)
    && (mpz_cmp_si(&u->z,(long)ui)==0))
    {
      mpz_clear(&u->z);
      Free((ADDRESS)u,sizeof(rnumber));
      u=INT_TO_SR(ui);
    }
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

/*2
* u := a / b
*/
number nlDiv (number a, number b)
{
  number u;
  if (nlIsZero(b))
  {
    WerrorS("div. by 0");
    return INT_TO_SR(0);
  }
  u=(number)Alloc(sizeof(rnumber));
  u->s=0;
#ifdef LDEBUG
  u->debug=123456;
#endif
// ---------- short / short ------------------------------------
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    int i=SR_TO_INT(a);
    int j=SR_TO_INT(b);
    int r=i%j;
    if (r==0)
    {
      Free((ADDRESS)u,sizeof(rnumber));
      return INT_TO_SR(i/j);
    }
    mpz_init_set_si(&u->z,(long)i);
    mpz_init_set_si(&u->n,(long)j);
  }
  else
  {
    mpz_init(&u->z);
// ---------- short / long ------------------------------------
    if (SR_HDL(a) & SR_INT)
    {
      // short a / (z/n) -> (a*n)/z
      if (b->s<2)
      {
        if ((int)a>=0)
          mpz_mul_ui(&u->z,&b->n,SR_TO_INT(a));
        else
        {
          mpz_mul_ui(&u->z,&b->n,-SR_TO_INT(a));
          mpz_neg(&u->z,&u->z);
        }
      }
      else
      // short a / long z -> a/z
      {
        mpz_set_si(&u->z,SR_TO_INT(a));
      }
      if (mpz_cmp(&u->z,&b->z)==0)
      {
        mpz_clear(&u->z);
        Free((ADDRESS)u,sizeof(rnumber));
        return INT_TO_SR(1);
      }
      mpz_init_set(&u->n,&b->z);
    }
// ---------- long / short ------------------------------------
    else if (SR_HDL(b) & SR_INT)
    {
      mpz_set(&u->z,&a->z);
      // (z/n) / b -> z/(n*b)
      if (a->s<2)
      {
        mpz_init_set(&u->n,&a->n);
        if ((int)b>0)
          mpz_mul_ui(&u->n,&u->n,SR_TO_INT(b));
        else
        {
          mpz_mul_ui(&u->n,&u->n,-SR_TO_INT(b));
          mpz_neg(&u->z,&u->z);
        }
      }
      else
      // long z / short b -> z/b
      {
        //mpz_set(&u->z,&a->z);
        mpz_init_set_si(&u->n,SR_TO_INT(b));
      }
    }
// ---------- long / long ------------------------------------
    else
    {
      //u->s=0;
      mpz_set(&u->z,&a->z);
      mpz_init_set(&u->n,&b->z);
      if (a->s<2) mpz_mul(&u->n,&u->n,&a->n);
      if (b->s<2) mpz_mul(&u->z,&u->z,&b->n);
    }
  }
  if (mpz_isNeg(&u->n))
  {
    mpz_neg(&u->z,&u->z);
    mpz_neg(&u->n,&u->n);
  }
  if (mpz_cmp_si(&u->n,(long)1)==0)
  {
    mpz_clear(&u->n);
    if (mpz_size1(&u->z)<=MP_SMALL)
    {
      int ui=(int)mpz_get_si(&u->z);
      if ((((ui<<3)>>3)==ui)
      && (mpz_cmp_si(&u->z,(long)ui)==0))
      {
        mpz_clear(&u->z);
        Free((ADDRESS)u,sizeof(rnumber));
        return INT_TO_SR(ui);
      }
    }
    u->s=3;
  }
#ifdef LDEBUG
  nlTest(u);
#endif
  return u;
}

#if (defined(i386)) && (defined(HAVE_LIBGMP1))
/*
* compute x^exp for x in Z
* there seems to be an bug in mpz_pow_ui for i386
*/
static inline void nlPow (MP_INT * res,MP_INT * x,int exp)
{
  if (exp==0)
  {
    mpz_set_si(res,(long)1);
  }
  else
  {
    MP_INT xh;
    mpz_init(&xh);
    mpz_set(res,x);
    exp--;
    while (exp!=0)
    {
      mpz_mul(&xh,res,x);
      mpz_set(res,&xh);
      exp--;
    }
    mpz_clear(&xh);
  }
}
#endif

/*2
* u:= x ^ exp
*/
void nlPower (number x,int exp,number * u)
{
  if (!nlIsZero(x))
  {
#ifdef LDEBUG
    nlTest(x);
#endif
    number aa=NULL;
    if (SR_HDL(x) & SR_INT)
    {
      aa=nlRInit(SR_TO_INT(x));
      x=aa;
    }
    *u=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
    (*u)->debug=123456;
#endif
    mpz_init(&(*u)->z);
    (*u)->s = x->s;
#if (!defined(i386)) || (defined(HAVE_LIBGMP2))
    mpz_pow_ui(&(*u)->z,&x->z,(unsigned long)exp);
#else
    nlPow(&(*u)->z,&x->z,exp);
#endif
    if (x->s<2)
    {
      mpz_init(&(*u)->n);
#if (!defined(i386)) || (defined(HAVE_LIBGMP2))
      mpz_pow_ui(&(*u)->n,&x->n,(unsigned long)exp);
#else
      nlPow(&(*u)->n,&x->n,exp);
#endif
    }
    if (aa!=NULL)
    {
      mpz_clear(&aa->z);
      Free((ADDRESS)aa,sizeof(rnumber));
    }
    if (((*u)->s<=2) && (mpz_cmp_si(&(*u)->n,(long)1)==0))
    {
      mpz_clear(&(*u)->n);
      (*u)->s=3;
    }
    if (((*u)->s==3) && (mpz_size1(&(*u)->z)<=MP_SMALL))
    {
      int ui=(int)mpz_get_si(&(*u)->z);
      if ((((ui<<3)>>3)==ui)
      && (mpz_cmp_si(&(*u)->z,(long)ui)==0))
      {
        mpz_clear(&(*u)->z);
        Free((ADDRESS)*u,sizeof(rnumber));
        *u=INT_TO_SR(ui);
      }
    }
  }
  else
    *u = INT_TO_SR(0);
#ifdef LDEBUG
  nlTest(*u);
#endif
}

BOOLEAN nlIsZero (number a)
{
#ifdef LDEBUG
  nlTest(a);
#endif
  //if (a==INT_TO_SR(0)) return TRUE;
  //if (SR_HDL(a) & SR_INT) return FALSE;
  //number aa=nlCopy(a);
  //nlNormalize(aa);
  return (a==INT_TO_SR(0));
}

/*2
* za >= 0 ?
*/
BOOLEAN nlGreaterZero (number a)
{
#ifdef LDEBUG
  nlTest(a);
#endif
  if (SR_HDL(a) & SR_INT) return SR_HDL(a)>=0;
  return (!mpz_isNeg(&a->z));
}

/*2
* a > b ?
*/
BOOLEAN nlGreater (number a, number b)
{
#ifdef LDEBUG
  nlTest(a);
  nlTest(b);
#endif
  number r;
  BOOLEAN rr;
  r=nlSub(a,b);
  rr=(!nlIsZero(r)) && (nlGreaterZero(r));
  nlDelete(&r);
  return rr;
}

/*2
* a = b ?
*/
BOOLEAN nlEqual (number a, number b)
{
#ifdef LDEBUG
  nlTest(a);
  nlTest(b);
#endif
// short - short
  if (SR_HDL(a) & SR_HDL(b) & SR_INT) return a==b;
//  long - short
  BOOLEAN bo;
  if (SR_HDL(b) & SR_INT)
  {
    if (a->s!=0) return FALSE;
    number n=b; b=a; a=n;
  }
//  short - long
  if (SR_HDL(a) & SR_INT)
  {
    if (b->s!=0)
      return FALSE;
    if (((int)a > 0) && (mpz_isNeg(&b->z)))
      return FALSE;
    if (((int)a < 0) && (!mpz_isNeg(&b->z)))
      return FALSE;
    MP_INT  bb;
    mpz_init_set(&bb,&b->n);
    if ((int)a<0)
    {
      mpz_neg(&bb,&bb);
      mpz_mul_ui(&bb,&bb,(long)-SR_TO_INT(a));
    }
    else
    {
      mpz_mul_ui(&bb,&bb,(long)SR_TO_INT(a));
    }
    bo=(mpz_cmp(&bb,&b->z)==0);
    mpz_clear(&bb);
    return bo;
  }
// long - long
  if (((a->s==1) && (b->s==3))
  ||  ((b->s==1) && (a->s==3)))
    return FALSE;
  if (mpz_isNeg(&a->z)&&(!mpz_isNeg(&b->z)))
    return FALSE;
  if (mpz_isNeg(&b->z)&&(!mpz_isNeg(&a->z)))
    return FALSE;
  nlGmpSimple(&a->z);
  nlGmpSimple(&b->z);
  if (a->s<2)
    nlGmpSimple(&a->n);
  if (b->s<2)
    nlGmpSimple(&b->n);
  MP_INT  aa;
  MP_INT  bb;
  mpz_init_set(&aa,&a->z);
  mpz_init_set(&bb,&b->z);
  if (a->s<2) mpz_mul(&bb,&bb,&a->n);
  if (b->s<2) mpz_mul(&aa,&aa,&b->n);
  bo=(mpz_cmp(&aa,&bb)==0);
  mpz_clear(&aa);
  mpz_clear(&bb);
  return bo;
}

/*2
* a == 1 ?
*/
BOOLEAN nlIsOne (number a)
{
#ifdef LDEBUG
  if (a==NULL) return FALSE;
  nlTest(a);
#endif
  if (SR_HDL(a) & SR_INT) return (a==INT_TO_SR(1));
  return FALSE;
}

/*2
* a == -1 ?
*/
BOOLEAN nlIsMOne (number a)
{
#ifdef LDEBUG
  if (a==NULL) return FALSE;
  nlTest(a);
#endif
  if (SR_HDL(a) & SR_INT) return (a==INT_TO_SR(-1));
  return FALSE;
}

/*2
* result =gcd(a,b)
*/
number nlGcd(number a, number b)
{
  number result;
#ifdef LDEBUG
  nlTest(a);
  nlTest(b);
#endif
  //nlNormalize(a);
  //nlNormalize(b);
  if ((SR_HDL(a)==5)||(a==INT_TO_SR(-1))
  ||  (SR_HDL(b)==5)||(b==INT_TO_SR(-1))) return INT_TO_SR(1);
  if (SR_HDL(a) & SR_HDL(b) & SR_INT)
  {
    int i=SR_TO_INT(a);
    int j=SR_TO_INT(b);
    if((i==0)||(j==0))
      return INT_TO_SR(1);
    int l;
    i=ABS(i);
    j=ABS(j);
    do
    {
      l=i%j;
      i=j;
      j=l;
    } while (l!=0);
    return INT_TO_SR(i);
  }
  if (((!(SR_HDL(a) & SR_INT))&&(a->s<2))
  ||  ((!(SR_HDL(b) & SR_INT))&&(b->s<2))) return INT_TO_SR(1);
  number aa=NULL;
  if (SR_HDL(a) & SR_INT)
  {
    aa=nlRInit(SR_TO_INT(a));
    a=aa;
  }
  else
  if (SR_HDL(b) & SR_INT)
  {
    aa=nlRInit(SR_TO_INT(b));
    b=aa;
  }
  result=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  result->debug=123456;
#endif
  mpz_init(&result->z);
  mpz_gcd(&result->z,&a->z,&b->z);
  nlGmpSimple(&result->z);
  result->s = 3;
  if (mpz_size1(&result->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&result->z);
    if ((((ui<<3)>>3)==ui)
    && (mpz_cmp_si(&result->z,(long)ui)==0))
    {
      mpz_clear(&result->z);
      Free((ADDRESS)result,sizeof(rnumber));
      result=INT_TO_SR(ui);
    }
  }
  if (aa!=NULL)
  {
    mpz_clear(&aa->z);
    Free((ADDRESS)aa,sizeof(rnumber));
  }
#ifdef LDEBUG
  nlTest(result);
#endif
  return result;
}

/*2
* simplify x
*/
void nlNormalize (number &x)
{
  if ((SR_HDL(x) & SR_INT) ||(x==NULL))
    return;
#ifdef LDEBUG
  nlTest(x);
#endif
  if (x->s==3)
  {
    if (mpz_cmp_ui(&x->z,(long)0)==0)
    {
      nlDelete(&x);
      x=nlInit(0);
      return;
    }
    if (mpz_size1(&x->z)<=MP_SMALL)
    {
      int ui=(int)mpz_get_si(&x->z);
      if ((((ui<<3)>>3)==ui)
      && (mpz_cmp_si(&x->z,(long)ui)==0))
      {
        mpz_clear(&x->z);
        Free((ADDRESS)x,sizeof(rnumber));
        x=INT_TO_SR(ui);
        return;
      }
    }
  }
  if (x->s!=1) nlGmpSimple(&x->z);
  if (x->s==0)
  {
    BOOLEAN divided=FALSE;
    MP_INT gcd;
    mpz_init(&gcd);
    mpz_gcd(&gcd,&x->z,&x->n);
    x->s=1;
    if (mpz_cmp_si(&gcd,(long)1)!=0)
    {
      MP_INT r;
      mpz_init(&r);
      MPZ_EXACTDIV(&r,&x->z,&gcd);
      mpz_set(&x->z,&r);
      MPZ_EXACTDIV(&r,&x->n,&gcd);
      mpz_set(&x->n,&r);
      mpz_clear(&r);
      nlGmpSimple(&x->z);
      divided=TRUE;
    }
    mpz_clear(&gcd);
    nlGmpSimple(&x->n);
    if (mpz_cmp_si(&x->n,(long)1)==0)
    {
      mpz_clear(&x->n);
      if (mpz_size1(&x->z)<=MP_SMALL)
      {
        int ui=(int)mpz_get_si(&x->z);
        if ((((ui<<3)>>3)==ui)
        && (mpz_cmp_si(&x->z,(long)ui)==0))
        {
          mpz_clear(&x->z);
#ifdef LDEBUG
          x->debug=654324;
#endif
          Free((ADDRESS)x,sizeof(rnumber));
          x=INT_TO_SR(ui);
          return;
        }
      }
      x->s=3;
    }
    else if (divided)
    {
      _mpz_realloc(&x->n,mpz_size1(&x->n));
    }
    if (divided) _mpz_realloc(&x->z,mpz_size1(&x->z));
  }
#ifdef LDEBUG
  nlTest(x);
#endif
}

/*2
* returns in result->z the lcm(a->z,b->n)
*/
number nlLcm(number a, number b)
{
  number result;
#ifdef LDEBUG
  nlTest(a);
  nlTest(b);
#endif
  if ((SR_HDL(b) & SR_INT)
  || (b->s==3))
  {
    // b is 1/(b->n) => b->n is 1 => result is a
    return nlCopy(a);
  }
  number aa=NULL;
  if (SR_HDL(a) & SR_INT)
  {
    aa=nlRInit(SR_TO_INT(a));
    a=aa;
  }
  result=(number)Alloc(sizeof(rnumber));
#ifdef LDEBUG
  result->debug=123456;
#endif
  result->s=3;
  MP_INT gcd;
  mpz_init(&gcd);
  mpz_init(&result->z);
  mpz_gcd(&gcd,&a->z,&b->n);
  if (mpz_cmp_si(&gcd,(long)1)!=0)
  {
    MP_INT bt;
    mpz_init_set(&bt,&b->n);
    MPZ_EXACTDIV(&bt,&bt,&gcd);
    mpz_mul(&result->z,&a->z,&bt);
    mpz_clear(&bt);
  }
  else
    mpz_mul(&result->z,&a->z,&b->n);
  mpz_clear(&gcd);
  if (aa!=NULL)
  {
    mpz_clear(&aa->z);
    Free((ADDRESS)aa,sizeof(rnumber));
  }
  nlGmpSimple(&result->z);
  if (mpz_size1(&result->z)<=MP_SMALL)
  {
    int ui=(int)mpz_get_si(&result->z);
    if ((((ui<<3)>>3)==ui)
    && (mpz_cmp_si(&result->z,(long)ui)==0))
    {
      mpz_clear(&result->z);
      Free((ADDRESS)result,sizeof(rnumber));
      return INT_TO_SR(ui);
    }
  }
#ifdef LDEBUG
  nlTest(result);
#endif
  return result;
}

int nlModP(number n, int p)
{
  if (SR_HDL(n) & SR_INT)
  {
    int i=SR_TO_INT(n);
    if (i<0) return (p-((-i)%p));
    return i%p;
  }
  int iz=mpz_mmod_ui(NULL,&n->z,(unsigned long)p);
  if (n->s!=3)
  {
    int in=mpz_mmod_ui(NULL,&n->n,(unsigned long)p);
    return (int)npDiv((number)iz,(number)in);
  }
  return iz;
}
