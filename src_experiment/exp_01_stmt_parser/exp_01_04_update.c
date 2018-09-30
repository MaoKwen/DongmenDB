//
// Created by Sam on 2018/2/13.
//

#include <parser/statement.h>

/**
 * 在现有实现基础上，实现update from子句
 *
 * 支持的update语法：
 *
 * UPDATE <table_name> SET <field1> = <expr1>[, <field2 = <expr2>, ..]
 * WHERE <logical_expr>
 *
 * 解析获得 sql_stmt_update 结构
 */

int parseAssignExpr(ParserT *parser, arraylist *fields, arraylist *fieldsExpr);

/* parse_sql_stmt_update， update语句解析*/
sql_stmt_update *parse_sql_stmt_update(ParserT *parser) {
    char *tableName = NULL;
    arraylist *fields = arraylist_create();
    arraylist *fieldsExpr = arraylist_create();
    SRA_t *where = NULL;
    SRA_t *tableExpr = NULL;

    TokenT *token;

    // update
    if (!matchToken(parser, TOKEN_RESERVED_WORD, "update")) {
        return NULL;
    }

    // table_name
    token = parseNextToken(parser);
    if (token->type == TOKEN_WORD) {
        tableName = new_id_name();
        strcpy(tableName, token->text);
    } else {
        strcpy(parser->parserMessage, "invalid sql: missing table name.");
        return NULL;
    }

    parseEatAndNextToken(parser);

    // set
    if (!matchToken(parser, TOKEN_RESERVED_WORD, "set")) {
        return NULL;
    }

    // <field1> = <expr1>[, <field2 = <expr2>, ..]
    if (!parseAssignExpr(parser, fields, fieldsExpr)) {
        return NULL;
    }

    // where
    token = parseNextToken(parser);
    TableReference_t *ref = TableReference_make(tableName, NULL);
    where = tableExpr = SRATable(ref);
    if (token != NULL && token->type != TOKEN_SEMICOLON) { // have a where expr
        if (!matchToken(parser, TOKEN_RESERVED_WORD, "where")) {
            return NULL;
        }
        Expression *cond = parseExpressionRD(parser);
        where = SRASelect(tableExpr, cond);
    }

    // return
    sql_stmt_update *sqlStmtUpdate = (sql_stmt_update *) calloc(sizeof(sql_stmt_update), 1);
    sqlStmtUpdate->tableName = tableName;
    sqlStmtUpdate->fields = fields;
    sqlStmtUpdate->fieldsExpr = fieldsExpr;
    sqlStmtUpdate->where = where;

    return sqlStmtUpdate;
};

int parseAssignExpr(ParserT *parser, arraylist *fields, arraylist *fieldsExpr) {
    TokenT *token = parseNextToken(parser);

    if (token == NULL || token->type != TOKEN_WORD) {
        strcpy(parser->parserMessage, "invalid sql: missing <field1> = <expr1>[, <field2 = <expr2>, ..].");
        return 0;
    }

    while (1) {
        // field
        if (token->type == TOKEN_WORD) {
            char *fieldName = new_id_name();
            strcpy(fieldName, token->text);
            arraylist_add(fields, fieldName);
            token = parseEatAndNextToken(parser);
            // =
            if (token->type == TOKEN_EQ) {
                parseEatAndNextToken(parser);
                // expr
                Expression *expr = parseExpressionRD(parser);
                arraylist_add(fieldsExpr, expr);
            } else {
                strcpy(parser->parserMessage, "invalid sql: missing '='.");
                return 0;
            }
        } else {
            strcpy(parser->parserMessage, "invalid sql: missing field name.");
            return 0;
        }

        token = parseNextToken(parser);
        if (token != NULL && token->type == TOKEN_COMMA) {
            token = parseEatAndNextToken(parser);
        } else {
            break;
        }
    }
    return 1;
};

