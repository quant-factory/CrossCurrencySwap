//
// Created by ricar on 30/03/2026.
//

#include "sensibilities.h"
#include "Market/YieldCurve.h"
#include "Market/Produits.h"
#include "Pricers/CCSPricer.h"

namespace Pricer {

    // Constructor of the class

    CCSSensitivities::CCSSensitivities(const Market::CCSwap &params, const Market::YieldCurve &forCurve, const Market::YieldCurve &domCurve) :
    m_params(params),
    m_forCurve(forCurve),
    m_domCurve(domCurve) {};


    double CCSSensitivities::basePV() const {
       const CCSPricer pricer(m_params, m_forCurve, m_domCurve);
        return pricer.pricing().PV_Xccy;
    }

    double CCSSensitivities::parl_bumpPV(const double &bump, const std::string &whichCurve) const {

        if (whichCurve == "FOR") {
            const auto bumpedFor = m_forCurve.parallel_bump(bump);
            const CCSPricer pricer(m_params, bumpedFor , m_domCurve);
            return pricer.pricing().PV_Xccy;
        }
        if (whichCurve == "DOM") {
            const auto bumpedDom = m_domCurve.parallel_bump(bump);
            const CCSPricer pricer(m_params, m_forCurve , bumpedDom);
            return pricer.pricing().PV_Xccy;
        }
        throw std::invalid_argument("parl_bumpPV : whichCurve should be 'DOM' or 'FOR'  ");
    }

    double CCSSensitivities::one_bumpPV(const double &bump, const std::size_t idx, const std::string &whichCurve) const {

        if (whichCurve == "FOR") {
            const auto bumpedFor = m_forCurve.pillar_bump(bump,idx);
            const CCSPricer pricer(m_params, bumpedFor, m_domCurve);
            return pricer.pricing().PV_Xccy;
        }
        if (whichCurve == "DOM") {
            const auto bumpedDom = m_domCurve.pillar_bump(bump,idx);
            const CCSPricer pricer(m_params, m_forCurve, bumpedDom);
            return pricer.pricing().PV_Xccy;
        }
        throw std::invalid_argument("parl_bumpPV : whichCurve should be 'DOM' or 'FOR'  ");
    }

    double CCSSensitivities::spread_bumpPV(const double &bump, const std::string &whichLeg) const {

        Market::CCSwap bumpedParams = m_params;

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

    double CCSSensitivities::spotFX_bumpPV(const double &bump_pct) const {
        auto bumpedParams = m_params;
        bumpedParams.fx_params.spotFX+=(1.0 + bump_pct);
        const CCSPricer pricer (bumpedParams, m_forCurve, m_domCurve);
        return pricer.pricing().PV_Xccy;
    }

    double CCSSensitivities::dv01_parallel(const double &bump, const std::string &whichCurve) const {

        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const double pv_up   = parl_bumpPV(+h / 2.0, "FOR");
            const double pv_down = parl_bumpPV(-h / 2.0, "FOR");
            return (pv_up - pv_down) / (bump);
        }
        if (whichCurve == "DOM") {
            const double pv_up   = parl_bumpPV(+h / 2.0, "DOM");
            const double pv_down = parl_bumpPV(-h / 2.0, "DOM");
            return (pv_up - pv_down) / (bump);
        }
        throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
    }

    double CCSSensitivities::sv01(const double &bump, const std::string &whichCurve) const {

        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const double pv_up   = spread_bumpPV(+h / 2.0, "FOR");
            const double pv_down = spread_bumpPV(-h / 2.0, "FOR");
            return (pv_up - pv_down) / (bump);
        }
        if (whichCurve == "DOM") {
            const double pv_up   = spread_bumpPV(+h / 2.0, "DOM");
            const double pv_down = spread_bumpPV(-h / 2.0, "DOM");
            return (pv_up - pv_down) / (bump);
        }
        throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
    }

    double CCSSensitivities::fx01(const double &bump) const {

        const double h = bump * 1e-4;
        const double pv_up   = spotFX_bumpPV(+h / 2.0);
        const double pv_down = spotFX_bumpPV(-h / 2.0);
        return (pv_up - pv_down) / bump;
    }

    std::vector<Core::Point> CCSSensitivities::bucket_DV01(const double bump, const std::string &whichCurve) const {

        const double h = bump * 1e-4;

        if (whichCurve == "FOR") {
            const std::size_t n = m_forCurve.n_pillars();
            const auto pillars = m_forCurve.get_pillars();
            std::vector<Core::Point> dv01;

            for (std::size_t k = 0; k < n; ++k) {
                const double pv_up   = one_bumpPV(+h / 2.0, k, "FOR");
                const double pv_down = one_bumpPV(-h / 2.0, k, "FOR");
                dv01.push_back( {pillars[k].time , (pv_up - pv_down) / bump} );
            }
            return dv01;
        }
        if (whichCurve == "DOM") {

            const std::size_t n = m_domCurve.n_pillars();
            const auto pillars = m_domCurve.get_pillars();
            std::vector<Core::Point> dv01;

            for (std::size_t k = 0; k < n; ++k) {
                const double pv_up   = one_bumpPV(+h / 2.0, k, "DOM");
                const double pv_down = one_bumpPV(-h / 2.0, k, "DOM");
                dv01.push_back( {pillars[k].time , (pv_up - pv_down) / (bump) });
            }
            return dv01;
        }
        throw std::invalid_argument("whichCurve should be 'FOR' or 'DOM'");
    }

    CCSSensitivities_results CCSSensitivities::sensitivities(const double bump, const double FxBump) const {
        CCSSensitivities_results res = {};
        res.DV01_FOR   = dv01_parallel(bump , "FOR") ;
        res.DV01_DOM   = dv01_parallel(bump, "DOM");
        res.DV01_total = res.DV01_FOR + res.DV01_DOM;
        res.SV01_FOR = sv01(bump, "FOR");
        res.SV01_DOM = sv01(bump, "DOM");
        res.FX01 = fx01(FxBump);
        res.DV01_buckets_FOR = bucket_DV01(bump, "FOR");
        res.DV01_buckets_DOM = bucket_DV01(bump, "DOM");
        return res;
    }



    }
