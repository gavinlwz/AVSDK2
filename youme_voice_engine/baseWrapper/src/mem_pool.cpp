#include "mem_pool.h"
#include <memory.h>
#include <stdlib.h>

MemPool::MemPool () : m_unit_size (0), m_unit_count (0), mm_pool_size (0), m_pool (NULL)
{
}

MemPool::~MemPool ()
{
    if (m_pool != NULL)
    {
        delete[] m_pool;
        m_pool = NULL;
    }
}

bool MemPool::init (size_t unit_size, size_t unit_count)
{
    if (m_free_vec.capacity () != unit_count)
    {
        m_free_vec.clear ();
        if (m_pool != NULL)
        {
            delete[] m_pool;
            m_pool = NULL;
        }
    }
    m_unit_size = unit_size;
    m_unit_count = unit_count;
    mm_pool_size = unit_size * unit_count;

    m_pool = new char[mm_pool_size];
    if (m_pool == NULL)
    {
        return false;
    }

    char *pool = m_pool;
    for (size_t i = 0; i < unit_count; i++)
    {
        m_free_vec.push_back ((void *)pool);
        pool += unit_size;
    }
    return true;
}

void *MemPool::alloc ()
{
    if (m_free_vec.empty ())
    {
        return NULL;
    }
    void *unit = m_free_vec.back ();
	memset(unit, 0, m_unit_size);
    
    m_free_vec.pop_back ();
    return unit;
}

bool MemPool::free (void *unit)
{
    char *pool_end = (char *)m_pool + mm_pool_size - 1;
    if (unit < m_pool || unit > (void *)pool_end)
    {
        return false;
    }

    m_free_vec.push_back (unit);
    return true;
}

size_t MemPool::getFreeUnitCount () const
{
    return m_free_vec.size ();
}

size_t MemPool::getUsedUnitCount () const
{
    return m_unit_count - m_free_vec.size ();
}

bool MemPool::isFull () const
{
    return m_free_vec.empty ();
}
