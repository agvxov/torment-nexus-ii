ifeq ($(DEBUG),1)
    DEBUG := -ggdb -pg
endif

CMNF := -std=c++23
WARN := -Wall -Wpedantic -Wno-c99-extensions -Wno-extra-semi -Winvalid-pch

torment-nexus-ii.out: pch.hpp.pch main.cpp gui.h buttons.hpp
	clang++ -fuse-ld=mold -o $@ main.cpp -include-pch pch.hpp.pch -lraylib -ltbb ${CMNF} ${WARN} ${DEBUG}

pch.hpp.pch: pch.hpp entt.hpp
	clang++ -x c++-header -c -Xclang -emit-pch -o pch.hpp.pch pch.hpp ${CMNF}

clean:
	-${RM} pch.hpp.pch
	-${RM} torment-nexus-ii.out
