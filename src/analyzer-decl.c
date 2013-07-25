#include "../inc/analyzer-decl.h"

#include "../inc/debug.h"
#include "../inc/type.h"
#include "../inc/ast.h"
#include "../inc/sym.h"

#include "../inc/analyzer.h"
#include "../inc/analyzer-value.h"

#include "stdlib.h"
#include "string.h"

static void analyzerDeclParam (analyzerCtx* ctx, ast* Node);

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node);
static void analyzerDeclStruct (analyzerCtx* ctx, ast* Node);
static void analyzerDeclUnion (analyzerCtx* ctx, ast* Node);

/**
 * Handles any node tag that parserDeclExpr may produce by passing it
 * off to one of the following specialized handlers.
 */
static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base);

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType);
static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base);
static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base);

void analyzerDecl (analyzerCtx* ctx, ast* Node) {
    debugEnter("Decl");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        const type* DT = analyzerDeclNode(ctx, Current, BasicDT);

        if (Current->symbol) {
            /*Assign the type*/
            if (!Current->symbol->dt) {
                Current->symbol->dt = typeDeepDuplicate(DT);
                reportSymbol(Current->symbol);

            /*Not the first declaration of this symbol, check type matches*/
            } else if (!typeIsEqual(Current->symbol->dt, DT))
                analyzerErrorConflictingDeclarations(ctx, Current, Current->symbol, DT);

            /*Even if types match, not allowed to be redeclared unless a
              function*/
            else if (!typeIsFunction(DT))
                analyzerErrorRedeclaredVar(ctx, Node, Current->symbol);

        } else
            reportType(DT);
    }

    debugLeave();
}

const type* analyzerType (struct analyzerCtx* ctx, struct ast* Node) {
    debugEnter("Type");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    debugLeave();

    return Node->dt;
}

static void analyzerDeclParam (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclParam");

    const type* BasicDT = analyzerDeclBasic(ctx, Node->l);
    Node->dt = typeDeepDuplicate(analyzerDeclNode(ctx, Node->r, BasicDT));

    /*Don't bother checking types if already assigned
      Any conflicts will be reported by the function itself*/
    if (Node->symbol && !Node->symbol->dt)
        Node->symbol->dt = typeDeepDuplicate(Node->dt);

    debugLeave();
}

static const type* analyzerDeclBasic (analyzerCtx* ctx, ast* Node) {
    (void) ctx;

    debugEnter("DeclBasic");

    if (Node->tag == astDeclStruct)
        analyzerDeclStruct(ctx, Node);

    else if (Node->tag == astDeclUnion)
        analyzerDeclUnion(ctx, Node);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            Node->dt = typeCreateBasic(Node->symbol);

        else {
            debugErrorUnhandled("analyzerDeclBasic", "literal tag", literalTagGetStr(Node->litTag));
            Node->dt = typeCreateInvalid();
        }

    } else {
        if (Node->tag != astInvalid)
            debugErrorUnhandled("analyzerDeclBasic", "AST tag", astTagGetStr(Node->tag));

        Node->dt = typeCreateInvalid();
    }

    debugLeave();

    return Node->dt;
}

static void analyzerDeclStruct (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclStruct");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current);
    }

    /*TODO: check compatiblity
            of? redecls or something?*/

    debugLeave();
}

static void analyzerDeclUnion (analyzerCtx* ctx, ast* Node) {
    debugEnter("DeclUnion");

    for (ast* Current = Node->firstChild;
         Current;
         Current = Current->nextSibling) {
        analyzerDecl(ctx, Current);
    }

    debugLeave();
}

static const type* analyzerDeclNode (analyzerCtx* ctx, ast* Node, const type* base) {
    if (Node->tag == astInvalid) {
        debugMsg("Invalid");
        return Node->dt = typeCreateInvalid();

    } else if (Node->tag == astEmpty)
        return Node->dt = typeDeepDuplicate(base);

    else if (Node->tag == astBOP) {
        if (!strcmp(Node->o, "="))
            return analyzerDeclAssignBOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astUOP) {
        if (!strcmp(Node->o, "*"))
            return analyzerDeclPtrUOP(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "operator", Node->o);
            return Node->dt = typeCreateInvalid();
        }

    } else if (Node->tag == astCall)
        return analyzerDeclCall(ctx, Node, base);

    else if (Node->tag == astIndex)
        return analyzerDeclIndex(ctx, Node, base);

    else if (Node->tag == astLiteral) {
        if (Node->litTag == literalIdent)
            return analyzerDeclIdentLiteral(ctx, Node, base);

        else {
            debugErrorUnhandled("analyzerDeclNode", "literal tag", literalTagGetStr(Node->litTag));
            return Node->dt = typeCreateInvalid();
        }

    } else {
        debugErrorUnhandled("analyzerDeclNode", "AST tag", astTagGetStr(Node->tag));
        return Node->dt = typeCreateInvalid();
    }
}

static const type* analyzerDeclAssignBOP (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclAssignBOP");

    const type* L = analyzerDeclNode(ctx, Node->l, base);
    const type* R = analyzerValue(ctx, Node->r);

    if (!typeIsCompatible(R, L))
        analyzerErrorExpectedType(ctx, Node->r, "variable initialization", L, R);

    //const type* DT;

    //TODO: is assignable?
    //if (typeIs)

    Node->dt = typeDeepDuplicate(L);

    debugLeave();

    return Node->dt;
}

static const type* analyzerDeclPtrUOP (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclPtrUOP");

    Node->dt = typeCreatePtr(typeDeepDuplicate(base));
    const type* DT = analyzerDeclNode(ctx, Node->r, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclCall (analyzerCtx* ctx, ast* Node, const type* returnType) {
    debugEnter("DeclCall");

    /*Param types*/

    bool variadic = false;
    type** paramTypes = malloc(Node->children*sizeof(type*));
    ast* Current;
    int i;

    for (Current = Node->firstChild, i = 0;
         Current;
         Current = Current->nextSibling) {
        if (Current->tag == astEllipsis) {
            variadic = true;
            debugMsg("Ellipsis");

        } else if (Current->tag == astDeclParam) {
            analyzerDeclParam(ctx, Current);
            paramTypes[i++] = typeDeepDuplicate(Current->dt);

        } else
            debugErrorUnhandled("analyzerDeclCall", "AST tag", astTagGetStr(Current->tag));
    }

    /* */

    Node->dt = typeCreateFunction(typeDeepDuplicate(returnType), paramTypes, i, variadic);
    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIndex (analyzerCtx* ctx, ast* Node, const type* base) {
    debugEnter("DeclIndex");

    if (Node->r->tag == astEmpty)
        Node->dt = typeCreatePtr(typeDeepDuplicate(base));

    else {
        /*!!!*/
        analyzerValue(ctx, Node->r);
        int size =      Node->r->tag == astLiteral
                     && Node->r->litTag == literalInt
                   ? *(int*) Node->r->literal
                   : 10; //lol

        Node->dt = typeCreateArray(typeDeepDuplicate(base), size);
    }

    const type* DT = analyzerDeclNode(ctx, Node->l, Node->dt);

    debugLeave();

    return DT;
}

static const type* analyzerDeclIdentLiteral (analyzerCtx* ctx, ast* Node, const type* base) {
    (void) ctx;

    debugEnter("DeclIdentLiteral");

    Node->dt = typeDeepDuplicate(base);

    debugLeave();

    return Node->dt;
}
