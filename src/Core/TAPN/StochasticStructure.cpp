#include "Core/TAPN/StochasticStructure.hpp"

namespace VerifyTAPN::SMC {

    std::string distributionName(DistributionType type) {
        switch(type) {
            case Constant:
                return "constant";
            case Uniform:
                return "uniform";
            case Exponential:
                return "exponential";
            case Normal:
                return "normal";
            case Gamma:
                return "gamma";
            case Erlang:
                return "erlang";
            case DiscreteUniform:
                return "discrete uniform";
            case Geometric:
                return "geometric";
            case Triangular:
                return "triangular";
            case LogNormal:
                return "log normal";
        }
        return "";
    }

    std::string firingModeName(FiringMode mode) {
        switch(mode) {
            case Oldest:
                return "Oldest";
            case Youngest:
                return "Youngest";
            case Random:
                return "Random";
        }
        return "";
    }

    Distribution Distribution::urgent() {
        DistributionParameters params;
        params.constant.value = 0;
        return Distribution { Constant, params };
    }
    Distribution Distribution::defaultDistribution() {
        DistributionParameters params;
        params.constant.value = 1;
        return Distribution { Constant, params };
    }
    Distribution Distribution::fromParams(int distrib_id, std::vector<double> raw_params) {
        DistributionType distrib = static_cast<DistributionType>(distrib_id);
        DistributionParameters params;
        switch(distrib){
            case Constant:
                params.constant.value = raw_params[0];
                break; 
            case Uniform:
                params.uniform.a = raw_params[0];
                params.uniform.b = raw_params[1];
                break;
            case Exponential:
                params.exp.rate = raw_params[0];
                break;
            case Normal:
                params.normal.mean = raw_params[0];
                params.normal.stddev = raw_params[1];
                break;
            case Gamma:
            case Erlang:
                params.gamma.shape = raw_params[0];
                params.gamma.scale = raw_params[1];
                break;
            case DiscreteUniform:
                params.discreteUniform.a = (int) raw_params[0];
                params.discreteUniform.b = (int) raw_params[1];
                break;
            case Geometric:
                params.geometric.p = raw_params[0];
                break;
            case Triangular:
                params.triangular.a = raw_params[0];
                params.triangular.b = raw_params[1];
                params.triangular.c = raw_params[2];
                break;
            case LogNormal:
                params.logNormal.logMean = raw_params[0];
                params.logNormal.logStddev = raw_params[1];
                break;
            default:
                break;
        }
        return Distribution { distrib, params };
    }
    std::string Distribution::toXML() const {
        std::string endField = "\" ";
        std::stringstream res;
        res << "distribution=\"";
        res << distributionName(type) << endField;
        switch(type) {
            case Constant:
                res << "value=\"" << parameters.constant.value << endField;
                break;
            case Uniform:
                res << "a=\"" << parameters.uniform.a << endField;
                res << "b=\"" << parameters.uniform.b << endField;
                break;
            case Exponential:
                res << "rate=\"" << parameters.exp.rate << endField;
                break;
            case Normal:
                res << "mean=\"" << parameters.normal.mean << endField;
                res << "stddev=\"" << parameters.normal.stddev << endField;
                break;
            case Gamma:
            case Erlang:
                res << "shape=\"" << parameters.gamma.shape << endField;
                res << "scale=\"" << parameters.gamma.scale << endField;
                break;
            case DiscreteUniform:
                res << "a=\"" << parameters.discreteUniform.a << endField;
                res << "b=\"" << parameters.discreteUniform.b << endField;
                break;
            case Geometric:
                res << "p=\"" << parameters.geometric.p << endField;
                break;
            case Triangular:
                res << "a=\"" << parameters.triangular.a << endField;
                res << "b=\"" << parameters.triangular.b << endField;
                res << "c=\"" << parameters.triangular.c << endField;
                break;
            case LogNormal:
                res << "logMean=\"" << parameters.logNormal.logMean << endField;
                res << "logStddev=\"" << parameters.logNormal.logStddev << endField;
                break;
        }
        return res.str();
    }

}