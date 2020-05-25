#ifndef _MemPool_H_INCLUDE
#define _MemPool_H_INCLUDE

#include <cstddef>
#include <vector>

class MemPool
{
public:
    MemPool ();
    virtual ~MemPool ();
    bool init (size_t unit_size, size_t unit_count);
    void *alloc ();
    bool free (void *unit);

    size_t getFreeUnitCount () const;
    size_t getUsedUnitCount () const;
    bool isFull () const;

private:
    MemPool &operator= (const MemPool &pool);

private:
    size_t m_unit_size;
    size_t m_unit_count;
    size_t mm_pool_size;
    char *m_pool;
    std::vector<void *> m_free_vec;
};

#endif
