/* 
 * File:   SafetySynthesis.cpp
 * Author: Peter G. Jensen
 * 
 * Created on 14 September 2015, 15:59
 */
#include "DiscreteVerification/VerificationTypes/SafetySynthesis.h"
#include "DiscreteVerification/DataStructures/SimpleMarkingStore.h"
#include "DiscreteVerification/DataStructures/PTrieMarkingStore.h"
#include "DiscreteVerification/Generators/ReducingGameGenerator.h"

#include <cassert>
#include <set>
#include <fstream>


namespace VerifyTAPN { namespace DiscreteVerification {

    SafetySynthesis::SafetySynthesis(TAPN::TimedArcPetriNet &tapn,
                                     NonStrictMarking &initialMarking,
                                     AST::Query *query,
                                     VerificationOptions &options)
            : tapn(tapn), initial_marking(initialMarking),
              query(query), options(options),
              placeStats(tapn.getNumberOfPlaces()),
              generator(nullptr), discovered(0), explored(0),
              largest(0) {
        if (options.getMemoryOptimization() == VerificationOptions::PTRIE)
            store = new PTrieMarkingStore<SafetyMeta>(tapn, options.getKBound());
        else
            store = new SimpleMarkingStore<SafetyMeta>();

        switch (options.getSearchType()) {
            case VerificationOptions::DEPTHFIRST:
                waiting = new dfs_queue<store_t::Pointer *>();
                break;
            case VerificationOptions::RANDOM:
                waiting = new random_queue<store_t::Pointer *>();
                break;
            case VerificationOptions::BREADTHFIRST:
            default:
                waiting = new bfs_queue<store_t::Pointer *>();
                break;
        }
        if(options.getPartialOrderReduction())
        {
            generator = std::make_unique<ReducingGameGenerator>(tapn, query);
        }
        else
            generator = std::make_unique<GameGenerator>(tapn, query);
    }

    bool SafetySynthesis::satisfies_query(NonStrictMarkingBase *m) {
        m->cut(placeStats);
        if (m->size() > options.getKBound()) return false;
        QueryVisitor<NonStrictMarkingBase> checker(*m, tapn);
        BoolResult context;
        query->accept(checker, context);
        return context.value;
    }

    SafetySynthesis::store_t::Pointer* SafetySynthesis::pop_waiting() {
        return waiting->pop();
    }

    bool SafetySynthesis::run() {
        backstack_t back;
        largest = initial_marking.size();

        // if initial satisfies and AF (return true), ok OR initial violates and AG (return false)
        if (satisfies_query(&initial_marking) == (query->getQuantifier() == Quantifier::CF))
            return query->getQuantifier() == Quantifier::CF;

        store_t::result_t m_0_res = store->insert_and_dealloc(&initial_marking);

        SafetyMeta &meta = store->get_meta(m_0_res.second);
        meta = {UNKNOWN, false, false, 0, 0, depends_t()};
        meta.waiting = true;

        waiting->push(m_0_res.second);

        while (meta.state != LOOSING && meta.state != WINNING) {
            ++explored;
            store_t::Pointer *next;

            while (!back.empty()) {
                next = back.top();
                back.pop();
                dependers_to_waiting(store->get_meta(next), back);
            }

            if (done()) break;

            next = pop_waiting();

            SafetyMeta &next_meta = store->get_meta(next);
            //std::cout   << "pop " << next << " State: " << &next_meta << " - > "
            //        << (int32_t)next_meta.state << std::endl;

            next_meta.waiting = false;
            if (next_meta.state == LOOSING ||
                next_meta.state == WINNING) {
                // these are allready handled in back stack
                continue;
            } else {
                // let's revalidate if we are needed
                bool has_some_undet = false;
                for(auto& d : next_meta.dependers)
                {
                    auto s = store->get_meta(d.second).state;
                    if(s == WINNING || s == LOOSING)
                        continue; // fully determined parent
                    if(s == MAYBE_LOSING && !d.first)
                        continue; // already has env strategy
                    if(s == MAYBE_WINNING && d.first)
                        continue; // already has ctrl strategy
                    has_some_undet = true;
                }
                if(!has_some_undet && &next_meta != &meta) {
                    next_meta.dependers.clear(); // no reason to unfold
                    continue;
                }

                assert(next_meta.state == UNKNOWN);
                next_meta.state = PROCESSED;
                //std::cerr << "PRE META " << meta.state << std::endl;
                NonStrictMarkingBase *marking = store->expand(next);
                // generate successors for environment
                generator->prepare(marking);
                auto env_successors = successors(next, next_meta, false, query);

                if (next_meta.state != LOOSING) {
                    // generate successors for controller
                    auto ctrl_successors = successors(next, next_meta, true, query);
                    if(next_meta.state != WINNING && next_meta.state != LOOSING) // only need to extend dependency-graph when node is not fully determined
                    {
                        add_successors(next, next_meta, ctrl_successors, true);
                        add_successors(next, next_meta, env_successors, false);
                    }
                }
                
                //std::cerr << "CHILDREN (" << next_meta.env_children << ", " << next_meta.ctrl_children << ")" << std::endl;
                //std::cerr << "NEW META " << next_meta.state << std::endl;

                if (next_meta.state == MAYBE_WINNING && next_meta.env_children == 0) {
                    next_meta.state = WINNING;
                }

                if (next_meta.state == LOOSING || next_meta.state == WINNING) {
                    assert(store->get_meta(next).state != PROCESSED);
                    //std::cerr << "PUSH " << &next_meta << std::endl;
                    back.push(next);
                }
                store->free(marking);
            }
            //std::cerr << "META " << meta.state << std::endl;
        }
        
        if(query->getQuantifier() == Quantifier::CG)
            return meta.state != LOOSING;
        else if(query->getQuantifier() == Quantifier::CF)
            return meta.state == WINNING;
        else
        {
            assert(false);
            return false;
        }
    }

