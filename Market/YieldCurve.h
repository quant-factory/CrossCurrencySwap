//
// Created by ricar on 30/03/2026.
//

#pragma once

#include"Core/numerics.h"
#include "Market/Produits.h"
#include "Core/Types.h"
#include <utility>
#include <vector>


namespace Core {
    class Date;
 }


namespace Market {


    class YieldCurve {

    public:

        explicit YieldCurve(const Core::Date refDate , std::string  ccy ) : m_ccy(std::move(ccy)) , m_refDate(refDate) {

        }

        void add_pillar(double t, double df);
        void pop_pillar();
        [[nodiscard]] std::string get_currency() const;
        [[nodiscard]] double discount(double t) const;
        [[nodiscard]] double forward_rate(double t1, double t2) const;

        YieldCurve parallel_bump(double bump) const;
        YieldCurve pillar_bump( double bump, std::size_t idx) const;
        std::vector<Core::Point> get_pillars() const;
        std::size_t n_pillars() const;


    private:
        const std::string m_ccy;
        Core::Date m_refDate{}  ;
        std::vector<Core::Point> m_pillars {};

    };


    class YieldCurveBoot {
    public:

        explicit YieldCurveBoot(const Core::Date refDate , const std::string &ccy)
        : m_curve(refDate, ccy) , m_brent(1e-8,100) {

        }

        void add_deposit(const Deposit& dep) {
            m_curve.add_pillar(dep.maturity, dep.implied_df());
        }

        void add_future(const Future& fut) {

            const double df = solve_df(fut.maturity_date, [&](const YieldCurve& yc)-> double {
                return fut.npv(yc);
            } );

            m_curve.add_pillar(fut.maturity_date, df);
        }

        void add_swap(const Market::Swap& sw) {
            const auto sched = sw.buildSchedule();
            const double df  = solve_df(sw.maturity, [&](const YieldCurve& yc)->double {
                return sw.npv(yc,sched);
            });
            m_curve.add_pillar(sw.maturity, df);
        }

        void add_deposits(const std::vector<Deposit>&   deps)  { for (auto& d : deps)  add_deposit(d); }
        void add_futures (const std::vector<Future>& futures)  { for (auto& f : futures)  add_future(f);  }
        void add_swaps   (const std::vector<Swap>&        swaps) { for (auto& s : swaps) add_swap(s);    }

        [[nodiscard]] const YieldCurve& curve() const { return m_curve; }




    private :

    Market::YieldCurve m_curve;
        Core::Brent m_brent ;

        template<typename F>
        double solve_df(const double maturity, F&& objective) {

            auto try_df = [&](const double df_guess) -> double {
                m_curve.add_pillar(maturity, df_guess);
                const double val = objective(m_curve);
                m_curve.pop_pillar();
                return val;
            };

            const double df_sol = m_brent.solve(try_df, 1e-12, 1);
            return df_sol;
        }


    };



}

