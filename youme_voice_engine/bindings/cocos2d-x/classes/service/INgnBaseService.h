
#ifndef __INGNBASESERVICE__
#define __INGNBASESERVICE__
class INgnBaseService
{
    public:
    INgnBaseService ()
    {
    }
    virtual ~INgnBaseService ()
    {
    }

    public:
    virtual bool start () = 0;
    virtual bool stop () = 0;
};
#endif
