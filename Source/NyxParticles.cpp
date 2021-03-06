#include <iomanip>
#include <Nyx.H>

#ifdef GRAVITY
#include <Gravity.H>
#include <Gravity_F.H> //needed for get_grav_constant, but there might be a better way
#endif

#include <Nyx_F.H>
#include <AMReX_Particles_F.H>

using namespace amrex;

namespace
{
    bool virtual_particles_set = false;
    
    std::string ascii_particle_file;
    std::string binary_particle_file;
    std::string    sph_particle_file;

#ifdef AGN
    std::string agn_particle_file;
#endif

#ifdef NEUTRINO_PARTICLES
    std::string neutrino_particle_file;
#endif

    const std::string chk_particle_file("DM");

    //
    // We want to call this routine when on exit to clean up particles.
    //

    //
    // Array of containers for all active particles
    //
    Array<NyxParticleContainerBase*> ActiveParticles;
    //
    // Array of containers for all virtual particles
    //
    Array<NyxParticleContainerBase*> VirtualParticles;
    //
    // Array of containers for all ghost particles
    //
    Array<NyxParticleContainerBase*> GhostParticles;

    //
    // Containers for the real "active" Particles
    //
    DarkMatterParticleContainer* DMPC = 0;
    StellarParticleContainer*     SPC = 0;
#ifdef AGN
    AGNParticleContainer*         APC = 0;
#endif
#ifdef NEUTRINO_PARTICLES
    NeutrinoParticleContainer*    NPC = 0;
#endif

    //
    // This is only used as a temporary container for
    //  reading in SPH particles and using them to
    //  initialize the density and velocity field on the
    //  grids.
    //
    DarkMatterParticleContainer*        SPHPC = 0;
    //
    // Container for temporary, virtual Particles
    //
    DarkMatterParticleContainer* VirtPC  = 0;
    StellarParticleContainer*    VirtSPC = 0;
#ifdef AGN
    AGNParticleContainer*        VirtAPC = 0;
#endif
#ifdef NEUTRINO_PARTICLES
    NeutrinoParticleContainer*   VirtNPC = 0;
#endif
    //
    // Container for temporary, ghost Particles
    //
    DarkMatterParticleContainer* GhostPC  = 0;
    StellarParticleContainer*    GhostSPC = 0;
#ifdef AGN
    AGNParticleContainer*        GhostAPC = 0;
#endif
#ifdef NEUTRINO_PARTICLES
    NeutrinoParticleContainer*   GhostNPC = 0;
#endif

    void RemoveParticlesOnExit ()
    {
        for (int i = 0; i < ActiveParticles.size(); i++)
        {
            delete ActiveParticles[i];
            ActiveParticles[i] = 0;
        }
        for (int i = 0; i < GhostParticles.size(); i++)
        {
            delete GhostParticles[i];
            GhostParticles[i] = 0;
        }
        for (int i = 0; i < VirtualParticles.size(); i++)
        {
            delete VirtualParticles[i];
            VirtualParticles[i] = 0;
        }
    }
}

bool Nyx::do_dm_particles = false;

std::string Nyx::particle_init_type = "";
std::string Nyx::particle_move_type = "";

// Allows us to output particles in the plotfile
//   in either single (IEEE32) or double (NATIVE) precision.  
// Particles are always written in double precision
//   in the checkpoint files.

bool Nyx::particle_initrandom_serialize = false;
Real Nyx::particle_initrandom_mass;
long Nyx::particle_initrandom_count;
long Nyx::particle_initrandom_count_per_box;
int  Nyx::particle_initrandom_iseed;

int Nyx::particle_verbose               = 1;
int Nyx::write_particles_in_plotfile    = 1;
int Nyx::write_particle_density_at_init = 0;
int Nyx::write_coarsened_particles      = 0;
Real Nyx::particle_cfl = 0.5;
#ifdef NEUTRINO_PARTICLES
Real Nyx::neutrino_cfl = 0.5;
#endif

IntVect Nyx::Nrep;

