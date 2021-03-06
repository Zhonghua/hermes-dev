#include "weakform/weakform.h"
#include "integrals/h1.h"
#include "boundaryconditions/essential_bcs.h"
#include "weakform_library/h1.h"

using namespace WeakFormsH1;

/* Right-hand side */

class CustomRightHandSide: public DefaultFunction
{
public:
  CustomRightHandSide(double poly_deg)
    : DefaultFunction(), poly_deg(poly_deg) {};

  virtual double value(double x, double y) const {
    double a = pow(2.0, 4.0*poly_deg);
    double b = pow(x-1.0, 8.0);
    double c = (38.0*pow(x, 2.0) - 38.0*x + 9.0);
    double d = pow(y-1.0, poly_deg);
    double e = pow(y-1.0, 8.0);
    double f = (38.0*pow(y, 2.0) - 38.0*y + 9.0);
    double g = pow(x-1.0, poly_deg);

    return -(poly_deg*a*pow(x, 8.0)*b*c*pow(y, poly_deg)*d
       + poly_deg*a*pow(y, 8.0)*e*f*pow(x, poly_deg)*g);
  }

  virtual Ord ord(Ord x, Ord y) const {
    return Ord(std::max(8, (int)(poly_deg + 0.51)));
  }

  double poly_deg;
};

/* Exact solution */

class CustomExactSolution : public ExactSolutionScalar
{
public:
  CustomExactSolution(Mesh* mesh, double poly_deg)
            : ExactSolutionScalar(mesh), poly_deg(poly_deg) {};

  double value(double x, double y) const {
    return pow(2, 4 * poly_deg) * pow(x, poly_deg) * pow(1 - x, poly_deg)
           * pow(y, poly_deg) * pow(1 - y, poly_deg);
  }

  virtual void derivatives (double x, double y, scalar& dx, scalar& dy) const {
    double A = pow((1.0-y), poly_deg);
    double B = pow((1.0-x), poly_deg);
    double C = pow(y, poly_deg);
    double D = pow(x, poly_deg);

    dx = ((poly_deg * pow(16.0, poly_deg)*A*C) / (x-1.0)
         + (poly_deg*pow(16.0, poly_deg)*A*C)/x)*B*D;
    dy = ((poly_deg*pow(16.0, poly_deg)*B*D)/(y-1.0)
         + (poly_deg*pow(16.0, poly_deg)*B*D)/y)*A*C;
  };

  virtual Ord ord(Ord x, Ord y) const {
    return Ord(poly_deg);
  }

  double poly_deg;
};

