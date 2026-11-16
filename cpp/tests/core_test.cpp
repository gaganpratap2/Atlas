#include <cassert>

#include "atlas/core/version.hpp"

int main() {
    assert(atlas::version_major == 0);
    assert(atlas::version_minor == 1);
    assert(atlas::version_patch == 0);

    return 0;
}