#define _USE_MATH_DEFINES

#include <vector>
#include <cmath>
#include <Eigen/Core>
#include "util.h"
#include "settings.h"
#include "vertex.h"
#include "panel.h"
#include "tripanel.h"
#include "quadpanel.h"
#include "singularities.h"
#include "transformations.h"
#include "geometry.h"
#include "wake.h"

/******************************************************************************/
//
// Wake class.
//
/******************************************************************************/

/******************************************************************************/
//
// Default constructor
//
/******************************************************************************/
Wake::Wake ()
{
    _idx = -1;
    _nspan = 0;
    _nstream = 0;
    _rcore = 1.0E-08;
    _verts.resize(0);
    _newx.resize(0);
    _newy.resize(0);
    _newz.resize(0);
    _newxinc.resize(0);
    _newyinc.resize(0);
    _newzinc.resize(0);
    _topteverts.resize(0);
    _botteverts.resize(0);
    _tris.resize(0); 
    _quads.resize(0); 
}

/******************************************************************************/
//
// Initialize with a vector of vertices along the wing trailing edge
//
/******************************************************************************/
void Wake::initialize ( const std::vector<Vertex *> & topteverts,
                        const std::vector<Vertex *> & botteverts,
                        const double & maxspan, int & next_global_vertidx,
                        int & next_global_elemidx, int wakeidx )
{
    int ntris, nquads;
    unsigned int i, j, blvert, brvert, trvert, tlvert;
    double mindist, dist;
    double x, y, z, xinc, yinc, zinc, xviz, yviz, zviz, dx, dy, dz;

    _idx = wakeidx;
    
    _topteverts = topteverts;
    _botteverts = botteverts;

    _infdist = 750.*maxspan;

    // Core radius 1/100 width of narrowest TE panel

    mindist = 1.E+08;
    for ( i = 0; int(i) < _nspan-1; i++ )
    {
        dist = _topteverts[i]->distance(*_topteverts[i+1]);
        if (dist < mindist)
            _rcore = 0.01*dist;
    }
    
    // Determine number of rows to store based on inputs
    
    _nspan = _topteverts.size();
    _nstream = wakeiters+1;     // Number of relaxable streamwise 
                                // points. One more will be added for
                                // the trailing "infinite" panel.
    _verts.resize(_nspan*(_nstream+1));
    _newx.resize(_nspan*(_nstream-1));
    _newy.resize(_nspan*(_nstream-1));
    _newz.resize(_nspan*(_nstream-1));
    _newxinc.resize(_nspan*(_nstream-1));
    _newyinc.resize(_nspan*(_nstream-1));
    _newzinc.resize(_nspan*(_nstream-1));

    // Add vertices along freestream direction
    
    for ( i = 0; int(i) < _nspan; i++ )
    {
        for ( j = 0; int(j) < _nstream+1; j++ )
        {
            if (j == 0)
            {
                x = _topteverts[i]->x();
                y = _topteverts[i]->y();
                z = _topteverts[i]->z();
                _verts[i*(_nstream+1)].setCoordinates(x, y, z);

                xinc = _topteverts[i]->xInc();
                yinc = _topteverts[i]->yInc();
                zinc = _topteverts[i]->zInc();
                _verts[i*(_nstream+1)].setIncompressibleCoordinates(xinc,
                                                                    yinc, zinc);
            }
            else if (int(j) < _nstream)
            {
                // Vertices that will roll up. Note freestream velocity is the
                // same in actual and incompressible coordinates (see BAH book).

                dx = std::cos(wakeangle*M_PI/180.)*uinf*double(j)*dt; 
                dy = 0.;
                dz = std::sin(wakeangle*M_PI/180.)*uinf*double(j)*dt;

                xinc = _verts[i*(_nstream+1)].xInc() + dx;
                yinc = _verts[i*(_nstream+1)].yInc() + dy;
                zinc = _verts[i*(_nstream+1)].zInc() + dz;
                _verts[i*(_nstream+1)+j].setIncompressibleCoordinates(xinc,
                                                                    yinc, zinc);

                x = _verts[i*(_nstream+1)].x() + dx;
                y = _verts[i*(_nstream+1)].y() + dy;
                z = _verts[i*(_nstream+1)].z() + dz;
                _verts[i*(_nstream+1)+j].setCoordinates(x, y, z);
            }
            else
            {
                // Trailing vertex at "infinity"

                dx = uinfvec(0)*_infdist/uinf; 
                dy = 0.;
                dz = uinfvec(2)*_infdist/uinf; 

                xinc = _verts[i*(_nstream+1)+_nstream-1].xInc() + dx;
                yinc = _verts[i*(_nstream+1)+_nstream-1].yInc() + dy;
                zinc = _verts[i*(_nstream+1)+_nstream-1].zInc() + dz;
                _verts[i*(_nstream+1)+j].setIncompressibleCoordinates(xinc,
                                                                    yinc, zinc);

                x = _verts[i*(_nstream+1)+_nstream-1].x() + dx;
                y = _verts[i*(_nstream+1)+_nstream-1].y() + dy;
                z = _verts[i*(_nstream+1)+_nstream-1].z() + dz;
                _verts[i*(_nstream+1)+j].setCoordinates(x, y, z);

                xviz = _verts[i*(_nstream+1)+_nstream-1].x() + uinfvec(0)*dt;
                yviz = _verts[i*(_nstream+1)+_nstream-1].y() + uinfvec(1)*dt;
                zviz = _verts[i*(_nstream+1)+_nstream-1].z() + uinfvec(2)*dt;
                _verts[i*(_nstream+1)+j].setVizCoordinates(xviz, yviz, zviz);
            }
            _verts[i*(_nstream+1)+j].setIdx(next_global_vertidx);
            _verts[i*(_nstream+1)+j].setWakeTime(-double(j)*dt);
            next_global_vertidx += 1;
        }
    }

    // Create tri panels (which can roll up). Tri panels are used for this
    // portion of the wake because panels are required to be planar, and
    // with quads there would be significant distortion.

    ntris = (_nspan-1)*(_nstream-1)*2;
    _tris.resize(ntris);
    for ( i = 0; int(i) < _nspan-1; i++ )
    {
        for ( j = 0; int(j) < _nstream-1; j++ )
        {
            blvert = i*(_nstream+1)+j+1;
            brvert = (i+1)*(_nstream+1)+j+1;
            trvert = (i+1)*(_nstream+1)+j;
            tlvert = i*(_nstream+1)+j;
            
            _tris[i*(_nstream-1)*2+j*2].setIdx(next_global_elemidx);
            _tris[i*(_nstream-1)*2+j*2].addVertex(&_verts[tlvert]);
            _tris[i*(_nstream-1)*2+j*2].addVertex(&_verts[blvert]);
            _tris[i*(_nstream-1)*2+j*2].addVertex(&_verts[brvert]);
            next_global_elemidx += 1;
            
            _tris[i*(_nstream-1)*2+j*2+1].setIdx(next_global_elemidx);
            _tris[i*(_nstream-1)*2+j*2+1].addVertex(&_verts[brvert]);
            _tris[i*(_nstream-1)*2+j*2+1].addVertex(&_verts[trvert]);
            _tris[i*(_nstream-1)*2+j*2+1].addVertex(&_verts[tlvert]);
            next_global_elemidx += 1;
        }
    }

    // Create quad panels which extend to 1000*maxspan
    
    nquads = _nspan-1;
    _quads.resize(nquads);
    for ( i = 0; int(i) < _nspan-1; i++ )
    {
        blvert = i*(_nstream+1)+_nstream;
        tlvert = i*(_nstream+1)+_nstream-1;
        trvert = (i+1)*(_nstream+1)+_nstream-1;
        brvert = (i+1)*(_nstream+1)+_nstream;
        _quads[i].setIdx(next_global_elemidx);
        _quads[i].addVertex(&_verts[tlvert]);
        _quads[i].addVertex(&_verts[blvert]);
        _quads[i].addVertex(&_verts[brvert]);
        _quads[i].addVertex(&_verts[trvert]);
        next_global_elemidx += 1;
    }
}

