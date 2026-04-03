//
// Created by ricar on 30/03/2026.
//

#include "CCSPricer.h"
#include "Core/Dates.h"
#include "Core/Types.h"
#include "Market/Produits.h"
#include "Market/YieldCurve.h"

namespace Pricer {

    CCSPricer::CCSPricer(const Market::CCSwap &params,
                            const Market::YieldCurve &forCurve,
                            const Market::YieldCurve &domCurve) :
    m_params(params),
    m_forCurve(forCurve),
    m_domCurve(domCurve)

    {
        const std::string& c1 = forCurve.get_currency();
        const std::string& c2 = domCurve.get_currency();
        const std::string& cFOR = params.for_leg.currency;
        const std::string& cDOM = params.dom_leg.currency;

        if (!((c1 == cFOR && c2 == cDOM) || (c1 == cDOM && c2 == cFOR)))

            throw std::invalid_argument(
                "CCSPricer : Curves currencies (" + c1 + ", " + c2 +
                ") doesn't correspond to leg denomination (" + cFOR + ", " + cDOM + ")");
    }

    const Market::YieldCurve &CCSPricer::curveFor(const std::string &ccy) const {

        if (m_forCurve.get_currency() == ccy) return m_forCurve;
        if (m_domCurve.get_currency() == ccy) return m_domCurve;
        throw std::invalid_argument("curveFor : None curve for the currency : " + ccy);

    }


    bool CCSPricer::isResetCurrency(const std::string& ccy) const
    {
        if (!m_params.isMtm) return false;

        switch (m_params.reset) {
            case Core::Reset_Currency::foreign:  return (ccy == m_params.for_leg.currency);
            case Core::Reset_Currency::domestic: return (ccy == m_params.dom_leg.currency);
            case Core::Reset_Currency::none:     return false;
        }
        return false;
    }

    double CCSPricer::forwardFX(const double t) const {
        const Market::YieldCurve& curFOR = curveFor(m_params.for_leg.currency);
        const Market::YieldCurve& curDOM = curveFor(m_params.dom_leg.currency);

        const double spot   = m_params.fx_params.spotFX;
        const double df_for = curFOR.discount(t);
        const double df_dom = curDOM.discount(t);

        if (df_dom < 1e-15)
            throw std::runtime_error("forwardFX : Discount factor of domestic currency almost zero in t=" +
                                     std::to_string(t));

        return spot * df_for / df_dom;
    }


    double CCSPricer::alpha(const std::string &legCcy) const {

        const std::string& valCcy = m_params.valuation_ccy;
        const std::string& forCcy = m_params.for_leg.currency;
        const std::string& domCcy = m_params.dom_leg.currency;
        const double       spot   = m_params.fx_params.spotFX;

        if (legCcy == valCcy) {
            return 1.0;
        }

        else if (legCcy == forCcy) {
            return spot;
        }

        else if (legCcy == domCcy) {
            if (std::abs(spot) < 1e-15)
                throw std::runtime_error("alpha : spot FX almost zero");
            return 1.0 / spot;
        }

        throw std::invalid_argument("alpha : Unknown currency for the  : " + legCcy);
    }


    double CCSPricer::beta(const Core::Date date, const std::string &legCcy) const {

        if (!isResetCurrency(legCcy))
            return 1.0;

        const double t = Core::year_fraction(m_params.ValuationDate , date , m_params.convention);
        const double spot    = m_params.fx_params.spotFX;
        const double fwd     = forwardFX(t);
        const std::string& forCcy = m_params.for_leg.currency;
        const std::string& domCcy = m_params.dom_leg.currency;

        if (std::abs(spot) < 1e-15)
            throw std::runtime_error("beta : Sport FX almost equal to zero ");

        if (legCcy == forCcy) {
            return fwd / spot;
        }
        else if (legCcy == domCcy) {
            if (std::abs(fwd) < 1e-15)
                throw std::runtime_error("beta : forward FX almost zero in t =" + std::to_string(t));
            return spot / fwd;
        }

        throw std::invalid_argument("beta : Unknown currency : " + legCcy);
    }


    double CCSPricer::psi(const Core::Date date, const std::string &legCcy) const {
        return alpha(legCcy) * beta(date, legCcy);
    }