Array<NyxParticleContainerBase*>&
Nyx::theActiveParticles ()
{
    return ActiveParticles;
}

Array<NyxParticleContainerBase*>&
Nyx::theGhostParticles ()
{
    return GhostParticles;
}

Array<NyxParticleContainerBase*>&
Nyx::theVirtualParticles ()
{
    return VirtualParticles;
}

DarkMatterParticleContainer*
Nyx::theDMPC ()
{
    return DMPC;
}

DarkMatterParticleContainer*
Nyx::theVirtPC ()
{
    return VirtPC;
}
DarkMatterParticleContainer*
Nyx::theGhostPC ()
{
    return GhostPC;
}

StellarParticleContainer* 
Nyx::theSPC ()
{
      return SPC;
}
StellarParticleContainer* 
Nyx::theVirtSPC ()
{
      return VirtSPC;
}
StellarParticleContainer* 
Nyx::theGhostSPC ()
{
      return GhostSPC;
}

#ifdef AGN
AGNParticleContainer* 
Nyx::theAPC ()
{
      return APC;
}
AGNParticleContainer* 
Nyx::theVirtAPC ()
{
      return VirtAPC;
}
AGNParticleContainer* 
Nyx::theGhostAPC ()
{
      return GhostAPC;
}
#endif

#ifdef NEUTRINO_PARTICLES
NeutrinoParticleContainer* 
Nyx::theNPC ()
{
      return NPC;
}
NeutrinoParticleContainer* 
Nyx::theVirtNPC ()
{
      return VirtNPC;
}
NeutrinoParticleContainer* 
Nyx::theGhostNPC ()
{
      return GhostNPC;
}
#endif

void
Nyx::read_particle_params ()
{
    ParmParse pp("nyx");
    pp.query("do_dm_particles", do_dm_particles);

#ifdef AGN
    pp.get("particle_init_type", particle_init_type);
    pp.get("particle_move_type", particle_move_type);
#else
    if (do_dm_particles)
    {
        pp.get("particle_init_type", particle_init_type);
        pp.get("particle_move_type", particle_move_type);
        pp.query("init_with_sph_particles", init_with_sph_particles);
    }
#endif

#ifdef GRAVITY
    if (!do_grav && particle_move_type == "Gravitational")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR:: doesnt make sense to have do_grav=false but move_type = Gravitational" << std::endl;
        amrex::Error();
    }
#endif

    pp.query("particle_initrandom_serialize", particle_initrandom_serialize);
    pp.query("particle_initrandom_count", particle_initrandom_count);
    pp.query("particle_initrandom_count_per_box", particle_initrandom_count_per_box);
    pp.query("particle_initrandom_mass", particle_initrandom_mass);
    pp.query("particle_initrandom_iseed", particle_initrandom_iseed);

    pp.query("ascii_particle_file", ascii_particle_file);

    // Input error check
    if (do_dm_particles && !ascii_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified ascii_particle_file" << std::endl;;
        amrex::Error();
    }

    pp.query("sph_particle_file", sph_particle_file);

    // Input error check
    if (init_with_sph_particles != 1 && !sph_particle_file.empty())
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::init_with_sph_particles is not 1 but you specified sph_particle_file" << std::endl;;
        amrex::Error();
    }

    // Input error check
    if (init_with_sph_particles == 1 && sph_particle_file.empty())
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::init_with_sph_particles is 1 but you did not specify sph_particle_file" << std::endl;;
        amrex::Error();
    }

    pp.query("binary_particle_file", binary_particle_file);

    // Input error check
    if (!binary_particle_file.empty() && (particle_init_type != "BinaryFile" &&
                                          particle_init_type != "BinaryMetaFile"))
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not BinaryFile or BinaryMetaFile but you specified binary_particle_file" << std::endl;
        amrex::Error();
    }

#ifdef AGN
    pp.query("agn_particle_file", agn_particle_file);
    if (!agn_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified agn_particle_file" << std::endl;;
        amrex::Error();
    }
#endif