/******************************************************************************/
//
// Get wake idx
//
/******************************************************************************/
int Wake::idx () const { return _idx; }

/******************************************************************************/
//
// Convects wake vertices downstream (a.k.a. wake rollup) using 1st-order
// Euler integration.
//
// 1st-order integration is still pretty accurate here because the wake
// convection algorithm allows each point to take only 1 time step. Increasing
// the itegration order causes difficulty because the evaluation points end up
// very close to panel edges, but not on them (which would make the velocity
// contribution identically 0 due to the vortex core model). The integration
// order can potentially be increased if that problem can be worked around, but
// it's probably not worthwhile since this method is already pretty accurate.
//
// Induced velocities are computed in incompressible coordinates. Compressible
// velocity is: Velocity_c = (U_i/beta, V_i, W_i)
//
/******************************************************************************/
void Wake::convectVertices ( const double & dt,
                             const std::vector<Panel *> & allsurf,
                             const std::vector<Panel *> & allwake )
{
    int i, j;
    unsigned int v, k, nsurfpan, nwakepan, nmove;
    Eigen::Vector3d k1, dvel, k1comp, dvelcomp;
    double xinc, yinc, zinc, x, y, z, beta;

    nsurfpan = allsurf.size();
    nwakepan = allwake.size();
    nmove = _nspan*(_nstream-1);
    beta = std::sqrt(1. - std::pow(minf, 2.0));

#pragma omp parallel for private(v,i,j,x,y,z,xinc,yinc,zinc,k,k1,dvel,k1comp,\
                                 dvelcomp)
    for ( v = 0; v < nmove; v++ )
    {
        i = int(floor(v/(_nstream-1)));
        j = v - i*(_nstream-1);
        
        xinc = _verts[i*(_nstream+1)+j].xInc();
        yinc = _verts[i*(_nstream+1)+j].yInc();
        zinc = _verts[i*(_nstream+1)+j].zInc();

        x = _verts[i*(_nstream+1)+j].x();
        y = _verts[i*(_nstream+1)+j].y();
        z = _verts[i*(_nstream+1)+j].z();
    
        // Calculate velocity and update position
    
        if (j == 0)
        {
            // At trailing edge, use average top + bottom surface velocity
            // Note on k1(0): induced part is scaled by 1/beta, so we have to
            // remove that scaling first to get the correct velocity in
            // incompressible coordinates.
        
            k1(0) = beta * (
                    0.5*(_topteverts[i]->data(2)+_botteverts[i]->data(2))
                  - uinfvec(0) ) + uinfvec(0);
            k1(1) = 0.5*(_topteverts[i]->data(3)+_botteverts[i]->data(3));
            k1(2) = 0.5*(_topteverts[i]->data(4)+_botteverts[i]->data(4));

            k1comp(0) = 0.5*(_topteverts[i]->data(2)+_botteverts[i]->data(2));
            k1comp(1) = 0.5*(_topteverts[i]->data(3)+_botteverts[i]->data(3));
            k1comp(2) = 0.5*(_topteverts[i]->data(4)+_botteverts[i]->data(4));
        }
        else
        {
          // Elsewhere in the wake, sum the surface and wake influences
        
            k1 = uinfvec;
            k1comp = uinfvec;
            for ( k = 0; k < nsurfpan; k++ )
            {
                dvel = allsurf[k]->inducedVelocity(xinc, yinc, zinc, false,
                                                   "top", true);
                k1 += dvel;

                dvelcomp << dvel(0)/beta, dvel(1), dvel(2);
                k1comp += dvelcomp;
            }
            for ( k = 0; k < nwakepan; k++ )
            {
                dvel = allwake[k]->vortexVelocity(xinc, yinc, zinc, _rcore,
                                                  true);
                k1 += dvel;

                dvelcomp << dvel(0)/beta, dvel(1), dvel(2);
                k1comp += dvelcomp;
            }
        }
        if (i == 0)
        {
            k1(1) = 0.;
            k1comp(1) = 0.;
        }
        k1 *= dt;
        k1comp *= dt;

        _newxinc[v] = xinc + k1(0);
        _newyinc[v] = yinc + k1(1);
        _newzinc[v] = zinc + k1(2);

        _newx[v] = x + k1comp(0);
        _newy[v] = y + k1comp(1);
        _newz[v] = z + k1comp(2);
    }
}