    void SafetySynthesis::dependers_to_waiting(SafetyMeta &next_meta, backstack_t &back) {
        //std::cerr << "DEP TO WAITING " << &next_meta << " STATE " << next_meta.state << std::endl;
        for (const auto& ancestor : next_meta.dependers) {
            SafetyMeta &a_meta = store->get_meta(ancestor.second);
            //std::cerr << "\tDEP" << &a_meta << " STATE " << a_meta.state << std::endl;
            if (a_meta.state == LOOSING || a_meta.state == WINNING) continue;

            bool ctrl_child = ancestor.first;
            if (ctrl_child) {
                a_meta.ctrl_children -= 1;
                if (next_meta.state == WINNING && a_meta.state == MAYBE_LOSING)
                    a_meta.state = WINNING;                
                else if (next_meta.state == WINNING) {
                    a_meta.state = MAYBE_WINNING;
                }
                
                if (a_meta.state != WINNING && a_meta.ctrl_children == 0 && a_meta.state != MAYBE_WINNING)
                    a_meta.state = LOOSING;
                

            } else {
                assert(a_meta.state != MAYBE_LOSING);
                a_meta.env_children -= 1;
                if (next_meta.state == LOOSING) a_meta.state = LOOSING;
                if (next_meta.state == WINNING && a_meta.env_children == 0 && a_meta.ctrl_children == 0)
                    a_meta.state = WINNING;
            }

            if (a_meta.env_children == 0 && a_meta.state == MAYBE_WINNING) {
                a_meta.state = WINNING;
            }

            if (a_meta.state == WINNING || a_meta.state == LOOSING) {
                assert(store->get_meta(ancestor.second).state != PROCESSED);
                if (!a_meta.waiting) back.push(ancestor.second);
                a_meta.waiting = true;
            }
        }
        next_meta.dependers.clear();
    }

