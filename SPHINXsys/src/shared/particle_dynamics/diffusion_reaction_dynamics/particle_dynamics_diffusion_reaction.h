/* -----------------------------------------------------------------------------*
 *                               SPHinXsys                                      *
 * -----------------------------------------------------------------------------*
 * SPHinXsys (pronunciation: s'finksis) is an acronym from Smoothed Particle    *
 * Hydrodynamics for industrial compleX systems. It provides C++ APIs for       *
 * physical accurate simulation and aims to model coupled industrial dynamic    *
 * systems including fluid, solid, multi-body dynamics and beyond with SPH      *
 * (smoothed particle hydrodynamics), a meshless computational method using     *
 * particle discretization.                                                     *
 *                                                                              *
 * SPHinXsys is partially funded by German Research Foundation                  *
 * (Deutsche Forschungsgemeinschaft) DFG HU1527/6-1, HU1527/10-1,               *
 * HU1527/12-1 and HU1527/12-4.                                                 *
 *                                                                              *
 * Portions copyright (c) 2017-2022 Technical University of Munich and          *
 * the authors' affiliations.                                                   *
 *                                                                              *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may      *
 * not use this file except in compliance with the License. You may obtain a    *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.           *
 *                                                                              *
 * -----------------------------------------------------------------------------*/
/**
 * @file 	particle_dynamics_diffusion_reaction.h
 * @brief 	This is the particle dynamics applicable for all type bodies
 * 			TODO: there is an issue on applying corrected configuration for contact bodies.
 * @author	Xiaojing Tang, Chi ZHang and Xiangyu Hu
 */

#ifndef PARTICLE_DYNAMICS_DIFFUSION_REACTION_H
#define PARTICLE_DYNAMICS_DIFFUSION_REACTION_H

#include "all_particle_dynamics.h"
#include "diffusion_reaction_particles.h"
#include "diffusion_reaction.h"

namespace SPH
{
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	using DiffusionReactionSimpleData =
		DataDelegateSimple<DiffusionReactionParticles<NUM_SPECIES, BaseParticlesType, BaseMaterialType>>;

	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	using DiffusionReactionInnerData =
		DataDelegateInner<DiffusionReactionParticles<NUM_SPECIES, BaseParticlesType, BaseMaterialType>>;

	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType,
			  class ContactBaseParticlesType, class ContactBaseMaterialType>
	using DiffusionReactionContactData =
		DataDelegateContact<DiffusionReactionParticles<NUM_SPECIES, BaseParticlesType, BaseMaterialType>,
							DiffusionReactionParticles<NUM_SPECIES, ContactBaseParticlesType, ContactBaseMaterialType>,
							DataDelegateEmptyBase>;
	/**
	 * @class  DiffusionReactionInitialCondition
	 * @brief pure abstract class for initial conditions
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class DiffusionReactionInitialCondition
		: public LocalDynamics,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		explicit DiffusionReactionInitialCondition(SPHBody &sph_body);
		virtual ~DiffusionReactionInitialCondition(){};

	protected:
		StdLargeVec<Vecd> &pos_;
		StdVec<StdLargeVec<Real>> &species_n_;
	};

	/**
	 * @class GetDiffusionTimeStepSize
	 * @brief Computing the time step size based on diffusion coefficient and particle smoothing length
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class GetDiffusionTimeStepSize
		: public BaseDynamics<Real>,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		explicit GetDiffusionTimeStepSize(SPHBody &sph_body);
		virtual ~GetDiffusionTimeStepSize(){};

		virtual Real exec(Real dt = 0.0) override { return diff_time_step_; };
		virtual Real parallel_exec(Real dt = 0.0) override { return exec(dt); };

	protected:
		Real diff_time_step_;
	};

	/**
	 * @class RelaxationOfAllDiffusionSpeciesInner
	 * @brief Compute the diffusion relaxation process of all species
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class RelaxationOfAllDiffusionSpeciesInner
		: public LocalDynamics,
		  public DiffusionReactionInnerData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	protected:
		/** all diffusion species and diffusion relation. */
		StdVec<BaseDiffusion *> species_diffusion_;
		StdVec<StdLargeVec<Real>> &species_n_;
		StdVec<StdLargeVec<Real>> &diffusion_dt_;

		void initializeDiffusionChangeRate(size_t particle_i);
		void getDiffusionChangeRate(size_t particle_i, size_t particle_j, Vecd &e_ij, Real surface_area_ij);
		virtual void updateSpeciesDiffusion(size_t particle_i, Real dt);

	public:
		static const int number_of_species_ = NUM_SPECIES;
		typedef BaseParticlesType InnerBaseParticlesType;
		typedef BaseMaterialType InnerBaseMaterialType;
		typedef BaseInnerRelation BodyRelationType;
		DiffusionReaction<NUM_SPECIES, BaseMaterialType> &diffusion_reaction_material_;

