#ifndef STDINC_H_3cb5a54f_fb1f_45b9_9235_3dfc7c485131
#define STDINC_H_3cb5a54f_fb1f_45b9_9235_3dfc7c485131


#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  \
    {                   \
        if (p)          \
        {               \
            delete (p); \
            (p) = NULL; \
        }               \
    }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) \
    {                        \
        if (p)               \
        {                    \
            delete[](p);     \
            (p) = NULL;      \
        }                    \
    }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      \
    {                        \
        if (p)               \
        {                    \
            (p)->Release (); \
            (p) = NULL;      \
        }                    \
    }
#endif

#ifndef STRLEN
#define STRLEN(s) ((s) ? strlen ((s)) : 0)
#endif


#endif // STDINC_H_3cb5a54f_fb1f_45b9_9235_3dfc7c485131
