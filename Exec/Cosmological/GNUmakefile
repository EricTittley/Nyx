# BOXLIB_HOME defines the directory in which we will find all the BoxLib code
BOXLIB_HOME = $(HOME)/Num/Nyx/src/BoxLib

# TOP defines the directory in which we will find Source, Exec, etc
TOP = ../..

# compilation options
COMP    = gcc
FCOMP   = gfortran
USE_OMP = FALSE
USE_MPI = TRUE

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
USE_EOS_PERFECT_FLUID = TRUE
#USE_EOS_PERFECT_FLUID = FALSE

Bpack := ./Make.package
Blocs := .

include $(TOP)/Exec/Make.Nyx
