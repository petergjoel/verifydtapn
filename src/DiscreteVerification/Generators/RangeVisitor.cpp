/* 
 * File:   RangeVisitor.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 15 November 2020, 11.19
 */

#include "DiscreteVerification/Generators/RangeVisitor.h"

namespace VerifyTAPN {
    namespace DiscreteVerification {

        void RangeVisitor::visit(NotExpression &expr, Result &context)
        {
            expr.getChild().accept(*this, context);
            auto& val = static_cast<IntResult&>(context);
            val.value *= -1;
        }

        void RangeVisitor::visit(OrExpression &expr, Result &context) {
            expr.getLeft().accept(*this, context);
            auto& val = static_cast<IntResult&>(context);
            if(val.value == 1) return; // true
            auto old = val.value;
            expr.getRight().accept(*this, context);
            if(val.value == 1) return; // true
            if(val.value == -1 && old == -1) return; // false;
            val.value = 0; // otherwise undecided
        }

        void RangeVisitor::visit(AndExpression &expr, Result &context) {
            expr.getLeft().accept(*this, context);
            auto& val = static_cast<IntResult&>(context);
            if(val.value == -1) return; // false
            auto old = val.value;
            expr.getRight().accept(*this, context);
            if(val.value == -1) return; // false
            if(val.value == 1 && old == 1) return; // true;
            val.value = 0; // otherwise undecided
        }

        void RangeVisitor::visit(AtomicProposition &expr, Result &context) {
            auto& val = static_cast<IntResult&>(context);
            PairResult ctxt;
            expr.getLeft().accept(*this, ctxt);
            auto lv = ctxt.value;
            expr.getLeft().accept(*this, ctxt);
            auto rv = ctxt.value;
            switch (expr.getOperator()) {
                case AtomicProposition::LT:
                {
                    if (lv.second < rv.first)
                        val.value = 1;
                    else if (lv.first > rv.second)
                        val.value = -1;
                    else
                        val.value = 0;
                    break;
                }
                case AtomicProposition::LE:
                {
                    if (lv.second <= rv.first)
                        val.value = 1;
                    else if (lv.first >= rv.second)
                        val.value = -1;
                    else
                        val.value = 0;
                    break;
                }
                case AtomicProposition::EQ: {
                    if (lv.second < rv.first)
                        val.value = -1;
                    else if (lv.first > rv.second)
                        val.value = -1;
                    else if (lv.first == lv.second && rv.first == rv.second && lv.first == rv.first)
                        val.value = 1;
                    else
                        val.value = 0;
                    break;
                }
                case AtomicProposition::NE:  {
                    if (lv.second < rv.first)
                        val.value = 1;
                    else if (lv.first > rv.second)
                        val.value = 1;
                    else if (lv.first == lv.second && rv.first == rv.second && lv.first == rv.first)
                        val.value = -1;
                    else
                        val.value = 0;
                    break;
                }
                default: assert(false);
            }
        }

        void RangeVisitor::visit(BoolExpression &expr, Result &context) {
            auto& val = static_cast<IntResult&>(context);
            if(expr.getValue())
                val.value = 1;
            else
                val.value = -1;
        }

        void RangeVisitor::visit(Query &query, Result &context) {
            query.getChild()->accept(*this, context);
        }

        void RangeVisitor::visit(DeadlockExpression &expr, Result &context) {
            auto& val = static_cast<IntResult&>(context);
            bool some_enabled = true;
            for(auto* trans : _tapn.getTransitions())
            {
                bool can_enable = true;
                bool always_enabled = true;
                for(auto* arc : trans->getInhibitorArcs())
                {
                    if(arc->getWeight() <= _bounds[arc->getInputPlace().getIndex()].first)
                        can_enable = false;
                    if(arc->getWeight() <= _bounds[arc->getInputPlace().getIndex()].second)
                        always_enabled = false;
                }
                for(auto* arc : trans->getPreset())
                {
                    if(arc->getWeight() > _bounds[arc->getInputPlace().getIndex()].second)
                        can_enable = false;
                    if(arc->getWeight() > _bounds[arc->getInputPlace().getIndex()].first)
                        always_enabled = false;
                }
                for(auto* arc : trans->getTransportArcs())
                {
                    if(arc->getWeight() > _bounds[arc->getSource().getIndex()].second)
                        can_enable = false;
                    if(arc->getWeight() > _bounds[arc->getSource().getIndex()].first)
                        always_enabled = false;
                }
                some_enabled &= can_enable;
                if(always_enabled)
                {
                    val.value = -1;
                    return;
                }
            }
            if(!some_enabled)
                val.value = 1;
            else
                val.value = 0; // unknown
        }

        void RangeVisitor::visit(NumberExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            val.value = std::make_pair<int64_t>(expr.getValue(), expr.getValue());
        }

        void RangeVisitor::visit(IdentifierExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            auto& bounds = _bounds[expr.getPlace()];
            val.value = std::make_pair<int64_t>(bounds.first, bounds.second);
        }

        void RangeVisitor::visit(MultiplyExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            expr.getLeft().accept(*this, context);
            auto lv = val.value;
            expr.getRight().accept(*this, context);
            auto rv = val.value;
            val.value.second = std::max({lv.first*rv.first, lv.first*rv.second, lv.second*rv.first, lv.second*rv.second});
            val.value.first = std::min({lv.first*rv.first, lv.first*rv.second, lv.second*rv.first, lv.second*rv.second});
        }

        void RangeVisitor::visit(MinusExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            expr.getValue().accept(*this, context);
            val.value.first *= -1;
            val.value.second *= -1;
            auto tmp = val.value.second;
            val.value.second = std::max(tmp, val.value.first);
            val.value.first = std::min(tmp, val.value.first);
        }

        void RangeVisitor::visit(SubtractExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            expr.getLeft().accept(*this, context);
            auto lv = val.value;
            expr.getRight().accept(*this, context);
            val.value.first -= lv.second;
            val.value.second -= lv.first;
        }

        void RangeVisitor::visit(PlusExpression &expr, Result &context) {
            auto& val = static_cast<PairResult&>(context);
            expr.getLeft().accept(*this, context);
            auto lv = val.value;
            expr.getRight().accept(*this, context);
            val.value.first += lv.first;
            val.value.second += lv.second;
        }
    }
}