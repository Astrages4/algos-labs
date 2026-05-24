#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

Node* createNode(int value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = value;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

Node* addNode(Node* root, int value) {
    if (root == NULL) return createNode(value);
    if (value < root->data)
        root->left = addNode(root->left, value);
    else if (value > root->data)
        root->right = addNode(root->right, value);
    return root;
}

void visualize(Node* root, int depth) {
    if (root == NULL) return;
    for (int i = 0; i < depth; i++) printf("    ");
    printf("%d\n", root->data);
    visualize(root->left, depth + 1);
    visualize(root->right, depth + 1);
}

Node* minValueNode(Node* node) {
    Node* current = node;
    while (current && current->left) current = current->left;
    return current;
}

Node* deleteNode(Node* root, int value) {
    if (root == NULL) return NULL;
    if (value < root->data)
        root->left = deleteNode(root->left, value);
    else if (value > root->data)
        root->right = deleteNode(root->right, value);
    else {
        if (root->left == NULL) {
            Node* temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            Node* temp = root->left;
            free(root);
            return temp;
        }
        Node* temp = minValueNode(root->right);
        root->data = temp->data;
        root->right = deleteNode(root->right, temp->data);
    }
    return root;
}

int countLeaves(Node* root) {
    if (root == NULL) return 0;
    if (root->left == NULL && root->right == NULL) return 1;
    return countLeaves(root->left) + countLeaves(root->right);
}

void freeTree(Node* root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

void runTests() {
    printf("\n========== Тесты ==========\n\n");
    
    printf("ТЕСТ 1: Добавление и визуализация\n");
    printf("Ожидается:\n");
    printf("50\n    30\n        20\n        40\n    70\n        60\n        80\n\n");
    
    Node* testTree = NULL;
    testTree = addNode(testTree, 50);
    testTree = addNode(testTree, 30);
    testTree = addNode(testTree, 70);
    testTree = addNode(testTree, 20);
    testTree = addNode(testTree, 40);
    testTree = addNode(testTree, 60);
    testTree = addNode(testTree, 80);
    
    printf("Результат:\n");
    visualize(testTree, 0);
    printf("\n");
    freeTree(testTree);
    
    printf("ТЕСТ 2: Удаление узла (удаляем 30, ожидается 50 -> 70)\n");
    
    testTree = NULL;
    testTree = addNode(testTree, 50);
    testTree = addNode(testTree, 30);
    testTree = addNode(testTree, 70);
    
    printf("До удаления:\n");
    visualize(testTree, 0);
    
    testTree = deleteNode(testTree, 30);
    
    printf("После удаления 30:\n");
    visualize(testTree, 0);
    printf("\n");
    freeTree(testTree);
    
    printf("ТЕСТ 3: Подсчёт листьев (ожидается 3 листа)\n");
    
    testTree = NULL;
    testTree = addNode(testTree, 50);
    testTree = addNode(testTree, 30);
    testTree = addNode(testTree, 70);
    testTree = addNode(testTree, 20);
    testTree = addNode(testTree, 40);
    testTree = addNode(testTree, 35);
    
    printf("Дерево:\n");
    visualize(testTree, 0);
    
    int leaves = countLeaves(testTree);
    printf("Результат: листьев = %d (ожидается 3)\n", leaves);
    printf("Листья: 20, 35, 70\n\n");
    freeTree(testTree);
    
}

int main() {
    Node* root = NULL;
    int choice, value;
    
    do {
        printf("\n================================\n");
        printf("МЕНЮ:\n");
        printf("1. Добавление нового узла\n");
        printf("2. Текстовая визуализация дерева\n");
        printf("3. Удаление узла\n");
        printf("4. Определить число листьев (вар.25)\n");
        printf("5. Запустить тесты\n");
        printf("6. Выход\n");
        printf("Ваш выбор: ");
        scanf("%d", &choice);
        
        switch(choice) {
            case 1:
                printf("Введите значение для добавления: ");
                scanf("%d", &value);
                root = addNode(root, value);
                printf("Узел %d добавлен\n", value);
                break;
            case 2:
                if (root == NULL) {
                    printf("Дерево пусто\n");
                } else {
                    printf("\nВизуализация дерева:\n");
                    visualize(root, 0);
                }
                break;
            case 3:
                printf("Введите значение для удаления: ");
                scanf("%d", &value);
                root = deleteNode(root, value);
                printf("Узел %d удален (если существовал)\n", value);
                break;
            case 4:
                printf("Количество листьев в дереве: %d\n", countLeaves(root));
                break;
            case 5:
                runTests();
                break;
            case 6:
                printf("Выход из программы\n");
                break;
            default:
                printf("Неверный выбор\n");
        }
    } while (choice != 0);
    
    freeTree(root);
    return 0;
}
