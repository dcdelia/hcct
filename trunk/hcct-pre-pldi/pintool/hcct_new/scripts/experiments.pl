# ======================================================================
#  experiments.pl
# ======================================================================

#  Author(s)       (c) 2010 D.C. D'Elia, C. Demetrescu, I. Finocchi
#  License:        See the end of this file for license information
#  Created:        Nov 6, 2010

#  Last changed:   $Date: 2010/11/14 17:12:19 $
#  Changed by:     $Author: demetres $
#  Revision:       $Revision: 1.10 $

#  Usage: require "experiments.pl"
# ======================================================================


# include
require "config.pl";
require "define-traces.pl";


# ======================================================================
#  DO_FOR_ALL_TRACES
# ======================================================================
# run driver for each trace
sub DO_FOR_ALL_TRACES {

    # grab subroutine parameters
    $DRIVER   = $_[0];
    $PHI      = $_[1];
    $EPSILON  = $_[2];
    $TAU      = $_[3];
    $SAMPLING = $_[4];
    $BURST    = $_[5];

    # iterate for all traces
    for ($i = 0; $i < scalar(@TRACES); $i += 2) {
        $cmd = $BIN_DIR.$DRIVER." ".
               $TRACE_DIR.$TRACES[$i]." ".
               $TRACES[$i+1]." ".
               $LOG_DIR.$DRIVER."-".$PHI."-".$EPSILON.".log ".
               "-phi ".$PHI." ".
               "-epsilon ".$EPSILON." ".
               "-tau ".$TAU." ".
               "-sampling ".$SAMPLING." ".
               "-burst ".$BURST." ";
        print $cmd."\n";
        system $cmd;
    }
}


# ======================================================================
#  DO_FOR_ALL_TRACES_ALL_PHI
# ======================================================================
# run driver for each trace
sub DO_FOR_ALL_TRACES_ALL_PHI {

    # grab subroutine parameters
    $DRIVER   = $_[0];
    $TAU      = $_[3];
    $SAMPLING = $_[4];
    $BURST    = $_[5];

    # iterate for all traces
    for ($i = 0; $i < scalar(@TRACES); $i += 2) {
        for ($PHI = 256; $PHI < (1 << 18); $PHI <<= 1) {
            $EPSILON = $PHI * 5;
            $cmd = $BIN_DIR.$DRIVER." ".
                   $TRACE_DIR.$TRACES[$i]." ".
                   $TRACES[$i+1]." ".
                   $LOG_DIR.$DRIVER."-".$TRACES[$i+1]."-phi.log ".
                   "-phi ".$PHI." ".
                   "-epsilon ".$EPSILON." ".
                   "-tau ".$TAU." ".
                   "-sampling ".$SAMPLING." ".
                   "-burst ".$BURST." ";
            print $cmd."\n";
            system $cmd;
        }
    }
}


# ======================================================================
#  DO_FOR_ALL_PHI
# ======================================================================
# run driver for each phi on a given trace
sub DO_FOR_ALL_PHI {

    # grab subroutine parameters
    $DRIVER     = $_[0];
    $TRACE      = $_[1];
    $SAMPLING   = $_[2];
    $BURST      = $_[3];

    # iterate for all traces
    for ($i = 0; $i < scalar(@TRACES); $i += 2) {
        if ($TRACES[$i+1] eq $TRACE ) {
            $TRACE_FILE = $TRACE_DIR.$TRACES[$i];
            break;
        }
    }

    if ($TRACE_FILE eq "") {
        print "*** error: unknown trace file ".$TRACE."\n";
    }
    else {
        for ($PHI = 2048; $PHI < (1 << 25); $PHI <<= 1) {
            $EPSILON = $PHI * 5;
            $cmd = $BIN_DIR.$DRIVER." ".
                   $TRACE_FILE." ".
                   $TRACE." ".
                   $LOG_DIR.$DRIVER."-".$TRACE."-phi.log ".
                   "-phi ".$PHI." ".
                   "-epsilon ".$EPSILON." ".
                   "-sampling ".$SAMPLING." ".
                   "-burst ".$BURST." ";
            print $cmd."\n";
            system $cmd;
        }
    }
}


# ======================================================================
#  DO_FOR_ALL_EPSILON
# ======================================================================
# run driver for each epsilon on a given trace
sub DO_FOR_ALL_EPSILON {

    # grab subroutine parameters
    $DRIVER     = $_[0];
    $TRACE      = $_[1];
    $SAMPLING   = $_[2];
    $BURST      = $_[3];

    # iterate for all traces
    for ($i = 0; $i < scalar(@TRACES); $i += 2) {
        if ($TRACES[$i+1] eq $TRACE ) {
            $TRACE_FILE = $TRACE_DIR.$TRACES[$i];
            break;
        }
    }

    if ($TRACE_FILE eq "") {
        print "*** error: unknown trace file ".$TRACE."\n";
    }
    else {
        $PHI = 10000;
        for ($DELTA = 0; $DELTA <= 140000; $DELTA = $DELTA + 10000) {
            $EPSILON = $PHI + $DELTA;
            $cmd = $BIN_DIR.$DRIVER." ".
                   $TRACE_FILE." ".
                   $TRACE." ".
                   $LOG_DIR.$DRIVER."-".$TRACE."-eps.log ".
                   "-phi ".$PHI." ".
                   "-epsilon ".$EPSILON." ".
                   "-sampling ".$SAMPLING." ".
                   "-burst ".$BURST." ";
            print $cmd."\n";
            system $cmd;
        }
    }
}


# ======================================================================
#  DO_FOR_ALL_TRACES_MAX_PHI
# ======================================================================
# run driver for each trace
sub DO_FOR_ALL_TRACES_MAX_PHI {

    # grab subroutine parameters
    $DRIVER   = $_[0];

    # iterate for all traces
    for ($i = 0; $i < scalar(@TRACES); $i += 2) {
        $cmd = $BIN_DIR.$DRIVER." ".
               $TRACE_DIR.$TRACES[$i]." ".
               $TRACES[$i+1]." ".
               $LOG_DIR.$DRIVER."-".$TRACES[$i+1].".log";
        print $cmd."\n";
        system $cmd;
    }
}



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
