#ifndef CAFFE_UTIL_MATH_FUNCTIONS_SCIDB_H_
#define CAFFE_UTIL_MATH_FUNCTIONS_SCIDB_H_

#include <stdint.h>
#include <cmath>  // for std::fabs and std::signbit

#include "glog/logging.h"

#include "caffe/common.hpp"
#include "caffe/util/device_alternate.hpp"
#include "caffe/util/mkl_alternate.hpp"

#include <time.h>       // for timing helpers only

#include <curl/curl.h>       // curl
#include <curl/easy.h>       // curl

namespace scidb {

//
// timing helpers
//

double clock_getsecs(long clock);
double getsecs();

// TODO: rename TimeStampedOstream
class TimeStampedStream
{
public:
    TimeStampedStream(std::ostream& os) : _os(os), _leastSecs(1.0/0.0/*inf*/), _lastNow(1.0/0.0) {;}

    // non-const version updates the timestamps, and returns a const one for the rest of the line to use
    template<typename T>
    const TimeStampedStream& operator<<(const T& t)
    {
        double now = getsecs();
        if(now < _leastSecs) {  // which starts at Inf
            _leastSecs = now;
            _lastNow = 0;
        }
        now -= _leastSecs;      // now relative to _leastSecs

        std::stringstream timestamp;
        timestamp << std::fixed << std::setprecision(6) << now << " " << (now - _lastNow) << " ";
        _lastNow = now;
        _os << timestamp.str();
        _os << t;
        return *this;
    }
    template<typename T>
    const TimeStampedStream& operator<<(const T& t) const { _os << t; return *this; }
    
    // above two resolves most operator<<(T), except std::endl, std::flush
    // so it needs the assistance of the following
    const TimeStampedStream& operator<<(std::ostream& (*funcPtr)(std::ostream&)) const
    {
        funcPtr(_os);
        return *this;
    }


    // const one does not update the timestamps
    // template<typename T>
    // const TimeStampedStream& operator<<(const T& t) { _os << t; }
private:
    std::ostream& _os;
    double   _leastSecs; // TODO: make this a singleton, so there is only 1 version of leastSecs
    double   _lastNow;
};

struct Shim {
    /*const*/ CURL*   curlHandle;       // underlying libcurl can't handle this being const
    const std::string baseURL;
    std::string       session;
    bool              verbose;
    bool              check;
    TimeStampedStream tsos;

    Shim(CURL* curlHandle_, const std::string& baseURL_, const std::string& session_, std::ostream& os)
    :
    curlHandle(curlHandle_),
    baseURL(baseURL_),
    session(session_),
    verbose(false),
    check(false),
    tsos(os)
    { }
};


Shim& getShim();

void dgemmScidbServer(const char& TRANSA, const char& TRANSB,
                      long M, long N, long K,
                      double ALPHA, const double* aData, long LDA,
                                    const double* bData, long LDB,
                      double BETA,        double* cData, long LDC,
                      Shim& shim);

} // namespace scidb

namespace caffe {

//
// Caffe gemm provides its own version of the gemm functions which
// are then redirected to a BLAS or GPU implementations.
// This version performs the computation on SciDB, to off-load the multiplication
// onto a parallel cluster, much like using a much like using a GPU
//

template <class Dtype>
void caffe_scidb_gemm(const CBLAS_TRANSPOSE TransA, const CBLAS_TRANSPOSE TransB,
                      const int M, const int N, const int K,
                      const Dtype alpha, const Dtype* aData, const int& lda,
                                         const Dtype* bData, const int& ldb, const Dtype beta,
                                               Dtype* cData, const int& ldc);

// NOTE: specializations for double and float are located
// in the cpp file

}  // namespace caffe

#endif  // CAFFE_UTIL_MATH_FUNCTIONS_SCIDB_H_