    double CCSPricer::pvCoupons(const Core::Leg &leg, const Market::YieldCurve &curve) const {

        double pv = 0.0;
        const Core::Date end_date = m_params.get_end_date();
        const std::vector<Core::Date >grid = leg.buildGrid(m_params.ValuationDate,end_date);

        for (int i = 0; i < grid.size() - 1; ++i) {

            const double dt = Core::year_fraction(grid[i] , grid[i + 1] ,m_params.convention );
            double const t0 = Core::year_fraction(m_params.ValuationDate , grid[i] , m_params.convention );
            double const t1 = Core::year_fraction(m_params.ValuationDate , grid[i+1] , m_params.convention );
            double const df = curve.discount(t1) ;
            double const fwd = curve.forward_rate(t0,t1);

            if (leg.type == Core::LegType::FLOAT) {
                pv += leg.nominalValue * psi(grid[i+1] , leg.currency ) * (fwd +leg.spread) * dt * df;
            }

            else {
                pv += leg.nominalValue * psi(grid[i+1] , leg.currency ) * leg.fix_rate * dt * df;
            }

        }
        return pv;
    }

    double CCSPricer::pvExchanges(const Core::Leg &leg, const Market::YieldCurve &curve) const {

        if (!leg.isNotionalExchange) {
            return 0.0;
        }

        const Core::Date end_date = m_params.get_end_date();
        const double N_t_0 = leg.nominalValue;
        const double psi_t_0 = psi(m_params.ValuationDate , leg.currency);
        const double psi_t_n = psi(end_date, leg.currency);
        const double t1 = Core::year_fraction(m_params.ValuationDate , end_date , m_params.convention);
        const double df_n = curve.discount(t1);
        return N_t_0 * (psi_t_n * df_n - psi_t_0) ;

    }

    double CCSPricer::pvResets(const Core::Leg &leg, const Market::YieldCurve &curve) const {

        if (!m_params.isMtm)
            return 0.0;

        if (!isResetCurrency(leg.currency)) return 0.0;

        const Core::Date end_date = m_params.get_end_date();
        const auto grid = leg.buildGrid(m_params.ValuationDate,end_date);
        const std::size_t m = grid.size();

        if (m < 2) return 0.0;

        double pv = 0.0;
        const double N_t0 = leg.nominalValue;

        for (std::size_t j = 0; j < m - 2; ++j)
        {
            const double t_j1   = Core::year_fraction(m_params.ValuationDate, grid[j+1], m_params.convention);
            const double Psi_j1 = psi(grid[j+1], leg.currency);
            const double Psi_j2 = psi(grid[j+2], leg.currency);
            const double df_j1  = curve.discount(t_j1);

            pv += N_t0 * (Psi_j1 - Psi_j2) * df_j1;
        }

        return pv;

    }

    double CCSPricer::pvLeg(const Core::Leg &leg, const Market::YieldCurve &curve) const {

        return pvCoupons(leg, curve)
         + pvExchanges(leg, curve)
         + pvResets(leg, curve);
    }


    double CCSPricer::annuity(const Core::Leg &leg, const Market::YieldCurve &curve) const {

        double an = 0.0;
        const double N_t0 = leg.nominalValue;
        const auto grid = leg.buildGrid(m_params.ValuationDate,m_params.get_end_date());
        std::size_t m = grid.size();


        for (std::size_t j = 1; j < m; ++j) {

            const double t_j   = Core::year_fraction(m_params.ValuationDate , grid[j] , m_params.convention);
            const double df    = curve.discount(t_j);
            const double Psi_j = psi(grid[j], leg.currency);
            const double dt = Core::year_fraction(grid[j-1],grid[j],m_params.convention);
            an += N_t0 * Psi_j * dt * df;
        }

        return an;

    }

    double CCSPricer::pvOnlyInternal(const Market::CCSwap &params, const Market::YieldCurve &curveA, const Market::YieldCurve &curveB) {

        const Market::YieldCurve& forCurve = (curveA.get_currency() ==  params.for_leg.currency) ? curveA : curveB;
        const Market::YieldCurve& domCurve = (curveB.get_currency() == params.dom_leg.currency) ? curveB : curveA;

        const CCSPricer pricer(params, curveA, curveB);

        const auto phi = static_cast<double>(params.phi);

        const double pv_for = pricer.pvCoupons  (params.for_leg, forCurve)
                      + pricer.pvExchanges(params.for_leg, forCurve)
                      + pricer.pvResets   (params.for_leg, forCurve);

        const double pv_dom = pricer.pvCoupons  (params.dom_leg, domCurve)
                      + pricer.pvExchanges(params.dom_leg, domCurve)
                      + pricer.pvResets   (params.dom_leg, domCurve);

        return phi * (pv_for - pv_dom);

    }


