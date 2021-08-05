/* 
 * File:   RangeVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 15 November 2020, 11.19
 */

#ifndef RANGEVISITOR_H
#define RANGEVISITOR_H

#include "Core/TAPN/TAPN.hpp"
#include "Core/QueryParser/Visitor.hpp"

#include "DiscreteVerification/DataStructures/NonStrictMarkingBase.hpp"
#include "DiscreteVerification/DataStructures/NonStrictMarking.hpp"

#include <tuple>

namespace VerifyTAPN {
    namespace DiscreteVerification {

        class RangeVisitor : public Visitor {
        private:
            const TAPN::TimedArcPetriNet &_tapn;
            const std::pair<uint32_t, uint32_t>* _bounds;
            const NonStrictMarkingBase& _parent;
        public:
            typedef SpecificResult<std::pair<int64_t, int64_t>> PairResult;
            explicit RangeVisitor(const TAPN::TimedArcPetriNet &tapn, const NonStrictMarkingBase& marking, const std::pair<uint32_t, uint32_t>* bounds)
            : _tapn(tapn), _bounds(bounds), _parent(marking) {};

            void visit(NotExpression &expr, Result &context) override;

            void visit(OrExpression &expr, Result &context) override;

            void visit(AndExpression &expr, Result &context) override;

            void visit(AtomicProposition &expr, Result &context) override;

            void visit(BoolExpression &expr, Result &context) override;

            void visit(Query &query, Result &context) override;

            void visit(DeadlockExpression &expr, Result &context) override;

            void visit(NumberExpression &expr, Result &context) override;

            void visit(IdentifierExpression &expr, Result &context) override;

            void visit(MultiplyExpression &expr, Result &context) override;

            void visit(MinusExpression &expr, Result &context) override;

            void visit(SubtractExpression &expr, Result &context) override;

            void visit(PlusExpression &expr, Result &context) override;
        };
    }
}
#endif /* RANGEVISITOR_H */

