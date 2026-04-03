#include <iostream>
#include <iomanip>
#include <cmath>
#include "Core/Dates.h"
#include "Core/Types.h"
#include "Market/YieldCurve.h"
#include "Market/Produits.h"
#include "Pricers/CCSPricer.h"
#include "Pricers/sensibilities.h"

int main()
{
    // ---------------------------------------------------------------
    // 1. DATES
    // ---------------------------------------------------------------

    const auto today  = Core::Date(2026, 1,  5);

    Market::YieldCurve jpyCurve(today, "JPY");
    jpyCurve.add_pillar(0.25,  std::exp(-0.0030 * 0.25));
    jpyCurve.add_pillar(0.50,  std::exp(-0.0040 * 0.50));
    jpyCurve.add_pillar(1.00,  std::exp(-0.0055 * 1.00));
    jpyCurve.add_pillar(2.00,  std::exp(-0.0075 * 2.00));
    jpyCurve.add_pillar(3.00,  std::exp(-0.0095 * 3.00));
    jpyCurve.add_pillar(5.00,  std::exp(-0.0130 * 5.00));
    jpyCurve.add_pillar(7.00,  std::exp(-0.0160 * 7.00));
    jpyCurve.add_pillar(10.00, std::exp(-0.0185 * 10.00));

    Market::YieldCurve usdCurve(today, "USD");
    usdCurve.add_pillar(0.25,  std::exp(-0.0380 * 0.25));
    usdCurve.add_pillar(0.50,  std::exp(-0.0400 * 0.50));
    usdCurve.add_pillar(1.00,  std::exp(-0.0420 * 1.00));
    usdCurve.add_pillar(2.00,  std::exp(-0.0445 * 2.00));
    usdCurve.add_pillar(3.00,  std::exp(-0.0465 * 3.00));
    usdCurve.add_pillar(5.00,  std::exp(-0.0490 * 5.00));
    usdCurve.add_pillar(7.00,  std::exp(-0.0510 * 7.00));
    usdCurve.add_pillar(10.00, std::exp(-0.0530 * 10.00));

    Market::CCSwap ccs;

    ccs.for_leg.currency        = "JPY";
    ccs.for_leg.type            = Core::LegType::FLOAT;
    ccs.for_leg.nominalValue    = 15'000'000.0;
    ccs.for_leg.spread          = 0.0020;
    ccs.for_leg.fix_rate        = 0.0;
    ccs.for_leg.frequency       = 3;
    ccs.for_leg.isNotionalExchange = true;

    ccs.dom_leg.currency        = "USD";
    ccs.dom_leg.type            = Core::LegType::FIXED;
    ccs.dom_leg.nominalValue    = 100'000.0;
    ccs.dom_leg.spread          = 0.0;
    ccs.dom_leg.fix_rate        = 0.0450;
    ccs.dom_leg.frequency       = 3;
    ccs.dom_leg.isNotionalExchange = true;

    ccs.ValuationDate   = today;
    ccs.maturity = 5.0;
    ccs.phi = Core::TradeDirection::receive;
    ccs.isMtm  = true;
    ccs.reset  = Core::Reset_Currency::domestic;
    ccs.fx_params = {1/150.00 , "JPY" , "USD"}; // How much USD for 1 JPY
    ccs.valuation_ccy   = "USD";
    ccs.convention      = Core::DayCount::ACT_360;

    // ---------------------------------------------------------------
    // 5. PRICING
    // ---------------------------------------------------------------

    const Pricer::CCSPricer pricer(ccs, jpyCurve, usdCurve);
    const auto res = pricer.pricing();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "========== CCS Pricing (USD/JPY MtM) ==========\n";
    std::cout << "PV Coupons  FOR (JPY) : " << res.PV_Cpn_FOR   << "\n";
    std::cout << "PV Exchange FOR (JPY) : " << res.PV_Exch_FOR  << "\n";
    std::cout << "PV Resets   FOR (JPY) : " << res.PV_Reset_FOR << "\n";
    std::cout << "PV FOR total          : " << res.PV_FOR        << "\n\n";
    std::cout << "PV Coupons  DOM (USD) : " << res.PV_Cpn_DOM   << "\n";
    std::cout << "PV Exchange DOM (USD) : " << res.PV_Exch_DOM  << "\n";
    std::cout << "PV Reset    DOM (USD) : " << res.PV_Reset_DOM << "\n";
    std::cout << "PV DOM total          : " << res.PV_DOM        << "\n\n";
    std::cout << "PV Xccy  (USD)        : " << res.PV_Xccy       << "\n";
    std::cout << "Annuity FOR           : " << res.AN_FOR         << "\n";
    std::cout << "Annuity DOM           : " << res.AN_DOM         << "\n";
    std::cout << "Par Spread FOR (bps)  : " << res.parSpread_FOR * 1e4 << "\n";
    std::cout << "Par Spread DOM (bps)  : " << res.parSpread_DOM * 1e4 << "\n";
    std::cout << "Par Rate   FOR        : " << res.parRate_FOR   << "\n";
    std::cout << "Par Rate   DOM        : " << res.parRate_DOM   << "\n";

    // ---------------------------------------------------------------
    // 6. SENSITIVITIES   (bump = 1bp)
    // ---------------------------------------------------------------

    const Pricer::CCSSensitivities sens(ccs, jpyCurve, usdCurve);

    const auto s = sens.sensitivities(1.0 , 1.0 );

    std::cout << "\n========== Sensitivities ==========\n";

    std::cout << "DV01 FOR  (USD/bp) : " << s.DV01_FOR   << "\n";
    std::cout << "DV01 DOM  (USD/bp) : " << s.DV01_DOM   << "\n";
    std::cout << "DV01 total(USD/bp) : " << s.DV01_total << "\n";
    std::cout << "SV01 FOR  (USD/bp) : " << s.SV01_FOR   << "\n";
    std::cout << "SV01 DOM  (USD/bp) : " << s.SV01_DOM   << "\n";
    std::cout << "FX01      (USD/%)  : " << s.FX01       << "\n";

    std::cout << "\n========== Sanity checks ==========\n";

    Market::CCSwap atm = ccs;
    atm.for_leg.spread = res.parSpread_FOR;
    const Pricer::CCSPricer atmPricer(atm, jpyCurve, usdCurve);
    const auto atmRes = atmPricer.pricing();

    std::cout << "PV at parSpread_FOR (should be ~0) : " << atmRes.PV_Xccy << "\n";


    return 0;
}