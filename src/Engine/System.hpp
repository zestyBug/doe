#if !defined(SYSTEM_HPP)
#define SYSTEM_HPP

namespace DOTS
{
    class System {
        friend class Register;
    protected:
        virtual void start(){};
        virtual void update(){};
        virtual void stop(){};
    public:
        System(){}
        virtual ~System(){}
    };
}

#endif // SYSTEM_HPP