    std::vector<SafetySynthesis::store_t::Pointer*>
    SafetySynthesis::successors(store_t::Pointer *parent,
                                     SafetyMeta &meta,
                                     bool is_controller,
                                     const Query* query) {

//        std::cout << (is_controller ? "controller" : "env ");
//        std::cout << " : " << *marking << std::endl;

        NonStrictMarkingBase *next = nullptr;

        std::vector<store_t::Pointer *> successors;
        size_t number_of_children = 0;
        bool terminated = false;
        bool all_loosing = true;
        bool some_winning = false;
        generator->reset();
        while ((next = generator->next(is_controller)) != nullptr) {

            largest = std::max(next->size(), largest);
            ++discovered;
            ++number_of_children;

//            std::cout << "\tchild  " << " : " << *next << std::endl;

            if (query->getQuantifier() == Quantifier::CG) {
                if (!satisfies_query(next)) {
    //            std::cout << "\t\tdoes not satisfy phi" << std::endl;
                    delete next;

                    if (is_controller) {
                        continue;
                    } else {
                        meta.state = LOOSING;
                        //std::cout << "LOOSING : " << parent << std::endl;
                        terminated = true;
                        break;
                    }
                }
            } else if (query->getQuantifier() == Quantifier::CF) {
                if (satisfies_query(next)) {
                    //std::cerr << "\t\tSAT!" << std::endl;
                    delete next;
                    if (!is_controller) {
                        //std::cerr << "NOT CTRL" << std::endl;
                        some_winning = true;
                        continue;
                    } else {
                        //std::cerr << "CTRL" << std::endl;
                        if(meta.state == MAYBE_LOSING)
                            meta.state = WINNING;
                        else
                            meta.state = MAYBE_WINNING;
                        terminated = true;
                        break;
                    }
                } 
            }
            else
            {
                std::cerr << "Using EG, EF, AG or AF without control is no longer supported" << std::endl;
                assert(false);
               std::exit(-1);
            }

            store_t::result_t res = store->insert_and_dealloc(next);

            store_t::Pointer *p = res.second;

            if (res.first) {
                //std::cerr << "\t\tNEW!" << std::endl;
                SafetyMeta childmeta = {UNKNOWN, false, false, 0, 0, depends_t()};
                store->set_meta(p, childmeta);
                successors.push_back(p);
                all_loosing = false;
//            std::cout << "\t\t" << p << std::endl;
                continue;
            }

            SafetyMeta &childmeta = store->get_meta(p);
            if (!is_controller && childmeta.state == LOOSING) {
                meta.state = LOOSING;
                //std::cout << "LOOSING2 : " << parent << std::endl;
                terminated = true;
                break;
            } else if (is_controller && (childmeta.state == WINNING || 
                    (p == parent && query->getQuantifier() == Quantifier::CG))) {
                if(meta.state == MAYBE_LOSING)
                    meta.state = WINNING;
                else
                    meta.state = MAYBE_WINNING;
                terminated = true;
                break;
            } else if (!is_controller && p == parent) {
                if(query->getQuantifier() == Quantifier::CF)
                {
                    meta.state = LOOSING;
                    //std::cout << "LOOSING3 : " << parent << std::endl;
                    terminated = true;
                    break;
                }
                else 
                    continue;
            }

            if (childmeta.state != WINNING && childmeta.state != LOOSING) {
                successors.push_back(p);
            }
            some_winning = some_winning || (childmeta.state == WINNING);
            all_loosing = all_loosing && (childmeta.state == LOOSING);
        }

        if (terminated)
        {
//            std::cerr << "TERMINATED" << std::endl;
            return {}; // Add nothing to waiting, we already have result
        }

        if (is_controller) {
            if(query->getQuantifier() == Quantifier::CG)
            {
                if (number_of_children == 0) {
                    meta.state = MAYBE_WINNING;
                    return {};
                } else if (all_loosing) {
                    meta.state = LOOSING;
//                    std::cout << "LOOSING4 : " << parent << std::endl;
                    return {};
                }
            }
            else if(query->getQuantifier() == Quantifier::CF)
            {
                if(number_of_children == 0 && meta.state == MAYBE_LOSING)
                {
                    meta.state = WINNING;
                    return {};
                }
            }
        } else {
            if(query->getQuantifier() == Quantifier::CF)
            {
                // no unknown or losing successors, but some winning (and thus all winning)
                if(successors.size() == 0 && some_winning)
                {
                    meta.state = MAYBE_LOSING;
                    return {};
                }
            }
        }
        if(successors.empty())
            return successors;
        std::sort(successors.begin(), successors.end());
        size_t j = 0;
        for(size_t i = 1; i < successors.size(); ++i)
        {
            if(successors[j] == successors[i]) continue;
            else {
               ++j;
               successors[j] = successors[i];
            }
        }
        successors.resize(j+1);
        return successors;
    }

    void SafetySynthesis::add_successors(store_t::Pointer *parent,
                                     SafetyMeta &meta, const std::vector<store_t::Pointer*>& successors, bool is_controller)
    {
        for(auto* child : successors) {
            SafetyMeta &childmeta = store->get_meta(child);
            childmeta.dependers.push_front(
                    depender_t(is_controller, parent));
            if (childmeta.state == UNKNOWN &&
                !childmeta.waiting) {
                childmeta.waiting = true;
                waiting->push(child);
            }
        }

        if (is_controller) meta.ctrl_children = successors.size();
        else meta.env_children = successors.size();
    }

    void SafetySynthesis::print_stats() {
        std::cout << "  discovered markings:\t" << discovered << std::endl;
        std::cout << "  explored markings:\t" << explored << std::endl;
        std::cout << "  stored markings:\t" << store->size() << std::endl;
    }
    
