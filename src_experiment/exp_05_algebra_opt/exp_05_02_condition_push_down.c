//
// Created by Sam on 2018/2/13.
//

#include <dongmensql/optimizer.h>

/**
 * 使用关于选择的等价变换规则将条件下推。
 *
 */

bool containFields(SRA_t *sra, Expression *cond) {
    return false;
}

/**
 * cnfPartition 递归地遍历合取范式的左右分支，拆分条件并把 *sra 包裹成 SRA_SELECT
 * @param expr 表达式
 * @param sra 输出参数，sra 的地址
 * @return expression 的下一个节点
 *
 * Select((((a = 1) AND (b = 2)) AND (c = 3)),
 *      Table(F))
 * ==>
 * Select((c = 3),
 *    Select((a = 1),
 *       Select((b = 2),
 *          Table(F))))
 */

Expression *skipSubexpression(Expression *expr);
Expression *findBefore(Expression *begin, Expression *end);
void splitCNF(Expression *expr, SRA_t **sra);

/**
 * 等价变换：将满足条件的 SRA_SELECT 类型的节点进行条件串接
 */
void selectCNFOptimize(SRA_t **sra);


/*输入一个关系代数表达式，输出优化后的关系代数表达式
 * 要求：在查询条件符合合取范式的前提下，根据等价变换规则将查询条件移动至合适的位置。
 * */
SRA_t *dongmengdb_algebra_optimize_condition_pushdown(SRA_t *sra) {
    selectCNFOptimize(&sra);
    return sra;
}

Expression *skipSubexpression(Expression *expr) {
    if (!expr) return NULL;
    if (expr->term)
        return expr->nextexpr;

    if (expr->opType <= TOKEN_COMMA) {
        int op_num = operators[expr->opType].numbers;
        expr = expr->nextexpr;
        while (op_num--)
            expr = skipSubexpression(expr);

        return expr;
    }

    return NULL;
}

Expression *findBefore(Expression *begin, Expression *end) {
    Expression *iter = begin;
    while (iter->nextexpr != end && iter->nextexpr != NULL)
        iter = iter->nextexpr;
    return iter;
}

bool isCnf(Expression *expr) {
    if (expr->opType == TOKEN_AND) return true;
    return false;
}

void splitCNF(Expression *expr, SRA_t **sra) {
    Expression *left = expr->nextexpr;
    Expression *right = skipSubexpression(left);

    Expression *left_tail = findBefore(left, right);
    left_tail->nextexpr = NULL;

    (*sra)->select.sra = SRASelect((*sra)->select.sra, left);
    (*sra)->select.cond = right;
}

void selectCNFOptimize(SRA_t **sra) {
    SRA_t *s = *sra;
    if (!s) return;
    switch (s->t) {
        case SRA_SELECT:
            if (isCnf(s->select.cond))
                splitCNF(s->select.cond, sra);
            selectCNFOptimize(&(s->select.sra));
            break;
        case SRA_PROJECT:
            selectCNFOptimize(&(s->project.sra));
            break;
        case SRA_JOIN:
            selectCNFOptimize(&(s->join.sra1));
            selectCNFOptimize(&(s->join.sra2));
            break;
        default:
            break;
    }
}