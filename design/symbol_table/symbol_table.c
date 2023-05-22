#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "symbol_table.h"
#include "../semantics/semantics.h"

/* current scope */

int current_scope = 0;
int declare = 0; // 1: declaring, 0: not
/* flag variable for function declaring */
int function_decl = 0; // 1: declaring function, 0: not
void init_symbol_table()
{
    symbol_table = malloc(sizeof(ListNode *) * HASHTABLESIZE);
    for (int i = 0; i < HASHTABLESIZE; i++)
    {
        symbol_table[i] = NULL;
    }
    printf("initialized symbol table \n");
}

unsigned int hash(char *key)
{
    unsigned int hashval = 0;
    for (int i = 0; i < strlen(key); i++)
    {
        hashval += key[i] * pow(37, i); // this hash function is used by java
    }

    return hashval % HASHTABLESIZE;
}

void insert(char *name, int lineno, int length, int type)
{
    // get the hash value of the token
    unsigned int hashval = hash(name);
    // printf("hash value is %d \n", hashval);
    // get the list at the hash value
    ListNode *list = symbol_table[hashval];
    while (list != NULL && (strcmp(name, list->name) != 0)) // go to the end of the list
    {
        // printf("name on list: %s, name given: %s\n", list->name, name);

        list = list->next;
    }

    if (list == NULL) // if the list is empty
    {
        /* check if we are really declaring */
        if (declare == 1)
        {
            /* set up entry */
            list = (ListNode *)malloc(sizeof(ListNode));
            strcpy(list->name, name);
            list->scope = current_scope;
            list->stype = type;
            list->lines = (Ref *)malloc(sizeof(Ref));
            list->lines->lineNo = lineno;
            list->lines->next = NULL;
            /* add to hashtable */
            list->next = symbol_table[hashval];
            symbol_table[hashval] = list;
            // printf("Inserted %s for the first time with linenumber %d!\n", name, lineno);
        }
        else
        {
            /* add it to check it again later  as function call */
            list = (ListNode *)malloc(sizeof(ListNode));
            strcpy(list->name, name);
            list->stype = type;
            list->scope = current_scope;
            list->lines = (Ref *)malloc(sizeof(Ref));
            list->lines->lineNo = lineno;
            list->lines->next = NULL;
            list->next = symbol_table[hashval];
            symbol_table[hashval] = list;

            /* Adding identifier to the revisit queue! */
            // printf("** %s added to queue with linenumber %d!\n", name, lineno);
            add_to_queue(list, list->name, PARAM_CHECK);
        }
    }
    /* found in table */
    else
    {
        // just add line number
        if (declare == 0)
        { // here we refrance for Identifier
            /* find last reference */
            Ref *t = list->lines;
            while (t->next != NULL)
                t = t->next;

            /* add linenumber to reference list */
            t->next = (Ref *)malloc(sizeof(Ref));
            t->next->lineNo = lineno;
            t->next->next = NULL;
            // printf("Found %s again at line %d!\n", name, lineno);
        }
        /* new entry */ // identification for same variable for the scond time
        else
        {
            /* same scope - multiple declaration error! */
            if (list->scope == current_scope)
            {
                fprintf(stderr,
                        "A multiple declaration of variable %s at line %d\n",
                        name, lineno);
                exit(1);
            }
            /* other scope - but function declaration */
            else if (function_decl == 1)
            {
                /* find last reference */
                Ref *t = list->lines;
                while (t->next != NULL)
                    t = t->next;

                /* add linenumber to reference list */
                t->next = (Ref *)malloc(sizeof(Ref));
                t->next->lineNo = lineno;
                t->next->next = NULL;
            }
            /* other scope - create new entry */
            else
            {
                /* set up entry */
                list = (ListNode *)malloc(sizeof(ListNode));
                strcpy(list->name, name);
                list->scope = current_scope;
                list->stype = type;
                list->lines = (Ref *)malloc(sizeof(Ref));
                list->lines->lineNo = lineno;
                list->lines->next = NULL;
                /* add to hashtable */
                list->next = symbol_table[hashval];
                symbol_table[hashval] = list;
                // printf("Inserted %s for a new scope with linenumber %d!\n", name, lineno);
            }
        }
    }
}

ListNode *lookup(char *name)
{
    // get the hash value of the token
    unsigned int hashval = hash(name);
    // get the list at the hash value
    ListNode *list = symbol_table[hashval];

    while (list != NULL && (strcmp(name, list->name) != 0)) // go to the end of the list
    {
        list = list->next;
    }

    return list; // if NULL , the token is not in the list
}

