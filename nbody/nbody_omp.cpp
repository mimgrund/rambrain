#include <iostream>
#include <cmath>
#include <cstdlib>
#include <omp.h>

#include <rambrain/rambrainDefinitionsHeader.h>
#include <rambrain/managedPtr.h>

using namespace std;
using namespace rambrain;


int main(int argc, char** argv)
{
    int N = 1000;
    if (argc >= 2)
    {
        N = atoi(argv[1]);
    }
    // Divide data into blocks, each containing blocksize particles
    int blocksize = 100;
    if (argc == 3)
    {
        blocksize = atoi(argv[2]);
    }
    const int nblocks = ceil(static_cast<double>(N) / blocksize);
    // In case N / blocksize is not an integer the last block will contain less particles
    const int lastBlocksize = N - blocksize * (nblocks - 1);

    // Initialize particle positions, velocities, masses inside a box of 1000 x 1000; Now two dimensional arrays, blockindex then particleindex
    managedPtr<double>* pos = new managedPtr<double>[nblocks];
    managedPtr<double>* vel = new managedPtr<double>[nblocks];
    managedPtr<double>* mass = new managedPtr<double>[nblocks];

    srand48(time(NULL));
    for (int b = 0; b < nblocks; ++b)
    {
        pos[b] = managedPtr<double>(3 * blocksize);
        vel[b] = managedPtr<double>(3 * blocksize);
        mass[b] = managedPtr<double>(blocksize);

        adhereTo<double> posGlue(pos[b]);
        adhereTo<double> velGlue(vel[b]);
        adhereTo<double> massGlue(mass[b]);

        double* posLoc = posGlue;
        double* velLoc = velGlue;
        double* massLoc = massGlue;

        int max = (b == nblocks - 1 ? lastBlocksize : blocksize);
        for (int i = 0; i < 3 * max; ++i)
        {
            posLoc[i] = drand48() * 1000.0;
            velLoc[i] = 0.0;
            if (i < max)
                massLoc[i] = (drand48() + 1.0) * 100.0;
        }
    }

    // Execute gravitational force on each particle pair using
    // v = v0 + a * t
    // x = x0 + v0 * t + 0.5 * a * t^2
    const double T = 1.0;
    const double dt = T * 1E-1;
    const double forceConst = 1E5;
    const double softening = 1.0;
    const int trackBlock = 0;
    const int track = 0;

    #pragma omp parallel
    if (omp_get_thread_num() == 0) {
        cout << "Running with " << omp_get_num_threads() << " threads" << endl;
    }

    for (double t = 0.0; t < T; t += dt)
    {

        #pragma omp for
        for (int b = 0; b < nblocks; ++b)
        {

            adhereTo<double> posGlueB(pos[b]);
            adhereTo<double> velGlueB(vel[b]);
            const adhereTo<double> massGlueB(mass[b]);

            double* posLocB = posGlueB;
            double* velLocB = velGlueB;
            const double* massLocB = massGlueB;

            for (int c = b; c < nblocks; ++c) // Use symmetry
            {

                adhereTo<double> posGlueC(pos[c]);
                adhereTo<double> velGlueC(vel[c]);
                const adhereTo<double> massGlueC(mass[c]);

                double* posLocC = posGlueC;
                double* velLocC = velGlueC;
                const double* massLocC = massGlueC;

                int imax = (b == nblocks - 1 ? lastBlocksize : blocksize);
                for (int i = 0; i < imax; ++i)
                {
                    int jmin = (c == b ? i + 1 : 0); // Use symmetry if both partners are in the same block
                    int jmax = (c == nblocks - 1 ? lastBlocksize : blocksize);
                    for (int j = jmin; j < jmax; ++j)
                    {
                        // Apply force between particle i and j
                        double* pi = &(posLocB[3*i]), * pj = &(posLocC[3*j]);
                        double* vi = &(velLocB[3*i]), * vj = &(velLocC[3*j]);
                        double mi = massLocB[i], mj = massLocC[j];

                        // Softening to prevent singularity
                        double distanceVector[3] = {pi[0] - pj[0], pi[1] - pj[1], pi[2] - pj[2]};
                        double distance = fabs(distanceVector[0]*distanceVector[0] + distanceVector[1]*distanceVector[1] + distanceVector[2]*distanceVector[2]) + softening;
                        double force = forceConst / (distance * sqrt(distance));

                        // Adjust position
                        for (int k = 0; k < 3; ++k)
                        {
                            pi[k] -= vi[k] * dt + 0.5 * force / mi * distanceVector[k] * dt * dt;
                            pj[k] += vj[k] * dt + 0.5 * force / mj * distanceVector[k] * dt * dt;
                        }

                        // Adjust velocity
                        for (int k = 0; k < 3; ++k)
                        {
                            vi[k] -= force / mi * distanceVector[k] * dt;
                            vj[k] += force / mj * distanceVector[k] * dt;
                        }
                    }
                }
            }
        }

        const adhereTo<double> posGlue(pos[trackBlock]);
        const adhereTo<double> velGlue(vel[trackBlock]);

        const double* posLoc = posGlue;
        const double* velLoc = velGlue;

        // Report
        cout << "Time: " << t << ": Particle " << track << " of block " << trackBlock
             << ": Pos = (" << posLoc[track*3] << "," << posLoc[track*3+1] << "," << posLoc[track*3+2]
             << "); Vel = (" << velLoc[track*3] << "," << velLoc[track*3+1] << "," << velLoc[track*3+2] << ")" << endl;
    }

    // Done, cleanup
    delete[] mass;
    delete[] vel;
    delete[] pos;

    cout << "Done, exiting... " << endl;
    return 0;
}
