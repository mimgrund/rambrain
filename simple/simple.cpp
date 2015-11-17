#include <math.h>
#include <stdio.h>




double field_value(double x, double y) {
    return sin(x)+sin(y);
};


int main() {

    const unsigned int x_len=1024,y_len=1024;
    const double x_max=2.*M_PI,y_max = 2.*M_PI;
    const unsigned int gp_step = 10.;


    double *field[x_len];

    //Allocation
    for(unsigned int x=0; x<x_len; ++x)
        field[x] = new double[y_len];

    //Stepping:
    double xx_step = x_max / x_len,xx = 0;
    double yy_step = y_max / y_len;

    //Initialization
    for(unsigned int x=0; x<x_len; ++x) {
        double yy = 0;


        for(unsigned int y=0; y<y_len; ++y) {
            field[x][y] = field_value(xx,yy);
            yy+= yy_step;
        }
        xx+=xx_step;
    }

    //Data output (gnuplottable)
    for(unsigned int x=0; x<x_len; x+=gp_step) {


        for(unsigned int y=0; y<y_len; y+=gp_step) {
            printf("%e\t",field[x][y]);
        }
        printf("\n");
    }


    //Deallocation
    for(unsigned int x=0; x<x_len; ++x)
        delete[] field[x];

    return 0;
};