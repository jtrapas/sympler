/*
 * This file is part of the SYMPLER package.
 * https://github.com/kauzlari/sympler
 *
 * Copyright 2002-2013, 
 * David Kauzlaric <david.kauzlaric@frias.uni-freiburg.de>,
 * and others authors stated in the AUTHORS file in the top-level 
 * source directory.
 *
 * SYMPLER is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SYMPLER is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SYMPLER.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Please cite the research papers on SYMPLER in your own publications. 
 * Check out the PUBLICATIONS file in the top-level source directory.
 *
 * You are very welcome to contribute extensions to the code. Please do 
 * so by making a pull request on https://github.com/kauzlari/sympler
 * 
 */


#include "gen_f.h"
#include "phase.h"
#include "threads.h"
#include "particle.h"
#include "controller.h"
#include "simulation.h"
#include "integrator_tensor_lambda.h"

using namespace std;


#define M_CONTROLLER  ((Controller*) m_parent)
#define M_SIMULATION  ((Simulation*) M_CONTROLLER->parent())
#define M_PHASE  M_SIMULATION->phase()


const Integrator_Register<IntegratorTensorLambda> integrator_tensor_lambda("IntegratorTensorLambda");

//---- Constructors/Destructor ----

IntegratorTensorLambda::IntegratorTensorLambda(Controller *controller): IntegratorTensor(controller), m_laterStep(false)
{
  init();
}


IntegratorTensorLambda::~IntegratorTensorLambda()
{
}



//---- Methods ----

void IntegratorTensorLambda::init()
{
//   MSG_DEBUG("IntegratorScalar::init()", "running");

  m_properties.setClassName("IntegratorTensorLambda");

  m_properties.setDescription(
      "Adds an additional tensor degree of freedom, "
      "to the particles specified and integrates it with a second order "
      "accurate predictor-corrector scheme."
      "\nPredictor step: predicted = old + lambda*dt*f_new"
      "\nCorrector step: new = predicted + (0.5-lambda)*dt*f_old + 0.5*dt*f_new"
      "\nUsually, the forces are updated between the two steps. So f_new from " 
      "the predictor step is equal to f_old from the corrector step."
      "\nNote that, currently, after the first predictor step, the corrector " 
      "step is merged into the further predictor steps giving:"
      "\nLater predictor step: new = old + (0.5-lambda)*dt*f_old + " 
      "(0.5+lambda)*dt*f_new"
      "\nLater corrector step: does nothing"
      
                             );

  DOUBLEPC
      (lambda,
       m_lambda,
       0,
       "Lambda parameter for adjustement of predictor-corrector scheme.");

  m_lambda = 0.5;
}

void IntegratorTensorLambda::setup()
{
  IntegratorTensor::setup();
  
  m_laterStep = false;
  
  m_lambda_diff = 0.5 - m_lambda;
  m_lambda_sum = 0.5 + m_lambda;
}


void IntegratorTensorLambda::integrateStep1()
{
  Phase *phase = M_PHASE;

  //  MSG_DEBUG("IntegratorScalar::integrateStep1", "m_dt = " << m_dt);

  size_t force_index = M_CONTROLLER->forceIndex();
  size_t other_force_index = (force_index+1)&(FORCE_HIST_SIZE-1);

  
/*  FOR_EACH_FREE_PARTICLE_C__PARALLEL
      (phase, m_colour, this,
       size_t j = 3; size_t k = 2;
       cout << (i->tag.tensorByOffset((((IntegratorTensorLambda*) data)->m_force_offset)[0]))(3, 2) << endl;
      );*/
  
  if(m_laterStep)
  {
//     MSG_DEBUG("IntegratorTensorLambda::integrateStep1", "later step");
    FOR_EACH_FREE_PARTICLE_C__PARALLEL
        (phase, m_colour, this,
         
         for(size_t j = 0; j < SPACE_DIMS; ++j)
         {
           for(size_t k = 0; k < SPACE_DIMS; ++k)
         {  
         // Debugging
           if (std::isnan(i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
               -> m_force_offset[force_index])(j, k))) 
           {
             cout << "slot = " << i->mySlot << ", "
                 << ((IntegratorTensorLambda*) data)->m_tensor_name << " = " 
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) 
                 << ", "
                 << "new force = "
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
                 -> m_force_offset[force_index])(j, k)
                 << endl;
  
             throw gError("IntegratorTensorLambda::integrateStep1", "Force was not-a-number!");
           }
  
           if (std::isnan(i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
               -> m_force_offset[other_force_index])(j, k))) 
           {
             cout << "slot = " << i->mySlot << ", "
                 << ((IntegratorTensorLambda*) data)->m_tensor_name << " = " 
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) 
                 << ", "
                 << "old force = "
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
                 -> m_force_offset[other_force_index])(j, k)
                 << endl;
  
             throw gError("IntegratorTensorLambda::integrateStep1", "Force was not-a-number!");
           }
  
          // Integration
           i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) += 
               ((IntegratorTensorLambda*) data)->m_dt *
               (
               (m_lambda_diff)*i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
               -> m_force_offset[other_force_index])(j, k) + 
               (m_lambda_sum)*i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
               -> m_force_offset[force_index])(j, k)
               );