/******************************************************************************/
//
// Updates wake with new positions computed during the convect routine
//
/******************************************************************************/
void Wake::update ()
{
    unsigned int i, j, ntris, nquads;
    double xinc, yinc, zinc, x, y, z, xviz, yviz, zviz;
    
    // Update vertex positions. New position for vertex (i,j) is equal to
    // convected position of vertex (i,j-1).

#pragma omp parallel for private(i,j,x,y,z,xinc,yinc,zinc,xviz,yviz,zviz)
    for ( i = 0; int(i) < _nspan; i++ )
    {
        _verts[i*(_nstream+1)].incrementWakeTime(dt);
        for ( j = 1; int(j) < _nstream; j++ )
        {
            x = _newx[i*(_nstream-1)+j-1];
            y = _newy[i*(_nstream-1)+j-1];
            z = _newz[i*(_nstream-1)+j-1];
            _verts[i*(_nstream+1)+j].setCoordinates(x, y, z);

            xinc = _newxinc[i*(_nstream-1)+j-1];
            yinc = _newyinc[i*(_nstream-1)+j-1];
            zinc = _newzinc[i*(_nstream-1)+j-1];
            _verts[i*(_nstream+1)+j].setIncompressibleCoordinates(xinc, yinc,
                                                                  zinc);
            _verts[i*(_nstream+1)+j].incrementWakeTime(dt);
        }
    
        // Trailing vertices at "infinity" extend along freestream direction

        x = _newx[i*(_nstream-1)+_nstream-2] + uinfvec(0)*_infdist/uinf;
        y = _newy[i*(_nstream-1)+_nstream-2] + uinfvec(1)*_infdist/uinf;
        z = _newz[i*(_nstream-1)+_nstream-2] + uinfvec(2)*_infdist/uinf;
        _verts[i*(_nstream+1)+_nstream].setCoordinates(x, y, z);
        
        xinc = _newxinc[i*(_nstream-1)+_nstream-2] + uinfvec(0)*_infdist/uinf;
        yinc = _newyinc[i*(_nstream-1)+_nstream-2] + uinfvec(1)*_infdist/uinf;
        zinc = _newzinc[i*(_nstream-1)+_nstream-2] + uinfvec(2)*_infdist/uinf;
        _verts[i*(_nstream+1)+_nstream].setIncompressibleCoordinates(xinc, yinc,
                                                                     zinc);

        xviz = _newx[i*(_nstream-1)+_nstream-2] + uinfvec(0)*dt;
        yviz = _newy[i*(_nstream-1)+_nstream-2] + uinfvec(1)*dt;
        zviz = _newz[i*(_nstream-1)+_nstream-2] + uinfvec(2)*dt;
        _verts[i*(_nstream+1)+_nstream].setVizCoordinates(xviz, yviz, zviz);

        _verts[i*(_nstream+1)+_nstream].incrementWakeTime(dt);
    }

    // Recompute panel geometry
    
    ntris = _tris.size(); 
#pragma omp parallel for private(i)
    for ( i = 0; i < ntris; i++ )
    {
        _tris[i].recomputeGeometry();
    }

    nquads = _quads.size();
#pragma omp parallel for private(i)
    for ( i = 0; i < nquads; i++ )
    {
        _quads[i].recomputeGeometry();
    }
}

