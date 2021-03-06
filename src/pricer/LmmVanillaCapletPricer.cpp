#include <LMM/pricer/LmmVanillaCapletPricer.h>
#include <LMM/numeric/NumericalMethods.h>
#include <cassert>
#include <vector>
#include <LMM/LmmModel/Shifted_HGVolatilityFunction.h>
#include <boost/pointer_cast.hpp>


LmmVanillaCapletPricer::LmmVanillaCapletPricer(const Lmm_PTR& lmm) 
	: lmm_(lmm)
	  ,buffer_ZC0_(lmm->get_horizon()+2)
	  ,constShifts_(lmm->get_nbLIBOR(), -1.0e100)
{
	//! Why cannot do the cast ???
	//shifted_HGVolatilityFunction_ = boost::dynamic_pointer_cast<Shifted_HGVolatilityFunction_PTR>(lmm_->get_dispersionRef().get_VolatilityFunction_PTR(););
	//assert(shifted_HGVolatilityFunction_ != 0); // if ==0, cast is not succesful.

	assert(lmm_->get_dispersionRef().get_VolatilityFunction_PTR()->isConstShift()); // This approximation works only for constant shift	
	initializeConstShift();
};

void LmmVanillaCapletPricer::initializeConstShift()   // for each libor: shift as a function of time should be const for Rebonato Approxi.
{	
	for(size_t i=0; i<constShifts_.size(); ++i)
	{
		constShifts_[i] = lmm_->get_dispersionRef().get_VolatilityFunction_PTR()->get_shift_timeIndexVersion(i,i);
	}
}


void LmmVanillaCapletPricer::precalculateZC0(const std::vector<double>& liborsInitValue) const // P(T0,Ti)
{
	const std::vector<double>& deltaT = lmm_->get_LMMTenorStructure()->get_deltaT(); // it's the same as swap floating tenor, but need to check

	buffer_ZC0_[0] = 1.0;
	for(size_t i=1; i<buffer_ZC0_.size(); ++i)
	{
		size_t indexLibor = i-1;
		// P(0,T_{i-1}) / P(0,T_i)   = 1 + (T_{i}-T_{i-1}) L(0,T_{i-1},T_i)
		buffer_ZC0_[i] = buffer_ZC0_[i-1]/(1+deltaT[indexLibor]*liborsInitValue[indexLibor]);
	}
}

double LmmVanillaCapletPricer::price(const VanillaCaplet& caplet, 
									 const std::vector<double>& liborsInitValue) const
{
	assert( lmm_->get_LMMTenorStructure()->get_tenorType() == caplet.get_lmmTenorStructureTenorType()); // this pricer can price this Caplet
	assert(liborsInitValue.size() == lmm_->get_nbLIBOR());

	precalculateZC0(liborsInitValue);

	LMM::Index fixingIndex  = caplet.get_indexFixing();
	LMM::Index paymentIndex = caplet.get_indexPayment();

	double fwd    = liborsInitValue[fixingIndex];
	double strike = caplet.get_strike();
	double shift  = constShifts_[fixingIndex];  // Only for Shifted_HGVolatilityFunction

	//LMM::Index maturityIndex = liborIndex;
	double maturityTime  = lmm_->get_LMMTenorStructure()->get_tenorDate(fixingIndex);
	double varianceBlack = lmm_->get_cumulatedcovarianceTensor(fixingIndex, fixingIndex, fixingIndex); // int_{0}^{capletMaturityIndex} <vol_i,vol_i> dt
	double volBlack = std::sqrt(varianceBlack/maturityTime);

	double deltaT =  caplet.get_lmmTenorStructure()->get_deltaT(fixingIndex);

	double df = buffer_ZC0_[paymentIndex];
	double price = deltaT*df*NumericalMethods::Black_Price(fwd+shift, strike+shift, volBlack, maturityTime);
	return price;
}

//
//double LmmVanillaCapletPricer::blackVol (const VanillaCaplet& caplet, const std::vector<double>& liborsInitValue) const
//{
//	double capletPrice = price(caplet,liborsInitValue);
//
//	LMM::Index fixingIndex  = caplet.get_indexFixing();
//	LMM::Index paymentIndex = caplet.get_indexPayment();
//	double maturityTime  = lmm_->get_LMMTenorStructure()->get_tenorDate(fixingIndex);
//	double df = buffer_ZC0_[paymentIndex];
//
//	double fwd    = liborsInitValue[fixingIndex];
//	double strike = caplet.get_strike();
//	double shift  = lmm_->get_shift(fixingIndex);
//	double deltaT =  caplet.get_lmmTenorStructure()->get_deltaT(fixingIndex);
//
//	double impliedBlackVol = NumericalMethods::Black_impliedVolatility(capletPrice/(df*deltaT), fwd+shift, strike+shift, maturityTime);
//	return impliedBlackVol;
//} 


//! YY: TODO strange function, can be used in wrong way so easily ... to delete it ...  
double LmmVanillaCapletPricer::convertPriceToBlackVol(const VanillaCaplet& caplet, const std::vector<double>& liborsInitValue, double price) const
{



	LMM::Index fixingIndex  = caplet.get_indexFixing();
	LMM::Index paymentIndex = caplet.get_indexPayment();
	double maturityTime  = lmm_->get_LMMTenorStructure()->get_tenorDate(fixingIndex);
	double df = buffer_ZC0_[paymentIndex];

	double fwd    = liborsInitValue[fixingIndex];
	double strike = caplet.get_strike();
	double shift  = constShifts_[fixingIndex];
	double deltaT =  caplet.get_lmmTenorStructure()->get_deltaT(fixingIndex);

	double impliedBlackVol = NumericalMethods::Black_impliedVolatility(price/(df*deltaT), fwd+shift, strike+shift, maturityTime);
	return impliedBlackVol;
}
