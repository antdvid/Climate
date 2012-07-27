/*******************************************************************
 * 		C_CARTESIAN.c
 *******************************************************************/
/*
      To-Do: Line 392, the values of ilower and iupper 
      
*/

#include "crystal.h"
#include "crystal_basic.h"
#include<solver.h>


//----------------------------------------------------------------
//		C_RECTANGLE
//----------------------------------------------------------------

C_RECTANGLE::C_RECTANGLE(): index(-1), comp(-1)
{
}

void C_RECTANGLE::setCoords(
	double *crds,
	int dim)
{
	int i;
	for (i = 0; i < dim; ++i)
	    coords[i] = crds[i];
}
//--------------------------------------------------------------------------
// 		C_CARTESIAN
//--------------------------------------------------------------------------

C_CARTESIAN::~C_CARTESIAN()
{
}

//---------------------------------------------------------------
//	initMesh
// include the following parts
// 1) setup edges:		
// 2) setup cell_center
//---------------------------------------------------------------
void C_CARTESIAN::initMesh(void)
{
	int i,j,k, index;
	double crds[MAXD];
	int num_cells;
	C_RECTANGLE       rectangle;

	// init vertices,edges & cell_center
	if (debugging("trace")) printf("Entering initMesh()\n");
	FT_MakeGridIntfc(front);
	if (debugging("trace")) printf("Passed FT_MakeGridIntfc()\n");
	setDomain();
	if (debugging("trace")) printf("Passed setDomain()\n");

	num_cells = 1;
	for (i = 0; i < dim; ++i)
	{
	    num_cells *= (top_gmax[i] + 1);
	}
	cell_center.insert(cell_center.end(),num_cells,rectangle);
	
	// setup vertices
	// left to right, down to up
	switch (dim)
	{
	case 1:
	    for (j = 0; j <= top_gmax[1]; j++)
	    {
	    	crds[0] = top_L[0] + top_h[0]*i;
		index = d_index1d(i,top_gmax);
	    	cell_center[index].setCoords(crds,dim);
	    	cell_center[index].icoords[0] = i;
	    }
	case 2:
	    for (j = 0; j <= top_gmax[1]; j++)
	    for (i = 0; i <= top_gmax[0]; i++)
	    {
	    	crds[0] = top_L[0] + top_h[0]*i;
	    	crds[1] = top_L[1] + top_h[1]*j;
		index = d_index2d(i,j,top_gmax);
	    	cell_center[index].setCoords(crds,dim);
	    	cell_center[index].icoords[0] = i;
	    	cell_center[index].icoords[1] = j;
	    }
	    break;
	case 3:
	    for (k = 0; k <= top_gmax[2]; k++)
	    for (j = 0; j <= top_gmax[1]; j++)
	    for (i = 0; i <= top_gmax[0]; i++)
	    {
	    	crds[0] = top_L[0] + top_h[0]*i;
	    	crds[1] = top_L[1] + top_h[1]*j;
	    	crds[2] = top_L[2] + top_h[2]*k;
		index = d_index3d(i,j,k,top_gmax);
	    	cell_center[index].setCoords(crds,dim);
	    	cell_center[index].icoords[0] = i;
	    	cell_center[index].icoords[1] = j;
	    	cell_center[index].icoords[2] = k;
	    }
	}
	setComponent();
	FT_FreeGridIntfc(front);
	if (debugging("trace")) printf("Leaving initMesh()\n");
}

void C_CARTESIAN::setComponent(void)
{
	int i;
	static STATE    *state = NULL;
	double *coords;
	int size = (int)cell_center.size();
	
	// cell center components
	if(state == NULL)
            FT_ScalarMemoryAlloc((POINTER*)&state,sizeof(STATE));

	for (i = 0; i < size; i++)
	{
            coords = cell_center[i].coords;
	    if (cell_center[i].comp != -1 &&
                cell_center[i].comp != top_comp[i])
            {
		if (!FrontNearestIntfcState(front,coords,top_comp[i],
                                (POINTER)state))
                {
                    (void) printf("In setComponent()\n");
                    (void) printf("FrontNearestIntfcState() failed\n");
                    (void) printf("old_comp = %d new_comp = %d\n",
                                        cell_center[i].comp,top_comp[i]);
                    clean_up(ERROR);
                }
	    	field->solute[i] = state->solute;
	    }
	    cell_center[i].comp = top_comp[i];
	}
}

void C_CARTESIAN::setInitialCondition(void)
{
	int i,j;
	COMPONENT c;
	double rho_s = cRparams->rho_s;
	double coords[MAXD],distance;
	int size = (int)cell_center.size();
	double **point;
	double ans[MAXD],t[MAXD];
	HYPER_SURF_ELEMENT *hse;
	HYPER_SURF *hs;
	RECT_GRID *gr = front->rect_grid;
	double h;
	int range;

	/* Initialize states at the interface */
	FT_MakeGridIntfc(front);
	setDomain();

	h = gr->h[0];
	for (j = 0; j < dim; j++)
	    h = std::min(gr->h[j],h);
	range = (int)(2*cRparams->gap/h+1);
	// cell_center
	for (i = 0; i < size; i++)
	{
	    c = top_comp[i];

	    if (c == SOLUTE_COMP)
	    {
	    	getRectangleCenter(i,coords);

		if (dim == 1) 
		{
		    POINT **p,*pc;
		    for (p = front->interf->points; p && *p; ++p)
			if (wave_type(*p) == GROWING_BODY_BOUNDARY)
			{
			   pc = *p;
			   break;
			}
		    distance = coords[0] - Coords(pc)[0];
		}
		else
		{
		    if (FT_FindNearestIntfcPointInRange(front,c,coords,
				ans,t,&hse,&hs,range))
		    {
			distance = 0.0;
			for (j = 0; j < dim; j++)
			    distance += sqr(coords[j] - ans[j]);
		    	distance = sqrt(distance);
		    }
		}
		if (distance < cRparams->gap)
	    	    field->solute[i] = cRparams->C_eq;
		else
	    	    field->solute[i] = cRparams->C_0;

	    }
	    else if (c == CRYSTAL_COMP)
	    	field->solute[i] = rho_s;
	    else
	    	field->solute[i] = 0.0;
	}
}	/* end setInteriorStates */

