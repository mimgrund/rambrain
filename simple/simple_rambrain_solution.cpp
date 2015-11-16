#include <math.h>
#include <stdio.h>
#include <rambrain/rambrainDefinitionsHeader.h>
#include <rambrain/managedPtr.h>			//<- We will use managedPtrs
using namespace rambrain;				//<- We will use classes without rambrain:: prefix

double field_value(double x, double y) {
    return sin(x)+sin(y);
};


int main() {

    const unsigned int x_len=1024,y_len=1024;
    const double x_max=2.*M_PI,y_max = 2.*M_PI;
    const unsigned int gp_step = 10.;


    managedPtr<double> *field[x_len];			//<- Initialize an array of pointers to managedPtrs

    //Allocation
    for(unsigned int x=0; x<x_len; ++x)
        field[x] = new managedPtr<double>(y_len);	//<- create an array of managedPtrs, not an array of double ptrs.

    //Stepping:
    double xx_step = x_max / x_len,xx = 0;
    double yy_step = y_max / y_len;

    //Initialization
    for(unsigned int x=0; x<x_len; ++x) {
        double yy = 0;
        adhereTo<double> glue(field[x]);		//<- Tell rambrain that the field row x is needed
        double *field_x = glue;				//<- Get an actual pointer to this row.
        for(unsigned int y=0; y<y_len; ++y) {
            field_x[y] = field_value(xx,yy);		//<- Operate on this row as usual
            yy+= yy_step;
        }
        xx+=xx_step;
    }

    //Data output (gnuplottable)
    for(unsigned int x=0; x<x_len; x+=gp_step) {
        const adhereTo<double> glue(field[x]);		//<- Tell rambrain that you will need the row const this time
        const double *field_x = glue;			//<- ...and only pull a const pointer.
        for(unsigned int y=0; y<y_len; y+=gp_step) {
            printf("%e\t",field_x[y]);
        }
        printf("\n");
    }


    //Deallocation
    for(unsigned int x=0; x<x_len; ++x)
        delete field[x];

    return 0;
};
