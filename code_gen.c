#include "code_gen.h"

// Funkcie na prácu so zásobníkom parametrov

void ParamStackInit(TParamStack *s) {
    s->top = NULL;
}

void ParamStackPush(TParamStack *s, char *id) {
    // Alokovanie miesta pre nový parameter na zásobníku
    TParam *newParam = (TParam *) malloc(sizeof(TParam));
    if (newParam == NULL) return ;
    // Alokovanie miesta pre identifikátor parametra
    newParam->id = (char *) malloc(sizeof(char) * strlen(id));
    if (newParam->id == NULL) {
        free(newParam);
        return ;
    }
    strcpy(newParam->id, id);
    newParam->next = s->top;
    s->top = newParam;
}

void ParamStackPop(TParamStack *s) {
    TParam *paramPtr;
    if (s->top != NULL) {
        paramPtr = s->top;
        s->top = s->top->next;
        free(paramPtr->id);
        free(paramPtr);
    }
}
char * ParamStackTop(TParamStack *s) {
    return s->top->id;
}

bool ParamStackEmpty(TParamStack *s) {
    return (s->top == NULL);
}

// Funkcia na prípravu generovania kódu
int code_gen_start() {
    // inicializovať kam sa bude ukladať kód
    listInit(&instr_list);

    // Inicializovať štruktúru, do ktorej sa bude zapisovať aktuálna inštrukcia
    curr_instr = tInstr_init();

    // Inicializácia špeciálnej lokálnej tabuľky s návestiami
    local_table_init(&label_table);

    // Vložiť hlavičku, JUMP $main, LABEL $main
    set_and_post_instr(&instr_list, curr_instr, I_HEADER, NULL, NULL, NULL); // Hlavička
    set_and_post_instr(&instr_list, curr_instr, I_JUMP, "$main", NULL, NULL); // JUMP $main
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$main", NULL, NULL); // LABEL $main
    // Vytvorenie lokálneho rámca pre $main
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME

    return ERR_OK;
}

int code_gen_end() {

    // uvolniť štruktúru pre kód
    listFree(&instr_list);

    // uvoľniť štrukturú, v ktorej bola aktuálna inštrukcia
    tInst_free_instruction(curr_instr);

    // Uvoľnenie lokálnej tabuľky s návestiami
    local_table_dispose(&label_table);

    return ERR_OK;
}

char *get_string_with_prefix(char *str, char *prefix) {
    char *str_complete = malloc(sizeof(char) * (strlen(str) + strlen(prefix))+END_OF_STRING);
    if (str_complete == NULL) {
        // Chyba
        return NULL;
    }

    strcpy(str_complete, prefix);
    strcat(str_complete, str);

    return str_complete;
}

int add_instruction_ternal(tInstruction_type type, char *var_name, char *symbol1, char *symbol2, char *symbol1_prefix, char *symbol2_prefix) {
    if (var_name == NULL || symbol1 == NULL || symbol2 == NULL) // Chyba
        return ERR_INTERNAL;

    // Pred var a pred symboly dať predponu
    char *var_complete = get_string_with_prefix(var_name, VAR_PREFIX);
    char *symbol1_complete = get_string_with_prefix(symbol1, symbol1_prefix );
    char *symbol2_complete = get_string_with_prefix(symbol2, symbol2_prefix );
    if (var_complete == NULL || symbol1_complete == NULL || symbol2_complete == NULL) // Chyba pri alokácii
        return ERR_INTERNAL;

    // Nastaviť správne aktuálnu inštrukciu a vložiť inštrukciu do zoznamu
    set_and_post_instr(&instr_list, curr_instr, type, var_complete, symbol1_complete, symbol2_complete);

    // Uvoľniť miesto, kde boli var_complete, symbol1_complete, symbol2_complete
    free(var_complete);
    free(symbol1_complete);
    free(symbol2_complete);

    return ERR_OK;
}

int add_instruction_binary(tInstruction_type type, char *var_name, char *symbol, char *symbol_prefix) {
    if (var_name == NULL || symbol == NULL) // Chyba
        return ERR_INTERNAL;

    // Pred var a pred symboly dať predponu
    char *var_complete = get_string_with_prefix(var_name, VAR_PREFIX);
    char *symbol_complete = get_string_with_prefix(symbol, symbol_prefix );
    if (var_complete == NULL || symbol_complete == NULL) // Chyba pri alokácii
        return ERR_INTERNAL;

    // Nastaviť správne aktuálnu inštrukciu a vložiť inštrukciu do zoznamu
    set_and_post_instr(&instr_list, curr_instr, type, var_complete, symbol_complete, NULL);

    // Uvoľniť miesto, kde boli var_complete, symbol1_complete, symbol2_complete
    free(var_complete);
    free(symbol_complete);

    return ERR_OK;

}