void C_CARTESIAN::setIndexMap(void)
{
	static boolean first = YES;
	int i,j,k,ic,index;
	int llbuf[MAXD],uubuf[MAXD];

	if (debugging("trace")) printf("Entering setIndexMap()\n");
	if (first)
	{
	    first = NO;
	    switch (dim)
	    {
	    case 1:
	    	FT_VectorMemoryAlloc((POINTER*)&i_to_I,top_gmax[0]+1,INT);
	    	break;
	    case 2:
	    	FT_MatrixMemoryAlloc((POINTER*)&ij_to_I,top_gmax[0]+1,
					top_gmax[1]+1,INT);
	    	break;
	    case 3:
	    	FT_TriArrayMemoryAlloc((POINTER*)&ijk_to_I,top_gmax[0]+1,
					top_gmax[1]+1,top_gmax[2]+1,INT);
	    	break;
	    }
	}

	index = 0;
	for (i = 0; i < dim; ++i)
	{
	    llbuf[i] = lbuf[i] != 0 ? lbuf[i] : 1;
	    uubuf[i] = ubuf[i] != 0 ? ubuf[i] : 1;
	}
	switch (dim)
	{
	case 1:
	    for (i = imin; i <= imax; i++)
	    {
		ic = d_index1d(i,top_gmax);
		if (cell_center[ic].comp == SOLUTE_COMP)
		{
	    	    i_to_I[i] = index + ilower;
	    	    index++;
		}
		else
		    i_to_I[i] = -1;
	    }
	    FT_ParallelExchCellIndex(front,llbuf,uubuf,(POINTER)i_to_I);
	    break;
	case 2:
	    for (i = imin; i <= imax; i++)
	    for (j = jmin; j <= jmax; j++)
	    {
		ic = d_index2d(i,j,top_gmax);
		if (cell_center[ic].comp == SOLUTE_COMP)
		{
	    	    ij_to_I[i][j] = index + ilower;
	    	    index++;
		}
		else
		    ij_to_I[i][j] = -1;
	    }
	    FT_ParallelExchCellIndex(front,llbuf,uubuf,(POINTER)ij_to_I);
	    break;
	case 3:
	    for (i = imin; i <= imax; i++)
	    for (j = jmin; j <= jmax; j++)
	    for (k = kmin; k <= kmax; k++)
	    {
		ic = d_index3d(i,j,k,top_gmax);
		if (cell_center[ic].comp == SOLUTE_COMP)
		{
	    	    ijk_to_I[i][j][k] = index + ilower;
	    	    index++;
		}
		else
		    ijk_to_I[i][j][k] = -1;
	    }
	    FT_ParallelExchCellIndex(front,llbuf,uubuf,(POINTER)ijk_to_I);
	    break;
	}
	if (debugging("trace")) printf("Leaving setIndexMap()\n");

}	/* end setIndexMap */

void C_CARTESIAN::computeAdvection(void)
{
	if (debugging("trace")) printf("Entering computeAdvection()\n");
	switch (cRparams->num_scheme)
	{
	case UNSPLIT_EXPLICIT:
	    return computeAdvectionExplicit();
	case UNSPLIT_IMPLICIT:
	    return computeAdvectionImplicit();
	case CRANK_NICOLSON:
	    return computeAdvectionCN();
	}
}
    
