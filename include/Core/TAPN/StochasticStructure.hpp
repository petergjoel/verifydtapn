#ifndef STOCHASTIC_STRUCTURE
#define STOCHASTIC_STRUCTURE

#include <random>
#include <sstream>
#include <string>

namespace VerifyTAPN::SMC {

    enum FiringMode {
        Oldest,
        Youngest,
        Random
    };

    std::string firingModeName(FiringMode type);

    enum DistributionType {
        Constant,
        Uniform,
        Exponential,
        Normal,
        Gamma,
        Erlang,
        DiscreteUniform,
        Geometric,
        Triangular,
        LogNormal
    };

    std::string distributionName(DistributionType type);

    struct SMCUniformParameters {
        double a;
        double b;
    };
    struct SMCExponentialParameters {
        double rate;
    };
    struct SMCNormalParameters {
        double mean;
        double stddev;
    };
    struct SMCConstantParameters {
        double value;
    };
    struct SMCGammaParameters {
        double shape;
        double scale;
    };
    struct SMCDiscreteUniformParameters {
        int a;
        int b;
    };
    struct SMCGeometricParameters {
        double p;
    };
    struct SMCTriangularParameters {
        double a;
        double b;
        double c;
    };
    struct SMCLogNormalParameters {
        double logMean;
        double logStddev;
    };

    union DistributionParameters {
        SMCUniformParameters uniform;
        SMCExponentialParameters exp;
        SMCNormalParameters normal;
        SMCConstantParameters constant;
        SMCGammaParameters gamma;
        SMCDiscreteUniformParameters discreteUniform;
        SMCGeometricParameters geometric;
        SMCTriangularParameters triangular;
        SMCLogNormalParameters logNormal;
    };

    struct Distribution {
        DistributionType type;
        DistributionParameters parameters;

        template<typename T>
        double sample(T& engine, const unsigned int precision = 0) const {
            double date = 0;
            switch(type) {
                case Constant:
                    date = parameters.constant.value;
                    break;
                case Uniform:
                    date = std::uniform_real_distribution(parameters.uniform.a, parameters.uniform.b)(engine);
                    break;
                case Exponential:
                    date = std::exponential_distribution(parameters.exp.rate)(engine);
                    break;
                case Normal:
                    date = std::normal_distribution(parameters.normal.mean, parameters.normal.stddev)(engine);
                    break;
                case Gamma:
                case Erlang:
                    date = std::gamma_distribution(parameters.gamma.shape, parameters.gamma.scale)(engine);
                    break;
                case DiscreteUniform:
                    date = std::uniform_int_distribution(parameters.discreteUniform.a, parameters.discreteUniform.b)(engine);
                    break;
                case Geometric:
                    date = std::geometric_distribution(parameters.geometric.p)(engine);
                    break;
                case Triangular: {
                        std::vector<double> points{parameters.triangular.a, parameters.triangular.c, parameters.triangular.c};
                        std::vector<double> grad{0,1,0};
                        date = std::piecewise_linear_distribution(points.begin(), points.end(), grad.begin())(engine);
                    }
                    break;
                case LogNormal: 
                    date = std::lognormal_distribution(parameters.logNormal.logMean, parameters.logNormal.logStddev)(engine);
                    break;
            }
            if(precision > 0) {
                double factor = pow(10.0, precision);
                date = round(date * factor) / factor;
            }
            return std::max(date, 0.0);
        }

        static Distribution urgent();

        static Distribution defaultDistribution();

        static Distribution fromParams(int distrib_id, std::vector<double> params);

        std::string toXML() const;

    };

}

#endif