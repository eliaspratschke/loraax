#include <vector>
#include <string>
#include "util.h"
#include "algorithms.h"
#include "vertex.h"
#include "airfoil.h"
#include "section.h"

/******************************************************************************/
//
// Section class. Defines a wing section.
// 
/******************************************************************************/

/******************************************************************************/
//
// Default constructor
//
/******************************************************************************/
Section::Section ()
{
  _y = 0.;
  _xle = 0.;
  _zle = 0.;
  _chord = 0.;
  _twist = 0.;
  _roll = 0.;
  _nverts = 0;
  _verts.resize(0); 
}

/******************************************************************************/
//
// Set or access geometry
//
/******************************************************************************/
void Section::setGeometry ( const double & y, const double & xle,
                            const double & zle, const double & chord,
                            const double & twist, const double & roll )
{
  _y = y;
  _xle = xle;
  _zle = zle;
  _chord = chord;
  _twist = twist;
  _roll = roll;
}

const double & Section::y () const { return _y; }
const double & Section::xle () const { return _xle; }
const double & Section::zle () const { return _zle; }
const double & Section::chord () const { return _chord; }
const double & Section::twist () const { return _twist; }
const double & Section::roll () const { return _roll; }

/******************************************************************************/
//
// Access vertex
//
/******************************************************************************/
Vertex & Section::vert ( unsigned int idx )
{
  if ( idx >= _nverts )
  {
    conditional_stop(1, "Section::vert", "Index out of range.");
  }

  return _verts[idx];
} 

/******************************************************************************/
//
// Set vertices from spacing distribution
//   nchord: number of points along chord
//   lesprat: spacing at leading edge as fraction of uniform spacing
//   tesprat: spacing at leading edge as fraction of uniform spacing
//
/******************************************************************************/
void Section::setVertices ( unsigned int nchord, const double & lesprat,
                            const double & tesprat )
{
  double slen, sle, unisp, lesp, tesp;
  double a4top, a5top, a4bot, a5bot;
  std::vector<double> sv;
  std::vector<double> xf, zf;			// Vertices in foil coordinates
  unsigned int i, nverts;

#ifdef DEBUG
  if (_foil.nBuffer() == 0)
  {
    conditional_stop(1, "Section::setVertices", "Airfoil not loaded yet.");
  }

  if (! _foil.splined())
  {
    conditional_stop(1, "Section::setVertices", "Must call splineFit first.");
  }
#endif

  // Get spacings 

  slen = _foil.sLen();
  sle = _foil.sLE();
  unisp = slen / float(2*nchord-2);	// Uniform spacing
  lesp = unisp * lesprat;		// LE spacing
  tesp = unisp * tesprat;		// TE spacing

  // Optimize tanh spacing coefficients to minimize stretching

  opt_tanh_spacing(nchord, sle, tesp, lesp, a4top, a5top);
  opt_tanh_spacing(nchord, slen-sle, lesp, tesp, a4bot, a5bot);

  // Set spacing vector

  nverts = 2*nchord - 1;
  sv.resize(nverts);
  sv[0] = 0.;
  for ( i = 1; i < nchord; i++ )
  {
    sv[i] = sv[i-1] +
      tanh_spacing(i-1, a4top, a5top, nchord, sle, tesp, lesp);
  }
  for ( i = 1; i < nchord; i++ )
  {
    sv[i+nchord-1] = sv[i+nchord-2] +
      tanh_spacing(i-1, a4bot, a5bot, nchord, slen-sle, lesp, tesp);
  }

  // Get vertices in foil coordinate system (unit chord, 0 <= x <= 1)

  xf.resize(nverts);
  zf.resize(nverts);
  for ( i = 0; i < nverts; i++ )
  {
    _foil.splineInterp(sv[i], xf[i], zf[i]);
  }

  // Transform to section coordinates

} 

/******************************************************************************/
//
// Access airfoil
//
/******************************************************************************/
Airfoil & Section::airfoil () { return _foil; }