void C_CARTESIAN::computeAdvectionCN(void)
{
	int i,j,k,l,m,ic,icn,I,I_nb,icoords[MAXD];
	int gmin[MAXD],ipn[MAXD];
	double crx_coords[MAXD];
	double C0,C_nb,D,rho_s,lambda,coeff,coeff_nb,rhs;
	COMPONENT comp;
	PETSc solver;
	double *x;
        int num_iter = 0;
	int size;
        double rel_residual = 0;
        boolean fr_crx_grid_seg;
        const GRID_DIRECTION dir[3][2] =
                {{WEST,EAST},{SOUTH,NORTH},{LOWER,UPPER}};

	D = cRparams->D;
	rho_s = cRparams->rho_s;

	if (m_dt < 0.1*sqr(hmin)/D/(double)dim)
	    return computeAdvectionExplicit();

	start_clock("computeAdvectionCN");
	if (debugging("trace")) printf("Entering computeAdvectionCN()\n");

	for (i = 0; i < dim; ++i) gmin[i] = 0;

	setIndexMap();
	size = iupper - ilower;
        
	start_clock("set_coefficients");
	switch(dim)
	{
	case 1:
	    solver.Create(ilower, iupper-1, 3, 3);
	    for (i = imin; i <= imax; ++i)
	    {
	    	icoords[0] = i;
	    	ic = d_index1d(i,top_gmax);
	    	comp = top_comp[ic];
	    	if (comp != SOLUTE_COMP) 
	    	{
		    array[ic] = rho_s;
	    	    continue;
	        }
		I = i_to_I[i];
                C0 = field->solute[ic];
		rhs = C0;
		coeff = 1.0;
		for (l = 0; l < dim; ++l)
		{
		    lambda = D*m_dt/sqr(top_h[l]);
                    for (m = 0; m < 2; ++m)
                    {
                        next_ip_in_dir(icoords,dir[l][m],ipn,gmin,top_gmax);
                        icn = d_index1d(ipn[0],top_gmax);
			I_nb = i_to_I[ipn[0]];
			coeff_nb = -0.5*lambda;
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &C_nb,crx_coords);
                        if (!fr_crx_grid_seg)
                        {
			    solver.Add_A(I,I_nb,coeff_nb);
			    C_nb = field->solute[icn];
			    rhs -= coeff_nb*(C_nb - C0);
		    	    coeff += 0.5*lambda;
                        }
			else
			{
			    rhs -= coeff_nb*2.0*C_nb;
		    	    coeff += lambda;
			}
                    }
		}
		solver.Add_A(I,I,coeff);
		solver.Add_b(I,rhs);
	    }
	    break;
	case 2:
	    solver.Create(ilower, iupper-1, 5, 5);
	    for (i = imin; i <= imax; ++i)
	    for (j = jmin; j <= jmax; ++j)
	    {
	    	icoords[0] = i;
	    	icoords[1] = j;
	    	ic = d_index2d(i,j,top_gmax);
	    	comp = top_comp[ic];
		I = ij_to_I[i][j];
	    	if (comp != SOLUTE_COMP) 
	    	{
		    array[ic] = rho_s;
	    	    continue;
	        }
                C0 = field->solute[ic];
		rhs = C0;
		coeff = 1.0;
		for (l = 0; l < dim; ++l)
		{
		    lambda = D*m_dt/sqr(top_h[l]);
                    for (m = 0; m < 2; ++m)
                    {
                        next_ip_in_dir(icoords,dir[l][m],ipn,gmin,top_gmax);
                        icn = d_index2d(ipn[0],ipn[1],top_gmax);
			I_nb = ij_to_I[ipn[0]][ipn[1]];
			coeff_nb = -0.5*lambda;
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &C_nb,crx_coords);
                        if (!fr_crx_grid_seg)
                        {
			    solver.Add_A(I,I_nb,coeff_nb);
			    C_nb = field->solute[icn];
			    rhs -= coeff_nb*(C_nb - C0);
			    coeff += 0.5*lambda;
                        }
			else
			{
			    rhs -= coeff_nb*2.0*C_nb;
			    coeff += lambda;
			}
                    }
		}
		solver.Add_A(I,I,coeff);
		solver.Add_b(I,rhs);
	    }
	    break;
	case 3:
	    solver.Create(ilower, iupper-1, 7, 7);
	    for (i = imin; i <= imax; ++i)
	    for (j = jmin; j <= jmax; ++j)
	    for (k = kmin; k <= kmax; ++k)
	    {
	    	icoords[0] = i;
	    	icoords[1] = j;
	    	icoords[2] = k;
	    	ic = d_index3d(i,j,k,top_gmax);
	    	comp = top_comp[ic];
		I = ijk_to_I[i][j][k];
	    	if (comp != SOLUTE_COMP) 
	    	{
		    array[ic] = rho_s;
	    	    continue;
	        }
                C0 = field->solute[ic];
		rhs = C0;
		coeff = 1.0;
		for (l = 0; l < dim; ++l)
		{
		    lambda = D*m_dt/sqr(top_h[l]);
                    for (m = 0; m < 2; ++m)
                    {
                        next_ip_in_dir(icoords,dir[l][m],ipn,gmin,top_gmax);
                        icn = d_index3d(ipn[0],ipn[1],ipn[2],top_gmax);
			I_nb = ijk_to_I[ipn[0]][ipn[1]][ipn[2]];
			coeff_nb = -0.5*lambda;
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &C_nb,crx_coords);
                        if (!fr_crx_grid_seg)
                        {
			    solver.Add_A(I,I_nb,coeff_nb);
			    C_nb = field->solute[icn];
			    rhs -= coeff_nb*(C_nb - C0);
			    coeff += 0.5*lambda;
                        }
			else
			{
			    rhs -= coeff_nb*2.0*C_nb;
			    coeff += lambda;
			}
                    }
		}
		solver.Add_A(I,I,coeff);
		solver.Add_b(I,rhs);
	    }
	    break;
	}
	stop_clock("set_coefficients");
	start_clock("petsc_solve");
	solver.SetMaxIter(500);   
	solver.SetTol(1e-8);   
	solver.Solve();

	if (debugging("PETSc"))
	{
	    (void) printf("C_CARTESIAN::computeAdvectionCN: "
	       		"num_iter = %d, rel_residual = %g \n", 
			num_iter, rel_residual);
	}

	FT_VectorMemoryAlloc((POINTER*)&x,size,sizeof(double));
	solver.Get_x(x);
	stop_clock("petsc_solve");

	start_clock("scatter_data");
	switch (dim)
	{
        case 1:
            for (i = imin; i <= imax; i++)
            {
	    	I = i_to_I[i];
	    	ic = d_index1d(i,top_gmax);
	    	comp = cell_center[ic].comp;
	    	if (comp == SOLUTE_COMP)
	    	    array[ic] = x[I-ilower];
	    	else
	    	    array[ic] = rho_s;
	    }
	    break;
        case 2:
            for (i = imin; i <= imax; i++)
            for (j = jmin; j <= jmax; j++)
            {
	    	I = ij_to_I[i][j];
	    	ic = d_index2d(i,j,top_gmax);
	    	comp = cell_center[ic].comp;
	    	if (comp == SOLUTE_COMP)
	    	    array[ic] = x[I-ilower];
	    	else
	    	    array[ic] = rho_s;
	    }
	    break;
        case 3:
            for (i = imin; i <= imax; i++)
            for (j = jmin; j <= jmax; j++)
            for (k = kmin; k <= kmax; k++)
            {
	    	I = ijk_to_I[i][j][k];
	    	ic = d_index3d(i,j,k,top_gmax);
	    	comp = cell_center[ic].comp;
	    	if (comp == SOLUTE_COMP)
	    	    array[ic] = x[I-ilower];
	    	else
	    	    array[ic] = rho_s;
	    }
	    break;
	}
	scatMeshArray();
	switch (dim)
	{
        case 1:
            for (i = 0; i <= top_gmax[0]; ++i)
            {
		ic = d_index1d(i,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
        case 2:
            for (i = 0; i <= top_gmax[0]; ++i)
            for (j = 0; j <= top_gmax[1]; ++j)
            {
		ic = d_index2d(i,j,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
        case 3:
            for (i = 0; i <= top_gmax[0]; ++i)
            for (j = 0; j <= top_gmax[1]; ++j)
            for (k = 0; k <= top_gmax[2]; ++k)
            {
		ic = d_index3d(i,j,k,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
	}
	stop_clock("scatter_data");
	FT_FreeThese(1,x);

	if (debugging("trace")) printf("Leaving computeAdvectionCN()\n");
	stop_clock("computeAdvectionCN");
}	/* end computeAdvectionCN */

void C_CARTESIAN::computeAdvectionImplicit(void)
{
	static PARABOLIC_SOLVER parab_solver(*front);

        if (m_dt < 0.1*sqr(hmin)/cRparams->D/(double)dim)
            return computeAdvectionExplicit();

	start_clock("computeAdvectionIM");
        setIndexMap();

	parab_solver.soln_comp = SOLUTE_COMP;
	parab_solver.obst_comp = CRYSTAL_COMP;
	parab_solver.var = field->solute;
	parab_solver.soln = field->solute;
	parab_solver.a = cRparams->field->vel;
	parab_solver.getStateVarFunc = getStateSolute;
	parab_solver.source = NULL;
	parab_solver.D = cRparams->D;
	parab_solver.order = cRparams->pde_order;
	parab_solver.var_obst = cRparams->rho_s;
	parab_solver.ilower = ilower;
	parab_solver.iupper = iupper;
	parab_solver.dt = m_dt;
	parab_solver.set_solver_domain();

	switch(dim)
	{
	case 1:
	    parab_solver.i_to_I = i_to_I;
	    break;
	case 2:
	    parab_solver.ij_to_I = ij_to_I;
	    break;
	case 3:
	    parab_solver.ijk_to_I = ijk_to_I;
	    break;
	}
	parab_solver.runge_kutta();
	stop_clock("computeAdvectionIM");

}	/* end computeAdvectionImplicit */

void C_CARTESIAN::solve(double dt)
{

	if (debugging("trace")) printf("Entering solve()\n");
	m_dt = dt;
	start_clock("solve");

	setDomain();
	if (debugging("trace")) printf("Passing setDomain()\n");

	setComponent();
	if (debugging("trace")) printf("Passing setComponent()\n");

	setGlobalIndex();
	if (debugging("trace")) printf("Passing setGlobalIndex()\n");

	computeAdvection();
	if (debugging("trace")) printf("Passing computeAdvection()\n");

	setAdvectionDt();
	if (debugging("trace")) printf("Passing setAdvectionDt()\n");

	setBoundary();
	if (debugging("trace")) printf("Passing setBoundary()\n");

	stop_clock("solve");
	if (debugging("trace")) printf("Leaving solve()\n");
}


void C_CARTESIAN::setAdvectionDt()
{
	double D,k;

	cRparams = (CRT_PARAMS*)front->extra2;
	D = cRparams->D;
	k = cRparams->k;

	if (cRparams->num_scheme == UNSPLIT_EXPLICIT)
	    max_dt = 0.5*sqr(hmin)/D/(double)dim;
	else
	    max_dt = 0.5*hmin/D/(double)dim;

	if (cRparams->point_prop_scheme == EXPLICIT_EULER)
            max_dt = std::min(max_dt,0.25*hmin/k);

	if (debugging("trace"))
	{
	    printf("In setAdvectionDt: max_dt = %24.18g\n",max_dt);
	}
}	/* end setAdvectionDt */

void C_CARTESIAN::getRectangleIndex(int index, int &i, int &j)
{
	i = cell_center[index].icoords[0];
	j = cell_center[index].icoords[1];
}

void C_CARTESIAN::getRectangleIndex(int index, int &i, int &j, int &k)
{
	i = cell_center[index].icoords[0];
	j = cell_center[index].icoords[1];
	k = cell_center[index].icoords[2];
}


void C_CARTESIAN::getRectangleCenter(
	int index, 
	double *coords)
{
	int i;
	for (i = 0; i < dim; ++i)
	    coords[i] = cell_center[index].coords[i];
}

void C_CARTESIAN::getRectangleCenter(
	int index0, 
	int index1, 
	double *coords)
{
	int i;
	for (i = 0; i < dim; ++i)
	{
	    coords[i] = 0.5*(cell_center[index0].coords[i] +
	    		     cell_center[index1].coords[i]);
	}
}

void C_CARTESIAN::save(char *filename)
{
	INTERFACE *intfc    = front->interf;
		
	FILE *hfile = fopen(filename, "w");
	if(hfile==NULL)
	{
		printf("\n can't open %s in "
		       "SaveAsTecplot_rect_grid_and_interface().", filename);
		exit(0);
	}
	
	// secondly print out the interface
		
	if(exists_interface(intfc))
	{
	    CURVE		**cur;
	    CURVE		*curve;
	    BOND		*bond;
			
	    for(cur=intfc->curves; cur && *cur; cur++)
	    {
		curve = *cur;
		fprintf(hfile, "ZONE I=%d J=%d F=POINT \n", 
				curve->num_points, 1);
		bond=curve->first;
		fprintf(hfile, "%.4f %.4f \n",bond->start->_coords[0], 
				bond->start->_coords[1]);
		for(bond=curve->first; bond!=NULL; bond=bond->next)
		    fprintf(hfile, "%.4f %.4f \n",bond->end->_coords[0], 
		    		bond->end->_coords[1]);
		}					
	}		
	fclose(hfile);
}

C_CARTESIAN::C_CARTESIAN(Front &front):front(&front)
{
}

void C_CARTESIAN::setDomain()
{
	static boolean first = YES;
	INTERFACE *grid_intfc;
	Table *T;
	int i;

	grid_intfc = front->grid_intfc;
	top_grid = &topological_grid(grid_intfc);
	lbuf = front->rect_grid->lbuf;
	ubuf = front->rect_grid->ubuf;
	top_gmax = top_grid->gmax;
	top_L = top_grid->L;
	top_U = top_grid->U;
	top_h = top_grid->h;
	dim = grid_intfc->dim;
	T = table_of_interface(grid_intfc);
	top_comp = T->components;
	cRparams = (CRT_PARAMS*)front->extra2;
	hmin = top_h[0];
	for (i = 1; i < dim; ++i)
	    if (hmin > top_h[i]) hmin = top_h[i];

	switch (dim)
	{
	case 1:
            if (first)
            {
		FT_ScalarMemoryAlloc((POINTER*)&field,sizeof(CRT_FIELD));
                FT_VectorMemoryAlloc((POINTER*)&array,top_gmax[0]+1,FLOAT);
                FT_VectorMemoryAlloc((POINTER*)&field->solute,top_gmax[0]+1,
						FLOAT);
                first = NO;
            }	
	    imin = (lbuf[0] == 0) ? 1 : lbuf[0];
	    imax = (ubuf[0] == 0) ? top_gmax[0] - 1 : top_gmax[0] - ubuf[0];
	    cRparams->field = field;
	    break;
	case 2:
	    if (first)
	    {
		FT_ScalarMemoryAlloc((POINTER*)&field,sizeof(CRT_FIELD));
	    	FT_VectorMemoryAlloc((POINTER*)&array,
				(top_gmax[0]+1)*(top_gmax[1]+1),FLOAT);
	    	FT_VectorMemoryAlloc((POINTER*)&field->solute,
				(top_gmax[0]+1)*(top_gmax[1]+1),FLOAT);
	    	first = NO;
	    }
	    imin = (lbuf[0] == 0) ? 1 : lbuf[0];
	    jmin = (lbuf[1] == 0) ? 1 : lbuf[1];
	    imax = (ubuf[0] == 0) ? top_gmax[0] - 1 : top_gmax[0] - ubuf[0];
	    jmax = (ubuf[1] == 0) ? top_gmax[1] - 1 : top_gmax[1] - ubuf[1];
	    cRparams->field = field;
	    break;
	case 3:
	    if (first)
	    {
		FT_ScalarMemoryAlloc((POINTER*)&field,sizeof(CRT_FIELD));
	    	FT_VectorMemoryAlloc((POINTER*)&array,
			(top_gmax[0]+1)*(top_gmax[1]+1)*(top_gmax[2]+1),FLOAT);
	    	FT_VectorMemoryAlloc((POINTER*)&field->solute,
			(top_gmax[0]+1)*(top_gmax[1]+1)*(top_gmax[2]+1),FLOAT);
	    	first = NO;
	    }
	    imin = (lbuf[0] == 0) ? 1 : lbuf[0];
	    jmin = (lbuf[1] == 0) ? 1 : lbuf[1];
	    kmin = (lbuf[2] == 0) ? 1 : lbuf[2];
	    imax = (ubuf[0] == 0) ? top_gmax[0] - 1 : top_gmax[0] - ubuf[0];
	    jmax = (ubuf[1] == 0) ? top_gmax[1] - 1 : top_gmax[1] - ubuf[1];
	    kmax = (ubuf[2] == 0) ? top_gmax[2] - 1 : top_gmax[2] - ubuf[2];
	    cRparams->field = field;
	    break;
	}
}	/* end setDomain */

void C_CARTESIAN::scatMeshArray()
{
	FT_ParallelExchGridArrayBuffer(array,front);
}

void C_CARTESIAN::setGlobalIndex()
{
	int i,j,k,ic;
	int num_nodes = pp_numnodes();
	int myid = pp_mynode();
	static boolean first = YES;

	if (first)
	{
	    first = NO;
	    FT_VectorMemoryAlloc((POINTER*)&n_dist,num_nodes,sizeof(int));
	}
	NLblocks = 0;
	switch (dim)
	{
	case 1:
	    for (i = imin; i <= imax; i++)
	    {
		ic = d_index1d(i,top_gmax);
		if (cell_center[ic].comp != SOLUTE_COMP) continue;
		NLblocks++;
	    }
	    break;
	case 2:
	    for (j = jmin; j <= jmax; j++)
	    for (i = imin; i <= imax; i++)
	    {
		ic = d_index2d(i,j,top_gmax);
		if (cell_center[ic].comp != SOLUTE_COMP) continue;
		NLblocks++;
	    }
	    break;
	case 3:
	    for (k = kmin; k <= kmax; k++)
	    for (j = jmin; j <= jmax; j++)
	    for (i = imin; i <= imax; i++)
	    {
		ic = d_index3d(i,j,k,top_gmax);
		if (cell_center[ic].comp != SOLUTE_COMP) continue;
		NLblocks++;
	    }
	    break;
	}

	for (i = 0; i < num_nodes; ++i) n_dist[i] = 0;
	n_dist[myid] = NLblocks;
	pp_global_imax(n_dist,num_nodes);
	ilower = 0;
        iupper = n_dist[0];

        for (i = 1; i <= myid; i++)
        {
            ilower += n_dist[i-1];
            iupper += n_dist[i];
        }	
}


void C_CARTESIAN::initMovieVariables()
{
	int n;
	static HDF_MOVIE_VAR *hdf_movie_var;
	CRT_MOVIE_OPTION *movie_option = cRparams->movie_option;

	if (hdf_movie_var == NULL)
	{
	    FT_ScalarMemoryAlloc((POINTER*)&hdf_movie_var,
				sizeof(HDF_MOVIE_VAR));
	    switch (dim)
	    {
	    case 1:
		hdf_movie_var->num_var = 1;
                FT_MatrixMemoryAlloc((POINTER*)&hdf_movie_var->var_name,1,
                                        100,sizeof(char));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->top_var,1,
                                        sizeof(double*));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->obstacle_comp,1,
                                        sizeof(COMPONENT));
		FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->preset_bound,1,
                                        sizeof(boolean));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_min,1,
                                        sizeof(double));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_max,1,
                                        sizeof(double));
                if (movie_option->plot_solute)
                {
                    sprintf(hdf_movie_var->var_name[0],"solute");
                    hdf_movie_var->get_state_var[0] = getStateSolute;
                    hdf_movie_var->top_var[0] = cRparams->field->solute;
                    hdf_movie_var->obstacle_comp[0] = CRYSTAL_COMP;
                }
		break;
	    case 2:
	    	hdf_movie_var->num_var = 1;
	    	FT_MatrixMemoryAlloc((POINTER*)&hdf_movie_var->var_name,1,
					100,sizeof(char));
	    	FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->top_var,1,
					sizeof(double*));
	    	FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->obstacle_comp,1,
					sizeof(COMPONENT));
		FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->preset_bound,1,
                                        sizeof(boolean));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_min,1,
                                        sizeof(double));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_max,1,
                                        sizeof(double));
		if (movie_option->plot_solute)
		{
	    	    sprintf(hdf_movie_var->var_name[0],"solute");
	    	    hdf_movie_var->get_state_var[0] = getStateSolute;
		    hdf_movie_var->top_var[0] = cRparams->field->solute;
		    hdf_movie_var->obstacle_comp[0] = CRYSTAL_COMP;
		}
		break;
	    case 3:
	    	hdf_movie_var->num_var = n = 0;
	    	FT_MatrixMemoryAlloc((POINTER*)&hdf_movie_var->var_name,3,100,
					sizeof(char));
	    	FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->top_var,3,
					sizeof(double*));
		FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->idir,3,
					sizeof(int));
	    	FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->obstacle_comp,
					3,sizeof(COMPONENT));
		FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->preset_bound,3,
                                        sizeof(boolean));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_min,3,
                                        sizeof(double));
                FT_VectorMemoryAlloc((POINTER*)&hdf_movie_var->var_max,3,
                                        sizeof(double));
		if (movie_option->plot_solute)
		{
		    if (movie_option->plot_cross_section[0])
		    {
	    	    	sprintf(hdf_movie_var->var_name[n],"solute-yz");
	    	    	hdf_movie_var->get_state_var[n] = getStateSolute;
		    	hdf_movie_var->top_var[n] = cRparams->field->solute;
			hdf_movie_var->idir[n] = 0;
		    	hdf_movie_var->obstacle_comp[n] = CRYSTAL_COMP;
			hdf_movie_var->num_var = ++n;
		    }
		    if (movie_option->plot_cross_section[1])
		    {
			sprintf(hdf_movie_var->var_name[n],"solute-xz");
			hdf_movie_var->get_state_var[n] = getStateSolute;
		    	hdf_movie_var->top_var[n] = cRparams->field->solute;
			hdf_movie_var->idir[n] = 1;
		    	hdf_movie_var->obstacle_comp[n] = CRYSTAL_COMP;
			hdf_movie_var->num_var = ++n;
		    }
		    if (movie_option->plot_cross_section[2])
		    {
	    	    	sprintf(hdf_movie_var->var_name[n],"solute-xy");
	    	    	hdf_movie_var->get_state_var[n] = getStateSolute;
		    	hdf_movie_var->top_var[n] = cRparams->field->solute;
			hdf_movie_var->idir[n] = 2;
		    	hdf_movie_var->obstacle_comp[n] = CRYSTAL_COMP;
			hdf_movie_var->num_var = ++n;
		    }
		}
	    }
	    front->hdf_movie_var = hdf_movie_var;
	}
}	/* end initMovieVariables */

