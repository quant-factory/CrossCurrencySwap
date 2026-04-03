//
// Created by ricar on 30/03/2026.
//

#pragma once

#pragma once
#include <string>
#include <vector>
#include "Core/Dates.h"
#include "Core/Types.h"


namespace Market {

    class YieldCurve;

    struct Deposit {
        double maturity;
        double rate;
        [[nodiscard]] double implied_df() const;
    };

    struct Future {
        double effective_date;
        double maturity_date;
        double price;
        double volatility;
        [[nodiscard]] double fra_market_rate() const;
        [[nodiscard]] double npv(const Market::YieldCurve &y_curve)const ;
    };

    struct Swap {
        double maturity;
        double fixedRate;
        double fixedLeg_freq;
        double floatLeg_freq;

        struct Schedule {
            std::vector<double> fixed;
            std::vector<double> floating;
        };

        [[nodiscard]] Schedule buildSchedule() const;

        [[nodiscard]] double npv(const Market::YieldCurve &y_curve , const Schedule &s) const;

    };


    struct CCSwap {
        Core::Date ValuationDate;
        double maturity ;
        Core::TradeDirection phi;
        bool isMtm;
        Core::Reset_Currency reset;
        Core::Leg for_leg;
        Core::Leg dom_leg;
        Core::FX_params fx_params;
        std::string valuation_ccy ;
        Core::DayCount convention;

        [[nodiscard]] Core::Date get_end_date() const ;

        struct CCS_results {
            double PV_Xccy;
            double PV_Cpn_FOR;
            double PV_Exch_FOR;
            double PV_Reset_FOR;
            double PV_FOR;
            double PV_Cpn_DOM;
            double PV_Exch_DOM;
            double PV_Reset_DOM;
            double PV_DOM;
            double AN_FOR;
            double AN_DOM;
            double parSpread_FOR;
            double parSpread_DOM;
            double parRate_FOR;
            double parRate_DOM;
        };

    };



}