#include <cassert>
#include <iostream>
#include <cmath>

#include <LMM/pricer/McLmmVanillaSwapPricer.h>
#include <LMM/helper/LMMTenorStructure.h>

//double McLmmVanillaSwapPricer::swapRate(const VanillaSwap& vanillaSwap, size_t nbSimulation) const
//{
//	throw("Error: not implemented");
//}

double McLmmVanillaSwapPricer::swapNPV(const VanillaSwap& vanillaSwap, size_t nbSimulation)  const
{
	double result = 0.0;
	double variance = 0.0;

	LMM::Index indexValuationDate = 0;
	for(size_t itrSimulation=0; itrSimulation<nbSimulation; ++itrSimulation)
	{
		mcLmm_->simulateLMM();  // YY TODO: not efficient at all, don't need to do all the simulation ... 
		double npvFloatingLeg = pvFloatingLeg(indexValuationDate, vanillaSwap, mcLmm_->get_numeraire(), mcLmm_->get_liborMatrix());
		double npvFixedLeg    = pvFixedLeg   (indexValuationDate, vanillaSwap, mcLmm_->get_numeraire());
		double npvSwap		  = npvFloatingLeg - npvFixedLeg;
		result				 += npvSwap;
		variance += npvSwap*npvSwap;
	}
	result   /=nbSimulation; 

	if(LMM::DEUBGLMM())
	{
		variance = (variance/nbSimulation-result*result);	
	    std::cout << LMM::NOTIF_MSG 
			      << " mcScheme=" << MCSchemeType::mcSchemeType2String(mcLmm_->get_mcSchemeType())
			      << ", SwapPriceMC=" << result
				  << ", nbSimualtion=" << nbSimulation 
			      << ",  99% confidential interval = " << 2.57*std::sqrt(variance / nbSimulation) << std::endl;
	}	

	return result;
}


//! for MC
double McLmmVanillaSwapPricer::pvFloatingLeg(LMM::Index indexValuationDate,
											 const VanillaSwap& vanillaSwap,
										     const std::vector<double>& numeraire, //numeraire[i]: numeraire value at T_i
									         const matrix& liborMatrix) const
{
	assert(indexValuationDate <= vanillaSwap.get_indexStart()); //YY TODO: this test too slow, esp: within MC simulation
	assert(pLMMTenorStructure_->get_horizon() >= vanillaSwap.get_indexEnd());  // if not cannot price this swap;

	double price = 0.0;
	const std::vector<LMM::Index>& floatingLegPaymentIndexSchedule = vanillaSwap.get_floatingLegPaymentIndexSchedule();
	for(size_t itr = 0; itr < floatingLegPaymentIndexSchedule.size(); ++itr)
	{
		//! At time T_{i+1}, pay: L_i(T_i)
		size_t floatingLegPaymentIndex = floatingLegPaymentIndexSchedule[itr]; // i+1
		//! YY TODO: restriction, Attention: this line suppose: MCLMM simulation use the same lmmTenor to simulate libor !!!
		size_t indexLibor = floatingLegPaymentIndex-1; // condition: floatingTenor = lmmTenor
		size_t indexT     = indexLibor;                                        // = i
		double libor      = liborMatrix(indexLibor, indexT);
		double delta_T    = vanillaSwap.get_DeltaTFloatLeg(itr);
		price			 += delta_T*libor*numeraire[indexValuationDate]/numeraire[floatingLegPaymentIndex];		
	}
	return price;
}


double McLmmVanillaSwapPricer:: swapRate(LMM::Index indexValuationDate,
										 const VanillaSwap& vanillaSwap,
										 const std::vector<double>& numeraire, 
										 const matrix& liborMatrix) const
{
	double pvFloating = pvFloatingLeg( indexValuationDate,vanillaSwap,numeraire,liborMatrix);
	double annuity = pvFixedLeg   (indexValuationDate, vanillaSwap, numeraire);
	return pvFloating / annuity;
}

//double MCLmmVanillaSwapPricer::annuity( LMM::Index indexValuationDate,
//										const VanillaSwap& vanillaSwap,
//									    const std::vector<double>& numeraire) const
//{
//	assert(indexValuationDate <= vanillaSwap.get_indexStart()); //YY TODO: this test too slow, esp: within MC simulation
//	assert(pLMMTenorStructure_.get_horizon() >= vanillaSwap.get_indexEnd());  // if not cannot price this swap;
//
//	double price = 0.0;
//	const std::vector<LMM::Index>& fixedLegPaymentIndexSchedule  = vanillaSwap.get_fixedLegPaymentIndexSchedule();
//	for(size_t itr = 0; itr < fixedLegPaymentIndexSchedule.size(); ++itr)
//	{
//		size_t fixedLegPaymentIndex = fixedLegPaymentIndexSchedule[itr]; // = i+1
//		double delta_T              = deltaTFixedLeg_[itr];
//		//std::cout << "numeraire[indexValuationDate]/numeraire[fixedLegPaymentIndex] = " << numeraire[indexValuationDate]/numeraire[fixedLegPaymentIndex] << std::endl;
//		price			           += delta_T*numeraire[indexValuationDate]/numeraire[fixedLegPaymentIndex];		
//	}
//	return price;
//}
//
//double MCLmmVanillaSwapPricer::pvFixedLeg(LMM::Index indexValuationDate,
//	                                      const VanillaSwap& vanillaSwap,
//									      const std::vector<double>& numeraire)  const
//{
//	assert(indexValuationDate <= vanillaSwap.get_indexStart()); //YY TODO: this test too slow, esp: within MC simulation
//	assert(pLMMTenorStructure_.get_horizon() >= vanillaSwap.get_indexEnd());  // if not cannot price this swap;
//
//	return vanillaSwap.get_strike()*annuity(indexValuationDate,vanillaSwap,numeraire);
//}
//
