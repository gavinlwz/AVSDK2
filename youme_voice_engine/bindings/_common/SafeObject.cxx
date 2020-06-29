
#include "SafeObject.h"

SafeObject::SafeObject()
{
	this->mutex = tsk_mutex_create();
}

int SafeObject::Lock()const
{
	return tsk_mutex_lock(this->mutex);
}

int SafeObject::UnLock()const
{
	return tsk_mutex_unlock(this->mutex);
}

SafeObject::~SafeObject()
{
	tsk_mutex_destroy(&this->mutex);
}
