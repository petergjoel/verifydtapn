#include "DiscreteVerification/VerificationTypes/ProbabilityEstimation.hpp"
#include "DiscreteVerification/QueryVisitor.hpp"

#include <math.h>

namespace VerifyTAPN::DiscreteVerification {

ProbabilityEstimation::ProbabilityEstimation(
    TAPN::TimedArcPetriNet &tapn, RealMarking &initialMarking, AST::SMCQuery *query, VerificationOptions options
)
: SMCVerification(tapn, initialMarking, query, options), validRuns(0)
{
    computeChernoffHoeffdingBound(smcSettings.estimationIntervalWidth, smcSettings.confidence);
}

bool ProbabilityEstimation::mustDoAnotherRun() {
    return numberOfRuns < runsNeeded;
}

void ProbabilityEstimation::prepare()
{
    std::cout << "Need to execute " << runsNeeded << " runs to produce estimation" << std::endl;
}

void ProbabilityEstimation::handleRunResult(const bool decisive, int steps, double delay, unsigned int thread_id)
{
    //bool valid = (query->getQuantifier() == PF && decisive) || (query->getQuantifier() == PG && !decisive);
    for(int i = 0 ; i < watch_aggrs.size() ; i++) {
        Watch* w = &watchs[i][thread_id];
        watch_aggrs[i].new_watch(w);
        w->reset();
    }
    if(decisive) {
        validRuns++;
        validRunsTime += delay;
        validRunsSteps += steps;
        if(validPerStep.size() <= steps) {
            validPerStep.resize(steps + 1, 0);
        }
        validPerStep[steps] += 1;
        validPerDelay.push_back(delay);
        if(delay > maxValidDuration) {
            maxValidDuration = delay;
        }
    } else {
        violatingRunTime += delay;
        violatingRunSteps += steps;
        if(violatingPerStep.size() <= steps) {
            violatingPerStep.resize(steps + 1, 0);
        }
        violatingPerStep[steps] += 1;
        violatingPerDelay.push_back(delay);
    }
}

bool ProbabilityEstimation::handleSuccessor(RealMarking* marking) {
    QueryVisitor<RealMarking> checker(*marking, tapn);
    AST::BoolResult context;
    query->accept(checker, context);

    for(int i = 0 ; i < watchs.size() ; i++) {
        Watch& w = watchs[i][marking->_thread_id];
        w.new_marking(marking);
    }

    delete marking;

    return context.value;
}

float ProbabilityEstimation::getEstimation() {
    float proba = ((float) validRuns) / numberOfRuns;
    return (query->getQuantifier() == PG) ? 1 - proba : proba;
}

void ProbabilityEstimation::computeChernoffHoeffdingBound(const float intervalWidth, const float confidence) {
    // https://link.springer.com/content/pdf/10.1007/b94790.pdf p.78-79
    float bound = log(2.0 / (1 - confidence)) / (2.0 * pow(intervalWidth, 2));
    runsNeeded = (unsigned int) ceil(bound);
}

void ProbabilityEstimation::printStats() {
    SMCVerification::printStats();
    printGlobalRunsStats();
    printValidRunsStats();
    printViolatingRunsStats();
    if(options.mustPrintCumulative()) printCumulativeStats();
    printWatchStats();
}

void ProbabilityEstimation::printValidRunsStats() {
    std::string category = "valid";
    if(query->getQuantifier() == PF) { 
        printRunsStats(category, validRuns, validRunsSteps, validRunsTime, validPerStep, validPerDelay);
    } else {
        printRunsStats(category, numberOfRuns - validRuns, violatingRunSteps, violatingRunTime, violatingPerStep, violatingPerDelay);
    }
}

void ProbabilityEstimation::printViolatingRunsStats() {
    std::string category = "violating";
    if(query->getQuantifier() == PG) {
        printRunsStats(category, validRuns, validRunsSteps, validRunsTime, validPerStep, validPerDelay);
    } else {
        printRunsStats(category, numberOfRuns - validRuns, violatingRunSteps, violatingRunTime, violatingPerStep, violatingPerDelay);
    }
}

void ProbabilityEstimation::printRunsStats(const std::string category, unsigned long n, unsigned long totalSteps, double totalDelay, std::vector<int> perStep, std::vector<float> perDelay) {
    if(n == 0) {
        std::cout << "  no " + category + " runs, unable to compute specific statistics" << std::endl;
        return;
    }
    double stepsMean = (totalSteps / (double) n);
    double timeMean = (totalDelay / n);
    double stepsAcc = 0;
    double delayAcc = 0;
    for(int i = 0 ; i < perStep.size() ; i++) {
        stepsAcc += pow(i - stepsMean, 2.0) * perStep[i];
    }
    for(int i = 0 ; i < perDelay.size() ; i++) {
        delayAcc += pow(perDelay[i] - timeMean, 2.0);
    }
    double stepsStdDev = sqrt( stepsAcc / n );
    double delayStdDev = sqrt( delayAcc / n );
    std::cout << "  statistics of " + category + " runs:" << std::endl;
    std::cout << "    number of " << category << " runs: " << n << std::endl;
    std::cout << "    duration of a " + category + " run (average):\t" << timeMean << std::endl;
    std::cout << "    duration of a " + category + " run (std. dev.):\t" << delayStdDev << std::endl;
    std::cout << "    length of a " + category + " run (average):\t" << stepsMean << std::endl;
    std::cout << "    length of a " + category + " run (std. dev.):\t" << stepsStdDev << std::endl;
}

void ProbabilityEstimation::printGlobalRunsStats() {
    double stepsMean = (totalSteps / (double) numberOfRuns);
    double timeMean = (totalTime / numberOfRuns);
    double stepsAcc = 0;
    double delayAcc = 0;
    for(int i = 0 ; i < validPerStep.size() ; i++) {
        stepsAcc += pow(i - stepsMean, 2.0) * validPerStep[i];
    }
    for(int i = 0 ; i < violatingPerStep.size() ; i++) {
        stepsAcc += pow(i - stepsMean, 2.0) * violatingPerStep[i];
    }
    for(int i = 0 ; i < validPerDelay.size() ; i++) {
        delayAcc += pow(validPerDelay[i] - timeMean, 2.0);
    }
    for(int i = 0 ; i < violatingPerDelay.size() ; i++) {
        delayAcc += pow(violatingPerDelay[i] - timeMean, 2.0);
    }
    double stepsStdDev = sqrt( stepsAcc / numberOfRuns );
    double delayStdDev = sqrt( delayAcc / numberOfRuns );
    std::cout << "  run duration (std. dev.):\t" << delayStdDev << std::endl;
    std::cout << "  run length (std. dev.):\t" << stepsStdDev << std::endl;
}

void ProbabilityEstimation::printCumulativeStats() {
    unsigned int digits = options.getCumulativeRoundingDigits();
    unsigned int stepScale = options.getStepsStatsScale();
    unsigned int timeScale = options.getTimeStatsScale();
    double mult = pow(10.0f, digits);

    double fact = (query->getQuantifier() == PF) ? 1 : -1;
    double initial = (query->getQuantifier() == PF) ? 0 : 1;

    std::cout << "  cumulative probability / step :" << std::endl;
    double acc = initial;
    double binSize = stepScale == 0 ? 1 : validPerStep.size() / (double) stepScale;
    double bin = 0;
    double lastAcc = acc;
    std::cout << 0 << ":" << acc << ";";
    for(int i = 0 ; i < validPerStep.size() ; i++) {
        double toPrint = round(acc * mult) / mult;
        if(i >= bin + binSize) {
            if(toPrint != lastAcc) {
                std::cout << bin << ":" << lastAcc << ";";
                std::cout << bin << ":" << toPrint << ";";
                lastAcc = toPrint;
            }
            bin = round(i / binSize) * binSize;
        }
        acc += fact * (validPerStep[i] / (double) numberOfRuns);  
    }
    if(!validPerStep.empty()) {
        std::cout << (validPerStep.size() - 1) << ":" << lastAcc << ";";
        std::cout << (validPerStep.size() - 1) << ":" << getEstimation() << ";";
    }
    std::cout << std::endl;

    std::cout << "  cumulative probability / delay :" << std::endl;
    acc = initial;
    binSize = timeScale == 0 ? 1 : (maxValidDuration / (double) timeScale);
    std::vector<double> bins(
        binSize > 0 ? (size_t) round(maxValidDuration / binSize) : 1
        , 0.0f);
    lastAcc = acc;
    for(int i = 0 ; i < validPerDelay.size() ; i++) {
        double delay = validPerDelay[i];
        int binIndex = std::min((size_t) round(delay / binSize), bins.size() - 1);
        bins[binIndex] += 1;
    }
    std::cout << 0 << ":" << acc << ";";
    for(int i = 0 ; i < bins.size() ; i++) {
        acc += fact * (bins[i] / (double) numberOfRuns);
        double toPrint = round(acc * mult) / mult;
        if(toPrint != lastAcc) {
            double binIndex = (i) * binSize;
            std::cout << binIndex << ":" << lastAcc << ";";
            std::cout << binIndex << ":" << toPrint << ";";
            lastAcc = toPrint;
        }
    }
    if(validRuns > 0) {
        std::cout << maxValidDuration << ":" << lastAcc << ";";
        std::cout << maxValidDuration << ":" << getEstimation() << ";";
    }
    std::cout << std::endl;
}

void ProbabilityEstimation::printWatchStats() {
    auto& obs = getSmcQuery()->getObservables();
    for(int i = 0 ; i < obs.size() ; i++) {
        watch_aggrs[i].aggregate(options.getStepsStatsScale(), options.getTimeStatsScale());
        std::string plot = watch_aggrs[i].get_plots(std::get<0>(obs[i]));
        std::cout << plot;
        watch_aggrs[i].reset();
    }
}

void ProbabilityEstimation::printResult() {
    /*if (options.getXmlTrace()) {
        printXMLTrace(m, printStack, query, tapn);
    } else {
        printHumanTrace(m, printStack, query->getQuantifier());
    }*/
    float result = getEstimation();
    float width = smcSettings.estimationIntervalWidth;
    std::cout << "Probability estimation:" << std::endl;
    std::cout << "\tConfidence: " << smcSettings.confidence * 100 << "%" << std::endl;
    std::cout << "\tP = " << result << " ± " << width << std::endl;
}

}
