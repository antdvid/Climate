/**********************************************************************
 * 		iFluid.h
 **********************************************************************/

#ifndef _FT_IFLUID_H_
#define _FT_IFLUID_H_

#include <vector>
#include <petscksp.h>
#include <assert.h>

#include "FronTier.h"
#include "solver.h"

#define         SOLID_COMP		1
#define         LIQUID_COMP1		2
#define         LIQUID_COMP2		3

#define		ifluid_comp(comp)   (((comp) == LIQUID_COMP1 || 	\
		comp == LIQUID_COMP2) ? YES : NO)

enum EBM_COORD
{
    COORD_X = 0,  COORD_Y = 1,  COORD_Z = 2
};

struct _IF_FIELD {
	double **vel;			/* Velocities */
	double *phi;
	double *pres;			/* Pressure */
	double *vort;			/* Vorticity in 2D */
	double **vort3d;		/* Vorticity in 3D */
	double *div;
};
typedef struct _IF_FIELD IF_FIELD;

struct _IF_MOVIE_OPTION {
	/* HDF movie options */
	boolean plot_bullet;		/* For gd movie */
	boolean plot_comp;
	boolean plot_pres;
	boolean plot_vort;
	boolean plot_velo;
	boolean plot_cross_section[MAXD]; /* 3D 0: yz; 1: zx; 2: xy */
	/* VTK movie options */
	boolean plot_vel_vector;	  /* Plot velocity vector field */
};
typedef struct _IF_MOVIE_OPTION IF_MOVIE_OPTION;

enum _PROJC_METHOD {
	ERROR_PROJC_SCHEME		= -1,
        SIMPLE			=  1,
        BELL_COLELLA,
        KIM_MOIN,
        PEROT_BOTELLA
};
typedef enum _PROJC_METHOD PROJC_METHOD;

enum _ADVEC_METHOD {
	ERROR_ADVEC_SCHEME		= -1,
        UPWIND			=  1,
        WENO
};
typedef enum _ADVEC_METHOD ADVEC_METHOD;

enum _ELLIP_METHOD {
	ERROR_ELLIP_SCHEME		= -1,
	SIMPLE_ELLIP		= 1,
	CIM_ELLIP
};
typedef enum _ELLIP_METHOD ELLIP_METHOD;

struct _NS_SCHEME {
	PROJC_METHOD projc_method;
	ADVEC_METHOD advec_method;
	ELLIP_METHOD ellip_method;
};
typedef struct _NS_SCHEME NS_SCHEME;

typedef struct {
        int dim;
        POINTER level_func_params;
	IF_MOVIE_OPTION *movie_option;
	NS_SCHEME num_scheme;
        double rho1;
        double rho2;
	double mu1;
	double mu2;
	double U1[MAXD];
	double U2[MAXD];
	double gravity[MAXD];
	double surf_tension;
	double smoothing_radius;
	COMPONENT m_comp1;
	COMPONENT m_comp2;
	IF_FIELD *field;
	int adv_order;
} IF_PARAMS;

struct _FLOW_THROUGH_PARAMS {
        POINT *oldp;
        COMPONENT comp;
};
typedef struct _FLOW_THROUGH_PARAMS FLOW_THROUGH_PARAMS;

enum _TIME_FUNC_TYPE {
	CONSTANT		=  1,
	PULSE_FUNC,
	SINE_FUNC	
};
typedef enum _TIME_FUNC_TYPE TIME_FUNC_TYPE;

struct _TIME_DEPENDENT_PARAMS {
	TIME_FUNC_TYPE td_type;
	double v_base[MAXD],p_base;
	double v_peak[MAXD],p_peak;
	double v_tail[MAXD],p_tail;
	double v_amp[MAXD],p_amp;
	double omega,phase;
	double T[10];
};
typedef struct _TIME_DEPENDENT_PARAMS TIME_DEPENDENT_PARAMS;

/******************************************************************************
 * 		lcartsn.h
 * A simple incompressible flow solver using the ghost fluid method and the
 * projection method.
 *
 * the main function is 
 * 	L_CARTESIAN::solve().
 *
 * References:
 ******************************************************************************/

class SOLVER;
class Incompress_Solver_Basis;

//enum VISITED_TYPE {UNVISITED, VISITED, PARTIAL_VISITED, FULL_VISITED};

//-------------------------------------------------
//		STATES
// NOTE:
//      L_STATE/L_STATE_RECT_EDGEshould be put into 
// lcartsn.h. However, there are some trouble
// to compile in that way.
//-------------------------------------------------
// states inside a cell

