#define HERMES_REPORT_ALL
#include "hermes2d.h"

using namespace RefinementSelectors;

//  This test makes sure that the example "neutronics/iron-water" works correctly.
//  It is only executed if WITH_EXODUSII is set to YES

const int INIT_REF_NUM = 0;                       // Number of initial uniform mesh refinements.
const int P_INIT = 1;                             // Initial polynomial degree of all mesh elements.
const double THRESHOLD = 0.6;                     // This is a quantitative parameter of the adapt(...) function and
                                                  // it has different meanings for various adaptive strategies (see below).
const int STRATEGY = 0;                           // Adaptive strategy:
                                                  // STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
                                                  //   error is processed. If more elements have similar errors, refine
                                                  //   all to keep the mesh symmetric.
                                                  // STRATEGY = 1 ... refine all elements whose error is larger
                                                  //   than THRESHOLD times maximum element error.
                                                  // STRATEGY = 2 ... refine all elements whose error is larger
                                                  //   than THRESHOLD.
                                                  // More adaptive strategies can be created in adapt_ortho_h1.cpp.
const CandList CAND_LIST = H2D_HP_ANISO;          // Predefined list of element refinement candidates. Possible values are
                                                  // H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
                                                  // H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
                                                  // See User Documentation for details.
const int MESH_REGULARITY = -1;                   // Maximum allowed level of hanging nodes:
                                                  // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                                  // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                                  // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                                  // Note that regular meshes are not supported, this is due to
                                                  // their notoriously bad performance.
const double ERR_STOP = 1.0;                      // Stopping criterion for adaptivity (rel. error tolerance between the
                                                  // reference mesh and coarse mesh solution in percent).
const double CONV_EXP = 1.0;                      // Default value is 1.0. This parameter influences the selection of
                                                  // cancidates in hp-adaptivity. See get_optimal_refinement() for details.
const int NDOF_STOP = 60000;                      // Adaptivity process stops when the number of degrees of freedom grows
                                                  // over this limit. This is to prevent h-adaptivity to go on forever.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Problem parameters.
double L = 30;                           // Edge of square.
double L0 = 0.75*0.5*L;                  // End of first water layer.
double L1 = 0.5*L;                       // End of second water layer.
double L2 = 0.75*L;                      // End of iron layer.
double Q_EXT = 1.0;                      // Neutron source (nonzero in domain 1 only).
double SIGMA_T_WATER = 3.33;             // Total cross-section.
double SIGMA_T_IRON = 1.33;
double C_WATER = 0.994;                  // Scattering ratio.
double C_IRON = 0.831;
double D_WATER = 1./(3.*SIGMA_T_WATER);  // Diffusion coefficient.
double D_IRON = 1./(3.*SIGMA_T_IRON);
double SIGMA_A_WATER = SIGMA_T_WATER - C_WATER*SIGMA_T_WATER;  // Absorbing cross-section.
double SIGMA_A_IRON = SIGMA_T_IRON - C_IRON*SIGMA_T_IRON;

// Materials.
// Mesh file generated by Cubit - do not convert markers to strings.
const std::string WATER_1 = "1";
const std::string WATER_2 = "2";
const std::string IRON = "3";

// Boundaries.
const std::string ZERO_FLUX_BOUNDARY = "2";

// Weak forms.
#include "weakform_library/neutronics.h"

