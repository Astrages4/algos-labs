#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 17

typedef struct {
    int num;
    char letter;
    char data[50];
} Table;

// Сравнение комбинированного ключа
int less(Table *a, Table *b) {
    if (a->num != b->num) return a->num < b->num;
    return a->letter < b->letter;
}

// Линейный выбор с обменом
void sort(Table arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        int min = i;
        for (int j = i+1; j < n; j++)
            if (less(&arr[j], &arr[min])) min = j;
        if (min != i) {
            Table tmp = arr[i];
            arr[i] = arr[min];
            arr[min] = tmp;
        }
    }
}

// Двоичный поиск
int search(Table arr[], int n, int num, char letter) {
    int l = 0, r = n-1;
    while (l <= r) {
        int m = (l+r)/2;
        if (arr[m].num == num && arr[m].letter == letter) return m;
        if (arr[m].num < num || (arr[m].num == num && arr[m].letter < letter))
            l = m+1;
        else r = m-1;
    }
    return -1;
}

// Печать таблицы
void print(Table arr[], int n) {
    for (int i = 0; i < n; i++)
        printf("%2d: %3d '%c' -> %s\n", i+1, arr[i].num, arr[i].letter, arr[i].data);
    printf("\n");
}

// Заполнение таблицы
void fill(Table arr[], int n, int type) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        if (type == 0) { // упорядоченная
            arr[i].num = i+1;
            arr[i].letter = 'a' + i%26;
        } else if (type == 1) { // обратный порядок
            arr[i].num = n-i;
            arr[i].letter = 'z' - i%26;
        } else { // случайная
            arr[i].num = rand() % 100;
            arr[i].letter = 'a' + rand()%26;
        }
        sprintf(arr[i].data, "data_%d", i+1);
    }
}

int main() {
    Table original[SIZE], working[SIZE];
    char *names[] = {"упорядочена", "в обратном порядке", "не упорядочена"};
    
    for (int test = 0; test < 3; test++) {
        printf("\n=== Случай: таблица %s ===\n", names[test]);
        
        fill(original, SIZE, test);
        memcpy(working, original, sizeof(original));
        
        printf("До сортировки:\n");
        print(working, SIZE);
        
        sort(working, SIZE);
        
        printf("После сортировки:\n");
        print(working, SIZE);
        
        printf("Поиск 5 ключей:\n");
        for (int q = 0; q < 5; q++) {
            int key_num;
            char key_letter;
            printf("  Ключ %d (число символ): ", q+1);
            scanf("%d %c", &key_num, &key_letter);
            
            int pos = search(working, SIZE, key_num, key_letter);
            if (pos >= 0)
                printf("    Найден! Позиция %d: (%d,'%c') данные: %s\n", 
                       pos+1, working[pos].num, working[pos].letter, working[pos].data);
            else
                printf("    Не найден: (%d,'%c')\n", key_num, key_letter);
        }
    }
    return 0;
}
