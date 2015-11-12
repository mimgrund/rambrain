#include <math.h>
#include <stdio.h>






//Header:

class field_row {
public:
    field_row (unsigned int size) : size(size) {
        data = new double[size];
    }
    double* getData () {
        return data;
    }

    void printRow(unsigned int modulus) const {
        for(unsigned int y=0; y<size; y+=modulus) {
            printf("%e\t",data[y]);
        }
    }
    ~field_row() {
        delete data;
    }
private:
    double* data;
    unsigned int size;
};




//An abstract class for specifying initial conditions:
class initialConditions {
public:
    initialConditions(double x_max, double y_max):x_max(x_max),y_max(y_max) {};
    const double x_max,y_max;
    virtual double field_value(double x, double y) const = 0;
};

//A specialization thereof:
class simpleWave : public initialConditions {
public:
    simpleWave(double x_max, double y_max, double k_x, double k_y) :
        initialConditions(x_max,y_max), k_x(k_x),k_y(k_y) {};

    virtual double field_value(double x, double y) const {
        return sin(k_x*x+k_y*y);
    };
private:
    const double k_x,k_y;
};


//A field class that is encapsulating everyting.
class field {
public:
    field(unsigned int x_len, unsigned int y_len, initialConditions & initial) : x_len(x_len),y_len(y_len) {
        allocate_field();
        init_field(initial);
    }

    void print(unsigned int modulus = 1) const{
        for(unsigned int x=0; x<x_len; x+=modulus) {
            field_values[x]->printRow(modulus);
            printf("\n");
        }

    }
private:
    void allocate_field() {
        field_values = new field_row*[x_len];
        for(int x = 0; x<x_len; ++x) {
            field_values[x] = new field_row(y_len);
        }
    }

    void init_field(initialConditions &initial) {
        //Stepping:
        double xx_step = initial.x_max / x_len,xx = 0;
        double yy_step = initial.y_max / y_len;

        //Initialization
        for(unsigned int x=0; x<x_len; ++x) {
            double yy = 0;
            for(unsigned int y=0; y<y_len; ++y) {
                double *data = field_values[x]->getData();
                data[y] = initial.field_value(xx,yy);
                yy+= yy_step;
            }
            xx+=xx_step;
        }
    };

    unsigned int x_len,y_len;
    managedPtr<field_row> field_values;

};


//Actual Program:

int main() {

    simpleWave wave(2*M_PI,2*M_PI,1.,0);

    field electricField(1024,1024,wave);

    electricField.print(10);

    return 0;
};