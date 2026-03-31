#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "namo.h"
#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define NAMO_HELP_MAX_TOPICS 256
#define NAMO_HELP_VIEW_MENU   0
#define NAMO_HELP_VIEW_TOPIC  1
#define NAMO_HELP_MARGIN      2

struct namo_config namo_cfg;
char file_reserve[NAMO_SLOT_POOL][NAMO_PATH_MAX];
int should_redraw_underbar;

static int lamp_state = NAMO_LAMP_OFF;

static char startup_queue[NAMO_SLOT_POOL][NAMO_PATH_MAX];
static int startup_queue_count;

struct help_topic {
    char title[128];
    char *body;
};

static struct help_topic *help_topics;
static size_t help_topic_count;
static size_t help_topic_cap;
static bool help_loaded;
static bool help_active;
static int help_view_mode;
static int help_selected;
static int help_menu_scroll;
static int help_topic_scroll;

static void config_defaults(void);
static void load_config(void);
static bool parse_bool(const char *value, bool fallback);
static char *trim_whitespace(char *s);
static void apply_config_option(const char *section, const char *key, const char *value);
static void load_config_file(const char *path);
static void normalize_path(const char *input, char *output, size_t outsz);
static void slot_remove_from_queue(int index);
static void slot_fill_from_queue(void);
static bool slot_contains(const char *path);
static int open_slot_index(int idx);
static int switch_to_file(const char *path);
static void free_help_topics(void);
static void ensure_help_capacity(size_t needed);
static void load_help_topics(void);
static int locate_help_file(char *out, size_t outsz);
static void render_menu(void);
static void render_topic_view(void);
static void exit_help_view(void);
static void clamp_menu_scroll(void);
static void clamp_topic_scroll(void);
static int key_to_slot_index(int key);

void namo_init(void)
{
    memset(&namo_cfg, 0, sizeof(namo_cfg));
    memset(file_reserve, 0, sizeof(file_reserve));
    memset(startup_queue, 0, sizeof(startup_queue));
    startup_queue_count = 0;
    should_redraw_underbar = 0;
    lamp_state = NAMO_LAMP_OFF;
    config_defaults();
    load_config();
    namo_apply_config();
}

void namo_cleanup(void)
{
    free_help_topics();
    help_loaded = false;
}

static void config_defaults(void)
{
    namo_cfg.hint_bar = true;
    namo_cfg.no_function_slot = false;
    namo_cfg.warning_lamp = true;
    namo_cfg.nonr = false;
    namo_cfg.soft_tab = true;
    namo_cfg.soft_tab_width = 4;
    namo_cfg.autocomplete = true;
    namo_cfg.use_lsp = false;
    namo_cfg.case_sensitive_default = false;
    mystrscpy(namo_cfg.warning_format, "--W", sizeof(namo_cfg.warning_format));
    mystrscpy(namo_cfg.error_format, "--E", sizeof(namo_cfg.error_format));
    mystrscpy(namo_cfg.help_key, "F1", sizeof(namo_cfg.help_key));
    mystrscpy(namo_cfg.help_language, "en", sizeof(namo_cfg.help_language));
    mystrscpy(namo_cfg.message_prefix, "Message: ", sizeof(namo_cfg.message_prefix));
}

static const char *config_dirs[] = {
    NULL, /* XDG_CONFIG_HOME */
    NULL, /* ~/.config */
    NULL, /* XDG_DATA_HOME */
    NULL, /* ~/.local/share */
    "configs/namo"
};

static void init_default_dirs(void)
{
    const char *home = getenv("HOME");
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    const char *xdg_data = getenv("XDG_DATA_HOME");
    static char user_config[PATH_MAX];
    static char fallback_config[PATH_MAX];
    static char user_data[PATH_MAX];

    if (xdg_config && *xdg_config) {
        snprintf(user_config, sizeof(user_config), "%s/namo", xdg_config);
        config_dirs[0] = user_config;
    }
    if (home && *home) {
        snprintf(fallback_config, sizeof(fallback_config), "%s/.config/namo", home);
        config_dirs[1] = fallback_config;
    }
    if (xdg_data && *xdg_data) {
        snprintf(user_data, sizeof(user_data), "%s/namo", xdg_data);
        config_dirs[2] = user_data;
    }
    if (home && *home) {
        static char fallback_data[PATH_MAX];
        snprintf(fallback_data, sizeof(fallback_data), "%s/.local/share/namo", home);
        config_dirs[3] = fallback_data;
    }
}

