#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_KEY_LEN 6
#define ORDER 2       // 2 - минимальная степень
#define MAX_KEYS (2 * ORDER - 1)   // 3 ключа
#define MAX_CHILDREN (2 * ORDER)   // 4 ребенка

typedef struct BTreeNode {
    int num_keys;                           // текущее количество ключей
    char keys[MAX_KEYS][MAX_KEY_LEN + 1];   // массив ключей
    double values[MAX_KEYS];                // соответствующие значения
    struct BTreeNode *children[MAX_CHILDREN];
    int is_leaf;                            // 1 - лист, 0 - внутренний
} BTreeNode;

// Создание нового узла
BTreeNode* create_node(int is_leaf) {
    BTreeNode *node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Разбиение полного дочернего узла
void split_child(BTreeNode *parent, int child_index) {
    BTreeNode *child = parent->children[child_index];
    BTreeNode *new_node = create_node(child->is_leaf);
    new_node->num_keys = ORDER - 1;  // t-1 ключей

    // Копируем последние (t-1) ключей в новый узел
    for (int i = 0; i < ORDER - 1; i++) {
        strcpy(new_node->keys[i], child->keys[i + ORDER]);
        new_node->values[i] = child->values[i + ORDER];
    }

    // Если не лист, копируем детей
    if (!child->is_leaf) {
        for (int i = 0; i < ORDER; i++) {
            new_node->children[i] = child->children[i + ORDER];
        }
    }

    child->num_keys = ORDER - 1;  // t-1 ключей

    for (int i = parent->num_keys; i >= child_index + 1; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[child_index + 1] = new_node;

    for (int i = parent->num_keys - 1; i >= child_index; i--) {
        strcpy(parent->keys[i + 1], parent->keys[i]);
        parent->values[i + 1] = parent->values[i];
    }
    strcpy(parent->keys[child_index], child->keys[ORDER - 1]);
    parent->values[child_index] = child->values[ORDER - 1];
    parent->num_keys++;
}

// Вставка в неполный узел
void insert_non_full(BTreeNode *node, const char *key, double value) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            strcpy(node->keys[i + 1], node->keys[i]);
            node->values[i + 1] = node->values[i];
            i--;
        }
        strcpy(node->keys[i + 1], key);
        node->values[i + 1] = value;
        node->num_keys++;
    } else {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            i--;
        }
        i++;

        if (node->children[i]->num_keys == MAX_KEYS) {
            split_child(node, i);
            if (strcmp(key, node->keys[i]) > 0) {
                i++;
            }
        }
        insert_non_full(node->children[i], key, value);
    }
}

// 1. Добавление узла
BTreeNode* insert(BTreeNode *root, const char *key, double value) {
    if (!root) {
        root = create_node(1);
        strcpy(root->keys[0], key);
        root->values[0] = value;
        root->num_keys = 1;
        return root;
    }

    if (root->num_keys == MAX_KEYS) {
        BTreeNode *new_root = create_node(0);
        new_root->children[0] = root;
        split_child(new_root, 0);
        insert_non_full(new_root, key, value);
        return new_root;
    } else {
        insert_non_full(root, key, value);
        return root;
    }
}

// Поиск значения по ключу
double* search(BTreeNode *root, const char *key) {
    if (!root) return NULL;

    int i = 0;
    while (i < root->num_keys && strcmp(key, root->keys[i]) > 0) {
        i++;
    }

    if (i < root->num_keys && strcmp(key, root->keys[i]) == 0) {
        return &root->values[i];
    }

    if (root->is_leaf) {
        return NULL;
    }

    return search(root->children[i], key);
}

// функция для поиска предшественника
int find_predecessor(BTreeNode *node, char *key) {
    int i = node->num_keys - 1;
    while (i >= 0 && strcmp(key, node->keys[i]) < 0) i--;
    return i;
}

// Получение минимального ключа в поддереве
char* get_min_key(BTreeNode *node) {
    BTreeNode *curr = node;
    while (!curr->is_leaf) {
        curr = curr->children[0];
    }
    return curr->keys[0];
}

// Слияние узлов
void merge(BTreeNode *parent, int idx) {
    BTreeNode *left = parent->children[idx];
    BTreeNode *right = parent->children[idx + 1];

    // Забираем ключ из родителя
    strcpy(left->keys[left->num_keys], parent->keys[idx]);
    left->values[left->num_keys] = parent->values[idx];
    left->num_keys++;

    // Копируем ключи из правого узла
    for (int i = 0; i < right->num_keys; i++) {
        strcpy(left->keys[left->num_keys + i], right->keys[i]);
        left->values[left->num_keys + i] = right->values[i];
    }
    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++) {
            left->children[left->num_keys + i] = right->children[i];
        }
    }
    left->num_keys += right->num_keys;

    // Сдвигаем ключи и детей родителя
    for (int i = idx; i < parent->num_keys - 1; i++) {
        strcpy(parent->keys[i], parent->keys[i + 1]);
        parent->values[i] = parent->values[i + 1];
        parent->children[i + 1] = parent->children[i + 2];
    }
    parent->num_keys--;

    free(right);
}

