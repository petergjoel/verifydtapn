#ifndef WATCH_EXPR_HPP_
#define WATCH_EXPR_HPP_

#include "Core/Query/AST.hpp"

namespace VerifyTAPN::DiscreteVerification {
    class RealMarking;
}
using VerifyTAPN::DiscreteVerification::RealMarking;

using VerifyTAPN::AST::ArithmeticExpression;

namespace VerifyTAPN::TAPN {

    class Watch {
        friend class WatchAggregator;

        protected:
            ArithmeticExpression* _expr;
            std::vector<float> _values;
            std::vector<float> _timestamps;
            TimedArcPetriNet* _tapn;

        public:
            Watch(TimedArcPetriNet* tapn, ArithmeticExpression* expr) : _expr(expr), _tapn(tapn) { }

            float new_marking(RealMarking* marking);

            std::string get_plots(const std::string& name) const;

            void reset();

            size_t n_values() const;
    };

    class WatchAggregator {

        public:

            void new_watch(Watch* watch);

            void aggregate(unsigned int stepBins, unsigned int timeBins);
            void reset();

            std::string get_plots(const std::string& name) const;

            int n_watchs = 0;
            std::vector<float> step_bins;
            std::vector<float> step_avg;
            std::vector<float> step_min;
            std::vector<float> step_max;
            std::vector<float> step_runs_count;
            std::vector<float> time_values;
            std::vector<float> time_avg;
            std::vector<float> time_min;
            std::vector<float> time_max;
            std::vector<float> timestamps;

            float global_steps_avg = 0.0;
            float global_time_avg = 0.0;
    };

    using Observable = std::tuple<std::string, ArithmeticExpression*>;

    void printPlot(std::stringstream& stream, const std::string& title, const std::vector<float>& x, const std::vector<float> y);
    void printIntXPlot(std::stringstream& stream, const std::string& title, const std::vector<float> y);

}

#endif