static void load_config(void)
{
    init_default_dirs();
    for (size_t i = 0; i < sizeof(config_dirs) / sizeof(config_dirs[0]); ++i) {
        if (!config_dirs[i])
            continue;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/config", config_dirs[i]);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            load_config_file(path);
            return;
        }
    }
    load_config_file("configs/namo/config");
}

static bool parse_bool(const char *value, bool fallback)
{
    if (value == NULL)
        return fallback;
    if (strcasecmp(value, "1") == 0 || strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0)
        return true;
    if (strcasecmp(value, "0") == 0 || strcasecmp(value, "false") == 0 ||
        strcasecmp(value, "no") == 0 || strcasecmp(value, "off") == 0)
        return false;
    return fallback;
}

static char *trim_whitespace(char *s)
{
    if (s == NULL)
        return NULL;
    while (isspace((unsigned char)*s))
        ++s;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return s;
}

static void apply_config_option(const char *section, const char *key, const char *value)
{
    if (!section || !key || !value)
        return;
    if (strcasecmp(section, "ui") == 0) {
        if (strcasecmp(key, "hint_bar") == 0)
            namo_cfg.hint_bar = parse_bool(value, namo_cfg.hint_bar);
        else if (strcasecmp(key, "warning_lamp") == 0)
            namo_cfg.warning_lamp = parse_bool(value, namo_cfg.warning_lamp);
        else if (strcasecmp(key, "warning_format") == 0)
            mystrscpy(namo_cfg.warning_format, trim_whitespace((char *)value), sizeof(namo_cfg.warning_format));
        else if (strcasecmp(key, "error_format") == 0)
            mystrscpy(namo_cfg.error_format, trim_whitespace((char *)value), sizeof(namo_cfg.error_format));
        else if (strcasecmp(key, "help_key") == 0)
            mystrscpy(namo_cfg.help_key, trim_whitespace((char *)value), sizeof(namo_cfg.help_key));
        else if (strcasecmp(key, "help_language") == 0)
            mystrscpy(namo_cfg.help_language, trim_whitespace((char *)value), sizeof(namo_cfg.help_language));
        else if (strcasecmp(key, "no_function_slot") == 0)
            namo_cfg.no_function_slot = parse_bool(value, namo_cfg.no_function_slot);
        else if (strcasecmp(key, "nonr") == 0)
            namo_cfg.nonr = parse_bool(value, namo_cfg.nonr);
        else if (strcasecmp(key, "message_prefix") == 0)
            mystrscpy(namo_cfg.message_prefix, trim_whitespace((char *)value), sizeof(namo_cfg.message_prefix));
    } else if (strcasecmp(section, "edit") == 0) {
        if (strcasecmp(key, "soft_tab") == 0)
            namo_cfg.soft_tab = parse_bool(value, namo_cfg.soft_tab);
        else if (strcasecmp(key, "soft_tab_width") == 0)
            namo_cfg.soft_tab_width = atoi(value);
        else if (strcasecmp(key, "autocomplete") == 0)
            namo_cfg.autocomplete = parse_bool(value, namo_cfg.autocomplete);
        else if (strcasecmp(key, "use_lsp") == 0)
            namo_cfg.use_lsp = parse_bool(value, namo_cfg.use_lsp);
    } else if (strcasecmp(section, "search") == 0) {
        if (strcasecmp(key, "case_sensitive_default") == 0)
            namo_cfg.case_sensitive_default = parse_bool(value, namo_cfg.case_sensitive_default);
    }
}

static void load_config_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return;
    char line[1024];
    char section[64] = "";
    while (fgets(line, sizeof(line), fp)) {
        char *cursor = trim_whitespace(line);
        if (*cursor == '\0' || *cursor == '#' || *cursor == ';')
            continue;
        if (*cursor == '[') {
            char *end = strchr(cursor, ']');
            if (end) {
                *end = '\0';
                mystrscpy(section, cursor + 1, sizeof(section));
            }
            continue;
        }
        char *equals = strchr(cursor, '=');
        if (!equals)
            continue;
        *equals = '\0';
        char *key = trim_whitespace(cursor);
        char *value = trim_whitespace(equals + 1);
        apply_config_option(section, key, value);
    }
    fclose(fp);
}

void namo_apply_config(void)
{
    if (namo_cfg.soft_tab_width <= 0)
        namo_cfg.soft_tab_width = 4;
    tab_width = namo_cfg.soft_tab_width;
}

