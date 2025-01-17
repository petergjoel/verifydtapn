#include "DiscreteVerification/VerificationTypes/SMCVerification.hpp"

#include <thread>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

#define STEP_MS 5000

std::string printDouble(double value, unsigned int precision) {
    std::ostringstream oss;
    if(precision == 0) precision = std::numeric_limits<double>::max_digits10;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

namespace VerifyTAPN::DiscreteVerification {

bool SMCVerification::parallel_run() {
    prepare();
    runGenerator.prepare(&initialMarking);
    runGenerator.recordTrace = mustSaveTrace();
    auto start = std::chrono::steady_clock::now();

    size_t n_threads = std::thread::hardware_concurrency();
    std::cout << ". Using " << n_threads << " threads..." << std::endl;
    initWatchs(n_threads);

    std::vector<std::thread*> handles;
    for(int i = 0 ; i < n_threads ; i++) {
        auto handle = new std::thread([this, i]() {
            SMCRunGenerator generator = runGenerator.copy();
            generator._thread_id = i;
            bool continueExecution = true;
            while(continueExecution) {
                bool runRes = executeRun(&generator);
                double runDuration = std::min(generator.getRunDelay(), (double) smcSettings.timeBound);
                int runSteps = std::min(generator.getRunSteps(), smcSettings.stepBound);
                {
                    std::lock_guard<std::mutex> lock(run_res_mutex);
                    totalTime += runDuration;
                    totalSteps += runSteps;
                    numberOfRuns++;
                    handleRunResult(runRes, runSteps, runDuration, generator._thread_id);
                    if(mustSaveTrace()) handleTrace(runRes, &generator);
                    generator.recordTrace = mustSaveTrace();
                    continueExecution = mustDoAnotherRun();
                }
                generator.reset();
            }
        });
        handles.push_back(handle);
    }
    for(int i = 0 ; i < n_threads ; i++) {
        handles[i]->join();
        delete handles[i];
    }

    auto stop = std::chrono::steady_clock::now();
    durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    return true;
}

bool SMCVerification::run() {
    prepare();
    runGenerator.recordTrace = mustSaveTrace();
    runGenerator.prepare(&initialMarking);
    initWatchs();
    auto start = std::chrono::steady_clock::now();
    auto step1 = std::chrono::steady_clock::now();
    int64_t stepDuration;
    while(mustDoAnotherRun()) {
        bool runRes = executeRun();
        double runDuration = std::min(runGenerator.getRunDelay(), (double) smcSettings.timeBound);
        int runSteps = std::min(runGenerator.getRunSteps(), smcSettings.stepBound);
        handleRunResult(runRes, runSteps, runDuration);
        if(mustSaveTrace()) handleTrace(runRes);
        runGenerator.recordTrace = mustSaveTrace();
        
        totalTime += runDuration;
        totalSteps += runSteps;
        numberOfRuns++;
        runGenerator.reset();
        
        if(numberOfRuns % 100 != 0) continue;
        auto step2 = std::chrono::steady_clock::now();
        stepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(step2 - step1).count();
        if(stepDuration >= STEP_MS) {
            step1 = step2;
            stepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(step2 - start).count();
            std::cout << ". Duration : " << stepDuration << "ms ; Runs executed : " << numberOfRuns << std::endl;
        }
    }
    auto stop = std::chrono::steady_clock::now();
    durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    return true;
}

bool SMCVerification::executeRun(SMCRunGenerator* generator) {
    bool runRes = false;
    if(generator == nullptr) generator = &runGenerator;
    RealMarking* newMarking = generator->getMarking();
    while(!generator->reachedEnd() && !reachedRunBound(generator)) {
        RealMarking* child = new RealMarking(*newMarking);
        child->_thread_id = generator->_thread_id;
        runRes = handleSuccessor(child);
        if(runRes) break;
        newMarking = generator->next();
    }
    return runRes;
}

void SMCVerification::printStats() {
    std::cout << "  runs executed:\t" << numberOfRuns << std::endl;
    std::cout << "  average run length:\t" << (totalSteps / (double) numberOfRuns) << std::endl;
    std::cout << "  average run duration:\t" << (totalTime / (double) numberOfRuns) << std::endl;
    std::cout << "  verification time:\t" << ((double) durationNs / 1.0E9) << "s" << std::endl;
}

void SMCVerification::printTransitionStatistics() const {
    runGenerator.printTransitionStatistics(std::cout);
}

unsigned int SMCVerification::maxUsedTokens() {
    return maxTokensSeen;
}

void SMCVerification::setMaxTokensIfGreater(unsigned int i) {
    if(i > maxTokensSeen) {
        maxTokensSeen = i;
    }
}

bool SMCVerification::reachedRunBound(SMCRunGenerator* generator) {
    if(generator == nullptr) generator = &runGenerator;
    return 
        generator->getRunDelay() > smcSettings.timeBound ||
        generator->getRunSteps() > smcSettings.stepBound;
}

void SMCVerification::handleTrace(const bool runRes, SMCRunGenerator* generator) {
    if(generator == nullptr) generator = &runGenerator; 
    bool valid = query->getQuantifier() == PG ? !runRes : runRes;
    VerificationOptions::SMCTracesType type = options.getSMCTracesType();
    if(type == VerificationOptions::ANY_TRACE) {
        saveTrace(generator);
    } else if(type == VerificationOptions::SATISFYING_TRACES && valid) {
        saveTrace(generator);
    } else if(type == VerificationOptions::UNSATISFYING_TRACES && !valid) {
        saveTrace(generator);
    }
}

void SMCVerification::saveTrace(SMCRunGenerator* generator) {
    if(generator == nullptr) generator = &runGenerator;
    traces.push_back(generator->getTrace());
}

void SMCVerification::initWatchs(unsigned int n_threads) {
    std::vector<Observable>& obs = getSmcQuery()->getObservables();
    watchs.resize(obs.size());
    for(int i = 0 ; i < obs.size() ; i++) {
        ArithmeticExpression* expr = std::get<1>(obs[i]);
        Watch w(&tapn, expr);
        watchs[i].resize(n_threads, w);
    }
    watch_aggrs.resize(obs.size());
}

void SMCVerification::getTrace() {
    using namespace rapidxml;
    std::cerr << "Trace: " << std::endl;
    if(options.getXmlTrace()) {
        xml_document<> doc;
        xml_node<> *root = doc.allocate_node(node_element, "trace-list");
        doc.append_node(root);
        for(int i = 0 ; i < traces.size() ; i++) {
            std::string name = "Simulation" + std::to_string(i + 1);
            auto& stack = traces[i];
            printXMLTrace(stack, name, doc, root);
        }
        std::cerr << doc;
    } else {
        for(int i = 0 ; i < traces.size() ; i++) {
            std::string name = "Simulation" + std::to_string(i + 1);
            auto& stack = traces[i];
            printHumanTrace(stack, name);
        }
    }
}

void SMCVerification::printHumanTrace(std::stack<RealMarking *> &stack, const std::string& name) {
    bool isFirst = true;
    std::cout << "Name: " << name << std::endl;
    while (!stack.empty()) {
        if (isFirst) {
            isFirst = false;
        } else {
            RealMarking* marking = stack.top();
            if(marking->getPreviousDelay() > 0) {
                std::cout << "\tDelay: " << marking->getPreviousDelay() << std::endl;
            }
            if(marking->getGeneratedBy() != nullptr) {
                std::cout << "\tTransition:" << marking->getGeneratedBy()->getName() << std::endl;
            }
            if(marking->canDeadlock(tapn, 0)) {
                std::cout << "\tDeadlock: " << std::endl;
            }
        }
        std::cout << "Marking: ";
        for (auto& token_list : stack.top()->getPlaceList()) {
            for (auto& token : token_list.tokens) {
                for (int i = 0; i < token.getCount(); i++) {
                    std::cout << "(" << token_list.place->getName() << "," << token.getAge() << ") ";
                }
            }
        }
        stack.pop();
    }
}

void SMCVerification::printXMLTrace(std::stack<RealMarking *> &stack, const std::string& name, rapidxml::xml_document<> &doc, rapidxml::xml_node<>* list_node) {
    using namespace rapidxml;
    bool isFirst = true;
    RealMarking *old = nullptr;
    xml_node<> *root = doc.allocate_node(node_element, "trace");
    xml_attribute<> *name_attr = doc.allocate_attribute("name", doc.allocate_string(name.c_str()));
    root->append_attribute(name_attr);
    list_node->append_node(root);
    while (!stack.empty()) {
        if (isFirst) {
            isFirst = false;
        } else {
            RealMarking* marking = stack.top();
            if(marking->getPreviousDelay() > 0) {
                std::string str = printDouble(marking->getPreviousDelay(), options.getSMCNumericPrecision());
                xml_node<>* node = doc.allocate_node(node_element, "delay", doc.allocate_string(str.c_str()));
                root->append_node(node);
            }
            if(marking->getGeneratedBy() != nullptr) {
                root->append_node(createTransitionNode(old, marking, doc));
            }
            if(marking->canDeadlock(tapn, 0)) {
                root->append_node(doc.allocate_node(node_element, "deadlock"));
            }
        }
        old = stack.top();
        stack.pop();
    }
}

rapidxml::xml_node<> *SMCVerification::createTransitionNode(RealMarking *old, RealMarking *current, rapidxml::xml_document<> &doc) {
    using namespace rapidxml;
    xml_node<> *transitionNode = doc.allocate_node(node_element, "transition");
    xml_attribute<> *id = doc.allocate_attribute("id", current->getGeneratedBy()->getId().c_str());
    transitionNode->append_attribute(id);

    for (auto* arc : current->getGeneratedBy()->getPreset()) {
        createTransitionSubNodes(old, current, doc, transitionNode, arc->getInputPlace(),
                                     arc->getInterval(), arc->getWeight());
    }

    for (auto* arc : current->getGeneratedBy()->getTransportArcs()) {
        createTransitionSubNodes(old, current, doc, transitionNode, arc->getSource(),
                                     arc->getInterval(), arc->getWeight());
    }

    return transitionNode;
}

void SMCVerification::createTransitionSubNodes(RealMarking *old, RealMarking *current, rapidxml::xml_document<> &doc,
                                rapidxml::xml_node<> *transitionNode, const TAPN::TimedPlace &place,
                                const TAPN::TimeInterval &interval, int weight) {
    RealTokenList current_tokens = current->getTokenList(place.getIndex());
    RealTokenList old_tokens = old->getTokenList(place.getIndex());
    int tokensFound = 0;
    RealTokenList::const_iterator n_iter = current_tokens.begin();
    RealTokenList::const_iterator o_iter = old_tokens.begin();
    while (n_iter != current_tokens.end() && o_iter != old_tokens.end()) {
        if (n_iter->getAge() == o_iter->getAge()) {
            for (int i = 0; i < o_iter->getCount() - n_iter->getCount(); i++) {
                transitionNode->append_node(createTokenNode(doc, place, *n_iter));
                tokensFound++;
            }
            n_iter++;
            o_iter++;
        } else {
            if (n_iter->getAge() > o_iter->getAge()) {
                transitionNode->append_node(createTokenNode(doc, place, *o_iter));
                tokensFound++;
                o_iter++;
            } else {
                n_iter++;
            }
        }
    }
    for (RealTokenList::const_iterator iter = n_iter; iter != current_tokens.end(); iter++) {
        for (int i = 0; i < iter->getCount(); i++) {
            transitionNode->append_node(createTokenNode(doc, place, *iter));
            tokensFound++;
        }
    }
    for (auto& token : old_tokens) {
        if(tokensFound >= weight) break;
        if (token.getAge() >= interval.getLowerBound()) {
            for (int i = 0; i < token.getCount() && tokensFound < weight; i++) {
                transitionNode->append_node(createTokenNode(doc, place, token));
                tokensFound++;
            }
        }
    }
}

rapidxml::xml_node<> *
SMCVerification::createTokenNode(rapidxml::xml_document<> &doc, const TAPN::TimedPlace &place, const RealToken &token) {
    using namespace rapidxml;
    xml_node<> *tokenNode = doc.allocate_node(node_element, "token");
    xml_attribute<> *placeAttribute = doc.allocate_attribute("place",
                                                                doc.allocate_string(place.getName().c_str()));
    tokenNode->append_attribute(placeAttribute);
    auto str = printDouble(token.getAge(), options.getSMCNumericPrecision());
    xml_attribute<> *ageAttribute = doc.allocate_attribute("age", doc.allocate_string(
            str.c_str()));
    tokenNode->append_attribute(ageAttribute);
    return tokenNode;
}

}