    double CCSPricer::parSpread(const std::string &targetLeg) const {

        Market::CCSwap params_copy = m_params ;

        if (targetLeg == "FOR") {
            params_copy.for_leg.spread =  0 ;
        }

        else if (targetLeg == "DOM") {
            params_copy.dom_leg.spread =  0 ;
        }

        else {
            throw std::invalid_argument("parSpread : targetLeg should be either 'FOR' or 'DOM'");
        }

        const double pvZero = pvOnlyInternal(params_copy, m_forCurve, m_domCurve);
        const double an     = annuity((targetLeg == "FOR") ? params_copy.for_leg : params_copy.dom_leg,
                                 curveFor((targetLeg == "FOR") ? m_params.for_leg.currency
                                                               : m_params.dom_leg.currency));

        if (std::abs(an) < 1e-10)

            throw std::runtime_error("parSpread : Annuity almost zero for " + targetLeg);

        return -pvZero / an;
    }


    double CCSPricer::parRate(const std::string &targetLeg) const {

        Market::CCSwap params_copy = m_params;

        if (targetLeg == "FOR") {
            params_copy.for_leg.fix_rate =  0 ;
        }

        else if (targetLeg == "DOM") {
            params_copy.dom_leg.fix_rate =  0 ;
        }

        else {
            throw std::invalid_argument("parRate : targetLeg should be 'FOR' ou 'DOM'");
        }

        const double pvZero_r = pvOnlyInternal(params_copy, m_forCurve, m_domCurve);
        const double an_r     = annuity((targetLeg == "FOR") ? params_copy.for_leg : params_copy.dom_leg,
                                 curveFor((targetLeg == "FOR") ? m_params.for_leg.currency
                                                               : m_params.dom_leg.currency));

        if (std::abs(an_r) < 1e-10)

            throw std::runtime_error("parSpread : Annuity almost zero for " + targetLeg);

        return -pvZero_r / an_r;

    }

    Market::CCSwap::CCS_results CCSPricer::pricing() const {

        Market::CCSwap::CCS_results res{};

        const Market::YieldCurve& forCurve = curveFor(m_params.for_leg.currency);
        const Market::YieldCurve& domCurve = curveFor(m_params.dom_leg.currency);

        res.PV_Cpn_FOR   = pvCoupons(m_params.for_leg, forCurve);
        res.PV_Exch_FOR  = pvExchanges(m_params.for_leg, forCurve);
        res.PV_Reset_FOR = pvResets   (m_params.for_leg, forCurve);
        res.PV_FOR       = res.PV_Cpn_FOR + res.PV_Exch_FOR + res.PV_Reset_FOR;

        // --- DOM Leg ---
        res.PV_Cpn_DOM   = pvCoupons  (m_params.dom_leg, domCurve);
        res.PV_Exch_DOM  = pvExchanges(m_params.dom_leg, domCurve);
        res.PV_Reset_DOM = pvResets   (m_params.dom_leg, domCurve);
        res.PV_DOM       = res.PV_Cpn_DOM + res.PV_Exch_DOM + res.PV_Reset_DOM;

        // --- PV total  ---
        res.PV_Xccy = static_cast<double>(m_params.phi) * (res.PV_FOR - res.PV_DOM);

        // --- Annuities ---
        res.AN_FOR = annuity(m_params.for_leg, forCurve);
        res.AN_DOM = annuity(m_params.dom_leg, domCurve);

        // --- Par spreads ---

        res.parSpread_FOR = (std::abs(res.AN_FOR) > 1e-10) ? parSpread("FOR") : 0.0;
        res.parSpread_DOM = (std::abs(res.AN_DOM) > 1e-10) ? parSpread("DOM") : 0.0;

        // --- Par rates ---
        res.parRate_FOR = (std::abs(res.AN_FOR) > 1e-10) ? parRate("FOR") : 0.0;
        res.parRate_DOM = (std::abs(res.AN_DOM) > 1e-10) ? parRate("DOM") : 0.0;

        return res;

    }



}
