//
// Created by Sam on 2018/2/13.
//

#include <parser/statement.h>

/**
 * 在现有实现基础上，实现delete from子句
 *
 *  支持的delete语法：
 *
 *  DELETE FROM <table_nbame>
 *  WHERE <logical_expr>
 *
 * 解析获得 sql_stmt_delete 结构
 */

sql_stmt_delete *parse_sql_stmt_delete(ParserT *parser){
    char *tableName = NULL;
    SRA_t *where = NULL;

    TokenT *token;

    // delete
    if (!matchToken(parser, TOKEN_RESERVED_WORD, "delete")) {
        return NULL;
    }

    // table_name
    token = parseNextToken(parser);
    if (token && token->type == TOKEN_WORD) {
        tableName = new_id_name();
        strcpy(tableName, token->text);
    } else {
        strcpy(parser->parserMessage, "invalid sql: missing table name.");
        return NULL;
    }
    parseEatAndNextToken(parser);

    // where <logical_expr>
    if (!matchToken(parser, TOKEN_RESERVED_WORD, "where")) {
        return NULL;
    }
    SRA_t *table = SRATable(TableReference_make(tableName, NULL));
    Expression *cond = parseExpressionRD(parser);
    where = SRASelect(table, cond);

    // return
    sql_stmt_delete *sqlStmtDelete = (sql_stmt_delete *) calloc(sizeof(sql_stmt_delete), 1);
    sqlStmtDelete->tableName = tableName;
    sqlStmtDelete->where = where;

    return sqlStmtDelete;
};