static void normalize_path(const char *input, char *output, size_t outsz)
{
    if (!input || !*input) {
        if (outsz)
            output[0] = '\0';
        return;
    }
    char resolved[PATH_MAX];
    if (realpath(input, resolved)) {
        mystrscpy(output, resolved, (int)outsz);
        return;
    }
    mystrscpy(output, input, (int)outsz);
}

void namo_queue_startup_file(const char *path)
{
    char normalized[NAMO_PATH_MAX];
    normalize_path(path, normalized, sizeof(normalized));
    if (!normalized[0])
        return;
    if (slot_contains(normalized))
        return;
    if (startup_queue_count >= NAMO_SLOT_POOL)
        return;
    mystrscpy(startup_queue[startup_queue_count++], normalized, NAMO_PATH_MAX);
    slot_fill_from_queue();
}

int namo_open_startup_slot(void)
{
    struct buffer *mainbp = bfind("main", FALSE, 0);
    int status = open_slot_index(0);
    if (status && mainbp)
        zotbuf(mainbp);
    return status;
}

void namo_handle_closed_file(const char *path)
{
    if (!path || !*path)
        return;
    for (int i = 0; i < NAMO_SLOT_VISIBLE; ++i) {
        if (file_reserve[i][0] && strcmp(file_reserve[i], path) == 0) {
            file_reserve[i][0] = '\0';
            slot_fill_from_queue();
            break;
        }
    }
}

static bool slot_contains(const char *path)
{
    if (!path || !*path)
        return false;
    for (int i = 0; i < NAMO_SLOT_VISIBLE; ++i) {
        if (file_reserve[i][0] && strcmp(file_reserve[i], path) == 0)
            return true;
    }
    for (int i = 0; i < startup_queue_count; ++i) {
        if (strcmp(startup_queue[i], path) == 0)
            return true;
    }
    return false;
}

static void slot_remove_from_queue(int index)
{
    if (index < 0 || index >= startup_queue_count)
        return;
    for (int i = index + 1; i < startup_queue_count; ++i)
        mystrscpy(startup_queue[i - 1], startup_queue[i], NAMO_PATH_MAX);
    if (startup_queue_count > 0)
        startup_queue[--startup_queue_count][0] = '\0';
}

static void slot_fill_from_queue(void)
{
    for (int i = 0; i < NAMO_SLOT_VISIBLE && startup_queue_count > 0; ++i) {
        if (file_reserve[i][0] == '\0') {
            mystrscpy(file_reserve[i], startup_queue[0], NAMO_PATH_MAX);
            slot_remove_from_queue(0);
        }
    }
}

static int switch_to_file(const char *path)
{
    if (!path || !*path)
        return FALSE;
    struct buffer *bp = bheadp;
    while (bp) {
        if (strcmp(bp->b_fname, path) == 0) {
            swbuffer(bp);
            return TRUE;
        }
        bp = bp->b_bufp;
    }
    char bname[NBUFN];
    makename(bname, (char *)path);
    unqname(bname);
    bp = bfind(bname, TRUE, 0);
    if (!bp)
        return FALSE;
    mystrscpy(bp->b_fname, path, NFILEN);
    bp->b_active = FALSE;
    swbuffer(bp);
    return TRUE;
}

static int open_slot_index(int idx)
{
    if (idx < 0 || idx >= NAMO_SLOT_VISIBLE) {
        TTbeep();
        return FALSE;
    }
    if (file_reserve[idx][0] == '\0') {
        mlwrite("(Slot %d empty)", idx + 1);
        TTbeep();
        return FALSE;
    }
    if (switch_to_file(file_reserve[idx]))
        return TRUE;
    mlwrite("(Unable to open %s)", file_reserve[idx]);
    return FALSE;
}

int reserve_jump_1(int f, int n)
{
    (void)f; (void)n;
    return open_slot_index(0);
}

int reserve_jump_2(int f, int n)
{
    (void)f; (void)n;
    return open_slot_index(1);
}

int reserve_jump_3(int f, int n)
{
    (void)f; (void)n;
    return open_slot_index(2);
}

int reserve_jump_4(int f, int n)
{
    (void)f; (void)n;
    return open_slot_index(3);
}

int reserve_jump_fallback_1(int f, int n)
{
    return reserve_jump_1(f, n);
}

int reserve_jump_fallback_2(int f, int n)
{
    return reserve_jump_2(f, n);
}

int reserve_jump_fallback_3(int f, int n)
{
    return reserve_jump_3(f, n);
}

int reserve_jump_fallback_4(int f, int n)
{
    return reserve_jump_4(f, n);
}

