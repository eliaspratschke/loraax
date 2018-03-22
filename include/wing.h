// Header for wing class

#ifndef WING_H
#define WING_H

#include <vector>
#include <string>
#include "section.h"
#include "airfoil.h"

/******************************************************************************/
//
// Wing class. Contains sections, airfoils, paneling info, faces, a wake, etc.
//
/******************************************************************************/
class Wing {

  private:

    std::string _name;
    unsigned int _nchord, _nspan;	// Number of points along chord and
					//   span. nchord is on each side (top
					//   and bottom); total is 2*nchord-1.
    double _lesprat, _tesprat;		// LE and TE spacing ratios relative to
					//   uniform
    double _rootsprat, _tipsprat;	// Root and tip spacing ratios relative
					//   to uniform

    std::vector<Section> _sections;	// Spanwise sections used for
					//   calculations, set by _nspan and
					//   root & tip spacing rations
    std::vector<Airfoil> _foils;  	// User-specified airfoils

  public:

    // Constructor

    Wing ();

    // Set/get name

    void setName ( const std::string & name );
    const std::string & name () const;

    // Set discretization and spacing info

    void setDiscretization ( unsigned int nchord, unsigned int nspan,
                             const double & lesprat, const double & tesprat,
                             const double & rootsprat,
                             const double & tipsprat );

    // Set airfoils. Section airfoils are interpolated from these airfoils.
    // Airfoils do not need to be given in sorted order.

    void setAirfoils ( const std::vector<Airfoil> & foils );

    // Set sections based on user section inputs and spacing. User sections
    // define the planform shape, and the _sections variables defines the final
    // discretization. Sections do not need to be given in sorted order.

    int setupSections ( const std::vector<Section> & user_sections );
};

#endif