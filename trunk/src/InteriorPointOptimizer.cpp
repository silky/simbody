

/* Portions copyright (c) 2006 Stanford University and Jack Middleton.
 * Contributors:
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "InteriorPointOptimizer.h"

using std::cout;
using std::endl;


namespace SimTK {


InteriorPointOptimizer::InteriorPointOptimizer( OptimizerSystem& sys )
        : OptimizerRep( sys ) {

          int i,n,m;

          Index index_style = 0; /* C-style; start counting of rows and column indices at 0 */


         if( sys.getNumParameters() < 1 ) {
             char *where = " InteriorPointOptimizer Initialization";
             char *szName= "dimension";
             SimTK_THROW5(SimTK::Exception::ValueOutOfRange, szName, 1,  sys.getNumParameters(), INT_MAX, where); 
         } else {
            n = sys.getNumParameters();
            /* set the bounds on the equality constraint functions */
            m = sys.getNumConstraints();
            g_U = (double *)malloc(sizeof(double)*m); 
            g_L = (double *)malloc(sizeof(double)*m);

            if( sys.getHasLimits() ) {
               
               sys.getParameterLimits( &x_L, &x_U);
               freeLimits = false;
            } else {
               x_U = new Real[n];
               x_L = new Real[n];
               for(i=0;i<n;i++) {
                  x_U[i] = POSITIVE_INF;
                  x_L[i] = NEGATIVE_INF;
               }
               freeLimits = true;
            }
            for(i=0;i<sys.getNumEqualityConstraints();i++){
                g_U[i] = g_L[i] = 0.0;
            }

            Index nele_hess = 0;
            Index nele_jac = n*m; /* always assume dense
            
            /* set the bounds on the inequality constraint functions */
            for(i=sys.getNumEqualityConstraints();i<m;i++){
                g_U[i] = POSITIVE_INF;
                g_L[i] = 0.0;
            }

            mult_x_L = (Number*)malloc(sizeof(Number)*n);
            mult_x_U = (Number*)malloc(sizeof(Number)*n);
        

            nlp = CreateIpoptProblem(n, x_L, x_U, m, g_L, g_U, nele_jac, 
                  nele_hess, index_style, objectiveFuncWrapper, constraintFuncWrapper, 
                  gradientFuncWrapper, constraintJacobianWrapper, hessianWrapper);


          }
     } 


     double InteriorPointOptimizer::optimize(  Vector &results ) {

         double obj;
         double *x = &results[0];
         int status;
         char buf[1024];
         double bound_relax_factor = 1e-7;

         AddIpoptNumOption(nlp, "bound_relax_factor", bound_relax_factor);
    //     AddIpoptNumOption(nlp, "obj_scaling_factor", 0.3);

         AddIpoptNumOption(nlp, "tol", convergenceTolerance);
         AddIpoptStrOption(nlp, "mu_strategy", "adaptive");
         AddIpoptStrOption(nlp, "output_file", "ipopt.out");
         AddIpoptStrOption(nlp, "linear_solver", "lapack");
         AddIpoptStrOption(nlp, "hessian_approximation", "limited-memory");
         AddIpoptIntOption(nlp, "print_level", diagnosticsLevel); // default is 4

         status = IpoptSolve(nlp, x, NULL, &obj, NULL, mult_x_L, mult_x_U, (void *)this );

         if (status != Solve_Succeeded) {
            sprintf(buf, "Ipopt failed with status = %d",status );
            SimTK_THROW1(SimTK::Exception::OptimizerFailed, SimTK::String(buf));
         }

         return(obj);
      }


} // namespace SimTK
