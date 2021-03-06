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


#include "val_calculator_kernel.h"
#include "simulation.h"
#include "manager_cell.h"
#include "colour_pair.h"

const SymbolRegister<ValCalculatorKernel> val_calc_kernel("ValCalculatorKernel");

#define M_SIMULATION  ((Simulation*) m_parent)
#define M_PHASE  M_SIMULATION->phase()
#define M_MANAGER  M_PHASE->manager()
#define M_CONTROLLER M_SIMULATION->controller()

using namespace std;

ValCalculatorKernel::ValCalculatorKernel(WeightingFunction *wf, string symbol)
  : ValCalculatorPair(symbol), m_wf(wf)
{
//   MSG_DEBUG("ValCalculatorKernel::ValCalculatorKernel", "CONSTRUCTOR");
}

ValCalculatorKernel::ValCalculatorKernel(/*Node*/Simulation* parent)
  : ValCalculatorPair(parent)
{
  m_stage = 0;
  init();
}

void ValCalculatorKernel::init()
{
  m_properties.setClassName("ValCalculatorKernel");

  m_properties.setDescription("Saves the evaluation of the interpolation function (the kernel) in each pair of particles.");
  
  STRINGPC
      (symbol, m_symbolName,
       "Name of the symbol for the interpolation value.");
  
  STRINGPC
      (weightingFunction, m_wfName,
       "The weighting function to be used.");
  
  m_wfName = "default";
  m_symbolName = "W";

#ifdef _OPENMP
  m_particleCalculator = false;
#endif
}

void ValCalculatorKernel::setup()
{
  m_wf = M_SIMULATION->findWeightingFunction(m_wfName);
  
//  ColourPair* cp = M_MANAGER->cp(M_MANAGER->getColour(m_species.first), M_MANAGER->getColour(m_species.second)/*m_species*/);

  ValCalculatorPair::setup();
  
}

void /*pair<size_t, size_t>*/ ValCalculatorKernel::setSlot(ColourPair* cp, size_t& slot, bool oneProp)
{
  m_slot = slot = cp->tagFormat().addAttribute
      ("ValCalculator_" + myName() + "_" + cp->toString(), DataFormat::DOUBLE, false, "W").offset;
}