/******************************************************************************/
//
// Access to verts and panels
//
/******************************************************************************/
unsigned int Wake::nVerts () const { return _verts.size(); }
unsigned int Wake::nTris () const { return _tris.size(); }
unsigned int Wake::nQuads () const { return _quads.size(); }
Vertex * Wake::vert ( unsigned int vidx )
{
#ifdef DEBUG
    if (vidx >= _verts.size())
        conditional_stop(1, "Wake::vert", "Index out of range.");
#endif

    return &_verts[vidx];
}

TriPanel * Wake::triPanel ( unsigned int tidx )
{
#ifdef DEBUG
    if (tidx >= _tris.size())
        conditional_stop(1, "Wake::triPanel", "Index out of range.");
#endif

    return &_tris[tidx];
}

QuadPanel * Wake::quadPanel ( unsigned int qidx )
{
#ifdef DEBUG
    if (qidx >= _quads.size())
        conditional_stop(1, "Wake::quadPanel", "Index out of range.");
#endif

    return &_quads[qidx];
}

/******************************************************************************/
//
// Induced velocity at a point due to wake modeled as planar and aligned with
// the freestream direction. Influence of bound leg (at wing TE) is optional,
// and it is also possible to specify a spanwise index where the point x, y, z
// is assumed to lie on the trailing leg (to ignore self-induced velocity).
//
// This is used for Trefftz plane induced drag calculation, where a planar
// wake is preferred (see Kroo paper and comments for farfieldForces method).
//
// This calculation uses incompressible coordinates for wake geometry.
//
/******************************************************************************/
Eigen::Vector3d Wake::planarInducedVelocity ( const double & x,
                                             const double & y, const double & z,
                                             bool include_bound_leg,
                                             int on_trailing_leg ) const
{
    int i;
    double gamma, x1, y1, z1, x2, y2, z2;
    Eigen::Vector3d w;

    w << 0., 0., 0.;

    // Influence of trailing legs (do not include centerline, because net
    // circulation of trailing leg is 0 due to symmetry)

    for ( i = 1; i < _nspan; i++ )
    {
        if (i < _nspan-1)
            gamma = _quads[i-1].doubletStrength()
                  - _quads[i].doubletStrength();
        else
            gamma = _quads[i-1].doubletStrength();

        x1 = _verts[i*(_nstream+1)+0].xInc();
        y1 = _verts[i*(_nstream+1)+0].yInc();
        z1 = _verts[i*(_nstream+1)+0].zInc();
        x2 = x1 + _infdist*uinfvec(0)/uinf;
        y2 = y1;
        z2 = z1 + _infdist*uinfvec(2)/uinf;

        // Induced velocity due to leg + mirror contribution. Do not count
        // self-induced velocity of a trailing leg.

        if (i != on_trailing_leg)
            w += vortex_velocity(x, y, z, x1, y1, z1, x2, y2, z2, _rcore,
                                 true)*gamma;
        w -= vortex_velocity(x, y, z, x1, -y1, z1, x2, -y2, z2, _rcore,
                             true)*gamma;
    }

    // Influence of bound legs at TE

    if (include_bound_leg)
    {
        for ( int(i) = 0; i < _nspan-1; i++ )
        {
            gamma = _quads[i].doubletStrength();

            x1 = _verts[i*(_nstream+1)+0].xInc();
            y1 = _verts[i*(_nstream+1)+0].yInc();
            z1 = _verts[i*(_nstream+1)+0].zInc();
            x2 = _verts[(i+1)*(_nstream+1)+0].xInc();
            y2 = _verts[(i+1)*(_nstream+1)+0].yInc();
            z2 = _verts[(i+1)*(_nstream+1)+0].zInc();

            // Induced velocity + mirror contribution

            w += vortex_velocity(x, y, z, x1, y1, z1, x2, y2, z2, _rcore,
                                 false)*gamma;
            w += vortex_velocity(x, y, z, x2, -y2, z2, x1, -y1, z1, _rcore,
                                 false)*gamma;
        }
    }

    return w;
}