#ifdef NEUTRINO_PARTICLES
    pp.query("neutrino_particle_file", neutrino_particle_file);
    if (!neutrino_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified neutrino_particle_file" << std::endl;;
        amrex::Error();
    }
#endif

    pp.query("write_particle_density_at_init", write_particle_density_at_init);
    pp.query("write_coarsened_particles", write_coarsened_particles);
    //
    // Control the verbosity of the Particle class
    //
    ParmParse ppp("particles");
    ppp.query("v", particle_verbose);
    ppp.query("write_in_plotfile", write_particles_in_plotfile);

    for (int i = 0; i < BL_SPACEDIM; i++) Nrep[i] = 1; // Initialize to one (no replication)
    ppp.query("replicate",Nrep);
    //
    // Set the cfl for particle motion (fraction of cell that a particle can
    // move in a timestep).
    //
    ppp.query("cfl", particle_cfl);
#ifdef NEUTRINO_PARTICLES
    ppp.query("neutrino_cfl", neutrino_cfl);
#endif
}

void
Nyx::init_particles ()
{
    BL_PROFILE("Nyx::init_particles()");

    if (level > 0)
        return;

    //
    // Need to initialize particles before defining gravity.
    //
    if (do_dm_particles)
    {
        BL_ASSERT (DMPC == 0);
        DMPC = new DarkMatterParticleContainer(parent);
        ActiveParticles.push_back(DMPC); 

        if (init_with_sph_particles == 1)
           SPHPC = new DarkMatterParticleContainer(parent);

	if (parent->subCycle())
	{
	    VirtPC = new DarkMatterParticleContainer(parent);
            VirtualParticles.push_back(VirtPC); 

	    GhostPC = new DarkMatterParticleContainer(parent);
            GhostParticles.push_back(GhostPC); }
        //
        // Make sure to call RemoveParticlesOnExit() on exit.
        //
        amrex::ExecOnFinalize(RemoveParticlesOnExit);
        //
        // 2 gives more stuff than 1.
        //
        DMPC->SetVerbose(particle_verbose);

        if (particle_init_type == "Random")
        {
#ifdef DISABLE_INIT_RANDOM
            amrex::Abort("Nyx::init_particles(): Random Disabled (ERT)");
#endif
            if (particle_initrandom_count <= 0)
            {
                amrex::Abort("Nyx::init_particles(): particle_initrandom_count must be > 0");
            }
            if (particle_initrandom_iseed <= 0)
            {
                amrex::Abort("Nyx::init_particles(): particle_initrandom_iseed must be > 0");
            }

            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing DM with cloud of "
                          << particle_initrandom_count
                          << " random particles with initial seed: "
                          << particle_initrandom_iseed << "\n\n";
            }

#ifndef DISABLE_INIT_RANDOM
            DMPC->InitRandom(particle_initrandom_count,
                             particle_initrandom_iseed,
                             particle_initrandom_mass,
                             particle_initrandom_serialize);
#endif
        }
        else if (particle_init_type == "RandomPerBox")
        {
#ifdef DISABLE_INIT_RANDOM
            amrex::Abort("Nyx::init_particles(): RaondomPerBox disabled (ERT)");
#endif
            if (particle_initrandom_count_per_box <= 0)
            {
                amrex::Abort("Nyx::init_particles(): particle_initrandom_count_per_box must be > 0");
            }
            if (particle_initrandom_iseed <= 0)
            {
                amrex::Abort("Nyx::init_particles(): particle_initrandom_iseed must be > 0");
            }

            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing DM with of " << particle_initrandom_count_per_box
                          << " random particles per box with initial seed: "
                          << particle_initrandom_iseed << "\n\n";
            }

            // We just make this MultiFab in order to iterate over it ...
            MultiFab particle_mf(grids,dmap,1,1);

#ifndef DISABLE_INIT_RANDOM
            DMPC->InitRandomPerBox(particle_initrandom_count_per_box,
                                   particle_initrandom_iseed,
                                   particle_initrandom_mass,
                                   particle_mf);
#endif
        }
        else if (particle_init_type == "AsciiFile")
        {
            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing DM particles from \""
                          << ascii_particle_file << "\" ...\n\n";
                if (init_with_sph_particles == 1)
                    std::cout << "\nInitializing SPH particles from ascii \""
                              << sph_particle_file << "\" ...\n\n";
            }
            //
            // The second argument is how many Reals we read into `m_data[]`
            // after reading in `m_pos[]`. Here we're reading in the particle
            // mass and velocity.
            //
            DMPC->InitFromAsciiFile(ascii_particle_file, BL_SPACEDIM + 1, &Nrep);
            if (init_with_sph_particles == 1)
                SPHPC->InitFromAsciiFile(ascii_particle_file, BL_SPACEDIM + 1, &Nrep);
        }
        else if (particle_init_type == "BinaryFile")
        {
            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing DM particles from \""
                          << binary_particle_file << "\" ...\n\n";
                if (init_with_sph_particles == 1)
                    std::cout << "\nInitializing SPH particles from binary \""
                              << sph_particle_file << "\" ...\n\n";
            }
            //
            // The second argument is how many Reals we read into `m_data[]`
            // after reading in `m_pos[]`. Here we're reading in the particle
            // mass and velocity.
            //
            DMPC->InitFromBinaryFile(binary_particle_file, BL_SPACEDIM + 1);
            if (init_with_sph_particles == 1)
                SPHPC->InitFromBinaryFile(ascii_particle_file, BL_SPACEDIM + 1);
        }
        else if (particle_init_type == "BinaryMetaFile")
        {
            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing DM particles from meta file\""
                          << binary_particle_file << "\" ...\n\n";
                if (init_with_sph_particles == 1)
                    std::cout << "\nInitializing SPH particles from meta file\""
                              << sph_particle_file << "\" ...\n\n";
            }
            //
            // The second argument is how many Reals we read into `m_data[]`
            // after reading in `m_pos[]` in each of the binary particle files.
            // Here we're reading in the particle mass and velocity.
            //
            DMPC->InitFromBinaryMetaFile(binary_particle_file, BL_SPACEDIM + 1);
            if (init_with_sph_particles == 1)
                SPHPC->InitFromBinaryMetaFile(sph_particle_file, BL_SPACEDIM + 1);
        }
        else
        {
            amrex::Error("not a valid input for nyx.particle_init_type");
        }

        if (write_particle_density_at_init == 1)
        {
            MultiFab particle_mf(grids,dmap,1,1);
            DMPC->AssignDensitySingleLevel(particle_mf,0,1,0);

            writeMultiFabAsPlotFile("ParticleDensity", particle_mf, "density");
            exit(0);
        }

        if (write_coarsened_particles)
        {
            DMPC->WriteCoarsenedAsciiFile("coarse_particle_file.ascii");
            exit(0);
        }
    }