		explicit RelaxationOfAllDiffusionSpeciesInner(BaseInnerRelation &inner_relation);
		virtual ~RelaxationOfAllDiffusionSpeciesInner(){};
		void interaction(size_t index_i, Real dt = 0.0);
		void update(size_t index_i, Real dt = 0.0);
	};

	/**
	 * @class RelaxationOfAllDiffusionSpeciesComplex
	 * Complex diffusion relaxation between two different bodies
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType,
			  class ContactBaseParticlesType, class ContactBaseMaterialType>
	class RelaxationOfAllDiffusionSpeciesComplex
		: public RelaxationOfAllDiffusionSpeciesInner<NUM_SPECIES, BaseParticlesType, BaseMaterialType>,
		  public DiffusionReactionContactData<NUM_SPECIES, BaseParticlesType, BaseMaterialType,
											  ContactBaseParticlesType, ContactBaseMaterialType>
	{
		StdVec<BaseDiffusion *> species_diffusion_;
		StdVec<StdLargeVec<Real>> &species_n_;
		StdVec<StdLargeVec<Real>> &diffusion_dt_;
		StdVec<StdVec<StdLargeVec<Real>> *> contact_species_n_;

	protected:
		void getDiffusionChangeRateContact(size_t particle_i, size_t particle_j, Vecd &e_ij,
										   Real surface_area_ij, const StdVec<StdLargeVec<Real>> &species_n_k);

	public:
		typedef ComplexRelation BodyRelationType;
		explicit RelaxationOfAllDiffusionSpeciesComplex(ComplexRelation &complex_relation);
		virtual ~RelaxationOfAllDiffusionSpeciesComplex(){};
		void interaction(size_t index_i, Real dt = 0.0);
	};

	/**
	 * @class InitializationRK
	 * @brief initialization of a runge-kutta integration scheme
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class InitializationRK
		: public LocalDynamics,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
		StdVec<BaseDiffusion *> species_diffusion_;
		StdVec<StdLargeVec<Real>> &species_n_, &species_s_;

		void initializeIntermediateValue(size_t particle_i);

	public:
		InitializationRK(SPHBody &sph_body, StdVec<StdLargeVec<Real>> &species_s);
		virtual ~InitializationRK(){};

		void update(size_t index_i, Real dt = 0.0);
	};

	/**
	 * @class SecondStageRK2
	 * @brief the second stage of the 2nd-order Runge-Kutta scheme
	 */
	template <class FirstStageType>
	class SecondStageRK2 : public FirstStageType
	{
		StdVec<BaseDiffusion *> species_diffusion_;
		StdVec<StdLargeVec<Real>> &species_n_;
		StdVec<StdLargeVec<Real>> &diffusion_dt_;

	protected:
		StdVec<StdLargeVec<Real>> &species_s_;
		virtual void updateSpeciesDiffusion(size_t particle_i, Real dt) override;

	public:
		SecondStageRK2(typename FirstStageType::BodyRelationType &body_relation,
					   StdVec<StdLargeVec<Real>> &species_s);
		virtual ~SecondStageRK2(){};
	};

	/**
	 * @class RelaxationOfAllDiffusionSpeciesRK2
	 * @brief Compute the diffusion relaxation process of all species
	 * with second order Runge-Kutta time stepping
	 */
	template <class FirstStageType>
	class RelaxationOfAllDiffusionSpeciesRK2 : public BaseDynamics<void>
	{
	protected:
		StdVec<BaseDiffusion *> species_diffusion_;
		/** Intermediate Value */
		StdVec<StdLargeVec<Real>> species_s_;

		SimpleDynamics<InitializationRK<FirstStageType::number_of_species_,
										typename FirstStageType::InnerBaseParticlesType,
										typename FirstStageType::InnerBaseMaterialType>>
			rk2_initialization_;
		InteractionWithUpdate<FirstStageType> rk2_1st_stage_;
		InteractionWithUpdate<SecondStageRK2<FirstStageType>> rk2_2nd_stage_;

	public:
		explicit RelaxationOfAllDiffusionSpeciesRK2(typename FirstStageType::BodyRelationType &body_relation);
		virtual ~RelaxationOfAllDiffusionSpeciesRK2(){};

		virtual void exec(Real dt = 0.0) override;
		virtual void parallel_exec(Real dt = 0.0) override;
	};

	struct UpdateAReactionSpecies
	{
		Real operator()(Real input, Real production_rate, Real loss_rate, Real dt) const
		{
			return input * exp(-loss_rate * dt) + production_rate * (1.0 - exp(-loss_rate * dt)) / (loss_rate + TinyReal);
		};
	};

	/**
	 * @class BaseRelaxationOfAllReactions
	 * @brief Base class for computing the reaction process of all species
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class BaseRelaxationOfAllReactions
		: public LocalDynamics,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
		BaseReactionModel<NUM_SPECIES> *species_reaction_;
		typedef std::array<Real, NUM_SPECIES> LocalSpecies;
		StdVec<StdLargeVec<Real>> &species_n_;
		IndexVector &reactive_species_;
		UpdateAReactionSpecies updateAReactionSpecies;
		void loadLocalSpecies(LocalSpecies &local_species, size_t index_i);
		void applyGlobalSpecies(LocalSpecies &local_species, size_t index_i);

	public:
		explicit BaseRelaxationOfAllReactions(SPHBody &sph_body);
		virtual ~BaseRelaxationOfAllReactions(){};

	protected:
		void advanceForwardStep(size_t index_i, Real dt);
		void advanceBackwardStep(size_t index_i, Real dt);
	};

	/**
	 * @class RelaxationOfAllReactionsForward
	 * @brief Compute the reaction process of all species by forward splitting
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class RelaxationOfAllReactionsForward
		: public BaseRelaxationOfAllReactions<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		RelaxationOfAllReactionsForward(SPHBody &sph_body)
			: BaseRelaxationOfAllReactions<NUM_SPECIES, BaseParticlesType, BaseMaterialType>(sph_body){};
		virtual ~RelaxationOfAllReactionsForward(){};
		void update(size_t index_i, Real dt = 0.0) { this->advanceForwardStep(index_i, dt); };
	};

	/**
	 * @class RelaxationOfAllReactionsBackward
	 * @brief Compute the reaction process of all species by backward splitting
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class RelaxationOfAllReactionsBackward
		: public BaseRelaxationOfAllReactions<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		explicit RelaxationOfAllReactionsBackward(SPHBody &sph_body)
			: BaseRelaxationOfAllReactions<NUM_SPECIES, BaseParticlesType, BaseMaterialType>(sph_body){};
		virtual ~RelaxationOfAllReactionsBackward(){};
		void update(size_t index_i, Real dt = 0.0) { this->advanceBackwardStep(index_i, dt); };
	};

	/**
	 * @class DiffusionReactionSpeciesConstraint
	 * @brief set boundary condition for diffusion problem
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class DiffusionReactionSpeciesConstraint
		: public LocalDynamics,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		DiffusionReactionSpeciesConstraint(SPHBody &sph_body, const std::string &species_name)
			: LocalDynamics(sph_body),
			  DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>(sph_body),
			  diffusion_reaction_material_(this->particles_->diffusion_reaction_material_),
			  phi_(diffusion_reaction_material_.SpeciesIndexMap()[species_name]),
			  species_(this->particles_->species_n_[phi_]){};
		DiffusionReactionSpeciesConstraint(BodyPartByParticle &body_part, const std::string &species_name)
			: DiffusionReactionSpeciesConstraint(body_part.getSPHBody(), species_name){};
		virtual ~DiffusionReactionSpeciesConstraint(){};

	protected:
		DiffusionReaction<NUM_SPECIES, BaseMaterialType> &diffusion_reaction_material_;
		size_t phi_;
		StdLargeVec<Real> &species_;
	};

	/**
	 * @class DiffusionBasedMapping
	 * @brief Mapping inside of body according to diffusion.
	 * This is a abstract class to be override for case specific implementation
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class DiffusionBasedMapping
		: public LocalDynamics,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	public:
		explicit DiffusionBasedMapping(SPHBody &sph_body)
			: LocalDynamics(sph_body),
			  DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>(sph_body),
			  pos_(this->particles_->pos_), species_n_(this->particles_->species_n_){};
		virtual ~DiffusionBasedMapping(){};

	protected:
		StdLargeVec<Vecd> &pos_;
		StdVec<StdLargeVec<Real>> &species_n_;
	};

	/**
	 * @class 	DiffusionReactionSpeciesSummation
	 * @brief 	Computing the total averaged parameter on the whole diffusion body.
	 * 			TODO: need a test using this method
	 */
	template <int NUM_SPECIES, class BaseParticlesType, class BaseMaterialType>
	class DiffusionReactionSpeciesSummation
		: public LocalDynamicsReduce<Real, ReduceSum<Real>>,
		  public DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>
	{
	protected:
		DiffusionReaction<NUM_SPECIES, BaseMaterialType> &diffusion_reaction_material_;
		StdVec<StdLargeVec<Real>> &species_n_;
		size_t phi_;

	public:
		DiffusionReactionSpeciesSummation(SPHBody &sph_body, const std::string &species_name)
			: LocalDynamicsReduce<Real, ReduceSum<Real>>(sph_body, Real(0)),
			  DiffusionReactionSimpleData<NUM_SPECIES, BaseParticlesType, BaseMaterialType>(sph_body),
			  diffusion_reaction_material_(this->particles_->diffusion_reaction_material_),
			  species_n_(this->particles_->species_n_),
			  phi_(diffusion_reaction_material_.SpeciesIndexMap()[species_name])
		{
			quantity_name_ = "DiffusionReactionSpeciesAverage";
		};
		DiffusionReactionSpeciesSummation(BodyPartByParticle &body_part, const std::string &species_name)
			: DiffusionReactionSpeciesSummation(body_part.getSPHBody(), species_name){};
		virtual ~DiffusionReactionSpeciesSummation(){};

		Real reduce(size_t index_i, Real dt = 0.0)
		{
			return species_n_[phi_][index_i];
		};
	};
}
#endif // PARTICLE_DYNAMICS_DIFFUSION_REACTION_H