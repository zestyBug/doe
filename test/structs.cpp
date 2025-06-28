#include "cutil/map.hpp"
#include "cutil/set.hpp"
#include "cutil/HashHelper.hpp"
#include "ECS/defs.hpp"
#include "cutil/mini_test.hpp"
#include <vector>

struct Archetype {
    const_span<ECS::TypeID> types;

    uint32_t getHash() const noexcept {
        uint32_t result = HashHelper::FNV1A32(this->types);
        if (result == 0xFFFFFFFF || result == 0)
            result = 1;
        return result;
    }
    /// @brief check if this archetype matches exact the same with param _types
    /// @note flag sensitive
    /// @param _types sorted array of types
    /// @return true uf matches
    inline bool operator ==(const_span<ECS::TypeID> _types) const noexcept {
        return (this->types) == _types;
    }
};

TEST(Test1) {
    map<const_span<ECS::TypeID>,Archetype> list;
    list.init(4);
    std::vector<Archetype> archs;
    // pointers must not be changed, otherwise map will not work currectly
    archs.reserve(100);

    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<int>()}));
    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<float>()}));
    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<char>()}));
    
    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<int,char>()}));
    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<float,char>()}));
    list.add(&archs.emplace_back(Archetype{ECS::componentTypes<float,int>()}));
    
    Archetype *v;
    uint32_t bbuffer = list.unoccupiedNodes();
    
    v = list.tryGet(ECS::componentTypes<int>());
    EXPECT_EQ(v, &archs[0]);
    EXPECT_EQ(list.occupiedNodes(), 6u);
    EXPECT_EQ(list.contains(&archs[0]), true);

    list.remove(&archs[0]);

    v = list.tryGet(ECS::componentTypes<int>());
    EXPECT_EQ(v, nullptr);
    EXPECT_EQ(list.occupiedNodes(), 5u);
    EXPECT_EQ(list.unoccupiedNodes(), bbuffer + 1);
    EXPECT_EQ(list.contains(&archs[0]), false);
}


TEST(Test2) {
    vector_set<uint32_t> vs;
    vs.insert(1);
    vs.insert(2);
    vs.insert(3);

    const uint32_t bbuffer = vs.unoccupiedNodes();
    int32_t i;
    

    i = vs.indexOf(4);
    EXPECT_EQ(i, -1);

    EXPECT_EQ(vs.occupiedNodes(), 3u);

    EXPECT_EQ(vs.contains(1), true);
    EXPECT_EQ(vs.contains(2), true);
    EXPECT_EQ(vs.contains(3), true);
    EXPECT_EQ(vs.contains(4), false);

    vs.remove(1);

    i = vs.indexOf(1);
    EXPECT_EQ(i, -1);

    EXPECT_EQ(vs.occupiedNodes(), 2u);
    EXPECT_EQ(vs.unoccupiedNodes(), bbuffer + 1);
    EXPECT_EQ(vs.contains(1), false);
}


int main() {
    mtest::run_all();
    return 0;
}