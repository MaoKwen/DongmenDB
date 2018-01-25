#ifndef DONGMENGSQL_H_
#define DONGMENGSQL_H_

#define SQL_NOTVALID (-1)
#define SQL_NULL (0)
#define SQL_INTEGER_1BYTE (1)
#define SQL_INTEGER_2BYTE (2)
#define SQL_INTEGER_4BYTE (4)
#define SQL_TEXT (13)

#define STMT_CREATE (0)
#define STMT_SELECT (1)
#define STMT_INSERT (2)
#define STMT_DELETE (3)

typedef struct dongmengsqlStatement
{
    /*
    bool explain;
    char *text;
    uint8_t type;*/
    /*
    union {
        Create_t *create;
        SRA_t    *select;
        Insert_t *insert;
        Delete_t *delete;
    } stmt;*/
} dongmengsqlStatement_t;

int dongmengsqlParser(const char *sql, dongmengsqlStatement_t **stmt);
int dongmengsqlStmtPrint(dongmengsqlStatement_t *stmt);

#endif /* SQL_TYPES_H_ */
