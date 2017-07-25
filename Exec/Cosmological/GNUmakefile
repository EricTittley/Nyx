# BOXLIB_HOME defines the directory in which we will find all the BoxLib code
AMREX_HOME = $(PWD)/../../../amrex

# TOP defines the directory in which we will find Source, Exec, etc
TOP = ../..

# compilation options
USE_GCC = TRUE
COMP    = gcc
FCOMP   = gfortran
USE_OMP = TRUE
#USE_OMP = FALSE
#USE_MPI = TRUE
#USE_MPI = FALSE
# Turns on features specific to MPI-3.
# I can't get it to compile. Requires -std=c++11 which breaks things if GCC < 6
#USE_MPI3 = TRUE

PRECISION = DOUBLE
DEBUG     = FALSE
#DEBUG     = TRUE

# Use single precision for particles?
#
# If you set this to be TRUE be sure to do a "make realclean"
# to make sure all your code is consistent.
#
USE_SINGLE_PRECISION_PARTICLES = FALSE

# physics
DIM      = 3
USE_GRAV = TRUE

# Analysis
#USE_IN_SITU = TRUE
USE_IN_SITU = FALSE

# Equation of State.  Default is Perfect Gas
#USE_EOS_PERFECT_FLUID = TRUE
USE_EOS_PERFECT_FLUID = FALSE

# Compilation fails when InitRandom and InitRandomPerBox are called in
# NyxParticles.cpp
DEFINES += -DDISABLE_INIT_RANDOM

Bpack := ./Make.package
Blocs := .

MACHINE = Linux
DISTRIBUTION = Ubuntu
include $(TOP)/Exec/Make.Nyx
