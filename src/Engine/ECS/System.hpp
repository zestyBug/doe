#if !defined(SYSTEM_HPP)
#define SYSTEM_HPP

namespace DOTS
{
    // template for defining new system
    class System {
        friend class Register;
    protected:
        virtual void start(){};
        // systems can be whether multi threaded via a threadpool
        // or simply single threaded
        virtual void update(){};
        virtual void stop(){};
    public:
        System(){}
        virtual ~System(){}
    };
}

#endif // SYSTEM_HPP
