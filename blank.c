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

/*  НОВОЕ: порядок перебора букв и суффиксные границы  */

static int       order[MAX_LETTERS];
static long long suffix_max[MAX_LETTERS + 1];

/*  Парсинг входной строки  */

static int parse(const char* rebus)
{
    char buf[256];
    strncpy(buf, rebus, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* eq = strchr(buf, '=');
    if (!eq) return 0;
    *eq = '\0';
    char* left = buf;
    char* right = eq + 1;

    while (*right == ' ') right++;
    int rlen = strlen(right);
    while (rlen > 0 && (right[rlen - 1] == ' ' || right[rlen - 1] == '\n')) rlen--;
    right[rlen] = '\0';
    strcpy(result, right);

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

/*  порядок букв и границы отсечения  */

static void compute_order(void)
{
    for (int i = 0; i < n_letters; i++) order[i] = i;

    /*  буквы с большим |coeff| перебираем первыми —
        отсечение срабатывает раньше  */
    for (int i = 1; i < n_letters; i++) {
        int key = order[i];
        int j = i - 1;
        while (j >= 0 && labs((long)coeff[order[j]]) < labs((long)coeff[key])) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }
}

static void compute_suffix_max(void)
{
    suffix_max[n_letters] = 0;
    for (int k = n_letters - 1; k >= 0; k--)
        suffix_max[k] = suffix_max[k + 1] + 9LL * labs((long)coeff[order[k]]);
}

/*  Число из слова  */

static long long word_to_num(const char* w)
{
    long long v = 0;
    for (const char* p = w; *p; p++) {
        int id = find_letter(*p);
        v = v * 10 + digit[id];
    }
    return v;
}

/*  ОПТИМИЗАЦИЯ: проверка арифметики столбика для младших разрядов,
    в которых все буквы уже назначены  */

static int check_low_columns(void)
{
    int rlen = strlen(result);
    int maxlen = rlen;
    for (int w = 0; w < n_addends; w++) {
        int l = strlen(addends[w]);
        if (l > maxlen) maxlen = l;
    }

    int carry = 0;
    for (int t = 0; t < maxlen; t++) {
        int complete = 1;
        int sum = carry;

        for (int w = 0; w < n_addends; w++) {
            int l = strlen(addends[w]);
            if (t < l) {
                int i = find_letter(addends[w][l - 1 - t]);
                if (digit[i] == -1) { complete = 0; break; }
                sum += digit[i];
            }
        }
        if (!complete) break;

        int res_digit = -1;
        if (t < rlen) {
            int ri = find_letter(result[rlen - 1 - t]);
            if (digit[ri] == -1) complete = 0;
            else res_digit = digit[ri];
        }
        if (!complete) break;

        if (res_digit == -1) {
            if (sum % 10 != 0) return 0;
        } else {
            if (sum % 10 != res_digit) return 0;
        }
        carry = sum / 10;
    }
    return 1;
}

/*  Рекурсивный перебор с отсечением по частичной сумме  */

static int dfs(int k, long long partial)
{
    if (k == n_letters) {
        return partial == 0;
    }

    /*  ОПТИМИЗАЦИЯ: если оставшиеся буквы физически не могут
        скомпенсировать partial до нуля — ветка мертва  */
    if (labs(partial) > suffix_max[k]) return 0;

    int i = order[k];
    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;
        if (d == 0 && is_leading[i]) continue;

        digit[i] = d;
        used_digit[d] = 1;
        if (check_low_columns() &&
            dfs(k + 1, partial + (long long)coeff[i] * d)) return 1;
        used_digit[d] = 0;
        digit[i] = -1;
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
    compute_order();        /* НОВОЕ */
    compute_suffix_max();   /* НОВОЕ */
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
