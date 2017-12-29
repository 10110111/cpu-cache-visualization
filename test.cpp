void work(int* mat, int size)
{
   for(int i=0; i<size; i++)
   {
       for(int j=0; j<size; j++)
       {
           if(0)
           {
               const int aux = mat[i*size+j];
               mat[i*size+j] = mat[j*size+i];
               mat[j*size+i] = aux;
           }
           else
           {
               volatile int x=mat[j*size+i];
           }
       }
   }
}

#include <chrono>
typedef long double Time;
Time currentTime()
{
    using namespace std::chrono;
#define TIME_UNIT "microseconds"
    return 1e6*duration<Time>(steady_clock::now().time_since_epoch()).count();
}

#include <algorithm>
#include <iostream>
#include <cstring>
#include <random>
#include <memory>
#include <cmath>

constexpr auto maxSize=700;
constexpr int nSamples=200;
constexpr int nTries=100;

int workspace[maxSize*maxSize];
Time test(int size)
{
    // touch the whole workspace to make initial conditions the same for all runs
    std::memset(workspace,0,sizeof workspace);

    const auto t0=currentTime();
    const auto realSampleCount = nSamples > 2*size ? nSamples/size : 2;
    for (int i=0; i<realSampleCount; i++)
        work(workspace, size);
    const auto elapsed=currentTime()-t0;
    return elapsed/realSampleCount;
}

int main()
{
    std::cout << "matrix side size," TIME_UNIT " per work()\n";

    std::array<int, maxSize> sizes;
    for(int i=0;i<maxSize;++i) sizes[i]=i+1;

    // index in timingsPerSize is size-1
    std::array<Time, maxSize> timingsPerSize;
    for(auto& timing : timingsPerSize) timing=INFINITY;

    std::random_device rd;
    std::mt19937 rng(rd());

    for(int tryNumber=0; tryNumber<nTries; ++tryNumber)
    {
        std::cerr << "Try " << tryNumber+1 << "...";
        // randomize the order in which we'll test different
        // sizes to prevent spurious correlations
        std::shuffle(sizes.begin(), sizes.end(), rng);
        for(const auto size : sizes)
        {
            const auto timing=test(size);
            // looking for minimum timing
            if(timingsPerSize[size-1]>timing)
                timingsPerSize[size-1]=timing;
        }
        std::cerr << "done\n";
    }
    for(int i=0;i<timingsPerSize.size();++i)
        std::cout << i+1 << "," << timingsPerSize[i] << "\n";
}
