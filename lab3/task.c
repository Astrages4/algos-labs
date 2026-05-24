#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_EXPR 256
#define MAX_STACK 100

// Типы узлов в дереве
typedef enum {
    TYPE_NUM,
    TYPE_VAR,
    TYPE_BINOP
} NodeType;

// Структура узла дерева
typedef struct Node {
    NodeType type;
    union {
        double num;           // для TYPE_NUM
        char var;            // для TYPE_VAR
        struct {
            char op;         // операторы: '+', '-', '*', '/', '^'
            struct Node* left;
            struct Node* right;
        } binop;
    } data;
} Node;

typedef struct {
    Node* items[MAX_STACK];
    int top;
} Stack;

// Прототипы функций
Node* create_num_node(double val);
Node* create_var_node(char var);
Node* create_binop_node(char op, Node* left, Node* right);
void destroy_tree(Node* root);
void print_tree(Node* root, int level);
void print_expr(Node* root);
Node* parse_expression(const char* expr);
int get_precedence(char op);
int is_operator(char c);
Node* apply_operator(char op, Node* a, Node* b);
Node* transform_power(Node* root);
int is_power_with_sum(Node* node);
Node* create_power_product(Node* base, Node* sum_node);
int count_sum_terms(Node* node);
Node* copy_tree(Node* root);
char* expr_to_string(Node* root);
void expr_to_string_rec(Node* root, char* buffer, int* pos, int parent_prec);
void print_separator(int n);

// Стек операций
void stack_init(Stack* s) { s->top = -1; }
void stack_push(Stack* s, Node* node) { s->items[++s->top] = node; }
Node* stack_pop(Stack* s) { return s->items[s->top--]; }
Node* stack_peek(Stack* s) { return s->items[s->top]; }
int stack_empty(Stack* s) { return s->top == -1; }

// Создание узлов
Node* create_num_node(double val) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = TYPE_NUM;
    node->data.num = val;
    return node;
}

Node* create_var_node(char var) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = TYPE_VAR;
    node->data.var = var;
    return node;
}

Node* create_binop_node(char op, Node* left, Node* right) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = TYPE_BINOP;
    node->data.binop.op = op;
    node->data.binop.left = left;
    node->data.binop.right = right;
    return node;
}

// Копирование дерева
Node* copy_tree(Node* root) {
    if (!root) return NULL;
    if (root->type == TYPE_NUM) {
        return create_num_node(root->data.num);
    }
    if (root->type == TYPE_VAR) {
        return create_var_node(root->data.var);
    }
    return create_binop_node(root->data.binop.op,
                            copy_tree(root->data.binop.left),
                            copy_tree(root->data.binop.right));
}

// Удаление дерева
void destroy_tree(Node* root) {
    if (!root) return;
    if (root->type == TYPE_BINOP) {
        destroy_tree(root->data.binop.left);
        destroy_tree(root->data.binop.right);
    }
    free(root);
}

// Приоритет операторов
int get_precedence(char op) {
    switch(op) {
        case '+': case '-': return 1;
        case '*': case '/': return 2;
        case '^': return 3;
        default: return 0;
    }
}

int is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

// Применение оператора к операндам
Node* apply_operator(char op, Node* a, Node* b) {
    return create_binop_node(op, a, b);
}

