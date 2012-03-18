# ======================================================================
#  do-for-all-traces.pl
# ======================================================================

#  Author(s)       (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
#  License:        See the end of this file for license information
#  Created:        Nov 6, 2010

#  Last changed:   $Date: 2010/11/09 22:37:08 $
#  Changed by:     $Author: demetres $
#  Revision:       $Revision: 1.2 $

#  Usage: > perl do-for-all-traces.pl driver \
#                [phi epsilon tau sampling burst]
# ======================================================================


# include
require "experiments.pl";

# grab command line parameters
$DRIVER     = $ARGV[0];
$PHI        = $ARGV[1];
$EPSILON    = $ARGV[2];
$TAU        = $ARGV[3];
$SAMPLING   = $ARGV[4];
$BURST      = $ARGV[5];

# run experiment
&DO_FOR_ALL_TRACES($DRIVER, $PHI, $EPSILON, $TAU, $SAMPLING, $BURST);


# ======================================================================
# Copyright (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
# USA
# ======================================================================
