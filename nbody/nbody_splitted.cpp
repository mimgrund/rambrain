#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;


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
    double** pos = new double*[nblocks];
    double** vel = new double*[nblocks];
    double** mass = new double*[nblocks];

    srand48(time(NULL));
    for (int b = 0; b < nblocks; ++b)
    {
        pos[b] = new double[3 * blocksize];
        vel[b] = new double[3 * blocksize];
        mass[b] = new double[blocksize];
        int max = (b == nblocks - 1 ? lastBlocksize : blocksize);
        for (int i = 0; i < 3 * max; ++i)
        {
            pos[b][i] = drand48() * 1000.0;
            vel[b][i] = 0.0;
            if (i < max)
                mass[b][i] = (drand48() + 1.0) * 100.0;
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

    for (double t = 0.0; t < T; t += dt)
    {
        for (int b = 0; b < nblocks; ++b)
        {
            for (int c = b; c < nblocks; ++c) // Use symmetry
            {
                int imax = (b == nblocks - 1 ? lastBlocksize : blocksize);
                for (int i = 0; i < imax; ++i)
                {
                    int jmin = (c == b ? i + 1 : 0); // Use symmetry if both partners are in the same block
                    int jmax = (c == nblocks - 1 ? lastBlocksize : blocksize);
                    for (int j = jmin; j < jmax; ++j)
                    {
                        // Apply force between particle i and j
                        double* pi = &(pos[b][3*i]), * pj = &(pos[c][3*j]);
                        double* vi = &(vel[b][3*i]), * vj = &(vel[c][3*j]);
                        double mi = mass[b][i], mj = mass[c][j];

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

        // Report
        cout << "Time: " << t << ": Particle " << track << " of block " << trackBlock
             << ": Pos = (" << pos[trackBlock][track*3] << "," << pos[trackBlock][track*3+1] << "," << pos[trackBlock][track*3+2]
             << "); Vel = (" << vel[trackBlock][track*3] << "," << vel[trackBlock][track*3+1] << "," << vel[trackBlock][track*3+2] << ")" << endl;
    }

    // Done, cleanup
    for (int b = 0; b < nblocks; ++b)
    {
        delete[] mass[b];
        delete[] vel[b];
        delete[] pos[b];
    }
    delete[] mass;
    delete[] vel;
    delete[] pos;

    cout << "Done, exiting... " << endl;
    return 0;
}
