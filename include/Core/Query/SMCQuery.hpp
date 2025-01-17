#ifndef SMCQUERIES_H
#define SMCQUERIES_H

#include "AST.hpp"
#include <PQL/SMCExpressions.h>
#include "Core/TAPN/WatchExpression.hpp"

using VerifyTAPN::TAPN::Observable;

namespace VerifyTAPN::AST {

    struct SMCSettings {
        int timeBound;
        int stepBound;
        float falsePositives;
        float falseNegatives;
        float indifferenceRegionUp;
        float indifferenceRegionDown;
        float confidence;
        float estimationIntervalWidth;
        bool compareToFloat;
        float geqThan;

        static SMCSettings fromPQL(unfoldtacpn::PQL::SMCSettings settings);
    };

    class SMCQuery : public Query {
        public:

            SMCQuery(Quantifier quantifier, SMCSettings settings, Expression *expr) 
                : Query(quantifier, expr), smcSettings(settings)
            { };

            SMCQuery(const SMCQuery &other) 
                : Query(other.quantifier, other.expr->clone()), smcSettings(other.smcSettings)
            { };

            SMCQuery &operator=(const SMCQuery &other) {
                if (&other != this) {
                    delete expr;
                    expr = other.expr->clone();
                    smcSettings = other.smcSettings;
                }
                return *this;
            }

            virtual SMCQuery *clone() const;

            void accept(Visitor &visitor, Result &context) override;


            void setSMCSettings(SMCSettings newSettings) {
                smcSettings = newSettings;
            }

            SMCSettings& getSmcSettings() {
                return smcSettings;
            }

            std::vector<Observable>& getObservables() {
                return observables;
            }

        private:
            Quantifier quantifier;
            Expression *expr;
            SMCSettings smcSettings;
            std::vector<Observable> observables;
            
        };

}

#endif