int add_instruction_unary(tInstruction_type type, char *symbol, char *symbol_prefix) {
    if (symbol == NULL) // Chyba
        return ERR_INTERNAL;

    // Pred var a pred symboly dať predponu
    char *symbol_complete = get_string_with_prefix(symbol, symbol_prefix );
    if (symbol_complete == NULL) // Chyba pri alokácii
        return ERR_INTERNAL;

    // Nastaviť správne aktuálnu inštrukciu a vložiť inštrukciu do zoznamu
    set_and_post_instr(&instr_list, curr_instr, type, symbol_complete, NULL, NULL);

    // Uvoľniť miesto
    free(symbol_complete);

    return ERR_OK;
}

char *determine_prefix(tDataType type, bool is_var) {
    char *prefix;
    if (is_var) { // Premenná
        prefix = malloc(sizeof(char) * VAR_PREFIX_LEN+END_OF_STRING);
        if (prefix == NULL) return NULL;
        strcpy(prefix, VAR_PREFIX);
    }
    else { // INT/FLOAT
        if (type == T_INT) { // INT
            prefix = malloc(sizeof(char) * strlen("int@")+END_OF_STRING);
            if (prefix == NULL) return NULL;
            strcpy(prefix, "int@");
        }
        else if (type == T_FLOAT) { // FLOAT
            prefix = malloc(sizeof(char) * strlen("float@")+END_OF_STRING);
            if (prefix == NULL) return NULL;
            strcpy(prefix, "float@");
        }
        else if (type == T_STRING) {
            prefix = malloc(sizeof(char) * strlen("string@")+END_OF_STRING);
            if (prefix == NULL) return NULL;
            strcpy(prefix, "string@");
        }
        else if (type == T_BOOLEAN) {
            prefix = malloc(sizeof(char) * strlen("boolean@")+END_OF_STRING);
            if (prefix == NULL) return NULL;
            strcpy(prefix, "boolean@");
        }
        else { // NIL
            prefix = malloc(sizeof(char) * strlen("nil@")+END_OF_STRING);
            if (prefix == NULL) return NULL;
            strcpy(prefix, "nil@");
        }
    }
    return prefix;
}

