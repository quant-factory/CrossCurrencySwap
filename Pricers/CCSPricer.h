//
// Created by ricar on 30/03/2026.
//

#pragma once

#pragma once
#include "Market/Produits.h"
#include "Core/Types.h"

namespace Market {
    class YieldCurve;
}

namespace Pricer {


    class CCSPricer {

    public :

    CCSPricer(const Market::CCSwap &params ,
               const Market::YieldCurve &forCurve,
               const Market::YieldCurve &domCurve);

        [[nodiscard]] Market::CCSwap::CCS_results pricing() const ;

        [[nodiscard]] double forwardFX(double t) const ;

        [[nodiscard]] double alpha(const std::string & legCcy) const ;
        [[nodiscard]] double beta(Core::Date date, const std::string & legCcy)  const;
        [[nodiscard]] double psi(Core::Date date, const std::string& legCcy) const;

        [[nodiscard]] double pvCoupons(const Core::Leg &leg, const Market::YieldCurve &curve) const;
        [[nodiscard]] double pvExchanges(const Core::Leg& leg, const Market::YieldCurve& curve) const;
        [[nodiscard]] double pvResets(const Core::Leg& leg, const Market::YieldCurve& curve) const;
        [[nodiscard]] double pvLeg(const Core::Leg& leg, const Market::YieldCurve& curve) const;


        [[nodiscard]] double annuity(const Core::Leg& leg, const Market::YieldCurve& curve) const;

        [[nodiscard]] double parSpread(const std::string& targetLeg) const;
        [[nodiscard]] double parRate(const std::string& targetLeg) const;

        [[nodiscard]] const Market::YieldCurve& curveFor(const std::string& ccy) const;
        [[nodiscard]] bool isResetCurrency(const std::string& ccy) const ;


    private:

        const Market::CCSwap & m_params;
        const Market::YieldCurve&    m_forCurve;
        const Market::YieldCurve&    m_domCurve;


        [[nodiscard]] static double pvOnlyInternal(const Market::CCSwap &params, const Market::YieldCurve &curveA, const Market::YieldCurve &curveB);

    };



}