#define FALSE 0
#define TRUE 1

typedef unsigned char boolean_t;
typedef unsigned long db_addr_t;

extern db_addr_t db_disasm(db_addr_t loc, boolean_t altfmt, boolean_t flag16);
