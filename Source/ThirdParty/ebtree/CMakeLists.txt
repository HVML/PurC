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

PURC_ADD_TARGET_C_FLAGS(ebtree -DBITS_PER_LONG=${BITS_PER_LONG})

set(EBTREE_INCLUDE_DIRS "${EBTREE_DIR}" PARENT_SCOPE)