class L_STATE{
public:
	double m_U[MAXD];		// velocity vector
	double m_exactU[MAXD];
	double m_P;			// pressure
	double m_exactP;
	double m_phi;			// (Brown's) phi*dt
	double m_q;
	double m_adv[MAXD];

	double m_mu;		// smoothed 
	double m_rho;		// smoothed
	double div_U;			// Velocity divergence
	double grad_q[MAXD];		// Gradient of q
	double f_surf[MAXD];		// Surface force (such as tension)

	L_STATE();
	void setZero(void);
};
// states on edge

//------------------------------------------------------
//		MESH
//------------------------------------------------------
// note that the following VERTEX2D/RECT_EDGE are different 
// from those defined in MESH2D.h

class L_RECTANGLE {
public:
	int m_index;			// rectangle index
	int comp;			 
	L_STATE m_state;
	double m_coords[MAXD];	
	int icoords[MAXD];

	L_RECTANGLE();

	void setCoords(double*,int);
};

class Incompress_Solver_Basis{
public:
       	Incompress_Solver_Basis() {}; // constructor
	virtual ~Incompress_Solver_Basis() {};

	virtual void setInitialCondition(void) = 0; //Initialization
	virtual void solve(double dt) = 0; //main step function
};

class Incompress_Solver_Smooth_Basis:public Incompress_Solver_Basis{
public:
        //constructor
	Incompress_Solver_Smooth_Basis(Front &front);
	virtual ~Incompress_Solver_Smooth_Basis() {};

	double m_dt;
	double accum_dt;
	double max_speed;
	double min_pressure;
        double max_pressure;
        double min_value; //for debugging
	double max_value; //for debugging
	double max_dt;
	double min_dt;
	double *top_h;
	int dim;

	void initMesh(void);
	void setAdvectionDt(void); 
			//using max speed and hmin to determine max_dt, min_dt
	void readFrontInteriorStates(char *state_name);
	void printFrontInteriorStates(char *state_name);
	void initMovieVariables(void);
	void augmentMovieVariables(void);
	void getVelocity(double *p, double *U);
	void initSampleVelocity(char *in_name);

	//Initialization of States
	void (*getInitialState) (COMPONENT,double*,L_STATE&,int,IF_PARAMS*);
	int (*findStateAtCrossing)(Front*,int*,GRID_DIRECTION,int,
				POINTER*,HYPER_SURF**,double*);
	void applicationSetComponent();
	void applicationSetStates();


	//User interface
	virtual void setInitialCondition(void) = 0;
	virtual void solve(double dt) = 0; // main step function

protected:
	Front *front;
	// On topological grid
	RECT_GRID *top_grid;
	double *array;
	double *source;
	double *diff_coeff;
	COMPONENT *top_comp;
	IF_PARAMS *iFparams;
	IF_FIELD  *field;

	int *top_gmax;
	int *lbuf, *ubuf;
	double *top_L, *top_U;
	int **ij_to_I, **I_to_ij;
	int ***ijk_to_I, **I_to_ijk;

	// Sweeping limites
	int imin, jmin, kmin;
	int imax, jmax, kmax;

	//member data: mesh storage
	std::vector<L_RECTANGLE>   cell_center;

	//member data:
	int    m_comp[2];
	double m_mu[2];
	double m_rho[2];// two components at most
	double m_sigma; //surface tension
	double m_smoothing_radius;// used by smoothing function

	double hmin; //smallest spacing
	double mu_min; //smallest viscocity
	double rho_min;// smallest density
	double m_t;
	double m_t_old, m_t_int, m_t_new;

	// for parallel partition
	int NLblocks, ilower, iupper;
	int *n_dist;

protected:
	void setComponent(void); //init components;
	void setDomain();

	// parallelization related functions
	void scatMeshArray(void);
	void setGlobalIndex(void);
	void setIndexMap(void);

/*  These functions should be rewritten in 2D basis and 3D basis classes */
	virtual double getSmoothingFunction(double r) = 0; //Heaviside function
	virtual double getSmoothingFunctionD(double*, double*) = 0; 
		//Heaviside function
	virtual double smoothedDeltaFunction(double*, double*) = 0;
	virtual double smoothedStepFunction(double*, double*, int) = 0;
	virtual void   sampleVelocity() = 0;
	virtual void   setSmoothedProperties(void) = 0; 
		//smooth discontinuous properties
	
/****************  Functions related to solve() *********/