#ifdef AGN
    {
        // Note that we don't initialize any actual AGN particles here, we just create the container.
        BL_ASSERT (APC == 0);
        APC = new AGNParticleContainer(parent);
        ActiveParticles.push_back(APC); 

	if (parent->subCycle())
	{
	    VirtAPC = new AGNParticleContainer(parent);
            VirtualParticles.push_back(VirtAPC); 

	    GhostAPC = new AGNParticleContainer(parent);
            GhostParticles.push_back(GhostAPC); 
	}
        //
        // 2 gives more stuff than 1.
        //
        APC->SetVerbose(particle_verbose);
    }
#endif
#ifdef NEUTRINO_PARTICLES
    {
        BL_ASSERT (NPC == 0);
        NPC = new NeutrinoParticleContainer(parent);
        ActiveParticles.push_back(NPC); 

        // Set the relativistic flag to 1 so that the density will be gamma-weighted
        //     when returned by AssignDensity calls.
        NPC->SetRelativistic(1);

        // We must set the value for csquared which is used in computing gamma = 1 / sqrt(1-vsq/csq)
        // Obviously this value is just a place-holder for now.
        NPC->SetCSquared(1.);

	if (parent->subCycle())
	{
	    VirtNPC = new NeutrinoParticleContainer(parent);
            VirtualParticles.push_back(VirtNPC); 

            // Set the relativistic flag to 1 so that the density will be gamma-weighted
            //     when returned by AssignDensity calls.
            VirtNPC->SetRelativistic(1);

	    GhostNPC = new NeutrinoParticleContainer(parent);
            GhostParticles.push_back(GhostNPC); 

            // Set the relativistic flag to 1 so that the density will be gamma-weighted
            //     when returned by AssignDensity calls.
            GhostNPC->SetRelativistic(1);
	}
        //
        // Make sure to call RemoveParticlesOnExit() on exit.
        //   (if do_dm_particles then we have already called ExecOnFinalize)
        //
        if (!do_dm_particles)
            amrex::ExecOnFinalize(RemoveParticlesOnExit);
        //
        // 2 gives more stuff than 1.
        //
        NPC->SetVerbose(particle_verbose);
        if (particle_init_type == "AsciiFile")
        {
            if (verbose && ParallelDescriptor::IOProcessor())
                std::cout << "\nInitializing Neutrino particles from \""
                          << neutrino_particle_file << "\" ...\n\n";
            //
            // The second argument is how many Reals we read into `m_data[]`
            // after reading in `m_pos[]`. Here we're reading in the particle
            // mass, velocity and angles.
            //
            NPC->InitFromAsciiFile(neutrino_particle_file, 2*BL_SPACEDIM + 1, &Nrep);
        }
        else if (particle_init_type == "BinaryFile")
        {
            if (verbose && ParallelDescriptor::IOProcessor())
            {
                std::cout << "\nInitializing Neutrino particles from \""
                          << neutrino_particle_file << "\" ...\n\n";
            }
            //
            // The second argument is how many Reals we read into `m_data[]`
            // after reading in `m_pos[]`. Here we're reading in the particle
            // mass and velocity.
            //
            NPC->InitFromBinaryFile(neutrino_particle_file, BL_SPACEDIM + 1);
        }

        else
        {
            amrex::Error("for right now we only init Neutrino particles with ascii or binary");
        }
    }
