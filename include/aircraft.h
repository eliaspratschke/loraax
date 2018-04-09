// Header for aircraft class

#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <vector>
#include <string>
#include <Eigen/Dense>
#include "wing.h"

class Vertex;
class Panel;
class Vortex;

/******************************************************************************/
//
// Aircraft class. Contains some number of wings and related data and members.
//
/******************************************************************************/
class Aircraft {

  private:

    std::vector<Wing> _wings;		// Wings

    double _sref;			// Reference area
    double _lref;			// Pitching moment reference length
    Eigen::Vector3d _momcen;		// Pitching moment reference point

    std::vector<Vertex *> _verts;	// Pointers to vertices
    std::vector<Panel *> _panels;	// Pointers to panels 
    std::vector<Vertex *> _wakeverts;   // Pointers to wake vertices
    std::vector<Vortex *> _vorts;	// Pointers to wake vortex panels and
                                        //   horseshoes

    Eigen::MatrixXd _aic;		// Aero influence coefficients matrix
    Eigen::VectorXd _mu;		// Unknown doublet strengths vector
    Eigen::VectorXd _rhs;		// Right hand side vector
    Eigen::PartialPivLU<Eigen::MatrixXd> _lu;
					// LU factorization of AIC matrix

    // Set up pointers to vertices, panels, and wake elements

    void setGeometryPointers ();

    // Write VTK viz

    int writeSurfaceViz ( const std::string & fname ) const;
    int writeWakeViz ( const std::string & fname ) const;
    int writeWakeStripViz ( const std::string & prefix );

  public:

    // Constructor

    Aircraft ();

    // Read from XML

    int readXML ( const std::string & geom_file );

    // Set source, doublet, and vortex strengths

    void setSourceStrengths ();
    void setDoubletStrengths ();
    void setWakeCirculation ();

    // Construct AIC matrix and RHS vector, factorize, and solve

    void constructSystem ();
    void factorize ();
    void solveSystem ();

    // Gives size of system of equations (= number of panels)

    unsigned int systemSize () const;

    // Write VTK viz

    int writeViz ( const std::string & prefix ) const;
};

#endif
