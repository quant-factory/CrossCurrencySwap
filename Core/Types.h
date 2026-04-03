//
// Created by ricar on 30/03/2026.
//

#pragma once

#pragma once
#include <algorithm>
#include "Dates.h"

namespace Core {

    struct Point {
        double time;
        double value;
    };

    enum class DayCount { ACT_360, ACT_365, DC_30_360 };

    inline double year_fraction(const Core::Date& d1, const Core::Date& d2, const DayCount dc) {
        switch (dc) {
            case DayCount::ACT_360: return static_cast<double>(d2 - d1) / 360.0;
            case DayCount::ACT_365: return static_cast<double>(d2 - d1) / 365.0;
            case DayCount::DC_30_360: {
                auto [y1, m1, dd1] = Core::Date::fromJulian(d1.getJulianDays());
                auto [y2, m2, dd2] = Core::Date::fromJulian(d2.getJulianDays());
                dd1 = std::min(dd1, 30);
                if (dd1 == 30) dd2 = std::min(dd2, 30);
                return ((y2-y1)*360.0 + (m2-m1)*30.0 + (dd2-dd1)) / 360.0;
            }
        }
        return static_cast<double>(d2 - d1) / 365.0;
    }

    /*  CROSS CURRENCY SWAP RELATED TYPES */

    enum class TradeDirection {
        receive = +1 ,
        pay = -1,
    } ;

    enum class LegType {
        FLOAT,
        FIXED,
    };

    enum class Reset_Currency {
        foreign,
        domestic,
        none
    };

    struct FX_params {
        double spotFX ;
        std::string fgn_ccy;
        std::string dom_ccy;

    };

    struct Leg {

        std::string currency;
        double nominalValue;
        int frequency;
        bool isNotionalExchange;
        LegType type;
        double spread;
        double fix_rate;
        std::vector<Core::Date> payment_grid;

        [[nodiscard]] std::vector<Core::Date> buildGrid(const Core::Date& startDate, const Core::Date& endDate) const {

            std::vector<Core::Date> grid;
            Core::Date current = startDate;

            while (current < endDate) {
                grid.push_back(current);
                current = current.add_months(frequency);
            }

            grid.push_back(endDate);

            return grid;
        }
        
    };

}