#endif
}

#ifdef GRAVITY
#ifndef NO_HYDRO
void
Nyx::init_santa_barbara (int init_sb_vels)
{
    BL_PROFILE("Nyx::init_santa_barbara()");
    Real cur_time = state[State_Type].curTime();
    Real a = old_a;

    if (ParallelDescriptor::IOProcessor()) {
        std::cout << "... time and comoving a when data is initialized at level " 
                  << level << " " << cur_time << " " << a << '\n';
    }

    if (level == 0)
    {
        Real frac_for_hydro, omb, omm;
        fort_get_omb(&omb);
        fort_get_omm(&omm);
        frac_for_hydro = omb;
        Real omfrac = 1.0 - frac_for_hydro;
 
        if ( (init_with_sph_particles == 0) && (frac_for_hydro != 1.0) ) {
            DMPC->MultiplyParticleMass(level, omfrac);
	}

        Array<std::unique_ptr<MultiFab> > particle_mf;
        if (init_sb_vels == 1)
        {
            if (init_with_sph_particles == 1) {
               SPHPC->AssignDensityAndVels(particle_mf);
	    } else {
               DMPC->AssignDensityAndVels(particle_mf);
	    }

        } else {
            if (init_with_sph_particles == 1) {
                SPHPC->AssignDensity(particle_mf);
	    } else {
                DMPC->AssignDensity(particle_mf);
	    }
        }

        // As soon as we have used the SPH particles to define the density
        //    and velocity on the grid, we can go ahead and destroy them.
        if (init_with_sph_particles == 1) {
            delete SPHPC;
	}

        for (int lev = parent->finestLevel()-1; lev >= 0; lev--)
        {
            amrex::average_down(*particle_mf[lev+1], *particle_mf[lev],
                                 parent->Geom(lev+1), parent->Geom(lev), 0, 1,
                                 parent->refRatio(lev));
        }

        // Only multiply the density, not the velocities
        if (init_with_sph_particles == 0)
        {
            if (frac_for_hydro == 1.0)
            {
                particle_mf[level]->mult(0,0,1);
            }
            else
            {
                particle_mf[level]->mult(frac_for_hydro / omfrac,0,1);
            }
        }

        const Real * dx = geom.CellSize();
        MultiFab& S_new = get_new_data(State_Type);
        MultiFab& D_new = get_new_data(DiagEOS_Type);

        int ns = S_new.nComp();
        int nd = D_new.nComp(); 
        for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
        {
            RealBox gridloc = RealBox(grids[mfi.index()],
                                      geom.CellSize(),
                                      geom.ProbLo());
            const Box& box = mfi.validbox();
            const int* lo = box.loVect();
            const int* hi = box.hiVect();

            // Temp unused for GammaLaw, set it here so that pltfiles have
            // defined numbers
            D_new[mfi].setVal(0, Temp_comp);
            D_new[mfi].setVal(0,   Ne_comp);

            fort_initdata
                (level, cur_time, lo, hi, 
                 ns,BL_TO_FORTRAN(S_new[mfi]), 
                 nd,BL_TO_FORTRAN(D_new[mfi]), dx,
                 gridloc.lo(), gridloc.hi());
        }

        // Add the particle density to the gas density 
        MultiFab::Add(S_new, *particle_mf[level], 0, Density, 1, S_new.nGrow());

        if (init_sb_vels == 1)
        {
            // Convert velocity to momentum
            for (int i = 0; i < BL_SPACEDIM; ++i) {
               MultiFab::Multiply(*particle_mf[level], *particle_mf[level], 0, 1+i, 1, 0);
	    }

            // Add the particle momenta to the gas momenta (initially zero)
            MultiFab::Add(S_new, *particle_mf[level], 1, Xmom, BL_SPACEDIM, S_new.nGrow());
        }

    } else {

        MultiFab& S_new = get_new_data(State_Type);
        FillCoarsePatch(S_new, 0, cur_time, State_Type, 0, S_new.nComp());

        MultiFab& D_new = get_new_data(DiagEOS_Type);
        FillCoarsePatch(D_new, 0, cur_time, DiagEOS_Type, 0, D_new.nComp());
 
        MultiFab& Phi_new = get_new_data(PhiGrav_Type);
        FillCoarsePatch(Phi_new, 0, cur_time, PhiGrav_Type, 0, Phi_new.nComp());

       // Convert (rho X)_i to X_i before calling init_e_from_T
       if (use_const_species == 0) {
           for (int i = 0; i < NumSpec; ++i) {
               MultiFab::Divide(S_new, S_new, Density, FirstSpec+i, 1, 0);
	   }
       }
    }

    // Make sure we've finished initializing the density before calling this.
    MultiFab& S_new = get_new_data(State_Type);
    MultiFab& D_new = get_new_data(DiagEOS_Type);
    int ns = S_new.nComp();
    int nd = D_new.nComp();

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S_new,true); mfi.isValid(); ++mfi)
    {
        const Box& box = mfi.tilebox();
        const int* lo = box.loVect();
        const int* hi = box.hiVect();

        fort_init_e_from_t
            (BL_TO_FORTRAN(S_new[mfi]), &ns, 
             BL_TO_FORTRAN(D_new[mfi]), &nd, lo, hi, &a);
    }

    // Convert X_i to (rho X)_i
    if (use_const_species == 0) {
        for (int i = 0; i < NumSpec; ++i) {
            MultiFab::Multiply(S_new, S_new, Density, FirstSpec+i, 1, 0);
	}
    }
}
#endif
#endif