static int key_to_slot_index(int key)
{
    int base = key & ~(CONTROL | META | SHIFT | SPEC | CTLX | SUPER);
    if (base >= '1' && base <= '9')
        return base - '1';
    if (base == '0')
        return 9;
    return -1;
}

int reserve_jump_numeric_mode(int f, int n)
{
    (void)f; (void)n;
    int idx = key_to_slot_index(lastkey);
    if (idx < 0)
        return FALSE;
    if (idx >= NAMO_SLOT_VISIBLE)
        idx %= NAMO_SLOT_VISIBLE;
    return open_slot_index(idx);
}

void namo_set_lamp(int state)
{
    if (!namo_cfg.warning_lamp) {
        lamp_state = NAMO_LAMP_OFF;
        return;
    }
    if (state < NAMO_LAMP_OFF)
        state = NAMO_LAMP_OFF;
    if (state > NAMO_LAMP_ERROR)
        state = NAMO_LAMP_ERROR;
    lamp_state = state;
    should_redraw_underbar = 1;
}

const char *namo_lamp_label(void)
{
    if (!namo_cfg.warning_lamp)
        return "";
    if (lamp_state == NAMO_LAMP_WARN)
        return namo_cfg.warning_format;
    if (lamp_state == NAMO_LAMP_ERROR)
        return namo_cfg.error_format;
    return "";
}

void namo_notify_message(const char *text)
{
    (void)text;
    if (lamp_state != NAMO_LAMP_OFF)
        namo_set_lamp(NAMO_LAMP_OFF);
}

void namo_message_prefix(const char *input, char *output, size_t outsz)
{
    if (!output || outsz == 0)
        return;
    const char *prefix = namo_cfg.message_prefix;
    if (!prefix)
        prefix = "";
    if (!input)
        input = "";
    if (*prefix)
        snprintf(output, outsz, "%s%s", prefix, input);
    else
        mystrscpy(output, input, (int)outsz);
}

int namo_text_rows(void)
{
    if (!term)
        return 0;
    int rows = term->t_nrow;
    if (namo_cfg.hint_bar && rows >= 2)
        rows -= 2;
    return rows;
}

int namo_text_cols(void)
{
    if (!term)
        return 0;
    int cols = term->t_ncol;
    if (!namo_cfg.nonr && cols > 6)
        cols -= 6;
    return cols;
}

int namo_hint_top_row(void)
{
    if (!term || !namo_cfg.hint_bar)
        return -1;
    return term->t_nrow - 2;
}

int namo_hint_bottom_row(void)
{
    if (!term || !namo_cfg.hint_bar)
        return -1;
    return term->t_nrow - 1;
}

void namo_request_underbar_redraw(void)
{
    should_redraw_underbar = 1;
}

bool namo_help_is_active(void)
{
    return help_active;
}

static void free_help_topics(void)
{
    if (!help_topics)
        return;
    for (size_t i = 0; i < help_topic_count; ++i)
        free(help_topics[i].body);
    free(help_topics);
    help_topics = NULL;
    help_topic_count = 0;
    help_topic_cap = 0;
}

static void ensure_help_capacity(size_t needed)
{
    if (needed <= help_topic_cap)
        return;
    size_t newcap = help_topic_cap ? help_topic_cap * 2 : 16;
    if (newcap < needed)
        newcap = needed;
    if (newcap > NAMO_HELP_MAX_TOPICS)
        newcap = NAMO_HELP_MAX_TOPICS;
    struct help_topic *tmp = realloc(help_topics, newcap * sizeof(*help_topics));
    if (!tmp)
        return;
    help_topics = tmp;
    help_topic_cap = newcap;
}

struct help_builder {
    char *data;
    size_t len;
    size_t cap;
};

