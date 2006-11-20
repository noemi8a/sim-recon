// $Id$
//
//    File: DReferenceTrajectory.cc
// Created: Wed Jul 19 13:42:58 EDT 2006
// Creator: davidl (on Darwin swire-b241.jlab.org 8.7.0 powerpc)
//

#include <TVector3.h>

#include "DReferenceTrajectory.h"
#include "DTrackCandidate.h"
#include "DMagneticFieldStepper.h"

//---------------------------------
// DReferenceTrajectory    (Constructor)
//---------------------------------
DReferenceTrajectory::DReferenceTrajectory(const DMagneticFieldMap *bfield
														, double q, const TVector3 &pos, const TVector3 &mom
														, swim_step_t *swim_steps
														, int max_swim_steps
														, double step_size)
{
	// It turns out that the greatest bottleneck in speed here comes from
	// allocating/deallocating the large block of memory required to hold
	// all of the trajectory info. The preferred way of calling this is 
	// with a pointer allocated once at program startup. This code block
	// though allows it to be allocated here if necessary.
	if(!swim_steps){
		own_swim_steps = true;
		this->max_swim_steps = 50000;
		this->swim_steps = new swim_step_t[this->max_swim_steps];
	}else{
		own_swim_steps = false;
		this->max_swim_steps = max_swim_steps;
		this->swim_steps = swim_steps;
	}

	// Copy some values into data members
	this->q = q;
	this->step_size = step_size;
	this->bfield = bfield;
	
	// Swim the trajectory
	Reswim(pos, mom);
}

//---------------------------------
// Reswim
//---------------------------------
void DReferenceTrajectory::Reswim(const TVector3 &pos, const TVector3 &mom)
{
	/// (Re)Swim the trajectory starting from pos with momentum mom.
	/// This will use the charge and step size (if given) passed to
	/// the constructor when the object was created. It will also
	/// (re)use the sim_step buffer, replacing it's contents.

	DMagneticFieldStepper stepper(bfield, q, &pos, &mom);
	if(step_size>0.0)stepper.SetStepSize(step_size);
		
	// Step until we hit a boundary (don't track more than 20 meters)
	swim_step_t *swim_step = this->swim_steps;
	Nswim_steps = 0;
	for(double s=0; fabs(s)<2000.0; Nswim_steps++, swim_step++){

		if(Nswim_steps>=this->max_swim_steps){
			cerr<<__FILE__<<":"<<__LINE__<<" Too many steps in trajectory. Truncating..."<<endl;
			break;
		}

		stepper.GetDirs(swim_step->sdir, swim_step->tdir, swim_step->udir);
		stepper.GetPosition(swim_step->origin);
		stepper.GetMomentum(swim_step->mom);
		swim_step->Ro = stepper.GetRo();
		swim_step->s = s;

		// Exit loop if we leave the tracking volume
		if(swim_step->origin.Perp()>65.0){break;} // ran into BCAL
		if(swim_step->origin.Z()>650.0){break;} // ran into FCAL
		if(swim_step->origin.Z()<-50.0){break;} // ran into UPV

		// Swim to next
		s += stepper.Step(NULL);
	}

	// OK. At this point the positions of the trajectory in the lab
	// frame have been recorded along with the momentum of the
	// particle and the directions of reference trajectory
	// coordinate system at each point.
}

//---------------------------------
// ~DReferenceTrajectory    (Destructor)
//---------------------------------
DReferenceTrajectory::~DReferenceTrajectory()
{
	if(own_swim_steps){
		delete[] swim_steps;
	}
}