void
Nyx::particle_post_restart (const std::string& restart_file, bool is_checkpoint)
{
    BL_PROFILE("Nyx::particle_post_restart()");

    if (level > 0)
        return;
     
    if (do_dm_particles)
    {
        BL_ASSERT(DMPC == 0);
        DMPC = new DarkMatterParticleContainer(parent);
        ActiveParticles.push_back(DMPC);
 
        if (parent->subCycle())
        {
            VirtPC = new DarkMatterParticleContainer(parent);
            VirtualParticles.push_back(VirtPC);
 
            GhostPC = new DarkMatterParticleContainer(parent);
            GhostParticles.push_back(GhostPC);
        }

        //
        // Make sure to call RemoveParticlesOnExit() on exit.
        //
        amrex::ExecOnFinalize(RemoveParticlesOnExit);
        //
        // 2 gives more stuff than 1.
        //
        DMPC->SetVerbose(particle_verbose);
        DMPC->Restart(restart_file, chk_particle_file, is_checkpoint);
        //
        // We want the ability to write the particles out to an ascii file.
        //
        ParmParse pp("particles");

        std::string particle_output_file;
        pp.query("particle_output_file", particle_output_file);

        if (!particle_output_file.empty())
        {
            DMPC->WriteAsciiFile(particle_output_file);
        }
    }
}