void C_CARTESIAN::printFrontInteriorStates(char *out_name)
{
	int i,j,k,index;
	char filename[100];
	FILE *outfile;
	double *solute = field->solute;

	sprintf(filename,"%s/state.ts%s",out_name,
			right_flush(front->step,7));
#if defined(__MPI__)
	if (pp_numnodes() > 1)
            sprintf(filename,"%s-nd%s",filename,right_flush(pp_mynode(),4));
#endif /* defined(__MPI__) */
	sprintf(filename,"%s-solute",filename);
	outfile = fopen(filename,"w");
	
	solute_print_front_states(outfile,front);

	fprintf(outfile,"\nInterior solute states:\n");
	switch (dim)
	{
	case 1:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    {
		index = d_index1d(i,top_gmax);
	        fprintf(outfile,"%24.18g\n",solute[index]);
	    }
	    break;
	case 2:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    for (j = 0; j <= top_gmax[1]; ++j)
	    {
		index = d_index2d(i,j,top_gmax);
	        fprintf(outfile,"%24.18g\n",solute[index]);
	    }
	    break;
	case 3:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    for (j = 0; j <= top_gmax[1]; ++j)
	    for (k = 0; k <= top_gmax[2]; ++k)
	    {
		index = d_index3d(i,j,k,top_gmax);
	        fprintf(outfile,"%24.18g\n",solute[index]);
	    }
	    break;
	}
	fclose(outfile);
}

