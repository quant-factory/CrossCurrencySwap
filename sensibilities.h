//
// Created by ricar on 30/03/2026.
//

#pragma once

#include "Core/Types.h"
#include "ccs_pricer.hpp"

namespace CCS {

    class CCS_sensitivities {

    public:

        // Constructor of the class

        CCS_sensitivities( const CCS_params & params , const YieldCurve & forCurve , const YieldCurve & domCurve) ;


        double dv01_parl(const double &bump, const std::string & whichCurve) const;

        double sv01(const double &bump , const std::string & whichCurve) const ;

        double fx01(const double &bump) const ;

        std::vector<double> bucket_DV01(double bump, const std::string & whichCurve) const;

        SensitivityResult computeAll(double bump , double FxBump) const;


    private:

        const CCS_params &m_params;
        const YieldCurve &m_forCurve;
        const YieldCurve &m_domCurve;

        static YieldCurve YieldCurve_parl_bump(const YieldCurve & curve , double bump);
        static YieldCurve YieldCurve_one_bump(const YieldCurve & curve , double bump , std::size_t idx);

        double basePV() const ;
        double parl_bumpPV(const double &bump , const std::string & whichCurve) const;
        double one_bumpPV(const double &bump , std::size_t idx, const std::string & whichCurve) const;

        double spread_bumpPV(const double &bump , const std::string & whichLeg) const;
        double spotFX_bumpPV(const double &bump_pct) const;



    };

}