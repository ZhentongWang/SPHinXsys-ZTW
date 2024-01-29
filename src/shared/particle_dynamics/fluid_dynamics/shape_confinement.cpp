#include "shape_confinement.h"

namespace SPH
{
namespace fluid_dynamics
{
//=================================================================================================//
StaticConfinementDensity::StaticConfinementDensity(NearShapeSurface &near_surface)
    : BaseLocalDynamics<BodyPartByCell>(near_surface), FluidDataSimple(sph_body_),
      rho0_(sph_body_.base_material_->ReferenceDensity()),
      inv_sigma0_(1.0 / sph_body_.sph_adaptation_->LatticeNumberDensity()),
      mass_(particles_->mass_), rho_sum_(*particles_->getVariableByName<Real>("DensitySummation")),
      pos_(particles_->pos_), level_set_shape_(&near_surface.level_set_shape_) {}
//=================================================================================================//
void StaticConfinementDensity::update(size_t index_i, Real dt)
{
    Real inv_Vol_0_i = rho0_ / mass_[index_i];
    rho_sum_[index_i] +=
        level_set_shape_->computeKernelIntegral(pos_[index_i]) * inv_Vol_0_i * rho0_ * inv_sigma0_;
}
//=================================================================================================//
StaticConfinementIntegration1stHalf::StaticConfinementIntegration1stHalf(NearShapeSurface &near_surface)
    : BaseLocalDynamics<BodyPartByCell>(near_surface), FluidDataSimple(sph_body_),
      fluid_(DynamicCast<Fluid>(this, particles_->getBaseMaterial())),
      rho_(particles_->rho_), p_(*particles_->getVariableByName<Real>("Pressure")),
      mass_(particles_->mass_), pos_(particles_->pos_), vel_(particles_->vel_),
      force_(particles_->force_),
      level_set_shape_(&near_surface.level_set_shape_),
      riemann_solver_(fluid_, fluid_) {}
//=================================================================================================//
void StaticConfinementIntegration1stHalf::update(size_t index_i, Real dt)
{
    Vecd kernel_gradient = level_set_shape_->computeKernelGradientIntegral(pos_[index_i]);
    force_[index_i] -= 2.0 * mass_[index_i] * p_[index_i] * kernel_gradient / rho_[index_i];
}
//=================================================================================================//
StaticConfinementIntegration2ndHalf::StaticConfinementIntegration2ndHalf(NearShapeSurface &near_surface)
    : BaseLocalDynamics<BodyPartByCell>(near_surface), FluidDataSimple(sph_body_),
      fluid_(DynamicCast<Fluid>(this, particles_->getBaseMaterial())),
      rho_(particles_->rho_), p_(*particles_->getVariableByName<Real>("Pressure")),
      drho_dt_(*particles_->getVariableByName<Real>("DensityChangeRate")),
      pos_(particles_->pos_), vel_(particles_->vel_),
      level_set_shape_(&near_surface.level_set_shape_),
      riemann_solver_(fluid_, fluid_) {}
//=================================================================================================//
void StaticConfinementIntegration2ndHalf::update(size_t index_i, Real dt)
{
    Vecd kernel_gradient = level_set_shape_->computeKernelGradientIntegral(pos_[index_i]);
    Vecd vel_in_wall = -vel_[index_i];
    drho_dt_[index_i] += rho_[index_i] * (vel_[index_i] - vel_in_wall).dot(kernel_gradient);
}
//=================================================================================================//
StaticConfinement::StaticConfinement(NearShapeSurface &near_surface)
    : density_summation_(near_surface), pressure_relaxation_(near_surface),
      density_relaxation_(near_surface), surface_bounding_(near_surface) {}
//=================================================================================================//
} // namespace fluid_dynamics
} // namespace SPH