void set_type(char *name, int stype, int inf_type)
{
    /* lookup entry */
    ListNode *l = lookup(name);
    //    printf("setting type of %s to %d\n", name, stype);
    /* set "main" type */
    l->stype = stype;

    /* if array, pointer or function */
    if (inf_type != UNDEF)
    {
        l->inf_type = inf_type;
    }
}

int get_type(char *name)
{
    /* lookup entry */
    ListNode *l = lookup(name);

    /* if "simple" type */
    if (l->stype == INT_TYPE || l->stype == REAL_TYPE || l->stype == CHAR_TYPE || l->stype == STR_TYPE || l->stype == BOOL_TYPE)
    {
        return l->stype;
    }
    /* if array, pointer or function */
    else
    {
        return l->inf_type;
    }
    printf("getting type of %s\n", name);
}

ListNode *lookup_scope(char *name, int scope)
{
    // get the hash value of the token
    unsigned int hashval = hash(name);
    // get the list at the hash value
    ListNode *list = symbol_table[hashval];

    while (list != NULL && (strcmp(name, list->name) != 0) && list->scope != scope)
    {
        list = list->next;
    }

    return list; // if NULL , the token is not in the list
}

void hide_scope()
{ /* hide the current scope */
    ListNode *list;
    int i;
    // printf("Hiding scope \'%d\':\n", current_scope);
    /* for all the lists */
    for (i = 0; i < HASHTABLESIZE; i++)
    {
        if (symbol_table[i] != NULL)
        {
            list = symbol_table[i];
            /* Find the first item that is from another scope */
            while (list != NULL && list->scope == current_scope)
            {
                // printf("Hiding %s..\n", list->name);
                list = list->next;
            }
            /* Set the list equal to that item */
            symbol_table[i] = list;
        }
    }
    current_scope--;
}

void incr_scope()
{ /* go to next scope */
    current_scope++;
}

Prameter def_param(int par_type, char *param_name, int pass)
{
    Prameter param; /* Parameter struct */

    /* set the information */
    param.type = par_type;
    strcpy(param.name, param_name);
    param.pass = pass;

    /* return the structure */
    return param;
}

int func_declare(char *name, int ret_type,
                 int num_of_pars, Prameter *parameters)
{
    /* lookup entry */
    ListNode *l = lookup(name);
    if (l != NULL)
    { /* if type is not defined yet */
        if (l->stype == UNDEF)
        {
            /* entry is of function type */
            l->stype = FUNCTION_TYPE;

            /* return type is ret_type */
            l->inf_type = ret_type;

            /* parameter stuff */
            l->num_of_params = num_of_pars;
            l->params = parameters;

            return 0; /* success */
        }
        /* already declared error */
        else
        {
            fprintf(stderr, "Function %s already declared!\n", name);
            exit(1);
        }
    }
}

int func_param_check(char *name, int num_of_calls,
                     int **par_types, int *num_of_pars)
{
    int i, type_1, type_2;
    int j;

    /* lookup entry */
    ListNode *l = lookup(name);
    /* for all function calls */
    for (i = 0; i < num_of_calls; i++)
    {
        /* check number of parameters */
        if (l->num_of_params != num_of_pars[i])
        {
            fprintf(stderr,
                    "Function call of %s has wrong num of parameters!\n",
                    name);
            exit(1);
        }
        /* check if parameters are compatible */
        for (j = 0; j < num_of_pars[i]; j++)
        {
            /* type of parameter in function declaration */
            type_1 = l->params[j].type;

            /* type of parameter in function call*/
            type_2 = par_types[i][j];

            /* check compatibility for function call */
            get_result_type(type_1, type_2, NONE);
            /* error occurs automatically in the function */
        }
    }

    return 0; /* success */
}

