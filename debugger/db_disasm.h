#define FALSE 0
#define TRUE 1

typedef unsigned char boolean_t;
typedef unsigned long db_addr_t;

extern db_addr_t db_disasm(unsigned int segment, db_addr_t loc,
                           boolean_t flag16);
