#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#define MAX_LETTERS 26
#define MAX_WORDS   8
#define MAX_LEN     32

/*  Глобальные структуры  */

static char  letters[MAX_LETTERS];      /* список уникальных букв */
static int   n_letters;

static int   coeff[MAX_LETTERS];        /* вклад буквы в сумму  */
static int   digit[MAX_LETTERS];        /* назначенная цифра */
static int   used_digit[10];            /* какие цифры уже заняты */
static int   is_leading[MAX_LETTERS];   /* буква является первой в слове */

static char  addends[MAX_WORDS][MAX_LEN];
static int   n_addends;
static char  result[MAX_LEN];

/*  Парсинг входной строки  */

static int parse(const char* rebus)
{
    char buf[256];
    strncpy(buf, rebus, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Разделяем по '=' */
    char* eq = strchr(buf, '=');
    if (!eq) return 0;
    *eq = '\0';
    char* left = buf;
    char* right = eq + 1;

    /* Правая часть */
    while (*right == ' ') right++;
    int rlen = strlen(right);
    while (rlen > 0 && (right[rlen - 1] == ' ' || right[rlen - 1] == '\n')) rlen--;
    right[rlen] = '\0';
    strcpy(result, right);

    /* Левая часть: разбиваем по '+' */
    n_addends = 0;
    char* p = left;
    while (*p) {
        while (*p == ' ') p++;
        char* start = p;
        while (*p && *p != '+') p++;
        int len = p - start;
        while (len > 0 && start[len - 1] == ' ') len--;
        strncpy(addends[n_addends], start, len);
        addends[n_addends][len] = '\0';
        n_addends++;
        if (*p == '+') p++;
    }
    return 1;
}

/*  Сбор уникальных букв  */

static int find_letter(char ch)
{
    for (int i = 0; i < n_letters; i++)
        if (letters[i] == ch) return i;
    letters[n_letters] = ch;
    return n_letters++;
}

static void collect_letters(void)
{
    n_letters = 0;
    for (int i = 0; i < n_addends; i++)
        for (char* p = addends[i]; *p; p++)
            if (*p >= 'A' && *p <= 'Z')
                find_letter(*p);
    for (char* p = result; *p; p++)
        if (*p >= 'A' && *p <= 'Z')
            find_letter(*p);
}

/*  Подсчёт коэффициентов  */

static void compute_coeffs(void)
{
    for (int i = 0; i < n_letters; i++) coeff[i] = 0;

    for (int w = 0; w < n_addends; w++) {
        int len = strlen(addends[w]);
        int p10 = 1;
        for (int i = len - 1; i >= 0; i--) {
            int id = find_letter(addends[w][i]);
            coeff[id] += p10;
            p10 *= 10;
        }
    }
    int len = strlen(result);
    int p10 = 1;
    for (int i = len - 1; i >= 0; i--) {
        int id = find_letter(result[i]);
        coeff[id] -= p10;
        p10 *= 10;
    }
}

/*  Ведущие нули  */

static void mark_leading(void)
{
    for (int i = 0; i < n_letters; i++) is_leading[i] = 0;
    for (int w = 0; w < n_addends; w++) {
        int id = find_letter(addends[w][0]);
        is_leading[id] = 1;
    }
    int id = find_letter(result[0]);
    is_leading[id] = 1;
}

/*  Число из слова  */

static long long word_to_num(const char* w)
{
    long long v = 0;
    for (char* p = w; *p; p++) {
        int id = find_letter(*p);
        v = v * 10 + digit[id];
    }
    return v;
}

/*  Рекурсивный перебор  */

static int dfs(int k, long long partial);

static int try_assign(int k, long long partial, int i, int d)
{
    digit[i] = d;
    used_digit[d] = 1;
    long long np = partial + (long long)coeff[i] * d;
    int ok = dfs(k + 1, np);
    if (!ok) {
        used_digit[d] = 0;
        digit[i] = -1;
    }
    return ok;
}

static int dfs(int k, long long partial)
{
    if (k == n_letters) {
        return partial == 0;
    }
    int i = k;
    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;
        if (d == 0 && is_leading[i]) continue;
        if (try_assign(k, partial, i, d)) return 1;
    }
    return 0;
}

/* Печать и решение */

static void print_solution(void)
{
    for (int w = 0; w < n_addends; w++) {
        if (w) printf(" + ");
        printf("%lld", word_to_num(addends[w]));
    }
    printf(" = %lld\n", word_to_num(result));
}

int solve_rebus(const char* rebus)
{
    if (!parse(rebus)) return 0;
    collect_letters();
    compute_coeffs();
    mark_leading();
    for (int i = 0; i < n_letters; i++) digit[i] = -1;
    for (int d = 0; d < 10; d++) used_digit[d] = 0;

    if (dfs(0, 0)) {
        print_solution();
        return 1;
    }
    return 0;
}

int main(void)
{
    setlocale(LC_ALL, "rus");
    char line[256];
    printf("Введите ребус (например: SEND + MORE = MONEY):\n> ");
    fflush(stdout);

    if (!fgets(line, sizeof(line), stdin)) return 1;

    /* Убираем завершающий '\n' */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';

    if (len == 0) return 0;

    printf("Input:  %s\n", line);
    printf("Output: ");
    if (!solve_rebus(line)) printf("no solution");
    printf("\n");
    return 0;
}