//---------------------------------
// DistToRT
//---------------------------------
double DReferenceTrajectory::DistToRT(TVector3 hit)
{
	// First, find closest step to point
	swim_step_t *swim_step = swim_steps;
	swim_step_t *step=NULL;
	double min_delta2 = 1.0E6;
	for(int i=0; i<Nswim_steps; i++, swim_step++){

		TVector3 pos_diff = swim_step->origin - hit;
		double delta2 = pos_diff.Mag2();

		if(delta2 < min_delta2){
			min_delta2 = delta2;
			step = swim_step;
		}
	}
	
	// Next, define a point on the helical segment defined by the
	// swim step it the RT coordinate system. The directions of
	// the RT coordinate system are defined by step->xdir, step->ydir,
	// and step->zdir. The coordinates of a point on the helix
	// in this coordinate system are:
	//
	//   x = Ro*(cos(phi) - 1)
	//   y = Ro*sin(phi)
	//   z = phi*(dz/dphi)
	//
	// where phi is the phi angle of the point in this coordinate system.
	// phi=0 corresponds to the swim step point itself
	//
	// Transform the given coordinates to the RT coordinate system
	// and call these x0,y0,z0. Then, the distance of point to a
	// point on the helical segment is given by:
	//
	//   d^2 = (x0-x)^2 + (y0-y)^2 + (z0-z)^2
	//
	// where x,y,z are all functions of phi as given above.
	//
	// writing out d^2 in terms of phi, but using the small angle
	// approximation for the trig functions, an equation for the
	// distance in only phi is obtained. Taking the derivative 
	// and setting it equal to zero leaves a 3rd order polynomial
	// in phi whose root corresponds to the minimum distance.
	// Skipping some math, this equation has the form:
	//
	// d(d^2)/dphi = 0 = Ro^2*phi^3 + 2*alpha*phi + beta
	//
	// where:
	//       alpha = x0*Ro + Ro^2 + (dz/dphi)^2
	//
	//        beta = -2*y0*Ro - 2*z0*(dz/dphi)
	//
	// The above 3rd order poly is convenient in that it does not
	// contain a phi^2 term. This means we can skip the step
	// done in the general case where a change of variables is
	// made such that the 2nd order term disappears.
	//
	// In general, an equation of the form
	//
	//  w^3 + 3.0*b*w + 2*c = 0 
	//
	// has one real root:
	//
	//  w0 = q - p
	//
	// where:
	//    q^3 = d - c
	//    p^3 = d + c
	//    d^2 = b^3 + c^2      (don't confuse with d^2 above!)
	//
	// So for us ...
	//
	//    3b = 2*alpha/(Ro^2)
	//    2c = beta/(Ro^2)

	hit -= step->origin;
	double x0 = hit.Dot(step->sdir);
	double y0 = hit.Dot(step->tdir);
	double z0 = hit.Dot(step->udir);
	double &Ro = step->Ro;
	double Ro2 = Ro*Ro;
	double delta_z = step->mom.Dot(step->udir);
	double delta_phi = step->mom.Dot(step->tdir)/Ro;
	double dz_dphi = delta_z/delta_phi;

	double alpha = x0*Ro + Ro2 + pow(dz_dphi,2.0);
	double beta = -2.0*y0*Ro - 2.0*z0*dz_dphi;
	double b = (2.0*alpha/Ro2)/3.0;
	double c = (beta/Ro2)/2.0;
	double d = sqrt(pow(b,3.0) + pow(c,2.0));
	double q = pow(d-c, 1.0/3.0);
	double p = pow(d+c, 1.0/3.0);
	double phi = q - p;
	
	double dist2 = Ro2/4.0*pow(phi,4.0) + alpha*pow(phi,2.0) + beta*phi + x0*x0 + y0*y0 + z0*z0;

	return sqrt(dist2);
}

//---------------------------------
// FindClosestSwimStep
//---------------------------------
DReferenceTrajectory::swim_step_t* DReferenceTrajectory::FindClosestSwimStep(const DCoordinateSystem *wire, double L)
{
	/// Find the closest swim step to the given wire. The value of
	/// "L" should be the active wire length. The coordinate system
	/// defined by "wire" should have its origin at the center of
	/// the wire with the wire running in the direction of udir.
	
	// Loop over swim steps and find the one closest to the wire
	swim_step_t *swim_step = swim_steps;
	swim_step_t *step=NULL;
	double min_delta2 = 1.0E6;
	double L_over_2 = L/2.0; // half-length of wire in cm
	for(int i=0; i<Nswim_steps; i++, swim_step++){
		// Find the point's position along the wire. Skip this
		// swim step if it is past the end of the wire
		TVector3 pos_diff = swim_step->origin - wire->origin;
		double u = wire->udir.Dot(pos_diff);
		if(fabs(u)>L_over_2)continue;
		
		// Find distance perpendicular to wire
		double s = wire->sdir.Dot(pos_diff);
		double t = wire->tdir.Dot(pos_diff);
		double delta2 = s*s + t*t;

		if(delta2 < min_delta2){
			min_delta2 = delta2;
			step = swim_step;
		}
	}

	return step;	
}

//---------------------------------
// DistToRT
//---------------------------------
double DReferenceTrajectory::DistToRT(const DCoordinateSystem *wire, double L, double *s)
{
	/// Find the closest distance to the given wire in cm. The value of
	/// "L" should be the active wire length (in cm). The coordinate system
	/// defined by "wire" should have its origin at the center of
	/// the wire with the wire running in the direction of udir.
	swim_step_t *step=FindClosestSwimStep(wire, L);
	
	return step ? DistToRT(wire, step, s):std::numeric_limits<double>::quiet_NaN();
}

