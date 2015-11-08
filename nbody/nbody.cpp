#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace std;


int main(int argc, char** argv)
{
    int N = 1000;
    if (argc == 2)
    {
        N = atoi(argv[1]);
    }

    // Initialize particle positions, velocities, masses inside a box of 1000 x 1000
    double* pos = new double[3 * N];
    double* vel = new double[3 * N];
    double* mass = new double[N];

    srand48(time(NULL));
    for (int i = 0; i < 3 * N; ++i)
    {
        pos[i] = drand48() * 1000.0;
        vel[i] = 0.0;
        if (i < N)
            mass[i] = (drand48() + 1.0) * 100.0;
    }

    // Execute gravitational force on each particle pair using
    // v = v0 + a * t
    // x = x0 + v0 * t + 0.5 * a * t^2
    const double T = 1.0;
    const double dt = T * 1E-1;
    const double forceConst = 1E5;
    const double softening = 1.0;
    const int track = 0;

    for (double t = 0.0; t < T; t += dt)
    {
        for (int i = 0; i < N; ++i)
        {
            for (int j = i+1; j < N; ++j) // Use symmetry
            {
                // Apply force between particle i and j
                double* pi = &(pos[3*i]), * pj = &(pos[3*j]);
                double* vi = &(vel[3*i]), * vj = &(vel[3*j]);
                double mi = mass[i], mj = mass[j];

                // Softening to prevent singularity
                double distance = fabs((pi[0] - pj[0]) * (pi[0] - pj[0]) + (pi[1] - pj[1]) * (pi[1] - pj[1]) + (pi[2] - pj[2]) * (pi[2] - pj[2])) + softening;
                double force = forceConst / (distance * sqrt(distance));

                // Adjust position
                for (int k = 0; k < 3; ++k)
                {
                    pi[k] += vi[k] * dt + 0.5 * force / mi * (pj[k] - pi[k]) * dt * dt;
                    pj[k] += vj[k] * dt + 0.5 * force / mj * (pi[k] - pj[k]) * dt * dt;
                }

                // Adjust velocity
                for (int k = 0; k < 3; ++k)
                {
                    vi[k] += force / mi * (pj[k] - pi[k]) * dt;
                    vj[k] += force / mj * (pi[k] - pj[k]) * dt;
                }
            }
        }

        // Report
        cout << "Time: " << t << ": Particle " << track
             << ": Pos = (" << pos[track*3] << "," << pos[track*3+1] << "," << pos[track*3+2]
             << "); Vel = (" << vel[track*3] << "," << vel[track*3+1] << "," << vel[track*3+2] << ")" << endl;
    }

    // Done, cleanup
    delete[] mass;
    delete[] vel;
    delete[] pos;

    cout << "Done, exiting... " << endl;
    return 0;
}
