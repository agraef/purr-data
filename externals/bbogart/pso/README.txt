This is the readme for "PSO" a Particle Swarm Optimizer object for PD/MAX. 

This implimentation of a PSO minimizes towards a user-defined target. It is based on code from Jim Kennedy and Thomas Grill.

PSO is Copyright Ben Bogart 2003,2007,2008

If you have any questions/comments you can reach the author at ben@ekran.org

This program is distributed under the terms of the GNU General Public 
License 

PSO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or 
(at your option) any later version.

PSO is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. 

You should have received a copy of the GNU General Public License
along with PSO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

USAGE:

Follow the README.build.txt file for compilation instructions.

MAX: Put it in your externals folder.
PD:  Put it in your extra folder.

The PSO object requires three arguments (see below). The object has one inlet and three outlets. 
In order to calculate one iteration of the PSO send it a bang. The three outlets are:

Outlet 1: A index prepended list of the positions in each dimention of each particle.
Outlet 2: The Minimization value (should move towards 0)
Outlet 3: Outputs a bang when Minimization is reached. (This is a user defined threshold, not 0)

Arguments: [Population Size] [Number of Dimensions] [Neighborhood Size]

PSO Help Method:
  'help'                        - This message.

PSO Setup Methods:
  'target [list]'               - Specify the optimization target with list of values for each dimension.
  'rand_pop [min] [max]'        - Randomize the population with values ranging from min to max.
  'set_pop [list]'              - Set population with list of values for each particle for each dimension.
  'set_part [particle] [list]'  - Set particle with values for each dimension.
  'optim_thresh [int]'          - Set the threshold at which minimization is considered reached. (0-10)

PSO Run-Time Methods:
  'reset'                       - Reset the PSO.
  bang                          - Calculate one interation of the PSO.

Have Fun.