void dump_symboltable(FILE *output)
{
    fprintf(output, "------------ -------------- ------ ------------\n");
    fprintf(output, "Name         Type           Scope  Line Numbers\n");
    fprintf(output, "------------ -------------- ------ ------------\n");
    for (int i = 0; i < HASHTABLESIZE; i++)
    {
        if (symbol_table[i] != NULL)
        {
            ListNode *list = symbol_table[i];
            while (list != NULL)
            {
                fprintf(output, "%-13s", list->name);
                if (list->stype == INT_TYPE)
                    fprintf(output, "%-15s", "INT");
                else if (list->stype == REAL_TYPE)
                    fprintf(output, "%-15s", "REAL");
                else if (list->stype == CHAR_TYPE)
                    fprintf(output, "%-15s", "CHAR");
                else if (list->stype == STR_TYPE)
                    fprintf(output, "%-15s", "STRING");
                else if (list->stype == ARRAY_TYPE)
                    fprintf(output, "%-15s", "ARRAY");
                // else if (list->stype == 7)  fprintf(output,"%-15s","FUNCTION");
                else if (list->stype == VOID_TYPE)
                    fprintf(output, "%-15s", "VOID");
                else if (list->stype == BOOL_TYPE)
                    fprintf(output, "%-15s", "BOOLEAN");

                // for function type
                else if (list->stype == FUNCTION_TYPE)
                {
                    fprintf(output, "func ret ");
                    if (list->inf_type == INT_TYPE)
                        fprintf(output, "%-6s", "INT");
                    else if (list->inf_type == REAL_TYPE)
                        fprintf(output, "%-6s", "REAL");
                    else if (list->inf_type == STR_TYPE)
                        fprintf(output, "%-6s", "STRING");
                    else if (list->inf_type == CHAR_TYPE)
                        fprintf(output, "%-6s", "CHAR");
                    else if (list->inf_type == VOID_TYPE)
                        fprintf(output, "%-6s", "VOID");
                    else if (list->inf_type == BOOL_TYPE)
                        fprintf(output, "%-6s", "BOOLEAN");
                    else
                        fprintf(output, "%-4s", "UNDEF");
                }

                else
                    fprintf(output, "%-15s", "UNDEF"); // if UNDEF or 0
                fprintf(output, "   %d   ", list->scope);
                Ref *temp = list->lines;
                while (temp != NULL)
                {
                    fprintf(output, "%-6d ", temp->lineNo);
                    // i++;
                    temp = temp->next;
                }
                fprintf(output, "\n");
                list = list->next;
            }
        }
    }
}

// Revisit Queue Functions

void add_to_queue(ListNode *entry, char *name, int type)
{ /* add to queue */
    revisit_queue *q;

    /* queue is empty */
    if (queue == NULL)
    {
        /* set up entry */
        q = (revisit_queue *)malloc(sizeof(revisit_queue));
        q->entry = entry;
        q->st_name = name;
        q->revisit_type = type;
        q->next = NULL;
        /* additional info */
        if (type == PARAM_CHECK)
        {
            q->num_of_calls = 0;
        }

        /* q "becomes" the queue */
        queue = q;
    }
    /* queue not empty */
    else
    {
        /* find last element */
        q = queue;
        while (q->next != NULL)
            q = q->next;

        /* add element to the end */
        q->next = (revisit_queue *)malloc(sizeof(revisit_queue));
        q->next->entry = entry;
        q->next->st_name = name;
        q->next->revisit_type = type;
        q->next->next = NULL;
        /* additional info */
        if (type == PARAM_CHECK)
        {
            q->next->num_of_calls = 0;
        }
    }
}

revisit_queue *search_queue(char *name)
{
    revisit_queue *q;
    /* search for the entry */
    q = queue;
    while ((q != NULL) && strcmp(q->st_name, name) != 0)
        q = q->next;

    return q;
}
revisit_queue *search_prev_queue(char *name)
{
    revisit_queue *q;

    /* queue is empty */
    if (queue == NULL)
    {
        return NULL;
    }

    /* special case - first entry */
    if (strcmp(queue->st_name, name) == 0)
    {
        return NULL;
    }

    /* search for the entry */
    q = queue;
    while ((q != NULL) && (strcmp(q->next->st_name, name) != 0))
        q = q->next;

    return q;
}
// This function will be called when functions get declared, to take care of function calls that has not been checked yet!
int revisit(char *name)
{ /* revisit entry by also removing it from queue */
    revisit_queue *q = search_queue(name);

    if (q == NULL)
    {
        return -1; // revisit is not needed
    }

    /* revisit entry depending on the type */
    switch (q->revisit_type)
    {
    case PARAM_CHECK:
        /* run parameter check */
        if (!func_param_check(name, q->num_of_calls, q->par_types, q->num_of_pars))
        {
            printf("Successful Parameter Check of %s\n", name);
        }

        /* remove entry by making it point to it's next */
        revisit_queue *q2 = search_prev_queue(name);
        if (q2 == NULL)
        { /* special case: first entry */
            queue = queue->next;
        }
        else
        {
            q2->next = q2->next->next;
        }

        break;
    case ASSIGN_CHECK:
        /* run assignment check */
        break;
        /* ... */
    }

    return 0; // success
}

void revisit_dump(FILE *of)
{
    int i;
    revisit_queue *q;
    q = queue;

    fprintf(of, "------------ -------------\n");
    fprintf(of, "Identifier   Revisit Type\n");
    fprintf(of, "------------ -------------\n");
    while (q != NULL)
    {
        fprintf(of, "%-13s", q->st_name);
        if (q->revisit_type == PARAM_CHECK)
        {
            fprintf(of, "%s", "Parameter Check");
            fprintf(of, "for %d function calls", q->num_of_calls);
        }
        // more later on
        fprintf(of, "\n");
        q = q->next;
    }
}