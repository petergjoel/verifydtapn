#include "Core/TAPN/WatchExpression.hpp"
#include "DiscreteVerification/QueryVisitor.hpp"
#include "DiscreteVerification/DataStructures/RealMarking.hpp"

using namespace VerifyTAPN::TAPN;
using VerifyTAPN::DiscreteVerification::RealMarking;
using VerifyTAPN::DiscreteVerification::QueryVisitor;

using std::vector;

float Watch::new_marking(RealMarking *marking)
{
    float timestamp = marking->getTotalAge();
    QueryVisitor<RealMarking> checker(*marking, *_tapn);
    IntResult res;
    _expr->accept(checker, res);
    double value = res.value;
    _values.push_back(value);
    _timestamps.push_back(timestamp);
    return value;
}

void Watch::reset()
{
    _values.clear();
    _timestamps.clear();
}

size_t Watch::n_values() const {
    return _values.size();
}

std::string Watch::get_plots(const std::string& name) const 
{
    std::stringstream plots;
    std::vector<float> bins;
    std::string id = "(" + name + ")";
    printIntXPlot(plots, id + " value/step", _values);
    printPlot(plots, id + " value/time", _timestamps, _values);
    return plots.str();
}

void WatchAggregator::new_watch(Watch* watch)
{
    for(int i = 0 ; i < watch->_values.size() ; i++) {
        float x = watch->_values[i];
        if(i >= step_avg.size()) {
            step_avg.push_back(x);
            step_min.push_back(x);
            step_max.push_back(x);
            step_runs_count.push_back(1);
        } else {
            if(x > step_max[i]) step_max[i] = x;
            if(x < step_min[i]) step_min[i] = x;
            step_avg[i] += x;
            step_runs_count[i]++;
        }
        float tstamp = watch->_timestamps[i];
        timestamps.push_back(tstamp); // Might be faster to insert whole vector after, but not sure :/
        time_values.push_back(x);
    }
    n_watchs++;
}

void WatchAggregator::aggregate(unsigned int stepBins, unsigned int timeBins)
{
    size_t longest = step_avg.size();
    step_bins.clear();
    if(stepBins == 0 || stepBins >= longest) {
        for(int i = 0 ; i < longest ; i++) {
            step_avg[i] /= step_runs_count[i];
            step_bins.push_back(i);
        }
    }
    else {
        vector<float> step_avg_bins, step_min_bins, step_max_bins;
        vector<size_t> bin_runs_count(stepBins, (size_t) 0);
        float binSize = ((float) longest) / stepBins;
        for(int i = 0 ; i < step_avg.size() ; i++) {
            size_t bin = floor(i / binSize);
            bin = std::min(bin, (size_t) stepBins - 1);
            if(bin >= step_bins.size()) {
                step_avg_bins.push_back(step_avg[i]);
                step_max_bins.push_back(step_max[i]);
                step_min_bins.push_back(step_min[i]);
                bin_runs_count[bin] += step_runs_count[i];
                step_bins.push_back(bin * binSize);
                continue;
            }
            step_avg_bins[bin] += step_avg[i];
            bin_runs_count[bin] += step_runs_count[i];
            if(step_max[i] > step_max_bins[bin]) {
                step_max_bins[bin] = step_max[i];
            }
            if(step_min[i] < step_min_bins[bin]) {
                step_min_bins[bin] = step_min[i];
            }
        }
        for(int b = 0 ; b < stepBins ; b++) {
            step_avg_bins[b] /= bin_runs_count[b];
            global_steps_avg += step_avg_bins[b] / stepBins;
        }
        step_avg = step_avg_bins;
        step_min = step_min_bins;
        step_max = step_max_bins;
        bin_runs_count.clear();
    }
    step_runs_count.clear();

    float timestampmax = timestamps.back();
    float binSize = timestampmax / timeBins;
    std::vector<float> timestamps_bins(timeBins, std::numeric_limits<float>::quiet_NaN());
    time_avg = std::vector(timestamps_bins); 
    time_min = std::vector(timestamps_bins);
    time_max = std::vector(timestamps_bins);
    std::vector<size_t> time_runs_count = std::vector(timeBins, (size_t) 0);
    for(int i = 0 ; i < time_values.size() ; i++) {
        float timestamp = timestamps[i];
        float value = time_values[i];
        size_t bin = floor(timestamp / binSize);
        bin = std::min(bin, (size_t) timeBins - 1);
        if(std::isnan(timestamps_bins[bin])) {
            timestamps_bins[bin] = bin * binSize;
            time_avg[bin] = value;
            time_max[bin] = value;
            time_min[bin] = value;
            time_runs_count[bin]++;
            continue;
        }
        time_avg[bin] += value;
        if(value > time_max[bin]) time_max[bin] = value;
        if(value < time_min[bin]) time_min[bin] = value;
        time_runs_count[bin]++;
    }
    timestamps = timestamps_bins;
    for(size_t i = 0 ; i < timeBins ; i++) {
        if(!std::isnan(timestamps[i])) {
            time_avg[i] /= time_runs_count[i];
            global_time_avg += time_avg[i] / timeBins;
        }   
    }
    time_runs_count.clear();
    time_values.clear();
}

void WatchAggregator::reset()
{
    n_watchs = 0;
    step_bins.clear();
    step_avg.clear();
    step_min.clear();
    step_max.clear();
    step_runs_count.clear();
    time_values.clear();
    time_avg.clear();
    time_min.clear();
    time_max.clear();
    timestamps.clear();
    global_steps_avg = 0.0;
    global_time_avg = 0.0;
}

std::string WatchAggregator::get_plots(const std::string& name) const
{
    std::stringstream plots;
    std::string id = "(" + name + ")";
    printPlot(plots, id + " avg/step", step_bins, step_avg);
    printPlot(plots, id + " min/step", step_bins, step_min);
    printPlot(plots, id + " max/step", step_bins, step_max);
    printPlot(plots, id + " avg/time", timestamps, time_avg);
    printPlot(plots, id + " min/time", timestamps, time_min);
    printPlot(plots, id + " max/time", timestamps, time_max);
    plots << id << " Global steps avg.: " << global_steps_avg << std::endl;
    plots << id << " Global time avg.: " << global_time_avg << std::endl;
    return plots.str();
}

void VerifyTAPN::TAPN::printPlot(std::stringstream &stream, const std::string &title, const std::vector<float> &x, const std::vector<float> y)
{
    stream << title << std::endl;
    for(int i = 0 ; i < x.size() ; i++) {
        if(std::isnan(x[i]) || std::isnan(y[i])) {
            continue;
        }
        stream << x[i] << ":" << y[i] << ";";
    }
    stream << std::endl;
}

void VerifyTAPN::TAPN::printIntXPlot(std::stringstream &stream, const std::string &title, const std::vector<float> y)
{
    stream << title << std::endl;
    for(int i = 0 ; i < y.size() ; i++) {
        stream << i << ":" << y[i] << ";";
    }
    stream << std::endl;
}