    void SafetySynthesis::write_strategy(std::ostream& out)
    {
        //const auto k = store->max_tokens();
        /*out << "{\"version\":1.1,\"type\":\"state->constraint->actions\","
                "\"representation\":\"map\",\"actions\":[\n\"wait\"";
       
        for(const TAPN::TimedTransition* t : tapn.getTransitions())
        {
            if(t)
                out << ",\"" << t->getName() << "\"";
            else
                out << ",\"null\"";
        }
        out << "],\"statevars\":[";
        bool first = true;
        for(const TAPN::TimedPlace* p : tapn.getPlaces())
        {
            if(!first) out << ",";
            first = false;
            out << "\"token-count(" << p->getName() << ")\"";
        }

        out << "],\"pointvars\":[";
        first = true;
        for(size_t i = 0; i < k; ++i)
        {
            if(!first) out << ",";
            first = false;
            out << "\"token-age(" << i << ")\"";
        }
        out << "],\"constraints\":[\n";*/
        std::stack<store_t::Pointer*> missing;
        const auto add_new = [this,&missing](NonStrictMarkingBase* marking, bool controllable)
        {
            store_t::result_t res = store->insert_and_dealloc(marking);
            auto& meta = store->get_meta(res.second);
            if(!meta.printed) {
                meta.printed = true;
                if(!controllable ||
                   meta.state == WINNING || (query->getQuantifier() == Quantifier::CG && meta.state != UNKNOWN && meta.state != LOOSING))
                    missing.push(res.second);
            }
        };
        {
            auto copy = new NonStrictMarking(initial_marking);
            add_new(copy, true);
        }
        if(&out == &std::cout) out << "\n##BEGIN STRATEGY##\n";
        out << "{\n";
        bool first_marking = true;
        while(!missing.empty())
        {
            store_t::Pointer* ptr = missing.top();
            missing.pop();
            auto& meta = store->get_meta(ptr);
            if(meta.state == WINNING ||
               (query->getQuantifier() == Quantifier::CG && meta.state != UNKNOWN && meta.state != LOOSING))
            {
                auto marking = store->expand(ptr);
                generator->prepare(marking);
                generator->reset();
                NonStrictMarkingBase* next;
                while ((next = generator->next(false)) != nullptr) {
                    add_new(next, false);
                }

                bool first = true;
                generator->reset();
                while ((next = generator->next(true)) != nullptr) {
                    if(first) {
                        if(!first_marking) out << ",\n";
                        first_marking = false;
                        out << "\"" << *next << "\":[\n";
                        add_new(next, true);
                    }
                    if(!first)
                        out << ",\n";
                    first = false;
                    out << '\t';
                    if(generator->last_fired() == nullptr)
                        out << "\"+1\"";
                    else
                        out << "\"" << generator->last_fired()->getName() << "\"";
                    /*if(first) {
                        out << "{\"state\":[";
                        size_t last_seen = 0;
                        for(auto& p : next->getPlaceList())
                        {
                            while(last_seen < p.place->getIndex())
                            {
                                if(last_seen != 0) out << ",";
                                out << "0";
                                ++last_seen;
                            }
                            if(last_seen != 0) out << ",";
                            out << p.numberOfTokens();
                            ++last_seen;
                        }
                        for(;last_seen < tapn.getNumberOfPlaces();++last_seen)
                        {
                            if(last_seen != 0) out << ",";
                            out << "0";
                        }
                        out << "],\"point\":[";
                        bool fp = true;
                        size_t sum = 0;
                        for(auto& p : next->getPlaceList()) {
                            for(auto& t : p.tokens)
                            {
                                for(size_t n = 0; n < t.getCount(); ++n)
                                {
                                    if(!fp) out << ",";
                                    out << t.getAge();
                                    fp = false;
                                    ++sum;
                                }
                            }
                        }
                        for(;sum < k;++sum) {
                            if(!fp) out << ",";
                            out << "0";
                        }
                        out << "],\"actions\":[";
                    }
                    if(!first) out << ",";
                    if(generator.last_fired() == nullptr)
                        out << "0";
                    else
                        out << generator.last_fired()->getIndex()+1;
                    
                    first = false;*/
                }
                if(!first) out << "]";
            }
        }
        out << "\n}\n";
        if(&out == &std::cout)
            out << "##END STRATEGY##\n";
        //out << "]}\n";
    }

    SafetySynthesis::~SafetySynthesis() {
        delete store;
        delete waiting;
    }

} }