	virtual void copyMeshStates(void) = 0;
	virtual void computeAdvection(void) = 0;
	virtual void compDiffWithSmoothProperty_1st_coupled(void) = 0; 
	//1st order coupled diffusion solver
	virtual void compDiffWithSmoothProperty_1st_decoupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_coupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_decoupled(void) = 0;
        //2nd order decoupled diffusion solver
	virtual void computeProjection(void) = 0;
	virtual void computeProjectionCim(void) = 0;
	virtual void computeProjectionSimple(void) = 0;
	virtual void computePressure(void) = 0;
	virtual void computePressurePmI(void) = 0;
	virtual void computePressurePmII(void) = 0;
	virtual void computePressurePmIII(void) = 0;
	virtual void computeGradientQ(void) = 0;
	virtual void computeNewVelocity(void) = 0;
	virtual void computeSourceTerm(double *coords, L_STATE &state) = 0;
	virtual void computeSourceTerm(double *coords, double t, L_STATE 
		&state) = 0;
	virtual void surfaceTension(double*, HYPER_SURF_ELEMENT*,
		HYPER_SURF*, double*, double) = 0;

	
/***********************  Utility functions  *******************/

	void   computeExactSolution(double *coords, L_STATE &state);
	void   getRectangleIndex(int indexRectangle, int &i, int &j);
	void   getRectangleIndex(int indexRectangle, int &i, int &j, int &k);
	int    getRectangleComponent(int index);	// the center component
	void   getRectangleCenter(int index, double *coords);
	double getDistance(double *coords0, double *coords1);
	int    getComponent(int *icoords);	
	int    getComponent(double *coords);	
	void   save(char *filename);

/************* TMP Functions which are not implemented or used ***********/

	void computeSubgridModel(void);    // subgrid model by Hyunkyung Lim
	void getNearestInterfacePoint(COMPONENT,double*,double*,double*,
					double*); 
};

///////////////Interface for Embedded Boundary Method////////////////////

class Incompress_Solver_EBM:public Incompress_Solver_Basis{
public:
        Incompress_Solver_EBM(Front &front) {};//constructor
	~Incompress_Solver_EBM() {};

	virtual void setInitialCondition() {};
	virtual void solve(double dt) {};
};
/////////////////////////////////////////////////////////////////////////////////
//
class Incompress_Solver_Smooth_2D_Basis:
public 	Incompress_Solver_Smooth_Basis{
public:
        Incompress_Solver_Smooth_2D_Basis(Front &front):
	Incompress_Solver_Smooth_Basis(front) {};
	virtual ~Incompress_Solver_Smooth_2D_Basis() {};

	virtual void setInitialCondition(void) = 0;
	virtual void solve(double dt) = 0;
protected:
	double getSmoothingFunction(double r);
	double getSmoothingFunctionD(double*, double*);
	double smoothedDeltaFunction(double*, double*);
	double smoothedStepFunction(double*, double*, int);
	void sampleVelocity();
	void setSmoothedProperties(void);

	virtual void copyMeshStates(void) = 0;
	virtual void computeAdvection(void) = 0;
	virtual void compDiffWithSmoothProperty_1st_coupled(void) = 0; 
	//1st order coupled diffusion solver
	virtual void compDiffWithSmoothProperty_1st_decoupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_coupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_decoupled(void) = 0;
        //2nd order decoupled diffusion solver
	virtual void computeProjection(void) = 0;
	virtual void computeProjectionCim(void) = 0;
	virtual void computeProjectionSimple(void) = 0;
	virtual void computePressure(void) = 0;
	virtual void computePressurePmI(void) = 0;
	virtual void computePressurePmII(void) = 0;
	virtual void computePressurePmIII(void) = 0;
	virtual void computeGradientQ(void) = 0;
	virtual void computeNewVelocity(void) = 0;
	virtual void computeSourceTerm(double *coords, L_STATE &state) = 0;
	virtual void computeSourceTerm(double *coords, double t, L_STATE 
		&state) = 0;
	virtual void surfaceTension(double*, HYPER_SURF_ELEMENT*,
		HYPER_SURF*, double*, double) = 0;
};