/******************************************************************************/
//
// Computes lift and induced drag in Trefftz plane. Note that this models the
// wake as planar and aligned with the freestream. Using the rolled-up wake
// results in high sensitivity to the rolled up shape, and it was found to
// underpredict induced drag and be very sensitive to the rollup distance.
// See Kroo paper for more information.
//
// Trefftz plane calculation is performed using incompressible coordinates,
// though results should be equivalent either way (see BAH book).
//
/******************************************************************************/
void Wake::farfieldForces ( const double & xtrefftz, const double & ztrefftz,
                            const std::vector<Wake *> & allwake,
                            double & lift, double & drag ) const
{
    int i;
    unsigned int j, nwake;
    double a, b, c, d, dx, dy, dz, wy, wz;
    std::vector<Eigen::Vector3d> w;
    Eigen::Vector3d p0, p, p1, p2, edge;
    Eigen::Matrix3d transform;

    // Transformation from inertial to Trefftz frame

    transform = euler_rotation(0., -alpha, 0.);

    // Get equation for Trefftz plane

    p0 << xtrefftz, 0., ztrefftz;
    compute_plane(p0, uinfvec/uinf, a, b, c, d);

    // Wake induced velocity

    w.resize(_nspan-1);
    nwake = allwake.size();
#pragma omp parallel for private(i,p0,p,j)
    for ( i = 0; i < _nspan-1; i++ )
    {
        // Aft TE midpoint projected on Trefftz plane

        p0(0) = 0.5*(_verts[    i*(_nstream+1)+0].xInc()
              +      _verts[(i+1)*(_nstream+1)+0].xInc());
        p0(1) = 0.5*(_verts[    i*(_nstream+1)+0].yInc()
              +      _verts[(i+1)*(_nstream+1)+0].yInc());
        p0(2) = 0.5*(_verts[    i*(_nstream+1)+0].zInc()
              +      _verts[(i+1)*(_nstream+1)+0].zInc());
        p = line_plane_intersection(p0, uinfvec/uinf, a, b, c, d);

        // Induced velocity from all wakes and transform to Trefftz plane

        w[i] << 0., 0., 0.;
        for ( j = 0; j < nwake; j++ )
        {
            w[i] += allwake[j]->planarInducedVelocity(p(0), p(1), p(2),
                                                      false);
        }
        w[i] = transform*w[i];
    }

    // Forces

    lift = 0.;
    drag = 0.;
    for ( i = 0; i < _nspan-1; i++ )
    {
        // TE points projected on Trefftz plane

        p1(0) = _verts[i*(_nstream+1)+0].xInc();
        p1(1) = _verts[i*(_nstream+1)+0].yInc();
        p1(2) = _verts[i*(_nstream+1)+0].zInc();
        p1 = line_plane_intersection(p1, uinfvec/uinf, a, b, c ,d);

        p2(0) = _verts[(i+1)*(_nstream+1)+0].xInc();
        p2(1) = _verts[(i+1)*(_nstream+1)+0].yInc();
        p2(2) = _verts[(i+1)*(_nstream+1)+0].zInc();
        p2 = line_plane_intersection(p2, uinfvec/uinf, a, b, c ,d);

        dx = p2(0) - p1(0);
        dy = p2(1) - p1(1);
        dz = p2(2) - p1(2);
        edge << dx, dy, dz;
        edge = transform*edge;

        // In-plane velocity components

        wy = w[i](1);
        wz = w[i](2);

        // Lift and induced drag

        lift += _quads[i].doubletStrength()*edge(1);
        drag += (edge(2)*wy - edge(1)*wz) * _quads[i].doubletStrength();
    }

    // Final factors including mirror image factors

    lift *= 2.*rhoinf*uinf;
    drag *= rhoinf;
}