int gen_add(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_ADD, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_sub(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_SUB, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_mul(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_MUL, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_div(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_DIV, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_idiv(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_IDIV, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_int2float(char *var_name, char *symbol, bool is_var) {
    // Získanie prefixu
    char *prefix;
    if (is_var) {
        prefix = malloc(sizeof(char) * VAR_PREFIX_LEN);
        if (prefix == NULL) // Chyba alokácie
            return ERR_INTERNAL;
        strcpy(prefix, VAR_PREFIX);
    }
    else {
        prefix = malloc(sizeof(char) * strlen("int@"));
        if (prefix == NULL) // Chyba alokácie
            return ERR_INTERNAL;
        strcpy(prefix, "int@");
    }

    int ret_val = add_instruction_binary(I_INT2FLOAT, var_name, symbol, prefix);

    // Uvoľnenie prefixu
    free(prefix);
    return ret_val;
}

int gen_concat(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_CONCAT, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}


int gen_defvar(char *var_name) {
    if (var_name == NULL) // Chyba
        return ERR_INTERNAL;

    // Pred var a pred symboly dať predponu
    char *var_complete = get_string_with_prefix(var_name, VAR_PREFIX);
    if (var_complete == NULL) // Chyba pri alokácii
        return ERR_INTERNAL;

    // Nastaviť správne aktuálnu inštrukciu a vložiť inštrukciu do zoznamu
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, var_complete, NULL, NULL);

    // Uvoľniť miesto, kde boli var_complete
    free(var_complete);

    return ERR_OK;
}

int gen_lt(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_LT, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_gt(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_GT, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

int gen_eq(char *var_name, char *symbol1, tDataType symbol1_type, bool s1_is_var, char *symbol2, tDataType symbol2_type, bool s2_is_var) {
    // Nastavenie prefixov
    char *prefix1 = determine_prefix(symbol1_type, s1_is_var);
    char *prefix2 = determine_prefix(symbol2_type, s2_is_var);

    int ret_val = add_instruction_ternal(I_EQ, var_name, symbol1, symbol2, prefix1, prefix2);

    // Uvoľnenie prefixov
    free(prefix1);
    free(prefix2);
    return ret_val;
}

void convert_int_2_float(tLocalTableNodePtr *actual_function_ptr, char *symbol, char **converted_name) {
    *converted_name = expr_parser_create_unique_name(*actual_function_ptr); // Získam meno premennej, do ktorej sa bude ukladať konverzia

    // Vytvorenie premennej v loc. tabuľke symbolov
    variable_set_defined(actual_function_ptr, *converted_name);
    variable_set_type(*actual_function_ptr, *converted_name, T_FLOAT);

    // Vygenerovanie premennej, ktorá vznikne pri konverzii
    gen_defvar(*converted_name);

    // Pridať inštrukciu INT2FLOAT
    gen_int2float(*converted_name, symbol, is_variable(*actual_function_ptr, symbol));
}

int gen_or(char *var_name, char *symbol1, char *symbol2) {
    return add_instruction_ternal(I_OR, var_name, symbol1, symbol2, VAR_PREFIX, VAR_PREFIX);
}

int gen_not(char *var_name, char *symbol) {
    return add_instruction_binary(I_OR, var_name, symbol, VAR_PREFIX);
}

int gen_move_var(char *var_name, char *symbol) {
    return add_instruction_binary(I_MOVE, var_name, symbol, VAR_PREFIX);
}

char *get_and_set_unique_label(tLocalTableNodePtr *label_table, char *prefix) {
    int n = 1;
    char *name = NULL;

    do {
        char *n_str = malloc(sizeof(char) * sizeof(int) * 4 + 1);
        name = realloc(name, sizeof(char) * (strlen(n_str) + strlen(prefix)));
        if (n_str)
            sprintf(n_str, "%d", n);

        if (name == NULL) // Chyba pri alokácii
            return NULL;

        strcpy(name, prefix);
        strcat(name, n_str);

        free(n_str);
        n++;
    } while (is_variable(*label_table, name));

    // Vložiť do lokálnej tabuľky symbolov
    variable_set_defined(label_table, name);

    return name;
}

int gen_push_var(char *symbol, tDataType type, bool is_var) {
    int ret;
    if (is_var)
        ret = add_instruction_unary(I_PUSHS, symbol, VAR_PREFIX);
    else if (type == T_INT)
        ret = add_instruction_unary(I_PUSHS, symbol, "int@");
    else if (type == T_FLOAT)
        ret = add_instruction_unary(I_PUSHS, symbol, "float@");
    else if (type == T_STRING)
        ret = add_instruction_unary(I_PUSHS, symbol, "string@");
    else if (type == T_NIL)
        ret = add_instruction_unary(I_PUSHS, symbol, "nil@");

    return ret;
}

int gen_pop_var(char *var_name) {
    return add_instruction_unary(I_POPS, var_name, VAR_PREFIX);
}

int gen_call(char *function_name) {
    return add_instruction_unary(I_CALL, function_name, FUNCTION_PREFIX);
}

int gen_function_header(char *function_name) {
    int ret = add_instruction_unary(I_LABEL, function_name, FUNCTION_PREFIX);
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL);
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL);
}

void prepare_for_func() {
    listFirst(&instr_list); // Aktívny je prvý -> .IFJcode2018
    listNext(&instr_list); // Aktívny je druhý -> JUMP $main
    // teraz sa bude vkladať za JUMP $main
}

void set_and_post_instr(tListOfInstr *L, tInstr *I, tInstruction_type type, char *addr1, char *addr2, char *addr3) {
    tInstr_set_instruction(I, type, addr1, addr2, addr3);
    listInsertPostActive(L, I);
}

void end_function() {
    // RETURN, POPFRAME a presune aktivitu na posledný prvok
    set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL);
    set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL);

    listLast(&instr_list);
}

void generate_built_in_function(char *function_id) {
    if (strcmp(function_id, "inputs") == 0) gen_inputs();
    else if (strcmp(function_id, "inputi") == 0) gen_inputi();
    else if (strcmp(function_id, "inputf") == 0) gen_inputf();
    else if (strcmp(function_id, "print") == 0) gen_print();
    else if (strcmp(function_id, "length") == 0) gen_length();
    else if (strcmp(function_id, "substr") == 0) gen_substr();
    else if (strcmp(function_id, "ord") == 0) gen_ord();
    else if (strcmp(function_id, "chr") == 0) gen_chr();
}

/** Generovanie vstavanej funkcie inputs **/
void gen_inputs() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie inputs", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?inputs", NULL, NULL); // LABEL ?inputs
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_READ, "LF@%ret", "string", NULL); // READ LF@%ret string
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavenj funkcie inputi **/
void gen_inputi() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie inputi", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?inputi", NULL, NULL); // LABEL ?inputi
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_READ, "LF@%ret", "int", NULL); // READ LF@%ret int
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie inputf **/
void gen_inputf() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie inputf", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?inputf", NULL, NULL); // LABEL ?inputf
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_READ, "LF@%ret", "float", NULL); // READ LF@%ret float
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie print **/
void gen_print() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie print", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?print", NULL, NULL); // LABEL ?print
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p1", NULL, NULL); // DEFVAR LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p1", NULL, NULL); // POPS LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_WRITE, "LF@%p1", NULL, NULL); // WRITE LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_PUSHS, "nil@%nil", NULL, NULL); // PUSHS nil@nil
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie length **/
void gen_length() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie length", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?length", NULL, NULL); // LABEL ?length
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p1", NULL, NULL); // DEFVAR LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p1", NULL, NULL); // POPS LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_STRLEN, "LF@%ret", "LF@%p1", NULL); // STRLEN LF@%ret LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_PUSHS, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie substr **/
void gen_substr() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie substr", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?substr", NULL, NULL); // LABEL ?substr
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p3", NULL, NULL); // DEFVAR LF@%p3
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p3", NULL, NULL); // POPS LF@%p3
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p2", NULL, NULL); // DEFVAR LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p2", NULL, NULL); // POPS LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p1", NULL, NULL); // DEFVAR LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p1", NULL, NULL); // POPS LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%compare", NULL, NULL); // DEFVAR LF@%compare
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%length", NULL, NULL); // DEFVAR LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%substr", NULL, NULL); // DEFVAR LF@%substr
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%substr", "string@x", NULL); // MOVE LF@%substr string@x
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%ret", "string@", NULL); // MOVE LF@%ret string@
    set_and_post_instr(&instr_list, curr_instr, I_STRLEN, "LF@%length", "LF@%p1", NULL); // STRLEN LF@%length LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_LT, "LF@%compare", "LF@%p2", "int@0"); // LT LF@%compare LF@%p2 int@0
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$substr$err", "LF@%compare", "bool@true"); // JUMPIFEQ $substr$err LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_GT, "LF@%compare", "LF@%p2", "LF@%length"); // GT LF@%compare LF@%p2 LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$substr$err", "LF@%compare", "bool@true"); // JUMPIFEQ $substr$err LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_LT, "LF@%compare", "LF@%p3", "int@0"); // LT LF@%compare LF@%p3 int@0
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$substr$err", "LF@%compare", "bool@true"); // JUMPIFEQ $substr$err LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%until", NULL, NULL); // DEFVAR LF@%until
    set_and_post_instr(&instr_list, curr_instr, I_ADD, "LF@%until", "LF@%p2", "LF@%p3"); // ADD LF@%until LF@%p2 LF@%p3
    set_and_post_instr(&instr_list, curr_instr, I_GT, "LF@%compare", "LF@%until", "LF@%length"); // GT LF@%compare LF@%until LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$substr$curb", "LF@%compare", "bool@true"); // JUMPIFNEQ $subst$curb LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%until", "LF@%length", NULL); // MOVE LF@%until LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$subst$curb", NULL, NULL); // LABEL $subst$curb
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%now", NULL, NULL); // DEFVAR LF@%now
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%now", "LF@%p2", NULL); // MOVE LF@%now LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$subst$loop", NULL, NULL); // LABEL $subst$loop
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$substr$noerr", "LF@%now", "LF@%until"); // JUMPIFEQ $substr$noerr LF@%now LF@%until
    set_and_post_instr(&instr_list, curr_instr, I_GETCHAR, "LF@%substr", "LF@%p1", "LF@%now"); // GETCHAR LF@%substr LF@%p1 LF@%now
    set_and_post_instr(&instr_list, curr_instr, I_CONCAT, "LF@%ret", "LF@%ret", "LF@%substr"); // CONCAT LF@%ret LF@%ret LF@%substr
    set_and_post_instr(&instr_list, curr_instr, I_ADD, "LF@%now", "LF@%now", "int@1"); // ADD LF@%now LF@%now int@1
    set_and_post_instr(&instr_list, curr_instr, I_JUMP, "$substr$loop", NULL, NULL); // JUMP $substr$loop
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$subst$err", NULL, NULL); // LABEL $substr$err
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%ret", "nil@nil", NULL); // MOVE LF@%ret nil@nil
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$subst$noerr", NULL, NULL); // LABEL $substr$noerr
    set_and_post_instr(&instr_list, curr_instr, I_PUSHS, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie ord **/
void gen_ord() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie ord", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?ord", NULL, NULL); // LABEL ?ord
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p2", NULL, NULL); // DEFVAR LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p2", NULL, NULL); // POPS LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p1", NULL, NULL); // DEFVAR LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p1", NULL, NULL); // POPS LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%compare", NULL, NULL); // DEFVAR LF@%compare
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%length", NULL, NULL); // DEFVAR LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_STRLEN, "LF@%length", "LF@%p1", NULL); // STRLEN LF@%length LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_SUB, "LF@%length", "LF@%length", "int@1"); // SUB LF@%length LF@%length int@1
    set_and_post_instr(&instr_list, curr_instr, I_LT, "LF@%compare", "LF@%p2", "int@0"); // LT LF@%compare LF@%p2 int@0
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$ord$err", "LF@%compare", "bool@true"); // JUMPIFEQ $ord$err LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_GT, "LF@%compare", "LF@%p2", "LF@%length"); // GT LF@%compare LF@%p2 LF@%length
    set_and_post_instr(&instr_list, curr_instr, I_JUMPIFEQ, "$ord$err", "LF@%compare", "bool@true"); // JUMPIFEQ $ord$err LF@%compare bool@true
    set_and_post_instr(&instr_list, curr_instr, I_STRI2INT, "LF@%ret", "LF@%ret", "LF@%p2"); // STRI2INT LF@%ret LF@%p1 LF@%p2
    set_and_post_instr(&instr_list, curr_instr, I_JUMP, "$ord$noerr", NULL, NULL); // JUMP $ord$noerr
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$ord$err", NULL, NULL); // LABEL $orr$err
    set_and_post_instr(&instr_list, curr_instr, I_MOVE, "LF@%ret", "nil@nil", NULL); // MOVE LF@%ret nil@nil
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "$ord$noerr", NULL, NULL); // LABEL $orr$noerr
    set_and_post_instr(&instr_list, curr_instr, I_PUSHS, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}