static void builder_reset(struct help_builder *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void builder_append(struct help_builder *b, const char *line)
{
    if (!line)
        return;
    size_t add = strlen(line);
    if (add == 0)
        return;
    if (b->len + add + 1 >= b->cap) {
        size_t newcap = (b->cap ? b->cap * 2 : 256);
        while (newcap < b->len + add + 1)
            newcap *= 2;
        char *tmp = realloc(b->data, newcap);
        if (!tmp)
            return;
        b->data = tmp;
        b->cap = newcap;
    }
    memcpy(b->data + b->len, line, add);
    b->len += add;
    b->data[b->len] = '\0';
}

static void add_help_topic(const char *title, struct help_builder *body)
{
    if (!title || !*title)
        return;
    ensure_help_capacity(help_topic_count + 1);
    if (help_topic_count >= help_topic_cap)
        return;
    struct help_topic *topic = &help_topics[help_topic_count++];
    mystrscpy(topic->title, title, sizeof(topic->title));
    if (body->len == 0) {
        topic->body = strdup("(No content)");
    } else {
        topic->body = malloc(body->len + 1);
        if (topic->body)
            memcpy(topic->body, body->data, body->len + 1);
    }
}

static int locate_help_file(char *out, size_t outsz)
{
    init_default_dirs();
    char localized[64];
    snprintf(localized, sizeof(localized), "emacs-%s.hlp", namo_cfg.help_language);
    for (size_t i = 0; i < sizeof(config_dirs) / sizeof(config_dirs[0]); ++i) {
        if (!config_dirs[i])
            continue;
        char attempt[PATH_MAX];
        snprintf(attempt, sizeof(attempt), "%s/%s", config_dirs[i], localized);
        struct stat st;
        if (stat(attempt, &st) == 0 && S_ISREG(st.st_mode)) {
            mystrscpy(out, attempt, (int)outsz);
            return 1;
        }
    }
    for (size_t i = 0; i < sizeof(config_dirs) / sizeof(config_dirs[0]); ++i) {
        if (!config_dirs[i])
            continue;
        char attempt[PATH_MAX];
        snprintf(attempt, sizeof(attempt), "%s/emacs.hlp", config_dirs[i]);
        struct stat st;
        if (stat(attempt, &st) == 0 && S_ISREG(st.st_mode)) {
            mystrscpy(out, attempt, (int)outsz);
            return 1;
        }
    }
    mystrscpy(out, "emacs.hlp", (int)outsz);
    struct stat st;
    return stat(out, &st) == 0 && S_ISREG(st.st_mode);
}

static void load_help_topics(void)
{
    if (help_loaded)
        return;
    free_help_topics();
    char path[PATH_MAX];
    if (!locate_help_file(path, sizeof(path))) {
        help_loaded = true;
        return;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) {
        help_loaded = true;
        return;
    }
    struct help_builder builder = {0};
    char current_title[128] = "";
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "=>", 2) == 0) {
            if (*current_title)
                add_help_topic(current_title, &builder);
            builder_reset(&builder);
            char *title = trim_whitespace(line + 2);
            mystrscpy(current_title, title, sizeof(current_title));
            continue;
        }
        builder_append(&builder, line);
    }
    if (*current_title)
        add_help_topic(current_title, &builder);
    builder_reset(&builder);
    fclose(fp);
    help_loaded = true;
}

int namo_help_command(int f, int n)
{
    (void)f; (void)n;
    if (namo_help_is_active()) {
        exit_help_view();
        return TRUE;
    }
    load_help_topics();
    help_active = true;
    help_view_mode = NAMO_HELP_VIEW_MENU;
    help_selected = 0;
    help_menu_scroll = 0;
    help_topic_scroll = 0;
    namo_help_render();
    return TRUE;
}

static void exit_help_view(void)
{
    help_active = false;
    sgarbf = TRUE;
}

void namo_help_render(void)
{
    if (!help_active)
        return;
    if (!term)
        return;
    int rows = term->t_nrow;
    int cols = term->t_ncol;
    for (int r = 0; r < rows; ++r) {
        TTmove(r, 0);
        for (int c = 0; c < cols; ++c)
            TTputc(' ');
    }
    TTmove(0, 0);
    char header[128];
    snprintf(header, sizeof(header), "Namo Help (%zu topics)", help_topic_count);
    for (int i = 0; header[i] && i < cols; ++i)
        TTputc(header[i]);
    if (help_view_mode == NAMO_HELP_VIEW_MENU)
        render_menu();
    else
        render_topic_view();
    TTflush();
}

static void render_menu(void)
{
    if (!term)
        return;
    if (help_selected >= (int)help_topic_count)
        help_selected = (int)help_topic_count - 1;
    clamp_menu_scroll();
    int rows = term->t_nrow;
    int cols = term->t_ncol;
    int start_row = 2;
    int available = rows - start_row - 2;
    for (int i = 0; i < available; ++i) {
        int topic_index = help_menu_scroll + i;
        if (topic_index >= (int)help_topic_count)
            break;
        TTmove(start_row + i, NAMO_HELP_MARGIN);
        if (topic_index == help_selected)
            TTputc('>');
        else
            TTputc(' ');
        TTputc(' ');
        const char *title = help_topics[topic_index].title;
        for (int col = NAMO_HELP_MARGIN + 2; col < cols && *title; ++col)
            TTputc(*title++);
    }
    TTmove(rows - 2, 0);
    const char *footer = "Up/Down or k/j to move, Enter to open, Esc to exit";
    for (int i = 0; footer[i] && i < cols; ++i)
        TTputc(footer[i]);
}