class Incompress_Solver_Smooth_3D_Basis:
public 	Incompress_Solver_Smooth_Basis{
public:
        Incompress_Solver_Smooth_3D_Basis(Front &front):
	Incompress_Solver_Smooth_Basis(front) {};
	virtual ~Incompress_Solver_Smooth_3D_Basis() {};

	virtual void setInitialCondition(void) = 0;
	virtual void solve(double dt) = 0;
protected:
	double getSmoothingFunction(double r);
	double getSmoothingFunctionD(double*, double*);
	double smoothedDeltaFunction(double*, double*);
	double smoothedStepFunction(double*, double*, int);
	void sampleVelocity();
	void setSmoothedProperties(void);

	virtual void copyMeshStates(void) = 0;
	virtual void computeAdvection(void) = 0;
	virtual void compDiffWithSmoothProperty_1st_coupled(void) = 0; 
	//1st order coupled diffusion solver
	virtual void compDiffWithSmoothProperty_1st_decoupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_coupled(void) = 0;
        virtual void compDiffWithSmoothProperty_2nd_decoupled(void) = 0;
        //2nd order decoupled diffusion solver

	virtual void computeProjection(void) = 0;
	virtual void computeProjectionCim(void) = 0;
	virtual void computeProjectionSimple(void) = 0;
	virtual void computePressure(void) = 0;
	virtual void computePressurePmI(void) = 0;
	virtual void computePressurePmII(void) = 0;
	virtual void computePressurePmIII(void) = 0;
	virtual void computeGradientQ(void) = 0;
	virtual void computeNewVelocity(void) = 0;
	virtual void computeSourceTerm(double *coords, L_STATE &state) = 0;
	virtual void computeSourceTerm(double *coords, double t, L_STATE 
		&state) = 0;
	virtual void surfaceTension(double*, HYPER_SURF_ELEMENT*,
		HYPER_SURF*, double*, double) = 0;
};

class Incompress_Solver_Smooth_2D_Cartesian:
public 	Incompress_Solver_Smooth_2D_Basis{
public:
        Incompress_Solver_Smooth_2D_Cartesian(Front &front):
	Incompress_Solver_Smooth_2D_Basis(front) {};
	~Incompress_Solver_Smooth_2D_Cartesian() {};

	void setInitialCondition(void);
	void solve(double dt);
protected:
	void copyMeshStates(void);
	void computeAdvection(void);
	void compDiffWithSmoothProperty_1st_coupled(void); 
	//1st order coupled diffusion solver
	void compDiffWithSmoothProperty_1st_decoupled(void);
        void compDiffWithSmoothProperty_2nd_coupled(void);
        void compDiffWithSmoothProperty_2nd_decoupled(void);
        //2nd order decoupled diffusion solver
	void computeProjection(void);
	void computeProjectionCim(void);
	void computeProjectionSimple(void);
	void computePressure(void);
	void computePressurePmI(void);
	void computePressurePmII(void);
	void computePressurePmIII(void);
	void computeGradientQ(void);
	void computeNewVelocity(void);
	void computeSourceTerm(double *coords, L_STATE &state);
	void computeSourceTerm(double *coords, double t, L_STATE &state);
	void surfaceTension(double*, HYPER_SURF_ELEMENT*, HYPER_SURF*, double*, double);

	/***************   Low level computation functions  *************/
	double computeFieldPointDiv(int*, double**);
	void   computeFieldPointGrad(int*, double*, double*);
	double computeFieldPointCurl(int*, double**, double*);
	double getVorticity(int i, int j);
};

class Incompress_Solver_Smooth_3D_Cartesian:
public 	Incompress_Solver_Smooth_3D_Basis{
public:
        Incompress_Solver_Smooth_3D_Cartesian(Front &front):
	Incompress_Solver_Smooth_3D_Basis(front) {};
	~Incompress_Solver_Smooth_3D_Cartesian() {};

	void setInitialCondition(void);
	void solve(double dt);
protected:
	void copyMeshStates(void);
	void computeAdvection(void);
	void compDiffWithSmoothProperty_1st_coupled(void); 
	//1st order coupled diffusion solver
	void compDiffWithSmoothProperty_1st_decoupled(void);
        void compDiffWithSmoothProperty_2nd_coupled(void);
        void compDiffWithSmoothProperty_2nd_decoupled(void);
        //2nd order decoupled diffusion solver
	void computeProjection(void);
	void computeProjectionCim(void);
	void computeProjectionSimple(void);
	void computePressure(void);
	void computePressurePmI(void);
	void computePressurePmII(void);
	void computePressurePmIII(void);
	void computeGradientQ(void);
	void computeNewVelocity(void);
	void computeSourceTerm(double *coords, L_STATE &state);
	void computeSourceTerm(double *coords, double t, L_STATE &state);
	void surfaceTension(double*, HYPER_SURF_ELEMENT*, HYPER_SURF*, 
				double*, double);
	double computeFieldPointDiv(int*, double**);
	void   computeFieldPointGrad(int*, double*, double*);
	double computeFieldPointCurl(int*, double**, double*);
	double getVorticityX(int i, int j, int k);
	double getVorticityY(int i, int j, int k);
	double getVorticityZ(int i, int j, int k);
};

