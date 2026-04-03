#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include "Core/Dates.h"
#include "Market/YieldCurve.h"
#include "Market/Produits.h"
#include "Pricers/CCSPricer.h"

int main()
{
    std::cout << std::fixed;

    std::cout << std::string(70, '*') << "\n";
    std::cout << "  Cross Currency Swap Pricer\n";
    std::cout << std::string(70, '*') << "\n";

    // =========================================================================
    // 1. YIELD CURVES
    // =========================================================================
    Core::Date valDate(2025, 1, 15);

    Market::YieldCurve eurCurve(valDate, "EUR");
    Market::YieldCurve usdCurve(valDate, "USD");

    std::vector<double> times = {
        0.00274, 0.0833, 0.25,  0.50,  0.75,
        1.0,     2.0,    3.0,   4.0,   5.0,
        7.0,     10.0,   12.0,  15.0,  20.0
    };

    std::vector<double> eur_dfs = {
        0.999910, 0.997200, 0.992100, 0.984500, 0.977000,
        0.969800, 0.945000, 0.920500, 0.897000, 0.875200,
        0.830000, 0.780500, 0.745000, 0.700200, 0.610000
    };

    std::vector<double> usd_dfs = {
        0.999820, 0.994900, 0.984200, 0.969000, 0.953500,
        0.939000, 0.900500, 0.860000, 0.820000, 0.785000,
        0.720000, 0.640000, 0.600000, 0.545000, 0.450000
    };

    for (size_t i = 0; i < times.size(); ++i) {
        eurCurve.add_pillar(times[i], eur_dfs[i]);
        usdCurve.add_pillar(times[i], usd_dfs[i]);
    }

    // =========================================================================
    // 2. LEGS
    // =========================================================================

    Core::Date endDate = valDate.add_months(12*5);

    Core::Leg eurLeg;
    eurLeg.currency           = "EUR";
    eurLeg.nominalValue       = 876962.0;
    eurLeg.frequency          = 3;
    eurLeg.isNotionalExchange = true;
    eurLeg.type               = Core::LegType::FLOAT;
    eurLeg.spread             = 0.00;
    eurLeg.fix_rate           = 0.0;
    eurLeg.payment_grid       = eurLeg.buildGrid(valDate, endDate);

    Core::Leg usdLeg;
    usdLeg.currency           = "USD";
    usdLeg.nominalValue       = 1'000'000.0;
    usdLeg.frequency          = 3;
    usdLeg.isNotionalExchange = true;
    usdLeg.type               = Core::LegType::FLOAT;
    usdLeg.spread             = 0.00;
    usdLeg.fix_rate           = 0.0;
    usdLeg.payment_grid       = usdLeg.buildGrid(valDate, endDate);

    // =========================================================================
    // 3. GRID PRINT (FIX ICI)
    // =========================================================================
    auto printGrid = [&](const std::string& label, const std::vector<Core::Date>& grid) {
        std::cout << "\n--- " << label << " (" << grid.size() << " dates) ---\n";

        for (const auto& d : grid) {
            auto [y, m, dd] = Core::Date::fromJulian(d.getJulianDays());

            std::cout << "  " << y << "-"
                      << std::setw(2) << std::setfill('0') << m << "-"
                      << std::setw(2) << std::setfill('0') << dd << "\n";
        }

        std::cout << std::setfill(' '); //
    };

    printGrid("EUR leg grid", eurLeg.payment_grid);
    printGrid("USD leg grid", usdLeg.payment_grid);

    // =========================================================================
    // 4. PRICER
    // =========================================================================
    Market::CCSwap swap;
    swap.ValuationDate = valDate;
    swap.maturity      = 5.0;
    swap.phi           = Core::TradeDirection::receive;
    swap.isMtm         = true;
    swap.reset         = Core::Reset_Currency::domestic;
    swap.for_leg       = eurLeg;
    swap.dom_leg       = usdLeg;
    swap.fx_params     = { 1.200,"EUR", "USD" };
    swap.valuation_ccy = "USD";
    swap.convention    = Core::DayCount::ACT_360;

    Pricer::CCSPricer pricer(swap, eurCurve, usdCurve);
    bool reset = pricer.isResetCurrency("EUR");
    std::cout << " Euro is Reset currency : " << reset;
    auto res = pricer.pricing();

    // =========================================================================
    // 5. OUTPUT (nickel maintenant)
    // =========================================================================
    auto printRow = [](const std::string& label, double value,
                       const std::string& unit = "", int width = 35) {

        std::cout << "  "
                  << std::left  << std::setw(width) << label
                  << std::right << std::setw(15)
                  << std::fixed << std::setprecision(3)
                  << value
                  << "  " << unit << "\n";
    };

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  PV Decomposition\n";
    std::cout << std::string(70, '=') << "\n";

    printRow("PV(Cpn,  EUR leg)", res.PV_Cpn_FOR,   "USD");
    printRow("PV(Exch, EUR leg)", res.PV_Exch_FOR,  "USD");
    printRow("PV(Reset,EUR leg)", res.PV_Reset_FOR, "USD");
    printRow("PV(EUR leg)",       res.PV_FOR,       "USD");

    printRow("PV(Cpn,  USD leg)", res.PV_Cpn_DOM,   "USD");
    printRow("PV(Exch, USD leg)", res.PV_Exch_DOM,  "USD");
    printRow("PV(Reset,USD leg)", res.PV_Reset_DOM, "USD");
    printRow("PV(USD leg)",       res.PV_DOM,       "USD");

    std::cout << "\n  --- Total ---\n";
    printRow("PV(CCS)", res.PV_Xccy, "USD");
    printRow("Par spread DOM", res.parSpread_DOM*1e4, "bp");
    printRow("Par spread FOR", res.parSpread_FOR*1e4, "bp");


    std::cout << "\n" << std::string(70, '*') << "\n";
    return 0;
}