void C_CARTESIAN::readFrontInteriorStates(char *restart_name)
{
	FILE *infile;
	int i,j,k,index;
	char fname[100];
	double *solute = field->solute;

	sprintf(fname,"%s-solute",restart_name);
        infile = fopen(fname,"r");

        /* Initialize states in the interior regions */

	solute_read_front_states(infile,front);

	FT_MakeGridIntfc(front);
	setDomain();

	next_output_line_containing_string(infile,"Interior solute states:");

	switch (dim)
	{
	case 1:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    {
		index = d_index1d(i,top_gmax);
	    	fscanf(infile,"%lf",&solute[index]);
	    }
	    break;
	case 2:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    for (j = 0; j <= top_gmax[1]; ++j)
	    {
		index = d_index2d(i,j,top_gmax);
	    	fscanf(infile,"%lf",&solute[index]);
	    }
	    break;
	case 3:
	    for (i = 0; i <= top_gmax[0]; ++i)
	    for (j = 0; j <= top_gmax[1]; ++j)
	    for (k = 0; k <= top_gmax[2]; ++k)
	    {
		index = d_index3d(i,j,k,top_gmax);
	    	fscanf(infile,"%lf",&solute[index]);
	    }
	    break;
	}
	fclose(infile);
}	/* end readInteriorStates */

