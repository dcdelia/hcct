# ======================================================================
#  do-for-all-traces-max-phi.pl
# ======================================================================

#  Author(s)       (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
#  License:        See the end of this file for license information
#  Created:        Nov 6, 2010

#  Last changed:   $Date: 2011/03/07 09:29:47 $
#  Changed by:     $Author: demetres $
#  Revision:       $Revision: 1.1.1.1 $

#  Usage: > perl do-for-all-traces-max-phi.pl driver
# ======================================================================


# include
require "experiments.pl";

# grab command line parameters
$DRIVER = $ARGV[0];

# run experiment
&DO_FOR_ALL_TRACES_MAX_PHI($DRIVER);


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
