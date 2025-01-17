/*
 * File:   TranslateVisitor.h
 * Author: Peter G. Jensen
 *
 * Created on 1 December 2021, 15.04
 */

#ifndef TRANSLATEVISITOR_H
#define TRANSLATEVISITOR_H

#include "AST.hpp"

#include <PQL/Visitor.h>
#include <iostream>

#include "Core/TAPN/WatchExpression.hpp"

namespace VerifyTAPN {
    namespace TAPN {
        class TimedArcPetriNet;
    }

    namespace AST {

        class TranslationVisitor : public unfoldtacpn::PQL::Visitor {
            std::unique_ptr<Query> _result;
            AST::Expression* _e_result = nullptr;
            AST::ArithmeticExpression* _a_result = nullptr;
            bool _first_element = true;
            bool _is_synth = false;
            const TAPN::TimedArcPetriNet& _net;

            void check_first(bool is_quant = false);
            AST::Expression* get_e_result();
            AST::ArithmeticExpression* get_a_result();
        public:
            TranslationVisitor(const TAPN::TimedArcPetriNet& net);

            TAPN::Observable translateObservable(const unfoldtacpn::PQL::Observable& obs);

            std::unique_ptr<Query> translate(const unfoldtacpn::PQL::Condition& condition);
            void _accept(const unfoldtacpn::PQL::NotCondition *element) override;
            void _accept(const unfoldtacpn::PQL::AndCondition *element) override;
            void _accept(const unfoldtacpn::PQL::OrCondition *element) override;
            void _accept(const unfoldtacpn::PQL::LessThanCondition *element) override;
            void _accept(const unfoldtacpn::PQL::LessThanOrEqualCondition *element) override;
            void _accept(const unfoldtacpn::PQL::EqualCondition *element) override;
            void _accept(const unfoldtacpn::PQL::NotEqualCondition *element) override;
            void _accept(const unfoldtacpn::PQL::DeadlockCondition *element) override;
            void _accept(const unfoldtacpn::PQL::ControlCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::EFCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::EGCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::AGCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::AFCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::PFCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::PGCondition *condition) override;
            void _accept(const unfoldtacpn::PQL::BooleanCondition *element) override;
            void _accept(const unfoldtacpn::PQL::UnfoldedIdentifierExpr *element) override;
            void _accept(const unfoldtacpn::PQL::LiteralExpr *element) override;
            void _accept(const unfoldtacpn::PQL::PlusExpr *element) override;
            void _accept(const unfoldtacpn::PQL::MultiplyExpr *element) override;
            void _accept(const unfoldtacpn::PQL::MinusExpr *element) override;
            void _accept(const unfoldtacpn::PQL::SubtractExpr *element) override;
            void _accept(const unfoldtacpn::PQL::IdentifierExpr *element) override;
        };
    }
}

#endif /* TRANSLATEVISITOR_H */