#ifdef GRAVITY
void
Nyx::particle_est_time_step (Real& est_dt)
{
    BL_PROFILE("Nyx::particle_est_time_step()");
    if (DMPC && particle_move_type == "Gravitational")
    {
        const Real cur_time = state[PhiGrav_Type].curTime();
        const Real a = get_comoving_a(cur_time);
        MultiFab& grav = get_new_data(Gravity_Type);
        const Real est_dt_particle = DMPC->estTimestep(grav, a, level, particle_cfl);

        if (est_dt_particle > 0) {
            est_dt = std::min(est_dt, est_dt_particle);
	}

#ifdef NEUTRINO_PARTICLES
        const Real est_dt_neutrino = NPC->estTimestep(grav, a, level, neutrino_cfl);
        if (est_dt_neutrino > 0) {
            est_dt = std::min(est_dt, est_dt_neutrino);
	}
#endif

        if (verbose && ParallelDescriptor::IOProcessor())
        {
            if (est_dt_particle > 0)
            {
                std::cout << "...estdt from particles at level "
                          << level << ": " << est_dt_particle << '\n';
            }
            else
            {
                std::cout << "...there are no particles at level "
                          << level << '\n';
            }
#ifdef NEUTRINO_PARTICLES
            if (est_dt_neutrino > 0)
            {
                std::cout << "...estdt from neutrinos at level "
                          << level << ": " << est_dt_neutrino << '\n';
            }
#endif
        }
    }
}
#endif

void
Nyx::particle_redistribute (int lbase, bool init)
{
    BL_PROFILE("Nyx::particle_redistribute()");
    if (DMPC)
    {
        //  
        // If we are calling with init = true, then we want to force the redistribute
        //    without checking whether the grids have changed.
        //  
        if (init)
        {
            DMPC->Redistribute(lbase);
            return;
        }

        //
        // These are usually the BoxArray and DMap from the last regridding.
        //
        static Array<BoxArray>            ba;
        static Array<DistributionMapping> dm;

        bool changed = false;

        int flev = parent->finestLevel();
	
        while ( parent->getAmrLevels()[flev] == nullptr ) {
            flev--;
	}
 
        if (ba.size() != flev+1)
        {
            ba.resize(flev+1);
            dm.resize(flev+1);
            changed = true;
        }
        else
        {
            for (int i = 0; i <= flev && !changed; i++)
            {
                if (ba[i] != parent->boxArray(i))
                    //
                    // The BoxArrays have changed in the regridding.
                    //
                    changed = true;

                if ( ! changed)
                {
                    if (dm[i] != parent->getLevel(i).get_new_data(0).DistributionMap())
                        //
                        // The DistributionMaps have changed in the regridding.
                        //
                        changed = true;
                }
            }
        }

        if (changed)
        {
            //
            // We only need to call Redistribute if the BoxArrays or DistMaps have changed.
	    // We also only call it for particles >= lbase. This is
	    // because of we called redistribute during a subcycle, there may be particles not in
	    // the proper position on coarser levels.
            //
            if (verbose && ParallelDescriptor::IOProcessor())
                std::cout << "Calling redistribute because changed " << '\n';

            DMPC->Redistribute(lbase);
            //
            // Use the new BoxArray and DistMap to define ba and dm for next time.
            //
            for (int i = 0; i <= flev; i++)
            {
                ba[i] = parent->boxArray(i);
                dm[i] = parent->getLevel(i).get_new_data(0).DistributionMap();
            }
        }
        else
        {
            if (verbose && ParallelDescriptor::IOProcessor())
                std::cout << "NOT calling redistribute because NOT changed " << '\n';
        }
    }
}