/*          MSG_DEBUG("IntegratorTensorLambda::integrateStep1", "old_force" << other_force_index << " = " << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
           -> m_force_offset[other_force_index]) << ", new_force" << force_index << " = " << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
           -> m_force_offset[force_index]));*/
          
                    
          // Debugging
//            if (i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) < 0) 
//            {
//              cout << "slot = " << i->mySlot << ", "
//                  << ((IntegratorTensorLambda*) data)->m_tensor_name << " = " 
//                  << i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) 
//                  << ", "
//                  << "new force = "
//                  << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
//                  -> m_force_offset[force_index])(j, k)
//                  << endl
//                  << "old force = "
//                  << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
//                  -> m_force_offset[other_force_index])(j, k)
//                  << endl;

//            i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset) = 0.1;
       
//        // added next line to check when this happens
// //             throw gError("IntegratorTensorLambda::integrateStep1", "tensor negative !!!");
// 		   }
         }
  }
        );
  }
  else
  {
//     MSG_DEBUG("IntegratorTensorLambda::integrateStep1", "first step");
    
    FOR_EACH_FREE_PARTICLE_C__PARALLEL
        (phase, m_colour, this,
          
         for(size_t j = 0; j < SPACE_DIMS; ++j)
         {
           for(size_t k = 0; k < SPACE_DIMS; ++k)
         {
         // Debugging
           if (std::isnan(i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
               -> m_force_offset[force_index])(j, k))) 
           {
             cout << "slot = " << i->mySlot << ", "
                 << ((IntegratorTensorLambda*) data)->m_tensor_name << " = " 
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) 
                 << ", "
                 << "new force = "
                 << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
                 -> m_force_offset[force_index])(j, k)
                 << endl;
  
             throw gError("IntegratorTensorLambda::integrateStep1(first step)", "Force was " 
                 "not-a-number!");
           }
         
          // Integration 
           i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) += 
               ((IntegratorTensorLambda*) data)->m_dt *
               (
               m_lambda*i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_force_offset[force_index])(j, k)
               );

//          MSG_DEBUG("IntegratorTensorLambda::integrateStep1", "old_force" << other_force_index << " = " << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
//              -> m_force_offset[other_force_index]) << ", new_force" << force_index << " = " << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
//                  -> m_force_offset[force_index]));

                   
          // Debugging
//            if (i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) < 0) 
//            {
//              cout << "slot = " << i->mySlot << ", "
//                  << ((IntegratorTensorLambda*) data)->m_tensor_name << " = " 
//                  << i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset)(j, k) << ", "
//                  << "force = "
//                  << i->tag.tensorByOffset(((IntegratorTensorLambda*) data) 
//                  -> m_force_offset[force_index])(j, k)
//                  << endl;
// 
// //            i->tag.tensorByOffset(((IntegratorTensorLambda*) data)->m_tensor_offset) = 0.1;
//        
//        // added next line to check when this happens
//              throw gError("IntegratorTensorLambda::integrateStep1(first step)",
//                           "tensor negative !!!");
//            }
         }
         }
        );
        m_laterStep = true;
  }
}


void IntegratorTensorLambda::integrateStep2()
{
}
