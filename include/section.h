// Header for section class

#ifndef SECTION_H
#define SECTION_H

#include <vector>
#include <string>
#include "sectional_object.h"
#include "vertex.h"
#include "airfoil.h"

/** Struct used for interpolation. Gives bounds and weights. **/
struct interpdata
{
	int point1;
	int point2;
	double weight1;
	double weight2;
};

/******************************************************************************/
//
// Section class. Defines a wing section.
//
/******************************************************************************/
class Section: public SectionalObject {

	private:

	double _xle, _zle;	// Leading edge location
	double _chord;
	double _twist;		// Twist angle, positive leading edge up
	double _roll;		// Roll angle, positive CCW viewed from back
	
	unsigned int _nverts;
	unsigned int _nwake;
	std::vector<Vertex>	_verts, _uverts;
						// Vertices defining panel endpoints, and non-rotated,
						// non-translated version of same
	std::vector<Vertex> _wverts;
						// Xfoil wake vertices transformed to inertial frame
	
	Airfoil _foil;		// Airfoil at this section
	double _re;			// Reynolds number
	double _fa, _fn;	// Axial and normal force/span in section frame
	double _clp, _clv, _cdp, _cdv, _cmp, _cmv;
						// Sectional lift, drag, and moment coefficients
	double _cl2dprev, _cl2dguess, _cl2dguessprev;
						// Stores previous values of Cl for Xfoil
	bool _converged;	// Whether Xfoil BL calculations converged
	unsigned int _unconverged_count;
						// Number of unconverged attempts
	bool _reinitialized;// Whether BL was reinitialized following last attempt
	std::vector<interpdata> _foilinterp;
						// Airfoil interpolation points & weights for verts
	
	// Interpolates BL data from airfoil points to section vertices and sets
	// vertex data
	
	void setVertexBLData ( const std::vector<double> & bldata,
	                       unsigned int dataidx, double scale=1.0 );

	public:

	// Constructor and destructor
	
	Section (); 
	virtual ~Section ();
	
	// Set or access position, orientation, and scale
	
	void setGeometry ( const double & xle, const double & y, const double & zle,
	                   const double & chord, const double & twist,
	                   const double & roll );
	void setRoll ( const double & roll );
	const double & xle () const;
	const double & zle () const;
	const double & chord () const;
	const double & twist () const;
	const double & roll () const;
	
	// Access vertices
	
	unsigned int nVerts () const;
	Vertex & vert ( unsigned int idx );
	
	// Set vertices from spacing distribution
	
	void setVertices ( unsigned int nchord, const double & lesprat,
	                   const double & tesprat );

	// Prandtl-Glauert geometric transformation (xinc = x/beta)

	void transformPrandtlGlauert ( const double & minf );
	
	// Access airfoil
	
	Airfoil & airfoil ();
	
	// Computes Reynolds number and sets it for airfoil
	
	void computeReynoldsNumber ( const double & rhoinf, const double & uinf,
	                             const double & muinf );
	const double & reynoldsNumber () const; 
	
	// Sets Mach number for airfoil
	
	void setMachNumber ( const double & mach );
	
	// BL calculations with Xfoil
	
	void computeBL ( const Eigen::Vector3d & uinfvec, const double & rhoinf,
		             const double & pinf, const double & alpha,
		             int reinit_freq );
	bool blConverged () const;
	bool blReinitialized () const;
	
	// Computes section forces and moments
	
	void computeForceMoment ( const double & alpha, const double & uinf,
	                          const double & rhoinf, const double & pinf,
	                          bool viscous );
	
	// Sectional force and moment coefficients
	
	double liftCoefficient () const;
	const double & pressureLiftCoefficient () const;
	const double & viscousLiftCoefficient () const;
	
	double dragCoefficient () const;
	const double & pressureDragCoefficient () const;
	const double & viscousDragCoefficient () const;
	
	double pitchingMomentCoefficient () const;
	const double & pressurePitchingMomentCoefficient () const;
	const double & viscousPitchingMomentCoefficient () const;

	// Viscous wake vertices, scaled and transformed to inertial frame

	unsigned int nWake () const;
	Vertex & wakeVert ( unsigned int idx );

	// Gets BL data from two other sections

	void interpolateBL ( Section & sec1, Section & sec2,
	                     const double & weight1, const double & weight2 );
};

#endif
