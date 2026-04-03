//
// Created by ricar on 30/03/2026.
//


#include "Produits.h"
#include "YieldCurve.h"
#include "Core/Dates.h"
#include <cmath>

namespace Market {

    double Deposit::implied_df() const {

        const double df = 1/(1+ maturity*rate);
        return df;
    }

    double Future::fra_market_rate() const {
        const double future_rate = (100.0 - price) / 100.0;
        const double conv_adj    = 0.5 * volatility * volatility * effective_date * maturity_date;
        return future_rate - conv_adj;
    }

    double Future::npv(const Market::YieldCurve &y_curve) const {

        const double fra_rate   = fra_market_rate();
        const double implied_fra_rate = y_curve.forward_rate(effective_date,maturity_date);
        return implied_fra_rate - fra_rate;
    }

    Swap::Schedule Swap::buildSchedule() const {

        Schedule s;

        const int n_fixed = static_cast<int>(std::round(maturity / fixedLeg_freq));
        for (int i = 0; i <= n_fixed; ++i) {
            double t = i * fixedLeg_freq;
            s.fixed.push_back(t);
        }

        const int n_floating = static_cast<int>(std::round(maturity / floatLeg_freq));
        for (int i = 0; i <= n_floating; ++i) {
            double t = i * floatLeg_freq;
            s.floating.push_back(t);
        }

        return s;
    }

    double Swap::npv(const Market::YieldCurve &y_curve, const Schedule &s) const {

        double pv_01 = 0.0;

        for (size_t i = 1; i < s.fixed.size(); ++i) {
            pv_01 += fixedLeg_freq * y_curve.discount(s.fixed[i]);
        }

        const double pv_floating =
            y_curve.discount(s.floating.front()) - y_curve.discount(s.floating.back());

        return fixedRate * pv_01 - pv_floating;
    }

    Core::Date CCSwap::get_end_date() const {
        return ValuationDate.add_months(12*maturity);
    }


}
