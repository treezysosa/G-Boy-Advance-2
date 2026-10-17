#include <atomic>
#include <cstdlib>
#include <new>
std::atomic<bool> trackAllocations {false};
std::atomic<int> allocations {0};
void* operator new(std::size_t n) {if(trackAllocations) ++allocations;if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

