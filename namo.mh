/*
 * namo.h
 *
 * Public declarations for the Namo-specific subsystems (configuration,
 * status lamps, help UI, startup file slots, etc.).
 */

#ifndef NAMO_H_
#define NAMO_H_

#include <stdbool.h>
#include <stddef.h>

#ifndef NAMO_PATH_MAX
#define NAMO_PATH_MAX 4096
#endif

#define NAMO_LAMP_OFF   0
#define NAMO_LAMP_WARN  1
#define NAMO_LAMP_ERROR 2

#define NAMO_SLOT_VISIBLE 4
#define NAMO_SLOT_POOL    64

struct namo_config {
    bool hint_bar;
    bool no_function_slot;
    bool warning_lamp;
    bool nonr;
    bool soft_tab;
    int soft_tab_width;
    bool autocomplete;
    bool use_lsp;
    bool case_sensitive_default;
    char warning_format[16];
    char error_format[16];
    char help_key[16];
    char help_language[16];
    char message_prefix[64];
};

extern struct namo_config namo_cfg;
extern char file_reserve[NAMO_SLOT_POOL][NAMO_PATH_MAX];
extern int should_redraw_underbar;

void namo_init(void);
void namo_apply_config(void);
void namo_cleanup(void);

void namo_set_lamp(int state);
const char *namo_lamp_label(void);
void namo_notify_message(const char *text);
void namo_message_prefix(const char *input, char *output, size_t outsz);

int namo_text_rows(void);
int namo_text_cols(void);
int namo_hint_top_row(void);
int namo_hint_bottom_row(void);
void namo_request_underbar_redraw(void);

bool namo_help_is_active(void);
int namo_help_command(int f, int n);
int namo_help_handle_key(int key);
void namo_help_render(void);

void namo_queue_startup_file(const char *path);
int namo_open_startup_slot(void);
void namo_handle_closed_file(const char *path);

int reserve_jump_1(int f, int n);
int reserve_jump_2(int f, int n);
int reserve_jump_3(int f, int n);
int reserve_jump_4(int f, int n);
int reserve_jump_fallback_1(int f, int n);
int reserve_jump_fallback_2(int f, int n);
int reserve_jump_fallback_3(int f, int n);
int reserve_jump_fallback_4(int f, int n);
int reserve_jump_numeric_mode(int f, int n);

int check_paste_slot_active(void);
void paste_slot_handle_key(int key);

#endif /* NAMO_H_ */
