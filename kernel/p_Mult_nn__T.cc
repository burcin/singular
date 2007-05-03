/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/***************************************************************
 *  File:    p_Mult_n__Template.cc
 *  Purpose: template for p_Mult_n
 *  Author:  obachman (Olaf Bachmann)
 *  Created: 8/00
 *  Version: $Id: p_Mult_nn__T.cc,v 1.8 2007-05-03 14:04:34 Singular Exp $
 *******************************************************************/

/***************************************************************
 *
 *   Returns:  p*n
 *   Destroys: p
 *   Const:    n
 *
 ***************************************************************/
LINKAGE poly p_Mult_nn(poly p, const number n, const ring r)
{
  pAssume(!n_IsZero(n,r));
  p_Test(p, r);

  poly q = p;
#if defined(HAVE_RING2TOM) || defined(HAVE_RINGMODN)
  poly old = NULL;
#endif
  while (p != NULL)
  {
#if !defined(HAVE_RING2TOM) && !defined(HAVE_RINGMODN)
    number nc = pGetCoeff(p);
    pSetCoeff0(p, n_Mult(n, nc, r));
    n_Delete(&nc, r);
    pIter(p);
#else
    number tmp = n_Mult(n, pGetCoeff(p), r);
    if (r->cring==0 || (r->cring > 0 && tmp != NULL))
    {
       p_SetCoeff(p, tmp, r);
       old = p;
       pIter(p);
    }
    else
    {
      n_Delete(&tmp, r);
      if (old == NULL) { q = pNext(q); }
      else { pNext(old) = pNext(p); }
      pIter(p);    // TODO Free Monom OLIVER
    }
#endif
  }
  p_Test(q, r);
  return q;
}