// Парсинг выражения (алгоритм сортировочной станции)
Node* parse_expression(const char* expr) {
    Stack output, operators;
    stack_init(&output);
    stack_init(&operators);
    
    int i = 0;
    int len = strlen(expr);
    
    while (i < len) {
        char c = expr[i];
        
        if (isspace(c)) {
            i++;
            continue;
        }
        
        // Число
        if (isdigit(c) || (c == '.')) {
            char num_str[32];
            int j = 0;
            while (i < len && (isdigit(expr[i]) || expr[i] == '.')) {
                num_str[j++] = expr[i++];
            }
            num_str[j] = '\0';
            double val = atof(num_str);
            stack_push(&output, create_num_node(val));
            continue;
        }
        
        // Переменная (буква)
        if (isalpha(c)) {
            stack_push(&output, create_var_node(c));
            i++;
            continue;
        }
        
        // Оператор
        if (is_operator(c)) {
            int prec = get_precedence(c);
            while (!stack_empty(&operators)) {
                Node* top_node = stack_peek(&operators);
                char top_op = top_node->data.binop.op;
                if (is_operator(top_op) && 
                    ((c != '^' && prec <= get_precedence(top_op)) ||
                     (c == '^' && prec < get_precedence(top_op)))) {
                    Node* op_node = stack_pop(&operators);
                    Node* b = stack_pop(&output);
                    Node* a = stack_pop(&output);
                    stack_push(&output, apply_operator(op_node->data.binop.op, a, b));
                    free(op_node);
                } else {
                    break;
                }
            }
            stack_push(&operators, create_binop_node(c, NULL, NULL));
            i++;
            continue;
        }
        
        // Открывающая скобка
        if (c == '(') {
            stack_push(&operators, create_binop_node('(', NULL, NULL));
            i++;
            continue;
        }
        
        // Закрывающая скобка
        if (c == ')') {
            while (!stack_empty(&operators) && stack_peek(&operators)->data.binop.op != '(') {
                Node* op_node = stack_pop(&operators);
                Node* b = stack_pop(&output);
                Node* a = stack_pop(&output);
                stack_push(&output, apply_operator(op_node->data.binop.op, a, b));
                free(op_node);
            }
            // Удаляем '('
            Node* paren = stack_pop(&operators);
            free(paren);
            i++;
            continue;
        }
        
        i++;
    }
    
    // Выгружаем оставшиеся операторы
    while (!stack_empty(&operators)) {
        Node* op_node = stack_pop(&operators);
        Node* b = stack_pop(&output);
        Node* a = stack_pop(&output);
        stack_push(&output, apply_operator(op_node->data.binop.op, a, b));
        free(op_node);
    }
    
    return stack_pop(&output);
}

// Проверка, является ли узел степенью с суммой в показателе
int is_power_with_sum(Node* node) {
    if (!node || node->type != TYPE_BINOP) return 0;
    if (node->data.binop.op != '^') return 0;
    
    Node* exponent = node->data.binop.right;
    if (!exponent || exponent->type != TYPE_BINOP) return 0;
    if (exponent->data.binop.op != '+') return 0;
    
    return 1;
}

// Подсчёт количества слагаемых в сумме
int count_sum_terms(Node* node) {
    if (!node) return 0;
    if (node->type != TYPE_BINOP || node->data.binop.op != '+') return 1;
    return count_sum_terms(node->data.binop.left) + 
           count_sum_terms(node->data.binop.right);
}

// Преобразование a^(b+c) -> a^b * a^c
Node* create_power_product(Node* base, Node* sum_node) {
    if (sum_node->type != TYPE_BINOP || sum_node->data.binop.op != '+') {
        // Одиночное слагаемое
        return create_binop_node('^', copy_tree(base), copy_tree(sum_node));
    }
    
    // Левый операнд суммы
    Node* left_term = sum_node->data.binop.left;
    // Правый операнд суммы
    Node* right_term = sum_node->data.binop.right;
    
    Node* left_power = create_binop_node('^', copy_tree(base), copy_tree(left_term));
    Node* right_power = create_binop_node('^', copy_tree(base), copy_tree(right_term));
    
    return create_binop_node('*', left_power, right_power);
}

// Основная функция преобразования
Node* transform_power(Node* root) {
    if (!root) return NULL;
    
    // Сначала рекурсивно преобразуем поддеревья
    if (root->type == TYPE_BINOP) {
        root->data.binop.left = transform_power(root->data.binop.left);
        root->data.binop.right = transform_power(root->data.binop.right);
        
        // Проверяем текущий узел
        if (is_power_with_sum(root)) {
            Node* base = root->data.binop.left;
            Node* exponent = root->data.binop.right;
            
            Node* result = create_power_product(base, exponent);
            
            // Заменяем текущий узел на результат
            root->type = TYPE_BINOP;
            root->data.binop.op = result->data.binop.op;
            root->data.binop.left = result->data.binop.left;
            root->data.binop.right = result->data.binop.right;
            
            // Освобождаем временный узел
            free(result);
        }
    }
    
    return root;
}

