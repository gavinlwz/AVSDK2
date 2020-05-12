
#ifndef TINYWRAP_SAFEOBJECT_H
#define TINYWRAP_SAFEOBJECT_H

#include "tinyWRAP_config.h"

#include "tsk_mutex.h"

class TINYWRAP_API SafeObject
{
public:
	SafeObject();
	virtual ~SafeObject();

/* protected: */
	int Lock()const;
	int UnLock()const;

private:
	tsk_mutex_handle_t *mutex;
};

#endif /* TINYWRAP_SAFEOBJECT_H */