/** Generovanie vstavanej funkcie chr **/
void gen_chr() {
    set_and_post_instr(&instr_list, curr_instr, I_COMENT, "Definícia funkcie chr", NULL, NULL); // Komentár pred definíciou funkcie
    set_and_post_instr(&instr_list, curr_instr, I_LABEL, "?chr", NULL, NULL); // LABEL ?chr
    set_and_post_instr(&instr_list, curr_instr, I_CREATEFRAME, NULL, NULL, NULL); // CREATEFRAME
    set_and_post_instr(&instr_list, curr_instr, I_PUSHFRAME, NULL, NULL, NULL); // PUSHFRAME
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%p1", NULL, NULL); // DEFVAR LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_POPS, "LF@%p1", NULL, NULL); // POPS LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_DEFVAR, "LF@%ret", NULL, NULL); // DEFVAR LF@%ret
    set_and_post_instr(&instr_list, curr_instr, I_INT2CHAR, "LF@%ret", "LF@%p1", NULL); // INT2CHAR LF@%ret LF@%p1
    set_and_post_instr(&instr_list, curr_instr, I_PUSHS, "LF@%ret", NULL, NULL); // PUSHS LF@%ret
    //set_and_post_instr(&instr_list, curr_instr, I_POPFRAME, NULL, NULL, NULL); // POPFRAME
    //set_and_post_instr(&instr_list, curr_instr, I_RETURN, NULL, NULL, NULL); // RETURN
}