int main(int argc, char* argv[])
{
  // Instantiate a class with global functions.
  Hermes2D hermes2d;

  // Load the mesh.
  Mesh mesh;
  ExodusIIReader mloader;
  if (!mloader.load("../iron-water.e", &mesh)) error("ExodusII mesh load failed.");

  // Perform initial uniform mesh refinement.
  for (int i = 0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();
  
  // Set essential boundary conditions.
  DefaultEssentialBCConst bc_essential(ZERO_FLUX_BOUNDARY, 0.0);
  EssentialBCs bcs(&bc_essential);
  
  // Create an H1 space with default shapeset.
  H1Space space(&mesh, &bcs, P_INIT);
  
  // Associate element markers (corresponding to physical regions) 
  // with material properties (diffusion coefficient, absorption 
  // cross-section, external sources).
  Hermes::vector<std::string> regions(WATER_1, WATER_2, IRON);
  Hermes::vector<double> D_map(D_WATER, D_WATER, D_IRON);
  Hermes::vector<double> Sigma_a_map(SIGMA_A_WATER, SIGMA_A_WATER, SIGMA_A_IRON);
  Hermes::vector<double> Sources_map(Q_EXT, 0.0, 0.0);
  
  // Initialize the weak formulation.
  WeakFormsNeutronDiffusion::DefaultWeakFormSimpleMonoenergetic 
    wf(regions, D_map, Sigma_a_map, Sources_map);

  // Initialize coarse and reference mesh solution.
  Solution sln, ref_sln;
  
  // Initialize refinement selector.
  H1ProjBasedSelector selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);
  
  // DOF and CPU convergence graphs initialization.
  SimpleGraph graph_dof, graph_cpu;
  
  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Adaptivity loop:
  int as = 1; bool done = false;
  do
  {
    info("---- Adaptivity step %d:", as);

    // Construct globally refined reference mesh and setup reference space.
    Space* ref_space = Space::construct_refined_space(&space);

    int ndof_ref = Space::get_num_dofs(ref_space);

    // Initialize the FE problem.
    DiscreteProblem dp(&wf, ref_space);

    // Initialize the FE problem.
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

    // Initial coefficient vector for the Newton's method.  
    scalar* coeff_vec = new scalar[ndof_ref];
    memset(coeff_vec, 0, ndof_ref*sizeof(scalar));

    // Perform Newton's iteration on reference emesh.
    info("Solving on reference mesh.");
    if (!hermes2d.solve_newton(coeff_vec, &dp, solver, matrix, rhs)) error("Newton's iteration failed.");

    // Translate the resulting coefficient vector into the Solution sln.
    Solution::vector_to_solution(coeff_vec, ref_space, &ref_sln);

    // Project the fine mesh solution onto the coarse mesh.
    info("Projecting reference solution on coarse mesh.");
    OGProjection::project_global(&space, &ref_sln, &sln, matrix_solver); 

    // Calculate element errors and total error estimate.
    info("Calculating error estimate."); 
    Adapt* adaptivity = new Adapt(&space);
    double err_est_rel = adaptivity->calc_err_est(&sln, &ref_sln) * 100;

    // Report results.
    info("ndof_coarse: %d, ndof_fine: %d, err_est_rel: %g%%", 
	 Space::get_num_dofs(&space), Space::get_num_dofs(ref_space), err_est_rel);

    // Time measurement.
    cpu_time.tick();

    // Add entry to DOF and CPU convergence graphs.
    graph_dof.add_values(Space::get_num_dofs(&space), err_est_rel);
    graph_dof.save("conv_dof_est.dat");
    graph_cpu.add_values(cpu_time.accumulated(), err_est_rel);
    graph_cpu.save("conv_cpu_est.dat");

    // If err_est too large, adapt the mesh.
    if (err_est_rel < ERR_STOP) done = true;
    else 
    {
      info("Adapting coarse mesh.");
      done = adaptivity->adapt(&selector, THRESHOLD, STRATEGY, MESH_REGULARITY);
      
      // Increase the counter of performed adaptivity steps.
      if (done == false)  as++;
    }
    if (Space::get_num_dofs(&space) >= NDOF_STOP) done = true;

    // Clean up.
    delete [] coeff_vec;
    delete solver;
    delete matrix;
    delete rhs;
    delete adaptivity;
    if(done == false) delete ref_space->get_mesh();
    delete ref_space;
  }
  while (done == false);
  
  verbose("Total running time: %g s", cpu_time.accumulated());

  int ndof = Space::get_num_dofs(&space);

#define ERROR_SUCCESS                               0
#define ERROR_FAILURE                               -1
  int n_dof_allowed = 170;
  printf("n_dof_actual = %d\n", ndof);
  printf("n_dof_allowed = %d\n", n_dof_allowed);// ndofs was 158 at the time this test was created
  if (ndof <= n_dof_allowed) {
    printf("Success!\n");
    return ERROR_SUCCESS;
  }
  else {
    printf("Failure!\n");
    return ERROR_FAILURE;
  }
}