// Печать дерева (отступы)
void print_tree(Node* root, int level) {
    if (!root) return;
    
    for (int i = 0; i < level; i++) printf("  ");
    
    if (root->type == TYPE_NUM) {
        printf("NUM: %.2f\n", root->data.num);
    } else if (root->type == TYPE_VAR) {
        printf("VAR: %c\n", root->data.var);
    } else {
        printf("OP: %c\n", root->data.binop.op);
        print_tree(root->data.binop.left, level + 1);
        print_tree(root->data.binop.right, level + 1);
    }
}

// Преобразование дерева в строку (инфиксная запись)
void expr_to_string_rec(Node* root, char* buffer, int* pos, int parent_prec) {
    if (!root) return;
    
    int prec = 0;
    if (root->type == TYPE_BINOP) {
        switch(root->data.binop.op) {
            case '+': case '-': prec = 1; break;
            case '*': case '/': prec = 2; break;
            case '^': prec = 3; break;
        }
    }
    
    int need_paren = (parent_prec > prec) || 
                     (parent_prec == prec && root->type == TYPE_BINOP && root->data.binop.op == '^');
    
    if (need_paren && root->type == TYPE_BINOP) {
        buffer[(*pos)++] = '(';
    }
    
    if (root->type == TYPE_NUM) {
        // Убираем .00 если число целое
        if (root->data.num == (int)root->data.num) {
            *pos += sprintf(buffer + *pos, "%d", (int)root->data.num);
        } else {
            *pos += sprintf(buffer + *pos, "%.2f", root->data.num);
        }
    } else if (root->type == TYPE_VAR) {
        buffer[(*pos)++] = root->data.var;
    } else {
        expr_to_string_rec(root->data.binop.left, buffer, pos, prec);
        buffer[(*pos)++] = ' ';
        buffer[(*pos)++] = root->data.binop.op;
        buffer[(*pos)++] = ' ';
        int right_prec = prec;
        if (root->data.binop.op == '^') right_prec = prec + 1;
        expr_to_string_rec(root->data.binop.right, buffer, pos, right_prec);
    }
    
    if (need_paren && root->type == TYPE_BINOP) {
        buffer[(*pos)++] = ')';
    }
}

char* expr_to_string(Node* root) {
    char* buffer = (char*)malloc(MAX_EXPR);
    int pos = 0;
    expr_to_string_rec(root, buffer, &pos, 0);
    buffer[pos] = '\0';
    return buffer;
}

// Печать разделителя
void print_separator(int n) {
    for (int i = 0; i < n; i++) printf("=");
    printf("\n");
}

// Тестовые примеры
void test_expression(const char* expr_str) {
    printf("\n");
    print_separator(50);
    printf("Исходное выражение: %s\n", expr_str);
    
    Node* tree = parse_expression(expr_str);
    if (!tree) {
        printf("Ошибка парсинга!\n");
        return;
    }
    
    printf("\nДерево исходного выражения:\n");
    print_tree(tree, 0);
    
    char* expr_str_out = expr_to_string(tree);
    printf("\nТекст исходного выражения: %s\n", expr_str_out);
    free(expr_str_out);
    
    // Преобразование
    Node* transformed = transform_power(tree);
    
    printf("\nДерево после преобразования:\n");
    print_tree(transformed, 0);
    
    expr_str_out = expr_to_string(transformed);
    printf("\nТекст после преобразования: %s\n", expr_str_out);
    free(expr_str_out);
    
    destroy_tree(transformed);
}

int main() {
    printf("Лабораторная работа №3\n");
    printf("Вариант 25: a^(b+c) -> a^b * a^c\n");
    print_separator(50);
    
    // Тест 1: простое преобразование a^(b+c)
    test_expression("a^(b+c)");
    
    // Тест 2: a^(b+c+d) - три слагаемых
    test_expression("a^(b+c+d)");
    
    // Тест 3: выражение без преобразования
    test_expression("x^y");
    
    // Тест 4: вложенное преобразование (a^(b+c))^(d+e)
    test_expression("(a^(b+c))^(d+e)");
    
    // Тест 5: сложное выражение с числами
    test_expression("2^(x+3)");
    
    // Тест 6: несколько преобразований в разных местах
    test_expression("a^(b+c)*d^(e+f)");
    
    // Тест 7: выражение без показателя степени
    test_expression("a+b*c");
    
    // Тест 8: степень с одним слагаемым (не должно преобразовываться)
    test_expression("a^(b)");
    
    printf("\n");
    print_separator(50);
    printf("Программа завершена.\n");
    
    return 0;
}
