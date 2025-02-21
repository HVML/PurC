set(EBTREE_DIR "${THIRDPARTY_DIR}/ebtree")

set(EBTREE_SOURCES
    ${EBTREE_DIR}/ebtree/ebtree.c
    ${EBTREE_DIR}/ebtree/eb32tree.c
    ${EBTREE_DIR}/ebtree/eb64tree.c
    ${EBTREE_DIR}/ebtree/ebmbtree.c
    ${EBTREE_DIR}/ebtree/ebsttree.c
    ${EBTREE_DIR}/ebtree/ebimtree.c
    ${EBTREE_DIR}/ebtree/ebistree.c
)

add_library(ebtree STATIC ${EBTREE_SOURCES})

PURC_ADD_TARGET_C_FLAGS(ebtree -DCONFIG_REGPARM=0 -DBITS_PER_LONG=${BITS_PER_LONG} -Wno-cast-align)

if (COMPILER_IS_GCC_OR_CLANG)
    PURC_ADD_TARGET_C_FLAGS(ebtree -Wno-gnu-pointer-arith)
endif ()

set(EBTREE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ebtree/ebtree")
