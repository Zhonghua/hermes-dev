#include "weakform/weakform.h"
#include "integrals/h1.h"
#include "boundaryconditions/essential_bcs.h"
#include "weakform_library/h1.h"

using namespace WeakFormsH1;

/* Right-hand side */

class CustomRightHandSide: public DefaultFunction
{
public:
  CustomRightHandSide(double alpha) : DefaultFunction(), alpha(alpha) {};

  virtual double value(double x, double y) const {
    return - alpha * (alpha - 1.) * pow(x, alpha - 2.);
  }

  virtual Ord ord(Ord x, Ord y) const {
    return Ord((int)(alpha + 3.1));
  }

  double alpha;
};

/* Exact solution */

class CustomExactSolution : public ExactSolutionScalar
{
public:
  CustomExactSolution(Mesh* mesh, double alpha)
        : ExactSolutionScalar(mesh), alpha(alpha) {};

  virtual scalar value(double x, double y) const {
    return pow(x, alpha);
  };

  virtual void derivatives (double x, double y, scalar& dx, scalar& dy) const {
    dx = alpha * pow(x, alpha - 1.);
    dy = 0;
  };

  virtual Ord ord(Ord x, Ord y) const {
    return Ord((int)(alpha + 1));
  }

  double alpha;
};


