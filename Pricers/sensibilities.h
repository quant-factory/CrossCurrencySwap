//
// Created by ricar on 30/03/2026.
//

#pragma once
#include <string>
#include <vector>

namespace Core {
    struct Point;
}

namespace Market {
    class YieldCurve;
    struct CCSwap;
}

namespace Pricer  {

    struct CCSSensitivities_results {
        double DV01_FOR;
        double DV01_DOM;
        double DV01_total;
        double SV01_FOR;
        double SV01_DOM;
        double FX01;
        std::vector<Core::Point> DV01_buckets_FOR;
        std::vector<Core::Point> DV01_buckets_DOM;
    };


    class CCSSensitivities {

    public:

        CCSSensitivities( const Market::CCSwap & params , const Market::YieldCurve & forCurve , const Market::YieldCurve & domCurve) ;


        double dv01_parallel(const double &bump, const std::string & whichCurve) const;

        double sv01(const double &bump , const std::string & whichCurve) const ;

        double fx01(const double &bump) const ;

        std::vector<Core::Point> bucket_DV01(double bump, const std::string & whichCurve) const;

        CCSSensitivities_results sensitivities(double bump , double FxBump) const;



    private:

        const Market::CCSwap &m_params;
        const Market::YieldCurve &m_forCurve;
        const Market::YieldCurve &m_domCurve;


        double basePV() const ;
        double parl_bumpPV(const double &bump , const std::string & whichCurve) const;
        double one_bumpPV(const double &bump , std::size_t idx, const std::string & whichCurve) const;

        double spread_bumpPV(const double &bump , const std::string & whichLeg) const;
        double spotFX_bumpPV(const double &bump_pct) const;

    };

}