//---------------------------------
// DistToRTBruteForce
//---------------------------------
double DReferenceTrajectory::DistToRTBruteForce(const DCoordinateSystem *wire, double L, double *s)
{
	/// Find the closest distance to the given wire in cm. The value of
	/// "L" should be the active wire length (in cm). The coordinate system
	/// defined by "wire" should have its origin at the center of
	/// the wire with the wire running in the direction of udir.
	swim_step_t *step=FindClosestSwimStep(wire, L);
	
	return step ? DistToRTBruteForce(wire, step, s):std::numeric_limits<double>::quiet_NaN();
}

//------------------
// GetDistToRT
//------------------
double DReferenceTrajectory::DistToRT(const DCoordinateSystem *wire, const swim_step_t *step, double *s)
{
	/// Calculate the distance of the given wire(in the lab
	/// reference frame) to the Reference Trajectory which the
	/// given swim step belongs to. This uses the momentum directions
	/// and positions of the swim step
	/// to define a curve and calculate the distance of the hit
	/// from it. The swim step should be the closest one to the wire.
	/// IMPORTANT: This approximates the helix locally by a parabola.
	/// This means the swim step should be fairly close
	/// to the wire so that this approximation is valid. If the
	/// reference trajectory from which the swim step came is too
	/// sparse, the results will not be nearly as good.
	
	// Interestingly enough, this is one of the harder things to figure
	// out in the tracking code which is why the explanations may be
	// a bit long.

	// The general idea is to define the helix in a coordinate system
	// in which the wire runs along the z-axis. The distance to the
	// wire is then defined just in the X/Y plane of this coord. system.
	// The distance is expressed as a function of the phi angle in the
	// natural coordinate system of the helix. This way, phi=0 corresponds
	// to the swim step point itself and the DOCA point should be
	// at a small phi angle.
	//
	// The minimum distance between the helical segment and the wire
	// will be a function of sin(phi), cos(phi) and phi. Approximating
	// sin(phi) by phi and cos(phi) by (1-phi^2) leaves a 4th order
	// polynomial in phi. Taking the derivative leaves a 3rd order
	// polynomial whose root is the phi corresponding to the 
	// Distance Of Closest Approach(DOCA) point on the helix. Plugging
	// that value of phi back into the distance formula gives
	// us the minimum distance between the track and the wire.

	// First, we need to define the coordinate system in which the 
	// wire runs along the z-axis. This is actually done already
	// in the CDC package for each wire once, at program start.
	// The directions of the axes are defined in wire->sdir,
	// wire->tdir, and wire->udir.
	
	// Next, define a point on the helical segment defined by the
	// swim step it the RT coordinate system. The directions of
	// the RT coordinate system are defined by step->xdir, step->ydir,
	// and step->zdir. The coordinates of a point on the helix
	// in this coordinate system are:
	//
	//   x = Ro*(cos(phi) - 1)
	//   y = Ro*sin(phi)
	//   z = phi*(dz/dphi)
	//
	// where phi is the phi angle of the point in this coordinate system.
	
	// Now, a vector describing the helical point in the LAB coordinate
	// system is:
	//
	//  h = x*xdir + y*ydir + z*zdir + pos
	//
	// where h,xdir,ydir,zdir and pos are all 3-vectors.
	// xdir,ydir,zdir are unit vectors defining the directions
	// of the RT coord. system axes in the lab coord. system.
	// pos is a vector defining the position of the swim step
	// in the lab coord.system 
	
	// Now we just need to find the extent of "h" in the wire's
	// coordinate system (period . means dot product):
	//
	// s = (h-wpos).sdir
	// t = (h-wpos).tdir
	// u = (h-wpos).udir
	//
	// where wpos is the position of the center of the wire in
	// the lab coord. system and is given by wire->wpos.

	// At this point, the values of s,t, and u repesent a point
	// on the helix in the coord. system of the wire with the
	// wire in the "u" direction and positioned at the origin.
	// The distance(squared) from the wire to the point on the helix
	// is given by:
	//
	// d^2 = s^2 + t^2
	//
	// where s and t are both functions of phi.

	// So, we'll define the values of "s" and "t" above as:
	//
	//   s = A*x + B*y + C*z + D
	//   t = E*x + F*y + G*z + H
	//
	// where A,B,C,D,E,F,G, and H are constants defined below
	// and x,y,z are all functions of phi defined above.
	// (period . means dot product)
	//
	// A = sdir.xdir
	// B = sdir.ydir
	// C = sdir.zdir
	// D = sdir.(pos-wpos)
	//
	// E = tdir.xdir
	// F = tdir.ydir
	// G = tdir.zdir
	// H = tdir.(pos-wpos)
	const TVector3 &xdir = step->sdir;
	const TVector3 &ydir = step->tdir;
	const TVector3 &zdir = step->udir;
	const TVector3 &sdir = wire->sdir;
	const TVector3 &tdir = wire->tdir;
	TVector3 pos_diff = step->origin - wire->origin;
	
	double A = sdir.Dot(xdir);
	double B = sdir.Dot(ydir);
	double C = sdir.Dot(zdir);
	double D = sdir.Dot(pos_diff);

	double E = tdir.Dot(xdir);
	double F = tdir.Dot(ydir);
	double G = tdir.Dot(zdir);
	double H = tdir.Dot(pos_diff);

	// OK, here is the dirty part. Using the approximations given above
	// to write the x and y functions in terms of phi^2 and phi (instead
	// of cos and sin) we put them into the equations for s and t above.
	// Then, inserting those into the equation for d^2 above that, we
	// get a very long equation in terms of the constants A,...H and
	// phi up to 4th order. Combining coefficients for similar powers
	// of phi yields an equation of the form:
	//
	// d^2 = Q*phi^4 + R*phi^3 + S*phi^2 + T*phi + U
	//
	// The dirty part is that it takes the better part of a sheet of
	// paper to work out the relations for Q,...U in terms of
	// A,...H, and Ro, dz/dphi. You can work it out yourself on
	// paper to verify that the equations below are correct.
	double Ro = step->Ro;
	double Ro2 = Ro*Ro;
	double delta_z = step->mom.Dot(step->udir);
	double delta_phi = step->mom.Dot(step->tdir)/Ro;
	double dz_dphi = delta_z/delta_phi;

	double Q = pow(A*Ro/2.0, 2.0) + pow(E*Ro/2.0, 2.0);
	double R = -(2.0*A*B*Ro2 + 2.0*A*C*Ro*dz_dphi + 2.0*E*F*Ro2 + 2.0*E*G*Ro*dz_dphi)/2.0;
	double S = pow(B*Ro, 2.0) + pow(C*dz_dphi,2.0) + 2.0*B*C*Ro*dz_dphi - 2.0*A*D*Ro/2.0
					+ pow(F*Ro, 2.0) + pow(G*dz_dphi,2.0) + 2.0*F*G*Ro*dz_dphi - 2.0*E*H*Ro/2.0;
	double T = 2.0*B*D*Ro + 2.0*C*D*dz_dphi + 2.0*F*H*Ro + 2.0*G*H*dz_dphi;
	double U = D*D + H*H;
	
	// Aaarghh! my fingers hurt just from typing all of that!
	//
	// OK, now we differentiate the above equation for d^2 to get:
	//
	// d(d^2)/dphi = 4*Q*phi^3 + 3*R*phi^2 + 2*S*phi + T
	//
	// NOTE: don't confuse "R" with "Ro" in the above equations!
	//
	// Now we have to solve the 3rd order polynomial for the phi value of
	// the point of closest approach on the RT. This is a well documented
	// procedure. Essentially, when you have an equation of the form:
	//
	//  x^3 + a2*x^2 + a1*x + a0 = 0;
	//
	// a change of variables is made such that w = x + a2/3 which leads
	// to a third order poly with no w^2 term:
	//
	//  w^3 + 3.0*b*w + 2*c = 0 
	//
	// where:
	//    b = a1/3 - (a2^2)/9
	//    c = a0/2 - a1*a2/6  + (a2^3)/27
	//
	// The one real root of this is:
	//
	//  w0 = q - p
	//
	// where:
	//    q^3 = d - c
	//    p^3 = d + c
	//    d^2 = b^3 + c^2      (don't confuse with d^2 above!)
	//
	// For us this means that:
	//    a2 = 3*R/(4*Q)
	//    a1 = 2*S/(4*Q)
	//    a0 =   T/(4*Q)
	//
	// A potential problem could occur if Q is at or very close to zero.
	// This situation occurs when both A and E are zero. This would mean
	// that both sdir and tdir are perpendicular to xdir which means
	// xdir is in the same direction as udir (got that?). Physically,
	// this corresponds to the situation when both the momentum and
	// the magnetic field are perpendicular to the wire (though not
	// necessarily perpendicular to each other). This situation can't
	// really occur in the CDC detector where the chambers are well
	// contained in a region where the field is essentially along z as
	// are the wires.
	//
	// Just to be safe, we check that Q is greater than
	// some minimum before solving for phi. If it is too small, we fall
	// back to solving the quadratic equation for phi.
	double phi =0.0;
	if(fabs(Q)>1.0E-6){
		double fourQ = 4.0*Q;
		double a2 = 3.0*R/fourQ;
		double a1 = 2.0*S/fourQ;
		double a0 =     T/fourQ;

		double b = a1/3.0 - a2*a2/9.0;
		double c = a0/2.0 - a1*a2/6.0 + a2*a2*a2/27.0;

		double d = sqrt(pow(b, 3.0) + pow(c, 2.0)); // occasionally, this is zero. See below
		double q = pow(d - c, 1.0/3.0);
		double p = pow(d + c, 1.0/3.0);

		double w0 = q - p;
		phi = w0 - a2/3.0;
	}else{
		double a = 3.0*R;
		double b = 2.0*S;
		double c = 1.0*T;
		phi = (-b + sqrt(b*b - 4.0*a*c))/(2.0*a); 
	}

	// Sometimes the "d" used in solving the 3rd order polynmial above 
	// can be nan due to the sqrt argument being negative. I'm not sure
	// exactly what this means (e.g. is this due to round-off, are there
	// no roots, ...) Also unclear is how to handle it. The only two choices
	// I can think of are : 1.) set phi to zero or 2.) return the nan
	// value. Option 1.) tries to keep the hit while option 2 ties to ignore
	// it. Both options should probably be studied at some point. For now
	// though, it looks (at least preliminarily) like this occurs slightly
	// less than 1% of the time on valid hits so we go ahead with option 2.

	// Use phi to calculate DOCA
	double d2 = U + phi*(T + phi*(S + phi*(R + phi*Q)));
	double d = sqrt(d2);
	
	// Calculate distance along track ("s")
	double dz = dz_dphi*phi;
	double Rodphi = Ro*phi;
	double ds = sqrt(dz*dz + Rodphi*Rodphi);
	if(s)*s=step->s + (phi>0.0 ? ds:-ds);

	return d; // WARNING: This could return nan!
}

