#ifndef PERFORMANCETESTCLASSES_H
#define PERFORMANCETESTCLASSES_H

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
#include <cstdlib>

#include "tester.h"

#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "membrainconfig.h"

using namespace std;
using namespace membrain;

class testParameterBase
{
public:
    virtual ~testParameterBase() {}

    virtual string valueAsString() = 0;
    virtual string valueAsString ( unsigned int step ) = 0;

    unsigned int steps;
    bool deltaLog;
    string name;
};


template<typename T>
class testParameter : public testParameterBase
{

public:
    virtual ~testParameter() {}

    virtual inline string valueAsString() {
        return toString ( mean );
    }

    virtual inline string valueAsString ( unsigned int step ) {
        return toString ( valueAtStep ( step ) );
    }

    T min, max, mean;

protected:
    T valueAtStep ( unsigned int step ) {
        if ( deltaLog ) {
            return pow ( 10.0, ( log10 ( max ) - log10 ( min ) ) * step / steps ) * min;
        } else {
            return ( max - min ) * step / steps + min;
        }
    }

    string toString ( const T &t ) {
        stringstream ss;
        ss << t;
        return ss.str();
    }

};


template<typename... U>
class performanceTest;


template<typename T>
class performanceTest<T>
{

public:
    performanceTest ( const char *name ) : name ( name ) {
        parameters.push_back ( &parameter );
    }
    virtual ~performanceTest() {}

    virtual bool itsMe ( const string &name ) const {
        return this->name == name;
    }

    virtual void runTestsIfMe ( const string &name, unsigned int repetitions ) {
        if ( itsMe ( name ) ) {
            runTests ( repetitions );
        }
    }

    virtual void runTests ( unsigned int repetitions ) {
        cout << "Running test case " << name << std::endl;
        for ( int param = parameters.size() - 1; param >= 0; --param ) {
            unsigned int steps = getStepsForParam ( param );
            ofstream temp ( "temp.dat" );

            for ( unsigned int step = 0; step < steps; ++step ) {
                string params = getParamsString ( param, step );
                stringstream call;
                call << "./membrain-performancetests " << repetitions << " " << name << " " << params;
                cout << "Calling: " << call.str() << endl;
                system ( call.str().c_str() );

                resultToTempFile ( param, step, temp );
                temp << endl;
            }

            temp.close();
            ofstream gnutemp ( "temp.gnuplot" );
            string outname = name + param;
            gnutemp << generateGnuplotScript ( outname, parameters[param]->name, "Execution time [ms]", name, parameters[param]->deltaLog );
            gnutemp.close();

            cout << "Calling gnuplot and displaying result" << endl;
            system ( "gnuplot temp.gnuplot" );
            system ( ( "convert -density 300 -resize 1920x " + outname + ".eps -flatten " + outname + ".png" ).c_str() );
            system ( ( "display " + outname + ".png &" ).c_str() );
        }
    }

protected:
    virtual inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[parameters.size() - varryParam - 1]->steps;
    }

    virtual string getParamsString ( int varryParam, unsigned int step, const string &delimiter = " " ) {
        stringstream ss;
        for ( int i = parameters.size() - 1; i >= 0; --i ) {
            if ( i == varryParam ) {
                ss << parameters[i]->valueAsString ( step );
            } else {
                ss << parameters[i]->valueAsString();
            }
            ss << delimiter;
        }
        return ss.str();
    }

    virtual string getTestOutfile ( int varryParam, unsigned int step ) {
        stringstream ss;
        ss << "performancetest_" << name;
        for ( int i = parameters.size() - 1; i >= 0; --i ) {
            if ( i == varryParam ) {
                ss << "#" << parameters[i]->valueAsString ( step );
            } else {
                ss << "#" << parameters[i]->valueAsString();
            }
        }
        return ss.str();
    }

    virtual void resultToTempFile ( int varryParam, unsigned int step, ofstream &file ) {
        file << getParamsString ( varryParam, step, "\t" );
        ifstream test ( getTestOutfile ( varryParam, step ) );
        string line;
        while ( getline ( test, line ) ) {
            if ( line.find ( '#' ) != 0 ) {
                stringstream ss ( line );
                vector<string> parts;
                string part;
                while ( getline ( ss, part, '\t' ) ) {
                    parts.push_back ( part );
                }
                file << parts[parts.size() - 2] << '\t';
            }
        }
    }

    virtual string generateGnuplotScript ( const string &name, const string &xlabel, const string &ylabel, const string &title, bool log ) {
        stringstream ss;
        ss << "set terminal postscript eps enhanced color 'Helvetica,10'" << endl;
        ss << "set output \"" << name << ".eps\"" << endl;
        ss << "set xlabel \"" << xlabel << "\"" << endl;
        ss << "set ylabel \"" << ylabel << "\"" << endl;
        ss << "set title \"" << title << "\"" << endl;
        if ( log ) {
            ss << "set log xy" << endl;
        } else {
            ss << "set log y" << endl;
        }
        ss << generateMyGnuplotPlotPart ( "temp.dat" );
        return ss.str();
    }

    virtual string generateMyGnuplotPlotPart ( const string &file ) = 0;

    const char *name;

    testParameter<T> parameter;
    vector<testParameterBase *> parameters;

};


template<typename T, typename... U>
class performanceTest<T, U...> : public performanceTest<U...>
{

public:
    performanceTest ( const char *name ) : performanceTest<U...> ( name ) {
        this->parameters.push_back ( &parameter );
    }

    virtual ~performanceTest() {}

protected:
    testParameter<T> parameter;

};


#define PARAMREFS(number, param, params...) testParameter<param> &parameter##number = performanceTest<param, ##params>::parameter

#define ONEPARAM(param) PARAMREFS(1, param)
#define TWOPARAMS(param1, param2) PARAMREFS(1, param1, param2); \
                                  PARAMREFS(2, param2)
#define THREEARAMS(param1, param2, param3) PARAMREFS(1, param1, param2, param3); \
                                           PARAMREFS(2, param2, param3) \
                                           PARAMREFS(3, param3)

#define TESTCLASS(name, parammacro, params...) \
    class name : public performanceTest<params> \
    { \
    public: \
        name(); \
        virtual ~name() {} \
        static void actualTestMethod(tester&, params); \
        virtual string generateMyGnuplotPlotPart(const string& file); \
        parammacro; \
        static string comment; \
    }; \
    extern name name##Instance

#define ONEPARAMTEST(name, param) TESTCLASS(name, ONEPARAM(param), param)
#define TWOPARAMTEST(name, param1, param2) TESTCLASS(name, TWOPARAMS(param1, param2), param1, param2)
#define THREEPARAMTEST(name, param1, param2, param3) TESTCLASS(name, THREEPARAMS(param1, param2, param3), param1, param2, param3)


// Actual performance test classes come here

TWOPARAMTEST ( matrixTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTransposeOpenMPTest, int, int );
TWOPARAMTEST ( matrixCleverBlockTransposeOpenMPTest, int, int );

#endif // PERFORMANCETESTCLASSES_H