void C_CARTESIAN::setBoundary()
{
	int i,j,k,index0,index1;
	INTERFACE *intfc = front->interf;
	double *solute = field->solute;

	switch (dim)
	{
	case 1:
	    if (rect_boundary_type(intfc,0,0) == NEUMANN_BOUNDARY)
	    {
		index0 = d_index1d(0,top_gmax);
		index1 = d_index1d(1,top_gmax);
	    	solute[index0] = solute[index1];
	    }
	    if (rect_boundary_type(intfc,0,1) == NEUMANN_BOUNDARY)
	    {
		index0 = d_index1d(top_gmax[0],top_gmax);
		index1 = d_index1d(top_gmax[0]-1,top_gmax);
	    	solute[index0] = solute[index1];
	    }
	    break;
	case 2:
	    if (rect_boundary_type(intfc,0,0) == NEUMANN_BOUNDARY)
	    {
		for (j = 0; j <= top_gmax[1]; ++j)
		{
		    index0 = d_index2d(0,j,top_gmax);
		    index1 = d_index2d(1,j,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,0,1) == NEUMANN_BOUNDARY)
	    {
		for (j = 0; j <= top_gmax[1]; ++j)
		{
		    index0 = d_index2d(top_gmax[0],j,top_gmax);
		    index1 = d_index2d(top_gmax[0]-1,j,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,1,0) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		{
		    index0 = d_index2d(i,0,top_gmax);
		    index1 = d_index2d(i,1,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,1,1) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		{
		    index0 = d_index2d(i,top_gmax[1],top_gmax);
		    index1 = d_index2d(i,top_gmax[1]-1,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    break;
	case 3:
	    if (rect_boundary_type(intfc,0,0) == NEUMANN_BOUNDARY)
	    {
		for (j = 0; j <= top_gmax[1]; ++j)
		for (k = 0; k <= top_gmax[2]; ++k)
		{
		    index0 = d_index3d(0,j,k,top_gmax);
		    index1 = d_index3d(1,j,k,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,0,1) == NEUMANN_BOUNDARY)
	    {
		for (j = 0; j <= top_gmax[1]; ++j)
		for (k = 0; k <= top_gmax[2]; ++k)
		{
		    index0 = d_index3d(top_gmax[0],j,k,top_gmax);
		    index1 = d_index3d(top_gmax[0]-1,j,k,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,1,0) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		for (k = 0; k <= top_gmax[2]; ++k)
		{
		    index0 = d_index3d(i,0,k,top_gmax);
		    index1 = d_index3d(i,1,k,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,1,1) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		for (k = 0; k <= top_gmax[2]; ++k)
		{
		    index0 = d_index3d(i,top_gmax[1],k,top_gmax);
		    index1 = d_index3d(i,top_gmax[1]-1,k,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,2,0) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		for (j = 0; j <= top_gmax[1]; ++j)
		{
		    index0 = d_index3d(i,j,0,top_gmax);
		    index1 = d_index3d(i,j,1,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    if (rect_boundary_type(intfc,2,1) == NEUMANN_BOUNDARY)
	    {
		for (i = 0; i <= top_gmax[0]; ++i)
		for (j = 0; j <= top_gmax[1]; ++j)
		{
		    index0 = d_index3d(i,j,top_gmax[2],top_gmax);
		    index1 = d_index3d(i,j,top_gmax[2]-1,top_gmax);
	    	    solute[index0] = solute[index1];
		}
	    }
	    break;
	}
}	/* end setBoundary */

void C_CARTESIAN::computeAdvectionExplicit(void)
{
	int i,j,k,l,m,ic,icn,icoords[MAXD];
	int gmin[MAXD],ipn[MAXD];
	double crx_coords[MAXD];
	double solute,solute_nb[2],dgrad[MAXD];
	double rho_s,coef;
	COMPONENT comp;
	boolean fr_crx_grid_seg;
	const GRID_DIRECTION dir[3][2] = 
		{{WEST,EAST},{SOUTH,NORTH},{LOWER,UPPER}};

	start_clock("computeAdvectionExplicit");

	coef = cRparams->D*m_dt;
	rho_s = cRparams->rho_s;

	for (i = 0; i < dim; ++i) gmin[i] = 0;

	switch (dim)
	{
        case 1:
            for (i = imin; i <= imax; ++i)
            {
                icoords[0] = i;
                ic = d_index1d(i,top_gmax);
                comp = top_comp[ic];
                if (comp != SOLUTE_COMP)
                {
                     array[ic] = rho_s;
                     continue;
                }
                array[ic] = solute = field->solute[ic];
                for (l = 0; l < dim; ++l)
                {
                    dgrad[l] = 0.0;
                    for (m = 0; m < 2; ++m)
                    {
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &solute_nb[m],crx_coords);
                        if (!fr_crx_grid_seg)
                        {
                             next_ip_in_dir(icoords,dir[l][m],ipn,gmin,
							top_gmax);
                             icn = d_index1d(ipn[0],top_gmax);
			     solute_nb[m] = field->solute[icn];
                        }
                        dgrad[l] += (solute_nb[m] - solute)/top_h[l];
                    }
                    array[ic] += coef*dgrad[l]/top_h[l];
                }
            }
            break;
	case 2:
	    for (i = imin; i <= imax; ++i)
	    for (j = jmin; j <= jmax; ++j)
	    {
	    	icoords[0] = i;
	    	icoords[1] = j;
	    	ic = d_index2d(i,j,top_gmax);
	    	comp = top_comp[ic];
	    	if (comp != SOLUTE_COMP) 
	    	{
		    array[ic] = rho_s;
	    	    continue;
	        }
                array[ic] = solute = field->solute[ic];
		for (l = 0; l < dim; ++l)
		{
	            dgrad[l] = 0.0;
                    for (m = 0; m < 2; ++m)
                    {
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &solute_nb[m],crx_coords);
                        if (!fr_crx_grid_seg)
                        {
                            next_ip_in_dir(icoords,dir[l][m],ipn,gmin,top_gmax);
                            icn = d_index2d(ipn[0],ipn[1],top_gmax);
			    solute_nb[m] = field->solute[icn];
                        }
                        dgrad[l] += (solute_nb[m] - solute)/top_h[l];
                    }
		    array[ic] += coef*dgrad[l]/top_h[l];
		}
	    }
	    break;
	case 3:
	    for (i = imin; i <= imax; ++i)
	    for (j = jmin; j <= jmax; ++j)
	    for (k = kmin; k <= kmax; ++k)
	    {
	    	icoords[0] = i;
	    	icoords[1] = j;
	    	icoords[2] = k;
	    	ic = d_index3d(i,j,k,top_gmax);
	    	comp = top_comp[ic];
	    	if (comp != SOLUTE_COMP) 
	    	{
		    array[ic] = rho_s;
	    	    continue;
	        }
                array[ic] = solute = field->solute[ic];
		for (l = 0; l < dim; ++l)
		{
	            dgrad[l] = 0.0;
                    for (m = 0; m < 2; ++m)
                    {
                        fr_crx_grid_seg = FT_StateVarAtGridCrossing(front,
                                        icoords,dir[l][m],comp,getStateSolute,
                                        &solute_nb[m],crx_coords);
                        if (!fr_crx_grid_seg)
                        {
                            next_ip_in_dir(icoords,dir[l][m],ipn,gmin,top_gmax);
                            icn = d_index3d(ipn[0],ipn[1],ipn[2],top_gmax);
                	    solute_nb[m] = field->solute[icn];
                        }
                        dgrad[l] += (solute_nb[m] - solute)/top_h[l];
                    }
		    array[ic] += coef*dgrad[l]/top_h[l];
		}
	    }
	    break;
	}
	scatMeshArray();
	switch (dim)
	{
        case 1:
            for (i = 0; i <= top_gmax[0]; ++i)
            {
		ic = d_index1d(i,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
        case 2:
            for (i = 0; i <= top_gmax[0]; ++i)
            for (j = 0; j <= top_gmax[1]; ++j)
            {
		ic = d_index2d(i,j,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
        case 3:
            for (i = 0; i <= top_gmax[0]; ++i)
            for (j = 0; j <= top_gmax[1]; ++j)
            for (k = 0; k <= top_gmax[2]; ++k)
            {
		ic = d_index3d(i,j,k,top_gmax);
		field->solute[ic] = array[ic];
	    }
	    break;
	}

	stop_clock("computeAdvectionExplicit");
}	/* computeAdvectionExplicit */

void C_CARTESIAN::oneDimPlot(char *outname)
{
	xgraphOneDimPlot(outname);
}	/* end solutePlot */

void C_CARTESIAN::xgraphOneDimPlot(char *outname)
{
	int i,index;
	char filename[100];
	FILE *outfile;

	if (debugging("trace"))
	    printf("Entering xgraphSolute1()\n");
        sprintf(filename,"%s/solute-xg.ts%s",
			outname,right_flush(front->step,7));
#if defined(__MPI__)
        sprintf(filename,"%s-nd%s",filename,right_flush(pp_mynode(),4));
#endif /* defined(__MPI__) */
        outfile = fopen(filename,"w");
	fprintf(outfile,"\"OP at %6.3f\"\n",front->time);
	for (i = 0; i <= top_gmax[0]; ++i)
	{
	    index = d_index1d(i,top_gmax);
	    fprintf(outfile,"%24.18g  %24.18g\n",
	    		cell_center[index].coords[0],
	    		field->solute[index]);
	}
	fclose(outfile);
	if (debugging("trace"))
	    printf("Leaving xgraphSolute1()\n");
}	/* end xgraphOneDimPlot */

void read_crt_movie_options(
        char *inname,
        CRT_PARAMS *cRparams)
{
        static CRT_MOVIE_OPTION *movie_option;
        FILE *infile = fopen(inname,"r");
        char string[100];

        FT_ScalarMemoryAlloc((POINTER*)&movie_option,sizeof(CRT_MOVIE_OPTION));
        cRparams->movie_option = movie_option;

        if (cRparams->dim == 1) 
	{
	    movie_option->plot_solute = YES;
	    return;
	}

        CursorAfterString(infile,"Type y to make movie of solute:");
        fscanf(infile,"%s",string);
	(void) printf("%s\n",string);
        if (string[0] == 'Y' || string[0] == 'y')
            movie_option->plot_solute = YES;

        if (cRparams->dim == 3)
        {
            CursorAfterString(infile,"Type y to make yz cross section movie:");
            fscanf(infile,"%s",string);
	    (void) printf("%s\n",string);
            if (string[0] == 'Y' || string[0] == 'y')
                movie_option->plot_cross_section[0] = YES;
            CursorAfterString(infile,"Type y to make xz cross section movie:");
            fscanf(infile,"%s",string);
	    (void) printf("%s\n",string);
            if (string[0] == 'Y' || string[0] == 'y')
                movie_option->plot_cross_section[1] = YES;
            CursorAfterString(infile,"Type y to make xy cross section movie:");
            fscanf(infile,"%s",string);
	    (void) printf("%s\n",string);
            if (string[0] == 'Y' || string[0] == 'y')
                movie_option->plot_cross_section[2] = YES;
        }
        fclose(infile);
}       /* end read_crt_movie_options */