// Заимствование у левого брата
void borrow_from_left(BTreeNode *parent, int idx) {
    BTreeNode *child = parent->children[idx];
    BTreeNode *left = parent->children[idx - 1];

    // Сдвигаем ключи ребенка
    for (int i = child->num_keys; i > 0; i--) {
        strcpy(child->keys[i], child->keys[i - 1]);
        child->values[i] = child->values[i - 1];
    }
    if (!child->is_leaf) {
        for (int i = child->num_keys + 1; i > 0; i--) {
            child->children[i] = child->children[i - 1];
        }
        child->children[0] = left->children[left->num_keys];
    }

    // Спускаем ключ из родителя в ребенка
    strcpy(child->keys[0], parent->keys[idx - 1]);
    child->values[0] = parent->values[idx - 1];
    child->num_keys++;

    // Поднимаем последний ключ левого брата в родителя
    strcpy(parent->keys[idx - 1], left->keys[left->num_keys - 1]);
    parent->values[idx - 1] = left->values[left->num_keys - 1];
    left->num_keys--;
}

// Заимствование у правого брата
void borrow_from_right(BTreeNode *parent, int idx) {
    BTreeNode *child = parent->children[idx];
    BTreeNode *right = parent->children[idx + 1];

    // Копируем ключ из родителя в конец ребенка
    strcpy(child->keys[child->num_keys], parent->keys[idx]);
    child->values[child->num_keys] = parent->values[idx];
    if (!child->is_leaf) {
        child->children[child->num_keys + 1] = right->children[0];
    }
    child->num_keys++;

    // Поднимаем первый ключ правого брата в родителя
    strcpy(parent->keys[idx], right->keys[0]);
    parent->values[idx] = right->values[0];

    // Сдвигаем ключи правого брата
    for (int i = 0; i < right->num_keys - 1; i++) {
        strcpy(right->keys[i], right->keys[i + 1]);
        right->values[i] = right->values[i + 1];
    }
    if (!right->is_leaf) {
        for (int i = 0; i <= right->num_keys - 1; i++) {
            right->children[i] = right->children[i + 1];
        }
    }
    right->num_keys--;
}

// Удаление из узла
int delete_from_node(BTreeNode *node, const char *key);

// Удаление из внутреннего узла
void delete_from_internal(BTreeNode *node, int idx) {
    BTreeNode *left_child = node->children[idx];
    BTreeNode *right_child = node->children[idx + 1];

    if (left_child->num_keys >= ORDER) {
        // Удаляем предшественника
        char *pred_key = get_min_key(left_child);
        double pred_val = left_child->values[0];
        delete_from_node(left_child, pred_key);
        strcpy(node->keys[idx], pred_key);
        node->values[idx] = pred_val;
    } else if (right_child->num_keys >= ORDER) {
        // Удаляем последователя (минимальный ключ правого поддерева)
        char succ_key[MAX_KEY_LEN + 1];
        double succ_val;
        BTreeNode *curr = right_child;
        while (!curr->is_leaf) curr = curr->children[0];
        strcpy(succ_key, curr->keys[0]);
        succ_val = curr->values[0];
        delete_from_node(right_child, succ_key);
        strcpy(node->keys[idx], succ_key);
        node->values[idx] = succ_val;
    } else {
        // Объединяем с правым ребенком
        merge(node, idx);
        delete_from_node(left_child, node->keys[idx]);
    }
}

// Основная функция удаления
int delete_from_node(BTreeNode *node, const char *key) {
    if (!node) return 0;

    int i = 0;
    while (i < node->num_keys && strcmp(key, node->keys[i]) > 0) i++;

    if (i < node->num_keys && strcmp(key, node->keys[i]) == 0) {
        if (node->is_leaf) {
            for (int j = i; j < node->num_keys - 1; j++) {
                strcpy(node->keys[j], node->keys[j + 1]);
                node->values[j] = node->values[j + 1];
            }
            node->num_keys--;
            return 1;
        } else {
            delete_from_internal(node, i);
            return 1;
        }
    } else {
        if (node->is_leaf) return 0;
        if (node->children[i]->num_keys < ORDER) {
            if (i > 0 && node->children[i - 1]->num_keys >= ORDER) {
                borrow_from_left(node, i);
            } else if (i < node->num_keys && node->children[i + 1]->num_keys >= ORDER) {
                borrow_from_right(node, i);
            } else {
                if (i < node->num_keys) {
                    merge(node, i);
                } else {
                    merge(node, i - 1);
                    i--;
                }
            }
        }
        return delete_from_node(node->children[i], key);
    }
}