class Incompress_Solver_Smooth_3D_Cylindrical:
public Incompress_Solver_Smooth_3D_Basis{
public:Incompress_Solver_Smooth_3D_Cylindrical(Front &front):
Incompress_Solver_Smooth_3D_Basis(front) {};
	~Incompress_Solver_Smooth_3D_Cylindrical() {};

	void printInteriorVelocity(char *out_name);
	void setInitialCondition(void);
	void solve(double dt);
	void computeError(void);

protected:
	void copyMeshStates(void);
	void computeAdvection(void); 		//First order advection, 
						//operator splitting
	void computeAdvection_test(void); 	//Paper version, first order 
						//advection, operator splitting

	void compAdvectionTerm_coupled(void); //not implemented
	void compAdvectionTerm_coupled_upgraded(void) {}; //not implemented

	void compAdvectionTerm_decoupled(void);
	void compAdvectionTerm_decoupled_upgraded(void) {}; 
						//not implemented
	void compDiffWithSmoothProperty_1st_coupled(void) {};
						//not implemented
	void compDiffWithSmoothProperty_1st_decoupled(void);
	void compDiffWithSmoothProperty_1st_decoupled_test(void);
						//Paper version
	void compDiffWithSmoothProperty_1st_decoupled_source(void);
						//half Crank-Nicholson

	void compDiffWithSmoothProperty_2nd_coupled(void); //no implemented
	void compDiffWithSmoothProperty_2nd_decoupled(void); 
	//just adjusting coefficients, do not support complext B.C.

	//--------------------------------------------------
	//   utility functions for diffusion and projection 
	//--------------------------------------------------

	void compDiff_CellFace( //Set the coefficient for cell corners
		PETSc *pSolver,
		int I,
		int I_nb[18],
		double U_center[3],
		double U_nb[3][18],
		int flag[6],
		int equation_index,
		int vel_comp,
		int face_index,
		double coeff);


	void compDiff_CellCorner( //Set the coefficient for cell corners
		PETSc *pSolver,
		int I,
		int I_nb[18],
		double U_center[3],
		double U_nb[3][18],
		int flag[6],
		int equation_index,
		int vel_comp,
		int corner_index,
		double coeff);

	double getFaceArea(int *icoords, GRID_DIRECTION dir);
	double getCellVolume(int *icoords);
	void getFaceCenter(int *icoords, GRID_DIRECTION dir, 
				double faceCenter[3]);

	void compDiffWithSmoothProperty_2nd_decoupled_Shuqiang(void);
	void compDiffWithSmoothProperty_cellFace(
		PETSc *pSolver,
		int *icoords,
		int I, int I_nb,
		GRID_DIRECTION dir,
		double dh,
		double faceArea,
		double cellVolume,
		double r,
		double mu,
		double rho,
		L_STATE &U_nb,
		L_STATE &U_nb_new,
		L_STATE &U_center);
	
	void compDiffWithSmoothProperty_Dirichlet(
		PETSc *pSolver,
		int *icoords,
		int I, int I_nb,
		GRID_DIRECTION dir,
		double dh,
		double faceArea,
		double cellVolume,
		double r,
		double mu,
		double rho,
		L_STATE &U_nb,
		L_STATE &U_nb_new,
		L_STATE &U_center);

	void compDiffWithSmoothProperty_cellInterior(
		PETSc *pSolver,
		int *icoords,
		int I,
		double cellVolume,
		double r,
		double mu,
		double rho,
		L_STATE &U_center);


	void computeProjection_Shuqiang(void);

	//--------------------------------------------------------

	void computeProjection(void);
	void computeProjectionCim(void);
	void computeProjectionSimple(void);
	void computePressure(void);
	void computePressurePmI(void);
	void computePressurePmII(void);
	void computePressurePmIII(void);
	void computeGradientQ(void);
	void computeNewVelocity(void);
	virtual void computeSourceTerm(double *coords, L_STATE &state);
	virtual void computeSourceTerm(double *coords,double t,L_STATE &state);
	void surfaceTension(double*, HYPER_SURF_ELEMENT*, HYPER_SURF*, 
					double*, double);

	double computeFieldPointDiv(int*, double**);
	void   computeFieldPointGrad(int*, double*, double*);
	double computeFieldPointCurl(int*, double**, double*);
	double getVorticityX(int i, int j, int k);
	double getVorticityY(int i, int j, int k);
	double getVorticityZ(int i, int j, int k);

	//void   computeVelDivergence(void);
	//void getVelocityGradient(double *p,double *gradU,double *gradV);
	//-------------------------------------------------
	//	utility function for the advection step
	//-------------------------------------------------
	void getAdvectionTerm_decoupled(int *icoords, double convectionTerm[3]);
	void getAdvectionTerm_decoupled_upgraded(int *icoords, 
			double convectionTerm[3]) {};//not implemented yet

	void getAdvectionTerm_coupled(int *icoords, double convectionTerm[3]);
	void getAdvectionTerm_coupled_upgraded(int *icoords, 
			double convectionTerm[3]) {};
	//not implemented yet 
	
	void getFaceVelocity_middleStep(int *icoords,GRID_DIRECTION dir,
			L_STATE &state_face);
	void getFaceVelocity_middleStep_hat(int *icoords,GRID_DIRECTION dir, 
			L_STATE &state_hat) {}; //not implemented
	void getFaceVelocity_middleStep_bar(int *icoords,GRID_DIRECTION dir, 
			L_STATE &state_bar, double transverseD[3], 
			L_STATE state_hat) {}; //not implemented
 
	void getFaceVelocity_middleStep_coupled(int *icoords,
			GRID_DIRECTION dir,L_STATE &state_face);
	void getFaceVelocity_middleStep_coupled_hat(int *icoords,
			GRID_DIRECTION dir, L_STATE &state_hat) {}; 
			//not implemented
	void getFaceVelocity_middleStep_coupled_bar(int *icoords,
			GRID_DIRECTION dir, L_STATE &state_bar, 
			double transverseD[3], L_STATE state_hat) {}; 
			//not implemented


	void getDifffusion(int *icoords,double diffusion[3]);
	void getDiffusion_coupled(int *icoords, double diffusinon[3]);

	void getDU2(int *icoords,EBM_COORD xyz,double dU2[3], double dU[3]);
	void getLimitedSlope(int *icoords,EBM_COORD xzy,double slope[3]);
	void getLimitedSlope_Vanleer(int *icoords, EBM_COORD xyz, 
			double slope[3]);

	double EBM_minmod(double x, double y);

	bool getNeighborOrBoundaryState(int icoords[3],GRID_DIRECTION dir,
			L_STATE &state,double t);
	void getRiemannSolution(EBM_COORD xyz,L_STATE &u_left,
			L_STATE &state_right,L_STATE &ans);
};