void
Nyx::particle_move_random ()
{
    BL_PROFILE("Nyx::particle_move_random()");
    if (DMPC && particle_move_type == "Random")
    {
        BL_ASSERT(level == 0);

        DMPC->MoveRandom();
    }
}

void
Nyx::setup_virtual_particles()
{
    BL_PROFILE("Nyx::setup_virtual_particles()");
    if(Nyx::theDMPC() != 0 && !virtual_particles_set)
    {
        DarkMatterParticleContainer::AoS virts;
        if (level < parent->finestLevel())
        {
    	    get_level(level + 1).setup_virtual_particles();
	    Nyx::theVirtPC()->CreateVirtualParticles(level+1, virts);
	    Nyx::theVirtPC()->AddParticlesAtLevel(virts, level);
	    Nyx::theDMPC()->CreateVirtualParticles(level+1, virts);
	    Nyx::theVirtPC()->AddParticlesAtLevel(virts, level);
        }
        virtual_particles_set = true;
    }
}

void
Nyx::remove_virtual_particles()
{
    BL_PROFILE("Nyx::remove_virtual_particles()");
    for (int i = 0; i < VirtualParticles.size(); i++)
    {
        if (VirtualParticles[i] != 0)
            VirtualParticles[i]->RemoveParticlesAtLevel(level);
        virtual_particles_set = false;
    }
}

void
Nyx::setup_ghost_particles()
{
    BL_PROFILE("Nyx::setup_ghost_particles()");
    BL_ASSERT(level < parent->finestLevel());
    int nGrow = Nyx::grav_n_grow - 1;
    if(Nyx::theDMPC() != 0)
    {
        DarkMatterParticleContainer::AoS ghosts;
        Nyx::theDMPC()->CreateGhostParticles(level, nGrow, ghosts);
        Nyx::theGhostPC()->AddParticlesAtLevel(ghosts, level+1, nGrow);
    }
#ifdef AGN
    if(Nyx::theAPC() != 0)
    {
        AGNParticleContainer::AoS ghosts;
        Nyx::theAPC()->CreateGhostParticles(level, nGrow, ghosts);
        Nyx::theGhostAPC()->AddParticlesAtLevel(ghosts, level+1, nGrow);
    }
#endif
#ifdef NEUTRINO_PARTICLES
    if(Nyx::theNPC() != 0)
    {
        NeutrinoParticleContainer::AoS ghosts;
        Nyx::theNPC()->CreateGhostParticles(level, nGrow, ghosts);
        Nyx::theGhostNPC()->AddParticlesAtLevel(ghosts, level+1, nGrow);
    }
#endif
}

void
Nyx::remove_ghost_particles()
{
    BL_PROFILE("Nyx::setup_ghost_particles()");
    for (int i = 0; i < GhostParticles.size(); i++)
    {
        if (GhostParticles[i] != 0)
            GhostParticles[i]->RemoveParticlesAtLevel(level);
    }
}



void
Nyx::NyxParticlesAddProcsToComp(Amr *aptr, int nSidecarProcs, int prevSidecarProcs,
                    int ioProcNumSCS, int ioProcNumAll, int scsMyId,
		                        MPI_Comm scsComm)
{
#if 0
// What is this doing here???
if(ParallelDescriptor::IOProcessor()) {
  std::cout << "PPPPPPPP:  DMPC SPC APC NPC = " << DMPC << "  " << SPC << "  " << APC << "  " << NPC << std::endl;
}
#endif
}

//NyxParticleContainerBase::~NyxParticleContainerBase() {}
