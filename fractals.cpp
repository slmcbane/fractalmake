#include "fractals.hpp"

#include <mutex>

namespace fractals
{

namespace
{
    unsigned domain_start = 0;
    std::mutex decomp_mutex;
}

void lock_decomposition_mutex()
{
    decomp_mutex.lock();
}

void unlock_decomposition_mutex()
{
    decomp_mutex.unlock();
}

unsigned& get_domain_start()
{
    return domain_start;
}

}
