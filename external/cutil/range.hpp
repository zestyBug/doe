#ifndef __RANGE_HPP__
#define __RANGE_HPP__ 1

#include <cstdio>
#include <cstdint>


template<typename CT>
struct range final
{
    using value = CT;
    struct Iterator final {
        value VALUE;
        const value step;
        Iterator(value _v, value _step):VALUE{_v},step{_step} {}
        Iterator(const Iterator& obj) = default;
        Iterator& operator = (const Iterator& obj) = default;
        inline bool operator != (const Iterator &obj) const {
            // a little tricky
            return this->VALUE < obj.VALUE;
        }
        inline void operator ++ () {
            this->VALUE += this->step;
        }
        inline Iterator& operator += (const value b) {
            this->VALUE += b;
            return *this;
        }
        inline value operator * () const {
            return this->VALUE;
        }
        inline auto operator [] (const value i) const {
            const auto result = i * this->step + this->VALUE;
            return result;
        }
    };
    const value from;
    const value to;
    const value step;
    constexpr range(CT _from, CT _to, CT _step=1):from{_from},to{_to},step{_step}{};
    constexpr range(CT _to):from{0},to{_to},step{1}{};

    Iterator begin() const{
        return Iterator{this->from,this->step};
    }
    Iterator end() const{
        return Iterator{this->to,this->step};
    }
};

#endif