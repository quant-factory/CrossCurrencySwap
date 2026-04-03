//
// Created by ricar on 30/03/2026.
//

#include "sensibilities.h"

namespace CCS {

    // Constructor of the class

    CCS_sensitivities::CCS_sensitivities(const CCS_params &params ,
                                         const YieldCurve &forCurve ,
                                         const YieldCurve &domCurve) :
    m_params(params),
    m_forCurve(forCurve),
    m_domCurve(domCurve) {} ;


    YieldCurve CCS_sensitivities::YieldCurve_parl_bump(const YieldCurve &curve, const double bump) {

        YieldCurve bumped = curve;
        for (std::size_t i = 0; i < bumped.times.size(); ++i) {

            bumped.disc_Factors[i] *= std::exp(-bump * bumped.times[i]);
        }
        return bumped;
    }

    YieldCurve CCS_sensitivities::YieldCurve_one_bump(const YieldCurve &curve, const double bump, const std::size_t idx) {

        YieldCurve bumped = curve;
        if (idx >= bumped.times.size())

            throw std::out_of_range("One bump : Index out of limits");

        bumped.disc_Factors[idx] *= std::exp(-bump * bumped.times[idx]);
        return bumped;
    }

    double CCS_sensitivities::basePV() const {
        CCSPricer pricer(m_params, m_forCurve, m_domCurve);
        return pricer.pricing().PV_Xccy;
    }

    double CCS_sensitivities::parl_bumpPV(const double &bump, const std::string &whichCurve) const {

        YieldCurve bumpedFor = m_forCurve;
        YieldCurve bumpedDom = m_domCurve;

        if (whichCurve == "FOR")
            bumpedFor = YieldCurve_parl_bump(m_forCurve, bump);
        else if (whichCurve == "DOM")
            bumpedDom = YieldCurve_parl_bump(m_domCurve, bump);
        else
            throw std::invalid_argument("parl_bumpPV : whichCurve doit should be 'DOM' or 'FOR'  ");

        const CCSPricer pricer(m_params, bumpedFor, bumpedDom);

        return pricer.pricing().PV_Xccy;
    }

    double CCS_sensitivities::one_bumpPV(const double &bump, std::size_t idx, const std::string &whichCurve) const {

        YieldCurve bumpedFor = m_forCurve;
        YieldCurve bumpedDom = m_domCurve;

        if (whichCurve == "FOR")
            bumpedFor = YieldCurve_one_bump(m_forCurve, bump, idx);
        else if (whichCurve == "DOM")
            bumpedDom = YieldCurve_one_bump(m_domCurve, bump, idx);
        else
            throw std::invalid_argument("one_bumpPV : whichCurve should be either 'FOR' or 'DOM");

        const CCSPricer pricer(m_params, bumpedFor, bumpedDom);
        return pricer.pricing().PV_Xccy;
    }

    double CCS_sensitivities::spread_bumpPV(const double &bump, const std::string &whichLeg) const {

        CCS_params bumpedParams = m_params;

        if (whichLeg == "FOR") {
            bumpedParams.for_leg.spread+=bump;
        }
        else if (whichLeg == "DOM") {
            bumpedParams.dom_leg.spread+=bump;
        }
        else {
            throw std::invalid_argument("spread_bumpPV: whichLeg should be 'FOR' or 'DOM'");
        }
        const CCSPricer pricer(bumpedParams, m_forCurve, m_domCurve);
        return pricer.pricing().PV_Xccy;

    }

    double CCS_sensitivities::spotFX_bumpPV(const double &bump_pct) const {
        CCS_params bumpedParams = m_params;
        bumpedParams.fx_params.spotFX+=(1.0 + bump_pct);
        const CCSPricer pricer (bumpedParams, m_forCurve, m_domCurve);
        return pricer.pricing().PV_Xccy;
    }

    double CCS_sensitivities::dv01_parl(const double &bump, const std::string &whichCurve) const {

        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const double pv_up   = parl_bumpPV(+h / 2.0, "FOR");
            const double pv_down = parl_bumpPV(-h / 2.0, "FOR");
            return (pv_up - pv_down) / (bump);
        }
        else if (whichCurve == "DOM") {
            const double pv_up   = parl_bumpPV(+h / 2.0, "DOM");
            const double pv_down = parl_bumpPV(-h / 2.0, "DOM");
            return (pv_up - pv_down) / (bump);
        }
        else {
            throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
        }

    }

    double CCS_sensitivities::sv01(const double &bump, const std::string &whichCurve) const {

        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const double pv_up   = spread_bumpPV(+h / 2.0, "FOR");
            const double pv_down = spread_bumpPV(-h / 2.0, "FOR");
            return (pv_up - pv_down) / (bump);
        }

        else if (whichCurve == "DOM") {
            const double pv_up   = spread_bumpPV(+h / 2.0, "DOM");
            const double pv_down = spread_bumpPV(-h / 2.0, "DOM");
            return (pv_up - pv_down) / (bump);
        }
        else {
            throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
        }
    }

    double CCS_sensitivities::fx01(const double &bump) const {

        const double h = bump * 1e-4;
        const double pv_up   = spotFX_bumpPV(+h / 2.0);
        const double pv_down = spotFX_bumpPV(-h / 2.0);
        return (pv_up - pv_down) / bump;
    }

    std::vector<double> CCS_sensitivities::bucket_DV01(const double bump, const std::string &whichCurve) const {
        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const std::size_t n = m_domCurve.times.size();
            std::vector<double> dv01(n, 0.0);

            for (std::size_t k = 0; k < n; ++k) {
                double pv_up   = one_bumpPV(+h / 2.0, k, "DOM");
                double pv_down = one_bumpPV(-h / 2.0, k, "DOM");
                dv01[k] = (pv_up - pv_down) / (bump);
            }
            return dv01;
        }

        else if (whichCurve == "DOM") {

            const std::size_t n = m_domCurve.times.size();
            std::vector<double> dv01(n, 0.0);

            for (std::size_t k = 0; k < n; ++k) {
                double pv_up   = one_bumpPV(+h / 2.0, k, "DOM");
                double pv_down = one_bumpPV(-h / 2.0, k, "DOM");
                dv01[k] = (pv_up - pv_down) / (h);
            }
            return dv01;
        }

        else {
            throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
        }

    }


    SensitivityResult CCS_sensitivities::computeAll(const double bump, double FxBump) const

    {
        SensitivityResult res{};


        res.DV01_FOR   = dv01_parl(bump , "FOR") ;
        res.DV01_DOM   = dv01_parl(bump, "DOM");
        res.DV01_total = res.DV01_FOR + res.DV01_DOM;

        res.SV01_FOR = sv01(bump, "FOR");
        res.SV01_DOM = sv01(bump, "DOM");

        res.FX01 = fx01(FxBump);

        return res;
    }





    }