static void render_topic_view(void)
{
    if (!term || help_selected < 0 || help_selected >= (int)help_topic_count)
        return;
    clamp_topic_scroll();
    const struct help_topic *topic = &help_topics[help_selected];
    int rows = term->t_nrow;
    int cols = term->t_ncol;
    TTmove(1, NAMO_HELP_MARGIN);
    const char *title = topic->title;
    for (int i = 0; title[i] && i + NAMO_HELP_MARGIN < cols; ++i)
        TTputc(title[i]);
    TTmove(rows - 2, 0);
    const char *footer = "Up/Down scroll, Backspace to menu, Esc to exit";
    for (int i = 0; footer[i] && i < cols; ++i)
        TTputc(footer[i]);
    int start_row = 3;
    int available = rows - start_row - 3;
    const char *body = topic->body ? topic->body : "";
    int line = 0;
    const char *cursor = body;
    while (*cursor && line < help_topic_scroll) {
        const char *next = strchr(cursor, '\n');
        cursor = next ? next + 1 : cursor + strlen(cursor);
        ++line;
    }
    for (int r = 0; r < available && *cursor; ++r) {
        const char *next = strchr(cursor, '\n');
        size_t len = next ? (size_t)(next - cursor) : strlen(cursor);
        TTmove(start_row + r, NAMO_HELP_MARGIN);
        for (size_t c = 0; c < len && (int)(NAMO_HELP_MARGIN + c) < cols; ++c)
            TTputc(cursor[c]);
        cursor = next ? next + 1 : cursor + len;
    }
}

static void clamp_menu_scroll(void)
{
    if (help_selected < 0)
        help_selected = 0;
    if (help_selected >= (int)help_topic_count)
        help_selected = (int)help_topic_count - 1;
    int rows = term ? term->t_nrow : 0;
    int available = rows - 4;
    if (available < 1)
        available = 1;
    if (help_selected < help_menu_scroll)
        help_menu_scroll = help_selected;
    if (help_selected >= help_menu_scroll + available)
        help_menu_scroll = help_selected - available + 1;
    if (help_menu_scroll < 0)
        help_menu_scroll = 0;
}

static void clamp_topic_scroll(void)
{
    if (!term)
        return;
    int rows = term->t_nrow;
    int available = rows - 6;
    if (available < 1)
        available = 1;
    int total_lines = 0;
    const char *body = help_topics[help_selected].body;
    for (const char *p = body; p && *p; ++p)
        if (*p == '\n')
            ++total_lines;
    if (help_topic_scroll < 0)
        help_topic_scroll = 0;
    if (help_topic_scroll > total_lines)
        help_topic_scroll = total_lines;
    (void)available;
}

int namo_help_handle_key(int key)
{
    if (!help_active)
        return FALSE;
    switch (key) {
    case CONTROL | '[':
    case 27:
        exit_help_view();
        return TRUE;
    case CONTROL | 'M':
    case '\r':
    case '\n':
        if (help_view_mode == NAMO_HELP_VIEW_MENU && help_topic_count > 0) {
            help_view_mode = NAMO_HELP_VIEW_TOPIC;
            help_topic_scroll = 0;
        }
        break;
    case 127:
    case CONTROL | 'H':
        help_view_mode = NAMO_HELP_VIEW_MENU;
        break;
    case 'k':
    case SPEC | 'A':
    case CONTROL | 'P':
        if (help_view_mode == NAMO_HELP_VIEW_MENU) {
            if (help_selected > 0)
                --help_selected;
            clamp_menu_scroll();
        } else if (help_topic_scroll > 0) {
            --help_topic_scroll;
        }
        break;
    case 'j':
    case SPEC | 'B':
    case CONTROL | 'N':
        if (help_view_mode == NAMO_HELP_VIEW_MENU) {
            if (help_selected + 1 < (int)help_topic_count)
                ++help_selected;
            clamp_menu_scroll();
        } else {
            ++help_topic_scroll;
        }
        break;
    case 'q':
    case 'Q':
        exit_help_view();
        return TRUE;
    default:
        break;
    }
    namo_help_render();
    return TRUE;
}