//------------------
// DistToRTBruteForce
//------------------
double DReferenceTrajectory::DistToRTBruteForce(const DCoordinateSystem *wire, const swim_step_t *step, double *s)
{
	/// Calculate the distance of the given wire(in the lab
	/// reference frame) to the Reference Trajectory which the
	/// given swim step belongs to. This uses the momentum directions
	/// and positions of the swim step
	/// to define a curve and calculate the distance of the hit
	/// from it. The swim step should be the closest one to the wire.
	/// IMPORTANT: This calculates the distance using a "brute force"
	/// method of taking tiny swim steps to find the minimum distance.
	/// It is vey SLOW and you should be using DistToRT(...) instead.
	/// This is only here to provide an independent check of DistToRT(...).
	
	const TVector3 &xdir = step->sdir;
	const TVector3 &ydir = step->tdir;
	const TVector3 &zdir = step->udir;
	const TVector3 &sdir = wire->sdir;
	const TVector3 &tdir = wire->tdir;
	TVector3 pos_diff = step->origin - wire->origin;
	
	double Ro = step->Ro;
	double delta_z = step->mom.Dot(step->udir);
	double delta_phi = step->mom.Dot(step->tdir)/Ro;
	double dz_dphi = delta_z/delta_phi;

	// Brute force
	double min_d2 = 1.0E6;
	double phi=M_PI;
	for(int i=-2000; i<2000; i++){
		double myphi=(double)i*0.000005;
		TVector3 d = Ro*(cos(myphi)-1.0)*xdir
	            	+ Ro*sin(myphi)*ydir
						+ dz_dphi*myphi*zdir
						+ pos_diff;

		double d2 = pow(d.Dot(sdir),2.0) + pow(d.Dot(tdir),2.0);
		if(d2<min_d2){
			min_d2 = d2;
			phi = myphi;
		}
	}
	double d2 = min_d2;
	double d = sqrt(d2);
	
	// Calculate distance along track ("s")
	double dz = dz_dphi*phi;
	double Rodphi = Ro*phi;
	double ds = sqrt(dz*dz + Rodphi*Rodphi);
	if(s)*s=step->s + (phi>0.0 ? ds:-ds);

	return d;
}