extern double getStatePres(POINTER);
extern double getStateVort(POINTER);
extern double getStateXvel(POINTER);
extern double getStateYvel(POINTER);
extern double getStateZvel(POINTER);
extern double getStateXimp(POINTER);
extern double getStateYimp(POINTER);
extern double getStateZimp(POINTER);
extern double getStateXvort(POINTER);
extern double getStateYvort(POINTER);
extern double getStateZvort(POINTER);
extern double getStateComp(POINTER);
extern double burger_flux(double,double,double);
extern double linear_flux(double,double,double,double);
extern void fluid_print_front_states(FILE*,Front*);
extern void fluid_read_front_states(FILE*,Front*);
extern void read_iF_movie_options(char*,IF_PARAMS*);
extern void read_iF_dirichlet_bdry_data(char*,Front*,F_BASIC_DATA);
extern boolean isDirichletPresetBdry(Front*,int*,GRID_DIRECTION,COMPONENT);
extern int ifluid_find_state_at_crossing(Front*,int*,GRID_DIRECTION,
			int,POINTER*,HYPER_SURF**,double*);
extern int ifluid_find_projection_crossing(Front*,int*,GRID_DIRECTION,
			int,POINTER*,HYPER_SURF**,double*);
extern double p_jump(POINTER,int,double*);
extern double grad_p_jump_n(POINTER,int,double*,double*);
extern double grad_p_jump_t(POINTER,int,int,double*,double*);

#endif