// 2. Удаление узла (обертка)
BTreeNode* delete_key(BTreeNode *root, const char *key) {
    if (!root) return NULL;
    delete_from_node(root, key);
    if (root->num_keys == 0 && !root->is_leaf) {
        BTreeNode *new_root = root->children[0];
        free(root);
        return new_root;
    }
    return root;
}

// 3. Печать дерева (обход с отступами)
void print_tree(BTreeNode *node, int level) {
    if (!node) return;

    printf("  Уровень %d: [", level);
    for (int i = 0; i < node->num_keys; i++) {
        printf("(%s, %.2f)", node->keys[i], node->values[i]);
        if (i < node->num_keys - 1) printf(", ");
    }
    printf("]\n");

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            print_tree(node->children[i], level + 1);
        }
    }
}

// Освобождение памяти
void free_tree(BTreeNode *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            free_tree(node->children[i]);
        }
    }
    free(node);
}

int is_valid_key(const char *key) {
    int len = strlen(key);
    if (len == 0 || len > MAX_KEY_LEN) return 0;
    for (int i = 0; i < len; i++) {
        if (!isalpha(key[i])) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    FILE *input;
    
    if (argc != 2) {
        printf("Файл не указан. Используем commands.txt по умолчанию\n");
        input = fopen("commands.txt", "r");
        if (!input) {
            fprintf(stderr, "Ошибка: файл commands.txt не найден!\n");
            return 1;
        }
    } else {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("Cannot open input file");
            return 1;
        }
    }

    FILE *output = fopen("output.txt", "w");
    if (!output) {
        perror("Cannot open output file");
        fclose(input);
        return 1;
    }

    BTreeNode *root = NULL;
    char line[256];
    int line_num = 0;

    fprintf(output, "=== B-ДЕРЕВО ПОРЯДКА 2 (t=2) ===\n");
    fprintf(output, "Максимум ключей в узле: %d\n\n", MAX_KEYS);

    while (fgets(line, sizeof(line), input)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        int op;
        char key[MAX_KEY_LEN + 1];
        double val;
        char result[1024] = "";

        if (sscanf(line, "%d", &op) != 1) {
            fprintf(output, "Ошибка в строке %d: неверный формат\n", line_num);
            continue;
        }

        switch (op) {
            case 1: // Добавление
                if (sscanf(line, "%*d %s %lf", key, &val) != 2) {
                    sprintf(result, "Ошибка: неверные параметры для добавления");
                } else if (!is_valid_key(key)) {
                    sprintf(result, "Ошибка: неверный ключ '%s' (только латиница, длина 1-6)", key);
                } else {
                    root = insert(root, key, val);
                    sprintf(result, "Добавлено: %s = %.2f", key, val);
                }
                break;

            case 2: // Удаление
                if (sscanf(line, "%*d %s", key) != 1) {
                    sprintf(result, "Ошибка: не указан ключ для удаления");
                } else {
                    double *found = search(root, key);
                    if (found) {
                        root = delete_key(root, key);
                        sprintf(result, "Удален ключ: %s", key);
                    } else {
                        sprintf(result, "Ошибка: ключ '%s' не найден", key);
                    }
                }
                break;

            case 3: // Печать
                if (root) {
                    printf("\n=== B-ДЕРЕВО ===\n");
                    print_tree(root, 0);
                    printf("================\n\n");
                    sprintf(result, "Дерево выведено на экран");
                } else {
                    sprintf(result, "Дерево пусто");
                }
                break;

            case 4: // Поиск
                if (sscanf(line, "%*d %s", key) != 1) {
                    sprintf(result, "Ошибка: не указан ключ для поиска");
                } else {
                    double *found = search(root, key);
                    if (found) {
                        sprintf(result, "Найдено: %s = %.2f", key, *found);
                    } else {
                        sprintf(result, "Не найдено: ключ '%s' отсутствует", key);
                    }
                }
                break;

            default:
                sprintf(result, "Ошибка: неизвестная операция %d", op);
                break;
        }

        fprintf(output, "%s\n", line);
        fprintf(output, "  -> %s\n\n", result);
    }

    fclose(input);
    fclose(output);
    free_tree(root);

    printf("Готово! Результаты записаны в output.txt\n");
    return 0;
}
